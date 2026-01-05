// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QJNIOBJECT_H
#define QJNIOBJECT_H

#include <QtCore/qsharedpointer.h>

#if defined(Q_QDOC) || defined(Q_OS_ANDROID)
#include <jni.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qxptype_traits.h>

QT_BEGIN_NAMESPACE

class QJniObjectPrivate;
class QJniObject;

namespace QtJniTypes
{
namespace Detail
{
// any type with an "jobject object()" member function stores a global reference
template <typename T, typename = void> struct StoresGlobalRefTest : std::false_type {};
template <typename T>
struct StoresGlobalRefTest<T, std::void_t<decltype(std::declval<T>().object())>>
    : std::is_same<decltype(std::declval<T>().object()), jobject>
{};

// detect if a type is std::expected-like
template <typename R, typename = void>
struct CallerHandlesException : std::false_type {
    using value_type = R;
};
template <typename R>
struct CallerHandlesException<R, std::void_t<typename R::unexpected_type,
                                                typename R::value_type,
                                                typename R::error_type>> : std::true_type
{
    using value_type = typename R::value_type;
};

template <typename ReturnType>
static constexpr bool callerHandlesException = CallerHandlesException<ReturnType>::value;

template <typename ...Args>
struct LocalFrame
{
    mutable JNIEnv *env;
    bool hasFrame = false;

    explicit LocalFrame(JNIEnv *env = nullptr) noexcept
        : env(env)
    {
    }
    ~LocalFrame()
    {
        if (hasFrame)
            env->PopLocalFrame(nullptr);
    }
    bool ensureFrame()
    {
        if (!hasFrame)
            hasFrame = jniEnv()->PushLocalFrame(sizeof...(Args)) == 0;
        return hasFrame;
    }
    JNIEnv *jniEnv() const
    {
        if (!env)
            env = QJniEnvironment::getJniEnv();
        return env;
    }

    template <typename T>
    auto convertToJni(T &&value)
    {
        using Type = q20::remove_cvref_t<T>;
        using ResultType = decltype(QtJniTypes::Traits<Type>::convertToJni(jniEnv(),
                                                                           std::declval<T&&>()));
        if constexpr (std::is_base_of_v<std::remove_pointer_t<jobject>,
                                        std::remove_pointer_t<ResultType>>) {
            // Make sure the local frame is engaged if we create a jobject, unless
            // we know that the value stores a global reference that it returns.
            if constexpr (!qxp::is_detected_v<StoresGlobalRefTest, Type>) {
                if (!ensureFrame())
                    return ResultType{};
            }
        }
        return QtJniTypes::Traits<Type>::convertToJni(jniEnv(), std::forward<T>(value));
    }
    template <typename T>
    auto convertFromJni(QJniObject &&object)
    {
        using Type = q20::remove_cvref_t<T>;
        return QtJniTypes::Traits<Type>::convertFromJni(std::move(object));
    }
};

template <typename Ret, typename ...Args>
struct LocalFrameWithReturn : LocalFrame<Args...>
{
    using ReturnType = Ret;

    using LocalFrame<Args...>::LocalFrame;

    template <typename T>
    auto convertFromJni(QJniObject &&object)
    {
        return LocalFrame<Args...>::template convertFromJni<T>(std::move(object));
    }

    template <typename T>
    auto convertFromJni(jobject object);

    bool checkAndClearExceptions() const
    {
        if constexpr (callerHandlesException<ReturnType>)
            return false;
        else
            return QJniEnvironment::checkAndClearExceptions(this->jniEnv());
    }

    auto makeResult()
    {
        if constexpr (callerHandlesException<ReturnType>) {
            JNIEnv *env = this->jniEnv();
            if (env->ExceptionCheck()) {
                jthrowable exception = env->ExceptionOccurred();
                env->ExceptionClear();
                return ReturnType(typename ReturnType::unexpected_type(exception));
            }
            return ReturnType();
        } else {
            checkAndClearExceptions();
        }
    }

