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

#include <QtTest/QtTest>
#include <QtCore/QThread>
#include <QtCore/QSemaphore>
#include <QtCore/QElapsedTimer>
#include <QtCore/QSharedPointer>
#include <QtCore/QVector>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

#ifdef QT_BUILD_INTERNAL
# include <private/qnetworkaccessmanager_p.h>
#endif

#include "minihttpserver.h"
#include "../../auto/network-settings.h"

#include <qplatformdefs.h>
#ifdef Q_OS_UNIX
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/select.h>
# include <netinet/in.h>
# include <errno.h>
# include <netdb.h>
# include <signal.h>
# include <unistd.h>
# include <fcntl.h>

typedef int SOCKET;
# define INVALID_SOCKET -1
# define SOCKET_ERROR -1

#elif defined(Q_OS_WIN)
# include <winsock2.h>
#endif

class tst_NetworkStressTest : public QObject
{
    Q_OBJECT
public:
    enum { AttemptCount = 100 };
    tst_NetworkStressTest();
    MiniHttpServer server;

    qint64 byteCounter;
    QNetworkAccessManager manager;
    bool intermediateDebug;

private:
    void clearManager();

public slots:
    void initTestCase_data();
    void initTestCase();
    void init();

    void slotReadAll() { byteCounter += static_cast<QIODevice *>(sender())->readAll().size(); }

private Q_SLOTS:
    void nativeBlockingConnectDisconnect();
    void nativeNonBlockingConnectDisconnect();
    void blockingConnectDisconnect();
    void blockingPipelined();
    void blockingMultipleRequests();
    void connectDisconnect();
    void parallelConnectDisconnect_data();
    void parallelConnectDisconnect();
    void namGet_data();
    void namGet();
};

tst_NetworkStressTest::tst_NetworkStressTest()
    : intermediateDebug(qgetenv("STRESSDEBUG").toInt() > 0)
{
#ifdef Q_OS_WIN
    WSAData wsadata;

    // IPv6 requires Winsock v2.0 or better.
    WSAStartup(MAKEWORD(2,0), &wsadata);
#elif defined(Q_OS_UNIX)
    ::signal(SIGALRM, SIG_IGN);
#endif
}

void tst_NetworkStressTest::initTestCase_data()
{
    QTest::addColumn<bool>("isLocalhost");
    QTest::addColumn<QString>("hostname");
    QTest::addColumn<int>("port");

    QTest::newRow("localhost") << true << "localhost" << server.port();
    QTest::newRow("remote") << false << QtNetworkSettings::serverName() << 80;
}

void tst_NetworkStressTest::initTestCase()
{
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
}

void tst_NetworkStressTest::init()
{
    // clear the internal cache
#ifndef QT_BUILD_INTERNAL
    if (strncmp(QTest::currentTestFunction(), "nam", 3) == 0)
        QSKIP("QNetworkAccessManager tests disabled");
#endif
}

void tst_NetworkStressTest::clearManager()
{
#ifdef QT_BUILD_INTERNAL
    QNetworkAccessManagerPrivate::clearAuthenticationCache(&manager);
    QNetworkAccessManagerPrivate::clearConnectionCache(&manager);
    manager.setProxy(QNetworkProxy());
    manager.setCache(0);
#endif
}

bool nativeLookup(const char *hostname, int port, QByteArray &buf)
{
#if 0
    addrinfo *res = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;

    int result = getaddrinfo(QUrl::toAce(hostname).constData(), QByteArray::number(port).constData(), &hints, &res);
    if (!result)
        return false;
    for (addrinfo *node = res; node; node = node->ai_next) {
        if (node->ai_family == AF_INET) {
            buf = QByteArray((char *)node->ai_addr, node->ai_addrlen);
            break;
        }
    }
    freeaddrinfo(res);
#else
    hostent *result = gethostbyname(hostname);
    if (!result || result->h_addrtype != AF_INET)
        return false;

    struct sockaddr_in s;
    s.sin_family = AF_INET;
    s.sin_port = htons(port);
    s.sin_addr =  *(struct in_addr *) result->h_addr_list[0];

    buf = QByteArray((char *)&s, sizeof s);
#endif

    return !buf.isEmpty();
}

