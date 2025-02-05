#include <3escore/ByteValue.h>
#include <3escore/CollatedPacketDecoder.h>
#include <3escore/Endian.h>
#include <3escore/Log.h>
#include <3escore/Messages.h>
#include <3escore/MeshMessages.h>
#include <3escore/PacketBuffer.h>
#include <3escore/PacketReader.h>
#include <3escore/StreamUtil.h>
#include <3escore/TcpSocket.h>
#include <3escore/PacketStreamReader.h>

#include <array>
#include <csignal>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <type_traits>

#include <cxxopts.hpp>

struct PacketKey
{
  uint16_t routing_id = 0;
  uint16_t message_id = 0;

  uint32_t key() const noexcept
  {
    return static_cast<uint32_t>(routing_id) | static_cast<uint32_t>(message_id) << 16u;
  }
};

inline bool operator==(const PacketKey &a, const PacketKey &b)
{
  const uint32_t key_a = a.key();
  const uint32_t key_b = b.key();
  return key_a == key_b;
}

inline bool operator<(const PacketKey &a, const PacketKey &b)
{
  const uint32_t key_a = a.key();
  const uint32_t key_b = b.key();
  return key_a < key_b;
}

namespace std
{
template <>
struct hash<PacketKey>
{
  size_t operator()(const PacketKey &key) const noexcept { return key.key(); }
};
}  // namespace std


struct PacketInfo
{
  PacketKey key = {};
  size_t total_size_uncompressed = 0;
  // size_t total_size_compressed = 0;
  size_t total_payload_size = 0;
  uint32_t count = 0;
  /// Number of packets with a CRC.
  uint32_t crc_count = 0;
};

inline bool operator<(const PacketInfo &a, const PacketInfo &b)
{
  return a.key < b.key;
}

struct InfoComparator
{
public:
  using is_transparent = std::true_type;
  bool operator()(const PacketKey &a, const PacketInfo &b) const { return a < b.key; }
  bool operator()(const PacketInfo &a, const PacketKey &b) const { return a.key < b; }
  bool operator()(const PacketInfo &a, const PacketInfo &b) { return a < b; }
};

using InfoMap = std::unordered_map<PacketKey, PacketInfo>;
using RoutingMap = std::unordered_map<uint16_t, std::string>;
using MessageMap = std::unordered_map<PacketKey, std::string>;

struct Working
{
  RoutingMap routing_names;
  MessageMap message_names;
};

enum class ParseResult
{
  Ok,
  Help,
  ParseError,
  ValidationError
};

struct Options
{
  std::string filename;
  std::optional<tes::ByteUnit> display_unit;
};

bool validate(Options &opt)
{
  bool ok = true;
  if (opt.filename.empty())
  {
    tes::log::error("Filename must be specified");
    ok = false;
  }
  return ok;
}


ParseResult parseArgs(int argc, char *argv[], Options &opt)
{
  cxxopts::Options parser("3esinfo", "Information about 3es file content.");

  auto display_unit = tes::ByteUnit::Bytes;
  // clang-format off
  parser.add_options()
    ("help", "Show command line help.")
    ("file", "Data file to open (.3es)", cxxopts::value(opt.filename))
    ("du", "Size display unit: B, KiB, MiB, ...", cxxopts::value(display_unit))
    ;
  // clang-format on

  parser.parse_positional({ "file" });

  try
  {
    cxxopts::ParseResult parsed = parser.parse(argc, argv);

    if (parsed.count("help"))
    {
      // Force output: don't log as that could be filtered by log level.
      std::cout << parser.help() << std::endl;
      // Help already shown.
      return ParseResult::Help;
    }

    if (parsed.count("du"))
    {
      opt.display_unit = display_unit;
    }

    if (!validate(opt))
    {
      return ParseResult::ValidationError;
    }
  }
  catch (const cxxopts::exceptions::parsing &e)
  {
    tes::log::error("Argument error\n", e.what());
    return ParseResult::ParseError;
  }

  return ParseResult::Ok;
}


