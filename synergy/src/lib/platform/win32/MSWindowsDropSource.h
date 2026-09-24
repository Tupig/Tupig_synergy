/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <oleidl.h>

//! Minimal IDropSource used with CF_HDROP when offering received files to Explorer.
class MSWindowsDropSource : public IDropSource
{
public:
  MSWindowsDropSource() = default;

  HRESULT __stdcall QueryInterface(REFIID iid, void **object) override;
  ULONG __stdcall AddRef() override;
  ULONG __stdcall Release() override;

  HRESULT __stdcall QueryContinueDrag(BOOL escapePressed, DWORD keyState) override;
  HRESULT __stdcall GiveFeedback(DWORD effect) override;

private:
  long m_refCount = 1;
};

//! IDataObject wrapping a CF_HDROP HGLOBAL built from UTF-8 paths.
class MSWindowsDropDataObject : public IDataObject
{
public:
  explicit MSWindowsDropDataObject(const std::vector<std::string> &utf8Paths);
  ~MSWindowsDropDataObject();

  HRESULT __stdcall QueryInterface(REFIID iid, void **object) override;
  ULONG __stdcall AddRef() override;
  ULONG __stdcall Release() override;

  HRESULT __stdcall GetData(FORMATETC *format, STGMEDIUM *medium) override;
  HRESULT __stdcall GetDataHere(FORMATETC *, STGMEDIUM *) override;
  HRESULT __stdcall QueryGetData(FORMATETC *format) override;
  HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC *, FORMATETC *out) override;
  HRESULT __stdcall SetData(FORMATETC *, STGMEDIUM *, BOOL) override;
  HRESULT __stdcall EnumFormatEtc(DWORD direction, IEnumFORMATETC **enumerator) override;
  HRESULT __stdcall DAdvise(FORMATETC *, DWORD, IAdviseSink *, DWORD *) override;
  HRESULT __stdcall DUnadvise(DWORD) override;
  HRESULT __stdcall EnumDAdvise(IEnumSTATDATA **) override;

private:
  bool matchesHdrop(const FORMATETC *format) const;

  long m_refCount = 1;
  HGLOBAL m_hdrop = nullptr;
};
