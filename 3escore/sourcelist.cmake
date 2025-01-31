
list(APPEND PUBLIC_HEADERS
  # General headers
  AssertRange.h
  BaseConnection.h
  Bounds.h
  ByteValue.h
  CollatedPacket.h
  CollatedPacketDecoder.h
  Colour.h
  CompressionLevel.h
  Connection.h
  ConnectionMonitor.h
  CoordinateFrame.h
  CoreUtil.h
  Crc.h
  Debug.h
  Endian.h
  Enum.h
  Exception.h
  Feature.h
  FileConnection.h
  Finally.h
  IntArg.h
  Log.h
  Maths.h
  MathsManip.h
  MathsStream.h
  Matrix3.h
  Matrix3.inl
  Matrix4.h
  Matrix4.inl
  MeshMessages.h
  Messages.h
  Meta.h
  PacketBuffer.h
  PacketHeader.h
  PacketReader.h
  PacketStream.h
  PacketStreamReader.h
  PacketWriter.h
  PlaneGeom.h
  Ptr.h
  Quaternion.h
  Quaternion.inl
  QuaternionArg.h
  Resource.h
  ResourcePacker.h
  Rotation.h
  Rotation.inl
  Server.h
  ServerApi.h
  ServerApiMinimal.h
  ServerUtil.h
  StreamUtil.h
  TcpListenSocket.h
  TcpSocket.h
  Throw.h
  Timer.h
  TransferProgress.h
  Transform.h
  TriGeom.h
  TriGeom.inl
  V3Arg.h
  Vector3.h
  Vector4.h
  VectorHash.h
  DataBuffer.h
  DataBuffer.inl
)

list(APPEND PUBLIC_SHAPE_HEADERS
  # Shape headers
  shapes/Arrow.h
  shapes/Box.h
  shapes/Capsule.h
  shapes/Cone.h
  shapes/Cylinder.h
  shapes/Id.h
  shapes/MeshComponentFlag.h
  shapes/MeshPlaceholder.h
  shapes/MeshResource.h
  shapes/MeshSet.h
  shapes/MeshShape.h
  shapes/MultiShape.h
  shapes/MutableMesh.h
  shapes/Plane.h
  shapes/PointCloud.h
  shapes/Pose.h
  shapes/Shape.h
  shapes/Shapes.h
  shapes/SimpleMesh.h
  shapes/Sphere.h
  shapes/Star.h
  shapes/Text2D.h
  shapes/Text3D.h
)

list(APPEND PUBLIC_TESSELLATE_HEADERS
  tessellate/Arrow.h
  tessellate/Box.h
  tessellate/Capsule.h
  tessellate/Cone.h
  tessellate/Cylinder.h
  tessellate/Sphere.h
)


list(APPEND SOURCES
  BaseConnection.cpp
  Bounds.cpp
  ByteValue.cpp
  CollatedPacket.cpp
  CollatedPacketDecoder.cpp
  Colour.cpp
  ConnectionMonitor.cpp
  CoreUtil.cpp
  Crc.cpp
  Endian.cpp
  Exception.cpp
  Feature.cpp
  FileConnection.cpp
  Log.cpp
  Maths.cpp
  MathsManip.cpp
  Matrix3.cpp
  Matrix4.cpp
  Messages.cpp
  PacketBuffer.cpp
  PacketHeader.cpp
  PacketReader.cpp
  PacketStream.cpp
  PacketStreamReader.cpp
  PacketWriter.cpp
  PlaneGeom.cpp
  Ptr.cpp
  Quaternion.cpp
  Resource.cpp
  ResourcePacker.cpp
  Rotation.cpp
  ServerApi.cpp
  ServerApiOff.cpp
  StreamUtil.cpp
  Throw.cpp
  Timer.cpp
  Transform.cpp
  TriGeom.cpp
  Vector3.cpp
  Vector4.cpp
  DataBuffer.cpp

  shapes/Arrow.cpp
  shapes/Box.cpp
  shapes/Capsule.cpp
  shapes/Cone.cpp
  shapes/Cylinder.cpp
  shapes/MeshPlaceholder.cpp
  shapes/MeshResource.cpp
  shapes/MeshSet.cpp
  shapes/MeshShape.cpp
  shapes/MultiShape.cpp
  shapes/MutableMesh.cpp
  shapes/Plane.cpp
  shapes/PointCloud.cpp
  shapes/Pose.cpp
  shapes/Shape.cpp
  shapes/Shapes.cpp
  shapes/SimpleMesh.cpp
  shapes/Sphere.cpp
  shapes/Star.cpp
  shapes/Text2D.cpp
  shapes/Text3D.cpp

  tessellate/Arrow.cpp
  tessellate/Box.cpp
  tessellate/Capsule.cpp
  tessellate/Cone.cpp
  tessellate/Cylinder.cpp
  tessellate/Sphere.cpp
)

list(APPEND PRIVATE_SOURCES
  private/CollatedPacketZip.cpp
  private/CollatedPacketZip.h
  private/TcpConnection.cpp
  private/TcpConnection.h
  private/TcpConnectionMonitor.cpp
  private/TcpConnectionMonitor.h
  private/TcpServer.cpp
  private/TcpServer.h
)

if(MSVC)
  list(APPEND PRIVATE_SOURCES
    win/Debug.cpp
  )
else(MSVC)
  list(APPEND PRIVATE_SOURCES
    nix/Debug.cpp
  )
endif(MSVC)

if(TES_SOCKETS STREQUAL "custom")
  list(APPEND PRIVATE_HEADERS
    tcp/TcpBase.h
    tcp/TcpDetail.h
  )

  list(APPEND PRIVATE_SOURCES
    tcp/TcpBase.cpp
    tcp/TcpListenSocket.cpp
    tcp/TcpSocket.cpp
  )
elseif(TES_SOCKETS STREQUAL "Qt")
  list(APPEND PRIVATE_HEADERS
    qt/TcpDetail.h
  )

  list(APPEND PRIVATE_SOURCES
    qt/TcpListenSocket.cpp
    qt/TcpSocket.cpp
  )
endif()
