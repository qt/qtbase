/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "connectionmanager.h"
#include "peerwireclient.h"
#include "ratecontroller.h"
#include "torrentclient.h"
#include "torrentserver.h"

Q_GLOBAL_STATIC(TorrentServer, torrentServer)

TorrentServer *TorrentServer::instance()
{
    return torrentServer();
}

void TorrentServer::addClient(TorrentClient *client)
{
    clients << client;
}

void TorrentServer::removeClient(TorrentClient *client)
{
    clients.removeAll(client);
}

void TorrentServer::incomingConnection(qintptr socketDescriptor)
{
    PeerWireClient *client =
        new PeerWireClient(ConnectionManager::instance()->clientId(), this);

    if (client->setSocketDescriptor(socketDescriptor)) {
        if (ConnectionManager::instance()->canAddConnection() && !clients.isEmpty()) {
            connect(client, &PeerWireClient::infoHashReceived,
                    this, &TorrentServer::processInfoHash);
            connect(client, &PeerWireClient::errorOccurred,
                    this, QOverload<>::of(&TorrentServer::removeClient));
            RateController::instance()->addSocket(client);
            ConnectionManager::instance()->addConnection(client);
            return;
        }
    }
    client->abort();
    delete client;
}

void TorrentServer::removeClient()
{
    PeerWireClient *peer = qobject_cast<PeerWireClient *>(sender());
    RateController::instance()->removeSocket(peer);
    ConnectionManager::instance()->removeConnection(peer);
    peer->deleteLater();
}

void TorrentServer::processInfoHash(const QByteArray &infoHash)
{
    PeerWireClient *peer = qobject_cast<PeerWireClient *>(sender());
    for (TorrentClient *client : qAsConst(clients)) {
        if (client->state() >= TorrentClient::Searching && client->infoHash() == infoHash) {
            peer->disconnect(peer, nullptr, this, nullptr);
            client->setupIncomingConnection(peer);
            return;
        }
    }
    removeClient();
}
