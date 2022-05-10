// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "chatprovider.h"
#include <QString>
#include <QSctpSocket>
#include <QHostAddress>

ChatProvider::ChatProvider(QObject *parent)
    : Provider(parent)
{
}

void ChatProvider::readDatagram(QSctpSocket &from, const QByteArray &ba)
{
    emit writeDatagram(0, QString(QLatin1String("<%1:%2> %3"))
                      .arg(from.peerAddress().toString())
                      .arg(QString::number(from.peerPort()))
                      .arg(QString::fromUtf8(ba)).toUtf8());
}

void ChatProvider::newConnection(QSctpSocket &client)
{
    readDatagram(client, QString(tr("has joined")).toUtf8());
}

void ChatProvider::clientDisconnected(QSctpSocket &client)
{
    readDatagram(client, QString(tr("has left")).toUtf8());
}
