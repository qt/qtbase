/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qglobal.h>
// To prevent windows system header files from re-defining min/max
#define NOMINMAX 1
#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#define SOCKET int
#define INVALID_SOCKET -1
#endif

#include <QtTest/QtTest>

#ifndef Q_OS_WIN
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include <qcoreapplication.h>
#include <qtcpsocket.h>
#include <qtcpserver.h>
#include <qhostaddress.h>
#if QT_CONFIG(process)
# include <qprocess.h>
#endif
#include <qstringlist.h>
#include <qplatformdefs.h>
#include <qhostinfo.h>

#include <QNetworkProxy>

#include <QNetworkSession>
#include <QNetworkConfiguration>
#include <QNetworkConfigurationManager>
#include "../../../network-settings.h"

#if defined(Q_OS_LINUX)
#define SHOULD_CHECK_SYSCALL_SUPPORT
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#endif

class tst_QTcpServer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase_data();
    void initTestCase();
    void init();
    void cleanup();
    void getSetCheck();
    void constructing();
    void clientServerLoop();
    void ipv6Server();
    void dualStack_data();
    void dualStack();
    void ipv6ServerMapped();
    void crashTests();
    void maxPendingConnections();
    void listenError();
    void waitForConnectionTest();
#ifndef Q_OS_WINRT
    void setSocketDescriptor();
#endif
    void listenWhileListening();
    void addressReusable();
    void setNewSocketDescriptorBlocking();
#ifndef QT_NO_NETWORKPROXY
    void invalidProxy_data();
    void invalidProxy();
    void proxyFactory_data();
    void proxyFactory();
#endif // !QT_NO_NETWORKPROXY

    void qtbug14268_peek();

    void serverAddress_data();
    void serverAddress();

    void qtbug6305_data() { serverAddress_data(); }
    void qtbug6305();

    void linkLocal();

    void eagainBlockingAccept();

    void canAccessPendingConnectionsWhileNotListening();

private:
    bool shouldSkipIpv6TestsForBrokenGetsockopt();
#ifdef SHOULD_CHECK_SYSCALL_SUPPORT
    bool ipv6GetsockoptionMissing(int level, int optname);
#endif

#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkSession *networkSession;
#endif
    QString crashingServerDir;
};

// Testing get/set functions
void tst_QTcpServer::getSetCheck()
{
    QTcpServer obj1;
    // int QTcpServer::maxPendingConnections()
    // void QTcpServer::setMaxPendingConnections(int)
    obj1.setMaxPendingConnections(0);
    QCOMPARE(0, obj1.maxPendingConnections());
    obj1.setMaxPendingConnections(INT_MIN);
    QCOMPARE(INT_MIN, obj1.maxPendingConnections());
    obj1.setMaxPendingConnections(INT_MAX);
    QCOMPARE(INT_MAX, obj1.maxPendingConnections());
}

void tst_QTcpServer::initTestCase_data()
{
    QTest::addColumn<bool>("setProxy");
    QTest::addColumn<int>("proxyType");

    QTest::newRow("WithoutProxy") << false << 0;
#if QT_CONFIG(socks5)
    QTest::newRow("WithSocks5Proxy") << true << int(QNetworkProxy::Socks5Proxy);
#endif

    crashingServerDir = QFINDTESTDATA("crashingServer");
    QVERIFY2(!crashingServerDir.isEmpty(), qPrintable(
        QString::fromLatin1("Couldn't find crashingServer dir starting from %1.").arg(QDir::currentPath())));
}

void tst_QTcpServer::initTestCase()
{
#ifdef QT_TEST_SERVER
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::socksProxyServerName(), 1080));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::httpProxyServerName(), 3128));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::ftpProxyServerName(), 2121));
    QVERIFY(QtNetworkSettings::verifyConnection(QtNetworkSettings::imapServerName(), 143));
#else
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
#endif
#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkConfigurationManager man;
    networkSession = new QNetworkSession(man.defaultConfiguration(), this);
    networkSession->open();
    QVERIFY(networkSession->waitForOpened());
#endif
}

