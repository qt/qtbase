// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtTest/QTestEventLoop>

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtCore/QSocketNotifier>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <private/qnativesocketengine_p.h>
#define NATIVESOCKETENGINE QNativeSocketEngine
#ifdef Q_OS_UNIX
#include <private/qnet_unix_p.h>
#include <sys/select.h>
#endif
#include <limits>

#if defined (Q_CC_MSVC) && defined(max)
#  undef max
#  undef min
#endif // Q_CC_MSVC

using namespace std::chrono_literals;

class tst_QSocketNotifier : public QObject
{
    Q_OBJECT
private slots:
    void constructing();
    void unexpectedDisconnection();
    void mixingWithTimers();
#ifdef Q_OS_UNIX
    void posixSockets();
#endif
    void asyncMultipleDatagram();
    void activationReason_data();
    void activationReason();
    void legacyConnect();

protected slots:
    void async_readDatagramSlot();
    void async_writeDatagramSlot();

private:
    QUdpSocket *m_asyncSender;
    QUdpSocket *m_asyncReceiver;
};

static QHostAddress makeNonAny(const QHostAddress &address,
                               QHostAddress::SpecialAddress preferForAny = QHostAddress::LocalHost)
{
    if (address == QHostAddress::Any)
        return preferForAny;
    if (address == QHostAddress::AnyIPv4)
        return QHostAddress::LocalHost;
    if (address == QHostAddress::AnyIPv6)
        return QHostAddress::LocalHostIPv6;
    return address;
}

void tst_QSocketNotifier::constructing()
{
    const qintptr fd = 15;

    // Test constructing with no descriptor assigned.
    {
        QSocketNotifier notifier(QSocketNotifier::Read);

        QVERIFY(!notifier.isValid());
        QCOMPARE(notifier.socket(), Q_INT64_C(-1));
        QCOMPARE(notifier.type(), QSocketNotifier::Read);
        QVERIFY(!notifier.isEnabled());

        notifier.setEnabled(true);
        QVERIFY(!notifier.isEnabled());

        notifier.setSocket(fd);
        QVERIFY(notifier.isValid());
        QCOMPARE(notifier.socket(), fd);
        QVERIFY(!notifier.isEnabled());
        notifier.setEnabled(true);
        QVERIFY(notifier.isEnabled());
    }

    // Test constructing with the notifications enabled by default.
    {
        QSocketNotifier notifier(fd, QSocketNotifier::Write);

        QVERIFY(notifier.isValid());
        QCOMPARE(notifier.socket(), fd);
        QCOMPARE(notifier.type(), QSocketNotifier::Write);
        QVERIFY(notifier.isEnabled());

        notifier.setSocket(fd);
        QVERIFY(!notifier.isEnabled());

        notifier.setEnabled(true);
        QVERIFY(notifier.isEnabled());
        notifier.setSocket(-1);
        QVERIFY(!notifier.isValid());
        QCOMPARE(notifier.socket(), Q_INT64_C(-1));
        QVERIFY(!notifier.isEnabled());
    }
}

class UnexpectedDisconnectTester : public QObject
{
    Q_OBJECT
public:
    NATIVESOCKETENGINE *readEnd1, *readEnd2;
    int sequence;

    UnexpectedDisconnectTester(NATIVESOCKETENGINE *s1, NATIVESOCKETENGINE *s2)
        : readEnd1(s1), readEnd2(s2), sequence(0)
    {
        QSocketNotifier *notifier1 =
            new QSocketNotifier(readEnd1->socketDescriptor(), QSocketNotifier::Read, this);
        connect(notifier1, SIGNAL(activated(QSocketDescriptor)), SLOT(handleActivated()));
        QSocketNotifier *notifier2 =
            new QSocketNotifier(readEnd2->socketDescriptor(), QSocketNotifier::Read, this);
        connect(notifier2, SIGNAL(activated(QSocketDescriptor)), SLOT(handleActivated()));
    }

public slots:
    void handleActivated()
    {
        char data1[1], data2[1];
        ++sequence;
        if (sequence == 1) {
            // read from both ends
            QCOMPARE(readEnd1->read(data1, sizeof(data1)), 1);
            QCOMPARE(readEnd2->read(data2, sizeof(data2)), 1);
            emit finished();
        } else if (sequence == 2) {
            // check that we can't read now because we've read our byte
            QCOMPARE(readEnd2->read(data2, sizeof(data2)), qint64(-2));
            QVERIFY(readEnd2->isValid());
        }
    }

signals:
    void finished();
};

