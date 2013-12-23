/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qfileinfo.h>
#include <qdatastream.h>
#include <qudpsocket.h>
#include <qhostaddress.h>
#include <qhostinfo.h>
#include <qtcpsocket.h>
#include <qmap.h>
#include <QNetworkProxy>
#include <QNetworkInterface>

#include <qstringlist.h>
#include "../../../network-settings.h"

#ifndef QT_NO_BEARERMANAGEMENT
#include <QtNetwork/qnetworkconfigmanager.h>
#include <QtNetwork/qnetworkconfiguration.h>
#include <QtNetwork/qnetworksession.h>
#endif

Q_DECLARE_METATYPE(QHostAddress)
Q_DECLARE_METATYPE(QNetworkInterface)

QT_FORWARD_DECLARE_CLASS(QUdpSocket)

class tst_QUdpSocket : public QObject
{
    Q_OBJECT

public:
    tst_QUdpSocket();
    virtual ~tst_QUdpSocket();


public slots:
    void initTestCase_data();
    void initTestCase();
    void init();
    void cleanup();
private slots:
    void constructing();
    void unconnectedServerAndClientTest();
    void broadcasting();
    void loop_data();
    void loop();
    void ipv6Loop_data();
    void ipv6Loop();
    void dualStack();
    void dualStackAutoBinding();
    void dualStackNoIPv4onV6only();
    void readLine();
    void pendingDatagramSize();
    void writeDatagram();
    void performance();
    void bindMode();
    void writeDatagramToNonExistingPeer_data();
    void writeDatagramToNonExistingPeer();
    void writeToNonExistingPeer_data();
    void writeToNonExistingPeer();
#ifndef QT_NO_PROCESS
    void outOfProcessConnectedClientServerTest();
    void outOfProcessUnconnectedClientServerTest();
#endif
    void zeroLengthDatagram();
    void multicastTtlOption_data();
    void multicastTtlOption();
    void multicastLoopbackOption_data();
    void multicastLoopbackOption();
    void multicastJoinBeforeBind_data();
    void multicastJoinBeforeBind();
    void multicastLeaveAfterClose_data();
    void multicastLeaveAfterClose();
    void setMulticastInterface_data();
    void setMulticastInterface();
    void multicast_data();
    void multicast();
    void echo_data();
    void echo();
    void linkLocalIPv6();
    void linkLocalIPv4();

protected slots:
    void empty_readyReadSlot();
    void empty_connectedSlot();

private:
#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkConfigurationManager *netConfMan;
    QNetworkConfiguration networkConfiguration;
    QSharedPointer<QNetworkSession> networkSession;
#endif
};

tst_QUdpSocket::tst_QUdpSocket()
{
}

tst_QUdpSocket::~tst_QUdpSocket()
{
}

void tst_QUdpSocket::initTestCase_data()
{
    // hack: we only enable the Socks5 over UDP tests on the old
    // test server, because they fail on the new one. See QTBUG-35490
    bool newTestServer = true;
    QTcpSocket socket;
    socket.connectToHost(QtNetworkSettings::serverName(), 22);
    if (socket.waitForConnected(10000)) {
        socket.waitForReadyRead(5000);
        QByteArray ba = socket.readAll();
        if (ba.startsWith("SSH-2.0-OpenSSH_5.8p1"))
            newTestServer = false;
        socket.disconnectFromHost();
    }

    QTest::addColumn<bool>("setProxy");
    QTest::addColumn<int>("proxyType");

    QTest::newRow("WithoutProxy") << false << 0;
#ifndef QT_NO_SOCKS5
    if (!newTestServer)
        QTest::newRow("WithSocks5Proxy") << true << int(QNetworkProxy::Socks5Proxy);
#endif

#ifndef QT_NO_BEARERMANAGEMENT
    netConfMan = new QNetworkConfigurationManager(this);
    networkConfiguration = netConfMan->defaultConfiguration();
    networkSession = QSharedPointer<QNetworkSession>(new QNetworkSession(networkConfiguration));
    if (!networkSession->isOpen()) {
        networkSession->open();
        QVERIFY(networkSession->waitForOpened(30000));
    }
#endif
}

void tst_QUdpSocket::initTestCase()
{
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
}

void tst_QUdpSocket::init()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_SOCKS5
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080));
        }
#else
        QSKIP("No proxy support");
#endif // !QT_NO_SOCKS5
    }
}

void tst_QUdpSocket::cleanup()
{
#ifndef QT_NO_NETWORKPROXY
        QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
#endif // !QT_NO_NETWORKPROXY
}


//----------------------------------------------------------------------------------

void tst_QUdpSocket::constructing()
{
    QUdpSocket socket;
#ifdef FORCE_SESSION
    socket.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif

    QVERIFY(socket.isSequential());
    QVERIFY(!socket.isOpen());
    QVERIFY(socket.socketType() == QUdpSocket::UdpSocket);
    QCOMPARE((int) socket.bytesAvailable(), 0);
    QCOMPARE(socket.canReadLine(), false);
    QCOMPARE(socket.readLine(), QByteArray());
    QCOMPARE(socket.socketDescriptor(), (qintptr)-1);
    QCOMPARE(socket.error(), QUdpSocket::UnknownSocketError);
    QCOMPARE(socket.errorString(), QString("Unknown error"));

    // Check the state of the socket api
}

void tst_QUdpSocket::unconnectedServerAndClientTest()
{
    QUdpSocket serverSocket;
#ifdef FORCE_SESSION
    serverSocket.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif

    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");

    QSignalSpy stateChangedSpy(&serverSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)));
    QVERIFY2(serverSocket.bind(), serverSocket.errorString().toLatin1().constData());
    QCOMPARE(stateChangedSpy.count(), 1);

    const char *message[] = {"Yo mista", "Yo", "Wassap"};

    QHostAddress serverAddress = QHostAddress::LocalHost;
    if (!(serverSocket.localAddress() == QHostAddress::AnyIPv4 || serverSocket.localAddress() == QHostAddress::AnyIPv6))
        serverAddress = serverSocket.localAddress();

    for (int i = 0; i < 3; ++i) {
        QUdpSocket clientSocket;
#ifdef FORCE_SESSION
        clientSocket.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
        QCOMPARE(int(clientSocket.writeDatagram(message[i], strlen(message[i]),
                                               serverAddress, serverSocket.localPort())),
                int(strlen(message[i])));
        char buf[1024];
        QHostAddress host;
        quint16 port;
        QVERIFY(serverSocket.waitForReadyRead(5000));
        QCOMPARE(int(serverSocket.readDatagram(buf, sizeof(buf), &host, &port)),
                int(strlen(message[i])));
        buf[strlen(message[i])] = '\0';
        QCOMPARE(QByteArray(buf), QByteArray(message[i]));
    }
}

//----------------------------------------------------------------------------------

void tst_QUdpSocket::broadcasting()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy)
            QSKIP("With socks5 Broadcast is not supported.");
#else // !QT_NO_NETWORKPROXY
        QSKIP("No proxy support");
#endif // QT_NO_NETWORKPROXY
    }
#ifdef Q_OS_AIX
    QSKIP("Broadcast does not work on darko");