RoutingMap buildRoutingNames()
{
  return  //
    {
      { static_cast<uint16_t>(tes::MtNull), "Null" },
      { static_cast<uint16_t>(tes::MtServerInfo), "ServerInfo" },
      { static_cast<uint16_t>(tes::MtControl), "Control" },
      { static_cast<uint16_t>(tes::MtCollatedPacket), "CollatedPacket" },
      { static_cast<uint16_t>(tes::MtMesh), "Mesh" },
      { static_cast<uint16_t>(tes::MtCamera), "Camera" },
      { static_cast<uint16_t>(tes::MtCategory), "Category" },
      { static_cast<uint16_t>(tes::MtMaterial), "Material" },
      { static_cast<uint16_t>(tes::SIdSphere), "Sphere" },
      { static_cast<uint16_t>(tes::SIdBox), "Box" },
      { static_cast<uint16_t>(tes::SIdCone), "Cone" },
      { static_cast<uint16_t>(tes::SIdCylinder), "Cylinder" },
      { static_cast<uint16_t>(tes::SIdCapsule), "Capsule" },
      { static_cast<uint16_t>(tes::SIdPlane), "Plane" },
      { static_cast<uint16_t>(tes::SIdStar), "Star" },
      { static_cast<uint16_t>(tes::SIdArrow), "Arrow" },
      { static_cast<uint16_t>(tes::SIdMeshShape), "MeshShape" },
      { static_cast<uint16_t>(tes::SIdMeshSet), "MeshSet" },
      { static_cast<uint16_t>(tes::SIdPointCloudDeprecated), "PointCloudDeprecated" },
      { static_cast<uint16_t>(tes::SIdText3D), "Text3D" },
      { static_cast<uint16_t>(tes::SIdText2D), "Text2D" },
      { static_cast<uint16_t>(tes::SIdPose), "Pose" },
    };
}

