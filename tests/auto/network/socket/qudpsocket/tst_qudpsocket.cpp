/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2017 Intel Corporation.
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

#include <qcoreapplication.h>
#include <qfileinfo.h>
#include <qdatastream.h>
#include <qdebug.h>
#include <qrandom.h>
#include <qudpsocket.h>
#include <qhostaddress.h>
#include <qhostinfo.h>
#include <qtcpsocket.h>
#include <qmap.h>
#include <qnetworkdatagram.h>
#include <QNetworkProxy>
#include <QNetworkInterface>

#include <qstringlist.h>
#include "../../../network-settings.h"
#include "emulationdetector.h"

#if defined(Q_OS_LINUX)
#define SHOULD_CHECK_SYSCALL_SUPPORT
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#endif

#ifdef Q_OS_UNIX
#  include <sys/socket.h>
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN) || defined(SO_NREAD)
#  define RELIABLE_BYTES_AVAILABLE
#endif

Q_DECLARE_METATYPE(QHostAddress)

QT_FORWARD_DECLARE_CLASS(QUdpSocket)

class tst_QUdpSocket : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase_data();
    void initTestCase();
    void init();
    void cleanup();
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
    void connectToHost();
    void bindAndConnectToHost();
    void pendingDatagramSize();
    void writeDatagram();
    void performance();
    void bindMode();
    void writeDatagramToNonExistingPeer_data();
    void writeDatagramToNonExistingPeer();
    void writeToNonExistingPeer_data();
    void writeToNonExistingPeer();
    void outOfProcessConnectedClientServerTest();
    void outOfProcessUnconnectedClientServerTest();
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
    void readyRead();
    void readyReadForEmptyDatagram();
    void asyncReadDatagram();
    void writeInHostLookupState();

protected slots:
    void empty_readyReadSlot();
    void empty_connectedSlot();
    void async_readDatagramSlot();

private:
    bool shouldSkipIpv6TestsForBrokenSetsockopt();
    bool shouldWorkaroundLinuxKernelBug();
#ifdef SHOULD_CHECK_SYSCALL_SUPPORT
    bool ipv6SetsockoptionMissing(int level, int optname);
#endif
    QNetworkInterface interfaceForGroup(const QHostAddress &multicastGroup);

    bool m_skipUnsupportedIPv6Tests;
    bool m_workaroundLinuxKernelBug;
    QList<QHostAddress> allAddresses;
    QHostAddress multicastGroup4, multicastGroup6;
    QVector<QHostAddress> linklocalMulticastGroups;
    QUdpSocket *m_asyncSender;
    QUdpSocket *m_asyncReceiver;
};

#ifdef SHOULD_CHECK_SYSCALL_SUPPORT
bool tst_QUdpSocket::ipv6SetsockoptionMissing(int level, int optname)
{
    int testSocket;

    testSocket = socket(PF_INET6, SOCK_DGRAM, 0);

    // If we can't test here, assume it's not missing
    if (testSocket == -1)
        return false;

    bool result = false;
    if (setsockopt(testSocket, level, optname, nullptr, 0) == -1)
        if (errno == ENOPROTOOPT)
            result = true;

    close(testSocket);
    return result;
}
#endif //SHOULD_CHECK_SYSCALL_SUPPORT

bool tst_QUdpSocket::shouldSkipIpv6TestsForBrokenSetsockopt()
{
#ifdef SHOULD_CHECK_SYSCALL_SUPPORT
    // Following parameters for setsockopt are not supported by all QEMU versions:
    if (ipv6SetsockoptionMissing(SOL_IPV6, IPV6_JOIN_GROUP)
        || ipv6SetsockoptionMissing(SOL_IPV6, IPV6_MULTICAST_HOPS)
        || ipv6SetsockoptionMissing(SOL_IPV6, IPV6_MULTICAST_IF)
        || ipv6SetsockoptionMissing(SOL_IPV6, IPV6_MULTICAST_LOOP)
        || ipv6SetsockoptionMissing(SOL_IPV6, IPV6_RECVHOPLIMIT)) {
        return true;
    }
#endif //SHOULD_CHECK_SYSCALL_SUPPORT

    return false;
}

QNetworkInterface tst_QUdpSocket::interfaceForGroup(const QHostAddress &multicastGroup)
{
    if (multicastGroup.protocol() == QAbstractSocket::IPv4Protocol)
        return QNetworkInterface();

    QString scope = multicastGroup.scopeId();
    if (!scope.isEmpty())
        return QNetworkInterface::interfaceFromName(scope);

    static QNetworkInterface ipv6if = [=]() {
        // find any link local address in the allAddress list
        for (const QHostAddress &addr: qAsConst(allAddresses)) {
            if (addr.isLoopback())
                continue;

            QString scope = addr.scopeId();
            if (!scope.isEmpty()) {
                QNetworkInterface iface = QNetworkInterface::interfaceFromName(scope);
                qDebug() << "Will bind IPv6 sockets to" << iface;
                return iface;
            }
        }

        qWarning("interfaceForGroup(%s) could not find any link-local IPv6 address! "
                 "Make sure this test is behind a check of QtNetworkSettings::hasIPv6().",
                 qUtf8Printable(multicastGroup.toString()));
        return QNetworkInterface();
    }();
    return ipv6if;
}

bool tst_QUdpSocket::shouldWorkaroundLinuxKernelBug()
{
#ifdef Q_OS_LINUX
    const QVersionNumber version = QVersionNumber::fromString(QSysInfo::kernelVersion());
    return version.majorVersion() == 4 && version.minorVersion() >= 6 && version.minorVersion() < 13;
#else
    return false;
#endif
}

static QHostAddress makeNonAny(const QHostAddress &address, QHostAddress::SpecialAddress preferForAny = QHostAddress::LocalHost)
{
    if (address == QHostAddress::Any)
        return preferForAny;
    if (address == QHostAddress::AnyIPv4)
        return QHostAddress::LocalHost;
    if (address == QHostAddress::AnyIPv6)
        return QHostAddress::LocalHostIPv6;
    return address;
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
#if QT_CONFIG(socks5)
    if (!newTestServer)
        QTest::newRow("WithSocks5Proxy") << true << int(QNetworkProxy::Socks5Proxy);
#endif
}

