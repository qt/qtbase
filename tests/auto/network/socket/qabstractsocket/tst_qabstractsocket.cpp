// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qabstractsocket.h>

class tst_QAbstractSocket : public QObject
{
Q_OBJECT

public:
    tst_QAbstractSocket();
    virtual ~tst_QAbstractSocket();

private slots:
    void getSetCheck();
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

QTEST_MAIN(tst_QAbstractSocket)
#include "tst_qabstractsocket.moc"
