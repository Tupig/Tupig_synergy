# SPDX-FileCopyrightText: (C) 2024 - 2026 TuPig
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.24)

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

# Git short SHA
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE GIT_SHA_SHORT
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )
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
set(REQUIRED_QT_VERSION 6.4.0)

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

# MSVC: static CRT handled by vcpkg x64-windows-static triplet (/MT)
if(MSVC)
  message(STATUS "MSVC: static CRT via vcpkg triplet")
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
