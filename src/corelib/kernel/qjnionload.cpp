/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <jni.h>
#include "qjnihelpers_p.h"
#include <android/log.h>

static const char logTag[] = "QtCore";


Q_CORE_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    Q_UNUSED(reserved)

    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    typedef union {
        JNIEnv *nenv;
        void *venv;
    } _JNIEnv;

    __android_log_print(ANDROID_LOG_INFO, logTag, "Start");

    _JNIEnv uenv;
    uenv.venv = nullptr;

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_6) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_FATAL, logTag, "GetEnv failed");
        return JNI_ERR;
    }

    JNIEnv *env = uenv.nenv;
    const jint ret = QT_PREPEND_NAMESPACE(QtAndroidPrivate::initJNI(vm, env));
    if (ret != 0)
    {
        __android_log_print(ANDROID_LOG_FATAL, logTag, "initJNI failed");
        return ret;
    }

    return JNI_VERSION_1_6;
}
