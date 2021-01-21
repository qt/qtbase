/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#include "qandroidplatformservices.h"
#include <QUrl>
#include <QFile>
#include <QDebug>
#include <QMimeDatabase>
#include <QtCore/private/qjni_p.h>
#include <private/qjnihelpers_p.h>

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
    QLatin1String fileScheme("file");
    if ((url.scheme().isEmpty() || url.scheme() == fileScheme) && QFile::exists(url.path())) {
        // a real URL including the scheme is needed, else the Intent can not be started
        url.setScheme(fileScheme);
        QMimeDatabase mimeDb;
        mime = mimeDb.mimeTypeForUrl(url).name();
    }

    QJNIObjectPrivate urlString = QJNIObjectPrivate::fromString(url.toString());
    QJNIObjectPrivate mimeString = QJNIObjectPrivate::fromString(mime);
    return QJNIObjectPrivate::callStaticMethod<jboolean>(
            QtAndroid::applicationClass(), "openURL",
            "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Z",
            QtAndroidPrivate::context(), urlString.object(), mimeString.object());
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
