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
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

#ifdef QT_BUILD_INTERNAL
# include <private/qnetworkaccessmanager_p.h>
#endif

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


class tst_NetworkRemoteStressTest : public QObject
{
    Q_OBJECT
public:
    enum { AttemptCount = 100 };
    tst_NetworkRemoteStressTest();

    qint64 byteCounter;
    QNetworkAccessManager manager;
    QVector<QUrl> httpUrls, httpsUrls, mixedUrls;
    bool intermediateDebug;

private:
    void clearManager();

public slots:
    void initTestCase_data();
    void init();

    void slotReadAll() { byteCounter += static_cast<QIODevice *>(sender())->readAll().size(); }

private Q_SLOTS:
    void blockingSequentialRemoteHosts();
    void sequentialRemoteHosts();
    void parallelRemoteHosts_data();
    void parallelRemoteHosts();
    void namRemoteGet_data();
    void namRemoteGet();
};

tst_NetworkRemoteStressTest::tst_NetworkRemoteStressTest()
    : intermediateDebug(qgetenv("STRESSDEBUG").toInt() > 0)
{
#ifdef Q_OS_WIN
    WSAData wsadata;

    // IPv6 requires Winsock v2.0 or better.
    WSAStartup(MAKEWORD(2,0), &wsadata);
#elif defined(Q_OS_UNIX)
    ::signal(SIGALRM, SIG_IGN);
#endif

    QFile urlList(":/url-list.txt");
    if (urlList.open(QIODevice::ReadOnly)) {
        while (!urlList.atEnd()) {
            QByteArray line = urlList.readLine().trimmed();
            QUrl url = QUrl::fromEncoded(line);
            if (url.scheme() == "http") {
                httpUrls << url;
                mixedUrls << url;
            } else if (url.scheme() == "https") {
                httpsUrls << url;
                mixedUrls << url;
            }
        }
    }

    httpUrls << httpUrls;
    httpsUrls << httpsUrls;
}

void tst_NetworkRemoteStressTest::initTestCase_data()
{
    QTest::addColumn<QVector<QUrl> >("urlList");
    QTest::addColumn<bool>("useSslSocket");

    QTest::newRow("no-ssl") << httpUrls << false;
//    QTest::newRow("no-ssl-in-sslsocket") << httpUrls << true;
    QTest::newRow("ssl") << httpsUrls << true;
    QTest::newRow("mixed") << mixedUrls << false;
//    QTest::newRow("mixed-in-sslsocket") << mixedUrls << true;
}

void tst_NetworkRemoteStressTest::init()
{
    // clear the internal cache
#ifndef QT_BUILD_INTERNAL
    if (strncmp(QTest::currentTestFunction(), "nam", 3) == 0)
        QSKIP("QNetworkAccessManager tests disabled");
#endif
}

void tst_NetworkRemoteStressTest::clearManager()
{
#ifdef QT_BUILD_INTERNAL
    QNetworkAccessManagerPrivate::clearCache(&manager);
    manager.setProxy(QNetworkProxy());
    manager.setCache(0);
#endif
}

