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
#
# 若探测到的 Qt6 版本低于 REQUIRED_QT_VERSION（debian-12 / ubuntu-24.04 自带
# 6.4.x，项目下限 6.7.0），回退探测 Qt5 而不是硬失败——这些发行版的 qtbase5-dev
# 可用，且 Qt5 路径已由 rocky CI 腿覆盖。回退后须同步关闭依赖 Qt6 Test 的测试
# （矩阵对该腿传 -DBUILD_TESTS=OFF）。
macro(find_qt_with_fallback)
  find_package(QT NAMES Qt6 Qt5 QUIET COMPONENTS Core Widgets Network)
  if(QT_FOUND AND QT_VERSION_MAJOR EQUAL 6 AND QT_VERSION VERSION_LESS "${REQUIRED_QT_VERSION}")
    message(STATUS "Qt6 ${QT_VERSION} < required ${REQUIRED_QT_VERSION}; probing Qt5")
    find_package(QT NAMES Qt5 QUIET COMPONENTS Core Widgets Network)
    if(NOT QT_FOUND)
      find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network)
    endif()
  elseif(NOT QT_FOUND)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network)
  endif()
  if(QT_VERSION_MAJOR EQUAL 5)
    set(REQUIRED_QT_VERSION 5.13)
    set(REQUIRED_OPENSSL_VERSION 1.1.1)
  endif()
  find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Widgets Network)
  set(QT_FOUND TRUE)
  message(STATUS "Qt: ${Qt${QT_VERSION_MAJOR}_VERSION}")
endmacro()
