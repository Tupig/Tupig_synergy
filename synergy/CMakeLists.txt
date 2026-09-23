# SPDX-FileCopyrightText: (C) 2024 - 2026 TuPig
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

# Floor is 3.20 so RHEL/Rocky 8 can configure with its distro cmake (see
# ../.github/actions/install-dependencies); the upper bound only selects policy
# defaults. Do not raise the floor without checking CI's rhel targets.
cmake_minimum_required(VERSION 3.20...3.24)

# Link items by full path
cmake_policy(SET CMP0003 NEW)

# Fix define escaping
cmake_policy(SET CMP0005 NEW)

# De-duplicate libraries on linker calls
if(POLICY CMP0156)
  cmake_policy(SET CMP0156 NEW)
endif()

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project version
include(${CMAKE_CURRENT_SOURCE_DIR}/extra/cmake/Version.cmake)
set(DESKFLOW_VERSION_MAJOR ${SYNERGY_VERSION_MAJOR})
set(DESKFLOW_VERSION_MINOR ${SYNERGY_VERSION_MINOR})
set(DESKFLOW_VERSION_PATCH ${SYNERGY_VERSION_PATCH})
set(DESKFLOW_VERSION_TWEAK 0)

# Git short SHA.
#
# The repository root is the PARENT of this CMakeLists - the project lives in a
# subdirectory of a repository that also holds unrelated content. A bare
# `EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git` test is therefore false in every real
# clone, which silently left GIT_SHA_SHORT empty: VersionInfo.h's kVersionGitSha
# compiled to "", and snapshot builds lost the commit from their display version.
# Ask git itself instead, and treat a failure as "not a checkout" rather than as
# an error.
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_SHA_SHORT
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GIT_SHA_RESULT
  )
  if(NOT GIT_SHA_RESULT EQUAL 0)
    set(GIT_SHA_SHORT "")
  endif()
endif()

# Project definition
project(
  deskflow
  VERSION "${DESKFLOW_VERSION_MAJOR}.${DESKFLOW_VERSION_MINOR}.${DESKFLOW_VERSION_PATCH}.${DESKFLOW_VERSION_TWEAK}"
  DESCRIPTION "TuPig Synergy - Keyboard and mouse sharing utility"
  HOMEPAGE_URL "https://tupig.com"
  LANGUAGES C CXX
)

# Project metadata
set(CMAKE_PROJECT_PROPER_NAME "TuPig Synergy")
set(CMAKE_PROJECT_VENDOR "TuPig")
set(CMAKE_PROJECT_COPYRIGHT "(C) 2024-2026 ${CMAKE_PROJECT_VENDOR}")

set(UPSTREAM_PROJECT_NAME ${CMAKE_PROJECT_NAME})
set(CMAKE_PROJECT_VERSION_BASE "${CMAKE_PROJECT_VERSION}")

# Branding
include(${CMAKE_SOURCE_DIR}/extra/cmake/Synergy.cmake)

unset(DESKFLOW_VERSION_MAJOR)
unset(DESKFLOW_VERSION_MINOR)
unset(DESKFLOW_VERSION_PATCH)
unset(DESKFLOW_VERSION_TWEAK)

message(STATUS "Building ${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}")

# Library versions
set(REQUIRED_OPENSSL_VERSION 3.0)
set(REQUIRED_LIBEI_VERSION 1.0)
set(REQUIRED_LIBPORTAL_VERSION 0.8.0)
set(REQUIRED_QT_VERSION 6.7.0)

# Platform detection
if (WIN32)
  if ("$ENV{VSCMD_ARG_TGT_ARCH}" STREQUAL "")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "[Aa][Rr][Mm]64")
      set(BUILD_ARCHITECTURE arm64)
    else()
      set(BUILD_ARCHITECTURE x64)
    endif()
  else()
    set(BUILD_ARCHITECTURE $ENV{VSCMD_ARG_TGT_ARCH})
  endif()
else()
  set(BUILD_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
endif()

# 64-bit only, on all three platforms.
#
# The project ships 64-bit exclusively: every vcpkg triplet we maintain
# (x64-windows-static, x64-linux, arm64-osx, x64-osx, arm64-windows-static) is
# 64-bit, and no 32-bit dependency set exists. Reject a 32-bit configure here so
# the failure is explicit, instead of surfacing later as confusing ABI or link
# errors against mismatched binaries.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR
    "64-bit target required, but the compiler produces ${CMAKE_SIZEOF_VOID_P}-byte pointers (32-bit). "
    "Configure with a 64-bit toolchain and triplet, e.g. "
    "-A x64 plus -DVCPKG_TARGET_TRIPLET=x64-windows-static on Windows, "
    "-DVCPKG_TARGET_TRIPLET=x64-linux on Linux, arm64-osx/x64-osx on macOS."
  )
