// Copyright (C) 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTPROTOCOLHANDLER_H
#define QABSTRACTPROTOCOLHANDLER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE

class QHttpNetworkConnectionChannel;
class QHttpNetworkReply;
class QAbstractSocket;
class QHttpNetworkConnection;

class QAbstractProtocolHandler {
public:
    QAbstractProtocolHandler(QHttpNetworkConnectionChannel *channel);
    virtual ~QAbstractProtocolHandler();

    virtual void _q_receiveReply() = 0;
    virtual void _q_readyRead() = 0;
    virtual bool sendRequest() = 0;
    void setReply(QHttpNetworkReply *reply);

protected:
    QHttpNetworkConnectionChannel *m_channel;
    QHttpNetworkReply *m_reply;
    QAbstractSocket *m_socket;
    QHttpNetworkConnection *m_connection;
};

QT_END_NAMESPACE

#endif // QABSTRACTPROTOCOLHANDLER_H
