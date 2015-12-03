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

#include <qtextstream.h>
#include <QtNetwork/qlocalsocket.h>
#include <QtNetwork/qlocalserver.h>

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h> // for unlink()
#endif

Q_DECLARE_METATYPE(QLocalSocket::LocalSocketError)
Q_DECLARE_METATYPE(QLocalSocket::LocalSocketState)
Q_DECLARE_METATYPE(QLocalServer::SocketOption)
Q_DECLARE_METATYPE(QFile::Permissions)

class tst_QLocalSocket : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void init();
    void cleanup();

private slots:
    // basics
    void server_basic();
    void server_connectionsCount();
    void socket_basic();

    void listen_data();
    void listen();

    void listenAndConnect_data();
    void listenAndConnect();

    void connectWithOpen();
    void connectWithOldOpen();

    void sendData_data();
    void sendData();

    void readBufferOverflow();

    void fullPath();

    void hitMaximumConnections_data();
    void hitMaximumConnections();

    void setSocketDescriptor();

    void threadedConnection_data();
    void threadedConnection();

    void processConnection_data();
    void processConnection();

    void longPath();
    void waitForDisconnect();
    void waitForDisconnectByServer();

    void removeServer();

    void recycleServer();
    void recycleClientSocket();

    void multiConnect();
    void writeOnlySocket();
    void writeToClientAndDisconnect();
    void debug();
    void bytesWrittenSignal();
    void syncDisconnectNotify();
    void asyncDisconnectNotify();

    void verifySocketOptions();
    void verifySocketOptions_data();

    void verifyListenWithDescriptor();
    void verifyListenWithDescriptor_data();

};

void tst_QLocalSocket::init()
{
    qRegisterMetaType<QLocalSocket::LocalSocketState>("QLocalSocket::LocalSocketState");
    qRegisterMetaType<QLocalSocket::LocalSocketError>("QLocalSocket::LocalSocketError");
    qRegisterMetaType<QLocalServer::SocketOption>("QLocalServer::SocketOption");
    qRegisterMetaType<QFile::Permissions>("QFile::Permissions");
}

void tst_QLocalSocket::cleanup()
{
}

class LocalServer : public QLocalServer
{
    Q_OBJECT

public:
    LocalServer() : QLocalServer()
    {
        connect(this, SIGNAL(newConnection()), this, SLOT(slotNewConnection()));
    }

    bool listen(const QString &name)
    {
        removeServer(name);
        return QLocalServer::listen(name);
    }

    QList<int> hits;

protected:
    void incomingConnection(quintptr socketDescriptor)
    {
        hits.append(socketDescriptor);
        QLocalServer::incomingConnection(socketDescriptor);
    }

private slots:
    void slotNewConnection() {
        QVERIFY(!hits.isEmpty());
        QVERIFY(hasPendingConnections());
    }
};

class LocalSocket : public QLocalSocket
{
    Q_OBJECT

public:
    LocalSocket(QObject *parent = 0) : QLocalSocket(parent)
    {
        connect(this, SIGNAL(connected()),
                this, SLOT(slotConnected()));
        connect(this, SIGNAL(disconnected()),
                this, SLOT(slotDisconnected()));
        connect(this, SIGNAL(error(QLocalSocket::LocalSocketError)),
                this, SLOT(slotError(QLocalSocket::LocalSocketError)));
        connect(this, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)),
                this, SLOT(slotStateChanged(QLocalSocket::LocalSocketState)));
        connect(this, SIGNAL(readyRead()),
                this, SLOT(slotReadyRead()));
    }

private slots:
    void slotConnected()
    {
        QCOMPARE(state(), QLocalSocket::ConnectedState);
    }
    void slotDisconnected()
    {
        QCOMPARE(state(), QLocalSocket::UnconnectedState);
    }
    void slotError(QLocalSocket::LocalSocketError newError)
    {
        QVERIFY(errorString() != "Unknown error");
        QCOMPARE(error(), newError);
    }
    void slotStateChanged(QLocalSocket::LocalSocketState newState)
    {
        QCOMPARE(state(), newState);
    }
    void slotReadyRead()
    {
        QVERIFY(bytesAvailable() > 0);
    }
};

// basic test make sure no segfaults and check default values
void tst_QLocalSocket::server_basic()
{
    LocalServer server;
    QSignalSpy spyNewConnection(&server, SIGNAL(newConnection()));
    server.close();
    QCOMPARE(server.errorString(), QString());
    QCOMPARE(server.hasPendingConnections(), false);
    QCOMPARE(server.isListening(), false);
    QCOMPARE(server.maxPendingConnections(), 30);
    QCOMPARE(server.nextPendingConnection(), (QLocalSocket*)0);
    QCOMPARE(server.serverName(), QString());
    QCOMPARE(server.fullServerName(), QString());
    QCOMPARE(server.serverError(), QAbstractSocket::UnknownSocketError);
    server.setMaxPendingConnections(20);
    bool timedOut = true;
    QCOMPARE(server.waitForNewConnection(3000, &timedOut), false);
    QVERIFY(!timedOut);
    QCOMPARE(server.listen(QString()), false);

    QCOMPARE(server.hits.count(), 0);
    QCOMPARE(spyNewConnection.count(), 0);
}

void tst_QLocalSocket::server_connectionsCount()
{
    LocalServer server;
    server.setMaxPendingConnections(10);
    QCOMPARE(server.maxPendingConnections(), 10);
}

