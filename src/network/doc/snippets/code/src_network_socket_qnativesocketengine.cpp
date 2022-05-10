// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QNativeSocketEngine socketLayer;
socketLayer.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol);
socketLayer.connectToHost(QHostAddress::LocalHost, 22);
// returns false

socketLayer.waitForWrite();
socketLayer.connectToHost(QHostAddress::LocalHost, 22);
// returns true
//! [0]


//! [1]
QNativeSocketEngine socketLayer;
socketLayer.bind(QHostAddress::Any, 4000);
socketLayer.listen();
if (socketLayer.waitForRead()) {
    int clientSocket = socketLayer.accept();
    // a client is connected
}
//! [1]
