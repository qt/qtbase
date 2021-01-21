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

#ifndef QNETWORKSESSION_H
#define QNETWORKSESSION_H

#if 0
#pragma qt_class(QNetworkSession)
#endif

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtNetwork/qnetworkinterface.h>
#include <QtCore/qvariant.h>
#include <QtNetwork/qnetworkconfiguration.h>

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

#ifndef QT_NO_BEARERMANAGEMENT

#if defined(Q_OS_WIN) && defined(interface)
#undef interface
#endif

#include <QtCore/qshareddata.h>
QT_BEGIN_NAMESPACE

class QNetworkSessionPrivate;
class QT_DEPRECATED_BEARER_MANAGEMENT Q_NETWORK_EXPORT QNetworkSession : public QObject
{
    Q_OBJECT

public:
    enum State {
        Invalid = 0,
        NotAvailable,
        Connecting,
        Connected,
        Closing,
        Disconnected,
        Roaming
    };

    enum SessionError {
        UnknownSessionError = 0,
        SessionAbortedError,
        RoamingError,
        OperationNotSupportedError,
        InvalidConfigurationError
    };

    enum UsagePolicy {
        NoPolicy = 0,
        NoBackgroundTrafficPolicy = 1
    };

    Q_DECLARE_FLAGS(UsagePolicies, UsagePolicy)

    explicit QNetworkSession(const QNetworkConfiguration &connConfig, QObject *parent = nullptr);
    virtual ~QNetworkSession();

    bool isOpen() const;
    QNetworkConfiguration configuration() const;
#ifndef QT_NO_NETWORKINTERFACE
    QNetworkInterface interface() const;
#endif

    State state() const;
    SessionError error() const;
    QString errorString() const;
    QVariant sessionProperty(const QString &key) const;
    void setSessionProperty(const QString &key, const QVariant &value);

    quint64 bytesWritten() const;
    quint64 bytesReceived() const;
    quint64 activeTime() const;

    QNetworkSession::UsagePolicies usagePolicies() const;

    bool waitForOpened(int msecs = 30000);

public Q_SLOTS:
    void open();
    void close();
    void stop();

    //roaming related slots
    void migrate();
    void ignore();
    void accept();
    void reject();

Q_SIGNALS:
    void stateChanged(QNetworkSession::State);
    void opened();
    void closed();
    void error(QNetworkSession::SessionError);
    void preferredConfigurationChanged(const QNetworkConfiguration &config, bool isSeamless);
    void newConfigurationActivated();
    void usagePoliciesChanged(QNetworkSession::UsagePolicies usagePolicies);

protected:
    virtual void connectNotify(const QMetaMethod &signal) override;
    virtual void disconnectNotify(const QMetaMethod &signal) override;

private:
    Q_DISABLE_COPY(QNetworkSession)
    friend class QNetworkSessionPrivate;
    QNetworkSessionPrivate *d;
};

QT_END_NAMESPACE
Q_DECLARE_METATYPE(QNetworkSession::State)
Q_DECLARE_METATYPE(QNetworkSession::SessionError)
Q_DECLARE_METATYPE(QNetworkSession::UsagePolicies)

#endif // QT_NO_BEARERMANAGEMENT

QT_WARNING_POP

#endif // QNETWORKSESSION_H
