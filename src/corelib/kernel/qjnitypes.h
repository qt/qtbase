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
// A generic thin wrapper around jobject, convertible to jobject.
// We need this as a baseclass so that QJniObject can be implicitly
// constructed from the various subclasses. We can also pass instances
// of this type (or of any of the generated subclasses) as if it was
// a jobject.
struct Object
{
    jobject _object;
    constexpr operator jobject() const { return _object; }
    operator QJniObject() const { return QJniObject(_object); }
};

} // namespace QtJniTypes

#define Q_DECLARE_JNI_TYPE_HELPER(Type)                         \
namespace QtJniTypes {                                          \
struct Type : Object                                            \
{                                                               \
    constexpr Type(jobject o) noexcept : Object{o} {}           \
};                                                              \
}                                                               \


#define Q_DECLARE_JNI_TYPE(Type, Signature)                     \
Q_DECLARE_JNI_TYPE_HELPER(Type)                                 \
template<>                                                      \
constexpr auto QtJniTypes::typeSignature<QtJniTypes::Type>()    \
{                                                               \
    static_assert((Signature[0] == 'L' || Signature[0] == '[')  \
                && Signature[sizeof(Signature) - 2] == ';',     \
                "Type signature needs to start with 'L' or '['" \
                " and end with ';'");                           \
    return QtJniTypes::CTString(Signature);                     \
}                                                               \

#define Q_DECLARE_JNI_CLASS(Type, Signature)                    \
Q_DECLARE_JNI_TYPE_HELPER(Type)                                 \
template<>                                                      \
constexpr auto QtJniTypes::className<QtJniTypes::Type>()        \
{                                                               \
    return QtJniTypes::CTString(Signature);                     \
}                                                               \
template<>                                                      \
constexpr auto QtJniTypes::typeSignature<QtJniTypes::Type>()    \
{                                                               \
    return QtJniTypes::CTString("L")                            \
         + QtJniTypes::CTString(Signature)                      \
         + QtJniTypes::CTString(";");                           \
}                                                               \

#define Q_DECLARE_JNI_NATIVE_METHOD(...)                           \
    QT_OVERLOADED_MACRO(QT_DECLARE_JNI_NATIVE_METHOD, __VA_ARGS__) \

#define QT_DECLARE_JNI_NATIVE_METHOD_2(Method, Name)            \
namespace QtJniMethods {                                        \
static constexpr auto Method##_signature =                      \
    QtJniTypes::nativeMethodSignature(Method);                  \
static const JNINativeMethod Method##_method = {                \
    #Name, Method##_signature.data(),                           \
    reinterpret_cast<void *>(Method)                            \
};                                                              \
}                                                               \

#define QT_DECLARE_JNI_NATIVE_METHOD_1(Method)                  \
    QT_DECLARE_JNI_NATIVE_METHOD_2(Method, Method)              \

#define Q_JNI_NATIVE_METHOD(Method) QtJniMethods::Method##_method

#define Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(...)                                        \
    QT_OVERLOADED_MACRO(QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE, __VA_ARGS__)              \

#define QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_2(Method, Name)                            \
    static inline constexpr auto Method##_signature = QtJniTypes::nativeMethodSignature(Method); \
    static inline const JNINativeMethod Method##_method = {                                      \
        #Name, Method##_signature.data(), reinterpret_cast<void *>(Method)                       \
    };

#define QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_1(Method)                                  \
    QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_2(Method, Method)                              \

#define Q_JNI_NATIVE_SCOPED_METHOD(Method, Scope) Scope::Method##_method

QT_END_NAMESPACE

#endif // defined(Q_QDOC) || defined(Q_OS_ANDROID)

#endif // QJNITYPES_H
