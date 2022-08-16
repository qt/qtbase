// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Kurt Pattyn <pattyn.kurt@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSSLSERVER_P_H
#define QSSLSERVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <QtCore/qhash.h>

#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/private/qtcpserver_p.h>
#include <utility>

QT_BEGIN_NAMESPACE

class Q_NETWORK_EXPORT QSslServerPrivate : public QTcpServerPrivate
{
public:
    Q_DECLARE_PUBLIC(QSslServer)

    QSslServerPrivate();
    void checkClientHelloAndContinue();
    void initializeHandshakeProcess(QSslSocket *socket);
    void removeSocketData(quintptr socket);

    struct SocketData {
        QMetaObject::Connection readyReadConnection;
        QMetaObject::Connection destroyedConnection;

        SocketData(QMetaObject::Connection readyRead, QMetaObject::Connection destroyed)
            : readyReadConnection(readyRead), destroyedConnection(destroyed)
        {
        }

        void disconnectSignals()
        {
            QObject::disconnect(std::exchange(readyReadConnection, {}));
            QObject::disconnect(std::exchange(destroyedConnection, {}));
        }
    };
    QHash<quintptr, SocketData> socketData;

    QSslConfiguration sslConfiguration;
};


QT_END_NAMESPACE

#endif // QSSLSERVER_P_H
