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

Q_DECLARE_JNI_TYPE(UriType, "Landroid/net/Uri;")
Q_DECLARE_JNI_TYPE(FileType, "Ljava/io/File;")
Q_DECLARE_JNI_CLASS(File, "java/io/File")
Q_DECLARE_JNI_CLASS(FileProvider, "androidx/core/content/FileProvider");

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

    const QJniObject mimeString = QJniObject::fromString(mime);

    using namespace QNativeInterface;

    auto openUrl = [mimeString](const QJniObject &url) {
        return QJniObject::callStaticMethod<jboolean>(QtAndroid::applicationClass(), "openURL",
            QAndroidApplication::context(), url.object<jstring>(), mimeString.object<jstring>());
    };

    if (url.scheme() != fileScheme || QNativeInterface::QAndroidApplication::sdkVersion() < 24)
        return openUrl(QJniObject::fromString(url.toString()));

    // Use FileProvider for file scheme with sdk >= 24
    const QJniObject context = QAndroidApplication::context();
    const auto appId = context.callMethod<jstring>("getPackageName").toString();
    const auto providerName = QJniObject::fromString(appId + ".qtprovider"_L1);

    const auto urlPath = QJniObject::fromString(url.path());
    const auto urlFile = QJniObject(QtJniTypes::className<QtJniTypes::File>(),
                                    urlPath.object<jstring>());

    const auto fileProviderUri = QJniObject::callStaticMethod<QtJniTypes::UriType>(
            QtJniTypes::className<QtJniTypes::FileProvider>(), "getUriForFile",
            QAndroidApplication::context(), providerName.object<jstring>(),
            urlFile.object<QtJniTypes::FileType>());

    if (fileProviderUri.isValid())
        return openUrl(fileProviderUri.callMethod<jstring>("toString"));

    return false;
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
