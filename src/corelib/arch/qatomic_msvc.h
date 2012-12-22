/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QATOMIC_MSVC_H
#define QATOMIC_MSVC_H

#include <QtCore/qgenericatomic.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef Q_OS_WINCE

// use compiler intrinsics for all atomic functions
# define QT_INTERLOCKED_PREFIX _
# define QT_INTERLOCKED_PROTOTYPE __cdecl
# define QT_INTERLOCKED_DECLARE_PROTOTYPES
# define QT_INTERLOCKED_INTRINSIC

#else // Q_OS_WINCE

# if _WIN32_WCE < 0x600 && defined(_X86_)
// For X86 Windows CE, include winbase.h to catch inline functions which
// override the regular definitions inside of coredll.dll.
// Though one could use the original version of Increment/Decrement, others are
// not exported at all.
#  include <winbase.h>

// It's safer to remove the volatile and let the compiler add it as needed.
#  define QT_INTERLOCKED_VOLATILE

# else // _WIN32_WCE >= 0x600 || !_X86_

#  define QT_INTERLOCKED_PROTOTYPE __cdecl
#  define QT_INTERLOCKED_DECLARE_PROTOTYPES

#  if _WIN32_WCE >= 0x600
#   if defined(_X86_)
#    define QT_INTERLOCKED_PREFIX _
#    define QT_INTERLOCKED_INTRINSIC
#   endif
#  else
#   define QT_INTERLOCKED_VOLATILE
#  endif

# endif // _WIN32_WCE >= 0x600 || !_X86_

#endif // Q_OS_WINCE

////////////////////////////////////////////////////////////////////////////////////////////////////
// Prototype declaration

#define QT_INTERLOCKED_CONCAT_I(prefix, suffix) \
    prefix ## suffix
#define QT_INTERLOCKED_CONCAT(prefix, suffix) \
    QT_INTERLOCKED_CONCAT_I(prefix, suffix)

// MSVC intrinsics prefix function names with an underscore. Also, if platform
// SDK headers have been included, the Interlocked names may be defined as
// macros.
// To avoid double underscores, we paste the prefix with Interlocked first and
// then the remainder of the function name.
#define QT_INTERLOCKED_FUNCTION(name) \
    QT_INTERLOCKED_CONCAT( \
        QT_INTERLOCKED_CONCAT(QT_INTERLOCKED_PREFIX, Interlocked), name)

#ifndef QT_INTERLOCKED_VOLATILE
# define QT_INTERLOCKED_VOLATILE volatile
#endif

#ifndef QT_INTERLOCKED_PREFIX
#define QT_INTERLOCKED_PREFIX
#endif

#ifndef QT_INTERLOCKED_PROTOTYPE
#define QT_INTERLOCKED_PROTOTYPE
#endif

#ifdef QT_INTERLOCKED_DECLARE_PROTOTYPES
#undef QT_INTERLOCKED_DECLARE_PROTOTYPES

extern "C" {

    long QT_INTERLOCKED_PROTOTYPE QT_INTERLOCKED_FUNCTION( Increment )(long QT_INTERLOCKED_VOLATILE *);
    long QT_INTERLOCKED_PROTOTYPE QT_INTERLOCKED_FUNCTION( Decrement )(long QT_INTERLOCKED_VOLATILE *);
    long QT_INTERLOCKED_PROTOTYPE QT_INTERLOCKED_FUNCTION( CompareExchange )(long QT_INTERLOCKED_VOLATILE *, long, long);
    long QT_INTERLOCKED_PROTOTYPE QT_INTERLOCKED_FUNCTION( Exchange )(long QT_INTERLOCKED_VOLATILE *, long);
    long QT_INTERLOCKED_PROTOTYPE QT_INTERLOCKED_FUNCTION( ExchangeAdd )(long QT_INTERLOCKED_VOLATILE *, long);

# if !defined(Q_OS_WINCE) && !defined(__i386__) && !defined(_M_IX86)
    void * QT_INTERLOCKED_FUNCTION( CompareExchangePointer )(void * QT_INTERLOCKED_VOLATILE *, void *, void *);
    void * QT_INTERLOCKED_FUNCTION( ExchangePointer )(void * QT_INTERLOCKED_VOLATILE *, void *);
    __int64 QT_INTERLOCKED_FUNCTION( ExchangeAdd64 )(__int64 QT_INTERLOCKED_VOLATILE *, __int64);
# endif

}

#endif // QT_INTERLOCKED_DECLARE_PROTOTYPES

#undef QT_INTERLOCKED_PROTOTYPE

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef QT_INTERLOCKED_INTRINSIC
#undef QT_INTERLOCKED_INTRINSIC

