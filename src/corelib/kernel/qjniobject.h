// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QJNIOBJECT_H
#define QJNIOBJECT_H

#include <QtCore/qsharedpointer.h>

#if defined(Q_QDOC) || defined(Q_OS_ANDROID)
#include <jni.h>
#include <QtCore/qjnienvironment.h>

QT_BEGIN_NAMESPACE

class QJniObjectPrivate;

class Q_CORE_EXPORT QJniObject
{
    template <typename ...Args>
    struct LocalFrame {
        QJniEnvironment env;
        bool hasFrame = false;
        ~LocalFrame() {
            if (hasFrame)
                env->PopLocalFrame(nullptr);
        }
        template <typename T>
        auto newLocalRef(QJniObject &&object) {
            if (!hasFrame) {
                if (env->PushLocalFrame(sizeof...(Args)) < 0)
                    return T{}; // JVM is out of memory, avoid making matters worse
                hasFrame = true;
            }
            return static_cast<T>(env->NewLocalRef(object.template object<T>()));
        }
        JNIEnv *jniEnv() const { return env.jniEnv(); }
        bool checkAndClearExceptions() { return env.checkAndClearExceptions(); }

        template <typename T>
        auto convertToJni(T &&value);
        template <typename T>
        auto convertFromJni(QJniObject &&object);
    };
public:
    QJniObject();
    explicit QJniObject(const char *className);
    explicit QJniObject(const char *className, const char *signature, ...);
    template<typename ...Args
#ifndef Q_QDOC
        , std::enable_if_t<!std::disjunction_v<QtJniTypes::IsStringType<std::decay_t<Args>>...>>* = nullptr
#endif
        >
    explicit QJniObject(const char *className, Args &&...args)
        : QJniObject(Qt::Uninitialized)
    {
        LocalFrame<Args...> localFrame;
        *this = QJniObject(className, QtJniTypes::constructorSignature<Args...>().data(),
                           localFrame.convertToJni(std::forward<Args>(args))...);
    }
    explicit QJniObject(jclass clazz);
    explicit QJniObject(jclass clazz, const char *signature, ...);
    template<typename ...Args
#ifndef Q_QDOC
        , std::enable_if_t<!std::disjunction_v<QtJniTypes::IsStringType<std::decay_t<Args>>...>>* = nullptr
#endif
        >
    explicit QJniObject(jclass clazz, Args &&...args)
        : QJniObject(clazz, QtJniTypes::constructorSignature<Args...>().data(),
                     std::forward<Args>(args)...)
    {}
    QJniObject(jobject globalRef);

    QJniObject(const QJniObject &other) noexcept = default;
    QJniObject(QJniObject &&other) noexcept = default;
    QJniObject &operator=(const QJniObject &other) noexcept = default;
    QJniObject &operator=(QJniObject &&other) noexcept = default;

    ~QJniObject();

    template<typename Class, typename ...Args>
    static inline QJniObject construct(Args &&...args)
    {
        LocalFrame<Args...> frame;
        return QJniObject(QtJniTypes::Traits<Class>::className().data(),
                          QtJniTypes::constructorSignature<Args...>().data(),
                          frame.convertToJni(std::forward<Args>(args))...);
    }

    jobject object() const;
    template <typename T> T object() const
    {
        QtJniTypes::assertObjectType<T>();
        return static_cast<T>(javaObject());
    }

