add_executable(3esrec Rec.cpp FrameDisplay.cpp FrameDisplay.h)
set_target_properties(3esrec PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries(3esrec 3escore)
target_include_directories(3esrec PRIVATE $<TARGET_PROPERTY:3escore,INCLUDE_DIRECTORIES>)
target_include_directories(3esrec SYSTEM
  PRIVATE
    # Add cxxopts include directories from the import target. It's a header only library so we don't
    # need to link anything or propagate the dependency.
    $<TARGET_PROPERTY:cxxopts,INTERFACE_INCLUDE_DIRECTORIES>
)

tes_configure_target(3esrec TARGET_COMPONENT Util)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
