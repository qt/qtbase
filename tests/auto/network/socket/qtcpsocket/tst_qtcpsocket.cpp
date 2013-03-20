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

#include <qglobal.h>

// To prevent windows system header files from re-defining min/max
#define NOMINMAX 1
#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#endif

#include <qplatformdefs.h>

#include <QtTest/QtTest>

#include <QAuthenticator>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QHostAddress>
#include <QHostInfo>
#include <QMap>
#include <QPointer>
#include <QProcess>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#ifndef QT_NO_SSL
#include <QSslSocket>
#endif
#include <QTextStream>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QDebug>
// RVCT compiles also unused inline methods
# include <QNetworkProxy>

#ifdef Q_OS_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "private/qhostinfo_p.h"

#include "../../../network-settings.h"

QT_FORWARD_DECLARE_CLASS(QTcpSocket)
class SocketPair;

class tst_QTcpSocket : public QObject
{
    Q_OBJECT

public:
    tst_QTcpSocket();
    virtual ~tst_QTcpSocket();

    static void enterLoop(int secs)
    {
        ++loopLevel;
        QTestEventLoop::instance().enterLoop(secs);
        --loopLevel;
    }
    static void exitLoop()
    {
        // Safe exit - if we aren't in an event loop, don't
        // exit one.
        if (loopLevel > 0)
            QTestEventLoop::instance().exitLoop();
    }
    static bool timeout()
    {
        return QTestEventLoop::instance().timeout();
    }

public slots:
    void initTestCase_data();
    void initTestCase();
    void init();
    void cleanup();
private slots:
    void socketsConstructedBeforeEventLoop();
    void constructing();
    void bind_data();
    void bind();
    void setInvalidSocketDescriptor();
    void setSocketDescriptor();
    void socketDescriptor();
    void blockingIMAP();
    void nonBlockingIMAP();
    void hostNotFound();
    void timeoutConnect_data();
    void timeoutConnect();
    void delayedClose();
    void partialRead();
    void unget();
    void readAllAfterClose();
    void openCloseOpenClose();
    void connectDisconnectConnectDisconnect();
    void disconnectWhileConnecting_data();
    void disconnectWhileConnecting();
    void disconnectWhileConnectingNoEventLoop_data();
    void disconnectWhileConnectingNoEventLoop();
    void disconnectWhileLookingUp_data();
    void disconnectWhileLookingUp();
    void downloadBigFile();
    void readLine();
    void readLineString();
    void readChunks();
    void waitForBytesWritten();
    void waitForBytesWrittenMinusOne();
    void waitForReadyRead();
    void waitForReadyReadMinusOne();
    void flush();
    void synchronousApi();
    void dontCloseOnTimeout();
    void recursiveReadyRead();
    void atEnd();
    void socketInAThread();
    void socketsInThreads();
    void waitForReadyReadInASlot();
    void remoteCloseError();
    void nestedEventLoopInErrorSlot();
#ifndef Q_OS_WIN
    void connectToLocalHostNoService();
#endif
    void waitForConnectedInHostLookupSlot();
    void waitForConnectedInHostLookupSlot2();
    void readyReadSignalsAfterWaitForReadyRead();
#ifdef Q_OS_LINUX
    void linuxKernelBugLocalSocket();
#endif
    void abortiveClose();
    void localAddressEmptyOnBSD();
    void zeroAndMinusOneReturns();
    void connectionRefused();
    void suddenRemoteDisconnect_data();
    void suddenRemoteDisconnect();
    void connectToMultiIP();
    void moveToThread0();
    void increaseReadBufferSize();
    void taskQtBug5799ConnectionErrorWaitForConnected();
    void taskQtBug5799ConnectionErrorEventLoop();
    void taskQtBug7054TimeoutErrorResetting();

    void invalidProxy_data();
    void invalidProxy();
    void proxyFactory_data();
    void proxyFactory();

    void qtbug14268_peek();

    void setSocketOption();


protected slots:
    void nonBlockingIMAP_hostFound();
    void nonBlockingIMAP_connected();
    void nonBlockingIMAP_closed();
    void nonBlockingIMAP_readyRead();
    void nonBlockingIMAP_bytesWritten(qint64);
    void readRegularFile_readyRead();
    void exitLoopSlot();
    void downloadBigFileSlot();
    void recursiveReadyReadSlot();
    void waitForReadyReadInASlotSlot();
    void enterLoopSlot();
    void hostLookupSlot();
    void abortiveClose_abortSlot();
    void remoteCloseErrorSlot();
    void proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth);
    void earlySocketBytesSent(qint64 bytes);
    void earlySocketReadyRead();

private:
    QByteArray expectedReplyIMAP();
    void fetchExpectedReplyIMAP();
    QTcpSocket *newSocket() const;
    QTcpSocket *nonBlockingIMAP_socket;
    QStringList nonBlockingIMAP_data;
    qint64 nonBlockingIMAP_totalWritten;

    QTcpSocket *tmpSocket;
    qint64 bytesAvailable;
    qint64 expectedLength;
    bool readingBody;
    QTime timer;

    QByteArray expectedReplyIMAP_cached;

    mutable int proxyAuthCalled;

    int numConnections;
    static int loopLevel;

    SocketPair *earlyConstructedSockets;
    int earlyBytesWrittenCount;
    int earlyReadyReadCount;
    QString stressTestDir;
};

enum ProxyTests {
    NoProxy = 0x00,
    Socks5Proxy = 0x01,
    HttpProxy = 0x02,
    TypeMask = 0x0f,

    NoAuth = 0x00,
    AuthBasic = 0x10,
    AuthNtlm = 0x20,
    AuthMask = 0xf0
};

int tst_QTcpSocket::loopLevel = 0;

class SocketPair: public QObject
{
    Q_OBJECT
public:
    QTcpSocket *endPoints[2];

    SocketPair(QObject *parent = 0)
        : QObject(parent)
    {
        endPoints[0] = endPoints[1] = 0;
    }

    bool create()
    {
        QTcpServer server;
        server.listen();

        QTcpSocket *active = new QTcpSocket(this);
        active->connectToHost("127.0.0.1", server.serverPort());

        if (!active->waitForConnected(1000))
            return false;

        if (!server.waitForNewConnection(1000))
            return false;

        QTcpSocket *passive = server.nextPendingConnection();
        passive->setParent(this);

        endPoints[0] = active;
        endPoints[1] = passive;
        return true;
    }
};

tst_QTcpSocket::tst_QTcpSocket()
{
    tmpSocket = 0;

    //This code relates to the socketsConstructedBeforeEventLoop test case
    earlyConstructedSockets = new SocketPair;
    QVERIFY(earlyConstructedSockets->create());
    earlyBytesWrittenCount = 0;
    earlyReadyReadCount = 0;
    connect(earlyConstructedSockets->endPoints[0], SIGNAL(readyRead()), this, SLOT(earlySocketReadyRead()));
    connect(earlyConstructedSockets->endPoints[1], SIGNAL(bytesWritten(qint64)), this, SLOT(earlySocketBytesSent(qint64)));
    earlyConstructedSockets->endPoints[1]->write("hello work");
}

tst_QTcpSocket::~tst_QTcpSocket()
{

}

void tst_QTcpSocket::initTestCase_data()
{
    QTest::addColumn<bool>("setProxy");
    QTest::addColumn<int>("proxyType");
    QTest::addColumn<bool>("ssl");

    qDebug() << QtNetworkSettings::serverName();
    QTest::newRow("WithoutProxy") << false << 0 << false;
    QTest::newRow("WithSocks5Proxy") << true << int(Socks5Proxy) << false;
    QTest::newRow("WithSocks5ProxyAuth") << true << int(Socks5Proxy | AuthBasic) << false;

    QTest::newRow("WithHttpProxy") << true << int(HttpProxy) << false;
    QTest::newRow("WithHttpProxyBasicAuth") << true << int(HttpProxy | AuthBasic) << false;
//    QTest::newRow("WithHttpProxyNtlmAuth") << true << int(HttpProxy | AuthNtlm) << false;

#ifndef QT_NO_SSL
    QTest::newRow("WithoutProxy SSL") << false << 0 << true;
    QTest::newRow("WithSocks5Proxy SSL") << true << int(Socks5Proxy) << true;
    QTest::newRow("WithSocks5AuthProxy SSL") << true << int(Socks5Proxy | AuthBasic) << true;

    QTest::newRow("WithHttpProxy SSL") << true << int(HttpProxy) << true;
    QTest::newRow("WithHttpProxyBasicAuth SSL") << true << int(HttpProxy | AuthBasic) << true;
//    QTest::newRow("WithHttpProxyNtlmAuth SSL") << true << int(HttpProxy | AuthNtlm) << true;
#endif

    stressTestDir = QFINDTESTDATA("stressTest");
    QVERIFY2(!stressTestDir.isEmpty(), qPrintable(
        QString::fromLatin1("Couldn't find stressTest dir starting from %1.").arg(QDir::currentPath())));
}

void tst_QTcpSocket::initTestCase()
{
    QVERIFY(QtNetworkSettings::verifyTestNetworkSettings());
}

void tst_QTcpSocket::init()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
        QFETCH_GLOBAL(int, proxyType);
        QList<QHostAddress> addresses = QHostInfo::fromName(QtNetworkSettings::serverName()).addresses();
        QVERIFY2(addresses.count() > 0, "failed to get ip address for test server");
        QString fluke = addresses.first().toString();
        QNetworkProxy proxy;

        switch (proxyType) {
        case Socks5Proxy:
            proxy = QNetworkProxy(QNetworkProxy::Socks5Proxy, fluke, 1080);
            break;

        case Socks5Proxy | AuthBasic:
            proxy = QNetworkProxy(QNetworkProxy::Socks5Proxy, fluke, 1081);
            break;

        case HttpProxy | NoAuth:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, fluke, 3128);
            break;

        case HttpProxy | AuthBasic:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, fluke, 3129);
            break;

        case HttpProxy | AuthNtlm:
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy, fluke, 3130);
            break;
        }
        QNetworkProxy::setApplicationProxy(proxy);
    }

    qt_qhostinfo_clear_cache();
}

QTcpSocket *tst_QTcpSocket::newSocket() const
{
    QTcpSocket *socket;
#ifndef QT_NO_SSL
    QFETCH_GLOBAL(bool, ssl);
    socket = ssl ? new QSslSocket : new QTcpSocket;
#else
    socket = new QTcpSocket;
#endif

    proxyAuthCalled = 0;
    connect(socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            Qt::DirectConnection);
    return socket;
}

