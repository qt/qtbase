// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QSignalSpy>
#include <QtTest/private/qpropertytesthelper_p.h>
#if QT_CONFIG(process)
#include <QProcess>
#endif
#include <QWaitCondition>
#include <QLoggingCategory>
#include <QMutex>
#include <QList>

#include <qtextstream.h>
#include <qdatastream.h>
#include <qelapsedtimer.h>
#include <qproperty.h>
#include <QtNetwork/qlocalsocket.h>
#include <QtNetwork/qlocalserver.h>

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h> // for unlink()
#endif

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif

Q_DECLARE_METATYPE(QLocalSocket::LocalSocketError)
Q_DECLARE_METATYPE(QLocalSocket::LocalSocketState)
Q_DECLARE_METATYPE(QLocalServer::SocketOption)
Q_DECLARE_METATYPE(QLocalSocket::SocketOption)
Q_DECLARE_METATYPE(QFile::Permissions)

class tst_QLocalSocket : public QObject
{
    Q_OBJECT

public:
    using ByteArrayList = QList<QByteArray>;
    tst_QLocalSocket();

private slots:
    // basics
    void server_basic();
    void server_connectionsCount();
    void socket_basic();

    void listen_data();
    void listen();

    void listenAndConnect_data();
    void listenAndConnect();

    void listenAndConnectAbstractNamespace_data();
    void listenAndConnectAbstractNamespace();

    void listenAndConnectAbstractNamespaceTrailingZeros_data();
    void listenAndConnectAbstractNamespaceTrailingZeros();

    void connectWithOpen();
    void connectWithOldOpen();

    void sendData_data();
    void sendData();

    void readLine_data();
    void readLine();

    void skip_data();
    void skip();

    void readBufferOverflow();

    void simpleCommandProtocol1();
    void simpleCommandProtocol2();

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
    void waitForReadyReadOnDisconnected();
    void delayedDisconnect();

    void removeServer();

    void recycleServer();
    void recycleClientSocket();

    void multiConnect();
    void writeOnlySocket();

    void writeToClientAndDisconnect_data();
    void writeToClientAndDisconnect();
    void writeToDisconnected();

    void debug();
    void bytesWrittenSignal();
    void syncDisconnectNotify();
    void asyncDisconnectNotify();

    void verifySocketOptions();
    void verifySocketOptions_data();

    void verifyListenWithDescriptor();
    void verifyListenWithDescriptor_data();

    void serverBindingsAndProperties();
    void socketBindings();

protected slots:
    void socketClosedSlot();
};

tst_QLocalSocket::tst_QLocalSocket()
{
    qRegisterMetaType<QLocalSocket::LocalSocketState>("QLocalSocket::LocalSocketState");
    qRegisterMetaType<QLocalSocket::LocalSocketError>("QLocalSocket::LocalSocketError");
    qRegisterMetaType<QLocalServer::SocketOption>("QLocalServer::SocketOption");
    qRegisterMetaType<QLocalServer::SocketOption>("QLocalSocket::SocketOption");
    qRegisterMetaType<QFile::Permissions>("QFile::Permissions");
}

class CrashSafeLocalServer : public QLocalServer
{
    Q_OBJECT

public:
    CrashSafeLocalServer() : QLocalServer()
    {
    }

    bool listen(const QString &name)
    {
        removeServer(name);
        return QLocalServer::listen(name);
    }

    bool listen(qintptr socketDescriptor) { return QLocalServer::listen(socketDescriptor); }
};

class LocalServer : public CrashSafeLocalServer
{
    Q_OBJECT

public:
    LocalServer() : CrashSafeLocalServer()
    {
        connect(this, SIGNAL(newConnection()), this, SLOT(slotNewConnection()));
    }

    QList<int> hits;

protected:
    void incomingConnection(quintptr socketDescriptor) override
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
    LocalSocket(QObject *parent = nullptr) : QLocalSocket(parent)
    {
        connect(this, SIGNAL(connected()),
                this, SLOT(slotConnected()));
        connect(this, SIGNAL(disconnected()),
                this, SLOT(slotDisconnected()));
        connect(this, SIGNAL(errorOccurred(QLocalSocket::LocalSocketError)),
                this, SLOT(slotErrorOccurred(QLocalSocket::LocalSocketError)));
        connect(this, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)),
                this, SLOT(slotStateChanged(QLocalSocket::LocalSocketState)));
        connect(this, SIGNAL(readyRead()),
                this, SLOT(slotReadyRead()));
    }

private slots:
    void slotConnected()
    {
        QCOMPARE(state(), QLocalSocket::ConnectedState);
        QVERIFY(isOpen());
    }
    void slotDisconnected()
    {
        QCOMPARE(state(), QLocalSocket::UnconnectedState);
    }
    void slotErrorOccurred(QLocalSocket::LocalSocketError newError)
    {
        QVERIFY(errorString() != QLatin1String("Unknown error"));
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

    QCOMPARE(server.hits.size(), 0);
    QCOMPARE(spyNewConnection.size(), 0);
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
    QSignalSpy spyError(&socket, SIGNAL(errorOccurred(QLocalSocket::LocalSocketError)));
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

    QCOMPARE(spyConnected.size(), 0);
    QCOMPARE(spyDisconnected.size(), 0);
    QCOMPARE(spyError.size(), 0);
    QCOMPARE(spyStateChanged.size(), 0);
    QCOMPARE(spyReadyRead.size(), 0);
}

