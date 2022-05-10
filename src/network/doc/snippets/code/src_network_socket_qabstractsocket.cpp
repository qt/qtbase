// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
socket->connectToHost("imap", 143);
if (socket->waitForConnected(1000))
    qDebug("Connected!");
//! [0]


//! [1]
socket->disconnectFromHost();
if (socket->state() == QAbstractSocket::UnconnectedState
    || socket->waitForDisconnected(1000)) {
        qDebug("Disconnected!");
}
//! [1]


//! [2]
socket->setProxy(QNetworkProxy::NoProxy);
//! [2]
