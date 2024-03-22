// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qgtk3portalinterface_p.h"
#include "qgtk3storage_p.h"

#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusPendingCall>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusPendingReply>
#include <QtDBus/QDBusVariant>
#include <QtDBus/QtDBus>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQGtk3PortalInterface, "qt.qpa.gtk");

using namespace Qt::StringLiterals;

static constexpr QLatin1StringView appearanceInterface("org.freedesktop.appearance");
static constexpr QLatin1StringView colorSchemeKey("color-scheme");

const QDBusArgument &operator>>(const QDBusArgument &argument, QMap<QString, QVariantMap> &map)
{
    argument.beginMap();
    map.clear();

    while (!argument.atEnd()) {
        QString key;
        QVariantMap value;
        argument.beginMapEntry();
        argument >> key >> value;
        argument.endMapEntry();
        map.insert(key, value);
    }

    argument.endMap();
    return argument;
}

QGtk3PortalInterface::QGtk3PortalInterface(QGtk3Storage *s)
    : m_storage(s) {
    qRegisterMetaType<QDBusVariant>();
    qDBusRegisterMetaType<QMap<QString, QVariantMap>>();

    queryColorScheme();

    if (!s) {
        qCDebug(lcQGtk3PortalInterface) << "QGtk3PortalInterface instantiated without QGtk3Storage."
                                        << "No reaction to runtime theme changes.";
    }
}

Qt::ColorScheme QGtk3PortalInterface::colorScheme() const
{
   return m_colorScheme;
}

void QGtk3PortalInterface::queryColorScheme() {
    QDBusConnection connection = QDBusConnection::sessionBus();
    QDBusMessage message = QDBusMessage::createMethodCall(
            "org.freedesktop.portal.Desktop"_L1,
            "/org/freedesktop/portal/desktop"_L1,
            "org.freedesktop.portal.Settings"_L1, "ReadAll"_L1);
    message << QStringList{ appearanceInterface };

    QDBusPendingCall pendingCall = connection.asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    QObject::connect(
            watcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *watcher) {
                QDBusPendingReply<QMap<QString, QVariantMap>> reply = *watcher;
                if (reply.isValid()) {
                    QMap<QString, QVariantMap> settings = reply.value();
                    if (!settings.isEmpty()) {
                        settingChanged(appearanceInterface, colorSchemeKey,
                                       QDBusVariant(settings.value(appearanceInterface).value(colorSchemeKey)));
                    }
                } else {
                    qCDebug(lcQGtk3PortalInterface) << "Failed to query org.freedesktop.portal.Settings: "
                                                    << reply.error().message();
                }
                watcher->deleteLater();
            });

    QDBusConnection::sessionBus().connect(
            "org.freedesktop.portal.Desktop"_L1, "/org/freedesktop/portal/desktop"_L1,
            "org.freedesktop.portal.Settings"_L1, "SettingChanged"_L1, this,
            SLOT(settingChanged(QString, QString, QDBusVariant)));
}

void QGtk3PortalInterface::settingChanged(const QString &group, const QString &key,
                                          const QDBusVariant &value)
{
    if (group == appearanceInterface && key == colorSchemeKey) {
        const uint colorScheme = value.variant().toUInt();
        // From org.freedesktop.portal.Settings.xml
        // "1" - Prefer dark appearance
        Qt::ColorScheme newColorScheme = colorScheme == 1 ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
        if (m_colorScheme != newColorScheme) {
            m_colorScheme = newColorScheme;
            if (m_storage)
                m_storage->handleThemeChange();
        }
    }
}

QT_END_NAMESPACE

#include "moc_qgtk3portalinterface_p.cpp"
