// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QDebug>
#include <QSignalSpy>
#include <QTimer>

#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslKey>
#include "private/qtlsbackend_p.h"

#include <QtTest/private/qtesthelpers_p.h>

class tst_QSslServer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testOneSuccessfulConnection();
    void testSelfSignedCertificateRejectedByServer();
    void testSelfSignedCertificateRejectedByClient();
#if QT_CONFIG(openssl)
    void testHandshakeInterruptedOnError();
    void testPreSharedKeyAuthenticationRequired();
#endif
    void plaintextClient();
    void quietClient();
    void twoGoodAndManyBadClients();

private:
    QString testDataDir;
    bool isTestingOpenSsl = false;
    QSslConfiguration selfSignedClientQSslConfiguration();
    QSslConfiguration selfSignedServerQSslConfiguration();
    QSslConfiguration createQSslConfiguration(QString keyFileName, QString certificateFileName);
};

class SslServerSpy : public QObject
{
    Q_OBJECT

public:
    SslServerSpy(QSslConfiguration &configuration);

    QSslServer server;
    QSignalSpy sslErrorsSpy;
    QSignalSpy peerVerifyErrorSpy;
    QSignalSpy errorOccurredSpy;
    QSignalSpy pendingConnectionAvailableSpy;
    QSignalSpy preSharedKeyAuthenticationRequiredSpy;
    QSignalSpy alertSentSpy;
    QSignalSpy alertReceivedSpy;
    QSignalSpy handshakeInterruptedOnErrorSpy;
    QSignalSpy startedEncryptionHandshakeSpy;
};

SslServerSpy::SslServerSpy(QSslConfiguration &configuration)
    : server(),
      sslErrorsSpy(&server, &QSslServer::sslErrors),
      peerVerifyErrorSpy(&server, &QSslServer::peerVerifyError),
      errorOccurredSpy(&server, &QSslServer::errorOccurred),
      pendingConnectionAvailableSpy(&server, &QSslServer::pendingConnectionAvailable),
      preSharedKeyAuthenticationRequiredSpy(&server,
                                            &QSslServer::preSharedKeyAuthenticationRequired),
      alertSentSpy(&server, &QSslServer::alertSent),
      alertReceivedSpy(&server, &QSslServer::alertReceived),
      handshakeInterruptedOnErrorSpy(&server, &QSslServer::handshakeInterruptedOnError),
      startedEncryptionHandshakeSpy(&server, &QSslServer::startedEncryptionHandshake)
{
    server.setSslConfiguration(configuration);
}

void tst_QSslServer::initTestCase()
{
    testDataDir = QFileInfo(QFINDTESTDATA("certs")).absolutePath();
    if (testDataDir.isEmpty())
        testDataDir = QCoreApplication::applicationDirPath();
    if (!testDataDir.endsWith(QLatin1String("/")))
        testDataDir += QLatin1String("/");

    const QString openSslBackend = QTlsBackend::builtinBackendNames[QTlsBackend::nameIndexOpenSSL];
    const auto &tlsBackends = QSslSocket::availableBackends();
    if (tlsBackends.contains(openSslBackend)) {
        isTestingOpenSsl = true;
    }
}

QSslConfiguration tst_QSslServer::selfSignedClientQSslConfiguration()
{
    return createQSslConfiguration(testDataDir + "certs/selfsigned-client.key",
                                   testDataDir + "certs/selfsigned-client.crt");
}

QSslConfiguration tst_QSslServer::selfSignedServerQSslConfiguration()
{
    return createQSslConfiguration(testDataDir + "certs/selfsigned-server.key",
                                   testDataDir + "certs/selfsigned-server.crt");
}

