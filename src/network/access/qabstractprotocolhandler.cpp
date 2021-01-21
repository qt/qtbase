/****************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