void tst_QSocketNotifier::unexpectedDisconnection()
{
    /*
      Given two sockets and two QSocketNotifiers registered on each
      their socket. If both sockets receive data, and the first slot
      invoked by one of the socket notifiers empties both sockets, the
      other notifier will also emit activated(). This was causing an
      unexpected disconnection in QAbstractSocket.

      The use case is that somebody calls one of the
      waitFor... functions in a QSocketNotifier activated slot, and
      the waitFor... functions do local selects that can empty both
      stdin and stderr while waiting for fex bytes to be written.
    */

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    NATIVESOCKETENGINE readEnd1;
    readEnd1.initialize(QAbstractSocket::TcpSocket);
    readEnd1.connectToHost(server.serverAddress(), server.serverPort());
    QVERIFY(readEnd1.waitForWrite());
    QCOMPARE(readEnd1.state(), QAbstractSocket::ConnectedState);
    QVERIFY(server.waitForNewConnection(5000));
    QTcpSocket *writeEnd1 = server.nextPendingConnection();
    QVERIFY(writeEnd1 != 0);

    NATIVESOCKETENGINE readEnd2;
    readEnd2.initialize(QAbstractSocket::TcpSocket);
    readEnd2.connectToHost(server.serverAddress(), server.serverPort());
    QVERIFY(readEnd2.waitForWrite());
    QCOMPARE(readEnd2.state(), QAbstractSocket::ConnectedState);
    QVERIFY(server.waitForNewConnection(5000));
    QTcpSocket *writeEnd2 = server.nextPendingConnection();
    QVERIFY(writeEnd2 != 0);

    writeEnd1->write("1", 1);
    writeEnd2->write("2", 1);

    writeEnd1->waitForBytesWritten();
    writeEnd2->waitForBytesWritten();

    // ensure both read ends are ready for reading, before the event loop
    QVERIFY(readEnd1.waitForRead(5s));
    QVERIFY(readEnd2.waitForRead(5s));

    UnexpectedDisconnectTester tester(&readEnd1, &readEnd2);

    QTimer timer;
    timer.setSingleShot(true);
    timer.start(30000);
    do {
        // we have to wait until sequence value changes
        // as any event can make us jump out processing
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
        QVERIFY(timer.isActive()); //escape if test would hang
    }  while(tester.sequence <= 0);

    QCOMPARE(readEnd1.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(readEnd2.state(), QAbstractSocket::ConnectedState);

    QCOMPARE(tester.sequence, 2);

    readEnd1.close();
    readEnd2.close();
    writeEnd1->close();
    writeEnd2->close();
    server.close();
}

class MixingWithTimersHelper : public QObject
{
    Q_OBJECT

public:
    MixingWithTimersHelper(QTimer *timer, QTcpServer *server);

    bool timerActivated;
    bool socketActivated;

private slots:
    void timerFired();
    void socketFired();
};

MixingWithTimersHelper::MixingWithTimersHelper(QTimer *timer, QTcpServer *server)
{
    timerActivated = false;
    socketActivated = false;

    connect(timer, SIGNAL(timeout()), SLOT(timerFired()));
    connect(server, SIGNAL(newConnection()), SLOT(socketFired()));
}

void MixingWithTimersHelper::timerFired()
{
    timerActivated = true;
}

void MixingWithTimersHelper::socketFired()
{
    socketActivated = true;
}

void tst_QSocketNotifier::mixingWithTimers()
{
    QTimer timer;
    timer.setInterval(0);
    timer.start();

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    MixingWithTimersHelper helper(&timer, &server);

    QCoreApplication::processEvents();

    QCOMPARE(helper.timerActivated, true);
    QCOMPARE(helper.socketActivated, false);

    helper.timerActivated = false;
    helper.socketActivated = false;

    QTcpSocket socket;
    socket.connectToHost(server.serverAddress(), server.serverPort());

    QCoreApplication::processEvents();

    QCOMPARE(helper.timerActivated, true);
    QTRY_COMPARE(helper.socketActivated, true);
}

#ifdef Q_OS_UNIX
// test only for posix
void tst_QSocketNotifier::posixSockets()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    int posixSocket = qt_safe_socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_addr.s_addr = htonl(0x7f000001);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server.serverPort());
    qt_safe_connect(posixSocket, (const struct sockaddr*)&addr, sizeof(sockaddr_in));
    QVERIFY(server.waitForNewConnection(5000));
    QScopedPointer<QTcpSocket> passive(server.nextPendingConnection());

    ::fcntl(posixSocket, F_SETFL, ::fcntl(posixSocket, F_GETFL) | O_NONBLOCK);

    {
        QSocketNotifier rn(posixSocket, QSocketNotifier::Read);
        connect(&rn, SIGNAL(activated(QSocketDescriptor)), &QTestEventLoop::instance(), SLOT(exitLoop()));
        QSignalSpy readSpy(&rn, &QSocketNotifier::activated);
        QVERIFY(readSpy.isValid());
        // No write notifier, some systems trigger write notification on socket creation, but not all
        QSocketNotifier en(posixSocket, QSocketNotifier::Exception);
        connect(&en, SIGNAL(activated(QSocketDescriptor)), &QTestEventLoop::instance(), SLOT(exitLoop()));
        QSignalSpy errorSpy(&en, &QSocketNotifier::activated);
        QVERIFY(errorSpy.isValid());

        passive->write("hello",6);
        passive->waitForBytesWritten(5000);

        QTestEventLoop::instance().enterLoop(3);
        QCOMPARE(readSpy.size(), 1);
        QCOMPARE(errorSpy.size(), 0);

        char buffer[100];
        int r = qt_safe_read(posixSocket, buffer, 100);
        QCOMPARE(r, 6);
        QCOMPARE(buffer, "hello");

        QSocketNotifier wn(posixSocket, QSocketNotifier::Write);
        connect(&wn, SIGNAL(activated(QSocketDescriptor)), &QTestEventLoop::instance(), SLOT(exitLoop()));
        QSignalSpy writeSpy(&wn, &QSocketNotifier::activated);
        QVERIFY(writeSpy.isValid());
        qt_safe_write(posixSocket, "goodbye", 8);

        QTestEventLoop::instance().enterLoop(3);
        QCOMPARE(readSpy.size(), 1);
        QCOMPARE(writeSpy.size(), 1);
        QCOMPARE(errorSpy.size(), 0);

        // Write notifier may have fired before the read notifier inside
        // QTcpSocket, give QTcpSocket a chance to see the incoming data
        passive->waitForReadyRead(100);
        QCOMPARE(passive->readAll(), QByteArray("goodbye",8));
    }
    qt_safe_close(posixSocket);
}
#endif

