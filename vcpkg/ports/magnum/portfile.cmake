
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mosra/magnum
    # Commit from late August 2023
    REF ec018af93947b3a3c371e0b6909e831aca608747
    SHA512 90d57c072645f54241322f4288ddd9704972d695a339101259de795e7314f47d0e4bc74f37f5ece5bf0efcb25287ff178b06e06b61963af7e1df647e24bbac77
    HEAD_REF master
    PATCHES
        002-sdl-includes.patch
        003-fix-FindGLFW.patch
        004-fix-FindOpenAL.patch
        005-fix-find-sdl2.patch
)

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static" BUILD_STATIC)
string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static" BUILD_PLUGINS_STATIC)

# Remove platform-specific feature that are not available
# on current target platform from all features.

# For documentation on VCPKG_CMAKE_SYSTEM_NAME see
# https://github.com/microsoft/vcpkg/blob/master/docs/users/triplets.md#vcpkg_cmake_system_name

set(ALL_SUPPORTED_FEATURES ${ALL_FEATURES})
# Windows Desktop
if(NOT "${VCPKG_CMAKE_SYSTEM_NAME}" STREQUAL "")
    list(REMOVE_ITEM ALL_SUPPORTED_FEATURES wglcontext windowlesswglapplication)
endif()

# Universal Windows Platform
if(NOT "${VCPKG_CMAKE_SYSTEM_NAME}" STREQUAL "WindowsStore")
    # No UWP specific features
endif()

# Mac OSX
if(NOT "${VCPKG_CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    list(REMOVE_ITEM ALL_SUPPORTED_FEATURES cglcontext windowlesscglapplication)
endif()

# Linux
if(NOT "${VCPKG_CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    list(REMOVE_ITEM ALL_SUPPORTED_FEATURES glxcontext windowlessglxapplication)
endif()

# WebAssembly / Linux
if(NOT "${VCPKG_CMAKE_SYSTEM_NAME}" MATCHES "(Emscripten|Linux)")
    list(REMOVE_ITEM ALL_SUPPORTED_FEATURES eglcontext windowlesseglapplication)
endif()

# Head only features
if(NOT VCPKG_USE_HEAD_VERSION)
    list(REMOVE_ITEM ALL_SUPPORTED_FEATURES anyshaderconverter shadertools shaderconverter
        vk-info)
    message(WARNING "Features anyshaderconverter, shadertools, shaderconverter and vk-info are not avaliable when building non-head version.")
endif()

set(_COMPONENTS "")
# Generate cmake parameters from feature names
foreach(_feature IN LISTS ALL_SUPPORTED_FEATURES)
    # Uppercase the feature name and replace "-" with "_"
    string(TOUPPER "${_feature}" _FEATURE)
    string(REPLACE "-" "_" _FEATURE "${_FEATURE}")

    # Final feature is empty, ignore it
    if(_feature)
        list(APPEND _COMPONENTS ${_feature} MAGNUM_WITH_${_FEATURE})
    endif()
endforeach()

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS FEATURES ${_COMPONENTS})

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        -DMAGNUM_BUILD_STATIC=${BUILD_STATIC}
        -DMAGNUM_BUILD_PLUGINS_STATIC=${BUILD_PLUGINS_STATIC}
        -DMAGNUM_PLUGINS_DEBUG_DIR=${CURRENT_INSTALLED_DIR}/debug/bin/magnum-d
        -DMAGNUM_PLUGINS_RELEASE_DIR=${CURRENT_INSTALLED_DIR}/bin/magnum
        -DMAGNUM_BUILD_DEPRECATED=Off
        -DMAGNUM_WITH_SDL2APPLICATION=Off
)

vcpkg_cmake_install()

vcpkg_copy_pdbs()

# Copy tools into vcpkg's tools directory
set(_TOOL_EXEC_NAMES "")
set(_TOOLS
    al-info
    distancefieldconverter
    fontconverter
    gl-info
    imageconverter
    sceneconverter)
if(VCPKG_USE_HEAD_VERSION)
list(APPEND _TOOLS
    shaderconverter
    vk-info)
