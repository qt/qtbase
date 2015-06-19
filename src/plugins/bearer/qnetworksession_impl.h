/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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
        : engine(0), startTime(0), lastError(QNetworkSession::UnknownSessionError), sessionTimeout(-1), currentPolicies(QNetworkSession::NoPolicy), opened(false)
    {}
    ~QNetworkSessionPrivateImpl()
    {}

    //called by QNetworkSession constructor and ensures
    //that the state is immediately updated (w/o actually opening
    //a session). Also this function should take care of
    //notification hooks to discover future state changes.
    void syncStateWithInterface() Q_DECL_OVERRIDE;

#ifndef QT_NO_NETWORKINTERFACE
    QNetworkInterface currentInterface() const Q_DECL_OVERRIDE;
#endif
    QVariant sessionProperty(const QString& key) const Q_DECL_OVERRIDE;
    void setSessionProperty(const QString& key, const QVariant& value) Q_DECL_OVERRIDE;

    void open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;
    void stop() Q_DECL_OVERRIDE;
    void migrate() Q_DECL_OVERRIDE;
    void accept() Q_DECL_OVERRIDE;
    void ignore() Q_DECL_OVERRIDE;
    void reject() Q_DECL_OVERRIDE;

    QString errorString() const Q_DECL_OVERRIDE; //must return translated string
    QNetworkSession::SessionError error() const Q_DECL_OVERRIDE;

    quint64 bytesWritten() const Q_DECL_OVERRIDE;
    quint64 bytesReceived() const Q_DECL_OVERRIDE;
    quint64 activeTime() const Q_DECL_OVERRIDE;

    QNetworkSession::UsagePolicies usagePolicies() const Q_DECL_OVERRIDE;
    void setUsagePolicies(QNetworkSession::UsagePolicies) Q_DECL_OVERRIDE;

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
