/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <oleidl.h>

//! OLE drop target that captures `CF_HDROP` paths via Win32DropData.
/*!
Restored from the skeleton removed in 5365e34f0, but the CF_HDROP walk itself
lives in Win32DropData: this class only speaks COM (`IDropTarget` / `IUnknown`)
and hands the locked block to the tested parser.

`DragEnter` records the paths; `Drop` accepts a copy effect so a real drop onto
our window also works. The old Escape + fake mouse-up capture hack is
deliberately not restored here.
*/
class MSWindowsDropTarget : public IDropTarget
{
public:
  MSWindowsDropTarget();
  ~MSWindowsDropTarget() = default;

  // IUnknown
  HRESULT __stdcall QueryInterface(REFIID iid, void **object) override;
  ULONG __stdcall AddRef() override;
  ULONG __stdcall Release() override;

  // IDropTarget
  HRESULT __stdcall DragEnter(IDataObject *dataObject, DWORD keyState, POINTL point, DWORD *effect) override;
  HRESULT __stdcall DragOver(DWORD keyState, POINTL point, DWORD *effect) override;
  HRESULT __stdcall DragLeave() override;
  HRESULT __stdcall Drop(IDataObject *dataObject, DWORD keyState, POINTL point, DWORD *effect) override;

  //! Paths captured by the most recent DragEnter/Drop that carried CF_HDROP.
  [[nodiscard]] const std::vector<std::string> &draggingPaths() const;

  //! Clear the captured paths (e.g. after a transfer has consumed them).
  void clearDraggingPaths();

  //! Whether the current drag offers CF_HDROP.
  [[nodiscard]] bool allowDrop() const;

private:
  bool queryDataObject(IDataObject *dataObject) const;
  void capturePaths(IDataObject *dataObject);

  long m_refCount = 1;
  bool m_allowDrop = false;
  std::vector<std::string> m_draggingPaths;
};