endif()

# Keep BUILD_ARCHITECTURE to the 64-bit values the triplets actually provide.
if(NOT BUILD_ARCHITECTURE MATCHES "^(x64|x86_64|arm64|aarch64)$")
  message(FATAL_ERROR
    "Unsupported architecture '${BUILD_ARCHITECTURE}'; this project builds 64-bit only "
    "(x64/x86_64 or arm64/aarch64)."
  )
endif()

# AddressSanitizer. MSVC's ASan runtime (clang_rt.asan_dynamic-*.dll) cannot link
# against a static CRT, so an ASan build needs a dynamic-CRT triplet - see the
# runtime block below, which enforces that.
option(SYNERGY_ENABLE_ASAN "Build with AddressSanitizer (requires a dynamic-CRT triplet)" OFF)

# MSVC: the CRT must match the vcpkg triplet's linkage (/MT with a static
# triplet's dependencies, /MD with a dynamic one) or the link fails with
# LNK2038 "RuntimeLibrary mismatch". It is not enough to leave the choice to
# vcpkg: its buildsystems/vcpkg.cmake only includes the platform toolchain when
# VCPKG_CHAINLOAD_TOOLCHAIN_FILE is set (scripts/buildsystems/vcpkg.cmake:209)
# and never defaults that variable, so the Windows toolchain - the file that
# would derive CMAKE_MSVC_RUNTIME_LIBRARY from VCPKG_CRT_LINKAGE - does not run
# on a plain Windows configure. Measured: with a static triplet and no override
# here, CMake's own default (/MD) is used and the build cannot link.
#
# So the linkage is resolved here from the triplet, preferring the most
# authoritative source available, and the outcome is reported so a surprise is
# visible in the configure log. The ASan option below depends on this block.
if(MSVC)
  set(_vcpkg_crt_linkage "${VCPKG_CRT_LINKAGE}")
  set(_crt_linkage_source "VCPKG_CRT_LINKAGE")

  if(NOT _vcpkg_crt_linkage)
    # Look for the triplet file: overlay directories first (that is vcpkg's own
    # search order), then the checkout's builtin triplets.
    set(_crt_triplet_dirs ${VCPKG_OVERLAY_TRIPLETS})
    if(CMAKE_TOOLCHAIN_FILE)
      get_filename_component(_vcpkg_root "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)   # .../scripts/buildsystems
      get_filename_component(_vcpkg_root "${_vcpkg_root}" DIRECTORY)            # .../scripts
      get_filename_component(_vcpkg_root "${_vcpkg_root}" DIRECTORY)            # .../<vcpkg>
      list(APPEND _crt_triplet_dirs "${_vcpkg_root}/triplets")
    endif()

    foreach(_dir IN LISTS _crt_triplet_dirs)
      set(_triplet_file "${_dir}/${VCPKG_TARGET_TRIPLET}.cmake")
      if(EXISTS "${_triplet_file}")
        file(STRINGS "${_triplet_file}" _crt_setting REGEX "VCPKG_CRT_LINKAGE")
        string(REGEX MATCH "(static|dynamic)" _vcpkg_crt_linkage "${_crt_setting}")
        if(_vcpkg_crt_linkage)
          set(_crt_linkage_source "${_triplet_file}")
          break()
        endif()
      endif()
    endforeach()
  endif()

  if(NOT _vcpkg_crt_linkage)
    # Last resort: vcpkg's naming convention. Its "-static" suffix means both
    # static libraries and a static CRT, which is what every triplet in
    # triplets/overlay uses; anything else is dynamic.
    if(VCPKG_TARGET_TRIPLET MATCHES "-static$")
      set(_vcpkg_crt_linkage "static")
    else()
      set(_vcpkg_crt_linkage "dynamic")
    endif()
    set(_crt_linkage_source "the '-static' triplet naming convention")
  endif()

  if(_vcpkg_crt_linkage STREQUAL "static")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(_crt_form "/MT")
  else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    set(_crt_form "/MD")
  endif()
  message(STATUS "MSVC runtime: ${_crt_form} (${_vcpkg_crt_linkage} CRT, from ${_crt_linkage_source})")

  # ASan genuinely constrains the CRT: clang_rt.asan_dynamic-*.dll cannot link
  # against a static CRT. Now that the linkage is known, reject the impossible
  # combination here instead of leaving a puzzling link error.
  if(SYNERGY_ENABLE_ASAN AND _vcpkg_crt_linkage STREQUAL "static")
    message(FATAL_ERROR
      "MSVC AddressSanitizer requires a dynamic-CRT triplet, but triplet "
      "'${VCPKG_TARGET_TRIPLET}' uses a static CRT (/MT). Configure with a dynamic "
      "triplet - the windows-msvc-asan preset uses x64-windows."
    )
  endif()

  # ArchMiscWindows::guardRuntimeVersion() compares the running CRT against
  # REQUIRED_MSVC_RUNTIME_*, and Constants.h.in substitutes those into
  # kWindowsRuntimeMajor/Minor. Leaving them unset yields an empty initializer
  # and a hard compile error, so bind them here from the compiler toolset
  # (cl major 19 maps to CRT major 14; the minor matches, e.g. 19.44 -> 14.44).
  set(REQUIRED_MSVC_RUNTIME_MAJOR 14)
  if(CMAKE_CXX_COMPILER_VERSION MATCHES "^[0-9]+\\.([0-9]+)")
    set(REQUIRED_MSVC_RUNTIME_MINOR "${CMAKE_MATCH_1}")
  endif()
  message(STATUS "MSVC runtime version: ${REQUIRED_MSVC_RUNTIME_MAJOR}.${REQUIRED_MSVC_RUNTIME_MINOR}")
endif()

# ---------------------------------------------------------------------------
# Optimization and hardening
# ---------------------------------------------------------------------------
# Assembled here so the flags apply tree-wide; project() has already run, which
# check_ipo_supported() requires.

include(CheckIPOSupported)

option(SYNERGY_ENABLE_LTO "Enable link-time optimization (IPO/LTO) in release builds" ON)
option(SYNERGY_ENABLE_AVX2 "Target AVX2 on x86-64; requires Haswell (2013) or newer" ON)
option(SYNERGY_ENABLE_HARDENING "Enable control-flow guard and extra security checks" ON)

# ASan and LTO instrument/transform at different stages and do not compose
# reliably, so an ASan build forces LTO off rather than producing misleading
# reports.
if(SYNERGY_ENABLE_ASAN)
  set(SYNERGY_ENABLE_LTO OFF)
  message(STATUS "ASan enabled: link-time optimization disabled")
endif()

# x86-64 (as opposed to arm64): both are 64-bit, but only x86-64 has AVX2.
set(_synergy_arch_x86_64 FALSE)
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "[Aa][Rr][Mm]64")
  set(_synergy_arch_x86_64 TRUE)
