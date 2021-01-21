/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
