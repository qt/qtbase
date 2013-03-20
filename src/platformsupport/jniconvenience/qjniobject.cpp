/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qjniobject_p.h"

#include "qjnihelpers_p.h"
#include <qhash.h>

QT_BEGIN_NAMESPACE

static QHash<QString, jclass> g_cachedClasses;

static jclass getCachedClass(JNIEnv *env, const char *className)
{
    jclass clazz = 0;
    QString key = QLatin1String(className);
    QHash<QString, jclass>::iterator it = g_cachedClasses.find(key);
    if (it == g_cachedClasses.end()) {
        jclass c = env->FindClass(className);
        if (env->ExceptionCheck()) {
            c = 0;
            env->ExceptionClear();
        }
        if (c)
            clazz = static_cast<jclass>(env->NewGlobalRef(c));
        g_cachedClasses.insert(key, clazz);
    } else {
        clazz = it.value();
    }
    return clazz;
}

static QHash<QString, jmethodID> g_cachedMethodIDs;
static QString g_keyBase(QLatin1String("%1%2%3"));

static jmethodID getCachedMethodID(JNIEnv *env,
                                   jclass clazz,
                                   const char *name,
                                   const char *sig,
                                   bool isStatic = false)
{
    jmethodID id = 0;
    QString key = g_keyBase.arg(size_t(clazz)).arg(QLatin1String(name)).arg(QLatin1String(sig));
    QHash<QString, jmethodID>::iterator it = g_cachedMethodIDs.find(key);
    if (it == g_cachedMethodIDs.end()) {
        if (isStatic)
            id = env->GetStaticMethodID(clazz, name, sig);
        else
            id = env->GetMethodID(clazz, name, sig);

        if (env->ExceptionCheck()) {
            id = 0;
            env->ExceptionClear();
        }

        g_cachedMethodIDs.insert(key, id);
    } else {
        id = it.value();
    }
    return id;
}

static QHash<QString, jfieldID> g_cachedFieldIDs;

static jfieldID getCachedFieldID(JNIEnv *env,
                                 jclass clazz,
                                 const char *name,
                                 const char *sig,
                                 bool isStatic = false)
{
    jfieldID id = 0;
    QString key = g_keyBase.arg(size_t(clazz)).arg(QLatin1String(name)).arg(QLatin1String(sig));
    QHash<QString, jfieldID>::iterator it = g_cachedFieldIDs.find(key);
    if (it == g_cachedFieldIDs.end()) {
        if (isStatic)
            id = env->GetStaticFieldID(clazz, name, sig);
        else
            id = env->GetFieldID(clazz, name, sig);

        if (env->ExceptionCheck()) {
            id = 0;
            env->ExceptionClear();
        }

        g_cachedFieldIDs.insert(key, id);
    } else {
        id = it.value();
    }
    return id;
}

QJNIObject::QJNIObject(const char *className)
    : m_jobject(0)
    , m_jclass(0)
    , m_own_jclass(false)
{
    QAttachedJNIEnv env;
    m_jclass = getCachedClass(env, className);
    if (m_jclass) {
        // get default constructor
        jmethodID constructorId = getCachedMethodID(env, m_jclass, "<init>", "()V");
        if (constructorId) {
            jobject obj = env->NewObject(m_jclass, constructorId);
            if (obj) {
                m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJNIObject::QJNIObject(const char *className, const char *sig, ...)
    : m_jobject(0)
    , m_jclass(0)
    , m_own_jclass(false)
{
    QAttachedJNIEnv env;
    m_jclass = getCachedClass(env, className);
    if (m_jclass) {
        jmethodID constructorId = getCachedMethodID(env, m_jclass, "<init>", sig);
        if (constructorId) {
            va_list args;
            va_start(args, sig);
            jobject obj = env->NewObjectV(m_jclass, constructorId, args);
            va_end(args);
            if (obj) {
                m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJNIObject::QJNIObject(jclass clazz)
    : m_jobject(0)
    , m_jclass(0)
    , m_own_jclass(true)
{
    QAttachedJNIEnv env;
    m_jclass = static_cast<jclass>(env->NewGlobalRef(clazz));
    if (m_jclass) {
        // get default constructor
        jmethodID constructorId = getCachedMethodID(env, m_jclass, "<init>", "()V");
        if (constructorId) {
            jobject obj = env->NewObject(m_jclass, constructorId);
            if (obj) {
                m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJNIObject::QJNIObject(jclass clazz, const char *sig, ...)
    : m_jobject(0)
    , m_jclass(0)
    , m_own_jclass(true)
{
    QAttachedJNIEnv env;
    if (clazz) {
        m_jclass = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (m_jclass) {
            jmethodID constructorId = getCachedMethodID(env, m_jclass, "<init>", sig);
            if (constructorId) {
                va_list args;
                va_start(args, sig);
                jobject obj = env->NewObjectV(m_jclass, constructorId, args);
                va_end(args);
                if (obj) {
                    m_jobject = env->NewGlobalRef(obj);
                    env->DeleteLocalRef(obj);
                }
            }
        }
    }
}

QJNIObject::QJNIObject(jobject obj)
    : m_jobject(0)
    , m_jclass(0)
    , m_own_jclass(true)
{
    QAttachedJNIEnv env;
    m_jobject = env->NewGlobalRef(obj);
    m_jclass = static_cast<jclass>(env->NewGlobalRef(env->GetObjectClass(m_jobject)));
}

QJNIObject::~QJNIObject()
{
    QAttachedJNIEnv env;
    if (m_jobject)
        env->DeleteGlobalRef(m_jobject);
    if (m_jclass && m_own_jclass)
        env->DeleteGlobalRef(m_jclass);
}

bool QJNIObject::isClassAvailable(const char *className)
{
    QAttachedJNIEnv env;

    if (!env.jniEnv)
        return false;

    jclass clazz = getCachedClass(env, className);

    return (clazz != 0);
}

template <>
void QJNIObject::callMethod<void>(const char *methodName, const char *sig, ...)
{
    QAttachedJNIEnv env;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        env->CallVoidMethodV(m_jobject, id, args);
        va_end(args);
    }
}

template <>
jboolean QJNIObject::callMethod<jboolean>(const char *methodName, const char *sig, ...)
{
    QAttachedJNIEnv env;
    jboolean res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallBooleanMethodV(m_jobject, id, args);
        va_end(args);
    }
    return res;
}

template <>
jbyte QJNIObject::callMethod<jbyte>(const char *methodName, const char *sig, ...)
{
    QAttachedJNIEnv env;
    jbyte res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallByteMethodV(m_jobject, id, args);
        va_end(args);
    }
    return res;
}

template <>
jchar QJNIObject::callMethod<jchar>(const char *methodName, const char *sig, ...)
{
    QAttachedJNIEnv env;
    jchar res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallCharMethodV(m_jobject, id, args);
        va_end(args);
    }
    return res;
}

template <>
jshort QJNIObject::callMethod<jshort>(const char *methodName, const char *sig, ...)
{
    QAttachedJNIEnv env;
    jshort res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallShortMethodV(m_jobject, id, args);
        va_end(args);
    }
    return res;
}

template <>
jint QJNIObject::callMethod<jint>(const char *methodName, const char *sig, ...)
{
    QAttachedJNIEnv env;
    jint res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallIntMethodV(m_jobject, id, args);
        va_end(args);
    }
    return res;
}

template <>
jlong QJNIObject::callMethod<jlong>(const char *methodName, const char *sig, ...)
{
    QAttachedJNIEnv env;
    jlong res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallLongMethodV(m_jobject, id, args);
        va_end(args);
    }
    return res;
}

template <>
jfloat QJNIObject::callMethod<jfloat>(const char *methodName, const char *sig, ...)
{
    QAttachedJNIEnv env;
    jfloat res = 0.f;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallFloatMethodV(m_jobject, id, args);
        va_end(args);
    }
    return res;
}

template <>
jdouble QJNIObject::callMethod<jdouble>(const char *methodName, const char *sig, ...)
{
    QAttachedJNIEnv env;
    jdouble res = 0.;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallDoubleMethodV(m_jobject, id, args);
        va_end(args);
    }
    return res;
}

template <>
QJNILocalRef<jobject> QJNIObject::callObjectMethod<jobject>(const char *methodName,
                                                            const char *sig,
                                                            ...)
{
    QAttachedJNIEnv env;
    jobject res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallObjectMethodV(m_jobject, id, args);
        va_end(args);
    }
    return QJNILocalRef<jobject>(res);
}

template <>
QJNILocalRef<jstring> QJNIObject::callObjectMethod<jstring>(const char *methodName,
                                                            const char *sig,
                                                            ...)
{
    QAttachedJNIEnv env;
    jstring res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jstring>(env->CallObjectMethodV(m_jobject, id, args));
        va_end(args);
    }
    return QJNILocalRef<jstring>(res);
}

template <>
QJNILocalRef<jobjectArray> QJNIObject::callObjectMethod<jobjectArray>(const char *methodName,
                                                                      const char *sig,
                                                                      ...)
{
    QAttachedJNIEnv env;
    jobjectArray res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jobjectArray>(env->CallObjectMethodV(m_jobject, id, args));
        va_end(args);
    }
    return QJNILocalRef<jobjectArray>(res);
}