void tst_QLocalSocket::listen_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("canListen");
    QTest::addColumn<bool>("close");
    QTest::newRow("null") << QString() << false << false;
    QTest::newRow("tst_localsocket,close") << "tst_localsocket" << true << true;
    QTest::newRow("tst_localsocket,no-close") << "tst_localsocket" << true << false;
}

// start a server that listens, but don't connect a socket, make sure everything is in order
void tst_QLocalSocket::listen()
{
    LocalServer server;
    QSignalSpy spyNewConnection(&server, SIGNAL(newConnection()));

    QFETCH(QString, name);
    QFETCH(bool, canListen);
    QFETCH(bool, close);
    QVERIFY2((server.listen(name) == canListen), qUtf8Printable(server.errorString()));

    // test listening
    QCOMPARE(server.serverName(), name);
    QVERIFY(server.fullServerName().contains(name));
    QCOMPARE(server.isListening(), canListen);
    QCOMPARE(server.hasPendingConnections(), false);
    QCOMPARE(server.nextPendingConnection(), (QLocalSocket*)0);
    QCOMPARE(server.hits.size(), 0);
    QCOMPARE(spyNewConnection.size(), 0);
    if (canListen) {
        QVERIFY(server.errorString().isEmpty());
        QCOMPARE(server.serverError(), QAbstractSocket::UnknownSocketError);
        // already isListening
        QTest::ignoreMessage(QtWarningMsg, "QLocalServer::listen() called when already listening");
        QVERIFY(!server.listen(name));
        QVERIFY(server.socketDescriptor() != -1);
    } else {
        QVERIFY(!server.errorString().isEmpty());
        QCOMPARE(server.serverError(), QAbstractSocket::HostNotFoundError);
        QCOMPARE(server.socketDescriptor(), -1);
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
        const QByteArray iB = QByteArray::number(i);
        QTest::newRow(("null " + iB).constData()) << QString() << false << connections;
        QTest::newRow(("tst_localsocket " + iB).constData()) << "tst_localsocket" << true << connections;
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
        QSignalSpy spyError(socket, SIGNAL(errorOccurred(QLocalSocket::LocalSocketError)));
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
            QCOMPARE(spyError.size(), 0);
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

        QTRY_COMPARE(spyConnected.size(), canListen ? 1 : 0);
        QCOMPARE(spyDisconnected.size(), 0);

        // error signals
        QVERIFY(spyError.size() >= 0);
        if (canListen) {
            if (spyError.size() > 0)
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
        QCOMPARE(spyStateChanged.size(), 2);
        QCOMPARE(spyReadyRead.size(), 0);

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
            QTRY_COMPARE(server.hits.size(), i + 1);
            QCOMPARE(spyNewConnection.size(), i + 1);
            QVERIFY(server.errorString().isEmpty());
            QCOMPARE(server.serverError(), QAbstractSocket::UnknownSocketError);
        } else {
            QVERIFY(server.serverName().isEmpty());
            QVERIFY(server.fullServerName().isEmpty());
            QCOMPARE(server.nextPendingConnection(), (QLocalSocket*)0);
            QCOMPARE(spyNewConnection.size(), 0);
            QCOMPARE(server.hits.size(), 0);
            QVERIFY(!server.errorString().isEmpty());
            QCOMPARE(server.serverError(), QAbstractSocket::HostNotFoundError);
        }
    }
    qDeleteAll(sockets.begin(), sockets.end());

    server.close();

    QCOMPARE(server.hits.size(), (canListen ? connections : 0));
    QCOMPARE(spyNewConnection.size(), (canListen ? connections : 0));
}

void tst_QLocalSocket::connectWithOpen()
{
    LocalServer server;
    QVERIFY2(server.listen("tst_qlocalsocket"), qUtf8Printable(server.errorString()));

    LocalSocket socket;
    QSignalSpy spyAboutToClose(&socket, SIGNAL(aboutToClose()));
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

    QCOMPARE(spyAboutToClose.size(), 1);
}

void tst_QLocalSocket::listenAndConnectAbstractNamespaceTrailingZeros_data()
{
#ifdef Q_OS_LINUX
    QTest::addColumn<bool>("server_0");
    QTest::addColumn<bool>("client_0");
    QTest::addColumn<bool>("success");
    QTest::newRow("srv0_cli0") << true << true << true;
    QTest::newRow("srv_cli0") << false << true << false;
    QTest::newRow("srv0_cli") << true << false << false;
    QTest::newRow("srv_cli") << false << false << true;
#else
    return;
#endif
}

void tst_QLocalSocket::listenAndConnectAbstractNamespaceTrailingZeros()
{
#ifdef Q_OS_LINUX
    QFETCH(bool, server_0);
    QFETCH(bool, client_0);
    QFETCH(bool, success);
    bool expectedTimeOut = !success;
    QString server_path("tst_qlocalsocket");
    QString client_path("tst_qlocalsocket");

    if (server_0)
        server_path.append(QChar('\0'));
    if (client_0)
        client_path.append(QChar('\0'));
    LocalServer server;
    server.setSocketOptions(QLocalServer::AbstractNamespaceOption);
    QVERIFY2(server.listen(server_path), qUtf8Printable(server.errorString()));
    QCOMPARE(server.fullServerName(), server_path);

    LocalSocket socket;
    socket.setSocketOptions(QLocalSocket::AbstractNamespaceOption);
    socket.setServerName(client_path);
    QCOMPARE(socket.open(), success);
    if (success)
        QCOMPARE(socket.fullServerName(), client_path);
    else
        QVERIFY(socket.fullServerName().isEmpty());

    bool timedOut = true;
    QCOMPARE(server.waitForNewConnection(3000, &timedOut), success);

#if defined(QT_LOCALSOCKET_TCP)
    QTest::qWait(250);
#endif
    QCOMPARE(timedOut, expectedTimeOut);

    socket.close();
    server.close();
#else
    return;
#endif
}

