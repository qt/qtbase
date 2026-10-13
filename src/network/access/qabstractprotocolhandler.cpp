// Copyright (C) 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <private/qabstractprotocolhandler_p.h>
#include <private/qhttpnetworkconnectionchannel_p.h>

QT_BEGIN_NAMESPACE

QAbstractProtocolHandler::QAbstractProtocolHandler(QHttpNetworkConnectionChannel *channel)
    : m_channel(channel), m_reply(nullptr), m_socket(m_channel->socket), m_connection(m_channel->connection)
{
    Q_ASSERT(m_channel);
    Q_ASSERT(m_socket);
    Q_ASSERT(m_connection);
}

QAbstractProtocolHandler::~QAbstractProtocolHandler()
{
}

void QAbstractProtocolHandler::setReply(QHttpNetworkReply *reply)
{
    m_reply = reply;
}

QT_END_NAMESPACE
