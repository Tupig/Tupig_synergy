/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsDropTarget.h"

#include "Win32DropData.h"
#include "base/Log.h"

#include <ShlObj.h>

namespace {

FORMATETC hdropFormat()
{
  FORMATETC fmt = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  return fmt;
}

//! Lock an HGLOBAL HDROP block and parse it with the shared, tested walker.
std::vector<std::string> pathsFromHGlobal(HGLOBAL handle)
{
  if (handle == nullptr) {
    return {};
  }

  const SIZE_T size = ::GlobalSize(handle);
  if (size == 0) {
    return {};
  }

  void *locked = ::GlobalLock(handle);
  if (locked == nullptr) {
    LOG_ERR("CF_HDROP GlobalLock failed: %lu", ::GetLastError());
    return {};
  }

  auto paths = deskflow::win32::readDropFilePaths(locked, static_cast<std::size_t>(size));
  ::GlobalUnlock(handle);
  return paths;
}

std::vector<std::string> pathsFromDataObject(IDataObject *dataObject)
{
  if (dataObject == nullptr) {
    return {};
  }

  FORMATETC fmt = hdropFormat();
  STGMEDIUM medium{};
  if (dataObject->GetData(&fmt, &medium) != S_OK) {
    return {};
  }

  std::vector<std::string> paths;
  if (medium.tymed == TYMED_HGLOBAL) {
    paths = pathsFromHGlobal(medium.hGlobal);
  }

  ::ReleaseStgMedium(&medium);
  return paths;
}

} // namespace

MSWindowsDropTarget::MSWindowsDropTarget() = default;

HRESULT __stdcall MSWindowsDropTarget::QueryInterface(REFIID iid, void **object)
{
  if (object == nullptr) {
    return E_POINTER;
  }

  if (iid == IID_IDropTarget || iid == IID_IUnknown) {
    AddRef();
    *object = static_cast<IDropTarget *>(this);
    return S_OK;
  }

  *object = nullptr;
  return E_NOINTERFACE;
}

ULONG __stdcall MSWindowsDropTarget::AddRef()
{
  return static_cast<ULONG>(::InterlockedIncrement(&m_refCount));
}

ULONG __stdcall MSWindowsDropTarget::Release()
{
  const LONG count = ::InterlockedDecrement(&m_refCount);
  if (count == 0) {
    delete this;
    return 0;
  }
  return static_cast<ULONG>(count);
}

HRESULT __stdcall MSWindowsDropTarget::DragEnter(IDataObject *dataObject, DWORD, POINTL, DWORD *effect)
{
  if (effect == nullptr) {
    return E_POINTER;
  }

  m_allowDrop = queryDataObject(dataObject);
  if (m_allowDrop) {
    capturePaths(dataObject);
    *effect = DROPEFFECT_COPY;
  } else {
    m_draggingPaths.clear();
    *effect = DROPEFFECT_NONE;
  }

  return S_OK;
}

HRESULT __stdcall MSWindowsDropTarget::DragOver(DWORD, POINTL, DWORD *effect)
{
  if (effect == nullptr) {
    return E_POINTER;
  }

  *effect = m_allowDrop ? DROPEFFECT_COPY : DROPEFFECT_NONE;
  return S_OK;
}

HRESULT __stdcall MSWindowsDropTarget::DragLeave()
{
  m_allowDrop = false;
  return S_OK;
}

HRESULT __stdcall MSWindowsDropTarget::Drop(IDataObject *dataObject, DWORD, POINTL, DWORD *effect)
{
  if (effect == nullptr) {
    return E_POINTER;
  }

  if (!m_allowDrop) {
    *effect = DROPEFFECT_NONE;
    return S_OK;
  }

  // Prefer the data object handed to Drop: DragEnter's copy can be stale if the
  // source changed the payload between enter and drop.
  capturePaths(dataObject);
  *effect = m_draggingPaths.empty() ? DROPEFFECT_NONE : DROPEFFECT_COPY;
  m_allowDrop = false;
  return S_OK;
}

const std::vector<std::string> &MSWindowsDropTarget::draggingPaths() const
{
  return m_draggingPaths;
}

void MSWindowsDropTarget::clearDraggingPaths()
{
  m_draggingPaths.clear();
}

bool MSWindowsDropTarget::allowDrop() const
{
  return m_allowDrop;
}

bool MSWindowsDropTarget::queryDataObject(IDataObject *dataObject) const
{
  if (dataObject == nullptr) {
    return false;
  }

  FORMATETC fmt = hdropFormat();
  return dataObject->QueryGetData(&fmt) == S_OK;
}

void MSWindowsDropTarget::capturePaths(IDataObject *dataObject)
{
  m_draggingPaths = pathsFromDataObject(dataObject);
  LOG_DEBUG("drop target captured %zu path(s)", m_draggingPaths.size());
}
