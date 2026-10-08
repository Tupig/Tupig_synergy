# SPDX-FileCopyrightText: (C) 2024 Deskflow Developers
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_subdirectory(arch)
add_subdirectory(base)
add_subdirectory(client)
add_subdirectory(common)
add_subdirectory(synergy)
add_subdirectory(io)
add_subdirectory(mt)
add_subdirectory(net)
add_subdirectory(platform)
add_subdirectory(server)
if(BUILD_GUI)
  add_subdirectory(gui)
  # Synergy GUI overlay wiring: the synergy-gui sublibrary (built by the
  # add_subdirectory(synergy) above, from its gui/ subdirectory) provides
  # synergy/hooks/gui_hook.h behind the SYNERGY_EXTRA_HEADER guard. The gui
  # target only exists from here on, so the define, the src/lib include root
  # and the link edge are attached now.
  target_compile_definitions(gui PRIVATE SYNERGY_EXTRA_HEADER)
  target_include_directories(gui PRIVATE ${CMAKE_SOURCE_DIR}/src/lib)
  target_link_libraries(gui synergy-gui)
endif()
