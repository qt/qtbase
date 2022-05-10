// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QT_NO_NATIVE_POLL
#define QT_NO_NATIVE_POLL
#endif

#include <QTest>
#include <QtNetwork>

#include <private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE
// defined in qpoll.cpp
int qt_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts);
QT_END_NAMESPACE

class tst_qt_poll : public QObject
{
    Q_OBJECT

private slots:
    void pollout();
    void pollin();
    void pollnval();
    void pollprihup();
};

void tst_qt_poll::pollout()
{
    int fds[2];
    QCOMPARE(pipe(fds), 0);

    struct pollfd pfd = { fds[1], POLLOUT, 0 };
    const int nready = qt_poll(&pfd, 1, NULL);

    QCOMPARE(nready, 1);
    QCOMPARE(pfd.revents, short(POLLOUT));

    qt_safe_close(fds[0]);
    qt_safe_close(fds[1]);
}

void tst_qt_poll::pollin()
{
    int fds[2];
    QCOMPARE(pipe(fds), 0);

    const char data = 'Q';
    QCOMPARE(qt_safe_write(fds[1], &data, 1), 1);

    struct pollfd pfd = { fds[0], POLLIN, 0 };
    const int nready = qt_poll(&pfd, 1, NULL);

    QCOMPARE(nready, 1);
    QCOMPARE(pfd.revents, short(POLLIN));

    qt_safe_close(fds[0]);
    qt_safe_close(fds[1]);
}

void tst_qt_poll::pollnval()
{
    struct pollfd pfd = { 42, POLLOUT, 0 };

    int nready = qt_poll(&pfd, 1, NULL);
    QCOMPARE(nready, 1);
    QCOMPARE(pfd.revents, short(POLLNVAL));

    pfd.events = 0;
    pfd.revents = 0;

    nready = qt_poll(&pfd, 1, NULL);
    QCOMPARE(nready, 1);
    QCOMPARE(pfd.revents, short(POLLNVAL));
}

void tst_qt_poll::pollprihup()
{
    QTcpServer server;
    QTcpSocket client_socket;

    QVERIFY(server.listen(QHostAddress::LocalHost));

    const quint16 server_port = server.serverPort();
    client_socket.connectToHost(server.serverAddress(), server_port);

    QVERIFY(client_socket.waitForConnected());
    QVERIFY(server.waitForNewConnection());

    QTcpSocket *server_socket = server.nextPendingConnection();
    server.close();

    // TCP supports only a single byte of urgent data
    static const char oob_out = 'Q';
    QCOMPARE(::send(server_socket->socketDescriptor(), &oob_out, 1, MSG_OOB),
             ssize_t(1));

    struct pollfd pfd = {
        int(client_socket.socketDescriptor()),
        POLLPRI | POLLIN,
        0
    };
    int res = qt_poll(&pfd, 1, NULL);

    QCOMPARE(res, 1);
    QCOMPARE(pfd.revents, short(POLLPRI | POLLIN));

    char oob_in = 0;
    // We do not specify MSG_OOB here as SO_OOBINLINE is turned on by default
    // in the native socket engine
    QCOMPARE(::recv(client_socket.socketDescriptor(), &oob_in, 1, 0),
             ssize_t(1));
    QCOMPARE(oob_in, oob_out);

    server_socket->close();
    pfd.events = POLLIN;
    res = qt_poll(&pfd, 1, NULL);

    QCOMPARE(res, 1);
    QCOMPARE(pfd.revents, short(POLLHUP));
}

QTEST_APPLESS_MAIN(tst_qt_poll)
#include "tst_qt_poll.moc"
