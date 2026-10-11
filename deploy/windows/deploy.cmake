# SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <https://github.com/Tupig/Tupig_synergy/issues>
# SPDX-License-Identifier: MIT

# HACK This is set when the files is included so its the real path
# calling CMAKE_CURRENT_LIST_DIR after include would return the wrong scope var
set(MY_DIR ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Only a dynamically linked CRT needs the VC++ redistributable copied next to
# the binaries. This build uses the static CRT (/MT) via the vcpkg
# x64-windows-static triplet, and the produced executables have no dependency on
# msvcp140/vcruntime140/concrt140 at all (verified with dumpbin /DEPENDENTS).
# Installing them anyway only bloats the portable package with unused files and
# contradicts the "no external runtime required" goal, so skip it for /MT.
if(CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL")
  include(InstallRequiredSystemLibraries)
else()
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS "")
  message(STATUS "Static CRT in use; not bundling the VC++ redistributable")
endif()

configure_file(${MY_DIR}/pre-cpack.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/pre-cpack.cmake @ONLY)
set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/pre-cpack.cmake)

configure_file(${MY_DIR}/cpack-options.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/cpack-options.cmake @ONLY)
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/cpack-options.cmake)

set(OS_STRING "win-${BUILD_ARCHITECTURE}")

list(APPEND CPACK_GENERATOR "7Z")

# If WiX 4+ is installed make a package. CPack drives the WiX v4 toolchain
# schema; CI installs wix 5.0.2 which still accepts v4 documents. Keep this
# in step with .github/actions/install-dependencies/action.yml (A-23 (#47)).
find_program(WIX_APP wix)
if (NOT "${WIX_APP}" STREQUAL "")
  set(CPACK_WIX_VERSION 4)
  set(CPACK_WIX_ARCHITECTURE ${BUILD_ARCHITECTURE})
  list(APPEND CPACK_GENERATOR "WIX")
endif()

set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_PROPER_NAME}")

# Menu Entry
set(CPACK_WIX_PROGRAM_MENU_FOLDER "${CMAKE_PROJECT_PROPER_NAME}")
set(CPACK_PACKAGE_EXECUTABLES "${SYNERGY_GUI_EXE_NAME}" "${CMAKE_PROJECT_PROPER_NAME}")

# Default Install Path
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_PROPER_NAME}")

# Wix Specific Values
set(CPACK_WIX_UPGRADE_GUID "027D1C8A-E7A5-4754-BB93-B2D45BFDBDC8")
set(CPACK_WIX_UI_BANNER "${CMAKE_SOURCE_DIR}/deploy/windows/wix-banner.png")
set(CPACK_WIX_UI_DIALOG "${CMAKE_SOURCE_DIR}/deploy/windows/wix-dialog.png")

# Required Extra Extenstions
list(APPEND CPACK_WIX_EXTENSIONS "WixToolset.Util.wixext" "WixToolset.Firewall.wixext")

# Make sure to also put the xmlns for the ext into the wix block on generated files
list(APPEND CPACK_WIX_CUSTOM_XMLNS "util=http://wixtoolset.org/schemas/v4/wxs/util" "firewall=http://wixtoolset.org/schemas/v4/wxs/firewall")

# Bundle the MSVC C++ runtime (CRT) as a merge module inside the MSI so that end
# users don't have to download and install the Visual C++ redistributable
# themselves. The merge module ships with the Visual Studio build environment.
# Docs: https://learn.microsoft.com/en-us/cpp/windows/redistributing-components-by-using-merge-modules
# VS 2022 names the file Microsoft_VC143_CRT_<arch>.msm and keeps it under
# Redist/MSVC/v143/MergeModules. VS 2026 uses VC145 and may use a different
# case for the architecture (ARM64 vs arm64). Search both the VS redist root
# and VCToolsRedistDir, then match the filename case-insensitively.
set(_crt_msm_arch "${BUILD_ARCHITECTURE}")
if(_crt_msm_arch STREQUAL "x86_64" OR _crt_msm_arch STREQUAL "amd64")
  set(_crt_msm_arch "x64")
elseif(_crt_msm_arch STREQUAL "aarch64")
  set(_crt_msm_arch "arm64")
endif()
string(TOLOWER "${_crt_msm_arch}" _crt_msm_arch)

set(_crt_msm_roots "")
if(NOT "$ENV{VCINSTALLDIR}" STREQUAL "")
  list(APPEND _crt_msm_roots "$ENV{VCINSTALLDIR}Redist/MSVC")
endif()
if(NOT "$ENV{VCToolsRedistDir}" STREQUAL "")
  list(APPEND _crt_msm_roots "$ENV{VCToolsRedistDir}")
endif()

