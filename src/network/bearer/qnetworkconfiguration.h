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

#ifndef QNETWORKCONFIGURATION_H
#define QNETWORKCONFIGURATION_H

#if 0
#pragma qt_class(QNetworkConfiguration)
#endif

#include <QtNetwork/qtnetworkglobal.h>

#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetatype.h>

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class QNetworkConfigurationPrivate;
class QT_DEPRECATED_BEARER_MANAGEMENT Q_NETWORK_EXPORT QNetworkConfiguration
{
public:
    QNetworkConfiguration();
    QNetworkConfiguration(const QNetworkConfiguration& other);
    QNetworkConfiguration &operator=(QNetworkConfiguration &&other) noexcept { swap(other); return *this; }
    QNetworkConfiguration &operator=(const QNetworkConfiguration &other);
    ~QNetworkConfiguration();

    void swap(QNetworkConfiguration &other) noexcept { qSwap(d, other.d); }

    bool operator==(const QNetworkConfiguration &other) const;
    inline bool operator!=(const QNetworkConfiguration &other) const
    { return !operator==(other); }

    enum Type {
        InternetAccessPoint = 0,
        ServiceNetwork,
        UserChoice,
        Invalid
    };

    enum Purpose {
        UnknownPurpose = 0,
        PublicPurpose,
        PrivatePurpose,
        ServiceSpecificPurpose
    };

    enum StateFlag {
        Undefined        = 0x0000001,
        Defined          = 0x0000002,
        Discovered       = 0x0000006,
        Active           = 0x000000e
    };
    Q_DECLARE_FLAGS(StateFlags, StateFlag)

    enum BearerType {
        BearerUnknown,
        BearerEthernet,
        BearerWLAN,
        Bearer2G,
        BearerCDMA2000,
        BearerWCDMA,
        BearerHSPA,
        BearerBluetooth,
        BearerWiMAX,
        BearerEVDO,
        BearerLTE,
        Bearer3G,
        Bearer4G
    };

    StateFlags state() const;
    Type type() const;
    Purpose purpose() const;

    BearerType bearerType() const;
    BearerType bearerTypeFamily() const;
    QString bearerTypeName() const;

    QString identifier() const;
    bool isRoamingAvailable() const;
    QList<QNetworkConfiguration> children() const;

    QString name() const;
    bool isValid() const;

    int connectTimeout() const;
    bool setConnectTimeout(int timeout);

private:
    friend class QNetworkConfigurationPrivate;
    friend class QNetworkConfigurationManager;
    friend class QNetworkConfigurationManagerPrivate;
    friend class QNetworkSessionPrivate;
    QExplicitlySharedDataPointer<QNetworkConfigurationPrivate> d;
};

Q_DECLARE_SHARED(QNetworkConfiguration)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QNetworkConfiguration)

#endif

QT_WARNING_POP

#endif // QNETWORKCONFIGURATION_H
