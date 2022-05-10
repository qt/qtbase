// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNETWORKINFORMATION_P_H
#define QNETWORKINFORMATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the Network Information API. This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <QtNetwork/qnetworkinformation.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

class Q_NETWORK_EXPORT QNetworkInformationBackend : public QObject
{
    Q_OBJECT

    using Reachability = QNetworkInformation::Reachability;
    using TransportMedium = QNetworkInformation::TransportMedium;

public:
    static inline const char16_t PluginNames[4][22] = {
        { u"networklistmanager" },
        { u"scnetworkreachability" },
        { u"android" },
        { u"networkmanager" },
    };
    static constexpr int PluginNamesWindowsIndex = 0;
    static constexpr int PluginNamesAppleIndex = 1;
    static constexpr int PluginNamesAndroidIndex = 2;
    static constexpr int PluginNamesLinuxIndex = 3;

    QNetworkInformationBackend() = default;
    ~QNetworkInformationBackend() override;

    virtual QString name() const = 0;
    virtual QNetworkInformation::Features featuresSupported() const = 0;

    Reachability reachability() const { return m_reachability; }
    bool behindCaptivePortal() const { return m_behindCaptivePortal; }
    TransportMedium transportMedium() const { return m_transportMedium; }
    bool isMetered() const { return m_metered; }

Q_SIGNALS:
    void reachabilityChanged(Reachability reachability);
    void behindCaptivePortalChanged(bool behindPortal);
    void transportMediumChanged(TransportMedium medium);
    void isMeteredChanged(bool isMetered);

protected:
    void setReachability(QNetworkInformation::Reachability reachability)
    {
        if (m_reachability != reachability) {
            m_reachability = reachability;
            emit reachabilityChanged(reachability);
        }
    }

    void setBehindCaptivePortal(bool behindPortal)
    {
        if (m_behindCaptivePortal != behindPortal) {
            m_behindCaptivePortal = behindPortal;
            emit behindCaptivePortalChanged(behindPortal);
        }
    }

    void setTransportMedium(TransportMedium medium)
    {
        if (m_transportMedium != medium) {
            m_transportMedium = medium;
            emit transportMediumChanged(medium);
        }
    }

    void setMetered(bool isMetered)
    {
        if (m_metered != isMetered) {
            m_metered = isMetered;
            emit isMeteredChanged(isMetered);
        }
    }

private:
    Reachability m_reachability = Reachability::Unknown;
    TransportMedium m_transportMedium = TransportMedium::Unknown;
    bool m_behindCaptivePortal = false;
    bool m_metered = false;

    Q_DISABLE_COPY_MOVE(QNetworkInformationBackend)
    friend class QNetworkInformation;
    friend class QNetworkInformationPrivate;
};

class Q_NETWORK_EXPORT QNetworkInformationBackendFactory : public QObject
{
    Q_OBJECT

    using Features = QNetworkInformation::Features;

public:
    QNetworkInformationBackendFactory();
    virtual ~QNetworkInformationBackendFactory();
    virtual QString name() const = 0;
    virtual QNetworkInformationBackend *create(Features requiredFeatures) const = 0;
    virtual Features featuresSupported() const = 0;

private:
    Q_DISABLE_COPY_MOVE(QNetworkInformationBackendFactory)
};
#define QNetworkInformationBackendFactory_iid "org.qt-project.Qt.NetworkInformationBackendFactory"
Q_DECLARE_INTERFACE(QNetworkInformationBackendFactory, QNetworkInformationBackendFactory_iid);

QT_END_NAMESPACE

#endif
