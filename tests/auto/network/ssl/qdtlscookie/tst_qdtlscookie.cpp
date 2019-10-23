/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qudpsocket.h>
#include <QtNetwork/qdtls.h>

#include <QtCore/qcryptographichash.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/qobject.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>

#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

#define STOP_ON_FAILURE \
    if (QTest::currentTestFailed()) \
        return;

class tst_QDtlsCookie : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void init();

private slots:
    // Tests:
    void construction();
    void validateParameters_data();
    void validateParameters();
    void verifyClient();
    void cookieGeneratorParameters();
    void verifyMultipleClients();

protected slots:
    // Aux. functions:
    void stopLoopOnMessage();
    void serverReadyRead();
    void clientReadyRead();
    void handleClientTimeout();
    void makeNoise();
    void spawnClients();

private:
    void sendClientHello(QUdpSocket *socket, QDtls *handshake,
                         const QByteArray &serverMessage  = {});
    void receiveMessage(QUdpSocket *socket, QByteArray *message,
                        QHostAddress *address = nullptr,
                        quint16 *port = nullptr);

    static QHostAddress toNonAny(const QHostAddress &addr);

    enum AddressType
    {
        ValidAddress,
        NullAddress,
        BroadcastAddress,
        MulticastAddress
    };

    QUdpSocket serverSocket;
    QHostAddress serverAddress;
    quint16 serverPort = 0;

    QTestEventLoop testLoop;
    int handshakeTimeoutMS = 500;

    QDtlsClientVerifier listener;
    using HandshakePtr = QSharedPointer<QDtls>;
    HandshakePtr dtls;

    const QCryptographicHash::Algorithm defaultHash =
#ifdef QT_CRYPTOGRAPHICHASH_ONLY_SHA1
        QCryptographicHash::Sha1;
#else
        QCryptographicHash::Sha256;
#endif

    using CookieParams = QDtlsClientVerifier::GeneratorParameters;

    QUdpSocket noiseMaker;
    QHostAddress spammerAddress;
    QTimer noiseTimer;
    quint16 spammerPort = 0;
    const int noiseTimeoutMS = 5;

    using SocketPtr = QSharedPointer<QUdpSocket>;
    using ValidClient = QPair<SocketPtr, HandshakePtr>;
    unsigned clientsToWait = 0;
    unsigned clientsToAdd = 0;
    std::vector<ValidClient> dtlsClients;
    QTimer spawnTimer;
};

QHostAddress tst_QDtlsCookie::toNonAny(const QHostAddress &addr)
{
    if (addr == QHostAddress::Any || addr == QHostAddress::AnyIPv4)
        return QHostAddress::LocalHost;
    if (addr == QHostAddress::AnyIPv6)
        return QHostAddress::LocalHostIPv6;
    return addr;
}

void tst_QDtlsCookie::initTestCase()
{
    QVERIFY(noiseMaker.bind());
    spammerAddress = toNonAny(noiseMaker.localAddress());
    spammerPort = noiseMaker.localPort();
}

void tst_QDtlsCookie::init()
{
    if (serverSocket.state() != QAbstractSocket::UnconnectedState) {
        serverSocket.close();
        // Disconnect stopLoopOnMessage or serverReadyRead slots:
        serverSocket.disconnect();
    }

    QCOMPARE(serverSocket.state(), QAbstractSocket::UnconnectedState);
    QVERIFY(serverSocket.bind());

    serverAddress = toNonAny(serverSocket.localAddress());
    serverPort = serverSocket.localPort();

    dtls.reset(new QDtls(QSslSocket::SslClientMode));
    dtls->setPeer(serverAddress, serverPort);
}

void tst_QDtlsCookie::construction()
{
    QDtlsClientVerifier verifier;

    QCOMPARE(verifier.dtlsError(), QDtlsError::NoError);
    QCOMPARE(verifier.dtlsErrorString(), QString());
    QCOMPARE(verifier.verifiedHello(), QByteArray());

    const auto params = verifier.cookieGeneratorParameters();
    QCOMPARE(params.hash, defaultHash);
    QVERIFY(params.secret.size() > 0);
}

