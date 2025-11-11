// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qgenericunixtheme_p.h"
#include "qgnometheme_p.h"

#include <QPalette>
#include <QFont>
#include <QGuiApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QHash>
#include <QLoggingCategory>
#include <QVariant>
#include <QStandardPaths>
#include <QStringList>
#if QT_CONFIG(mimetype)
#include <QMimeDatabase>
#endif
#if QT_CONFIG(settings)
#include <QSettings>
#if QT_CONFIG(dbus)
#include "qkdetheme_p.h"
#endif
#endif

#include <qpa/qplatformfontdatabase.h> // lcQpaFonts
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformservices.h>
#include <qpa/qplatformdialoghelper.h>
#include <qpa/qplatformtheme_p.h>

#if QT_CONFIG(dbus)
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>
#include <private/qdbusmenubar_p.h>
#endif
#if QT_CONFIG(dbus) && QT_CONFIG(systemtrayicon)
#include <private/qdbustrayicon_p.h>
#endif

#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <QtCore/QStandardPaths>
#if QT_CONFIG(dbus)
#include <QtDBus/QDBusConnectionInterface>
#endif
#if QT_CONFIG(mimetype)
#include <QtCore/QMimeDatabase>
#include <QtCore/QMimeData>
#endif


QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcTray)
using namespace Qt::StringLiterals;

const char *QGenericUnixTheme::name = "generic";

QGenericUnixThemePrivate::QGenericUnixThemePrivate()
    : QPlatformThemePrivate()
    , systemFont(QLatin1StringView(QGenericUnixTheme::defaultSystemFontNameC),
                 QGenericUnixTheme::defaultSystemFontSize)
    , fixedFont(QLatin1StringView(QGenericUnixTheme::defaultFixedFontNameC),
                systemFont.pointSize())
{
    fixedFont.setStyleHint(QFont::TypeWriter);
    qCDebug(lcQpaFonts) << "default fonts: system" << systemFont << "fixed" << fixedFont;
}

QGenericUnixTheme::QGenericUnixTheme(QGenericUnixThemePrivate *p)
    : QPlatformTheme(p)
{}

QGenericUnixTheme::QGenericUnixTheme()
    : QPlatformTheme(new QGenericUnixThemePrivate())
{}

const QFont *QGenericUnixTheme::font(Font type) const
{
    Q_D(const QGenericUnixTheme);
    switch (type) {
    case QPlatformTheme::SystemFont:
        return &d->systemFont;
    case QPlatformTheme::FixedFont:
        return &d->fixedFont;
    default:
        return nullptr;
    }
}

#if QT_CONFIG(dbus)
QPlatformMenuBar *QGenericUnixTheme::createPlatformMenuBar() const
{
    if (isDBusGlobalMenuAvailable())
        return new QDBusMenuBar();
    return nullptr;
}
#endif

#if QT_CONFIG(dbus) && QT_CONFIG(systemtrayicon)
QPlatformSystemTrayIcon *QGenericUnixTheme::createPlatformSystemTrayIcon() const
{
    if (shouldUseDBusTray())
        return new QDBusTrayIcon();
    return nullptr;
}
#endif

QVariant QGenericUnixTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(QString(QStringLiteral("hicolor")));
    case QPlatformTheme::IconThemeSearchPaths:
        return xdgIconThemePaths();
    case QPlatformTheme::IconFallbackSearchPaths:
        return iconFallbackPaths();
    case QPlatformTheme::DialogButtonBoxButtonsHaveIcons:
        return QVariant(true);
    case QPlatformTheme::StyleNames: {
        QStringList styleNames;
        styleNames << QStringLiteral("Fusion") << QStringLiteral("Windows");
        return QVariant(styleNames);
    }
    case QPlatformTheme::KeyboardScheme:
        return QVariant(int(X11KeyboardScheme));
    case QPlatformTheme::UiEffects:
        return QVariant(int(HoverEffect));
    case QPlatformTheme::MouseCursorTheme:
        return QVariant(mouseCursorTheme());
    case QPlatformTheme::MouseCursorSize:
        return QVariant(mouseCursorSize());
    case QPlatformTheme::PreferFileIconFromTheme:
        return true;
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

