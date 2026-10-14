/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025-2026 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcClient.h"

#include "common/IpcToken.h"
#include "common/VersionInfo.h"

#include <QDebug>
#include <QLocalSocket>
#include <QTimer>

#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <vector>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace synergy::gui::ipc {

namespace {

//! Split on the first '=' only. Values are paths and fingerprints, which may
//! themselves contain '='. Splitting every '=' truncates them.
QStringList splitIpcMessage(const QString &message)
{
  const auto separator = message.indexOf(QLatin1Char('='));
  if (separator < 0) {
    return {message};
  }
  return {message.left(separator), message.mid(separator + 1)};
}

#if defined(Q_OS_WIN)
//! Read the user SID out of a process token into \p buffer.
bool readTokenUserSid(HANDLE token, std::vector<unsigned char> &buffer)
{
  DWORD needed = 0;
  GetTokenInformation(token, TokenUser, nullptr, 0, &needed);
  if (needed < sizeof(TOKEN_USER))
    return false;
  buffer.resize(needed);
  return GetTokenInformation(token, TokenUser, buffer.data(), static_cast<DWORD>(buffer.size()), &needed) != 0;
}

//! True when the connected pipe server runs as this process's user or as
//! LocalSystem.
bool verifyServerIsTrusted(const QLocalSocket *socket)
{
  const auto handle = reinterpret_cast<HANDLE>(static_cast<qintptr>(socket->socketDescriptor()));
  if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
    return false;

  ULONG serverPid = 0;
  if (!GetNamedPipeServerProcessId(handle, &serverPid) || serverPid == 0)
    return false;

  HANDLE serverProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, serverPid);
  if (serverProcess == nullptr)
    return false;

  HANDLE serverToken = nullptr;
  const BOOL tokenOpened = OpenProcessToken(serverProcess, TOKEN_QUERY, &serverToken);
  CloseHandle(serverProcess);
  if (!tokenOpened)
    return false;

  std::vector<unsigned char> serverUser;
  const bool serverSidRead = readTokenUserSid(serverToken, serverUser);
  CloseHandle(serverToken);
  if (!serverSidRead)
    return false;
  const auto *serverSid = reinterpret_cast<const TOKEN_USER *>(serverUser.data())->User.Sid;

  HANDLE ownToken = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &ownToken))
    return false;

  std::vector<unsigned char> ownUser;
  const bool ownSidRead = readTokenUserSid(ownToken, ownUser);
  CloseHandle(ownToken);
  if (!ownSidRead)
    return false;
  const auto *ownSid = reinterpret_cast<const TOKEN_USER *>(ownUser.data())->User.Sid;

  if (EqualSid(serverSid, ownSid))
    return true;

  unsigned char systemSid[SECURITY_MAX_SID_SIZE];
  DWORD systemSidSize = sizeof(systemSid);
  return CreateWellKnownSid(WinLocalSystemSid, nullptr, systemSid, &systemSidSize) != 0 &&
         EqualSid(serverSid, systemSid) != 0;
}
#elif defined(Q_OS_LINUX)
//! True when the local socket's server runs as this process's user or as root.
bool verifyServerIsTrusted(const QLocalSocket *socket)
{
  const int fd = static_cast<int>(socket->socketDescriptor());
  if (fd < 0)
    return false;

  struct ucred credentials = {};
  socklen_t length = sizeof(credentials);
  if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &credentials, &length) != 0)
    return false;

  const auto uid = getuid();
  return credentials.uid == uid || credentials.uid == 0;
}
#else
//! macOS and other platforms: verify the peer with getpeereid().
bool verifyServerIsTrusted(const QLocalSocket *socket)
{
  const int fd = static_cast<int>(socket->socketDescriptor());
  if (fd < 0)
    return false;

  uid_t peerUid = 0;
  gid_t peerGid = 0;
  if (getpeereid(fd, &peerUid, &peerGid) != 0)
    return false;

  return peerUid == getuid() || peerUid == 0;
}
#endif

//! Maximum connection attempts per connectToServer() call.
constexpr int kRetryLimit = 3;

//! Base delay between connection attempts; attempt N waits N * this value.
constexpr int kRetryBackoffMs = 200;

} // namespace

