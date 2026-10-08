/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 TuPig
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "QtSocketFactoryTests.h"

#include "arch/Arch.h"
#include "base/Event.h"
#include "base/EventQueue.h"
#include "base/EventTypes.h"
#include "common/Settings.h"
#include "net/FingerprintDatabase.h"
#include "net/IDataSocket.h"
#include "net/IListenSocket.h"
#include "net/NetworkAddress.h"
#include "net/QtSocketFactory.h"
#include "net/SecureUtils.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSslCertificate>
#include <QTimer>

// End-to-end coverage for the Qt network adapters that now back every
// connection: real loopback sockets, the IEventQueue event contract, the TLS
// handshake and the TOFU fingerprint database. Settings are redirected to a
// temporary directory (via portable mode) so the certificate and trust
// databases never touch the machine's real configuration.

QtSocketFactoryTests::~QtSocketFactoryTests()
{
  delete m_events;
}

void QtSocketFactoryTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);

  // The event queue reaches for the ARCH singleton (signal handlers), which
  // only the application normally installs.
  static Arch arch;
#if defined(Q_OS_WIN)
  arch.init();
#endif

  const QString dir = QDir::tempPath() + "/synergy-qt-transport-tests";
  QDir().mkpath(dir);
  m_certPath = dir + "/test-cert.pem";
  synergy::generatePemSelfSignedCert(m_certPath, 2048);
  QVERIFY(QFile::exists(m_certPath));

  // Force portable mode so tlsTrustedServersDb()/tlsTrustedClientsDb() resolve
  // under our control instead of the real user profile.
  m_markerFile = Settings::portableSettingsFile();
  QDir().mkpath(QFileInfo(m_markerFile).absolutePath());
  QFile marker(m_markerFile);
  QVERIFY(marker.open(QIODevice::WriteOnly));
  marker.close();

  Settings::setSettingsFile(dir + "/test-settings.conf");
  Settings::setStateFile(dir + "/test-state.conf");
  Settings::setValue(Settings::Security::Certificate, m_certPath);
}

void QtSocketFactoryTests::cleanupTestCase()
{
  QFile::remove(m_markerFile);
  QDir(QFileInfo(m_markerFile).absolutePath()).rmdir(".");
  QDir(QDir::tempPath() + "/synergy-qt-transport-tests").removeRecursively();
}

void QtSocketFactoryTests::init()
{
  qunsetenv("USE_LEGACY_NETWORK");
  delete m_events;
  m_events = new EventQueue();
}

void QtSocketFactoryTests::cleanup()
{
  qunsetenv("USE_LEGACY_NETWORK");
  delete m_events;
  m_events = nullptr;
}

bool QtSocketFactoryTests::pump(const std::function<bool()> &done, int timeoutMs)
{
  QElapsedTimer timer;
  timer.start();

  // loop() runs the queue on this thread and pumps Qt inside its wait (so
  // QSslSocket/QTcpSocket signals fire between dispatched events). A checker
  // timer posts Quit once the predicate holds or the budget is spent.
  bool quitPosted = false;
  QTimer checker;
  const auto connection = QObject::connect(&checker, &QTimer::timeout, [&] {
    if (!quitPosted && (done() || timer.elapsed() >= timeoutMs)) {
      // Exactly one Quit: posting more would leave a stale Quit in the queue
      // that ends the next pump() immediately.
      quitPosted = true;
      m_events->addEvent(Event(synergy::EventTypes::Quit));
    }
  });
  checker.start(10);

  m_events->loop();

  QObject::disconnect(connection);
  return done();
}

void QtSocketFactoryTests::legacyNetworkFlagHonorsEnv()
{
  QVERIFY(!shouldUseLegacyNetwork());

  qputenv("USE_LEGACY_NETWORK", "1");
  QVERIFY(shouldUseLegacyNetwork());

  qputenv("USE_LEGACY_NETWORK", "0");
  QVERIFY(!shouldUseLegacyNetwork());
}

void QtSocketFactoryTests::plaintextRoundTrip()
{
  QtSocketFactory factory(m_events);

  auto listener = factory.createListen(IArchNetwork::AddressFamily::INet, SecurityLevel::PlainText);
  auto *qtListener = dynamic_cast<QtListenSocket *>(listener.get());
  QVERIFY(qtListener != nullptr);

  std::unique_ptr<IDataSocket> serverSocket;
  m_events->addHandler(synergy::EventTypes::ListenSocketConnecting, listener->getEventTarget(), [&](const auto &) {
    if (!serverSocket) {
      serverSocket = listener->accept();
    }
  });

  listener->bind(NetworkAddress("127.0.0.1", 0));
  const quint16 port = qtListener->serverPort();
  QVERIFY(port != 0);

  auto client = factory.create(IArchNetwork::AddressFamily::INet, SecurityLevel::PlainText);
  bool connected = false;
  m_events->addHandler(synergy::EventTypes::DataSocketConnected, client->getEventTarget(), [&](const auto &) {
    connected = true;
  });
  client->connect(NetworkAddress("127.0.0.1", port));

  QVERIFY(pump([&] { return connected && serverSocket != nullptr; }));

  // client -> server
  std::string received;
  m_events->addHandler(synergy::EventTypes::StreamInputReady, serverSocket->getEventTarget(), [&](const auto &) {
    char buffer[64];
    const auto n = serverSocket->read(buffer, sizeof(buffer));
    received.append(buffer, n);
  });
  const char request[] = "hello synergy";
  client->write(request, 13);
  QVERIFY(pump([&] { return received.size() == 13; }));
  QCOMPARE(received, std::string("hello synergy"));

  // server -> client
  std::string response;
  m_events->addHandler(synergy::EventTypes::StreamInputReady, client->getEventTarget(), [&](const auto &) {
    char buffer[64];
    const auto n = client->read(buffer, sizeof(buffer));
    response.append(buffer, n);
  });
  serverSocket->write("pong", 4);
  QVERIFY(pump([&] { return response.size() == 4; }));
  QCOMPARE(response, std::string("pong"));
}

