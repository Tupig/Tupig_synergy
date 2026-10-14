/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsClipboardBitmapConverter.h"

#include "base/Log.h"

//
// MSWindowsClipboardBitmapConverter
//

IClipboard::Format MSWindowsClipboardBitmapConverter::getFormat() const
{
  return IClipboard::Format::Bitmap;
}

UINT MSWindowsClipboardBitmapConverter::getWin32Format() const
{
  return CF_DIB;
}

HANDLE
MSWindowsClipboardBitmapConverter::fromIClipboard(const std::string &data) const
{
  // copy to memory handle
  HGLOBAL gData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, data.size());
  if (gData != nullptr) {
    // get a pointer to the allocated memory
    char *dst = (char *)GlobalLock(gData);
    if (dst != nullptr) {
      memcpy(dst, data.data(), data.size());
      GlobalUnlock(gData);
    } else {
      GlobalFree(gData);
      gData = nullptr;
    }
  }

  return gData;
}

std::string MSWindowsClipboardBitmapConverter::toIClipboard(HANDLE data) const
{
  // get datator
  LPVOID src = GlobalLock(data);
  if (src == nullptr) {
    return std::string();
  }
  uint32_t srcSize = (uint32_t)GlobalSize(data);

  // the header must fit in the handle before any of its fields are read
  const BITMAPINFO *bitmap = static_cast<const BITMAPINFO *>(src);
  if (srcSize < sizeof(BITMAPINFOHEADER) || bitmap->bmiHeader.biSize < sizeof(BITMAPINFOHEADER) ||
      bitmap->bmiHeader.biSize > srcSize) {
    GlobalUnlock(data);
    return {};
  }

  // check image type
  LOG(
      (CLOG_INFO "bitmap: %dx%d %d", bitmap->bmiHeader.biWidth, bitmap->bmiHeader.biHeight,
       (int)bitmap->bmiHeader.biBitCount)
  );
  if (bitmap->bmiHeader.biPlanes == 1 && (bitmap->bmiHeader.biBitCount == 24 || bitmap->bmiHeader.biBitCount == 32) &&
      bitmap->bmiHeader.biCompression == BI_RGB) {
    // already in canonical form
    std::string image(static_cast<char const *>(src), srcSize);
    GlobalUnlock(data);
    return image;
  }

  // reject empty dimensions and outputs whose 32-bit size would overflow
  const LONG w = bitmap->bmiHeader.biWidth;
  const LONG h = bitmap->bmiHeader.biHeight;
  if (w <= 0 || h == 0) {
    GlobalUnlock(data);
    return {};
  }
  const int64_t dstBytes = 4LL * w * (h > 0 ? static_cast<int64_t>(h) : -static_cast<int64_t>(h));
  if (dstBytes <= 0 || dstBytes > INT32_MAX) {
    GlobalUnlock(data);
    return {};
  }

  // create a destination DIB section
  LOG_INFO("convert image from: depth=%d comp=%d", bitmap->bmiHeader.biBitCount, bitmap->bmiHeader.biCompression);
  BITMAPINFOHEADER info;
  info.biSize = sizeof(BITMAPINFOHEADER);
  info.biWidth = w;
  info.biHeight = h;
  info.biPlanes = 1;
  info.biBitCount = 32;
  info.biCompression = BI_RGB;
  info.biSizeImage = 0;
  info.biXPelsPerMeter = 1000;
  info.biYPelsPerMeter = 1000;
  info.biClrUsed = 0;
  info.biClrImportant = 0;

  // find the start of the pixel data
  const char *srcBits = (const char *)bitmap + bitmap->bmiHeader.biSize;
  if (bitmap->bmiHeader.biBitCount >= 16) {
    if (bitmap->bmiHeader.biCompression == BI_BITFIELDS &&
        (bitmap->bmiHeader.biBitCount == 16 || bitmap->bmiHeader.biBitCount == 32)) {
      srcBits += 3 * sizeof(DWORD);
    }
  } else if (bitmap->bmiHeader.biClrUsed != 0) {
    srcBits += bitmap->bmiHeader.biClrUsed * sizeof(RGBQUAD);
  } else {
    // http://msdn.microsoft.com/en-us/library/ke55d167(VS.80).aspx
    srcBits += (1i64 << bitmap->bmiHeader.biBitCount) * sizeof(RGBQUAD);
  }

  // the palette or masks must still be inside the handle
  const auto srcOffset = static_cast<size_t>(srcBits - static_cast<const char *>(src));
  if (srcOffset >= srcSize) {
    GlobalUnlock(data);
    return {};
  }

  HDC dc = GetDC(nullptr);
  if (dc == nullptr) {
    GlobalUnlock(data);
    return {};
  }

  void *raw = nullptr;
  HBITMAP dst = CreateDIBSection(dc, (BITMAPINFO *)&info, DIB_RGB_COLORS, &raw, nullptr, 0);
  HDC dstDC = (dst != nullptr) ? CreateCompatibleDC(dc) : nullptr;
  HGDIOBJ oldBitmap = (dstDC != nullptr) ? SelectObject(dstDC, dst) : nullptr;
  if (dst == nullptr || raw == nullptr || dstDC == nullptr || oldBitmap == nullptr || oldBitmap == HGDI_ERROR) {
    if (dstDC != nullptr) {
      DeleteDC(dstDC);
    }
    if (dst != nullptr) {
      DeleteObject(dst);
    }
    ReleaseDC(nullptr, dc);
    GlobalUnlock(data);
    return {};
  }

  // copy source image to destination image
  SetDIBitsToDevice(dstDC, 0, 0, w, h, 0, 0, 0, h, srcBits, bitmap, DIB_RGB_COLORS);
  SelectObject(dstDC, oldBitmap);
  DeleteDC(dstDC);
  GdiFlush();

  // extract data
  std::string image((const char *)&info, info.biSize);
  image.append((const char *)raw, static_cast<size_t>(dstBytes));

  // clean up GDI
  DeleteObject(dst);
  ReleaseDC(nullptr, dc);

  // release handle
  GlobalUnlock(data);

  return image;
}
