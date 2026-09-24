# SPDX-FileCopyrightText: (C) 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: (C) 2024 Symless Ltd
# SPDX-License-Identifier: MIT

# CMake target names must not contain spaces; "TuPig Synergy" is rejected for
# add_executable and breaks every TARGET_BUNDLE_* generator expression. Keep the
# brand only in display/bundle metadata (BUNDLE_* / OUTPUT_NAME), not the target.
set(target ${CMAKE_PROJECT_NAME})
set(filename deskflow-gui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
  set(EXE_DESCRIPTION "${CMAKE_PROJECT_PROPER_NAME} GUI configuration tool")
  set(EXE_ICON "IDI_ICON1 ICON DISCARDABLE \"${CMAKE_SOURCE_DIR}/extra/src/apps/res/synergy.ico\" ")
  configure_file(${CMAKE_SOURCE_DIR}/src/apps/res/windows.rc.in deskflow.rc)
  set(platform_extra deskflow.rc)
elseif(BUILD_OSX_BUNDLE)
  # Bundle metadata carries the spaced brand name; the CMake target stays
  # ${CMAKE_PROJECT_NAME} (no spaces). CFBundleExecutable must match the
  # OUTPUT_NAME the bundle actually embeds.
  set(BUNDLE_EXECUTABLE_NAME "${CMAKE_PROJECT_PROPER_NAME}")
  set(BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_PROPER_NAME}")
  set(BUNDLE_DISPLAY_NAME "${CMAKE_PROJECT_PROPER_NAME}")
  set(BUNDLE_GUI_IDENTIFIER "${CMAKE_PROJECT_REV_FQDN}")
  set(BUNDLE_ICON_FILE "${CMAKE_PROJECT_PROPER_NAME}.icns")
  set(BUNDLE_INFO_STRING "${CMAKE_PROJECT_DESCRIPTION}")
  set(BUNDLE_COPYRIGHT "${CMAKE_PROJECT_COPYRIGHT}")

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../res/deskflow.plist.in
    ${CMAKE_CURRENT_BINARY_DIR}/deskflow.plist
    @ONLY
  )

  set(platform_extra ${CMAKE_SOURCE_DIR}/extra/deploy/mac/bundle/Contents/Resources/${CMAKE_PROJECT_PROPER_NAME}.icns)
  set_source_files_properties(${platform_extra} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

add_executable(${target} WIN32 MACOSX_BUNDLE
  ${platform_extra}
  ${CMAKE_SOURCE_DIR}/extra/src/apps/res/synergy.qrc
  ${filename}.cpp
)

target_link_libraries(
  ${target}
  gui
  common
  Qt6::Core
  Qt6::Widgets
  Qt6::Network)

# Set output name with version
include(${CMAKE_SOURCE_DIR}/extra/cmake/Synergy.cmake)
set_output_name(${target})
# On Apple the .app folder must be named "TuPig Synergy.app" (install/CPack
# paths use CMAKE_PROJECT_PROPER_NAME); override set_output_name's synergy.
if(APPLE AND BUILD_OSX_BUNDLE)
  set_target_properties(${target} PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_PROPER_NAME}")
endif()

install(
  TARGETS ${target}
  RUNTIME_DEPENDENCY_SET guiDeps
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  BUNDLE DESTINATION .
)

if(WIN32)
  # 注意：此处曾设置 LINK_FLAGS "/NODEFAULTLIB:LIBCMT"。该标志让链接器忽略静态 CRT，
  # 在早期使用 /MD（动态 CRT）时用于避免 CRT 冲突；但本项目改用 /MT 静态 CRT
  # （见根 CMakeLists.txt 的 CMAKE_MSVC_RUNTIME_LIBRARY 与 vcpkg 静态 triplet）后，
  # libcmt 正是运行库来源，排除它会导致 GUI 目标出现数千个未解析的 CRT 符号
  # （__acrt_iob_func、_CxxThrowException、operator new/delete、atexit 等）。
  # 已移除该标志。

  install(RUNTIME_DEPENDENCY_SET guiDeps
    ${WIN32_RUNTIME_DEPENDENCY_SET_ARGS}
    PRE_EXCLUDE_REGEXES ${WIN32_PRE_EXCLUDE_REGEXES}
    POST_EXCLUDE_REGEXES ${WIN32_POST_EXCLUDE_REGEXES}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  set(QT_DEPENDS_DIR ${CMAKE_BINARY_DIR}/qt-depends)

  # windeployqt 仅对共享链接的 Qt 有意义（把 Qt 运行时拷贝到可执行文件旁）。静态 Qt 下
  # DEPLOYQT 未定义（见 cmake/Libraries.cmake），此处必须跳过，否则会生成以
  # TARGET_FILE 开头的无效命令并导致构建失败。
  if(DEPLOYQT)
    add_custom_command(
      TARGET ${target} POST_BUILD
      COMMAND ${DEPLOYQT}
        --no-compiler-runtime
        --no-system-d3d-compiler
        --no-opengl-sw
        --no-quick-import
        --dir "${QT_DEPENDS_DIR}"
        --plugindir "${QT_DEPENDS_DIR}/plugins"
        $<TARGET_FILE:${target}>
      COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${QT_DEPENDS_DIR}
        $<TARGET_FILE_DIR:${target}>
    )

    install(
      DIRECTORY ${QT_DEPENDS_DIR}/
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
      PATTERN "dx*.dll" EXCLUDE
      PATTERN "icu*.dll" EXCLUDE
    )
  endif()

elseif(APPLE)
  if (BUILD_OSX_BUNDLE)
    set_target_properties(${target} PROPERTIES
      INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks"
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/deskflow.plist"
    )
    if (APPLE_CODESIGN_DEV)
      configure_mac_codesign(${target})
    endif()
  else()
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE FALSE)
  endif()
else()
  generate_app_man(${target} "${CMAKE_PROJECT_DESCRIPTION} \\(GUI\\)")
endif()
