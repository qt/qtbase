/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <QtCore/qglobal.h>
#include <QtCore/qcontainerfwd.h>
#include <variant>
#include <optional>

#ifndef QTYPEINFO_H
#define QTYPEINFO_H

QT_BEGIN_NAMESPACE

/*
   QTypeInfo     - type trait functionality
*/

template <typename T>
static constexpr bool qIsRelocatable()
{
    return std::is_trivially_copyable<T>::value && std::is_trivially_destructible<T>::value;
}

template <typename T>
static constexpr bool qIsTrivial()
{
    return std::is_trivial<T>::value;
}

/*
  The catch-all template.
*/

template <typename T>
class QTypeInfo
{
public:
    enum {
        isPointer = false,
        isIntegral = std::is_integral<T>::value,
        isComplex = !qIsTrivial<T>(),
        isStatic = true,
        isRelocatable = qIsRelocatable<T>(),
        sizeOf = sizeof(T)
    };
};

template<>
class QTypeInfo<void>
{
public:
    enum {
        isPointer = false,
        isIntegral = false,
        isComplex = false,
        isStatic = false,
        isRelocatable = false,
        sizeOf = 0
    };
};

template <typename T>
class QTypeInfo<T*>
{
public:
    enum {
        isPointer = true,
        isIntegral = false,
        isComplex = false,
        isStatic = false,
        isRelocatable = true,
        sizeOf = sizeof(T*)
    };
};

/*!
    \class QTypeInfoQuery
    \inmodule QtCore
    \internal
    \brief QTypeInfoQuery is used to query the values of a given QTypeInfo<T>

    We use it because there may be some QTypeInfo<T> specializations in user
    code that don't provide certain flags that we added after Qt 5.0. They are:
    \list
      \li isRelocatable: defaults to !isStatic
    \endlist

    DO NOT specialize this class elsewhere.
*/
// apply defaults for a generic QTypeInfo<T> that didn't provide the new values
template <typename T, typename = void>
struct QTypeInfoQuery : public QTypeInfo<T>
{
    enum { isRelocatable = !QTypeInfo<T>::isStatic };
};

// if QTypeInfo<T>::isRelocatable exists, use it
template <typename T>
struct QTypeInfoQuery<T, typename std::enable_if<QTypeInfo<T>::isRelocatable || true>::type> : public QTypeInfo<T>
{};

/*!
    \class QTypeInfoMerger
    \inmodule QtCore
    \internal

    \brief QTypeInfoMerger merges the QTypeInfo flags of T1, T2... and presents them
    as a QTypeInfo<T> would do.

    Let's assume that we have a simple set of structs:

    \snippet code/src_corelib_global_qglobal.cpp 50

    To create a proper QTypeInfo specialization for A struct, we have to check
    all sub-components; B, C and D, then take the lowest common denominator and call
    Q_DECLARE_TYPEINFO with the resulting flags. An easier and less fragile approach is to
    use QTypeInfoMerger, which does that automatically. So struct A would have
    the following QTypeInfo definition:

    \snippet code/src_corelib_global_qglobal.cpp 51
*/
template <class T, class...Ts>
class QTypeInfoMerger
{
    static_assert(sizeof...(Ts) > 0);
public:
    static constexpr bool isComplex = ((QTypeInfoQuery<Ts>::isComplex) || ...);
    static constexpr bool isStatic =  ((QTypeInfoQuery<Ts>::isStatic) || ...);
    static constexpr bool isRelocatable = ((QTypeInfoQuery<Ts>::isRelocatable) && ...);
    static constexpr bool isPointer = false;
    static constexpr bool isIntegral = false;
    static constexpr std::size_t sizeOf = sizeof(T);
};

#define Q_DECLARE_MOVABLE_CONTAINER(CONTAINER) \
template <typename T> class CONTAINER; \
template <typename T> \
class QTypeInfo< CONTAINER<T> > \
{ \
public: \
    enum { \
        isPointer = false, \
        isIntegral = false, \
        isComplex = true, \
        isRelocatable = true, \
        isStatic = false, \
        sizeOf = sizeof(CONTAINER<T>) \
    }; \
}

Q_DECLARE_MOVABLE_CONTAINER(QList);
Q_DECLARE_MOVABLE_CONTAINER(QQueue);
Q_DECLARE_MOVABLE_CONTAINER(QStack);
Q_DECLARE_MOVABLE_CONTAINER(QSet);

#undef Q_DECLARE_MOVABLE_CONTAINER

#define Q_DECLARE_MOVABLE_CONTAINER(CONTAINER) \
template <typename K, typename V> class CONTAINER; \
template <typename K, typename V> \
class QTypeInfo< CONTAINER<K, V> > \
{ \
public: \
    enum { \
        isPointer = false, \
        isIntegral = false, \
        isComplex = true, \
        isStatic = false, \
        isRelocatable = true, \
        sizeOf = sizeof(CONTAINER<K, V>) \
    }; \
}

