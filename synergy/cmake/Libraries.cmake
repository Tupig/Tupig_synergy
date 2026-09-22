# SPDX-FileCopyrightText: (C) 2024 - 2025 Deskflow Developers
# SPDX-FileCopyrightText: (C) 2024 Symless Ltd
# SPDX-License-Identifier: MIT

# 依赖发现模块。
#
# 必须在文件作用域 include，不能放进下面的宏里：configure_libs 是 macro，宏在其调用点
# （根 CMakeLists.txt）展开，此时 CMAKE_CURRENT_LIST_DIR 指向的是调用方所在目录（仓库
# 根），宏内用 ${CMAKE_CURRENT_LIST_DIR} 会解析到 cmake/DependencyFallback.cmake 之外
# 的位置并导致 configure 失败。放在文件作用域时，CMAKE_CURRENT_LIST_DIR 正确指向本文件
# 所在的 cmake/ 目录。
#
# 注意：本文件第 80 行上游既有的 include(cmake/CodeCoverage.cmake) 依赖同样机制，但它靠
# 「相对仓库根目录」恰好解析正确；若日后 configure_libs 改由子目录调用，两处都需改为
# 文件作用域或使用 CMAKE_CURRENT_FUNCTION_LIST_DIR。
include("${CMAKE_CURRENT_LIST_DIR}/DependencyFallback.cmake")