#endif
    const char *message[] = {"Yo mista", "", "Yo", "Wassap"};

    QList<QHostAddress> broadcastAddresses;
    foreach (QNetworkInterface iface, QNetworkInterface::allInterfaces()) {
        if ((iface.flags() & QNetworkInterface::CanBroadcast)
            && iface.flags() & QNetworkInterface::IsUp) {
            for (int i=0;i<iface.addressEntries().count();i++)
                broadcastAddresses.append(iface.addressEntries().at(i).broadcast());
        }
    }
    if (broadcastAddresses.isEmpty())
        QSKIP("No interface can broadcast");
    for (int i = 0; i < 4; ++i) {
        QUdpSocket serverSocket;
#ifdef FORCE_SESSION
        serverSocket.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
        QVERIFY2(serverSocket.bind(QHostAddress::Any, 5000), serverSocket.errorString().toLatin1().constData());

        QCOMPARE(serverSocket.state(), QUdpSocket::BoundState);

        connect(&serverSocket, SIGNAL(readyRead()), SLOT(empty_readyReadSlot()));

        QUdpSocket broadcastSocket;
#ifdef FORCE_SESSION
        broadcastSocket.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
        broadcastSocket.bind();

        for (int j = 0; j < 100; ++j) {
            for (int k = 0; k < 4; k++) {
                broadcastSocket.writeDatagram(message[i], strlen(message[i]),
                    QHostAddress::Broadcast, 5000);
                foreach (QHostAddress addr, broadcastAddresses)
                    broadcastSocket.writeDatagram(message[i], strlen(message[i]), addr, 5000);
            }
            QTestEventLoop::instance().enterLoop(15);
            if (QTestEventLoop::instance().timeout()) {
#if defined(Q_OS_FREEBSD)
                QEXPECT_FAIL("",
                             "Broadcasting to 255.255.255.255 does not work on FreeBSD",
                             Abort);
                QVERIFY(false); // seems that QFAIL() doesn't respect the QEXPECT_FAIL() :/
#endif
                QFAIL("Network operation timed out");
            }
            QVERIFY(serverSocket.hasPendingDatagrams());

            do {
                QByteArray arr; arr.resize(serverSocket.pendingDatagramSize() + 1);
                QHostAddress host;
                quint16 port;
                const int messageLength = int(strlen(message[i]));
                QCOMPARE((int) serverSocket.readDatagram(arr.data(), arr.size() - 1, &host, &port),
                         messageLength);
                arr.resize(messageLength);
                QCOMPARE(arr, QByteArray(message[i]));
            } while (serverSocket.hasPendingDatagrams());
        }
    }
}

//----------------------------------------------------------------------------------

void tst_QUdpSocket::loop_data()
{
    QTest::addColumn<QByteArray>("peterMessage");
    QTest::addColumn<QByteArray>("paulMessage");
    QTest::addColumn<bool>("success");

    QTest::newRow("\"Almond!\" | \"Joy!\"") << QByteArray("Almond!") << QByteArray("Joy!") << true;
    QTest::newRow("\"A\" | \"B\"") << QByteArray("A") << QByteArray("B") << true;
    QTest::newRow("\"AB\" | \"B\"") << QByteArray("AB") << QByteArray("B") << true;
    QTest::newRow("\"AB\" | \"BB\"") << QByteArray("AB") << QByteArray("BB") << true;
    QTest::newRow("\"A\\0B\" | \"B\\0B\"") << QByteArray::fromRawData("A\0B", 3) << QByteArray::fromRawData("B\0B", 3) << true;
    QTest::newRow("\"(nil)\" | \"(nil)\"") << QByteArray() << QByteArray() << true;
    QTest::newRow("Bigmessage") << QByteArray(600, '@') << QByteArray(600, '@') << true;
}

void tst_QUdpSocket::loop()
{
    QFETCH(QByteArray, peterMessage);
    QFETCH(QByteArray, paulMessage);
    QFETCH(bool, success);

    QUdpSocket peter;
    QUdpSocket paul;
#ifdef FORCE_SESSION
    peter.setProperty("_q_networksession", QVariant::fromValue(networkSession));
    paul.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif

    QVERIFY2(peter.bind(), peter.errorString().toLatin1().constData());
    QVERIFY2(paul.bind(), paul.errorString().toLatin1().constData());

    QHostAddress peterAddress = QHostAddress::LocalHost;
    if (!(peter.localAddress() == QHostAddress::AnyIPv4 || peter.localAddress() == QHostAddress::AnyIPv6))
        peterAddress = peter.localAddress();
    QHostAddress pualAddress = QHostAddress::LocalHost;
    if (!(paul.localAddress() == QHostAddress::AnyIPv4 || paul.localAddress() == QHostAddress::AnyIPv6))
        pualAddress = paul.localAddress();

    QCOMPARE(peter.writeDatagram(peterMessage.data(), peterMessage.length(),
                                pualAddress, paul.localPort()), qint64(peterMessage.length()));
    QCOMPARE(paul.writeDatagram(paulMessage.data(), paulMessage.length(),
                               peterAddress, peter.localPort()), qint64(paulMessage.length()));

    QVERIFY(peter.waitForReadyRead(9000));
    QVERIFY(paul.waitForReadyRead(9000));
    char peterBuffer[16*1024];
    char paulBuffer[16*1024];
    if (success) {
        QCOMPARE(peter.readDatagram(peterBuffer, sizeof(peterBuffer)), qint64(paulMessage.length()));
        QCOMPARE(paul.readDatagram(paulBuffer, sizeof(peterBuffer)), qint64(peterMessage.length()));
    } else {
        QVERIFY(peter.readDatagram(peterBuffer, sizeof(peterBuffer)) != paulMessage.length());
        QVERIFY(paul.readDatagram(paulBuffer, sizeof(peterBuffer)) != peterMessage.length());
    }

    QCOMPARE(QByteArray(peterBuffer, paulMessage.length()), paulMessage);
    QCOMPARE(QByteArray(paulBuffer, peterMessage.length()), peterMessage);
}

//----------------------------------------------------------------------------------

void tst_QUdpSocket::ipv6Loop_data()
{
    loop_data();
}

void tst_QUdpSocket::ipv6Loop()
{
    QFETCH(QByteArray, peterMessage);
    QFETCH(QByteArray, paulMessage);
    QFETCH(bool, success);

    QUdpSocket peter;
    QUdpSocket paul;
#ifdef FORCE_SESSION
    peter.setProperty("_q_networksession", QVariant::fromValue(networkSession));
    paul.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif

    quint16 peterPort = 28124;
    quint16 paulPort = 28123;

    if (!peter.bind(QHostAddress::LocalHostIPv6, peterPort)) {
    QCOMPARE(peter.error(), QUdpSocket::UnsupportedSocketOperationError);
    } else {
    QVERIFY(paul.bind(QHostAddress::LocalHostIPv6, paulPort));

    QCOMPARE(peter.writeDatagram(peterMessage.data(), peterMessage.length(), QHostAddress("::1"),
                                    paulPort), qint64(peterMessage.length()));
    QCOMPARE(paul.writeDatagram(paulMessage.data(), paulMessage.length(),
                                   QHostAddress("::1"), peterPort), qint64(paulMessage.length()));

    char peterBuffer[16*1024];
    char paulBuffer[16*1024];
#if !defined(Q_OS_WINCE)
        QVERIFY(peter.waitForReadyRead(5000));
        QVERIFY(paul.waitForReadyRead(5000));
#else
        QVERIFY(peter.waitForReadyRead(15000));
        QVERIFY(paul.waitForReadyRead(15000));
#endif
    if (success) {
        QCOMPARE(peter.readDatagram(peterBuffer, sizeof(peterBuffer)), qint64(paulMessage.length()));
        QCOMPARE(paul.readDatagram(paulBuffer, sizeof(peterBuffer)), qint64(peterMessage.length()));
    } else {
        QVERIFY(peter.readDatagram(peterBuffer, sizeof(peterBuffer)) != paulMessage.length());
        QVERIFY(paul.readDatagram(paulBuffer, sizeof(peterBuffer)) != peterMessage.length());
    }

    QCOMPARE(QByteArray(peterBuffer, paulMessage.length()), paulMessage);
    QCOMPARE(QByteArray(paulBuffer, peterMessage.length()), peterMessage);
    }
}