void tst_QTcpServer::init()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::socksProxyServerName(), 1080));
        }
#else // !QT_NO_NETWORKPROXY
        QSKIP("No proxy support");
#endif // QT_NO_NETWORKPROXY
    }
}

void tst_QTcpServer::cleanup()
{
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
#endif
}

#ifdef SHOULD_CHECK_SYSCALL_SUPPORT
bool tst_QTcpServer::ipv6GetsockoptionMissing(int level, int optname)
{
    int testSocket;

    testSocket = socket(PF_INET6, SOCK_STREAM, 0);

    // If we can't test here, assume it's not missing
    if (testSocket == -1)
        return false;

    bool result = false;
    if (getsockopt(testSocket, level, optname, nullptr, 0) == -1) {
        if (errno == EOPNOTSUPP) {
            result = true;
        }
    }

    close(testSocket);
    return result;
}
#endif //SHOULD_CHECK_SYSCALL_SUPPORT

bool tst_QTcpServer::shouldSkipIpv6TestsForBrokenGetsockopt()
{
#ifdef SHOULD_CHECK_SYSCALL_SUPPORT
    // Following parameters for setsockopt are not supported by all QEMU versions:
    if (ipv6GetsockoptionMissing(SOL_IPV6, IPV6_V6ONLY)) {
        return true;
    }
#endif //SHOULD_CHECK_SYSCALL_SUPPORT

    return false;
}


//----------------------------------------------------------------------------------

void tst_QTcpServer::constructing()
{
    QTcpServer socket;

    // Check the initial state of the QTcpSocket.
    QCOMPARE(socket.isListening(), false);
    QCOMPARE((int)socket.serverPort(), 0);
    QCOMPARE(socket.serverAddress(), QHostAddress());
    QCOMPARE(socket.maxPendingConnections(), 30);
    QCOMPARE(socket.hasPendingConnections(), false);
    QCOMPARE(socket.socketDescriptor(), (qintptr)-1);
    QCOMPARE(socket.serverError(), QAbstractSocket::UnknownSocketError);

    // Check the state of the socket layer?
}

//----------------------------------------------------------------------------------
void tst_QTcpServer::clientServerLoop()
{
    QTcpServer server;

    QSignalSpy spy(&server, SIGNAL(newConnection()));

    QVERIFY(!server.isListening());
    QVERIFY(!server.hasPendingConnections());
    QVERIFY(server.listen(QHostAddress::Any, 11423));
    QVERIFY(server.isListening());

    QTcpSocket client;

    QHostAddress serverAddress = QHostAddress::LocalHost;
    if (!(server.serverAddress() == QHostAddress::Any) && !(server.serverAddress() == QHostAddress::AnyIPv6) && !(server.serverAddress() == QHostAddress::AnyIPv4))
        serverAddress = server.serverAddress();

    client.connectToHost(serverAddress, server.serverPort());
    QVERIFY(client.waitForConnected(5000));

    QVERIFY(server.waitForNewConnection(5000));
    QVERIFY(server.hasPendingConnections());

    QCOMPARE(spy.count(), 1);

    QTcpSocket *serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket != 0);

    QVERIFY(serverSocket->write("Greetings, client!\n", 19) == 19);
    serverSocket->flush();

    QVERIFY(client.waitForReadyRead(5000));
    QByteArray arr = client.readAll();
    QCOMPARE(arr.constData(), "Greetings, client!\n");

    QVERIFY(client.write("Well, hello to you!\n", 20) == 20);
    client.flush();

    QVERIFY(serverSocket->waitForReadyRead(5000));
    arr = serverSocket->readAll();
    QCOMPARE(arr.constData(), "Well, hello to you!\n");
}

