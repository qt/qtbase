// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    void Server::readPendingDatagrams()
    {
        while (udpSocket->hasPendingDatagrams()) {
            QNetworkDatagram datagram = udpSocket->receiveDatagram();
            QByteArray replyData = processThePayload(datagram.data());
            udpSocket->writeDatagram(datagram.makeReply(replyData));
        }
    }
//! [0]

//! [1]
    udpSocket->writeDatagram(std::move(datagram).makeReply(replyData));
//! [1]