set(_crt_msm_candidates "")
foreach(_crt_msm_root IN LISTS _crt_msm_roots)
  file(GLOB_RECURSE _crt_msm_here "${_crt_msm_root}/*.msm")
  list(APPEND _crt_msm_candidates ${_crt_msm_here})
endforeach()

set(REDIST_MERGE_MODULE_PATHS "")
foreach(_crt_msm IN LISTS _crt_msm_candidates)
  cmake_path(GET _crt_msm FILENAME _crt_msm_name)
  string(TOLOWER "${_crt_msm_name}" _crt_msm_name)
  if(_crt_msm_name MATCHES "^microsoft_vc.+_crt_${_crt_msm_arch}\\.msm$")
    list(APPEND REDIST_MERGE_MODULE_PATHS "${_crt_msm}")
  endif()
endforeach()
if(REDIST_MERGE_MODULE_PATHS)
  list(REMOVE_DUPLICATES REDIST_MERGE_MODULE_PATHS)
endif()

if (REDIST_MERGE_MODULE_PATHS)
  # Multiple toolset versions may be installed; pick the newest.
  list(SORT REDIST_MERGE_MODULE_PATHS)
  list(GET REDIST_MERGE_MODULE_PATHS -1 REDIST_MERGE_MODULE_PATH)
  message(STATUS "MSVC CRT merge module: ${REDIST_MERGE_MODULE_PATH}")
  # XML snippets injected into wix-patch.xml.in to merge the CRT into the MSI.
  set(WIX_REDIST_MERGE
    "<DirectoryRef Id=\"TARGETDIR\"><Merge Id=\"VC_Redist\" SourceFile=\"${REDIST_MERGE_MODULE_PATH}\" DiskId=\"1\" Language=\"0\"/></DirectoryRef>")
  set(WIX_REDIST_MERGE_REF "<MergeRef Id=\"VC_Redist\"/>")
elseif(NOT CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL")
  # /MT already links the runtime into the executables. A missing merge module
  # is not a defect, and the build must not warn about it.
  message(STATUS "Static CRT in use; the installer does not merge the VC++ redistributable")
elseif(SYNERGY_VERSION_RELEASE OR SYNERGY_VERSION_SNAPSHOT)
  # The Windows ARM image ships the VC++ DLLs without the merge module. Put
  # those DLLs next to the binaries so the MSI and the portable archive still
  # run. Prefer the desktop redist over the Spectre and debug copies.
  set(_crt_dlls "")
  if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
    set(_crt_dlls "${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}")
  else()
    set(_crt_dll_found "")
    foreach(_crt_msm_root IN LISTS _crt_msm_roots)
      file(GLOB_RECURSE _crt_dll_here "${_crt_msm_root}/*.dll")
      list(APPEND _crt_dll_found ${_crt_dll_here})
    endforeach()
    foreach(_crt_dll IN LISTS _crt_dll_found)
      cmake_path(GET _crt_dll FILENAME _crt_dll_name)
      string(TOLOWER "${_crt_dll_name}" _crt_dll_name)
      string(TOLOWER "${_crt_dll}" _crt_dll_path)
      if(_crt_dll_name MATCHES "^(vcruntime140.*|msvcp140.*|concrt140.*)\\.dll$"
          AND NOT _crt_dll_path MATCHES "debug"
          AND NOT _crt_dll_path MATCHES "spectre"
          AND _crt_dll_path MATCHES "[/\\\\]${_crt_msm_arch}[/\\\\]")
        list(APPEND _crt_dlls "${_crt_dll}")
      endif()
    endforeach()
    if(_crt_dlls)
      list(REMOVE_DUPLICATES _crt_dlls)
    endif()
  endif()
  if(_crt_dlls)
    install(FILES ${_crt_dlls} DESTINATION ${CMAKE_INSTALL_BINDIR})
    message(STATUS "MSVC CRT merge module not found; installing the VC++ runtime DLLs beside the binaries")
  else()
    message(FATAL_ERROR
      "MSVC CRT merge module and runtime DLLs were not found under VCINSTALLDIR or VCToolsRedistDir; "
      "the installer will NOT bundle the Visual C++ runtime")
  endif()
else()
  # Dev builds routinely lack the redist merge module; not bundling it is expected.
  message(STATUS
    "MSVC CRT merge module not found; installer will not bundle the Visual C++ "
    "runtime (expected for dev builds)")
endif()

# The patch needs the full path of the CRT merge module (see above).
configure_file(
  ${MY_DIR}/wix-patch.xml.in
  ${CMAKE_CURRENT_BINARY_DIR}/wix-patch.xml @ONLY
)

# This patch sets up firewall rules, the service and the CRT merge module.
set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/wix-patch.xml")