macro(configure_libs)

  set(libs)
  if(UNIX)
    configure_unix_libs()
  elseif(WIN32)
    # /MP for parallel compilation; /MT for static CRT (vcpkg x64-windows-static triplet).
    # Do NOT add /MD here — it conflicts with vcpkg static triplets.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2")
    list(APPEND libs Wtsapi32 Userenv Wininet comsuppw Shlwapi version)
    add_definitions(
      /DWIN32
      /D_WINDOWS
      /D_CRT_SECURE_NO_WARNINGS
      /D_XKEYCHECK_H
    )
  endif()

  # 使用回退机制查找 Qt（版本无关探测与 Qt5 版本下调均在宏内完成）
  find_qt_with_fallback()

  if(UNIX AND NOT APPLE)
      find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS DBus Xml)
  endif()

  # Alias the Qt6:: targets onto Qt5:: so per-library CMake keeps its Qt6:: references
  # without a version-agnostic edit in every file. Drop this block when Qt5 is no longer needed.
  if(QT_VERSION_MAJOR EQUAL 5)
    foreach(_qt_comp IN ITEMS Core Gui Widgets Network DBus Xml)
      if(TARGET Qt5::${_qt_comp} AND NOT TARGET Qt6::${_qt_comp})
        add_library(Qt6::${_qt_comp} ALIAS Qt5::${_qt_comp})
      endif()
    endforeach()
  endif()

  # Qt 部署工具（windeployqt / macdeployqt）的用途是把 Qt 共享运行时拷贝到可执行文件旁，
  # 因此只在 Qt 为共享链接时才需要。本项目用 vcpkg 静态 triplet（目标是单产物独立运行），
  # 不存在可拷贝的共享运行时；且 vcpkg 的 qtbase 端口不安装该工具 —— 实测动态与静态
  # triplet 下都只有 qmake 的 windeployqt.prf，没有 windeployqt.exe。故按 Qt 链接形态判断：
  # 共享时缺失仍报错（保留原有保护），静态时跳过。
  #
  # 判据来自 Qt6::Core 导入目标：共享构建在 Windows 上带 IMPORTED_IMPLIB（.lib 导入库），
  # 静态构建不带；类 Unix 平台上共享构建的 IMPORTED_LOCATION 以 .so/.dylib 结尾。
  set(QT_IS_SHARED FALSE)
  if(TARGET Qt${QT_VERSION_MAJOR}::Core)
    get_target_property(_qt_imported_implib Qt${QT_VERSION_MAJOR}::Core IMPORTED_IMPLIB)
    if(_qt_imported_implib)
      set(QT_IS_SHARED TRUE)
    else()
      get_target_property(_qt_imported_location Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION)
      if(_qt_imported_location MATCHES "\\.(so|dylib)(\\.|$)")
        set(QT_IS_SHARED TRUE)
      endif()
    endif()
  endif()

  # Qt 部署工具（windeployqt / macdeployqt）的用途是把 Qt 共享运行时拷贝到可执行文件旁，
  # 因此只在 Qt 为共享链接时才相关。
  #
  # 重要：vcpkg 的 qtbase 端口**不提供该工具的二进制** —— 实测动态与静态 triplet 下都只有
  # qmake 的 windeployqt.prf，没有 windeployqt.exe。因此这里不能把它当作配置阶段的硬性条件，
  # 否则动态 triplet（如 ASan 构建所用的 x64-windows）会直接配置失败。
  #
  # 缺失时降级为警告：共享构建仍可编译运行（运行时需能找到 Qt DLL，例如把
  # vcpkg_installed/<triplet>/bin 加入 PATH）；仅当确实要“打包”共享版产物时才需要该工具，
  # 届时自行提供（如使用官方 Qt 安装包中的 windeployqt）。
  set(DEPLOY_TOOL "")
  if(WIN32)
    set(DEPLOY_TOOL windeployqt)
  elseif(APPLE)
    set(DEPLOY_TOOL macdeployqt)
  endif()

  if(DEPLOY_TOOL AND QT_IS_SHARED)
    find_program(DEPLOYQT ${DEPLOY_TOOL})
    if(DEPLOYQT STREQUAL "DEPLOYQT-NOTFOUND")
      message(WARNING
        "${DEPLOY_TOOL} not found; the Qt runtime will not be copied next to the binaries. "
        "vcpkg does not ship this tool, so this is expected. Add "
        "vcpkg_installed/<triplet>/bin to PATH when running, or supply the tool if packaging."
      )
    endif()
  elseif(DEPLOY_TOOL)
    message(STATUS "Qt is linked statically; ${DEPLOY_TOOL} is not required and is not provided by vcpkg")
  endif()
  unset(DEPLOY_TOOL)

  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_AUTORCC ON)

  message(STATUS "Qt version: ${Qt${QT_VERSION_MAJOR}_VERSION}")

  # Check if <format> header is available
  check_cxx_source_compiles("
    #include <format>
    int main() {
        char buffer[100];
        std::format_to_n(buffer, 100, \"test {}\", 42);
        return 0;
    }
    " HAVE_FORMAT)

  if(HAVE_FORMAT)
    add_definitions(-DHAVE_FORMAT)
  endif()

  option(ENABLE_COVERAGE "Enable test coverage" OFF)
  if(ENABLE_COVERAGE)
    message(STATUS "Enabling code coverage")
    include(cmake/CodeCoverage.cmake)
    append_coverage_compiler_flags()
    set(test_exclude subprojects/* build/* src/unittests/*)
    set(test_src ${PROJECT_SOURCE_DIR}/src)

    # Apparently solves the bug in gcov where it returns negative counts and confuses gcovr.
    # > Got negative hit value in gcov line 'branch  2 taken -1' caused by a bug in gcov tool
    # Bug report: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68080
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-update=atomic")

  endif()

endmacro()

#
# Unix (Mac, Linux, BSD, etc)
#
macro(configure_unix_libs)

  if(NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  endif()

  # For config.h, detect the libraries, functions, etc.
  include(CheckIncludeFiles)
  include(CheckLibraryExists)
  include(CheckFunctionExists)
  include(CheckTypeSize)
  include(CheckIncludeFileCXX)
  include(CheckSymbolExists)
  include(CheckCSourceCompiles)
  include(CheckCXXSourceCompiles)

  check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)
  if (NOT HAVE_SYS_SOCKET_H)
    message(FATAL_ERROR "Missing header: sys/socket.h")
  endif()


  check_include_files(unistd.h HAVE_UNISTD_H)
  if (NOT HAVE_UNISTD_H)
    message(FATAL_ERROR "Missing unistd.h")
  endif()

  check_function_exists(sigwait HAVE_POSIX_SIGWAIT)
  if (NOT HAVE_POSIX_SIGWAIT)
    message(FATAL_ERROR "Missing posix sigwait")
  endif()

  # pthread is used on both Linux and Mac
  check_library_exists("pthread" pthread_create "" HAVE_PTHREAD)
  if(HAVE_PTHREAD)
    list(APPEND libs pthread)
  else()
    message(FATAL_ERROR "Missing library: pthread")
  endif()

  if(APPLE)
    find_library(lib_ScreenSaver ScreenSaver)
    find_library(lib_IOKit IOKit)
    find_library(lib_ApplicationServices ApplicationServices)
    find_library(lib_Foundation Foundation)
    find_library(lib_Carbon Carbon)
    find_library(lib_UserNotifications UserNotifications)
    list(APPEND libs
      ${lib_ScreenSaver} ${lib_IOKit} ${lib_ApplicationServices}
      ${lib_Foundation} ${lib_Carbon} ${lib_UserNotifications}
    )
  else()

    if (BUILD_X11_SUPPORT)
      configure_xorg_libs()
    endif()

    include(FindPkgConfig)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(LIBXKBCOMMON REQUIRED xkbcommon)
      pkg_check_modules(GLIB2 REQUIRED glib-2.0)
      find_library(LIBM m)
      include_directories(${LIBXKBCOMMON_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS}
                          ${LIBM_INCLUDE_DIRS})
      
      message(STATUS "xkbcommon version: ${LIBXKBCOMMON_VERSION}")
    else()
      message(WARNING "pkg-config not found, skipping wayland libraries")
    endif()
  endif()
endmacro()

#
# X.org/X11 for Linux, BSD, etc
#
macro(configure_xorg_libs)

  # Runs on every X11 platform, not only BSD: the BSDs and some Linux setups keep
  # X11 headers under /usr/local, which GNUInstallDirs does not add to the search
  # path. Note this assigns rather than appends, so a caller-supplied
  # CMAKE_REQUIRED_INCLUDES is shadowed; Linux still resolves normally through the
  # compiler's own default paths.
  set(CMAKE_REQUIRED_INCLUDES "/usr/local/include")

  set(XKBlib "X11/Xlib.h;X11/XKBlib.h")
  set(CMAKE_EXTRA_INCLUDE_FILES "${XKBlib};X11/extensions/Xrandr.h")
  check_type_size("XRRNotifyEvent" X11_EXTENSIONS_XRANDR_H)
  set(HAVE_X11_EXTENSIONS_XRANDR_H "${X11_EXTENSIONS_XRANDR_H}")
  set(CMAKE_EXTRA_INCLUDE_FILES)

  check_include_files("${XKBlib};X11/extensions/dpms.h"
                      HAVE_X11_EXTENSIONS_DPMS_H)
  check_include_files("X11/extensions/Xinerama.h"
                      HAVE_X11_EXTENSIONS_XINERAMA_H)
  check_include_files("X11/extensions/XKB.h" HAVE_XKB_EXTENSION)
  check_include_files("X11/extensions/XTest.h" HAVE_X11_EXTENSIONS_XTEST_H)
  check_include_files("${XKBlib}" HAVE_X11_XKBLIB_H)
  check_include_files("X11/extensions/XInput2.h" HAVE_XI2)

  if(NOT HAVE_X11_XKBLIB_H)
    message(FATAL_ERROR "Missing header: " ${XKBlib})
  endif()

  # Same story for the library search path. On the FreeBSD CI, `link_directories`
  # is additionally required for this to be honoured.
  set(CMAKE_LIBRARY_PATH "/usr/local/lib")
  set(CMAKE_REQUIRED_FLAGS "-L${CMAKE_LIBRARY_PATH}")
  link_directories(${CMAKE_LIBRARY_PATH})

  check_library_exists("SM;ICE" IceConnectionNumber "" HAVE_ICE)
  check_library_exists("Xext;X11" DPMSQueryExtension "" HAVE_Xext)
  check_library_exists("Xtst;Xext;X11" XTestQueryExtension "" HAVE_Xtst)
  check_library_exists("Xinerama" XineramaQueryExtension "" HAVE_Xinerama)
  check_library_exists("Xi" XISelectEvents "" HAVE_Xi)
  check_library_exists("Xrandr" XRRQueryExtension "" HAVE_Xrandr)

  if(HAVE_ICE)

    # Assume we have SM if we have ICE.
    set(HAVE_SM 1)
    list(APPEND libs SM ICE)

  endif()

  if(!X11_xkbfile_FOUND)
    message(FATAL_ERROR "Missing library: xkbfile")
  endif()

  if(HAVE_Xtst)

    # Xtxt depends on X11.
    set(HAVE_X11)
    list(
      APPEND
      libs
      Xtst
      X11
      xkbfile)

  else()

    message(FATAL_ERROR "Missing library: Xtst")

  endif()

  if(HAVE_Xext)
    list(APPEND libs Xext)
  endif()

  if(HAVE_Xinerama)
    list(APPEND libs Xinerama)
  else(HAVE_Xinerama)
    if(HAVE_X11_EXTENSIONS_XINERAMA_H)
      set(HAVE_X11_EXTENSIONS_XINERAMA_H 0)
      message(WARNING "Old Xinerama implementation detected, disabled")
    endif()
  endif()

  if(HAVE_Xrandr)
    list(APPEND libs Xrandr)
  endif()

  # this was outside of the linux scope, not sure why, moving it back inside.
  if(HAVE_Xi)
    list(APPEND libs Xi)
  endif()

  add_definitions(-DWINAPI_XWINDOWS=1)

endmacro()
