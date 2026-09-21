# SPDX-FileCopyrightText: (C) 2024 - 2026 TuPig
# SPDX-License-Identifier: MIT

# 依赖发现模块 — vcpkg manifest 模式
# vcpkg 通过 CMAKE_TOOLCHAIN_FILE 自动注入 find_package 支持。
# 本模块仅提供简洁的包装宏，供 Libraries.cmake 调用。

# Qt6 发现宏
macro(find_qt_with_fallback)
  # vcpkg 模式: find_package 由 vcpkg toolchain 自动处理
  find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)
  set(QT_FOUND TRUE)
  set(QT_VERSION_MAJOR 6)
  message(STATUS "Qt6: ${Qt6_VERSION}")
endmacro()