void tst_QTcpSocket::cleanup()
{
    QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
}

void tst_QTcpSocket::proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth)
{
    ++proxyAuthCalled;
    auth->setUser("qsockstest");
    auth->setPassword("password");
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::socketsConstructedBeforeEventLoop()
{
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH_GLOBAL(bool, ssl);
    if (setProxy || ssl)
        return;
    //This test checks that sockets constructed before QCoreApplication::exec() still emit signals
    //see construction code in the tst_QTcpSocket constructor
    enterLoop(3);
    QCOMPARE(earlyBytesWrittenCount, 1);
    QCOMPARE(earlyReadyReadCount, 1);
    earlyConstructedSockets->endPoints[0]->close();
    earlyConstructedSockets->endPoints[1]->close();
}

void tst_QTcpSocket::earlySocketBytesSent(qint64 /* bytes */)
{
    earlyBytesWrittenCount++;
}

void tst_QTcpSocket::earlySocketReadyRead()
{
    earlyReadyReadCount++;
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::constructing()
{
    QTcpSocket *socket = newSocket();

    // Check the initial state of the QTcpSocket.
    QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);
    QVERIFY(socket->isSequential());
    QVERIFY(!socket->isOpen());
    QVERIFY(!socket->isValid());
    QCOMPARE(socket->socketType(), QTcpSocket::TcpSocket);

    char c;
    QCOMPARE(socket->getChar(&c), false);
    QCOMPARE((int) socket->bytesAvailable(), 0);
    QCOMPARE(socket->canReadLine(), false);
    QCOMPARE(socket->readLine(), QByteArray());
    QCOMPARE(socket->socketDescriptor(), (qintptr)-1);
    QCOMPARE((int) socket->localPort(), 0);
    QVERIFY(socket->localAddress() == QHostAddress());
    QCOMPARE((int) socket->peerPort(), 0);
    QVERIFY(socket->peerAddress() == QHostAddress());
    QCOMPARE(socket->error(), QTcpSocket::UnknownSocketError);
    QCOMPARE(socket->errorString(), QString("Unknown error"));

    // Check the state of the socket layer?
    delete socket;
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::bind_data()
{
    QTest::addColumn<QString>("stringAddr");
    QTest::addColumn<bool>("successExpected");
    QTest::addColumn<QString>("stringExpectedLocalAddress");

    // iterate all interfaces, add all addresses on them as test data
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    foreach (const QNetworkInterface &interface, interfaces) {
        if (!interface.isValid())
            continue;

        foreach (const QNetworkAddressEntry &entry, interface.addressEntries()) {
            if (entry.ip().isInSubnet(QHostAddress::parseSubnet("fe80::/10"))
                || entry.ip().isInSubnet(QHostAddress::parseSubnet("169.254/16")))
                continue; // link-local bind will fail, at least on Linux, so skip it.

            QString ip(entry.ip().toString());
            QTest::newRow(ip.toLatin1().constData()) << ip << true << ip;
        }
    }

    // additionally, try bind to known-bad addresses, and make sure this doesn't work
    // these ranges are guaranteed to be reserved for 'documentation purposes',
    // and thus, should be unused in the real world. Not that I'm assuming the
    // world is full of competent administrators, or anything.
    QStringList knownBad;
    knownBad << "198.51.100.1";
    knownBad << "2001:0DB8::1";
    foreach (const QString &badAddress, knownBad) {
        QTest::newRow(badAddress.toLatin1().constData()) << badAddress << false << QString();
    }
}

void tst_QTcpSocket::bind()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        QSKIP("QTBUG-22964");
    QFETCH(QString, stringAddr);
    QFETCH(bool, successExpected);
    QFETCH(QString, stringExpectedLocalAddress);

    QHostAddress addr(stringAddr);
    QHostAddress expectedLocalAddress(stringExpectedLocalAddress);

    QTcpSocket *socket = newSocket();
    qDebug() << "Binding " << addr;

    if (successExpected) {
        QVERIFY2(socket->bind(addr), qPrintable(socket->errorString()));
    } else {
        QVERIFY(!socket->bind(addr));
    }

    QCOMPARE(socket->localAddress(), expectedLocalAddress);

    delete socket;
}

//----------------------------------------------------------------------------------


void tst_QTcpSocket::setInvalidSocketDescriptor()
{
    QTcpSocket *socket = newSocket();
    QCOMPARE(socket->socketDescriptor(), (qintptr)-1);
    QVERIFY(!socket->setSocketDescriptor(-5, QTcpSocket::UnconnectedState));
    QCOMPARE(socket->socketDescriptor(), (qintptr)-1);

    QCOMPARE(socket->error(), QTcpSocket::UnsupportedSocketOperationError);

    delete socket;
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::setSocketDescriptor()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;                 // this test doesn't make sense with proxies

#ifdef Q_OS_WIN
    // need the dummy to ensure winsock is started
    QTcpSocket *dummy = newSocket();
    dummy->connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(dummy->waitForConnected());

    SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        qErrnoWarning(WSAGetLastError(), "INVALID_SOCKET");
    }
#else
    SOCKET sock = ::socket(AF_INET, SOCK_STREAM, 0);

    // artificially increase the value of sock
    SOCKET sock2 = ::fcntl(sock, F_DUPFD, sock + 50);
    ::close(sock);
    sock = sock2;
#endif

    QVERIFY(sock != INVALID_SOCKET);
    QTcpSocket *socket = newSocket();
    QVERIFY(socket->setSocketDescriptor(sock, QTcpSocket::UnconnectedState));
    QCOMPARE(socket->socketDescriptor(), (qintptr)sock);

    qt_qhostinfo_clear_cache(); //avoid the HostLookupState being skipped due to address being in cache from previous test.
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    QCOMPARE(socket->state(), QTcpSocket::HostLookupState);
    QCOMPARE(socket->socketDescriptor(), (qintptr)sock);
    QVERIFY(socket->waitForConnected(10000));
    // skip this, it has been broken for years, see task 260735
    // if somebody complains, consider fixing it, but it might break existing applications.
    QEXPECT_FAIL("", "bug has been around for years, will not fix without need", Continue);
    QCOMPARE(socket->socketDescriptor(), (qintptr)sock);
    delete socket;
#ifdef Q_OS_WIN
    delete dummy;
#endif
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::socketDescriptor()
{
    QTcpSocket *socket = newSocket();

    QCOMPARE(socket->socketDescriptor(), (qintptr)-1);
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY((socket->state() == QAbstractSocket::HostLookupState && socket->socketDescriptor() == -1) ||
            (socket->state() == QAbstractSocket::ConnectingState && socket->socketDescriptor() != -1));
    QVERIFY(socket->waitForConnected(10000));
    QVERIFY(socket->state() == QAbstractSocket::ConnectedState);
    QVERIFY(socket->socketDescriptor() != -1);

    delete socket;
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::blockingIMAP()
{
    QTcpSocket *socket = newSocket();

    // Connect
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(socket->waitForConnected(10000));
    QCOMPARE(socket->state(), QTcpSocket::ConnectedState);
    QVERIFY(socket->isValid());

    // Read greeting
    QVERIFY(socket->waitForReadyRead(5000));
    QString s = socket->readLine();
    // only test if an OK was returned, to make the test compatible between different
    // IMAP server versions
    QCOMPARE(s.left(4).toLatin1().constData(), "* OK");

    // Write NOOP
    QCOMPARE((int) socket->write("1 NOOP\r\n", 8), 8);
    QCOMPARE((int) socket->write("2 NOOP\r\n", 8), 8);

    if (!socket->canReadLine())
        QVERIFY(socket->waitForReadyRead(5000));

    // Read response
    s = socket->readLine();
    QCOMPARE(s.toLatin1().constData(), "1 OK Completed\r\n");

    // Write a third NOOP to verify that write doesn't clear the read buffer
    QCOMPARE((int) socket->write("3 NOOP\r\n", 8), 8);

    // Read second response
    if (!socket->canReadLine())
        QVERIFY(socket->waitForReadyRead(5000));
    s = socket->readLine();
    QCOMPARE(s.toLatin1().constData(), "2 OK Completed\r\n");

    // Read third response
    if (!socket->canReadLine())
        QVERIFY(socket->waitForReadyRead(5000));
    s = socket->readLine();
    QCOMPARE(s.toLatin1().constData(), "3 OK Completed\r\n");


    // Write LOGOUT
    QCOMPARE((int) socket->write("4 LOGOUT\r\n", 10), 10);

    if (!socket->canReadLine())
        QVERIFY(socket->waitForReadyRead(5000));

    // Read two lines of respose
    s = socket->readLine();
    QCOMPARE(s.toLatin1().constData(), "* BYE LOGOUT received\r\n");

    if (!socket->canReadLine())
        QVERIFY(socket->waitForReadyRead(5000));

    s = socket->readLine();
    QCOMPARE(s.toLatin1().constData(), "4 OK Completed\r\n");

    // Close the socket
    socket->close();

    // Check that it's closed
    QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);

    delete socket;
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::hostNotFound()
{
    QTcpSocket *socket = newSocket();

    socket->connectToHost("nosuchserver.qt-project.org", 80);
    QVERIFY(!socket->waitForConnected());
    QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);
    QCOMPARE(int(socket->error()), int(QTcpSocket::HostNotFoundError));

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::timeoutConnect_data()
{
    QTest::addColumn<QString>("address");
    QTest::newRow("host") << QtNetworkSettings::serverName();
    QTest::newRow("ip") << QtNetworkSettings::serverIP().toString();
}

void tst_QTcpSocket::timeoutConnect()
{
    QFETCH(QString, address);
    QTcpSocket *socket = newSocket();

    QElapsedTimer timer;
    timer.start();

    // Port 1357 is configured to drop packets on the test server
    socket->connectToHost(address, 1357);
    QVERIFY(timer.elapsed() < 150);
    QVERIFY(!socket->waitForConnected(1000)); //200ms is too short when using SOCKS proxy authentication
    QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);
    QCOMPARE(int(socket->error()), int(QTcpSocket::SocketTimeoutError));

    timer.start();
    socket->connectToHost(address, 1357);
    QVERIFY(timer.elapsed() < 150);
    QTimer::singleShot(50, &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(socket->state() == QTcpSocket::ConnectingState
            || socket->state() == QTcpSocket::HostLookupState);
    socket->abort();
    QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);
    QCOMPARE(socket->openMode(), QIODevice::NotOpen);

    delete socket;
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::nonBlockingIMAP()
{
    QTcpSocket *socket = newSocket();
    connect(socket, SIGNAL(hostFound()), SLOT(nonBlockingIMAP_hostFound()));
    connect(socket, SIGNAL(connected()), SLOT(nonBlockingIMAP_connected()));
    connect(socket, SIGNAL(disconnected()), SLOT(nonBlockingIMAP_closed()));
    connect(socket, SIGNAL(bytesWritten(qint64)), SLOT(nonBlockingIMAP_bytesWritten(qint64)));
    connect(socket, SIGNAL(readyRead()), SLOT(nonBlockingIMAP_readyRead()));
    nonBlockingIMAP_socket = socket;

    // Connect
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(socket->state() == QTcpSocket::HostLookupState ||
            socket->state() == QTcpSocket::ConnectingState);

    enterLoop(30);
    if (timeout()) {
        QFAIL("Timed out");
    }

    if (socket->state() == QTcpSocket::ConnectingState) {
        enterLoop(30);
        if (timeout()) {
            QFAIL("Timed out");
        }
    }

    QCOMPARE(socket->state(), QTcpSocket::ConnectedState);

    enterLoop(30);
    if (timeout()) {
        QFAIL("Timed out");
    }

    // Read greeting
    QVERIFY(!nonBlockingIMAP_data.isEmpty());
    QCOMPARE(nonBlockingIMAP_data.at(0).left(4).toLatin1().constData(), "* OK");
    nonBlockingIMAP_data.clear();

    nonBlockingIMAP_totalWritten = 0;

    // Write NOOP
    QCOMPARE((int) socket->write("1 NOOP\r\n", 8), 8);


    enterLoop(30);
    if (timeout()) {
        QFAIL("Timed out");
    }

    QVERIFY(nonBlockingIMAP_totalWritten == 8);


    enterLoop(30);
    if (timeout()) {
        QFAIL("Timed out");
    }


    // Read response
    QVERIFY(!nonBlockingIMAP_data.isEmpty());
    QCOMPARE(nonBlockingIMAP_data.at(0).toLatin1().constData(), "1 OK Completed\r\n");
    nonBlockingIMAP_data.clear();


    nonBlockingIMAP_totalWritten = 0;

    // Write LOGOUT
    QCOMPARE((int) socket->write("2 LOGOUT\r\n", 10), 10);

    enterLoop(30);
    if (timeout()) {
        QFAIL("Timed out");
    }

    QVERIFY(nonBlockingIMAP_totalWritten == 10);

    // Wait for greeting
    enterLoop(30);
    if (timeout()) {
        QFAIL("Timed out");
    }

    // Read two lines of respose
    QCOMPARE(nonBlockingIMAP_data.at(0).toLatin1().constData(), "* BYE LOGOUT received\r\n");
    QCOMPARE(nonBlockingIMAP_data.at(1).toLatin1().constData(), "2 OK Completed\r\n");
    nonBlockingIMAP_data.clear();

    // Close the socket
    socket->close();

    // Check that it's closed
    QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);

    delete socket;
}

void tst_QTcpSocket::nonBlockingIMAP_hostFound()
{
    exitLoop();
}

void tst_QTcpSocket::nonBlockingIMAP_connected()
{
    exitLoop();
}

void tst_QTcpSocket::nonBlockingIMAP_readyRead()
{
    while (nonBlockingIMAP_socket->canReadLine())
        nonBlockingIMAP_data.append(nonBlockingIMAP_socket->readLine());

    exitLoop();
}

void tst_QTcpSocket::nonBlockingIMAP_bytesWritten(qint64 written)
{
    nonBlockingIMAP_totalWritten += written;
    exitLoop();
}

void tst_QTcpSocket::nonBlockingIMAP_closed()
{
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::delayedClose()
{
    QTcpSocket *socket = newSocket();
    connect(socket, SIGNAL(connected()), SLOT(nonBlockingIMAP_connected()));
    connect(socket, SIGNAL(disconnected()), SLOT(exitLoopSlot()));

    socket->connectToHost(QtNetworkSettings::serverName(), 143);

    enterLoop(30);
    if (timeout())
        QFAIL("Timed out");

    QCOMPARE(socket->state(), QTcpSocket::ConnectedState);

    QCOMPARE((int) socket->write("1 LOGOUT\r\n", 10), 10);

    // Add a huge bulk of data to be written after the logout
    // command. The server will shut down after receiving the LOGOUT,
    // so this data will not be read. But our close call should
    // schedule a delayed close because all the data can not be
    // written in one go.
    QCOMPARE((int) socket->write(QByteArray(100000, '\n'), 100000), 100000);

    socket->close();

    QCOMPARE((int) socket->state(), (int) QTcpSocket::ClosingState);

    enterLoop(10);
    if (timeout())
        QFAIL("Timed out");

    QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);

    delete socket;
}


//----------------------------------------------------------------------------------

QByteArray tst_QTcpSocket::expectedReplyIMAP()
{
    if (expectedReplyIMAP_cached.isEmpty()) {
        fetchExpectedReplyIMAP();
    }

    return expectedReplyIMAP_cached;
}

// Figure out how the current IMAP server responds
void tst_QTcpSocket::fetchExpectedReplyIMAP()
{
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY2(socket->waitForConnected(10000), qPrintable(socket->errorString()));
    QVERIFY2(socket->state() == QTcpSocket::ConnectedState, qPrintable(socket->errorString()));

    QTRY_VERIFY(socket->canReadLine());

    QByteArray greeting = socket->readLine();
    delete socket;

    QVERIFY2(QtNetworkSettings::compareReplyIMAP(greeting), greeting.constData());

    expectedReplyIMAP_cached = greeting;
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::partialRead()
{
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(socket->waitForConnected(10000));
    QVERIFY(socket->state() == QTcpSocket::ConnectedState);
    char buf[512];

    QByteArray greeting = expectedReplyIMAP();
    QVERIFY(!greeting.isEmpty());

    for (int i = 0; i < 10; i += 2) {
        while (socket->bytesAvailable() < 2)
            QVERIFY(socket->waitForReadyRead(5000));
        QVERIFY(socket->read(buf, 2) == 2);
        buf[2] = '\0';
        QCOMPARE((char *)buf, greeting.mid(i, 2).data());
    }

    delete socket;
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::unget()
{
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(socket->waitForConnected(10000));
    QVERIFY(socket->state() == QTcpSocket::ConnectedState);
    char buf[512];

    QByteArray greeting = expectedReplyIMAP();
    QVERIFY(!greeting.isEmpty());

    for (int i = 0; i < 10; i += 2) {
        while (socket->bytesAvailable() < 2)
            QVERIFY(socket->waitForReadyRead(10000));
        int bA = socket->bytesAvailable();
        QVERIFY(socket->read(buf, 2) == 2);
        buf[2] = '\0';
        QCOMPARE((char *)buf, greeting.mid(i, 2).data());
        QCOMPARE((int)socket->bytesAvailable(), bA - 2);
        socket->ungetChar(buf[1]);
        socket->ungetChar(buf[0]);
        QCOMPARE((int)socket->bytesAvailable(), bA);
        QVERIFY(socket->read(buf, 2) == 2);
        buf[2] = '\0';
        QCOMPARE((char *)buf, greeting.mid(i, 2).data());
    }

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::readRegularFile_readyRead()
{
    exitLoop();
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::readAllAfterClose()
{
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    connect(socket, SIGNAL(readyRead()), SLOT(readRegularFile_readyRead()));
    enterLoop(10);
    if (timeout())
        QFAIL("Network operation timed out");

    socket->close();
    QByteArray array = socket->readAll();
    QCOMPARE(array.size(), 0);

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::openCloseOpenClose()
{
    QTcpSocket *socket = newSocket();

    for (int i = 0; i < 3; ++i) {
        QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);
        QCOMPARE(int(socket->openMode()), int(QIODevice::NotOpen));
        QVERIFY(socket->isSequential());
        QVERIFY(!socket->isOpen());
        QVERIFY(socket->socketType() == QTcpSocket::TcpSocket);

        char c;
        QCOMPARE(socket->getChar(&c), false);
        QCOMPARE((int) socket->bytesAvailable(), 0);
        QCOMPARE(socket->canReadLine(), false);
        QCOMPARE(socket->readLine(), QByteArray());
        QCOMPARE(socket->socketDescriptor(), (qintptr)-1);
        QCOMPARE((int) socket->localPort(), 0);
        QVERIFY(socket->localAddress() == QHostAddress());
        QCOMPARE((int) socket->peerPort(), 0);
        QVERIFY(socket->peerAddress() == QHostAddress());
        QCOMPARE(socket->error(), QTcpSocket::UnknownSocketError);
        QCOMPARE(socket->errorString(), QString("Unknown error"));

        QVERIFY(socket->state() == QTcpSocket::UnconnectedState);

        socket->connectToHost(QtNetworkSettings::serverName(), 143);
        QVERIFY(socket->waitForConnected(10000));
        socket->close();
    }

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::connectDisconnectConnectDisconnect()
{
    QTcpSocket *socket = newSocket();

    for (int i = 0; i < 3; ++i) {
        QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);
        QVERIFY(socket->socketType() == QTcpSocket::TcpSocket);

        socket->connectToHost(QtNetworkSettings::serverName(), 143);
        QVERIFY(socket->waitForReadyRead(10000));
        QCOMPARE(QString::fromLatin1(socket->read(4)), QString("* OK"));

        socket->disconnectFromHost();
        if (socket->state() != QTcpSocket::UnconnectedState)
            QVERIFY(socket->waitForDisconnected(10000));
        QCOMPARE(int(socket->openMode()), int(QIODevice::ReadWrite));
    }

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::disconnectWhileConnecting_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<bool>("closeDirectly");

    QTest::newRow("without-data") << QByteArray() << false;
    QTest::newRow("without-data+close") << QByteArray() << true;
    QTest::newRow("with-data") << QByteArray("Hello, world!") << false;
    QTest::newRow("with-data+close") << QByteArray("Hello, world!") << true;

    QByteArray bigData(1024*1024, '@');
    QTest::newRow("with-big-data") << bigData << false;
    QTest::newRow("with-big-data+close") << bigData << true;
}

void tst_QTcpSocket::disconnectWhileConnecting()
{
    QFETCH(QByteArray, data);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return; //proxy not useful for localhost test case

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    // proceed to the connect-write-disconnect
    QTcpSocket *socket = newSocket();
    socket->connectToHost("127.0.0.1", server.serverPort());
    if (!data.isEmpty())
        socket->write(data);
    if (socket->state() == QAbstractSocket::ConnectedState)
        QSKIP("localhost connections are immediate, test case is invalid");

    QFETCH(bool, closeDirectly);
    if (closeDirectly) {
        socket->close();
        QCOMPARE(int(socket->openMode()), int(QIODevice::NotOpen));
    } else {
        socket->disconnectFromHost();
    }

    connect(socket, SIGNAL(disconnected()), SLOT(exitLoopSlot()));
    enterLoop(10);
    QVERIFY2(!timeout(), "Network timeout");
    QVERIFY(socket->state() == QAbstractSocket::UnconnectedState);
    if (!closeDirectly) {
        QCOMPARE(int(socket->openMode()), int(QIODevice::ReadWrite));
        socket->close();
    }
    QCOMPARE(int(socket->openMode()), int(QIODevice::NotOpen));

    // accept the other side and verify that it was sent properly:
    QVERIFY(server.hasPendingConnections() || server.waitForNewConnection(0));
    QTcpSocket *othersocket = server.nextPendingConnection();
    if (othersocket->state() != QAbstractSocket::UnconnectedState)
        QVERIFY2(othersocket->waitForDisconnected(10000), "Network timeout");
    QVERIFY(othersocket->state() == QAbstractSocket::UnconnectedState);
    QCOMPARE(othersocket->readAll(), data);

    delete socket;
    delete othersocket;
}

//----------------------------------------------------------------------------------
class ReceiverThread: public QThread
{
    QTcpServer *server;
public:
    int serverPort;
    bool ok;
    QByteArray receivedData;
    volatile bool quit;

    ReceiverThread()
        : server(0), ok(false), quit(false)
    { }

    ~ReceiverThread() { }

    bool listen()
    {
        server = new QTcpServer;
        if (!server->listen(QHostAddress::LocalHost))
            return false;
        serverPort = server->serverPort();
        server->moveToThread(this);
        return true;
    }

    static void cleanup(void *ptr)
    {
        ReceiverThread* self = reinterpret_cast<ReceiverThread*>(ptr);
        self->quit = true;
        self->wait(30000);
        delete self;
    }

protected:
    void run()
    {
        bool timedOut = false;
        while (!quit) {
            if (server->waitForNewConnection(500, &timedOut))
                break;
            if (!timedOut)
                return;
        }

        QTcpSocket *socket = server->nextPendingConnection();
        while (!quit) {
            if (socket->waitForDisconnected(500))
                break;
            if (socket->error() != QAbstractSocket::SocketTimeoutError)
                return;
        }

        if (!quit) {
            receivedData = socket->readAll();
            ok = true;
        }
        delete socket;
        delete server;
        server = 0;
    }
};

void tst_QTcpSocket::disconnectWhileConnectingNoEventLoop_data()
{
    disconnectWhileConnecting_data();
}

void tst_QTcpSocket::disconnectWhileConnectingNoEventLoop()
{
    QFETCH(QByteArray, data);
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return; //proxy not useful for localhost test case

    QScopedPointer<ReceiverThread, ReceiverThread> thread (new ReceiverThread);
    QVERIFY(thread->listen());
    thread->start();

    // proceed to the connect-write-disconnect
    QTcpSocket *socket = newSocket();
    socket->connectToHost("127.0.0.1", thread->serverPort);
    if (!data.isEmpty())
        socket->write(data);
    if (socket->state() == QAbstractSocket::ConnectedState)
        QSKIP("localhost connections are immediate, test case is invalid");

    QFETCH(bool, closeDirectly);
    if (closeDirectly) {
        socket->close();
        QCOMPARE(int(socket->openMode()), int(QIODevice::NotOpen));
    } else {
        socket->disconnectFromHost();
    }

    QVERIFY2(socket->waitForDisconnected(10000), "Network timeout");
    QVERIFY(socket->state() == QAbstractSocket::UnconnectedState);
    if (!closeDirectly) {
        QCOMPARE(int(socket->openMode()), int(QIODevice::ReadWrite));
        socket->close();
    }
    QCOMPARE(int(socket->openMode()), int(QIODevice::NotOpen));
    delete socket;

    // check if the other side received everything ok
    QVERIFY(thread->wait(30000));
    QVERIFY(thread->ok);
    QCOMPARE(thread->receivedData, data);
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::disconnectWhileLookingUp_data()
{
    QTest::addColumn<bool>("doClose");

    QTest::newRow("disconnect") << false;
    QTest::newRow("close") << true;
}

void tst_QTcpSocket::disconnectWhileLookingUp()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;                 // we let the proxies do the lookup now

    // just connect and disconnect, then make sure nothing weird happened
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 21);

    // check that connect is in progress
    QVERIFY(socket->state() != QAbstractSocket::UnconnectedState);

    QFETCH(bool, doClose);
    if (doClose) {
        socket->close();
        QVERIFY(socket->openMode() == QIODevice::NotOpen);
    } else {
        socket->disconnectFromHost();
        QVERIFY(socket->openMode() == QIODevice::ReadWrite);
        QVERIFY(socket->waitForDisconnected(5000));
    }

    // let anything queued happen
    QEventLoop loop;
    QTimer::singleShot(50, &loop, SLOT(quit()));
    loop.exec();

    // recheck
    if (doClose) {
        QVERIFY(socket->openMode() == QIODevice::NotOpen);
    } else {
        QVERIFY(socket->openMode() == QIODevice::ReadWrite);
    }

    QVERIFY(socket->state() == QAbstractSocket::UnconnectedState);
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::downloadBigFile()
{
    if (tmpSocket)
        delete tmpSocket;
    tmpSocket = newSocket();

    connect(tmpSocket, SIGNAL(connected()), SLOT(exitLoopSlot()));
    connect(tmpSocket, SIGNAL(readyRead()), SLOT(downloadBigFileSlot()));
    connect(tmpSocket, SIGNAL(disconnected()), SLOT(exitLoopSlot()));

    tmpSocket->connectToHost(QtNetworkSettings::serverName(), 80);

    enterLoop(30);
    if (timeout()) {
        delete tmpSocket;
        tmpSocket = 0;
        QFAIL("Network operation timed out");
    }

    QByteArray hostName = QtNetworkSettings::serverName().toLatin1();
    QVERIFY(tmpSocket->state() == QAbstractSocket::ConnectedState);
    QVERIFY(tmpSocket->write("GET /qtest/mediumfile HTTP/1.0\r\n") > 0);
    QVERIFY(tmpSocket->write("HOST: ") > 0);
    QVERIFY(tmpSocket->write(hostName.data()) > 0);
    QVERIFY(tmpSocket->write("\r\n") > 0);
    QVERIFY(tmpSocket->write("\r\n") > 0);

    bytesAvailable = 0;
    expectedLength = 0;
    readingBody = false;

    QTime stopWatch;
    stopWatch.start();

    enterLoop(600);
    if (timeout()) {
        delete tmpSocket;
        tmpSocket = 0;
        if (bytesAvailable > 0)
            qDebug("Slow Connection, only downloaded %ld of %d", long(bytesAvailable), 10000281);
        QFAIL("Network operation timed out");
    }

    QCOMPARE(bytesAvailable, expectedLength);

    qDebug("\t\t%.1fMB/%.1fs: %.1fMB/s",
           bytesAvailable / (1024.0 * 1024.0),
           stopWatch.elapsed() / 1024.0,
           (bytesAvailable / (stopWatch.elapsed() / 1000.0)) / (1024 * 1024));

    delete tmpSocket;
    tmpSocket = 0;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::exitLoopSlot()
{
    exitLoop();
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::downloadBigFileSlot()
{
    if (!readingBody) {
        while (tmpSocket->canReadLine()) {
            QByteArray array = tmpSocket->readLine();
            if (array.startsWith("Content-Length"))
                expectedLength = array.simplified().split(' ').at(1).toInt();
            if (array == "\r\n") {
                readingBody = true;
                break;
            }
        }
    }
    if (readingBody) {
        bytesAvailable += tmpSocket->readAll().size();
        if (bytesAvailable == expectedLength)
            exitLoop();
    }
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::readLine()
{
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(socket->waitForConnected(5000));

    while (!socket->canReadLine())
        QVERIFY(socket->waitForReadyRead(10000));

    char buffer[1024];

    qint64 linelen = socket->readLine(buffer, sizeof(buffer));
    QVERIFY(linelen >= 3);
    QVERIFY(linelen < 1024);

    QByteArray reply = QByteArray::fromRawData(buffer, linelen);
    QCOMPARE((int) buffer[linelen-2], (int) '\r');
    QCOMPARE((int) buffer[linelen-1], (int) '\n');
    QCOMPARE((int) buffer[linelen],   (int) '\0');

    QVERIFY2(QtNetworkSettings::compareReplyIMAP(reply), reply.constData());

    QCOMPARE(socket->write("1 NOOP\r\n"), qint64(8));

    while (socket->bytesAvailable() < 10)
        QVERIFY(socket->waitForReadyRead(10000));

    QCOMPARE(socket->readLine(buffer, 11), qint64(10));
    QCOMPARE((const char *)buffer, "1 OK Compl");

    while (socket->bytesAvailable() < 6)
        QVERIFY(socket->waitForReadyRead(10000));

    QCOMPARE(socket->readLine(buffer, 11), qint64(6));
    QCOMPARE((const char *)buffer, "eted\r\n");

    QVERIFY(!socket->waitForReadyRead(100));
    QCOMPARE(socket->readLine(buffer, sizeof(buffer)), qint64(0));
    QVERIFY(socket->error() == QAbstractSocket::SocketTimeoutError
            || socket->error() == QAbstractSocket::RemoteHostClosedError);
    QCOMPARE(socket->bytesAvailable(), qint64(0));

    socket->close();
    QCOMPARE(socket->readLine(buffer, sizeof(buffer)), qint64(-1));

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::readLineString()
{
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(socket->waitForReadyRead(10000));

    QByteArray arr = socket->readLine();
    QVERIFY2(QtNetworkSettings::compareReplyIMAP(arr), arr.constData());

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::readChunks()
{
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(socket->waitForConnected(10000));
    QVERIFY(socket->waitForReadyRead(5000));

    char buf[4096];
    memset(buf, '@', sizeof(buf));
    qint64 dataLength = socket->read(buf, sizeof(buf));
    QVERIFY(dataLength > 0);

    QCOMPARE(buf[dataLength - 2], '\r');
    QCOMPARE(buf[dataLength - 1], '\n');
    QCOMPARE(buf[dataLength], '@');

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::waitForBytesWritten()
{
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 80);
    QVERIFY(socket->waitForConnected(10000));

    socket->write("GET / HTTP/1.0\r\n\r\n");
    qint64 toWrite = socket->bytesToWrite();
    QVERIFY(socket->waitForBytesWritten(5000));
    QVERIFY(toWrite > socket->bytesToWrite());

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::waitForBytesWrittenMinusOne()
{
#ifdef Q_OS_WIN
    QSKIP("QTBUG-24451 - indefinite wait may hang");
#endif
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 80);
    QVERIFY(socket->waitForConnected(10000));

    socket->write("GET / HTTP/1.0\r\n\r\n");
    qint64 toWrite = socket->bytesToWrite();
    QVERIFY(socket->waitForBytesWritten(-1));
    QVERIFY(toWrite > socket->bytesToWrite());

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::waitForReadyRead()
{
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 80);
    socket->write("GET / HTTP/1.0\r\n\r\n");
    QVERIFY(socket->waitForReadyRead(5000));
    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::waitForReadyReadMinusOne()
{
#ifdef Q_OS_WIN
    QSKIP("QTBUG-24451 - indefinite wait may hang");
#endif
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 80);
    socket->write("GET / HTTP/1.0\r\n\r\n");
    QVERIFY(socket->waitForReadyRead(-1));
    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::flush()
{
    QTcpSocket *socket = newSocket();
    socket->flush();

    connect(socket, SIGNAL(connected()), SLOT(exitLoopSlot()));
    socket->connectToHost(QtNetworkSettings::serverName(), 143);
    enterLoop(60);
    QVERIFY(socket->isOpen());

    socket->write("1 LOGOUT\r\n");
    QCOMPARE(socket->bytesToWrite(), qint64(10));
    socket->flush();
    QCOMPARE(socket->bytesToWrite(), qint64(0));
    socket->close();

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::synchronousApi()
{
    QTcpSocket *ftpSocket = newSocket();
    ftpSocket->connectToHost(QtNetworkSettings::serverName(), 21);
    ftpSocket->write("QUIT\r\n");
    QVERIFY(ftpSocket->waitForDisconnected(10000));
    QVERIFY(ftpSocket->bytesAvailable() > 0);
    QByteArray arr = ftpSocket->readAll();
    QVERIFY(arr.size() > 0);
    delete ftpSocket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::dontCloseOnTimeout()
{
    QTcpServer server;
    server.setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    QVERIFY(server.listen());

    QHostAddress serverAddress = QHostAddress::LocalHost;
    if (!(server.serverAddress() == QHostAddress::AnyIPv4)
        && !(server.serverAddress() == QHostAddress::AnyIPv6)
        && !(server.serverAddress() == QHostAddress::Any))
        serverAddress = server.serverAddress();

    QTcpSocket *socket = newSocket();
    socket->connectToHost(serverAddress, server.serverPort());
    QVERIFY(!socket->waitForReadyRead(100));
    QCOMPARE(socket->error(), QTcpSocket::SocketTimeoutError);
    QVERIFY(socket->isOpen());

    QVERIFY(!socket->waitForDisconnected(100));
    QCOMPARE(socket->error(), QTcpSocket::SocketTimeoutError);
    QVERIFY(socket->isOpen());

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::recursiveReadyRead()
{
    QTcpSocket *testSocket = newSocket();
    connect(testSocket, SIGNAL(connected()), SLOT(exitLoopSlot()));
    connect(testSocket, SIGNAL(readyRead()), SLOT(recursiveReadyReadSlot()));
    tmpSocket = testSocket;

    QSignalSpy spy(testSocket, SIGNAL(readyRead()));

    testSocket->connectToHost(QtNetworkSettings::serverName(), 25);
    enterLoop(30);
    QVERIFY2(!timeout(),
            "Timed out when connecting to QtNetworkSettings::serverName().");

    enterLoop(30);
    QVERIFY2(!timeout(),
            "Timed out when waiting for the readyRead() signal.");

    QCOMPARE(spy.count(), 1);

    delete testSocket;
}

void tst_QTcpSocket::recursiveReadyReadSlot()
{
    // make sure the server spits out more data
    tmpSocket->write("NOOP\r\n");
    tmpSocket->flush();

    // indiscriminately enter the event loop and start processing
    // events again. but oops! future socket notifications will cause
    // undesired recursive behavior. Unless QTcpSocket is smart, which
    // it of course is. :-)
    QEventLoop loop;
    for (int i = 0; i < 100; ++i)
        loop.processEvents();

    // all we really wanted to do was process some events, then exit
    // the loop
    exitLoop();
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::atEnd()
{
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 21);

    QVERIFY(socket->waitForReadyRead(15000));
    QTextStream stream(socket);
    QVERIFY(!stream.atEnd());
    QString greeting = stream.readLine();
    QVERIFY(stream.atEnd());

    // Test server must use some vsFTPd 2.x.x version
    QVERIFY2(greeting.length() == sizeof("220 (vsFTPd 2.x.x)")-1, qPrintable(greeting));
    QVERIFY2(greeting.startsWith("220 (vsFTPd 2."), qPrintable(greeting));
    QVERIFY2(greeting.endsWith(")"), qPrintable(greeting));

    delete socket;
}

class TestThread : public QThread
{
    Q_OBJECT

public:
    inline QByteArray data() const
    {
        return socketData;
    }

protected:
    inline void run()
    {
#ifndef QT_NO_SSL
        QFETCH_GLOBAL(bool, ssl);
        if (ssl)
            socket = new QSslSocket;
        else
#endif
        socket = new QTcpSocket;
        connect(socket, SIGNAL(readyRead()), this, SLOT(getData()), Qt::DirectConnection);
        connect(socket, SIGNAL(disconnected()), this, SLOT(closed()), Qt::DirectConnection);
        connect(socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), Qt::DirectConnection);

        socket->connectToHost(QtNetworkSettings::serverName(), 21);
        socket->write("QUIT\r\n");
        exec();

        delete socket;
    }

private slots:
    inline void getData()
    {
        socketData += socket->readAll();
    }

    inline void closed()
    {
        quit();
    }
    inline void proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth)
    {
        auth->setUser("qsockstest");
        auth->setPassword("password");
    }
private:
    int exitCode;
    QTcpSocket *socket;
    QByteArray socketData;
};

//----------------------------------------------------------------------------------
void tst_QTcpSocket::socketInAThread()
{
    for (int i = 0; i < 3; ++i) {
        TestThread thread;
        thread.start();
        QVERIFY(thread.wait(15000));
        QByteArray data = thread.data();
        QVERIFY2(QtNetworkSettings::compareReplyFtp(data), data.constData());
    }
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::socketsInThreads()
{
    for (int i = 0; i < 3; ++i) {
        TestThread thread1;
        TestThread thread2;
        TestThread thread3;

        thread1.start();
        thread2.start();
        thread3.start();

        QVERIFY(thread2.wait(15000));
        QVERIFY(thread3.wait(15000));
        QVERIFY(thread1.wait(15000));

        QByteArray data1 = thread1.data();
        QByteArray data2 = thread2.data();
        QByteArray data3 = thread3.data();

        QVERIFY2(QtNetworkSettings::compareReplyFtp(data1), data1.constData());
        QVERIFY2(QtNetworkSettings::compareReplyFtp(data2), data2.constData());
        QVERIFY2(QtNetworkSettings::compareReplyFtp(data3), data3.constData());
    }
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::waitForReadyReadInASlot()
{
    QTcpSocket *socket = newSocket();
    tmpSocket = socket;
    connect(socket, SIGNAL(connected()), this, SLOT(waitForReadyReadInASlotSlot()));

    socket->connectToHost(QtNetworkSettings::serverName(), 80);
    socket->write("GET / HTTP/1.0\r\n\r\n");

    enterLoop(30);
    QVERIFY(!timeout());

    delete socket;
}

void tst_QTcpSocket::waitForReadyReadInASlotSlot()
{
    QVERIFY(tmpSocket->waitForReadyRead(10000));
    exitLoop();
}

class RemoteCloseErrorServer : public QTcpServer
{
    Q_OBJECT
public:
    RemoteCloseErrorServer()
    {
        connect(this, SIGNAL(newConnection()),
                this, SLOT(getConnection()));
    }

private slots:
    void getConnection()
    {
        tst_QTcpSocket::exitLoop();
    }
};

//----------------------------------------------------------------------------------
void tst_QTcpSocket::remoteCloseError()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return; //proxy not useful for localhost test case
    RemoteCloseErrorServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    QCoreApplication::instance()->processEvents();

    QTcpSocket *clientSocket = newSocket();
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(exitLoopSlot()));

    clientSocket->connectToHost(server.serverAddress(), server.serverPort());

    enterLoop(30);
    QVERIFY(!timeout());

    QVERIFY(server.hasPendingConnections());
    QTcpSocket *serverSocket = server.nextPendingConnection();
    connect(clientSocket, SIGNAL(disconnected()), this, SLOT(exitLoopSlot()));

    serverSocket->write("Hello");

    enterLoop(30);
    QVERIFY(!timeout());

    QCOMPARE(clientSocket->bytesAvailable(), qint64(5));

    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    QSignalSpy errorSpy(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)));
    QSignalSpy disconnectedSpy(clientSocket, SIGNAL(disconnected()));

    clientSocket->write("World");
    serverSocket->disconnectFromHost();

    tmpSocket = clientSocket;
    connect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(remoteCloseErrorSlot()));

    enterLoop(30);
    QVERIFY(!timeout());

    QCOMPARE(disconnectedSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(clientSocket->error(), QAbstractSocket::RemoteHostClosedError);

    delete serverSocket;

    clientSocket->connectToHost(server.serverAddress(), server.serverPort());

    enterLoop(30);
    QVERIFY(!timeout());

    QVERIFY(server.hasPendingConnections());
    serverSocket = server.nextPendingConnection();
    serverSocket->disconnectFromHost();

    enterLoop(30);
    QVERIFY(!timeout());

    QCOMPARE(clientSocket->state(), QAbstractSocket::UnconnectedState);

    delete clientSocket;
}

void tst_QTcpSocket::remoteCloseErrorSlot()
{
    QCOMPARE(tmpSocket->state(), QAbstractSocket::ConnectedState);
    static_cast<QTcpSocket *>(sender())->close();
}

void tst_QTcpSocket::enterLoopSlot()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    socket->deleteLater();

    // enter nested event loop
    QEventLoop loop;
    QTimer::singleShot(100, &loop, SLOT(quit()));
    loop.exec();

    // Fire a non-0 singleshot to leave time for the delete
    QTimer::singleShot(250, this, SLOT(exitLoopSlot()));
}
//----------------------------------------------------------------------------------
void tst_QTcpSocket::nestedEventLoopInErrorSlot()
{
    QTcpSocket *socket = newSocket();
    QPointer<QTcpSocket> p(socket);
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(enterLoopSlot()));

    socket->connectToHost("hostnotfoundhostnotfound.qt-project.org", 9999);
    enterLoop(30);
    QVERIFY(!p);
}

//----------------------------------------------------------------------------------
#ifndef Q_OS_WIN
void tst_QTcpSocket::connectToLocalHostNoService()
{
    // This test was created after we received a report that claimed
    // QTcpSocket would crash if trying to connect to "localhost" on a random
    // port with no service listening.
    QTcpSocket *socket = newSocket();
    socket->connectToHost("localhost", 31415); // no service running here, one suspects

    while(socket->state() == QTcpSocket::HostLookupState || socket->state() == QTcpSocket::ConnectingState) {
        QTest::qWait(100);
    }
    QCOMPARE(socket->state(), QTcpSocket::UnconnectedState);
    delete socket;
}
#endif

//----------------------------------------------------------------------------------
void tst_QTcpSocket::waitForConnectedInHostLookupSlot()
{
    // This test tries to reproduce the problem where waitForConnected() is
    // called at a point where the host lookup is already done. QTcpSocket
    // will try to abort the "pending lookup", but since it's already done and
    // the queued signal is already underway, we will receive the signal after
    // waitForConnected() has returned, and control goes back to the event
    // loop. When the signal has been received, the connection is torn down,
    // then reopened. Yikes. If we reproduce this by calling
    // waitForConnected() inside hostLookupSlot(), it will even crash.
    tmpSocket = newSocket();
    QEventLoop loop;
    connect(tmpSocket, SIGNAL(connected()), &loop, SLOT(quit()));
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    QSignalSpy timerSpy(&timer, SIGNAL(timeout()));
    timer.start(15000);

    connect(tmpSocket, SIGNAL(hostFound()), this, SLOT(hostLookupSlot()));
    tmpSocket->connectToHost(QtNetworkSettings::serverName(), 143);

    // only execute the loop if not already connected
    if (tmpSocket->state() != QAbstractSocket::ConnectedState)
        loop.exec();

    QCOMPARE(timerSpy.count(), 0);

    delete tmpSocket;
}

void tst_QTcpSocket::hostLookupSlot()
{
    // This will fail to cancel the pending signal
    QVERIFY(tmpSocket->waitForConnected(10000));
}

class Foo : public QObject
{
    Q_OBJECT
    QTcpSocket *sock;
public:
    bool attemptedToConnect;
    bool networkTimeout;
    int count;

    inline Foo(QObject *parent = 0) : QObject(parent)
    {
        attemptedToConnect = false;
        networkTimeout = false;
        count = 0;
#ifndef QT_NO_SSL
        QFETCH_GLOBAL(bool, ssl);
        if (ssl)
            sock = new QSslSocket;
        else
#endif
        sock = new QTcpSocket;
        connect(sock, SIGNAL(connected()), this, SLOT(connectedToIt()));
        connect(sock, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    }

    inline ~Foo()
    {
        delete sock;
    }

public slots:
    inline void connectedToIt()
    { count++; }

    inline void doIt()
    {
        attemptedToConnect = true;
        sock->connectToHost(QtNetworkSettings::serverName(), 80);

#ifdef Q_OS_MAC
        pthread_yield_np();
#elif defined Q_OS_LINUX
        pthread_yield();
#endif
        if (!sock->waitForConnected()) {
            networkTimeout = true;
        }
        tst_QTcpSocket::exitLoop();
    }

    inline void exitLoop()
    {
        tst_QTcpSocket::exitLoop();
    }

    inline void proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth)
    {
        auth->setUser("qsockstest");
        auth->setPassword("password");
    }
};

//----------------------------------------------------------------------------------
void tst_QTcpSocket::waitForConnectedInHostLookupSlot2()
{
    Foo foo;

    QTimer::singleShot(100, &foo, SLOT(doIt()));
    QTimer::singleShot(5000, &foo, SLOT(exitLoop()));

    enterLoop(30);
    if (timeout() || foo.networkTimeout)
        QFAIL("Network timeout");

    QVERIFY(foo.attemptedToConnect);
    QCOMPARE(foo.count, 1);
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::readyReadSignalsAfterWaitForReadyRead()
{
    QTcpSocket *socket = newSocket();

    QSignalSpy readyReadSpy(socket, SIGNAL(readyRead()));

    // Connect
    socket->connectToHost(QtNetworkSettings::serverName(), 143);

    // Wait for the read
    QVERIFY(socket->waitForReadyRead(10000));

    QCOMPARE(readyReadSpy.count(), 1);

    QString s = socket->readLine();
    QVERIFY2(QtNetworkSettings::compareReplyIMAP(s.toLatin1()), s.toLatin1().constData());
    QCOMPARE(socket->bytesAvailable(), qint64(0));

    QCoreApplication::instance()->processEvents();
    QCOMPARE(socket->bytesAvailable(), qint64(0));
    QCOMPARE(readyReadSpy.count(), 1);

    delete socket;
}

class TestThread2 : public QThread
{
    Q_OBJECT
public:
    void run()
    {
        QFile fileWriter("fifo");
        QVERIFY(fileWriter.open(QFile::WriteOnly));
        QCOMPARE(fileWriter.write(QByteArray(32, '@')), qint64(32));
        QCOMPARE(fileWriter.write(QByteArray(32, '@')), qint64(32));
        QCOMPARE(fileWriter.write(QByteArray(32, '@')), qint64(32));
        QCOMPARE(fileWriter.write(QByteArray(32, '@')), qint64(32));
    }
};

//----------------------------------------------------------------------------------
#ifdef Q_OS_LINUX
void tst_QTcpSocket::linuxKernelBugLocalSocket()
{
    QFile::remove("fifo");
    mkfifo("fifo", 0666);

    TestThread2 test;
    test.start();

    QFile fileReader("fifo");
    QVERIFY(fileReader.open(QFile::ReadOnly));

    test.wait();

    QTcpSocket *socket = newSocket();
    socket->setSocketDescriptor(fileReader.handle());
    QVERIFY(socket->waitForReadyRead(5000));
    QCOMPARE(socket->bytesAvailable(), qint64(128));

    QFile::remove("fifo");

    delete socket;
}
#endif

//----------------------------------------------------------------------------------
void tst_QTcpSocket::abortiveClose()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return; //proxy not useful for localhost test case
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    connect(&server, SIGNAL(newConnection()), this, SLOT(exitLoopSlot()));

    QTcpSocket *clientSocket = newSocket();
    clientSocket->connectToHost(server.serverAddress(), server.serverPort());

    enterLoop(10);
    QVERIFY(server.hasPendingConnections());

    QVERIFY(tmpSocket = server.nextPendingConnection());

    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    QSignalSpy readyReadSpy(clientSocket, SIGNAL(readyRead()));
    QSignalSpy errorSpy(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)));

    connect(clientSocket, SIGNAL(disconnected()), this, SLOT(exitLoopSlot()));
    QTimer::singleShot(0, this, SLOT(abortiveClose_abortSlot()));

    enterLoop(5);

    QCOMPARE(readyReadSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 1);

    QCOMPARE(*static_cast<const int *>(errorSpy.at(0).at(0).constData()),
             int(QAbstractSocket::RemoteHostClosedError));

    delete clientSocket;
}

void tst_QTcpSocket::abortiveClose_abortSlot()
{
    tmpSocket->abort();
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::localAddressEmptyOnBSD()
{
#ifdef Q_OS_WIN
    QSKIP("QTBUG-24451 - indefinite wait may hang");
#endif
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return; //proxy not useful for localhost test case
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    QTcpSocket *tcpSocket = 0;
    // we try 10 times, but note that this doesn't always provoke the bug
    for (int i = 0; i < 10; ++i) {
        delete tcpSocket;
        tcpSocket = newSocket();
        tcpSocket->connectToHost(QHostAddress::LocalHost, server.serverPort());
        if (!tcpSocket->waitForConnected(0)) {
            // to provoke the bug, we need a local socket that connects immediately
            // --i;
            tcpSocket->abort();
            if (tcpSocket->state() != QTcpSocket::UnconnectedState)
                QVERIFY(tcpSocket->waitForDisconnected(-1));
            continue;
        }
        QCOMPARE(tcpSocket->localAddress(), QHostAddress(QHostAddress::LocalHost));
    }
    delete tcpSocket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::zeroAndMinusOneReturns()
{
    QTcpSocket *socket = newSocket();
    socket->connectToHost(QtNetworkSettings::serverName(), 80);
    socket->write("GET / HTTP/1.0\r\nConnection: keep-alive\r\n\r\n");
    QVERIFY(socket->waitForReadyRead(15000));

    char c[16];
    QVERIFY(socket->getChar(c));
    QCOMPARE(socket->read(c, 16), qint64(16));
    QVERIFY(socket->readLine(c, 16) > 0);
    QVERIFY(!socket->readAll().isEmpty());

    // the last operation emptied the read buffer
    // all read operations from this point on should fail
    // with return 0 because the socket is still open
    QVERIFY(socket->readAll().isEmpty());
    QCOMPARE(socket->read(c, 16), qint64(0));
    QCOMPARE(socket->readLine(c, 16), qint64(0));
    QVERIFY(!socket->getChar(c));

    socket->write("GET / HTTP/1.0\r\n\r\n");
    QVERIFY(socket->waitForDisconnected(15000));
    QCOMPARE(socket->error(), QAbstractSocket::RemoteHostClosedError);

    QCOMPARE(socket->write("BLUBBER"), qint64(-1));
    QVERIFY(socket->getChar(c));
    QCOMPARE(socket->read(c, 16), qint64(16));
    QVERIFY(socket->readLine(c, 16) > 0);
    QVERIFY(!socket->readAll().isEmpty());

    // the last operation emptied the read buffer
    // all read operations from this point on should fail
    // with return -1 because the socket is not connected
    QVERIFY(socket->readAll().isEmpty());
    QCOMPARE(socket->read(c, 16), qint64(-1));
    QCOMPARE(socket->readLine(c, 16), qint64(-1));
    QVERIFY(!socket->getChar(c));
    QVERIFY(!socket->putChar('a'));

    socket->close();

    // now the QIODevice is closed, which means getChar complains
    QCOMPARE(socket->write("BLUBBER"), qint64(-1));
    QCOMPARE(socket->read(c, 16), qint64(-1));
    QCOMPARE(socket->readLine(c, 16), qint64(-1));
    QVERIFY(!socket->getChar(c));
    QVERIFY(!socket->putChar('a'));

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::connectionRefused()
{
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");

    QTcpSocket *socket = newSocket();
    QSignalSpy stateSpy(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)));
    QSignalSpy errorSpy(socket, SIGNAL(error(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));

    socket->connectToHost(QtNetworkSettings::serverName(), 144);

    enterLoop(10);
    disconnect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
               &QTestEventLoop::instance(), SLOT(exitLoop()));
    QVERIFY2(!timeout(), "Network timeout");

    QCOMPARE(socket->state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(socket->error(), QAbstractSocket::ConnectionRefusedError);

    QCOMPARE(stateSpy.count(), 3);
    QCOMPARE(qvariant_cast<QAbstractSocket::SocketState>(stateSpy.at(0).at(0)), QAbstractSocket::HostLookupState);
    QCOMPARE(qvariant_cast<QAbstractSocket::SocketState>(stateSpy.at(1).at(0)), QAbstractSocket::ConnectingState);
    QCOMPARE(qvariant_cast<QAbstractSocket::SocketState>(stateSpy.at(2).at(0)), QAbstractSocket::UnconnectedState);
    QCOMPARE(errorSpy.count(), 1);

    delete socket;
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::suddenRemoteDisconnect_data()
{
    QTest::addColumn<QString>("client");
    QTest::addColumn<QString>("server");

    QTest::newRow("Qt4 Client <-> Qt4 Server") << QString::fromLatin1("qt4client") << QString::fromLatin1("qt4server");
}

void tst_QTcpSocket::suddenRemoteDisconnect()
{
    QFETCH(QString, client);
    QFETCH(QString, server);

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;
    QFETCH_GLOBAL(bool, ssl);
    if (ssl)
        return;

    QString processExe = stressTestDir + "/stressTest";

    // Start server
    QProcess serverProcess;
    serverProcess.setReadChannel(QProcess::StandardError);
    serverProcess.start(processExe, QStringList(server), QIODevice::ReadWrite | QIODevice::Text);
    QVERIFY2(serverProcess.waitForStarted(), qPrintable(
        QString::fromLatin1("Could not start %1: %2").arg(processExe, serverProcess.errorString())));
    while (!serverProcess.canReadLine())
        QVERIFY(serverProcess.waitForReadyRead(10000));
    QCOMPARE(serverProcess.readLine().data(), QByteArray(server.toLatin1() + "\n").data());

    // Start client
    QProcess clientProcess;
    clientProcess.setReadChannel(QProcess::StandardError);
    clientProcess.start(processExe, QStringList(client), QIODevice::ReadWrite | QIODevice::Text);
    QVERIFY2(clientProcess.waitForStarted(), qPrintable(
        QString::fromLatin1("Could not start %1: %2").arg(processExe, clientProcess.errorString())));
    while (!clientProcess.canReadLine())
        QVERIFY(clientProcess.waitForReadyRead(10000));
    QCOMPARE(clientProcess.readLine().data(), QByteArray(client.toLatin1() + "\n").data());

    // Let them play for a while
    qDebug("Running stress test for 5 seconds");
    QEventLoop loop;
    connect(&serverProcess, SIGNAL(finished(int)), &loop, SLOT(quit()));
    connect(&clientProcess, SIGNAL(finished(int)), &loop, SLOT(quit()));
    QTime stopWatch;
    stopWatch.start();
    QTimer::singleShot(20000, &loop, SLOT(quit()));

    while ((serverProcess.state() == QProcess::Running
           || clientProcess.state() == QProcess::Running) && stopWatch.elapsed() < 20000)
        loop.exec();

    QVERIFY(stopWatch.elapsed() < 20000);

    // Check that both exited normally.
#if defined(UBUNTU_ONEIRIC) && defined(__x86_64__)
    QEXPECT_FAIL("", "Fails on this platform", Abort);
#endif
    QCOMPARE(clientProcess.readAll().constData(), "SUCCESS\n");
    QCOMPARE(serverProcess.readAll().constData(), "SUCCESS\n");
}

//----------------------------------------------------------------------------------

void tst_QTcpSocket::connectToMultiIP()
{
    QSKIP("TODO: setup DNS in the new network");
#if defined(Q_OS_VXWORKS)
    QSKIP("VxSim in standard config doesn't even run a DNS resolver");
#else
    QFETCH_GLOBAL(bool, ssl);
    if (ssl)
        return;
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        QSKIP("This test takes too long if we also add the proxies.");

    qDebug("Please wait, this test can take a while...");

    QTcpSocket *socket = newSocket();
    // rationale: this domain resolves to 3 A-records, 2 of them are
    // invalid. QTcpSocket should never spend more than 30 seconds per IP, and
    // 30s*2 = 60s.
    QTime stopWatch;
    stopWatch.start();
    socket->connectToHost("multi.dev.qt-project.org", 80);
    QVERIFY(socket->waitForConnected(60500));
    QVERIFY(stopWatch.elapsed() < 70000);
    socket->abort();

    stopWatch.restart();
    socket->connectToHost("multi.dev.qt-project.org", 81);
    QVERIFY(!socket->waitForConnected(2000));
    QVERIFY(stopWatch.elapsed() < 2000);
    QCOMPARE(socket->error(), QAbstractSocket::SocketTimeoutError);

    delete socket;
#endif
}

//----------------------------------------------------------------------------------
void tst_QTcpSocket::moveToThread0()
{
    QFETCH_GLOBAL(int, proxyType);
    if (proxyType & AuthMask)
        return;

    {
        // Case 1: Moved after connecting, before waiting for connection.
        QTcpSocket *socket = newSocket();;
        socket->connectToHost(QtNetworkSettings::serverName(), 143);
        socket->moveToThread(0);
        QVERIFY(socket->waitForConnected(5000));
        socket->write("XXX LOGOUT\r\n");
        QVERIFY(socket->waitForBytesWritten(5000));
        QVERIFY(socket->waitForDisconnected());
        delete socket;
    }
    {
        // Case 2: Moved before connecting
        QTcpSocket *socket = newSocket();
        socket->moveToThread(0);
        socket->connectToHost(QtNetworkSettings::serverName(), 143);
        QVERIFY(socket->waitForConnected(5000));
        socket->write("XXX LOGOUT\r\n");
        QVERIFY(socket->waitForBytesWritten(5000));
        QVERIFY(socket->waitForDisconnected());
        delete socket;
    }
    {
        // Case 3: Moved after writing, while waiting for bytes to be written.
        QTcpSocket *socket = newSocket();
        socket->connectToHost(QtNetworkSettings::serverName(), 143);
        QVERIFY(socket->waitForConnected(5000));
        socket->write("XXX LOGOUT\r\n");
        socket->moveToThread(0);
        QVERIFY(socket->waitForBytesWritten(5000));
        QVERIFY(socket->waitForDisconnected());
        delete socket;
    }
    {
        // Case 4: Moved after writing, while waiting for response.
        QTcpSocket *socket = newSocket();
        socket->connectToHost(QtNetworkSettings::serverName(), 143);
        QVERIFY(socket->waitForConnected(5000));
        socket->write("XXX LOGOUT\r\n");
        QVERIFY(socket->waitForBytesWritten(5000));
        socket->moveToThread(0);
        QVERIFY(socket->waitForDisconnected());
        delete socket;
    }
}

void tst_QTcpSocket::increaseReadBufferSize()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return; //proxy not useful for localhost test case
    QTcpServer server;
    QTcpSocket *active = newSocket();
    connect(active, SIGNAL(readyRead()), SLOT(exitLoopSlot()));

    // connect two sockets to each other:
    QVERIFY(server.listen(QHostAddress::LocalHost));
    active->connectToHost("127.0.0.1", server.serverPort());
    QVERIFY(active->waitForConnected(5000));
    QVERIFY(server.waitForNewConnection(5000));

    QTcpSocket *passive = server.nextPendingConnection();
    QVERIFY(passive);

    // now write 512 bytes of data on one end
    QByteArray data(512, 'a');
    passive->write(data);
    QVERIFY2(passive->waitForBytesWritten(5000), "Network timeout");

    // set the read buffer size to less than what was written and iterate:
    active->setReadBufferSize(256);
    enterLoop(10);
    QVERIFY2(!timeout(), "Network timeout");
    QCOMPARE(active->bytesAvailable(), active->readBufferSize());

    // increase the buffer size and iterate again:
    active->setReadBufferSize(384);
    enterLoop(10);
    QVERIFY2(!timeout(), "Network timeout");
    QCOMPARE(active->bytesAvailable(), active->readBufferSize());

    // once more, but now it should read everything there was to read
    active->setReadBufferSize(1024);
    enterLoop(10);
    QVERIFY2(!timeout(), "Network timeout");
    QCOMPARE(active->bytesAvailable(), qint64(data.size()));

    // drain it and compare
    QCOMPARE(active->readAll(), data);

    // now one more test by setting the buffer size to unlimited:
    passive->write(data);
    QVERIFY2(passive->waitForBytesWritten(5000), "Network timeout");
    active->setReadBufferSize(256);
    enterLoop(10);
    QVERIFY2(!timeout(), "Network timeout");
    QCOMPARE(active->bytesAvailable(), active->readBufferSize());
    active->setReadBufferSize(0);
    enterLoop(10);
    QVERIFY2(!timeout(), "Network timeout");
    QCOMPARE(active->bytesAvailable(), qint64(data.size()));
    QCOMPARE(active->readAll(), data);

    delete active;
}

void tst_QTcpSocket::taskQtBug5799ConnectionErrorWaitForConnected()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    // check that we get a proper error connecting to port 12346
    // use waitForConnected, e.g. this should use a synchronous select() on the OS level

    QTcpSocket socket;
    socket.connectToHost(QtNetworkSettings::serverName(), 12346);
    QTime timer;
    timer.start();
    socket.waitForConnected(10000);
    QVERIFY2(timer.elapsed() < 9900, "Connection to closed port timed out instead of refusing, something is wrong");
    QVERIFY2(socket.state() == QAbstractSocket::UnconnectedState, "Socket connected unexpectedly!");
    QVERIFY2(socket.error() == QAbstractSocket::ConnectionRefusedError,
             QString("Could not reach server: %1").arg(socket.errorString()).toLocal8Bit());
}

void tst_QTcpSocket::taskQtBug5799ConnectionErrorEventLoop()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    // check that we get a proper error connecting to port 12346
    // This testcase uses an event loop
    QTcpSocket socket;
    connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), &QTestEventLoop::instance(), SLOT(exitLoop()));
    socket.connectToHost(QtNetworkSettings::serverName(), 12346);

    QTestEventLoop::instance().enterLoop(10);
    QVERIFY2(!QTestEventLoop::instance().timeout(), "Connection to closed port timed out instead of refusing, something is wrong");
    QVERIFY2(socket.state() == QAbstractSocket::UnconnectedState, "Socket connected unexpectedly!");
    QVERIFY2(socket.error() == QAbstractSocket::ConnectionRefusedError,
             QString("Could not reach server: %1").arg(socket.errorString()).toLocal8Bit());
}

void tst_QTcpSocket::taskQtBug7054TimeoutErrorResetting()
{
    QTcpSocket *socket = newSocket();

    socket->connectToHost(QtNetworkSettings::serverName(), 443);
    QVERIFY(socket->waitForConnected(5*1000));
    QVERIFY(socket->error() == QAbstractSocket::UnknownSocketError);

    // We connected to the HTTPS port. Wait two seconds to receive data. We will receive
    // nothing because we would need to start the SSL handshake
    QVERIFY(!socket->waitForReadyRead(2*1000));
    QVERIFY(socket->error() == QAbstractSocket::SocketTimeoutError);

    // Now write some crap to make the server disconnect us. 4 lines are enough.
    socket->write("a\r\nb\r\nc\r\nd\r\n");
    socket->waitForBytesWritten(2*1000);

    // we try to waitForReadyRead another time, but this time instead of a timeout we
    // should get a better error since the server disconnected us
    QVERIFY(!socket->waitForReadyRead(2*1000));
    // It must NOT be the SocketTimeoutError that had been set before
    QVERIFY(socket->error() == QAbstractSocket::RemoteHostClosedError);
}

void tst_QTcpSocket::invalidProxy_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<bool>("failsAtConnect");
    QTest::addColumn<int>("expectedError");

    QString fluke = QHostInfo::fromName(QtNetworkSettings::serverName()).addresses().first().toString();
    QTest::newRow("ftp-proxy") << int(QNetworkProxy::FtpCachingProxy) << fluke << 21 << true
                               << int(QAbstractSocket::UnsupportedSocketOperationError);
    QTest::newRow("http-caching-proxy") << int(QNetworkProxy::HttpCachingProxy) << fluke << 3128 << true
                                        << int(QAbstractSocket::UnsupportedSocketOperationError);
    QTest::newRow("no-such-host-socks5") << int(QNetworkProxy::Socks5Proxy)
                                         << "this-host-will-never-exist.qt-project.org" << 1080 << false
                                         << int(QAbstractSocket::ProxyNotFoundError);
    QTest::newRow("no-such-host-http") << int(QNetworkProxy::HttpProxy)
                                       << "this-host-will-never-exist.qt-project.org" << 3128 << false
                                       << int(QAbstractSocket::ProxyNotFoundError);
    QTest::newRow("http-on-socks5") << int(QNetworkProxy::HttpProxy) << fluke << 1080 << false
                                    << int(QAbstractSocket::ProxyConnectionClosedError);
    QTest::newRow("socks5-on-http") << int(QNetworkProxy::Socks5Proxy) << fluke << 3128 << false
                                    << int(QAbstractSocket::SocketTimeoutError);
}

void tst_QTcpSocket::invalidProxy()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QFETCH(int, type);
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(bool, failsAtConnect);
    QNetworkProxy::ProxyType proxyType = QNetworkProxy::ProxyType(type);
    QNetworkProxy proxy(proxyType, host, port);

    QTcpSocket *socket = newSocket();
    socket->setProxy(proxy);
    socket->connectToHost(QHostInfo::fromName(QtNetworkSettings::serverName()).addresses().first().toString(), 80);

    if (failsAtConnect) {
        QCOMPARE(socket->state(), QAbstractSocket::UnconnectedState);
    } else {
        QCOMPARE(socket->state(), QAbstractSocket::ConnectingState);
        QVERIFY(!socket->waitForConnected(5000));
    }
    QVERIFY(!socket->errorString().isEmpty());

    // note: the following test is not a hard failure.
    // Sometimes, error codes change for the better
    QTEST(int(socket->error()), "expectedError");

    delete socket;
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

void tst_QTcpSocket::proxyFactory_data()
{
    QTest::addColumn<QList<QNetworkProxy> >("proxyList");
    QTest::addColumn<QNetworkProxy>("proxyUsed");
    QTest::addColumn<bool>("failsAtConnect");
    QTest::addColumn<int>("expectedError");

    QList<QNetworkProxy> proxyList;

    // tests that do connect

    proxyList << QNetworkProxy(QNetworkProxy::HttpProxy, QtNetworkSettings::serverName(), 3129);
    QTest::newRow("http")
        << proxyList << proxyList.at(0)
        << false << int(QAbstractSocket::UnknownSocketError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1081);
    QTest::newRow("socks5")
        << proxyList << proxyList.at(0)
        << false << int(QAbstractSocket::UnknownSocketError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::serverName(), 3129)
              << QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1081);
    QTest::newRow("cachinghttp+socks5")
        << proxyList << proxyList.at(1)
        << false << int(QAbstractSocket::UnknownSocketError);

    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::FtpCachingProxy, QtNetworkSettings::serverName(), 2121)
              << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::serverName(), 3129)
              << QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1081);
    QTest::newRow("ftp+cachinghttp+socks5")
        << proxyList << proxyList.at(2)
        << false << int(QAbstractSocket::UnknownSocketError);

    // tests that fail to connect
    proxyList.clear();
    proxyList << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::serverName(), 3129);
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
              << QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::serverName(), 3129);
    QTest::newRow("ftp+cachinghttp")
        << proxyList << QNetworkProxy()
        << true << int(QAbstractSocket::UnsupportedSocketOperationError);
}

