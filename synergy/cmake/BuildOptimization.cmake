# SPDX-FileCopyrightText: (C) 2024 - 2026 TuPig
# SPDX-License-Identifier: MIT

# 构建性能优化配置

# 1. 启用 ccache/sccache（如果可用）
find_program(CCACHE_PROGRAM ccache)
find_program(SCCACHE_PROGRAM sccache)

if(SCCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
  message(STATUS "Using sccache: ${SCCACHE_PROGRAM}")
elseif(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

# 2. MSVC 并行编译
if(MSVC)
  add_compile_options(/MP)
endif()

# 3. 预编译头（可选，需要项目支持）
option(USE_PCH "Use precompiled headers" OFF)
if(USE_PCH)
  set(CMAKE_PRECOMPILE_HEADERS ON)
endif()

# 4. 链接优化
if(MSVC)
  # 减少链接器内存使用
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

# 5. 增量构建支持
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 6. 构建类型默认值
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()
