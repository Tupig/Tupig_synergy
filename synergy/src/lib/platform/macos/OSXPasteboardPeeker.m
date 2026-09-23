/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#import "OSXPasteboardPeeker.h"

#import <Cocoa/Cocoa.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

CFArrayRef copyDraggedFilePaths(void)
{
  @autoreleasepool {
    // NSPasteboardNameDrag is the modern name for the drag pasteboard; the
    // filenames property list is still what Explorer-equivalent drags publish.
    NSPasteboard *pboard = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];
    if (pboard == nil) {
      return NULL;
    }

    NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
    if (files == nil || files.count == 0) {
      return NULL;
    }

    // Retain across the autoreleasepool so the caller can CFRelease later.
    return (CFArrayRef)CFBridgingRetain(files);
  }
}

#pragma clang diagnostic pop
