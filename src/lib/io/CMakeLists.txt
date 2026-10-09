# SPDX-FileCopyrightText: (C) 2024 - 2025 Chris Rizzitello <https://github.com/Tupig/Tupig_synergy/issues>
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_library(io STATIC
  IOException.cpp
  IOException.h
  IStream.h
  StreamBuffer.cpp
  StreamBuffer.h
  StreamFilter.cpp
  StreamFilter.h
)

target_link_libraries(io PUBLIC common)