// basic test make sure no segfaults and check default values
void tst_QLocalSocket::socket_basic()
{
    LocalSocket socket;
    QSignalSpy spyConnected(&socket, SIGNAL(connected()));
    QSignalSpy spyDisconnected(&socket, SIGNAL(disconnected()));
    QSignalSpy spyError(&socket, SIGNAL(error(QLocalSocket::LocalSocketError)));
    QSignalSpy spyStateChanged(&socket, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)));
    QSignalSpy spyReadyRead(&socket, SIGNAL(readyRead()));

    QCOMPARE(socket.serverName(), QString());
    QCOMPARE(socket.fullServerName(), QString());
    socket.abort();
    QCOMPARE(socket.bytesAvailable(), 0);
    QCOMPARE(socket.bytesToWrite(), 0);
    QCOMPARE(socket.canReadLine(), false);
    socket.close();
    socket.disconnectFromServer();
    QCOMPARE(QLocalSocket::UnknownSocketError, socket.error());
    QVERIFY(!socket.errorString().isEmpty());
    QCOMPARE(socket.flush(), false);
    QCOMPARE(socket.isValid(), false);
    QCOMPARE(socket.readBufferSize(), 0);
    socket.setReadBufferSize(0);
    //QCOMPARE(socket.socketDescriptor(), (qintptr)-1);
    QCOMPARE(socket.state(), QLocalSocket::UnconnectedState);
    QCOMPARE(socket.waitForConnected(0), false);
    QTest::ignoreMessage(QtWarningMsg, "QLocalSocket::waitForDisconnected() is not allowed in UnconnectedState");
    QCOMPARE(socket.waitForDisconnected(0), false);
    QCOMPARE(socket.waitForReadyRead(0), false);

    QCOMPARE(spyConnected.count(), 0);
    QCOMPARE(spyDisconnected.count(), 0);
    QCOMPARE(spyError.count(), 0);
    QCOMPARE(spyStateChanged.count(), 0);
    QCOMPARE(spyReadyRead.count(), 0);
}

void tst_QLocalSocket::listen_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("canListen");
    QTest::addColumn<bool>("close");
    QTest::newRow("null") << QString() << false << false;
    QTest::newRow("tst_localsocket") << "tst_localsocket" << true << true;
    QTest::newRow("tst_localsocket") << "tst_localsocket" << true << false;
}

// start a server that listens, but don't connect a socket, make sure everything is in order
void tst_QLocalSocket::listen()
{
    LocalServer server;
    QSignalSpy spyNewConnection(&server, SIGNAL(newConnection()));

    QFETCH(QString, name);
    QFETCH(bool, canListen);
    QFETCH(bool, close);
    QVERIFY2((server.listen(name) == canListen), server.errorString().toLatin1().constData());

    // test listening
    QCOMPARE(server.serverName(), name);
    QVERIFY(server.fullServerName().contains(name));
    QCOMPARE(server.isListening(), canListen);
    QCOMPARE(server.hasPendingConnections(), false);
    QCOMPARE(server.nextPendingConnection(), (QLocalSocket*)0);
    QCOMPARE(server.hits.count(), 0);
    QCOMPARE(spyNewConnection.count(), 0);
    if (canListen) {
        QVERIFY(server.errorString().isEmpty());
        QCOMPARE(server.serverError(), QAbstractSocket::UnknownSocketError);
        // already isListening
        QTest::ignoreMessage(QtWarningMsg, "QLocalServer::listen() called when already listening");
        QVERIFY(!server.listen(name));
    } else {
        QVERIFY(!server.errorString().isEmpty());
        QCOMPARE(server.serverError(), QAbstractSocket::HostNotFoundError);
    }
    QCOMPARE(server.maxPendingConnections(), 30);
    bool timedOut = false;
    QCOMPARE(server.waitForNewConnection(3000, &timedOut), false);
    QCOMPARE(timedOut, canListen);
    if (close)
        server.close();
}

void tst_QLocalSocket::listenAndConnect_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("canListen");
    QTest::addColumn<int>("connections");
    for (int i = 0; i < 3; ++i) {
        int connections = i;
        if (i == 2)
            connections = 5;
        QTest::newRow(QString("null %1").arg(i).toLatin1()) << QString() << false << connections;
        QTest::newRow(QString("tst_localsocket %1").arg(i).toLatin1()) << "tst_localsocket" << true << connections;
    }
}

void tst_QLocalSocket::listenAndConnect()
{
    LocalServer server;
    QSignalSpy spyNewConnection(&server, SIGNAL(newConnection()));

    QFETCH(QString, name);
    QFETCH(bool, canListen);
    QCOMPARE(server.listen(name), canListen);
    QTRY_COMPARE(server.serverError(),
                 canListen ? QAbstractSocket::UnknownSocketError : QAbstractSocket::HostNotFoundError);

    // test creating connection(s)
    QFETCH(int, connections);
    QList<QLocalSocket*> sockets;
    for (int i = 0; i < connections; ++i) {
        LocalSocket *socket = new LocalSocket;

        QSignalSpy spyConnected(socket, SIGNAL(connected()));
        QSignalSpy spyDisconnected(socket, SIGNAL(disconnected()));
        QSignalSpy spyError(socket, SIGNAL(error(QLocalSocket::LocalSocketError)));
        QSignalSpy spyStateChanged(socket, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)));
        QSignalSpy spyReadyRead(socket, SIGNAL(readyRead()));

        socket->connectToServer(name);
#if defined(QT_LOCALSOCKET_TCP)
        QTest::qWait(250);