    template <typename Value>
    auto makeResult(Value &&value)
    {
        if constexpr (callerHandlesException<ReturnType>) {
            auto maybeValue = makeResult();
            if (maybeValue)
                return ReturnType(std::forward<Value>(value));
            return std::move(maybeValue);
        } else {
            checkAndClearExceptions();
            return std::forward<Value>(value);
        }
    }
};
}
}

class Q_CORE_EXPORT QJniObject
{
    template <typename Ret, typename ...Args> using LocalFrame
        = QtJniTypes::Detail::LocalFrameWithReturn<Ret, Args...>;

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
        : QJniObject(QtJniTypes::Detail::LocalFrame<Args...>{}, className, std::forward<Args>(args)...)
    {
    }
private:
    template<typename ...Args>
    explicit QJniObject(QtJniTypes::Detail::LocalFrame<Args...> localFrame, const char *className, Args &&...args)
        : QJniObject(className, QtJniTypes::constructorSignature<Args...>().data(),
                     localFrame.convertToJni(std::forward<Args>(args))...)
    {
    }
public:
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

    void swap(QJniObject &other) noexcept { d.swap(other.d); }

    template<typename Class, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidSignatureTypes<Class, Args...> = true
#endif
    >
    static inline auto construct(Args &&...args)
    {
        LocalFrame<Class, Args...> frame;
        jclass clazz = QJniObject::loadClassKeepExceptions(QtJniTypes::Traits<Class>::className().data(),
                                                           frame.jniEnv());
        auto res = clazz
                 ? QJniObject(clazz, QtJniTypes::constructorSignature<Args...>().data(),
                              frame.convertToJni(std::forward<Args>(args))...)
                 : QtJniTypes::Detail::callerHandlesException<Class>
                 ? QJniObject(Qt::Initialization::Uninitialized)
                 : QJniObject();
        return frame.makeResult(std::move(res));
    }

    jobject object() const;
    template <typename T> T object() const
    {
        QtJniTypes::assertObjectType<T>();
        return static_cast<T>(javaObject());
    }

    jclass objectClass() const;
    QByteArray className() const;

    template <typename ReturnType = void, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<ReturnType> = true
#endif
    >
    auto callMethod(const char *methodName, const char *signature, Args &&...args) const
    {
        using Ret = typename QtJniTypes::Detail::CallerHandlesException<ReturnType>::value_type;
        LocalFrame<ReturnType, Args...> frame(jniEnv());
        jmethodID id = getCachedMethodID(frame.jniEnv(), methodName, signature);

        if constexpr (QtJniTypes::isObjectType<Ret>()) {
            return frame.makeResult(frame.template convertFromJni<Ret>(callObjectMethodImpl(
                                        id, frame.convertToJni(std::forward<Args>(args))...))
                                    );
        } else {
            if (id) {
                if constexpr (std::is_same_v<Ret, void>) {
                    callVoidMethodV(frame.jniEnv(), id,
                                    frame.convertToJni(std::forward<Args>(args))...);
                } else {
                    Ret res{};
                    callMethodForType<Ret>(frame.jniEnv(), res, object(), id,
                                           frame.convertToJni(std::forward<Args>(args))...);
                    return frame.makeResult(res);
                }
            }
            if constexpr (!std::is_same_v<Ret, void>)
                return frame.makeResult(Ret{});
            else
                return frame.makeResult();
        }
    }

    template <typename ReturnType = void, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidSignatureTypes<ReturnType, Args...> = true
#endif
    >
    auto callMethod(const char *methodName, Args &&...args) const
    {
        constexpr auto signature = QtJniTypes::methodSignature<ReturnType, Args...>();
        return callMethod<ReturnType>(methodName, signature.data(), std::forward<Args>(args)...);
    }

    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidSignatureTypes<Ret, Args...> = true
#endif
    >
    QJniObject callObjectMethod(const char *methodName, Args &&...args) const
    {
        QtJniTypes::assertObjectType<Ret>();
        constexpr auto signature = QtJniTypes::methodSignature<Ret, Args...>();
        LocalFrame<Ret, Args...> frame(jniEnv());
        auto object = frame.template convertFromJni<Ret>(callObjectMethod(methodName, signature,
                                            frame.convertToJni(std::forward<Args>(args))...));
        frame.checkAndClearExceptions();
        return object;
    }

