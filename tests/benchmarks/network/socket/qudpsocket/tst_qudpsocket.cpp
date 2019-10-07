/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/qglobal.h>
#include <QtCore/qcoreapplication.h>
#include <QtNetwork/qudpsocket.h>
#include <QtNetwork/qnetworkdatagram.h>

class tst_QUdpSocket : public QObject
{
    Q_OBJECT
public:
    tst_QUdpSocket();

private slots:
    void pendingDatagramSize_data();
    void pendingDatagramSize();
};

tst_QUdpSocket::tst_QUdpSocket()
{
}

void tst_QUdpSocket::pendingDatagramSize_data()
{
    QTest::addColumn<int>("size");
    for (int value : {52, 1024, 2049, 4500, 4098, 8192, 12000, 25000, 32 * 1024, 63 * 1024})
        QTest::addRow("%d", value) << value;
}

void tst_QUdpSocket::pendingDatagramSize()
{
    QFETCH(int, size);
    QUdpSocket socket;
    socket.bind();

    QNetworkDatagram datagram;
    datagram.setData(QByteArray(size, 'a'));
    datagram.setDestination(QHostAddress::SpecialAddress::LocalHost, socket.localPort());

    auto sent = socket.writeDatagram(datagram);
    QCOMPARE(sent, size);

    auto res = QTest::qWaitFor([&socket]() { return socket.hasPendingDatagrams(); }, 5000);
    QVERIFY(res);

    QBENCHMARK {
        auto pendingSize = socket.pendingDatagramSize();
        Q_UNUSED(pendingSize);
    }
}

QTEST_MAIN(tst_QUdpSocket)
#include "tst_qudpsocket.moc"
