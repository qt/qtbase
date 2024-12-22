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
// Various helpers to forward the call to the registered function (with JNI types
// as arguments) to the real function with proper type conversion. This is needed
// because we want to write functions that take QJniObjects (subclasses), while
// Java can only call functions that take jobjects.

// Map any QJniObject type to jobject
template <typename Arg>
struct JNITypeForArgImpl
{
    using Type = std::conditional_t<std::disjunction_v<std::is_base_of<QJniObject, Arg>,
                                                       std::is_base_of<QtJniTypes::JObjectBase, Arg>>,
                                    jobject, Arg>;
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
} // namespace Detail
} // namespace QtJniMethods

// Declaring a JNI method results in a struct with a template function call() that
// gets instantiated with the return type and arguments of the declared method,
// and registered with JNI. That template is implemented to call the declared
// method, with arguments explicitly converted to the types the declared method
// expects (e.g. jobject becomes QJniObject, a QString, a QList etc).
#define Q_DECLARE_JNI_NATIVE_METHOD_HELPER(Method, Postfix, Name)                           \
struct Method##_##Postfix {                                                                 \
template<typename Ret, typename JType, typename... Args>                                    \
JNICALL static                                                                              \
Ret call(JNIEnv *env, JType thiz, QtJniMethods::Detail::JNITypeForArg<Args> ...args)        \
{                                                                                           \
    return Method(env, thiz, QtJniMethods::Detail::JNITypeForArgImpl<                       \
                                                std::decay_t<Args>>::fromVarArg(args)...    \
                 );                                                                         \
}                                                                                           \
static constexpr auto signature = QtJniTypes::nativeMethodSignature(Method);                \
template<typename Ret, typename JType, typename ...Args>                                    \
static constexpr JNINativeMethod makeJNIMethod(Ret(*)(JNIEnv *, JType, Args...))            \
{                                                                                           \
    return JNINativeMethod {                                                                \
        #Name, signature.data(),                                                            \
        reinterpret_cast<void *>(&call<Ret, JType, Args...>)                                \
    };                                                                                      \
}                                                                                           \
};                                                                                          \

#define Q_DECLARE_JNI_NATIVE_METHOD(...)                                        \
    QT_OVERLOADED_MACRO(QT_DECLARE_JNI_NATIVE_METHOD, __VA_ARGS__)              \

#define QT_DECLARE_JNI_NATIVE_METHOD_2(Method, Name)                            \
namespace QtJniMethods {                                                        \
Q_DECLARE_JNI_NATIVE_METHOD_HELPER(Method, Helper, Name)                        \
}                                                                               \

#define QT_DECLARE_JNI_NATIVE_METHOD_1(Method)                                  \
    QT_DECLARE_JNI_NATIVE_METHOD_2(Method, Method)                              \

#define Q_JNI_NATIVE_METHOD(Method)                                             \
    QtJniMethods::Method##_Helper::makeJNIMethod(::Method)

#define Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(...)                                   \
    QT_OVERLOADED_MACRO(QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE, __VA_ARGS__)         \

#define QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_2(Method, Name)                       \
Q_DECLARE_JNI_NATIVE_METHOD_HELPER(Method, QtJniMethod, Name)                               \

#define QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_1(Method)                             \
    QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_2(Method, Method)                         \

#define Q_JNI_NATIVE_SCOPED_METHOD(Method, Scope)                                           \
    Scope::Method##_QtJniMethod::makeJNIMethod(Scope::Method)

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
