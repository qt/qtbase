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
        return QNetworkInformation::Features(Feature::Reachability | Feature::CaptivePortal
                                             | Feature::Metered);
    }

    bool isValid() const;

private:
    Q_DISABLE_COPY_MOVE(QGlibNetworkInformationBackend)

    static void updateConnectivity(QGlibNetworkInformationBackend *backend);
    static void updateMetered(QGlibNetworkInformationBackend *backend);

    GNetworkMonitor *networkMonitor = nullptr;
    gulong connectivityHandlerId = 0;
    gulong meteredHandlerId = 0;
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
    updateConnectivity(this);
    updateMetered(this);

    connectivityHandlerId = g_signal_connect_swapped(networkMonitor, "notify::connectivity",
                                                     G_CALLBACK(updateConnectivity), this);

    meteredHandlerId = g_signal_connect_swapped(networkMonitor, "notify::network-metered",
                                                G_CALLBACK(updateMetered), this);
}

QGlibNetworkInformationBackend::~QGlibNetworkInformationBackend()
{
    g_signal_handler_disconnect(networkMonitor, meteredHandlerId);
    g_signal_handler_disconnect(networkMonitor, connectivityHandlerId);
}

bool QGlibNetworkInformationBackend::isValid() const
{
    return QLatin1StringView(G_OBJECT_TYPE_NAME(networkMonitor)) != "GNetworkMonitorBase"_L1;
}

void QGlibNetworkInformationBackend::updateConnectivity(QGlibNetworkInformationBackend *backend)
{
    const auto connectivityState = g_network_monitor_get_connectivity(backend->networkMonitor);
    const bool behindPortal = (connectivityState == G_NETWORK_CONNECTIVITY_PORTAL);
    backend->setReachability(reachabilityFromGNetworkConnectivity(connectivityState));
    backend->setBehindCaptivePortal(behindPortal);
}

void QGlibNetworkInformationBackend::updateMetered(QGlibNetworkInformationBackend *backend)
{
    backend->setMetered(g_network_monitor_get_network_metered(backend->networkMonitor));
}

QT_END_NAMESPACE

#include "qglibnetworkinformationbackend.moc"