void tst_QDtlsCookie::validateParameters_data()
{
    QTest::addColumn<bool>("invalidSocket");
    QTest::addColumn<bool>("emptyDatagram");
    QTest::addColumn<int>("addressType");

    QTest::addRow("socket") << true << false << int(ValidAddress);
    QTest::addRow("dgram") << false << true << int(ValidAddress);
    QTest::addRow("addr(invalid)") << false << false << int(NullAddress);
    QTest::addRow("addr(broadcast)") << false << false << int(BroadcastAddress);
    QTest::addRow("addr(multicast)") << false << false << int(MulticastAddress);

    QTest::addRow("socket-dgram") << true << true << int(ValidAddress);
    QTest::addRow("socket-dgram-addr(invalid)") << true << true << int(NullAddress);
    QTest::addRow("socket-dgram-addr(broadcast)") << true << true << int(BroadcastAddress);
    QTest::addRow("socket-dgram-addr(multicast)") << true << true << int(MulticastAddress);

    QTest::addRow("dgram-addr(invalid)") << false << true << int(NullAddress);
    QTest::addRow("dgram-addr(broadcast)") << false << true << int(BroadcastAddress);
    QTest::addRow("dgram-addr(multicast)") << false << true << int(MulticastAddress);

    QTest::addRow("socket-addr(invalid)") << true << false << int(NullAddress);
    QTest::addRow("socket-addr(broadcast)") << true << false << int(BroadcastAddress);
    QTest::addRow("socket-addr(multicast)") << true << false << int(MulticastAddress);
}

void tst_QDtlsCookie::validateParameters()
{
    connect(&serverSocket, &QUdpSocket::readyRead, this,
            &tst_QDtlsCookie::stopLoopOnMessage);

    QFETCH(const bool, invalidSocket);
    QFETCH(const bool, emptyDatagram);
    QFETCH(const int, addressType);

    QUdpSocket clientSocket;
    QByteArray hello;
    QHostAddress clientAddress;
    quint16 clientPort = 0;

    sendClientHello(&clientSocket, dtls.data());
    STOP_ON_FAILURE
    receiveMessage(&serverSocket, &hello, &clientAddress, &clientPort);
    STOP_ON_FAILURE

    switch (addressType) {
    case MulticastAddress:
        clientAddress.setAddress(QStringLiteral("224.0.0.0"));
        break;
    case BroadcastAddress:
        clientAddress = QHostAddress::Broadcast;
        break;
    case NullAddress:
        clientAddress = {};
        break;
    }

    if (emptyDatagram)
        hello.clear();

    QUdpSocket *socket = invalidSocket ? nullptr : &serverSocket;
    QCOMPARE(listener.verifyClient(socket, hello, clientAddress, clientPort), false);
    QCOMPARE(listener.verifiedHello(), QByteArray());
    QCOMPARE(listener.dtlsError(), QDtlsError::InvalidInputParameters);
}

