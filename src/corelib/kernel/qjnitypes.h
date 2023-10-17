// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QJNITYPES_H
#define QJNITYPES_H

#if defined(Q_QDOC) || defined(Q_OS_ANDROID)

#include <QtCore/qjnitypes_impl.h>
#include <QtCore/qjniobject.h>

QT_BEGIN_NAMESPACE

namespace QtJniTypes
{
// A generic base class for specialized QJniObject types, to be used by
// subclasses via CRTP. It's implicitly convertible to and from jobject, which
// allows the QJniObject implementation to implicitly pass instances of this
// type through the variadic argument JNI APIs.
template<typename Type>
struct Object : QJniObject
{
    using Class = Type;
    operator jobject() const noexcept { return object(); }

    Q_IMPLICIT Object(jobject object) : QJniObject(object) {}
    Q_IMPLICIT Object(const QJniObject &object) : QJniObject(object) {}
    Q_IMPLICIT Object(QJniObject &&object) : QJniObject(std::move(object)) {}

    // Compiler-generated copy/move semantics based on QJniObject's shared d-pointer are fine!
    Object(const Object<Type> &other) = default;
    Object(Object<Type> &&other) = default;
    Object<Type> &operator=(const Object<Type> &other) = default;
    Object<Type> &operator=(Object<Type> &&other) = default;

    // avoid ambiguities with deleted const char * constructor
    Q_IMPLICIT Object(std::nullptr_t) : QJniObject() {}

    // this intentionally includes the default constructor
    template<typename ...Args
            , ValidSignatureTypes<Args...> = true
    >
    explicit Object(Args &&...args)
        : QJniObject(Qt::Initialization::Uninitialized)
    {
        *this = Object<Type>{QJniObject::construct<Class>(std::forward<Args>(args)...)};
    }

    // named constructors avoid ambiguities
    static Object<Type> fromJObject(jobject object) { return Object<Type>(object); }
    template <typename ...Args>
    static Object<Type> construct(Args &&...args)
    {
        return Object<Type>{QJniObject::construct<Class, Args...>(std::forward<Args>(args)...)};
    }

    // public API forwarding to QJniObject, with the implicit Class template parameter
    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::ValidSignatureTypes<Ret, Args...> = true
#endif
    >
    static auto callStaticMethod(const char *name, Args &&...args)
    {
        return QJniObject::callStaticMethod<Class, Ret, Args...>(name,
                                                                 std::forward<Args>(args)...);
    }
    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    static auto getStaticField(const char *field)
    {
        return QJniObject::getStaticField<Class, T>(field);
    }
    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    static void setStaticField(const char *field, T &&value)
    {
        QJniObject::setStaticField<Class, T>(field, std::forward<T>(value));
    }

    // keep only these overloads, the rest is made private
    template <typename Ret, typename ...Args
#ifndef Q_QDOC
        , QtJniTypes::ValidSignatureTypes<Ret, Args...> = true
#endif
    >
    auto callMethod(const char *method, Args &&...args) const
    {
        return QJniObject::callMethod<Ret>(method, std::forward<Args>(args)...);
    }
    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    auto getField(const char *fieldName) const
    {
        return QJniObject::getField<T>(fieldName);
    }

    template <typename T
#ifndef Q_QDOC
        , QtJniTypes::ValidFieldType<T> = true
#endif
    >
    void setField(const char *fieldName, T &&value)
    {
        QJniObject::setField(fieldName, std::forward<T>(value));
    }

private:
    // The following declutters the API of these types compared to the QJniObject API.
    // 1) 'Object' methods; the functions we have have return type auto and will return
    //     the type specified by the template parameter.
    using QJniObject::callObjectMethod;
    using QJniObject::callStaticObjectMethod;
    using QJniObject::getObjectField;
    using QJniObject::getStaticObjectField;

    // 2) Constructors that take a class name, signature string, or class template argument
    explicit Object(const char *className) = delete;
    explicit Object(const char *className, const char *signature, ...) = delete;
    template<typename ...Args>
    explicit Object(const char *className, Args &&...args) = delete;
    explicit Object(jclass clazz, const char *signature, ...) = delete;
    template<typename Class, typename ...Args>
    static QJniObject construct(Args &&...args) = delete;