QSslConfiguration tst_QSslServer::createQSslConfiguration(QString keyFileName,
                                                          QString certificateFileName)
{
    QSslConfiguration configuration(QSslConfiguration::defaultConfiguration());

    QFile keyFile(keyFileName);
    if (keyFile.open(QIODevice::ReadOnly)) {
        QSslKey key(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        if (!key.isNull()) {
            configuration.setPrivateKey(key);
        } else {
            qCritical() << "Could not parse key: " << keyFileName;
        }
    } else {
        qCritical() << "Could not find key: " << keyFileName;
    }

    QList<QSslCertificate> localCert = QSslCertificate::fromPath(certificateFileName);
    if (!localCert.isEmpty() && !localCert.first().isNull()) {
        configuration.setLocalCertificate(localCert.first());
    } else {
        qCritical() << "Could not find certificate: " << certificateFileName;
    }
    return configuration;
}

void tst_QSslServer::testOneSuccessfulConnection()
{
    if (QTestPrivate::isSecureTransportBlockingTest())
        QSKIP("SecureTransport will block this test while requesting keychain access");
    // Setup server
    QSslConfiguration serverConfiguration = selfSignedServerQSslConfiguration();
    SslServerSpy server(serverConfiguration);
    QVERIFY(server.server.listen());

    // Check that all signal spys are valid
    QVERIFY(server.sslErrorsSpy.isValid());
    QVERIFY(server.peerVerifyErrorSpy.isValid());
    QVERIFY(server.errorOccurredSpy.isValid());
    QVERIFY(server.pendingConnectionAvailableSpy.isValid());
    QVERIFY(server.preSharedKeyAuthenticationRequiredSpy.isValid());
    QVERIFY(server.alertSentSpy.isValid());
    QVERIFY(server.alertReceivedSpy.isValid());
    QVERIFY(server.handshakeInterruptedOnErrorSpy.isValid());
    QVERIFY(server.startedEncryptionHandshakeSpy.isValid());

    // Check that no connections has occurred
    QCOMPARE(server.sslErrorsSpy.size(), 0);
    QCOMPARE(server.peerVerifyErrorSpy.size(), 0);
    QCOMPARE(server.errorOccurredSpy.size(), 0);
    QCOMPARE(server.pendingConnectionAvailableSpy.size(), 0);
    QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.size(), 0);
    QCOMPARE(server.alertSentSpy.size(), 0);
    QCOMPARE(server.alertReceivedSpy.size(), 0);
    QCOMPARE(server.handshakeInterruptedOnErrorSpy.size(), 0);
    QCOMPARE(server.startedEncryptionHandshakeSpy.size(), 0);

    // Connect client
    QSslSocket client;
    QSslConfiguration clientConfiguration = QSslConfiguration::defaultConfiguration();
    client.setSslConfiguration(clientConfiguration);
    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(),
                                  server.server.serverPort());

    // Type of certificate error to expect
    const auto certificateError =
            isTestingOpenSsl ? QSslError::SelfSignedCertificate : QSslError::CertificateUntrusted;
    // Expected errors
    connect(&client, &QSslSocket::sslErrors,
            [&certificateError, &client](const QList<QSslError> &errors) {
                QCOMPARE(errors.size(), 2);
                for (auto error : errors) {
                    QVERIFY(error.error() == certificateError
                            || error.error() == QSslError::HostNameMismatch);
                }
                client.ignoreSslErrors();
            });

    QEventLoop loop;
    int waitFor = 2;
    connect(&client, &QSslSocket::encrypted, [&loop, &waitFor]() {
        if (!--waitFor)
            loop.quit();
    });
    connect(&server.server, &QTcpServer::pendingConnectionAvailable, [&loop, &waitFor]() {
        if (!--waitFor)
            loop.quit();
    });
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();

    // Check that one encrypted connection has occurred without error
    QCOMPARE(server.sslErrorsSpy.size(), 0);
    QCOMPARE(server.peerVerifyErrorSpy.size(), 0);
    QCOMPARE(server.errorOccurredSpy.size(), 0);
    QCOMPARE(server.pendingConnectionAvailableSpy.size(), 1);
    QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.size(), 0);
    QCOMPARE(server.alertSentSpy.size(), 0);
    QCOMPARE(server.alertReceivedSpy.size(), 0);
    QCOMPARE(server.handshakeInterruptedOnErrorSpy.size(), 0);
    QCOMPARE(server.startedEncryptionHandshakeSpy.size(), 1);

    // Check client socket
    QVERIFY(client.isEncrypted());
    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
}

