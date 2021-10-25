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
