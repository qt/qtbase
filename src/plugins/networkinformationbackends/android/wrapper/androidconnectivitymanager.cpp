/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

static void networkConnectivityChanged(JNIEnv *env, jobject obj)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    Q_EMIT androidConnManagerInstance->connManager->connectivityChanged();
}

static void behindCaptivePortalChanged(JNIEnv *env, jobject obj, jboolean state)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    Q_EMIT androidConnManagerInstance->connManager->captivePortalChanged(state);
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

AndroidConnectivityManager::AndroidConnectivity AndroidConnectivityManager::networkConnectivity()
{
    QJniEnvironment env;
    QJniObject networkReceiver(networkInformationClass);
    jclass clazz = env->GetObjectClass(networkReceiver.object());
    static const QByteArray functionSignature =
            QByteArray(QByteArray("()L") + networkInformationClass + "$AndroidConnectivity;");
    QJniObject enumObject =
            QJniObject::callStaticObjectMethod(clazz, "state", functionSignature.data());
    if (!enumObject.isValid())
        return AndroidConnectivityManager::AndroidConnectivity::Unknown;

    QJniObject enumName = enumObject.callObjectMethod<jstring>("name");
    if (!enumName.isValid())
        return AndroidConnectivityManager::AndroidConnectivity::Unknown;

    QString name = enumName.toString();
    if (name == u"Connected")
        return AndroidConnectivity::Connected;
    if (name == u"Disconnected")
        return AndroidConnectivity::Disconnected;
    return AndroidConnectivity::Unknown;
}

bool AndroidConnectivityManager::registerNatives()
{
    QJniEnvironment env;
    QJniObject networkReceiver(networkInformationClass);
    if (!networkReceiver.isValid())
        return false;

    jclass clazz = env->GetObjectClass(networkReceiver.object());
    static JNINativeMethod methods[] = {
        { "connectivityChanged", "()V", reinterpret_cast<void *>(networkConnectivityChanged) },
        { "behindCaptivePortalChanged", "(Z)V",
          reinterpret_cast<void *>(behindCaptivePortalChanged) }
    };
    const bool ret = (env->RegisterNatives(clazz, methods, std::size(methods)) == JNI_OK);
    env->DeleteLocalRef(clazz);
    return ret;
}

QT_END_NAMESPACE
