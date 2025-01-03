// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformservices.h"

#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QMimeDatabase>
#include <QtCore/QJniObject>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qscopedvaluerollback.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QAndroidPlatformServices::QAndroidPlatformServices()
{
    m_actionView = QJniObject::getStaticObjectField("android/content/Intent", "ACTION_VIEW",
                                                    "Ljava/lang/String;")
                           .toString();

    QtAndroidPrivate::registerNewIntentListener(this);

    QMetaObject::invokeMethod(
            this,
            [this] {
                QJniObject context = QJniObject(QtAndroidPrivate::context());
                QJniObject intent =
                        context.callObjectMethod("getIntent", "()Landroid/content/Intent;");
                handleNewIntent(nullptr, intent.object());
            },
            Qt::QueuedConnection);
}

bool QAndroidPlatformServices::openUrl(const QUrl &theUrl)
{
    QString mime;
    QUrl url(theUrl);

    // avoid recursing back into self
    if (url == m_handlingUrl)
        return false;

    // if the file is local, we need to pass the MIME type, otherwise Android
    // does not start an Intent to view this file
    const auto fileScheme = "file"_L1;

    // a real URL including the scheme is needed, else the Intent can not be started
    if (url.scheme().isEmpty())
        url.setScheme(fileScheme);

    if (url.scheme() == fileScheme)
        mime = QMimeDatabase().mimeTypeForUrl(url).name();

    using namespace QNativeInterface;
    QJniObject urlString = QJniObject::fromString(url.toString());
    QJniObject mimeString = QJniObject::fromString(mime);
    return QJniObject::callStaticMethod<jboolean>(
            QtAndroid::applicationClass(), "openURL",
            "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Z",
            QAndroidApplication::context(), urlString.object(), mimeString.object());
}

bool QAndroidPlatformServices::openDocument(const QUrl &url)
{
    return openUrl(url);
}

QByteArray QAndroidPlatformServices::desktopEnvironment() const
{
    return QByteArray("Android");
}

bool QAndroidPlatformServices::handleNewIntent(JNIEnv *env, jobject intent)
{
    Q_UNUSED(env);

    const QJniObject jniIntent(intent);

    const QString action = jniIntent.callObjectMethod<jstring>("getAction").toString();
    if (action != m_actionView)
        return false;

    const QString url = jniIntent.callObjectMethod<jstring>("getDataString").toString();
    QScopedValueRollback<QUrl> rollback(m_handlingUrl, url);
    return QDesktopServices::openUrl(url);
}

QT_END_NAMESPACE