void tst_QSslServer::testSelfSignedCertificateRejectedByServer()
{
    if (QTestPrivate::isSecureTransportBlockingTest())
        QSKIP("SecureTransport will block this test while requesting keychain access");
    // Set up server that verifies client
    QSslConfiguration serverConfiguration = selfSignedServerQSslConfiguration();
    serverConfiguration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    SslServerSpy server(serverConfiguration);
    QVERIFY(server.server.listen());

    // Connect client
    QSslSocket client;
    QSslConfiguration clientConfiguration = selfSignedClientQSslConfiguration();
    clientConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    client.setSslConfiguration(clientConfiguration);
    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(),
                                  server.server.serverPort());

    QEventLoop loop;
    QObject::connect(&client, SIGNAL(disconnected()), &loop, SLOT(quit()));
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();

    // Check that one encrypted connection has failed
    QCOMPARE(server.sslErrorsSpy.size(), 1);
    QCOMPARE(server.peerVerifyErrorSpy.size(), 1);
    QCOMPARE(server.errorOccurredSpy.size(), 1);
    QCOMPARE(server.pendingConnectionAvailableSpy.size(), 0);
    QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.size(), 0);
    QCOMPARE(server.alertSentSpy.size(),
             isTestingOpenSsl ? 1 : 0); // OpenSSL only signal
    QCOMPARE(server.alertReceivedSpy.size(), 0);
    QCOMPARE(server.handshakeInterruptedOnErrorSpy.size(), 0);
    QCOMPARE(server.startedEncryptionHandshakeSpy.size(), 1);

    // Type of certificate error to expect
    const auto certificateError =
            isTestingOpenSsl ? QSslError::SelfSignedCertificate : QSslError::CertificateUntrusted;

    // Check the sslErrorsSpy
    const auto sslErrorsSpyErrors =
            qvariant_cast<QList<QSslError>>(std::as_const(server.sslErrorsSpy).first()[1]);
    QCOMPARE(sslErrorsSpyErrors.size(), 1);
    QCOMPARE(sslErrorsSpyErrors.first().error(), certificateError);

    // Check the peerVerifyErrorSpy
    const auto peerVerifyErrorSpyError =
            qvariant_cast<QSslError>(std::as_const(server.peerVerifyErrorSpy).first()[1]);
    QCOMPARE(peerVerifyErrorSpyError.error(), certificateError);

    // Check client socket
    QVERIFY(!client.isEncrypted());
    QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);
}

