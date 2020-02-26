/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <QtTest/QTest>

#include <qcoreapplication.h>
#include <qdatastream.h>
#include <qhostaddress.h>
#include <qelapsedtimer.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

#ifdef Q_OS_VXWORKS
#include <sockLib.h>
#endif

#include <stddef.h>

#define PLATFORMSOCKETENGINE QNativeSocketEngine
#define PLATFORMSOCKETENGINESTRING "QNativeSocketEngine"
#ifndef Q_OS_WINRT
#  include <private/qnativesocketengine_p.h>
#else
#  include <private/qnativesocketengine_winrt_p.h>
#endif

#include <qstringlist.h>

#include "../../../network-settings.h"

class tst_PlatformSocketEngine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void construction();
    void simpleConnectToIMAP();
    void udpLoopbackTest();
    void udpIPv6LoopbackTest();
    void broadcastTest();
    void serverTest();
    void udpLoopbackPerformance();
    void tcpLoopbackPerformance();
#if 0
    void readWriteBufferSize();
#endif
    void bind();
    void networkError();
    void setSocketDescriptor();
    void invalidSend();
#ifndef Q_OS_WINRT
    void receiveUrgentData();
#endif
    void tooManySockets();
};

void tst_PlatformSocketEngine::initTestCase()
{
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::construction()
{
    PLATFORMSOCKETENGINE socketDevice;

    QVERIFY(!socketDevice.isValid());

    // Initialize device
    QVERIFY(socketDevice.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QVERIFY(socketDevice.isValid());
    QCOMPARE(socketDevice.protocol(), QAbstractSocket::IPv4Protocol);
    QCOMPARE(socketDevice.socketType(), QAbstractSocket::TcpSocket);
    QCOMPARE(socketDevice.state(), QAbstractSocket::UnconnectedState);
    QVERIFY(socketDevice.socketDescriptor() != -1);
    QCOMPARE(socketDevice.localAddress(), QHostAddress());
    QCOMPARE(socketDevice.localPort(), quint16(0));
    QCOMPARE(socketDevice.peerAddress(), QHostAddress());
    QCOMPARE(socketDevice.peerPort(), quint16(0));
    QCOMPARE(socketDevice.error(), QAbstractSocket::UnknownSocketError);
    QCOMPARE(socketDevice.option(QNativeSocketEngine::NonBlockingSocketOption), -1);

    QTest::ignoreMessage(QtWarningMsg, PLATFORMSOCKETENGINESTRING "::bytesAvailable() was called in QAbstractSocket::UnconnectedState");
    QCOMPARE(socketDevice.bytesAvailable(), -1);

    QTest::ignoreMessage(QtWarningMsg, PLATFORMSOCKETENGINESTRING "::hasPendingDatagrams() was called in QAbstractSocket::UnconnectedState");
    QVERIFY(!socketDevice.hasPendingDatagrams());
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::simpleConnectToIMAP()
{
    PLATFORMSOCKETENGINE socketDevice;

    // Initialize device
    QVERIFY(socketDevice.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QCOMPARE(socketDevice.state(), QAbstractSocket::UnconnectedState);

    const bool isConnected = socketDevice.connectToHost(QtNetworkSettings::serverIP(), 143);
    if (!isConnected) {
        QCOMPARE(socketDevice.state(), QAbstractSocket::ConnectingState);
        QVERIFY(socketDevice.waitForWrite());
        QCOMPARE(socketDevice.state(), QAbstractSocket::ConnectedState);
    }
    QCOMPARE(socketDevice.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socketDevice.peerAddress(), QtNetworkSettings::serverIP());

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
    QCOMPARE(socketDevice.error(), QAbstractSocket::RemoteHostClosedError);
    QCOMPARE(socketDevice.state(), QAbstractSocket::UnconnectedState);
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::udpLoopbackTest()
{
    PLATFORMSOCKETENGINE udpSocket;

    // Initialize device #1
    QVERIFY(udpSocket.initialize(QAbstractSocket::UdpSocket));
    QVERIFY(udpSocket.isValid());
    QVERIFY(udpSocket.socketDescriptor() != -1);
    QCOMPARE(udpSocket.protocol(), QAbstractSocket::IPv4Protocol);
    QCOMPARE(udpSocket.socketType(), QAbstractSocket::UdpSocket);
    QCOMPARE(udpSocket.state(), QAbstractSocket::UnconnectedState);

    // Bind #1 to localhost
    QVERIFY(udpSocket.bind(QHostAddress("127.0.0.1"), 0));
    QCOMPARE(udpSocket.state(), QAbstractSocket::BoundState);
    quint16 port = udpSocket.localPort();
    QVERIFY(port != 0);

    // Initialize device #2
    PLATFORMSOCKETENGINE udpSocket2;
    QVERIFY(udpSocket2.initialize(QAbstractSocket::UdpSocket));

    // Connect device #2 to #1
    QVERIFY(udpSocket2.connectToHost(QHostAddress("127.0.0.1"), port));
    QCOMPARE(udpSocket2.state(), QAbstractSocket::ConnectedState);

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
    QIpPacketHeader header;
    QCOMPARE(udpSocket.readDatagram(answer.data(), answer.size(),
                                    &header, QAbstractSocketEngine::WantDatagramSender),
             qint64(message1.size()));
    QVERIFY(header.senderAddress == QHostAddress("127.0.0.1"));
    QCOMPARE(header.senderAddress, QHostAddress("127.0.0.1"));
    QVERIFY(header.senderPort != 0);
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::udpIPv6LoopbackTest()
{
    PLATFORMSOCKETENGINE udpSocket;

    // Initialize device #1
    bool init = udpSocket.initialize(QAbstractSocket::UdpSocket, QAbstractSocket::IPv6Protocol);

    if (!init) {
        QCOMPARE(udpSocket.error(), QAbstractSocket::UnsupportedSocketOperationError);
    } else {
        QCOMPARE(udpSocket.protocol(), QAbstractSocket::IPv6Protocol);

        // Bind #1 to localhost
        QVERIFY(udpSocket.bind(QHostAddress("::1"), 0));
        QCOMPARE(udpSocket.state(), QAbstractSocket::BoundState);
        quint16 port = udpSocket.localPort();
        QVERIFY(port != 0);

        // Initialize device #2
        PLATFORMSOCKETENGINE udpSocket2;
        QVERIFY(udpSocket2.initialize(QAbstractSocket::UdpSocket, QAbstractSocket::IPv6Protocol));

        // Connect device #2 to #1
        QVERIFY(udpSocket2.connectToHost(QHostAddress("::1"), port));
        QCOMPARE(udpSocket2.state(), QAbstractSocket::ConnectedState);

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
        QIpPacketHeader header;
        QCOMPARE(udpSocket.readDatagram(answer.data(), answer.size(),
                                        &header, QAbstractSocketEngine::WantDatagramSender),
                 qint64(message1.size()));
        QVERIFY(header.senderAddress == QHostAddress("::1"));
        QCOMPARE(header.senderAddress, QHostAddress("::1"));
        QVERIFY(header.senderPort != 0);
    }
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::broadcastTest()
{
#ifdef Q_OS_AIX
    QSKIP("Broadcast does not work on darko");
#endif
    PLATFORMSOCKETENGINE broadcastSocket;

    // Initialize a regular Udp socket
    QVERIFY(broadcastSocket.initialize(QAbstractSocket::UdpSocket, QAbstractSocket::AnyIPProtocol));

    // Bind to any port on all interfaces
    QVERIFY(broadcastSocket.bind(QHostAddress::Any, 0));
    QCOMPARE(broadcastSocket.state(), QAbstractSocket::BoundState);
    quint16 port = broadcastSocket.localPort();
    QVERIFY(port > 0);

    // Broadcast an inappropriate troll message
    QByteArray trollMessage
        = "MOOT wtf is a MOOT? talk english not your sutpiD ENGLISH.";
    qint64 written = broadcastSocket.writeDatagram(trollMessage.data(),
                                         trollMessage.size(),
                                         QIpPacketHeader(QHostAddress::Broadcast, port));

    QVERIFY2(written != -1, qt_error_string().toLocal8Bit());
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
    QCOMPARE(server.state(), QAbstractSocket::BoundState);
    quint16 port = server.localPort();

    // Listen for incoming connections
    QVERIFY(server.listen());
    QCOMPARE(server.state(), QAbstractSocket::ListeningState);

    // Initialize a Tcp socket
    PLATFORMSOCKETENGINE client;
    QVERIFY(client.initialize(QAbstractSocket::TcpSocket));
    if (!client.connectToHost(QHostAddress("127.0.0.1"), port)) {
        QCOMPARE(client.state(), QAbstractSocket::ConnectingState);
        QVERIFY(client.waitForWrite());
        QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
    }

    // The server accepts the connection
    int socketDescriptor = server.accept();
    QVERIFY(socketDescriptor > 0);

    // A socket device is initialized on the server side, passing the
    // socket descriptor from accept(). It's pre-connected.
    PLATFORMSOCKETENGINE serverSocket;
    QVERIFY(serverSocket.initialize(socketDescriptor));
    QCOMPARE(serverSocket.state(), QAbstractSocket::ConnectedState);

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
    PLATFORMSOCKETENGINE udpSocket;

    // Initialize device #1
    QVERIFY(udpSocket.initialize(QAbstractSocket::UdpSocket));
    QVERIFY(udpSocket.isValid());
    QVERIFY(udpSocket.socketDescriptor() != -1);
    QCOMPARE(udpSocket.protocol(), QAbstractSocket::IPv4Protocol);
    QCOMPARE(udpSocket.socketType(), QAbstractSocket::UdpSocket);
    QCOMPARE(udpSocket.state(), QAbstractSocket::UnconnectedState);

    // Bind #1 to localhost
    QVERIFY(udpSocket.bind(QHostAddress("127.0.0.1"), 0));
    QCOMPARE(udpSocket.state(), QAbstractSocket::BoundState);
    quint16 port = udpSocket.localPort();
    QVERIFY(port != 0);

    // Initialize device #2
    PLATFORMSOCKETENGINE udpSocket2;
    QVERIFY(udpSocket2.initialize(QAbstractSocket::UdpSocket));

    // Connect device #2 to #1
    QVERIFY(udpSocket2.connectToHost(QHostAddress("127.0.0.1"), port));
    QCOMPARE(udpSocket2.state(), QAbstractSocket::ConnectedState);

    const int messageSize = 8192;
    QByteArray message1(messageSize, '@');
    QByteArray answer(messageSize, '@');

    QHostAddress localhost = QHostAddress::LocalHost;

    qlonglong readBytes = 0;
    QElapsedTimer timer;
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
    QCOMPARE(server.state(), QAbstractSocket::BoundState);
    quint16 port = server.localPort();

    // Listen for incoming connections
    QVERIFY(server.listen());
    QCOMPARE(server.state(), QAbstractSocket::ListeningState);

    // Initialize a Tcp socket
    PLATFORMSOCKETENGINE client;
    QVERIFY(client.initialize(QAbstractSocket::TcpSocket));

    // Connect to our server
    if (!client.connectToHost(QHostAddress("127.0.0.1"), port)) {
        QCOMPARE(client.state(), QAbstractSocket::ConnectingState);
        QVERIFY(client.waitForWrite());
        QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
    }

    // The server accepts the connection
    int socketDescriptor = server.accept();
    QVERIFY(socketDescriptor > 0);

    // A socket device is initialized on the server side, passing the
    // socket descriptor from accept(). It's pre-connected.
    PLATFORMSOCKETENGINE serverSocket;
    QVERIFY(serverSocket.initialize(socketDescriptor));
    QCOMPARE(serverSocket.state(), QAbstractSocket::ConnectedState);

    const int messageSize = 1024 * 256;
    QByteArray message1(messageSize, '@');
    QByteArray answer(messageSize, '@');

    QElapsedTimer timer;
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

#if 0   // unused
//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::readWriteBufferSize()
{
    PLATFORMSOCKETENGINE device;

    QVERIFY(device.initialize(QAbstractSocket::TcpSocket));

    qint64 bufferSize = device.receiveBufferSize();
    QVERIFY(bufferSize != -1);
    device.setReceiveBufferSize(bufferSize + 1);
    QVERIFY(device.receiveBufferSize() > bufferSize);

    bufferSize = device.sendBufferSize();
    QVERIFY(bufferSize != -1);
    device.setSendBufferSize(bufferSize + 1);
    QVERIFY(device.sendBufferSize() > bufferSize);

}
#endif

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::tooManySockets()
{
#if defined Q_OS_WIN
    QSKIP("Certain windows machines suffocate and spend too much time in this test.");
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
    PLATFORMSOCKETENGINE binder;
    QVERIFY(binder.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QCOMPARE(binder.bind(QHostAddress::AnyIPv4, 82), QtNetworkSettings::canBindToLowPorts());
    if (!QtNetworkSettings::canBindToLowPorts())
        QCOMPARE(binder.error(), QAbstractSocket::SocketAccessError);

    PLATFORMSOCKETENGINE binder2;
    QVERIFY(binder2.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QVERIFY(binder2.bind(QHostAddress::AnyIPv4, 31180));

    PLATFORMSOCKETENGINE binder3;
    QVERIFY(binder3.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QVERIFY(!binder3.bind(QHostAddress::AnyIPv4, 31180));
    QCOMPARE(binder3.error(), QAbstractSocket::AddressInUseError);

    if (QtNetworkSettings::hasIPv6()) {
        PLATFORMSOCKETENGINE binder;
        QVERIFY(binder.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv6Protocol));
        QCOMPARE(binder.bind(QHostAddress::AnyIPv6, 82), QtNetworkSettings::canBindToLowPorts());
        if (!QtNetworkSettings::canBindToLowPorts())
            QCOMPARE(binder.error(), QAbstractSocket::SocketAccessError);

        PLATFORMSOCKETENGINE binder4;
        QVERIFY(binder4.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv6Protocol));
        QVERIFY(binder4.bind(QHostAddress::AnyIPv6, 31180));

        PLATFORMSOCKETENGINE binder5;
        QVERIFY(binder5.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv6Protocol));
        QVERIFY(!binder5.bind(QHostAddress::AnyIPv6, 31180));
        QCOMPARE(binder5.error(), QAbstractSocket::AddressInUseError);
    }

    PLATFORMSOCKETENGINE binder6;
    QVERIFY(binder6.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::AnyIPProtocol));
    QVERIFY(binder6.bind(QHostAddress::Any, 31181));
}

//---------------------------------------------------------------------------
void tst_PlatformSocketEngine::networkError()
{
    PLATFORMSOCKETENGINE client;

    QVERIFY(client.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));

    const bool isConnected = client.connectToHost(QtNetworkSettings::serverIP(), 143);
    if (!isConnected) {
        QCOMPARE(client.state(), QAbstractSocket::ConnectingState);
        QVERIFY(client.waitForWrite());
        QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
    }
    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);

    // An unexpected network error!
#ifdef Q_OS_WINRT
    client.close();
#elif defined(Q_OS_WIN)
    // could use shutdown to produce different errors
    ::closesocket(client.socketDescriptor());
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

    QTest::ignoreMessage(QtWarningMsg, PLATFORMSOCKETENGINESTRING "::writeDatagram() was called"
                         " not in QAbstractSocket::BoundState or QAbstractSocket::ConnectedState");
    QCOMPARE(socket.writeDatagram("hei", 3, QIpPacketHeader(QHostAddress::LocalHost, 143)),
            (qlonglong) -1);
}

//---------------------------------------------------------------------------
#ifndef Q_OS_WINRT
void tst_PlatformSocketEngine::receiveUrgentData()
{
    PLATFORMSOCKETENGINE server;

    QVERIFY(server.initialize(QAbstractSocket::TcpSocket));

    // Bind to any port on all interfaces
    QVERIFY(server.bind(QHostAddress("0.0.0.0"), 0));
    QCOMPARE(server.state(), QAbstractSocket::BoundState);
    quint16 port = server.localPort();

    QVERIFY(server.listen());
    QCOMPARE(server.state(), QAbstractSocket::ListeningState);

    PLATFORMSOCKETENGINE client;
    QVERIFY(client.initialize(QAbstractSocket::TcpSocket));

    if (!client.connectToHost(QHostAddress("127.0.0.1"), port)) {
        QCOMPARE(client.state(), QAbstractSocket::ConnectingState);
        QVERIFY(client.waitForWrite());
        QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
    }

    int socketDescriptor = server.accept();
    QVERIFY(socketDescriptor > 0);

    PLATFORMSOCKETENGINE serverSocket;
    QVERIFY(serverSocket.initialize(socketDescriptor));
    QCOMPARE(serverSocket.state(), QAbstractSocket::ConnectedState);

    char msg;
    int available;
    QByteArray response;

    // Native OOB data test doesn't work on HP-UX or WinCE
#if !defined(Q_OS_HPUX)
    // The server sends an urgent message
    msg = 'Q';
    QCOMPARE(int(::send(socketDescriptor, &msg, sizeof(msg), MSG_OOB)), 1);

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
    QCOMPARE(int(::send(clientDescriptor, &msg, sizeof(msg), MSG_OOB)), 1);

    // The server receives the urgent message
    QVERIFY(serverSocket.waitForRead());
    available = serverSocket.bytesAvailable();
    QCOMPARE(available, 1);
    response.resize(available);
    QCOMPARE(serverSocket.read(response.data(), response.size()), qint64(1));
    QCOMPARE(response.at(0), msg);
#endif
}
#endif // !Q_OS_WINRT

QTEST_MAIN(tst_PlatformSocketEngine)
#include "tst_platformsocketengine.moc"
