/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QJNIOBJECT_H
#define QJNIOBJECT_H

#include <QtCore/qsharedpointer.h>

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
#include <jni.h>
#include <QtCore/qjnienvironment.h>

QT_BEGIN_NAMESPACE

class QJniObjectPrivate;

class Q_CORE_EXPORT QJniObject
{
public:
    QJniObject();
    explicit QJniObject(const char *className);
    explicit QJniObject(const char *className, const char *signature, ...);
    explicit QJniObject(jclass clazz);
    explicit QJniObject(jclass clazz, const char *signature, ...);
    QJniObject(jobject globalRef);
    ~QJniObject();

    jobject object() const;
    template <typename T> T object() const
    {
        assertJniObjectType<T>();
        return static_cast<T>(javaObject());
    }

    template <typename T>
    T callMethod(const char *methodName, const char *signature, ...) const
    {
        assertJniPrimitiveType<T>();
        QJniEnvironment env;
        T res{};
        jmethodID id = getCachedMethodID(env.jniEnv(), methodName, signature);
        if (id) {
            va_list args;
            va_start(args, signature);
            callMethodForType<T>(env.jniEnv(), res, object(), id, args);
            va_end(args);
            if (env.checkAndClearExceptions())
                res = {};
        }
        return res;
    }

    template <>
    void callMethod<void>(const char *methodName, const char *signature, ...) const
    {
        QJniEnvironment env;
        jmethodID id = getCachedMethodID(env.jniEnv(), methodName, signature);
        if (id) {
            va_list args;
            va_start(args, signature);
            callVoidMethodV(env.jniEnv(), id, args);
            va_end(args);
            env.checkAndClearExceptions();
        }
    }

    template <typename T>
    T callMethod(const char *methodName) const
    {
        assertJniPrimitiveType<T>();
        constexpr const char *signature = getTypeSignature<T>();
        return callMethod<T>(methodName, QByteArray(signature).prepend("()").constData());
    }

    template <>
    void callMethod<void>(const char *methodName) const
    {
        callMethod<void>(methodName, "()V");
    }

    template <typename T>
    QJniObject callObjectMethod(const char *methodName) const
    {
        assertJniObjectType<T>();
        constexpr const char *signature = getTypeSignature<T>();
        return callObjectMethod(methodName, QByteArray(signature).prepend("()").constData());
    }

    QJniObject callObjectMethod(const char *methodName, const char *signature, ...) const;

    template <typename T>
    static T callStaticMethod(const char *className, const char *methodName,
                              const char *signature, ...)
    {
        assertJniPrimitiveType<T>();
        QJniEnvironment env;
        T res{};
        jclass clazz = QJniObject::loadClass(className, env.jniEnv());
        if (clazz) {
            jmethodID id = getCachedMethodID(env.jniEnv(), clazz,
                                             QJniObject::toBinaryEncClassName(className),
                                             methodName, signature, true);
            if (id) {
                va_list args;
                va_start(args, signature);
                callStaticMethodForType<T>(env.jniEnv(), res, clazz, id, args);
                va_end(args);
                if (env.checkAndClearExceptions())
                    res = {};
            }
        }
        return res;
    }

    template <>
    void callStaticMethod<void>(const char *className, const char *methodName,
                                const char *signature, ...)
    {
        QJniEnvironment env;
        jclass clazz = QJniObject::loadClass(className, env.jniEnv());
        if (clazz) {
            jmethodID id = getCachedMethodID(env.jniEnv(), clazz,
                                             QJniObject::toBinaryEncClassName(className),
                                             methodName, signature, true);
            if (id) {
                va_list args;
                va_start(args, signature);
                env->CallStaticVoidMethodV(clazz, id, args);
                va_end(args);
                env.checkAndClearExceptions();
            }
        }
    }

    template <typename T>
    static T callStaticMethod(const char *className, const char *methodName)
    {
        assertJniPrimitiveType<T>();
        constexpr const char *signature = getTypeSignature<T>();
        return callStaticMethod<T>(className, methodName, QByteArray(signature).prepend("()").constData());
    }