void tst_QSslServer::testSelfSignedCertificateRejectedByClient()
{
    if (QTestPrivate::isSecureTransportBlockingTest())
        QSKIP("SecureTransport will block this test while requesting keychain access");
    // Set up server without verification of client
    QSslConfiguration serverConfiguration = selfSignedServerQSslConfiguration();
    SslServerSpy server(serverConfiguration);
    QVERIFY(server.server.listen());

    // Connect client that authenticates server
    QSslSocket client;
    QSslConfiguration clientConfiguration = selfSignedClientQSslConfiguration();
    if (isTestingOpenSsl) {
        clientConfiguration.setHandshakeMustInterruptOnError(true);
        QVERIFY(clientConfiguration.handshakeMustInterruptOnError());
    }
    client.setSslConfiguration(clientConfiguration);
    QSignalSpy clientConnectedSpy(&client, SIGNAL(connected()));
    QSignalSpy clientHostFoundSpy(&client, SIGNAL(hostFound()));
    QSignalSpy clientDisconnectedSpy(&client, SIGNAL(disconnected()));
    QSignalSpy clientConnectionEncryptedSpy(&client, SIGNAL(encrypted()));
    QSignalSpy clientSslErrorsSpy(&client, SIGNAL(sslErrors(QList<QSslError>)));
    QSignalSpy clientErrorOccurredSpy(&client, SIGNAL(errorOccurred(QAbstractSocket::SocketError)));
    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(),
                                  server.server.serverPort());
    QEventLoop loop;
    QTimer::singleShot(1000, &loop, SLOT(quit()));
    loop.exec();

    // Type of socket error to expect
    const auto socketError = isTestingOpenSsl
            ? QAbstractSocket::SocketError::SslHandshakeFailedError
            : QAbstractSocket::SocketError::RemoteHostClosedError;

    QTcpSocket *connection = server.server.nextPendingConnection();
    if (connection == nullptr) {
        // Client disconnected before connection accepted by server
        QCOMPARE(server.sslErrorsSpy.size(), 0);
        QCOMPARE(server.peerVerifyErrorSpy.size(), 0);
        QCOMPARE(server.errorOccurredSpy.size(), 1); // Client rejected first
        QCOMPARE(server.pendingConnectionAvailableSpy.size(), 0);
        QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.size(), 0);
        QCOMPARE(server.alertSentSpy.size(), 0);
        QCOMPARE(server.alertReceivedSpy.size(),
                 isTestingOpenSsl ? 1 : 0); // OpenSSL only signal
        QCOMPARE(server.handshakeInterruptedOnErrorSpy.size(), 0);
        QCOMPARE(server.startedEncryptionHandshakeSpy.size(), 1);

        const auto errrOccuredSpyError = qvariant_cast<QAbstractSocket::SocketError>(
                std::as_const(server.errorOccurredSpy).first()[1]);
        QCOMPARE(errrOccuredSpyError, socketError);
    } else {
        // Client disconnected after connection accepted by server
        QCOMPARE(server.sslErrorsSpy.size(), 0);
        QCOMPARE(server.peerVerifyErrorSpy.size(), 0);
        QCOMPARE(server.errorOccurredSpy.size(), 0); // Server accepted first
        QCOMPARE(server.pendingConnectionAvailableSpy.size(), 1);
        QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.size(), 0);
        QCOMPARE(server.alertSentSpy.size(), 0);
        QCOMPARE(server.alertReceivedSpy.size(),
                 isTestingOpenSsl ? 1 : 0); // OpenSSL only signal
        QCOMPARE(server.handshakeInterruptedOnErrorSpy.size(), 0);
        QCOMPARE(server.startedEncryptionHandshakeSpy.size(), 1);

        QCOMPARE(connection->state(), QAbstractSocket::UnconnectedState);
        QCOMPARE(connection->error(), socketError);
        auto sslConnection = qobject_cast<QSslSocket *>(connection);
        QVERIFY(sslConnection);
        QVERIFY(!sslConnection->isEncrypted());
    }

    // Check that client has rejected server
    QCOMPARE(clientConnectedSpy.size(), 1);
    QCOMPARE(clientHostFoundSpy.size(), 1);
    QCOMPARE(clientDisconnectedSpy.size(), 1);
    QCOMPARE(clientConnectionEncryptedSpy.size(), 0);
    QCOMPARE(clientSslErrorsSpy.size(), isTestingOpenSsl ? 0 : 1);
    QCOMPARE(clientErrorOccurredSpy.size(), 1);

    // Check client socket
    QVERIFY(!client.isEncrypted());
    QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);
}

#if QT_CONFIG(openssl)

