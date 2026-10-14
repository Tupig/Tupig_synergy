# SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

if(WIN32)
  set(extra_libs version app mt net)
endif()

create_test(
  NAME ServerConfigTests
  DEPENDS server
  LIBS base arch ${extra_libs}
  SOURCE ServerConfigTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/server"
)

create_test(
  NAME ServerTests
  DEPENDS server
  LIBS base arch ${extra_libs}
  SOURCE ServerTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/server"
)

create_test(
  NAME ConfigReadTests
  DEPENDS server
  LIBS base arch ${extra_libs}
  SOURCE ConfigReadTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/server"
)

create_test(
  NAME InputFilterTests
  DEPENDS server
  LIBS base arch ${extra_libs}
  SOURCE InputFilterTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/server"
)

