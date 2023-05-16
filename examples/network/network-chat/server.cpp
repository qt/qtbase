// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "connection.h"
#include "server.h"

Server::Server(QObject *parent)
    : QTcpServer(parent)
{
    listen(QHostAddress::Any);
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    Connection *connection = new Connection(socketDescriptor, this);
    emit newConnection(connection);
}
