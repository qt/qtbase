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
struct Type##Tag { explicit Type##Tag() = default; };           \
using Type = JObject<Type##Tag>;                                \

// QT_TECH_PREVIEW_API
#define Q_DECLARE_JNI_TYPE(Type, Signature)                     \
namespace QtJniTypes {                                          \
Q_DECLARE_JNI_TYPE_HELPER(Type)                                 \
}                                                               \
template<>                                                      \
struct ::QtJniTypes::Traits<QtJniTypes::Type##Tag> {            \
    static constexpr auto signature()                           \
    {                                                           \
        constexpr QtJniTypes::CTString sig(Signature);          \
        static_assert((sig.startsWith('L') || sig.startsWith("[L"))    \
                    && sig.endsWith(';'),                       \
                    "Type signature needs to start with 'L' or" \
                    " '[L', and end with ';'");                 \
        return sig;                                             \
    }                                                           \
};                                                              \

#define Q_DECLARE_JNI_CLASS_2(Type, _)                          \
Q_DECLARE_JNI_TYPE_HELPER(Type)                                 \

#define Q_DECLARE_JNI_CLASS_SPECIALIZATION_2(Type, Signature)   \
template<>                                                      \
struct QtJniTypes::Traits<QtJniTypes::Type##Tag> {              \
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

#define Q_DECLARE_JNI_CLASS_3(NS0, NS1, Type)                   \
namespace NS0 {                                                 \
namespace NS1 {                                                 \
Q_DECLARE_JNI_CLASS_2(Type, Q_UNUSED(0))                        \
}                                                               \
}                                                               \

