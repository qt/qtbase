// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

static void networkConnectivityChanged(JNIEnv *env, jobject obj, jint enumValue)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    const auto connectivity =
            static_cast<AndroidConnectivityManager::AndroidConnectivity>(enumValue);
    Q_EMIT androidConnManagerInstance->connManager->connectivityChanged(connectivity);
}
Q_DECLARE_JNI_NATIVE_METHOD(networkConnectivityChanged)

static void genericInfoChanged(JNIEnv *env, jobject obj, jboolean captivePortal, jboolean metered)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    Q_EMIT androidConnManagerInstance->connManager->captivePortalChanged(captivePortal);
    Q_EMIT androidConnManagerInstance->connManager->meteredChanged(metered);
}
Q_DECLARE_JNI_NATIVE_METHOD(genericInfoChanged)

static void transportMediumChanged(JNIEnv *env, jobject obj, jint enumValue)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    const auto transport = static_cast<AndroidConnectivityManager::AndroidTransport>(enumValue);
    emit androidConnManagerInstance->connManager->transportMediumChanged(transport);
}
Q_DECLARE_JNI_NATIVE_METHOD(transportMediumChanged)

Q_DECLARE_JNI_TYPE(ConnectivityManager, "Landroid/net/ConnectivityManager;")

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
