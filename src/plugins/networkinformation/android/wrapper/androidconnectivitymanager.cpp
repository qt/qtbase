// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "androidconnectivitymanager.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjnienvironment.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface;

struct AndroidConnectivityManagerInstance
{
    AndroidConnectivityManagerInstance() : connManager(new AndroidConnectivityManager) { }
    std::unique_ptr<AndroidConnectivityManager> connManager = nullptr;
};
Q_GLOBAL_STATIC(AndroidConnectivityManagerInstance, androidConnManagerInstance)

static const char networkInformationClass[] =
        "org/qtproject/qt/android/networkinformation/QtAndroidNetworkInformation";

static AndroidConnectivityManager *liveConnManager()
{
    if (!androidConnManagerInstance())
        return nullptr;
    return androidConnManagerInstance->connManager.get();
}

static void networkConnectivityChanged(JNIEnv *, jobject, jint enumValue)
{
    AndroidConnectivityManager *manager = liveConnManager();
    if (!manager)
        return;
    const auto connectivity =
            static_cast<AndroidConnectivityManager::AndroidConnectivity>(enumValue);
    QMetaObject::invokeMethod(manager, [manager, connectivity] {
        Q_EMIT manager->connectivityChanged(connectivity);
    }, Qt::QueuedConnection);
}
Q_DECLARE_JNI_NATIVE_METHOD(networkConnectivityChanged)

static void genericInfoChanged(JNIEnv *, jobject, jboolean captivePortal, jboolean metered)
{
    AndroidConnectivityManager *manager = liveConnManager();
    if (!manager)
        return;
    QMetaObject::invokeMethod(manager, [manager, captivePortal, metered] {
        Q_EMIT manager->captivePortalChanged(captivePortal);
        Q_EMIT manager->meteredChanged(metered);
    }, Qt::QueuedConnection);
}
Q_DECLARE_JNI_NATIVE_METHOD(genericInfoChanged)

static void transportMediumChanged(JNIEnv *, jobject, jint enumValue)
{
    AndroidConnectivityManager *manager = liveConnManager();
    if (!manager)
        return;
    const auto transport = static_cast<AndroidConnectivityManager::AndroidTransport>(enumValue);
    QMetaObject::invokeMethod(manager, [manager, transport] {
        Q_EMIT manager->transportMediumChanged(transport);
    }, Qt::QueuedConnection);
}
Q_DECLARE_JNI_NATIVE_METHOD(transportMediumChanged)

Q_DECLARE_JNI_CLASS(ConnectivityManager, "android/net/ConnectivityManager")

AndroidConnectivityManager::AndroidConnectivityManager()
{
    if (!registerNatives())
        return;

    QJniObject::callStaticMethod<void>(networkInformationClass, "registerReceiver",
                                       QAndroidApplication::context());
}

AndroidConnectivityManager *AndroidConnectivityManager::getInstance()
{
    if (!androidConnManagerInstance())
        return nullptr;
    return androidConnManagerInstance->connManager->isValid()
            ? androidConnManagerInstance->connManager.get()
            : nullptr;
}

bool AndroidConnectivityManager::isValid() const
{
    return registerNatives();
}

AndroidConnectivityManager::~AndroidConnectivityManager()
{
    QJniObject::callStaticMethod<void>(networkInformationClass, "unregisterReceiver",
                                       QAndroidApplication::context());
}

bool AndroidConnectivityManager::registerNatives() const
{
    static const bool registered = []() {
        QJniEnvironment env;
        return env.registerNativeMethods(networkInformationClass, {
            Q_JNI_NATIVE_METHOD(networkConnectivityChanged),
            Q_JNI_NATIVE_METHOD(genericInfoChanged),
            Q_JNI_NATIVE_METHOD(transportMediumChanged),
        });
    }();
    return registered;
}

QT_END_NAMESPACE

#include "moc_androidconnectivitymanager.cpp"