template <>
QJNILocalRef<jbooleanArray> QJNIObject::callObjectMethod<jbooleanArray>(const char *methodName,
                                                                        const char *sig,
                                                                        ...)
{
    QAttachedJNIEnv env;
    jbooleanArray res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jbooleanArray>(env->CallObjectMethodV(m_jobject, id, args));
        va_end(args);
    }
    return QJNILocalRef<jbooleanArray>(res);
}

template <>
QJNILocalRef<jbyteArray> QJNIObject::callObjectMethod<jbyteArray>(const char *methodName,
                                                                  const char *sig,
                                                                  ...)
{
    QAttachedJNIEnv env;
    jbyteArray res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jbyteArray>(env->CallObjectMethodV(m_jobject, id, args));
        va_end(args);
    }
    return QJNILocalRef<jbyteArray>(res);
}

template <>
QJNILocalRef<jcharArray> QJNIObject::callObjectMethod<jcharArray>(const char *methodName,
                                                                  const char *sig,
                                                                  ...)
{
    QAttachedJNIEnv env;
    jcharArray res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jcharArray>(env->CallObjectMethodV(m_jobject, id, args));
        va_end(args);
    }
    return QJNILocalRef<jcharArray>(res);
}

template <>
QJNILocalRef<jshortArray> QJNIObject::callObjectMethod<jshortArray>(const char *methodName,
                                                                    const char *sig,
                                                                    ...)
{
    QAttachedJNIEnv env;
    jshortArray res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jshortArray>(env->CallObjectMethodV(m_jobject, id, args));
        va_end(args);
    }
    return QJNILocalRef<jshortArray>(res);
}

template <>
QJNILocalRef<jintArray> QJNIObject::callObjectMethod<jintArray>(const char *methodName,
                                                                const char *sig,
                                                                ...)
{
    QAttachedJNIEnv env;
    jintArray res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jintArray>(env->CallObjectMethodV(m_jobject, id, args));
        va_end(args);
    }
    return QJNILocalRef<jintArray>(res);
}

template <>
QJNILocalRef<jlongArray> QJNIObject::callObjectMethod<jlongArray>(const char *methodName,
                                                                  const char *sig,
                                                                  ...)
{
    QAttachedJNIEnv env;
    jlongArray res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jlongArray>(env->CallObjectMethodV(m_jobject, id, args));
        va_end(args);
    }
    return QJNILocalRef<jlongArray>(res);
}

template <>
QJNILocalRef<jfloatArray> QJNIObject::callObjectMethod<jfloatArray>(const char *methodName,
                                                                    const char *sig,
                                                                    ...)
{
    QAttachedJNIEnv env;
    jfloatArray res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jfloatArray>(env->CallObjectMethodV(m_jobject, id, args));
        va_end(args);
    }
    return QJNILocalRef<jfloatArray>(res);
}

template <>
QJNILocalRef<jdoubleArray> QJNIObject::callObjectMethod<jdoubleArray>(const char *methodName,
                                                                      const char *sig,
                                                                      ...)
{
    QAttachedJNIEnv env;
    jdoubleArray res = 0;
    jmethodID id = getCachedMethodID(env, m_jclass, methodName, sig);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jdoubleArray>(env->CallObjectMethodV(m_jobject, id, args));
        va_end(args);
    }
    return QJNILocalRef<jdoubleArray>(res);
}

template <>
void QJNIObject::callMethod<void>(const char *methodName)
{
    callMethod<void>(methodName, "()V");
}

template <>
jboolean QJNIObject::callMethod<jboolean>(const char *methodName)
{
    return callMethod<jboolean>(methodName, "()Z");
}

template <>
jbyte QJNIObject::callMethod<jbyte>(const char *methodName)
{
    return callMethod<jbyte>(methodName, "()B");
}

template <>
jchar QJNIObject::callMethod<jchar>(const char *methodName)
{
    return callMethod<jchar>(methodName, "()C");
}

template <>
jshort QJNIObject::callMethod<jshort>(const char *methodName)
{
    return callMethod<jshort>(methodName, "()S");
}

template <>
jint QJNIObject::callMethod<jint>(const char *methodName)
{
    return callMethod<jint>(methodName, "()I");
}

template <>
jlong QJNIObject::callMethod<jlong>(const char *methodName)
{
    return callMethod<jlong>(methodName, "()J");
}

template <>
jfloat QJNIObject::callMethod<jfloat>(const char *methodName)
{
    return callMethod<jfloat>(methodName, "()F");
}

template <>
jdouble QJNIObject::callMethod<jdouble>(const char *methodName)
{
    return callMethod<jdouble>(methodName, "()D");
}

template <>
QJNILocalRef<jstring> QJNIObject::callObjectMethod<jstring>(const char *methodName)
{
    return callObjectMethod<jstring>(methodName, "()Ljava/lang/String;");
}

template <>
QJNILocalRef<jbooleanArray> QJNIObject::callObjectMethod<jbooleanArray>(const char *methodName)
{
    return callObjectMethod<jbooleanArray>(methodName, "()[Z");
}

template <>
QJNILocalRef<jbyteArray> QJNIObject::callObjectMethod<jbyteArray>(const char *methodName)
{
    return callObjectMethod<jbyteArray>(methodName, "()[B");
}

template <>
QJNILocalRef<jshortArray> QJNIObject::callObjectMethod<jshortArray>(const char *methodName)
{
    return callObjectMethod<jshortArray>(methodName, "()[S");
}

template <>
QJNILocalRef<jintArray> QJNIObject::callObjectMethod<jintArray>(const char *methodName)
{
    return callObjectMethod<jintArray>(methodName, "()[I");
}

template <>
QJNILocalRef<jlongArray> QJNIObject::callObjectMethod<jlongArray>(const char *methodName)
{
    return callObjectMethod<jlongArray>(methodName, "()[J");
}

template <>
QJNILocalRef<jfloatArray> QJNIObject::callObjectMethod<jfloatArray>(const char *methodName)
{
    return callObjectMethod<jfloatArray>(methodName, "()[F");
}

template <>
QJNILocalRef<jdoubleArray> QJNIObject::callObjectMethod<jdoubleArray>(const char *methodName)
{
    return callObjectMethod<jdoubleArray>(methodName, "()[D");
}

template <>
void QJNIObject::callStaticMethod<void>(const char *className,
                                        const char *methodName,
                                        const char *sig,
                                        ...)
{
    QAttachedJNIEnv env;
    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            env->CallStaticVoidMethodV(clazz, id, args);
            va_end(args);
        }
    }
}

template <>
void QJNIObject::callStaticMethod<void>(jclass clazz,
                                        const char *methodName,
                                        const char *sig,
                                        ...)
{
    QAttachedJNIEnv env;
    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        env->CallStaticVoidMethodV(clazz, id, args);
        va_end(args);
    }
}