    QJniObject callObjectMethod(const char *methodName, const char *signature, ...) const;

    template <typename Ret = void, typename ...Args>
    static auto callStaticMethod(const char *className, const char *methodName, const char *signature, Args &&...args)
    {
        LocalFrame<Ret, Args...> frame;
        jclass clazz = QJniObject::loadClass(className, frame.jniEnv());
        return callStaticMethod<Ret>(clazz, methodName, signature, std::forward<Args>(args)...);
    }

    template <typename Ret = void, typename ...Args>
    static auto callStaticMethod(jclass clazz, const char *methodName, const char *signature, Args &&...args)
    {
        LocalFrame<Ret, Args...> frame;
        jmethodID id = clazz ? getMethodID(frame.jniEnv(), clazz, methodName, signature, true)
                             : 0;
        return callStaticMethod<Ret>(clazz, id, std::forward<Args>(args)...);
    }

    template <typename ReturnType = void, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<ReturnType> = true
#endif
    >
    static auto callStaticMethod(jclass clazz, jmethodID methodId, Args &&...args)
    {
        using Ret = typename QtJniTypes::Detail::CallerHandlesException<ReturnType>::value_type;
        LocalFrame<ReturnType, Args...> frame;
        if constexpr (QtJniTypes::isObjectType<Ret>()) {
            return frame.makeResult(frame.template convertFromJni<Ret>(callStaticObjectMethod(
                                        clazz, methodId,
                                        frame.convertToJni(std::forward<Args>(args))...))
                                    );
        } else {
            if (clazz && methodId) {
                if constexpr (std::is_same_v<Ret, void>) {
                    callStaticMethodForVoid(frame.jniEnv(), clazz, methodId,
                                            frame.convertToJni(std::forward<Args>(args))...);
                } else {
                    Ret res{};
                    callStaticMethodForType<Ret>(frame.jniEnv(), res, clazz, methodId,
                                                 frame.convertToJni(std::forward<Args>(args))...);
                    return frame.makeResult(res);
                }
            }
            if constexpr (!std::is_same_v<Ret, void>)
                return frame.makeResult(Ret{});
            else
                return frame.makeResult();
        }
    }

    template <typename ReturnType = void, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidSignatureTypes<ReturnType, Args...> = true
#endif
    >
    static auto callStaticMethod(const char *className, const char *methodName, Args &&...args)
    {
        using Ret = typename QtJniTypes::Detail::CallerHandlesException<ReturnType>::value_type;
        LocalFrame<Ret, Args...> frame;
        jclass clazz = QJniObject::loadClass(className, frame.jniEnv());
        const jmethodID id = clazz ? getMethodID(frame.jniEnv(), clazz, methodName,
                                         QtJniTypes::methodSignature<Ret, Args...>().data(), true)
                                   : 0;
        return callStaticMethod<ReturnType>(clazz, id, std::forward<Args>(args)...);
    }

    template <typename ReturnType = void, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidSignatureTypes<ReturnType, Args...> = true
#endif
    >
    static auto callStaticMethod(jclass clazz, const char *methodName, Args &&...args)
    {
        constexpr auto signature = QtJniTypes::methodSignature<ReturnType, Args...>();
        return callStaticMethod<ReturnType>(clazz, methodName, signature.data(), std::forward<Args>(args)...);
    }
    template <typename Klass, typename ReturnType = void, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidSignatureTypes<ReturnType, Args...> = true
#endif
    >
    static auto callStaticMethod(const char *methodName, Args &&...args)
    {
        LocalFrame<ReturnType, Args...> frame;
        const jclass clazz = QJniObject::loadClass(QtJniTypes::Traits<Klass>::className().data(),
                                                   frame.jniEnv());
        const jmethodID id = clazz ? getMethodID(frame.jniEnv(), clazz, methodName,
                                         QtJniTypes::methodSignature<ReturnType, Args...>().data(), true)
                                   : 0;
        return callStaticMethod<ReturnType>(clazz, id, std::forward<Args>(args)...);
    }

