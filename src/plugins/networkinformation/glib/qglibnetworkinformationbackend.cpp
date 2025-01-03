// Copyright (C) 2021 Ilya Fedin <fedin-ilja2010@ya.ru>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtNetwork/private/qnetworkinformation_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/private/qobject_p.h>

#include <gio/gio.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_DECLARE_LOGGING_CATEGORY(lcNetInfoGlib)
Q_LOGGING_CATEGORY(lcNetInfoGlib, "qt.network.info.glib");

namespace {
QNetworkInformation::Reachability reachabilityFromGNetworkConnectivity(GNetworkConnectivity connectivity)
{
    switch (connectivity) {
    case G_NETWORK_CONNECTIVITY_LOCAL:
        return QNetworkInformation::Reachability::Disconnected;
    case G_NETWORK_CONNECTIVITY_LIMITED:
    case G_NETWORK_CONNECTIVITY_PORTAL:
        return QNetworkInformation::Reachability::Site;
    case G_NETWORK_CONNECTIVITY_FULL:
        return QNetworkInformation::Reachability::Online;
    }
    return QNetworkInformation::Reachability::Unknown;
}
}

static QString backendName = QStringLiteral("glib");

class QGlibNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QGlibNetworkInformationBackend();
    ~QGlibNetworkInformationBackend();

    QString name() const override { return backendName; }
    QNetworkInformation::Features featuresSupported() const override
    {
        if (!isValid())
            return {};
        return featuresSupportedStatic();
    }

    static QNetworkInformation::Features featuresSupportedStatic()
    {
        using Feature = QNetworkInformation::Feature;
        return QNetworkInformation::Features(Feature::Reachability | Feature::CaptivePortal);
    }

    bool isValid() const;

private:
    Q_DISABLE_COPY_MOVE(QGlibNetworkInformationBackend)

    static void updateInformation(QGlibNetworkInformationBackend *backend);

    GNetworkMonitor *networkMonitor = nullptr;
    gulong handlerId = 0;
};

class QGlibNetworkInformationBackendFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QNetworkInformationBackendFactory_iid)
    Q_INTERFACES(QNetworkInformationBackendFactory)
public:
    QGlibNetworkInformationBackendFactory() = default;
    ~QGlibNetworkInformationBackendFactory() = default;
    QString name() const override { return backendName; }
    QNetworkInformation::Features featuresSupported() const override
    {
        return QGlibNetworkInformationBackend::featuresSupportedStatic();
    }

    QNetworkInformationBackend *create(QNetworkInformation::Features requiredFeatures) const override
    {
        if ((requiredFeatures & featuresSupported()) != requiredFeatures)
            return nullptr;
        auto backend = new QGlibNetworkInformationBackend();
        if (!backend->isValid())
            delete std::exchange(backend, nullptr);
        return backend;
    }
private:
    Q_DISABLE_COPY_MOVE(QGlibNetworkInformationBackendFactory)
};

QGlibNetworkInformationBackend::QGlibNetworkInformationBackend()
: networkMonitor(g_network_monitor_get_default())
{
    updateInformation(this);

    handlerId = g_signal_connect_swapped(networkMonitor, "notify::connectivity",
                                         G_CALLBACK(updateInformation), this);
}

QGlibNetworkInformationBackend::~QGlibNetworkInformationBackend()
{
    g_signal_handler_disconnect(networkMonitor, handlerId);
}

bool QGlibNetworkInformationBackend::isValid() const
{
    return G_OBJECT_TYPE_NAME(networkMonitor) != "GNetworkMonitorBase"_L1;
}

void QGlibNetworkInformationBackend::updateInformation(QGlibNetworkInformationBackend *backend)
{
    const auto connectivityState = g_network_monitor_get_connectivity(backend->networkMonitor);
    const bool behindPortal = (connectivityState == G_NETWORK_CONNECTIVITY_PORTAL);
    backend->setReachability(reachabilityFromGNetworkConnectivity(connectivityState));
    backend->setBehindCaptivePortal(behindPortal);
}

QT_END_NAMESPACE

#include "qglibnetworkinformationbackend.moc"
