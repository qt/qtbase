/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qstandardpaths.h"
#include <qdir.h>

#ifndef QT_NO_STANDARDPATHS

#include <qstring.h>

QT_BEGIN_NAMESPACE

static bool qsp_testMode = false;

void QStandardPaths::enableTestMode(bool testMode)
{
    qsp_testMode = testMode;
}

static QString testModeInsert() {
    if (qsp_testMode)
        return QStringLiteral("/.qttest");
    else
        return QStringLiteral("");
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    QDir sharedDir = QDir::home();
    sharedDir.cd(QLatin1String("../shared"));

    const QString sharedRoot = sharedDir.absolutePath();

    switch (type) {
    case DataLocation:
        return QDir::homePath() + testModeInsert();
    case DesktopLocation:
    case HomeLocation:
        return QDir::homePath();
    case RuntimeLocation:
    case TempLocation:
        return QDir::tempPath();
    case CacheLocation:
    case GenericCacheLocation:
        return QDir::homePath() + testModeInsert() + QLatin1String("/Cache");
    case ConfigLocation:
        return QDir::homePath() + testModeInsert() + QLatin1String("/Settings");
    case GenericDataLocation:
        return sharedRoot + testModeInsert() + QLatin1String("/misc");
    case DocumentsLocation:
        return sharedRoot + QLatin1String("/documents");
    case PicturesLocation:
        return sharedRoot + QLatin1String("/photos");
    case FontsLocation:
        // this is not a writable location
        return QString();
    case MusicLocation:
        return sharedRoot + QLatin1String("/music");
    case MoviesLocation:
        return sharedRoot + QLatin1String("/videos");
    case DownloadLocation:
        return sharedRoot + QLatin1String("/downloads");
    case ApplicationsLocation:
        return QString();
    default:
        break;
    }

    return QString();
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    if (type == FontsLocation)
        return QStringList(QLatin1String("/base/usr/fonts"));

    return QStringList(writableLocation(type));
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
