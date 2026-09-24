# SPDX-FileCopyrightText: (C) 2024 Deskflow Developers
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_subdirectory(lib)
add_subdirectory(apps)

option(BUILD_TESTS "Build tests" ON)
# Unit tests link Qt::Test and hard-require the Qt6 Test module; a Qt5 fallback
# build (rocky-*, debian-12, ubuntu-24.04, and any leg where system Qt6 is
# below the floor) already aliases Qt6::* onto Qt5::*, so find_package(Qt6 Test)
# then collides with the alias (Qt6CoreTargets.cmake "Targets Defined"). Skip.
if(BUILD_TESTS AND QT_VERSION_MAJOR EQUAL 6)
  add_subdirectory(unittests)
elseif(BUILD_TESTS)
  message(STATUS "BUILD_TESTS=ON but Qt${QT_VERSION_MAJOR} selected; unit tests require Qt6 - skipping")
endif()

