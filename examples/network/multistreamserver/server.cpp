/****************************************************************************
**
** Copyright (C) 2015 Alex Trotsenko <alex1973tr@gmail.com>
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

#include <QtWidgets>
#include <QtNetwork>
#include <QtAlgorithms>

#include "server.h"
#include "movieprovider.h"
#include "timeprovider.h"
#include "chatprovider.h"

#include "../shared/sctpchannels.h"

Server::Server(QWidget *parent)
    : QDialog(parent)
    , providers(SctpChannels::NumberOfChannels)
{
    setWindowTitle(tr("Multi-stream Server"));

    sctpServer = new QSctpServer(this);
    sctpServer->setMaximumChannelCount(NumberOfChannels);

    statusLabel = new QLabel;
    QPushButton *quitButton = new QPushButton(tr("Quit"));

    providers[SctpChannels::Movie] = new MovieProvider(this);
    providers[SctpChannels::Time] = new TimeProvider(this);
    providers[SctpChannels::Chat] = new ChatProvider(this);

    connect(sctpServer, &QSctpServer::newConnection, this, &Server::newConnection);
    connect(quitButton, &QPushButton::clicked, this, &Server::accept);
    connect(providers[SctpChannels::Movie], &Provider::writeDatagram, this, &Server::writeDatagram);
    connect(providers[SctpChannels::Time], &Provider::writeDatagram, this, &Server::writeDatagram);
    connect(providers[SctpChannels::Chat], &Provider::writeDatagram, this, &Server::writeDatagram);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(quitButton);
    setLayout(mainLayout);
}

Server::~Server()
{
    qDeleteAll(connections.begin(), connections.end());
}

int Server::exec()
{
    if (!sctpServer->listen()) {
        QMessageBox::critical(this, windowTitle(),
                              tr("Unable to start the server: %1.")
                              .arg(sctpServer->errorString()));
        return QDialog::Rejected;
    }

    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    statusLabel->setText(tr("The server is running on\n\nIP: %1\nport: %2\n\n"
                            "Run the Multi-stream Client example now.")
                         .arg(ipAddress).arg(sctpServer->serverPort()));

    return QDialog::exec();
}

void Server::newConnection()
{
    QSctpSocket *connection = sctpServer->nextPendingDatagramConnection();

    connections.append(connection);
    connect(connection, &QSctpSocket::channelReadyRead, this, &Server::readDatagram);
    connect(connection, &QSctpSocket::disconnected, this, &Server::clientDisconnected);

    for (Provider *provider : providers)
        provider->newConnection(*connection);
}

void Server::clientDisconnected()
{
    QSctpSocket *connection = static_cast<QSctpSocket *>(sender());

    connections.removeOne(connection);
    connection->disconnect();

    for (Provider *provider : providers)
        provider->clientDisconnected(*connection);

    connection->deleteLater();
}

void Server::readDatagram(int channel)
{
    QSctpSocket *connection = static_cast<QSctpSocket *>(sender());

    connection->setCurrentReadChannel(channel);
    providers[channel]->readDatagram(*connection, connection->readDatagram().data());
}

void Server::writeDatagram(QSctpSocket *to, const QByteArray &ba)
{
    int channel = providers.indexOf(static_cast<Provider *>(sender()));

    if (to) {
        to->setCurrentWriteChannel(channel);
        to->writeDatagram(ba);
        return;
    }

    for (QSctpSocket *connection : connections) {
        connection->setCurrentWriteChannel(channel);
        connection->writeDatagram(ba);
    }
}
