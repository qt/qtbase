// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtNetwork/private/qnetworkinformation_p.h>

#include <QtNetwork/private/qnetconmonitor_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcNetInfoSCR)
Q_LOGGING_CATEGORY(lcNetInfoSCR, "qt.network.info.scnetworkreachability");

static QString backendName()
{
    return QString::fromUtf16(QNetworkInformationBackend::PluginNames
                                      [QNetworkInformationBackend::PluginNamesAppleIndex]);
}

class QSCNetworkReachabilityNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QSCNetworkReachabilityNetworkInformationBackend();
    ~QSCNetworkReachabilityNetworkInformationBackend();

    QString name() const override { return backendName(); }
    QNetworkInformation::Features featuresSupported() const override
    {
        return featuresSupportedStatic();
    }

    static QNetworkInformation::Features featuresSupportedStatic()
    {
        return QNetworkInformation::Features(QNetworkInformation::Feature::Reachability
#ifdef QT_PLATFORM_UIKIT
                                             | QNetworkInformation::Feature::TransportMedium
#endif
                                             );
    }

private Q_SLOTS:
    void reachabilityChanged(bool isOnline);

#ifdef QT_PLATFORM_UIKIT
    void isWwanChanged(bool isOnline);
#endif

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
    QString name() const override { return backendName(); }
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
#ifdef QT_PLATFORM_UIKIT
    bool isWwan = false;
#endif
    if (ipv4Probe.setTargets(QHostAddress::AnyIPv4, {})) {
        // We manage to create SCNetworkReachabilityRef for IPv4, let's
        // read the last known state then!
        isOnline |= ipv4Probe.isReachable();
#ifdef QT_PLATFORM_UIKIT
        isWwan |= ipv4Probe.isWwan();
#endif
        ipv4Probe.startMonitoring();
    }

    if (ipv6Probe.setTargets(QHostAddress::AnyIPv6, {})) {
        // We manage to create SCNetworkReachability ref for IPv6, let's
        // read the last known state then!
        isOnline |= ipv6Probe.isReachable();
#ifdef QT_PLATFORM_UIKIT
        isWwan |= ipv6Probe.isWwan();
#endif
        ipv6Probe.startMonitoring();
    }
    reachabilityChanged(isOnline);
#ifdef QT_PLATFORM_UIKIT
    isWwanChanged(isWwan);
#endif

    connect(&ipv4Probe, &QNetworkConnectionMonitor::reachabilityChanged, this,
            &QSCNetworkReachabilityNetworkInformationBackend::reachabilityChanged,
            Qt::QueuedConnection);
    connect(&ipv6Probe, &QNetworkConnectionMonitor::reachabilityChanged, this,
            &QSCNetworkReachabilityNetworkInformationBackend::reachabilityChanged,
            Qt::QueuedConnection);

#ifdef QT_PLATFORM_UIKIT
    connect(&ipv4Probe, &QNetworkConnectionMonitor::isWwanChanged, this,
            &QSCNetworkReachabilityNetworkInformationBackend::isWwanChanged,
            Qt::QueuedConnection);
    connect(&ipv6Probe, &QNetworkConnectionMonitor::isWwanChanged, this,
            &QSCNetworkReachabilityNetworkInformationBackend::isWwanChanged,
            Qt::QueuedConnection);
#endif
}

QSCNetworkReachabilityNetworkInformationBackend::~QSCNetworkReachabilityNetworkInformationBackend()
{
}

void QSCNetworkReachabilityNetworkInformationBackend::reachabilityChanged(bool isOnline)
{
    setReachability(isOnline ? QNetworkInformation::Reachability::Online
                             : QNetworkInformation::Reachability::Disconnected);
}

#ifdef QT_PLATFORM_UIKIT
void QSCNetworkReachabilityNetworkInformationBackend::isWwanChanged(bool isWwan)
{
    // The reachability API from Apple only has one entry regarding transport medium: "IsWWAN"[0].
    // This is _serviceable_ on iOS where the only other credible options are "WLAN" or
    // "Disconnected". But on macOS you could be connected by Ethernet as well, so how would that be
    // reported? It doesn't matter anyway since "IsWWAN" is not available on macOS.
    // [0]: https://developer.apple.com/documentation/systemconfiguration/scnetworkreachabilityflags/kscnetworkreachabilityflagsiswwan?language=objc
    if (reachability() == QNetworkInformation::Reachability::Disconnected) {
        setTransportMedium(QNetworkInformation::TransportMedium::Unknown);
    } else {
        setTransportMedium(isWwan ? QNetworkInformation::TransportMedium::Cellular
                                  : QNetworkInformation::TransportMedium::WiFi);
    }
}
#endif

QT_END_NAMESPACE

#include "qscnetworkreachabilitynetworkinformationbackend.moc"