void tst_QSslServer::testHandshakeInterruptedOnError()
{
    if (!isTestingOpenSsl)
        QSKIP("This test requires OpenSSL as the active TLS backend");

    auto serverConfiguration = selfSignedServerQSslConfiguration();
    serverConfiguration.setHandshakeMustInterruptOnError(true);
    QVERIFY(serverConfiguration.handshakeMustInterruptOnError());
    serverConfiguration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    SslServerSpy server(serverConfiguration);
    server.server.listen();

    QSslSocket client;
    auto clientConfiguration = selfSignedClientQSslConfiguration();
    clientConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    client.setSslConfiguration(clientConfiguration);
    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(),
                                  server.server.serverPort());

    QEventLoop loop;
    QObject::connect(&client, SIGNAL(disconnected()), &loop, SLOT(quit()));
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();

    // Check that client certificate causes handshake interrupted signal to be emitted
    QCOMPARE(server.sslErrorsSpy.size(), 0);
    QCOMPARE(server.peerVerifyErrorSpy.size(), 0);
    QCOMPARE(server.errorOccurredSpy.size(), 1);
    QCOMPARE(server.pendingConnectionAvailableSpy.size(), 0);
    QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.size(), 0);
    QCOMPARE(server.alertSentSpy.size(), 1);
    QCOMPARE(server.alertReceivedSpy.size(), 0);
    QCOMPARE(server.handshakeInterruptedOnErrorSpy.size(), 1);
    QCOMPARE(server.startedEncryptionHandshakeSpy.size(), 1);
}

void tst_QSslServer::testPreSharedKeyAuthenticationRequired()
{
    if (!isTestingOpenSsl)
        QSKIP("This test requires OpenSSL as the active TLS backend");

    auto serverConfiguration = QSslConfiguration::defaultConfiguration();
    serverConfiguration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    serverConfiguration.setProtocol(QSsl::TlsV1_2);
    serverConfiguration.setCiphers({ QSslCipher("PSK-AES256-CBC-SHA") });
    serverConfiguration.setPreSharedKeyIdentityHint("Server Y");
    SslServerSpy server(serverConfiguration);
    connect(&server.server, &QSslServer::preSharedKeyAuthenticationRequired,
            [](QSslSocket *, QSslPreSharedKeyAuthenticator *authenticator) {
                QCOMPARE(authenticator->identity(), QByteArray("Client X"));
                authenticator->setPreSharedKey("123456");
            });
    server.server.listen();

    QSslSocket client;
    auto clientConfiguration = QSslConfiguration::defaultConfiguration();
    clientConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    clientConfiguration.setProtocol(QSsl::TlsV1_2);
    clientConfiguration.setCiphers({ QSslCipher("PSK-AES256-CBC-SHA") });
    client.setSslConfiguration(clientConfiguration);
    connect(&client, &QSslSocket::preSharedKeyAuthenticationRequired,
            [](QSslPreSharedKeyAuthenticator *authenticator) {
                QCOMPARE(authenticator->identityHint(), QByteArray("Server Y"));
                authenticator->setPreSharedKey("123456");
                authenticator->setIdentity("Client X");
            });
    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(),
                                  server.server.serverPort());

    connect(&server.server, &QSslServer::sslErrors,
            [](QSslSocket *socket, const QList<QSslError> &errors) {
                for (auto error : errors) {
                    QCOMPARE(error.error(), QSslError::NoPeerCertificate);
                }
                socket->ignoreSslErrors();
            });

    QEventLoop loop;
    QObject::connect(&client, SIGNAL(encrypted()), &loop, SLOT(quit()));
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();

    // Check that server is connected
    QCOMPARE(server.sslErrorsSpy.size(), 1);
    QCOMPARE(server.peerVerifyErrorSpy.size(), 1);
    QCOMPARE(server.errorOccurredSpy.size(), 0);
    QCOMPARE(server.pendingConnectionAvailableSpy.size(), 1);
    QCOMPARE(server.preSharedKeyAuthenticationRequiredSpy.size(), 1);
    QCOMPARE(server.alertSentSpy.size(), 0);
    QCOMPARE(server.alertReceivedSpy.size(), 0);
    QCOMPARE(server.handshakeInterruptedOnErrorSpy.size(), 0);
    QCOMPARE(server.startedEncryptionHandshakeSpy.size(), 1);

    // Check client socket
    QVERIFY(client.isEncrypted());
    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
}

#endif