//----------------------------------------------------------------------------------
void tst_QTcpServer::ipv6Server()
{
    if (!QtNetworkSettings::hasIPv6())
        QSKIP("system doesn't support ipv6!");
    //### need to enter the event loop for the server to get the connection ?? ( windows)
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHostIPv6, 8944)) {
        QCOMPARE(server.serverError(), QAbstractSocket::UnsupportedSocketOperationError);
        return;
    }

    QCOMPARE(server.serverPort(), quint16(8944));
    QVERIFY(server.serverAddress() == QHostAddress::LocalHostIPv6);

    QTcpSocket client;
    client.connectToHost("::1", 8944);
    QVERIFY(client.waitForConnected(5000));

    QVERIFY(server.waitForNewConnection());
    QVERIFY(server.hasPendingConnections());

    QTcpSocket *serverSocket = 0;
    QVERIFY((serverSocket = server.nextPendingConnection()));
    serverSocket->close();
    delete serverSocket;
}

Q_DECLARE_METATYPE(QHostAddress);

void tst_QTcpServer::dualStack_data()
{
    QTest::addColumn<QHostAddress>("bindAddress");
    QTest::addColumn<bool>("v4ok");
    QTest::addColumn<bool>("v6ok");
    QTest::newRow("any") << QHostAddress(QHostAddress::Any) << true << true;
    QTest::newRow("anyIPv4") << QHostAddress(QHostAddress::AnyIPv4) << true << false;
    QTest::newRow("anyIPv6") << QHostAddress(QHostAddress::AnyIPv6) << false << true;
}

void tst_QTcpServer::dualStack()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        QSKIP("test server proxy doesn't support ipv6");
    if (!QtNetworkSettings::hasIPv6())
        QSKIP("system doesn't support ipv6!");
    QFETCH(QHostAddress, bindAddress);
    QFETCH(bool, v4ok);
    QFETCH(bool, v6ok);

    QTcpServer server;
    QVERIFY(server.listen(bindAddress));

    QTcpSocket v4client;
    v4client.connectToHost(QHostAddress::LocalHost, server.serverPort());

    QTcpSocket v6client;
    v6client.connectToHost(QHostAddress::LocalHostIPv6, server.serverPort());

    QCOMPARE(v4client.waitForConnected(5000), v4ok);
    QCOMPARE(v6client.waitForConnected(5000), v6ok);
}

//----------------------------------------------------------------------------------
void tst_QTcpServer::ipv6ServerMapped()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    // let's try the normal case
    QTcpSocket client1;
    client1.connectToHost("127.0.0.1", server.serverPort());
    QVERIFY(server.waitForNewConnection(5000));
    delete server.nextPendingConnection();

    if (!QtNetworkSettings::hasIPv6())
        QSKIP("system doesn't support ipv6!");

    // let's try the mapped one in the nice format
    QTcpSocket client2;
    client2.connectToHost("::ffff:127.0.0.1", server.serverPort());
    QVERIFY(server.waitForNewConnection(5000));
    delete server.nextPendingConnection();

    // let's try the mapped in hex format
    QTcpSocket client3;
    client3.connectToHost("::ffff:7F00:0001", server.serverPort());
    QVERIFY(server.waitForNewConnection(5000));
    delete server.nextPendingConnection();

    // However connecting to the v6 localhost should not work
    QTcpSocket client4;
    client4.connectToHost("::1", server.serverPort());
    QVERIFY(!server.waitForNewConnection(5000));
}

//----------------------------------------------------------------------------------
void tst_QTcpServer::crashTests()
{
    QTcpServer server;
    server.close();
    QVERIFY(server.listen());
}

//----------------------------------------------------------------------------------
void tst_QTcpServer::maxPendingConnections()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy)
            QSKIP("With socks5 only 1 connection is allowed ever");
#else // !QT_NO_NETWORKPROXY
        QSKIP("No proxy support");