void tst_QUdpSocket::initTestCase()
{
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
    allAddresses = QNetworkInterface::allAddresses();
    m_skipUnsupportedIPv6Tests = shouldSkipIpv6TestsForBrokenSetsockopt();

    // Create a pair of random multicast groups so we avoid clashing with any
    // other tst_qudpsocket running on the same network at the same time.
    quint64 r[2] = {
        // ff14:: is temporary, not prefix-based, admin-local
        qToBigEndian(Q_UINT64_C(0xff14) << 48),
        QRandomGenerator64::global()->generate64()
    };
    multicastGroup6.setAddress(*reinterpret_cast<Q_IPV6ADDR *>(&r));

    // 239.0.0.0/8 is "Organization-Local Scope"
    multicastGroup4.setAddress((239U << 24) | (r[1] & 0xffffff));

    // figure out some link-local IPv6 multicast groups
    // ff12:: is temporary, not prefix-based, link-local
    r[0] = qToBigEndian(Q_UINT64_C(0xff12) << 48);
    QHostAddress llbase(*reinterpret_cast<Q_IPV6ADDR *>(&r));
    for (const QHostAddress &a : qAsConst(allAddresses)) {
        QString scope = a.scopeId();
        if (scope.isEmpty())
            continue;
        llbase.setScopeId(scope);
        linklocalMulticastGroups << llbase;
    }

    qDebug() << "Will use multicast groups" << multicastGroup4 << multicastGroup6 << linklocalMulticastGroups;

    m_workaroundLinuxKernelBug = shouldWorkaroundLinuxKernelBug();
    if (EmulationDetector::isRunningArmOnX86())
        QSKIP("This test is unreliable due to QEMU emulation shortcomings.");
}

void tst_QUdpSocket::init()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#if QT_CONFIG(socks5)
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080));
        }
#else
        QSKIP("No proxy support");
#endif // QT_CONFIG(socks5)
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

    QVERIFY(socket.isSequential());
    QVERIFY(!socket.isOpen());
    QCOMPARE(socket.socketType(), QUdpSocket::UdpSocket);
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

    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");

    QSignalSpy stateChangedSpy(&serverSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)));
    QVERIFY2(serverSocket.bind(), serverSocket.errorString().toLatin1().constData());
    QCOMPARE(stateChangedSpy.count(), 1);

    const char *message[] = {"Yo mista", "Yo", "Wassap"};

    QHostAddress serverAddress = makeNonAny(serverSocket.localAddress());
    for (int i = 0; i < 3; ++i) {
        QUdpSocket clientSocket;
        QCOMPARE(int(clientSocket.writeDatagram(message[i], strlen(message[i]),
                                               serverAddress, serverSocket.localPort())),
                int(strlen(message[i])));
        char buf[1024];
        QHostAddress host;
        quint16 port;
        QVERIFY2(serverSocket.waitForReadyRead(5000), QtNetworkSettings::msgSocketError(serverSocket).constData());
        QCOMPARE(int(serverSocket.readDatagram(buf, sizeof(buf), &host, &port)),
                int(strlen(message[i])));
        buf[strlen(message[i])] = '\0';
        QCOMPARE(QByteArray(buf), QByteArray(message[i]));
        QCOMPARE(port, clientSocket.localPort());
        if (host.toIPv4Address()) // in case the sender is IPv4 mapped in IPv6
            QCOMPARE(host.toIPv4Address(), makeNonAny(clientSocket.localAddress()).toIPv4Address());
        else
            QCOMPARE(host, makeNonAny(clientSocket.localAddress()));
    }
}

//----------------------------------------------------------------------------------

