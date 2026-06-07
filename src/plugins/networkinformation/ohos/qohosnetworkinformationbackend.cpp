// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtCore/qglobal.h>
#include <QtCore/qloggingcategory.h>
#include <QtNetwork/private/qnetworkinformation_p.h>
#include <qohosnetconnection_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcNetInfoOhos)
Q_LOGGING_CATEGORY(lcNetInfoOhos, "qt.network.info.ohos");

namespace {

QString backendName()
{
    return QString::fromUtf16(
        QNetworkInformationBackend::PluginNames[QNetworkInformationBackend::PluginNamesOhosIndex]);
}

class QOhosNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QOhosNetworkInformationBackend();

    QString name() const override;
    QNetworkInformation::Features featuresSupported() const override;

    static QNetworkInformation::Features featuresSupportedStatic();

private:
    void updateConnectivity();

    QOhosSupplier<QtOhosNetConnection::NetState> m_netStateSupplier;

    Q_DISABLE_COPY_MOVE(QOhosNetworkInformationBackend)
};

class QOhosNetworkInformationBackendFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QNetworkInformationBackendFactory_iid)
    Q_INTERFACES(QNetworkInformationBackendFactory)
public:
    QOhosNetworkInformationBackendFactory() = default;
    ~QOhosNetworkInformationBackendFactory() override = default;
    QString name() const override;
    QNetworkInformation::Features featuresSupported() const override;
    QNetworkInformationBackend *
    create(QNetworkInformation::Features requiredFeatures) const override;

private:
    Q_DISABLE_COPY_MOVE(QOhosNetworkInformationBackendFactory)
};

QNetworkInformation::TransportMedium mapTransportMedium(QtOhosNetConnection::NetBearType bearerType)
{
    switch (bearerType) {
    case QtOhosNetConnection::NetBearType::BEARER_CELLULAR:
        return QNetworkInformation::TransportMedium::Cellular;
    case QtOhosNetConnection::NetBearType::BEARER_WIFI:
        return QNetworkInformation::TransportMedium::WiFi;
    case QtOhosNetConnection::NetBearType::BEARER_BLUETOOTH:
        return QNetworkInformation::TransportMedium::Bluetooth;
    case QtOhosNetConnection::NetBearType::BEARER_ETHERNET:
        return QNetworkInformation::TransportMedium::Ethernet;
    case QtOhosNetConnection::NetBearType::BEARER_VPN:
        return QNetworkInformation::TransportMedium::Unknown;
    }

    return QNetworkInformation::TransportMedium::Unknown;
}

QNetworkInformation::Reachability mapReachability(QtOhosNetConnection::NetworkReachability reachability)
{
    switch (reachability) {
    case QtOhosNetConnection::NetworkReachability::Disconnected:
        return QNetworkInformation::Reachability::Disconnected;
    case QtOhosNetConnection::NetworkReachability::Local:
        return QNetworkInformation::Reachability::Local;
    case QtOhosNetConnection::NetworkReachability::Site:
        return QNetworkInformation::Reachability::Site;
    case QtOhosNetConnection::NetworkReachability::Online:
        return QNetworkInformation::Reachability::Online;
    }
    return QNetworkInformation::Reachability::Unknown;
}

QString QOhosNetworkInformationBackend::name() const
{
    return backendName();
}

QNetworkInformation::Features QOhosNetworkInformationBackend::featuresSupported() const
{
    return featuresSupportedStatic();
}

QNetworkInformation::Features QOhosNetworkInformationBackend::featuresSupportedStatic()
{
    return QNetworkInformation::Feature::Reachability
        | QNetworkInformation::Feature::TransportMedium;
}

QOhosNetworkInformationBackend::QOhosNetworkInformationBackend()
    : m_netStateSupplier(QtOhosNetConnection::makeOhosNetStateDataSource(
          [this](const QtOhosNetConnection::NetState &) {
              updateConnectivity();
          }))
{
    updateConnectivity();
}

void QOhosNetworkInformationBackend::updateConnectivity()
{
    const auto state = m_netStateSupplier();

    setReachability(mapReachability(state.reachability));

    auto transport = state.transport
        ? mapTransportMedium(*state.transport)
        : QNetworkInformation::TransportMedium::Unknown;
    setTransportMedium(transport);

    qCDebug(lcNetInfoOhos) << "updated reachability:" << reachability()
        << "transport:" << transportMedium();
}

QString QOhosNetworkInformationBackendFactory::name() const
{
    return backendName();
}

QNetworkInformation::Features QOhosNetworkInformationBackendFactory::featuresSupported() const
{
    return QOhosNetworkInformationBackend::featuresSupportedStatic();
}

QNetworkInformationBackend *
QOhosNetworkInformationBackendFactory::create(QNetworkInformation::Features requiredFeatures) const
{
    if ((requiredFeatures & featuresSupported()) != requiredFeatures)
        return nullptr;
    return new QOhosNetworkInformationBackend();
}

}

QT_END_NAMESPACE

#include "qohosnetworkinformationbackend.moc"