void tst_QTcpSocket::proxyFactory()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QFETCH(QList<QNetworkProxy>, proxyList);
    QFETCH(QNetworkProxy, proxyUsed);
    QFETCH(bool, failsAtConnect);

    MyProxyFactory *factory = new MyProxyFactory;
    factory->toReturn = proxyList;
    QNetworkProxyFactory::setApplicationProxyFactory(factory);

    QTcpSocket *socket = newSocket();
    QString host = QtNetworkSettings::serverName();
    socket->connectToHost(host, 80);

    // Verify that the factory was called properly
    QCOMPARE(factory->callCount, 1);
    QCOMPARE(factory->lastQuery, QNetworkProxyQuery(host, 80));

    if (failsAtConnect) {
        QCOMPARE(socket->state(), QAbstractSocket::UnconnectedState);
    } else {
        QCOMPARE(socket->state(), QAbstractSocket::ConnectingState);
        QVERIFY(socket->waitForConnected(5000));
        QCOMPARE(proxyAuthCalled, 1);
    }
    QVERIFY(!socket->errorString().isEmpty());

    // note: the following test is not a hard failure.
    // Sometimes, error codes change for the better
    QTEST(int(socket->error()), "expectedError");

    delete socket;
}

// there is a similar test inside tst_qtcpserver that uses the event loop instead
void tst_QTcpSocket::qtbug14268_peek()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SocketPair socketPair;
    QVERIFY(socketPair.create());
    QTcpSocket *outgoing = socketPair.endPoints[0];
    QTcpSocket *incoming = socketPair.endPoints[1];

    QVERIFY(incoming->state() == QTcpSocket::ConnectedState);
    QVERIFY(outgoing->state() == QTcpSocket::ConnectedState);

    outgoing->write("abc\n");
    QVERIFY(outgoing->waitForBytesWritten(2000));
    QVERIFY(incoming->waitForReadyRead(2000));
    QVERIFY(incoming->peek(128*1024) == QByteArray("abc\n"));

    outgoing->write("def\n");
    QVERIFY(outgoing->waitForBytesWritten(2000));
    QVERIFY(incoming->waitForReadyRead(2000));
    QVERIFY(incoming->peek(128*1024) == QByteArray("abc\ndef\n"));

    outgoing->write("ghi\n");
    QVERIFY(outgoing->waitForBytesWritten(2000));
    QVERIFY(incoming->waitForReadyRead(2000));
    QVERIFY(incoming->peek(128*1024) == QByteArray("abc\ndef\nghi\n"));

    QVERIFY(incoming->read(128*1024) == QByteArray("abc\ndef\nghi\n"));
}

