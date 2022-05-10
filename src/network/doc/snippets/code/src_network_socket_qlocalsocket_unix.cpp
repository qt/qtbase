// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
socket->connectToServer("market");
if (socket->waitForConnected(1000))
    qDebug("Connected!");
//! [0]


//! [1]
socket->disconnectFromServer();
if (socket->state() == QLocalSocket::UnconnectedState
    || socket->waitForDisconnected(1000)) {
    qDebug("Disconnected!");
}
//! [1]
