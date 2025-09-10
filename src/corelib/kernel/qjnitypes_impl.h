// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QJNITYPES_IMPL_H
#define QJNITYPES_IMPL_H

#include <QtCore/qglobal.h>
#include <QtCore/q20type_traits.h>

#if defined(Q_QDOC) || defined(Q_OS_ANDROID)
#include <jni.h>

QT_BEGIN_NAMESPACE

namespace QtJniTypes
{

// a constexpr type for string literals of any character width, aware of the length
// of the string.
template<size_t N_WITH_NULL, typename BaseType = char>
struct CTString
{
    BaseType m_data[N_WITH_NULL] = {};

    constexpr CTString() noexcept {}
    // Can be instantiated (only) with a string literal
    constexpr explicit CTString(const BaseType (&data)[N_WITH_NULL]) noexcept
    {
        for (size_t i = 0; i < N_WITH_NULL - 1; ++i)
            m_data[i] = data[i];
    }

    constexpr BaseType at(size_t i) const { return m_data[i]; }
    constexpr BaseType operator[](size_t i) const { return at(i); }
    static constexpr size_t size() noexcept { return N_WITH_NULL; }
    constexpr operator const BaseType *() const noexcept { return m_data; }
    constexpr const BaseType *data() const noexcept { return m_data; }
    template<size_t N2_WITH_NULL>
    constexpr bool startsWith(const BaseType (&lit)[N2_WITH_NULL]) const noexcept
    {
        if constexpr (N2_WITH_NULL > N_WITH_NULL) {
            return false;
        } else {
            for (size_t i = 0; i < N2_WITH_NULL - 1; ++i) {
                if (m_data[i] != lit[i])
                    return false;
            }
        }
        return true;
    }
    constexpr bool startsWith(BaseType c) const noexcept
    {
        return N_WITH_NULL > 1 && m_data[0] == c;
    }
    template<size_t N2_WITH_NULL>
    constexpr bool endsWith(const BaseType (&lit)[N2_WITH_NULL]) const noexcept
    {
        if constexpr (N2_WITH_NULL > N_WITH_NULL) {
            return false;
        } else {
            for (size_t i = 0; i < N2_WITH_NULL; ++i) {
                if (m_data[N_WITH_NULL - i - 1] != lit[N2_WITH_NULL - i - 1])
                    return false;
            }
        }
        return true;
    }
    constexpr bool endsWith(BaseType c) const noexcept
    {
        return N_WITH_NULL > 1 && m_data[N_WITH_NULL - 2] == c;
    }

    template<size_t N2_WITH_NULL>
    friend inline constexpr bool operator==(const CTString<N_WITH_NULL> &lhs,
                                            const CTString<N2_WITH_NULL> &rhs) noexcept
    {
        if constexpr (N_WITH_NULL != N2_WITH_NULL) {
            return false;
        } else {
            for (size_t i = 0; i < N_WITH_NULL - 1; ++i) {
                if (lhs.at(i) != rhs.at(i))
                    return false;
            }
        }
        return true;
    }

    template<size_t N2_WITH_NULL>
    friend inline constexpr bool operator!=(const CTString<N_WITH_NULL> &lhs,
                                            const CTString<N2_WITH_NULL> &rhs) noexcept
    {
        return !operator==(lhs, rhs);
    }

    template<size_t N2_WITH_NULL>
    friend inline constexpr bool operator==(const CTString<N_WITH_NULL> &lhs,
                                            const BaseType (&rhs)[N2_WITH_NULL]) noexcept
    {
        return operator==(lhs, CTString<N2_WITH_NULL>(rhs));
    }
    template<size_t N2_WITH_NULL>
    friend inline constexpr bool operator==(const BaseType (&lhs)[N2_WITH_NULL],
                                            const CTString<N_WITH_NULL> &rhs) noexcept
    {
        return operator==(CTString<N2_WITH_NULL>(lhs), rhs);
    }

    template<size_t N2_WITH_NULL>
    friend inline constexpr bool operator!=(const CTString<N_WITH_NULL> &lhs,
                                            const BaseType (&rhs)[N2_WITH_NULL]) noexcept
    {
        return operator!=(lhs, CTString<N2_WITH_NULL>(rhs));
    }
    template<size_t N2_WITH_NULL>
    friend inline constexpr bool operator!=(const BaseType (&lhs)[N2_WITH_NULL],
                                            const CTString<N_WITH_NULL> &rhs) noexcept
    {
        return operator!=(CTString<N2_WITH_NULL>(lhs), rhs);
    }