    template <>
    void callStaticMethod<void>(const char *className, const char *methodName)
    {
        callStaticMethod<void>(className, methodName, "()V");
    }

    template <typename T>
    static T callStaticMethod(jclass clazz, const char *methodName, const char *signature, ...)
    {
        assertJniPrimitiveType<T>();
        QJniEnvironment env;
        T res{};
        if (clazz) {
            jmethodID id = getMethodID(env.jniEnv(), clazz, methodName, signature, true);
            if (id) {
                va_list args;
                va_start(args, signature);
                callStaticMethodForType<T>(env.jniEnv(), res, clazz, id, args);
                va_end(args);
                if (env.checkAndClearExceptions())
                    res = {};
            }
        }
        return res;
    }

    template <>
    void callStaticMethod<void>(jclass clazz, const char *methodName,
                                const char *signature, ...)
    {
        QJniEnvironment env;
        if (clazz) {
            jmethodID id = getMethodID(env.jniEnv(), clazz, methodName, signature, true);
            if (id) {
                va_list args;
                va_start(args, signature);
                env->CallStaticVoidMethodV(clazz, id, args);
                va_end(args);
                env.checkAndClearExceptions();
            }
        }
    }

    template <typename T> static T callStaticMethod(jclass clazz, const char *methodName)
    {
        assertJniPrimitiveType<T>();
        constexpr const char *signature = getTypeSignature<T>();
        return callStaticMethod<T>(clazz, methodName, QByteArray(signature).prepend("()").constData());
    }

    template <>
    void callStaticMethod<void>(jclass clazz, const char *methodName)
    {
        callStaticMethod<void>(clazz, methodName, "()V");
    }

    template <typename T>
    static QJniObject callStaticObjectMethod(const char *className, const char *methodName)
    {
        assertJniObjectType<T>();
        constexpr const char *signature = getTypeSignature<T>();
        return callStaticObjectMethod(className, methodName, QByteArray(signature).prepend("()").constData());
    }

    static QJniObject callStaticObjectMethod(const char *className, const char *methodName,
                                             const char *signature, ...);

    template <typename T>
    static QJniObject callStaticObjectMethod(jclass clazz, const char *methodName)
    {
        assertJniObjectType<T>();
        constexpr const char *signature = getTypeSignature<T>();
        return callStaticObjectMethod(clazz, methodName, QByteArray(signature).prepend("()").constData());
    }

    static QJniObject callStaticObjectMethod(jclass clazz, const char *methodName,
                                             const char *signature, ...);

    template <typename T> T getField(const char *fieldName) const
    {
        assertJniPrimitiveType<T>();
        QJniEnvironment env;
        T res{};
        constexpr const char *signature = getTypeSignature<T>();
        jfieldID id = getCachedFieldID(env.jniEnv(), fieldName, signature);
        if (id) {
            getFieldForType<T>(env.jniEnv(), res, object(), id);
            if (env.checkAndClearExceptions())
                res = {};
        }
        return res;
    }

    template <typename T>
    static T getStaticField(const char *className, const char *fieldName)
    {
        assertJniPrimitiveType<T>();
        QJniEnvironment env;
        jclass clazz = QJniObject::loadClass(className, env.jniEnv());
        if (!clazz)
            return 0;

        constexpr const char *signature = getTypeSignature<T>();
        jfieldID id = getCachedFieldID(env.jniEnv(), clazz,
                                       QJniObject::toBinaryEncClassName(className),
                                       fieldName,
                                       signature, true);
        if (!id)
            return 0;

        T res{};
        getStaticFieldForType<T>(env.jniEnv(), res, clazz, id);
        if (env.checkAndClearExceptions())
            res = {};
        return res;
    }

    template <typename T>
    static T getStaticField(jclass clazz, const char *fieldName)
    {
        assertJniPrimitiveType<T>();
        QJniEnvironment env;
        T res{};
        constexpr const char *signature = getTypeSignature<T>();
        jfieldID id = getFieldID(env.jniEnv(), clazz, fieldName, signature, true);
        if (id) {
            getStaticFieldForType<T>(env.jniEnv(), res, clazz, id);
            if (env.checkAndClearExceptions())
                res = {};
        }
        return res;
    }