void tst_QLocalSocket::listenAndConnectAbstractNamespace_data()
{
    QTest::addColumn<QLocalServer::SocketOption>("serverOption");
    QTest::addColumn<QLocalSocket::SocketOption>("socketOption");
    QTest::addColumn<bool>("success");
    QTest::newRow("abs_abs") << QLocalServer::AbstractNamespaceOption << QLocalSocket::AbstractNamespaceOption << true;
    QTest::newRow("reg_reg") << QLocalServer::NoOptions << QLocalSocket::NoOptions << true;
#ifdef Q_OS_LINUX
    QTest::newRow("reg_abs") << QLocalServer::UserAccessOption << QLocalSocket::AbstractNamespaceOption << false;
    QTest::newRow("abs_reg") << QLocalServer::AbstractNamespaceOption << QLocalSocket::NoOptions << false;
#endif
}

void tst_QLocalSocket::listenAndConnectAbstractNamespace()
{
    QFETCH(QLocalServer::SocketOption, serverOption);
    QFETCH(QLocalSocket::SocketOption, socketOption);
    QFETCH(bool, success);
    bool expectedTimeOut = !success;

    LocalServer server;
    server.setSocketOptions(serverOption);
    QVERIFY2(server.listen("tst_qlocalsocket"), qUtf8Printable(server.errorString()));

    LocalSocket socket;
    socket.setSocketOptions(socketOption);
    socket.setServerName("tst_qlocalsocket");
    QCOMPARE(socket.open(), success);

    bool timedOut = true;
    QCOMPARE(server.waitForNewConnection(3000, &timedOut), success);

#if defined(QT_LOCALSOCKET_TCP)
    QTest::qWait(250);
#endif
    QCOMPARE(timedOut, expectedTimeOut);

    socket.close();
    server.close();
}

void tst_QLocalSocket::connectWithOldOpen()
{
    class OverriddenOpen : public LocalSocket
    {
    public:
        virtual bool open(OpenMode mode) override
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
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("canListen");

    QTest::newRow("null") << QString() << false;
    QTest::newRow("tst_localsocket") << "tst_localsocket" << true;
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
    QSignalSpy spyError(&socket, SIGNAL(errorOccurred(QLocalSocket::LocalSocketError)));
    QSignalSpy spyStateChanged(&socket, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)));
    QSignalSpy spyReadyRead(&socket, SIGNAL(readyRead()));

    // test creating a connection
    socket.connectToServer(name);
    bool timedOut = true;
    int expectedReadyReadSignals = 0;

    QCOMPARE(server.waitForNewConnection(3000, &timedOut), canListen);

#if defined(QT_LOCALSOCKET_TCP)
    QTest::qWait(250);
#endif
    QVERIFY(!timedOut);
    QCOMPARE(spyConnected.size(), canListen ? 1 : 0);
    QCOMPARE(socket.state(), canListen ? QLocalSocket::ConnectedState : QLocalSocket::UnconnectedState);

    // test sending/receiving data
    if (server.hasPendingConnections()) {
        QString testLine = "test";
        for (int i = 0; i < 50000; ++i)
            testLine += QLatin1Char('a');
        QLocalSocket *serverSocket = server.nextPendingConnection();
        QVERIFY(serverSocket);
        QCOMPARE(serverSocket->state(), QLocalSocket::ConnectedState);
        QTextStream out(serverSocket);
        QTextStream in(&socket);
        out << testLine << Qt::endl;
        bool wrote = serverSocket->waitForBytesWritten(3000);

        if (!socket.canReadLine()) {
            expectedReadyReadSignals = 1;
            QVERIFY(socket.waitForReadyRead());
        }

        QVERIFY(socket.bytesAvailable() >= 0);
        QCOMPARE(socket.bytesToWrite(), (qint64)0);
        QCOMPARE(socket.flush(), false);
        QCOMPARE(socket.isValid(), canListen);
        QCOMPARE(socket.readBufferSize(), (qint64)0);
        QCOMPARE(spyReadyRead.size(), expectedReadyReadSignals);

        QVERIFY(testLine.startsWith(in.readLine()));

        QVERIFY(wrote || serverSocket->waitForBytesWritten(1000));

        QCOMPARE(serverSocket->errorString(), QString("Unknown error"));
        QCOMPARE(socket.errorString(), QString("Unknown error"));
    }

    socket.disconnectFromServer();
    QCOMPARE(spyConnected.size(), canListen ? 1 : 0);
    QCOMPARE(spyDisconnected.size(), canListen ? 1 : 0);
    QCOMPARE(spyError.size(), canListen ? 0 : 1);
    QCOMPARE(spyStateChanged.size(), canListen ? 4 : 2);
    QCOMPARE(spyReadyRead.size(), canListen ? expectedReadyReadSignals : 0);

    server.close();

    QCOMPARE(server.hits.size(), (canListen ? 1 : 0));
    QCOMPARE(spy.size(), (canListen ? 1 : 0));
}

