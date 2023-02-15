// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2021 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtTest/qtesteventloop.h>

#include <QtCore/qglobal.h>
#include <QtCore/qthread.h>
#include <QtCore/qsemaphore.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qvector.h>
#include <QtCore/qelapsedtimer.h>
#include <QtNetwork/qlocalsocket.h>
#include <QtNetwork/qlocalserver.h>

using namespace std::chrono_literals;

class tst_QLocalSocket : public QObject
{
    Q_OBJECT

private slots:
    void pingPong_data();
    void pingPong();
    void dataExchange_data();
    void dataExchange();
};

class ServerThread : public QThread
{
public:
    QSemaphore running;

    explicit ServerThread(int chunkSize)
    {
        buffer.resize(chunkSize);
    }

    void run() override
    {
        QLocalServer server;

        connect(&server, &QLocalServer::newConnection, [this, &server]() {
            auto socket = server.nextPendingConnection();

            connect(socket, &QLocalSocket::readyRead, [this, socket]() {
                const qint64 bytesAvailable = socket->bytesAvailable();
                Q_ASSERT(bytesAvailable <= this->buffer.size());

                QCOMPARE(socket->read(this->buffer.data(), bytesAvailable), bytesAvailable);
                QCOMPARE(socket->write(this->buffer.data(), bytesAvailable), bytesAvailable);
            });
        });

        // TODO QTBUG-95136: on failure, remove the socket file and retry.
        QVERIFY2(server.listen("foo"), qPrintable(server.errorString()));

        running.release();
        exec();
    }

protected:
    QByteArray buffer;
};

class SocketFactory : public QObject
{
    Q_OBJECT

public:
    bool stopped = false;

    explicit SocketFactory(int chunkSize, int connections)
    {
        buffer.resize(chunkSize);
        for (int i = 0; i < connections; ++i) {
            QLocalSocket *socket = new QLocalSocket(this);
            Q_CHECK_PTR(socket);

            connect(this, &SocketFactory::start, [this, socket]() {
               QCOMPARE(socket->write(this->buffer), this->buffer.size());
            });

            connect(socket, &QLocalSocket::readyRead, [i, this, socket]() {
                const qint64 bytesAvailable = socket->bytesAvailable();
                Q_ASSERT(bytesAvailable <= this->buffer.size());

                QCOMPARE(socket->read(this->buffer.data(), bytesAvailable), bytesAvailable);
                emit this->bytesReceived(i, bytesAvailable);

                if (!this->stopped)
                    QCOMPARE(socket->write(this->buffer.data(), bytesAvailable), bytesAvailable);
            });

            socket->connectToServer("foo");
            QVERIFY2(socket->waitForConnected(), "The system is probably reaching the maximum "
                                                 "number of open file descriptors. On Unix, "
                                                 "try to increase the limit with 'ulimit -n 32000' "
                                                 "and run the test again.");
            QCOMPARE(socket->state(), QLocalSocket::ConnectedState);
        }
    }

signals:
    void start();
    void bytesReceived(int channel, qint64 bytes);

protected:
    QByteArray buffer;
};

void tst_QLocalSocket::pingPong_data()
{
    QTest::addColumn<int>("connections");
    for (int value : {10, 50, 100, 1000, 5000})
        QTest::addRow("connections: %d", value) << value;
}

void tst_QLocalSocket::pingPong()
{
    QFETCH(int, connections);

    const int iterations = 100000;
    Q_ASSERT(iterations >= connections && connections > 0);

    ServerThread serverThread(1);
    serverThread.start();
    // Wait for server to start.
    QVERIFY(serverThread.running.tryAcquire(1, 3000));

    SocketFactory factory(1, connections);
    QTestEventLoop eventLoop;
    QVector<qint64> bytesToRead;
    QElapsedTimer timer;

    bytesToRead.fill(iterations / connections, connections);
    connect(&factory, &SocketFactory::bytesReceived,
            [&bytesToRead, &connections, &factory, &eventLoop](int channel, qint64 bytes) {
        Q_UNUSED(bytes);

        if (--bytesToRead[channel] == 0 && --connections == 0) {
            factory.stopped = true;
            eventLoop.exitLoop();
        }
    });

    timer.start();
    emit factory.start();
    // QtTestLib's Watchdog defaults to 300 seconds; we want to give up before
    // it bites.
    eventLoop.enterLoop(290);

    if (eventLoop.timeout())
        qDebug("Timed out after %.1f s", timer.elapsed() / 1000.0);
    else if (!QTest::currentTestFailed())
        qDebug("Elapsed time: %.1f s", timer.elapsed() / 1000.0);
    serverThread.quit();
    serverThread.wait();
}

void tst_QLocalSocket::dataExchange_data()
{
    QTest::addColumn<int>("connections");
    QTest::addColumn<int>("chunkSize");
    for (int connections : {1, 5, 10}) {
        for (int chunkSize : {100, 1000, 10000, 100000}) {
            QTest::addRow("connections: %d, chunk size: %d",
                          connections, chunkSize) << connections << chunkSize;
        }
    }
}

void tst_QLocalSocket::dataExchange()
{
    QFETCH(int, connections);
    QFETCH(int, chunkSize);

    Q_ASSERT(chunkSize > 0 && connections > 0);
    const auto timeToTest = 5000ms;

    ServerThread serverThread(chunkSize);
    serverThread.start();
    // Wait for server to start.
    QVERIFY(serverThread.running.tryAcquire(1, 3000));

    SocketFactory factory(chunkSize, connections);
    QTestEventLoop eventLoop;
    qint64 totalReceived = 0;
    QElapsedTimer timer;

    connect(&factory, &SocketFactory::bytesReceived, [&](int channel, qint64 bytes) {
        Q_UNUSED(channel);

        totalReceived += bytes;
        if (timer.elapsed() >= timeToTest.count()) {
            factory.stopped = true;
            eventLoop.exitLoop();
        }
    });

    timer.start();
    emit factory.start();
    eventLoop.enterLoop(timeToTest * 2);

    if (!QTest::currentTestFailed())
        qDebug("Transfer rate: %.1f MB/s", totalReceived / 1048.576 / timer.elapsed());
    serverThread.quit();
    serverThread.wait();
}

QTEST_MAIN(tst_QLocalSocket)

#include "tst_qlocalsocket.moc"