    jclass objectClass() const;
    QByteArray className() const;

    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<Ret> = true
#endif
    >
    auto callMethod(const char *methodName, const char *signature, Args &&...args) const
    {
        LocalFrame<Args...> frame;
        if constexpr (QtJniTypes::isObjectType<Ret>()) {
            return frame.template convertFromJni<Ret>(callObjectMethod(methodName, signature,
                                                frame.convertToJni(std::forward<Args>(args))...));
        } else {
            jmethodID id = getCachedMethodID(frame.jniEnv(), methodName, signature);
            if (id) {
                if constexpr (std::is_same_v<Ret, void>) {
                    callVoidMethodV(frame.jniEnv(), id,
                                    frame.convertToJni(std::forward<Args>(args))...);
                    frame.checkAndClearExceptions();
                } else {
                    Ret res{};
                    callMethodForType<Ret>(frame.jniEnv(), res, object(), id,
                                           frame.convertToJni(std::forward<Args>(args))...);
                    if (frame.checkAndClearExceptions())
                        res = {};
                    return res;
                }
            }
            if constexpr (!std::is_same_v<Ret, void>)
                return Ret{};
        }
    }

    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::ValidSignatureTypes<Ret, Args...> = true
#endif
    >
    auto callMethod(const char *methodName, Args &&...args) const
    {
        constexpr auto signature = QtJniTypes::methodSignature<Ret, Args...>();
        return callMethod<Ret>(methodName, signature.data(), std::forward<Args>(args)...);
    }

    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::ValidSignatureTypes<Ret, Args...> = true
#endif
    >
    QJniObject callObjectMethod(const char *methodName, Args &&...args) const
    {
        QtJniTypes::assertObjectType<Ret>();
        constexpr auto signature = QtJniTypes::methodSignature<Ret, Args...>();
        LocalFrame<Args...> frame;
        return frame.template convertFromJni<Ret>(callObjectMethod(methodName, signature,
                                            frame.convertToJni(std::forward<Args>(args))...));
    }

    QJniObject callObjectMethod(const char *methodName, const char *signature, ...) const;

    template <typename Ret, typename ...Args>
    static auto callStaticMethod(const char *className, const char *methodName, const char *signature, Args &&...args)
    {
        QJniEnvironment env;
        jclass clazz = QJniObject::loadClass(className, env.jniEnv());
        return callStaticMethod<Ret>(clazz, methodName, signature, std::forward<Args>(args)...);
    }

    template <typename Ret, typename ...Args>
    static auto callStaticMethod(jclass clazz, const char *methodName, const char *signature, Args &&...args)
    {
        QJniEnvironment env;
        jmethodID id = clazz ? getMethodID(env.jniEnv(), clazz, methodName, signature, true)
                             : 0;
        return callStaticMethod<Ret>(clazz, id, std::forward<Args>(args)...);
    }

    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<Ret> = true
#endif
    >
    static auto callStaticMethod(jclass clazz, jmethodID methodId, Args &&...args)
    {
        LocalFrame<Args...> frame;
        if constexpr (QtJniTypes::isObjectType<Ret>()) {
            return frame.template convertFromJni<Ret>(callStaticObjectMethod(clazz, methodId,
                                                frame.convertToJni(std::forward<Args>(args))...));
        } else {
            if (clazz && methodId) {
                if constexpr (std::is_same_v<Ret, void>) {
                    callStaticMethodForVoid(frame.jniEnv(), clazz, methodId,
                                            frame.convertToJni(std::forward<Args>(args))...);
                    frame.checkAndClearExceptions();
                } else {
                    Ret res{};
                    callStaticMethodForType<Ret>(frame.jniEnv(), res, clazz, methodId,
                                                 frame.convertToJni(std::forward<Args>(args))...);
                    if (frame.checkAndClearExceptions())
                        res = {};
                    return res;
                }
            }
            if constexpr (!std::is_same_v<Ret, void>)
                return Ret{};
        }
    }

    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::ValidSignatureTypes<Ret, Args...> = true
#endif
    >
    static auto callStaticMethod(const char *className, const char *methodName, Args &&...args)
    {
        QJniEnvironment env;
        jclass clazz = QJniObject::loadClass(className, env.jniEnv());
        return callStaticMethod<Ret>(clazz, methodName, std::forward<Args>(args)...);
    }

    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::ValidSignatureTypes<Ret, Args...> = true
#endif
    >
    static auto callStaticMethod(jclass clazz, const char *methodName, Args &&...args)
    {
        constexpr auto signature = QtJniTypes::methodSignature<Ret, Args...>();
        return callStaticMethod<Ret>(clazz, methodName, signature.data(), std::forward<Args>(args)...);
    }
    template <typename Klass, typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::ValidSignatureTypes<Ret, Args...> = true
#endif
    >
    static auto callStaticMethod(const char *methodName, Args &&...args)
    {
        QJniEnvironment env;
        const jclass clazz = QJniObject::loadClass(QtJniTypes::Traits<Klass>::className().data(),
                                                   env.jniEnv());
        const jmethodID id = clazz ? getMethodID(env.jniEnv(), clazz, methodName,
                                         QtJniTypes::methodSignature<Ret, Args...>().data(), true)
                                   : 0;
        return callStaticMethod<Ret>(clazz, id, std::forward<Args>(args)...);
    }