void tst_QLocalSocket::readLine_data()
{
    QTest::addColumn<ByteArrayList>("input");
    QTest::addColumn<ByteArrayList>("output");
    QTest::addColumn<int>("maxSize");
    QTest::addColumn<bool>("wholeLinesOnly");

    QTest::newRow("0") << ByteArrayList{ "\n", "A", "\n", "B", "B", "A", "\n" }
                       << ByteArrayList{ "\n", "", "", "A\n", "", "", "", "",
                                         "BBA\n", "", "" }
                       << 80 << true;
    QTest::newRow("1") << ByteArrayList{ "A", "\n", "\n", "B", "B", "\n", "A", "A" }
                       << ByteArrayList{ "", "A\n", "", "\n", "", "", "", "BB\n",
                                         "", "", "", "AA", "" }
                       << 80 << true;

    QTest::newRow("2") << ByteArrayList{ "\nA\nA\n" }
                       << ByteArrayList{ "\n", "A", "\n", "A", "\n", "", "" }
                       << 1 << false;
    QTest::newRow("3") << ByteArrayList{ "A\n\n\nA", "A" }
                       << ByteArrayList{ "A\n", "\n", "\n", "A", "", "A", "", "" }
                       << 2 << false;

    QTest::newRow("4") << ByteArrayList{ "He", "ll", "o\n", " \n", "wo", "rl", "d", "!\n" }
                       << ByteArrayList{ "", "Hel", "", "lo\n", "", " \n", "", "", "wor",
                                         "", "", "ld!", "\n", "", "" }
                       << 3 << true;
    QTest::newRow("5") << ByteArrayList{ "Hello\n world!" }
                       << ByteArrayList{ "Hello\n", "", " world!", "" }
                       << 80 << true;

    QTest::newRow("6") << ByteArrayList{ "\nHello", " \n", " wor", "ld!\n" }
                       << ByteArrayList{ "\n", "Hell", "o", "", " \n", "", " wor", "",
                                         "ld!\n", "", "" }
                       << 4 << false;
    QTest::newRow("7") << ByteArrayList{ "Hello\n world", "!" }
                       << ByteArrayList{ "Hello\n", " world", "", "!", "", "" }
                       << 80 << false;
}

void tst_QLocalSocket::readLine()
{
    QFETCH(ByteArrayList, input);
    QFETCH(ByteArrayList, output);
    QFETCH(int, maxSize);
    QFETCH(bool, wholeLinesOnly);

    const QString serverName = QLatin1String("tst_localsocket");
    LocalServer server;
    QVERIFY2(server.listen(serverName), qUtf8Printable(server.errorString()));

    LocalSocket client;
    client.connectToServer(serverName);
    QVERIFY(server.waitForNewConnection());
    QLocalSocket *serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    QCOMPARE(client.state(), QLocalSocket::ConnectedState);

    ByteArrayList result;
    qsizetype pos = 0;
    do {
        // This test assumes that such small chunks of data are synchronously
        // delivered to the receiver on all supported platforms.
        if (pos < input.size()) {
            const QByteArray &chunk = input.at(pos);
            QCOMPARE(serverSocket->write(chunk), qint64(chunk.size()));
            QVERIFY(serverSocket->waitForBytesWritten());
            QCOMPARE(serverSocket->bytesToWrite(), qint64(0));
            QVERIFY(client.waitForReadyRead());
        } else {
            serverSocket->close();
            QVERIFY(!client.waitForReadyRead());
        }

        while (!wholeLinesOnly || (client.bytesAvailable() >= qint64(maxSize))
               || client.canReadLine() || (pos == input.size())) {
            const bool chunkEmptied = (client.bytesAvailable() == 0);
            QByteArray line(maxSize, Qt::Uninitialized);

            const qint64 readResult = client.readLine(line.data(), maxSize + 1);
            if (chunkEmptied) {
                if (pos == input.size())
                    QCOMPARE(readResult, qint64(-1));
                else
                    QCOMPARE(readResult, qint64(0));
                break;
            }
            QVERIFY((readResult > 0) && (readResult <= maxSize));
            line.resize(readResult);
            result.append(line);
        }
        result.append(QByteArray());
    } while (++pos <= input.size());
    QCOMPARE(client.state(), QLocalSocket::UnconnectedState);
    QCOMPARE(result, output);
}

void tst_QLocalSocket::skip_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<int>("read");
    QTest::addColumn<int>("skip");
    QTest::addColumn<int>("skipped");
    QTest::addColumn<char>("expect");

    QByteArray bigData;
    bigData.fill('a', 20000);
    bigData[10001] = 'x';

    QTest::newRow("small_data") << QByteArray("abcdefghij") << 3 << 6 << 6 << 'j';
    QTest::newRow("big_data") << bigData << 1 << 10000 << 10000 << 'x';
    QTest::newRow("beyond_the_end") << bigData << 1 << 20000 << 19999 << '\0';
}

void tst_QLocalSocket::skip()
{
    QFETCH(QByteArray, data);
    QFETCH(int, read);
    QFETCH(int, skip);
    QFETCH(int, skipped);
    QFETCH(char, expect);
    char lastChar = '\0';

    const QString serverName = QLatin1String("tst_localsocket");
    LocalServer server;
    QVERIFY2(server.listen(serverName), qUtf8Printable(server.errorString()));

    LocalSocket client;
    client.connectToServer(serverName);
    QVERIFY(server.waitForNewConnection());
    QLocalSocket *serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    QCOMPARE(client.state(), QLocalSocket::ConnectedState);

    QCOMPARE(serverSocket->write(data), data.size());
    while (serverSocket->waitForBytesWritten())
        QVERIFY(client.waitForReadyRead());
    QCOMPARE(serverSocket->bytesToWrite(), qint64(0));
    serverSocket->close();
    QVERIFY(client.waitForDisconnected());

    for (int i = 0; i < read; ++i)
        client.getChar(nullptr);

    QCOMPARE(client.skip(skip), skipped);
    client.getChar(&lastChar);
    QCOMPARE(lastChar, expect);
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
    // On Windows, the flushing is done by an asynchronous write operation.
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

static qint64 writeCommand(const QVariant &command, QIODevice *device, int commandCounter)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << qint64(0);
    out << commandCounter;
    out << command;
    out.device()->seek(0);
    out << qint64(block.size() - sizeof(qint64));
    return device->write(block);
}