#endif

        QCOMPARE(socket->serverName(), name);
        QVERIFY(socket->fullServerName().contains(name));
        sockets.append(socket);
        if (canListen) {
            QVERIFY(socket->waitForConnected());
            QVERIFY(socket->isValid());
            QCOMPARE(socket->errorString(), QString("Unknown error"));
            QCOMPARE(socket->error(), QLocalSocket::UnknownSocketError);
            QCOMPARE(socket->state(), QLocalSocket::ConnectedState);
            //QVERIFY(socket->socketDescriptor() != -1);
            QCOMPARE(spyError.count(), 0);
        } else {
            QVERIFY(!socket->errorString().isEmpty());
            QVERIFY(socket->error() != QLocalSocket::UnknownSocketError);
            QCOMPARE(socket->state(), QLocalSocket::UnconnectedState);
            //QCOMPARE(socket->socketDescriptor(), -1);
            QCOMPARE(qvariant_cast<QLocalSocket::LocalSocketError>(spyError.first()[0]),
                     QLocalSocket::ServerNotFoundError);
        }

        QCOMPARE(socket->bytesAvailable(), 0);
        QCOMPARE(socket->bytesToWrite(), 0);
        QCOMPARE(socket->canReadLine(), false);
        QCOMPARE(socket->flush(), false);
        QCOMPARE(socket->isValid(), canListen);
        QCOMPARE(socket->readBufferSize(), (qint64)0);
        QCOMPARE(socket->waitForConnected(0), canListen);
        QCOMPARE(socket->waitForReadyRead(0), false);

        QTRY_COMPARE(spyConnected.count(), canListen ? 1 : 0);
        QCOMPARE(spyDisconnected.count(), 0);

        // error signals
        QVERIFY(spyError.count() >= 0);
        if (canListen) {
            if (spyError.count() > 0)
                QCOMPARE(qvariant_cast<QLocalSocket::LocalSocketError>(spyError.first()[0]),
                         QLocalSocket::SocketTimeoutError);
        } else {
            QCOMPARE(qvariant_cast<QLocalSocket::LocalSocketError>(spyError.first()[0]),
                     QLocalSocket::ServerNotFoundError);
        }

        // Check first and last state
        QCOMPARE(qvariant_cast<QLocalSocket::LocalSocketState>(spyStateChanged.first()[0]),
                 QLocalSocket::ConnectingState);

        if (canListen)
            QCOMPARE(qvariant_cast<QLocalSocket::LocalSocketState>(spyStateChanged.last()[0]),
                     QLocalSocket::ConnectedState);
        QCOMPARE(spyStateChanged.count(), 2);
        QCOMPARE(spyReadyRead.count(), 0);

        bool timedOut = true;
        QCOMPARE(server.waitForNewConnection(3000, &timedOut), canListen);
        QVERIFY(!timedOut);
        QCOMPARE(server.hasPendingConnections(), canListen);
        QCOMPARE(server.isListening(), canListen);
        // NOTE: socket disconnecting is not tested here

        // server checks post connection
        if (canListen) {
            QCOMPARE(server.serverName(), name);
            QVERIFY(server.fullServerName().contains(name));
            QVERIFY(server.nextPendingConnection() != (QLocalSocket*)0);
            QTRY_COMPARE(server.hits.count(), i + 1);
            QCOMPARE(spyNewConnection.count(), i + 1);
            QVERIFY(server.errorString().isEmpty());
            QCOMPARE(server.serverError(), QAbstractSocket::UnknownSocketError);
        } else {
            QVERIFY(server.serverName().isEmpty());
            QVERIFY(server.fullServerName().isEmpty());
            QCOMPARE(server.nextPendingConnection(), (QLocalSocket*)0);
            QCOMPARE(spyNewConnection.count(), 0);
            QCOMPARE(server.hits.count(), 0);
            QVERIFY(!server.errorString().isEmpty());
            QCOMPARE(server.serverError(), QAbstractSocket::HostNotFoundError);
        }
    }
    qDeleteAll(sockets.begin(), sockets.end());

    server.close();

    QCOMPARE(server.hits.count(), (canListen ? connections : 0));
    QCOMPARE(spyNewConnection.count(), (canListen ? connections : 0));
}

void tst_QLocalSocket::connectWithOpen()
{
    LocalServer server;
    QVERIFY(server.listen("tst_qlocalsocket"));

    LocalSocket socket;
    socket.setServerName("tst_qlocalsocket");
    QVERIFY(socket.open());

    bool timedOut = true;
    QVERIFY(server.waitForNewConnection(3000, &timedOut));

#if defined(QT_LOCALSOCKET_TCP)
    QTest::qWait(250);
#endif
    QVERIFY(!timedOut);

    socket.close();
    server.close();
}

void tst_QLocalSocket::connectWithOldOpen()
{
    class OverriddenOpen : public LocalSocket
    {
    public:
        virtual bool open(OpenMode mode) Q_DECL_OVERRIDE
        { return QIODevice::open(mode); }
    };

    LocalServer server;
    QCOMPARE(server.listen("tst_qlocalsocket"), true);

    OverriddenOpen socket;
    socket.connectToServer("tst_qlocalsocket");

    bool timedOut = true;
    QVERIFY(server.waitForNewConnection(3000, &timedOut));

#if defined(QT_LOCALSOCKET_TCP)
    QTest::qWait(250);
#endif
    QVERIFY(!timedOut);

    socket.close();
    server.close();
}

void tst_QLocalSocket::sendData_data()
{
    listenAndConnect_data();
}