void tst_QSocketNotifier::async_readDatagramSlot()
{
    char buf[1];
    QVERIFY(m_asyncReceiver->hasPendingDatagrams());
    do {
        QCOMPARE(m_asyncReceiver->pendingDatagramSize(), qint64(1));
        QCOMPARE(m_asyncReceiver->readDatagram(buf, sizeof(buf)), qint64(1));
        if (buf[0] == '1') {
            // wait for the second datagram message.
            QTest::qSleep(100);
        }
    } while (m_asyncReceiver->hasPendingDatagrams());

    if (buf[0] == '3')
        QTestEventLoop::instance().exitLoop();
}

void tst_QSocketNotifier::async_writeDatagramSlot()
{
    m_asyncSender->writeDatagram("3", makeNonAny(m_asyncReceiver->localAddress()),
                                                 m_asyncReceiver->localPort());
}

void tst_QSocketNotifier::asyncMultipleDatagram()
{
    m_asyncSender = new QUdpSocket;
    m_asyncReceiver = new QUdpSocket;

    QVERIFY(m_asyncReceiver->bind(QHostAddress(QHostAddress::AnyIPv4), 0));
    quint16 port = m_asyncReceiver->localPort();
    QVERIFY(port != 0);

    QSignalSpy spy(m_asyncReceiver, &QIODevice::readyRead);
    connect(m_asyncReceiver, &QIODevice::readyRead, this,
            &tst_QSocketNotifier::async_readDatagramSlot);

    // activate socket notifiers
    QTestEventLoop::instance().enterLoop(100ms);

    m_asyncSender->writeDatagram("1", makeNonAny(m_asyncReceiver->localAddress()), port);
    m_asyncSender->writeDatagram("2", makeNonAny(m_asyncReceiver->localAddress()), port);
    // wait a little to ensure that the datagrams we've just sent
    // will be delivered on receiver side.
    QTest::qSleep(100);
    QVERIFY(m_asyncReceiver->hasPendingDatagrams());

    QTimer::singleShot(500, this, &tst_QSocketNotifier::async_writeDatagramSlot);

    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(spy.size(), 2);

    delete m_asyncSender;
    delete m_asyncReceiver;
}

