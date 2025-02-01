add_executable(3esRec Rec.cpp FrameDisplay.cpp FrameDisplay.h)
set_target_properties(3esRec PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries(3esRec 3escore)
target_include_directories(3esRec PRIVATE $<TARGET_PROPERTY:3escore,INCLUDE_DIRECTORIES>)

install(TARGETS 3esRec EXPORT 3es-config-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX source FILES ${SOURCES})
