// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtNetwork/private/qnetworkinformation_p.h>

#include "qnetworklistmanagerevents.h"

#include <QtCore/qglobal.h>
#include <QtCore/private/qobject_p.h>
#include <QtCore/qscopeguard.h>

#include <QtCore/private/qfunctions_win_p.h>

QT_BEGIN_NAMESPACE

// Declared in qnetworklistmanagerevents.h
Q_LOGGING_CATEGORY(lcNetInfoNLM, "qt.network.info.netlistmanager");

static QString backendName()
{
    return QString::fromUtf16(QNetworkInformationBackend::PluginNames
                                      [QNetworkInformationBackend::PluginNamesWindowsIndex]);
}

namespace {
bool testCONNECTIVITY(NLM_CONNECTIVITY connectivity, NLM_CONNECTIVITY flag)
{
    return (connectivity & flag) == flag;
}

QNetworkInformation::Reachability reachabilityFromNLM_CONNECTIVITY(NLM_CONNECTIVITY connectivity)
{
    if (connectivity == NLM_CONNECTIVITY_DISCONNECTED)
        return QNetworkInformation::Reachability::Disconnected;
    if (testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV6_INTERNET)
        || testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV4_INTERNET)) {
        return QNetworkInformation::Reachability::Online;
    }
    if (testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV6_SUBNET)
        || testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV4_SUBNET)) {
        return QNetworkInformation::Reachability::Site;
    }
    if (testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV6_LOCALNETWORK)
        || testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV4_LOCALNETWORK)) {
        return QNetworkInformation::Reachability::Local;
    }
    if (testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV6_NOTRAFFIC)
        || testCONNECTIVITY(connectivity, NLM_CONNECTIVITY_IPV4_NOTRAFFIC)) {
        return QNetworkInformation::Reachability::Unknown;
    }

    return QNetworkInformation::Reachability::Unknown;
}
}

class QNetworkListManagerNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QNetworkListManagerNetworkInformationBackend();
    ~QNetworkListManagerNetworkInformationBackend();

    QString name() const override { return backendName(); }
    QNetworkInformation::Features featuresSupported() const override
    {
        return featuresSupportedStatic();
    }

    static QNetworkInformation::Features featuresSupportedStatic()
    {
        return QNetworkInformation::Features(QNetworkInformation::Feature::Reachability
                                             | QNetworkInformation::Feature::CaptivePortal
#if QT_CONFIG(cpp_winrt)
                                             | QNetworkInformation::Feature::TransportMedium
                                             | QNetworkInformation::Feature::Metered
#endif
                                             );
    }

    [[nodiscard]] bool start();
    void stop();

private:
    bool event(QEvent *event) override;
    void setConnectivity(NLM_CONNECTIVITY newConnectivity);
    void checkCaptivePortal();

    QComHelper comHelper;

    ComPtr<QNetworkListManagerEvents> managerEvents;

    NLM_CONNECTIVITY connectivity = NLM_CONNECTIVITY_DISCONNECTED;

    bool monitoring = false;
};

class QNetworkListManagerNetworkInformationBackendFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QNetworkInformationBackendFactory_iid)
    Q_INTERFACES(QNetworkInformationBackendFactory)
public:
    QNetworkListManagerNetworkInformationBackendFactory() = default;
    ~QNetworkListManagerNetworkInformationBackendFactory() = default;
    QString name() const override { return backendName(); }
    QNetworkInformation::Features featuresSupported() const override
    {
        return QNetworkListManagerNetworkInformationBackend::featuresSupportedStatic();
    }

    QNetworkInformationBackend *
    create(QNetworkInformation::Features requiredFeatures) const override
    {
        if ((requiredFeatures & featuresSupported()) != requiredFeatures)
            return nullptr;
        auto backend = new QNetworkListManagerNetworkInformationBackend();
        if (!backend->start()) {
            qCWarning(lcNetInfoNLM) << "Failed to start listening to events";
            delete backend;
            backend = nullptr;
        }
        return backend;
    }
};

QNetworkListManagerNetworkInformationBackend::QNetworkListManagerNetworkInformationBackend()
{
    if (!comHelper.isValid())
        return;

    managerEvents = new QNetworkListManagerEvents();
    connect(managerEvents.Get(), &QNetworkListManagerEvents::connectivityChanged, this,
            &QNetworkListManagerNetworkInformationBackend::setConnectivity);

    connect(managerEvents.Get(), &QNetworkListManagerEvents::transportMediumChanged, this,
            &QNetworkListManagerNetworkInformationBackend::setTransportMedium);

    connect(managerEvents.Get(), &QNetworkListManagerEvents::isMeteredChanged, this,
            &QNetworkListManagerNetworkInformationBackend::setMetered);
}

QNetworkListManagerNetworkInformationBackend::~QNetworkListManagerNetworkInformationBackend()
{
    stop();
}

void QNetworkListManagerNetworkInformationBackend::setConnectivity(NLM_CONNECTIVITY newConnectivity)
{
    if (reachabilityFromNLM_CONNECTIVITY(connectivity)
        != reachabilityFromNLM_CONNECTIVITY(newConnectivity)) {
        connectivity = newConnectivity;
        setReachability(reachabilityFromNLM_CONNECTIVITY(newConnectivity));

        // @future: only check if signal is connected
        checkCaptivePortal();
    }
}

void QNetworkListManagerNetworkInformationBackend::checkCaptivePortal()
{
    setBehindCaptivePortal(managerEvents->checkBehindCaptivePortal());
}

bool QNetworkListManagerNetworkInformationBackend::event(QEvent *event)
{
    if (event->type() == QEvent::ThreadChange)
        qFatal("Moving QNetworkListManagerNetworkInformationBackend to different thread is not supported");

    return QObject::event(event);
}

bool QNetworkListManagerNetworkInformationBackend::start()
{
    Q_ASSERT(!monitoring);

    if (!comHelper.isValid())
        return false;

    if (!managerEvents)
        managerEvents = new QNetworkListManagerEvents();

    if (managerEvents->start())
        monitoring = true;
    return monitoring;
}

void QNetworkListManagerNetworkInformationBackend::stop()
{
    if (monitoring) {
        Q_ASSERT(managerEvents);
        managerEvents->stop();
        monitoring = false;
        managerEvents.Reset();
    }
}

QT_END_NAMESPACE

#include "qnetworklistmanagernetworkinformationbackend.moc"
