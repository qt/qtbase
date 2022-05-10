// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2012 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Christoph Schleifenbaum <christoph.schleifenbaum@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformsystemtrayicon.h"

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#ifndef QT_NO_SYSTEMTRAYICON

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformSystemTrayIcon
    \inmodule QtGui
    \brief The QPlatformSystemTrayIcon class abstracts the system tray icon and interaction.

    \internal
    \sa QSystemTrayIcon
*/

/*!
    \enum QPlatformSystemTrayIcon::MessageIcon

    This enum describes the icon that is shown when a balloon message is displayed.

    \value NoIcon      No icon is shown.
    \value Information An information icon is shown.
    \value Warning     A standard warning icon is shown.
    \value Critical    A critical warning icon is shown.

    \sa updateIcon(), showMessage(), QMessageBox
*/

/*!
    \enum QPlatformSystemTrayIcon::ActivationReason

     This enum describes the reason the system tray was activated.

     \value Unknown     Unknown reason
     \value Context     The context menu for the system tray entry was requested
     \value DoubleClick The system tray entry was double clicked
     \value Trigger     The system tray entry was clicked
     \value MiddleClick The system tray entry was clicked with the middle mouse button

     \sa activated()
*/

QPlatformSystemTrayIcon::QPlatformSystemTrayIcon()
{
}

QPlatformSystemTrayIcon::~QPlatformSystemTrayIcon()
{
}

/*!
    \fn void QPlatformSystemTrayIcon::init()
    This method is called to initialize the platform dependent implementation.
*/

/*!
    \fn void QPlatformSystemTrayIcon::cleanup()
    This method is called to cleanup the platform dependent implementation.
*/

/*!
    \fn void QPlatformSystemTrayIcon::updateIcon(const QIcon &icon)
    This method is called when the \a icon did change.
*/

/*!
    \fn void QPlatformSystemTrayIcon::updateToolTip(const QString &tooltip)
    This method is called when the \a tooltip text did change.
*/

/*!
    \fn void QPlatformSystemTrayIcon::updateMenu(QPlatformMenu *menu)
    This method is called when the system tray \a menu did change.
*/

/*!
    \fn QRect QPlatformSystemTrayIcon::geometry() const
    This method returns the geometry of the platform dependent system tray icon on the screen.
*/

/*!
    \fn void QPlatformSystemTrayIcon::showMessage(const QString &title, const QString &msg,
                                                  const QIcon &icon, MessageIcon iconType, int msecs)
    Shows a balloon message for the entry with the given \a title, message \a msg and \a icon for
    the time specified in \a msecs. \a iconType is used as a hint for the implementing platform.
    \sa QSystemTrayIcon::showMessage()
*/

/*!
    \fn bool QPlatformSystemTrayIcon::isSystemTrayAvailable() const
    Returns \c true if the system tray is available on the platform.
*/

/*!
    \fn bool QPlatformSystemTrayIcon::supportsMessages() const
    Returns \c true if the system tray supports messages on the platform.
*/

/*!
    \fn void QPlatformSystemTrayIcon::contextMenuRequested(QPoint globalPos, const QPlatformScreen *screen)
    This signal is emitted when the context menu is requested.
    In particular, on platforms where createMenu() returns nullptr,
    its emission will cause QSystemTrayIcon to show a QMenu-based menu.
    \sa activated()
    \since 5.10
*/

/*!
    \fn void QPlatformSystemTrayIcon::activated(QPlatformSystemTrayIcon::ActivationReason reason)
    This signal is emitted when the user activates the system tray icon.
    \a reason specifies the reason for activation.
    \sa QSystemTrayIcon::ActivationReason, contextMenuRequested()
*/

/*!
    \fn void QPlatformSystemTrayIcon::messageClicked()

    This signal is emitted when the message displayed using showMessage()
    was clicked by the user.

    \sa activated()
*/

/*!
    This method allows platforms to use a different QPlatformMenu for system
    tray menus than what would normally be used for e.g. menu bars. The default
    implementation falls back to a platform menu created by the platform theme,
    which may be null on platforms without native menus.

    \sa updateMenu()
    \since 5.3
 */

QPlatformMenu *QPlatformSystemTrayIcon::createMenu() const
{
    return QGuiApplicationPrivate::platformTheme()->createPlatformMenu();
}

QT_END_NAMESPACE

#include "moc_qplatformsystemtrayicon.cpp"

#endif // QT_NO_SYSTEMTRAYICON