void tst_QUdpSocket::dualStack()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        QSKIP("test server SOCKS proxy doesn't support IPv6");
    QUdpSocket dualSock;
    QByteArray dualData("dual");
    QVERIFY(dualSock.bind(QHostAddress(QHostAddress::Any), 0));

    QUdpSocket v4Sock;
    QByteArray v4Data("v4");
    QVERIFY(v4Sock.bind(QHostAddress(QHostAddress::AnyIPv4), 0));

    QHostAddress from;
    quint16 port;
    QByteArray buffer;
    //test v4 -> dual
    QCOMPARE((int)v4Sock.writeDatagram(v4Data.constData(), v4Data.length(), QHostAddress(QHostAddress::LocalHost), dualSock.localPort()), v4Data.length());
    QVERIFY(dualSock.waitForReadyRead(5000));
    buffer.reserve(100);
    qint64 size = dualSock.readDatagram(buffer.data(), 100, &from, &port);
    QCOMPARE((int)size, v4Data.length());
    buffer.resize(size);
    QCOMPARE(buffer, v4Data);

    if (QtNetworkSettings::hasIPv6()) {
        QUdpSocket v6Sock;
        QByteArray v6Data("v6");
        QVERIFY(v6Sock.bind(QHostAddress(QHostAddress::AnyIPv6), 0));

        //test v6 -> dual
        QCOMPARE((int)v6Sock.writeDatagram(v6Data.constData(), v6Data.length(), QHostAddress(QHostAddress::LocalHostIPv6), dualSock.localPort()), v6Data.length());
        QVERIFY(dualSock.waitForReadyRead(5000));
        buffer.reserve(100);
        size = dualSock.readDatagram(buffer.data(), 100, &from, &port);
        QCOMPARE((int)size, v6Data.length());
        buffer.resize(size);
        QCOMPARE(buffer, v6Data);

        //test dual -> v6
        QCOMPARE((int)dualSock.writeDatagram(dualData.constData(), dualData.length(), QHostAddress(QHostAddress::LocalHostIPv6), v6Sock.localPort()), dualData.length());
        QVERIFY(v6Sock.waitForReadyRead(5000));
        buffer.reserve(100);
        size = v6Sock.readDatagram(buffer.data(), 100, &from, &port);
        QCOMPARE((int)size, dualData.length());
        buffer.resize(size);
        QCOMPARE(buffer, dualData);
    }

    //test dual -> v4
    QCOMPARE((int)dualSock.writeDatagram(dualData.constData(), dualData.length(), QHostAddress(QHostAddress::LocalHost), v4Sock.localPort()), dualData.length());
    QVERIFY(v4Sock.waitForReadyRead(5000));
    buffer.reserve(100);
    size = v4Sock.readDatagram(buffer.data(), 100, &from, &port);
    QCOMPARE((int)size, dualData.length());
    buffer.resize(size);
    QCOMPARE(buffer, dualData);
}

void tst_QUdpSocket::dualStackAutoBinding()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        QSKIP("test server SOCKS proxy doesn't support IPv6");
    if (!QtNetworkSettings::hasIPv6())
        QSKIP("system doesn't support ipv6!");
    QUdpSocket v4Sock;
    QVERIFY(v4Sock.bind(QHostAddress(QHostAddress::AnyIPv4), 0));

    QUdpSocket v6Sock;
    QVERIFY(v6Sock.bind(QHostAddress(QHostAddress::AnyIPv6), 0));

    QByteArray dualData("dual");
    QHostAddress from;
    quint16 port;
    QByteArray buffer;
    int size;

    {
        //test an autobound socket can send to both v4 and v6 addresses (v4 first)
        QUdpSocket dualSock;

        QCOMPARE((int)dualSock.writeDatagram(dualData.constData(), dualData.length(), QHostAddress(QHostAddress::LocalHost), v4Sock.localPort()), dualData.length());
        QVERIFY(v4Sock.waitForReadyRead(5000));
        buffer.reserve(100);
        size = v4Sock.readDatagram(buffer.data(), 100, &from, &port);
        QCOMPARE((int)size, dualData.length());
        buffer.resize(size);
        QCOMPARE(buffer, dualData);

        QCOMPARE((int)dualSock.writeDatagram(dualData.constData(), dualData.length(), QHostAddress(QHostAddress::LocalHostIPv6), v6Sock.localPort()), dualData.length());
        QVERIFY(v6Sock.waitForReadyRead(5000));
        buffer.reserve(100);
        size = v6Sock.readDatagram(buffer.data(), 100, &from, &port);
        QCOMPARE((int)size, dualData.length());
        buffer.resize(size);
        QCOMPARE(buffer, dualData);
    }

    {
        //test an autobound socket can send to both v4 and v6 addresses (v6 first)
        QUdpSocket dualSock;

        QCOMPARE((int)dualSock.writeDatagram(dualData.constData(), dualData.length(), QHostAddress(QHostAddress::LocalHostIPv6), v6Sock.localPort()), dualData.length());
        QVERIFY(v6Sock.waitForReadyRead(5000));
        buffer.reserve(100);
        size = v6Sock.readDatagram(buffer.data(), 100, &from, &port);
        QCOMPARE((int)size, dualData.length());
        buffer.resize(size);
        QCOMPARE(buffer, dualData);

        QCOMPARE((int)dualSock.writeDatagram(dualData.constData(), dualData.length(), QHostAddress(QHostAddress::LocalHost), v4Sock.localPort()), dualData.length());
        QVERIFY(v4Sock.waitForReadyRead(5000));
        buffer.reserve(100);
        size = v4Sock.readDatagram(buffer.data(), 100, &from, &port);
        QCOMPARE((int)size, dualData.length());
        buffer.resize(size);
        QCOMPARE(buffer, dualData);
    }
}

void tst_QUdpSocket::dualStackNoIPv4onV6only()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        QSKIP("test server SOCKS proxy doesn't support IPv6");
    if (!QtNetworkSettings::hasIPv6())
        QSKIP("system doesn't support ipv6!");
    QUdpSocket v4Sock;
    QVERIFY(v4Sock.bind(QHostAddress(QHostAddress::AnyIPv4), 0));
    QByteArray v4Data("v4");

    QUdpSocket v6Sock;
    QVERIFY(v6Sock.bind(QHostAddress(QHostAddress::AnyIPv6), 0));

    //test v4 -> v6 (should not be received as this is a v6 only socket)
    QCOMPARE((int)v4Sock.writeDatagram(v4Data.constData(), v4Data.length(), QHostAddress(QHostAddress::LocalHost), v6Sock.localPort()), v4Data.length());
    QVERIFY(!v6Sock.waitForReadyRead(1000));
}

void tst_QUdpSocket::empty_readyReadSlot()
{
    QTestEventLoop::instance().exitLoop();
}

void tst_QUdpSocket::empty_connectedSlot()
{
    QTestEventLoop::instance().exitLoop();
}

//----------------------------------------------------------------------------------

