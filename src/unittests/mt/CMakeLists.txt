# SPDX-FileCopyrightText: (C) 2026 TuPig
# SPDX-License-Identifier: MIT

create_test(
  NAME MutexTests
  DEPENDS mt
  LIBS arch base
  SOURCE MutexTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/mt"
)

create_test(
  NAME CondVarTests
  DEPENDS mt
  LIBS arch base
  SOURCE CondVarTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/mt"
)

create_test(
  NAME ThreadTests
  DEPENDS mt
  LIBS arch base
  SOURCE ThreadTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/mt"
)