MessageMap buildMessageNames()
{
  return  //
    {
      { { tes::MtNull, 0 }, "Null" },
      { { tes::MtServerInfo, 0 }, "ServerInfo" },
      { { tes::MtControl, tes::CIdNull }, "Null" },
      { { tes::MtControl, tes::CIdFrame }, "Frame" },
      { { tes::MtControl, tes::CIdCoordinateFrame }, "CoordinateFrame" },
      { { tes::MtControl, tes::CIdFrameCount }, "FrameCount" },
      { { tes::MtControl, tes::CIdForceFrameFlush }, "ForceFrameFlush" },
      { { tes::MtControl, tes::CIdReset }, "Reset" },
      { { tes::MtControl, tes::CIdKeyframe }, "Keyframe" },
      { { tes::MtControl, tes::CIdEnd }, "End" },
      { { tes::MtCollatedPacket, 0 }, "CollatedPacket" },
      { { tes::MtMesh, tes::MmtInvalid }, "Invalid" },
      { { tes::MtMesh, tes::MmtDestroy }, "Destroy" },
      { { tes::MtMesh, tes::MmtCreate }, "Create" },
      { { tes::MtMesh, tes::MmtVertex }, "Vertex" },
      { { tes::MtMesh, tes::MmtIndex }, "Index" },
      { { tes::MtMesh, tes::MmtVertexColour }, "VertexColour" },
      { { tes::MtMesh, tes::MmtNormal }, "Normal" },
      { { tes::MtMesh, tes::MmtUv }, "Uv" },
      { { tes::MtMesh, tes::MmtSetMaterial }, "SetMaterial" },
      { { tes::MtMesh, tes::MmtRedefine }, "Redefine" },
      { { tes::MtMesh, tes::MmtFinalise }, "Finalise" },
      { { tes::MtCamera, 0 }, "Camera" },
      { { tes::MtCategory, tes::CMIdName }, "Name" },
      { { tes::MtMaterial, 0 }, "Material" },
      { { tes::SIdSphere, tes::OIdNull }, "Null" },
      { { tes::SIdSphere, tes::OIdCreate }, "Create" },
      { { tes::SIdSphere, tes::OIdUpdate }, "Update" },
      { { tes::SIdSphere, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdSphere, tes::OIdData }, "Data" },
      { { tes::SIdBox, tes::OIdNull }, "Null" },
      { { tes::SIdBox, tes::OIdCreate }, "Create" },
      { { tes::SIdBox, tes::OIdUpdate }, "Update" },
      { { tes::SIdBox, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdBox, tes::OIdData }, "Data" },
      { { tes::SIdCone, tes::OIdNull }, "Null" },
      { { tes::SIdCone, tes::OIdCreate }, "Create" },
      { { tes::SIdCone, tes::OIdUpdate }, "Update" },
      { { tes::SIdCone, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdCone, tes::OIdData }, "Data" },
      { { tes::SIdCylinder, tes::OIdNull }, "Null" },
      { { tes::SIdCylinder, tes::OIdCreate }, "Create" },
      { { tes::SIdCylinder, tes::OIdUpdate }, "Update" },
      { { tes::SIdCylinder, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdCylinder, tes::OIdData }, "Data" },
      { { tes::SIdCapsule, tes::OIdNull }, "Null" },
      { { tes::SIdCapsule, tes::OIdCreate }, "Create" },
      { { tes::SIdCapsule, tes::OIdUpdate }, "Update" },
      { { tes::SIdCapsule, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdCapsule, tes::OIdData }, "Data" },
      { { tes::SIdPlane, tes::OIdNull }, "Null" },
      { { tes::SIdPlane, tes::OIdCreate }, "Create" },
      { { tes::SIdPlane, tes::OIdUpdate }, "Update" },
      { { tes::SIdPlane, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdPlane, tes::OIdData }, "Data" },
      { { tes::SIdStar, tes::OIdNull }, "Null" },
      { { tes::SIdStar, tes::OIdCreate }, "Create" },
      { { tes::SIdStar, tes::OIdUpdate }, "Update" },
      { { tes::SIdStar, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdStar, tes::OIdData }, "Data" },
      { { tes::SIdArrow, tes::OIdNull }, "Null" },
      { { tes::SIdArrow, tes::OIdCreate }, "Create" },
      { { tes::SIdArrow, tes::OIdUpdate }, "Update" },
      { { tes::SIdArrow, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdArrow, tes::OIdData }, "Data" },
      { { tes::SIdMeshShape, tes::OIdNull }, "Null" },
      { { tes::SIdMeshShape, tes::OIdCreate }, "Create" },
      { { tes::SIdMeshShape, tes::OIdUpdate }, "Update" },
      { { tes::SIdMeshShape, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdMeshShape, tes::OIdData }, "Data" },
      { { tes::SIdMeshSet, tes::OIdNull }, "Null" },
      { { tes::SIdMeshSet, tes::OIdCreate }, "Create" },
      { { tes::SIdMeshSet, tes::OIdUpdate }, "Update" },
      { { tes::SIdMeshSet, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdMeshSet, tes::OIdData }, "Data" },
      { { tes::SIdPointCloudDeprecated, tes::OIdNull }, "Null" },
      { { tes::SIdPointCloudDeprecated, tes::OIdCreate }, "Create" },
      { { tes::SIdPointCloudDeprecated, tes::OIdUpdate }, "Update" },
      { { tes::SIdPointCloudDeprecated, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdPointCloudDeprecated, tes::OIdData }, "Data" },
      { { tes::SIdText3D, tes::OIdNull }, "Null" },
      { { tes::SIdText3D, tes::OIdCreate }, "Create" },
      { { tes::SIdText3D, tes::OIdUpdate }, "Update" },
      { { tes::SIdText3D, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdText3D, tes::OIdData }, "Data" },
      { { tes::SIdText2D, tes::OIdNull }, "Null" },
      { { tes::SIdText2D, tes::OIdCreate }, "Create" },
      { { tes::SIdText2D, tes::OIdUpdate }, "Update" },
      { { tes::SIdText2D, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdText2D, tes::OIdData }, "Data" },
      { { tes::SIdPose, tes::OIdNull }, "Null" },
      { { tes::SIdPose, tes::OIdCreate }, "Create" },
      { { tes::SIdPose, tes::OIdUpdate }, "Update" },
      { { tes::SIdPose, tes::OIdDestroy }, "Destroy" },
      { { tes::SIdPose, tes::OIdData }, "Data" },
    };
}


bool checkCompatibility(const tes::PacketReader &reader)
{
  const auto version_major = reader.versionMajor();
  const auto version_minor = reader.versionMinor();

  // Exact version match.
  if (version_major == tes::kPacketVersionMajor &&
      version_minor == tes::kPacketCompatibilityVersionMinor)
  {
    return true;
  }
  // Check major version is in the allowed range (open interval).
  if (tes::kPacketCompatibilityVersionMajor < version_major &&
      version_major < tes::kPacketVersionMajor)
  {
    // Major version is between the allowed range.
    return true;
  }

  // Major version match, ensure minor version is in range.
  if (version_major == tes::kPacketVersionMajor && version_minor <= tes::kPacketVersionMinor)
  {
    return true;
  }

  // Major version compatibility match, ensure minor version is in range.
  if (version_major == tes::kPacketCompatibilityVersionMajor &&
      version_minor >= tes::kPacketCompatibilityVersionMinor)
  {
    return true;
  }

  return false;
}