template <>
jboolean QJNIObject::callStaticMethod<jboolean>(const char *className,
                                                const char *methodName,
                                                const char *sig,
                                                ...)
{
    QAttachedJNIEnv env;

    jboolean res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = env->CallStaticBooleanMethodV(clazz, id, args);
            va_end(args);
        }
    }

    return res;
}

template <>
jboolean QJNIObject::callStaticMethod<jboolean>(jclass clazz,
                                                const char *methodName,
                                                const char *sig,
                                                ...)
{
    QAttachedJNIEnv env;

    jboolean res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallStaticBooleanMethodV(clazz, id, args);
        va_end(args);
    }

    return res;
}

template <>
jbyte QJNIObject::callStaticMethod<jbyte>(const char *className,
                                          const char *methodName,
                                          const char *sig,
                                          ...)
{
    QAttachedJNIEnv env;

    jbyte res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = env->CallStaticByteMethodV(clazz, id, args);
            va_end(args);
        }
    }

    return res;
}

template <>
jbyte QJNIObject::callStaticMethod<jbyte>(jclass clazz,
                                          const char *methodName,
                                          const char *sig,
                                          ...)
{
    QAttachedJNIEnv env;

    jbyte res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallStaticByteMethodV(clazz, id, args);
        va_end(args);
    }

    return res;
}

template <>
jchar QJNIObject::callStaticMethod<jchar>(const char *className,
                                          const char *methodName,
                                          const char *sig,
                                          ...)
{
    QAttachedJNIEnv env;

    jchar res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = env->CallStaticCharMethodV(clazz, id, args);
            va_end(args);
        }
    }

    return res;
}

template <>
jchar QJNIObject::callStaticMethod<jchar>(jclass clazz,
                                          const char *methodName,
                                          const char *sig,
                                          ...)
{
    QAttachedJNIEnv env;

    jchar res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallStaticCharMethodV(clazz, id, args);
        va_end(args);
    }

    return res;
}


template <>
jshort QJNIObject::callStaticMethod<jshort>(const char *className,
                                            const char *methodName,
                                            const char *sig,
                                            ...)
{
    QAttachedJNIEnv env;

    jshort res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = env->CallStaticShortMethodV(clazz, id, args);
            va_end(args);
        }
    }

    return res;
}

template <>
jshort QJNIObject::callStaticMethod<jshort>(jclass clazz,
                                            const char *methodName,
                                            const char *sig,
                                            ...)
{
    QAttachedJNIEnv env;

    jshort res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallStaticShortMethodV(clazz, id, args);
        va_end(args);
    }

    return res;
}

template <>
jint QJNIObject::callStaticMethod<jint>(const char *className,
                                        const char *methodName,
                                        const char *sig,
                                        ...)
{
    QAttachedJNIEnv env;

    jint res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = env->CallStaticIntMethodV(clazz, id, args);
            va_end(args);
        }
    }

    return res;
}

template <>
jint QJNIObject::callStaticMethod<jint>(jclass clazz,
                                        const char *methodName,
                                        const char *sig,
                                        ...)
{
    QAttachedJNIEnv env;

    jint res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallStaticIntMethodV(clazz, id, args);
        va_end(args);
    }

    return res;
}

template <>
jlong QJNIObject::callStaticMethod<jlong>(const char *className,
                                          const char *methodName,
                                          const char *sig,
                                          ...)
{
    QAttachedJNIEnv env;

    jlong res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = env->CallStaticLongMethodV(clazz, id, args);
            va_end(args);
        }
    }

    return res;
}

template <>
jlong QJNIObject::callStaticMethod<jlong>(jclass clazz,
                                          const char *methodName,
                                          const char *sig,
                                          ...)
{
    QAttachedJNIEnv env;

    jlong res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallStaticLongMethodV(clazz, id, args);
        va_end(args);
    }

    return res;
}

template <>
jfloat QJNIObject::callStaticMethod<jfloat>(const char *className,
                                            const char *methodName,
                                            const char *sig,
                                            ...)
{
    QAttachedJNIEnv env;

    jfloat res = 0.f;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = env->CallStaticFloatMethodV(clazz, id, args);
            va_end(args);
        }
    }

    return res;
}

template <>
jfloat QJNIObject::callStaticMethod<jfloat>(jclass clazz,
                                            const char *methodName,
                                            const char *sig,
                                            ...)
{
    QAttachedJNIEnv env;

    jfloat res = 0.f;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallStaticFloatMethodV(clazz, id, args);
        va_end(args);
    }

    return res;
}

template <>
jdouble QJNIObject::callStaticMethod<jdouble>(const char *className,
                                              const char *methodName,
                                              const char *sig,
                                              ...)
{
    QAttachedJNIEnv env;

    jdouble res = 0.;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = env->CallStaticDoubleMethodV(clazz, id, args);
            va_end(args);
        }
    }

    return res;
}

template <>
jdouble QJNIObject::callStaticMethod<jdouble>(jclass clazz,
                                              const char *methodName,
                                              const char *sig,
                                              ...)
{
    QAttachedJNIEnv env;

    jdouble res = 0.;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallStaticDoubleMethodV(clazz, id, args);
        va_end(args);
    }

    return res;
}

template <>
QJNILocalRef<jobject> QJNIObject::callStaticObjectMethod<jobject>(const char *className,
                                                                  const char *methodName,
                                                                  const char *sig,
                                                                  ...)
{
    QAttachedJNIEnv env;

    jobject res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = env->CallStaticObjectMethodV(clazz, id, args);
            va_end(args);
        }
    }

    return QJNILocalRef<jobject>(res);
}

template <>
QJNILocalRef<jobject> QJNIObject::callStaticObjectMethod<jobject>(jclass clazz,
                                                                  const char *methodName,
                                                                  const char *sig,
                                                                  ...)
{
    QAttachedJNIEnv env;

    jobject res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = env->CallStaticObjectMethodV(clazz, id, args);
        va_end(args);
    }

    return QJNILocalRef<jobject>(res);
}

template <>
QJNILocalRef<jstring> QJNIObject::callStaticObjectMethod<jstring>(const char *className,
                                                                  const char *methodName,
                                                                  const char *sig,
                                                                  ...)
{
    QAttachedJNIEnv env;

    jstring res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = static_cast<jstring>(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
        }
    }

    return QJNILocalRef<jstring>(res);
}

template <>
QJNILocalRef<jstring> QJNIObject::callStaticObjectMethod<jstring>(jclass clazz,
                                                                  const char *methodName,
                                                                  const char *sig,
                                                                  ...)
{
    QAttachedJNIEnv env;

    jstring res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jstring>(env->CallStaticObjectMethodV(clazz, id, args));
        va_end(args);
    }

    return QJNILocalRef<jstring>(res);
}

template <>
QJNILocalRef<jobjectArray> QJNIObject::callStaticObjectMethod<jobjectArray>(const char *className,
                                                                            const char *methodName,
                                                                            const char *sig,
                                                                            ...)
{
    QAttachedJNIEnv env;

    jobjectArray res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = static_cast<jobjectArray>(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
        }
    }

    return QJNILocalRef<jobjectArray>(res);
}

template <>
QJNILocalRef<jobjectArray> QJNIObject::callStaticObjectMethod<jobjectArray>(jclass clazz,
                                                                            const char *methodName,
                                                                            const char *sig,
                                                                            ...)
{
    QAttachedJNIEnv env;

    jobjectArray res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jobjectArray>(env->CallStaticObjectMethodV(clazz, id, args));
        va_end(args);
    }

    return QJNILocalRef<jobjectArray>(res);
}

template <>
QJNILocalRef<jbooleanArray> QJNIObject::callStaticObjectMethod<jbooleanArray>(const char *className,
                                                                              const char *methodName,
                                                                              const char *sig,
                                                                              ...)
{
    QAttachedJNIEnv env;

    jbooleanArray res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = static_cast<jbooleanArray>(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
        }
    }

    return QJNILocalRef<jbooleanArray>(res);
}

