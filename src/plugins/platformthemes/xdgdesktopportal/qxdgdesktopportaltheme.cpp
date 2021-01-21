/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
****************************************************************************/

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

QT_BEGIN_NAMESPACE

class QXdgDesktopPortalThemePrivate : public QPlatformThemePrivate
{
public:
    QXdgDesktopPortalThemePrivate()
        : QPlatformThemePrivate()
    { }

    ~QXdgDesktopPortalThemePrivate()
    {
        delete baseTheme;
    }

    QPlatformTheme *baseTheme = nullptr;
    uint fileChooserPortalVersion = 0;
};

QXdgDesktopPortalTheme::QXdgDesktopPortalTheme()
    : d_ptr(new QXdgDesktopPortalThemePrivate)
{
    Q_D(QXdgDesktopPortalTheme);

    QStringList themeNames;
    themeNames += QGuiApplicationPrivate::platform_integration->themeNames();
    // 1) Look for a theme plugin.
    for (const QString &themeName : qAsConst(themeNames)) {
        d->baseTheme = QPlatformThemeFactory::create(themeName, nullptr);
        if (d->baseTheme)
            break;
    }

    // 2) If no theme plugin was found ask the platform integration to
    // create a theme
    if (!d->baseTheme) {
        for (const QString &themeName : qAsConst(themeNames)) {
            d->baseTheme = QGuiApplicationPrivate::platform_integration->createPlatformTheme(themeName);
            if (d->baseTheme)
                break;
        }
        // No error message; not having a theme plugin is allowed.
    }

    // 3) Fall back on the built-in "null" platform theme.
    if (!d->baseTheme)
        d->baseTheme = new QPlatformTheme;

    // Get information about portal version
    QDBusMessage message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.portal.Desktop"),
                                                          QLatin1String("/org/freedesktop/portal/desktop"),
                                                          QLatin1String("org.freedesktop.DBus.Properties"),
                                                          QLatin1String("Get"));
    message << QLatin1String("org.freedesktop.portal.FileChooser") << QLatin1String("version");
    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, [d] (QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QVariant> reply = *watcher;
        if (reply.isValid()) {
            d->fileChooserPortalVersion = reply.value().toUInt();
        }
    });
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

    if (type == FileDialog) {
        // Older versions of FileChooser portal don't support opening directories, therefore we fallback
        // to native file dialog opened inside the sandbox to open a directory.
        if (d->fileChooserPortalVersion < 3 && d->baseTheme->usePlatformNativeDialog(type))
            return new QXdgDesktopPortalFileDialog(static_cast<QPlatformFileDialogHelper*>(d->baseTheme->createPlatformDialogHelper(type)));

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

QT_END_NAMESPACE
