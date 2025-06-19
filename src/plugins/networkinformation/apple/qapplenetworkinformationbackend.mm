// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtNetwork/private/qnetworkinformation_p.h>

#include <QtNetwork/private/qnetconmonitor_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcNetInfoSCR)
Q_LOGGING_CATEGORY(lcNetInfoSCR, "qt.network.info.applenetworkinfo");

static QString backendName()
{
    return QString::fromUtf16(QNetworkInformationBackend::PluginNames
                                      [QNetworkInformationBackend::PluginNamesAppleIndex]);
}

class QAppleNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QAppleNetworkInformationBackend();
    ~QAppleNetworkInformationBackend();

    QString name() const override { return backendName(); }
    QNetworkInformation::Features featuresSupported() const override
    {
        return featuresSupportedStatic();
    }

    static QNetworkInformation::Features featuresSupportedStatic()
    {
        return QNetworkInformation::Features(QNetworkInformation::Feature::Reachability
                                             | QNetworkInformation::Feature::TransportMedium);
    }

private Q_SLOTS:
    void reachabilityChanged(bool isOnline);
    void interfaceTypeChanged(QNetworkConnectionMonitor::InterfaceType type);

private:
    Q_DISABLE_COPY_MOVE(QAppleNetworkInformationBackend)

    QNetworkConnectionMonitor probe;
};

class QAppleNetworkInformationBackendFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QNetworkInformationBackendFactory_iid)
    Q_INTERFACES(QNetworkInformationBackendFactory)
public:
    QAppleNetworkInformationBackendFactory() = default;
    ~QAppleNetworkInformationBackendFactory() = default;
    QString name() const override { return backendName(); }
    QNetworkInformation::Features featuresSupported() const override
    {
        return QAppleNetworkInformationBackend::featuresSupportedStatic();
    }

    QNetworkInformationBackend *create(
        QNetworkInformation::Features requiredFeatures) const override
    {
        if ((requiredFeatures & featuresSupported()) != requiredFeatures)
            return nullptr;
        return new QAppleNetworkInformationBackend();
    }

private:
    Q_DISABLE_COPY_MOVE(QAppleNetworkInformationBackendFactory)
};

QAppleNetworkInformationBackend::QAppleNetworkInformationBackend()
{
    connect(&probe, &QNetworkConnectionMonitor::reachabilityChanged, this,
            &QAppleNetworkInformationBackend::reachabilityChanged,
            Qt::QueuedConnection);
    connect(&probe, &QNetworkConnectionMonitor::interfaceTypeChanged, this,
            &QAppleNetworkInformationBackend::interfaceTypeChanged,
            Qt::QueuedConnection);
    probe.startMonitoring();
}

QAppleNetworkInformationBackend::~QAppleNetworkInformationBackend()
{
}

void QAppleNetworkInformationBackend::reachabilityChanged(bool isOnline)
{
    setReachability(isOnline ? QNetworkInformation::Reachability::Online
                             : QNetworkInformation::Reachability::Disconnected);
}

void QAppleNetworkInformationBackend::interfaceTypeChanged(
    QNetworkConnectionMonitor::InterfaceType type)
{

    if (reachability() == QNetworkInformation::Reachability::Disconnected) {
        setTransportMedium(QNetworkInformation::TransportMedium::Unknown);
    } else {
        switch (type) {
        case QNetworkConnectionMonitor::InterfaceType::Ethernet:
            setTransportMedium(QNetworkInformation::TransportMedium::Ethernet);
            break;
        case QNetworkConnectionMonitor::InterfaceType::Cellular:
            setTransportMedium(QNetworkInformation::TransportMedium::Cellular);
            break;
        case QNetworkConnectionMonitor::InterfaceType::WiFi:
            setTransportMedium(QNetworkInformation::TransportMedium::WiFi);
            break;
        case QNetworkConnectionMonitor::InterfaceType::Unknown:
            setTransportMedium(QNetworkInformation::TransportMedium::Unknown);
            break;
        }
    }
}

QT_END_NAMESPACE

#include "qapplenetworkinformationbackend.moc"
