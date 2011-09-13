/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qabstractsocket.h>

//TESTED_CLASS=
//TESTED_FILES=

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
