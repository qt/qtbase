/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qstandardpaths.h"

#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QAtomicPointer>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QRegularExpressionMatch>

#ifndef QT_NO_STANDARDPATHS

QT_BEGIN_NAMESPACE

class QStandardPathsPrivate {
public:
    QStandardPathsPrivate() : object(0){}
    ~QStandardPathsPrivate() { delete object.load(); }
    QAtomicPointer<QJsonObject> object;
};

Q_GLOBAL_STATIC(QStandardPathsPrivate, configCache);

/*!
    \internal
    Substitute environment variables in the form ${name}

    The JSON QStandardPaths implementation can be configured on a per user
    (or per application) basis through the use of environment variables,
    which are evaluated each time a location is queried. This function
    performs that evaluation on \a value. No substitution is performed
    for undefined variables.

    This slightly underselects according to the 2009-09-20 version of
    the GNU setenv(3) manual page: It disallows '}' within the variable
    name. ${var}} will look for a variable named "var", not "var}".
 */
static QString substituteEnvVars(const QJsonValue & value)
{
    QString str = value.toString();
    if (str.isEmpty() || !str.contains(QLatin1String("${")))
        return str;

    // optimize for a common case
    str.replace(QLatin1String("${HOME}"), QDir::homePath());

    // Do ${} format environment variable substitution if necessary
    // repeat this test because ${HOME} might expand to the empty string
    if (!str.isEmpty() && str.contains(QLatin1String("${"))) {
        QRegularExpression varRegExp(QLatin1String("\\$\\{([^\\}=]*)\\}"));
        QRegularExpressionMatchIterator matchIterator =
                varRegExp.globalMatch(str);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            QByteArray envValue =
                    qgetenv(match.captured(1).toLatin1().data());
            if (!envValue.isNull()) {
                QString replacement =
                        QFile::decodeName(envValue);
                str.replace(match.captured(0), replacement);
            }
        }
    }
    return str;
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
    QStringList locations = QStandardPaths::standardLocations(type);
    if (locations.isEmpty())
        return QString();
    return locations.first();
}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    switch (type) {
    case HomeLocation:
        return QStringList(QDir::homePath()); // set $HOME
    case TempLocation:
        return QStringList(QDir::tempPath()); // set $TMPDIR
    default:
        break;
    }

    if (isTestModeEnabled()) {
        const QString qttestDir = QDir::homePath() + QLatin1String("/.qttest");
        QString path;
        switch (type) {
        case GenericDataLocation:
        case DataLocation:
            path = qttestDir + QLatin1String("/share");
            if (type == DataLocation)
                appendOrganizationAndApp(path);
            return QStringList(path);
        case GenericCacheLocation:
        case CacheLocation:
            path = qttestDir + QLatin1String("/cache");
            if (type == CacheLocation)
                appendOrganizationAndApp(path);
            return QStringList(path);
        case ConfigLocation:
            return QStringList(qttestDir + QLatin1String("/config"));
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
                return QStringList();

            QJsonObject myConfigObject = configDoc.object();
            localConfigObject = new QJsonObject(myConfigObject);
            if (!configCache()->object.testAndSetRelease(0, localConfigObject)) {
                delete localConfigObject;
                localConfigObject = configCache()->object.loadAcquire();
            }
        } else {
            return QStringList();
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
        return QStringList();
    }

    QJsonObject::const_iterator iter = localConfigObject->constFind(key);
    if (iter == localConfigObject->constEnd())
        return QStringList();

    switch (iter.value().type()) {
    case QJsonValue::Array: {
        QStringList resultList;
        foreach (const QJsonValue &item, iter.value().toArray())
            resultList.append(substituteEnvVars(item));
        return resultList;
    }
    case QJsonValue::String:
        return QStringList(substituteEnvVars(iter.value()));
    default:
        break;
    }
    return QStringList();
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
