// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "client.h"
#include "connection.h"
#include "peermanager.h"

#include <QNetworkInterface>
#include <QUuid>

static const qint32 BroadcastInterval = 2000;
static const unsigned broadcastPort = 45000;

PeerManager::PeerManager(Client *client)
    : QObject(client), client(client)
{
    static const char *envVariables[] = {
        "USERNAME", "USER", "USERDOMAIN", "HOSTNAME", "DOMAINNAME"
    };

    for (const char *varname : envVariables) {
        username = qEnvironmentVariable(varname);
        if (!username.isNull())
            break;
    }

    if (username.isEmpty())
        username = "unknown";

    // We generate a unique per-process identifier so we can avoid multiple
    // connections to/from the same remote peer as well as ignore our own
    // broadcasts.
    localUniqueId = QUuid::createUuid().toByteArray();

    updateAddresses();

    broadcastSocket.bind(QHostAddress::Any, broadcastPort, QUdpSocket::ShareAddress
                         | QUdpSocket::ReuseAddressHint);
    connect(&broadcastSocket, &QUdpSocket::readyRead,
            this, &PeerManager::readBroadcastDatagram);

    broadcastTimer.setInterval(BroadcastInterval);
    connect(&broadcastTimer, &QTimer::timeout,
            this, &PeerManager::sendBroadcastDatagram);
}

void PeerManager::setServerPort(int port)
{
    serverPort = port;
}

QString PeerManager::userName() const
{
    return username;
}

QByteArray PeerManager::uniqueId() const
{
    return localUniqueId;
}

void PeerManager::startBroadcasting()
{
    broadcastTimer.start();
}

bool PeerManager::isLocalHostAddress(const QHostAddress &address) const
{
    return ipAddresses.contains(address);
}

void PeerManager::sendBroadcastDatagram()
{
    QByteArray datagram;
    {
        QCborStreamWriter writer(&datagram);
        writer.startArray(2);
        writer.append(localUniqueId);
        writer.append(serverPort);
        writer.endArray();
    }

    bool validBroadcastAddresses = true;
    for (const QHostAddress &address : std::as_const(broadcastAddresses)) {
        if (broadcastSocket.writeDatagram(datagram, address, broadcastPort) == -1)
            validBroadcastAddresses = false;
    }

    if (!validBroadcastAddresses)
        updateAddresses();
}

void PeerManager::readBroadcastDatagram()
{
    while (broadcastSocket.hasPendingDatagrams()) {
        QHostAddress senderIp;
        quint16 senderPort;
        QByteArray datagram;
        datagram.resize(broadcastSocket.pendingDatagramSize());
        if (broadcastSocket.readDatagram(datagram.data(), datagram.size(),
                                         &senderIp, &senderPort) == -1)
            continue;

        int senderServerPort;
        QByteArray peerUniqueId;
        {
            // decode the datagram
            QCborStreamReader reader(datagram);
            if (reader.lastError() != QCborError::NoError || !reader.isArray())
                continue;
            if (!reader.isLengthKnown() || reader.length() != 2)
                continue;

            reader.enterContainer();
            if (reader.lastError() != QCborError::NoError || !reader.isByteArray())
                continue;
            auto r = reader.readByteArray();
            while (r.status == QCborStreamReader::Ok) {
                peerUniqueId = r.data;
                r = reader.readByteArray();
            }

            if (reader.lastError() != QCborError::NoError || !reader.isUnsignedInteger())
                continue;
            senderServerPort = reader.toInteger();
        }

        if (peerUniqueId == localUniqueId)
            continue;

        if (!client->hasConnection(peerUniqueId)) {
            Connection *connection = new Connection(this);
            emit newConnection(connection);
            connection->connectToHost(senderIp, senderServerPort);
        }
    }
}

void PeerManager::updateAddresses()
{
    broadcastAddresses.clear();
    ipAddresses.clear();
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &interface : interfaces) {
        const QList<QNetworkAddressEntry> entries = interface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            QHostAddress broadcastAddress = entry.broadcast();
            if (broadcastAddress != QHostAddress::Null && entry.ip() != QHostAddress::LocalHost) {
                broadcastAddresses << broadcastAddress;
                ipAddresses << entry.ip();
            }
        }
    }
}