void tst_QLocalSocket::sendData()
{
    QFETCH(QString, name);
    QFETCH(bool, canListen);

    LocalServer server;
    QSignalSpy spy(&server, SIGNAL(newConnection()));

    QCOMPARE(server.listen(name), canListen);

    LocalSocket socket;
    QSignalSpy spyConnected(&socket, SIGNAL(connected()));
    QSignalSpy spyDisconnected(&socket, SIGNAL(disconnected()));
    QSignalSpy spyError(&socket, SIGNAL(error(QLocalSocket::LocalSocketError)));
    QSignalSpy spyStateChanged(&socket, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)));
    QSignalSpy spyReadyRead(&socket, SIGNAL(readyRead()));

    // test creating a connection
    socket.connectToServer(name);
    bool timedOut = true;

    QCOMPARE(server.waitForNewConnection(3000, &timedOut), canListen);

#if defined(QT_LOCALSOCKET_TCP)
    QTest::qWait(250);
#endif
    QVERIFY(!timedOut);
    QCOMPARE(spyConnected.count(), canListen ? 1 : 0);
    QCOMPARE(socket.state(), canListen ? QLocalSocket::ConnectedState : QLocalSocket::UnconnectedState);

    // test sending/receiving data
    if (server.hasPendingConnections()) {
        QString testLine = "test";
        for (int i = 0; i < 50000; ++i)
            testLine += "a";
        QLocalSocket *serverSocket = server.nextPendingConnection();
        QVERIFY(serverSocket);
        QCOMPARE(serverSocket->state(), QLocalSocket::ConnectedState);
        QTextStream out(serverSocket);
        QTextStream in(&socket);
        out << testLine << endl;
        bool wrote = serverSocket->waitForBytesWritten(3000);

        if (!socket.canReadLine())
            QVERIFY(socket.waitForReadyRead());

        QVERIFY(socket.bytesAvailable() >= 0);
        QCOMPARE(socket.bytesToWrite(), (qint64)0);
        QCOMPARE(socket.flush(), false);
        QCOMPARE(socket.isValid(), canListen);
        QCOMPARE(socket.readBufferSize(), (qint64)0);
        QCOMPARE(spyReadyRead.count(), 1);

        QVERIFY(testLine.startsWith(in.readLine()));

        QVERIFY(wrote || serverSocket->waitForBytesWritten(1000));

        QCOMPARE(serverSocket->errorString(), QString("Unknown error"));
        QCOMPARE(socket.errorString(), QString("Unknown error"));
    }

    socket.disconnectFromServer();
    QCOMPARE(spyConnected.count(), canListen ? 1 : 0);
    QCOMPARE(spyDisconnected.count(), canListen ? 1 : 0);
    QCOMPARE(spyError.count(), canListen ? 0 : 1);
    QCOMPARE(spyStateChanged.count(), canListen ? 4 : 2);
    QCOMPARE(spyReadyRead.count(), canListen ? 1 : 0);

    server.close();

    QCOMPARE(server.hits.count(), (canListen ? 1 : 0));
    QCOMPARE(spy.count(), (canListen ? 1 : 0));
}

void tst_QLocalSocket::readBufferOverflow()
{
    const int readBufferSize = 128;
    const int dataBufferSize = readBufferSize * 2;
    const QString serverName = QLatin1String("myPreciousTestServer");
    LocalServer server;
    server.listen(serverName);
    QVERIFY(server.isListening());

    LocalSocket client;
    client.setReadBufferSize(readBufferSize);
    client.connectToServer(serverName);

    bool timedOut = true;
    QVERIFY(server.waitForNewConnection(3000, &timedOut));
    QVERIFY(!timedOut);

    QCOMPARE(client.state(), QLocalSocket::ConnectedState);
    QVERIFY(server.hasPendingConnections());

    QLocalSocket* serverSocket = server.nextPendingConnection();
    char buffer[dataBufferSize];
    memset(buffer, 0, dataBufferSize);
    serverSocket->write(buffer, dataBufferSize);
#ifndef Q_OS_WIN
    // The data is not immediately sent, but buffered.
    // On Windows, the flushing is done asynchronously by a separate thread.
    // However, this operation will never complete as long as the data is not
    // read by the other end, so the call below always times out.
    // On Unix, the flushing is synchronous and thus needs to be done before
    // attempting to read the data in the same thread. Buffering by the OS
    // prevents the deadlock seen on Windows.
    serverSocket->waitForBytesWritten();
#endif

    // wait until the first 128 bytes are ready to read
    QVERIFY(client.waitForReadyRead());
    QCOMPARE(client.read(buffer, readBufferSize), qint64(readBufferSize));
    // wait until the second 128 bytes are ready to read
    QVERIFY(client.waitForReadyRead());
    QCOMPARE(client.read(buffer, readBufferSize), qint64(readBufferSize));
    // no more bytes available
    QCOMPARE(client.bytesAvailable(), 0);
}

// QLocalSocket/Server can take a name or path, check that it works as expected
void tst_QLocalSocket::fullPath()
{
    QLocalServer server;
    QString name = "qlocalsocket_pathtest";
#if defined(QT_LOCALSOCKET_TCP)
    QString path = "QLocalServer";
#elif defined(Q_OS_WIN)
    QString path = "\\\\.\\pipe\\";
#else
    QString path = "/tmp";
#endif
    QString serverName = path + '/' + name;
    QVERIFY2(server.listen(serverName), server.errorString().toLatin1().constData());
    QCOMPARE(server.serverName(), serverName);
    QCOMPARE(server.fullServerName(), serverName);

    LocalSocket socket;
    socket.connectToServer(serverName);

    QCOMPARE(socket.serverName(), serverName);
    QCOMPARE(socket.fullServerName(), serverName);
    socket.disconnectFromServer();
#ifdef QT_LOCALSOCKET_TCP
    QTest::qWait(250);
#endif
    QCOMPARE(socket.serverName(), QString());
    QCOMPARE(socket.fullServerName(), QString());
}

