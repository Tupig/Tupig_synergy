# SPDX-FileCopyrightText: (C) 2026 TuPig
# SPDX-License-Identifier: MIT

# arch pulls in the Windows version resource helpers (ArchMiscWindows), so
# these tests need the Version.lib import library like the other arch tests
if(WIN32)
  set(extra_libs version)
endif()

create_test(
  NAME MutexTests
  DEPENDS mt
  LIBS arch base ${extra_libs}
  SOURCE MutexTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/mt"
)

create_test(
  NAME CondVarTests
  DEPENDS mt
  LIBS arch base ${extra_libs}
  SOURCE CondVarTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/mt"
)

create_test(
  NAME ThreadTests
  DEPENDS mt
  LIBS arch base ${extra_libs}
  SOURCE ThreadTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/mt"
)