    static QJniObject callStaticObjectMethod(const char *className, const char *methodName,
                                             const char *signature, ...);

    static QJniObject callStaticObjectMethod(jclass clazz, const char *methodName,
                                             const char *signature, ...);

    static QJniObject callStaticObjectMethod(jclass clazz, jmethodID methodId, ...);


    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidSignatureTypes<Ret, Args...> = true
#endif
    >
    static QJniObject callStaticObjectMethod(const char *className, const char *methodName, Args &&...args)
    {
        QtJniTypes::assertObjectType<Ret>();
        constexpr auto signature = QtJniTypes::methodSignature<Ret, Args...>();
        LocalFrame<QJniObject, Args...> frame;
        return frame.template convertFromJni<Ret>(callStaticObjectMethod(className, methodName, signature.data(),
                                            frame.convertToJni(std::forward<Args>(args))...));
    }

    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidSignatureTypes<Ret, Args...> = true
#endif
    >
    static QJniObject callStaticObjectMethod(jclass clazz, const char *methodName, Args &&...args)
    {
        QtJniTypes::assertObjectType<Ret>();
        constexpr auto signature = QtJniTypes::methodSignature<Ret, Args...>();
        LocalFrame<QJniObject, Args...> frame;
        return frame.template convertFromJni<Ret>(callStaticObjectMethod(clazz, methodName, signature.data(),
                                                  frame.convertToJni(std::forward<Args>(args))...));
    }

    template <typename Type
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<Type> = true
#endif
    >
    auto getField(const char *fieldName) const
    {
        using T = typename QtJniTypes::Detail::CallerHandlesException<Type>::value_type;
        LocalFrame<Type, T> frame(jniEnv());
        constexpr auto signature = QtJniTypes::fieldSignature<T>();
        jfieldID id = getCachedFieldID(frame.jniEnv(), fieldName, signature);

        if constexpr (QtJniTypes::isObjectType<T>()) {
            return frame.makeResult(frame.template convertFromJni<T>(getObjectFieldImpl(
                                        frame.jniEnv(), id))
                                   );
        } else {
            T res{};
            if (id)
                getFieldForType<T>(frame.jniEnv(), res, object(), id);
            return frame.makeResult(res);
        }
    }

    template <typename Klass, typename T
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<T> = true
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

    template <typename Type
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<Type> = true
#endif
    >
    static auto getStaticField(const char *className, const char *fieldName)
    {
        using T = typename QtJniTypes::Detail::CallerHandlesException<Type>::value_type;
        LocalFrame<Type, T> frame;
        jclass clazz = QJniObject::loadClass(className, frame.jniEnv());
        return getStaticField<Type>(clazz, fieldName);
    }

    template <typename Type
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<Type> = true
#endif
    >
    static auto getStaticField(jclass clazz, const char *fieldName)
    {
        using T = typename QtJniTypes::Detail::CallerHandlesException<Type>::value_type;
        LocalFrame<Type, T> frame;
        constexpr auto signature = QtJniTypes::fieldSignature<T>();
        jfieldID id = clazz ? getFieldID(frame.jniEnv(), clazz, fieldName, signature, true)
                    : nullptr;
        if constexpr (QtJniTypes::isObjectType<T>()) {
            return frame.makeResult(frame.template convertFromJni<T>(getStaticObjectFieldImpl(
                                        frame.jniEnv(), clazz, id))
                                   );
        } else {
            T res{};
            if (id)
                getStaticFieldForType<T>(frame.jniEnv(), res, clazz, id);
            return frame.makeResult(res);
        }
    }

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