template <>
QJNILocalRef<jbooleanArray> QJNIObject::callStaticObjectMethod<jbooleanArray>(jclass clazz,
                                                                              const char *methodName,
                                                                              const char *sig,
                                                                              ...)
{
    QAttachedJNIEnv env;

    jbooleanArray res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jbooleanArray>(env->CallStaticObjectMethodV(clazz, id, args));
        va_end(args);
    }

    return QJNILocalRef<jbooleanArray>(res);
}

template <>
QJNILocalRef<jbyteArray> QJNIObject::callStaticObjectMethod<jbyteArray>(const char *className,
                                                                        const char *methodName,
                                                                        const char *sig,
                                                                        ...)
{
    QAttachedJNIEnv env;

    jbyteArray res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = static_cast<jbyteArray>(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
        }
    }

    return QJNILocalRef<jbyteArray>(res);
}

template <>
QJNILocalRef<jbyteArray> QJNIObject::callStaticObjectMethod<jbyteArray>(jclass clazz,
                                                                        const char *methodName,
                                                                        const char *sig,
                                                                        ...)
{
    QAttachedJNIEnv env;

    jbyteArray res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jbyteArray>(env->CallStaticObjectMethodV(clazz, id, args));
        va_end(args);
    }

    return QJNILocalRef<jbyteArray>(res);
}

template <>
QJNILocalRef<jcharArray> QJNIObject::callStaticObjectMethod<jcharArray>(const char *className,
                                                                        const char *methodName,
                                                                        const char *sig,
                                                                        ...)
{
    QAttachedJNIEnv env;

    jcharArray res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = static_cast<jcharArray>(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
        }
    }

    return QJNILocalRef<jcharArray>(res);
}

template <>
QJNILocalRef<jcharArray> QJNIObject::callStaticObjectMethod<jcharArray>(jclass clazz,
                                                                        const char *methodName,
                                                                        const char *sig,
                                                                        ...)
{
    QAttachedJNIEnv env;

    jcharArray res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jcharArray>(env->CallStaticObjectMethodV(clazz, id, args));
        va_end(args);
    }

    return QJNILocalRef<jcharArray>(res);
}

template <>
QJNILocalRef<jshortArray> QJNIObject::callStaticObjectMethod<jshortArray>(const char *className,
                                                                          const char *methodName,
                                                                          const char *sig,
                                                                          ...)
{
    QAttachedJNIEnv env;

    jshortArray res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = static_cast<jshortArray>(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
        }
    }

    return QJNILocalRef<jshortArray>(res);
}

template <>
QJNILocalRef<jshortArray> QJNIObject::callStaticObjectMethod<jshortArray>(jclass clazz,
                                                                          const char *methodName,
                                                                          const char *sig,
                                                                          ...)
{
    QAttachedJNIEnv env;

    jshortArray res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jshortArray>(env->CallStaticObjectMethodV(clazz, id, args));
        va_end(args);
    }

    return QJNILocalRef<jshortArray>(res);
}

template <>
QJNILocalRef<jintArray> QJNIObject::callStaticObjectMethod<jintArray>(const char *className,
                                                                      const char *methodName,
                                                                      const char *sig,
                                                                      ...)
{
    QAttachedJNIEnv env;

    jintArray res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = static_cast<jintArray>(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
        }
    }

    return QJNILocalRef<jintArray>(res);
}

template <>
QJNILocalRef<jintArray> QJNIObject::callStaticObjectMethod<jintArray>(jclass clazz,
                                                                      const char *methodName,
                                                                      const char *sig,
                                                                      ...)
{
    QAttachedJNIEnv env;

    jintArray res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jintArray>(env->CallStaticObjectMethodV(clazz, id, args));
        va_end(args);
    }

    return QJNILocalRef<jintArray>(res);
}

template <>
QJNILocalRef<jlongArray> QJNIObject::callStaticObjectMethod<jlongArray>(const char *className,
                                                                        const char *methodName,
                                                                        const char *sig,
                                                                        ...)
{
    QAttachedJNIEnv env;

    jlongArray res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = static_cast<jlongArray>(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
        }
    }

    return QJNILocalRef<jlongArray>(res);
}

template <>
QJNILocalRef<jlongArray> QJNIObject::callStaticObjectMethod<jlongArray>(jclass clazz,
                                                                        const char *methodName,
                                                                        const char *sig,
                                                                        ...)
{
    QAttachedJNIEnv env;

    jlongArray res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jlongArray>(env->CallStaticObjectMethodV(clazz, id, args));
        va_end(args);
    }

    return QJNILocalRef<jlongArray>(res);
}

template <>
QJNILocalRef<jfloatArray> QJNIObject::callStaticObjectMethod<jfloatArray>(const char *className,
                                                                          const char *methodName,
                                                                          const char *sig,
                                                                          ...)
{
    QAttachedJNIEnv env;

    jfloatArray res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = static_cast<jfloatArray>(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
        }
    }

    return QJNILocalRef<jfloatArray>(res);
}

template <>
QJNILocalRef<jfloatArray> QJNIObject::callStaticObjectMethod<jfloatArray>(jclass clazz,
                                                                          const char *methodName,
                                                                          const char *sig,
                                                                          ...)
{
    QAttachedJNIEnv env;

    jfloatArray res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jfloatArray>(env->CallStaticObjectMethodV(clazz, id, args));
        va_end(args);
    }

    return QJNILocalRef<jfloatArray>(res);
}

template <>
QJNILocalRef<jdoubleArray> QJNIObject::callStaticObjectMethod<jdoubleArray>(const char *className,
                                                                            const char *methodName,
                                                                            const char *sig,
                                                                            ...)
{
    QAttachedJNIEnv env;

    jdoubleArray res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            va_list args;
            va_start(args, sig);
            res = static_cast<jdoubleArray>(env->CallStaticObjectMethodV(clazz, id, args));
            va_end(args);
        }
    }

    return QJNILocalRef<jdoubleArray>(res);
}

template <>
QJNILocalRef<jdoubleArray> QJNIObject::callStaticObjectMethod<jdoubleArray>(jclass clazz,
                                                                            const char *methodName,
                                                                            const char *sig,
                                                                            ...)
{
    QAttachedJNIEnv env;

    jdoubleArray res = 0;

    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        va_list args;
        va_start(args, sig);
        res = static_cast<jdoubleArray>(env->CallStaticObjectMethodV(clazz, id, args));
        va_end(args);
    }

    return QJNILocalRef<jdoubleArray>(res);
}

template <>
void QJNIObject::callStaticMethod<void>(const char *className, const char *methodName)
{
    callStaticMethod<void>(className, methodName, "()V");
}

template <>
void QJNIObject::callStaticMethod<void>(jclass clazz, const char *methodName)
{
    callStaticMethod<void>(clazz, methodName, "()V");
}

template <>
jboolean QJNIObject::callStaticMethod<jboolean>(const char *className, const char *methodName)
{
    return callStaticMethod<jboolean>(className, methodName, "()Z");
}

template <>
jboolean QJNIObject::callStaticMethod<jboolean>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jboolean>(clazz, methodName, "()Z");
}

template <>
jbyte QJNIObject::callStaticMethod<jbyte>(const char *className, const char *methodName)
{
    return callStaticMethod<jbyte>(className, methodName, "()B");
}

template <>
jbyte QJNIObject::callStaticMethod<jbyte>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jbyte>(clazz, methodName, "()B");
}

template <>
jchar QJNIObject::callStaticMethod<jchar>(const char *className, const char *methodName)
{
    return callStaticMethod<jchar>(className, methodName, "()C");
}

template <>
jchar QJNIObject::callStaticMethod<jchar>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jchar>(clazz, methodName, "()C");
}

template <>
jshort QJNIObject::callStaticMethod<jshort>(const char *className, const char *methodName)
{
    return callStaticMethod<jshort>(className, methodName, "()S");
}

template <>
jshort QJNIObject::callStaticMethod<jshort>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jshort>(clazz, methodName, "()S");
}

template <>
jint QJNIObject::callStaticMethod<jint>(const char *className, const char *methodName)
{
    return callStaticMethod<jint>(className, methodName, "()I");
}