endif()
foreach(_tool IN LISTS _TOOLS)
    if("${_tool}" IN_LIST FEATURES)
        list(APPEND _TOOL_EXEC_NAMES magnum-${_tool})
    endif()
endforeach()
message(STATUS ${_TOOL_EXEC_NAMES})
if(_TOOL_EXEC_NAMES)
    vcpkg_copy_tools(TOOL_NAMES ${_TOOL_EXEC_NAMES} AUTO_CLEAN)
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

# Special handling for plugins.
#
# For static plugins, in order to make MSBuild auto-linking magic work, where 
# the linker implicitly takes everything from the root lib/ folder, the 
# static libraries have to be moved out of lib/magnum/blah/ directly to lib/.
# Possibly would be enough to do this just for Windows, doing it also on other
# platforms for consistency.
#
# For dynamic plugins, auto-linking is not desirable as those are meant to be 
# loaded dynamically at runtime instead. In order to prevent that, on Windows 
# the *.lib files corresponding to the plugin *.dlls are removed. However, we 
# cannot remove the *.lib files entirely here, as plugins from magnum-plugins 
# are linked to them on Windows (e.g. AssimpImporter depends on 
# AnyImageImporter). Thus the Any* plugin lib files are kept, but also not 
# moved to the root lib/ folder, to prevent autolinking. A consequence of the 
# *.lib file removal is that downstream projects can't implement Magnum plugins
# that would depend on (and thus link to) these, but that's considered a very 
# rare use case and so it's fine.
#
# See https://github.com/microsoft/vcpkg/pull/1235#issuecomment-308805989 for 
# futher info.
if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/bin")
    # move plugin libs to conventional place
    file(GLOB_RECURSE LIB_TO_MOVE "${CURRENT_PACKAGES_DIR}/lib/magnum/*")
    file(COPY ${LIB_TO_MOVE} DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib/magnum")

    file(GLOB_RECURSE LIB_TO_MOVE_DBG "${CURRENT_PACKAGES_DIR}/debug/lib/magnum/*")
    file(COPY ${LIB_TO_MOVE_DBG} DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/lib/magnum")
else()
    if(WIN32)
        file(GLOB_RECURSE LIB_TO_REMOVE "${CURRENT_PACKAGES_DIR}/lib/magnum/*")
        file(GLOB_RECURSE LIB_TO_KEEP "${CURRENT_PACKAGES_DIR}/lib/magnum/*Any*")
        if(LIB_TO_KEEP)
            list(REMOVE_ITEM LIB_TO_REMOVE ${LIB_TO_KEEP})
        endif()
        if(LIB_TO_REMOVE)
            file(REMOVE ${LIB_TO_REMOVE})
        endif()

        file(GLOB_RECURSE LIB_TO_REMOVE_DBG "${CURRENT_PACKAGES_DIR}/debug/lib/magnum-d/*")
        file(GLOB_RECURSE LIB_TO_KEEP_DBG "${CURRENT_PACKAGES_DIR}/debug/lib/magnum-d/*Any*")
        if(LIB_TO_KEEP_DBG)
            list(REMOVE_ITEM LIB_TO_REMOVE_DBG ${LIB_TO_KEEP_DBG})
        endif()
        if(LIB_TO_REMOVE_DBG)
            file(REMOVE ${LIB_TO_REMOVE_DBG})
        endif()

        # fonts and fontconverters don't have Any* plugins
        file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib/magnum/fonts")
        file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib/magnum/fontconverters")
        file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/lib/magnum-d/fonts")
        file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/lib/magnum-d/fontconverters")
    endif()

    file(COPY "${CMAKE_CURRENT_LIST_DIR}/magnumdeploy.ps1" DESTINATION "${CURRENT_PACKAGES_DIR}/bin/magnum")
    file(COPY "${CMAKE_CURRENT_LIST_DIR}/magnumdeploy.ps1" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin/magnum-d")
endif()

file(INSTALL "${SOURCE_PATH}/COPYING" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
