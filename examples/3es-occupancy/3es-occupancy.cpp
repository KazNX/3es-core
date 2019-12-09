//
// author Kazys Stepanas
//
#include "3es-occupancy.h"

#include "3esvector3.h"

#include <3esservermacros.h>

#include "occupancyloader.h"
#include "occupancymesh.h"
#include "p2p.h"

#include <csignal>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <unordered_set>

// Forced bug ideas to show now 3es highlights the issue(s).
// 1. Skip inserting the sample voxel key assuming that the ray will do so.
// 2. call integrateMiss() instead of integrateHit().
// 3. no trajectory data.

using namespace tes;

TES_SERVER_DECL(g_tesServer);

namespace
{
bool quit = false;

void onSignal(int arg)
{
  if (arg == SIGINT || arg == SIGTERM)
  {
    quit = true;
  }
}


enum RayLevel
{
  Rays_Off,
  Rays_Lines = (1 << 0),
  Rays_Voxels = (1 << 1),
  Rays_All = Rays_Lines | Rays_Voxels
};


enum SampleLevel
{
  Samples_Off,
  Samples_Voxels = (1 << 0),
  Samples_Points = (1 << 1),
  Samples_All = Samples_Voxels | Samples_Points
};


struct Options
{
  std::string cloudFile;
  std::string trajectoryFile;
  std::string outStream;
  uint64_t pointLimit;
  double startTime;
  double endTime;
  float resolution;
  float probHit;
  float probMiss;
  unsigned batchSize;
  int rays;
  int samples;
  bool quiet;

  inline Options()
    : pointLimit(0)
    , startTime(0)
    , endTime(0)
    , resolution(0.1f)
    , probHit(0.7f)
    , probMiss(0.49f)
    , batchSize(1000)
    , rays(Rays_Lines)
    , samples(Samples_Voxels)
    , quiet(false)
  {}
};

typedef std::unordered_set<octomap::OcTreeKey, octomap::OcTreeKey::KeyHash> KeySet;


bool matchArg(const char *arg, const char *expect)
{
  return strncmp(arg, expect, strlen(expect)) == 0;
}

bool optionValue(const char *arg, int argc, char *argv[], std::string &value)
{
  TES_UNUSED(argc);
  TES_UNUSED(argv);
  if (*arg == '=')
  {
    ++arg;
  }
  value = arg;
  return true;
}

template <typename NUMERIC>
bool optionValue(const char *arg, int argc, char *argv[], NUMERIC &value)
{
  std::string strValue;
  if (optionValue(arg, argc, argv, strValue))
  {
    std::istringstream instr(strValue);
    instr >> value;
    return !instr.fail();
  }

  return false;
}


void shiftToSet(UnorderedKeySet &dst, UnorderedKeySet &src, const octomap::OcTreeKey &key)
{
  auto iter = src.find(key);
  if (iter != src.end())
  {
    src.erase(iter);
  }
  dst.insert(key);
}

#ifdef TES_ENABLE
void renderVoxels(const UnorderedKeySet &keys, const octomap::OcTree &map, const tes::Colour &colour, uint16_t category)
{
  // Convert to voxel centres.
  if (!keys.empty())
  {
    std::vector<Vector3f> centres(keys.size());
    size_t index = 0;
    for (auto key : keys)
    {
      centres[index++] = p2p(map.keyToCoord(key));
    }

    // Render slightly smaller than the actual voxel size.
    TES_VOXELS(g_tesServer, colour, 0.95f * float(map.getResolution()), centres.data()->v, unsigned(centres.size()),
               sizeof(*centres.data()), 0u, category);
  }
}
#endif  // TES_ENABLE
}  // namespace