// QLocalSocket::errorOccurred was added in Qt 5.15; the pre-5.15 signal is
// error(QLocalSocket::LocalSocketError), which is overloaded against the
// error() const getter — qOverload disambiguates. Resolve once so the connect
// sites stay readable. With QT_NO_KEYWORDS both spellings are plain pointers.
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
static const auto kLocalSocketError = &QLocalSocket::errorOccurred;
#else
static const auto kLocalSocketError = qOverload<QLocalSocket::LocalSocketError>(&QLocalSocket::error);
#endif

IpcClient::IpcClient(QObject *parent, const QString &socketName, const QString &typeName)
    : QObject(parent),
      m_socket{new QLocalSocket(this)},
      m_socketName(socketName), // NOSONAR - Qt memory
      m_typeName(typeName)
{
  connect(m_socket, &QLocalSocket::disconnected, this, &IpcClient::handleDisconnected);
  connect(m_socket, kLocalSocketError, this, &IpcClient::handleErrorOccurred);
  connect(m_socket, &QLocalSocket::readyRead, this, &IpcClient::handleReadyRead);
}

void IpcClient::connectToServer()
{
  if (m_state == State::Connecting) {
    qWarning().noquote() << QStringLiteral("%1 ipc client already connecting to server").arg(m_typeName);
    return;
  }

  if (m_state != State::Unconnected) {
    qDebug().noquote() << QStringLiteral("%1 ipc client not in unconnected state, disconnecting").arg(m_typeName);
    disconnectFromServer();
  }

  if (m_socket->state() != QLocalSocket::UnconnectedState) {
    qWarning().noquote(
    ) << QStringLiteral("%1 ipc client socket not in unconnected state, disconnecting").arg(m_typeName);
    disconnectFromServer();
  }

  m_retryCount = 0;
  attemptConnection();
}

void IpcClient::attemptConnection()
{
  if (m_retryCount >= kRetryLimit) {
    qWarning().noquote() << QStringLiteral("%1 ipc client failed to connect after %2 attempts")
                                .arg(m_typeName, QString::number(kRetryLimit));
    m_state = State::Unconnected;
    Q_EMIT connectionFailed();
    return;
  }

  if (m_retryCount == 0) {
    qDebug().noquote() << QStringLiteral("%1 ipc client connecting to server: %2").arg(m_typeName, m_socketName);
  } else {
    qDebug().noquote() << QStringLiteral("%1 ipc client retrying connection, attempt: %2")
                              .arg(m_typeName, QString::number(m_retryCount + 1));
  }

  m_state = State::Connecting;
  m_retryCount++;

  // Connection failures are handled by handleErrorOccurred, which is
  // connected once in the constructor. Only the success path needs a
  // one-shot handler, so a stale connection lambda can never accumulate.
  connect(
      m_socket, &QLocalSocket::connected, this,
      [this] {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        disconnect(m_socket, &QLocalSocket::connected, this, nullptr);
#endif
        // A low-privilege process can take the pipe name before the real
        // server listens.  Never hand the hello token to such a squatter:
        // only accept a server that runs with the same identity as the
        // daemon/core (this user, SYSTEM on Windows, root on Unix).
        if (!verifyServerIsTrusted(m_socket)) {
          qWarning().noquote(
          ) << QStringLiteral("%1 ipc server failed peer verification, refusing to send hello").arg(m_typeName);
          disconnectFromServer();
          Q_EMIT connectionFailed();
          return;
        }
        // Divergence from upstream: kVersion already carries the build metadata; don't re-append the git sha.
        const auto versionId = QString::fromUtf8(kVersion);
        const auto token = synergy::readIpcToken(m_socketName);
        m_socket->write(QStringLiteral("hello=%1 %2\n").arg(versionId, token).toUtf8());
        qDebug().noquote() << QStringLiteral("%1 ipc client sent hello with version: %2").arg(m_typeName, versionId);
      },
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      Qt::SingleShotConnection
#else
      Qt::AutoConnection
#endif
  );

  m_socket->connectToServer(m_socketName);
}

void IpcClient::scheduleRetry()
{
  // linear backoff (0ms, 200ms, 400ms) so a missing server does not
  // burn all attempts in a single event-loop turn
  QTimer::singleShot(kRetryBackoffMs * m_retryCount, this, &IpcClient::attemptConnection);
}