    template <typename Ret = void, typename Type
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<Type> = true
#endif
    >
    auto setField(const char *fieldName, Type value)
    {
        // handle old code that explicitly specifies the field type (i.e. Ret != void)
        using T = std::conditional_t<std::is_void_v<Ret> || QtJniTypes::Detail::callerHandlesException<Ret>,
                                     Type, Ret>;
        LocalFrame<Ret, T> frame(jniEnv());
        constexpr auto signature = QtJniTypes::fieldSignature<T>();
        jfieldID id = getCachedFieldID(jniEnv(), fieldName, signature);
        if (id)
            setFieldForType<T>(jniEnv(), object(), id, value);
        return frame.makeResult();
    }

    template <typename Ret = void, typename Type
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<Type> = true
#endif
    >
    auto setField(const char *fieldName, const char *signature, Type value)
    {
        // handle old code that explicitly specifies the field type (i.e. Ret != void)
        using T = std::conditional_t<std::is_void_v<Ret> || QtJniTypes::Detail::callerHandlesException<Ret>,
                                     Type, Ret>;
        LocalFrame<Ret, T> frame(jniEnv());
        jfieldID id = getCachedFieldID(frame.jniEnv(), fieldName, signature);
        if (id)
            setFieldForType<T>(jniEnv(), object(), id, value);
        return frame.makeResult();
    }

    template <typename Ret = void, typename Type
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<Type> = true
#endif
    >
    static auto setStaticField(const char *className, const char *fieldName, Type value)
    {
        // handle old code that explicitly specifies the field type (i.e. Ret != void)
        using T = std::conditional_t<std::is_void_v<Ret> || QtJniTypes::Detail::callerHandlesException<Ret>,
                                     Type, Ret>;
        LocalFrame<Ret, T> frame;
        if (jclass clazz = QJniObject::loadClass(className, frame.jniEnv())) {
            constexpr auto signature = QtJniTypes::fieldSignature<q20::remove_cvref_t<T>>();
            jfieldID id = getCachedFieldID(frame.jniEnv(), clazz, className, fieldName,
                                           signature, true);
            if (id)
                setStaticFieldForType<T>(frame.jniEnv(), clazz, id, value);
        }
        return frame.makeResult();
    }

    template <typename Ret = void, typename Type
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<Type> = true
#endif
    >
    static auto setStaticField(const char *className, const char *fieldName,
                               const char *signature, Type value)
    {
        // handle old code that explicitly specifies the field type (i.e. Ret != void)
        using T = std::conditional_t<std::is_void_v<Ret> || QtJniTypes::Detail::callerHandlesException<Ret>,
                                     Type, Ret>;
        LocalFrame<Ret, T> frame;
        if (jclass clazz = QJniObject::loadClass(className, frame.jniEnv())) {
            jfieldID id = getCachedFieldID(frame.jniEnv(), clazz, className, fieldName,
                                           signature, true);
            if (id)
                setStaticFieldForType<T>(frame.jniEnv(), clazz, id, value);
        }
        return frame.makeResult();
    }

    template <typename Ret = void, typename Type
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<Type> = true
#endif
    >
    static auto setStaticField(jclass clazz, const char *fieldName,
                               const char *signature, Type value)
    {
        // handle old code that explicitly specifies the field type (i.e. Ret != void)
        using T = std::conditional_t<std::is_void_v<Ret> || QtJniTypes::Detail::callerHandlesException<Ret>,
                                     Type, Ret>;
        LocalFrame<Ret, T> frame;
        jfieldID id = getFieldID(frame.jniEnv(), clazz, fieldName, signature, true);

        if (id)
            setStaticFieldForType<T>(frame.jniEnv(), clazz, id, value);
        return frame.makeResult();
    }