bool nativeSelect(int fd, int timeout, bool selectForWrite)
{
    if (timeout < 0)
        return false;

    // wait for connected
    fd_set fds, fde;
    FD_ZERO(&fds);
    FD_ZERO(&fde);
    FD_SET(fd, &fds);
    FD_SET(fd, &fde);

    int ret;
    do {
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = timeout % 1000;
        if (selectForWrite)
            ret = ::select(fd + 1, 0, &fds, &fde, &tv);
        else
            ret = ::select(fd + 1, &fds, 0, &fde, &tv);
    } while (ret == -1 && errno == EINTR);
    return ret != 0;
}

void tst_NetworkStressTest::nativeBlockingConnectDisconnect()
{
    QFETCH_GLOBAL(QString, hostname);
    QFETCH_GLOBAL(int, port);

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

    for (int i = 0; i < AttemptCount; ++i) {
        QElapsedTimer timeout;
        byteCounter = 0;
        timeout.start();

#if defined(Q_OS_UNIX)
        alarm(10);
#endif

        // look up the host
        QByteArray addr;
        if (!nativeLookup(QUrl::toAce(hostname).constData(), port, addr))
            QFAIL("Lookup failed");

        // connect
        SOCKET fd = ::socket(AF_INET, SOCK_STREAM, 0);
        QVERIFY(fd != INVALID_SOCKET);
        QVERIFY(::connect(fd, (sockaddr *)addr.data(), addr.size()) != SOCKET_ERROR);

        // send request
        {
            QByteArray request = "GET /qtest/bigfile HTTP/1.1\r\n"
                                 "Connection: close\r\n"
                                 "User-Agent: tst_QTcpSocket_stresstest/1.0\r\n"
                                 "Host: " + hostname.toLatin1() + "\r\n"
                                 "\r\n";
            qint64 bytesWritten = 0;
            while (bytesWritten < request.size()) {
                qint64 ret = ::send(fd, request.constData() + bytesWritten, request.size() - bytesWritten, 0);
                if (ret == -1) {
                    ::close(fd);
                    QFAIL("Timeout");
                }
                bytesWritten += ret;
            }
        }

        // receive reply
        char buf[16384];
        while (true) {
            qint64 ret = ::recv(fd, buf, sizeof buf, 0);
            if (ret == -1) {
                ::close(fd);
                QFAIL("Timeout");
            } else if (ret == 0) {
                break; // EOF
            }
            byteCounter += ret;
        }
        ::close(fd);

        totalBytes += byteCounter;
        if (intermediateDebug) {
            double rate = (byteCounter * 1.0 / timeout.elapsed());
            qDebug() << i << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                    << (rate / 1024.0 / 1024 * 1000) << "MB/s";
        }
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 / 1024 * 1000 / outerTimer.elapsed()) << "MB/s";

#if defined(Q_OS_UNIX)
    alarm(0);
#endif
}