void tst_QUdpSocket::broadcasting()
{
    if (m_workaroundLinuxKernelBug)
        QSKIP("This test can fail due to linux kernel bug");

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
            for (int i=0;i<iface.addressEntries().count();i++) {
                QHostAddress broadcast = iface.addressEntries().at(i).broadcast();
                if (broadcast.protocol() == QAbstractSocket::IPv4Protocol)
                    broadcastAddresses.append(broadcast);
            }
        }
    }
    if (broadcastAddresses.isEmpty())
        QSKIP("No interface can broadcast");
    for (int i = 0; i < 4; ++i) {
        QUdpSocket serverSocket;
        QVERIFY2(serverSocket.bind(QHostAddress(QHostAddress::AnyIPv4), 0), serverSocket.errorString().toLatin1().constData());
        quint16 serverPort = serverSocket.localPort();

        QCOMPARE(serverSocket.state(), QUdpSocket::BoundState);

        connect(&serverSocket, SIGNAL(readyRead()), SLOT(empty_readyReadSlot()));

        QUdpSocket broadcastSocket;
        broadcastSocket.bind(QHostAddress(QHostAddress::AnyIPv4), 0);

        for (int j = 0; j < 10; ++j) {
            for (int k = 0; k < 4; k++) {
                broadcastSocket.writeDatagram(message[i], strlen(message[i]),
                    QHostAddress::Broadcast, serverPort);
                foreach (QHostAddress addr, broadcastAddresses)
                    broadcastSocket.writeDatagram(message[i], strlen(message[i]), addr, serverPort);
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
                const int messageLength = int(strlen(message[i]));
                QNetworkDatagram dgram = serverSocket.receiveDatagram();
                QVERIFY(dgram.isValid());
                QByteArray arr = dgram.data();

                QCOMPARE(arr.length(), messageLength);
                arr.resize(messageLength);
                QCOMPARE(arr, QByteArray(message[i]));

                if (dgram.senderAddress().toIPv4Address()) // in case it's a v6-mapped address
                    QVERIFY2(allAddresses.contains(QHostAddress(dgram.senderAddress().toIPv4Address())),
                             dgram.senderAddress().toString().toLatin1());
                else if (!dgram.senderAddress().isNull())
                    QVERIFY2(allAddresses.contains(dgram.senderAddress()),
                             dgram.senderAddress().toString().toLatin1());
                QCOMPARE(dgram.senderPort(), int(broadcastSocket.localPort()));
                if (!dgram.destinationAddress().isNull()) {
                    QVERIFY2(dgram.destinationAddress() == QHostAddress::Broadcast
                            || broadcastAddresses.contains(dgram.destinationAddress()),
                             dgram.destinationAddress().toString().toLatin1());
                    QCOMPARE(dgram.destinationPort(), int(serverSocket.localPort()));
                }

                int ttl = dgram.hopLimit();
                if (ttl != -1)
                    QVERIFY(ttl != 0);
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

    // make sure we bind to IPv4
    QHostAddress localhost = QHostAddress::LocalHost;
    QVERIFY2(peter.bind(localhost), peter.errorString().toLatin1().constData());
    QVERIFY2(paul.bind(localhost), paul.errorString().toLatin1().constData());

    QHostAddress peterAddress = makeNonAny(peter.localAddress());
    QHostAddress paulAddress = makeNonAny(paul.localAddress());

    QCOMPARE(peter.writeDatagram(peterMessage.data(), peterMessage.length(),
                                paulAddress, paul.localPort()), qint64(peterMessage.length()));
    QCOMPARE(paul.writeDatagram(paulMessage.data(), paulMessage.length(),
                               peterAddress, peter.localPort()), qint64(paulMessage.length()));

    QVERIFY2(peter.waitForReadyRead(9000), QtNetworkSettings::msgSocketError(peter).constData());
    QVERIFY2(paul.waitForReadyRead(9000), QtNetworkSettings::msgSocketError(paul).constData());

    QNetworkDatagram peterDatagram = peter.receiveDatagram(paulMessage.length() * 2);
    QNetworkDatagram paulDatagram = paul.receiveDatagram(peterMessage.length() * 2);
    if (success) {
        QCOMPARE(peterDatagram.data().length(), qint64(paulMessage.length()));
        QCOMPARE(paulDatagram.data().length(), qint64(peterMessage.length()));
    } else {
        // this code path seems to never be executed
        QVERIFY(peterDatagram.data().length() != paulMessage.length());
        QVERIFY(paulDatagram.data().length() != peterMessage.length());
    }

    QCOMPARE(peterDatagram.data().left(paulMessage.length()), paulMessage);
    QCOMPARE(paulDatagram.data().left(peterMessage.length()), peterMessage);

    QCOMPARE(peterDatagram.senderAddress(), paulAddress);
    QCOMPARE(paulDatagram.senderAddress(), peterAddress);
    QCOMPARE(paulDatagram.senderPort(), int(peter.localPort()));
    QCOMPARE(peterDatagram.senderPort(), int(paul.localPort()));

    // Unlike for IPv6 with IPV6_PKTINFO, IPv4 has no standardized way of
    // obtaining the packet's destination addresses. The destinationAddress and
    // destinationPort calls could fail, so whitelist the OSes for which we
    // know we have an implementation.
#if defined(Q_OS_LINUX) || defined(Q_OS_BSD4) || defined(Q_OS_WIN)
    QVERIFY(peterDatagram.destinationPort() != -1);
    QVERIFY(paulDatagram.destinationPort() != -1);
#endif
    if (peterDatagram.destinationPort() == -1) {
        QCOMPARE(peterDatagram.destinationAddress().protocol(), QAbstractSocket::UnknownNetworkLayerProtocol);
        QCOMPARE(paulDatagram.destinationAddress().protocol(), QAbstractSocket::UnknownNetworkLayerProtocol);
    } else {
        QCOMPARE(peterDatagram.destinationAddress(), makeNonAny(peter.localAddress()));
        QCOMPARE(paulDatagram.destinationAddress(), makeNonAny(paul.localAddress()));
        QVERIFY(peterDatagram.destinationAddress().isEqual(makeNonAny(peter.localAddress())));
        QVERIFY(paulDatagram.destinationAddress().isEqual(makeNonAny(paul.localAddress())));
    }
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

    int peterPort;
    int paulPort;

    if (!peter.bind(QHostAddress(QHostAddress::LocalHostIPv6), 0)) {
        QCOMPARE(peter.error(), QUdpSocket::UnsupportedSocketOperationError);
        return;
    }

    QVERIFY(paul.bind(QHostAddress(QHostAddress::LocalHostIPv6), 0));

    QHostAddress peterAddress = makeNonAny(peter.localAddress());
    QHostAddress paulAddress = makeNonAny(paul.localAddress());
    peterPort = peter.localPort();
    paulPort = paul.localPort();

    QCOMPARE(peter.writeDatagram(peterMessage.data(), peterMessage.length(), QHostAddress("::1"),
                                    paulPort), qint64(peterMessage.length()));
    QCOMPARE(paul.writeDatagram(paulMessage.data(), paulMessage.length(),
                                   QHostAddress("::1"), peterPort), qint64(paulMessage.length()));

    QVERIFY(peter.waitForReadyRead(5000));
    QVERIFY(paul.waitForReadyRead(5000));
    QNetworkDatagram peterDatagram = peter.receiveDatagram(paulMessage.length() * 2);
    QNetworkDatagram paulDatagram = paul.receiveDatagram(peterMessage.length() * 2);

    if (success) {
        QCOMPARE(peterDatagram.data().length(), qint64(paulMessage.length()));
        QCOMPARE(paulDatagram.data().length(), qint64(peterMessage.length()));
    } else {
        // this code path seems to never be executed
        QVERIFY(peterDatagram.data().length() != paulMessage.length());
        QVERIFY(paulDatagram.data().length() != peterMessage.length());
    }

    QCOMPARE(peterDatagram.data().left(paulMessage.length()), paulMessage);
    QCOMPARE(paulDatagram.data().left(peterMessage.length()), peterMessage);

    QCOMPARE(peterDatagram.senderAddress(), paulAddress);
    QCOMPARE(paulDatagram.senderAddress(), peterAddress);
    QCOMPARE(paulDatagram.senderPort(), peterPort);
    QCOMPARE(peterDatagram.senderPort(), paulPort);

    // For IPv6, IPV6_PKTINFO is a mandatory feature (RFC 3542).
    QCOMPARE(peterDatagram.destinationAddress(), makeNonAny(peter.localAddress()));
    QCOMPARE(paulDatagram.destinationAddress(), makeNonAny(paul.localAddress()));
    QCOMPARE(peterDatagram.destinationPort(), peterPort);
    QCOMPARE(paulDatagram.destinationPort(), paulPort);
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

    //test v4 -> dual
    QCOMPARE((int)v4Sock.writeDatagram(v4Data.constData(), v4Data.length(), QHostAddress(QHostAddress::LocalHost), dualSock.localPort()), v4Data.length());
    QVERIFY2(dualSock.waitForReadyRead(5000), QtNetworkSettings::msgSocketError(dualSock).constData());
    QNetworkDatagram dgram = dualSock.receiveDatagram(100);
    QVERIFY(dgram.isValid());
    QCOMPARE(dgram.data(), v4Data);
    QCOMPARE(dgram.senderPort(), int(v4Sock.localPort()));
    // receiving v4 on dual stack will receive as IPv6, so use isEqual()
    QVERIFY(dgram.senderAddress().isEqual(makeNonAny(v4Sock.localAddress(), QHostAddress::Null)));
    if (dualSock.localAddress().protocol() == QAbstractSocket::IPv4Protocol)
        QCOMPARE(dgram.senderAddress(), makeNonAny(v4Sock.localAddress(), QHostAddress::Null));
    if (dgram.destinationPort() != -1) {
        QCOMPARE(dgram.destinationPort(), int(dualSock.localPort()));
        QVERIFY(dgram.destinationAddress().isEqual(dualSock.localAddress()));
    } else {
        qInfo("Getting IPv4 destination address failed.");
    }

    if (QtNetworkSettings::hasIPv6()) {
        QUdpSocket v6Sock;
        QByteArray v6Data("v6");
        QVERIFY(v6Sock.bind(QHostAddress(QHostAddress::AnyIPv6), 0));

        //test v6 -> dual
        QCOMPARE((int)v6Sock.writeDatagram(v6Data.constData(), v6Data.length(), QHostAddress(QHostAddress::LocalHostIPv6), dualSock.localPort()), v6Data.length());
        QVERIFY2(dualSock.waitForReadyRead(5000), QtNetworkSettings::msgSocketError(dualSock).constData());
        dgram = dualSock.receiveDatagram(100);
        QVERIFY(dgram.isValid());
        QCOMPARE(dgram.data(), v6Data);
        QCOMPARE(dgram.senderPort(), int(v6Sock.localPort()));
        QCOMPARE(dgram.senderAddress(), makeNonAny(v6Sock.localAddress(), QHostAddress::LocalHostIPv6));
        QCOMPARE(dgram.destinationPort(), int(dualSock.localPort()));
        QCOMPARE(dgram.destinationAddress(), makeNonAny(dualSock.localAddress(), QHostAddress::LocalHostIPv6));

        //test dual -> v6
        QCOMPARE((int)dualSock.writeDatagram(dualData.constData(), dualData.length(), QHostAddress(QHostAddress::LocalHostIPv6), v6Sock.localPort()), dualData.length());
        QVERIFY2(v6Sock.waitForReadyRead(5000), QtNetworkSettings::msgSocketError(v6Sock).constData());
        dgram = v6Sock.receiveDatagram(100);
        QVERIFY(dgram.isValid());
        QCOMPARE(dgram.data(), dualData);
        QCOMPARE(dgram.senderPort(), int(dualSock.localPort()));
        QCOMPARE(dgram.senderAddress(), makeNonAny(dualSock.localAddress(), QHostAddress::LocalHostIPv6));
        QCOMPARE(dgram.destinationPort(), int(v6Sock.localPort()));
        QCOMPARE(dgram.destinationAddress(), makeNonAny(v6Sock.localAddress(), QHostAddress::LocalHostIPv6));
    }

    //test dual -> v4
    QCOMPARE((int)dualSock.writeDatagram(dualData.constData(), dualData.length(), QHostAddress(QHostAddress::LocalHost), v4Sock.localPort()), dualData.length());
    QVERIFY2(v4Sock.waitForReadyRead(5000), QtNetworkSettings::msgSocketError(v4Sock).constData());
    dgram = v4Sock.receiveDatagram(100);
    QVERIFY(dgram.isValid());
    QCOMPARE(dgram.data(), dualData);
    QCOMPARE(dgram.senderPort(), int(dualSock.localPort()));
    QCOMPARE(dgram.senderAddress(), makeNonAny(dualSock.localAddress(), QHostAddress::LocalHost));
#if defined(Q_OS_LINUX) || defined(Q_OS_BSD4) || defined(Q_OS_WIN)
    QVERIFY(dgram.destinationPort() != -1);
#endif
    if (dgram.destinationPort() != -1) {
        QCOMPARE(dgram.destinationPort(), int(v4Sock.localPort()));
        QCOMPARE(dgram.destinationAddress(), makeNonAny(v4Sock.localAddress(), QHostAddress::LocalHost));
    }
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
        QVERIFY2(v4Sock.waitForReadyRead(5000), QtNetworkSettings::msgSocketError(v4Sock).constData());
        buffer.reserve(100);
        size = v4Sock.readDatagram(buffer.data(), 100, &from, &port);
        QCOMPARE((int)size, dualData.length());
        buffer.resize(size);
        QCOMPARE(buffer, dualData);

        QCOMPARE((int)dualSock.writeDatagram(dualData.constData(), dualData.length(), QHostAddress(QHostAddress::LocalHostIPv6), v6Sock.localPort()), dualData.length());
        QVERIFY2(v6Sock.waitForReadyRead(5000), QtNetworkSettings::msgSocketError(v6Sock).constData());
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
        QVERIFY2(v6Sock.waitForReadyRead(5000), QtNetworkSettings::msgSocketError(v6Sock).constData());
        buffer.reserve(100);
        size = v6Sock.readDatagram(buffer.data(), 100, &from, &port);
        QCOMPARE((int)size, dualData.length());
        buffer.resize(size);
        QCOMPARE(buffer, dualData);

        QCOMPARE((int)dualSock.writeDatagram(dualData.constData(), dualData.length(), QHostAddress(QHostAddress::LocalHost), v4Sock.localPort()), dualData.length());
        QVERIFY2(v4Sock.waitForReadyRead(5000), QtNetworkSettings::msgSocketError(v4Sock).constData());
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

void tst_QUdpSocket::connectToHost()
{
    QUdpSocket socket1;
    QUdpSocket socket2;

    QVERIFY2(socket1.bind(), socket1.errorString().toLatin1().constData());

    socket2.connectToHost(makeNonAny(socket1.localAddress()), socket1.localPort());
    QVERIFY(socket2.waitForConnected(5000));
}

//----------------------------------------------------------------------------------

void tst_QUdpSocket::bindAndConnectToHost()
{
    QUdpSocket socket1;
    QUdpSocket socket2;
    QUdpSocket dummysocket;

    // we use the dummy socket to use up a file descriptor
    dummysocket.bind();

    QVERIFY2(socket2.bind(), socket2.errorString().toLatin1());
    quint16 boundPort = socket2.localPort();
    qintptr fd = socket2.socketDescriptor();

    QVERIFY2(socket1.bind(), socket1.errorString().toLatin1().constData());

    dummysocket.close();
    socket2.connectToHost(makeNonAny(socket1.localAddress()), socket1.localPort());
    QVERIFY(socket2.waitForConnected(5000));

    QCOMPARE(socket2.localPort(), boundPort);
    QCOMPARE(socket2.socketDescriptor(), fd);
}

//----------------------------------------------------------------------------------

void tst_QUdpSocket::pendingDatagramSize()
{
    if (m_workaroundLinuxKernelBug)
        QSKIP("This test can fail due to linux kernel bug");

    QUdpSocket server;
    QVERIFY2(server.bind(), server.errorString().toLatin1().constData());

    QHostAddress serverAddress = makeNonAny(server.localAddress());
    QUdpSocket client;
    QVERIFY(client.writeDatagram("this is", 7, serverAddress, server.localPort()) == 7);
    QVERIFY(client.writeDatagram(0, 0, serverAddress, server.localPort()) == 0);
    QVERIFY(client.writeDatagram("3 messages", 10, serverAddress, server.localPort()) == 10);

    char c = 0;
    QVERIFY2(server.waitForReadyRead(), QtNetworkSettings::msgSocketError(server).constData());
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
    QVERIFY2(server.bind(), server.errorString().toLatin1().constData());

    QHostAddress serverAddress = makeNonAny(server.localAddress());
    QUdpSocket client;

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
        QCOMPARE(bytesspy.count(), 1);
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
    QVERIFY2(server.bind(), server.errorString().toLatin1().constData());

    QHostAddress serverAddress = makeNonAny(server.localAddress());
    QUdpSocket client;
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
    QVERIFY2(socket.bind(), socket.errorString().toLatin1().constData());
    QUdpSocket socket2;
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
    if (!socket2.bind(socket.localPort(), QUdpSocket::ReuseAddressHint)) {
        qWarning("Failed to bind with QUdpSocket::ReuseAddressHint(%s), user isn't an administrator?",
                 qPrintable(socket2.errorString()));
    }
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
    QList<QHostAddress> serverAddresses(QHostInfo::fromName(QtNetworkSettings::serverName()).addresses());
    if (serverAddresses.isEmpty())
        return;

    QHostAddress remote = serverAddresses.first();

    QTest::newRow("localhost-unbound") << false << localhost;
    QTest::newRow("localhost-bound") << true << localhost;
    QTest::newRow("remote-unbound") << false << remote;
    QTest::newRow("remote-bound") << true << remote;
}

void tst_QUdpSocket::writeDatagramToNonExistingPeer()
{
    if (QHostInfo::fromName(QtNetworkSettings::serverName()).addresses().isEmpty())
        QFAIL("Could not find test server address");
    QFETCH(bool, bind);
    QFETCH(QHostAddress, peerAddress);

    quint16 peerPort = 33533 + int(bind);

    QUdpSocket sUdp;
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
    QList<QHostAddress> serverAddresses(QHostInfo::fromName(QtNetworkSettings::serverName()).addresses());
    if (serverAddresses.isEmpty())
        return;

    QHostAddress remote = serverAddresses.first();
    // write (required to be connected)
    QTest::newRow("localhost") << localhost;
    QTest::newRow("remote") << remote;
}

void tst_QUdpSocket::writeToNonExistingPeer()
{
    QSKIP("Connected-mode UDP sockets and their behaviour are erratic");
    if (QHostInfo::fromName(QtNetworkSettings::serverName()).addresses().isEmpty())
        QFAIL("Could not find test server address");
    QFETCH(QHostAddress, peerAddress);
    quint16 peerPort = 34534;
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");

    QUdpSocket sConnected;
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

void tst_QUdpSocket::outOfProcessConnectedClientServerTest()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
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
#endif
}

void tst_QUdpSocket::outOfProcessUnconnectedClientServerTest()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
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
#endif
}

void tst_QUdpSocket::zeroLengthDatagram()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QUdpSocket receiver;
    QVERIFY(receiver.bind());

    QVERIFY(!receiver.waitForReadyRead(100));
    QVERIFY(!receiver.hasPendingDatagrams());

    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(QNetworkDatagram(QByteArray(), QHostAddress::LocalHost, receiver.localPort())), qint64(0));

    QVERIFY2(receiver.waitForReadyRead(1000), QtNetworkSettings::msgSocketError(receiver).constData());
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
    addresses += QHostAddress(QHostAddress::AnyIPv4);
    addresses += QHostAddress(QHostAddress::AnyIPv6);

    foreach (const QHostAddress &address, addresses) {
        const QByteArray addressB = address.toString().toLatin1();
        QTest::newRow((addressB + " 0").constData()) << address << 0 << 0;
        QTest::newRow((addressB + " 1").constData()) << address << 1 << 1;
        QTest::newRow((addressB + " 2").constData()) << address << 2 << 2;
        QTest::newRow((addressB + " 128").constData()) << address << 128 << 128;
        QTest::newRow((addressB + " 255").constData()) << address << 255 << 255;
        QTest::newRow((addressB + " 1024").constData()) << address << 1024 << 1;
    }
}

