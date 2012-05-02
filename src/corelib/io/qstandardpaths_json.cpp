/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QRegExp>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QDir>
#include <QAtomicPointer>
#include <QCoreApplication>

#ifndef QT_NO_STANDARDPATHS

QT_BEGIN_NAMESPACE

class QStandardPathsPrivate {
public:
    QStandardPathsPrivate() : object(0){}
    ~QStandardPathsPrivate() { delete object.load(); }
    QAtomicPointer<QJsonObject> object;
};

Q_GLOBAL_STATIC(QStandardPathsPrivate, configCache);

static bool qsp_testMode = false;

void QStandardPaths::enableTestMode(bool testMode)
{
    qsp_testMode = testMode;
}

static void appendOrganizationAndApp(QString &path)
{
    const QString org = QCoreApplication::organizationName();
    if (!org.isEmpty())
        path += QLatin1Char('/') + org;
    const QString appName = QCoreApplication::applicationName();
    if (!appName.isEmpty())
        path += QLatin1Char('/') + appName;
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    switch (type) {
    case HomeLocation:
        return QDir::homePath(); // set $HOME
    case TempLocation:
        return QDir::tempPath(); // set $TMPDIR
    default:
        break;
    }

    if (qsp_testMode) {
        const QString qttestDir = QDir::homePath() + QLatin1String("/.qttest");
        QString path;
        switch (type) {
        case GenericDataLocation:
        case DataLocation:
            path = qttestDir + QLatin1String("/share");
            if (type == DataLocation)
                appendOrganizationAndApp(path);
            return path;
        case GenericCacheLocation:
        case CacheLocation:
            path = qttestDir + QLatin1String("/cache");
            if (type == CacheLocation)
                appendOrganizationAndApp(path);
            return path;
        case ConfigLocation:
            return qttestDir + QLatin1String("/config");
        default:
            break;
        }
    }


    QJsonObject * localConfigObject = configCache()->object.loadAcquire();
    if (localConfigObject == 0) {
        QString configHome = QFile::decodeName(qgetenv("PATH_CONFIG_HOME"));
        if (configHome.isEmpty())
            configHome = QLatin1String("/etc/user-dirs.json");
        QFile file(configHome);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument configDoc = QJsonDocument::fromJson(file.readAll());
            if (configDoc.isNull())
                return QString();

            QJsonObject myConfigObject = configDoc.object();
            localConfigObject = new QJsonObject(myConfigObject);
            if (!configCache()->object.testAndSetRelease(0, localConfigObject)) {
                delete localConfigObject;
                localConfigObject = configCache()->object.loadAcquire();
            }
        } else {
            return QString();
        }
    }

    QLatin1String key("");

    switch (type) {
    case DocumentsLocation:
        key = QLatin1String("DOCUMENTS");
        break;
    case PicturesLocation:
        key = QLatin1String("PICTURES");
        break;
    case MusicLocation:
        key = QLatin1String("MUSIC");
        break;
    case MoviesLocation:
        key = QLatin1String("VIDEOS");
        break;
    case DownloadLocation:
        key = QLatin1String("DOWNLOAD");
        break;
    case ApplicationsLocation:
        key = QLatin1String("APPLICATIONS");
        break;
    case CacheLocation:
        key = QLatin1String("CACHE");
        break;
    case GenericCacheLocation:
        key = QLatin1String("GENERIC_CACHE");
        break;
    case DataLocation:
        key = QLatin1String("DATA");
        break;
    case GenericDataLocation:
        key = QLatin1String("GENERIC_DATA");
        break;
    case ConfigLocation:
        key = QLatin1String("CONFIG");
        break;
    case RuntimeLocation:
        key = QLatin1String("RUNTIME");
        break;
    case DesktopLocation:
        key = QLatin1String("DESKTOP");
        break;
    case FontsLocation:
        key = QLatin1String("FONTS");
        break;

    default:
        return QString();
    }

    QJsonObject::const_iterator iter = localConfigObject->constFind(key);
    if (iter == localConfigObject->constEnd() || ! iter.value().isString())
        return QString();
    QString value = iter.value().toString();

    // optimize for a common case
    value.replace(QLatin1String("${HOME}"), QDir::homePath());

    // Do ${} format environment variable substitution if necessary
    if (!value.isEmpty() && value.contains(QLatin1String("${"))) {
        QRegExp varRegExp(QLatin1String("\\$\\{([^\\}]*)\\}"));
        while (value.contains(varRegExp)) {
            QString replacement =
                    QFile::decodeName(qgetenv(varRegExp.cap(1).toLatin1().data()));
            value.replace(varRegExp.cap(0), replacement);
        }
    }
    return value;
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    QStringList dirs;
    const QString localDir = writableLocation(type);
    dirs.prepend(localDir);
    return dirs;
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