    static QJniObject callStaticObjectMethod(const char *className, const char *methodName,
                                             const char *signature, ...);

    static QJniObject callStaticObjectMethod(jclass clazz, const char *methodName,
                                             const char *signature, ...);

    static QJniObject callStaticObjectMethod(jclass clazz, jmethodID methodId, ...);


    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::ValidSignatureTypes<Ret, Args...> = true
#endif
    >
    static QJniObject callStaticObjectMethod(const char *className, const char *methodName, Args &&...args)
    {
        QtJniTypes::assertObjectType<Ret>();
        constexpr auto signature = QtJniTypes::methodSignature<Ret, Args...>();
        LocalFrame<Args...> frame;
        return frame.template convertFromJni<Ret>(callStaticObjectMethod(className, methodName, signature.data(),
                                            frame.convertToJni(std::forward<Args>(args))...));
    }

    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::ValidSignatureTypes<Ret, Args...> = true
#endif
    >
    static QJniObject callStaticObjectMethod(jclass clazz, const char *methodName, Args &&...args)
    {
        QtJniTypes::assertObjectType<Ret>();
        constexpr auto signature = QtJniTypes::methodSignature<Ret, Args...>();
        LocalFrame<Args...> frame;
        return frame.template convertFromJni<Ret>(callStaticObjectMethod(clazz, methodName, signature.data(),
                                            frame.convertToJni(std::forward<Args>(args))...));
    }

    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    auto getField(const char *fieldName) const
    {
        LocalFrame<T> frame;
        if constexpr (QtJniTypes::isObjectType<T>()) {
            return frame.template convertFromJni<T>(getObjectField<T>(fieldName));
        } else {
            T res{};
            constexpr auto signature = QtJniTypes::fieldSignature<T>();
            jfieldID id = getCachedFieldID(frame.jniEnv(), fieldName, signature);
            if (id) {
                getFieldForType<T>(frame.jniEnv(), res, object(), id);
                if (frame.checkAndClearExceptions())
                    res = {};
            }
            return res;
        }
    }

    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    static auto getStaticField(const char *className, const char *fieldName)
    {
        LocalFrame<T> frame;
        if constexpr (QtJniTypes::isObjectType<T>()) {
            return frame.template convertFromJni<T>(getStaticObjectField<T>(className, fieldName));
        } else {
            jclass clazz = QJniObject::loadClass(className, frame.jniEnv());
            if (!clazz)
                return T{};
            return getStaticField<T>(clazz, fieldName);
        }
    }

    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    static auto getStaticField(jclass clazz, const char *fieldName)
    {
        LocalFrame<T> frame;
        if constexpr (QtJniTypes::isObjectType<T>()) {
            return frame.template convertFromJni<T>(getStaticObjectField<T>(clazz, fieldName));
        } else {
            T res{};
            constexpr auto signature = QtJniTypes::fieldSignature<T>();
            jfieldID id = getFieldID(frame.jniEnv(), clazz, fieldName, signature, true);
            if (id) {
                getStaticFieldForType<T>(frame.jniEnv(), res, clazz, id);
                if (frame.checkAndClearExceptions())
                    res = {};
            }
            return res;
        }
    }

    template <typename Klass, typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    static auto getStaticField(const char *fieldName)
    {
        return getStaticField<T>(QtJniTypes::Traits<Klass>::className(), fieldName);
    }

