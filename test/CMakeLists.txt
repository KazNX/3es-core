find_package(GTest QUIET)

add_subdirectory(3estBandwidth)
add_subdirectory(3estPrimitiveServer)
add_subdirectory(3estServer)
add_subdirectory(3estTessellate)

set_target_properties(3estBandwidth PROPERTIES FOLDER test)
set_target_properties(3estPrimitiveServer PROPERTIES FOLDER test)
set_target_properties(3estServer PROPERTIES FOLDER test)
set_target_properties(3estTessellate PROPERTIES FOLDER test)

# Add unit tests
if(GTEST_FOUND)
  add_subdirectory(3estUnit)
  set_target_properties(3estUnit PROPERTIES FOLDER test)

  if(TES_BUILD_VIEWER)
    add_subdirectory(3estViewer)
    set_target_properties(3estViewer PROPERTIES FOLDER test)
  endif(TES_BUILD_VIEWER)
else(GTEST_FOUND)
  message("Missing GTest. Unit tests will not be built.")
endif(GTEST_FOUND)