void tst_QUdpSocket::readLine()
{
    QUdpSocket socket1;
    QUdpSocket socket2;
#ifdef FORCE_SESSION
    socket1.setProperty("_q_networksession", QVariant::fromValue(networkSession));
    socket2.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QVERIFY2(socket1.bind(), socket1.errorString().toLatin1().constData());

    socket2.connectToHost("127.0.0.1", socket1.localPort());
    QVERIFY(socket2.waitForConnected(5000));
}

//----------------------------------------------------------------------------------

void tst_QUdpSocket::pendingDatagramSize()
{
    QUdpSocket server;
#ifdef FORCE_SESSION
    server.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QVERIFY2(server.bind(), server.errorString().toLatin1().constData());

    QHostAddress serverAddress = QHostAddress::LocalHost;
    if (!(server.localAddress() == QHostAddress::AnyIPv4 || server.localAddress() == QHostAddress::AnyIPv6))
        serverAddress = server.localAddress();

    QUdpSocket client;
#ifdef FORCE_SESSION
    client.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QVERIFY(client.writeDatagram("this is", 7, serverAddress, server.localPort()) == 7);
    QVERIFY(client.writeDatagram(0, 0, serverAddress, server.localPort()) == 0);
    QVERIFY(client.writeDatagram("3 messages", 10, serverAddress, server.localPort()) == 10);

    char c = 0;
    QVERIFY(server.waitForReadyRead());
    if (server.hasPendingDatagrams()) {
#if defined Q_OS_HPUX && defined __ia64
        QEXPECT_FAIL("", "HP-UX 11i v2 can't determine the datagram size correctly.", Abort);
#endif
        QCOMPARE(server.pendingDatagramSize(), qint64(7));
        c = '\0';
        QCOMPARE(server.readDatagram(&c, 1), qint64(1));
        QCOMPARE(c, 't');
        c = '\0';
    } else
        QSKIP("does not have the 1st datagram");

    if (server.hasPendingDatagrams()) {
        QCOMPARE(server.pendingDatagramSize(), qint64(0));
        QCOMPARE(server.readDatagram(&c, 1), qint64(0));
        QCOMPARE(c, '\0'); // untouched
        c = '\0';
    } else
        QSKIP("does not have the 2nd datagram");

    if (server.hasPendingDatagrams()) {
        QCOMPARE(server.pendingDatagramSize(), qint64(10));
        QCOMPARE(server.readDatagram(&c, 1), qint64(1));
        QCOMPARE(c, '3');
    } else
        QSKIP("does not have the 3rd datagram");
}


void tst_QUdpSocket::writeDatagram()
{
    QUdpSocket server;
#ifdef FORCE_SESSION
    server.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QVERIFY2(server.bind(), server.errorString().toLatin1().constData());

    QHostAddress serverAddress = QHostAddress::LocalHost;
    if (!(server.localAddress() == QHostAddress::AnyIPv4 || server.localAddress() == QHostAddress::AnyIPv6))
        serverAddress = server.localAddress();

    QUdpSocket client;
#ifdef FORCE_SESSION
    client.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif

    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");

    for(int i=0;;i++) {
        QSignalSpy errorspy(&client, SIGNAL(error(QAbstractSocket::SocketError)));
        QSignalSpy bytesspy(&client, SIGNAL(bytesWritten(qint64)));

        qint64 written = client.writeDatagram(QByteArray(i * 1024, 'w'), serverAddress,
                                                  server.localPort());

        if (written != i * 1024) {
#if defined (Q_OS_HPUX)
            QSKIP("HP-UX 11.11 on hai (PA-RISC 64) truncates too long datagrams.");
#endif
            QCOMPARE(bytesspy.count(), 0);
            QCOMPARE(errorspy.count(), 1);
            QCOMPARE(*static_cast<const int *>(errorspy.at(0).at(0).constData()),
                    int(QUdpSocket::DatagramTooLargeError));
            QCOMPARE(client.error(), QUdpSocket::DatagramTooLargeError);
            break;
        }
        QVERIFY(bytesspy.count() == 1);
        QCOMPARE(*static_cast<const qint64 *>(bytesspy.at(0).at(0).constData()),
                qint64(i * 1024));
        QCOMPARE(errorspy.count(), 0);
        if (!server.waitForReadyRead(5000))
            QSKIP(QString("UDP packet lost at size %1, unable to complete the test.").arg(i * 1024).toLatin1().data());
        QCOMPARE(server.pendingDatagramSize(), qint64(i * 1024));
        QCOMPARE(server.readDatagram(0, 0), qint64(0));
    }
}

void tst_QUdpSocket::performance()
{
    QByteArray arr(8192, '@');

    QUdpSocket server;
#ifdef FORCE_SESSION
    server.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QVERIFY2(server.bind(), server.errorString().toLatin1().constData());

    QHostAddress serverAddress = QHostAddress::LocalHost;
    if (!(server.localAddress() == QHostAddress::AnyIPv4 || server.localAddress() == QHostAddress::AnyIPv6))
        serverAddress = server.localAddress();

    QUdpSocket client;
#ifdef FORCE_SESSION
    client.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    client.connectToHost(serverAddress, server.localPort());
    QVERIFY(client.waitForConnected(10000));

    QTime stopWatch;
    stopWatch.start();

    qint64 nbytes = 0;
    while (stopWatch.elapsed() < 5000) {
        for (int i = 0; i < 100; ++i) {
            if (client.write(arr.data(), arr.size()) > 0) {
                do {
                    nbytes += server.readDatagram(arr.data(), arr.size());
                } while (server.hasPendingDatagrams());
            }
        }
    }

    float secs = stopWatch.elapsed() / 1000.0;
    qDebug("\t%.2fMB/%.2fs: %.2fMB/s", float(nbytes / (1024.0*1024.0)),
           secs, float(nbytes / (1024.0*1024.0)) / secs);
}

void tst_QUdpSocket::bindMode()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy)
            QSKIP("With socks5 explicit port binding is not supported.");
#else // !QT_NO_NETWORKPROXY
        QSKIP("No proxy support");
#endif // QT_NO_NETWORKPROXY
    }

    QUdpSocket socket;
#ifdef FORCE_SESSION
    socket.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QVERIFY2(socket.bind(), socket.errorString().toLatin1().constData());
    QUdpSocket socket2;
#ifdef FORCE_SESSION
    socket2.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QVERIFY(!socket2.bind(socket.localPort()));
#if defined(Q_OS_UNIX)
    QVERIFY(!socket2.bind(socket.localPort(), QUdpSocket::ReuseAddressHint));
    socket.close();
    QVERIFY2(socket.bind(0, QUdpSocket::ShareAddress), socket.errorString().toLatin1().constData());
    QVERIFY2(socket2.bind(socket.localPort()), socket2.errorString().toLatin1().constData());
    socket2.close();
    QVERIFY2(socket2.bind(socket.localPort(), QUdpSocket::ReuseAddressHint), socket2.errorString().toLatin1().constData());
#else

    // Depending on the user's privileges, this or will succeed or
    // fail. Admins are allowed to reuse the address, but nobody else.
    if (!socket2.bind(socket.localPort(), QUdpSocket::ReuseAddressHint), socket2.errorString().toLatin1().constData())
        qWarning("Failed to bind with QUdpSocket::ReuseAddressHint, user isn't an administrator?");
    socket.close();
    QVERIFY2(socket.bind(0, QUdpSocket::ShareAddress), socket.errorString().toLatin1().constData());
    QVERIFY(!socket2.bind(socket.localPort()));
    socket.close();
    QVERIFY2(socket.bind(0, QUdpSocket::DontShareAddress), socket.errorString().toLatin1().constData());
    QVERIFY(!socket2.bind(socket.localPort()));
    QVERIFY(!socket2.bind(socket.localPort(), QUdpSocket::ReuseAddressHint));