endif()

if(SYNERGY_ENABLE_LTO)
  check_ipo_supported(RESULT _synergy_ipo_ok OUTPUT _synergy_ipo_msg LANGUAGES C CXX)
  if(_synergy_ipo_ok)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    message(STATUS "Link-time optimization enabled for release builds")
  else()
    message(WARNING "Link-time optimization unsupported by this toolchain, leaving it off: ${_synergy_ipo_msg}")
  endif()
endif()

if(MSVC)
  # /Gw (whole-program data) and /Gy (function-level COMDATs) are what give LTO
  # something to work with; without them the link-time pass has little to inline
  # or discard.
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Gw /Gy")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Gw /Gy")

  if(SYNERGY_ENABLE_AVX2)
    if(_synergy_arch_x86_64)
      set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /arch:AVX2")
      set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX2")
      message(STATUS "AVX2 code generation enabled (/arch:AVX2); needs Haswell (2013) or newer")
    else()
      message(STATUS "AVX2 requested but the target is not x86-64; /arch:AVX2 not applied")
    endif()
  endif()

  if(SYNERGY_ENABLE_HARDENING)
    # /guard:cf instruments indirect calls and validates them at runtime, which
    # blunts control-flow hijacking after a memory-corruption bug; it needs the
    # matching linker flag. /sdl adds extra security checks and promotes some
    # security-relevant warnings to errors. /GS (stack cookies) is on by default.
    add_compile_options(/guard:cf /sdl)
    add_link_options(/guard:cf)
    message(STATUS "Hardening enabled: /guard:cf /sdl")
  endif()

  if(SYNERGY_ENABLE_ASAN)
    # MSVC's AddressSanitizer instruments heap/stack/globals of this code;
    # dependencies need not be instrumented, but they must share the dynamic CRT.
    # /fsanitize=address is a compiler option only: cl records the request in the
    # object files and link.exe then pulls in the ASan runtime on its own
    # (/INFERASANLIBS). Passing it to the linker is not just unnecessary, link.exe
    # reports LNK4044 "unrecognized option" and ignores it.
    add_compile_options(/fsanitize=address /Zi)
    message(STATUS "Hardening enabled: /fsanitize=address")
  endif()
