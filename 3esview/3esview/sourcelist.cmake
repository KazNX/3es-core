
list(APPEND PUBLIC_HEADERS
  # General headers
  BoundsCuller.h
  Constants.h
  DrawParams.h
  EdlEffect.h
  FboEffect.h
  FramesPerSecondWindow.h
  FrameStamp.h
  Magnum.h
  MagnumColour.h
  MagnumV3.h
  ThirdEyeScene.h
  ViewableWindow.h
  Viewer.h
  ViewerLog.h
  camera/Camera.h
  camera/Controller.h
  camera/Fly.h
  command/Args.h
  command/Command.h
  command/CommandResult.h
  command/DefaultCommands.h
  command/Set.h
  command/Shortcut.h
  command/camera/SetCamera.h
  command/connection/Close.h
  command/connection/OpenFile.h
  command/connection/OpenTcp.h
  command/playback/Loop.h
  command/playback/Record.h
  command/playback/Pause.h
  command/playback/SkipBackward.h
  command/playback/SkipForward.h
  command/playback/SkipToFrame.h
  command/playback/Speed.h
  command/playback/StepBackward.h
  command/playback/StepForward.h
  command/playback/Stop.h
  data/DataThread.h
  data/KeyframeStore.h
  data/NetworkThread.h
  data/StreamRecorder.h
  data/StreamThread.h
  handler/Camera.h
  handler/Category.h
  handler/MeshResource.h
  handler/MeshSet.h
  handler/MeshShape.h
  handler/Message.h
  handler/Shape.h
  handler/Text.h
  handler/Text2D.h
  handler/Text3D.h
  mesh/Converter.h
  painter/Arrow.h
  painter/Box.h
  painter/CategoryState.h
  painter/Capsule.h
  painter/Cone.h
  painter/Cylinder.h
  painter/Plane.h
  painter/Pose.h
  painter/ShapeCache.h
  painter/ShapePainter.h
  painter/Sphere.h
  painter/Star.h
  painter/Text.h
  settings/Camera.h
  settings/Colour.h
  settings/Enum.h
  settings/Loader.h
  settings/Log.h
  settings/Numeric.h
  settings/Playback.h
  settings/Render.h
  settings/Settings.h
  settings/Values.h
  shaders/Edl.h
  shaders/Flat.h
  shaders/PointGeom.h
  shaders/Pvm.h
  shaders/Shader.h
  shaders/ShaderLibrary.h
  shaders/VertexColour.h
  shaders/VoxelGeom.h
  util/Enum.h
  util/PendingAction.h
  util/ResourceList.h
)

list(APPEND SOURCES
  BoundsCuller.cpp
  EdlEffect.cpp
  FboEffect.cpp
  FramesPerSecondWindow.cpp
  FrameStamp.cpp
  ThirdEyeScene.cpp
  Viewer.cpp
  ViewerLog.cpp
  camera/Camera.cpp
  camera/Controller.cpp
  camera/Fly.cpp
  command/Args.cpp
  command/Command.cpp
  command/CommandResult.cpp
  command/DefaultCommands.cpp
  command/Set.cpp
  command/Shortcut.cpp
  command/camera/SetCamera.cpp
  command/connection/Close.cpp
  command/connection/OpenFile.cpp
  command/connection/OpenTcp.cpp
  command/playback/Loop.cpp
  command/playback/Pause.cpp
  command/playback/Record.cpp
  command/playback/SkipBackward.cpp
  command/playback/SkipForward.cpp
  command/playback/SkipToFrame.cpp
  command/playback/Speed.cpp
  command/playback/StepBackward.cpp
  command/playback/StepForward.cpp
  command/playback/Stop.cpp
  data/DataThread.cpp
  data/KeyframeStore.cpp
  data/NetworkThread.cpp
  data/StreamRecorder.cpp
  data/StreamThread.cpp
  handler/Camera.cpp
  handler/Category.cpp
  handler/MeshResource.cpp
  handler/MeshSet.cpp
  handler/MeshShape.cpp
  handler/Message.cpp
  handler/Shape.cpp
  handler/Text2D.cpp
  handler/Text3D.cpp
  mesh/Converter.cpp
  painter/Arrow.cpp
  painter/Box.cpp
  painter/Capsule.cpp
  painter/CategoryState.cpp
  painter/Cone.cpp
  painter/Cylinder.cpp
  painter/Plane.cpp
  painter/Pose.cpp
  painter/ShapeCache.cpp
  painter/ShapePainter.cpp
  painter/Sphere.cpp
  painter/Star.cpp
  painter/Text.cpp
  settings/Loader.cpp
  settings/Settings.cpp
  shaders/Edl.cpp
  shaders/Edl.frag
  shaders/Edl.vert
  shaders/Flat.cpp
  shaders/Point.frag
  shaders/Point.geom
  shaders/Point.vert
  shaders/PointGeom.cpp
  shaders/Shader.cpp
  shaders/ShaderLibrary.cpp
  shaders/VertexColour.cpp
  shaders/Voxel.frag
  shaders/Voxel.geom
  shaders/Voxel.vert
  shaders/VoxelGeom.cpp
  util/ResourceList.cpp
)

list(APPEND PRIVATE_SOURCES
  3p/cfgpath.h
)
