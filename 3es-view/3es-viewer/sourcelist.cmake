
list(APPEND PUBLIC_HEADERS
  # General headers
  3esboundsculler.h
  3esedleffect.h
  3esfboeffect.h
  3esframestamp.h
  3esmagnum.h
  3esmagnumcolour.h
  3esmagnumv3.h
  3esthirdeyescene.h
  3esviewablewindow.h
  3esviewer.h
  camera/3escamera.h
  camera/3escontroller.h
  camera/3esfly.h
  data/3esdatathread.h
  data/3esstreamthread.h
  handler/3escamera.h
  handler/3escategory.h
  handler/3esmeshresource.h
  handler/3esmeshset.h
  handler/3esmeshshape.h
  handler/3esmessage.h
  handler/3esshape.h
  painter/3esshapecache.h
  painter/3esshapepainter.h
  painter/3esarrow.h
  painter/3esbox.h
  painter/3escapsule.h
  painter/3escone.h
  painter/3escylinder.h
  painter/3esplane.h
  painter/3espose.h
  painter/3essphere.h
  painter/3esstar.h
  mesh/3esconverter.h
  shaders/3esedl.h
  util/3esenum.h
  util/3esresourcelist.h
)

list(APPEND SOURCES
  3esboundsculler.cpp
  3esedleffect.cpp
  3esfboeffect.cpp
  3esframestamp.cpp
  3esthirdeyescene.cpp
  3esviewer.cpp
  camera/3escamera.cpp
  camera/3escontroller.cpp
  camera/3esfly.cpp
  data/3esdatathread.cpp
  data/3esstreamthread.cpp
  handler/3escamera.cpp
  handler/3escategory.cpp
  handler/3esmeshresource.cpp
  handler/3esmeshset.cpp
  handler/3esmeshshape.cpp
  handler/3esmessage.cpp
  handler/3esshape.cpp
  painter/3esshapecache.cpp
  painter/3esshapepainter.cpp
  painter/3esarrow.cpp
  painter/3esbox.cpp
  painter/3escapsule.cpp
  painter/3escone.cpp
  painter/3escylinder.cpp
  painter/3esplane.cpp
  painter/3espose.cpp
  painter/3essphere.cpp
  painter/3esstar.cpp
  mesh/3esconverter.cpp
  shaders/3esedl.cpp
  shaders/3esedl.frag
  shaders/3esedl.vert
  util/3esresourcelist.cpp
)

list(APPEND PRIVATE_SOURCES
)
