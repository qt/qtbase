/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidplatformservices.h"
#include <QUrl>
#include <QFile>
#include <QDebug>
#include <QMimeDatabase>
#include <QtCore/private/qjni_p.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformServices::QAndroidPlatformServices()
{
}

bool QAndroidPlatformServices::openUrl(const QUrl &theUrl)
{
    QString mime;
    QUrl url(theUrl);

    // if the file is local, we need to pass the MIME type, otherwise Android
    // does not start an Intent to view this file
    if ((url.scheme().isEmpty() && QFile::exists(url.path())) || url.isLocalFile()) {
        // a real URL including the scheme is needed, else the Intent can not be started
        url.setScheme(QLatin1String("file"));

        QMimeDatabase mimeDb;
        mime = mimeDb.mimeTypeForUrl(url).name();
    }

    QJNIObjectPrivate urlString = QJNIObjectPrivate::fromString(url.toString());
    QJNIObjectPrivate mimeString = QJNIObjectPrivate::fromString(mime);
    return QJNIObjectPrivate::callStaticMethod<jboolean>(QtAndroid::applicationClass(),
                                                         "openURL",
                                                         "(Ljava/lang/String;Ljava/lang/String;)Z",
                                                         urlString.object(), mimeString.object());
}

bool QAndroidPlatformServices::openDocument(const QUrl &url)
{
    return openUrl(url);
}

QByteArray QAndroidPlatformServices::desktopEnvironment() const
{
    return QByteArray("Android");
}

QT_END_NAMESPACE