    template <typename T>
    QJniObject getObjectField(const char *fieldName) const
    {
        assertJniObjectType<T>();
        constexpr const char *signature = getTypeSignature<T>();
        return getObjectField(fieldName, signature);
    }

    QJniObject getObjectField(const char *fieldName, const char *signature) const;

    template <typename T>
    static QJniObject getStaticObjectField(const char *className, const char *fieldName)
    {
        assertJniObjectType<T>();
        constexpr const char *signature = getTypeSignature<T>();
        return getStaticObjectField(className, fieldName, signature);
    }

    static QJniObject getStaticObjectField(const char *className,
                                           const char *fieldName,
                                           const char *signature);

    template <typename T>
    static QJniObject getStaticObjectField(jclass clazz, const char *fieldName)
    {
        assertJniObjectType<T>();
        constexpr const char *signature = getTypeSignature<T>();
        return getStaticObjectField(clazz, fieldName, signature);
    }

    static QJniObject getStaticObjectField(jclass clazz, const char *fieldName,
                                           const char *signature);

    template <typename T> void setField(const char *fieldName, T value)
    {
        assertJniType<T>();
        QJniEnvironment env;
        constexpr const char *signature = getTypeSignature<T>();
        jfieldID id = getCachedFieldID(env.jniEnv(), fieldName, signature);
        if (id) {
            setFieldForType<T>(env, object(), id, value);
            env.checkAndClearExceptions();
        }
    }

    template <typename T>
    void setField(const char *fieldName, const char *signature, T value)
    {
        assertJniType<T>();
        QJniEnvironment env;
        jfieldID id = getCachedFieldID(env.jniEnv(), fieldName, signature);
        if (id) {
            setFieldForType<T>(env, object(), id, value);
            env.checkAndClearExceptions();
        }
    }

    template <typename T>
    static void setStaticField(const char *className, const char *fieldName, T value)
    {
        assertJniType<T>();
        QJniEnvironment env;
        jclass clazz = QJniObject::loadClass(className, env.jniEnv());
        if (!clazz)
            return;

        constexpr const char *signature = getTypeSignature<T>();
        jfieldID id = getCachedFieldID(env.jniEnv(), clazz, className, fieldName,
                                       signature, true);
        if (!id)
            return;

        setStaticFieldForType<T>(env, clazz, id, value);
        env.checkAndClearExceptions();
    }

    template <typename T>
    static void setStaticField(const char *className, const char *fieldName,
                               const char *signature, T value)
    {
        assertJniType<T>();
        QJniEnvironment env;
        jclass clazz = QJniObject::loadClass(className, env.jniEnv());

        if (!clazz)
            return;

        jfieldID id = getCachedFieldID(env.jniEnv(), clazz, className, fieldName,
                                       signature, true);
        if (id) {
            setStaticFieldForType<T>(env, clazz, id, value);
            env.checkAndClearExceptions();
        }
    }

    template <typename T>
    static void setStaticField(jclass clazz, const char *fieldName,
                               const char *signature, T value)
    {
        assertJniType<T>();
        QJniEnvironment env;
        jfieldID id = getFieldID(env.jniEnv(), clazz, fieldName, signature, true);

        if (id) {
            setStaticFieldForType<T>(env, clazz, id, value);
            env.checkAndClearExceptions();
        }
    }

    template <typename T>
    static void setStaticField(jclass clazz, const char *fieldName, T value)
    {
        assertJniType<T>();
        QJniEnvironment env;
        constexpr const char *signature = getTypeSignature<T>();
        jfieldID id = getFieldID(env.jniEnv(), clazz, fieldName, signature, true);
        if (id) {
            setStaticFieldForType<T>(env, clazz, id, value);
            env.checkAndClearExceptions();
        }
    }

    static QJniObject fromString(const QString &string);
    QString toString() const;

    static bool isClassAvailable(const char *className);
    bool isValid() const;

    // This function takes ownership of the jobject and releases the local ref. before returning.
    static QJniObject fromLocalRef(jobject lref);

