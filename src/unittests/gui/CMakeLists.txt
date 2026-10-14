# SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

add_subdirectory(config)
add_subdirectory(core)

create_test(
  NAME LoggerTests
  DEPENDS gui
  SOURCE LoggerTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)

create_test(
  NAME KeySequenceTests
  DEPENDS gui
  SOURCE KeySequenceTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)

create_test(
  NAME ScreenSetupModelTests
  DEPENDS gui
  SOURCE ScreenSetupModelTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)

# The migration overlay lives in the synergy-gui sublibrary, which only
# exists when the GUI is built.
if(BUILD_GUI)
  create_test(
    NAME SettingsMigrationTests
    DEPENDS gui
    LIBS synergy-gui
    SOURCE SettingsMigrationTests.cpp
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
  )
  target_include_directories(SettingsMigrationTests PRIVATE ${CMAKE_SOURCE_DIR}/src/lib)
endif()