template <>
jint QJNIObject::callStaticMethod<jint>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jint>(clazz, methodName, "()I");
}

template <>
jlong QJNIObject::callStaticMethod<jlong>(const char *className, const char *methodName)
{
    return callStaticMethod<jlong>(className, methodName, "()J");
}

template <>
jlong QJNIObject::callStaticMethod<jlong>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jlong>(clazz, methodName, "()J");
}

template <>
jfloat QJNIObject::callStaticMethod<jfloat>(const char *className, const char *methodName)
{
    return callStaticMethod<jfloat>(className, methodName, "()F");
}

template <>
jfloat QJNIObject::callStaticMethod<jfloat>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jfloat>(clazz, methodName, "()F");
}

template <>
jdouble QJNIObject::callStaticMethod<jdouble>(const char *className, const char *methodName)
{
    return callStaticMethod<jdouble>(className, methodName, "()D");
}

template <>
jdouble QJNIObject::callStaticMethod<jdouble>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jdouble>(clazz, methodName, "()D");
}

template <>
QJNILocalRef<jstring> QJNIObject::callStaticObjectMethod<jstring>(const char *className,
                                                                  const char *methodName)
{
    return callStaticObjectMethod<jstring>(className, methodName, "()Ljava/lang/String;");
}

template <>
QJNILocalRef<jstring> QJNIObject::callStaticObjectMethod<jstring>(jclass clazz,
                                                                  const char *methodName)
{
    return callStaticObjectMethod<jstring>(clazz, methodName, "()Ljava/lang/String;");
}

template <>
QJNILocalRef<jbooleanArray> QJNIObject::callStaticObjectMethod<jbooleanArray>(const char *className,
                                                                              const char *methodName)
{
    return callStaticObjectMethod<jbooleanArray>(className, methodName, "()[Z");
}

template <>
QJNILocalRef<jbooleanArray> QJNIObject::callStaticObjectMethod<jbooleanArray>(jclass clazz,
                                                                              const char *methodName)
{
    return callStaticObjectMethod<jbooleanArray>(clazz, methodName, "()[Z");
}

template <>
QJNILocalRef<jbyteArray> QJNIObject::callStaticObjectMethod<jbyteArray>(const char *className,
                                                                        const char *methodName)
{
    return callStaticObjectMethod<jbyteArray>(className, methodName, "()[B");
}

template <>
QJNILocalRef<jbyteArray> QJNIObject::callStaticObjectMethod<jbyteArray>(jclass clazz,
                                                                        const char *methodName)
{
    return callStaticObjectMethod<jbyteArray>(clazz, methodName, "()[B");
}

template <>
QJNILocalRef<jcharArray> QJNIObject::callStaticObjectMethod<jcharArray>(const char *className,
                                                                        const char *methodName)
{
    return callStaticObjectMethod<jcharArray>(className, methodName, "()[C");
}

template <>
QJNILocalRef<jcharArray> QJNIObject::callStaticObjectMethod<jcharArray>(jclass clazz,
                                                                        const char *methodName)
{
    return callStaticObjectMethod<jcharArray>(clazz, methodName, "()[C");
}

template <>
QJNILocalRef<jshortArray> QJNIObject::callStaticObjectMethod<jshortArray>(const char *className,
                                                                          const char *methodName)
{
    return callStaticObjectMethod<jshortArray>(className, methodName, "()[S");
}

template <>
QJNILocalRef<jshortArray> QJNIObject::callStaticObjectMethod<jshortArray>(jclass clazz,
                                                                          const char *methodName)
{
    return callStaticObjectMethod<jshortArray>(clazz, methodName, "()[S");
}

template <>
QJNILocalRef<jintArray> QJNIObject::callStaticObjectMethod<jintArray>(const char *className,
                                                                      const char *methodName)
{
    return callStaticObjectMethod<jintArray>(className, methodName, "()[I");
}

template <>
QJNILocalRef<jintArray> QJNIObject::callStaticObjectMethod<jintArray>(jclass clazz,
                                                                      const char *methodName)
{
    return callStaticObjectMethod<jintArray>(clazz, methodName, "()[I");
}

template <>
QJNILocalRef<jlongArray> QJNIObject::callStaticObjectMethod<jlongArray>(const char *className,
                                                                        const char *methodName)
{
    return callStaticObjectMethod<jlongArray>(className, methodName, "()[J");
}

template <>
QJNILocalRef<jlongArray> QJNIObject::callStaticObjectMethod<jlongArray>(jclass clazz,
                                                                        const char *methodName)
{
    return callStaticObjectMethod<jlongArray>(clazz, methodName, "()[J");
}

template <>
QJNILocalRef<jfloatArray> QJNIObject::callStaticObjectMethod<jfloatArray>(const char *className,
                                                                          const char *methodName)
{
    return callStaticObjectMethod<jfloatArray>(className, methodName, "()[F");
}

template <>
QJNILocalRef<jfloatArray> QJNIObject::callStaticObjectMethod<jfloatArray>(jclass clazz,
                                                                          const char *methodName)
{
    return callStaticObjectMethod<jfloatArray>(clazz, methodName, "()[F");
}

template <>
QJNILocalRef<jdoubleArray> QJNIObject::callStaticObjectMethod<jdoubleArray>(const char *className,
                                                                            const char *methodName)
{
    return callStaticObjectMethod<jdoubleArray>(className, methodName, "()[D");
}

template <>
QJNILocalRef<jdoubleArray> QJNIObject::callStaticObjectMethod<jdoubleArray>(jclass clazz,
                                                                            const char *methodName)
{
    return callStaticObjectMethod<jdoubleArray>(clazz, methodName, "()[D");
}

template <>
jboolean QJNIObject::getField<jboolean>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jboolean res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "Z");
    if (id)
        res = env->GetBooleanField(m_jobject, id);

    return res;
}

template <>
jbyte QJNIObject::getField<jbyte>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jbyte res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "B");
    if (id)
        res = env->GetByteField(m_jobject, id);

    return res;
}

template <>
jchar QJNIObject::getField<jchar>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jchar res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "C");
    if (id)
        res = env->GetCharField(m_jobject, id);

    return res;
}

template <>
jshort QJNIObject::getField<jshort>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jshort res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "S");
    if (id)
        res = env->GetShortField(m_jobject, id);

    return res;
}

template <>
jint QJNIObject::getField<jint>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jint res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "I");
    if (id)
        res = env->GetIntField(m_jobject, id);

    return res;
}

template <>
jlong QJNIObject::getField<jlong>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jlong res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "J");
    if (id)
        res = env->GetLongField(m_jobject, id);

    return res;
}

template <>
jfloat QJNIObject::getField<jfloat>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jfloat res = 0.f;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "F");
    if (id)
        res = env->GetFloatField(m_jobject, id);

    return res;
}

template <>
jdouble QJNIObject::getField<jdouble>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jdouble res = 0.;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "D");
    if (id)
        res = env->GetDoubleField(m_jobject, id);

    return res;
}

template <>
QJNILocalRef<jobject> QJNIObject::getObjectField<jobject>(const char *fieldName, const char *sig)
{
    QAttachedJNIEnv env;
    jobject res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, sig);
    if (id)
        res = env->GetObjectField(m_jobject, id);

    return QJNILocalRef<jobject>(res);
}

template <>
QJNILocalRef<jbooleanArray> QJNIObject::getObjectField<jbooleanArray>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jbooleanArray res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[Z");
    if (id)
        res = static_cast<jbooleanArray>(env->GetObjectField(m_jobject, id));

    return QJNILocalRef<jbooleanArray>(res);
}

template <>
QJNILocalRef<jbyteArray> QJNIObject::getObjectField<jbyteArray>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jbyteArray res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[B");
    if (id)
        res = static_cast<jbyteArray>(env->GetObjectField(m_jobject, id));

    return QJNILocalRef<jbyteArray>(res);
}