void tst_QLocalSocket::hitMaximumConnections_data()
{
    QTest::addColumn<int>("max");
    QTest::newRow("none") << 0;
    QTest::newRow("1") << 1;
    QTest::newRow("3") << 3;
}

void tst_QLocalSocket::hitMaximumConnections()
{
    QFETCH(int, max);
    LocalServer server;
    QString name = "tst_localsocket";
    server.setMaxPendingConnections(max);
    QVERIFY2(server.listen(name), server.errorString().toLatin1().constData());
    int connections = server.maxPendingConnections() + 1;
    QList<QLocalSocket*> sockets;
    for (int i = 0; i < connections; ++i) {
        LocalSocket *socket = new LocalSocket;
        sockets.append(socket);
        socket->connectToServer(name);
    }
   bool timedOut = true;
   QVERIFY(server.waitForNewConnection(3000, &timedOut));
   QVERIFY(!timedOut);
   QVERIFY(server.hits.count() > 0);
   qDeleteAll(sockets.begin(), sockets.end());
}

// check that state and mode are kept
void tst_QLocalSocket::setSocketDescriptor()
{
    LocalSocket socket;
    qintptr minusOne = -1;
    socket.setSocketDescriptor(minusOne, QLocalSocket::ConnectingState, QIODevice::Append);
    QCOMPARE(socket.socketDescriptor(), minusOne);
    QCOMPARE(socket.state(), QLocalSocket::ConnectingState);
    QVERIFY((socket.openMode() & QIODevice::Append) != 0);
}

class Client : public QThread
{

public:
    void run()
    {
        QString testLine = "test";
        LocalSocket socket;
        QSignalSpy spyReadyRead(&socket, SIGNAL(readyRead()));
        socket.connectToServer("qlocalsocket_threadtest");
        QVERIFY(socket.waitForConnected(1000));

        // We should *not* have this signal yet!
        QCOMPARE(spyReadyRead.count(), 0);
        socket.waitForReadyRead();
        QCOMPARE(spyReadyRead.count(), 1);
        QTextStream in(&socket);
        QCOMPARE(in.readLine(), testLine);
        socket.close();
    }
};

class Server : public QThread
{

public:
    int clients;
    QMutex mutex;
    QWaitCondition wc;
    void run()
    {
        QString testLine = "test";
        LocalServer server;
        server.setMaxPendingConnections(10);
        QVERIFY2(server.listen("qlocalsocket_threadtest"),
                 server.errorString().toLatin1().constData());
        mutex.lock();
        wc.wakeAll();
        mutex.unlock();
        int done = clients;
        while (done > 0) {
            bool timedOut = true;
            QVERIFY2(server.waitForNewConnection(7000, &timedOut),
                     (QByteArrayLiteral("done=") + QByteArray::number(done)
                      + QByteArrayLiteral(", timedOut=")
                      + (timedOut ? "true" : "false")).constData());
            QVERIFY(!timedOut);
            QLocalSocket *serverSocket = server.nextPendingConnection();
            QVERIFY(serverSocket);
            QTextStream out(serverSocket);
            out << testLine << endl;
            QCOMPARE(serverSocket->state(), QLocalSocket::ConnectedState);
            QVERIFY2(serverSocket->waitForBytesWritten(), serverSocket->errorString().toLatin1().constData());
            QCOMPARE(serverSocket->errorString(), QString("Unknown error"));
            --done;
            delete serverSocket;
        }
        QCOMPARE(server.hits.count(), clients);
    }
};

void tst_QLocalSocket::threadedConnection_data()
{
    QTest::addColumn<int>("threads");
    QTest::newRow("1 client") << 1;
    QTest::newRow("2 clients") << 2;
    QTest::newRow("5 clients") << 5;
#ifndef Q_OS_WINCE
    QTest::newRow("10 clients") << 10;
    QTest::newRow("20 clients") << 20;
#endif
}

void tst_QLocalSocket::threadedConnection()
{
    QFETCH(int, threads);
    Server server;
    server.clients = threads;
    server.mutex.lock();
    server.start();
    server.wc.wait(&server.mutex);
    server.mutex.unlock();

    QList<Client*> clients;
    for (int i = 0; i < threads; ++i) {
        clients.append(new Client());
        clients.last()->start();
    }

    server.wait();
    while (!clients.isEmpty()) {
        QVERIFY(clients.first()->wait(3000));
        delete clients.takeFirst();
    }
}

void tst_QLocalSocket::processConnection_data()
{
    QTest::addColumn<int>("processes");
    QTest::newRow("1 client") << 1;
    QTest::newRow("2 clients") << 2;
    QTest::newRow("5 clients") << 5;
    QTest::newRow("30 clients") << 30;
}

#ifndef QT_NO_PROCESS
class ProcessOutputDumper
{
public:
    ProcessOutputDumper(QProcess *p = 0)
        : process(p)
    {}

    ~ProcessOutputDumper()
    {
        if (process)
            fputs(process->readAll().data(), stdout);
    }

    void clear()
    {
        process = 0;
    }

private:
    QProcess *process;
};
#endif

