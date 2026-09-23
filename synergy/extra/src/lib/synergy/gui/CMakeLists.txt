# Synergy -- mouse and keyboard sharing utility
# Copyright (C) 2024 - 2026 Synergy App Ltd
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

set(target synergy-gui)

set(res_dir ${GUI_RES_DIR})
set(qrc_file ${GUI_QRC_FILE})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB_RECURSE sources *.cpp)
file(GLOB_RECURSE ui_files *.ui)

# Headers are deliberately not listed: CMake does not need them to build a static
# library, and AUTOMOC finds Q_OBJECT headers on its own. An
# `if(ADD_HEADERS_TO_SOURCES)` branch used to guard a header glob here, but nothing
# in the tree ever defined that variable, so the branch could not run and the glob
# was unused.

add_library(${target} STATIC ${sources} ${ui_files} ${qrc_file})

target_link_libraries(
  ${target}
  gui
  Qt6::Core
  Qt6::Widgets
  Qt6::Network)