void tst_QUdpSocket::multicastTtlOption()
{
#ifdef Q_OS_WINRT
    QSKIP("WinRT does not support multicast.");
#endif
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH(QHostAddress, bindAddress);
    QFETCH(int, ttl);
    QFETCH(int, expected);
    if (setProxy) {
        // UDP multicast does not work with proxies
        expected = 0;
    }

    // Some syscalls needed for ipv6 udp multicasting are not functional
    if (m_skipUnsupportedIPv6Tests) {
        if (bindAddress.protocol() == QAbstractSocket::IPv6Protocol) {
            QSKIP("Syscalls needed for ipv6 udp multicasting missing functionality");
        }
    }

    QUdpSocket udpSocket;
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
    addresses += QHostAddress(QHostAddress::AnyIPv4);
    addresses += QHostAddress(QHostAddress::AnyIPv6);

    foreach (const QHostAddress &address, addresses) {
        const QByteArray addressB = address.toString().toLatin1();
        QTest::newRow((addressB + " 0").constData()) << address << 0 << 0;
        QTest::newRow((addressB + " 1").constData()) << address << 1 << 1;
        QTest::newRow((addressB + " 2").constData()) << address << 2 << 1;
        QTest::newRow((addressB + " 0 again").constData()) << address << 0 << 0;
        QTest::newRow((addressB + " 2 again").constData()) << address << 2 << 1;
        QTest::newRow((addressB + " 0 last time").constData()) << address << 0 << 0;
        QTest::newRow((addressB + " 1 again").constData()) << address << 1 << 1;
    }
}

