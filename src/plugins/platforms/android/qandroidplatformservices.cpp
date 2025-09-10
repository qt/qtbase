// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformservices.h"

#if QT_CONFIG(desktopservices)
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QMimeDatabase>
#include <QtCore/QJniObject>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qscopedvaluerollback.h>
#endif

QT_BEGIN_NAMESPACE

using namespace QtJniTypes;
using namespace Qt::StringLiterals;

#if QT_CONFIG(desktopservices)
static constexpr auto s_defaultScheme = "file"_L1;
static constexpr auto s_defaultProvider = "qtprovider"_L1;
#endif

QAndroidPlatformServices::QAndroidPlatformServices()
{
#if QT_CONFIG(desktopservices)
    m_actionView = QJniObject::getStaticObjectField("android/content/Intent", "ACTION_VIEW",
                                                    "Ljava/lang/String;")
                           .toString();

    QtAndroidPrivate::registerNewIntentListener(this);

    // Qt applications without Activity contexts cannot retrieve intents from the Activity.
    if (QNativeInterface::QAndroidApplication::isActivityContext()) {
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
#endif // QT_CONFIG(desktopservices)
}

QByteArray QAndroidPlatformServices::desktopEnvironment() const
{
    return QByteArray("Android");
}

Q_DECLARE_JNI_CLASS(FileProvider, "androidx/core/content/FileProvider");
Q_DECLARE_JNI_CLASS(PackageManager, "android/content/pm/PackageManager");
Q_DECLARE_JNI_CLASS(PackageInfo, "android/content/pm/PackageInfo");
Q_DECLARE_JNI_CLASS(ProviderInfo, "android/content/pm/ProviderInfo");

#if QT_CONFIG(desktopservices)
bool QAndroidPlatformServices::openUrl(const QUrl &theUrl)
{
    QUrl url(theUrl);

    // avoid recursing back into self
    if (url == m_handlingUrl)
        return false;

    // a real URL including the scheme is needed, else the Intent can not be started
    if (url.scheme().isEmpty())
        url.setScheme(s_defaultScheme);

    const int sdkVersion = QNativeInterface::QAndroidApplication::sdkVersion();
    if (url.scheme() != s_defaultScheme || sdkVersion < 24 )
        return openURL(url);
    return openUrlWithFileProvider(url);
}

QString QAndroidPlatformServices::getMimeOfUrl(const QUrl &url) const
{
    QString mime;
    if (url.scheme() == s_defaultScheme)
        mime = QMimeDatabase().mimeTypeForUrl(url).name();
    return mime;
}

bool QAndroidPlatformServices::openURL(const QUrl &url) const
{
    return  openURL(url.toString());
}

bool QAndroidPlatformServices::openURL(const QString &url) const
{
    return  QJniObject::callStaticMethod<jboolean>(
            QtAndroid::applicationClass(), "openURL",
            QNativeInterface::QAndroidApplication::context(),
            url,
            getMimeOfUrl(url));
}

bool QAndroidPlatformServices::openUrlWithFileProvider(const QUrl &url)
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    auto authorities = getFileProviderAuthorities(context);
    if (authorities.isEmpty())
        return false;
    return openUrlWithAuthority(url, getAdequateFileproviderAuthority(authorities));
}


QString QAndroidPlatformServices::getAdequateFileproviderAuthority(const QStringList &authorities) const
{
    if (authorities.size() == 1)
        return authorities[0];

    QString nonQtAuthority;
    for (const auto &authority : authorities) {
        if (!authority.endsWith(s_defaultProvider, Qt::CaseSensitive)) {
            nonQtAuthority = authority;
            break;
        }
    }
    return nonQtAuthority;
}

bool QAndroidPlatformServices::openUrlWithAuthority(const QUrl &url, const QString &authority)
{
    const auto urlPath = QJniObject::fromString(url.path());
    const auto urlFile = QJniObject(Traits<File>::className(),
                                    urlPath.object<jstring>());
    const auto fileProviderUri = QJniObject::callStaticMethod<Uri>(
            Traits<FileProvider>::className(), "getUriForFile",
            QNativeInterface::QAndroidApplication::context(), authority,
            urlFile.object<File>());
    if (fileProviderUri.isValid())
        return openURL(fileProviderUri.toString());
    return false;
}

QStringList QAndroidPlatformServices::getFileProviderAuthorities(const QJniObject &context) const
{
    QStringList authorityList;

    const auto packageManager = context.callMethod<PackageManager>("getPackageManager");
    const auto packageName = context.callMethod<QString>("getPackageName");
    const auto packageInfo = packageManager.callMethod<PackageInfo>("getPackageInfo",
                                                                    packageName,
                                                                    8 /* PackageManager.GET_PROVIDERS */);
    const auto providersArray = packageInfo.getField<ProviderInfo[]>("providers");

    if (providersArray.isValid()) {
        const auto className = Traits<FileProvider>::className();
        for (const auto &fileProvider : providersArray) {
            auto providerName = fileProvider.getField<QString>("name");
            if (providerName.replace(".", "/").contains(className.data())) {
                const auto authority = fileProvider.getField<QString>("authority");
                if (!authority.isEmpty())
                    authorityList << authority;
            }
        }
    }
    if (authorityList.isEmpty())
        qWarning() << "No file provider found in the AndroidManifest.xml.";

    return authorityList;
}

bool QAndroidPlatformServices::openDocument(const QUrl &url)
{
    return openUrl(url);
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
#endif // QT_CONFIG(desktopservices)

QT_END_NAMESPACE
