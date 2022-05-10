// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
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