void tst_QUdpSocket::multicastLoopbackOption()
{
#ifdef Q_OS_WINRT
    QSKIP("WinRT does not support multicast.");
#endif
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH(QHostAddress, bindAddress);
    QFETCH(int, loopback);
    QFETCH(int, expected);
    if (setProxy) {
        // UDP multicast does not work with proxies
        expected = 0;
    }

    // Some syscalls needed for ipv6 udp multicasting are not functional
    if (m_skipUnsupportedIPv6Tests) {
        if (bindAddress.protocol() == QAbstractSocket::IPv6Protocol) {
            QSKIP("Syscalls needed for ipv6 udp multicasting missing functionality");
        }
    }

    QUdpSocket udpSocket;
    // bind, but ignore the result, we are only interested in initializing the socket
    (void) udpSocket.bind(bindAddress, 0);
    udpSocket.setSocketOption(QUdpSocket::MulticastLoopbackOption, loopback);
    QCOMPARE(udpSocket.socketOption(QUdpSocket::MulticastLoopbackOption).toInt(), expected);
}

void tst_QUdpSocket::multicastJoinBeforeBind_data()
{
    QTest::addColumn<QHostAddress>("groupAddress");
    QTest::newRow("valid ipv4 group address") << multicastGroup4;
    QTest::newRow("invalid ipv4 group address") << QHostAddress(QHostAddress::Broadcast);
    QTest::newRow("valid ipv6 group address") << multicastGroup6;
    for (const QHostAddress &a : qAsConst(linklocalMulticastGroups))
        QTest::addRow("valid ipv6 %s-link group address", a.scopeId().toLatin1().constData()) << a;
    QTest::newRow("invalid ipv6 group address") << QHostAddress(QHostAddress::AnyIPv6);
}

