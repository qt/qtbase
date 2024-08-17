// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QJNITYPES_H
#define QJNITYPES_H

#if defined(Q_QDOC) || defined(Q_OS_ANDROID)

#include <QtCore/qjnitypes_impl.h>
#include <QtCore/qjniobject.h>

QT_BEGIN_NAMESPACE

// QT_TECH_PREVIEW_API
#define Q_DECLARE_JNI_TYPE_HELPER(Type)                         \
namespace QtJniTypes {                                          \
struct Type : JObject<Type>                                     \
{                                                               \
    using JObject::JObject;                                     \
};                                                              \
}                                                               \

// QT_TECH_PREVIEW_API
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

// QT_TECH_PREVIEW_API
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
namespace Detail {
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
struct JNITypeForArgImpl
{
    using Type = std::conditional_t<std::disjunction_v<std::is_base_of<QJniObject, Arg>,
                                                       std::is_base_of<QtJniTypes::JObjectBase, Arg>>,
                                    jobject, typename PromotedType<Arg>::Type>;
    static Arg fromVarArg(Type t)
    {
        return static_cast<Arg>(t);
    }
};

template <>
struct JNITypeForArgImpl<QString>
{
    using Type = jstring;

    static QString fromVarArg(Type t)
    {
        return QJniObject(t).toString();
    }
};

template <typename Arg>
using JNITypeForArg = typename JNITypeForArgImpl<std::decay_t<Arg>>::Type;
template <typename Arg, typename Type>
static inline auto methodArgFromVarArg(Type t) // Type comes from a va_arg, so is always POD
{
    return JNITypeForArgImpl<std::decay_t<Arg>>::fromVarArg(t);
}

// Turn a va_list into a tuple of typed arguments
template <typename ...Args>
static constexpr auto makeTupleFromArgsHelper(va_list args)
{
    return std::tuple(methodArgFromVarArg<Args>(va_arg(args, JNITypeForArg<Args>))...);
}

template <typename Ret, typename ...Args>
static constexpr auto makeTupleFromArgs(Ret (*)(JNIEnv *, jobject, Args...), va_list args)
{
    return makeTupleFromArgsHelper<Args...>(args);
}
template <typename Ret, typename ...Args>
static constexpr auto makeTupleFromArgs(Ret (*)(JNIEnv *, jclass, Args...), va_list args)
{
    return makeTupleFromArgsHelper<Args...>(args);
}

template <typename>
struct NativeFunctionReturnType {};

template<typename Ret, typename... Args>
struct NativeFunctionReturnType<Ret(Args...)>
{
  using type = Ret;
};

} // namespace Detail
} // namespace QtJniMethods

// A va_ variadic arguments function that we register with JNI as a proxy
// for the function we have. This function uses the helpers to unpack the
// variadic arguments into a tuple of typed arguments, which we then call
// the actual function with. This then takes care of implicit conversions,
// e.g. a jobject becomes a QJniObject.
#define Q_DECLARE_JNI_NATIVE_METHOD_HELPER(Method)                              \
static QtJniMethods::Detail::NativeFunctionReturnType<decltype(Method)>::type   \
va_##Method(JNIEnv *env, jclass thiz, ...)                                      \
{                                                                               \
    va_list args;                                                               \
    va_start(args, thiz);                                                       \
    auto va_cleanup = qScopeGuard([&args]{ va_end(args); });                    \
    auto argTuple = QtJniMethods::Detail::makeTupleFromArgs(Method, args);      \
    return std::apply([env, thiz](auto &&... args) {                            \
        return Method(env, thiz, args...);                                      \
    }, argTuple);                                                               \
}                                                                               \

// QT_TECH_PREVIEW_API
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

// QT_TECH_PREVIEW_API
#define Q_JNI_NATIVE_METHOD(Method) QtJniMethods::Method##_method

// QT_TECH_PREVIEW_API
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

// QT_TECH_PREVIEW_API
#define Q_JNI_NATIVE_SCOPED_METHOD(Method, Scope) Scope::Method##_method

QT_END_NAMESPACE

#endif // defined(Q_QDOC) || defined(Q_OS_ANDROID)

#endif // QJNITYPES_H