#endif
}

void tst_QUdpSocket::writeDatagramToNonExistingPeer_data()
{
    QTest::addColumn<bool>("bind");
    QTest::addColumn<QHostAddress>("peerAddress");
    QHostAddress localhost(QHostAddress::LocalHost);
    QHostAddress remote = QHostInfo::fromName(QtNetworkSettings::serverName()).addresses().first();

    QTest::newRow("localhost-unbound") << false << localhost;
    QTest::newRow("localhost-bound") << true << localhost;
    QTest::newRow("remote-unbound") << false << remote;
    QTest::newRow("remote-bound") << true << remote;
}

void tst_QUdpSocket::writeDatagramToNonExistingPeer()
{
    QFETCH(bool, bind);
    QFETCH(QHostAddress, peerAddress);

    quint16 peerPort = 33533 + int(bind);

    QUdpSocket sUdp;
#ifdef FORCE_SESSION
    sUdp.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QSignalSpy sReadyReadSpy(&sUdp, SIGNAL(readyRead()));
    if (bind)
        QVERIFY(sUdp.bind());
    QCOMPARE(sUdp.writeDatagram("", 1, peerAddress, peerPort), qint64(1));
    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(sReadyReadSpy.count(), 0);
}

void tst_QUdpSocket::writeToNonExistingPeer_data()
{
    QTest::addColumn<QHostAddress>("peerAddress");
    QHostAddress localhost(QHostAddress::LocalHost);
    QHostAddress remote = QHostInfo::fromName(QtNetworkSettings::serverName()).addresses().first();
    // write (required to be connected)
    QTest::newRow("localhost") << localhost;
    QTest::newRow("remote") << remote;
}

void tst_QUdpSocket::writeToNonExistingPeer()
{
    QSKIP("Connected-mode UDP sockets and their behaviour are erratic");
    QFETCH(QHostAddress, peerAddress);
    quint16 peerPort = 34534;
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");

    QUdpSocket sConnected;
#ifdef FORCE_SESSION
    sConnected.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QSignalSpy sConnectedReadyReadSpy(&sConnected, SIGNAL(readyRead()));
    QSignalSpy sConnectedErrorSpy(&sConnected, SIGNAL(error(QAbstractSocket::SocketError)));
    sConnected.connectToHost(peerAddress, peerPort, QIODevice::ReadWrite);
    QVERIFY(sConnected.waitForConnected(10000));

    // the first write succeeds...
    QCOMPARE(sConnected.write("", 1), qint64(1));

    // the second one should fail!
    QTest::qSleep(1000);                   // do not process events
    QCOMPARE(sConnected.write("", 1), qint64(-1));
    QCOMPARE(int(sConnected.error()), int(QUdpSocket::ConnectionRefusedError));

    // the third one will succeed...
    QCOMPARE(sConnected.write("", 1), qint64(1));
    QTestEventLoop::instance().enterLoop(1);
    QCOMPARE(sConnectedReadyReadSpy.count(), 0);
    QCOMPARE(sConnectedErrorSpy.count(), 1);
    QCOMPARE(int(sConnected.error()), int(QUdpSocket::ConnectionRefusedError));

    // we should now get a read error
    QCOMPARE(sConnected.write("", 1), qint64(1));
    QTest::qSleep(1000);                   // do not process events
    char buf[2];
    QVERIFY(!sConnected.hasPendingDatagrams());
    QCOMPARE(sConnected.bytesAvailable(), Q_INT64_C(0));
    QCOMPARE(sConnected.pendingDatagramSize(), Q_INT64_C(-1));
    QCOMPARE(sConnected.readDatagram(buf, 2), Q_INT64_C(-1));
    QCOMPARE(int(sConnected.error()), int(QUdpSocket::ConnectionRefusedError));

    QCOMPARE(sConnected.write("", 1), qint64(1));
    QTest::qSleep(1000);                   // do not process events
    QCOMPARE(sConnected.read(buf, 2), Q_INT64_C(0));
    QCOMPARE(int(sConnected.error()), int(QUdpSocket::ConnectionRefusedError));

    // we should still be connected
    QCOMPARE(int(sConnected.state()), int(QUdpSocket::ConnectedState));
}

#ifndef QT_NO_PROCESS
void tst_QUdpSocket::outOfProcessConnectedClientServerTest()
{
#if defined(Q_OS_WINCE)
    QSKIP("This test depends on reading data from QProcess (not supported on Qt/WinCE).");
#endif
    QProcess serverProcess;
    serverProcess.start(QLatin1String("clientserver/clientserver server 1 1"),
                        QIODevice::ReadWrite | QIODevice::Text);
    QVERIFY2(serverProcess.waitForStarted(3000),
             qPrintable("Failed to start subprocess: " + serverProcess.errorString()));

    // Wait until the server has started and reports success.
    while (!serverProcess.canReadLine())
        QVERIFY(serverProcess.waitForReadyRead(3000));
    QByteArray serverGreeting = serverProcess.readLine();
    QVERIFY(serverGreeting != QByteArray("XXX\n"));
    int serverPort = serverGreeting.trimmed().toInt();
    QVERIFY(serverPort > 0 && serverPort < 65536);

    QProcess clientProcess;
    clientProcess.start(QString::fromLatin1("clientserver/clientserver connectedclient %1 %2")
                        .arg(QLatin1String("127.0.0.1")).arg(serverPort),
                        QIODevice::ReadWrite | QIODevice::Text);
    QVERIFY2(clientProcess.waitForStarted(3000),
             qPrintable("Failed to start subprocess: " + clientProcess.errorString()));

    // Wait until the server has started and reports success.
    while (!clientProcess.canReadLine())
        QVERIFY(clientProcess.waitForReadyRead(3000));
    QByteArray clientGreeting = clientProcess.readLine();
    QCOMPARE(clientGreeting, QByteArray("ok\n"));

    // Let the client and server talk for 3 seconds
    QTest::qWait(3000);

    QStringList serverData = QString::fromLocal8Bit(serverProcess.readAll()).split("\n");
    QStringList clientData = QString::fromLocal8Bit(clientProcess.readAll()).split("\n");
    QVERIFY(serverData.size() > 5);
    QVERIFY(clientData.size() > 5);

    for (int i = 0; i < clientData.size() / 2; ++i) {
        QCOMPARE(clientData.at(i * 2), QString("readData()"));
        QCOMPARE(serverData.at(i * 3), QString("readData()"));

        QString cdata = clientData.at(i * 2 + 1);
        QString sdata = serverData.at(i * 3 + 1);
        QVERIFY(cdata.startsWith(QLatin1String("got ")));

        QCOMPARE(cdata.mid(4).trimmed().toInt(), sdata.mid(4).trimmed().toInt() * 2);
        QVERIFY(serverData.at(i * 3 + 2).startsWith(QLatin1String("sending ")));
        QCOMPARE(serverData.at(i * 3 + 2).trimmed().mid(8).toInt(),
                 sdata.mid(4).trimmed().toInt() * 2);
    }

    clientProcess.kill();
    QVERIFY(clientProcess.waitForFinished());
    serverProcess.kill();
    QVERIFY(serverProcess.waitForFinished());
}
#endif