void tst_QUdpSocket::multicastJoinBeforeBind()
{
#ifdef Q_OS_WINRT
    QSKIP("WinRT does not support multicast.");
#endif
    QFETCH(QHostAddress, groupAddress);

    QUdpSocket udpSocket;
    // cannot join group before binding
    QTest::ignoreMessage(QtWarningMsg, "QUdpSocket::joinMulticastGroup() called on a QUdpSocket when not in QUdpSocket::BoundState");
    QVERIFY(!udpSocket.joinMulticastGroup(groupAddress));
}

void tst_QUdpSocket::multicastLeaveAfterClose_data()
{
    QTest::addColumn<QHostAddress>("groupAddress");
    QTest::newRow("ipv4") << multicastGroup4;
    QTest::newRow("ipv6") << multicastGroup6;
    for (const QHostAddress &a : qAsConst(linklocalMulticastGroups))
        QTest::addRow("ipv6-link-%s", a.scopeId().toLatin1().constData()) << a;
}

void tst_QUdpSocket::multicastLeaveAfterClose()
{
#ifdef Q_OS_WINRT
    QSKIP("WinRT does not support multicast.");
#endif
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH(QHostAddress, groupAddress);
    if (setProxy)
        QSKIP("UDP Multicast does not work with proxies");
    if (!QtNetworkSettings::hasIPv6() && groupAddress.protocol() == QAbstractSocket::IPv6Protocol)
        QSKIP("system doesn't support ipv6!");

    // Some syscalls needed for ipv6 udp multicasting are not functional
    if (m_skipUnsupportedIPv6Tests) {
        if (groupAddress.protocol() == QAbstractSocket::IPv6Protocol) {
            QSKIP("Syscalls needed for ipv6 udp multicasting missing functionality");
        }
    }

    QUdpSocket udpSocket;
    QHostAddress bindAddress = QHostAddress::AnyIPv4;
    if (groupAddress.protocol() == QAbstractSocket::IPv6Protocol)
        bindAddress = QHostAddress::AnyIPv6;
    QVERIFY2(udpSocket.bind(bindAddress, 0),
             qPrintable(udpSocket.errorString()));
    QVERIFY2(udpSocket.joinMulticastGroup(groupAddress, interfaceForGroup(groupAddress)),
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
        if ((iface.flags() & QNetworkInterface::IsUp) == 0)
            continue;
        foreach (const QNetworkAddressEntry &entry, iface.addressEntries()) {
            const QByteArray testName = iface.name().toLatin1() + ':' + entry.ip().toString().toLatin1();
            QTest::newRow(testName.constData()) << iface << entry.ip();
        }
    }
}

