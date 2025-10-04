// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qxdgdesktopportaltheme.h"
#include "qxdgdesktopportalfiledialog_p.h"

#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme_p.h>
#include <qpa/qplatformthemefactory_p.h>
#include <qpa/qplatformintegration.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static constexpr QLatin1StringView appearanceInterface("org.freedesktop.appearance");
static constexpr QLatin1StringView colorSchemeKey("color-scheme");
static constexpr QLatin1StringView contrastKey("contrast");

class QXdgDesktopPortalThemePrivate : public QObject
    {
    Q_OBJECT
public:
    enum XdgColorschemePref {
        None,
        PreferDark,
        PreferLight
    };

    QXdgDesktopPortalThemePrivate()
        : QObject()
    { }

    ~QXdgDesktopPortalThemePrivate()
    {
        delete baseTheme;
    }

    /*! \internal

        Converts the given Freedesktop color scheme setting \a colorschemePref to a Qt::ColorScheme value.
        Specification: https://github.com/flatpak/xdg-desktop-portal/blob/d7a304a00697d7d608821253cd013f3b97ac0fb6/data/org.freedesktop.impl.portal.Settings.xml#L33-L45

        Unfortunately the enum numerical values are not defined identically, so we have to convert them.

        The mapping is as follows:

        Enum Index: Freedesktop definition  | Qt definition
        ----------------------------------- | -------------
        0: No preference                    | 0: Unknown
        1: Prefer dark appearance           | 2: Dark
        2: Prefer light appearance          | 1: Light
    */
    static Qt::ColorScheme colorSchemeFromXdgPref(const XdgColorschemePref colorschemePref)
    {
        switch (colorschemePref) {
            case PreferDark: return Qt::ColorScheme::Dark;
            case PreferLight: return Qt::ColorScheme::Light;
            default: return Qt::ColorScheme::Unknown;
        }
    }

public Q_SLOTS:
    void settingChanged(const QString &group, const QString &key,
                        const QDBusVariant &value)
    {
        if (group == appearanceInterface) {
            if (key == colorSchemeKey) {
                colorScheme = colorSchemeFromXdgPref(static_cast<XdgColorschemePref>(value.variant().toUInt()));
                QWindowSystemInterface::handleThemeChange();
            } else if (key == contrastKey) {
                contrast = static_cast<Qt::ContrastPreference>(value.variant().toUInt());
                QWindowSystemInterface::handleThemeChange();
            }
        }
    }

public:
    QPlatformTheme *baseTheme = nullptr;
    uint fileChooserPortalVersion = 0;
    Qt::ColorScheme colorScheme = Qt::ColorScheme::Unknown;
    Qt::ContrastPreference contrast = Qt::ContrastPreference::NoPreference;
};

QXdgDesktopPortalTheme::QXdgDesktopPortalTheme()
    : d_ptr(new QXdgDesktopPortalThemePrivate)
{
    Q_D(QXdgDesktopPortalTheme);

    const QStringList themeNames = QGuiApplicationPrivate::platform_integration->themeNames();
    for (const QString &themeName : themeNames) {
        if (QXdgDesktopPortalTheme::isXdgPlugin(themeName))
            continue;
        // 1) Look for a theme plugin.
        d->baseTheme = QPlatformThemeFactory::create(themeName, nullptr);
        if (d->baseTheme)
            break;

        // 2) If no theme plugin was found ask the platform integration to create a theme
        d->baseTheme = QGuiApplicationPrivate::platform_integration->createPlatformTheme(themeName);
        if (d->baseTheme)
            break;
    }

    // 3) Fall back on the built-in "null" platform theme.
    if (!d->baseTheme)
        d->baseTheme = new QPlatformTheme;

    // Get information about portal version
    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop"_L1,
                                                          "/org/freedesktop/portal/desktop"_L1,
                                                          "org.freedesktop.DBus.Properties"_L1,
                                                          "Get"_L1);
    message << "org.freedesktop.portal.FileChooser"_L1 << "version"_L1;
    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher, [d] (QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QVariant> reply = *watcher;
        if (reply.isValid()) {
            d->fileChooserPortalVersion = reply.value().toUInt();
        } else {
            qWarning() << "Call for getting org.freedesktop.portal.FileChooser version failed"
                       << reply.error();
        }
        watcher->deleteLater();
    });

    // Get information about system theme preference
    message = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop"_L1,
                                             "/org/freedesktop/portal/desktop"_L1,
                                             "org.freedesktop.portal.Settings"_L1,
                                             "ReadAll"_L1);

    QStringList namespaces = { appearanceInterface };
    message << namespaces;

    // this must not be asyncCall() because we have to set appearance now
    QDBusReply<QMap<QString, QVariantMap>> reply = QDBusConnection::sessionBus().call(message);
    if (reply.isValid()) {
        const QMap<QString, QVariantMap> settingsMap = reply.value();
        if (!settingsMap.isEmpty()) {
            const auto xdgColorSchemePref = static_cast<QXdgDesktopPortalThemePrivate::XdgColorschemePref>(settingsMap.value(appearanceInterface).value(colorSchemeKey).toUInt());
            d->colorScheme = QXdgDesktopPortalThemePrivate::colorSchemeFromXdgPref(xdgColorSchemePref);
            d->contrast = static_cast<Qt::ContrastPreference>(settingsMap.value(appearanceInterface).value(contrastKey).toUInt());
        }
    } else {
        qWarning() << "Call to org.freedesktop.portal.Settings.ReadAll failed" << reply.error();
    }

    QDBusConnection::sessionBus().connect(
            "org.freedesktop.portal.Desktop"_L1, "/org/freedesktop/portal/desktop"_L1,
            "org.freedesktop.portal.Settings"_L1, "SettingChanged"_L1, d_ptr.get(),
            SLOT(settingChanged(QString,QString,QDBusVariant)));
}

