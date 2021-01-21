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

#ifndef QNETWORKSESSION_IMPL_H
#define QNETWORKSESSION_IMPL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qbearerengine_impl.h"

#include <QtNetwork/private/qnetworkconfigmanager_p.h>
#include <QtNetwork/private/qnetworksession_p.h>

QT_BEGIN_NAMESPACE

class QBearerEngineImpl;

class QNetworkSessionPrivateImpl : public QNetworkSessionPrivate
{
    Q_OBJECT

public:
    QNetworkSessionPrivateImpl()
        : engine(nullptr), startTime(0), lastError(QNetworkSession::UnknownSessionError), sessionTimeout(-1), currentPolicies(QNetworkSession::NoPolicy), opened(false)
    {}
    ~QNetworkSessionPrivateImpl()
    {}

    //called by QNetworkSession constructor and ensures
    //that the state is immediately updated (w/o actually opening
    //a session). Also this function should take care of
    //notification hooks to discover future state changes.
    void syncStateWithInterface() override;

#ifndef QT_NO_NETWORKINTERFACE
    QNetworkInterface currentInterface() const override;
#endif
    QVariant sessionProperty(const QString& key) const override;
    void setSessionProperty(const QString& key, const QVariant& value) override;

    void open() override;
    void close() override;
    void stop() override;
    void migrate() override;
    void accept() override;
    void ignore() override;
    void reject() override;

    QString errorString() const override; //must return translated string
    QNetworkSession::SessionError error() const override;

    quint64 bytesWritten() const override;
    quint64 bytesReceived() const override;
    quint64 activeTime() const override;

    QNetworkSession::UsagePolicies usagePolicies() const override;
    void setUsagePolicies(QNetworkSession::UsagePolicies) override;

private Q_SLOTS:
    void networkConfigurationsChanged();
    void configurationChanged(QNetworkConfigurationPrivatePointer config);
    void forcedSessionClose(const QNetworkConfiguration &config);
    void connectionError(const QString &id, QBearerEngineImpl::ConnectionError error);
    void decrementTimeout();

private:
    void updateStateFromServiceNetwork();
    void updateStateFromActiveConfig();

private:
    QBearerEngineImpl *engine;

    quint64 startTime;

    QNetworkSession::SessionError lastError;

    int sessionTimeout;
    QNetworkSession::UsagePolicies currentPolicies;

    bool opened;
};

QT_END_NAMESPACE

#endif // QNETWORKSESSION_IMPL_H
