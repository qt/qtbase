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


QT_END_NAMESPACE
