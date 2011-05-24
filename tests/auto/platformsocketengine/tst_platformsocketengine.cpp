/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#ifdef Q_OS_WIN
#include <winsock2.h>
#endif

#include <qcoreapplication.h>


#include <qdatastream.h>

#include <qhostaddress.h>
#include <qdatetime.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include <stddef.h>

#ifdef Q_OS_SYMBIAN
#include <QNetworkConfigurationManager>
#include <QNetworkConfiguration>
#include <QNetworkSession>
#include <QScopedPointer>
#define PLATFORMSOCKETENGINE QSymbianSocketEngine
#define PLATFORMSOCKETENGINESTRING "QSymbianSocketEngine"
#include <private/qsymbiansocketengine_p.h>
#include <private/qcore_symbian_p.h>
#else
#define PLATFORMSOCKETENGINE QNativeSocketEngine
#define PLATFORMSOCKETENGINESTRING "QNativeSocketEngine"
#include <private/qnativesocketengine_p.h>
#endif

#include <qstringlist.h>

#include "../network-settings.h"

//TESTED_FILES=network/qnativesocketengine.cpp network/qnativesocketengine_p.h network/qnativesocketengine_unix.cpp

class tst_PlatformSocketEngine : public QObject
{
    Q_OBJECT

public:
    tst_PlatformSocketEngine();
    virtual ~tst_PlatformSocketEngine();


public slots:
    void init();
    void cleanup();
private slots:
    void construction();
    void simpleConnectToIMAP();
    void udpLoopbackTest();
    void udpIPv6LoopbackTest();
    void broadcastTest();
    void serverTest();
    void udpLoopbackPerformance();
    void tcpLoopbackPerformance();
    void readWriteBufferSize();
    void bind();
    void networkError();
    void setSocketDescriptor();
    void invalidSend();
    void receiveUrgentData();
    void tooManySockets();
};

tst_PlatformSocketEngine::tst_PlatformSocketEngine()
{
    Q_SET_DEFAULT_IAP
}

tst_PlatformSocketEngine::~tst_PlatformSocketEngine()
{
}

void tst_PlatformSocketEngine::init()
{
}

