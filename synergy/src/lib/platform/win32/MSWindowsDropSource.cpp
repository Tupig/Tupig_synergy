/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsDropSource.h"

#include "Win32DropData.h"
#include "base/Log.h"

#include <ShlObj.h>

#include <cstring>

namespace {

FORMATETC hdropFormat()
{
  FORMATETC fmt = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  return fmt;
}

class DropFormatEnumerator : public IEnumFORMATETC
{
public:
  DropFormatEnumerator() = default;

  HRESULT __stdcall QueryInterface(REFIID iid, void **object) override
  {
    if (object == nullptr) {
      return E_POINTER;
    }
    if (iid == IID_IEnumFORMATETC || iid == IID_IUnknown) {
      AddRef();
      *object = static_cast<IEnumFORMATETC *>(this);
      return S_OK;
    }
    *object = nullptr;
    return E_NOINTERFACE;
  }

  ULONG __stdcall AddRef() override
  {
    return static_cast<ULONG>(::InterlockedIncrement(&m_refCount));
  }

  ULONG __stdcall Release() override
  {
    const LONG count = ::InterlockedDecrement(&m_refCount);
    if (count == 0) {
      delete this;
      return 0;
    }
    return static_cast<ULONG>(count);
  }

  HRESULT __stdcall Next(ULONG count, FORMATETC *formats, ULONG *fetched) override
  {
    if (formats == nullptr) {
      return E_POINTER;
    }
    ULONG n = 0;
    if (count > 0 && !m_done) {
      formats[0] = hdropFormat();
      n = 1;
      m_done = true;
    }
    if (fetched != nullptr) {
      *fetched = n;
    }
    return n == count ? S_OK : S_FALSE;
  }

  HRESULT __stdcall Skip(ULONG count) override
  {
    if (count > 0) {
      m_done = true;
    }
    return S_OK;
  }

  HRESULT __stdcall Reset() override
  {
    m_done = false;
    return S_OK;
  }

  HRESULT __stdcall Clone(IEnumFORMATETC **out) override
  {
    if (out == nullptr) {
      return E_POINTER;
    }
    auto *clone = new DropFormatEnumerator();
    clone->m_done = m_done;
    *out = clone;
    return S_OK;
  }

private:
  long m_refCount = 1;
  bool m_done = false;
};

} // namespace

//
// MSWindowsDropSource
//

HRESULT __stdcall MSWindowsDropSource::QueryInterface(REFIID iid, void **object)
{
  if (object == nullptr) {
    return E_POINTER;
  }
  if (iid == IID_IDropSource || iid == IID_IUnknown) {
    AddRef();
    *object = static_cast<IDropSource *>(this);
    return S_OK;
  }
  *object = nullptr;
  return E_NOINTERFACE;
}

ULONG __stdcall MSWindowsDropSource::AddRef()
{
  return static_cast<ULONG>(::InterlockedIncrement(&m_refCount));
}

ULONG __stdcall MSWindowsDropSource::Release()
{
  const LONG count = ::InterlockedDecrement(&m_refCount);
  if (count == 0) {
    delete this;
    return 0;
  }
  return static_cast<ULONG>(count);
}

HRESULT __stdcall MSWindowsDropSource::QueryContinueDrag(BOOL escapePressed, DWORD keyState)
{
  if (escapePressed) {
    return DRAGDROP_S_CANCEL;
  }
  if ((keyState & (MK_LBUTTON | MK_RBUTTON)) == 0) {
    return DRAGDROP_S_DROP;
  }
  return S_OK;
}

HRESULT __stdcall MSWindowsDropSource::GiveFeedback(DWORD /*effect*/)
{
  return DRAGDROP_S_USEDEFAULTCURSORS;
}

//
// MSWindowsDropDataObject
//

MSWindowsDropDataObject::MSWindowsDropDataObject(const std::vector<std::string> &utf8Paths)
{
  m_hdrop = deskflow::win32::createDropFilesHGlobal(utf8Paths);
}

MSWindowsDropDataObject::~MSWindowsDropDataObject()
{
  if (m_hdrop != nullptr) {
    ::GlobalFree(m_hdrop);
    m_hdrop = nullptr;
  }
}

HRESULT __stdcall MSWindowsDropDataObject::QueryInterface(REFIID iid, void **object)
{
  if (object == nullptr) {
    return E_POINTER;
  }
  if (iid == IID_IDataObject || iid == IID_IUnknown) {
    AddRef();
    *object = static_cast<IDataObject *>(this);
    return S_OK;
  }
  *object = nullptr;
  return E_NOINTERFACE;
}

ULONG __stdcall MSWindowsDropDataObject::AddRef()
{
  return static_cast<ULONG>(::InterlockedIncrement(&m_refCount));
}

ULONG __stdcall MSWindowsDropDataObject::Release()
{
  const LONG count = ::InterlockedDecrement(&m_refCount);
  if (count == 0) {
    delete this;
    return 0;
  }
  return static_cast<ULONG>(count);
}