void tst_QSslServer::plaintextClient()
{
    QSslConfiguration serverConfiguration = selfSignedServerQSslConfiguration();
    SslServerSpy server(serverConfiguration);
    QVERIFY(server.server.listen());

    QTcpSocket socket;
    QSignalSpy socketDisconnectedSpy(&socket, &QTcpSocket::disconnected);
    socket.connectToHost(QHostAddress::LocalHost, server.server.serverPort());
    QVERIFY(socket.waitForConnected());
    QTest::qWait(100);
    // No disconnect from short break...:
    QCOMPARE(socket.state(), QAbstractSocket::SocketState::ConnectedState);

    // ... but we write some plaintext data...:
    socket.write("Hello World!");
    socket.waitForBytesWritten();
    // ... and quickly get disconnected:
    QTRY_COMPARE_GT(socketDisconnectedSpy.size(), 0);
    QCOMPARE(socket.state(), QAbstractSocket::SocketState::UnconnectedState);
}

void tst_QSslServer::quietClient()
{
    QSslConfiguration serverConfiguration = selfSignedServerQSslConfiguration();
    SslServerSpy server(serverConfiguration);
    server.server.setHandshakeTimeout(1'000);
    QVERIFY(server.server.listen());

    quint16 serverPeerPort = 0;
    auto grabServerPeerPort = [&serverPeerPort](QSslSocket *socket) {
        serverPeerPort = socket->peerPort();
    };
    QObject::connect(&server.server, &QSslServer::errorOccurred, &server.server,
                     grabServerPeerPort);

    QTcpSocket socket;
    QSignalSpy socketDisconnectedSpy(&socket, &QTcpSocket::disconnected);
    socket.connectToHost(QHostAddress::LocalHost, server.server.serverPort());
    quint16 clientLocalPort = socket.localPort();
    QVERIFY(socket.waitForConnected());
    // Disconnects after overlong break:
    QVERIFY(socketDisconnectedSpy.wait(5'000));
    QCOMPARE(socket.state(), QAbstractSocket::SocketState::UnconnectedState);

    QCOMPARE_GT(server.errorOccurredSpy.size(), 0);
    QCOMPARE(serverPeerPort, clientLocalPort);
}

void tst_QSslServer::twoGoodAndManyBadClients()
{
    if (QTestPrivate::isSecureTransportBlockingTest())
        QSKIP("SecureTransport will block this test while requesting keychain access");

    QSslConfiguration serverConfiguration = selfSignedServerQSslConfiguration();
    SslServerSpy server(serverConfiguration);
    server.server.setHandshakeTimeout(750);
    constexpr qsizetype ExpectedConnections = 5;
    server.server.setMaxPendingConnections(ExpectedConnections);
    QVERIFY(server.server.listen());

    auto connectGoodClient = [&server](QSslSocket *socket) {
        QObject::connect(socket, &QSslSocket::sslErrors, socket,
                         qOverload<const QList<QSslError> &>(&QSslSocket::ignoreSslErrors));
        socket->connectToHostEncrypted("127.0.0.1", server.server.serverPort());
    };
    // Connect one socket encrypted so we have a socket in the regular queue
    QSslSocket tlsSocket;
    connectGoodClient(&tlsSocket);

    // Then we connect a bunch of TCP sockets who will not send any data at all
    std::array<QTcpSocket, size_t(ExpectedConnections) * 2> sockets;
    for (QTcpSocket &socket : sockets)
        socket.connectToHost(QHostAddress::LocalHost, server.server.serverPort());
    QTest::qWait(500); // some leeway to let connections try to connect...

    // I happen to know the sockets are all children of the server, so let's see
    // how many are created:
    qsizetype connectedCount = server.server.findChildren<QSslSocket *>().size();
    QCOMPARE(connectedCount, ExpectedConnections);
    // 1 socket is ready and pending
    QCOMPARE(server.pendingConnectionAvailableSpy.size(), 1);

    // Connect another client to make sure that the server is accepting connections again even after
    // all the bad actors tried to connect:
    QSslSocket goodClient;
    connectGoodClient(&goodClient);
    QTRY_COMPARE(server.pendingConnectionAvailableSpy.size(), 2);
}

QTEST_MAIN(tst_QSslServer)

#include "tst_qsslserver.moc"