template <>
QJNILocalRef<jcharArray> QJNIObject::getObjectField<jcharArray>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jcharArray res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[C");
    if (id)
        res = static_cast<jcharArray>(env->GetObjectField(m_jobject, id));

    return QJNILocalRef<jcharArray>(res);
}

template <>
QJNILocalRef<jshortArray> QJNIObject::getObjectField<jshortArray>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jshortArray res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[S");
    if (id)
        res = static_cast<jshortArray>(env->GetObjectField(m_jobject, id));

    return QJNILocalRef<jshortArray>(res);
}

template <>
QJNILocalRef<jintArray> QJNIObject::getObjectField<jintArray>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jintArray res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[I");
    if (id)
        res = static_cast<jintArray>(env->GetObjectField(m_jobject, id));

    return QJNILocalRef<jintArray>(res);
}

template <>
QJNILocalRef<jlongArray> QJNIObject::getObjectField<jlongArray>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jlongArray res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[J");
    if (id)
        res = static_cast<jlongArray>(env->GetObjectField(m_jobject, id));

    return QJNILocalRef<jlongArray>(res);
}

template <>
QJNILocalRef<jfloatArray> QJNIObject::getObjectField<jfloatArray>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jfloatArray res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[F");
    if (id)
        res = static_cast<jfloatArray>(env->GetObjectField(m_jobject, id));

    return QJNILocalRef<jfloatArray>(res);
}

template <>
QJNILocalRef<jdoubleArray> QJNIObject::getObjectField<jdoubleArray>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jdoubleArray res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[D");
    if (id)
        res = static_cast<jdoubleArray>(env->GetObjectField(m_jobject, id));

    return QJNILocalRef<jdoubleArray>(res);
}

template <>
QJNILocalRef<jstring> QJNIObject::getObjectField<jstring>(const char *fieldName)
{
    QAttachedJNIEnv env;
    jstring res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "Ljava/lang/String;");
    if (id)
        res = static_cast<jstring>(env->GetObjectField(m_jobject, id));

    return QJNILocalRef<jstring>(res);
}

template <>
QJNILocalRef<jobjectArray> QJNIObject::getObjectField<jobjectArray>(const char *fieldName,
                                                                    const char *sig)
{
    QAttachedJNIEnv env;
    jobjectArray res = 0;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, sig);
    if (id)
        res = static_cast<jobjectArray>(env->GetObjectField(m_jobject, id));

    return QJNILocalRef<jobjectArray>(res);
}

template <>
void QJNIObject::setField<jboolean>(const char *fieldName, jboolean value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "Z");
    if (id)
        env->SetBooleanField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jbyte>(const char *fieldName, jbyte value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "B");
    if (id)
        env->SetByteField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jchar>(const char *fieldName, jchar value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "C");
    if (id)
        env->SetCharField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jshort>(const char *fieldName, jshort value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "S");
    if (id)
        env->SetShortField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jint>(const char *fieldName, jint value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "I");
    if (id)
        env->SetIntField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jlong>(const char *fieldName, jlong value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "J");
    if (id)
        env->SetLongField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jfloat>(const char *fieldName, jfloat value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "F");
    if (id)
        env->SetFloatField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jdouble>(const char *fieldName, jdouble value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "D");
    if (id)
        env->SetDoubleField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jbooleanArray>(const char *fieldName, jbooleanArray value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[Z");
    if (id)
        env->SetObjectField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jbyteArray>(const char *fieldName, jbyteArray value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[B");
    if (id)
        env->SetObjectField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jcharArray>(const char *fieldName, jcharArray value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[C");
    if (id)
        env->SetObjectField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jshortArray>(const char *fieldName, jshortArray value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[S");
    if (id)
        env->SetObjectField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jintArray>(const char *fieldName, jintArray value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[I");
    if (id)
        env->SetObjectField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jlongArray>(const char *fieldName, jlongArray value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[J");
    if (id)
        env->SetObjectField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jfloatArray>(const char *fieldName, jfloatArray value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[F");
    if (id)
        env->SetObjectField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jdoubleArray>(const char *fieldName, jdoubleArray value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "[D");
    if (id)
        env->SetObjectField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jstring>(const char *fieldName, jstring value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, "Ljava/lang/String;");
    if (id)
        env->SetObjectField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jobject>(const char *fieldName, const char *sig, jobject value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, sig);
    if (id)
        env->SetObjectField(m_jobject, id, value);

}

template <>
void QJNIObject::setField<jobjectArray>(const char *fieldName,
                                        const char *sig,
                                        jobjectArray value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, m_jclass, fieldName, sig);
    if (id)
        env->SetObjectField(m_jobject, id, value);

}

template <>
jboolean QJNIObject::getStaticField<jboolean>(jclass clazz, const char *fieldName)
{
    QAttachedJNIEnv env;

    jboolean res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "Z", true);
    if (id)
        res = env->GetStaticBooleanField(clazz, id);

    return res;
}

template <>
jboolean QJNIObject::getStaticField<jboolean>(const char *className, const char *fieldName)
{
    QAttachedJNIEnv env;

    jboolean res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jboolean>(clazz, fieldName);

    return res;
}

template <>
jbyte QJNIObject::getStaticField<jbyte>(jclass clazz, const char *fieldName)
{
    QAttachedJNIEnv env;

    jbyte res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "B", true);
    if (id)
        res = env->GetStaticByteField(clazz, id);

    return res;
}

template <>
jbyte QJNIObject::getStaticField<jbyte>(const char *className, const char *fieldName)
{
    QAttachedJNIEnv env;

    jbyte res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jbyte>(clazz, fieldName);

    return res;
}

template <>
jchar QJNIObject::getStaticField<jchar>(jclass clazz, const char *fieldName)
{
    QAttachedJNIEnv env;

    jchar res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "C", true);
    if (id)
        res = env->GetStaticCharField(clazz, id);

    return res;
}

template <>
jchar QJNIObject::getStaticField<jchar>(const char *className, const char *fieldName)
{
    QAttachedJNIEnv env;

    jchar res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jchar>(clazz, fieldName);

    return res;
}

template <>
jshort QJNIObject::getStaticField<jshort>(jclass clazz, const char *fieldName)
{
    QAttachedJNIEnv env;

    jshort res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "S", true);
    if (id)
        res = env->GetStaticShortField(clazz, id);

    return res;
}

template <>
jshort QJNIObject::getStaticField<jshort>(const char *className, const char *fieldName)
{
    QAttachedJNIEnv env;

    jshort res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jshort>(clazz, fieldName);

    return res;
}

template <>
jint QJNIObject::getStaticField<jint>(jclass clazz, const char *fieldName)
{
    QAttachedJNIEnv env;

    jint res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "I", true);
    if (id)
        res = env->GetStaticIntField(clazz, id);

    return res;
}

template <>
jint QJNIObject::getStaticField<jint>(const char *className, const char *fieldName)
{
    QAttachedJNIEnv env;

    jint res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jint>(clazz, fieldName);

    return res;
}

template <>
jlong QJNIObject::getStaticField<jlong>(jclass clazz, const char *fieldName)
{
    QAttachedJNIEnv env;

    jlong res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "J", true);
    if (id)
        res = env->GetStaticLongField(clazz, id);

    return res;
}

template <>
jlong QJNIObject::getStaticField<jlong>(const char *className, const char *fieldName)
{
    QAttachedJNIEnv env;

    jlong res = 0;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jlong>(clazz, fieldName);

    return res;
}

template <>
jfloat QJNIObject::getStaticField<jfloat>(jclass clazz, const char *fieldName)
{
    QAttachedJNIEnv env;

    jfloat res = 0.f;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "F", true);
    if (id)
        res = env->GetStaticFloatField(clazz, id);

    return res;
}

template <>
jfloat QJNIObject::getStaticField<jfloat>(const char *className, const char *fieldName)
{
    QAttachedJNIEnv env;

    jfloat res = 0.f;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jfloat>(clazz, fieldName);

    return res;
}

template <>
jdouble QJNIObject::getStaticField<jdouble>(jclass clazz, const char *fieldName)
{
    QAttachedJNIEnv env;

    jdouble res = 0.;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "D", true);
    if (id)
        res = env->GetStaticDoubleField(clazz, id);

    return res;
}

template <>
jdouble QJNIObject::getStaticField<jdouble>(const char *className, const char *fieldName)
{
    QAttachedJNIEnv env;

    jdouble res = 0.;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jdouble>(clazz, fieldName);

    return res;
}

template <>
QJNILocalRef<jobject> QJNIObject::getStaticObjectField<jobject>(jclass clazz,
                                                                const char *fieldName,
                                                                const char *sig)
{
    QAttachedJNIEnv env;

    jobject res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, sig, true);
    if (id)
        res = env->GetStaticObjectField(clazz, id);

    return QJNILocalRef<jobject>(res);
}

template <>
QJNILocalRef<jobject> QJNIObject::getStaticObjectField<jobject>(const char *className,
                                                                const char *fieldName,
                                                                const char *sig)
{
    QAttachedJNIEnv env;

    QJNILocalRef<jobject> res;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField<jobject>(clazz, fieldName, sig);

    return res;
}

template <>
QJNILocalRef<jstring> QJNIObject::getStaticObjectField<jstring>(jclass clazz,
                                                                const char *fieldName)
{
    QAttachedJNIEnv env;

    jstring res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "Ljava/lang/String;", true);
    if (id)
        res = static_cast<jstring>(env->GetStaticObjectField(clazz, id));

    return QJNILocalRef<jstring>(res);
}

template <>
QJNILocalRef<jstring> QJNIObject::getStaticObjectField<jstring>(const char *className,
                                                                const char *fieldName)
{
    QAttachedJNIEnv env;

    QJNILocalRef<jstring> res;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField<jstring>(clazz, fieldName);

    return res;
}

template <>
QJNILocalRef<jbooleanArray> QJNIObject::getStaticObjectField<jbooleanArray>(jclass clazz,
                                                                            const char *fieldName)
{
    QAttachedJNIEnv env;

    jbooleanArray res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "[Z", true);
    if (id)
        res = static_cast<jbooleanArray>(env->GetStaticObjectField(clazz, id));

    return QJNILocalRef<jbooleanArray>(res);
}

template <>
QJNILocalRef<jbooleanArray> QJNIObject::getStaticObjectField<jbooleanArray>(const char *className,
                                                                            const char *fieldName)
{
    QAttachedJNIEnv env;

    QJNILocalRef<jbooleanArray> res;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField<jbooleanArray>(clazz, fieldName);

    return res;
}

template <>
QJNILocalRef<jbyteArray> QJNIObject::getStaticObjectField<jbyteArray>(jclass clazz,
                                                                      const char *fieldName)
{
    QAttachedJNIEnv env;

    jbyteArray res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "[B", true);
    if (id)
        res = static_cast<jbyteArray>(env->GetStaticObjectField(clazz, id));

    return QJNILocalRef<jbyteArray>(res);
}