void QtSocketFactoryTests::tlsRejectsUntrustedServer()
{
  QtSocketFactory factory(m_events);

  auto listener = factory.createListen(IArchNetwork::AddressFamily::INet, SecurityLevel::Encrypted);
  auto *qtListener = dynamic_cast<QtListenSocket *>(listener.get());
  QVERIFY(qtListener != nullptr);

  // No trusted-servers database: the client must reject the server.
  QFile::remove(Settings::tlsTrustedServersDb());

  std::unique_ptr<IDataSocket> serverSocket;
  bool serverAccepted = false;
  m_events->addHandler(synergy::EventTypes::ListenSocketConnecting, listener->getEventTarget(), [&](const auto &) {
    if (!serverSocket) {
      serverSocket = listener->accept();
      if (serverSocket) {
        m_events->addHandler(
            synergy::EventTypes::ClientListenerAccepted, serverSocket->getEventTarget(),
            [&](const auto &) { serverAccepted = true; }
        );
      }
    }
  });

  listener->bind(NetworkAddress("127.0.0.1", 0));
  const quint16 port = qtListener->serverPort();
  QVERIFY(port != 0);

  auto client = factory.create(IArchNetwork::AddressFamily::INet, SecurityLevel::Encrypted);
  bool clientConnected = false;
  bool connectFailed = false;
  m_events->addHandler(synergy::EventTypes::DataSocketSecureConnected, client->getEventTarget(), [&](const auto &) {
    clientConnected = true;
  });
  m_events->addHandler(synergy::EventTypes::DataSocketConnectionFailed, client->getEventTarget(), [&](const auto &) {
    connectFailed = true;
  });

  client->connect(NetworkAddress("127.0.0.1", port));

  QVERIFY(pump([&] { return connectFailed || clientConnected; }, 20000));
  QVERIFY(connectFailed);
  QVERIFY(!clientConnected);
  // (serverAccepted is not asserted here: when the client aborts right after
  // the handshake, QSslServer may never surface the connection at all.)
}

void QtSocketFactoryTests::tlsTrustedServerRoundTrip()
{
  QtSocketFactory factory(m_events);

  // Trust the generated identity as if the user had accepted it (TOFU).
  const QList<QSslCertificate> certs = QSslCertificate::fromPath(m_certPath);
  QVERIFY(!certs.isEmpty());
  FingerprintDatabase db;
  db.addTrusted({QCryptographicHash::Sha256, certs.first().digest(QCryptographicHash::Sha256)});
  const QString dbPath = Settings::tlsTrustedServersDb();
  QDir().mkpath(QFileInfo(dbPath).absolutePath());
  QVERIFY(db.write(dbPath));

  auto listener = factory.createListen(IArchNetwork::AddressFamily::INet, SecurityLevel::Encrypted);
  auto *qtListener = dynamic_cast<QtListenSocket *>(listener.get());
  QVERIFY(qtListener != nullptr);

  std::unique_ptr<IDataSocket> serverSocket;
  bool serverAccepted = false;
  m_events->addHandler(synergy::EventTypes::ListenSocketConnecting, listener->getEventTarget(), [&](const auto &) {
    if (!serverSocket) {
      serverSocket = listener->accept();
      if (serverSocket) {
        m_events->addHandler(
            synergy::EventTypes::ClientListenerAccepted, serverSocket->getEventTarget(),
            [&](const auto &) { serverAccepted = true; }
        );
      }
    }
  });

  listener->bind(NetworkAddress("127.0.0.1", 0));
  const quint16 port = qtListener->serverPort();
  QVERIFY(port != 0);

  auto client = factory.create(IArchNetwork::AddressFamily::INet, SecurityLevel::Encrypted);
  bool clientConnected = false;
  m_events->addHandler(synergy::EventTypes::DataSocketSecureConnected, client->getEventTarget(), [&](const auto &) {
    clientConnected = true;
  });

  client->connect(NetworkAddress("127.0.0.1", port));

  QVERIFY(pump([&] { return clientConnected && serverAccepted && serverSocket != nullptr; }, 20000));

  // Prove the encrypted channel carries data in both directions.
  std::string received;
  m_events->addHandler(synergy::EventTypes::StreamInputReady, serverSocket->getEventTarget(), [&](const auto &) {
    char buffer[64];
    const auto n = serverSocket->read(buffer, sizeof(buffer));
    received.append(buffer, n);
  });
  client->write("tls ping", 8);
  QVERIFY(pump([&] { return received.size() == 8; }));
  QCOMPARE(received, std::string("tls ping"));

  std::string response;
  m_events->addHandler(synergy::EventTypes::StreamInputReady, client->getEventTarget(), [&](const auto &) {
    char buffer[64];
    const auto n = client->read(buffer, sizeof(buffer));
    response.append(buffer, n);
  });
  serverSocket->write("tls pong", 8);
  QVERIFY(pump([&] { return response.size() == 8; }));
  QCOMPARE(response, std::string("tls pong"));
}

QTEST_MAIN(QtSocketFactoryTests)