    template <typename T
#ifndef Q_QDOC
        , std::enable_if_t<QtJniTypes::isObjectType<T>(), bool> = true
#endif
    >
    QJniObject getObjectField(const char *fieldName) const
    {
        constexpr auto signature = QtJniTypes::fieldSignature<T>();
        return getObjectField(fieldName, signature);
    }

    QJniObject getObjectField(const char *fieldName, const char *signature) const;

    template <typename T
#ifndef Q_QDOC
        , std::enable_if_t<QtJniTypes::isObjectType<T>(), bool> = true
#endif
    >
    static QJniObject getStaticObjectField(const char *className, const char *fieldName)
    {
        constexpr auto signature = QtJniTypes::fieldSignature<T>();
        return getStaticObjectField(className, fieldName, signature);
    }

    static QJniObject getStaticObjectField(const char *className,
                                           const char *fieldName,
                                           const char *signature);

    template <typename T
#ifndef Q_QDOC
        , std::enable_if_t<QtJniTypes::isObjectType<T>(), bool> = true
#endif
    >
    static QJniObject getStaticObjectField(jclass clazz, const char *fieldName)
    {
        constexpr auto signature = QtJniTypes::fieldSignature<T>();
        return getStaticObjectField(clazz, fieldName, signature);
    }

    static QJniObject getStaticObjectField(jclass clazz, const char *fieldName,
                                           const char *signature);

    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    void setField(const char *fieldName, T value)
    {
        QJniEnvironment env;
        constexpr auto signature = QtJniTypes::fieldSignature<T>();
        jfieldID id = getCachedFieldID(env.jniEnv(), fieldName, signature);
        if (id) {
            setFieldForType<T>(env.jniEnv(), object(), id, value);
            env.checkAndClearExceptions();
        }
    }

    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    void setField(const char *fieldName, const char *signature, T value)
    {
        QJniEnvironment env;
        jfieldID id = getCachedFieldID(env.jniEnv(), fieldName, signature);
        if (id) {
            setFieldForType<T>(env.jniEnv(), object(), id, value);
            env.checkAndClearExceptions();
        }
    }

    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    static void setStaticField(const char *className, const char *fieldName, T value)
    {
        LocalFrame<T> frame;
        jclass clazz = QJniObject::loadClass(className, frame.jniEnv());
        if (!clazz)
            return;

        constexpr auto signature = QtJniTypes::fieldSignature<T>();
        jfieldID id = getCachedFieldID(frame.jniEnv(), clazz, className, fieldName,
                                       signature, true);
        if (!id)
            return;

        setStaticFieldForType<T>(frame.jniEnv(), clazz, id, value);
        frame.checkAndClearExceptions();
    }

    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    static void setStaticField(const char *className, const char *fieldName,
                               const char *signature, T value)
    {
        QJniEnvironment env;
        jclass clazz = QJniObject::loadClass(className, env.jniEnv());

        if (!clazz)
            return;

        jfieldID id = getCachedFieldID(env.jniEnv(), clazz, className, fieldName,
                                       signature, true);
        if (id) {
            setStaticFieldForType<T>(env.jniEnv(), clazz, id, value);
            env.checkAndClearExceptions();
        }
    }

    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    static void setStaticField(jclass clazz, const char *fieldName,
                               const char *signature, T value)
    {
        QJniEnvironment env;
        jfieldID id = getFieldID(env.jniEnv(), clazz, fieldName, signature, true);

        if (id) {
            setStaticFieldForType<T>(env.jniEnv(), clazz, id, value);
            env.checkAndClearExceptions();
        }
    }

    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    static void setStaticField(jclass clazz, const char *fieldName, T value)
    {
        setStaticField(clazz, fieldName, QtJniTypes::fieldSignature<T>(), value);
    }

    template <typename Klass, typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    static void setStaticField(const char *fieldName, T value)
    {
        setStaticField(QtJniTypes::Traits<Klass>::className(), fieldName, value);
    }

    static QJniObject fromString(const QString &string);
    QString toString() const;

