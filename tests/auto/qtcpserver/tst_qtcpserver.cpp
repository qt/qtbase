/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Just to get Q_OS_SYMBIAN
#include <qglobal.h>
#if defined(_WIN32) && !defined(Q_OS_SYMBIAN)
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
#include <qprocess.h>
#include <qstringlist.h>
#include <qplatformdefs.h>
#include <qhostinfo.h>

#include <QNetworkProxy>
Q_DECLARE_METATYPE(QNetworkProxy)
Q_DECLARE_METATYPE(QList<QNetworkProxy>)

#include <QNetworkSession>
#include <QNetworkConfiguration>
#include <QNetworkConfigurationManager>
#include "../network-settings.h"

//TESTED_CLASS=
//TESTED_FILES=

class tst_QTcpServer : public QObject
{
    Q_OBJECT

public:
    tst_QTcpServer();
    virtual ~tst_QTcpServer();


public slots:
    void initTestCase_data();
    void initTestCase();
    void init();
    void cleanup();
private slots:
    void getSetCheck();
    void constructing();
    void clientServerLoop();
    void ipv6Server();
    void ipv6ServerMapped();
    void crashTests();
    void maxPendingConnections();
    void listenError();
    void waitForConnectionTest();
    void setSocketDescriptor();
    void listenWhileListening();
    void addressReusable();
    void setNewSocketDescriptorBlocking();
    void invalidProxy_data();
    void invalidProxy();
    void proxyFactory_data();
    void proxyFactory();

    void qtbug14268_peek();

private:
#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkSession *networkSession;
#endif
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

tst_QTcpServer::tst_QTcpServer()
{
    Q_SET_DEFAULT_IAP
}

tst_QTcpServer::~tst_QTcpServer()
{
}

void tst_QTcpServer::initTestCase_data()
{
    QTest::addColumn<bool>("setProxy");
    QTest::addColumn<int>("proxyType");

    QTest::newRow("WithoutProxy") << false << 0;
    QTest::newRow("WithSocks5Proxy") << true << int(QNetworkProxy::Socks5Proxy);
}

void tst_QTcpServer::initTestCase()
{
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
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080));
        }
    }
}

void tst_QTcpServer::cleanup()
{
    QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
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
    QCOMPARE(socket.socketDescriptor(), -1);
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
    if (!(server.serverAddress() == QHostAddress::Any) && !(server.serverAddress() == QHostAddress::AnyIPv6))
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
    //### need to enter the event loop for the server to get the connection ?? ( windows)
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHostIPv6, 8944)) {
        QVERIFY(server.serverError() == QAbstractSocket::UnsupportedSocketOperationError);
        return;
    }

    QVERIFY(server.serverPort() == 8944);
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
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QSKIP("With socks5 only 1 connection is allowed ever", SkipAll);
        }
    }
    //### sees to fail sometimes ... a timing issue with the test on windows
    QTcpServer server;
    server.setMaxPendingConnections(2);

    QTcpSocket socket1;
    QTcpSocket socket2;
    QTcpSocket socket3;

    QVERIFY(server.listen());

    socket1.connectToHost(QHostAddress::LocalHost, server.serverPort());
    socket2.connectToHost(QHostAddress::LocalHost, server.serverPort());
    socket3.connectToHost(QHostAddress::LocalHost, server.serverPort());

    QVERIFY(server.waitForNewConnection(5000));

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
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QSKIP("With socks5 we can not make hard requirements on the address or port", SkipAll);
        }
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
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QSKIP("Localhost servers don't work well with SOCKS5", SkipAll);
        }
    }

    QTcpSocket findLocalIpSocket;
    findLocalIpSocket.connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(findLocalIpSocket.waitForConnected(5000));

    QTcpServer server;
    bool timeout = false;
    QVERIFY(server.listen(findLocalIpSocket.localAddress()));
    QVERIFY(!server.waitForNewConnection(1000, &timeout));
    QCOMPARE(server.serverError(), QAbstractSocket::SocketTimeoutError);
    QVERIFY(timeout);

    ThreadConnector connector(findLocalIpSocket.localAddress(), server.serverPort());
    connector.start();

#if defined(Q_OS_WINCE) || defined(Q_OS_SYMBIAN)
    QVERIFY(server.waitForNewConnection(9000, &timeout));
#else
    QVERIFY(server.waitForNewConnection(3000, &timeout));
#endif
    QVERIFY(!timeout);
}

//----------------------------------------------------------------------------------
void tst_QTcpServer::setSocketDescriptor()
{
    QTcpServer server;
#ifdef Q_OS_SYMBIAN
    QTest::ignoreMessage(QtWarningMsg, "QSymbianSocketEngine::initialize - socket descriptor not found");
#endif
    QVERIFY(!server.setSocketDescriptor(42));
    QCOMPARE(server.serverError(), QAbstractSocket::UnsupportedSocketOperationError);
#ifndef Q_OS_SYMBIAN
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
#endif
}

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
    void incomingConnection(int socketDescriptor)
    {
        // how a user woulddo it (qabstractsocketengine is not public)
        unsigned long arg = 0;
#if defined(Q_OS_SYMBIAN)
        arg = fcntl(socketDescriptor, F_GETFL, NULL);
        arg &= (~O_NONBLOCK);
        ok = ::fcntl(socketDescriptor, F_SETFL, arg) != -1;
#elif defined(Q_OS_WIN)
        ok = ::ioctlsocket(socketDescriptor, FIONBIO, &arg) == 0;
        ::closesocket(socketDescriptor);
#else
        ok = ::ioctl(socketDescriptor, FIONBIO, &arg) == 0;
        ::close(socketDescriptor);
#endif
    }
};