void tst_QUdpSocket::setMulticastInterface()
{
#ifdef Q_OS_WINRT
    QSKIP("WinRT does not support multicast.");
#endif
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH(QNetworkInterface, iface);
    QFETCH(QHostAddress, address);

    // Some syscalls needed for udp multicasting are not functional
    if (m_skipUnsupportedIPv6Tests) {
        QSKIP("Syscalls needed for udp multicasting missing functionality");
    }

    QUdpSocket udpSocket;
    // bind initializes the socket
    bool bound = udpSocket.bind((address.protocol() == QAbstractSocket::IPv6Protocol
                                 ? QHostAddress(QHostAddress::AnyIPv6)
                                 : QHostAddress(QHostAddress::AnyIPv4)),
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
    QHostAddress groupAddress = multicastGroup4;
    QHostAddress any6Address = QHostAddress(QHostAddress::AnyIPv6);
    QHostAddress group6Address = multicastGroup6;
    QHostAddress dualAddress = QHostAddress(QHostAddress::Any);

    QTest::addColumn<QHostAddress>("bindAddress");
    QTest::addColumn<bool>("bindResult");
    QTest::addColumn<QHostAddress>("groupAddress");
    QTest::addColumn<bool>("joinResult");
    QTest::newRow("valid bind, group ipv4 address") << anyAddress << true << groupAddress << true;
    QTest::newRow("valid bind, invalid group ipv4 address") << anyAddress << true << anyAddress << false;
    QTest::newRow("valid bind, group ipv6 address") << any6Address << true << group6Address << true;
    for (const QHostAddress &a : qAsConst(linklocalMulticastGroups))
        QTest::addRow("valid bind, %s-link group ipv6 address", a.scopeId().toLatin1().constData())
                << any6Address << true << a << true;
    QTest::newRow("valid bind, invalid group ipv6 address") << any6Address << true << any6Address << false;
    QTest::newRow("dual bind, group ipv4 address") << dualAddress << true << groupAddress << false;
    QTest::newRow("dual bind, group ipv6 address") << dualAddress << true << group6Address << true;
    for (const QHostAddress &a : qAsConst(linklocalMulticastGroups))
        QTest::addRow("dual bind, %s-link group ipv6 address", a.scopeId().toLatin1().constData())
                << dualAddress << true << a << true;
}

void tst_QUdpSocket::multicast()
{
#ifdef Q_OS_WINRT
    QSKIP("WinRT does not support multicast.");
#endif
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH(QHostAddress, bindAddress);
    QFETCH(bool, bindResult);
    QFETCH(QHostAddress, groupAddress);
    QFETCH(bool, joinResult);
    if (groupAddress.protocol() == QAbstractSocket::IPv6Protocol && !QtNetworkSettings::hasIPv6())
        QSKIP("system doesn't support ipv6!");
    if (setProxy) {
        // UDP multicast does not work with proxies
        return;
    }

    // Some syscalls needed for ipv6 udp multicasting are not functional
    if (m_skipUnsupportedIPv6Tests) {
        if (groupAddress.protocol() == QAbstractSocket::IPv6Protocol) {
            QSKIP("Syscalls needed for ipv6 udp multicasting missing functionality");
        }
    }

    QUdpSocket receiver;

    // bind first, then verify that we can join the multicast group
    QVERIFY2(receiver.bind(bindAddress, 0) == bindResult,
             qPrintable(receiver.errorString()));
    if (!bindResult)
        return;

    if (bindAddress == QHostAddress::Any && groupAddress.protocol() == QAbstractSocket::IPv4Protocol) {
        QCOMPARE(joinResult, false);
        QTest::ignoreMessage(QtWarningMsg,
                             "QAbstractSocket: cannot bind to QHostAddress::Any (or an IPv6 address) and join an IPv4 multicast group;"
                             " bind to QHostAddress::AnyIPv4 instead if you want to do this");
    }
    QVERIFY2(receiver.joinMulticastGroup(groupAddress, interfaceForGroup(groupAddress)) == joinResult,
             qPrintable(receiver.errorString()));
    if (!joinResult)
        return;

    QList<QByteArray> datagrams = QList<QByteArray>()
                                  << QByteArray("0123")
                                  << QByteArray("4567")
                                  << QByteArray("89ab")
                                  << QByteArray("cdef");

    QUdpSocket sender;
    sender.bind();
    foreach (const QByteArray &datagram, datagrams) {
        QNetworkDatagram dgram(datagram, groupAddress, receiver.localPort());
        dgram.setInterfaceIndex(interfaceForGroup(groupAddress).index());
        QCOMPARE(int(sender.writeDatagram(dgram)),
                 int(datagram.size()));
    }

    QVERIFY2(receiver.waitForReadyRead(), QtNetworkSettings::msgSocketError(receiver).constData());
    QVERIFY(receiver.hasPendingDatagrams());
    QList<QByteArray> receivedDatagrams;
    while (receiver.hasPendingDatagrams()) {
        QNetworkDatagram dgram = receiver.receiveDatagram();
        receivedDatagrams << dgram.data();

        QVERIFY2(allAddresses.contains(dgram.senderAddress()),
                dgram.senderAddress().toString().toLatin1());
        QCOMPARE(dgram.senderPort(), int(sender.localPort()));
        if (!dgram.destinationAddress().isNull()) {
            QCOMPARE(dgram.destinationAddress(), groupAddress);
            QCOMPARE(dgram.destinationPort(), int(receiver.localPort()));
        }

        int ttl = dgram.hopLimit();
        if (ttl != -1)
            QVERIFY(ttl != 0);
    }
    QCOMPARE(receivedDatagrams, datagrams);

    QVERIFY2(receiver.leaveMulticastGroup(groupAddress, interfaceForGroup(groupAddress)),
             qPrintable(receiver.errorString()));
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
    QVERIFY2(successes >= 9, QByteArray::number(successes).constData());
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
#if defined(Q_OS_WIN)
            // Do not add the Teredo Tunneling Pseudo Interface on Windows.
            if (iface.humanReadableName().contains("Teredo"))
                continue;
#elif defined(Q_OS_DARWIN)
            // Do not add "utun" interfaces on macOS: nothing ever gets received
            // (we don't know why)
            if (iface.name().startsWith("utun"))
                continue;
#endif

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
        QVERIFY2(s->bind(addr, port), addr.toString().toLatin1()
                 + '/' + QByteArray::number(port) + ": " + qPrintable(s->errorString()));
        port = s->localPort(); //bind same port, different networks
        sockets << s;
    }

    QUdpSocket neutral;
    QVERIFY(neutral.bind(QHostAddress(QHostAddress::AnyIPv6)));
    QSignalSpy neutralReadSpy(&neutral, SIGNAL(readyRead()));

    QByteArray testData("hello");
    foreach (QUdpSocket *s, sockets) {
        QSignalSpy spy(s, SIGNAL(readyRead()));

        neutralReadSpy.clear();
        QVERIFY(s->writeDatagram(testData, s->localAddress(), neutral.localPort()));
        QTRY_VERIFY(neutralReadSpy.count() > 0); //note may need to accept a firewall prompt

        QNetworkDatagram dgram = neutral.receiveDatagram(testData.length() * 2);
        QVERIFY(dgram.isValid());
        QCOMPARE(dgram.senderAddress(), s->localAddress());
        QCOMPARE(dgram.senderPort(), int(s->localPort()));
        QCOMPARE(dgram.destinationAddress(), s->localAddress());
        QCOMPARE(dgram.destinationPort(), int(neutral.localPort()));
        QCOMPARE(dgram.data().length(), testData.length());
        QCOMPARE(dgram.data(), testData);

        QVERIFY(neutral.writeDatagram(dgram.makeReply(testData)));
        QTRY_VERIFY(spy.count() > 0); //note may need to accept a firewall prompt

        dgram = s->receiveDatagram(testData.length() * 2);
        QCOMPARE(dgram.data(), testData);

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
#if defined(Q_OS_WIN)
            // Do not add the Teredo Tunneling Pseudo Interface on Windows.
            if (iface.humanReadableName().contains("Teredo"))
                continue;
#elif defined(Q_OS_DARWIN)
            // Do not add "utun" interfaces on macOS: nothing ever gets received
            // (we don't know why)
            if (iface.name().startsWith("utun"))
                continue;
#endif
            foreach (QNetworkAddressEntry addr, iface.addressEntries()) {
                if (addr.ip().isInSubnet(localMask, 16)) {
                    addresses << addr.ip();
                    qDebug() << "Found IPv4 link local address" << addr.ip();
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
    foreach (QUdpSocket *s, sockets) {
        QVERIFY(s->writeDatagram(testData, s->localAddress(), neutral.localPort()));
        QVERIFY2(neutral.waitForReadyRead(10000), QtNetworkSettings::msgSocketError(neutral).constData());

        QNetworkDatagram dgram = neutral.receiveDatagram(testData.length() * 2);
        QVERIFY(dgram.isValid());
        QCOMPARE(dgram.senderAddress(), s->localAddress());
        QCOMPARE(dgram.senderPort(), int(s->localPort()));
        QCOMPARE(dgram.data().length(), testData.length());
        QCOMPARE(dgram.data(), testData);

        // Unlike for IPv6 with IPV6_PKTINFO, IPv4 has no standardized way of
        // obtaining the packet's destination addresses. The destinationAddress
        // and destinationPort calls could fail, so whitelist the OSes we know
        // we have an implementation.
#if defined(Q_OS_LINUX) || defined(Q_OS_BSD4) || defined(Q_OS_WIN)
        QVERIFY(dgram.destinationPort() != -1);
#endif
        if (dgram.destinationPort() == -1) {
            QCOMPARE(dgram.destinationAddress().protocol(), QAbstractSocket::UnknownNetworkLayerProtocol);
        } else {
            QCOMPARE(dgram.destinationAddress(), s->localAddress());
            QCOMPARE(dgram.destinationPort(), int(neutral.localPort()));
        }

        QVERIFY(neutral.writeDatagram(dgram.makeReply(testData)));
        QVERIFY2(s->waitForReadyRead(10000), QtNetworkSettings::msgSocketError(*s).constData());
        dgram = s->receiveDatagram(testData.length() * 2);
        QVERIFY(dgram.isValid());
        QCOMPARE(dgram.data(), testData);

        //sockets bound to other interfaces shouldn't have received anything
        foreach (QUdpSocket *s2, sockets) {
            QCOMPARE((int)s2->bytesAvailable(), 0);
        }
    }
    qDeleteAll(sockets);
}

void tst_QUdpSocket::readyRead()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    char buf[1];
    QUdpSocket sender, receiver;

    QVERIFY(receiver.bind(QHostAddress(QHostAddress::AnyIPv4), 0));
    quint16 port = receiver.localPort();
    QVERIFY(port != 0);

    QSignalSpy spy(&receiver, SIGNAL(readyRead()));

    // send a datagram to that port
    sender.writeDatagram("aa", makeNonAny(receiver.localAddress()), port);

    // wait a little
    // if QTBUG-43857 is still going, we'll live-lock on socket notifications from receiver's socket
    QTest::qWait(100);

    // make sure only one signal was emitted
    QCOMPARE(spy.count(), 1);
    QVERIFY(receiver.hasPendingDatagrams());
#ifdef RELIABLE_BYTES_AVAILABLE
    QCOMPARE(receiver.bytesAvailable(), qint64(2));
#endif
    QCOMPARE(receiver.pendingDatagramSize(), qint64(2));

    // write another datagram
    sender.writeDatagram("ab", makeNonAny(receiver.localAddress()), port);

    // no new signal should be emitted because we haven't read the first datagram yet
    QTest::qWait(100);
    QCOMPARE(spy.count(), 1);
    QVERIFY(receiver.hasPendingDatagrams());
    QVERIFY(receiver.bytesAvailable() >= 1);    // most likely is 1, but it could be 1 + 2 in the future
    QCOMPARE(receiver.pendingDatagramSize(), qint64(2));

    // read all the datagrams (we could read one only, but we can't be sure the OS is queueing)
    while (receiver.hasPendingDatagrams())
        receiver.readDatagram(buf, sizeof buf);

    // write a new datagram and ensure the signal is emitted now
    sender.writeDatagram("abc", makeNonAny(receiver.localAddress()), port);
    QTest::qWait(100);
    QCOMPARE(spy.count(), 2);
    QVERIFY(receiver.hasPendingDatagrams());
#ifdef RELIABLE_BYTES_AVAILABLE
    QCOMPARE(receiver.bytesAvailable(), qint64(3));
#endif
    QCOMPARE(receiver.pendingDatagramSize(), qint64(3));
}

void tst_QUdpSocket::readyReadForEmptyDatagram()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QUdpSocket sender, receiver;

    QVERIFY(receiver.bind(QHostAddress(QHostAddress::AnyIPv4), 0));
    quint16 port = receiver.localPort();
    QVERIFY(port != 0);

    connect(&receiver, SIGNAL(readyRead()), SLOT(empty_readyReadSlot()));

    // send an empty datagram to that port
    sender.writeDatagram("", makeNonAny(receiver.localAddress()), port);

    // ensure that we got a readyRead, despite bytesAvailable() == 0
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());

    char buf[1];
    QVERIFY(receiver.hasPendingDatagrams());
    QCOMPARE(receiver.pendingDatagramSize(), qint64(0));
#ifdef RELIABLE_BYTES_AVAILABLE
    QCOMPARE(receiver.bytesAvailable(), qint64(0));
#endif
    QCOMPARE(receiver.readDatagram(buf, sizeof buf), qint64(0));
}