bool nativeLookup(const char *hostname, int port, QByteArray &buf)
{
#if !defined(QT_NO_GETADDRINFO) && 0
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

void tst_NetworkRemoteStressTest::blockingSequentialRemoteHosts()
{
    QFETCH_GLOBAL(QVector<QUrl>, urlList);
    QFETCH_GLOBAL(bool, useSslSocket);

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

#ifdef QT_NO_SSL
    QVERIFY(!useSslSocket);
#endif // QT_NO_SSL

    for (int i = 0; i < urlList.size(); ++i) {
        const QUrl &url = urlList.at(i);
        bool isHttps = url.scheme() == "https";
        QElapsedTimer timeout;
        byteCounter = 0;
        timeout.start();

        QSharedPointer<QTcpSocket> socket;
#ifndef QT_NO_SSL
        if (useSslSocket || isHttps)
            socket = QSharedPointer<QTcpSocket>(new QSslSocket);
#endif // QT_NO_SSL
        if (socket.isNull())
            socket = QSharedPointer<QTcpSocket>(new QTcpSocket);

        socket->connectToHost(url.host(), url.port(isHttps ? 443 : 80));
        const QByteArray encodedHost = url.host(QUrl::FullyEncoded).toLatin1();
        QVERIFY2(socket->waitForConnected(10000), "Timeout connecting to " + encodedHost);

#ifndef QT_NO_SSL
        if (isHttps) {
            static_cast<QSslSocket *>(socket.data())->setProtocol(QSsl::TlsV1_0);
            static_cast<QSslSocket *>(socket.data())->startClientEncryption();
            static_cast<QSslSocket *>(socket.data())->ignoreSslErrors();
            QVERIFY2(static_cast<QSslSocket *>(socket.data())->waitForEncrypted(10000), "Timeout starting TLS with " + encodedHost);
        }
#endif // QT_NO_SSL

        socket->write("GET " + url.toEncoded(QUrl::RemoveScheme | QUrl::RemoveAuthority | QUrl::RemoveFragment) + " HTTP/1.0\r\n"
                      "Connection: close\r\n"
                      "User-Agent: tst_QTcpSocket_stresstest/1.0\r\n"
                      "Host: " + encodedHost + "\r\n"
                      "\r\n");
        while (socket->bytesToWrite())
            QVERIFY2(socket->waitForBytesWritten(10000), "Timeout writing to " + encodedHost);

        while (socket->state() == QAbstractSocket::ConnectedState && !timeout.hasExpired(10000)) {
            socket->waitForReadyRead(10000);
            byteCounter += socket->readAll().size(); // discard
        }
        QVERIFY2(!timeout.hasExpired(10000), "Timeout reading from " + encodedHost);

        totalBytes += byteCounter;
        if (intermediateDebug) {
            double rate = (byteCounter * 1.0 / timeout.elapsed());
            qDebug() << i << url << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                    << (rate / 1024.0 * 1000) << "kB/s";
        }
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 * 1000 / outerTimer.elapsed()) << "kB/s";
}

void tst_NetworkRemoteStressTest::sequentialRemoteHosts()
{
    QFETCH_GLOBAL(QVector<QUrl>, urlList);
    QFETCH_GLOBAL(bool, useSslSocket);

#ifdef QT_NO_SSL
    QVERIFY(!useSslSocket);
#endif // QT_NO_SSL

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

    for (int i = 0; i < urlList.size(); ++i) {
        const QUrl &url = urlList.at(i);
        bool isHttps = url.scheme() == "https";
        QElapsedTimer timeout;
        byteCounter = 0;
        timeout.start();

        QSharedPointer<QTcpSocket> socket;
#ifndef QT_NO_SSL
        if (useSslSocket || isHttps)
            socket = QSharedPointer<QTcpSocket>(new QSslSocket);
#endif // QT_NO_SSL
        if (socket.isNull())
            socket = QSharedPointer<QTcpSocket>(new QTcpSocket);
        if (isHttps) {
#ifndef QT_NO_SSL
            static_cast<QSslSocket *>(socket.data())->setProtocol(QSsl::TlsV1_0);
            static_cast<QSslSocket *>(socket.data())->connectToHostEncrypted(url.host(), url.port(443));
            static_cast<QSslSocket *>(socket.data())->ignoreSslErrors();
#endif // QT_NO_SSL
        } else {
            socket->connectToHost(url.host(), url.port(80));
        }

        const QByteArray encodedHost = url.host(QUrl::FullyEncoded).toLatin1();
        socket->write("GET " + url.toEncoded(QUrl::RemoveScheme | QUrl::RemoveAuthority | QUrl::RemoveFragment) + " HTTP/1.0\r\n"
                      "Connection: close\r\n"
                      "User-Agent: tst_QTcpSocket_stresstest/1.0\r\n"
                      "Host: " + encodedHost + "\r\n"
                      "\r\n");
        connect(socket.data(), SIGNAL(readyRead()), SLOT(slotReadAll()));

        QTestEventLoop::instance().connect(socket.data(), SIGNAL(disconnected()), SLOT(exitLoop()));
        QTestEventLoop::instance().enterLoop(30);
        QVERIFY2(!QTestEventLoop::instance().timeout(), "Timeout with " + encodedHost + "; "
                 + QByteArray::number(socket->bytesToWrite()) + " bytes to write");

        totalBytes += byteCounter;
        if (intermediateDebug) {
            double rate = (byteCounter * 1.0 / timeout.elapsed());
            qDebug() << i << url << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                    << (rate / 1024.0 * 1000) << "kB/s";
        }
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 * 1000 / outerTimer.elapsed()) << "kB/s";
}

void tst_NetworkRemoteStressTest::parallelRemoteHosts_data()
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
    QTest::newRow("500") << 500;
}