#endif // QT_NO_NETWORKPROXY
    }
    //### sees to fail sometimes ... a timing issue with the test on windows
    QTcpServer server;
    server.setMaxPendingConnections(2);

    QTcpSocket socket1;
    QTcpSocket socket2;
    QTcpSocket socket3;

    QSignalSpy spy(&server, SIGNAL(newConnection()));
    QVERIFY(server.listen());

    socket1.connectToHost(QHostAddress::LocalHost, server.serverPort());
    socket2.connectToHost(QHostAddress::LocalHost, server.serverPort());
    socket3.connectToHost(QHostAddress::LocalHost, server.serverPort());

    // We must have two and only two connections. First compare waits until
    // two connections have been made. The second compare makes sure no
    // more are accepted. Creating connections happens multithreaded so
    // qWait must be used for that.
    QTRY_COMPARE(spy.count(), 2);
    QTest::qWait(100);
    QCOMPARE(spy.count(), 2);

    QVERIFY(server.hasPendingConnections());
    QVERIFY(server.nextPendingConnection());
    QVERIFY(server.hasPendingConnections());
    QVERIFY(server.nextPendingConnection());
    QVERIFY(!server.hasPendingConnections());
    QCOMPARE(server.nextPendingConnection(), (QTcpSocket*)0);

    QVERIFY(server.waitForNewConnection(5000));

    QVERIFY(server.hasPendingConnections());
    QVERIFY(server.nextPendingConnection());
}

//----------------------------------------------------------------------------------
void tst_QTcpServer::listenError()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy)
            QSKIP("With socks5 we can not make hard requirements on the address or port");
#else // !QT_NO_NETWORKPROXY
        QSKIP("No proxy support");
#endif //QT_NO_NETWORKPROXY
    }
    QTcpServer server;
    QVERIFY(!server.listen(QHostAddress("1.2.3.4"), 0));
    QCOMPARE(server.serverError(), QAbstractSocket::SocketAddressNotAvailableError);
    QCOMPARE(server.errorString().toLatin1().constData(), "The address is not available");
}

class ThreadConnector : public QThread
{
public:
    ThreadConnector(const QHostAddress &host, quint16 port)
        : host(host), port(port)
    { }

    ~ThreadConnector()
    {
        wait();
    }

protected:
    void run()
    {
        sleep(2);

        QTcpSocket socket;
        socket.connectToHost(host, port);

        QEventLoop loop;
        QTimer::singleShot(5000, &loop, SLOT(quit()));
        loop.exec();
    }

private:
    QHostAddress host;
    quint16 port;
};

//----------------------------------------------------------------------------------
void tst_QTcpServer::waitForConnectionTest()
{

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy)
            QSKIP("Localhost servers don't work well with SOCKS5");
#else // !QT_NO_NETWORKPROXY
        QSKIP("No proxy support");
#endif // QT_NO_NETWORKPROXY
    }

    QTcpSocket findLocalIpSocket;
    findLocalIpSocket.connectToHost(QtNetworkSettings::imapServerName(), 143);
    QVERIFY(findLocalIpSocket.waitForConnected(5000));

    QTcpServer server;
    bool timeout = false;
    QVERIFY(server.listen(findLocalIpSocket.localAddress()));
    QVERIFY(!server.waitForNewConnection(1000, &timeout));
    QCOMPARE(server.serverError(), QAbstractSocket::SocketTimeoutError);
    QVERIFY(timeout);

    ThreadConnector connector(findLocalIpSocket.localAddress(), server.serverPort());
    connector.start();

    QVERIFY(server.waitForNewConnection(3000, &timeout));
    QVERIFY(!timeout);
}

