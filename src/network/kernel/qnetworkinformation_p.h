/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
public:
    QNetworkInformationBackend() = default;
    ~QNetworkInformationBackend() override;

    virtual QString name() const = 0;
    virtual QNetworkInformation::Features featuresSupported() const = 0;

    QNetworkInformation::Reachability reachability() const { return m_reachability; }
    bool behindCaptivePortal() const { return m_behindCaptivePortal; }

Q_SIGNALS:
    void reachabilityChanged();
    void behindCaptivePortalChanged();

protected:
    void setReachability(QNetworkInformation::Reachability reachability)
    {
        if (m_reachability != reachability) {
            m_reachability = reachability;
            emit reachabilityChanged();
        }
    }

    void setBehindCaptivePortal(bool behindPortal)
    {
        if (m_behindCaptivePortal != behindPortal) {
            m_behindCaptivePortal = behindPortal;
            emit behindCaptivePortalChanged();
        }
    }

private:
    QNetworkInformation::Reachability m_reachability = QNetworkInformation::Reachability::Unknown;
    bool m_behindCaptivePortal = false;

    Q_DISABLE_COPY_MOVE(QNetworkInformationBackend)
    friend class QNetworkInformation;
    friend class QNetworkInformationPrivate;
};

class Q_NETWORK_EXPORT QNetworkInformationBackendFactory : public QObject
{
    Q_OBJECT
public:
    QNetworkInformationBackendFactory();
    virtual ~QNetworkInformationBackendFactory();
    virtual QString name() const = 0;
    virtual QNetworkInformationBackend *create(QNetworkInformation::Features requiredFeatures) const = 0;
    virtual QNetworkInformation::Features featuresSupported() const = 0;

private:
    Q_DISABLE_COPY_MOVE(QNetworkInformationBackendFactory)
};
#define QNetworkInformationBackendFactory_iid "org.qt-project.Qt.NetworkInformationBackendFactory"
Q_DECLARE_INTERFACE(QNetworkInformationBackendFactory, QNetworkInformationBackendFactory_iid);

QT_END_NAMESPACE

#endif