void tst_PlatformSocketEngine::cleanup()
{
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::construction()
{
    PLATFORMSOCKETENGINE socketDevice;

    QVERIFY(!socketDevice.isValid());

    // Initialize device
    QVERIFY(socketDevice.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QVERIFY(socketDevice.isValid());
    QVERIFY(socketDevice.protocol() == QAbstractSocket::IPv4Protocol);
    QVERIFY(socketDevice.socketType() == QAbstractSocket::TcpSocket);
    QVERIFY(socketDevice.state() == QAbstractSocket::UnconnectedState);
    QVERIFY(socketDevice.socketDescriptor() != -1);
    QVERIFY(socketDevice.localAddress() == QHostAddress());
    QVERIFY(socketDevice.localPort() == 0);
    QVERIFY(socketDevice.peerAddress() == QHostAddress());
    QVERIFY(socketDevice.peerPort() == 0);
    QVERIFY(socketDevice.error() == QAbstractSocket::UnknownSocketError);

    QTest::ignoreMessage(QtWarningMsg, PLATFORMSOCKETENGINESTRING "::bytesAvailable() was called in QAbstractSocket::UnconnectedState");
    QVERIFY(socketDevice.bytesAvailable() == 0);

    QTest::ignoreMessage(QtWarningMsg, PLATFORMSOCKETENGINESTRING "::hasPendingDatagrams() was called in QAbstractSocket::UnconnectedState");
    QVERIFY(!socketDevice.hasPendingDatagrams());
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::simpleConnectToIMAP()
{
    PLATFORMSOCKETENGINE socketDevice;

    // Initialize device
    QVERIFY(socketDevice.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QVERIFY(socketDevice.state() == QAbstractSocket::UnconnectedState);

    const bool isConnected = socketDevice.connectToHost(QtNetworkSettings::serverIP(), 143);
    if (!isConnected) {
        QVERIFY(socketDevice.state() == QAbstractSocket::ConnectingState);
        QVERIFY(socketDevice.waitForWrite());
        QVERIFY(socketDevice.state() == QAbstractSocket::ConnectedState);
    }
    QVERIFY(socketDevice.state() == QAbstractSocket::ConnectedState);
    QVERIFY(socketDevice.peerAddress() == QtNetworkSettings::serverIP());

    // Wait for the greeting
    QVERIFY(socketDevice.waitForRead());

    // Read the greeting
    qint64 available = socketDevice.bytesAvailable();
    QVERIFY(available > 0);
    QByteArray array;
    array.resize(available);
    QVERIFY(socketDevice.read(array.data(), array.size()) == available);

    // Check that the greeting is what we expect it to be
    QVERIFY2(QtNetworkSettings::compareReplyIMAP(array), array.constData());

    // Write a logout message
    QByteArray array2 = "ZZZ LOGOUT\r\n";
    QVERIFY(socketDevice.write(array2.data(),
                              array2.size()) == array2.size());

    // Wait for the response
    QVERIFY(socketDevice.waitForRead());

    available = socketDevice.bytesAvailable();
    QVERIFY(available > 0);
    array.resize(available);
    QVERIFY(socketDevice.read(array.data(), array.size()) == available);

    // Check that the greeting is what we expect it to be
    QCOMPARE(array.constData(),
             "* BYE LOGOUT received\r\n"
             "ZZZ OK Completed\r\n");

    // Wait for the response
    QVERIFY(socketDevice.waitForRead());
    char c;
    QVERIFY(socketDevice.read(&c, sizeof(c)) == -1);
    QVERIFY(socketDevice.error() == QAbstractSocket::RemoteHostClosedError);
    QVERIFY(socketDevice.state() == QAbstractSocket::UnconnectedState);
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::udpLoopbackTest()
{
    PLATFORMSOCKETENGINE udpSocket;

    // Initialize device #1
    QVERIFY(udpSocket.initialize(QAbstractSocket::UdpSocket));
    QVERIFY(udpSocket.isValid());
    QVERIFY(udpSocket.socketDescriptor() != -1);
    QVERIFY(udpSocket.protocol() == QAbstractSocket::IPv4Protocol);
    QVERIFY(udpSocket.socketType() == QAbstractSocket::UdpSocket);
    QVERIFY(udpSocket.state() == QAbstractSocket::UnconnectedState);

    // Bind #1 to localhost
    QVERIFY(udpSocket.bind(QHostAddress("127.0.0.1"), 0));
    QVERIFY(udpSocket.state() == QAbstractSocket::BoundState);
    quint16 port = udpSocket.localPort();
    QVERIFY(port != 0);

    // Initialize device #2
    PLATFORMSOCKETENGINE udpSocket2;
    QVERIFY(udpSocket2.initialize(QAbstractSocket::UdpSocket));

    // Connect device #2 to #1
    QVERIFY(udpSocket2.connectToHost(QHostAddress("127.0.0.1"), port));
    QVERIFY(udpSocket2.state() == QAbstractSocket::ConnectedState);

    // Write a message to #1
    QByteArray message1 = "hei der";
    QVERIFY(udpSocket2.write(message1.data(),
                            message1.size()) == message1.size());

    // Read the message from #2
    QVERIFY(udpSocket.waitForRead());
    QVERIFY(udpSocket.hasPendingDatagrams());
    qint64 available = udpSocket.pendingDatagramSize();
    QVERIFY(available > 0);
    QByteArray answer;
    answer.resize(available);
    QHostAddress senderAddress;
    quint16 senderPort = 0;
    QVERIFY(udpSocket.readDatagram(answer.data(), answer.size(),
                                  &senderAddress,
                                  &senderPort) == message1.size());
    QVERIFY(senderAddress == QHostAddress("127.0.0.1"));
    QVERIFY(senderPort != 0);
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::udpIPv6LoopbackTest()
{
    PLATFORMSOCKETENGINE udpSocket;

    // Initialize device #1
    bool init = udpSocket.initialize(QAbstractSocket::UdpSocket, QAbstractSocket::IPv6Protocol);

    if (!init) {
        QVERIFY(udpSocket.error() == QAbstractSocket::UnsupportedSocketOperationError);
    } else {
        QVERIFY(udpSocket.protocol() == QAbstractSocket::IPv6Protocol);

        // Bind #1 to localhost
        QVERIFY(udpSocket.bind(QHostAddress("::1"), 0));
        QVERIFY(udpSocket.state() == QAbstractSocket::BoundState);
        quint16 port = udpSocket.localPort();
        QVERIFY(port != 0);

        // Initialize device #2
        PLATFORMSOCKETENGINE udpSocket2;
        QVERIFY(udpSocket2.initialize(QAbstractSocket::UdpSocket, QAbstractSocket::IPv6Protocol));

        // Connect device #2 to #1
        QVERIFY(udpSocket2.connectToHost(QHostAddress("::1"), port));
        QVERIFY(udpSocket2.state() == QAbstractSocket::ConnectedState);

        // Write a message to #1
        QByteArray message1 = "hei der";
        QVERIFY(udpSocket2.write(message1.data(),
                                message1.size()) == message1.size());

        // Read the message from #2
        QVERIFY(udpSocket.waitForRead());
        QVERIFY(udpSocket.hasPendingDatagrams());
        qint64 available = udpSocket.pendingDatagramSize();
        QVERIFY(available > 0);
        QByteArray answer;
        answer.resize(available);
        QHostAddress senderAddress;
        quint16 senderPort = 0;
        QVERIFY(udpSocket.readDatagram(answer.data(), answer.size(),
                                      &senderAddress,
                                      &senderPort) == message1.size());
        QVERIFY(senderAddress == QHostAddress("::1"));
        QVERIFY(senderPort != 0);
    }
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::broadcastTest()
{
#ifdef Q_OS_SYMBIAN
    //broadcast isn't supported on loopback connections, but is on WLAN
#ifndef QT_NO_BEARERMANAGEMENT
    QScopedPointer<QNetworkConfigurationManager> netConfMan(new QNetworkConfigurationManager());
    QNetworkConfiguration networkConfiguration(netConfMan->defaultConfiguration());
    QScopedPointer<QNetworkSession> networkSession(new QNetworkSession(networkConfiguration));
    if (!networkSession->isOpen()) {
        networkSession->open();
        bool ok = networkSession->waitForOpened(30000);
        qDebug() << networkSession->isOpen() << networkSession->error() << networkSession->errorString();
        QVERIFY(ok);
    }
#endif
#endif
#ifdef Q_OS_AIX
    QSKIP("Broadcast does not work on darko", SkipAll);
#endif
    PLATFORMSOCKETENGINE broadcastSocket;

    // Initialize a regular Udp socket
    QVERIFY(broadcastSocket.initialize(QAbstractSocket::UdpSocket));

    // Bind to any port on all interfaces
    QVERIFY(broadcastSocket.bind(QHostAddress::Any, 0));
    QVERIFY(broadcastSocket.state() == QAbstractSocket::BoundState);
    quint16 port = broadcastSocket.localPort();
    QVERIFY(port > 0);

    // Broadcast an inappropriate troll message
    QByteArray trollMessage
        = "MOOT wtf is a MOOT? talk english not your sutpiD ENGLISH.";
    qint64 written = broadcastSocket.writeDatagram(trollMessage.data(),
                                         trollMessage.size(),
                                         QHostAddress::Broadcast,
                                         port);

#ifdef Q_OS_SYMBIAN
    //On symbian, broadcasts return 0 bytes written if none of the interfaces support it.
    //Notably the loopback interfaces do not. (though they do support multicast!?)
    if (written == 0)
        QEXPECT_FAIL("", "No active interface supports broadcast", Abort);
#endif
    QCOMPARE((int)written, trollMessage.size());

    // Wait until we receive it ourselves
#if defined(Q_OS_FREEBSD)
    QEXPECT_FAIL("", "Broadcasting to 255.255.255.255 does not work on FreeBSD", Abort);
#endif
    QVERIFY(broadcastSocket.waitForRead());
    QVERIFY(broadcastSocket.hasPendingDatagrams());

    qlonglong available = broadcastSocket.pendingDatagramSize();
    QByteArray response;
    response.resize(available);
    QVERIFY(broadcastSocket.readDatagram(response.data(), response.size())
           == response.size());
    QCOMPARE(response, trollMessage);

}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::serverTest()
{
    PLATFORMSOCKETENGINE server;

    // Initialize a Tcp socket
    QVERIFY(server.initialize(QAbstractSocket::TcpSocket));

    // Bind to any port on all interfaces
    QVERIFY(server.bind(QHostAddress("0.0.0.0"), 0));
    QVERIFY(server.state() == QAbstractSocket::BoundState);
    quint16 port = server.localPort();

    // Listen for incoming connections
    QVERIFY(server.listen());
    QVERIFY(server.state() == QAbstractSocket::ListeningState);

    // Initialize a Tcp socket
    PLATFORMSOCKETENGINE client;
    QVERIFY(client.initialize(QAbstractSocket::TcpSocket));
    if (!client.connectToHost(QHostAddress("127.0.0.1"), port)) {
        QVERIFY(client.state() == QAbstractSocket::ConnectingState);
        QVERIFY(client.waitForWrite());
        QVERIFY(client.state() == QAbstractSocket::ConnectedState);
    }

    // The server accepts the connection
    int socketDescriptor = server.accept();
    QVERIFY(socketDescriptor > 0);

    // A socket device is initialized on the server side, passing the
    // socket descriptor from accept(). It's pre-connected.
    PLATFORMSOCKETENGINE serverSocket;
    QVERIFY(serverSocket.initialize(socketDescriptor));
    QVERIFY(serverSocket.state() == QAbstractSocket::ConnectedState);

    // The server socket sends a greeting to the clietn
    QByteArray greeting = "Greetings!";
    QVERIFY(serverSocket.write(greeting.data(),
                              greeting.size()) == greeting.size());

    // The client waits for the greeting to arrive
    QVERIFY(client.waitForRead());
    qint64 available = client.bytesAvailable();
    QVERIFY(available > 0);

    // The client reads the greeting and checks that it's correct
    QByteArray response;
    response.resize(available);
    QVERIFY(client.read(response.data(),
                       response.size()) == response.size());
    QCOMPARE(response, greeting);
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::udpLoopbackPerformance()
{
#ifdef SYMBIAN_WINSOCK_CONNECTIVITY
    QSKIP("Not working on Emulator without WinPCAP", SkipAll);
#endif
    PLATFORMSOCKETENGINE udpSocket;

    // Initialize device #1
    QVERIFY(udpSocket.initialize(QAbstractSocket::UdpSocket));
    QVERIFY(udpSocket.isValid());
    QVERIFY(udpSocket.socketDescriptor() != -1);
    QVERIFY(udpSocket.protocol() == QAbstractSocket::IPv4Protocol);
    QVERIFY(udpSocket.socketType() == QAbstractSocket::UdpSocket);
    QVERIFY(udpSocket.state() == QAbstractSocket::UnconnectedState);

    // Bind #1 to localhost
    QVERIFY(udpSocket.bind(QHostAddress("127.0.0.1"), 0));
    QVERIFY(udpSocket.state() == QAbstractSocket::BoundState);
    quint16 port = udpSocket.localPort();
    QVERIFY(port != 0);

    // Initialize device #2
    PLATFORMSOCKETENGINE udpSocket2;
    QVERIFY(udpSocket2.initialize(QAbstractSocket::UdpSocket));

    // Connect device #2 to #1
    QVERIFY(udpSocket2.connectToHost(QHostAddress("127.0.0.1"), port));
    QVERIFY(udpSocket2.state() == QAbstractSocket::ConnectedState);

    const int messageSize = 8192;
    QByteArray message1(messageSize, '@');
    QByteArray answer(messageSize, '@');

    QHostAddress localhost = QHostAddress::LocalHost;

    qlonglong readBytes = 0;
    QTime timer;
    timer.start();
    while (timer.elapsed() < 5000) {
        udpSocket2.write(message1.data(), message1.size());
        udpSocket.waitForRead();
        while (udpSocket.hasPendingDatagrams()) {
            readBytes += (qlonglong) udpSocket.readDatagram(answer.data(),
                                                             answer.size());
        }
    }

    qDebug("\t\t%.1fMB/%.1fs: %.1fMB/s",
           readBytes / (1024.0 * 1024.0),
           timer.elapsed() / 1024.0,
           (readBytes / (timer.elapsed() / 1000.0)) / (1024 * 1024));
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::tcpLoopbackPerformance()
{
    PLATFORMSOCKETENGINE server;

    // Initialize a Tcp socket
    QVERIFY(server.initialize(QAbstractSocket::TcpSocket));

    // Bind to any port on all interfaces
    QVERIFY(server.bind(QHostAddress("0.0.0.0"), 0));
    QVERIFY(server.state() == QAbstractSocket::BoundState);
    quint16 port = server.localPort();

    // Listen for incoming connections
    QVERIFY(server.listen());
    QVERIFY(server.state() == QAbstractSocket::ListeningState);

    // Initialize a Tcp socket
    PLATFORMSOCKETENGINE client;
    QVERIFY(client.initialize(QAbstractSocket::TcpSocket));

    // Connect to our server
    if (!client.connectToHost(QHostAddress("127.0.0.1"), port)) {
        QVERIFY(client.state() == QAbstractSocket::ConnectingState);
        QVERIFY(client.waitForWrite());
        QVERIFY(client.state() == QAbstractSocket::ConnectedState);
    }

    // The server accepts the connection
    int socketDescriptor = server.accept();
    QVERIFY(socketDescriptor > 0);

    // A socket device is initialized on the server side, passing the
    // socket descriptor from accept(). It's pre-connected.
    PLATFORMSOCKETENGINE serverSocket;
    QVERIFY(serverSocket.initialize(socketDescriptor));
    QVERIFY(serverSocket.state() == QAbstractSocket::ConnectedState);

#if defined (Q_OS_SYMBIAN) && defined (__WINS__)
    const int messageSize = 1024 * 16;
#else
    const int messageSize = 1024 * 256;
#endif
    QByteArray message1(messageSize, '@');
    QByteArray answer(messageSize, '@');

    QTime timer;
    timer.start();
    qlonglong readBytes = 0;
    while (timer.elapsed() < 5000) {
        qlonglong written = serverSocket.write(message1.data(), message1.size());
        while (written > 0) {
            client.waitForRead();
            if (client.bytesAvailable() > 0) {
                qlonglong readNow = client.read(answer.data(), answer.size());
                written -= readNow;
                readBytes += readNow;
            }
        }
    }

    qDebug("\t\t%.1fMB/%.1fs: %.1fMB/s",
           readBytes / (1024.0 * 1024.0),
           timer.elapsed() / 1024.0,
           (readBytes / (timer.elapsed() / 1000.0)) / (1024 * 1024));
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::readWriteBufferSize()
{
    PLATFORMSOCKETENGINE device;

    QVERIFY(device.initialize(QAbstractSocket::TcpSocket));

    qint64 bufferSize = device.receiveBufferSize();
    QVERIFY(bufferSize != -1);
    device.setReceiveBufferSize(bufferSize + 1);
#if defined(Q_OS_WINCE)
    QEXPECT_FAIL(0, "Not supported by default on WinCE", Continue);
#endif
    QVERIFY(device.receiveBufferSize() > bufferSize);

    bufferSize = device.sendBufferSize();
    QVERIFY(bufferSize != -1);
    device.setSendBufferSize(bufferSize + 1);
    QVERIFY(device.sendBufferSize() > bufferSize);

}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::tooManySockets()
{
#if defined Q_OS_WIN
    QSKIP("Certain windows machines suffocate and spend too much time in this test.", SkipAll);
#endif
    QList<PLATFORMSOCKETENGINE *> sockets;
    PLATFORMSOCKETENGINE *socketLayer = 0;
    for (;;) {
        socketLayer = new PLATFORMSOCKETENGINE;
        sockets.append(socketLayer);

        if (!socketLayer->initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol))
            break;
    }

    QCOMPARE(socketLayer->error(), QAbstractSocket::SocketResourceError);

    qDeleteAll(sockets);
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::bind()
{
#if !defined Q_OS_WIN && !defined Q_OS_SYMBIAN
    PLATFORMSOCKETENGINE binder;
    QVERIFY(binder.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QVERIFY(!binder.bind(QHostAddress::Any, 82));
    QVERIFY(binder.error() == QAbstractSocket::SocketAccessError);
#endif

    PLATFORMSOCKETENGINE binder2;
    QVERIFY(binder2.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QVERIFY(binder2.bind(QHostAddress::Any, 31180));

    PLATFORMSOCKETENGINE binder3;
    QVERIFY(binder3.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QVERIFY(!binder3.bind(QHostAddress::Any, 31180));

#ifdef SYMBIAN_WINSOCK_CONNECTIVITY
    qDebug("On Symbian Emulator (WinSock) we get EADDRNOTAVAIL instead of EADDRINUSE");
    QVERIFY(binder3.error() == QAbstractSocket::SocketAddressNotAvailableError);
#else
    QVERIFY(binder3.error() == QAbstractSocket::AddressInUseError);
#endif
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::networkError()
{
    PLATFORMSOCKETENGINE client;

    QVERIFY(client.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));

    const bool isConnected = client.connectToHost(QtNetworkSettings::serverIP(), 143);
    if (!isConnected) {
        QVERIFY(client.state() == QAbstractSocket::ConnectingState);
        QVERIFY(client.waitForWrite());
        QVERIFY(client.state() == QAbstractSocket::ConnectedState);
    }
    QVERIFY(client.state() == QAbstractSocket::ConnectedState);

    // An unexpected network error!
#ifdef Q_OS_WIN
    // could use shutdown to produce different errors
    ::closesocket(client.socketDescriptor());
#elif defined(Q_OS_SYMBIAN)
    RSocket sock;
    QVERIFY(QSymbianSocketManager::instance().lookupSocket(client.socketDescriptor(), sock));
    TRequestStatus stat;
    sock.Shutdown(RSocket::EImmediate, stat);
    User::WaitForRequest(stat);
#else
    ::close(client.socketDescriptor());
#endif

    QVERIFY(client.read(0, 0) == -1);
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::setSocketDescriptor()
{
    PLATFORMSOCKETENGINE socket1;
    QVERIFY(socket1.initialize(QAbstractSocket::TcpSocket));

    PLATFORMSOCKETENGINE socket2;
    QVERIFY(socket2.initialize(socket1.socketDescriptor()));
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::invalidSend()
{
    PLATFORMSOCKETENGINE socket;
    QVERIFY(socket.initialize(QAbstractSocket::TcpSocket));

    QTest::ignoreMessage(QtWarningMsg, PLATFORMSOCKETENGINESTRING "::writeDatagram() was"
                               " called by a socket other than QAbstractSocket::UdpSocket");
    QCOMPARE(socket.writeDatagram("hei", 3, QHostAddress::LocalHost, 143),
            (qlonglong) -1);
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::receiveUrgentData()
{
    PLATFORMSOCKETENGINE server;

    QVERIFY(server.initialize(QAbstractSocket::TcpSocket));

    // Bind to any port on all interfaces
    QVERIFY(server.bind(QHostAddress("0.0.0.0"), 0));
    QVERIFY(server.state() == QAbstractSocket::BoundState);
    quint16 port = server.localPort();

    QVERIFY(server.listen());
    QVERIFY(server.state() == QAbstractSocket::ListeningState);

    PLATFORMSOCKETENGINE client;
    QVERIFY(client.initialize(QAbstractSocket::TcpSocket));

    if (!client.connectToHost(QHostAddress("127.0.0.1"), port)) {
        QVERIFY(client.state() == QAbstractSocket::ConnectingState);
        QVERIFY(client.waitForWrite());
        QVERIFY(client.state() == QAbstractSocket::ConnectedState);
    }

    int socketDescriptor = server.accept();
    QVERIFY(socketDescriptor > 0);

    PLATFORMSOCKETENGINE serverSocket;
    QVERIFY(serverSocket.initialize(socketDescriptor));
    QVERIFY(serverSocket.state() == QAbstractSocket::ConnectedState);

    char msg;
    int available;
    QByteArray response;

#if defined Q_OS_HPUX
    QSKIP("Native OOB data test doesn't work on HP-UX.", SkipAll);
#elif defined (Q_OS_WINCE)
    QSKIP("Native OOB data test doesn't work on WinCE.", SkipAll);
#endif

    // The server sends an urgent message
    msg = 'Q';
#if defined(Q_OS_SYMBIAN)
    RSocket sock;
    QVERIFY(QSymbianSocketManager::instance().lookupSocket(socketDescriptor, sock));
    TRequestStatus stat;
    TSockXfrLength len;
    sock.Send(TPtrC8((TUint8*)&msg,1), KSockWriteUrgent, stat, len);
    User::WaitForRequest(stat);
    QVERIFY(stat == KErrNone);
    QCOMPARE(len(), 1);
#else
    QCOMPARE(int(::send(socketDescriptor, &msg, sizeof(msg), MSG_OOB)), 1);
#endif

    // The client receives the urgent message
    QVERIFY(client.waitForRead());
    available = client.bytesAvailable();
    QCOMPARE(available, 1);
    response.resize(available);
    QCOMPARE(client.read(response.data(), response.size()), qint64(1));
    QCOMPARE(response.at(0), msg);

    // The client sends an urgent message
    msg = 'T';
    int clientDescriptor = client.socketDescriptor();
#if defined(Q_OS_SYMBIAN)
    QVERIFY(QSymbianSocketManager::instance().lookupSocket(clientDescriptor, sock));
    sock.Send(TPtrC8((TUint8*)&msg,1), KSockWriteUrgent, stat, len);
    User::WaitForRequest(stat);
    QVERIFY(stat == KErrNone);
    QCOMPARE(len(), 1);
#else
    QCOMPARE(int(::send(clientDescriptor, &msg, sizeof(msg), MSG_OOB)), 1);
#endif

    // The server receives the urgent message
    QVERIFY(serverSocket.waitForRead());
    available = serverSocket.bytesAvailable();
    QCOMPARE(available, 1);
    response.resize(available);
    QCOMPARE(serverSocket.read(response.data(), response.size()), qint64(1));
    QCOMPARE(response.at(0), msg);

}

QTEST_MAIN(tst_PlatformSocketEngine)
#include "tst_platformsocketengine.moc"
