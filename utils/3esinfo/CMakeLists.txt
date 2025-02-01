add_executable(3esinfo Info.cpp)
set_target_properties(3esinfo PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries(3esinfo PUBLIC 3escore)
target_include_directories(3esinfo PRIVATE $<TARGET_PROPERTY:3escore,INCLUDE_DIRECTORIES>)

target_include_directories(3esinfo SYSTEM
  PRIVATE
    # Add cxxopts include directories from the import target. It's a header only library so we don't
    # need to link anything or propagate the dependency.
    $<TARGET_PROPERTY:cxxopts,INTERFACE_INCLUDE_DIRECTORIES>
)

install(TARGETS 3esinfo EXPORT 3es-config-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