//----------------------------------------------------------------------------------
#ifndef Q_OS_WINRT
void tst_QTcpServer::setSocketDescriptor()
{
    QTcpServer server;
    QVERIFY(!server.setSocketDescriptor(42));
    QCOMPARE(server.serverError(), QAbstractSocket::UnsupportedSocketOperationError);
    //adopting Open C sockets is not supported, neither is adopting externally created RSocket
#ifdef Q_OS_WIN
    // ensure winsock is started
    WSADATA wsaData;
    QVERIFY(WSAStartup(MAKEWORD(2,0), &wsaData) == NO_ERROR);
#endif

    SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    QVERIFY(sock != INVALID_SOCKET);

    sockaddr_in sin;
    memset(&sin, 0, sizeof(sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = 0;
    sin.sin_addr.s_addr = 0x00000000;
    QVERIFY(::bind(sock, (sockaddr*)&sin, sizeof(sockaddr_in)) == 0);
    QVERIFY(::listen(sock, 10) == 0);
    QVERIFY(server.setSocketDescriptor(sock));

#ifdef Q_OS_WIN
    WSACleanup();
#endif
}
#endif // !Q_OS_WINRT

//----------------------------------------------------------------------------------
void tst_QTcpServer::listenWhileListening()
{
    QTcpServer server;
    QVERIFY(server.listen());
    QTest::ignoreMessage(QtWarningMsg, "QTcpServer::listen() called when already listening");
    QVERIFY(!server.listen());
}

//----------------------------------------------------------------------------------

class SeverWithBlockingSockets : public QTcpServer
{
public:
    SeverWithBlockingSockets()
        : ok(false) { }

    bool ok;

protected:
#ifndef Q_OS_WINRT
    void incomingConnection(qintptr socketDescriptor)
    {
        // how a user woulddo it (qabstractsocketengine is not public)
        unsigned long arg = 0;
#if defined(Q_OS_WIN)
        ok = ::ioctlsocket(socketDescriptor, FIONBIO, &arg) == 0;
        ::closesocket(socketDescriptor);
#else
        ok = ::ioctl(socketDescriptor, FIONBIO, &arg) == 0;
        ::close(socketDescriptor);
#endif
    }
#endif // !Q_OS_WINRT
};

void tst_QTcpServer::addressReusable()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
#ifdef Q_OS_LINUX
    QSKIP("The addressReusable test is unstable on Linux. See QTBUG-39985.");
#endif
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy)
            QSKIP("With socks5 this test does not make senans at the momment");
#else // !QT_NO_NETWORKPROXY
        QSKIP("No proxy support");
#endif // QT_NO_NETWORKPROXY
    }
    // The crashingServer process will crash once it gets a connection.
    QProcess process;
    QString processExe = crashingServerDir + "/crashingServer";
    process.start(processExe);
    QVERIFY2(process.waitForStarted(), qPrintable(
        QString::fromLatin1("Could not start %1: %2").arg(processExe, process.errorString())));
    QVERIFY(process.waitForReadyRead(5000));

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 49199);
    QVERIFY(socket.waitForConnected(5000));

    QVERIFY(process.waitForFinished(30000));

    // Give the system some time.
    QTest::qSleep(10);

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 49199));
#endif
}

void tst_QTcpServer::setNewSocketDescriptorBlocking()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy)
            QSKIP("With socks5 we can not make the socket descripter blocking");
#else // !QT_NO_NETWORKPROXY
        QSKIP("No proxy support");
#endif // QT_NO_NETWORKPROXY
    }
    SeverWithBlockingSockets server;
    QVERIFY(server.listen());

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(server.waitForNewConnection(5000));
    QVERIFY(server.ok);
}

#ifndef QT_NO_NETWORKPROXY
void tst_QTcpServer::invalidProxy_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<int>("expectedError");

    const QString imapIp = QtNetworkSettings::imapServerIp().toString();
    const QString httpProxyIp = QtNetworkSettings::httpProxyServerIp().toString();
    const QString socksIp = QtNetworkSettings::socksProxyServerIp().toString();
    QTest::newRow("ftp-proxy") << int(QNetworkProxy::FtpCachingProxy) << imapIp << 143
                               << int(QAbstractSocket::UnsupportedSocketOperationError);
    QTest::newRow("http-proxy") << int(QNetworkProxy::HttpProxy) << httpProxyIp << 3128
                                << int(QAbstractSocket::UnsupportedSocketOperationError);

    QTest::newRow("no-such-host") << int(QNetworkProxy::Socks5Proxy)
                                  << "invalid.test.qt-project.org" << 1080
                                  << int(QAbstractSocket::ProxyNotFoundError);
    QTest::newRow("socks5-on-http") << int(QNetworkProxy::Socks5Proxy) << httpProxyIp << 3128
                                    << int(QAbstractSocket::SocketTimeoutError);
}