    template <typename T> QJniObject &operator=(T obj)
    {
        assertJniType<T>();
        assign(static_cast<T>(obj));
        return *this;
    }

private:
    struct QVaListPrivate { operator va_list &() const { return m_args; } va_list &m_args; };
    QJniObject(const char *className, const char *signature, const QVaListPrivate &args);
    QJniObject(jclass clazz, const char *signature, const QVaListPrivate &args);

    static jclass loadClass(const QByteArray &className, JNIEnv *env, bool binEncoded = false);
    static QByteArray toBinaryEncClassName(const QByteArray &className);
    static QJniObject getCleanJniObject(jobject obj);

    static jfieldID getCachedFieldID(JNIEnv *env, jclass clazz, const QByteArray &className,
                                     const char *name, const char *signature,
                                     bool isStatic = false);
    jfieldID getCachedFieldID(JNIEnv *env, const char *name, const char *signature,
                              bool isStatic = false) const;
    static jmethodID getCachedMethodID(JNIEnv *env, jclass clazz, const QByteArray &className,
                                       const char *name, const char *signature,
                                       bool isStatic = false);
    jmethodID getCachedMethodID(JNIEnv *env, const char *name, const char *signature,
                                bool isStatic = false) const;

    static jfieldID getFieldID(JNIEnv *env, jclass clazz, const char *name,
                               const char *signature, bool isStatic = false);
    static jmethodID getMethodID(JNIEnv *env, jclass clazz, const char *name,
                                 const char *signature, bool isStatic = false);

    void callVoidMethodV(JNIEnv *env, jmethodID id, va_list args) const;
    QJniObject callObjectMethodV(const char *methodName, const char *signature,
                                 va_list args) const;

    static QJniObject callStaticObjectMethodV(const char *className, const char *methodName,
                                              const char *signature, va_list args);

    static QJniObject callStaticObjectMethodV(jclass clazz, const char *methodName,
                                              const char *signature, va_list args);

    bool isSameObject(jobject obj) const;
    bool isSameObject(const QJniObject &other) const;
    void assign(jobject obj);
    jobject javaObject() const;

    friend bool operator==(const QJniObject &, const QJniObject &);
    friend bool operator!=(const QJniObject&, const QJniObject&);

    template<bool flag = false>
    static void staticAssertTypeMismatch()
    {
        static_assert(flag, "The used type is not supported by this template call. "
                            "Use a JNI based type instead.");
    }

    template<typename T>
    static constexpr bool isJniPrimitiveType()
    {
        if constexpr(!std::is_same<T, jboolean>::value
                && !std::is_same<T, jbyte>::value
                && !std::is_same<T, jchar>::value
                && !std::is_same<T, jshort>::value
                && !std::is_same<T, jint>::value
                && !std::is_same<T, jlong>::value
                && !std::is_same<T, jfloat>::value
                && !std::is_same<T, jdouble>::value) {
            return false;
        }

        return true;
    }