else()
  # Toolchain hardening that mainstream distributions ship by default.
  add_compile_options(-fstack-protector-strong)
  if(SYNERGY_ENABLE_HARDENING)
    add_link_options(-Wl,-z,relro,-z,now)
  endif()
  if(NOT APPLE)
    add_compile_options(-fPIE)
    add_link_options(-pie)
    if(SYNERGY_ENABLE_AVX2 AND _synergy_arch_x86_64)
      add_compile_options(-march=x86-64-v3)
      message(STATUS "AVX2-class code generation enabled (-march=x86-64-v3)")
    endif()
    # _FORTIFY_SOURCE needs optimization and is incompatible with sanitizers, so
    # it is scoped to release builds and omitted when ASan is on.
    if(NOT SYNERGY_ENABLE_ASAN)
      add_compile_options("$<$<CONFIG:Release>:-D_FORTIFY_SOURCE=2>")
    endif()
  endif()

  if(SYNERGY_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
    message(STATUS "Hardening enabled: -fsanitize=address")
  endif()
endif()

# Debug visibility
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Disabling debug build")
  add_definitions(-DNDEBUG)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

# Disable Qt keywords (conflict with glib)
add_definitions(-DQT_NO_KEYWORDS)

# Platform options
if(UNIX AND NOT APPLE)
  option(BUILD_X11_SUPPORT "Build with x11 support" ON)
elseif(APPLE)
  option(BUILD_OSX_BUNDLE "Build mac os bundle" ON)
endif()

# Libraries (vcpkg handles find_package)
include(cmake/Libraries.cmake)
configure_libs()

if(BUILD_OSX_BUNDLE AND APPLE_CODESIGN_DEV)
  include(cmake/MacCodesign.cmake)
endif()

# Install paths
include(GNUInstallDirs)
if(WIN32)
  set(CMAKE_INSTALL_BINDIR .)
  set(CMAKE_INSTALL_LIBDIR .)
  set(CMAKE_INSTALL_DOCDIR docs)
  set(CMAKE_INSTALL_LICENSE_DIR .)
  set(CMAKE_INSTALL_I18N_DIR translations)
elseif(BUILD_OSX_BUNDLE)
  set(CMAKE_INSTALL_BINDIR ${CMAKE_PROJECT_PROPER_NAME}.app/Contents/MacOS)
  set(CMAKE_INSTALL_LICENSE_DIR ${CMAKE_PROJECT_PROPER_NAME}.app/Contents/Resources)
  set(CMAKE_INSTALL_DOCDIR ${CMAKE_PROJECT_PROPER_NAME}.app/Contents/Resources)
else()
  set(CMAKE_INSTALL_LICENSE_DIR ${CMAKE_INSTALL_DATADIR}/licenses/${CMAKE_PROJECT_NAME})
  set(CMAKE_INSTALL_I18N_DIR ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/translations)
endif()

option(BUILD_GUI "Build GUI" ON)

if(BUILD_GUI AND NOT QT_VERSION_MAJOR EQUAL 5)
  add_subdirectory(translations)
endif()

add_subdirectory(src)
add_subdirectory(extra)

# Deploy the generated translation catalogues next to the executables.
#
# I18N looks for them in <applicationDirPath>/translations first. That holds for
# an installed tree (CMAKE_INSTALL_I18N_DIR puts them beside the binaries), but
# not for a plain build tree, where the catalogues live in
# <build>/translations while the binaries are in <build>/bin/<config>. Without
# this copy a freshly built binary reports "initial language: en" even on a
# Chinese system.
#
# Deliberately an always-run ALL target rather than a POST_BUILD step: POST_BUILD
# only fires when its target relinks, so the catalogues would silently go stale
# (or be missing entirely) on incremental builds.
if(BUILD_GUI AND TARGET synergy AND SYNERGY_TRANSLATION_FILES)
  add_custom_target(deploy_translations ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
      "$<TARGET_FILE_DIR:synergy>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy
      ${SYNERGY_TRANSLATION_FILES}
      "$<TARGET_FILE_DIR:synergy>/translations"
    DEPENDS synergy app_translations
    COMMENT "Deploying translations next to the executables"
    VERBATIM
  )
endif()

option(BUILD_INSTALLER "Build installer" ON)
if(BUILD_INSTALLER)
  add_subdirectory(deploy)
endif()

install(
  FILES ${PROJECT_SOURCE_DIR}/LICENSE
  DESTINATION ${CMAKE_INSTALL_LICENSE_DIR}
)

install(
  FILES ${PROJECT_SOURCE_DIR}/LICENSES/LicenseRef-OpenSSL-Exception.txt
  DESTINATION ${CMAKE_INSTALL_LICENSE_DIR}
  RENAME LICENSE_EXCEPTION
)