void tst_QTcpServer::invalidProxy()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QFETCH(int, type);
    QFETCH(QString, host);
    QFETCH(int, port);
    QNetworkProxy::ProxyType proxyType = QNetworkProxy::ProxyType(type);
    QNetworkProxy proxy(proxyType, host, port);

    QTcpServer server;
    server.setProxy(proxy);
    bool listenResult = server.listen();

    QVERIFY(!listenResult);
    QVERIFY(!server.errorString().isEmpty());

    // note: the following test is not a hard failure.
    // Sometimes, error codes change for the better
    QTEST(int(server.serverError()), "expectedError");
}

// copied from tst_qnetworkreply.cpp
class MyProxyFactory: public QNetworkProxyFactory
{
public:
    int callCount;
    QList<QNetworkProxy> toReturn;
    QNetworkProxyQuery lastQuery;
    inline MyProxyFactory() { clear(); }

    inline void clear()
    {
        callCount = 0;
        toReturn = QList<QNetworkProxy>() << QNetworkProxy::DefaultProxy;
        lastQuery = QNetworkProxyQuery();
    }

    virtual QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query)
    {
        lastQuery = query;
        ++callCount;
        return toReturn;
    }
};

void tst_QTcpServer::proxyFactory_data()
{
    QTest::addColumn<QList<QNetworkProxy> >("proxyList");
    QTest::addColumn<QNetworkProxy>("proxyUsed");
    QTest::addColumn<bool>("fails");
    QTest::addColumn<int>("expectedError");

    QList<QNetworkProxy> proxyList;

    // tests that do get to listen

    proxyList << QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::socksProxyServerName(), 1080);
    QTest::newRow("socks5")
        << proxyList << proxyList.at(0)
        << false << int(QAbstractSocket::UnknownSocketError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::httpProxyServerName(), 3128)
              << QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::socksProxyServerName(), 1080);
    QTest::newRow("cachinghttp+socks5")
        << proxyList << proxyList.at(1)
        << false << int(QAbstractSocket::UnknownSocketError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::FtpCachingProxy, QtNetworkSettings::ftpProxyServerName(), 2121)
              << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::httpProxyServerName(), 3128)
              << QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::socksProxyServerName(), 1080);
    QTest::newRow("ftp+cachinghttp+socks5")
        << proxyList << proxyList.at(2)
        << false << int(QAbstractSocket::UnknownSocketError);

    // tests that fail to listen
    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::HttpProxy, QtNetworkSettings::httpProxyServerName(), 3128);
    QTest::newRow("http")
        << proxyList << proxyList.at(0)
        << true << int(QAbstractSocket::UnsupportedSocketOperationError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::httpProxyServerName(), 3128);
    QTest::newRow("cachinghttp")
        << proxyList << QNetworkProxy()
        << true << int(QAbstractSocket::UnsupportedSocketOperationError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::FtpCachingProxy, QtNetworkSettings::ftpProxyServerName(), 2121);
    QTest::newRow("ftp")
        << proxyList << QNetworkProxy()
        << true << int(QAbstractSocket::UnsupportedSocketOperationError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::FtpCachingProxy, QtNetworkSettings::ftpProxyServerName(), 2121)
              << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::httpProxyServerName(), 3128);
    QTest::newRow("ftp+cachinghttp")
        << proxyList << QNetworkProxy()
        << true << int(QAbstractSocket::UnsupportedSocketOperationError);
}

void tst_QTcpServer::proxyFactory()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QFETCH(QList<QNetworkProxy>, proxyList);
    QFETCH(QNetworkProxy, proxyUsed);
    QFETCH(bool, fails);

    MyProxyFactory *factory = new MyProxyFactory;
    factory->toReturn = proxyList;
    QNetworkProxyFactory::setApplicationProxyFactory(factory);

    QTcpServer server;
    bool listenResult = server.listen();

    // Verify that the factory was called properly
    QCOMPARE(factory->callCount, 1);
    QCOMPARE(factory->lastQuery, QNetworkProxyQuery(0, QString(), QNetworkProxyQuery::TcpServer));

    QCOMPARE(listenResult, !fails);
    QCOMPARE(server.errorString().isEmpty(), !fails);

    // note: the following test is not a hard failure.
    // Sometimes, error codes change for the better
    QTEST(int(server.serverError()), "expectedError");
}
#endif // !QT_NO_NETWORKPROXY