# pragma intrinsic (_InterlockedIncrement)
# pragma intrinsic (_InterlockedDecrement)
# pragma intrinsic (_InterlockedExchange)
# pragma intrinsic (_InterlockedCompareExchange)
# pragma intrinsic (_InterlockedExchangeAdd)

# if !defined(Q_OS_WINCE) && !defined(_M_IX86)
#  pragma intrinsic (_InterlockedCompareExchangePointer)
#  pragma intrinsic (_InterlockedExchangePointer)
#  pragma intrinsic (_InterlockedExchangeAdd64)
# endif

#endif // QT_INTERLOCKED_INTRINSIC

////////////////////////////////////////////////////////////////////////////////////////////////////
// Interlocked* replacement macros

#define QT_INTERLOCKED_INCREMENT(value) \
    QT_INTERLOCKED_FUNCTION(Increment)(value)

#define QT_INTERLOCKED_DECREMENT(value) \
    QT_INTERLOCKED_FUNCTION(Decrement)(value)

#define QT_INTERLOCKED_COMPARE_EXCHANGE(value, newValue, expectedValue) \
    QT_INTERLOCKED_FUNCTION(CompareExchange)((value), (newValue), (expectedValue))

#define QT_INTERLOCKED_EXCHANGE(value, newValue) \
    QT_INTERLOCKED_FUNCTION(Exchange)((value), (newValue))

#define QT_INTERLOCKED_EXCHANGE_ADD(value, valueToAdd) \
    QT_INTERLOCKED_FUNCTION(ExchangeAdd)((value), (valueToAdd))

#if defined(Q_OS_WINCE) || defined(__i386__) || defined(_M_IX86)

# define QT_INTERLOCKED_COMPARE_EXCHANGE_POINTER(value, newValue, expectedValue) \
    reinterpret_cast<void *>( \
        QT_INTERLOCKED_FUNCTION(CompareExchange)( \
                reinterpret_cast<long QT_INTERLOCKED_VOLATILE *>(value), \
                long(newValue), \
                long(expectedValue)))

# define QT_INTERLOCKED_EXCHANGE_POINTER(value, newValue) \
    QT_INTERLOCKED_FUNCTION(Exchange)( \
            reinterpret_cast<long QT_INTERLOCKED_VOLATILE *>(value), \
            long(newValue))

# define QT_INTERLOCKED_EXCHANGE_ADD_POINTER(value, valueToAdd) \
    QT_INTERLOCKED_FUNCTION(ExchangeAdd)( \
            reinterpret_cast<long QT_INTERLOCKED_VOLATILE *>(value), \
            (valueToAdd))

#else // !defined(Q_OS_WINCE) && !defined(__i386__) && !defined(_M_IX86)

# define QT_INTERLOCKED_COMPARE_EXCHANGE_POINTER(value, newValue, expectedValue) \
    QT_INTERLOCKED_FUNCTION(CompareExchangePointer)( \
            (void * QT_INTERLOCKED_VOLATILE *)(value), \
            (void *) (newValue), \
            (void *) (expectedValue))

# define QT_INTERLOCKED_EXCHANGE_POINTER(value, newValue) \
    QT_INTERLOCKED_FUNCTION(ExchangePointer)( \
            (void * QT_INTERLOCKED_VOLATILE *)(value), \
            (void *) (newValue))

# define QT_INTERLOCKED_EXCHANGE_ADD_POINTER(value, valueToAdd) \
    QT_INTERLOCKED_FUNCTION(ExchangeAdd64)( \
            reinterpret_cast<qint64 QT_INTERLOCKED_VOLATILE *>(value), \
            (valueToAdd))

#endif // !defined(Q_OS_WINCE) && !defined(__i386__) && !defined(_M_IX86)

////////////////////////////////////////////////////////////////////////////////////////////////////

QT_BEGIN_NAMESPACE

#if 0
// silence syncqt warnings
QT_END_NAMESPACE
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_WAIT_FREE

#define Q_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_WAIT_FREE

#define Q_ATOMIC_INT32_IS_SUPPORTED

#define Q_ATOMIC_INT32_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_REFERENCE_COUNTING_IS_WAIT_FREE

#define Q_ATOMIC_INT32_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_INT32_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_INT32_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_FETCH_AND_ADD_IS_WAIT_FREE

#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_WAIT_FREE

////////////////////////////////////////////////////////////////////////////////////////////////////

template<> struct QAtomicIntegerTraits<int> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<unsigned int> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<long> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<unsigned long> { enum { IsInteger = 1 }; };
#ifdef Q_COMPILER_UNICODE_STRINGS
template<> struct QAtomicIntegerTraits<char32_t> { enum { IsInteger = 1 }; };
#endif

// No definition, needs specialization
template <int N> struct QAtomicOpsBySize;

template <>
struct QAtomicOpsBySize<4> : QGenericAtomicOps<QAtomicOpsBySize<4> >
{
    // The 32-bit Interlocked*() API takes parameters as longs.
    typedef long Type;