int populateMap(const Options &opt)
{
  printf("Loading points from %s with trajectory %s \n", opt.cloudFile.c_str(), opt.trajectoryFile.c_str());

  OccupancyLoader loader;
  if (!loader.open(opt.cloudFile.c_str(), opt.trajectoryFile.c_str()))
  {
    fprintf(stderr, "Error loading cloud %s with trajectory %s \n", opt.cloudFile.c_str(), opt.trajectoryFile.c_str());
    return -2;
  }

  octomap::KeyRay rayKeys;
  octomap::OcTree map(opt.resolution);
  octomap::OcTreeKey key;
  tes::Vector3f origin, sample;
  tes::Vector3f voxel, ext(opt.resolution);
  double timestamp;
  uint64_t pointCount = 0;
  size_t keyIndex;
  // Update map visualisation every N samples.
  const size_t rayBatchSize = opt.batchSize;
  double timebase = -1;
  double firstBatchTimestamp = -1;
  double lastTimestamp = -1;
#ifdef TES_ENABLE
  char timeStrBuffer[256];
  // Keys of voxels touched in the current batch.
  UnorderedKeySet becomeOccupied;
  UnorderedKeySet becomeFree;
  UnorderedKeySet touchedFree;
  UnorderedKeySet touchedOccupied;
  std::vector<Vector3f> rays;
  std::vector<Vector3f> samples;
  OccupancyMesh mapMesh(RES_MapMesh, map);
#endif  // TES_ENABLE

  map.setProbHit(opt.probHit);
  map.setProbMiss(opt.probMiss);

  // Prevent ready saturation to free.
  map.setClampingThresMin(0.01);
  // printf("min: %g\n", map.getClampingThresMinLog());

  TES_POINTCLOUDSHAPE(g_tesServer, TES_COLOUR(SteelBlue), &mapMesh, RES_Map, CAT_Map);
  // Ensure mesh is created for later update.
  TES_SERVER_UPDATE(g_tesServer, 0.0f);

  // Load the first point.
  bool havePoint = loader.nextPoint(sample, origin, &timestamp);
  if (!havePoint)
  {
    printf("No data to load\n");
    return -1;
  }

  timebase = timestamp;

  if (opt.startTime > 0)
  {
    // Get to the start time.
    printf("Skipping to start time offset: %g\n", opt.startTime);
    while ((havePoint = loader.nextPoint(sample, origin, &timestamp)))
    {
      if (timestamp - timebase >= opt.startTime)
      {
        break;
      }
    }
  }

  printf("Populating map\n");
  while (havePoint)
  {
    ++pointCount;
    TES_IF(opt.rays & Rays_Lines)
    {
      TES_STMT(rays.push_back(origin));
      TES_STMT(rays.push_back(sample));
    }
    TES_IF(opt.samples & Samples_Points) { TES_STMT(samples.push_back(sample)); }

    if (firstBatchTimestamp < 0)
    {
      firstBatchTimestamp = timestamp;
    }
    // Compute free ray.
    map.computeRayKeys(p2p(origin), p2p(sample), rayKeys);
    // Draw intersected voxels.
    keyIndex = 0;
    for (auto key : rayKeys)
    {
      if (octomap::OcTree::NodeType *node = map.search(key))
      {
        // Existing node.
        const bool initiallyOccupied = map.isNodeOccupied(node);
        map.integrateMiss(node);
        if (initiallyOccupied && !map.isNodeOccupied(node))
        {
          // Node became free.
#ifdef TES_ENABLE
          shiftToSet(becomeFree, becomeOccupied, key);
#endif  // TES_ENABLE
        }
      }
      else
      {
        // New node.
        map.updateNode(key, false, true);
      }
      voxel = p2p(map.keyToCoord(key));
      // Collate for render.
      TES_STMT(touchedFree.insert(key));
      ++keyIndex;
    }

    // Update the sample node.
    key = map.coordToKey(p2p(sample));
    if (octomap::OcTree::NodeType *node = map.search(key))
    {
      // Existing node.
      const bool initiallyOccupied = map.isNodeOccupied(node);
      map.integrateHit(node);
      if (!initiallyOccupied && map.isNodeOccupied(node))
      {
        // Node became occupied.
        TES_STMT(shiftToSet(becomeOccupied, becomeFree, key));
      }
    }
    else
    {
      // New node.
      map.updateNode(key, true, true);
      // Collate for render.
      TES_STMT(shiftToSet(becomeOccupied, becomeFree, key));
    }
    TES_STMT(shiftToSet(touchedOccupied, touchedFree, key));

    if (pointCount % rayBatchSize == 0 || quit)
    {
      //// Collapse the map.
      // map.isNodeCollapsible()
#ifdef TES_ENABLE
      double elapsedTime = (lastTimestamp >= 0) ? timestamp - lastTimestamp : timestamp - firstBatchTimestamp;
      // Handle time jumps back.
      elapsedTime = std::max(elapsedTime, 0.0);
      // Cull large time differences.
      elapsedTime = std::min(elapsedTime, 1.0);
      firstBatchTimestamp = -1;

#ifdef _MSC_VER
      sprintf_s(timeStrBuffer, "%g", timestamp - timebase);
#else   // _MSC_VER
      sprintf(timeStrBuffer, "%g", timestamp - timebase);
#endif  // _MSC_VER
      TES_TEXT2D_SCREEN(g_tesServer, TES_COLOUR(White), timeStrBuffer, 0u, CAT_Info, Vector3f(0.05f, 0.1f, 0.0f));
      // Draw sample lines.
      if (opt.rays & Rays_Lines)
      {
        TES_LINES(g_tesServer, TES_COLOUR(DarkOrange), rays.data()->v, unsigned(rays.size()), sizeof(*rays.data()), 0u,
                  CAT_Rays);
      }
      rays.clear();
      // Render touched voxels in bulk.
      if (opt.rays & Rays_Voxels)
      {
        renderVoxels(touchedFree, map, tes::Colour::Colours[tes::Colour::MediumSpringGreen], CAT_FreeCells);
      }
      if (opt.samples & Samples_Voxels)
      {
        renderVoxels(touchedOccupied, map, tes::Colour::Colours[tes::Colour::Turquoise], CAT_OccupiedCells);
      }
      if (opt.samples)
      {
        TES_POINTS(g_tesServer, TES_COLOUR(Orange), samples.data()->v, unsigned(samples.size()),
                   sizeof(*samples.data()), 0u, CAT_OccupiedCells);
      }
      samples.clear();
      // TES_SERVER_UPDATE(g_tesServer, 0.0f);

      // Ensure touchedOccupied does not contain newly occupied nodes for mesh update.
      for (auto key : becomeOccupied)
      {
        auto search = touchedOccupied.find(key);
        if (search != touchedOccupied.end())
        {
          touchedOccupied.erase(search);
        }
      }

      // Render changes to the map.
      mapMesh.update(becomeOccupied, becomeFree, touchedOccupied);

      touchedFree.clear();
      touchedOccupied.clear();
      becomeOccupied.clear();
      becomeFree.clear();
      TES_SERVER_UPDATE(g_tesServer, float(elapsedTime));
      if (opt.pointLimit && pointCount >= opt.pointLimit ||
          opt.endTime > 0 && lastTimestamp - timebase >= opt.endTime || quit)
      {
        break;
      }
#endif  // TES_ENABLE

      lastTimestamp = timestamp;
      if (!opt.quiet)
      {
        printf("\r%g        ", lastTimestamp - timebase);
        // fflush(stdout);
      }
    }

    havePoint = loader.nextPoint(sample, origin, &timestamp);
  }

  TES_SERVER_UPDATE(g_tesServer, 0.0f);

  if (!opt.quiet)
  {
    printf("\n");
  }

  printf("Processed %" PRIu64 " points.\n", pointCount);

  // Save the occupancy map.
  printf("Saving map");
  map.writeBinary("map.bt");

  return 0;
}