    static bool isClassAvailable(const char *className);
    bool isValid() const;

    // This function takes ownership of the jobject and releases the local ref. before returning.
    static QJniObject fromLocalRef(jobject lref);

    template <typename T>
    QJniObject &operator=(T obj)
    {
        assign(static_cast<T>(obj));
        return *this;
    }

protected:
    QJniObject(Qt::Initialization) {}

private:
    static jclass loadClass(const QByteArray &className, JNIEnv *env);

#if QT_CORE_REMOVED_SINCE(6, 7)
    // these need to stay in the ABI as they were used in inline methods before 6.7
    static jclass loadClass(const QByteArray &className, JNIEnv *env, bool binEncoded);
    static QByteArray toBinaryEncClassName(const QByteArray &className);
    void callVoidMethodV(JNIEnv *env, jmethodID id, va_list args) const;
#endif

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

    void callVoidMethodV(JNIEnv *env, jmethodID id, ...) const;

    bool isSameObject(jobject obj) const;
    bool isSameObject(const QJniObject &other) const;
    void assign(jobject obj);
    jobject javaObject() const;

    friend bool operator==(const QJniObject &, const QJniObject &);
    friend bool operator!=(const QJniObject&, const QJniObject&);

    template <typename Have, typename Want>
    static constexpr bool sameTypeForJni = (QtJniTypes::Traits<Have>::signature()
                                           == QtJniTypes::Traits<Want>::signature())
                                       && (sizeof(Have) == sizeof(Want));

    template<typename T>
    static constexpr void callMethodForType(JNIEnv *env, T &res, jobject obj,
                                            jmethodID id, ...)
    {
        va_list args = {};
        va_start(args, id);

        if constexpr (sameTypeForJni<T, jboolean>)
            res = T(env->CallBooleanMethodV(obj, id, args));
        else if constexpr (sameTypeForJni<T, jbyte>)
            res = T(env->CallByteMethodV(obj, id, args));
        else if constexpr (sameTypeForJni<T, jchar>)
            res = T(env->CallCharMethodV(obj, id, args));
        else if constexpr (sameTypeForJni<T, jshort>)
            res = T(env->CallShortMethodV(obj, id, args));
        else if constexpr (sameTypeForJni<T, jint>)
            res = T(env->CallIntMethodV(obj, id, args));
        else if constexpr (sameTypeForJni<T, jlong>)
            res = T(env->CallLongMethodV(obj, id, args));
        else if constexpr (sameTypeForJni<T, jfloat>)
            res = T(env->CallFloatMethodV(obj, id, args));
        else if constexpr (sameTypeForJni<T, jdouble>)
            res = T(env->CallDoubleMethodV(obj, id, args));
        else
            QtJniTypes::staticAssertTypeMismatch();
        va_end(args);
    }

    template<typename T>
    static constexpr void callStaticMethodForType(JNIEnv *env, T &res, jclass clazz,
                                                  jmethodID id, ...)
    {
        if (!clazz || !id)
            return;
        va_list args = {};
        va_start(args, id);
        if constexpr (sameTypeForJni<T, jboolean>)
            res = T(env->CallStaticBooleanMethodV(clazz, id, args));
        else if constexpr (sameTypeForJni<T, jbyte>)
            res = T(env->CallStaticByteMethodV(clazz, id, args));
        else if constexpr (sameTypeForJni<T, jchar>)
            res = T(env->CallStaticCharMethodV(clazz, id, args));
        else if constexpr (sameTypeForJni<T, jshort>)
            res = T(env->CallStaticShortMethodV(clazz, id, args));
        else if constexpr (sameTypeForJni<T, jint>)
            res = T(env->CallStaticIntMethodV(clazz, id, args));
        else if constexpr (sameTypeForJni<T, jlong>)
            res = T(env->CallStaticLongMethodV(clazz, id, args));
        else if constexpr (sameTypeForJni<T, jfloat>)
            res = T(env->CallStaticFloatMethodV(clazz, id, args));
        else if constexpr (sameTypeForJni<T, jdouble>)
            res = T(env->CallStaticDoubleMethodV(clazz, id, args));
        else
            QtJniTypes::staticAssertTypeMismatch();
        va_end(args);
    }

