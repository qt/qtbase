/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QBEARERENGINE_IMPL_H
#define QBEARERENGINE_IMPL_H

#include <QtNetwork/private/qbearerengine_p.h>

QT_BEGIN_NAMESPACE

class QBearerEngineImpl : public QBearerEngine
{
    Q_OBJECT

public:
    enum ConnectionError {
        InterfaceLookupError = 0,
        ConnectError,
        OperationNotSupported,
        DisconnectionError,
    };

    QBearerEngineImpl(QObject *parent = nullptr) : QBearerEngine(parent) {}
    ~QBearerEngineImpl() {}

    virtual void connectToId(const QString &id) = 0;
    virtual void disconnectFromId(const QString &id) = 0;

    virtual QString getInterfaceFromId(const QString &id) = 0;

    virtual QNetworkSession::State sessionStateForId(const QString &id) = 0;

    virtual quint64 bytesWritten(const QString &) { return Q_UINT64_C(0); }
    virtual quint64 bytesReceived(const QString &) { return Q_UINT64_C(0); }
    virtual quint64 startTime(const QString &) { return Q_UINT64_C(0); }

Q_SIGNALS:
    void connectionError(const QString &id, QBearerEngineImpl::ConnectionError error);
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QBearerEngineImpl::ConnectionError)

#endif // QBEARERENGINE_IMPL_H