static QVariant readCommand(QIODevice *ioDevice, int *readCommandCounter, bool readSize = true)
{
    QDataStream in(ioDevice);
    qint64 blockSize;
    int commandCounter;
    if (readSize)
        in >> blockSize;
    in >> commandCounter;
    *readCommandCounter = commandCounter;

    QVariant command;
    in >> command;

    return command;
}

void tst_QLocalSocket::simpleCommandProtocol1()
{
    CrashSafeLocalServer server;
    server.listen(QStringLiteral("simpleProtocol"));

    QLocalSocket localSocketWrite;
    localSocketWrite.connectToServer(server.serverName());
    QVERIFY(server.waitForNewConnection());
    QLocalSocket *localSocketRead = server.nextPendingConnection();
    QVERIFY(localSocketRead);

    int readCounter = 0;
    for (int i = 0; i < 2000; ++i) {
        const QVariant command(QRect(readCounter, i, 10, 10));
        const qint64 blockSize = writeCommand(command, &localSocketWrite, i);
        while (localSocketWrite.bytesToWrite())
            QVERIFY(localSocketWrite.waitForBytesWritten());
        while (localSocketRead->bytesAvailable() < blockSize) {
            QVERIFY(localSocketRead->waitForReadyRead(1000));
        }
        const QVariant variant = readCommand(localSocketRead, &readCounter);
        QCOMPARE(readCounter, i);
        QCOMPARE(variant, command);
    }
}

void tst_QLocalSocket::simpleCommandProtocol2()
{
    CrashSafeLocalServer server;
    server.listen(QStringLiteral("simpleProtocol"));

    QLocalSocket localSocketWrite;
    QSignalSpy spyDisconnected(&localSocketWrite, SIGNAL(disconnected()));
    localSocketWrite.connectToServer(server.serverName());
    QVERIFY(server.waitForNewConnection());
    QLocalSocket* localSocketRead = server.nextPendingConnection();
    QVERIFY(localSocketRead);

    int readCounter = 0;
    qint64 writtenBlockSize = 0;
    qint64 blockSize = 0;

    QObject::connect(localSocketRead, &QLocalSocket::readyRead, [&] {
        forever {
            if (localSocketRead->bytesAvailable() < qint64(sizeof(qint64)))
                return;

            if (blockSize == 0) {
                QDataStream in(localSocketRead);
                in >> blockSize;
            }

            if (localSocketRead->bytesAvailable() < blockSize)
                return;

            int commandNumber = 0;
            const QVariant variant = readCommand(localSocketRead, &commandNumber, false);
            QCOMPARE(writtenBlockSize, blockSize);
            QCOMPARE(readCounter, commandNumber);
            QCOMPARE(variant.userType(), (int)QMetaType::QRect);

            readCounter++;
            blockSize = 0;
        }
    });

    for (int i = 0; i < 500; ++i) {
        const QVariant command(QRect(readCounter, i, 10, 10));
        writtenBlockSize = writeCommand(command, &localSocketWrite, i) - sizeof(qint64);
        if (i % 10 == 0)
            QTest::qWait(1);
    }

    localSocketWrite.abort();
    QCOMPARE(localSocketWrite.state(), QLocalSocket::UnconnectedState);
    QCOMPARE(spyDisconnected.size(), 1);
    QCOMPARE(localSocketWrite.bytesToWrite(), 0);
    QVERIFY(!localSocketWrite.isOpen());

    QVERIFY(localSocketRead->waitForDisconnected(1000));
}

// QLocalSocket/Server can take a name or path, check that it works as expected
void tst_QLocalSocket::fullPath()
{
    CrashSafeLocalServer server;
    QString name = "qlocalsocket_pathtest";
#if defined(QT_LOCALSOCKET_TCP)
    QString path = "QLocalServer";
#elif defined(Q_OS_WIN)
    QString path = "\\\\.\\pipe\\";
#else
    QString path = "/tmp";
#endif
    QString serverName = path + '/' + name;
    QVERIFY2(server.listen(serverName), qUtf8Printable(server.errorString()));
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
    QVERIFY2(server.listen(name), qUtf8Printable(server.errorString()));
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
   QVERIFY(server.hits.size() > 0);
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
    void run() override
    {
        QString testLine = "test";
        LocalSocket socket;
        QSignalSpy spyReadyRead(&socket, SIGNAL(readyRead()));
        socket.connectToServer("qlocalsocket_threadtest");
        QVERIFY(socket.waitForConnected(1000));

        // We should *not* have this signal yet!
        QCOMPARE(spyReadyRead.size(), 0);
        socket.waitForReadyRead();
        QCOMPARE(spyReadyRead.size(), 1);
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
    void run() override
    {
        QString testLine = "test";
        LocalServer server;
        server.setMaxPendingConnections(10);
        QVERIFY2(server.listen("qlocalsocket_threadtest"),
                 qUtf8Printable(server.errorString()));
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
            out << testLine << Qt::endl;
            QCOMPARE(serverSocket->state(), QLocalSocket::ConnectedState);
            QVERIFY2(serverSocket->waitForBytesWritten(), serverSocket->errorString().toLatin1().constData());
            QCOMPARE(serverSocket->errorString(), QString("Unknown error"));
            --done;
            delete serverSocket;
        }
        QCOMPARE(server.hits.size(), clients);
    }
};