void usage(const Options &opt)
{
  printf("Usage:\n");
  printf("3es-occupancy [options] <cloud.ply> <trajectory.ply>\n");
  printf("\nGenerates an Octomap occupancy map from a PLY based point cloud and accompanying trajectory file.\n\n");
  printf(
    "The trajectory marks the scanner trajectory with timestamps loosely corresponding to cloud point timestamps. ");
  printf("Trajectory points are interpolated for each cloud point based on corresponding times in the trajectory.\n\n");
  printf(
    "Third Eye Scene render commands are interspersed throughout the code to visualise the generation process\n\n");
  printf("Options:\n");
  printf("-b=<batch-size> (%u)\n", opt.batchSize);
  printf("  The number of points to process in each batch. Controls debug display.\n");
  printf("-h=<hit-probability> (%g)\n", opt.probHit);
  printf("  The occupancy probability due to a hit. Must be >= 0.5.\n");
  printf("-m=<miss-probability> (%g)\n", opt.probMiss);
  printf("  The occupancy probability due to a miss. Must be < 0.5.\n");
  printf("-o=<stream-file>\n");
  printf("  Specifies a file to write a 3es stream to directly without the need for an external client.\n");
  printf("-p=<point-limit> (0)\n");
  printf("  The voxel resolution of the generated map.\n");
  printf("-q\n");
  printf("  Run in quiet mode. Suppresses progress messages.\n");
  printf("-r=<resolution> (%g)\n", opt.resolution);
  printf("  The voxel resolution of the generated map.\n");
  printf("-s=<time> (%g)\n", opt.startTime);
  printf("  Specifies a time offset for the start time. Ignore points until the time offset from the first point "
         "exceeds this value.\n");
  printf("-e=<time> (%g)\n", opt.endTime);
  printf("  Specifies an end time relative to the first point. Stop after processing time interval of points.\n");
  printf("--rays=[off,lines,voxels,all] (lines)\n");
  printf("  Enable or turn off visualisation of sample rays.\n");
  printf("    off: disable. Lowest throughput\n");
  printf("    lines: visualise line samples. Lower throughput\n");
  printf("    voxels: visualise intersected voxels. High throughput\n");
  printf("    all: visualise all previous options. Very high throughput\n");
  printf("--samples=[off,voxel,points,all] (voxels)\n");
  printf("  Enable visualisation of sample voxels in each batch (occupied).\n");
  printf("    off: disable. Lowest throughput\n");
  printf("    voxels : visualise intersected voxels. Lower throughput\n");
  printf("    points: visualise sample points. High throughput\n");
  printf("    all: visualise all previous options. Very high throughput\n");
}