void tst_QUdpSocket::async_readDatagramSlot()
{
    char buf[1];
    QVERIFY(m_asyncReceiver->hasPendingDatagrams());
    QCOMPARE(m_asyncReceiver->pendingDatagramSize(), qint64(1));
#ifdef RELIABLE_BYTES_AVAILABLE
    QCOMPARE(m_asyncReceiver->bytesAvailable(), qint64(1));
#endif
    QCOMPARE(m_asyncReceiver->readDatagram(buf, sizeof(buf)), qint64(1));

    if (buf[0] == '2') {
        QTestEventLoop::instance().exitLoop();
        return;
    }

    m_asyncSender->writeDatagram("2", makeNonAny(m_asyncReceiver->localAddress()), m_asyncReceiver->localPort());
    // wait a little to ensure that the datagram we've just sent
    // will be delivered on receiver side.
    QTest::qSleep(100);
}

void tst_QUdpSocket::asyncReadDatagram()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    m_asyncSender = new QUdpSocket;
    m_asyncReceiver = new QUdpSocket;

    QVERIFY(m_asyncReceiver->bind(QHostAddress(QHostAddress::AnyIPv4), 0));
    quint16 port = m_asyncReceiver->localPort();
    QVERIFY(port != 0);

    QSignalSpy spy(m_asyncReceiver, SIGNAL(readyRead()));
    connect(m_asyncReceiver, SIGNAL(readyRead()), SLOT(async_readDatagramSlot()));

    m_asyncSender->writeDatagram("1", makeNonAny(m_asyncReceiver->localAddress()), port);

    QTestEventLoop::instance().enterLoop(1);

    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(spy.count(), 2);

    delete m_asyncSender;
    delete m_asyncReceiver;
}

void tst_QUdpSocket::writeInHostLookupState()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QUdpSocket socket;
    socket.connectToHost("nosuchserver.qt-project.org", 80);
    QCOMPARE(socket.state(), QUdpSocket::HostLookupState);
    QVERIFY(!socket.putChar('0'));
}

QTEST_MAIN(tst_QUdpSocket)
#include "tst_qudpsocket.moc"