#define Q_DECLARE_JNI_CLASS_SPECIALIZATION_3(NS0, NS1, Type)    \
    Q_DECLARE_JNI_CLASS_SPECIALIZATION_2(NS0::NS1::Type,        \
        #NS0 "/" #NS1 "/" #Type)

#define Q_DECLARE_JNI_CLASS_4(NS0, NS1, NS2, Type)              \
namespace NS0 {                                                 \
Q_DECLARE_JNI_CLASS_3(NS1, NS2, Type)                           \
}                                                               \

#define Q_DECLARE_JNI_CLASS_SPECIALIZATION_4(NS0, NS1, NS2, Type) \
    Q_DECLARE_JNI_CLASS_SPECIALIZATION_2(NS0::NS1::NS2::Type,   \
        #NS0 "/" #NS1 "/" #NS2 "/" #Type)

#define Q_DECLARE_JNI_CLASS_5(NS0, NS1, NS2, NS3, Type)         \
namespace NS0 {                                                 \
Q_DECLARE_JNI_CLASS_4(NS1, NS2, NS3, Type)                      \
}                                                               \

#define Q_DECLARE_JNI_CLASS_SPECIALIZATION_5(NS0, NS1, NS2, NS3, Type) \
    Q_DECLARE_JNI_CLASS_SPECIALIZATION_2(NS0::NS1::NS2::NS3::Type,  \
        #NS0 "/" #NS1 "/" #NS2 "/" #NS3 "/" #Type)

#define Q_DECLARE_JNI_CLASS_6(NS0, NS1, NS2, NS3, NS4, Type)    \
namespace NS0 {                                                 \
Q_DECLARE_JNI_CLASS_5(NS1, NS2, NS3, NS4, Type)                 \
}                                                               \

#define Q_DECLARE_JNI_CLASS_SPECIALIZATION_6(NS0, NS1, NS2, NS3, NS4, Type) \
    Q_DECLARE_JNI_CLASS_SPECIALIZATION_2(NS0::NS1::NS2::NS3::NS4::Type, \
        #NS0 "/" #NS1 "/" #NS2 "/" #NS3 "/" #NS4 "/" #Type)

#define Q_DECLARE_JNI_CLASS_7(NS0, NS1, NS2, NS3, NS4, NS5, Type) \
namespace NS0 {                                                 \
Q_DECLARE_JNI_CLASS_6(NS1, NS2, NS3, NS4, NS5, Type)            \
}                                                               \

#define Q_DECLARE_JNI_CLASS_SPECIALIZATION_7(NS0, NS1, NS2, NS3, NS4, NS5, Type) \
    Q_DECLARE_JNI_CLASS_SPECIALIZATION_2(NS0::NS1::NS2::NS3::NS4::NS5::Type,    \
        #NS0 "/" #NS1 "/" #NS2 "/" #NS3 "/" #NS4 "/" #NS5 "/" #Type)

#define Q_DECLARE_JNI_CLASS_8(NS0, NS1, NS2, NS3, NS4, NS5, NS6, Type) \
namespace NS0 {                                                 \
Q_DECLARE_JNI_CLASS_7(NS1, NS2, NS3, NS4, NS5, NS6, Type)       \
}                                                               \

#define Q_DECLARE_JNI_CLASS_SPECIALIZATION_8(NS0, NS1, NS2, NS3, NS4, NS5, NS6, Type) \
    Q_DECLARE_JNI_CLASS_SPECIALIZATION_2(NS0::NS1::NS2::NS3::NS4::NS5::NS6::Type,   \
        #NS0 "/" #NS1 "/" #NS2 "/" #NS3 "/" #NS4 "/" #NS5 "/" #NS6 "/" #Type)

#define Q_DECLARE_JNI_CLASS_9(NS0, NS1, NS2, NS3, NS4, NS5, NS6, NS7, Type) \
namespace NS0 {                                                 \
Q_DECLARE_JNI_CLASS_8(NS1, NS2, NS3, NS4, NS5, NS6, NS7, Type)  \
}                                                               \

#define Q_DECLARE_JNI_CLASS_SPECIALIZATION_9(NS0, NS1, NS2, NS3, NS4, NS5, NS6, NS7, Type) \
    Q_DECLARE_JNI_CLASS_SPECIALIZATION_2(NS0::NS1::NS2::NS3::NS4::NS5::NS6::NS7::Type,  \
        #NS0 "/" #NS1 "/" #NS2 "/" #NS3 "/" #NS4 "/" #NS5 "/" #NS6 "/" #NS7 "/" #Type)

#define Q_DECLARE_JNI_CLASS(...)                                \
namespace QtJniTypes {                                          \
QT_OVERLOADED_MACRO(Q_DECLARE_JNI_CLASS, __VA_ARGS__)           \
}                                                               \
QT_OVERLOADED_MACRO(Q_DECLARE_JNI_CLASS_SPECIALIZATION, __VA_ARGS__)

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

template <typename T>
struct JNITypeForArgImpl<QJniArray<T>>
{
    using Type = jobject;

    static QJniArray<T> fromVarArg(Type t)
    {
        return QJniArray<T>(t);
    }
};

template <typename T>
struct JNITypeForArgImpl<QList<T>>
{
private:
    using ArrayType = decltype(QJniArrayBase::fromContainer(std::declval<QList<T>>()));
    using ArrayObjectType = decltype(std::declval<ArrayType>().arrayObject());
    using ElementType = typename ArrayType::value_type;
public:
    using Type = ArrayObjectType;

    static QList<T> fromVarArg(Type t)
    {
        return QJniArray<ElementType>(t).toContainer();
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

// Classes for value types
Q_DECLARE_JNI_CLASS(String, "java/lang/String")
Q_DECLARE_JNI_CLASS(Integer, "java/lang/Integer");
Q_DECLARE_JNI_CLASS(Long, "java/lang/Long");
Q_DECLARE_JNI_CLASS(Double, "java/lang/Double");
Q_DECLARE_JNI_CLASS(Float, "java/lang/Float");
Q_DECLARE_JNI_CLASS(Boolean, "java/lang/Boolean");
Q_DECLARE_JNI_CLASS(Void, "java/lang/Void");

// Utility and I/O
Q_DECLARE_JNI_CLASS(UUID, "java/util/UUID")
Q_DECLARE_JNI_CLASS(ArrayList, "java/util/ArrayList")
Q_DECLARE_JNI_CLASS(HashMap, "java/util/HashMap")
Q_DECLARE_JNI_CLASS(Set, "java/util/Set")
Q_DECLARE_JNI_CLASS(File, "java/io/File");

// Android specific types
Q_DECLARE_JNI_CLASS(Uri, "android/net/Uri");
Q_DECLARE_JNI_CLASS(Parcelable, "android/os/Parcelable");
Q_DECLARE_JNI_CLASS(Context, "android/content/Context");
Q_DECLARE_JNI_CLASS(Intent, "android/content/Intent");
Q_DECLARE_JNI_CLASS(ContentResolver, "android/content/ContentResolver");
Q_DECLARE_JNI_CLASS(Activity, "android/app/Activity");
Q_DECLARE_JNI_CLASS(Service, "android/app/Service");

#define QT_DECLARE_JNI_CLASS_STANDARD_TYPES

QT_END_NAMESPACE

#endif // defined(Q_QDOC) || defined(Q_OS_ANDROID)

#endif // QJNITYPES_H
