/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include <QtNetwork/private/qnetworkinformation_p.h>

#include <QtNetwork/private/qnetconmonitor_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcNetInfoSCR)
Q_LOGGING_CATEGORY(lcNetInfoSCR, "qt.network.info.scnetworkreachability");


static QString backendName = QStringLiteral("scnetworkreachability");

class QSCNetworkReachabilityNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QSCNetworkReachabilityNetworkInformationBackend();
    ~QSCNetworkReachabilityNetworkInformationBackend();

    QString name() const override { return backendName; }
    QNetworkInformation::Features featuresSupported() const override
    {
        return featuresSupportedStatic();
    }

    static QNetworkInformation::Features featuresSupportedStatic()
    {
        return QNetworkInformation::Features(QNetworkInformation::Feature::Reachability);
    }

private Q_SLOTS:
    void reachabilityChanged(bool isOnline);

private:
    Q_DISABLE_COPY_MOVE(QSCNetworkReachabilityNetworkInformationBackend);

    QNetworkConnectionMonitor ipv4Probe;
    QNetworkConnectionMonitor ipv6Probe;
};

class QSCNetworkReachabilityNetworkInformationBackendFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QNetworkInformationBackendFactory_iid)
    Q_INTERFACES(QNetworkInformationBackendFactory)
public:
    QSCNetworkReachabilityNetworkInformationBackendFactory() = default;
    ~QSCNetworkReachabilityNetworkInformationBackendFactory() = default;
    QString name() const override { return backendName; }
    QNetworkInformation::Features featuresSupported() const override
    {
        return QSCNetworkReachabilityNetworkInformationBackend::featuresSupportedStatic();
    }

    QNetworkInformationBackend *create(QNetworkInformation::Features requiredFeatures) const override
    {
        if ((requiredFeatures & featuresSupported()) != requiredFeatures)
            return nullptr;
        return new QSCNetworkReachabilityNetworkInformationBackend();
    }

private:
    Q_DISABLE_COPY_MOVE(QSCNetworkReachabilityNetworkInformationBackendFactory);
};

QSCNetworkReachabilityNetworkInformationBackend::QSCNetworkReachabilityNetworkInformationBackend()
{
    bool isOnline = false;
    if (ipv4Probe.setTargets(QHostAddress::AnyIPv4, {})) {
        // We manage to create SCNetworkReachabilityRef for IPv4, let's
        // read the last known state then!
        isOnline |= ipv4Probe.isReachable();
        ipv4Probe.startMonitoring();
    }

    if (ipv6Probe.setTargets(QHostAddress::AnyIPv6, {})) {
        // We manage to create SCNetworkReachability ref for IPv6, let's
        // read the last known state then!
        isOnline |= ipv6Probe.isReachable();
        ipv6Probe.startMonitoring();
    }
    reachabilityChanged(isOnline);

    connect(&ipv4Probe, &QNetworkConnectionMonitor::reachabilityChanged, this,
            &QSCNetworkReachabilityNetworkInformationBackend::reachabilityChanged,
            Qt::QueuedConnection);
    connect(&ipv6Probe, &QNetworkConnectionMonitor::reachabilityChanged, this,
            &QSCNetworkReachabilityNetworkInformationBackend::reachabilityChanged,
            Qt::QueuedConnection);
}

QSCNetworkReachabilityNetworkInformationBackend::~QSCNetworkReachabilityNetworkInformationBackend()
{
}

void QSCNetworkReachabilityNetworkInformationBackend::reachabilityChanged(bool isOnline)
{
    setReachability(isOnline ? QNetworkInformation::Reachability::Online
                             : QNetworkInformation::Reachability::Disconnected);
}

QT_END_NAMESPACE

#include "qscnetworkreachabilitynetworkinformationbackend.moc"
