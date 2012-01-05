/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNETWORKMANAGERENGINE_P_H
#define QNETWORKMANAGERENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "../qbearerengine_impl.h"

#include "qnetworkmanagerservice.h"

#include <QMap>
#include <QVariant>

#ifndef QT_NO_BEARERMANAGEMENT
#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QNetworkManagerEngine : public QBearerEngineImpl
{
    Q_OBJECT

public:
    QNetworkManagerEngine(QObject *parent = 0);
    ~QNetworkManagerEngine();

    bool networkManagerAvailable() const;

    QString getInterfaceFromId(const QString &id);
    bool hasIdentifier(const QString &id);

    void connectToId(const QString &id);
    void disconnectFromId(const QString &id);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate();

    QNetworkSession::State sessionStateForId(const QString &id);

    quint64 bytesWritten(const QString &id);
    quint64 bytesReceived(const QString &id);
    quint64 startTime(const QString &id);

    QNetworkConfigurationManager::Capabilities capabilities() const;

    QNetworkSessionPrivate *createSessionBackend();

    QNetworkConfigurationPrivatePointer defaultConfiguration();

private Q_SLOTS:
    void interfacePropertiesChanged(const QString &path,
                                    const QMap<QString, QVariant> &properties);
    void activeConnectionPropertiesChanged(const QString &path,
                                           const QMap<QString, QVariant> &properties);
    void devicePropertiesChanged(const QString &path,
                                 const QMap<QString, QVariant> &properties);

    void deviceAdded(const QDBusObjectPath &path);
    void deviceRemoved(const QDBusObjectPath &path);

    void newConnection(const QDBusObjectPath &path, QNetworkManagerSettings *settings = 0);
    void removeConnection(const QString &path);
    void updateConnection(const QNmSettingsMap &settings);
    void activationFinished(QDBusPendingCallWatcher *watcher);

    void newAccessPoint(const QString &path, const QDBusObjectPath &objectPath);
    void removeAccessPoint(const QString &path, const QDBusObjectPath &objectPath);
    void updateAccessPoint(const QMap<QString, QVariant> &map);

private:
    QNetworkConfigurationPrivate *parseConnection(const QString &service,
                                                  const QString &settingsPath,
                                                  const QNmSettingsMap &map);
    QNetworkManagerSettingsConnection *connectionFromId(const QString &id) const;

private:
    QNetworkManagerInterface *interface;
    QNetworkManagerSettings *systemSettings;
    QNetworkManagerSettings *userSettings;
    QHash<QString, QNetworkManagerInterfaceDeviceWireless *> wirelessDevices;
    QHash<QString, QNetworkManagerConnectionActive *> activeConnections;
    QList<QNetworkManagerSettingsConnection *> connections;
    QList<QNetworkManagerInterfaceAccessPoint *> accessPoints;
    QList<QNetworkManagerInterfaceAccessPoint *> configuredAccessPoints;
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif // QT_NO_BEARERMANAGEMENT

#endif