/*!
    Create external processes that produce and consume.
 */
void tst_QLocalSocket::processConnection()
{
#ifdef QT_NO_PROCESS
    QSKIP("No qprocess support", SkipAll);
#else
#ifdef Q_OS_MAC
    QSKIP("The processConnection test is unstable on Mac. See QTBUG-39986.");
#endif

#ifdef Q_OS_WIN
    const QString exeSuffix = QStringLiteral(".exe");
#else
    const QString exeSuffix;
#endif

    QString socketProcess = QStringLiteral("socketprocess/socketprocess") + exeSuffix;
    QVERIFY(QFile::exists(socketProcess));

    QFETCH(int, processes);
    QStringList serverArguments = QStringList() << "--server" << QString::number(processes);
    QProcess producer;
    ProcessOutputDumper producerOutputDumper(&producer);
    QList<QProcess*> consumers;
    producer.start(socketProcess, serverArguments);
    QVERIFY2(producer.waitForStarted(-1), qPrintable(producer.errorString()));
    for (int i = 0; i < processes; ++i) {
        QStringList arguments = QStringList() << "--client";
        QProcess *p = new QProcess;
        consumers.append(p);
        p->start(socketProcess, arguments);
    }

    while (!consumers.isEmpty()) {
        QProcess *consumer = consumers.takeFirst();
        ProcessOutputDumper consumerOutputDumper(consumer);
        consumer->waitForFinished(20000);
        QCOMPARE(consumer->exitStatus(), QProcess::NormalExit);
        QCOMPARE(consumer->exitCode(), 0);
        consumerOutputDumper.clear();
        consumer->terminate();
        delete consumer;
    }
    producer.waitForFinished(15000);
    producerOutputDumper.clear();
#endif
}

void tst_QLocalSocket::longPath()
{
#ifndef Q_OS_WIN
    QString name;
    for (int i = 0; i < 256; ++i)
        name += 'a';
    LocalServer server;
    QVERIFY(!server.listen(name));

    LocalSocket socket;
    socket.connectToServer(name);
    QCOMPARE(socket.state(), QLocalSocket::UnconnectedState);
#endif
}

void tst_QLocalSocket::waitForDisconnect()
{
    QString name = "tst_localsocket";
    LocalServer server;
    QVERIFY(server.listen(name));
    LocalSocket socket;
    socket.connectToServer(name);
    QVERIFY(socket.waitForConnected(3000));
    QVERIFY(server.waitForNewConnection(3000));
    QLocalSocket *serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    socket.disconnectFromServer();
    QTime timer;
    timer.start();
    QVERIFY(serverSocket->waitForDisconnected(3000));
    QVERIFY(timer.elapsed() < 2000);
}

void tst_QLocalSocket::waitForDisconnectByServer()
{
    QString name = "tst_localsocket";
    LocalServer server;
    QVERIFY(server.listen(name));
    LocalSocket socket;
    QSignalSpy spy(&socket, SIGNAL(disconnected()));
    QVERIFY(spy.isValid());
    socket.connectToServer(name);
    QVERIFY(socket.waitForConnected(3000));
    QVERIFY(server.waitForNewConnection(3000));
    QLocalSocket *serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    serverSocket->close();
    QCOMPARE(serverSocket->state(), QLocalSocket::UnconnectedState);
    QVERIFY(socket.waitForDisconnected(3000));
    QCOMPARE(spy.count(), 1);
}

void tst_QLocalSocket::removeServer()
{
    // this is a hostile takeover, but recovering from a crash results in the same
    QLocalServer server, server2;
    QVERIFY(QLocalServer::removeServer("cleanuptest"));
    QVERIFY(server.listen("cleanuptest"));
#ifndef Q_OS_WIN
    // on Windows, there can be several sockets listening on the same pipe
    // on Unix, there can only be one socket instance
    QVERIFY(! server2.listen("cleanuptest"));
#endif
    QVERIFY(QLocalServer::removeServer("cleanuptest"));
    QVERIFY(server2.listen("cleanuptest"));
}

void tst_QLocalSocket::recycleServer()
{
    QLocalServer server;
    QLocalSocket client;

    QVERIFY(server.listen("recycletest1"));
    client.connectToServer("recycletest1");
    QVERIFY(client.waitForConnected(201));
    QVERIFY(server.waitForNewConnection(201));
    QVERIFY(server.nextPendingConnection() != 0);

    server.close();
    client.disconnectFromServer();
    qApp->processEvents();

    QVERIFY(server.listen("recycletest2"));
    client.connectToServer("recycletest2");
    QVERIFY(client.waitForConnected(202));
    QVERIFY(server.waitForNewConnection(202));
    QVERIFY(server.nextPendingConnection() != 0);
}

void tst_QLocalSocket::recycleClientSocket()
{
    const QByteArrayList lines = QByteArrayList() << "Have you heard of that new band"
                                                  << "\"1023 Megabytes\"?"
                                                  << "They haven't made it to a gig yet.";
    QLocalServer server;
    const QString serverName = QStringLiteral("recycleClientSocket");
    QVERIFY(server.listen(serverName));
    QLocalSocket client;
    QSignalSpy clientReadyReadSpy(&client, SIGNAL(readyRead()));
    QSignalSpy clientErrorSpy(&client, SIGNAL(error(QLocalSocket::LocalSocketError)));
    for (int i = 0; i < lines.count(); ++i) {
        client.abort();
        clientReadyReadSpy.clear();
        client.connectToServer(serverName);
        QVERIFY(client.waitForConnected());
        QVERIFY(server.waitForNewConnection());
        QLocalSocket *serverSocket = server.nextPendingConnection();
        QVERIFY(serverSocket);
        connect(serverSocket, &QLocalSocket::disconnected, &QLocalSocket::deleteLater);
        serverSocket->write(lines.at(i));
        serverSocket->flush();
        QVERIFY(clientReadyReadSpy.wait());
        QCOMPARE(client.readAll(), lines.at(i));
        QVERIFY(clientErrorSpy.isEmpty());
    }
}