#ifndef QT_NO_PROCESS
void tst_QUdpSocket::outOfProcessUnconnectedClientServerTest()
{
#if defined(Q_OS_WINCE)
    QSKIP("This test depends on reading data from QProcess (not supported on Qt/WinCE).");
#endif
    QProcess serverProcess;
    serverProcess.start(QLatin1String("clientserver/clientserver server 1 1"),
                        QIODevice::ReadWrite | QIODevice::Text);
    QVERIFY2(serverProcess.waitForStarted(3000),
             qPrintable("Failed to start subprocess: " + serverProcess.errorString()));

    // Wait until the server has started and reports success.
    while (!serverProcess.canReadLine())
        QVERIFY(serverProcess.waitForReadyRead(3000));
    QByteArray serverGreeting = serverProcess.readLine();
    QVERIFY(serverGreeting != QByteArray("XXX\n"));
    int serverPort = serverGreeting.trimmed().toInt();
    QVERIFY(serverPort > 0 && serverPort < 65536);

    QProcess clientProcess;
    clientProcess.start(QString::fromLatin1("clientserver/clientserver unconnectedclient %1 %2")
                        .arg(QLatin1String("127.0.0.1")).arg(serverPort),
                        QIODevice::ReadWrite | QIODevice::Text);
    QVERIFY2(clientProcess.waitForStarted(3000),
             qPrintable("Failed to start subprocess: " + clientProcess.errorString()));

    // Wait until the server has started and reports success.
    while (!clientProcess.canReadLine())
        QVERIFY(clientProcess.waitForReadyRead(3000));
    QByteArray clientGreeting = clientProcess.readLine();
    QCOMPARE(clientGreeting, QByteArray("ok\n"));

    // Let the client and server talk for 3 seconds
    QTest::qWait(3000);

    QStringList serverData = QString::fromLocal8Bit(serverProcess.readAll()).split("\n");
    QStringList clientData = QString::fromLocal8Bit(clientProcess.readAll()).split("\n");

    QVERIFY(serverData.size() > 5);
    QVERIFY(clientData.size() > 5);

    for (int i = 0; i < clientData.size() / 2; ++i) {
        QCOMPARE(clientData.at(i * 2), QString("readData()"));
        QCOMPARE(serverData.at(i * 3), QString("readData()"));

        QString cdata = clientData.at(i * 2 + 1);
        QString sdata = serverData.at(i * 3 + 1);
        QVERIFY(cdata.startsWith(QLatin1String("got ")));

        QCOMPARE(cdata.mid(4).trimmed().toInt(), sdata.mid(4).trimmed().toInt() * 2);
        QVERIFY(serverData.at(i * 3 + 2).startsWith(QLatin1String("sending ")));
        QCOMPARE(serverData.at(i * 3 + 2).trimmed().mid(8).toInt(),
                 sdata.mid(4).trimmed().toInt() * 2);
    }

    clientProcess.kill();
    QVERIFY(clientProcess.waitForFinished());
    serverProcess.kill();
    QVERIFY(serverProcess.waitForFinished());
}
#endif

void tst_QUdpSocket::zeroLengthDatagram()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QUdpSocket receiver;
#ifdef FORCE_SESSION
    receiver.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QVERIFY(receiver.bind());

    QVERIFY(!receiver.waitForReadyRead(100));
    QVERIFY(!receiver.hasPendingDatagrams());

    QUdpSocket sender;
#ifdef FORCE_SESSION
    sender.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QCOMPARE(sender.writeDatagram(QByteArray(), QHostAddress::LocalHost, receiver.localPort()), qint64(0));

    QVERIFY(receiver.waitForReadyRead(1000));
    QVERIFY(receiver.hasPendingDatagrams());

    char buf;
    QCOMPARE(receiver.readDatagram(&buf, 1), qint64(0));
}

void tst_QUdpSocket::multicastTtlOption_data()
{
    QTest::addColumn<QHostAddress>("bindAddress");
    QTest::addColumn<int>("ttl");
    QTest::addColumn<int>("expected");

    QList<QHostAddress> addresses;
    addresses += QHostAddress(QHostAddress::Any);
    addresses += QHostAddress(QHostAddress::AnyIPv6);

    foreach (const QHostAddress &address, addresses) {
        QTest::newRow(QString("%1 0").arg(address.toString()).toLatin1()) << address << 0 << 0;
        QTest::newRow(QString("%1 1").arg(address.toString()).toLatin1()) << address << 1 << 1;
        QTest::newRow(QString("%1 2").arg(address.toString()).toLatin1()) << address << 2 << 2;
        QTest::newRow(QString("%1 128").arg(address.toString()).toLatin1()) << address << 128 << 128;
        QTest::newRow(QString("%1 255").arg(address.toString()).toLatin1()) << address << 255 << 255;
        QTest::newRow(QString("%1 1024").arg(address.toString()).toLatin1()) << address << 1024 << 1;
    }
}

void tst_QUdpSocket::multicastTtlOption()
{
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH(QHostAddress, bindAddress);
    QFETCH(int, ttl);
    QFETCH(int, expected);
    if (setProxy) {
        // UDP multicast does not work with proxies
        expected = 0;
    }

    QUdpSocket udpSocket;
#ifdef FORCE_SESSION
    udpSocket.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    // bind, but ignore the result, we are only interested in initializing the socket
    (void) udpSocket.bind(bindAddress, 0);
    udpSocket.setSocketOption(QUdpSocket::MulticastTtlOption, ttl);
    QCOMPARE(udpSocket.socketOption(QUdpSocket::MulticastTtlOption).toInt(), expected);
}

void tst_QUdpSocket::multicastLoopbackOption_data()
{
    QTest::addColumn<QHostAddress>("bindAddress");
    QTest::addColumn<int>("loopback");
    QTest::addColumn<int>("expected");

    QList<QHostAddress> addresses;
    addresses += QHostAddress(QHostAddress::Any);
    addresses += QHostAddress(QHostAddress::AnyIPv6);

    foreach (const QHostAddress &address, addresses) {
        QTest::newRow(QString("%1 0").arg(address.toString()).toLatin1()) << address << 0 << 0;
        QTest::newRow(QString("%1 1").arg(address.toString()).toLatin1()) << address << 1 << 1;
        QTest::newRow(QString("%1 2").arg(address.toString()).toLatin1()) << address << 2 << 1;
        QTest::newRow(QString("%1 0 again").arg(address.toString()).toLatin1()) << address << 0 << 0;
        QTest::newRow(QString("%1 2 again").arg(address.toString()).toLatin1()) << address << 2 << 1;
        QTest::newRow(QString("%1 0 last time").arg(address.toString()).toLatin1()) << address << 0 << 0;
        QTest::newRow(QString("%1 1 again").arg(address.toString()).toLatin1()) << address << 1 << 1;
    }
}

void tst_QUdpSocket::multicastLoopbackOption()
{
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH(QHostAddress, bindAddress);
    QFETCH(int, loopback);
    QFETCH(int, expected);
    if (setProxy) {
        // UDP multicast does not work with proxies
        expected = 0;
    }

    QUdpSocket udpSocket;
#ifdef FORCE_SESSION
    udpSocket.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    // bind, but ignore the result, we are only interested in initializing the socket
    (void) udpSocket.bind(bindAddress, 0);
    udpSocket.setSocketOption(QUdpSocket::MulticastLoopbackOption, loopback);
    QCOMPARE(udpSocket.socketOption(QUdpSocket::MulticastLoopbackOption).toInt(), expected);
}

void tst_QUdpSocket::multicastJoinBeforeBind_data()
{
    QTest::addColumn<QHostAddress>("groupAddress");
    QTest::newRow("valid ipv4 group address") << QHostAddress("239.255.118.62");
    QTest::newRow("invalid ipv4 group address") << QHostAddress(QHostAddress::Broadcast);
    QTest::newRow("valid ipv6 group address") << QHostAddress("FF01::114");
    QTest::newRow("invalid ipv6 group address") << QHostAddress(QHostAddress::AnyIPv6);
}

