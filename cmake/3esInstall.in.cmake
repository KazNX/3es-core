# Additional installation script. This is run at install time.

function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        if (ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            if (NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()

include(BundleUtilities)

set(FIXUP_APPS @FIXUP_APPS@)
set(FIXUP_SEARCH_PATHS "@FIXUP_SEARCH_PATHS@")

# Handle multi-config generator installation - ninja/VS
if(CMAKE_INSTALL_CONFIG_NAME)
  if(IS_DIRECTORY "${FIXUP_APP_BASE_PATH}/${CMAKE_INSTALL_CONFIG_NAME}")
    set(FIXUP_APP_BASE_PATH "${FIXUP_APP_BASE_PATH}/${CMAKE_INSTALL_CONFIG_NAME}")
  endif(IS_DIRECTORY "${FIXUP_APP_BASE_PATH}/${CMAKE_INSTALL_CONFIG_NAME}")
endif(CMAKE_INSTALL_CONFIG_NAME)

include(BundleUtilities)

# Check the install manifest for the listed FIXUP_APPS names.
foreach(INSTALL_FILE ${CMAKE_INSTALL_MANIFEST_FILES})
  # Breakup the path
  get_filename_component(INSTALL_DIR "${INSTALL_FILE}" DIRECTORY)
  get_filename_component(INSTALL_EXT "${INSTALL_FILE}" LAST_EXT)
  get_filename_component(INSTALL_NAME "${INSTALL_FILE}" NAME)

  # if(IN_LIST) did not work...
  list(FIND FIXUP_APPS "${INSTALL_NAME}" FIND_IDX)
  if(FIND_IDX EQUAL -1)
    continue()
  endif()

  # Fixup the file if it exists.
  if(EXISTS "${INSTALL_FILE}")
    message("fixup ${INSTALL_FILE}")
    fixup_bundle("${INSTALL_FILE}" "" "${FIXUP_SEARCH_PATHS}")
  endif(EXISTS "${INSTALL_FILE}")
endforeach(INSTALL_FILE)

# Install magnum plugins.
set(MARSHAL_MAGNUM_PLUGINS @MARSHAL_MAGNUM_PLUGINS@)
if(MARSHAL_MAGNUM_PLUGINS)
  foreach(SEARCH_DIR ${FIXUP_SEARCH_PATHS})
    if(IS_DIRECTORY "${SEARCH_DIR}/magnum")
      file(COPY "${SEARCH_DIR}/magnum" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
    endif()
  endforeach(SEARCH_DIR)
endif(MARSHAL_MAGNUM_PLUGINS)

dump_cmake_variables()