class Qtbug14268Helper : public QObject
{
    Q_OBJECT
public:
    QByteArray lastDataPeeked;
public slots:
    void newConnection() {
        QTcpServer* server=static_cast<QTcpServer*>(sender());
        QTcpSocket* s=server->nextPendingConnection();
        connect(s,SIGNAL(readyRead()),this,SLOT(onServerReadyRead()));
    }
    void onServerReadyRead() {
        QTcpSocket* clientSocket=static_cast<QTcpSocket*>(sender());
        lastDataPeeked = clientSocket->peek(128*1024).toHex();
        QTestEventLoop::instance().exitLoop();
    }
};

// there is a similar test inside tst_qtcpsocket that uses the waitFor* functions instead
void tst_QTcpServer::qtbug14268_peek()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QTcpServer server;
    server.listen();

    Qtbug14268Helper helper;
    QObject::connect(&server, SIGNAL(newConnection()), &helper, SLOT(newConnection()));

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(client.waitForConnected(2000));

    client.write("abc\n");
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(helper.lastDataPeeked, QByteArray("6162630a"));

    client.write("def\n");
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(helper.lastDataPeeked, QByteArray("6162630a6465660a"));

    client.write("ghi\n");
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(helper.lastDataPeeked, QByteArray("6162630a6465660a6768690a"));
}

void tst_QTcpServer::serverAddress_data()
{
    QTest::addColumn<QHostAddress>("listenAddress");
    QTest::addColumn<QHostAddress>("serverAddress");
    if (QtNetworkSettings::hasIPv6())
        QTest::newRow("Any") << QHostAddress(QHostAddress::Any) << QHostAddress(QHostAddress::Any);
    else
        QTest::newRow("Any") << QHostAddress(QHostAddress::Any) << QHostAddress(QHostAddress::AnyIPv4);
    QTest::newRow("AnyIPv4") << QHostAddress(QHostAddress::AnyIPv4) << QHostAddress(QHostAddress::AnyIPv4);
    if (QtNetworkSettings::hasIPv6())
        QTest::newRow("AnyIPv6") << QHostAddress(QHostAddress::AnyIPv6) << QHostAddress(QHostAddress::AnyIPv6);
    foreach (const QNetworkInterface &iface, QNetworkInterface::allInterfaces()) {
        if ((iface.flags() & QNetworkInterface::IsUp) == 0)
            continue;
        foreach (const QNetworkAddressEntry &entry, iface.addressEntries()) {
            QTest::newRow(qPrintable(entry.ip().toString())) << entry.ip() << entry.ip();
        }
    }
}

void tst_QTcpServer::serverAddress()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QFETCH(QHostAddress, listenAddress);
    QFETCH(QHostAddress, serverAddress);
    QTcpServer server;

    if (shouldSkipIpv6TestsForBrokenGetsockopt()
        && listenAddress == QHostAddress(QHostAddress::Any)) {
        QSKIP("Syscalls needed for ipv6 sockoptions missing functionality");
    }

    // TODO: why does this QSKIP?
    if (!server.listen(listenAddress))
        QSKIP(qPrintable(server.errorString()));
    QCOMPARE(server.serverAddress(), serverAddress);
}

// on OS X, calling listen() multiple times would succeed each time, which is
// most definitely not wanted.
void tst_QTcpServer::qtbug6305()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QFETCH(QHostAddress, listenAddress);

    QTcpServer server;
    QVERIFY2(server.listen(listenAddress), qPrintable(server.errorString()));

    QTcpServer server2;
    QVERIFY(!server2.listen(listenAddress, server.serverPort())); // second listen should fail
}

