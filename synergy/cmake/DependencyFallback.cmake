# SPDX-FileCopyrightText: (C) 2024 - 2026 TuPig
# SPDX-License-Identifier: MIT

# 依赖发现模块 — vcpkg manifest 模式
# vcpkg 通过 CMAKE_TOOLCHAIN_FILE 自动注入 find_package 支持。
# 本模块仅提供简洁的包装宏，供 Libraries.cmake 调用。

# Qt 发现宏
# 不能硬编码 Qt6：RHEL/Rocky 8 只有 Qt5（CI 的 rocky-8/9 目标即走该路径）。
# 先做版本无关探测（Qt6 优先、回退 Qt5），再按探测到的主版本查找组件；
# Qt5 时一并下调 REQUIRED_QT_VERSION 与 REQUIRED_OPENSSL_VERSION 到 RHEL 8.10
# 自带的 Qt 5.13 / OpenSSL 1.1.1。宏无独立作用域，故这些变量会传回调用方。
macro(find_qt_with_fallback)
  find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network)
  if(QT_VERSION_MAJOR EQUAL 5)
    set(REQUIRED_QT_VERSION 5.13)
    set(REQUIRED_OPENSSL_VERSION 1.1.1)
  endif()
  find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Widgets Network)
  set(QT_FOUND TRUE)
  message(STATUS "Qt: ${Qt${QT_VERSION_MAJOR}_VERSION}")
endmacro()
