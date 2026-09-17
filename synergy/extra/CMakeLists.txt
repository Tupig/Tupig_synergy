# SPDX-FileCopyrightText: (C) 2012 - 2026 Synergy App Ltd
# SPDX-License-Identifier: MIT

# Make synergy/... headers under this tree resolvable from anywhere that descends
# from this directory (and from upstream code once hooks are wired).
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/lib")

add_subdirectory(src)

# Wire the synergy::hooks::* callsites in upstream src/. The synergy-gui sublib
# provides the inline functions in synergy/hooks/gui_hook.h that upstream calls
# behind the SYNERGY_EXTRA_HEADER guard. Setting the define + include path on
# the upstream gui target lets MainWindow.cpp / SettingsDialog.cpp / etc. find
# the header without editing upstream's CMakeLists.
if(TARGET gui)
  target_compile_definitions(gui PRIVATE SYNERGY_EXTRA_HEADER)
  target_include_directories(gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/lib)
  # Cascade synergy-gui to anything that links gui (the synergy executable,
  # but also unit tests that link libgui.a, which pull in MainWindow.cpp.o
  # which now references synergy::hooks::* symbols). Plain signature matches
  # upstream's gui CMakeLists.
  target_link_libraries(gui synergy-gui)
endif()

# Pick the gui app target name (Apple uses the proper-cased brand name).
if(APPLE)
  set(_synergy_gui_app ${CMAKE_PROJECT_PROPER_NAME})
else()
  set(_synergy_gui_app ${CMAKE_PROJECT_NAME})
endif()
if(TARGET ${_synergy_gui_app})
  target_compile_definitions(${_synergy_gui_app} PRIVATE SYNERGY_EXTRA_HEADER)
  target_include_directories(${_synergy_gui_app} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/lib)
endif()
