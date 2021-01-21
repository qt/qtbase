/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QNETWORKSESSIONPRIVATE_H
#define QNETWORKSESSIONPRIVATE_H

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

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "qnetworksession.h"
#include "qnetworkconfiguration_p.h"
#include "QtCore/qsharedpointer.h"

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class Q_NETWORK_EXPORT QNetworkSessionPrivate : public QObject
{
    Q_OBJECT

    friend class QNetworkSession;

public:
    QNetworkSessionPrivate() : QObject(),
        state(QNetworkSession::Invalid), isOpen(false)
    {}
    virtual ~QNetworkSessionPrivate()
    {}

    //called by QNetworkSession constructor and ensures
    //that the state is immediately updated (w/o actually opening
    //a session). Also this function should take care of
    //notification hooks to discover future state changes.
    virtual void syncStateWithInterface() = 0;

#ifndef QT_NO_NETWORKINTERFACE
    virtual QNetworkInterface currentInterface() const = 0;
#endif
    virtual QVariant sessionProperty(const QString &key) const = 0;
    virtual void setSessionProperty(const QString &key, const QVariant &value) = 0;

    virtual void open() = 0;
    virtual void close() = 0;
    virtual void stop() = 0;

    virtual void setALREnabled(bool /*enabled*/) {}
    virtual void migrate() = 0;
    virtual void accept() = 0;
    virtual void ignore() = 0;
    virtual void reject() = 0;

    virtual QString errorString() const = 0; //must return translated string
    virtual QNetworkSession::SessionError error() const = 0;

    virtual quint64 bytesWritten() const = 0;
    virtual quint64 bytesReceived() const = 0;
    virtual quint64 activeTime() const = 0;

    virtual QNetworkSession::UsagePolicies usagePolicies() const = 0;
    virtual void setUsagePolicies(QNetworkSession::UsagePolicies) = 0;

    static void setUsagePolicies(QNetworkSession&, QNetworkSession::UsagePolicies); //for unit testing
protected:
    inline QNetworkConfigurationPrivatePointer privateConfiguration(const QNetworkConfiguration &config) const
    {
        return config.d;
    }

    inline void setPrivateConfiguration(QNetworkConfiguration &config,
                                        const QNetworkConfigurationPrivatePointer &ptr) const
    {
        config.d = ptr;
    }

Q_SIGNALS:
    //releases any pending waitForOpened() calls
    void quitPendingWaitsForOpened();

    void error(QNetworkSession::SessionError error);
    void stateChanged(QNetworkSession::State state);
    void closed();
    void newConfigurationActivated();
    void preferredConfigurationChanged(const QNetworkConfiguration &config, bool isSeamless);
    void usagePoliciesChanged(QNetworkSession::UsagePolicies);

protected:
    QNetworkSession *q;

    // The config set on QNetworkSession.
    QNetworkConfiguration publicConfig;

    // If publicConfig is a ServiceNetwork this is a copy of publicConfig.
    // If publicConfig is an UserChoice that is resolved to a ServiceNetwork this is the actual
    // ServiceNetwork configuration.
    QNetworkConfiguration serviceConfig;

    // This is the actual active configuration currently in use by the session.
    // Either a copy of publicConfig or one of serviceConfig.children().
    QNetworkConfiguration activeConfig;

    QNetworkSession::State state;
    bool isOpen;

    QRecursiveMutex mutex;
};

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

#endif // QNETWORKSESSIONPRIVATE_H