void tst_QTcpSocket::setSocketOption()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    SocketPair socketPair;
    QVERIFY(socketPair.create());
    QTcpSocket *outgoing = socketPair.endPoints[0];
    QTcpSocket *incoming = socketPair.endPoints[1];

    QVERIFY(incoming->state() == QTcpSocket::ConnectedState);
    QVERIFY(outgoing->state() == QTcpSocket::ConnectedState);

    outgoing->setSocketOption(QAbstractSocket::LowDelayOption, true);
    QVariant v = outgoing->socketOption(QAbstractSocket::LowDelayOption);
    QVERIFY(v.isValid() && v.toBool());
    outgoing->setSocketOption(QAbstractSocket::KeepAliveOption, true);
    v = outgoing->socketOption(QAbstractSocket::KeepAliveOption);
    QVERIFY(v.isValid() && v.toBool());

    outgoing->setSocketOption(QAbstractSocket::LowDelayOption, false);
    v = outgoing->socketOption(QAbstractSocket::LowDelayOption);
    QVERIFY(v.isValid() && !v.toBool());
    outgoing->setSocketOption(QAbstractSocket::KeepAliveOption, false);
    v = outgoing->socketOption(QAbstractSocket::KeepAliveOption);
    QVERIFY(v.isValid() && !v.toBool());

#ifdef Q_OS_WIN
    QEXPECT_FAIL("", "QTBUG-23323", Abort);
#endif
    outgoing->setSocketOption(QAbstractSocket::TypeOfServiceOption, 32); //high priority
    v = outgoing->socketOption(QAbstractSocket::TypeOfServiceOption);
    QVERIFY(v.isValid() && v.toInt() == 32);
}

QTEST_MAIN(tst_QTcpSocket)
#include "tst_qtcpsocket.moc"
