// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QSctpSocket *socket = new QSctpSocket(this);

socket->setMaxChannelCount(16);
socket->connectToHost(QHostAddress::LocalHost, 1973);

if (socket->waitForConnected(1000)) {
    int inputChannels = socket->readChannelCount();
    int outputChannels = socket->writeChannelCount();

    ....
}
//! [0]