void tst_QUdpSocket::multicastJoinBeforeBind()
{
    QFETCH(QHostAddress, groupAddress);

    QUdpSocket udpSocket;
#ifdef FORCE_SESSION
    udpSocket.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    // cannot join group before binding
    QTest::ignoreMessage(QtWarningMsg, "QUdpSocket::joinMulticastGroup() called on a QUdpSocket when not in QUdpSocket::BoundState");
    QVERIFY(!udpSocket.joinMulticastGroup(groupAddress));
}

void tst_QUdpSocket::multicastLeaveAfterClose_data()
{
    QTest::addColumn<QHostAddress>("groupAddress");
    QTest::newRow("valid ipv4 group address") << QHostAddress("239.255.118.62");
    QTest::newRow("valid ipv6 group address") << QHostAddress("FF01::114");
}

void tst_QUdpSocket::multicastLeaveAfterClose()
{
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH(QHostAddress, groupAddress);
    if (setProxy)
        QSKIP("UDP Multicast does not work with proxies");
    if (!QtNetworkSettings::hasIPv6() && groupAddress.protocol() == QAbstractSocket::IPv6Protocol)
        QSKIP("system doesn't support ipv6!");

    QUdpSocket udpSocket;
#ifdef FORCE_SESSION
    udpSocket.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    QVERIFY2(udpSocket.bind(groupAddress, 0),
             qPrintable(udpSocket.errorString()));
    QVERIFY2(udpSocket.joinMulticastGroup(groupAddress),
             qPrintable(udpSocket.errorString()));
    udpSocket.close();
    QTest::ignoreMessage(QtWarningMsg, "QUdpSocket::leaveMulticastGroup() called on a QUdpSocket when not in QUdpSocket::BoundState");
    QVERIFY(!udpSocket.leaveMulticastGroup(groupAddress));
}

void tst_QUdpSocket::setMulticastInterface_data()
{
    QTest::addColumn<QNetworkInterface>("iface");
    QTest::addColumn<QHostAddress>("address");
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    foreach (const QNetworkInterface &iface, interfaces) {
        foreach (const QNetworkAddressEntry &entry, iface.addressEntries()) {
            QTest::newRow(QString("%1:%2").arg(iface.name()).arg(entry.ip().toString()).toLatin1())
                    << iface
                    << entry.ip();
        }
    }
}

void tst_QUdpSocket::setMulticastInterface()
{
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH(QNetworkInterface, iface);
    QFETCH(QHostAddress, address);

    QUdpSocket udpSocket;
    // bind initializes the socket
    bool bound = udpSocket.bind((address.protocol() == QAbstractSocket::IPv6Protocol
                                 ? QHostAddress(QHostAddress::AnyIPv6)
                                 : QHostAddress(QHostAddress::Any)),
                                0);
    if (!bound)
        QTest::ignoreMessage(QtWarningMsg, "QUdpSocket::setMulticastInterface() called on a QUdpSocket when not in QUdpSocket::BoundState");
    udpSocket.setMulticastInterface(iface);
    if (!bound)
        QTest::ignoreMessage(QtWarningMsg, "QUdpSocket::multicastInterface() called on a QUdpSocket when not in QUdpSocket::BoundState");
    QNetworkInterface iface2 = udpSocket.multicastInterface();
    if (!setProxy) {
        QVERIFY(iface2.isValid());
        QCOMPARE(iface.name(), iface2.name());
    } else {
        QVERIFY(!iface2.isValid());
    }
}

void tst_QUdpSocket::multicast_data()
{
    QHostAddress anyAddress = QHostAddress(QHostAddress::AnyIPv4);
    QHostAddress groupAddress = QHostAddress("239.255.118.62");
    QHostAddress any6Address = QHostAddress(QHostAddress::AnyIPv6);
    QHostAddress group6Address = QHostAddress("FF01::114");
    QHostAddress dualAddress = QHostAddress(QHostAddress::Any);

    QTest::addColumn<QHostAddress>("bindAddress");
    QTest::addColumn<bool>("bindResult");
    QTest::addColumn<QHostAddress>("groupAddress");
    QTest::addColumn<bool>("joinResult");
    QTest::newRow("valid bind, group ipv4 address") << anyAddress << true << groupAddress << true;
    QTest::newRow("valid bind, invalid group ipv4 address") << anyAddress << true << anyAddress << false;
    QTest::newRow("same bind, group ipv4 address") << groupAddress << true << groupAddress << true;
    QTest::newRow("valid bind, group ipv6 address") << any6Address << true << group6Address << true;
    QTest::newRow("valid bind, invalid group ipv6 address") << any6Address << true << any6Address << false;
    QTest::newRow("same bind, group ipv6 address") << group6Address << true << group6Address << true;
    QTest::newRow("dual bind, group ipv4 address") << dualAddress << true << groupAddress << false;
    QTest::newRow("dual bind, group ipv6 address") << dualAddress << true << group6Address << true;
}

void tst_QUdpSocket::multicast()
{
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH(QHostAddress, bindAddress);
    QFETCH(bool, bindResult);
    QFETCH(QHostAddress, groupAddress);
    QFETCH(bool, joinResult);
    if (groupAddress.protocol() == QAbstractSocket::IPv6Protocol)
        QSKIP("system doesn't support ipv6!");
    if (setProxy) {
        // UDP multicast does not work with proxies
        if (
#ifndef Q_OS_WIN
            //windows native socket engine binds 0.0.0.0 instead of the requested multicast address
            (bindAddress.protocol() == QAbstractSocket::IPv4Protocol && (bindAddress.toIPv4Address() & 0xffff0000) == 0xefff0000) ||
#endif
            bindAddress.protocol() == QAbstractSocket::IPv6Protocol) {
            // proxy cannot bind to IPv6 or multicast addresses
            bindResult = false;
        }
        joinResult = false;
    }

    QUdpSocket receiver;
#ifdef FORCE_SESSION
    receiver.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif

    // bind first, then verify that we can join the multicast group
    QVERIFY2(receiver.bind(bindAddress, 0) == bindResult,
             qPrintable(receiver.errorString()));
    if (!bindResult)
        return;

    QVERIFY2(receiver.joinMulticastGroup(groupAddress) == joinResult,
             qPrintable(receiver.errorString()));
    if (!joinResult)
        return;

    QList<QByteArray> datagrams = QList<QByteArray>()
                                  << QByteArray("0123")
                                  << QByteArray("4567")
                                  << QByteArray("89ab")
                                  << QByteArray("cdef");

    QUdpSocket sender;
#ifdef FORCE_SESSION
    sender.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    sender.bind();
    foreach (const QByteArray &datagram, datagrams) {
        QCOMPARE(int(sender.writeDatagram(datagram, groupAddress, receiver.localPort())),
                 int(datagram.size()));
    }

    QVERIFY2(receiver.waitForReadyRead(),
             qPrintable(receiver.errorString()));
    QVERIFY(receiver.hasPendingDatagrams());
    QList<QByteArray> receivedDatagrams;
    while (receiver.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(receiver.pendingDatagramSize());
        receiver.readDatagram(datagram.data(), datagram.size(), 0, 0);
        receivedDatagrams << datagram;
    }
    QCOMPARE(receivedDatagrams, datagrams);

    QVERIFY2(receiver.leaveMulticastGroup(groupAddress), qPrintable(receiver.errorString()));
}

