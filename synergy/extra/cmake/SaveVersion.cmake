# SPDX-FileCopyrightText: (C) 2012 - 2026 Synergy App Ltd
# SPDX-License-Identifier: MIT

# Standalone driver: invoked from CI as
#   cmake -DVERSION_FILE=build/VERSION \
#         -DSYNERGY_VERSION_RELEASE=true|false \
#         -DSYNERGY_VERSION_SNAPSHOT=true|false \
#         -P extra/cmake/SaveVersion.cmake
#
# Includes Version.cmake (the same one Synergy.cmake includes during the build)
# and writes the result to VERSION_FILE. Guarantees the CI-side version matches
# what the binaries report.

include(${CMAKE_CURRENT_LIST_DIR}/Version.cmake)

if(NOT VERSION_FILE)
  message(FATAL_ERROR "VERSION_FILE not set (pass via -DVERSION_FILE=...)")
endif()

get_filename_component(_repo_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
synergy_compute_version("${_repo_root}" _version _tweak _base)
message(STATUS "Computed version: ${_version}")
file(WRITE "${VERSION_FILE}" "${_version}")