    static void callStaticMethodForVoid(JNIEnv *env, jclass clazz, jmethodID id, ...)
    {
        if (!clazz || !id)
            return;
        va_list args;
        va_start(args, id);
        env->CallStaticVoidMethodV(clazz, id, args);
        va_end(args);
    }


    template<typename T>
    static constexpr void getFieldForType(JNIEnv *env, T &res, jobject obj,
                                          jfieldID id)
    {
        if constexpr (sameTypeForJni<T, jboolean>)
            res = T(env->GetBooleanField(obj, id));
        else if constexpr (sameTypeForJni<T, jbyte>)
            res = T(env->GetByteField(obj, id));
        else if constexpr (sameTypeForJni<T, jchar>)
            res = T(env->GetCharField(obj, id));
        else if constexpr (sameTypeForJni<T, jshort>)
            res = T(env->GetShortField(obj, id));
        else if constexpr (sameTypeForJni<T, jint>)
            res = T(env->GetIntField(obj, id));
        else if constexpr (sameTypeForJni<T, jlong>)
            res = T(env->GetLongField(obj, id));
        else if constexpr (sameTypeForJni<T, jfloat>)
            res = T(env->GetFloatField(obj, id));
        else if constexpr (sameTypeForJni<T, jdouble>)
            res = T(env->GetDoubleField(obj, id));
        else
            QtJniTypes::staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr void getStaticFieldForType(JNIEnv *env, T &res, jclass clazz,
                                                jfieldID id)
    {
        if constexpr (sameTypeForJni<T, jboolean>)
            res = T(env->GetStaticBooleanField(clazz, id));
        else if constexpr (sameTypeForJni<T, jbyte>)
            res = T(env->GetStaticByteField(clazz, id));
        else if constexpr (sameTypeForJni<T, jchar>)
            res = T(env->GetStaticCharField(clazz, id));
        else if constexpr (sameTypeForJni<T, jshort>)
            res = T(env->GetStaticShortField(clazz, id));
        else if constexpr (sameTypeForJni<T, jint>)
            res = T(env->GetStaticIntField(clazz, id));
        else if constexpr (sameTypeForJni<T, jlong>)
            res = T(env->GetStaticLongField(clazz, id));
        else if constexpr (sameTypeForJni<T, jfloat>)
            res = T(env->GetStaticFloatField(clazz, id));
        else if constexpr (sameTypeForJni<T, jdouble>)
            res = T(env->GetStaticDoubleField(clazz, id));
        else
            QtJniTypes::staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr void setFieldForType(JNIEnv *env, jobject obj,
                                          jfieldID id, T value)
    {
        LocalFrame<T> frame;
        if constexpr (sameTypeForJni<T, jboolean>)
            env->SetBooleanField(obj, id, static_cast<jboolean>(value));
        else if constexpr (sameTypeForJni<T, jbyte>)
            env->SetByteField(obj, id, static_cast<jbyte>(value));
        else if constexpr (sameTypeForJni<T, jchar>)
            env->SetCharField(obj, id, static_cast<jchar>(value));
        else if constexpr (sameTypeForJni<T, jshort>)
            env->SetShortField(obj, id, static_cast<jshort>(value));
        else if constexpr (sameTypeForJni<T, jint>)
            env->SetIntField(obj, id, static_cast<jint>(value));
        else if constexpr (sameTypeForJni<T, jlong>)
            env->SetLongField(obj, id, static_cast<jlong>(value));
        else if constexpr (sameTypeForJni<T, jfloat>)
            env->SetFloatField(obj, id, static_cast<jfloat>(value));
        else if constexpr (sameTypeForJni<T, jdouble>)
            env->SetDoubleField(obj, id, static_cast<jdouble>(value));
        else if constexpr (QtJniTypes::isObjectType<T>())
            env->SetObjectField(obj, id, static_cast<jobject>(frame.convertToJni(value)));
        else
            QtJniTypes::staticAssertTypeMismatch();
    }

    template<typename T>
    static constexpr void setStaticFieldForType(JNIEnv *env, jclass clazz,
                                          jfieldID id, T value)
    {
        LocalFrame<T> frame;
        if constexpr (sameTypeForJni<T, jboolean>)
            env->SetStaticBooleanField(clazz, id, static_cast<jboolean>(value));
        else if constexpr (sameTypeForJni<T, jbyte>)
            env->SetStaticByteField(clazz, id, static_cast<jbyte>(value));
        else if constexpr (sameTypeForJni<T, jchar>)
            env->SetStaticCharField(clazz, id, static_cast<jchar>(value));
        else if constexpr (sameTypeForJni<T, jshort>)
            env->SetStaticShortField(clazz, id, static_cast<jshort>(value));
        else if constexpr (sameTypeForJni<T, jint>)
            env->SetStaticIntField(clazz, id, static_cast<jint>(value));
        else if constexpr (sameTypeForJni<T, jlong>)
            env->SetStaticLongField(clazz, id, static_cast<jlong>(value));
        else if constexpr (sameTypeForJni<T, jfloat>)
            env->SetStaticFloatField(clazz, id, static_cast<jfloat>(value));
        else if constexpr (sameTypeForJni<T, jdouble>)
            env->SetStaticDoubleField(clazz, id, static_cast<jdouble>(value));
        else if constexpr (QtJniTypes::isObjectType<T>())
            env->SetStaticObjectField(clazz, id, static_cast<jobject>(frame.convertToJni(value)));
        else
            QtJniTypes::staticAssertTypeMismatch();
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

// This cannot be included earlier as QJniArray is a QJniObject subclass, but it
// must be included so that we can implement QJniObject::LocalFrame conversion.
#include <QtCore/qjniarray.h>

template <typename ...Args>
template <typename T>
auto QJniObject::LocalFrame<Args...>::convertToJni(T &&value)
{
    using Type = q20::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Type, QString>) {
        return newLocalRef<jstring>(QJniObject::fromString(value));
    } else if constexpr (QtJniTypes::IsJniArray<Type>::value) {
        return value.arrayObject();
    } else if constexpr (QJniArrayBase::CanConvert<T>) {
        using QJniArrayType = decltype(QJniArrayBase::fromContainer(std::forward<T>(value)));
        using ArrayType = decltype(std::declval<QJniArrayType>().arrayObject());
        return newLocalRef<ArrayType>(QJniArrayBase::fromContainer(std::forward<T>(value)).template object<jobject>());
    } else if constexpr (std::is_base_of_v<QJniObject, Type>) {
        return value.object();
    } else {
        return static_cast<T &&>(value);
    }
}

template <typename ...Args>
template <typename T>
auto QJniObject::LocalFrame<Args...>::convertFromJni(QJniObject &&object)
{
    using Type = q20::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Type, QString>) {
        return object.toString();
    } else if constexpr (QtJniTypes::IsJniArray<Type>::value) {
        return T{object};
    } else if constexpr (QJniArrayBase::CanConvert<Type>) {
        // if we were to create a QJniArray from Type...
        using QJniArrayType = decltype(QJniArrayBase::fromContainer(std::declval<Type>()));
        // then that QJniArray would have elements of type
        using ElementType = typename QJniArrayType::Type;
        // construct a QJniArray from a jobject pointer of that type
        return QJniArray<ElementType>(object.template object<jarray>()).asContainer();
    } else if constexpr (std::is_array_v<Type>) {
        using ElementType = std::remove_extent_t<Type>;
        return QJniArray<ElementType>{object};
    } else if constexpr (std::is_base_of_v<QJniObject, Type>
                        && !std::is_same_v<QJniObject, Type>) {
        return T{std::move(object)};
    } else {
        return std::move(object);
    }
}


QT_END_NAMESPACE

#endif

#endif // QJNIOBJECT_H
