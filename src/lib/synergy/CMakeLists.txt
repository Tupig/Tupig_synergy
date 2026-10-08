# Synergy -- mouse and keyboard sharing utility
# SPDX-FileCopyrightText: (C) 2024 - 2026 Synergy App Ltd
# SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

add_subdirectory(gui)

# Wire the synergy::hooks::* callsites in the upstream gui library. The
# synergy-gui sublib provides the inline functions in synergy/hooks/gui_hook.h
# that upstream calls behind the SYNERGY_EXTRA_HEADER guard. Setting the define
# + include path on the gui target lets MainWindow.cpp / SettingsDialog.cpp /
# etc. find the header without editing upstream's CMakeLists.
#
# Cascade synergy-gui to anything that links gui (the synergy executable, but
# also unit tests that link libgui.a, which pull in MainWindow.cpp.o which now
# references synergy::hooks::* symbols). Plain signature matches upstream's gui
# CMakeLists.
if(TARGET gui)
  target_compile_definitions(gui PRIVATE SYNERGY_EXTRA_HEADER)
  target_include_directories(gui PRIVATE ${CMAKE_SOURCE_DIR}/src/lib)
  target_link_libraries(gui synergy-gui)
endif()
