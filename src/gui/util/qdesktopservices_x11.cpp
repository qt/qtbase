/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qdesktopservices.h"

#ifndef QT_NO_DESKTOPSERVICES

#include <qprocess.h>
#include <qurl.h>
#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <private/qt_x11_p.h>
#include <qcoreapplication.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

inline static bool launch(const QUrl &url, const QString &client)
{
#if !defined(QT_NO_PROCESS)
    return (QProcess::startDetached(client + QLatin1Char(' ') + QString::fromLatin1(url.toEncoded().constData())));
#else
    return (::system((client + QLatin1Char(' ') + QString::fromLatin1(url.toEncoded().constData())).toLocal8Bit().constData()) != -1);
#endif
}

static bool openDocument(const QUrl &url)
{
    if (!url.isValid())
        return false;

    if (launch(url, QLatin1String("xdg-open")))
        return true;

    // Use the X11->desktopEnvironment value if X11 is non-NULL,
    //  otherwise just attempt to launch command regardless of the desktop environment
    if ((!X11 || (X11 && X11->desktopEnvironment == DE_GNOME)) && launch(url, QLatin1String("gnome-open"))) {
        return true;
    } else {
        if ((!X11 || (X11 && X11->desktopEnvironment == DE_KDE)) && launch(url, QLatin1String("kfmclient exec")))
            return true;
    }

    if (launch(url, QLatin1String("firefox")))
        return true;
    if (launch(url, QLatin1String("mozilla")))
        return true;
    if (launch(url, QLatin1String("netscape")))
        return true;
    if (launch(url, QLatin1String("opera")))
        return true;

    return false;
}

static bool launchWebBrowser(const QUrl &url)
{
    if (!url.isValid())
        return false;
    if (url.scheme() == QLatin1String("mailto"))
        return openDocument(url);

    if (launch(url, QLatin1String("xdg-open")))
        return true;
    if (launch(url, QString::fromLocal8Bit(getenv("DEFAULT_BROWSER"))))
        return true;
    if (launch(url, QString::fromLocal8Bit(getenv("BROWSER"))))
        return true;

    // Use the X11->desktopEnvironment value if X11 is non-NULL,
    //  otherwise just attempt to launch command regardless of the desktop environment
    if ((!X11 || (X11 && X11->desktopEnvironment == DE_GNOME)) && launch(url, QLatin1String("gnome-open"))) {
        return true;
    } else {
        if ((!X11 || (X11 && X11->desktopEnvironment == DE_KDE)) && launch(url, QLatin1String("kfmclient openURL")))
            return true;
    }

    if (launch(url, QLatin1String("firefox")))
        return true;
    if (launch(url, QLatin1String("mozilla")))
        return true;
    if (launch(url, QLatin1String("netscape")))
        return true;
    if (launch(url, QLatin1String("opera")))
        return true;
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_DESKTOPSERVICES