void tst_QTcpServer::addressReusable()
{
#if defined(Q_OS_SYMBIAN) && defined(Q_CC_NOKIAX86)
    QSKIP("Symbian: Emulator does not support process launching", SkipAll );
#endif

#if defined(QT_NO_PROCESS)
    QSKIP("Qt was compiled with QT_NO_PROCESS", SkipAll);
#else

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QSKIP("With socks5 this test does not make senans at the momment", SkipAll);
        }
    }
#if defined(Q_OS_WINCE) || defined(Q_OS_SYMBIAN)
    QString signalName = QString::fromLatin1("/test_signal.txt");
    QFile::remove(signalName);
    // The crashingServer process will crash once it gets a connection.
    QProcess process;
    process.start("crashingServer/crashingServer");
    int waitCount = 5;
    while (waitCount-- && !QFile::exists(signalName))
        QTest::qWait(1000);
    QVERIFY(QFile::exists(signalName));
    QFile::remove(signalName);
#else
    // The crashingServer process will crash once it gets a connection.
    QProcess process;
    process.start("crashingServer/crashingServer");
    QVERIFY(process.waitForReadyRead(5000));
#endif

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 49199);
    QVERIFY(socket.waitForConnected(5000));

    QVERIFY(process.waitForFinished(5000));

    // Give the system some time.
    QTest::qSleep(10);

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 49199));
#endif
}

void tst_QTcpServer::setNewSocketDescriptorBlocking()
{
#ifdef Q_OS_SYMBIAN
    QSKIP("open C ioctls on Qt sockets not supported", SkipAll);
#else
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QSKIP("With socks5 we can not make the socket descripter blocking", SkipAll);
        }
    }
    SeverWithBlockingSockets server;
    QVERIFY(server.listen());

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(server.waitForNewConnection(5000));
    QVERIFY(server.ok);
#endif
}

void tst_QTcpServer::invalidProxy_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<int>("expectedError");

    QString fluke = QHostInfo::fromName(QtNetworkSettings::serverName()).addresses().first().toString();
    QTest::newRow("ftp-proxy") << int(QNetworkProxy::FtpCachingProxy) << fluke << 143
                               << int(QAbstractSocket::UnsupportedSocketOperationError);
    QTest::newRow("http-proxy") << int(QNetworkProxy::HttpProxy) << fluke << 3128
                                << int(QAbstractSocket::UnsupportedSocketOperationError);

    QTest::newRow("no-such-host") << int(QNetworkProxy::Socks5Proxy)
                                  << "this-host-will-never-exist.troll.no" << 1080
                                  << int(QAbstractSocket::ProxyNotFoundError);
    QTest::newRow("socks5-on-http") << int(QNetworkProxy::Socks5Proxy) << fluke << 3128
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

    proxyList << QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080);
    QTest::newRow("socks5")
        << proxyList << proxyList.at(0)
        << false << int(QAbstractSocket::UnknownSocketError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::serverName(), 3128)
              << QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080);
    QTest::newRow("cachinghttp+socks5")
        << proxyList << proxyList.at(1)
        << false << int(QAbstractSocket::UnknownSocketError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::FtpCachingProxy, QtNetworkSettings::serverName(), 2121)
              << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::serverName(), 3128)
              << QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080);
    QTest::newRow("ftp+cachinghttp+socks5")
        << proxyList << proxyList.at(2)
        << false << int(QAbstractSocket::UnknownSocketError);

    // tests that fail to listen
    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::HttpProxy, QtNetworkSettings::serverName(), 3128);
    QTest::newRow("http")
        << proxyList << proxyList.at(0)
        << true << int(QAbstractSocket::UnsupportedSocketOperationError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::serverName(), 3128);
    QTest::newRow("cachinghttp")
        << proxyList << QNetworkProxy()
        << true << int(QAbstractSocket::UnsupportedSocketOperationError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::FtpCachingProxy, QtNetworkSettings::serverName(), 2121);
    QTest::newRow("ftp")
        << proxyList << QNetworkProxy()
        << true << int(QAbstractSocket::UnsupportedSocketOperationError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::FtpCachingProxy, QtNetworkSettings::serverName(), 2121)
              << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::serverName(), 3128);
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
    QVERIFY(helper.lastDataPeeked == QByteArray("6162630a"));

    client.write("def\n");
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(helper.lastDataPeeked == QByteArray("6162630a6465660a"));

    client.write("ghi\n");
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(helper.lastDataPeeked == QByteArray("6162630a6465660a6768690a"));
}

QTEST_MAIN(tst_QTcpServer)
#include "tst_qtcpserver.moc"
