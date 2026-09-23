# SPDX-FileCopyrightText: (C) 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: (C) 2012 - 2025 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

set(lib_name app)

# Platform-specific code
if(WIN32)
  set(PLATFORM_CODE
    win32/AppUtilWindows.cpp
    win32/AppUtilWindows.h
  )
elseif(UNIX)
  set(PLATFORM_CODE
    unix/AppUtilUnix.cpp
    unix/AppUtilUnix.h
    unix/DeskflowXkbKeyboard.cpp
    unix/DeskflowXkbKeyboard.h
    unix/ISO639Table.h
    unix/X11LayoutsParser.cpp
    unix/X11LayoutsParser.h
  )
endif()

add_library(${lib_name} STATIC ${PLATFORM_CODE}
  # Core
  core/App.cpp
  core/App.h
  core/AppUtil.cpp
  core/AppUtil.h
  core/ClientApp.cpp
  core/ClientApp.h
  core/ServerApp.cpp
  core/ServerApp.h
  core/IApp.h
  core/IClient.h
  core/DeskflowException.cpp
  core/DeskflowException.h
  core/DisplayInvalidException.h
  # Clipboard
  clipboard/Clipboard.cpp
  clipboard/Clipboard.h
  clipboard/ClipboardChunk.cpp
  clipboard/ClipboardChunk.h
  clipboard/ClipboardTypes.h
  clipboard/IClipboard.cpp
  clipboard/IClipboard.h
  # Input
  input/KeyMap.cpp
  input/KeyMap.h
  input/KeyState.cpp
  input/KeyState.h
  input/KeyTypes.cpp
  input/KeyTypes.h
  input/MouseTypes.h
  input/InputValidator.cpp
  input/InputValidator.h
  input/KeyboardLayoutManager.cpp
  input/KeyboardLayoutManager.h
  input/IKeyState.cpp
  input/IKeyState.h
  # Protocol
  protocol/Chunk.cpp
  protocol/Chunk.h
  protocol/FileChunk.cpp
  protocol/FileChunk.h
  protocol/FileTransferPath.cpp
  protocol/FileTransferPath.h
  protocol/FileTransferReceiver.cpp
  protocol/FileTransferReceiver.h
  protocol/FileTransferSender.cpp
  protocol/FileTransferSender.h
  protocol/FileTransferSource.cpp
  protocol/FileTransferSource.h
  protocol/OptionTypes.h
  protocol/PacketStreamFilter.cpp
  protocol/PacketStreamFilter.h
  protocol/ProtocolTypes.cpp
  protocol/ProtocolTypes.h
  protocol/ProtocolUtil.cpp
  protocol/ProtocolUtil.h
  protocol/StreamChunker.cpp
  protocol/StreamChunker.h
  # Screen
  screen/IPlatformScreen.cpp
  screen/IPlatformScreen.h
  screen/IPrimaryScreen.cpp
  screen/IPrimaryScreen.h
  screen/IScreen.h
  screen/IScreenSaver.h
  screen/ISecondaryScreen.h
  screen/PlatformScreen.cpp
  screen/PlatformScreen.h
  screen/Screen.cpp
  screen/Screen.h
  screen/ScreenException.cpp
  screen/ScreenException.h
  # IPC
  ipc/CoreIpc.cpp
  ipc/CoreIpc.h
  ipc/CoreIpcServer.cpp
  ipc/CoreIpcServer.h
  ipc/DaemonIpcServer.cpp
  ipc/DaemonIpcServer.h
  ipc/IpcServer.cpp
  ipc/IpcServer.h
)

# Add deskflow subdirectories to include path so files can include each other
# using just the filename (e.g., #include "App.h" instead of #include "core/App.h")
target_include_directories(${lib_name} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/clipboard
  ${CMAKE_CURRENT_SOURCE_DIR}/input
  ${CMAKE_CURRENT_SOURCE_DIR}/protocol
  ${CMAKE_CURRENT_SOURCE_DIR}/screen
  ${CMAKE_CURRENT_SOURCE_DIR}/ipc
  ${CMAKE_CURRENT_SOURCE_DIR}/win32
  ${CMAKE_CURRENT_SOURCE_DIR}/unix
)

target_link_libraries(${lib_name} PUBLIC common Qt6::Core Qt6::Network)

if(WIN32)
  target_link_libraries(${lib_name} PRIVATE platform)
endif()

if(UNIX)
  target_link_libraries(
    ${lib_name}
    PRIVATE
    arch
    client
    net
    base
    platform
    mt
    server
  )

  if(NOT APPLE)
    target_link_libraries(${lib_name} PRIVATE Qt6::Xml)
  endif()
endif()