template <>
QJNILocalRef<jbyteArray> QJNIObject::getStaticObjectField<jbyteArray>(const char *className,
                                                                      const char *fieldName)
{
    QAttachedJNIEnv env;

    QJNILocalRef<jbyteArray> res;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField<jbyteArray>(clazz, fieldName);

    return res;
}

template <>
QJNILocalRef<jcharArray> QJNIObject::getStaticObjectField<jcharArray>(jclass clazz,
                                                                      const char *fieldName)
{
    QAttachedJNIEnv env;

    jcharArray res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "[C", true);
    if (id)
        res = static_cast<jcharArray>(env->GetStaticObjectField(clazz, id));

    return QJNILocalRef<jcharArray>(res);
}

template <>
QJNILocalRef<jcharArray> QJNIObject::getStaticObjectField<jcharArray>(const char *className,
                                                                      const char *fieldName)
{
    QAttachedJNIEnv env;

    QJNILocalRef<jcharArray> res;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField<jcharArray>(clazz, fieldName);

    return res;
}

template <>
QJNILocalRef<jshortArray> QJNIObject::getStaticObjectField<jshortArray>(jclass clazz,
                                                                        const char *fieldName)
{
    QAttachedJNIEnv env;

    jshortArray res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "[S", true);
    if (id)
        res = static_cast<jshortArray>(env->GetStaticObjectField(clazz, id));

    return QJNILocalRef<jshortArray>(res);
}

template <>
QJNILocalRef<jshortArray> QJNIObject::getStaticObjectField<jshortArray>(const char *className,
                                                                        const char *fieldName)
{
    QAttachedJNIEnv env;

    QJNILocalRef<jshortArray> res;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField<jshortArray>(clazz, fieldName);

    return res;
}

template <>
QJNILocalRef<jintArray> QJNIObject::getStaticObjectField<jintArray>(jclass clazz,
                                                                    const char *fieldName)
{
    QAttachedJNIEnv env;

    jintArray res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "[I", true);
    if (id)
        res = static_cast<jintArray>(env->GetStaticObjectField(clazz, id));

    return QJNILocalRef<jintArray>(res);
}

template <>
QJNILocalRef<jintArray> QJNIObject::getStaticObjectField<jintArray>(const char *className,
                                                                    const char *fieldName)
{
    QAttachedJNIEnv env;

    QJNILocalRef<jintArray> res;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField<jintArray>(clazz, fieldName);

    return res;
}

template <>
QJNILocalRef<jlongArray> QJNIObject::getStaticObjectField<jlongArray>(jclass clazz,
                                                                      const char *fieldName)
{
    QAttachedJNIEnv env;

    jlongArray res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "[J", true);
    if (id)
        res = static_cast<jlongArray>(env->GetStaticObjectField(clazz, id));

    return QJNILocalRef<jlongArray>(res);
}

template <>
QJNILocalRef<jlongArray> QJNIObject::getStaticObjectField<jlongArray>(const char *className,
                                                                      const char *fieldName)
{
    QAttachedJNIEnv env;

    QJNILocalRef<jlongArray> res;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField<jlongArray>(clazz, fieldName);

    return res;
}

template <>
QJNILocalRef<jfloatArray> QJNIObject::getStaticObjectField<jfloatArray>(jclass clazz,
                                                                        const char *fieldName)
{
    QAttachedJNIEnv env;

    jfloatArray res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "[F", true);
    if (id)
        res = static_cast<jfloatArray>(env->GetStaticObjectField(clazz, id));

    return QJNILocalRef<jfloatArray>(res);
}

template <>
QJNILocalRef<jfloatArray> QJNIObject::getStaticObjectField<jfloatArray>(const char *className,
                                                                        const char *fieldName)
{
    QAttachedJNIEnv env;

    QJNILocalRef<jfloatArray> res;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField<jfloatArray>(clazz, fieldName);

    return res;
}

template <>
QJNILocalRef<jdoubleArray> QJNIObject::getStaticObjectField<jdoubleArray>(jclass clazz,
                                                                          const char *fieldName)
{
    QAttachedJNIEnv env;

    jdoubleArray res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, "[D", true);
    if (id)
        res = static_cast<jdoubleArray>(env->GetStaticObjectField(clazz, id));

    return QJNILocalRef<jdoubleArray>(res);
}

template <>
QJNILocalRef<jdoubleArray> QJNIObject::getStaticObjectField<jdoubleArray>(const char *className,
                                                                          const char *fieldName)
{
    QAttachedJNIEnv env;

    QJNILocalRef<jdoubleArray> res;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField<jdoubleArray>(clazz, fieldName);

    return res;
}

template <>
QJNILocalRef<jobjectArray> QJNIObject::getStaticObjectField<jobjectArray>(jclass clazz,
                                                                          const char *fieldName,
                                                                          const char *sig)
{
    QAttachedJNIEnv env;

    jobjectArray res = 0;

    jfieldID id = getCachedFieldID(env, clazz, fieldName, sig, true);
    if (id)
        res = static_cast<jobjectArray>(env->GetStaticObjectField(clazz, id));

    return QJNILocalRef<jobjectArray>(res);
}

