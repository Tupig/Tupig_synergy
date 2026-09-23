# SPDX-FileCopyrightText: (C) 2012 - 2026 TuPig
# SPDX-License-Identifier: MIT

# Must be included after deskflow's project() call and the CMAKE_PROJECT_*
# defaults are set, so these overrides take effect.
set(CMAKE_PROJECT_PROPER_NAME "TuPig Synergy")
set(CMAKE_PROJECT_VENDOR "TuPig")
set(CMAKE_PROJECT_COPYRIGHT "(C) 2012-2026 ${CMAKE_PROJECT_VENDOR}")
set(CMAKE_PROJECT_CONTACT "${CMAKE_PROJECT_PROPER_NAME} <support@tupig.com>")
set(CMAKE_PROJECT_REV_FQDN "com.tupig.synergy")
set(CMAKE_PROJECT_DOMAIN "tupig.com")
set(CMAKE_PROJECT_HOMEPAGE_URL "https://tupig.com")

# Display brand. Window title / About use SYNERGY_DISPLAY_NAME.
# Default matches CMAKE_PROJECT_PROPER_NAME ("TuPig Synergy") so the UI and
# Linux desktop entry agree (U-17). Core flavor keeps a distinct headless label.
# Paths stay on CMAKE_PROJECT_PROPER_NAME (U-09 by design: dirs with spaces,
# file names use kAppId "synergy").
option(SYNERGY_CORE_FLAVOR "Build as TuPig Synergy Core" OFF)
if(SYNERGY_CORE_FLAVOR)
  set(SYNERGY_DISPLAY_NAME "TuPig Synergy Core")
else()
  set(SYNERGY_DISPLAY_NAME "TuPig Synergy")
endif()
add_compile_definitions(SYNERGY_DISPLAY_NAME="${SYNERGY_DISPLAY_NAME}")

# Single source of truth for the minimum macOS version. Synergy is long-term
# stable (unlike upstream, which tracks recent macOS), so we target the oldest
# macOS the linked Qt 6.x supports, the same value for every architecture.
# CMake uses this for the build; the packaged .app inherits it via the Xcode/
# Ninja generators. CI must NOT pass -DCMAKE_OSX_DEPLOYMENT_TARGET.
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "12")
endif()

# Core flavor seeds headless-build defaults (no GUI, no tests, no installer).
# No `FORCE` on the cache writes: the seed only fills empty slots, so a user
# passing -DBUILD_GUI=ON alongside the flavor flag still wins.
if(SYNERGY_CORE_FLAVOR)
  set(BUILD_GUI OFF CACHE BOOL "Build GUI")
  set(BUILD_TESTS OFF CACHE BOOL "Build tests")
  set(BUILD_INSTALLER OFF CACHE BOOL "Build installer")
  # On macOS the core normally nests into the GUI .app bundle
  # ($<TARGET_BUNDLE_CONTENT_DIR:Synergy>), but with BUILD_GUI=OFF that target
  # doesn't exist and cmake generation fails. Headless builds ship the core
  # binary directly, so disable the bundle too.
  set(BUILD_OSX_BUNDLE OFF CACHE BOOL "Build mac os bundle")
endif()

# Don't run unit tests as part of the build. Devs can opt back in with
# -DSKIP_BUILD_TESTS=OFF if they want post-build ctest invocation.
set(SKIP_BUILD_TESTS ON CACHE BOOL "Skip build time test")

# Resource paths consumed by extra/src/lib/synergy/gui/CMakeLists.txt.
set(GUI_RES_DIR "${CMAKE_SOURCE_DIR}/extra/src/apps/res")
set(GUI_QRC_FILE "${GUI_RES_DIR}/synergy.qrc")

# Override deskflow's project name. This cascades into binary names
# (${CMAKE_PROJECT_NAME}-core, etc.), install paths, package names,
# translation file naming, and CPack metadata. Source files in src/apps/*/
# are patched to use literal filenames since they previously assumed
# target name == source basename.
set(CMAKE_PROJECT_NAME synergy)

