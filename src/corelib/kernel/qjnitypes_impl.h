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
template<> struct IsStringType<const char*, 0> : std::true_type {};
template<size_t N> struct IsStringType<CTString<N>> : std::true_type {};
template<size_t N> struct IsStringType<const char[N]> : std::true_type {};

template<bool flag = false>
static void staticAssertTypeMismatch()
{
    static_assert(flag, "The used type is not supported by this template call. "
                        "Use a JNI based type instead.");
}

template<bool flag = false>
static void staticAssertClassNotRegistered()
{
    static_assert(flag, "Class not registered, use Q_DECLARE_JNI_CLASS");
}

template <typename T>
struct Traits {
    // The return type of className/signature becomes void for any type
    // not handled here. This indicates that the Traits type is not specialized
    // for the respective type, which we use to detect invalid types in the
    // ValidSignatureTypes and ValidFieldType predicates below.

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
        } else if constexpr (std::is_same_v<T, long>) {
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
        }
        // else: return void -> not implemented
    }
};

// compatibility until submodules are ported
template <typename T>
constexpr auto typeSignature()
{
    return Traits<T>::signature();
}

template <typename T>
constexpr auto className()
{
    return Traits<T>::className();
}

// have to use the compatibility functions here until porting is complete
template<typename T>
static constexpr bool isPrimitiveType()
{
    return typeSignature<T>().size() == 2;
}

template<typename T>
static constexpr bool isArrayType()
{
    constexpr auto signature = typeSignature<T>();
    return signature.startsWith('[') && signature.size() > 2;
}

template<typename T>
static constexpr bool isObjectType()
{
    if constexpr (std::is_convertible_v<T, jobject>) {
        return true;
    } else {
        constexpr auto signature = typeSignature<T>();
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

// A set of types is valid if typeSignature is implemented for all of them
template<typename ...Types>
constexpr bool ValidSignatureTypesDetail = !std::disjunction<std::is_same<
                                                    decltype(QtJniTypes::typeSignature<Types>()),
                                                    void>...,
                                                    IsStringType<Types>...>::value;
template<typename ...Types>
using ValidSignatureTypes = std::enable_if_t<
    ValidSignatureTypesDetail<q20::remove_cvref_t<Types>...>, bool>;

template<typename Type>
constexpr bool ValidFieldTypeDetail = isObjectType<Type>() || isPrimitiveType<Type>();
template<typename Type>
using ValidFieldType = std::enable_if_t<
    ValidFieldTypeDetail<q20::remove_cvref_t<Type>>, bool>;


template<typename R, typename ...Args, ValidSignatureTypes<R, Args...> = true>
static constexpr auto methodSignature()
{
    return (CTString("(") +
                ... + typeSignature<q20::remove_cvref_t<Args>>())
            + CTString(")")
            + typeSignature<R>();
}

template<typename T, ValidSignatureTypes<T> = true>
static constexpr auto fieldSignature()
{
    return QtJniTypes::typeSignature<T>();
}

template<typename ...Args, ValidSignatureTypes<Args...> = true>
static constexpr auto constructorSignature()
{
    return methodSignature<void, Args...>();
}

template<typename Ret, typename ...Args, ValidSignatureTypes<Ret, Args...> = true>
static constexpr auto nativeMethodSignature(Ret (*)(JNIEnv *, jobject, Args...))
{
    return methodSignature<Ret, Args...>();
}

template<typename Ret, typename ...Args, ValidSignatureTypes<Ret, Args...> = true>
static constexpr auto nativeMethodSignature(Ret (*)(JNIEnv *, jclass, Args...))
{
    return methodSignature<Ret, Args...>();
}

} // namespace QtJniTypes

QT_END_NAMESPACE

#endif

#endif // QJNITYPES_IMPL_H
