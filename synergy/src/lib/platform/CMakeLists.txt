# SPDX-FileCopyrightText: (C) 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: (C) 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: (C) 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

# Detected, not REQUIRED: RHEL ships no libei/libportal, so those legs build X11-only.
if(UNIX AND NOT APPLE)
  pkg_check_modules(LIBEI QUIET "libei-1.0 >= ${REQUIRED_LIBEI_VERSION}")
  message(STATUS "libei version: ${LIBEI_VERSION}")

  pkg_check_modules(LIBPORTAL QUIET "libportal >= ${REQUIRED_LIBPORTAL_VERSION}")
  message(STATUS "libportal version: ${LIBPORTAL_VERSION}")

  # Check if we have the restore ability in the IC portal
  include(CMakePushCheckState)
  include(CheckSymbolExists)
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES};${LIBPORTAL_INCLUDE_DIRS};${GLIB2_INCLUDE_DIRS}")
  set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES};${LIBPORTAL_LINK_LIBRARIES};${GLIB2_LINK_LIBRARIES}")
  check_symbol_exists(xdp_input_capture_session_get_restore_token "libportal/inputcapture.h" HAVE_IC_RESTORE)
  check_symbol_exists(xdp_session_request_clipboard "libportal/portal-helpers.h;libportal/clipboard.h" HAVE_LP_CLIPBOARD)
  cmake_pop_check_state()
endif()

if(WIN32)
  set(PLATFORM_SOURCES
    win32/IMSWindowsClipboardFacade.h
    win32/MSWindowsClipboard.cpp
    win32/MSWindowsClipboard.h
    win32/MSWindowsClipboardAnyTextConverter.cpp
    win32/MSWindowsClipboardAnyTextConverter.h
    win32/MSWindowsClipboardBitmapConverter.cpp
    win32/MSWindowsClipboardBitmapConverter.h
    win32/MSWindowsClipboardFacade.cpp
    win32/MSWindowsClipboardFacade.h
    win32/MSWindowsClipboardHTMLConverter.cpp
    win32/MSWindowsClipboardHTMLConverter.h
    win32/MSWindowsClipboardUTF16Converter.cpp
    win32/MSWindowsClipboardUTF16Converter.h
    win32/MSWindowsDebugOutputter.cpp
    win32/MSWindowsDebugOutputter.h
    win32/MSWindowsDesks.cpp
    win32/MSWindowsDesks.h
    win32/MSWindowsEventQueueBuffer.cpp
    win32/MSWindowsEventQueueBuffer.h
    win32/MSWindowsHandle.h
    win32/MSWindowsHook.cpp
    win32/MSWindowsHook.h
    win32/MSWindowsKeyState.cpp
    win32/MSWindowsKeyState.h
    win32/MSWindowsPowerManager.cpp
    win32/MSWindowsPowerManager.h
    win32/MSWindowsProcess.cpp
    win32/MSWindowsProcess.h
    win32/MSWindowsScreen.cpp
    win32/MSWindowsScreen.h
    win32/MSWindowsScreenSaver.cpp
    win32/MSWindowsScreenSaver.h
    win32/MSWindowsSession.cpp
    win32/MSWindowsSession.h
    win32/MSWindowsWatchdog.cpp
    win32/MSWindowsWatchdog.h
    win32/Win32DropData.cpp
    win32/Win32DropData.h
    win32/MSWindowsDropTarget.cpp
    win32/MSWindowsDropTarget.h
    win32/MSWindowsDropSource.cpp
    win32/MSWindowsDropSource.h
  )
elseif(APPLE)
  set(PLATFORM_SOURCES
    macos/IOSXKeyResource.cpp
    macos/IOSXKeyResource.h
    macos/OSXAutoTypes.h
    macos/OSXClipboard.cpp
    macos/OSXClipboard.h
    macos/OSXClipboardAnyBitmapConverter.cpp
    macos/OSXClipboardAnyBitmapConverter.h
    macos/OSXClipboardAnyTextConverter.cpp
    macos/OSXClipboardAnyTextConverter.h
    macos/OSXClipboardBMPConverter.cpp
    macos/OSXClipboardBMPConverter.h
    macos/OSXClipboardHTMLConverter.cpp
    macos/OSXClipboardHTMLConverter.h
    macos/OSXClipboardTextConverter.cpp
    macos/OSXClipboardTextConverter.h
    macos/OSXClipboardUTF8Converter.cpp
    macos/OSXClipboardUTF8Converter.h
    macos/OSXClipboardUTF16Converter.cpp
    macos/OSXClipboardUTF16Converter.h
    macos/OSXCocoaApp.h
    macos/OSXCocoaApp.m
    macos/OSXEventQueueBuffer.cpp
    macos/OSXEventQueueBuffer.h
    macos/OSXKeyState.cpp
    macos/OSXKeyState.h
    macos/OSXMediaKeySupport.h
    macos/OSXMediaKeySupport.m
    macos/OSXPasteboardPeeker.h
    macos/OSXPasteboardPeeker.m
    macos/OSXPowerManager.cpp
    macos/OSXPowerManager.h
    macos/OSXScreen.h
    macos/OSXScreen.mm
    macos/OSXScreenSaver.cpp
    macos/OSXScreenSaver.h
    macos/OSXScreenSaverControl.h
    macos/OSXScreenSaverUtil.h
    macos/OSXScreenSaverUtil.m
    macos/OSXUchrKeyResource.cpp
    macos/OSXUchrKeyResource.h
  )