void tst_QDtlsCookie::verifyClient()
{
    connect(&serverSocket, &QUdpSocket::readyRead, this,
            &tst_QDtlsCookie::stopLoopOnMessage);

    QUdpSocket clientSocket;
    connect(&clientSocket, &QUdpSocket::readyRead, this,
            &tst_QDtlsCookie::stopLoopOnMessage);

    // Client: send an initial ClientHello message without any cookie:
    sendClientHello(&clientSocket, dtls.data());
    STOP_ON_FAILURE
    // Server: read the first ClientHello message:
    QByteArray dgram;
    QHostAddress clientAddress;
    quint16 clientPort = 0;
    receiveMessage(&serverSocket, &dgram, &clientAddress, &clientPort);
    STOP_ON_FAILURE
    // Server: reply with a verify hello request (the client is not verified yet):
    QCOMPARE(listener.verifyClient(&serverSocket, dgram, clientAddress, clientPort), false);
    QCOMPARE(listener.verifiedHello(), QByteArray());
    QCOMPARE(listener.dtlsError(), QDtlsError::NoError);
    // Client: read hello verify request:
    receiveMessage(&clientSocket, &dgram);
    STOP_ON_FAILURE
    // Client: send a new hello message, this time with a cookie attached:
    sendClientHello(&clientSocket, dtls.data(), dgram);
    STOP_ON_FAILURE
    // Server: read a client-verified message:
    receiveMessage(&serverSocket, &dgram, &clientAddress, &clientPort);
    STOP_ON_FAILURE
    // Client's readyRead is not interesting anymore:
    clientSocket.close();

    // Verify with the address and port we extracted, do it twice (DTLS "listen"
    // must be stateless and work as many times as needed):
    for (int i = 0; i < 2; ++i) {
        QCOMPARE(listener.verifyClient(&serverSocket, dgram, clientAddress, clientPort), true);
        QCOMPARE(listener.verifiedHello(), dgram);
        QCOMPARE(listener.dtlsError(), QDtlsError::NoError);
    }

    // Test that another freshly created (stateless) verifier can verify:
    QDtlsClientVerifier anotherListener;
    QCOMPARE(anotherListener.verifyClient(&serverSocket, dgram, clientAddress,
                                          clientPort), true);
    QCOMPARE(anotherListener.verifiedHello(), dgram);
    QCOMPARE(anotherListener.dtlsError(), QDtlsError::NoError);
    // Now let's use a wrong port:
    QCOMPARE(listener.verifyClient(&serverSocket, dgram, clientAddress, serverPort), false);
    // Invalid cookie, no verified hello message:
    QCOMPARE(listener.verifiedHello(), QByteArray());
    // But it's UDP so we ignore this "fishy datagram", no error expected:
    QCOMPARE(listener.dtlsError(), QDtlsError::NoError);
}

void tst_QDtlsCookie::cookieGeneratorParameters()
{
    CookieParams params;// By defualt, 'secret' is empty.
    QCOMPARE(listener.setCookieGeneratorParameters(params), false);
    QCOMPARE(listener.dtlsError(), QDtlsError::InvalidInputParameters);
    params.secret = "abcdefghijklmnopqrstuvwxyz";
    QCOMPARE(listener.setCookieGeneratorParameters(params), true);
    QCOMPARE(listener.dtlsError(), QDtlsError::NoError);
}

void tst_QDtlsCookie::verifyMultipleClients()
{
    // 'verifyClient' above was quite simple - it's like working with blocking
    // sockets, step by step - we write, then make sure we read a datagram back
    // etc. This test is more asynchronous - we are running an event loop and don't
    // stop on the first datagram received, instead, we spawn many clients
    // with which to exchange handshake messages and verify requests, while at
    // the same time dealing with a 'noise maker' - a fake DTLS client, who keeps
    // spamming our server with non-DTLS datagrams and initial ClientHello
    // messages, but never replies to client verify requests.
    connect(&serverSocket, &QUdpSocket::readyRead, this, &tst_QDtlsCookie::serverReadyRead);

    noiseTimer.setInterval(noiseTimeoutMS);
    connect(&noiseTimer, &QTimer::timeout, this, &tst_QDtlsCookie::makeNoise);

    spawnTimer.setInterval(noiseTimeoutMS * 10);
    connect(&spawnTimer, &QTimer::timeout, this, &tst_QDtlsCookie::spawnClients);

    noiseTimer.start();
    spawnTimer.start();

    clientsToAdd = clientsToWait = 100;

    testLoop.enterLoop(handshakeTimeoutMS * clientsToWait);
    QVERIFY(!testLoop.timeout());
    QVERIFY(clientsToWait == 0);
}

void tst_QDtlsCookie::sendClientHello(QUdpSocket *socket, QDtls *dtls,
                                      const QByteArray &serverMessage)
{
    Q_ASSERT(socket && dtls);
    dtls->doHandshake(socket, serverMessage);
    // We don't really care about QDtls in this auto-test, but must be
    // sure that we, indeed, sent our hello and if not - stop early without
    // running event loop:
    QCOMPARE(dtls->dtlsError(), QDtlsError::NoError);
    // We never complete a handshake, so it must be 'HandshakeInProgress':
    QCOMPARE(dtls->handshakeState(), QDtls::HandshakeInProgress);
}