QPlatformMenuItem* QXdgDesktopPortalTheme::createPlatformMenuItem() const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->createPlatformMenuItem();
}

QPlatformMenu* QXdgDesktopPortalTheme::createPlatformMenu() const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->createPlatformMenu();
}

QPlatformMenuBar* QXdgDesktopPortalTheme::createPlatformMenuBar() const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->createPlatformMenuBar();
}

void QXdgDesktopPortalTheme::showPlatformMenuBar()
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->showPlatformMenuBar();
}

bool QXdgDesktopPortalTheme::usePlatformNativeDialog(DialogType type) const
{
    Q_D(const QXdgDesktopPortalTheme);

    if (type == FileDialog)
        return true;

    return d->baseTheme->usePlatformNativeDialog(type);
}

QPlatformDialogHelper* QXdgDesktopPortalTheme::createPlatformDialogHelper(DialogType type) const
{
    Q_D(const QXdgDesktopPortalTheme);

    if (type == FileDialog && d->fileChooserPortalVersion) {
        // Older versions of FileChooser portal don't support opening directories, therefore we fallback
        // to native file dialog opened inside the sandbox to open a directory.
        if (d->baseTheme->usePlatformNativeDialog(type))
            return new QXdgDesktopPortalFileDialog(static_cast<QPlatformFileDialogHelper*>(d->baseTheme->createPlatformDialogHelper(type)),
                                                   d->fileChooserPortalVersion);

        return new QXdgDesktopPortalFileDialog;
    }

    return d->baseTheme->createPlatformDialogHelper(type);
}

#ifndef QT_NO_SYSTEMTRAYICON
QPlatformSystemTrayIcon* QXdgDesktopPortalTheme::createPlatformSystemTrayIcon() const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->createPlatformSystemTrayIcon();
}
#endif

const QPalette *QXdgDesktopPortalTheme::palette(Palette type) const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->palette(type);
}

const QFont* QXdgDesktopPortalTheme::font(Font type) const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->font(type);
}

QVariant QXdgDesktopPortalTheme::themeHint(ThemeHint hint) const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->themeHint(hint);
}

Qt::ColorScheme QXdgDesktopPortalTheme::colorScheme() const
{
    Q_D(const QXdgDesktopPortalTheme);
    if (d->colorScheme == Qt::ColorScheme::Unknown)
        return d->baseTheme->colorScheme();
    return d->colorScheme;
}

Qt::ContrastPreference QXdgDesktopPortalTheme::contrastPreference() const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->contrast;
}

QPixmap QXdgDesktopPortalTheme::standardPixmap(StandardPixmap sp, const QSizeF &size) const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->standardPixmap(sp, size);
}

QIcon QXdgDesktopPortalTheme::fileIcon(const QFileInfo &fileInfo,
                              QPlatformTheme::IconOptions iconOptions) const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->fileIcon(fileInfo, iconOptions);
}

QIconEngine * QXdgDesktopPortalTheme::createIconEngine(const QString &iconName) const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->createIconEngine(iconName);
}

#if QT_CONFIG(shortcut)
QList<QKeySequence> QXdgDesktopPortalTheme::keyBindings(QKeySequence::StandardKey key) const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->keyBindings(key);
}
#endif

QString QXdgDesktopPortalTheme::standardButtonText(int button) const
{
    Q_D(const QXdgDesktopPortalTheme);
    return d->baseTheme->standardButtonText(button);
}

bool QXdgDesktopPortalTheme::isXdgPlugin(const QString &key)
{
    return key.compare("xdgdesktopportal"_L1, Qt::CaseInsensitive) == 0 ||
           key.compare("flatpak"_L1, Qt::CaseInsensitive) == 0 ||
           key.compare("snap"_L1, Qt::CaseInsensitive) == 0;
}

QT_END_NAMESPACE

#include "qxdgdesktopportaltheme.moc"