    // 3) Overloads that take a class name/jclass, methodID, signature string, or an
    //    explicit class template argument
    template <typename Ret, typename ...Args>
    auto callMethod(const char *methodName, const char *signature, Args &&...args) const = delete;
    template <typename Ret, typename ...Args>

    static auto callStaticMethod(const char *className, const char *methodName,
                                 const char *signature, Args &&...args) = delete;
    template <typename Ret, typename ...Args>
    static auto callStaticMethod(jclass clazz, const char *methodName,
                                 const char *signature, Args &&...args) = delete;
    template <typename Ret, typename ...Args>
    static auto callStaticMethod(jclass clazz, jmethodID methodId, Args &&...args) = delete;
    template <typename Ret, typename ...Args>
    static auto callStaticMethod(const char *className, const char *methodName,
                                 Args &&...args) = delete;
    template <typename Ret, typename ...Args>
    static auto callStaticMethod(jclass clazz, const char *methodName, Args &&...args) = delete;
    template <typename Klass, typename Ret, typename ...Args>
    static auto callStaticMethod(const char *methodName, Args &&...args) = delete;

    template <typename T>
    static auto getStaticField(const char *className, const char *fieldName) = delete;
    template <typename T>
    static auto getStaticField(jclass clazz, const char *fieldName) = delete;
    template <typename Klass, typename T>
    static auto getStaticField(const char *fieldName) = delete;

    template <typename T>
    void setField(const char *fieldName, const char *signature, T value) = delete;
    template <typename T>
    static void setStaticField(const char *className, const char *fieldName, T value) = delete;
    template <typename T>
    static void setStaticField(const char *className, const char *fieldName,
                               const char *signature, T value) = delete;
    template <typename T>
    static void setStaticField(jclass clazz, const char *fieldName,
                               const char *signature, T value) = delete;
    template <typename T>
    static void setStaticField(jclass clazz, const char *fieldName, T value) = delete;
    template <typename Klass, typename T>
    static void setStaticField(const char *fieldName, T value) = delete;
};

} // namespace QtJniTypes

#define Q_DECLARE_JNI_TYPE_HELPER(Type)                         \
namespace QtJniTypes {                                          \
struct Type : Object<Type>                                      \
{                                                               \
    using Object::Object;                                       \
};                                                              \
}                                                               \


#define Q_DECLARE_JNI_TYPE(Type, Signature)                     \
Q_DECLARE_JNI_TYPE_HELPER(Type)                                 \
template<>                                                      \
struct QtJniTypes::Traits<QtJniTypes::Type> {                   \
    static constexpr auto signature()                           \
    {                                                           \
        static_assert((Signature[0] == 'L'                      \
                    || Signature[0] == '[')                     \
                    && Signature[sizeof(Signature) - 2] == ';', \
                    "Type signature needs to start with 'L' or" \
                    " '[' and end with ';'");                   \
        return QtJniTypes::CTString(Signature);                 \
    }                                                           \
};                                                              \

#define Q_DECLARE_JNI_CLASS(Type, Signature)                    \
Q_DECLARE_JNI_TYPE_HELPER(Type)                                 \
template<>                                                      \
struct QtJniTypes::Traits<QtJniTypes::Type> {                   \
    static constexpr auto className()                           \
    {                                                           \
        return QtJniTypes::CTString(Signature);                 \
    }                                                           \
    static constexpr auto signature()                           \
    {                                                           \
        return QtJniTypes::CTString("L")                        \
            + className()                                       \
            + QtJniTypes::CTString(";");                        \
    }                                                           \
};                                                              \

// Macros for native methods