void tst_QDtlsCookie::receiveMessage(QUdpSocket *socket, QByteArray *message,
                                     QHostAddress *address, quint16 *port)
{
    Q_ASSERT(socket && message);

    if (socket->pendingDatagramSize() <= 0)
        testLoop.enterLoopMSecs(handshakeTimeoutMS);

    QVERIFY(!testLoop.timeout());
    QVERIFY(socket->pendingDatagramSize());

    message->resize(socket->pendingDatagramSize());
    const qint64 read = socket->readDatagram(message->data(), message->size(),
                                             address, port);
    QVERIFY(read > 0);

    message->resize(read);
    if (address)
        QVERIFY(!address->isNull());
}

void tst_QDtlsCookie::stopLoopOnMessage()
{
    testLoop.exitLoop();
}

void tst_QDtlsCookie::serverReadyRead()
{
    Q_ASSERT(clientsToWait);

    if (serverSocket.pendingDatagramSize() <= 0)
        return;

    QByteArray hello;
    QHostAddress clientAddress;
    quint16 clientPort = 0;

    receiveMessage(&serverSocket, &hello, &clientAddress, &clientPort);
    if (QTest::currentTestFailed())
        return testLoop.exitLoop();

    const bool ok = listener.verifyClient(&serverSocket, hello, clientAddress, clientPort);
    if (listener.dtlsError() != QDtlsError::NoError) {
        // exit early, let the test fail.
        return testLoop.exitLoop();
    }

    if (!ok) // not verified yet.
        return;

    if (clientAddress == spammerAddress && clientPort == spammerPort) // should never happen
        return testLoop.exitLoop();

    --clientsToWait;
    if (!clientsToWait) // done, success.
        testLoop.exitLoop();
}

void tst_QDtlsCookie::clientReadyRead()
{
    QUdpSocket *clientSocket = qobject_cast<QUdpSocket *>(sender());
    Q_ASSERT(clientSocket);

    if (clientSocket->pendingDatagramSize() <= 0)
        return;

    QDtls *handshake = nullptr;
    for (ValidClient &client : dtlsClients) {
        if (client.first.data() == clientSocket) {
            handshake = client.second.data();
            break;
        }
    }

    Q_ASSERT(handshake);

    QByteArray response;
    receiveMessage(clientSocket, &response);
    if (QTest::currentTestFailed() || !handshake->doHandshake(clientSocket, response))
        testLoop.exitLoop();
}

void tst_QDtlsCookie::makeNoise()
{
    noiseMaker.writeDatagram({"Hello, my little DTLS server, take this useless dgram!"},
                             serverAddress, serverPort);
    QDtls fakeHandshake(QSslSocket::SslClientMode);
    fakeHandshake.setPeer(serverAddress, serverPort);
    fakeHandshake.doHandshake(&noiseMaker, {});
}

void tst_QDtlsCookie::spawnClients()
{
    for (int i = 0; i < 10 && clientsToAdd; ++i, --clientsToAdd) {
        ValidClient newClient;
        newClient.first.reset(new QUdpSocket);
        connect(newClient.first.data(), &QUdpSocket::readyRead,
                this, &tst_QDtlsCookie::clientReadyRead);
        newClient.second.reset(new QDtls(QSslSocket::SslClientMode));
        newClient.second->setPeer(serverAddress, serverPort);
        connect(newClient.second.data(), &QDtls::handshakeTimeout,
                this, &tst_QDtlsCookie::handleClientTimeout);
        newClient.second->doHandshake(newClient.first.data(), {});
        dtlsClients.push_back(std::move(newClient));
    }
}

void tst_QDtlsCookie::handleClientTimeout()
{
    QDtls *handshake = qobject_cast<QDtls *>(sender());
    Q_ASSERT(handshake);

    QUdpSocket *clientSocket = nullptr;
    for (ValidClient &client : dtlsClients) {
        if (client.second.data() == handshake) {
            clientSocket = client.first.data();
            break;
        }
    }

    Q_ASSERT(clientSocket);
    handshake->handleTimeout(clientSocket);
}

QT_END_NAMESPACE

QTEST_MAIN(tst_QDtlsCookie)

#include "tst_qdtlscookie.moc"
