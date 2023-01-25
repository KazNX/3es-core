
list(APPEND PUBLIC_HEADERS
  # General headers
  BoundsCuller.h
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
  camera/Camera.h
  camera/Controller.h
  camera/Fly.h
  command/Args.h
  command/Command.h
  command/CommandResult.h
  command/connection/Close.h
  command/connection/OpenFile.h
  command/connection/OpenTcp.h
  command/playback/Loop.h
  command/playback/Pause.h
  command/playback/SkipBackward.h
  command/playback/SkipForward.h
  command/playback/SkipToFrame.h
  command/playback/StepBackward.h
  command/playback/StepForward.h
  command/playback/Stop.h
  data/DataThread.h
  data/NetworkThread.h
  data/StreamThread.h
  handler/Camera.h
  handler/Category.h
  handler/MeshResource.h
  handler/MeshSet.h
  handler/MeshShape.h
  handler/Message.h
  handler/Shape.h
  handler/Text2d.h
  handler/Text3d.h
  mesh/Converter.h
  painter/Arrow.h
  painter/Box.h
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
  shaders/Edl.h
  shaders/Flat.h
  shaders/PointGeom.h
  shaders/Pvm.h
  shaders/Shader.h
  shaders/ShaderLibrary.h
  shaders/VertexColour.h
  shaders/VoxelGeom.h
  util/Enum.h
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
  camera/Camera.cpp
  camera/Controller.cpp
  camera/Fly.cpp
  command/Args.cpp
  command/Command.cpp
  command/CommandResult.cpp
  command/connection/Close.cpp
  command/connection/OpenFile.cpp
  command/connection/OpenTcp.cpp
  command/playback/Loop.cpp
  command/playback/Pause.cpp
  command/playback/SkipBackward.cpp
  command/playback/SkipForward.cpp
  command/playback/SkipToFrame.cpp
  command/playback/StepBackward.cpp
  command/playback/StepForward.cpp
  command/playback/Stop.cpp
  data/DataThread.cpp
  data/NetworkThread.cpp
  data/StreamThread.cpp
  handler/Camera.cpp
  handler/Category.cpp
  handler/MeshResource.cpp
  handler/MeshSet.cpp
  handler/MeshShape.cpp
  handler/Message.cpp
  handler/Shape.cpp
  handler/Text2d.cpp
  handler/Text3d.cpp
  mesh/Converter.cpp
  painter/Arrow.cpp
  painter/Box.cpp
  painter/Capsule.cpp
  painter/Cone.cpp
  painter/Cylinder.cpp
  painter/Plane.cpp
  painter/Pose.cpp
  painter/ShapeCache.cpp
  painter/ShapePainter.cpp
  painter/Sphere.cpp
  painter/Star.cpp
  painter/Text.cpp
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
)