void tst_QLocalSocket::threadedConnection_data()
{
    QTest::addColumn<int>("threads");
    QTest::newRow("1 client") << 1;
    QTest::newRow("2 clients") << 2;
    QTest::newRow("5 clients") << 5;
    QTest::newRow("10 clients") << 10;
    QTest::newRow("20 clients") << 20;
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

#if QT_CONFIG(process)
class ProcessOutputDumper
{
public:
    ProcessOutputDumper(QProcess *p = nullptr)
        : process(p)
    {}

    ~ProcessOutputDumper()
    {
        if (process)
            fputs(process->readAll().data(), stdout);
    }

    void clear()
    {
        process = nullptr;
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
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else

#ifdef Q_OS_WIN
    const QString exeSuffix = QStringLiteral(".exe");
#else
    const QString exeSuffix;
#endif

    const QString socketProcess
            = QFINDTESTDATA(QStringLiteral("socketprocess/socketprocess") + exeSuffix);
    QVERIFY(QFile::exists(socketProcess));

    QFETCH(int, processes);
    QStringList serverArguments = QStringList() << "--server" << QString::number(processes);
    QProcess producer;
    ProcessOutputDumper producerOutputDumper(&producer);
    QList<QProcess*> consumers;
    producer.setProcessChannelMode(QProcess::MergedChannels);
    producer.start(socketProcess, serverArguments);
    QVERIFY2(producer.waitForStarted(-1), qPrintable(producer.errorString()));
    for (int i = 0; i < processes; ++i) {
        QStringList arguments = QStringList() << "--client";
        QProcess *p = new QProcess;
        consumers.append(p);
        p->setProcessChannelMode(QProcess::MergedChannels);
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
    QVERIFY2(server.listen(name), qUtf8Printable(server.errorString()));
    LocalSocket socket;
    socket.connectToServer(name);
    QVERIFY(socket.waitForConnected(3000));
    QVERIFY(server.waitForNewConnection(3000));
    QLocalSocket *serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    socket.disconnectFromServer();
    QElapsedTimer timer;
    timer.start();
    QVERIFY(serverSocket->waitForDisconnected(3000));
    QVERIFY(timer.elapsed() < 2000);
}

void tst_QLocalSocket::waitForDisconnectByServer()
{
    QString name = "tst_localsocket";
    LocalServer server;
    QVERIFY2(server.listen(name), qUtf8Printable(server.errorString()));
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
    QCOMPARE(spy.size(), 1);
}

void tst_QLocalSocket::waitForReadyReadOnDisconnected()
{
    QString name = "tst_localsocket";
    LocalServer server;
    QVERIFY2(server.listen(name), qUtf8Printable(server.errorString()));
    LocalSocket socket;
    connect(&socket, &QLocalSocket::readyRead, [&socket]() {
        QVERIFY(socket.getChar(nullptr));
        // The next call should not block because the socket was closed
        // by the peer.
        QVERIFY(!socket.waitForReadyRead(3000));
    });

    socket.connectToServer(name);
    QVERIFY(socket.waitForConnected(3000));
    QVERIFY(server.waitForNewConnection(3000));
    QLocalSocket *serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    QVERIFY(serverSocket->putChar(0));
    QVERIFY(serverSocket->waitForBytesWritten(3000));
    serverSocket->close();

#ifdef Q_OS_WIN
    // Ensure that the asynchronously delivered close notification is
    // already queued up before we consume the data.
    QTest::qSleep(250);
#endif

    QElapsedTimer timer;
    timer.start();
    QVERIFY(socket.waitForReadyRead(5000));
    QVERIFY(timer.elapsed() < 2000);
}

void tst_QLocalSocket::delayedDisconnect()
{
    QString name = "tst_localsocket";
    LocalServer server;
    QVERIFY2(server.listen(name), qUtf8Printable(server.errorString()));
    LocalSocket socket;
    socket.connectToServer(name);
    QVERIFY(socket.waitForConnected(3000));
    QVERIFY(server.waitForNewConnection(3000));
    QLocalSocket *serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    connect(serverSocket, &QLocalSocket::aboutToClose, [serverSocket]() {
        QVERIFY(serverSocket->isOpen());
        QCOMPARE(serverSocket->bytesAvailable(), qint64(1));
    });

    QVERIFY(socket.putChar(0));
    socket.disconnectFromServer();
    QCOMPARE(socket.state(), QLocalSocket::ClosingState);
    QVERIFY(socket.waitForDisconnected(3000));
    QCOMPARE(socket.state(), QLocalSocket::UnconnectedState);
    QVERIFY(socket.isOpen());

    QVERIFY(serverSocket->waitForReadyRead(3000));
    serverSocket->close();
    QCOMPARE(serverSocket->state(), QLocalSocket::UnconnectedState);
    QVERIFY(!serverSocket->isOpen());
    QCOMPARE(serverSocket->bytesAvailable(), qint64(0));
}

void tst_QLocalSocket::removeServer()
{
    // this is a hostile takeover, but recovering from a crash results in the same
    // Note: Explicitly not a CrashSafeLocalServer
    QLocalServer server, server2;

    QVERIFY(QLocalServer::removeServer("cleanuptest"));
    QVERIFY2(server.listen("cleanuptest"), qUtf8Printable(server.errorString()));
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
    CrashSafeLocalServer server;
    QLocalSocket client;

    QVERIFY2(server.listen("recycletest1"), qUtf8Printable(server.errorString()));
    client.connectToServer("recycletest1");
    QVERIFY(client.waitForConnected(201));
    QVERIFY(server.waitForNewConnection(201));
    QVERIFY(server.nextPendingConnection() != 0);

    server.close();
    client.disconnectFromServer();
    qApp->processEvents();

    QVERIFY2(server.listen("recycletest2"), qUtf8Printable(server.errorString()));
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
    CrashSafeLocalServer server;
    const QString serverName = QStringLiteral("recycleClientSocket");
    QVERIFY2(server.listen(serverName), qUtf8Printable(server.errorString()));
    QLocalSocket client;
    QSignalSpy clientReadyReadSpy(&client, SIGNAL(readyRead()));
    QSignalSpy clientErrorSpy(&client, SIGNAL(errorOccurred(QLocalSocket::LocalSocketError)));
    for (int i = 0; i < lines.size(); ++i) {
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
    CrashSafeLocalServer server;
    QLocalSocket client1;
    QLocalSocket client2;
    QLocalSocket client3;

    QVERIFY2(server.listen("multiconnect"), qUtf8Printable(server.errorString()));

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
    CrashSafeLocalServer server;
    QVERIFY2(server.listen("writeOnlySocket"), qUtf8Printable(server.errorString()));

    QLocalSocket client;
    client.connectToServer("writeOnlySocket", QIODevice::WriteOnly);
    QVERIFY(client.waitForConnected());
    QVERIFY(server.waitForNewConnection(200));
    QLocalSocket* serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);

    QCOMPARE(client.bytesAvailable(), qint64(0));
    QCOMPARE(client.state(), QLocalSocket::ConnectedState);

    serverSocket->abort();
    // On Windows, we need to test that the socket state is periodically
    // checked in a loop, even if no timeout value is specified (i.e.
    // waitForDisconnected(-1) does not fail immediately).
    QVERIFY(client.waitForDisconnected(-1));
    QCOMPARE(client.state(), QLocalSocket::UnconnectedState);
}

void tst_QLocalSocket::writeToClientAndDisconnect_data()
{
    QTest::addColumn<int>("chunks");
    QTest::newRow("one chunk") << 1;
    QTest::newRow("several chunks") << 20;
}

void tst_QLocalSocket::writeToClientAndDisconnect()
{
    QFETCH(int, chunks);
    CrashSafeLocalServer server;
    QLocalSocket client;
    QSignalSpy readChannelFinishedSpy(&client, SIGNAL(readChannelFinished()));

    QVERIFY2(server.listen("writeAndDisconnectServer"), qUtf8Printable(server.errorString()));
    client.connectToServer("writeAndDisconnectServer");
    QVERIFY(client.waitForConnected(200));
    QVERIFY(server.waitForNewConnection(200));
    QLocalSocket* clientSocket = server.nextPendingConnection();
    QVERIFY(clientSocket);
    server.close();

    char buffer[100];
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < chunks; ++i)
        QCOMPARE(clientSocket->write(buffer, sizeof(buffer)), qint64(sizeof(buffer)));
    clientSocket->close();
    QVERIFY(clientSocket->waitForDisconnected());

    QVERIFY(client.waitForDisconnected());
    QCOMPARE(readChannelFinishedSpy.size(), 1);
    const QByteArray received = client.readAll();
    QCOMPARE(received.size(), qint64(sizeof(buffer) * chunks));
    QCOMPARE(client.state(), QLocalSocket::UnconnectedState);
}

void tst_QLocalSocket::writeToDisconnected()
{
    CrashSafeLocalServer server;
    QVERIFY2(server.listen("writeToDisconnected"), qUtf8Printable(server.errorString()));

    QLocalSocket client;
    QSignalSpy spyError(&client, SIGNAL(errorOccurred(QLocalSocket::LocalSocketError)));
    client.connectToServer("writeToDisconnected");
    QVERIFY(client.waitForConnected(3000));
    QVERIFY(server.waitForNewConnection(3000));
    QLocalSocket *serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    serverSocket->abort();

    QCOMPARE(client.state(), QLocalSocket::ConnectedState);
    QVERIFY(client.putChar(0));

#ifdef Q_OS_WIN
    // Ensure the asynchronous write operation is finished.
    QTest::qSleep(250);
#endif

    QCOMPARE(client.bytesToWrite(), qint64(1));
    QVERIFY(!client.waitForBytesWritten());
    QCOMPARE(spyError.size(), 1);
    QCOMPARE(client.state(), QLocalSocket::UnconnectedState);
}

void tst_QLocalSocket::debug()
{
    // Make sure this compiles
    if (QLoggingCategory::defaultCategory()->isDebugEnabled())
        QTest::ignoreMessage(QtDebugMsg, "QLocalSocket::ConnectionRefusedError QLocalSocket::UnconnectedState");
    qDebug() << QLocalSocket::ConnectionRefusedError << QLocalSocket::UnconnectedState;
}

class WriteThread : public QThread
{
Q_OBJECT
public:
    void run() override
    {
        QLocalSocket socket;
        socket.connectToServer("qlocalsocket_readyread");

        if (!socket.waitForConnected(3000))
            exec();
        connect(&socket, SIGNAL(bytesWritten(qint64)),
        this, SLOT(bytesWritten(qint64)), Qt::QueuedConnection);
        socket.write("testing\n");
        exec();
    }
signals:
    void bytesWrittenReceived();
public slots:
    void bytesWritten(qint64) {
        emit bytesWrittenReceived();
        exit();
    }
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
    CrashSafeLocalServer server;
    QVERIFY2(server.listen("qlocalsocket_readyread"), qUtf8Printable(server.errorString()));
    WriteThread writeThread;
    QSignalSpy receivedSpy(&writeThread, &WriteThread::bytesWrittenReceived);
    writeThread.start();
    bool timedOut = false;
    QVERIFY(server.waitForNewConnection(3000, &timedOut));
    QVERIFY(!timedOut);
    QVERIFY(receivedSpy.wait(2000));
    QVERIFY(writeThread.wait(2000));
}

void tst_QLocalSocket::socketClosedSlot()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());

    QCOMPARE(socket->state(), QLocalSocket::UnconnectedState);
}

