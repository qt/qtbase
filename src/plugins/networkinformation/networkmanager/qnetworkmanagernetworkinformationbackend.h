// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKMANAGERINFORMATIONBACKEND_H
#define QNETWORKMANAGERINFORMATIONBACKEND_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qnetworkinformation_p.h>
#include "qnetworkmanagerservice.h"

QT_BEGIN_NAMESPACE

class QNetworkManagerNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QNetworkManagerNetworkInformationBackend();
    ~QNetworkManagerNetworkInformationBackend() = default;

    QString name() const override;
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
                                             | Feature::TransportMedium | Feature::Metered);
    }

    bool isValid() const { return iface.isValid(); }

    void onStateChanged(QNetworkManagerInterface::NMState state);
    void onConnectivityChanged(QNetworkManagerInterface::NMConnectivityState connectivityState);
    void onDeviceTypeChanged(QNetworkManagerInterface::NMDeviceType deviceType);
    void onMeteredChanged(QNetworkManagerInterface::NMMetered metered);

private:
    Q_DISABLE_COPY_MOVE(QNetworkManagerNetworkInformationBackend)

    QNetworkManagerInterface iface;
};

QT_END_NAMESPACE

#endif