void processPacket(tes::PacketReader &reader, InfoMap &info_map)
{
  const PacketKey key = { reader.routingId(), reader.messageId() };
  auto search_iter = info_map.find(key);
  PacketInfo *info = nullptr;
  if (search_iter == info_map.end())
  {
    const auto insert_pair = info_map.insert({ key, PacketInfo{ key } });
    search_iter = insert_pair.first;
  }
  info = &search_iter->second;

  info->total_size_uncompressed += reader.packetSize();
  info->total_payload_size += reader.payloadSize();
  ++info->count;
  info->crc_count += !(reader.flags() & tes::PFNoCrc);
}


std::string routingName(const RoutingMap &map, uint16_t id)
{
  const auto iter = map.find(id);
  if (iter != map.end())
  {
    return iter->second;
  }
  return "";
}


std::string routingName(const RoutingMap &map, const PacketKey &key)
{
  return routingName(map, key.routing_id);
}


std::string messageName(const MessageMap &map, const PacketKey &key)
{
  const auto iter = map.find(key);
  if (iter != map.end())
  {
    return iter->second;
  }
  return "";
}


tes::ByteValue byteValue(uint64_t bytes, const Options &opt)
{
  if (opt.display_unit.has_value())
  {
    return tes::ByteValue(bytes).as(*opt.display_unit);
  }
  return tes::ByteValue(bytes).succinct();
}


void displayInfo(const InfoMap &info, const Options &opt)
{
  const Working working = { buildRoutingNames(), buildMessageNames() };

  // CSV export
  std::cout << "routing_id,message_id,routing_name,message_name,count,total_size,"
               "total_payload_size,average_size,average_payload_size\n";

  char delim = ',';
  for (const auto &[key, packet_info] : info)
  {
    std::cout << key.routing_id << delim << key.message_id << delim
              << routingName(working.routing_names, key) << delim
              << messageName(working.message_names, key) << delim << packet_info.count << delim
              << byteValue(packet_info.total_size_uncompressed, opt) << delim
              << byteValue(packet_info.total_payload_size, opt) << delim
              << byteValue(packet_info.total_size_uncompressed / packet_info.count, opt) << delim
              << byteValue(packet_info.total_payload_size / packet_info.count, opt) << '\n';
  }

  std::cout.flush();
}


int main(int argc, char *argv[])
{
  Options opt = {};

  switch (parseArgs(argc, argv, opt))
  {
  case ParseResult::Ok:
    break;
  case ParseResult::Help:
    return 0;
  case ParseResult::ParseError:
    [[fallthrough]];
  case ParseResult::ValidationError:
    [[fallthrough]];
  default:
    return 1;
  }

  std::ifstream in_stream(opt.filename, std::ios::binary);
  if (!in_stream.is_open())
  {
    tes::log::error("Unable to open file ", opt.filename);
    return 1;
  }

  tes::PacketStreamReader reader(in_stream);
  tes::CollatedPacketDecoder packet_decoder;
  InfoMap info = {};

  bool ok = true;
  while (reader.isOk() && !reader.isEof() && ok)
  {
    auto [initial_packet_header, status, stream_pos] = reader.extractPacket();
    if (!initial_packet_header)
    {
      if (status != tes::PacketStreamReader::Status::End)
      {
        ok = false;
        tes::log::warn("Failed to load packet.");
      }
      continue;
    }

    // Handle collated packets by wrapping the header.
    // This is fine for normal packets too.
    packet_decoder.setPacket(initial_packet_header);

    // Iterate packets while we decode. These do not need to be released.
    while (const auto *packet_header = packet_decoder.next())
    {
      tes::PacketReader packet(packet_header);

      // Check the initial packet compatibility.
      if (!checkCompatibility(packet))
      {
        ok = false;
        tes::log::warn("Unsupported packet version: ", packet.versionMajor(), ".",
                       packet.versionMinor());
        continue;
      }

      processPacket(packet, info);
    }
  }

  displayInfo(info, opt);

  return 0;
}
