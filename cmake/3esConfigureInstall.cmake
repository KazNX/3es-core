# Install MSVC runtime libraries. This will also affect the CPack installation.
include(InstallRequiredSystemLibraries)

if(LINUX)
  return()
endif(LINUX)

# Which targets will we try install per component
# Only installs those actually resolved.

set(FIXUP_TARGETS_Util
  3esinfo
  3esRec
)
set(FIXUP_TARGETS_Viewer
  3rdEyeScene
)

set(INSTALL_COMPONENTS Util)

if(TES_BUILD_VIEWER)
  list(APPEND INSTALL_COMPONENTS Viewer)
endif(TES_BUILD_VIEWER)

# Set up paths to search for dll dependencies
set(FIXUP_SEARCH_PATHS)
if(VCPKG_INSTALLED_DIR)
  # With VCPKG, just use it's install path.
  list(APPEND FIXUP_SEARCH_PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
endif(VCPKG_INSTALLED_DIR)


foreach(COMPONENT ${INSTALL_COMPONENTS})
  set(MARSHAL_MAGNUM_PLUGINS OFF)
  if(COMPONENT STREQUAL Viewer)
    set(MARSHAL_MAGNUM_PLUGINS ON)
  endif(COMPONENT STREQUAL Viewer)

  # Resolve target names.
  unset(FIXUP_APPS)
  foreach(TARGET ${FIXUP_TARGETS_${COMPONENT}})
    message("Checking ${COMPONENT} - ${TARGET}")
    if(NOT TARGET ${TARGET})
      continue()
    endif()
    get_target_property(TARGET_NAME ${TARGET} OUTPUT_NAME)
    if(NOT TARGET_NAME)
      # Target name not overridden.
      set(TARGET_NAME ${TARGET})
    endif(NOT TARGET_NAME)
    list(APPEND FIXUP_APPS "${TARGET_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
  endforeach(TARGET)

  if(NOT FIXUP_APPS)
    # Nothing to install
    continue()
  endif(NOT FIXUP_APPS)

  # Configure the installation script
  configure_file("${CMAKE_CURRENT_LIST_DIR}/3esInstall.in.cmake" "${CMAKE_BINARY_DIR}/3esInstall${COMPONENT}.cmake" @ONLY)
  install(SCRIPT "${CMAKE_BINARY_DIR}/3esInstall${COMPONENT}.cmake" COMPONENT ${COMPONENT})
endforeach(COMPONENT)