bool MSWindowsDropDataObject::matchesHdrop(const FORMATETC *format) const
{
  return format != nullptr && format->cfFormat == CF_HDROP && (format->tymed & TYMED_HGLOBAL) != 0 &&
         (format->dwAspect == DVASPECT_CONTENT || format->dwAspect == -1);
}

HRESULT __stdcall MSWindowsDropDataObject::GetData(FORMATETC *format, STGMEDIUM *medium)
{
  if (format == nullptr || medium == nullptr) {
    return E_POINTER;
  }
  if (m_hdrop == nullptr || !matchesHdrop(format)) {
    return DV_E_FORMATETC;
  }

  // OLE takes ownership of a duplicate HGLOBAL; keep our original for further Gets.
  const SIZE_T size = ::GlobalSize(m_hdrop);
  HGLOBAL copy = ::GlobalAlloc(GHND, size);
  if (copy == nullptr) {
    return E_OUTOFMEMORY;
  }
  void *src = ::GlobalLock(m_hdrop);
  void *dst = ::GlobalLock(copy);
  if (src == nullptr || dst == nullptr) {
    if (src != nullptr) {
      ::GlobalUnlock(m_hdrop);
    }
    if (dst != nullptr) {
      ::GlobalUnlock(copy);
    }
    ::GlobalFree(copy);
    return E_OUTOFMEMORY;
  }
  std::memcpy(dst, src, size);
  ::GlobalUnlock(m_hdrop);
  ::GlobalUnlock(copy);

  medium->tymed = TYMED_HGLOBAL;
  medium->hGlobal = copy;
  medium->pUnkForRelease = nullptr;
  return S_OK;
}

HRESULT __stdcall MSWindowsDropDataObject::GetDataHere(FORMATETC *, STGMEDIUM *)
{
  return E_NOTIMPL;
}

HRESULT __stdcall MSWindowsDropDataObject::QueryGetData(FORMATETC *format)
{
  if (m_hdrop == nullptr) {
    return DV_E_FORMATETC;
  }
  return matchesHdrop(format) ? S_OK : DV_E_FORMATETC;
}

HRESULT __stdcall MSWindowsDropDataObject::GetCanonicalFormatEtc(FORMATETC *, FORMATETC *out)
{
  if (out == nullptr) {
    return E_POINTER;
  }
  *out = hdropFormat();
  return DATA_S_SAMEFORMATETC;
}

HRESULT __stdcall MSWindowsDropDataObject::SetData(FORMATETC *, STGMEDIUM *, BOOL)
{
  return E_NOTIMPL;
}

HRESULT __stdcall MSWindowsDropDataObject::EnumFormatEtc(DWORD direction, IEnumFORMATETC **enumerator)
{
  if (enumerator == nullptr) {
    return E_POINTER;
  }
  if (direction != DATADIR_GET || m_hdrop == nullptr) {
    return E_NOTIMPL;
  }
  *enumerator = new DropFormatEnumerator();
  return S_OK;
}

HRESULT __stdcall MSWindowsDropDataObject::DAdvise(FORMATETC *, DWORD, IAdviseSink *, DWORD *)
{
  return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT __stdcall MSWindowsDropDataObject::DUnadvise(DWORD)
{
  return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT __stdcall MSWindowsDropDataObject::EnumDAdvise(IEnumSTATDATA **)
{
  return OLE_E_ADVISENOTSUPPORTED;
}

//
// deskflow::win32::startDraggingFiles
//

bool deskflow::win32::startDraggingFiles(const std::vector<std::string> &utf8Paths)
{
  if (utf8Paths.empty()) {
    return false;
  }

  auto *dataObject = new MSWindowsDropDataObject(utf8Paths);
  if (dataObject == nullptr) {
    return false;
  }

  // Construction may have failed to allocate CF_HDROP; QueryGetData then fails.
  FORMATETC fmt = hdropFormat();
  if (dataObject->QueryGetData(&fmt) != S_OK) {
    dataObject->Release();
    LOG_ERR("startDraggingFiles: failed to build CF_HDROP");
    return false;
  }

  auto *source = new MSWindowsDropSource();
  DWORD effect = DROPEFFECT_NONE;
  const HRESULT hr = ::DoDragDrop(dataObject, source, DROPEFFECT_COPY, &effect);
  source->Release();
  dataObject->Release();

  if (hr == DRAGDROP_S_DROP) {
    LOG_INFO("startDraggingFiles: drop completed (effect=%lu)", effect);
    return true;
  }
  if (hr == DRAGDROP_S_CANCEL) {
    LOG_DEBUG("startDraggingFiles: cancelled");
    return false;
  }

  LOG_WARN("startDraggingFiles: DoDragDrop failed: 0x%08lx", static_cast<unsigned long>(hr));
  return false;
}
