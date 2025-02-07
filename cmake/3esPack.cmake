set(CPACK_PACKAGE_VENDOR "KazNX")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY 
    "3rd Eye Scene 3D visual debugger.")
set(CPACK_PACKAGE_DESCRIPTION
"3rd Eye Scene is a visual debugger and debugging aid in the vein of rviz or
physics engine viewers such as Havok Visual Debugger or PhysX Visual Debugger.
Whereas those tools are tightly bound to their respective SDKs, 3rd Eye Scene
is a generalised tool, which can be used to remotely visualise and debug any
real time or non real time 3D algorithm. Conceptually, it can be thought of as
a remote rendering application. A 3es server may be embedded into any program,
then 3es render commands are used to instrument the target program. The 3es
viewer client application is then used to view, record and playback these
commands."
)
set(CPACK_PACKAGE_CONTACT "KazNX")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/KazNX/3es-core")
set(CPACK_PACKAGE_VERSION "${TES_VERSION_MAJOR}.${TES_VERSION_MINOR}.${TES_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${TES_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${TES_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${TES_VERSION_PATCH}")

set(CPACK_PACKAGE_INSTALL_DIRECTORY "3escore")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/readme.md")

# set(CPACK_PACKAGE_ICON)
set(CPACK_PACKAGE_EXECUTABLES 3rdEyeScene;3rdEyeScene)

if(LINUX)
  set(CPACK_GENERATOR "DEB")
endif(LINUX)

if(WIN32)
  set(CPACK_GENERATOR "WIX")
endif(WIN32)

# ---- Debian packing
set(CPACK_DEBIAN_PACKAGE_NAME "3es")
set(CPACK_DEB_COMPONENT_INSTALL ON)

set(CPACK_DEBIAN_CORE_PACKAGE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-core")
set(CPACK_DEBIAN_UTIL_PACKAGE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-util")
set(CPACK_DEBIAN_DEVEL_PACKAGE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-core-devel")
set(CPACK_DEBIAN_DOC_PACKAGE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-core-doc")
set(CPACK_DEBIAN_VIEWER_PACKAGE_NAME "${CPACK_DEBIAN_PACKAGE_NAME}-viewer")

set(CPACK_DEBIAN_CORE_FILE_NAME "${CPACK_DEBIAN_CORE_PACKAGE_NAME}.deb")
set(CPACK_DEBIAN_UTIL_FILE_NAME "${CPACK_DEBIAN_UTIL_PACKAGE_NAME}.deb")
set(CPACK_DEBIAN_DEVEL_FILE_NAME "${CPACK_DEBIAN_DEVEL_PACKAGE_NAME}.deb")
set(CPACK_DEBIAN_DOC_FILE_NAME "${CPACK_DEBIAN_DOC_PACKAGE_NAME}.deb")
set(CPACK_DEBIAN_VIEWER_FILE_NAME "${CPACK_DEBIAN_VIEWER_PACKAGE_NAME}.deb")

set(CPACK_DEBIAN_PACKAGE_EPOCH 0)

set(CPACK_DEBIAN_CORE_PACKAGE_DEPENDS zlib1g)
set(CPACK_DEBIAN_UTIL_PACKAGE_DEPENDS ${CPACK_DEBIAN_CORE_PACKAGE_NAME})
set(CPACK_DEBIAN_PACKAGE_DEPENDS ${CPACK_DEBIAN_CORE_PACKAGE_DEPENDS})
set(CPACK_DEBIAN_DEVEL_PACKAGE_DEPENDS zlib1g-dev libglm-dev)
set(CPACK_DEBIAN_VIEWER_PACKAGE_DEPENDS ${CPACK_DEBIAN_CORE_PACKAGE_NAME} zenity)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "KazNX")

set(CPACK_DEBIAN_PACKAGE_CORE_DESCRIPTION "3rd Eye Scene core library.")
set(CPACK_DEBIAN_PACKAGE_UTIL_DESCRIPTION "3rd Eye Scene core utilities.")
set(CPACK_DEBIAN_PACKAGE_DEVEL_DESCRIPTION "3rd Eye Scene core development components.")
set(CPACK_DEBIAN_PACKAGE_DOC_DESCRIPTION "3rd Eye Scene core documentation.")
set(CPACK_DEBIAN_PACKAGE_VIEWER_DESCRIPTION "3rd Eye Scene viewer components.")

set(CPACK_DEBIAN_COMPRESSION_TYPE zstd)

# ---- Windows WIX packaging
set(CPACK_WIX_VERSION 4)
set(CPACK_WIX_UPGRADE_GUID "7de59d88-6afd-48a9-8ba4-71186f97f3e8")
set(CPACK_WIX_PRODUCT_GUID "b47efad3-89f6-41d5-9335-03348bf0bfde")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "${PROJECT_DISPLAY_NAME}")
set(CPACK_WIX_LICENSE_RTF "LICENSE.txt")
# set(CPACK_WIX_PRODUCT_ICON )

include(CPack)
