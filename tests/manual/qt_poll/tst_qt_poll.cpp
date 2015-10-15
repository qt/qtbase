/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtNetwork>

#include <private/qcore_unix_p.h>

#ifdef QT_BUILD_INTERNAL
QT_BEGIN_NAMESPACE
Q_AUTOTEST_EXPORT int qt_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts);
QT_END_NAMESPACE
#endif // QT_BUILD_INTERNAL

QT_USE_NAMESPACE

class tst_qt_poll : public QObject
{
    Q_OBJECT

#ifdef QT_BUILD_INTERNAL
private slots:
    void pollout();
    void pollin();
    void pollnval();
    void pollprihup();
#endif // QT_BUILD_INTERNAL
};

#ifdef QT_BUILD_INTERNAL
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
#endif // QT_BUILD_INTERNAL

QTEST_APPLESS_MAIN(tst_qt_poll)
#include "tst_qt_poll.moc"