    template<size_t N2_WITH_NULL>
    friend inline constexpr auto operator+(const CTString<N_WITH_NULL> &lhs,
                                           const CTString<N2_WITH_NULL> &rhs) noexcept
    {
        char data[N_WITH_NULL + N2_WITH_NULL - 1] = {};
        for (size_t i = 0; i < N_WITH_NULL - 1; ++i)
            data[i] = lhs[i];
        for (size_t i = 0; i < N2_WITH_NULL - 1; ++i)
            data[N_WITH_NULL - 1 + i] = rhs[i];
        return CTString<N_WITH_NULL + N2_WITH_NULL - 1>(data);
    }
};

// Helper types that allow us to disable variadic overloads that would conflict
// with overloads that take a const char*.
template<typename T, size_t N = 0> struct IsStringType : std::false_type {};
template<> struct IsStringType<const char *, 0> : std::true_type {};
template<> struct IsStringType<const char *&, 0> : std::true_type {};
template<size_t N> struct IsStringType<CTString<N>> : std::true_type {};
template<size_t N> struct IsStringType<const char[N]> : std::true_type {};
template<size_t N> struct IsStringType<const char(&)[N]> : std::true_type {};
template<size_t N> struct IsStringType<char[N]> : std::true_type {};

template <typename T>
struct Traits {
    // The return type of className/signature becomes void for any type
    // not handled here. This indicates that the Traits type is not specialized
    // for the respective type, which we use to detect invalid types in the
    // IfValidSignatureTypes and IfValidFieldType predicates below.

    static constexpr auto className()
    {
        if constexpr (std::is_same_v<T, jstring>)
            return CTString("java/lang/String");
        else if constexpr (std::is_same_v<T, jobject>)
            return CTString("java/lang/Object");
        else if constexpr (std::is_same_v<T, jclass>)
            return CTString("java/lang/Class");
        else if constexpr (std::is_same_v<T, jthrowable>)
            return CTString("java/lang/Throwable");
        // else: return void -> not implemented
    }