QStringList QGenericUnixTheme::themeNames()
{
    QStringList result;
    if (QGuiApplication::desktopSettingsAware()) {
        const QByteArray desktopEnvironment = QGuiApplicationPrivate::platformIntegration()->services()->desktopEnvironment();
        QList<QByteArray> gtkBasedEnvironments;
        gtkBasedEnvironments << "GNOME"
                             << "X-CINNAMON"
                             << "PANTHEON"
                             << "UNITY"
                             << "MATE"
                             << "XFCE"
                             << "LXDE";
        const QList<QByteArray> desktopNames = desktopEnvironment.split(':');
        for (const QByteArray &desktopName : desktopNames) {
#if QT_CONFIG(dbus) && QT_CONFIG(settings) && (QT_CONFIG(xcb) || QT_CONFIG(wayland))
            if (desktopEnvironment == "KDE") {
                result.push_back(QLatin1StringView(QKdeTheme::name));
            } else
#endif
           if (gtkBasedEnvironments.contains(desktopName)) {
                // prefer the GTK3 theme implementation with native dialogs etc.
                result.push_back(QStringLiteral("gtk3"));
                // fallback to the generic Gnome theme if loading the GTK3 theme fails
                result.push_back(QLatin1StringView(QGnomeTheme::name));
            } else {
                // unknown, but lowercase the name (our standard practice) and
                // remove any "x-" prefix
                QString s = QString::fromLatin1(desktopName.toLower());
                result.push_back(s.startsWith("x-"_L1) ? s.mid(2) : s);
            }
        }
    } // desktopSettingsAware
    result.append(QLatin1StringView(QGenericUnixTheme::name));
    return result;
}

/*!
    \internal
    \brief Creates a UNIX theme according to the given theme \a name
*/
QPlatformTheme *QGenericUnixTheme::createUnixTheme(const QString &name)
{
    if (name == QLatin1StringView(QGenericUnixTheme::name))
        return new QGenericUnixTheme;
#if QT_CONFIG(dbus) && QT_CONFIG(settings) && (QT_CONFIG(xcb) || QT_CONFIG(wayland))
    if (name == QLatin1StringView(QKdeTheme::name))
        return QKdeTheme::createKdeTheme();
#endif
    if (name == QLatin1StringView(QGnomeTheme::name))
        return new QGnomeTheme;
    return nullptr;
}

// Helper to return the icon theme paths from XDG.
QStringList QGenericUnixTheme::xdgIconThemePaths()
{
    QStringList paths;
    // Add home directory first in search path
    const QFileInfo homeIconDir(QDir::homePath() + "/.icons"_L1);
    if (homeIconDir.isDir())
        paths.prepend(homeIconDir.absoluteFilePath());

    paths.append(QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                           QStringLiteral("icons"),
                                           QStandardPaths::LocateDirectory));

    return paths;
}

QStringList QGenericUnixTheme::iconFallbackPaths()
{
    QStringList paths;
    const QFileInfo pixmapsIconsDir(QStringLiteral("/usr/share/pixmaps"));
    if (pixmapsIconsDir.isDir())
        paths.append(pixmapsIconsDir.absoluteFilePath());

    return paths;
}

QString QGenericUnixTheme::mouseCursorTheme()
{
    static QString themeName = qEnvironmentVariable("XCURSOR_THEME");
    return themeName;
}

QSize QGenericUnixTheme::mouseCursorSize()
{
    constexpr int defaultCursorSize = 24;
    static const int xCursorSize = qEnvironmentVariableIntValue("XCURSOR_SIZE");
    static const int s = xCursorSize > 0 ? xCursorSize : defaultCursorSize;
    return QSize(s, s);
}

#if QT_CONFIG(dbus)
static bool checkDBusGlobalMenuAvailable()
{
    const QDBusConnection connection = QDBusConnection::sessionBus();
    static const QString registrarService = QStringLiteral("com.canonical.AppMenu.Registrar");
    if (const auto iface = connection.interface())
        return iface->isServiceRegistered(registrarService);
    return false;
}

bool QGenericUnixTheme::isDBusGlobalMenuAvailable()
{
    static bool dbusGlobalMenuAvailable = checkDBusGlobalMenuAvailable();
    return dbusGlobalMenuAvailable;
}
#endif

#if QT_CONFIG(mimetype)
QIcon QGenericUnixTheme::xdgFileIcon(const QFileInfo &fileInfo)
{
    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileInfo);
    if (!mimeType.isValid())
        return QIcon();
    const QString &iconName = mimeType.iconName();
    if (!iconName.isEmpty()) {
        QIcon icon = QIcon::fromTheme(iconName);
        if (!icon.isNull())
            return icon;
    }
    const QString &genericIconName = mimeType.genericIconName();
    return genericIconName.isEmpty() ? QIcon() : QIcon::fromTheme(genericIconName);
}
#endif


#if QT_CONFIG(dbus) && QT_CONFIG(systemtrayicon)
bool QGenericUnixTheme::shouldUseDBusTray()
{
    // There's no other tray implementation to fallback to on non-X11
    // and QDBusTrayIcon can register the icon on the fly after creation
    if (QGuiApplication::platformName() != "xcb"_L1)
        return true;
    const bool result = QDBusMenuConnection().isWatcherRegistered();
    qCDebug(qLcTray) << "D-Bus tray available:" << result;
    return result;
}
#endif

// Helper functions for implementing QPlatformTheme::fileIcon() for XDG icon themes.
QList<QSize> QGenericUnixTheme::availableXdgFileIconSizes()
{
    return QIcon::fromTheme(QStringLiteral("inode-directory")).availableSizes();
}


QT_END_NAMESPACE