void tst_QTcpServer::linkLocal()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QList <QHostAddress> addresses;
    QSet <QString> scopes;
    QHostAddress localMaskv4("169.254.0.0");
    QHostAddress localMaskv6("fe80::");
    foreach (const QNetworkInterface& iface, QNetworkInterface::allInterfaces()) {
        //Windows preallocates link local addresses to interfaces that are down.
        //These may or may not work depending on network driver (they do not work for the Bluetooth PAN driver)
        if (iface.flags() & QNetworkInterface::IsUp) {
#if defined(Q_OS_WIN)
            // Do not connect to the Teredo Tunneling interface on Windows Xp.
            if (iface.humanReadableName() == QString("Teredo Tunneling Pseudo-Interface"))
                continue;
#elif defined(Q_OS_DARWIN)
            // Do not add "utun" interfaces on macOS: nothing ever gets received
            // (we don't know why)
            if (iface.name().startsWith("utun"))
                continue;
#endif
            foreach (QNetworkAddressEntry addressEntry, iface.addressEntries()) {
                QHostAddress addr = addressEntry.ip();
                if (addr.isInSubnet(localMaskv4, 16)) {
                    addresses << addr;
                    qDebug() << addr;
                }
                else if (!addr.scopeId().isEmpty() && addr.isInSubnet(localMaskv6, 64)) {
                    scopes << addr.scopeId();
                    addresses << addr;
                    qDebug() << addr;
                }
            }
        }
    }
    if (addresses.isEmpty())
        QSKIP("no link local addresses");

    QList<QTcpServer*> servers;
    quint16 port = 0;
    foreach (const QHostAddress& addr, addresses) {
        QTcpServer *server = new QTcpServer;
        QVERIFY(server->listen(addr, port));
        port = server->serverPort(); //listen to same port on different interfaces
        servers << server;
    }

    QList<QTcpSocket*> clients;
    foreach (const QHostAddress& addr, addresses) {
        //unbound socket
        QTcpSocket *socket = new QTcpSocket;
        socket->connectToHost(addr, port);
        QVERIFY(socket->waitForConnected(5000));
        clients << socket;
        //bound socket
        socket = new QTcpSocket;
        QVERIFY(socket->bind(addr));
        socket->connectToHost(addr, port);
        QVERIFY(socket->waitForConnected(5000));
        clients << socket;
    }

    //each server should have two connections
    foreach (QTcpServer* server, servers) {
        //qDebug() << "checking for connections" << server->serverAddress() << ":" << server->serverPort();
        QVERIFY(server->waitForNewConnection(5000));
        QTcpSocket* remote = server->nextPendingConnection();
        QVERIFY(remote != nullptr);
        remote->close();
        delete remote;
        if (!server->hasPendingConnections())
            QVERIFY(server->waitForNewConnection(5000));
        remote = server->nextPendingConnection();
        QVERIFY(remote != nullptr);
        remote->close();
        delete remote;
        QVERIFY(!server->hasPendingConnections());
    }

    //Connecting to the same address with different scope should normally fail
    //However it will pass if there are two interfaces connected to the same physical network,
    //e.g. connected via wired and wireless interfaces, or two wired NICs.
    //which is a reasonably common case.
    //So this is not auto tested.

    qDeleteAll(clients);
    qDeleteAll(servers);
}

void tst_QTcpServer::eagainBlockingAccept()
{
    QTcpServer server;
    server.listen(QHostAddress::LocalHost, 7896);

    // Receiving a new connection causes TemporaryError, but shouldn't pause accepting.
    QTcpSocket s;
    s.connectToHost(QHostAddress::LocalHost, 7896);
    QSignalSpy spy(&server, SIGNAL(newConnection()));
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 500);
    s.close();

    // To test try again, should connect just fine.
    s.connectToHost(QHostAddress::LocalHost, 7896);
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 500);
    s.close();
    server.close();
}

class NonListeningTcpServer : public QTcpServer
{
public:
    void addSocketFromOutside(QTcpSocket* s)
    {
        addPendingConnection(s);
    }
};

void tst_QTcpServer::canAccessPendingConnectionsWhileNotListening()
{
    NonListeningTcpServer server;
    QTcpSocket socket;
    server.addSocketFromOutside(&socket);
    QCOMPARE(&socket, server.nextPendingConnection());
}

QTEST_MAIN(tst_QTcpServer)
#include "tst_qtcpserver.moc"
