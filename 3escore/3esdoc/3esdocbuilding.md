# Building 3rd Eye Scene {#docbuild}

Building all the 3<sup>rd</sup> Eye Scene components requires the following tools and libraries:

Dependency                                  | For Components
------------------------------------------- | -----------------
Visual Studio or Xamarin Studio             | C# core
A C++17 compatible compiler                 | C++ core
CMake                                       | C++
Winsock2                                    | C++ Windows
Berkley Sockets                             | C++ Linux, MacOS
Magnum graphics (optional)                  | Viewer (updated)
Unity 3D (optional)                         | Viewer (legacy)

Visual Studio Community Edition and Unity Personal Edition are both sufficient for building.

The following components are optional:

Dependency                                  | For Components
------------------------------------------- | -----------------
zlib (highly recommended)                   | C++ core
doxygen                                     | Documentation
graphviz                                    | Documentation

All of these components should be installed before building. On Windows CMake may not automatically locate zlib and using "cmake-gui" is recommended to help locate zlib for CMake before building.

## Building C# 3es Core

> Note the 3es C# code is stored in a separate [git repository](https://github.com/csiro-robotics/3rdEyeScene).

Before building the 3esCore project, note that the 3esRuntime library depends on the UnityEngine libraries. The project file is set up to attempt to automatically locate UnityEngine.dll as follows:

1. Using `/Applications/Unity/Unity.app/Contents/Managed/UnityEngine.dll` if it exists (default MacOS install location).
2. Using `C:\Program Files\Unity\Editor\Data\Managed\UnityEngine.dll` if it exists (default Windows install location).
3. Using the environment variable `UNITY_DLL_PATH`.

This will only work automatically when Unity is installed to the default location on either MacOS or Windows. Otherwise, UNITY_DLL_PATH must be set to the correct location first.

Simply open up 3esCore.sln and build. Note that the default build configuration is for "Any CPU" to support 64-bit code.

**Important:** The 3esCore and 3esRuntime projects are configured to automatically output into the 3rdEyeScene Unity project Assets folder using a relative path. Specifically "../3rdEyeScene/Assets/plugins". This directly supports the 3<sup>rd</sup> Eye Scene viewer application. Both Debug and Release builds target this folder and will overwrite one another. Thus whatever was build last is used by Unity.

## Building Unity Viewer Client

To build the viewer application:

1. Build the [C# core"](#building-c-3es-core), preferably in `Release` mode.
2. Open Unity
3. Open the Unity project under the 3rdEyeScene directory.
4. Open the "3rdEyeScene.unity" scene.
5. Select "File->Build Settings"
6. Select the desired "Target Platform"
7. Select the x86_64 architecture if available
8. Click "Build" or "Build and Run" and choose the output directory

You may now run the viewer application.

### Supported Platforms

Unity has very good cross platform support. However, the viewer application has only been tested on the following
platforms. Other platforms may not work at all and may certainly have UI issues.

Supported platforms:

- Windows
- MacOS

## Building C++ Core

This section details how to build the C++ core library to include in your code. There is also an option of including the
3es code into your own build tree. The 3es core library acts as the server to the Unity visualisation client viewer or
the C# recording client.

-# Install the prerequisites listed above.
-# Open a command prompt or terminal window.
-# Change directories to the 3es cpp directory.
-# Create a "build" sub directory (may be named as you like)
-# Change into the build directory.
-# Run CMake to build the project files (see below)
-# Build your library against the generated files
-# Use 3es-

CMake supports many platforms and may need to be run differently in various build environments. The recommended ones are
detailed below.

### Windows CMake

Under Windows, the recommended CMake command is:

```bat
cmake -G "Visual Studio 16 2019" -Ax64 ..
```

Note that the generator may be changed to a different version of Visual Studio (below VS12 is definitely not supported), but appending "Win64" is recommended to generate 64-bit code. Omit "Win64" if 32-bit code is required.

At this point, it is likely that zlib has not been located. To address this, run "cmake-gui .." and set the following variables (advanced mode may be required):

- `ZLIB_INCLUDE_DIR`: set to the directory containing `zlib.h`
- `ZLIB_LIBRARY_DEBUG`: set to point to the debug version of the zlib library: `zlibd.lib` (optional)
- `ZLIB_LIBRARY_RELEASE`: set to point to the release version of the zlib library: `zlib.lib`
These steps can be omitted if compression is not required.

After running CMake, open 3rd-eye-scene.sln (in the build directory) and build.

This generates a the core 3es C++ static libraries. These do nothing on their own and need to be incorporated into another application. To use them in your application, first build the "INSTALL" project in 3rd-eye-scene.sln. By default, this installs to `C:\Program Files\3es`, which may not be desirable and requires elevated privileges to success (requires administrator mode). This can be changed using cmake-gui, setting the `CMAKE_INSTALL_PREFIX` to the desired installation location (recommended).

Next, ensure your library includes the installed header files and links against the installed static libraries. Finally, use the TES_ macros to execute 3es commands.

### Linux and MacOS CMake

Under Linux and MacOS, the recommended CMake and build commands are:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
make install
```

Next ensure your application includes and links the installed header and library files, then use TES_ macros to manage the 3es server.

## Integrating C++ Server Code Into Your Build Tree

An alternative way of using the C++ code is to put the 3escore code into your own build tree. There are several ways to do this, but the following section describes how to do so assuming you are also using CMake for your own build tree.

1. Copy the `3escore` code directories into your build tree (optionally in a sub-directory).
2. Copy the cmake directory (or just `cmake/3es.cmake`) into into the same directory. You should have three new directories all in the same parent directory: `3escore` and cmake.
3. Edit your project's CMakeLists.txt file to add `add_subdirectory(3escore)`.
4. Include 3es server into your own executable or library using the CMake commands listed below.

```cmake
# After your add_library() or add_executable() command

target_include_directories(mylibrary $<TARGET_PROPERTY:3escore,INCLUDE_DIRECTORIES>)
target_link_libraries(mylibrary 3escore)
```

Note: replace `mylibrary` with the name of your executable or library as passed to `add_library()` or `add_executable()`.

## Integrating C# Server Code

To use the C# server library simply build the C# code, then add a reference to the C# `3esServer.dll` in your project. Consult the Visual Studio or Xamarin Studio documentation for adding library references to C# projects for more information.