Q_DECLARE_MOVABLE_CONTAINER(QMap);
Q_DECLARE_MOVABLE_CONTAINER(QMultiMap);
Q_DECLARE_MOVABLE_CONTAINER(QHash);
Q_DECLARE_MOVABLE_CONTAINER(QMultiHash);

#undef Q_DECLARE_MOVABLE_CONTAINER

/*
   Specialize a specific type with:

     Q_DECLARE_TYPEINFO(type, flags);

   where 'type' is the name of the type to specialize and 'flags' is
   logically-OR'ed combination of the flags below.
*/
enum { /* TYPEINFO flags */
    Q_COMPLEX_TYPE = 0,
    Q_PRIMITIVE_TYPE = 0x1,
    Q_STATIC_TYPE = 0,
    Q_MOVABLE_TYPE = 0x2,               // ### Qt6: merge movable and relocatable once QList no longer depends on it
    Q_DUMMY_TYPE = 0x4,
    Q_RELOCATABLE_TYPE = 0x8
};

#define Q_DECLARE_TYPEINFO_BODY(TYPE, FLAGS) \
class QTypeInfo<TYPE > \
{ \
public: \
    enum { \
        isComplex = (((FLAGS) & Q_PRIMITIVE_TYPE) == 0) && !qIsTrivial<TYPE>(), \
        isStatic = (((FLAGS) & (Q_MOVABLE_TYPE | Q_PRIMITIVE_TYPE)) == 0), \
        isRelocatable = !isStatic || ((FLAGS) & Q_RELOCATABLE_TYPE) || qIsRelocatable<TYPE>(), \
        isPointer = false, \
        isIntegral = std::is_integral< TYPE >::value, \
        sizeOf = sizeof(TYPE) \
    }; \
    static inline const char *name() { return #TYPE; } \
}

#define Q_DECLARE_TYPEINFO(TYPE, FLAGS) \
template<> \
Q_DECLARE_TYPEINFO_BODY(TYPE, FLAGS)

/* Specialize QTypeInfo for QFlags<T> */
template<typename T> class QFlags;
template<typename T>
Q_DECLARE_TYPEINFO_BODY(QFlags<T>, Q_PRIMITIVE_TYPE);

/*
   Specialize a shared type with:

     Q_DECLARE_SHARED(type)

   where 'type' is the name of the type to specialize.  NOTE: shared
   types must define a member-swap, and be defined in the same
   namespace as Qt for this to work.

   If the type was already released without Q_DECLARE_SHARED applied,
   _and_ without an explicit Q_DECLARE_TYPEINFO(type, Q_MOVABLE_TYPE),
   then use Q_DECLARE_SHARED_NOT_MOVABLE_UNTIL_QT6(type) to mark the
   type shared (incl. swap()), without marking it movable (which
   would change the memory layout of QList, a BiC change.
*/

#define Q_DECLARE_SHARED_IMPL(TYPE, FLAGS) \
Q_DECLARE_TYPEINFO(TYPE, FLAGS); \
inline void swap(TYPE &value1, TYPE &value2) \
    noexcept(noexcept(value1.swap(value2))) \
{ value1.swap(value2); }
#define Q_DECLARE_SHARED(TYPE) Q_DECLARE_SHARED_IMPL(TYPE, Q_MOVABLE_TYPE)
#define Q_DECLARE_SHARED_NOT_MOVABLE_UNTIL_QT6(TYPE) \
                               Q_DECLARE_SHARED_IMPL(TYPE, QT_VERSION >= QT_VERSION_CHECK(6,0,0) ? Q_MOVABLE_TYPE : Q_RELOCATABLE_TYPE)

/*
   QTypeInfo primitive specializations
*/
Q_DECLARE_TYPEINFO(bool, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(char, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(signed char, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(uchar, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(short, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(ushort, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(int, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(uint, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(long, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(ulong, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(qint64, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(quint64, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(float, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(double, Q_PRIMITIVE_TYPE);

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
// ### Qt 6: remove the other branch
// This was required so that QList<T> for these types allocates out of the array storage
Q_DECLARE_TYPEINFO(long double, Q_PRIMITIVE_TYPE);
#  ifdef Q_COMPILER_UNICODE_STRINGS
Q_DECLARE_TYPEINFO(char16_t, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(char32_t, Q_PRIMITIVE_TYPE);
#  endif
#  if !defined(Q_CC_MSVC) || defined(_NATIVE_WCHAR_T_DEFINED)
Q_DECLARE_TYPEINFO(wchar_t, Q_PRIMITIVE_TYPE);
#  endif
#else
#  ifndef Q_OS_DARWIN
Q_DECLARE_TYPEINFO(long double, Q_PRIMITIVE_TYPE);
#  else
Q_DECLARE_TYPEINFO(long double, Q_RELOCATABLE_TYPE);
#  endif
#  ifdef Q_COMPILER_UNICODE_STRINGS
Q_DECLARE_TYPEINFO(char16_t, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(char32_t, Q_RELOCATABLE_TYPE);
#  endif
#  if !defined(Q_CC_MSVC) || defined(_NATIVE_WCHAR_T_DEFINED)
Q_DECLARE_TYPEINFO(wchar_t, Q_RELOCATABLE_TYPE);
#  endif
#endif // Qt 6

namespace QTypeTraits
{

/*
    The templates below aim to find out whether one can safely instantiate an operator==() or
    operator<() for a type.

    This is tricky for containers, as most containers have unconstrained comparison operators, even though they
    rely on the corresponding operators for its content.
    This is especially true for all of the STL template classes that have a comparison operator defined, and
    leads to the situation, that the compiler would try to instantiate the operator, and fail if any
    of its template arguments does not have the operator implemented.

    The code tries to cover the relevant cases for Qt and the STL, by checking (recusrsively) the value_type
    of a container (if it exists), and checking the template arguments of pair, tuple and variant.
*/
namespace detail {

// find out whether T is a conteiner
// this is required to check the value type of containers for the existence of the comparison operator
template <typename, typename = void>
struct is_container : std::false_type {};
template <typename T>
struct is_container<T, std::void_t<
        std::is_convertible<decltype(std::declval<T>().begin() != std::declval<T>().end()), bool>,
        typename T::value_type
>> : std::true_type {};


// Checks the existence of the comparison operator for the class itself
template <typename, typename = void>
struct has_operator_equal : std::false_type {};
template <typename T>
struct has_operator_equal<T, std::void_t<decltype(bool(std::declval<const T&>() == std::declval<const T&>()))>>
        : std::true_type {};

// Two forward declarations
template<typename T, bool = is_container<T>::value>
struct expand_operator_equal_container;
template<typename T>
struct expand_operator_equal_tuple;

// the entry point for the public method
template<typename T>
using expand_operator_equal = expand_operator_equal_container<T>;

// if T isn't a container check if it's a tuple like object
template<typename T, bool>
struct expand_operator_equal_container : expand_operator_equal_tuple<T> {};
// if T::value_type exists, check first T::value_type, then T itself
template<typename T>
struct expand_operator_equal_container<T, true> :
        std::conjunction<expand_operator_equal<typename T::value_type>, expand_operator_equal_tuple<T>> {};

// recursively check the template arguments of a tuple like object
template<typename ...T>
using expand_operator_equal_recursive = std::conjunction<expand_operator_equal<T>...>;

template<typename T>
struct expand_operator_equal_tuple : has_operator_equal<T> {};
template<typename T>
struct expand_operator_equal_tuple<std::optional<T>> : has_operator_equal<T> {};
template<typename T1, typename T2>
struct expand_operator_equal_tuple<std::pair<T1, T2>> : expand_operator_equal_recursive<T1, T2> {};
template<typename ...T>
struct expand_operator_equal_tuple<std::tuple<T...>> : expand_operator_equal_recursive<T...> {};
template<typename ...T>
struct expand_operator_equal_tuple<std::variant<T...>> : expand_operator_equal_recursive<T...> {};

// the same for operator<(), see above for explanations
template <typename, typename = void>
struct has_operator_less_than : std::false_type{};
template <typename T>
struct has_operator_less_than<T, std::void_t<decltype(bool(std::declval<const T&>() < std::declval<const T&>()))>>
        : std::true_type{};

template<typename T, bool = is_container<T>::value>
struct expand_operator_less_than_container;
template<typename T>
struct expand_operator_less_than_tuple;

template<typename T>
using expand_operator_less_than = expand_operator_less_than_container<T>;

template<typename T, bool>
struct expand_operator_less_than_container : expand_operator_less_than_tuple<T> {};
template<typename T>
struct expand_operator_less_than_container<T, true> :
        std::conjunction<expand_operator_less_than<typename T::value_type>, expand_operator_less_than_tuple<T>> {};

template<typename ...T>
using expand_operator_less_than_recursive = std::conjunction<expand_operator_less_than<T>...>;

template<typename T>
struct expand_operator_less_than_tuple : has_operator_less_than<T> {};
template<typename T1, typename T2>
struct expand_operator_less_than_tuple<std::pair<T1, T2>> : expand_operator_less_than_recursive<T1, T2> {};
template<typename ...T>
struct expand_operator_less_than_tuple<std::tuple<T...>> : expand_operator_less_than_recursive<T...> {};
template<typename ...T>
struct expand_operator_less_than_tuple<std::variant<T...>> : expand_operator_less_than_recursive<T...> {};

}

template<typename T>
struct has_operator_equal : detail::expand_operator_equal<T> {};
template<typename T>
constexpr bool has_operator_equal_v = has_operator_equal<T>::value;

template<typename T>
struct has_operator_less_than : detail::expand_operator_less_than<T> {};
template<typename T>
constexpr bool has_operator_less_than_v = has_operator_less_than<T>::value;

template <typename ...T>
using compare_eq_result = std::enable_if_t<std::conjunction_v<QTypeTraits::has_operator_equal<T>...>, bool>;

template <typename ...T>
using compare_lt_result = std::enable_if_t<std::conjunction_v<QTypeTraits::has_operator_less_than<T>...>, bool>;

}


QT_END_NAMESPACE
#endif // QTYPEINFO_H