void tst_QLocalSocket::syncDisconnectNotify()
{
    CrashSafeLocalServer server;
    QVERIFY2(server.listen("syncDisconnectNotify"), qUtf8Printable(server.errorString()));
    QLocalSocket client;
    connect(&client, &QLocalSocket::disconnected,
            this, &tst_QLocalSocket::socketClosedSlot);
    connect(&client, &QIODevice::readChannelFinished,
            this, &tst_QLocalSocket::socketClosedSlot);

    client.connectToServer("syncDisconnectNotify");
    QVERIFY(server.waitForNewConnection());
    QLocalSocket* serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    delete serverSocket;
    QCOMPARE(client.waitForReadyRead(), false);
    QVERIFY(!client.putChar(0));
}

void tst_QLocalSocket::asyncDisconnectNotify()
{
    CrashSafeLocalServer server;
    QVERIFY2(server.listen("asyncDisconnectNotify"), qUtf8Printable(server.errorString()));
    QLocalSocket client;
    QSignalSpy disconnectedSpy(&client, SIGNAL(disconnected()));
    QSignalSpy readChannelFinishedSpy(&client, SIGNAL(readChannelFinished()));
    connect(&client, &QLocalSocket::disconnected,
            this, &tst_QLocalSocket::socketClosedSlot);
    connect(&client, &QIODevice::readChannelFinished,
            this, &tst_QLocalSocket::socketClosedSlot);

    client.connectToServer("asyncDisconnectNotify");
    QVERIFY(server.waitForNewConnection());
    QLocalSocket* serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
    delete serverSocket;
    QTRY_VERIFY(!disconnectedSpy.isEmpty());
    QCOMPARE(readChannelFinishedSpy.size(), 1);
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
#if defined(Q_OS_LINUX) && !defined(Q_OS_WEBOS)
   QFETCH(QString, service);
   QFETCH(QLocalServer::SocketOption, opts);
   QFETCH(QFile::Permissions, perms);

   CrashSafeLocalServer server;
   server.setSocketOptions(opts);
   QVERIFY2(server.listen(service), qUtf8Printable(server.errorString()));

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

    // Construct the unix address
    struct ::sockaddr_un addr;
    addr.sun_family = PF_UNIX;

    if (bound) {
        // create the unix socket
        listenSocket = ::socket(PF_UNIX, SOCK_STREAM, 0);
        QVERIFY2(listenSocket != -1, "failed to create test socket");

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

    CrashSafeLocalServer server;
    QVERIFY2(server.listen(listenSocket), qUtf8Printable(server.errorString()));

#if defined(Q_OS_LINUX) || defined(Q_OS_QNX)
    if (!bound) {
        QCOMPARE(server.serverName().isEmpty(), true);
        QCOMPARE(server.fullServerName().isEmpty(), true);
    } else if (abstract) {
        QVERIFY2(server.fullServerName().at(0) == addr.sun_path[1],
                 "abstract sockets should match server path without leading null");
    } else {
        QCOMPARE(server.fullServerName(), path);
        if (path.contains(QLatin1Char('/'))) {
            QVERIFY2(server.serverName() == path.mid(path.lastIndexOf(QLatin1Char('/'))+1), "server name invalid short name");
        } else {
            QVERIFY2(server.serverName() == path, "servier name doesn't match the path provided");
        }
    }
#else
    if (bound) {
        QCOMPARE(server.fullServerName(), path);
        if (path.contains(QLatin1Char('/'))) {
            QVERIFY2(server.serverName() == path.mid(path.lastIndexOf(QLatin1Char('/'))+1), "server name invalid short name");
        } else {
            QVERIFY2(server.serverName() == path, "server name doesn't match the path provided");
        }
    } else {
        QVERIFY(server.serverName().isEmpty());
        QVERIFY(server.fullServerName().isEmpty());
    }
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
#if defined(Q_OS_LINUX) || defined(Q_OS_QNX)
    QTest::newRow("abstract") << QString::fromLatin1("abstractsocketname") << true << true;
    QTest::newRow("abstractwithslash") << QString::fromLatin1("abstractsocketwitha/inthename") << true << true;
#endif
    QTest::newRow("no path") << QString::fromLatin1("/invalid/no path name specified") << true << false;

#endif

}

void tst_QLocalSocket::serverBindingsAndProperties()
{
    CrashSafeLocalServer server;

    QTestPrivate::testReadWritePropertyBasics(
            server, QLocalServer::SocketOptions{QLocalServer::GroupAccessOption},
            QLocalServer::SocketOptions{QLocalServer::OtherAccessOption}, "socketOptions");
}

void tst_QLocalSocket::socketBindings()
{
    QLocalSocket socket;

    QTestPrivate::testReadWritePropertyBasics(
            socket, QLocalSocket::SocketOptions{QLocalSocket::AbstractNamespaceOption},
            QLocalSocket::SocketOptions{QLocalSocket::NoOptions}, "socketOptions");
}

QTEST_MAIN(tst_QLocalSocket)
#include "tst_qlocalsocket.moc"