void tst_QUdpSocket::echo_data()
{
    QTest::addColumn<bool>("connect");
    QTest::newRow("writeDatagram") << false;
    QTest::newRow("write") << true;
}

void tst_QUdpSocket::echo()
{
    QFETCH(bool, connect);
    QHostInfo info = QHostInfo::fromName(QtNetworkSettings::serverName());
    QVERIFY(info.addresses().count());
    QHostAddress remote = info.addresses().first();

    QUdpSocket sock;
#ifdef FORCE_SESSION
    sock.setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
    if (connect) {
        sock.connectToHost(remote, 7);
        QVERIFY(sock.waitForConnected(10000));
    } else {
        sock.bind();
    }
    QByteArray out(30, 'x');
    QByteArray in;
    int successes = 0;
    for (int i=0;i<10;i++) {
        if (connect) {
            sock.write(out);
        } else {
            sock.writeDatagram(out, remote, 7);
        }
        if (sock.waitForReadyRead(1000)) {
            while (sock.hasPendingDatagrams()) {
                QHostAddress from;
                quint16 port;
                if (connect) {
                    in = sock.read(sock.pendingDatagramSize());
                } else {
                    in.resize(sock.pendingDatagramSize());
                    sock.readDatagram(in.data(), in.length(), &from, &port);
                }
                if (in==out)
                    successes++;
            }
        }
        if (!sock.isValid())
            QFAIL(sock.errorString().toLatin1().constData());
        qDebug() << "packets in" << successes << "out" << i;
        QTest::qWait(50); //choke to avoid triggering flood/DDoS protections on echo service
    }
    QVERIFY(successes >= 9);
}

void tst_QUdpSocket::linkLocalIPv6()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QList <QHostAddress> addresses;
    QSet <QString> scopes;
    QHostAddress localMask("fe80::");
    foreach (const QNetworkInterface& iface, QNetworkInterface::allInterfaces()) {
        //Windows preallocates link local addresses to interfaces that are down.
        //These may or may not work depending on network driver
        if (iface.flags() & QNetworkInterface::IsUp) {
            // Do not add the Teredo Tunneling Pseudo Interface on Windows.
            if (iface.humanReadableName().contains("Teredo"))
                continue;
            foreach (QNetworkAddressEntry addressEntry, iface.addressEntries()) {
                QHostAddress addr(addressEntry.ip());
                if (!addr.scopeId().isEmpty() && addr.isInSubnet(localMask, 64)) {
                    scopes << addr.scopeId();
                    addresses << addr;
                    qDebug() << addr;
                }
            }
        }
    }
    if (addresses.isEmpty())
        QSKIP("No IPv6 link local addresses");

    QList <QUdpSocket*> sockets;
    quint16 port = 0;
    foreach (const QHostAddress& addr, addresses) {
        QUdpSocket *s = new QUdpSocket;
        QVERIFY2(s->bind(addr, port), qPrintable(s->errorString()));
        port = s->localPort(); //bind same port, different networks
        sockets << s;
    }

    QUdpSocket neutral;
    QVERIFY(neutral.bind(QHostAddress(QHostAddress::AnyIPv6)));
    QSignalSpy neutralReadSpy(&neutral, SIGNAL(readyRead()));

    QByteArray testData("hello");
    QByteArray receiveBuffer("xxxxx");
    foreach (QUdpSocket *s, sockets) {
        QSignalSpy spy(s, SIGNAL(readyRead()));

        neutralReadSpy.clear();
        QVERIFY(s->writeDatagram(testData, s->localAddress(), neutral.localPort()));
        QTRY_VERIFY(neutralReadSpy.count() > 0); //note may need to accept a firewall prompt
        QHostAddress from;
        quint16 fromPort;
        QCOMPARE((int)neutral.readDatagram(receiveBuffer.data(), receiveBuffer.length(), &from, &fromPort), testData.length());
        QCOMPARE(from, s->localAddress());
        QCOMPARE(fromPort, s->localPort());
        QCOMPARE(receiveBuffer, testData);

        QVERIFY(neutral.writeDatagram(testData, s->localAddress(), s->localPort()));
        QTRY_VERIFY(spy.count() > 0); //note may need to accept a firewall prompt
        QCOMPARE((int)s->readDatagram(receiveBuffer.data(), receiveBuffer.length(), &from, &fromPort), testData.length());
        QCOMPARE(receiveBuffer, testData);

        //sockets bound to other interfaces shouldn't have received anything
        foreach (QUdpSocket *s2, sockets) {
            QCOMPARE((int)s2->bytesAvailable(), 0);
        }

        //Sending to the same address with different scope should normally fail
        //However it will pass if there is a route between two interfaces,
        //e.g. connected to a home/office network via wired and wireless interfaces
        //which is a reasonably common case.
        //So this is not auto tested.
    }
    qDeleteAll(sockets);
}

void tst_QUdpSocket::linkLocalIPv4()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QList <QHostAddress> addresses;
    QHostAddress localMask("169.254.0.0");
    foreach (const QNetworkInterface& iface, QNetworkInterface::allInterfaces()) {
        //Windows preallocates link local addresses to interfaces that are down.
        //These may or may not work depending on network driver (they do not work for the Bluetooth PAN driver)
        if (iface.flags() & QNetworkInterface::IsUp) {
            // Do not add the Teredo Tunneling Pseudo Interface on Windows.
            if (iface.humanReadableName().contains("Teredo"))
                continue;
            foreach (QNetworkAddressEntry addr, iface.addressEntries()) {
                if (addr.ip().isInSubnet(localMask, 16)) {
                    addresses << addr.ip();
                    qDebug() << addr.ip();
                }
            }
        }
    }
    if (addresses.isEmpty())
        QSKIP("No IPv4 link local addresses");

    QList <QUdpSocket*> sockets;
    quint16 port = 0;
    foreach (const QHostAddress& addr, addresses) {
        QUdpSocket *s = new QUdpSocket;
        QVERIFY2(s->bind(addr, port), qPrintable(s->errorString()));
        port = s->localPort(); //bind same port, different networks
        sockets << s;
    }

    QUdpSocket neutral;
    QVERIFY(neutral.bind(QHostAddress(QHostAddress::AnyIPv4)));

    QByteArray testData("hello");
    QByteArray receiveBuffer("xxxxx");
    foreach (QUdpSocket *s, sockets) {
        QVERIFY(s->writeDatagram(testData, s->localAddress(), neutral.localPort()));
        QVERIFY(neutral.waitForReadyRead(10000));
        QHostAddress from;
        quint16 fromPort;
        QCOMPARE((int)neutral.readDatagram(receiveBuffer.data(), receiveBuffer.length(), &from, &fromPort), testData.length());
        QCOMPARE(from, s->localAddress());
        QCOMPARE(fromPort, s->localPort());
        QCOMPARE(receiveBuffer, testData);

        QVERIFY(neutral.writeDatagram(testData, s->localAddress(), s->localPort()));
        QVERIFY(s->waitForReadyRead(10000));
        QCOMPARE((int)s->readDatagram(receiveBuffer.data(), receiveBuffer.length(), &from, &fromPort), testData.length());
        QCOMPARE(receiveBuffer, testData);

        //sockets bound to other interfaces shouldn't have received anything
        foreach (QUdpSocket *s2, sockets) {
            QCOMPARE((int)s2->bytesAvailable(), 0);
        }
    }
    qDeleteAll(sockets);
}

QTEST_MAIN(tst_QUdpSocket)
#include "tst_qudpsocket.moc"