# Synergy version. Base semver and composition rules — dev/snapshot/release
# suffix, rev count — live in extra/cmake/Version.cmake via synergy_compute_version()
# so the CI-side version (package filenames, S3 paths, etc.) matches what the
# binaries report. Default mode is dev; flip with -DSYNERGY_VERSION_RELEASE=ON or
# -DSYNERGY_VERSION_SNAPSHOT=ON for CI/release builds.
option(SYNERGY_VERSION_RELEASE "Release version" OFF)
option(SYNERGY_VERSION_SNAPSHOT "Snapshot version" OFF)

include(${CMAKE_CURRENT_LIST_DIR}/Version.cmake)
synergy_compute_version("${CMAKE_SOURCE_DIR}"
  CMAKE_PROJECT_VERSION
  CMAKE_PROJECT_VERSION_TWEAK
  CMAKE_PROJECT_VERSION_BASE
)
set(CMAKE_PROJECT_VERSION_MAJOR ${SYNERGY_VERSION_MAJOR})
set(CMAKE_PROJECT_VERSION_MINOR ${SYNERGY_VERSION_MINOR})
set(CMAKE_PROJECT_VERSION_PATCH ${SYNERGY_VERSION_PATCH})

# Human-facing version. The composed version already carries its build metadata
# (dev: +<sha>, snapshot: +rN), so it is the display string as-is. Snapshot is the
# exception: +rN is a rev count, not the commit, so append the short sha for
# traceability. Dev already embeds the sha (don't double it); release stays clean.
# Consumed by VersionInfo.h.in (kDisplayVersion).
if(SYNERGY_VERSION_SNAPSHOT AND GIT_SHA_SHORT)
  set(CMAKE_PROJECT_VERSION_DISPLAY "${CMAKE_PROJECT_VERSION} (${GIT_SHA_SHORT})")
else()
  set(CMAKE_PROJECT_VERSION_DISPLAY "${CMAKE_PROJECT_VERSION}")
endif()

if(NOT SYNERGY_VERSION_RELEASE AND NOT SYNERGY_VERSION_SNAPSHOT)
  add_compile_definitions(SYNERGY_VERSION_DEV)
endif()

# Compile activation in for distributable builds only; dev builds opt in at
# runtime via Synergy.test.conf (licensing=true) so local iteration isn't gated
# on a serial key.
if(SYNERGY_VERSION_RELEASE OR SYNERGY_VERSION_SNAPSHOT)
  add_compile_definitions(SYNERGY_ENABLE_ACTIVATION)
endif()

# Function to set the output name of an executable.
# Usage: set_output_name(<target> [SUFFIX <suffix>])
# Example: set_output_name(synergy-core SUFFIX "-core")   -> synergy-core(.exe)
#
# The name deliberately carries NO version number. Versioned binaries
# (synergy-core-1.21.2.exe) broke several consumers that identify these files by
# their exact, unversioned names:
#   * Constants.h.in sets kCoreBinName/kDaemonBinName to "synergy-core"/"synergy-daemon",
#     which the GUI and daemon use to locate and launch the core, and which
#     MSWindowsWatchdog matches against running processes;
#   * extra/deploy/linux/com.tupig.synergy.desktop declares Exec=synergy;
#   * deploy/windows/wix-patch.xml.in keys WiX fragments on the component IDs
#     derived from the installed file names, so packaging failed outright;
#   * CI probes and signs build/bin/synergy-core.exe.
# The version is not lost: it is reported by --version, embedded in the binary's
# version resource, and still included in package file names.
function(set_output_name TARGET)
  cmake_parse_arguments(ARG "" "SUFFIX" "" ${ARGN})

  set(_output_name "${CMAKE_PROJECT_NAME}${ARG_SUFFIX}")

  set_target_properties(${TARGET} PROPERTIES
    OUTPUT_NAME ${_output_name}
  )

  message(STATUS "Set output name for ${TARGET}: ${_output_name}")
endfunction()
