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

static void networkConnectivityChanged(JNIEnv *env, jobject obj, jobject enumValue)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    const jint value = QJniObject(enumValue).callMethod<jint>("ordinal");
    const auto connectivity = static_cast<AndroidConnectivityManager::AndroidConnectivity>(value);
    Q_EMIT androidConnManagerInstance->connManager->connectivityChanged(connectivity);
}

static void genericInfoChanged(JNIEnv *env, jobject obj, jboolean captivePortal, jboolean metered)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    Q_EMIT androidConnManagerInstance->connManager->captivePortalChanged(captivePortal);
    Q_EMIT androidConnManagerInstance->connManager->meteredChanged(metered);
}

static void transportMediumChangedCallback(JNIEnv *env, jobject obj, jobject enumValue)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    const jint value = QJniObject(enumValue).callMethod<jint>("ordinal");
    const auto transport = static_cast<AndroidConnectivityManager::AndroidTransport>(value);
    emit androidConnManagerInstance->connManager->transportMediumChanged(transport);
}

AndroidConnectivityManager::AndroidConnectivityManager()
{
    if (!registerNatives())
        return;

    m_connectivityManager = QJniObject::callStaticObjectMethod(
            networkInformationClass, "getConnectivityManager",
            "(Landroid/content/Context;)Landroid/net/ConnectivityManager;",
            QAndroidApplication::context());
    if (!m_connectivityManager.isValid())
        return;

    QJniObject::callStaticMethod<void>(networkInformationClass, "registerReceiver",
                                       "(Landroid/content/Context;)V", QAndroidApplication::context());
}

AndroidConnectivityManager *AndroidConnectivityManager::getInstance()
{
    if (!androidConnManagerInstance())
        return nullptr;
    return androidConnManagerInstance->connManager->isValid()
            ? androidConnManagerInstance->connManager.get()
            : nullptr;
}

AndroidConnectivityManager::~AndroidConnectivityManager()
{
    QJniObject::callStaticMethod<void>(networkInformationClass, "unregisterReceiver",
                                       "(Landroid/content/Context;)V", QAndroidApplication::context());
}

bool AndroidConnectivityManager::registerNatives()
{
    QJniEnvironment env;
    QJniObject networkReceiver(networkInformationClass);
    if (!networkReceiver.isValid())
        return false;

    const QByteArray connectivityEnumSig =
            QByteArray("(L") + networkInformationClass + "$AndroidConnectivity;)V";
    const QByteArray transportEnumSig =
            QByteArray("(L") + networkInformationClass + "$Transport;)V";

    jclass clazz = env->GetObjectClass(networkReceiver.object());
    static JNINativeMethod methods[] = {
        { "connectivityChanged", connectivityEnumSig.data(),
          reinterpret_cast<void *>(networkConnectivityChanged) },
        { "genericInfoChanged", "(ZZ)V",
          reinterpret_cast<void *>(genericInfoChanged) },
        { "transportMediumChanged", transportEnumSig.data(),
          reinterpret_cast<void *>(transportMediumChangedCallback) },
    };
    const bool ret = (env->RegisterNatives(clazz, methods, std::size(methods)) == JNI_OK);
    env->DeleteLocalRef(clazz);
    return ret;
}

QT_END_NAMESPACE