elseif(UNIX)
  set(PLATFORM_SOURCES
    linux/XDGPortalRegistry.h
    linux/XDGPowerManager.cpp
    linux/XDGPowerManager.h
    linux/XDGKeyUtil.h
    linux/XDGKeyUtil.cpp
  )
  if (BUILD_X11_SUPPORT)
    configure_file(linux/XWindowsConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/XWindowsConfig.h @ONLY)
    list(APPEND PLATFORM_SOURCES
      linux/XWindowsClipboard.cpp
      linux/XWindowsClipboard.h
      linux/XWindowsClipboardAnyBitmapConverter.cpp
      linux/XWindowsClipboardAnyBitmapConverter.h
      linux/XWindowsClipboardBMPConverter.cpp
      linux/XWindowsClipboardBMPConverter.h
      linux/XWindowsClipboardHTMLConverter.cpp
      linux/XWindowsClipboardHTMLConverter.h
      linux/XWindowsClipboardTextConverter.cpp
      linux/XWindowsClipboardTextConverter.h
      linux/XWindowsClipboardUCS2Converter.cpp
      linux/XWindowsClipboardUCS2Converter.h
      linux/XWindowsClipboardUTF8Converter.cpp
      linux/XWindowsClipboardUTF8Converter.h
      linux/XWindowsEventQueueBuffer.cpp
      linux/XWindowsEventQueueBuffer.h
      linux/XWindowsKeyState.cpp
      linux/XWindowsKeyState.h
      linux/XWindowsScreen.cpp
      linux/XWindowsScreen.h
      linux/XWindowsScreenSaver.cpp
      linux/XWindowsScreenSaver.h
      linux/XWindowsUtil.cpp
      linux/XWindowsUtil.h
      ${CMAKE_CURRENT_BINARY_DIR}/XWindowsConfig.h
    )
  endif()

  if(LIBEI_FOUND)
    list(APPEND PLATFORM_SOURCES
      linux/EiClipboard.cpp
      linux/EiClipboard.h
      linux/EiEventQueueBuffer.cpp
      linux/EiEventQueueBuffer.h
      linux/EiKeyState.cpp
      linux/EiKeyState.h
      linux/EiScreen.cpp
      linux/EiScreen.h
    )
    # The Portal sources also require EI.
    if(LIBPORTAL_FOUND)
      list(APPEND PLATFORM_SOURCES
        linux/PortalInputCapture.cpp
        linux/PortalInputCapture.h
        linux/PortalRemoteDesktop.cpp
        linux/PortalRemoteDesktop.h
      )
      if(HAVE_LP_CLIPBOARD)
        list(APPEND PLATFORM_SOURCES
          linux/PortalClipboard.cpp
          linux/PortalClipboard.h
        )
      endif()
    endif()
  endif()
endif()

# wayland.h is included to check for wayland support
add_library(platform STATIC ${PLATFORM_SOURCES})

# Add platform subdirectories to include path so files can include each other
# using just the filename (e.g., #include "EiClipboard.h" instead of #include "linux/EiClipboard.h")
# PUBLIC so other libraries (deskflow, etc.) can also find platform headers
target_include_directories(platform PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/win32
  ${CMAKE_CURRENT_SOURCE_DIR}/macos
  ${CMAKE_CURRENT_SOURCE_DIR}/linux
)

target_link_libraries(platform client ${libs})

if(WIN32)
  # RegisterDragDrop / IDropTarget / OleInitialize
  target_link_libraries(platform ole32 shell32)
endif()

if(UNIX)
  target_link_libraries(
    platform
    io
    net
    app
    client
    ${libs})

  if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(platform ${COCOA_LIBRARY})
  else()
    if(${LIBXKBCOMMON_VERSION} VERSION_GREATER_EQUAL "1.10")
      target_compile_definitions(platform PRIVATE HAVE_XKB_KEYMAP_MOD_GET_MASK=1)
    endif()
    if(LIBEI_FOUND)
      target_compile_definitions(platform PUBLIC WINAPI_LIBEI)
    endif()
    if(LIBPORTAL_FOUND)
      target_compile_definitions(platform PUBLIC WINAPI_LIBPORTAL)
    endif()
    if (HAVE_IC_RESTORE)
      target_compile_definitions(platform PUBLIC HAVE_LIBPORTAL_INPUTCAPTURE_RESTORE)
    endif()
    if (HAVE_LP_CLIPBOARD)
      target_compile_definitions(platform PUBLIC HAVE_LIBPORTAL_CLIPBOARD)
    endif()
    target_include_directories(platform PUBLIC ${LIBEI_INCLUDE_DIRS} ${LIBPORTAL_INCLUDE_DIRS})
    target_link_libraries(
      platform
      Qt6::DBus
      Qt6::Gui
      ${LIBXKBCOMMON_LINK_LIBRARIES}
      ${GLIB2_LINK_LIBRARIES} ${LIBM_LIBRARIES}
      ${LIBEI_LINK_LIBRARIES} ${LIBPORTAL_LINK_LIBRARIES})
  endif()
endif()
