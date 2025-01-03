// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformservices.h"

#include <QtCore/QUrl>
#include <QtCore/QString>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformServices
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformServices provides the backend for desktop-related functionality.
*/

/*!
    \enum QPlatformServices::Capability

    Capabilities are used to determine a specific platform service's availability.

    \value ColorPickingFromScreen The platform natively supports color picking from screen.
    This capability indicates that the platform supports "opaque" color picking, where the
    platform implements a complete user experience for color picking and outputs a color.
    This is in contrast to the application implementing the color picking user experience
    (taking care of showing a cross hair, instructing the platform integration to obtain
    the color at a given pixel, etc.). The related service function is pickColor().
 */

QPlatformServices::QPlatformServices()
{ }

bool QPlatformServices::openUrl(const QUrl &url)
{
    qWarning("This plugin does not support QPlatformServices::openUrl() for '%s'.",
             qPrintable(url.toString()));
    return false;
}

bool QPlatformServices::openDocument(const QUrl &url)
{
    qWarning("This plugin does not support QPlatformServices::openDocument() for '%s'.",
             qPrintable(url.toString()));
    return false;
}

/*!
 * \brief QPlatformServices::desktopEnvironment returns the active desktop environment.
 *
 * On Unix this function returns the uppercase desktop environment name, such as
 * KDE, GNOME, UNITY, XFCE, LXDE etc. or UNKNOWN if none was detected.
 * The primary way to detect the desktop environment is the environment variable
 * XDG_CURRENT_DESKTOP.
 */
QByteArray QPlatformServices::desktopEnvironment() const
{
    return QByteArray("UNKNOWN");
}

QPlatformServiceColorPicker *QPlatformServices::colorPicker(QWindow *parent)
{
    Q_UNUSED(parent);
    return nullptr;
}

bool QPlatformServices::hasCapability(Capability capability) const
{
    Q_UNUSED(capability)
    return false;
}

QT_END_NAMESPACE
