/****************************************************************************
**
** Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
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
#include <QDebug>
#include <QEventLoop>
#include <QByteArray>
#include <QString>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QTime>

#include <QSctpSocket>
#include <QSctpServer>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

#define SOCKET int
#define INVALID_SOCKET -1

class tst_QSctpSocket : public QObject
{
    Q_OBJECT

public:
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

private slots:
    void constructing();
    void bind_data();
    void bind();
    void setInvalidSocketDescriptor();
    void setSocketDescriptor();
    void socketDescriptor();
    void hostNotFound();
    void connecting();
    void readAndWrite();
    void loop_data();
    void loop();
    void loopInTCPMode_data();
    void loopInTCPMode();
    void readDatagramAfterClose();
    void clientSendDataOnDelayedDisconnect();

protected slots:
    void exitLoopSlot();

private:
    static int loopLevel;
};

int tst_QSctpSocket::loopLevel = 0;

//----------------------------------------------------------------------------------
void tst_QSctpSocket::constructing()
{
    QSctpSocket socket;

    // Check the initial state of the QSctpSocket.
    QCOMPARE(socket.state(), QAbstractSocket::UnconnectedState);
    QVERIFY(socket.isSequential());
    QVERIFY(!socket.isOpen());
    QVERIFY(!socket.isValid());
    QCOMPARE(socket.socketType(), QAbstractSocket::SctpSocket);
    QCOMPARE(socket.maximumChannelCount(), 0);
    QCOMPARE(socket.readChannelCount(), 0);
    QCOMPARE(socket.writeChannelCount(), 0);

    char c;
    QCOMPARE(socket.getChar(&c), false);
    QCOMPARE(socket.bytesAvailable(), Q_INT64_C(0));
    QCOMPARE(socket.canReadLine(), false);
    QCOMPARE(socket.readLine(), QByteArray());
    QCOMPARE(socket.socketDescriptor(), qintptr(-1));
    QCOMPARE(int(socket.localPort()), 0);
    QVERIFY(socket.localAddress() == QHostAddress());
    QCOMPARE(int(socket.peerPort()), 0);
    QVERIFY(socket.peerAddress() == QHostAddress());
    QCOMPARE(socket.error(), QAbstractSocket::UnknownSocketError);
    QCOMPARE(socket.errorString(), QString("Unknown error"));
}

//----------------------------------------------------------------------------------
void tst_QSctpSocket::bind_data()
{
    QTest::addColumn<QString>("stringAddr");
    QTest::addColumn<bool>("successExpected");
    QTest::addColumn<QString>("stringExpectedLocalAddress");

    // iterate all interfaces, add all addresses on them as test data
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &interface : interfaces) {
        if (!interface.isValid())
            continue;

        for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
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

// Testing bind function
void tst_QSctpSocket::bind()
{
    QFETCH(QString, stringAddr);
    QFETCH(bool, successExpected);
    QFETCH(QString, stringExpectedLocalAddress);

    QHostAddress addr(stringAddr);
    QHostAddress expectedLocalAddress(stringExpectedLocalAddress);

    QSctpSocket socket;
    qDebug() << "Binding " << addr;

    if (successExpected)
        QVERIFY2(socket.bind(addr), qPrintable(socket.errorString()));
    else
        QVERIFY(!socket.bind(addr));

    QCOMPARE(socket.localAddress(), expectedLocalAddress);
}

//----------------------------------------------------------------------------------
void tst_QSctpSocket::setInvalidSocketDescriptor()
{
    QSctpSocket socket;
    QCOMPARE(socket.socketDescriptor(), qintptr(INVALID_SOCKET));
    QVERIFY(!socket.setSocketDescriptor(-5, QAbstractSocket::UnconnectedState));
    QCOMPARE(socket.socketDescriptor(), qintptr(INVALID_SOCKET));

    QCOMPARE(socket.error(), QAbstractSocket::UnsupportedSocketOperationError);
}

//----------------------------------------------------------------------------------
void tst_QSctpSocket::setSocketDescriptor()
{
    QSctpServer server;

    server.setMaximumChannelCount(16);
    QVERIFY(server.listen());

    SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

    QVERIFY(sock != INVALID_SOCKET);
    QSctpSocket socket;
    QVERIFY(socket.setSocketDescriptor(sock, QAbstractSocket::UnconnectedState));
    QCOMPARE(socket.socketDescriptor(), qintptr(sock));
    QCOMPARE(socket.readChannelCount(), 0);
    QCOMPARE(socket.writeChannelCount(), 0);

    socket.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(socket.waitForConnected(3000));
    QVERIFY(server.waitForNewConnection(3000));

    QCOMPARE(socket.readChannelCount(), server.maximumChannelCount());
    QVERIFY(socket.writeChannelCount() <= server.maximumChannelCount());

    QSctpSocket *acceptedSocket = server.nextPendingDatagramConnection();
    QVERIFY(acceptedSocket);
    QCOMPARE(acceptedSocket->readChannelCount(), socket.writeChannelCount());
    QCOMPARE(acceptedSocket->writeChannelCount(), socket.readChannelCount());
}

//----------------------------------------------------------------------------------
void tst_QSctpSocket::socketDescriptor()
{
    QSctpSocket socket;

    QSctpServer server;

    QVERIFY(server.listen());

    QCOMPARE(socket.socketDescriptor(), qintptr(INVALID_SOCKET));
    socket.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(server.waitForNewConnection(3000));
    if (socket.state() != QAbstractSocket::ConnectedState) {
        QVERIFY((socket.state() == QAbstractSocket::HostLookupState
                 && socket.socketDescriptor() == INVALID_SOCKET)
                || socket.state() == QAbstractSocket::ConnectingState);
        QVERIFY(socket.waitForConnected(3000));
        QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    }
    QVERIFY(socket.socketDescriptor() != INVALID_SOCKET);
}

//----------------------------------------------------------------------------------
void tst_QSctpSocket::hostNotFound()
{
    QSctpSocket socket;

    socket.connectToHost("nosuchserver.qt-project.org", 80);
    QVERIFY(!socket.waitForConnected(3000));
    QCOMPARE(socket.state(), QTcpSocket::UnconnectedState);
    QCOMPARE(socket.error(), QAbstractSocket::HostNotFoundError);
}

// Testing connect function
void tst_QSctpSocket::connecting()
{
    QSctpServer server;

    QVERIFY(server.listen());

    QSctpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(socket.waitForConnected(3000));

    QVERIFY(server.waitForNewConnection(3000));
    QSctpSocket *acceptedSocket = server.nextPendingDatagramConnection();
    QVERIFY(acceptedSocket);

    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(acceptedSocket->state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socket.readChannelCount(), acceptedSocket->readChannelCount());
    QCOMPARE(socket.writeChannelCount(),acceptedSocket->writeChannelCount());
}

// Testing read/write functions
void tst_QSctpSocket::readAndWrite()
{
    QSctpServer server;

    QVERIFY(server.listen());

    QSctpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(socket.waitForConnected(3000));

    QVERIFY(server.waitForNewConnection(3000));
    QSctpSocket *acceptedSocket = server.nextPendingDatagramConnection();
    QVERIFY(acceptedSocket);

    QByteArray ba(1000, 1);
    QVERIFY(acceptedSocket->writeDatagram(ba));
    QVERIFY(acceptedSocket->waitForBytesWritten(3000));

    QVERIFY(socket.waitForReadyRead(3000));
    QNetworkDatagram datagram = socket.readDatagram();
    QVERIFY(datagram.isValid());
    QCOMPARE(datagram.data(), ba);

    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socket.error(), QAbstractSocket::UnknownSocketError);
    QCOMPARE(socket.errorString(), QString("Unknown error"));
    QCOMPARE(acceptedSocket->state(), QAbstractSocket::ConnectedState);
    QCOMPARE(acceptedSocket->error(), QAbstractSocket::UnknownSocketError);
    QCOMPARE(acceptedSocket->errorString(), QString("Unknown error"));
}

//----------------------------------------------------------------------------------
void tst_QSctpSocket::loop_data()
{
    QTest::addColumn<QByteArray>("peterDatagram");
    QTest::addColumn<QByteArray>("paulDatagram");
    QTest::addColumn<int>("peterChannel");
    QTest::addColumn<int>("paulChannel");

    QTest::newRow("\"Almond!\" | \"Joy!\"") << QByteArray("Almond!") << QByteArray("Joy!") << 0 << 0;
    QTest::newRow("\"A\" | \"B\"") << QByteArray("A") << QByteArray("B") << 1 << 1;
    QTest::newRow("\"AB\" | \"B\"") << QByteArray("AB") << QByteArray("B") << 0 << 1;
    QTest::newRow("\"AB\" | \"BB\"") << QByteArray("AB") << QByteArray("BB") << 1 << 0;
    QTest::newRow("\"A\\0B\" | \"B\\0B\"") << QByteArray::fromRawData("A\0B", 3) << QByteArray::fromRawData("B\0B", 3) << 0 << 1;
    QTest::newRow("BigDatagram") << QByteArray(600, '@') << QByteArray(600, '@') << 1 << 0;
}

void tst_QSctpSocket::loop()
{
    QFETCH(QByteArray, peterDatagram);
    QFETCH(QByteArray, paulDatagram);
    QFETCH(int, peterChannel);
    QFETCH(int, paulChannel);

    QSctpServer server;

    server.setMaximumChannelCount(10);
    QVERIFY(server.listen());

    QSctpSocket peter;
    peter.setMaximumChannelCount(10);
    peter.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(peter.waitForConnected(3000));

    QVERIFY(server.waitForNewConnection(3000));
    QSctpSocket *paul = server.nextPendingDatagramConnection();
    QVERIFY(paul);

    peter.setCurrentWriteChannel(peterChannel);
    QVERIFY(peter.writeDatagram(peterDatagram));
    paul->setCurrentWriteChannel(paulChannel);
    QVERIFY(paul->writeDatagram(paulDatagram));
    QVERIFY(peter.flush());
    QVERIFY(paul->flush());

    peter.setCurrentReadChannel(paulChannel);
    QVERIFY(peter.waitForReadyRead(3000));
    QCOMPARE(peter.bytesAvailable(), paulDatagram.size());
    QCOMPARE(peter.readDatagram().data(), paulDatagram);

    paul->setCurrentReadChannel(peterChannel);
    QVERIFY(paul->waitForReadyRead(3000));
    QCOMPARE(paul->bytesAvailable(), peterDatagram.size());
    QCOMPARE(paul->readDatagram().data(), peterDatagram);
}

//----------------------------------------------------------------------------------
void tst_QSctpSocket::loopInTCPMode_data()
{
    QTest::addColumn<QByteArray>("peterDatagram");
    QTest::addColumn<QByteArray>("paulDatagram");

    QTest::newRow("\"Almond!\" | \"Joy!\"") << QByteArray("Almond!") << QByteArray("Joy!");
    QTest::newRow("\"A\" | \"B\"") << QByteArray("A") << QByteArray("B");
    QTest::newRow("\"AB\" | \"B\"") << QByteArray("AB") << QByteArray("B");
    QTest::newRow("\"AB\" | \"BB\"") << QByteArray("AB") << QByteArray("BB");
    QTest::newRow("\"A\\0B\" | \"B\\0B\"") << QByteArray::fromRawData("A\0B", 3) << QByteArray::fromRawData("B\0B", 3);
    QTest::newRow("BigDatagram") << QByteArray(600, '@') << QByteArray(600, '@');
}

void tst_QSctpSocket::loopInTCPMode()
{
    QFETCH(QByteArray, peterDatagram);
    QFETCH(QByteArray, paulDatagram);

    QSctpServer server;

    server.setMaximumChannelCount(-1);
    QVERIFY(server.listen());

    QSctpSocket peter;
    peter.setMaximumChannelCount(-1);
    peter.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(peter.waitForConnected(3000));
    QVERIFY(server.waitForNewConnection(3000));

    QTcpSocket *paul = server.nextPendingConnection();
    QVERIFY(paul);

    QCOMPARE(peter.write(peterDatagram), qint64(peterDatagram.size()));
    QCOMPARE(paul->write(paulDatagram), qint64(paulDatagram.size()));
    QVERIFY(peter.flush());
    QVERIFY(paul->flush());

    QVERIFY(peter.waitForReadyRead(3000));
    QVERIFY(paul->waitForReadyRead(3000));

    QCOMPARE(peter.bytesAvailable(), paulDatagram.size());
    QByteArray peterBuffer = peter.readAll();

    QCOMPARE(paul->bytesAvailable(), peterDatagram.size());
    QByteArray paulBuffer = paul->readAll();

    QCOMPARE(peterBuffer, paulDatagram);
    QCOMPARE(paulBuffer, peterDatagram);
}

//----------------------------------------------------------------------------------
void tst_QSctpSocket::exitLoopSlot()
{
    exitLoop();
}

//----------------------------------------------------------------------------------
void tst_QSctpSocket::readDatagramAfterClose()
{
    QSctpServer server;

    QVERIFY(server.listen());

    QSctpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(socket.waitForConnected(3000));
    QVERIFY(server.waitForNewConnection(3000));

    QSctpSocket *acceptedSocket = server.nextPendingDatagramConnection();
    QVERIFY(acceptedSocket);

    connect(&socket, &QIODevice::readyRead, this, &tst_QSctpSocket::exitLoopSlot);

    QByteArray ba(1000, 1);
    QVERIFY(acceptedSocket->writeDatagram(ba));

    enterLoop(10);
    if (timeout())
        QFAIL("Network operation timed out");

    QCOMPARE(socket.bytesAvailable(), ba.size());
    socket.close();
    QVERIFY(!socket.readDatagram().isValid());
}

// Test buffered socket properly send data on delayed disconnect
void tst_QSctpSocket::clientSendDataOnDelayedDisconnect()
{
    QSctpServer server;

    QVERIFY(server.listen());

    // Connect to server, write data and close socket
    QSctpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(socket.waitForConnected(3000));

    QByteArray sendData("GET /\r\n");
    sendData = sendData.repeated(1000);
    QVERIFY(socket.writeDatagram(sendData));
    socket.close();
    QCOMPARE(socket.state(), QAbstractSocket::ClosingState);
    QVERIFY(socket.waitForDisconnected(3000));

    QVERIFY(server.waitForNewConnection(3000));
    QSctpSocket *acceptedSocket = server.nextPendingDatagramConnection();
    QVERIFY(acceptedSocket);

    QVERIFY(acceptedSocket->waitForReadyRead(3000));
    QNetworkDatagram datagram = acceptedSocket->readDatagram();
    QVERIFY(datagram.isValid());
    QCOMPARE(datagram.data(), sendData);
}

QTEST_MAIN(tst_QSctpSocket)

#include "tst_qsctpsocket.moc"