    static constexpr auto signature()
    {
        if constexpr (!std::is_same_v<decltype(className()), void>) {
            // the type signature of any object class is L<className>;
            return CTString("L") + className() + CTString(";");
        } else if constexpr (std::is_array_v<T>) {
            using UnderlyingType = typename std::remove_extent_t<T>;
            static_assert(!std::is_array_v<UnderlyingType>,
                        "Traits::signature() does not handle multi-dimensional arrays");
            return CTString("[") + Traits<UnderlyingType>::signature();
        } else if constexpr (std::is_same_v<T, jobjectArray>) {
            return CTString("[Ljava/lang/Object;");
        } else if constexpr (std::is_same_v<T, jbooleanArray>) {
            return CTString("[Z");
        } else if constexpr (std::is_same_v<T, jbyteArray>) {
            return CTString("[B");
        } else if constexpr (std::is_same_v<T, jshortArray>) {
            return CTString("[S");
        } else if constexpr (std::is_same_v<T, jintArray>) {
            return CTString("[I");
        } else if constexpr (std::is_same_v<T, jlongArray>) {
            return CTString("[J");
        } else if constexpr (std::is_same_v<T, jfloatArray>) {
            return CTString("[F");
        } else if constexpr (std::is_same_v<T, jdoubleArray>) {
            return CTString("[D");
        } else if constexpr (std::is_same_v<T, jcharArray>) {
            return CTString("[C");
        } else if constexpr (std::is_same_v<T, jboolean>) {
            return CTString("Z");
        } else if constexpr (std::is_same_v<T, bool>) {
            return CTString("Z");
        } else if constexpr (std::is_same_v<T, jbyte>) {
            return CTString("B");
        } else if constexpr (std::is_same_v<T, jchar>) {
            return CTString("C");
        } else if constexpr (std::is_same_v<T, char>) {
            return CTString("C");
        } else if constexpr (std::is_same_v<T, jshort>) {
            return CTString("S");
        } else if constexpr (std::is_same_v<T, short>) {
            return CTString("S");
        } else if constexpr (std::is_same_v<T, jint>) {
            return CTString("I");
        } else if constexpr (std::is_same_v<T, int>) {
            return CTString("I");
        } else if constexpr (std::is_same_v<T, uint>) {
            return CTString("I");
        } else if constexpr (std::is_same_v<T, jlong>) {
            return CTString("J");
        } else if constexpr (std::is_same_v<T, quint64>) {
            return CTString("J");
        } else if constexpr (std::is_same_v<T, jfloat>) {
            return CTString("F");
        } else if constexpr (std::is_same_v<T, float>) {
            return CTString("F");
        } else if constexpr (std::is_same_v<T, jdouble>) {
            return CTString("D");
        } else if constexpr (std::is_same_v<T, double>) {
            return CTString("D");
        } else if constexpr (std::is_same_v<T, void>) {
            return CTString("V");
        } else if constexpr (std::is_enum_v<T>) {
            return Traits<std::underlying_type_t<T>>::signature();
        } else if constexpr (std::is_same_v<T, QString>) {
            return CTString("Ljava/lang/String;");
        }
        // else: return void -> not implemented
    }
};

template <typename Have, typename Want>
static constexpr bool sameTypeForJni = (QtJniTypes::Traits<Have>::signature()
                                        == QtJniTypes::Traits<Want>::signature())
                                    && (sizeof(Have) == sizeof(Want));

template <typename, typename = void>
struct Caller
{};

#define MAKE_CALLER(Type, Method) \
template <typename T> \
struct Caller<T, std::enable_if_t<sameTypeForJni<T, Type>>> \
{ \
    static constexpr void callMethodForType(JNIEnv *env, T &res, jobject obj, jmethodID id, va_list args) \
    { \
        res = T(env->Call##Method##MethodV(obj, id, args)); \
    } \
    static constexpr void callStaticMethodForType(JNIEnv *env, T &res, jclass clazz, jmethodID id, va_list args) \
    { \
        res = T(env->CallStatic##Method##MethodV(clazz, id, args)); \
    } \
    static constexpr void getFieldForType(JNIEnv *env, T &res, jobject obj, jfieldID id) \
    { \
        res = T(env->Get##Method##Field(obj, id)); \
    } \
    static constexpr void getStaticFieldForType(JNIEnv *env, T &res, jclass clazz, jfieldID id) \
    { \
        res = T(env->GetStatic##Method##Field(clazz, id)); \
    } \
    static constexpr void setFieldForType(JNIEnv *env, jobject obj, jfieldID id, T value) \
    { \
        env->Set##Method##Field(obj, id, static_cast<Type>(value)); \
    } \
    static constexpr void setStaticFieldForType(JNIEnv *env, jclass clazz, jfieldID id, T value) \
    { \
        env->SetStatic##Method##Field(clazz, id, static_cast<Type>(value)); \
    } \
}

MAKE_CALLER(jboolean, Boolean);
MAKE_CALLER(jbyte, Byte);
MAKE_CALLER(jchar, Char);
MAKE_CALLER(jshort, Short);
MAKE_CALLER(jint, Int);
MAKE_CALLER(jlong, Long);
MAKE_CALLER(jfloat, Float);
MAKE_CALLER(jdouble, Double);

#undef MAKE_CALLER

template<typename T>
static constexpr bool isPrimitiveType()
{
    return Traits<T>::signature().size() == 2;
}

template<typename T>
static constexpr bool isArrayType()
{
    constexpr auto signature = Traits<T>::signature();
    return signature.startsWith('[') && signature.size() > 2;
}

template<typename T>
static constexpr bool isObjectType()
{
    if constexpr (std::is_convertible_v<T, jobject>) {
        return true;
    } else {
        constexpr auto signature = Traits<T>::signature();
        return (signature.startsWith('L') && signature.endsWith(';')) || isArrayType<T>();
    }
}

template<typename T>
static constexpr void assertObjectType()
{
    static_assert(isObjectType<T>(),
                  "Type needs to be a JNI object type (convertible to jobject, or with "
                  "an object type signature registered)!");
}

// A set of types is valid if Traits::signature is implemented for all of them
template<typename ...Types>
constexpr bool ValidSignatureTypesDetail = !std::disjunction<std::is_same<
                                                    decltype(Traits<Types>::signature()),
                                                    void>...,
                                                    IsStringType<Types>...>::value;
template<typename ...Types>
using IfValidSignatureTypes = std::enable_if_t<
    ValidSignatureTypesDetail<q20::remove_cvref_t<Types>...>, bool>;

template<typename Type>
constexpr bool ValidFieldTypeDetail = isObjectType<Type>() || isPrimitiveType<Type>();
template<typename Type>
using IfValidFieldType = std::enable_if_t<
    ValidFieldTypeDetail<q20::remove_cvref_t<Type>>, bool>;


template<typename R, typename ...Args, IfValidSignatureTypes<R, Args...> = true>
static constexpr auto methodSignature()
{
    return (CTString("(") +
                ... + Traits<q20::remove_cvref_t<Args>>::signature())
            + CTString(")")
            + Traits<R>::signature();
}

template<typename T, IfValidSignatureTypes<T> = true>
static constexpr auto fieldSignature()
{
    return QtJniTypes::Traits<T>::signature();
}

template<typename ...Args, IfValidSignatureTypes<Args...> = true>
static constexpr auto constructorSignature()
{
    return methodSignature<void, Args...>();
}

template<typename Ret, typename ...Args, IfValidSignatureTypes<Ret, Args...> = true>
static constexpr auto nativeMethodSignature(Ret (*)(JNIEnv *, jobject, Args...))
{
    return methodSignature<Ret, Args...>();
}

template<typename Ret, typename ...Args, IfValidSignatureTypes<Ret, Args...> = true>
static constexpr auto nativeMethodSignature(Ret (*)(JNIEnv *, jclass, Args...))
{
    return methodSignature<Ret, Args...>();
}

} // namespace QtJniTypes

QT_END_NAMESPACE

#endif

#endif // QJNITYPES_IMPL_H
