# SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
# SPDX-License-Identifier: MIT

add_subdirectory(config)
add_subdirectory(core)

# Settings/SecureUtils pull in arch's Windows version resource helpers, so
# these tests need Version.lib like the other arch-linked test targets
if(WIN32)
  set(extra_libs version)
endif()

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

create_test(
  NAME ValidatorTests
  DEPENDS gui
  LIBS ${extra_libs}
  SOURCE ValidatorTests.cpp
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src/lib/gui"
)

create_test(
  NAME TlsUtilityTests
  DEPENDS gui
  LIBS ${extra_libs}
  SOURCE TlsUtilityTests.cpp
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
