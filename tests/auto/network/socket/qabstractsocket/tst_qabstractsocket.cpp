/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qabstractsocket.h>
#include <qtcpserver.h>
#include <qtcpsocket.h>
#ifndef QT_NO_SSL
#include <qsslsocket.h>
#endif

class tst_QAbstractSocket : public QObject
{
Q_OBJECT

public:
    tst_QAbstractSocket();
    virtual ~tst_QAbstractSocket();

private slots:
    void initTestCase();
    void getSetCheck();
    void serverDisconnectWithBuffered();
};

tst_QAbstractSocket::tst_QAbstractSocket()
{
}

tst_QAbstractSocket::~tst_QAbstractSocket()
{
}

class MyAbstractSocket : public QAbstractSocket
{
public:
    MyAbstractSocket() : QAbstractSocket(QAbstractSocket::TcpSocket, 0) {}
    void setLocalPort(quint16 port) { QAbstractSocket::setLocalPort(port); }
    void setPeerPort(quint16 port) { QAbstractSocket::setPeerPort(port); }
};

void tst_QAbstractSocket::initTestCase()
{
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
}

// Testing get/set functions
void tst_QAbstractSocket::getSetCheck()
{
    MyAbstractSocket obj1;
    // qint64 QAbstractSocket::readBufferSize()
    // void QAbstractSocket::setReadBufferSize(qint64)
    obj1.setReadBufferSize(qint64(0));
    QCOMPARE(qint64(0), obj1.readBufferSize());
    obj1.setReadBufferSize((Q_INT64_C(-9223372036854775807) - 1));
    QCOMPARE((Q_INT64_C(-9223372036854775807) - 1), obj1.readBufferSize());
    obj1.setReadBufferSize(Q_INT64_C(9223372036854775807));
    QCOMPARE(Q_INT64_C(9223372036854775807), obj1.readBufferSize());

    // quint16 QAbstractSocket::localPort()
    // void QAbstractSocket::setLocalPort(quint16)
    obj1.setLocalPort(quint16(0));
    QCOMPARE(quint16(0), obj1.localPort());
    obj1.setLocalPort(quint16(0xffff));
    QCOMPARE(quint16(0xffff), obj1.localPort());

    // quint16 QAbstractSocket::peerPort()
    // void QAbstractSocket::setPeerPort(quint16)
    obj1.setPeerPort(quint16(0));
    QCOMPARE(quint16(0), obj1.peerPort());
    obj1.setPeerPort(quint16(0xffff));
    QCOMPARE(quint16(0xffff), obj1.peerPort());
}

// Test buffered socket being properly closed on remote disconnect
void tst_QAbstractSocket::serverDisconnectWithBuffered()
{
    QTcpServer tcpServer;
#ifndef QT_NO_SSL
    QSslSocket testSocket;
#else
    QTcpSocket testSocket;
#endif

    QVERIFY(tcpServer.listen(QHostAddress::LocalHost));
    testSocket.connectToHost(tcpServer.serverAddress(), tcpServer.serverPort());
    // Accept connection on server side
    QVERIFY(tcpServer.waitForNewConnection(5000));
    QTcpSocket *newConnection = tcpServer.nextPendingConnection();
    // Send one char and drop link
    QVERIFY(newConnection != NULL);
    QVERIFY(newConnection->putChar(0));
    QVERIFY(newConnection->flush());
    delete newConnection;

    QVERIFY(testSocket.waitForConnected(5000)); // ready for write
    QVERIFY(testSocket.state() == QAbstractSocket::ConnectedState);

    QSignalSpy spyStateChanged(&testSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)));
    QSignalSpy spyDisconnected(&testSocket, SIGNAL(disconnected()));

    QVERIFY(testSocket.waitForReadyRead(5000)); // have one char already in internal buffer
    char buf[128];
    QCOMPARE(testSocket.read(buf, sizeof(buf)), Q_INT64_C(1));
    if (testSocket.state() != QAbstractSocket::UnconnectedState) {
        QVERIFY(testSocket.waitForDisconnected(5000));
        QVERIFY(testSocket.state() == QAbstractSocket::UnconnectedState);
    }
    // Test signal emitting
    QVERIFY(spyDisconnected.count() == 1);
    QVERIFY(spyStateChanged.count() > 0);
    QVERIFY(qvariant_cast<QAbstractSocket::SocketState>(spyStateChanged.last().first())
            == QAbstractSocket::UnconnectedState);
}

QTEST_MAIN(tst_QAbstractSocket)
#include "tst_qabstractsocket.moc"