void tst_QLocalSocket::multiConnect()
{
    QLocalServer server;
    QLocalSocket client1;
    QLocalSocket client2;
    QLocalSocket client3;

    QVERIFY(server.listen("multiconnect"));

    client1.connectToServer("multiconnect");
    client2.connectToServer("multiconnect");
    client3.connectToServer("multiconnect");

    QVERIFY(client1.waitForConnected(201));
    QVERIFY(client2.waitForConnected(202));
    QVERIFY(client3.waitForConnected(203));

    QVERIFY(server.waitForNewConnection(201));
    QVERIFY(server.nextPendingConnection() != 0);
    QVERIFY(server.waitForNewConnection(202));
    QVERIFY(server.nextPendingConnection() != 0);
    QVERIFY(server.waitForNewConnection(203));
    QVERIFY(server.nextPendingConnection() != 0);
}

void tst_QLocalSocket::writeOnlySocket()
{
    QLocalServer server;
    QVERIFY(server.listen("writeOnlySocket"));

    QLocalSocket client;
    client.connectToServer("writeOnlySocket", QIODevice::WriteOnly);
    QVERIFY(client.waitForConnected());
    QVERIFY(server.waitForNewConnection(200));
    QLocalSocket* serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);

    QCOMPARE(client.bytesAvailable(), qint64(0));
    QCOMPARE(client.state(), QLocalSocket::ConnectedState);
}

void tst_QLocalSocket::writeToClientAndDisconnect()
{
    QLocalServer server;
    QLocalSocket client;
    QSignalSpy readChannelFinishedSpy(&client, SIGNAL(readChannelFinished()));

    QVERIFY(server.listen("writeAndDisconnectServer"));
    client.connectToServer("writeAndDisconnectServer");
    QVERIFY(client.waitForConnected(200));
    QVERIFY(server.waitForNewConnection(200));
    QLocalSocket* clientSocket = server.nextPendingConnection();
    QVERIFY(clientSocket);

    char buffer[100];
    memset(buffer, 0, sizeof(buffer));
    QCOMPARE(clientSocket->write(buffer, sizeof(buffer)), (qint64)sizeof(buffer));
    clientSocket->waitForBytesWritten();
    clientSocket->close();
    server.close();

    client.waitForDisconnected();
    QCOMPARE(readChannelFinishedSpy.count(), 1);
    QCOMPARE(client.read(buffer, sizeof(buffer)), (qint64)sizeof(buffer));
    QCOMPARE(client.state(), QLocalSocket::UnconnectedState);
}

void tst_QLocalSocket::debug()
{
    // Make sure this compiles
    QTest::ignoreMessage(QtDebugMsg, "QLocalSocket::ConnectionRefusedError QLocalSocket::UnconnectedState");
    qDebug() << QLocalSocket::ConnectionRefusedError << QLocalSocket::UnconnectedState;
}

class WriteThread : public QThread
{
Q_OBJECT
public:
    void run() {
        QLocalSocket socket;
        socket.connectToServer("qlocalsocket_readyread");

        if (!socket.waitForConnected(3000))
            exec();
        connect(&socket, SIGNAL(bytesWritten(qint64)),
        this, SLOT(bytesWritten(qint64)), Qt::QueuedConnection);
        socket.write("testing\n");
        exec();
    }
public slots:
   void bytesWritten(qint64) {
        exit();
   }

private:
};

/*
    Tests the emission of the bytesWritten(qint64)
    signal.

    Create a thread that will write to a socket.
    If the bytesWritten(qint64) signal is generated,
    the slot connected to it will exit the thread,
    indicating test success.

*/
void tst_QLocalSocket::bytesWrittenSignal()
{
    QLocalServer server;
    QVERIFY(server.listen("qlocalsocket_readyread"));
    WriteThread writeThread;
    writeThread.start();
    bool timedOut = false;
    QVERIFY(server.waitForNewConnection(3000, &timedOut));
    QVERIFY(!timedOut);
    QTest::qWait(2000);
    QVERIFY(writeThread.wait(2000));
}

void tst_QLocalSocket::syncDisconnectNotify()
{
    QLocalServer server;
    QVERIFY(server.listen("syncDisconnectNotify"));
    QLocalSocket client;
    client.connectToServer("syncDisconnectNotify");
    QVERIFY(server.waitForNewConnection());
    QLocalSocket* serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    delete serverSocket;
    QCOMPARE(client.waitForReadyRead(), false);
}

void tst_QLocalSocket::asyncDisconnectNotify()
{
    QLocalServer server;
    QVERIFY(server.listen("asyncDisconnectNotify"));
    QLocalSocket client;
    QSignalSpy disconnectedSpy(&client, SIGNAL(disconnected()));
    client.connectToServer("asyncDisconnectNotify");
    QVERIFY(server.waitForNewConnection());
    QLocalSocket* serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    delete serverSocket;
    QTRY_VERIFY(!disconnectedSpy.isEmpty());
}