    static inline Q_DECL_CONSTEXPR bool isReferenceCountingNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isReferenceCountingWaitFree() Q_DECL_NOTHROW { return true; }
    static bool ref(long &_q_value) Q_DECL_NOTHROW;
    static bool deref(long &_q_value) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return true; }
    static bool testAndSetRelaxed(long &_q_value, long expectedValue, long newValue) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree() Q_DECL_NOTHROW { return true; }
    static long fetchAndStoreRelaxed(long &_q_value, long newValue) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isFetchAndAddNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree() Q_DECL_NOTHROW { return true; }
    static long fetchAndAddRelaxed(long &_q_value, QAtomicAdditiveType<long>::AdditiveT valueToAdd) Q_DECL_NOTHROW;
};

template <typename T>
struct QAtomicOps : QAtomicOpsBySize<sizeof(T)>
{ };

inline bool QAtomicOpsBySize<4>::ref(long &_q_value) Q_DECL_NOTHROW
{
    return QT_INTERLOCKED_INCREMENT(&_q_value) != 0;
}

inline bool QAtomicOpsBySize<4>::deref(long &_q_value) Q_DECL_NOTHROW
{
    return QT_INTERLOCKED_DECREMENT(&_q_value) != 0;
}

inline bool QAtomicOpsBySize<4>::testAndSetRelaxed(long &_q_value, long expectedValue, long newValue) Q_DECL_NOTHROW
{
    return QT_INTERLOCKED_COMPARE_EXCHANGE(&_q_value, newValue, expectedValue) == expectedValue;
}

inline long QAtomicOpsBySize<4>::fetchAndStoreRelaxed(long &_q_value, long newValue) Q_DECL_NOTHROW
{
    return QT_INTERLOCKED_EXCHANGE(&_q_value, newValue);
}

inline long QAtomicOpsBySize<4>::fetchAndAddRelaxed(long &_q_value, QAtomicAdditiveType<long>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    return QT_INTERLOCKED_EXCHANGE_ADD(&_q_value, valueToAdd * QAtomicAdditiveType<long>::AddScale);
}

// Specialization for pointer types, since we have Interlocked*Pointer() variants in some configurations
template <typename T>
struct QAtomicOps<T *> : QGenericAtomicOps<QAtomicOps<T *> >
{
    typedef T *Type;

    static inline Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return true; }
    static bool testAndSetRelaxed(T *&_q_value, T *expectedValue, T *newValue) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree() Q_DECL_NOTHROW { return true; }
    static T *fetchAndStoreRelaxed(T *&_q_value, T *newValue) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isFetchAndAddNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree() Q_DECL_NOTHROW { return true; }
    static T *fetchAndAddRelaxed(T *&_q_value, qptrdiff valueToAdd) Q_DECL_NOTHROW;
};

template <typename T>
inline bool QAtomicOps<T *>::testAndSetRelaxed(T *&_q_value, T *expectedValue, T *newValue) Q_DECL_NOTHROW
{
    return QT_INTERLOCKED_COMPARE_EXCHANGE_POINTER(&_q_value, newValue, expectedValue) == expectedValue;
}

template <typename T>
inline T *QAtomicOps<T *>::fetchAndStoreRelaxed(T *&_q_value, T *newValue) Q_DECL_NOTHROW
{
    return reinterpret_cast<T *>(QT_INTERLOCKED_EXCHANGE_POINTER(&_q_value, newValue));
}

template <typename T>
inline T *QAtomicOps<T *>::fetchAndAddRelaxed(T *&_q_value, qptrdiff valueToAdd) Q_DECL_NOTHROW
{
    return reinterpret_cast<T *>(QT_INTERLOCKED_EXCHANGE_ADD_POINTER(&_q_value, valueToAdd * sizeof(T)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Cleanup

#undef QT_INTERLOCKED_CONCAT_I
#undef QT_INTERLOCKED_CONCAT
#undef QT_INTERLOCKED_FUNCTION
#undef QT_INTERLOCKED_PREFIX

#undef QT_INTERLOCKED_VOLATILE

#undef QT_INTERLOCKED_INCREMENT
#undef QT_INTERLOCKED_DECREMENT
#undef QT_INTERLOCKED_COMPARE_EXCHANGE
#undef QT_INTERLOCKED_EXCHANGE
#undef QT_INTERLOCKED_EXCHANGE_ADD
#undef QT_INTERLOCKED_COMPARE_EXCHANGE_POINTER
#undef QT_INTERLOCKED_EXCHANGE_POINTER
#undef QT_INTERLOCKED_EXCHANGE_ADD_POINTER

QT_END_NAMESPACE
#endif // QATOMIC_MSVC_H