    template <typename Ret = void, typename Type
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<Type> = true
#endif
    >
    static auto setStaticField(jclass clazz, const char *fieldName, Type value)
    {
        return setStaticField<Ret, Type>(clazz, fieldName,
                                         QtJniTypes::fieldSignature<q20::remove_cvref_t<Type>>(),
                                         value);
    }

    template <typename Klass, typename Ret = void, typename Type
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<Type> = true
#endif
    >
    static auto setStaticField(const char *fieldName, Type value)
    {
        return setStaticField<Ret, Type>(QtJniTypes::Traits<Klass>::className(), fieldName, value);
    }

    static QJniObject fromString(const QString &string);
    QString toString() const;

    static bool isClassAvailable(const char *className);
    bool isValid() const;

    // This function takes ownership of the jobject and releases the local ref. before returning.
    static QJniObject fromLocalRef(jobject lref);

    template <typename T,
              std::enable_if_t<std::is_convertible_v<T, jobject>, bool> = true>
    QJniObject &operator=(T obj)
    {
        assign(static_cast<T>(obj));
        return *this;
    }

protected:
    QJniObject(Qt::Initialization) {}
    JNIEnv *jniEnv() const noexcept;

private:
    static jclass loadClass(const QByteArray &className, JNIEnv *env);
    static jclass loadClassKeepExceptions(const QByteArray &className, JNIEnv *env);

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

    template<typename T>
    static constexpr void callMethodForType(JNIEnv *env, T &res, jobject obj, jmethodID id, ...)
    {
        if (!id)
            return;

        va_list args = {};
        va_start(args, id);
        QtJniTypes::Caller<T>::callMethodForType(env, res, obj, id, args);
        va_end(args);
    }

    jobject callObjectMethodImpl(jmethodID method, ...) const
    {
        va_list args;
        va_start(args, method);
        jobject res = method ? jniEnv()->CallObjectMethodV(javaObject(), method, args)
                             : nullptr;
        va_end(args);
        return res;
    }