void tst_NetworkStressTest::nativeNonBlockingConnectDisconnect()
{
    QFETCH_GLOBAL(QString, hostname);
    QFETCH_GLOBAL(int, port);

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

    for (int i = 0; i < AttemptCount; ++i) {
        QElapsedTimer timeout;
        byteCounter = 0;
        timeout.start();

        // look up the host
        QByteArray addr;
        if (!nativeLookup(QUrl::toAce(hostname).constData(), port, addr))
            QFAIL("Lookup failed");

        SOCKET fd;

        {
#if defined(Q_OS_UNIX)
            fd = ::socket(AF_INET, SOCK_STREAM, 0);
            QVERIFY(fd != INVALID_SOCKET);

            // set the socket to non-blocking and start connecting
# if !defined(Q_OS_VXWORKS)
            int flags = ::fcntl(fd, F_GETFL, 0);
            QVERIFY(flags != -1);
            QVERIFY(::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
# else // Q_OS_VXWORKS
            int onoff = 1;
            QVERIFY(::ioctl(socketDescriptor, FIONBIO, &onoff) >= 0);
# endif // Q_OS_VXWORKS
            while (true) {
                if (::connect(fd, (sockaddr *)addr.data(), addr.size()) == -1) {
                    QVERIFY2(errno == EINPROGRESS, QByteArray("Error connecting: ").append(strerror(errno)).constData());
                    QVERIFY2(nativeSelect(fd, 10000 - timeout.elapsed(), true), "Timeout");
                } else {
                    break; // connected
                }
            }
#elif defined(Q_OS_WIN)
            fd = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
            QVERIFY(fd != INVALID_SOCKET);

            // set the socket to non-blocking and start connecting
            unsigned long buf = 0;
            unsigned long outBuf;
            DWORD sizeWritten = 0;
            QVERIFY(::WSAIoctl(fd, FIONBIO, &buf, sizeof(unsigned long), &outBuf, sizeof(unsigned long), &sizeWritten, 0,0) != SOCKET_ERROR);

            while (true) {
                int connectResult = ::WSAConnect(fd, (sockaddr *)addr.data(), addr.size(), 0,0,0,0);
                if (connectResult == 0 || WSAGetLastError() == WSAEISCONN) {
                    break; // connected
                } else {
                    QVERIFY2(WSAGetLastError() == WSAEINPROGRESS, "Unexpected error");
                    QVERIFY2(nativeSelect(fd, 10000 - timeout.elapsed(), true), "Timeout");
                }
            }
#endif
        }

        // send request
        {
            QByteArray request = "GET /qtest/bigfile HTTP/1.1\r\n"
                                 "Connection: close\r\n"
                                 "User-Agent: tst_QTcpSocket_stresstest/1.0\r\n"
                                 "Host: " + hostname.toLatin1() + "\r\n"
                                 "\r\n";
            qint64 bytesWritten = 0;
            while (bytesWritten < request.size()) {
                qint64 ret = ::send(fd, request.constData() + bytesWritten, request.size() - bytesWritten, 0);
                if (ret == -1 && errno != EWOULDBLOCK) {
                    ::close(fd);
                    QFAIL(QByteArray("Error writing: ").append(strerror(errno)).constData());
                } else if (ret == -1) {
                    // wait for writing
                    QVERIFY2(nativeSelect(fd, 10000 - timeout.elapsed(), true), "Timeout");
                    continue;
                }
                bytesWritten += ret;
            }
        }

        // receive reply
        char buf[16384];
        while (true) {
            qint64 ret = ::recv(fd, buf, sizeof buf, 0);
            if (ret == -1 && errno != EWOULDBLOCK) {
                ::close(fd);
                QFAIL("Timeout");
            } else if (ret == -1) {
                // wait for reading
                QVERIFY2(nativeSelect(fd, 10000 - timeout.elapsed(), false), "Timeout");
            } else if (ret == 0) {
                break; // EOF
            }
            byteCounter += ret;
        }
        ::close(fd);

        totalBytes += byteCounter;
        if (intermediateDebug) {
            double rate = (byteCounter * 1.0 / timeout.elapsed());
            qDebug() << i << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                    << (rate / 1024.0 / 1024 * 1000) << "MB/s";
        }
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 / 1024 * 1000 / outerTimer.elapsed()) << "MB/s";
}

void tst_NetworkStressTest::blockingConnectDisconnect()
{
    QFETCH_GLOBAL(QString, hostname);
    QFETCH_GLOBAL(int, port);

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

    for (int i = 0; i < AttemptCount; ++i) {
        QElapsedTimer timeout;
        byteCounter = 0;
        timeout.start();

        QTcpSocket socket;
        socket.connectToHost(hostname, port);
        QVERIFY2(socket.waitForConnected(), "Timeout");

        socket.write("GET /qtest/bigfile HTTP/1.1\r\n"
                     "Connection: close\r\n"
                     "User-Agent: tst_QTcpSocket_stresstest/1.0\r\n"
                     "Host: " + hostname.toLatin1() + "\r\n"
                     "\r\n");
        while (socket.bytesToWrite())
            QVERIFY2(socket.waitForBytesWritten(), "Timeout");

        while (socket.state() == QAbstractSocket::ConnectedState && !timeout.hasExpired(10000)) {
            socket.waitForReadyRead();
            byteCounter += socket.readAll().size(); // discard
        }

        totalBytes += byteCounter;
        if (intermediateDebug) {
            double rate = (byteCounter * 1.0 / timeout.elapsed());
            qDebug() << i << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                    << (rate / 1024.0 / 1024 * 1000) << "MB/s";
        }
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 / 1024 * 1000 / outerTimer.elapsed()) << "MB/s";
}

void tst_NetworkStressTest::blockingPipelined()
{
    QFETCH_GLOBAL(QString, hostname);
    QFETCH_GLOBAL(int, port);

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

    for (int i = 0; i < AttemptCount / 2; ++i) {
        QElapsedTimer timeout;
        byteCounter = 0;
        timeout.start();

        QTcpSocket socket;
        socket.connectToHost(hostname, port);
        QVERIFY2(socket.waitForConnected(), "Timeout");

        for (int j = 0 ; j < 3; ++j) {
            if (intermediateDebug)
                qDebug("Attempt %d%c", i, 'a' + j);
            socket.write("GET /qtest/bigfile HTTP/1.1\r\n"
                         "Connection: " + QByteArray(j == 2 ? "close" : "keep-alive") + "\r\n"
                         "User-Agent: tst_QTcpSocket_stresstest/1.0\r\n"
                         "Host: " + hostname.toLatin1() + "\r\n"
                         "\r\n");
            while (socket.bytesToWrite())
                QVERIFY2(socket.waitForBytesWritten(), "Timeout");
        }

        while (socket.state() == QAbstractSocket::ConnectedState && !timeout.hasExpired(10000)) {
            socket.waitForReadyRead();
            byteCounter += socket.readAll().size(); // discard
        }

        totalBytes += byteCounter;
        if (intermediateDebug) {
            double rate = (byteCounter * 1.0 / timeout.elapsed());
            qDebug() << i << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                    << (rate / 1024.0 / 1024 * 1000) << "MB/s";
        }
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 / 1024 * 1000 / outerTimer.elapsed()) << "MB/s";
}

void tst_NetworkStressTest::blockingMultipleRequests()
{
    QFETCH_GLOBAL(QString, hostname);
    QFETCH_GLOBAL(int, port);

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

    for (int i = 0; i < AttemptCount / 5; ++i) {
        QTcpSocket socket;
        socket.connectToHost(hostname, port);
        QVERIFY2(socket.waitForConnected(), "Timeout");

        for (int j = 0 ; j < 5; ++j) {
            QElapsedTimer timeout;
            byteCounter = 0;
            timeout.start();

            socket.write("GET /qtest/bigfile HTTP/1.1\r\n"
                         "Connection: keep-alive\r\n"
                         "User-Agent: tst_QTcpSocket_stresstest/1.0\r\n"
                         "Host: " + hostname.toLatin1() + "\r\n"
                         "\r\n");
            while (socket.bytesToWrite())
                QVERIFY2(socket.waitForBytesWritten(), "Timeout");

            qint64 bytesRead = 0;
            qint64 bytesExpected = -1;
            QByteArray buffer;
            while (socket.state() == QAbstractSocket::ConnectedState && !timeout.hasExpired(10000)) {
                socket.waitForReadyRead();
                buffer += socket.readAll();
                byteCounter += buffer.size();
                int pos = buffer.indexOf("\r\n\r\n");
                if (pos == -1)
                    continue;

                bytesRead = buffer.length() - pos - 4;

                buffer.truncate(pos + 2);
                buffer = buffer.toLower();
                pos = buffer.indexOf("\r\ncontent-length: ");
                if (pos == -1) {
                    qWarning() << "no content-length:" << QString(buffer);
                    break;
                }
                pos += int(strlen("\r\ncontent-length: "));

                int eol = buffer.indexOf("\r\n", pos + 2);
                if (eol == -1) {
                    qWarning() << "invalid header";
                    break;
                }

                bytesExpected = buffer.mid(pos, eol - pos).toLongLong();
                break;
            }
            QVERIFY(bytesExpected > 0);
            QVERIFY2(!timeout.hasExpired(10000), "Timeout");

            while (socket.state() == QAbstractSocket::ConnectedState && !timeout.hasExpired(10000) && bytesExpected > bytesRead) {
                socket.waitForReadyRead();
                int blocklen = socket.read(bytesExpected - bytesRead).length(); // discard
                bytesRead += blocklen;
                byteCounter += blocklen;
            }
            QVERIFY2(!timeout.hasExpired(10000), "Timeout");
            QCOMPARE(bytesRead, bytesExpected);

            totalBytes += byteCounter;
            if (intermediateDebug) {
                double rate = (byteCounter * 1.0 / timeout.elapsed());
                qDebug() << i << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                        << (rate / 1024.0 / 1024 * 1000) << "MB/s";
            }
        }

        socket.disconnectFromHost();
        QVERIFY(socket.state() == QAbstractSocket::UnconnectedState);
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 / 1024 * 1000 / outerTimer.elapsed()) << "MB/s";
}

void tst_NetworkStressTest::connectDisconnect()
{
    QFETCH_GLOBAL(QString, hostname);
    QFETCH_GLOBAL(int, port);

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

    for (int i = 0; i < AttemptCount; ++i) {
        QElapsedTimer timeout;
        byteCounter = 0;
        timeout.start();

        QTcpSocket socket;
        socket.connectToHost(hostname, port);

        socket.write("GET /qtest/bigfile HTTP/1.1\r\n"
                     "Connection: close\r\n"
                     "User-Agent: tst_QTcpSocket_stresstest/1.0\r\n"
                     "Host: " + hostname.toLatin1() + "\r\n"
                     "\r\n");
        connect(&socket, SIGNAL(readyRead()), SLOT(slotReadAll()));

        QTestEventLoop::instance().connect(&socket, SIGNAL(disconnected()), SLOT(exitLoop()));
        QTestEventLoop::instance().enterLoop(30);
        QVERIFY2(!QTestEventLoop::instance().timeout(), "Timeout");

        totalBytes += byteCounter;
        if (intermediateDebug) {
            double rate = (byteCounter * 1.0 / timeout.elapsed());
            qDebug() << i << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                    << (rate / 1024.0 / 1024 * 1000) << "MB/s";
        }
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 / 1024 * 1000 / outerTimer.elapsed()) << "MB/s";
}

void tst_NetworkStressTest::parallelConnectDisconnect_data()
{
    QTest::addColumn<int>("parallelAttempts");
    QTest::newRow("1") << 1;
    QTest::newRow("2") << 2;
    QTest::newRow("4") << 4;
    QTest::newRow("5") << 5;
    QTest::newRow("6") << 6;
    QTest::newRow("8") << 8;
    QTest::newRow("10") << 10;
    QTest::newRow("25") << 25;
    QTest::newRow("100") << 100;
    QTest::newRow("500") << 500;
}

void tst_NetworkStressTest::parallelConnectDisconnect()
{
    QFETCH_GLOBAL(QString, hostname);
    QFETCH_GLOBAL(int, port);
    QFETCH(int, parallelAttempts);

    if (parallelAttempts > 100) {
        QFETCH_GLOBAL(bool, isLocalhost);
        if (!isLocalhost)
            QSKIP("Localhost-only test");
    }

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

    for (int i = 0; i < qMax(2, AttemptCount/qMax(2, parallelAttempts/4)); ++i) {
        QElapsedTimer timeout;
        byteCounter = 0;
        timeout.start();

        QTcpSocket *socket = new QTcpSocket[parallelAttempts];
        for (int j = 0; j < parallelAttempts; ++j) {
            socket[j].connectToHost(hostname, port);

            socket[j].write("GET /qtest/bigfile HTTP/1.1\r\n"
                            "Connection: close\r\n"
                            "User-Agent: tst_QTcpSocket_stresstest/1.0\r\n"
                            "Host: " + hostname.toLatin1() + "\r\n"
                            "\r\n");
            connect(&socket[j], SIGNAL(readyRead()), SLOT(slotReadAll()));

            QTestEventLoop::instance().connect(&socket[j], SIGNAL(disconnected()), SLOT(exitLoop()));
        }

        while (!timeout.hasExpired(30000)) {
            QTestEventLoop::instance().enterLoop(10);
            int done = 0;
            for (int j = 0; j < parallelAttempts; ++j)
                done += socket[j].state() == QAbstractSocket::UnconnectedState ? 1 : 0;
            if (done == parallelAttempts)
                break;
        }
        delete[] socket;
        QVERIFY2(!timeout.hasExpired(30000), "Timeout");

        totalBytes += byteCounter;
        if (intermediateDebug) {
            double rate = (byteCounter * 1.0 / timeout.elapsed());
            qDebug() << i << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                    << (rate / 1024.0 / 1024 * 1000) << "MB/s";
        }
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 / 1024 * 1000 / outerTimer.elapsed()) << "MB/s";
}

void tst_NetworkStressTest::namGet_data()
{
    QTest::addColumn<int>("parallelAttempts");
    QTest::addColumn<bool>("pipelineAllowed");

    QTest::newRow("1") << 1 << false;
    QTest::newRow("1p") << 1 << true;
    QTest::newRow("2") << 2 << false;
    QTest::newRow("2p") << 2 << true;
    QTest::newRow("4") << 4 << false;
    QTest::newRow("4p") << 4 << true;
    QTest::newRow("5") << 5 << false;
    QTest::newRow("5p") << 5 << true;
    QTest::newRow("6") << 6 << false;
    QTest::newRow("6p") << 6 << true;
    QTest::newRow("8") << 8 << false;
    QTest::newRow("8p") << 8 << true;
    QTest::newRow("10") << 10 << false;
    QTest::newRow("10p") << 10 << true;
    QTest::newRow("25") << 25 << false;
    QTest::newRow("25p") << 25 << true;
    QTest::newRow("100") << 100 << false;
    QTest::newRow("100p") << 100 << true;
    QTest::newRow("500") << 500 << false;
    QTest::newRow("500p") << 500 << true;
}

void tst_NetworkStressTest::namGet()
{
    QFETCH_GLOBAL(QString, hostname);
    QFETCH_GLOBAL(int, port);
    QFETCH(int, parallelAttempts);
    QFETCH(bool, pipelineAllowed);

    if (parallelAttempts > 100) {
        QFETCH_GLOBAL(bool, isLocalhost);
        if (!isLocalhost)
            QSKIP("Localhost-only test");
    }

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

    for (int i = 0; i < qMax(2, AttemptCount/qMax(2, parallelAttempts/4)); ++i) {
        QElapsedTimer timeout;
        byteCounter = 0;
        timeout.start();

        QUrl url;
        url.setScheme("http");
        url.setHost(hostname);
        url.setPort(port);
        url.setEncodedPath("/qtest/bigfile");
        QNetworkRequest req(url);
        req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, pipelineAllowed);

        QVector<QSharedPointer<QNetworkReply> > replies;
        replies.resize(parallelAttempts);
        for (int j = 0; j < parallelAttempts; ++j) {
            QNetworkReply *r = manager.get(req);

            connect(r, SIGNAL(readyRead()), SLOT(slotReadAll()));
            QTestEventLoop::instance().connect(r, SIGNAL(finished()), SLOT(exitLoop()));

            replies[j] = QSharedPointer<QNetworkReply>(r);
        }

        while (!timeout.hasExpired(30000)) {
            QTestEventLoop::instance().enterLoop(10);
            int done = 0;
            for (int j = 0; j < parallelAttempts; ++j)
                done += replies[j]->isFinished() ? 1 : 0;
            if (done == parallelAttempts)
                break;
        }
        replies.clear();

        QVERIFY2(!timeout.hasExpired(30000), "Timeout");
        totalBytes += byteCounter;
        if (intermediateDebug) {
            double rate = (byteCounter * 1.0 / timeout.elapsed());
            qDebug() << i << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                    << (rate / 1024.0 / 1024 * 1000) << "MB/s";
        }
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 / 1024 * 1000 / outerTimer.elapsed()) << "MB/s";
}

QTEST_MAIN(tst_NetworkStressTest);

#include "tst_network_stresstest.moc"
