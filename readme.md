# 3rd Eye Scene

3rd Eye Scene is a visual debugger and debugging aid in the vein of [rviz](http://wiki.ros.org/rviz) or physics engine viewers such as [Havok Visual Debugger](https://www.havok.com/physics/) or [PhysX Visual Debugger](https://developer.nvidia.com/physx-visual-debugger). Whereas those tools are tightly bound to their respective SDKs, 3rd Eye Scene is a generalised tool, which can be used to remotely visualise and debug any real time or non real time 3D algorithm. Conceptually, it can be thought of as a remote rendering application. A 3es server may be embedded into any program, then 3es render commands are used to instrument the target program. The 3es viewer client application is then used to view, record and playback these commands.

## Key Features

- Remote 3D rendering from any application
- Record rendered data
- Playback and step through recorded data
- Open, extensible protocol
- Plugin extensible to visualise specialised geometry

## Use Cases

- Visualising geometric algorithms
  - Mesh operations
  - Geometric intersection tests
  - Point cloud processing
- Remote visualisation
  - Visualise 3D data from headless processes
- Real time visualisation
  - Remote visualisation
  - Visualise "hidden" data
    - Physics geometry
    - AI logic and constructs
- QA testing
  - Record test sessions and attach 3es files to bug reports.

## Building

## Prerequisites and recommendations

The following are prerequisites to building the 3rd Eye Scene C++ project.

- General prerequisites
  - A C++ 17 compatible compiler.
  - CMake version 3.16 or higher
    - CMake 3.17 is required when using the `Ninja Multi-Config` generator.
  - Berkley sockets on Winsock2
- Viewer prerequisites
  - [Magnum Graphics](https://magnum.graphics/) version 2020.06
  - [GLFW](https://www.glfw.org/)
  - [cxxopts](https://github.com/jarro2783/cxxopts)
  - [Native File Dialog](https://github.com/mlabbe/nativefiledialog)

The following items are recommended to aid in building 3rd Eye Scene.

- [zlib](https://zlib.net/) for message compression
- [vcpkg](https://vcpkg.io/) to automate fetching prerequisites
- [Google Test](https://github.com/google/googletest) for unit tests.

### Linux prerequisites

On Linux the following apt packages be installed before using a `vcpkg` build which includes the 3rd Eye Scene viewer as `vcpkg` does not provide these packages.

```bash
sudo apt install -y \
  autoconf \
  libegl1-mesa-dev \
  libibus-1.0-dev \
  libtool \
  libwayland-dev \
  libxkbcommon-dev \
  zenity
```

### Building with vcpkg

Building with VCPKG is supported in order to fetch the dependencies. This affects both 3escore and 3esviewer.

```shell
# Must cmake configure from the source directory so vcpkg can find the vcpkg.json manifest.
mkdir build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=<vcpkg_dir>/scripts/buildsystems/vcpkg.cmake -DVCPKG_MANIFEST_FEATURES=test;viewer
cmake --build build --targets all --
```

> Note: it's recommended to add `-G Ninja` as ninja makes for fast builds.

For a multi-configuration generator - such as Visual Studio or `Ninja Multi-Config` (since CMake 3.17) we specify the configuration when we build instead.

```shell
# Must cmake configure from the source directory so vcpkg can find the vcpkg.json manifest.
mkdir build
cmake -B build -S . -G "Ninja Multi-Config" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=<vcpkg_dir>/scripts/buildsystems/vcpkg.cmake -DVCPKG_MANIFEST_FEATURES=test;viewer
cmake --build build --config Release --targets all --
```

## 3rd Eye Scene Client

Source code for the 3rd Eye Scene C++ client viewer application is available as part of the 3es-core repo. The older C# viewer is also available on [GitHub](https://github.com/data61/3rdEyeScene)

## Integrating 3rd Eye Scene Server Code

The 3rd Eye Scene core includes code for both a C++ and a C# based server. This section focuses on integrating the C++ code to debug an application. The example presented here is the 3rd-occupancy example included in the TES source code release. The 3es macro interface is presented later.

Before sending TES messages, a `tes::Server` object must be declared and initialised as shown below.

```c++
#include <3escore/ConnectionMonitor.h>
#include <3escore/Server.h>

tes::Server *g_tes_server = nullptr;  // Global declaration.

void initialiseTes()
{
  // Configure settings: compression and collation on (required by compression)
  tes::ServerSettings settings(tes::SF_Compress | tes::SF_Collate);
  // Setup server info to the client.
  tes::ServerInfoMessage serverInfo;
  tes::initDefaultServerInfo(&serverInfo);
  // Coordinate axes listed as left/right, forward, up
  serverInfo.coordinateFrame = tes::XYZ;

  // Create the server.
  g_tes_server = tes::Server::create(settings, &serverInfo);
  // Setup asynchronous connection monitoring.
  // Connections must be committed synchronously.
  g_tes_server->connectionMonitor()->start(tes::ConnectionMonitor::Asynchronous);

  // Optional: wait 1000ms for the first connection before continuing.
  if (g_tes_server->connectionMonitor()->waitForConnection(1000) > 0)
  {
    g_tes_server->connectionMonitor()->commitConnections();
  }
}
```

Several key server methods must be called periodically to manage the connection. These calls are listed and explained below.

```c++
#include <3escore/ConnectionMonitor.h>
#include <3escore/Server.h>

void endFrame(float dt = 0.0f)
{
  // Mark the end of frame. Flushed collated packets.
  g_tes_server->updateFrame(dt);
  // In synchronous mode, listen for incoming connections.
  if (g_tes_server->connectionMonitor()->mode() == tes::ConnectionMonitor::Synchronous)
  {
    g_tes_server->connectionMonitor()->monitorConnections();
  }
  // Activate any newly accepted connections and expire old ones.
  g_tes_server->connectionMonitor()->commitConnections();
  // Update any bulk resource data transfer.
  g_tes_server->updateTransfers(0);
}
```

Once the server has been created and initialised it becomes possible to invoke object creation and update commands. The code below shows the creation and animation of a box shape as well as the creation of some transient objects.

```c++
#include <3escore/Colour.h>
#include <3escore/ConnectionMonitor.h>
#include <3escore/Maths.h>
#include <3escore/Server.h>
#include <3escore/Vector3f.h>

#include <3escore/shapes/Box.h>

void animateBox(tes::Server &server)
{
  // Declare a box.
  tes::Box box(
    1,  // ID
    tes::Vector3f(0, 0, 0), // Position
    tes::Vector3f(0.2f, 0.1f, 0.5f)); // Dimensions

  box.setColour(tes::Colour(0, 0, 255));

  // Create the box on the client.
  server.create(box);

  const int steps = 90;
  for (int i = 0; i <= steps; ++i)
  {
    // Update the box.
    box.setPosZ(std::sin(tes::degToRad(i / float(steps) * float(M_PI))));
    server.update(box);
    endFrame(1.0f / float(steps));
  }

  // Destroy the box.
  server.destroy(box);
  endFrame(0.0f);
}
```

To correct dispose of the server, call `dispose()`.

```c++
#include <3escore/Server.h>

void releaseTes()
{
  if (g_tes_server)
  {
    // Close connections.
    g_tes_server->close();
    // Destroy the server.
    g_tes_server->dispose();
    g_tes_server = nullptr;
  }
}
```

## Using Categories

Categories may be used to logically group objects in the viewer client. Objects from specific categories can be hidden and shown as a group. Categories are form a hierarchy, with each category having an optional parent. The code below shows an example category initialisation.

```c++
#include <3escore/Messages.h>
#include <3escore/Server.h>
#include <3escore/ServerApi.h>

void initCategories()
{
  // CAT_Xxx are members of an enum. This builds the following tree:
  // - Map
  // - Populate
  //   - Rays
  //   - Free
  //   - Occupied
  // - Info
  defineCategory(g_tes_server, "Map", CAT_Map, 0, true);
  defineCategory(g_tes_server, "Populate", CAT_Populate, 0, true);
  defineCategory(g_tes_server, "Rays", CAT_Rays, CAT_Populate, true);
  defineCategory(g_tes_server, "Free", CAT_FreeCells, CAT_Populate, false);
  defineCategory(g_tes_server, "Occupied", CAT_OccupiedCells, CAT_Populate, true);
  defineCategory(g_tes_server, "Info", CAT_Info, 0, true);
}
```

## Conditional instrumentation

It is also possible to use preprocessor macros to wrap 3rd Eye Scene API calls. This is to support removing all 3es instrumentation code via the preprocessor thereby eliminating all associated overhead. The examples above can be rewritten using the macro interface as shown below. The `animateBox2()` function is equivalent to the `animateBox()` function, but uses transient objects instead of updating a single object.

```c++
#include <3escore/ServerApi.h>

// Declare global server pointer.
tes::ServerPtr g_tes_server;

void initialiseTes()
{
  // Note: Since every statement here is conditional on TES_ENABLE, it's also viable to wrap the
  // entire block in
  // #ifdef TES_ENABLE
  // #endif  // TES_ENABLE

  // Initialise TES
  TES_STMT(g_tes_server = createServer(
    tes::ServerSettings(tes::SFCompress | tes::SFCollate),
    tes::XYZ));
  TES_STMT(stopServer(g_tes_server));

  // Start the server and wait for the connection monitor to start.
  TES_STMT(startServer(g_tes_server, tes::ConnectionMode::Asynchronous));
  // Wait 1s for a connection before continuing.
  TES_STMT(waitForConnection(g_tes_server, 1000u));
}

void initCategories()
{
  // CAT_Xxx are members of an enum. This builds the following tree:
  // - Map
  // - Populate
  //   - Rays
  //   - Free
  //   - Occupied
  // - Info
  TES_STMT(tes::defineCategory(g_tes_server, "Map", CAT_Map, 0, true));
  TES_STMT(tes::defineCategory(g_tes_server, "Populate", CAT_Populate, 0, true));
  TES_STMT(tes::defineCategory(g_tes_server, "Rays", CAT_Rays, CAT_Populate, true));
  TES_STMT(tes::defineCategory(g_tes_server, "Free", CAT_FreeCells, CAT_Populate, false));
  TES_STMT(tes::defineCategory(g_tes_server, "Occupied", CAT_OccupiedCells, CAT_Populate, true));
  TES_STMT(tes::defineCategory(g_tes_server, "Info", CAT_Info, 0, true));
}

void animateBox(tes::Server &server)
{
  // Create a box on the client.
  TES_STMT(ScopedShape<Box> box(&server, tes::Box(
          1,  // ID
          tes::Transform(
            tes::Vector3(0.0f), // Position
            tes::Vector3f(0.2f, 0.1f, 0.5f)) // Dimensions
          ))
          ->setColour(tes::Colour::Blue));

  for (int i = 0; i <= steps; ++i)
  {
    // Update the box.
    TES_STMT(box->setPosition(
      tes::Vector3f(0, 0, std::sin(tes::deg2Rad(i / float(steps) * float(M_PI))))));
    TES_STMT(box.update(UFPosition));
    TES_STMT(updateServer(g_tes_server, 1.0f / float(steps)));
  }

  // Destroy the box.
  TES_STMT(box.destroy());
  TES_STMT(updateServer(g_tes_server));
}

void animateBox2(tes::Server &server)
{
  for (int i = 0; i <= steps; ++i)
  {
    // Create a transient box on each iteration.
    TES_STMT(create(g_tes_server,
      Box(
            0,  // Transient ID
            tes::Transform(
                tes::Vector3(0.0f, 0.0f, 
                  std::sin(tes::deg2Rad(i / float(steps) * float(M_PI)))), // Position
                tes::Vector3f(0.2f, 0.1f, 0.5f) // Dimensions
                )).setColour(tes::Colour::Blue)

    ));
    TES_STMT(updateServer(g_tes_server, 1.0f / float(steps)));
  }

  TES_STMT(updateServer(g_tes_server));
}

void releaseTes()
{
  TES_STMT(stopServer(g_tes_server));
}
```

Additional documentation can be found at [https://data61.github.io/3rdEyeScene/](https://data61.github.io/3rdEyeScene/)