namespace QtJniMethods {
// Various helpers to forward a call from a variadic argument function to
// the real function with proper type conversion. This is needed because we
// want to write functions that take QJniObjects (subclasses), while Java
// can only call functions that take jobjects.

// In Var-arg functions, any argument narrower than (unsigned) int or double
// is promoted to (unsigned) int or double.
template <typename Arg> struct PromotedType { using Type = Arg; };
template <> struct PromotedType<bool> { using Type = int; };
template <> struct PromotedType<char> { using Type = int; };
template <> struct PromotedType<signed char> { using Type = int; };
template <> struct PromotedType<unsigned char> { using Type = unsigned int; };
template <> struct PromotedType<short> { using Type = int; };
template <> struct PromotedType<unsigned short> { using Type = unsigned int; };
template <> struct PromotedType<float> { using Type = double; };

// Map any QJniObject type to jobject; that's what's on the va_list
template <typename Arg>
using JNITypeForArg = std::conditional_t<std::is_base_of_v<QJniObject, Arg>, jobject,
                                                                             typename PromotedType<Arg>::Type>;

// Turn a va_list into a tuple of typed arguments
template <typename Ret, typename ...Args>
static constexpr auto makeTupleFromArgs(Ret (*)(JNIEnv *, jobject, Args...), va_list args)
{
    return std::tuple<Args...>{ va_arg(args, JNITypeForArg<Args>)... };
}
template <typename Ret, typename ...Args>
static constexpr auto makeTupleFromArgs(Ret (*)(JNIEnv *, jclass, Args...), va_list args)
{
    return std::tuple<Args...>{ va_arg(args, JNITypeForArg<Args>)... };
}

// Get the return type of a function point
template <typename Ret, typename ...Args>
auto nativeFunctionReturnType(Ret(*function)(Args...))
{
    return function(std::declval<Args>()...);
}

} // QtJniMethods

// A va_ variadic arguments function that we register with JNI as a proxy
// for the function we have. This function uses the helpers to unpack the
// variadic arguments into a tuple of typed arguments, which we then call
// the actual function with. This then takes care of implicit conversions,
// e.g. a jobject becomes a QJniObject.
#define Q_DECLARE_JNI_NATIVE_METHOD_HELPER(Method)                      \
static decltype(QtJniMethods::nativeFunctionReturnType(Method))         \
va_##Method(JNIEnv *env, jclass thiz, ...)                              \
{                                                                       \
    va_list args;                                                       \
    va_start(args, thiz);                                               \
    auto va_cleanup = qScopeGuard([&args]{ va_end(args); });            \
    auto argTuple = QtJniMethods::makeTupleFromArgs(Method, args);      \
    return std::apply([env, thiz](auto &&... args) {                    \
        return Method(env, thiz, args...);                              \
    }, argTuple);                                                       \
}                                                                       \


#define Q_DECLARE_JNI_NATIVE_METHOD(...)                        \
    QT_OVERLOADED_MACRO(QT_DECLARE_JNI_NATIVE_METHOD, __VA_ARGS__) \

#define QT_DECLARE_JNI_NATIVE_METHOD_2(Method, Name)            \
namespace QtJniMethods {                                        \
Q_DECLARE_JNI_NATIVE_METHOD_HELPER(Method)                      \
static constexpr auto Method##_signature =                      \
    QtJniTypes::nativeMethodSignature(Method);                  \
static const JNINativeMethod Method##_method = {                \
    #Name, Method##_signature.data(),                           \
    reinterpret_cast<void *>(va_##Method)                       \
};                                                              \
}                                                               \

#define QT_DECLARE_JNI_NATIVE_METHOD_1(Method)                  \
    QT_DECLARE_JNI_NATIVE_METHOD_2(Method, Method)              \

#define Q_JNI_NATIVE_METHOD(Method) QtJniMethods::Method##_method

#define Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(...)                                        \
    QT_OVERLOADED_MACRO(QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE, __VA_ARGS__)              \

#define QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_2(Method, Name)                            \
    Q_DECLARE_JNI_NATIVE_METHOD_HELPER(Method)                                                   \
    static inline constexpr auto Method##_signature = QtJniTypes::nativeMethodSignature(Method); \
    static inline const JNINativeMethod Method##_method = {                                      \
        #Name, Method##_signature.data(), reinterpret_cast<void *>(va_##Method)                  \
    };

#define QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_1(Method)                                  \
    QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_2(Method, Method)                              \

#define Q_JNI_NATIVE_SCOPED_METHOD(Method, Scope) Scope::Method##_method

QT_END_NAMESPACE

#endif // defined(Q_QDOC) || defined(Q_OS_ANDROID)

#endif // QJNITYPES_H