void IpcClient::disconnectFromServer()
{
  m_state = State::Disconnecting;
  qDebug().noquote() << QStringLiteral("%1 ipc client disconnecting from server").arg(m_typeName);
  m_socket->disconnectFromServer();
  m_state = State::Unconnected;
}

void IpcClient::handleDisconnected()
{
  if (m_state == State::Connecting) {
    return;
  }

  qDebug().noquote() << QStringLiteral("%1 ipc client disconnected from server").arg(m_typeName);
  const auto wasConnected = m_state == State::Connected;
  m_state = State::Unconnected;

  if (wasConnected) {
    Q_EMIT connectionFailed();
  }
}

void IpcClient::handleErrorOccurred()
{
  if (m_state == State::Connecting) {
    qWarning().noquote(
    ) << QStringLiteral("%1 ipc client failed to connect: %2").arg(m_typeName, m_socket->errorString());
    m_socket->disconnectFromServer();
    m_state = State::Unconnected;
    scheduleRetry();
    return;
  }

  qWarning().noquote() << QStringLiteral("%1 ipc client error: %2").arg(m_typeName, m_socket->errorString());

  if (m_state == State::Connected) {
    disconnectFromServer();
    Q_EMIT connectionFailed();
  }
}

void IpcClient::handleReadyRead()
{
  QByteArray data = m_readBuffer + m_socket->readAll();
  m_readBuffer.clear();

  while (data.contains('\n')) {
    const auto index = data.indexOf('\n');
    const auto message = QString::fromUtf8(data.left(index));
    data.remove(0, index + 1);

    qDebug().noquote() << QStringLiteral("%1 ipc client message: %2").arg(m_typeName, message);
    const auto parts = splitIpcMessage(message);
    if (parts.isEmpty()) {
      qWarning().noquote() << QStringLiteral("%1 ipc client got invalid message: %2").arg(m_typeName, message);
      continue;
    }

    if (m_state == State::Connecting) {
      handleHandshakeMessage(parts);
      continue;
    }

    if (parts.at(0) == QStringLiteral("bye")) {
      qDebug().noquote() << QStringLiteral("%1 ipc server is shutting down").arg(m_typeName);
      disconnectFromServer();
      Q_EMIT serverShutdown();
      return;
    }

    processCommand(parts.at(0), parts);
  }

  if (!data.isEmpty()) {
    m_readBuffer = data;
  }
}

void IpcClient::handleHandshakeMessage(const QStringList &parts)
{
  if (parts.at(0) == QStringLiteral("error")) {
    const auto detail = parts.size() >= 2 ? parts.at(1) : QStringLiteral("unknown");
    qCritical().noquote() << QStringLiteral("%1 ipc server rejected connection: %2").arg(m_typeName, detail);
    disconnectFromServer();
    Q_EMIT connectionFailed();
    return;
  }

  // Divergence from upstream: kVersion already carries the build metadata; don't re-append the git sha.
  const auto versionId = QString::fromUtf8(kVersion);

  if (parts.at(0) == QStringLiteral("versionMismatch")) {
    const auto serverVersion = parts.size() >= 2 ? parts.at(1) : QStringLiteral("unknown");
    qWarning().noquote(
    ) << QStringLiteral("%1 ipc version mismatch (client: %2, server: %3)").arg(m_typeName, versionId, serverVersion);
    m_state = State::Connected;
    Q_EMIT versionMismatch();
    return;
  }

  if (parts.at(0) != QStringLiteral("hello")) {
    return;
  }

  if (parts.size() < 2) {
    qCritical().noquote() << QStringLiteral("%1 ipc server hello missing version").arg(m_typeName);
    disconnectFromServer();
    Q_EMIT connectionFailed();
    return;
  }

  m_state = State::Connected;
  qDebug().noquote() << QStringLiteral("%1 ipc client connected").arg(m_typeName);
  Q_EMIT connected();
}

void IpcClient::sendMessage(const QString &message)
{
  if (m_state != State::Connected) {
    qWarning().noquote() << QStringLiteral("%1 cannot send command, ipc client not connected").arg(m_typeName);
    return;
  }

  m_socket->write(message.toUtf8() + "\n");
  qDebug().noquote() << QStringLiteral("%1 ipc client sent message: %2").arg(m_typeName, message);
}

} // namespace synergy::gui::ipc
