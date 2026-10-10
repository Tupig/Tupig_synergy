/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig Synergy Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcToken.h"

#include "common/Settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>

#include <string>
#include <vector>

#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <aclapi.h>
#include <sddl.h>
#include <wtsapi32.h>
#endif

namespace synergy {
namespace {

#if defined(Q_OS_WIN)
std::wstring sidText(HANDLE token)
{
  DWORD needed = 0;
  GetTokenInformation(token, TokenUser, nullptr, 0, &needed);
  if (needed < sizeof(TOKEN_USER))
    return {};

  std::vector<unsigned char> buffer(needed);
  if (!GetTokenInformation(token, TokenUser, buffer.data(), static_cast<DWORD>(buffer.size()), &needed))
    return {};

  LPWSTR text = nullptr;
  const auto *user = reinterpret_cast<const TOKEN_USER *>(buffer.data());
  if (!ConvertSidToStringSidW(user->User.Sid, &text) || text == nullptr)
    return {};

  std::wstring sid(text);
  LocalFree(text);
  return sid;
}

std::wstring currentProcessSid()
{
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    return {};
  const auto sid = sidText(token);
  CloseHandle(token);
  return sid;
}

std::wstring consoleUserSid()
{
  const DWORD sessionId = WTSGetActiveConsoleSessionId();
  if (sessionId == 0xFFFFFFFF)
    return {};

  HANDLE token = nullptr;
  if (!WTSQueryUserToken(sessionId, &token))
    return {};
  const auto sid = sidText(token);
  CloseHandle(token);
  return sid;
}

void appendAce(std::wstring &sddl, const wchar_t *rights, const std::wstring &sid, std::vector<std::wstring> &seen)
{
  if (sid.empty())
    return;
  for (const auto &existing : seen) {
    if (existing == sid)
      return;
  }
  seen.push_back(sid);
  sddl += L"(A;;";
  sddl += rights;
  sddl += L";;;";
  sddl += sid;
  sddl += L")";
}

std::wstring tokenFileSddl()
{
  // SYSTEM and Administrators can replace the token. The active console user
  // can read it. Interactive (IU) is absent. The writing process is added only
  // when the console user cannot be resolved (a desktop core is not SYSTEM, so
  // WTSQueryUserToken fails); otherwise that process would be a fourth principal.
  std::wstring sddl = L"D:P";
  std::vector<std::wstring> seen;
  appendAce(sddl, L"GA", L"SY", seen);
  appendAce(sddl, L"GA", L"BA", seen);
  const auto process = currentProcessSid();
  const auto console = consoleUserSid();
  if (!console.empty()) {
    appendAce(sddl, console == process ? L"GA" : L"GR", console, seen);
  } else {
    appendAce(sddl, L"GA", process, seen);
  }
  return sddl;
}
#endif

bool restrictTokenFile(const QString &path)
{
#if defined(Q_OS_WIN)
  if (!QFileInfo::exists(path))
    return false;

  const auto sddl = tokenFileSddl();
  PSECURITY_DESCRIPTOR descriptor = nullptr;
  if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &descriptor, nullptr))
    return false;

  BOOL present = FALSE;
  BOOL defaulted = FALSE;
  PACL dacl = nullptr;
  if (!GetSecurityDescriptorDacl(descriptor, &present, &dacl, &defaulted) || !present || dacl == nullptr) {
    LocalFree(descriptor);
    return false;
  }

  // Named string, not a temporary c_str(): SetNamedSecurityInfoW keeps the pointer.
  std::wstring nativePath = path.toStdWString();
  const DWORD status = SetNamedSecurityInfoW(
      nativePath.data(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr,
      nullptr, dacl, nullptr
  );
  LocalFree(descriptor);
  return status == ERROR_SUCCESS;
#else
  if (!QFileInfo::exists(path))
    return false;
  return QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif
}

} // namespace

QString ipcTokenPath(const QString &serverName)
{
  // Installed Windows keeps this in ProgramData, which both the SYSTEM service
  // and the interactive GUI can use. The file DACL, not the directory, limits
  // who can read the token.
  return QDir(Settings::settingsPath()).filePath(serverName + QStringLiteral(".ipc-token"));
}

QString generateIpcToken()
{
  QByteArray bytes(32, Qt::Uninitialized);
  for (int i = 0; i < bytes.size(); ++i)
    bytes[i] = static_cast<char>(QRandomGenerator::system()->bounded(256));
  return QString::fromLatin1(bytes.toHex());
}

QString readIpcToken(const QString &serverName)
{
  QFile file(ipcTokenPath(serverName));
  if (!file.open(QIODevice::ReadOnly))
    return {};
  return QString::fromUtf8(file.readAll()).trimmed();
}

bool writeIpcToken(const QString &serverName, const QString &token)
{
  if (token.isEmpty())
    return false;

  const auto path = ipcTokenPath(serverName);
  if (!QDir().mkpath(QFileInfo(path).absolutePath()))
    return false;

  const auto bytes = token.toUtf8();
#if defined(Q_OS_WIN)
  // Replace any previous file so CreateFile applies the DACL at creation.
  // Opening an existing file and restricting it afterwards leaves a window
  // where the inherited DACL still allows every interactive user to read.
  if (QFileInfo::exists(path) && !QFile::remove(path))
    return false;

  const auto sddl = tokenFileSddl();
  PSECURITY_DESCRIPTOR descriptor = nullptr;
  if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &descriptor, nullptr))
    return false;

  SECURITY_ATTRIBUTES attributes{};
  attributes.nLength = sizeof(attributes);
  attributes.lpSecurityDescriptor = descriptor;
  attributes.bInheritHandle = FALSE;
  const auto nativePath = path.toStdWString();
  const HANDLE handle =
      CreateFileW(nativePath.c_str(), GENERIC_WRITE, 0, &attributes, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  LocalFree(descriptor);
  if (handle == INVALID_HANDLE_VALUE)
    return false;

  DWORD written = 0;
  const BOOL wrote = WriteFile(handle, bytes.constData(), static_cast<DWORD>(bytes.size()), &written, nullptr);
  CloseHandle(handle);
  if (!wrote || written != static_cast<DWORD>(bytes.size()) || !restrictTokenFile(path)) {
    QFile::remove(path);
    return false;
  }
  return true;
#else
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return false;
  if (!file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner) || file.write(bytes) != bytes.size()) {
    file.close();
    QFile::remove(path);
    return false;
  }
  file.close();
  return true;
#endif
}

bool refreshIpcTokenAccess(const QString &serverName)
{
  return restrictTokenFile(ipcTokenPath(serverName));
}

} // namespace synergy