    template<typename T>
    static constexpr void assertJniPrimitiveType()
    {
        if constexpr(!isJniPrimitiveType<T>())
            staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr void assertJniObjectType()
    {
        if constexpr(!std::is_convertible<T, jobject>::value)
            staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr void assertJniType()
    {
        if constexpr(!isJniPrimitiveType<T>() && !std::is_convertible<T, jobject>::value)
            staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr const char* getTypeSignature()
    {
        if constexpr(std::is_same<T, jobject>::value)
            return "Ljava/lang/Object;";
        else if constexpr(std::is_same<T, jclass>::value)
            return "Ljava/lang/Class;";
        else if constexpr(std::is_same<T, jstring>::value)
            return "Ljava/lang/String;";
        else if constexpr(std::is_same<T, jobjectArray>::value)
            return "[Ljava/lang/Object;";
        else if constexpr(std::is_same<T, jthrowable>::value)
            return "Ljava/lang/Throwable;";
        else if constexpr(std::is_same<T, jbooleanArray>::value)
            return "[Z";
        else if constexpr(std::is_same<T, jbyteArray>::value)
            return "[B";
        else if constexpr(std::is_same<T, jshortArray>::value)
            return "[S";
        else if constexpr(std::is_same<T, jintArray>::value)
            return "[I";
        else if constexpr(std::is_same<T, jlongArray>::value)
            return "[J";
        else if constexpr(std::is_same<T, jfloatArray>::value)
            return "[F";
        else if constexpr(std::is_same<T, jdoubleArray>::value)
            return "[D";
        else if constexpr(std::is_same<T, jcharArray>::value)
            return "[C";
        else if constexpr(std::is_same<T, jboolean>::value)
            return "Z";
        else if constexpr(std::is_same<T, jbyte>::value)
            return "B";
        else if constexpr(std::is_same<T, jchar>::value)
            return "C";
        else if constexpr(std::is_same<T, jshort>::value)
            return "S";
        else if constexpr(std::is_same<T, jint>::value)
            return "I";
        else if constexpr(std::is_same<T, jlong>::value)
            return "J";
        else if constexpr(std::is_same<T, jfloat>::value)
            return "F";
        else if constexpr(std::is_same<T, jdouble>::value)
            return "D";
        else
            staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr void callMethodForType(JNIEnv *env, T &res, jobject obj,
                                            jmethodID id, va_list args)
    {
        if constexpr(std::is_same<T, jboolean>::value)
            res = env->CallBooleanMethodV(obj, id, args);
        else if constexpr(std::is_same<T, jbyte>::value)
            res = env->CallByteMethodV(obj, id, args);
        else if constexpr(std::is_same<T, jchar>::value)
            res = env->CallCharMethodV(obj, id, args);
        else if constexpr(std::is_same<T, jshort>::value)
            res = env->CallShortMethodV(obj, id, args);
        else if constexpr(std::is_same<T, jint>::value)
            res = env->CallIntMethodV(obj, id, args);
        else if constexpr(std::is_same<T, jlong>::value)
            res = env->CallLongMethodV(obj, id, args);
        else if constexpr(std::is_same<T, jfloat>::value)
            res = env->CallFloatMethodV(obj, id, args);
        else if constexpr(std::is_same<T, jdouble>::value)
            res = env->CallDoubleMethodV(obj, id, args);
        else
            staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr void callStaticMethodForType(JNIEnv *env, T &res, jclass clazz,
                                                  jmethodID id, va_list args)
    {
        if constexpr(std::is_same<T, jboolean>::value)
            res = env->CallStaticBooleanMethodV(clazz, id, args);
        else if constexpr(std::is_same<T, jbyte>::value)
            res = env->CallStaticByteMethodV(clazz, id, args);
        else if constexpr(std::is_same<T, jchar>::value)
            res = env->CallStaticCharMethodV(clazz, id, args);
        else if constexpr(std::is_same<T, jshort>::value)
            res = env->CallStaticShortMethodV(clazz, id, args);
        else if constexpr(std::is_same<T, jint>::value)
            res = env->CallStaticIntMethodV(clazz, id, args);
        else if constexpr(std::is_same<T, jlong>::value)
            res = env->CallStaticLongMethodV(clazz, id, args);
        else if constexpr(std::is_same<T, jfloat>::value)
            res = env->CallStaticFloatMethodV(clazz, id, args);
        else if constexpr(std::is_same<T, jdouble>::value)
            res = env->CallStaticDoubleMethodV(clazz, id, args);
        else
            staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr void getFieldForType(JNIEnv *env, T &res, jobject obj,
                                          jfieldID id)
    {
        if constexpr(std::is_same<T, jboolean>::value)
            res = env->GetBooleanField(obj, id);
        else if constexpr(std::is_same<T, jbyte>::value)
            res = env->GetByteField(obj, id);
        else if constexpr(std::is_same<T, jchar>::value)
            res = env->GetCharField(obj, id);
        else if constexpr(std::is_same<T, jshort>::value)
            res = env->GetShortField(obj, id);
        else if constexpr(std::is_same<T, jint>::value)
            res = env->GetIntField(obj, id);
        else if constexpr(std::is_same<T, jlong>::value)
            res = env->GetLongField(obj, id);
        else if constexpr(std::is_same<T, jfloat>::value)
            res = env->GetFloatField(obj, id);
        else if constexpr(std::is_same<T, jdouble>::value)
            res = env->GetDoubleField(obj, id);
        else
            staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr void getStaticFieldForType(JNIEnv *env, T &res, jclass clazz,
                                                jfieldID id)
    {
        if constexpr(std::is_same<T, jboolean>::value)
            res = env->GetStaticBooleanField(clazz, id);
        else if constexpr(std::is_same<T, jbyte>::value)
            res = env->GetStaticByteField(clazz, id);
        else if constexpr(std::is_same<T, jchar>::value)
            res = env->GetStaticCharField(clazz, id);
        else if constexpr(std::is_same<T, jshort>::value)
            res = env->GetStaticShortField(clazz, id);
        else if constexpr(std::is_same<T, jint>::value)
            res = env->GetStaticIntField(clazz, id);
        else if constexpr(std::is_same<T, jlong>::value)
            res = env->GetStaticLongField(clazz, id);
        else if constexpr(std::is_same<T, jfloat>::value)
            res = env->GetStaticFloatField(clazz, id);
        else if constexpr(std::is_same<T, jdouble>::value)
            res = env->GetStaticDoubleField(clazz, id);
        else
            staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr void setFieldForType(JNIEnv *env, jobject obj,
                                          jfieldID id, T value)
    {
        if constexpr(std::is_same<T, jboolean>::value)
            env->SetBooleanField(obj, id, value);
        else if constexpr(std::is_same<T, jbyte>::value)
            env->SetByteField(obj, id, value);
        else if constexpr(std::is_same<T, jchar>::value)
            env->SetCharField(obj, id, value);
        else if constexpr(std::is_same<T, jshort>::value)
            env->SetShortField(obj, id, value);
        else if constexpr(std::is_same<T, jint>::value)
            env->SetIntField(obj, id, value);
        else if constexpr(std::is_same<T, jlong>::value)
            env->SetLongField(obj, id, value);
        else if constexpr(std::is_same<T, jfloat>::value)
            env->SetFloatField(obj, id, value);
        else if constexpr(std::is_same<T, jdouble>::value)
            env->SetDoubleField(obj, id, value);
        else if constexpr(std::is_convertible<T, jobject>::value)
            env->SetObjectField(obj, id, value);
        else
            staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr void setStaticFieldForType(JNIEnv *env, jclass clazz,
                                          jfieldID id, T value)
    {
        if constexpr(std::is_same<T, jboolean>::value)
            env->SetStaticBooleanField(clazz, id, value);
        else if constexpr(std::is_same<T, jbyte>::value)
            env->SetStaticByteField(clazz, id, value);
        else if constexpr(std::is_same<T, jchar>::value)
            env->SetStaticCharField(clazz, id, value);
        else if constexpr(std::is_same<T, jshort>::value)
            env->SetStaticShortField(clazz, id, value);
        else if constexpr(std::is_same<T, jint>::value)
            env->SetStaticIntField(clazz, id, value);
        else if constexpr(std::is_same<T, jlong>::value)
            env->SetStaticLongField(clazz, id, value);
        else if constexpr(std::is_same<T, jfloat>::value)
            env->SetStaticFloatField(clazz, id, value);
        else if constexpr(std::is_same<T, jdouble>::value)
            env->SetStaticDoubleField(clazz, id, value);
        else if constexpr(std::is_convertible<T, jobject>::value)
            env->SetStaticObjectField(clazz, id, value);
        else
            staticAssertTypeMismatch();
    }

    friend QJniObjectPrivate;
    QSharedPointer<QJniObjectPrivate> d;
};

inline bool operator==(const QJniObject &obj1, const QJniObject &obj2)
{
    return obj1.isSameObject(obj2);
}

inline bool operator!=(const QJniObject &obj1, const QJniObject &obj2)
{
    return !obj1.isSameObject(obj2);
}

QT_END_NAMESPACE

#endif

#endif // QJNIOBJECT_H
