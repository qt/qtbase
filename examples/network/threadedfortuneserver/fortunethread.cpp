// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "fortunethread.h"

#include <QtNetwork>

//! [0]
FortuneThread::FortuneThread(qintptr socketDescriptor, const QString &fortune, QObject *parent)
    : QThread(parent), socketDescriptor(socketDescriptor), text(fortune)
{
}
//! [0]

//! [1]
void FortuneThread::run()
{
    QTcpSocket tcpSocket;
//! [1] //! [2]
    if (!tcpSocket.setSocketDescriptor(socketDescriptor)) {
        emit error(tcpSocket.error());
        return;
    }
//! [2] //! [3]

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_5);
    out << text;
//! [3] //! [4]

    tcpSocket.write(block);
    tcpSocket.disconnectFromHost();
    tcpSocket.waitForDisconnected();
}
//! [4]
