// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androidbackendregister.h"

#include "androidjnimain.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcAndroidBackendRegister, "qt.qpa.androidbackendregister")

Q_DECLARE_JNI_CLASS(BackendRegister, "org/qtproject/qt/android/BackendRegister");

bool AndroidBackendRegister::registerNatives()
{
    return QtJniTypes::BackendRegister::registerNativeMethods(
            { Q_JNI_NATIVE_SCOPED_METHOD(isNull, AndroidBackendRegister),
              Q_JNI_NATIVE_SCOPED_METHOD(registerBackend, AndroidBackendRegister),
              Q_JNI_NATIVE_SCOPED_METHOD(unregisterBackend, AndroidBackendRegister) });
}

jboolean AndroidBackendRegister::isNull(JNIEnv *, jclass)
{
    return QtAndroid::backendRegister() == nullptr;
}

void AndroidBackendRegister::registerBackend(JNIEnv *, jclass, jclass interfaceClass,
                                             jobject interface)
{
    if (AndroidBackendRegister *reg = QtAndroid::backendRegister()) {
        const QJniObject classObject(static_cast<jobject>(interfaceClass));
        QString name = classObject.callMethod<jstring>("getName").toString();
        name.replace('.', '/');

        QMutexLocker lock(&reg->m_registerMutex);
        reg->m_register[name] = QJniObject(interface);
    } else {
        qCWarning(lcAndroidBackendRegister)
                << "AndroidBackendRegister pointer is null, cannot register functionality";
    }
}

void AndroidBackendRegister::unregisterBackend(JNIEnv *, jclass, jclass interfaceClass)
{
    if (AndroidBackendRegister *reg = QtAndroid::backendRegister()) {
        const QJniObject classObject(static_cast<jobject>(interfaceClass));
        QString name = classObject.callMethod<jstring>("getName").toString();
        name.replace('.', '/');

        QMutexLocker lock(&reg->m_registerMutex);
        reg->m_register.remove(name);
    } else {
        qCWarning(lcAndroidBackendRegister)
                << "AndroidBackendRegister pointer is null, cannot unregister functionality";
    }
}

QT_END_NAMESPACE
