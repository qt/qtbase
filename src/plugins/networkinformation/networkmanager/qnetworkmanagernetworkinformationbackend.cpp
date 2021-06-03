/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtNetwork/private/qnetworkinformation_p.h>

#include "qnetworkmanagerservice.h"

#include <QtCore/qglobal.h>
#include <QtCore/private/qobject_p.h>

#include <QtDBus/qdbusmessage.h>

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcNetInfoNLM)
Q_LOGGING_CATEGORY(lcNetInfoNM, "qt.network.info.networkmanager");

namespace {
QNetworkInformation::Reachability reachabilityFromNMState(QNetworkManagerInterface::NMState state)
{
    switch (state) {
    case QNetworkManagerInterface::NM_STATE_UNKNOWN:
    case QNetworkManagerInterface::NM_STATE_ASLEEP:
    case QNetworkManagerInterface::NM_STATE_CONNECTING:
        return QNetworkInformation::Reachability::Unknown;
    case QNetworkManagerInterface::NM_STATE_DISCONNECTING: // No point in starting new connections:
    case QNetworkManagerInterface::NM_STATE_DISCONNECTED:
        return QNetworkInformation::Reachability::Disconnected;
    case QNetworkManagerInterface::NM_STATE_CONNECTED_LOCAL:
        return QNetworkInformation::Reachability::Local;
    case QNetworkManagerInterface::NM_STATE_CONNECTED_SITE:
        return QNetworkInformation::Reachability::Site;
    case QNetworkManagerInterface::NM_STATE_CONNECTED_GLOBAL:
        return QNetworkInformation::Reachability::Online;
    }
    return QNetworkInformation::Reachability::Unknown;
}
}

static QString backendName = QStringLiteral("networkmanager");

class QNetworkManagerNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QNetworkManagerNetworkInformationBackend();
    ~QNetworkManagerNetworkInformationBackend() = default;

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

    bool isValid() const { return iface.isValid(); }

private:
    Q_DISABLE_COPY_MOVE(QNetworkManagerNetworkInformationBackend)

    QNetworkManagerInterface iface;
};

class QNetworkManagerNetworkInformationBackendFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QNetworkInformationBackendFactory_iid)
    Q_INTERFACES(QNetworkInformationBackendFactory)
public:
    QNetworkManagerNetworkInformationBackendFactory() = default;
    ~QNetworkManagerNetworkInformationBackendFactory() = default;
    QString name() const override { return backendName; }
    QNetworkInformation::Features featuresSupported() const override
    {
        if (!QNetworkManagerInterfaceBase::networkManagerAvailable())
            return {};
        return QNetworkManagerNetworkInformationBackend::featuresSupportedStatic();
    }

    QNetworkInformationBackend *create(QNetworkInformation::Features requiredFeatures) const override
    {
        if ((requiredFeatures & featuresSupported()) != requiredFeatures)
            return nullptr;
        if (!QNetworkManagerInterfaceBase::networkManagerAvailable())
            return nullptr;
        auto backend = new QNetworkManagerNetworkInformationBackend();
        if (!backend->isValid())
            delete std::exchange(backend, nullptr);
        return backend;
    }
private:
    Q_DISABLE_COPY_MOVE(QNetworkManagerNetworkInformationBackendFactory)
};

QNetworkManagerNetworkInformationBackend::QNetworkManagerNetworkInformationBackend()
{
    using NMState = QNetworkManagerInterface::NMState;
    setReachability(reachabilityFromNMState(iface.state()));
    connect(&iface, &QNetworkManagerInterface::stateChanged, this,
            [this](NMState newState) {
                setReachability(reachabilityFromNMState(newState));
            });

    using ConnectivityState = QNetworkManagerInterface::NMConnectivityState;

    const auto connectivityState = iface.connectivityState();
    const bool behindPortal = (connectivityState == ConnectivityState::NM_CONNECTIVITY_PORTAL);
    setBehindCaptivePortal(behindPortal);

    connect(&iface, &QNetworkManagerInterface::connectivityChanged, this,
            [this](ConnectivityState state) {
                const bool behindPortal = (state == ConnectivityState::NM_CONNECTIVITY_PORTAL);
                setBehindCaptivePortal(behindPortal);
            });
}

QT_END_NAMESPACE

#include "qnetworkmanagernetworkinformationbackend.moc"