void initialiseDebugCategories(const Options &opt)
{
  TES_CATEGORY(g_tesServer, "Map", CAT_Map, 0, true);
  TES_CATEGORY(g_tesServer, "Populate", CAT_Populate, 0, true);
  TES_IF(opt.rays & Rays_Lines)
  {
    TES_CATEGORY(g_tesServer, "Rays", CAT_Rays, CAT_Populate, (opt.rays & Rays_Lines) != 0);
  }
  TES_IF(opt.rays & Rays_Voxels)
  {
    TES_CATEGORY(g_tesServer, "Free", CAT_FreeCells, CAT_Populate, (opt.rays & Rays_Lines) == 0);
  }
  TES_IF(opt.samples) { TES_CATEGORY(g_tesServer, "Occupied", CAT_OccupiedCells, CAT_Populate, true); }
  TES_CATEGORY(g_tesServer, "Info", CAT_Info, 0, true);
}

int main(int argc, char *argv[])
{
  Options opt;

  signal(SIGINT, onSignal);

  if (argc < 3)
  {
    usage(opt);
    return 0;
  }

  std::string str;
  for (int i = 1; i < argc; ++i)
  {
    if (argv[i][0] == '-')
    {
      bool ok = true;
      switch (argv[i][1])
      {
      case 'b':  // batch size
        ok = optionValue(argv[i] + 2, argc, argv, opt.batchSize);
        break;
      case 'e':  // start time
        ok = optionValue(argv[i] + 2, argc, argv, opt.endTime);
        break;
      case 'h':
        ok = optionValue(argv[i] + 2, argc, argv, opt.probHit);
        break;
      case 'm':
        ok = optionValue(argv[i] + 2, argc, argv, opt.probMiss);
        break;
      case 'o':
        ok = optionValue(argv[i] + 2, argc, argv, opt.outStream);
        break;
      case 'p':  // point limit
        ok = optionValue(argv[i] + 2, argc, argv, opt.pointLimit);
        break;
      case 'q':  // quiet
        opt.quiet = true;
        break;
      case 'r':  // resolution
        ok = optionValue(argv[i] + 2, argc, argv, opt.resolution);
        break;
      case 's':  // start time
        ok = optionValue(argv[i] + 2, argc, argv, opt.startTime);
        break;
      case '-':  // Long option name.
      {
        if (matchArg(&argv[i][2], "rays"))
        {
          ok = optionValue(argv[i] + 6, argc, argv, str);
          if (ok)
          {
            if (str.compare("off") == 0)
            {
              opt.rays = Rays_Off;
            }
            else if (str.compare("lines") == 0)
            {
              opt.rays = Rays_Lines;
            }
            else if (str.compare("voxels") == 0)
            {
              opt.rays = Rays_Voxels;
            }
            else if (str.compare("all") == 0)
            {
              opt.rays = Rays_All;
            }
            else
            {
              ok = false;
            }
          }
        }
        else if (matchArg(&argv[i][2], "samples"))
        {
          ok = optionValue(argv[i] + 9, argc, argv, str);
          if (ok)
          {
            if (str.compare("off") == 0)
            {
              opt.samples = Samples_Off;
            }
            else if (str.compare("voxels") == 0)
            {
              opt.samples = Samples_Voxels;
            }
            else if (str.compare("points") == 0)
            {
              opt.samples = Samples_Points;
            }
            else if (str.compare("all") == 0)
            {
              opt.samples = Samples_All;
            }
            else
            {
              ok = false;
            }
          }
        }
        break;
      }
      }

      if (!ok)
      {
        fprintf(stderr, "Failed to read %s option value.\n", argv[i]);
      }
    }
    else if (opt.cloudFile.empty())
    {
      opt.cloudFile = argv[i];
    }
    else if (opt.trajectoryFile.empty())
    {
      opt.trajectoryFile = argv[i];
    }
  }

  if (opt.cloudFile.empty())
  {
    fprintf(stderr, "Missing input cloud (-i)\n");
    return -1;
  }
  if (opt.trajectoryFile.empty())
  {
    fprintf(stderr, "Missing trajectory file (-t)\n");
    return -1;
  }

  // Initialise TES
  TES_SETTINGS(settings, tes::SF_Default);
  // Initialise server info.
  TES_SERVER_INFO(info, tes::XYZ);
  // Create the server. Use tesServer declared globally above.
  TES_SERVER_CREATE(g_tesServer, settings, &info);

  // Start the server and wait for the connection monitor to start.
  TES_SERVER_START(g_tesServer, tes::ConnectionMonitor::Asynchronous);
  TES_SERVER_START_WAIT(g_tesServer, 1000);

#ifdef TES_ENABLE
  if (!opt.outStream.empty())
  {
    g_tesServer->connectionMonitor()->openFileStream(opt.outStream.c_str());
    g_tesServer->connectionMonitor()->commitConnections();
  }

  std::cout << "Starting with " << g_tesServer->connectionCount() << " connection(s)." << std::endl;
#endif  // TES_ENABLE

  initialiseDebugCategories(opt);

  int res = populateMap(opt);
  TES_SERVER_STOP(g_tesServer);
  return res;
}