void tst_QLocalSocket::verifySocketOptions_data()
{
#ifdef Q_OS_LINUX
    QTest::addColumn<QString>("service");
    QTest::addColumn<QLocalServer::SocketOption>("opts");
    QTest::addColumn<QFile::Permissions>("perms");

    QFile::Permissions p = QFile::ExeOwner|QFile::WriteOwner|QFile::ReadOwner |
                           QFile::ExeUser|QFile::WriteUser|QFile::ReadUser;
    QTest::newRow("user")  << "userPerms"  << QLocalServer::UserAccessOption << p;

    p = QFile::ExeGroup|QFile::WriteGroup|QFile::ReadGroup;
    QTest::newRow("group") << "groupPerms" << QLocalServer::GroupAccessOption << p;

    p = QFile::ExeOther|QFile::WriteOther|QFile::ReadOther;
    QTest::newRow("other") << "otherPerms" << QLocalServer::OtherAccessOption << p;

    p = QFile::ExeOwner|QFile::WriteOwner|QFile::ReadOwner|
        QFile::ExeUser|QFile::WriteUser|QFile::ReadUser |
        QFile::ExeGroup|QFile::WriteGroup|QFile::ReadGroup|
        QFile::ExeOther|QFile::WriteOther|QFile::ReadOther;
    QTest::newRow("all")   << "worldPerms" << QLocalServer::WorldAccessOption << p;
#endif
}

void tst_QLocalSocket::verifySocketOptions()
{
    // These are only guaranteed to be useful on linux at this time
#ifdef Q_OS_LINUX
   QFETCH(QString, service);
   QFETCH(QLocalServer::SocketOption, opts);
   QFETCH(QFile::Permissions, perms);


   QLocalServer::removeServer(service);
   QLocalServer server;
   server.setSocketOptions(opts);
   QVERIFY2(server.listen(service), "service failed to start listening");

   // find the socket
   QString fullServerPath = QDir::cleanPath(QDir::tempPath());
   fullServerPath += QLatin1Char('/') + service;

   QFile socketFile(fullServerPath);
   QVERIFY2(perms == socketFile.permissions(), "permissions on the socket don't match");
#endif
}

void tst_QLocalSocket::verifyListenWithDescriptor()
{
#ifdef Q_OS_UNIX
    QFETCH(QString, path);
    QFETCH(bool, abstract);
    QFETCH(bool, bound);

//    qDebug() << "socket" << path << abstract;

    int listenSocket;

    if (bound) {
        // create the unix socket
        listenSocket = ::socket(PF_UNIX, SOCK_STREAM, 0);
        QVERIFY2(listenSocket != -1, "failed to create test socket");

        // Construct the unix address
        struct ::sockaddr_un addr;
        addr.sun_family = PF_UNIX;

        QVERIFY2(sizeof(addr.sun_path) > ((uint)path.size() + 1), "path to large to create socket");

        ::memset(addr.sun_path, 0, sizeof(addr.sun_path));
        if (abstract)
            ::memcpy(addr.sun_path+1, path.toLatin1().data(), path.toLatin1().size());
        else
            ::memcpy(addr.sun_path, path.toLatin1().data(), path.toLatin1().size());

        if (path.startsWith(QLatin1Char('/'))) {
            ::unlink(path.toLatin1());
        }

        QVERIFY2(-1 != ::bind(listenSocket, (sockaddr *)&addr, sizeof(sockaddr_un)), "failed to bind test socket to address");

        // listen for connections
        QVERIFY2(-1 != ::listen(listenSocket, 50), "failed to call listen on test socket");
    } else {
        int fds[2];
        QVERIFY2(-1 != ::socketpair(PF_UNIX, SOCK_STREAM, 0, fds), "failed to create socket pair");

        listenSocket = fds[0];
        close(fds[1]);
    }

    QLocalServer server;
    QVERIFY2(server.listen(listenSocket), "failed to start create QLocalServer with local socket");

#ifdef Q_OS_LINUX
    const QChar at(QLatin1Char('@'));
    if (!bound) {
        QCOMPARE(server.serverName().at(0), at);
        QCOMPARE(server.fullServerName().at(0), at);
    } else if (abstract) {
        QVERIFY2(server.fullServerName().at(0) == at, "abstract sockets should start with a '@'");
    } else {
        QCOMPARE(server.fullServerName(), path);
        if (path.contains(QLatin1String("/"))) {
            QVERIFY2(server.serverName() == path.mid(path.lastIndexOf(QLatin1Char('/'))+1), "server name invalid short name");
        } else {
            QVERIFY2(server.serverName() == path, "servier name doesn't match the path provided");
        }
    }
#else
    QVERIFY(server.serverName().isEmpty());
    QVERIFY(server.fullServerName().isEmpty());
#endif


#endif
}

void tst_QLocalSocket::verifyListenWithDescriptor_data()
{
#ifdef Q_OS_UNIX
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("abstract");
    QTest::addColumn<bool>("bound");

    QTest::newRow("normal") << QDir::tempPath() + QLatin1String("/testsocket") << false << true;
#ifdef Q_OS_LINUX
    QTest::newRow("abstract") << QString::fromLatin1("abstractsocketname") << true << true;
    QTest::newRow("abstractwithslash") << QString::fromLatin1("abstractsocketwitha/inthename") << true << true;
#endif
    QTest::newRow("no path") << QString::fromLatin1("/invalid/no path name specified") << true << false;

#endif

}

QTEST_MAIN(tst_QLocalSocket)
#include "tst_qlocalsocket.moc"

