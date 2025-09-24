// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qgnomeportalinterface_p.h"
#include "qdbussettings_p.h"
#include <private/qdbusplatformmenu_p.h>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusPendingCall>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusVariant>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(dbus)
Q_STATIC_LOGGING_CATEGORY(lcQpaThemeGnome, "qt.qpa.theme.gnome")
#endif // QT_CONFIG(dbus)

using namespace Qt::StringLiterals;

QGnomePortalInterface::QGnomePortalInterface(QObject *parent)
    : QObject(parent), m_dbus{ new QDBusListener }
{
    QObject::connect(m_dbus.get(), &QDBusListener::settingChanged, this,
                     &QGnomePortalInterface::dbusSettingChanged);

    querySettings();
}

/*!
    \internal
    \brief QGnomePortalInterface::colorScheme
    This is a getter method for the color-scheme loaded from the DBus portal.
    \param fallback indicates the fallback color-scheme.
    \return returns the cached color-scheme. If the color-scheme was not loaded
    it returns the \a fallback color-scheme.
 */
Qt::ColorScheme QGnomePortalInterface::colorScheme(Qt::ColorScheme fallback) const
{
    if (!m_colorScheme.has_value())
        return fallback;
    return m_colorScheme.value();
}

/*!
    \internal
    \brief QGnomePortalInterface::contrastPreference
    This is a getter method for the contrast-preference loaded from the DBus portal.
    \param fallback indicates the fallback contrast-preference.
    \return returns the cached contrast-preference if it was loaded. Otherwise,
    returns the \a fallback contrast-preference.
 */
Qt::ContrastPreference
QGnomePortalInterface::contrastPreference(Qt::ContrastPreference fallback) const
{
    if (!m_contrast.has_value())
        return fallback;
    return m_contrast.value();
}

void QGnomePortalInterface::querySettings()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (!dbus.isConnected()) {
        qCWarning(lcQpaThemeGnome) << "dbus connection failed. Last error: " << dbus.lastError();
        return;
    }

    constexpr auto method = "ReadAll"_L1;
    auto message = QDBusMessage::createMethodCall(s_service, s_path, s_interface, method);

    message << QStringList{ QDBusSettings::XdgSettings::AppearanceNamespace,
                            QDBusSettings::GnomeSettings::AllyNamespace };

    QDBusPendingCall pendingCall = dbus.asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);

    auto onWatcherFinished = [&](QDBusPendingCallWatcher *watcher) {
        const QDBusMessage reply = watcher->reply();
        if (QDBusMessage::ErrorMessage == reply.type()) {
            qCWarning(lcQpaThemeGnome)
                    << "dbus reply error: [" << reply.errorName() << "]" << reply.errorMessage();
            watcher->deleteLater();
            return;
        }

        const auto convertXdgColorScheme = [](const QVariant &value) {
            using namespace QDBusSettings::XdgSettings;
            return QVariant::fromValue(convertColorScheme(value));
        };

        constexpr auto XdgContrastKey = QDBusSettings::XdgSettings::ContrastKey;
        const auto convertXdgContrast = [](const QVariant &value) {
            using namespace QDBusSettings::XdgSettings;
            return QVariant::fromValue(convertContrastPreference(value));
        };

        constexpr auto GnomeContrastKey = QDBusSettings::GnomeSettings::ContrastKey;
        const auto convrtGnomeContrast = [](const QVariant &value) {
            using namespace QDBusSettings::GnomeSettings;
            return QVariant::fromValue(convertContrastPreference(value));
        };

        const QVariantList &args = reply.arguments();
        for (const QVariant &arg_ : args) {
            const QMap<QString, QVariantMap> arg = qdbus_cast<QMap<QString, QVariantMap>>(arg_);
            for (auto aIt = arg.begin(); aIt != arg.end(); ++aIt) {
                const QString &namespace_ = aIt.key();
                const QVariantMap &settings = aIt.value();
                for (auto sIt = settings.begin(); sIt != settings.end(); ++sIt) {
                    const QString &key = sIt.key();
                    const QVariant &value = sIt.value();
                    if ((key == QDBusSettings::XdgSettings::ColorSchemeKey)
                        && (namespace_ == QDBusSettings::XdgSettings::AppearanceNamespace))
                        dbusSettingChanged(QDBusListener::Provider::Gnome,
                                           QDBusListener::Setting::ColorScheme,
                                           convertXdgColorScheme(value));
                    else if ((key == XdgContrastKey)
                             && (namespace_ == QDBusSettings::XdgSettings::AppearanceNamespace))
                        dbusSettingChanged(QDBusListener::Provider::Gnome,
                                           QDBusListener::Setting::Contrast,
                                           convertXdgContrast(value));
                    else if ((key == GnomeContrastKey)
                             && (namespace_ == QDBusSettings::GnomeSettings::AllyNamespace))
                        dbusSettingChanged(QDBusListener::Provider::Gnome,
                                           QDBusListener::Setting::Contrast,
                                           convrtGnomeContrast(value));
                }
            }
        }
        watcher->deleteLater();
    };
    connect(watcher, &QDBusPendingCallWatcher::finished, this, onWatcherFinished);
}

void QGnomePortalInterface::updateColorScheme(Qt::ColorScheme colorScheme)
{
    if (m_colorScheme.has_value() && (m_colorScheme.value() == colorScheme))
        return;
    m_colorScheme = colorScheme;
    emit colorSchemeChanged(colorScheme);
}

void QGnomePortalInterface::updateContrast(Qt::ContrastPreference contrast)
{
    if (m_contrast.has_value() && (m_contrast.value() == contrast))
        return;
    m_contrast = contrast;
    emit contrastChanged(contrast);
}

void QGnomePortalInterface::dbusSettingChanged(QDBusListener::Provider provider,
                                               QDBusListener::Setting setting,
                                               const QVariant &value)
{
    if (provider != QDBusListener::Provider::Gnome && provider != QDBusListener::Provider::Gtk)
        return;

    switch (setting) {
    case QDBusListener::Setting::ColorScheme:
        updateColorScheme(value.value<Qt::ColorScheme>());
        break;
    case QDBusListener::Setting::Contrast:
        updateContrast(value.value<Qt::ContrastPreference>());
        break;
    case QDBusListener::Setting::Theme:
        emit themeNameChanged(value.toString());
        break;
    default:
        break;
    }
}

QT_END_NAMESPACE

#include "moc_qgnomeportalinterface_p.cpp"