    template<typename T>
    static constexpr void callStaticMethodForType(JNIEnv *env, T &res, jclass clazz,
                                                  jmethodID id, ...)
    {
        if (!clazz || !id)
            return;
        va_list args = {};
        va_start(args, id);
        QtJniTypes::Caller<T>::callStaticMethodForType(env, res, clazz, id, args);
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
    static constexpr void getFieldForType(JNIEnv *env, T &res, jobject obj, jfieldID id)
    {
        if (!id)
            return;

        QtJniTypes::Caller<T>::getFieldForType(env, res, obj, id);
    }

    template<typename T>
    static constexpr void getStaticFieldForType(JNIEnv *env, T &res, jclass clazz, jfieldID id)
    {
        QtJniTypes::Caller<T>::getStaticFieldForType(env, res, clazz, id);
    }

    template<typename Type>
    static constexpr void setFieldForType(JNIEnv *env, jobject obj, jfieldID id, Type value)
    {
        if (!id)
            return;

        using T = q20::remove_cvref_t<Type>;
        if constexpr (QtJniTypes::isObjectType<T>()) {
            LocalFrame<T, T> frame(env);
            env->SetObjectField(obj, id, static_cast<jobject>(frame.convertToJni(value)));
        } else {
            using ValueType = typename QtJniTypes::Detail::CallerHandlesException<T>::value_type;
            QtJniTypes::Caller<ValueType>::setFieldForType(env, obj, id, value);
        }
    }

    jobject getObjectFieldImpl(JNIEnv *env, jfieldID field) const
    {
        return field ? env->GetObjectField(javaObject(), field) : nullptr;
    }

    static jobject getStaticObjectFieldImpl(JNIEnv *env, jclass clazz, jfieldID field)
    {
        return clazz && field ? env->GetStaticObjectField(clazz, field)
                              : nullptr;
    }

    template<typename Type>
    static constexpr void setStaticFieldForType(JNIEnv *env, jclass clazz, jfieldID id, Type value)
    {
        if (!clazz || !id)
            return;

        using T = q20::remove_cvref_t<Type>;
        if constexpr (QtJniTypes::isObjectType<T>()) {
            LocalFrame<T, T> frame(env);
            env->SetStaticObjectField(clazz, id, static_cast<jobject>(frame.convertToJni(value)));
        } else {
            QtJniTypes::Caller<T>::setStaticFieldForType(env, clazz, id, value);
        }
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

namespace QtJniTypes {
struct JObjectBase
{
    operator QJniObject() const { return m_object; }

    bool isValid() const { return m_object.isValid(); }
    jclass objectClass() const { return m_object.objectClass(); }
    QString toString() const { return m_object.toString(); }

    template <typename T = jobject>
    T object() const {
        return m_object.object<T>();
    }

protected:
    JObjectBase() = default;
    JObjectBase(const JObjectBase &) = default;
    JObjectBase(JObjectBase &&) = default;
    JObjectBase &operator=(const JObjectBase &) = default;
    JObjectBase &operator=(JObjectBase &&) = default;
    ~JObjectBase() = default;

    Q_IMPLICIT JObjectBase(jobject object) : m_object(object) {}
    Q_IMPLICIT JObjectBase(const QJniObject &object) : m_object(object) {}
    Q_IMPLICIT JObjectBase(QJniObject &&object) noexcept : m_object(std::move(object)) {}

    QJniObject m_object;
};

template<typename Type>
class JObject : public JObjectBase
{
public:
    using Class = Type;

    JObject()
        : JObjectBase{QJniObject(QtJniTypes::Traits<Class>::className())}
    {}
    Q_IMPLICIT JObject(jobject object) : JObjectBase(object) {}
    Q_IMPLICIT JObject(const QJniObject &object) : JObjectBase(object) {}
    Q_IMPLICIT JObject(QJniObject &&object) noexcept : JObjectBase(std::move(object)) {}

    // base class SMFs are protected, make them public:
    JObject(const JObject &other) = default;
    JObject(JObject &&other) noexcept = default;
    JObject &operator=(const JObject &other) = default;
    JObject &operator=(JObject &&other) noexcept = default;

    ~JObject() = default;

    template<typename Arg, typename ...Args
            , std::enable_if_t<!std::is_same_v<q20::remove_cvref_t<Arg>, JObject>, bool> = true
            , IfValidSignatureTypes<Arg, Args...> = true
    >
    explicit JObject(Arg && arg, Args &&...args)
        : JObjectBase{QJniObject(QtJniTypes::Traits<Class>::className(),
                                 std::forward<Arg>(arg), std::forward<Args>(args)...)}
    {}

    // named constructors avoid ambiguities
    static JObject fromJObject(jobject object)
    {
        return JObject(object);
    }
    template <typename ...Args>
    static JObject construct(Args &&...args)
    {
        return JObject(std::forward<Args>(args)...);
    }
    static JObject fromLocalRef(jobject lref)
    {
        return JObject(QJniObject::fromLocalRef(lref));
    }

#ifdef Q_QDOC // from JObjectBase, which we don't document
    bool isValid() const;
    jclass objectClass() const;
    QString toString() const;
    template <typename T = jobject> object() const;
#endif

    static bool registerNativeMethods(std::initializer_list<JNINativeMethod> methods)
    {
        QJniEnvironment env;
        return env.registerNativeMethods<Class>(methods);
    }

    // public API forwarding to QJniObject, with the implicit Class template parameter
    template <typename Ret = void, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidSignatureTypes<Ret, Args...> = true
#endif
    >
    static auto callStaticMethod(const char *name, Args &&...args)
    {
        return QJniObject::callStaticMethod<Class, Ret, Args...>(name,
                                                                 std::forward<Args>(args)...);
    }
    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<T> = true
#endif
    >
    static auto getStaticField(const char *field)
    {
        return QJniObject::getStaticField<Class, T>(field);
    }
    template <typename Ret = void, typename T
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<T> = true
#endif
    >
    static auto setStaticField(const char *field, T &&value)
    {
        return QJniObject::setStaticField<Class, Ret, T>(field, std::forward<T>(value));
    }

    // keep only these overloads, the rest is made private
    template <typename Ret = void, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::IfValidSignatureTypes<Ret, Args...> = true
#endif
    >
    auto callMethod(const char *method, Args &&...args) const
    {
        return m_object.callMethod<Ret>(method, std::forward<Args>(args)...);
    }
    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<T> = true
#endif
    >
    auto getField(const char *fieldName) const
    {
        return m_object.getField<T>(fieldName);
    }

    template <typename Ret = void, typename T
#ifndef Q_QDOC
        , QtJniTypes::IfValidFieldType<T> = true
#endif
    >
    auto setField(const char *fieldName, T &&value)
    {
        return m_object.setField<Ret>(fieldName, std::forward<T>(value));
    }

    QByteArray className() const {
        return QtJniTypes::Traits<Class>::className().data();
    }

    static bool isClassAvailable()
    {
        return QJniObject::isClassAvailable(QtJniTypes::Traits<Class>::className().data());
    }

private:
    friend bool comparesEqual(const JObject &lhs, const JObject &rhs)
    { return lhs.m_object == rhs.m_object; }
    Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(JObject);
};

template <typename T> struct Traits<JObject<T>> {
    static constexpr auto signature() { return Traits<T>::signature(); }
    static constexpr auto className() { return Traits<T>::className(); }
    static auto convertToJni(JNIEnv *, const JObject<T> &value)
    {
        return value.object();
    }
    static auto convertFromJni(QJniObject &&object)
    {
        return JObject<T>(std::move(object));
    }
};

template<>
struct Traits<QJniObject>
{
    static constexpr auto className()
    {
        return CTString("java/lang/Object");
    }

    static constexpr auto signature()
    {
        return CTString("Ljava/lang/Object;");
    }

    static auto convertToJni(JNIEnv *, const QJniObject &value)
    {
        return value.object();
    }
    static auto convertFromJni(QJniObject &&object)
    {
        return std::move(object);
    }
};

template<>
struct Traits<QString>
{
    static constexpr auto className()
    {
        return CTString("java/lang/String");
    }
    static constexpr auto signature()
    {
        return CTString("Ljava/lang/String;");
    }

    static auto convertToJni(JNIEnv *env, const QString &value)
    {
        return QtJniTypes::Detail::fromQString(value, env);
    }

    static auto convertFromJni(QJniObject &&object)
    {
        return object.toString();
    }
};

template <typename T>
struct Traits<T, std::enable_if_t<QtJniTypes::Detail::callerHandlesException<T>>>
{
    static constexpr auto className()
    {
        return Traits<typename T::value_type>::className();
    }

    static constexpr auto signature()
    {
        return Traits<typename T::value_type>::signature();
    }
};

}

template <typename ReturnType, typename ...Args>
template <typename T>
auto QtJniTypes::Detail::LocalFrameWithReturn<ReturnType, Args...>::convertFromJni(jobject object)
{
    // If the caller wants to handle exceptions through a std::expected-like
    // type, then we cannot turn the jobject into a QJniObject, as a
    // std::expected<jobject, jthrowable> cannot be constructed from a QJniObject.
    // The caller will have to take care of this themselves, by asking for a
    // std::expected<QJniObject, ...>, or (typically) using a declared JNI class
    // or implicitly supported Qt type (QString or array type).
    if constexpr (callerHandlesException<ReturnType> &&
                  std::is_base_of_v<std::remove_pointer_t<jobject>, std::remove_pointer_t<T>>)
        return static_cast<T>(object);
    else
        return convertFromJni<T>(object ? QJniObject::fromLocalRef(object) : QJniObject());
}

QT_END_NAMESPACE

#endif

#endif // QJNIOBJECT_H