void tst_QSocketNotifier::activationReason_data()
{
    QTest::addColumn<QSocketNotifier::Type>("type");
    QTest::addRow("read") << QSocketNotifier::Read;
    QTest::addRow("write") << QSocketNotifier::Write;
    QTest::addRow("exception") << QSocketNotifier::Exception;
}
void tst_QSocketNotifier::activationReason()
{
    QSocketDescriptor fd = 15;

    QFETCH(QSocketNotifier::Type, type);

    QSocketNotifier notifier(fd, type);
    auto activation = new QEvent(QEvent::SockAct);
    QCoreApplication::postEvent(&notifier, activation);

    QSocketNotifier::Type notifierType;
    connect(&notifier, &QSocketNotifier::activated, this,
            [&notifierType, fd](QSocketDescriptor sockfd, QSocketNotifier::Type sntype) {
                if (sockfd == fd)
                    notifierType = sntype;
                else
                    qWarning() << "Got an unexpected socket file descriptor:" << qintptr(sockfd);
            });

    QCoreApplication::processEvents();
    QCOMPARE(notifierType, type);
}

// This test ensures that we can connect QSocketNotifier::activated to a slot taking an integer
// or qintptr.
void tst_QSocketNotifier::legacyConnect()
{
    qintptr fd = 15;
    QSocketNotifier notifier(fd, QSocketNotifier::Read);
    auto activation = new QEvent(QEvent::SockAct);
    QCoreApplication::postEvent(&notifier, activation);

    bool receivedQIntPtr = false;
    connect(&notifier, &QSocketNotifier::activated, this, [&receivedQIntPtr, fd](qintptr q){
        if (q == fd)
            receivedQIntPtr = true;
    });
    bool receivedInt = false;
    connect(&notifier, &QSocketNotifier::activated, this, [&receivedInt, fd](int q){
        if (q == fd)
            receivedInt = true;
    });

    QCoreApplication::processEvents();
    QVERIFY(receivedQIntPtr);
    QVERIFY(receivedInt);
}


QTEST_MAIN(tst_QSocketNotifier)
#include <tst_qsocketnotifier.moc>