void tst_NetworkRemoteStressTest::parallelRemoteHosts()
{
    QFETCH_GLOBAL(QVector<QUrl>, urlList);
    QFETCH_GLOBAL(bool, useSslSocket);

    QFETCH(int, parallelAttempts);

#ifdef QT_NO_SSL
    QVERIFY(!useSslSocket);
#endif // QT_NO_SSL

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

    QVector<QUrl>::ConstIterator it = urlList.constBegin();
    while (it != urlList.constEnd()) {
        QElapsedTimer timeout;
        byteCounter = 0;
        timeout.start();

        QVector<QSharedPointer<QTcpSocket> > sockets;
        sockets.reserve(parallelAttempts);
        for (int j = 0; j < parallelAttempts && it != urlList.constEnd(); ++j, ++it) {
            const QUrl &url = *it;
            bool isHttps = url.scheme() == "https";
            QTcpSocket *socket = 0;
#ifndef QT_NO_SSL
            if (useSslSocket || isHttps)
                socket = new QSslSocket;
#endif // QT_NO_SSL
            if (!socket)
                socket = new QTcpSocket;
            if (isHttps) {
#ifndef QT_NO_SSL
                static_cast<QSslSocket *>(socket)->setProtocol(QSsl::TlsV1_0);
                static_cast<QSslSocket *>(socket)->connectToHostEncrypted(url.host(), url.port(443));
                static_cast<QSslSocket *>(socket)->ignoreSslErrors();
#endif // QT_NO_SSL
            } else {
                socket->connectToHost(url.host(), url.port(isHttps ? 443 : 80));
            }

            const QByteArray encodedHost = url.host(QUrl::FullyEncoded).toLatin1();
            socket->write("GET " + url.toEncoded(QUrl::RemoveScheme | QUrl::RemoveAuthority | QUrl::RemoveFragment) + " HTTP/1.0\r\n"
                          "Connection: close\r\n"
                          "User-Agent: tst_QTcpSocket_stresstest/1.0\r\n"
                          "Host: " + encodedHost + "\r\n"
                          "\r\n");
            connect(socket, SIGNAL(readyRead()), SLOT(slotReadAll()));
            QTestEventLoop::instance().connect(socket, SIGNAL(disconnected()), SLOT(exitLoop()));
            socket->setProperty("remoteUrl", url);

            sockets.append(QSharedPointer<QTcpSocket>(socket));
        }

        while (!timeout.hasExpired(10000)) {
            QTestEventLoop::instance().enterLoop(10);
            int done = 0;
            for (int j = 0; j < sockets.size(); ++j)
                done += sockets[j]->state() == QAbstractSocket::UnconnectedState ? 1 : 0;
            if (done == sockets.size())
                break;
        }
        for (int j = 0; j < sockets.size(); ++j)
            if (sockets[j]->state() != QAbstractSocket::UnconnectedState) {
                qDebug() << "Socket to" << sockets[j]->property("remoteUrl").toUrl() << "still open with"
                        << sockets[j]->bytesToWrite() << "bytes to write";
                QFAIL("Timed out");
            }

        totalBytes += byteCounter;
        if (intermediateDebug) {
            double rate = (byteCounter * 1.0 / timeout.elapsed());
            qDebug() << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                    << (rate / 1024.0 * 1000) << "kB/s";
        }
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 * 1000 / outerTimer.elapsed()) << "kB/s";
}

void tst_NetworkRemoteStressTest::namRemoteGet_data()
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
    QTest::newRow("500") << 500;
}

void tst_NetworkRemoteStressTest::namRemoteGet()
{
    QFETCH_GLOBAL(QVector<QUrl>, urlList);

    QFETCH(int, parallelAttempts);
    bool pipelineAllowed = false;// QFETCH(bool, pipelineAllowed);

    qint64 totalBytes = 0;
    QElapsedTimer outerTimer;
    outerTimer.start();

    QVector<QUrl>::ConstIterator it = urlList.constBegin();
    while (it != urlList.constEnd()) {
        QElapsedTimer timeout;
        byteCounter = 0;
        timeout.start();

        QNetworkRequest req;
        req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, pipelineAllowed);

        QVector<QSharedPointer<QNetworkReply> > replies;
        replies.reserve(parallelAttempts);
        for (int j = 0; j < parallelAttempts && it != urlList.constEnd(); ++j) {
            req.setUrl(*it++);
            QNetworkReply *r = manager.get(req);
            r->ignoreSslErrors();

            connect(r, SIGNAL(readyRead()), SLOT(slotReadAll()));
            QTestEventLoop::instance().connect(r, SIGNAL(finished()), SLOT(exitLoop()));

            replies.append(QSharedPointer<QNetworkReply>(r));
        }

        while (!timeout.hasExpired(30000)) {
            QTestEventLoop::instance().enterLoop(30 - timeout.elapsed() / 1000);
            int done = 0;
            for (int j = 0; j < replies.size(); ++j)
                done += replies[j]->isFinished() ? 1 : 0;
            if (done == replies.size())
                break;
        }
        if (timeout.hasExpired(30000)) {
            for (int j = 0; j < replies.size(); ++j)
                if (!replies[j]->isFinished())
                    qDebug() << "Request" << replies[j]->url() << "not finished";
            QFAIL("Timed out");
        }
        replies.clear();

        totalBytes += byteCounter;
        if (intermediateDebug) {
            double rate = (byteCounter * 1.0 / timeout.elapsed());
            qDebug() << byteCounter << "bytes in" << timeout.elapsed() << "ms:"
                    << (rate / 1024.0 * 1000) << "kB/s";
        }
    }
    qDebug() << "Average transfer rate was" << (totalBytes / 1024.0 * 1000 / outerTimer.elapsed()) << "kB/s";
}

QTEST_MAIN(tst_NetworkRemoteStressTest);

#include "tst_network_remote_stresstest.moc"