template <>
QJNILocalRef<jobjectArray> QJNIObject::getStaticObjectField<jobjectArray>(const char *className,
                                                                          const char *fieldName,
                                                                          const char *sig)
{
    QAttachedJNIEnv env;

    QJNILocalRef<jobjectArray> res;

    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField<jobjectArray>(clazz, fieldName, sig);

    return res;
}

template <>
void QJNIObject::setStaticField<jboolean>(jclass clazz, const char *fieldName, jboolean value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "Z", true);
    if (id)
        env->SetStaticBooleanField(clazz, id, value);
}

template <>
void QJNIObject::setStaticField<jboolean>(const char *className,
                                          const char *fieldName,
                                          jboolean value)
{
    QAttachedJNIEnv env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jboolean>(clazz, fieldName, value);
}

template <>
void QJNIObject::setStaticField<jbyte>(jclass clazz, const char *fieldName, jbyte value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "B", true);
    if (id)
        env->SetStaticByteField(clazz, id, value);
}

template <>
void QJNIObject::setStaticField<jbyte>(const char *className,
                                       const char *fieldName,
                                       jbyte value)
{
    QAttachedJNIEnv env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jbyte>(clazz, fieldName, value);
}

template <>
void QJNIObject::setStaticField<jchar>(jclass clazz, const char *fieldName, jchar value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "C", true);
    if (id)
        env->SetStaticCharField(clazz, id, value);
}

template <>
void QJNIObject::setStaticField<jchar>(const char *className,
                                       const char *fieldName,
                                       jchar value)
{
    QAttachedJNIEnv env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jchar>(clazz, fieldName, value);
}

template <>
void QJNIObject::setStaticField<jshort>(jclass clazz, const char *fieldName, jshort value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "S", true);
    if (id)
        env->SetStaticShortField(clazz, id, value);
}

template <>
void QJNIObject::setStaticField<jshort>(const char *className,
                                        const char *fieldName,
                                        jshort value)
{
    QAttachedJNIEnv env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jshort>(clazz, fieldName, value);
}

template <>
void QJNIObject::setStaticField<jint>(jclass clazz, const char *fieldName, jint value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "I", true);
    if (id)
        env->SetStaticIntField(clazz, id, value);
}

template <>
void QJNIObject::setStaticField<jint>(const char *className, const char *fieldName, jint value)
{
    QAttachedJNIEnv env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jint>(clazz, fieldName, value);
}

template <>
void QJNIObject::setStaticField<jlong>(jclass clazz, const char *fieldName, jlong value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "J", true);
    if (id)
        env->SetStaticLongField(clazz, id, value);
}

template <>
void QJNIObject::setStaticField<jlong>(const char *className,
                                       const char *fieldName,
                                       jlong value)
{
    QAttachedJNIEnv env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jlong>(clazz, fieldName, value);
}

template <>
void QJNIObject::setStaticField<jfloat>(jclass clazz, const char *fieldName, jfloat value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "F", true);
    if (id)
        env->SetStaticFloatField(clazz, id, value);
}

template <>
void QJNIObject::setStaticField<jfloat>(const char *className,
                                        const char *fieldName,
                                        jfloat value)
{
    QAttachedJNIEnv env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jfloat>(clazz, fieldName, value);
}

template <>
void QJNIObject::setStaticField<jdouble>(jclass clazz, const char *fieldName, jdouble value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "D", true);
    if (id)
        env->SetStaticDoubleField(clazz, id, value);
}

template <>
void QJNIObject::setStaticField<jdouble>(const char *className,
                                         const char *fieldName,
                                         jdouble value)
{
    QAttachedJNIEnv env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jdouble>(clazz, fieldName, value);
}

template <>
void QJNIObject::setStaticField<jobject>(jclass clazz,
                                         const char *fieldName,
                                         const char *sig,
                                         jobject value)
{
    QAttachedJNIEnv env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, sig, true);
    if (id)
        env->SetStaticObjectField(clazz, id, value);
}

template <>
void QJNIObject::setStaticField<jobject>(const char *className,
                                         const char *fieldName,
                                         const char *sig,
                                         jobject value)
{
    QAttachedJNIEnv env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jobject>(clazz, fieldName, sig, value);
}

template <>
void QJNIObject::setStaticField<jstring>(const char *className,
                                         const char *fieldName,
                                         jstring value)
{
    setStaticField<jobject>(className, fieldName, "Ljava/lang/String;", value);
}

template <>
void QJNIObject::setStaticField<jstring>(jclass clazz, const char *fieldName, jstring value)
{
    setStaticField<jobject>(clazz, fieldName, "Ljava/lang/String;", value);
}

template <>
void QJNIObject::setStaticField<jbooleanArray>(const char *className,
                                               const char *fieldName,
                                               jbooleanArray value)
{
    setStaticField<jobject>(className, fieldName, "[Z", value);
}

template <>
void QJNIObject::setStaticField<jbooleanArray>(jclass clazz,
                                               const char *fieldName,
                                               jbooleanArray value)
{
    setStaticField<jobject>(clazz, fieldName, "[Z", value);
}

template <>
void QJNIObject::setStaticField<jbyteArray>(const char *className,
                                            const char *fieldName,
                                            jbyteArray value)
{
    setStaticField<jobject>(className, fieldName, "[B", value);
}

template <>
void QJNIObject::setStaticField<jbyteArray>(jclass clazz,
                                            const char *fieldName,
                                            jbyteArray value)
{
    setStaticField<jobject>(clazz, fieldName, "[B", value);
}

template <>
void QJNIObject::setStaticField<jcharArray>(const char *className,
                                            const char *fieldName,
                                            jcharArray value)
{
    setStaticField<jobject>(className, fieldName, "[C", value);
}

template <>
void QJNIObject::setStaticField<jcharArray>(jclass clazz,
                                            const char *fieldName,
                                            jcharArray value)
{
    setStaticField<jobject>(clazz, fieldName, "[C", value);
}

template <>
void QJNIObject::setStaticField<jshortArray>(const char *className,
                                             const char *fieldName,
                                             jshortArray value)
{
    setStaticField<jobject>(className, fieldName, "[S", value);
}

template <>
void QJNIObject::setStaticField<jshortArray>(jclass clazz,
                                             const char *fieldName,
                                             jshortArray value)
{
    setStaticField<jobject>(clazz, fieldName, "[S", value);
}

template <>
void QJNIObject::setStaticField<jintArray>(const char *className,
                                           const char *fieldName,
                                           jintArray value)
{
    setStaticField<jobject>(className, fieldName, "[I", value);
}

template <>
void QJNIObject::setStaticField<jintArray>(jclass clazz,
                                           const char *fieldName,
                                           jintArray value)
{
    setStaticField<jobject>(clazz, fieldName, "[I", value);
}

template <>
void QJNIObject::setStaticField<jlongArray>(const char *className,
                                            const char *fieldName,
                                            jlongArray value)
{
    setStaticField<jobject>(className, fieldName, "[J", value);
}

template <>
void QJNIObject::setStaticField<jlongArray>(jclass clazz,
                                            const char *fieldName,
                                            jlongArray value)
{
    setStaticField<jobject>(clazz, fieldName, "[J", value);
}

template <>
void QJNIObject::setStaticField<jfloatArray>(const char *className,
                                             const char *fieldName,
                                             jfloatArray value)
{
    setStaticField<jobject>(className, fieldName, "[F", value);
}

template <>
void QJNIObject::setStaticField<jfloatArray>(jclass clazz,
                                             const char *fieldName,
                                             jfloatArray value)
{
    setStaticField<jobject>(clazz, fieldName, "[F", value);
}

template <>
void QJNIObject::setStaticField<jdoubleArray>(const char *className,
                                              const char *fieldName,
                                              jdoubleArray value)
{
    setStaticField<jobject>(className, fieldName, "[D", value);
}

template <>
void QJNIObject::setStaticField<jdoubleArray>(jclass clazz,
                                              const char *fieldName,
                                              jdoubleArray value)
{
    setStaticField<jobject>(clazz, fieldName, "[D", value);
}


QT_END_NAMESPACE
