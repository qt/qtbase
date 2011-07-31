/****************************************************************************
**
** Copyright (C) 2011 Thiago Macieira <thiago@kde.org>
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QATOMIC_CXX11_H
#define QATOMIC_CXX11_H

#include <QtCore/qgenericatomic.h>
#include <atomic>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

#if 0
#pragma qt_sync_stop_processing
#endif

#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#define Q_ATOMIC_INT_TEST_AND_SET_IS_SOMETIMES_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

#define Q_ATOMIC_INT32_IS_SUPPORTED
#define Q_ATOMIC_INT32_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#define Q_ATOMIC_INT32_TEST_AND_SET_IS_SOMETIMES_NATIVE
#define Q_ATOMIC_INT32_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#define Q_ATOMIC_INT32_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

#define Q_ATOMIC_POINTER_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

template<> struct QAtomicIntegerTraits<int> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<unsigned int> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<char> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<signed char> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<unsigned char> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<short> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<unsigned short> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<long> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<unsigned long> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<long long> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<unsigned long long> { enum { IsInteger = 1 }; };

#define Q_ATOMIC_INT8_IS_SUPPORTED
#define Q_ATOMIC_INT8_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT8_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT8_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT8_FETCH_AND_ADD_IS_ALWAYS_NATIVE

#define Q_ATOMIC_INT16_IS_SUPPORTED
#define Q_ATOMIC_INT16_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT16_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT16_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT16_FETCH_AND_ADD_IS_ALWAYS_NATIVE

#define Q_ATOMIC_INT64_IS_SUPPORTED
#define Q_ATOMIC_INT64_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_FETCH_AND_ADD_IS_ALWAYS_NATIVE

template <typename T> struct QAtomicOps
{
    typedef std::atomic<T> Type;
    typedef typename QAtomicAdditiveType<T>::AdditiveT _AdditiveType;
    static const int AddScale = QAtomicAdditiveType<T>::AddScale;

    static void acquireMemoryFence() { }
    static void releaseMemoryFence() { }
    static void orderedMemoryFence() { }

    static inline
    T load(const Type &_q_value)
    {
        return _q_value.load(std::memory_order_relaxed);
    }

    static inline
    T load(const volatile Type &_q_value)
    {
        return _q_value.load(std::memory_order_relaxed);
    }

    static inline
    T loadAcquire(const Type &_q_value)
    {
        return _q_value.load(std::memory_order_acquire);
    }

    static inline
    T loadAcquire(const volatile Type &_q_value)
    {
        return _q_value.load(std::memory_order_acquire);
    }

    static inline
    void store(Type &_q_value, T newValue)
    {
        _q_value.store(newValue, std::memory_order_relaxed);
    }

    static inline
    void storeRelease(Type &_q_value, T newValue)
    {
        _q_value.store(newValue, std::memory_order_release);
    }

    static inline bool isReferenceCountingNative() { return true; }
    static inline bool isReferenceCountingWaitFree() { return false; }
    static inline bool ref(Type &_q_value)
    {
        return ++_q_value != 0;
    }

    static inline bool deref(Type &_q_value)
    {
        return --_q_value != 0;
    }

    static inline bool isTestAndSetNative() { return false; }
    static inline bool isTestAndSetWaitFree() { return false; }

    static
    bool testAndSetRelaxed(Type &_q_value, T expectedValue, T newValue)
    {
        return _q_value.compare_exchange_strong(expectedValue, newValue, std::memory_order_relaxed);
    }

    static bool testAndSetAcquire(Type &_q_value, T expectedValue, T newValue)
    {
        return _q_value.compare_exchange_strong(expectedValue, newValue, std::memory_order_acquire);
    }

    static bool testAndSetRelease(Type &_q_value, T expectedValue, T newValue)
    {
        return _q_value.compare_exchange_strong(expectedValue, newValue, std::memory_order_release);
    }

    static bool testAndSetOrdered(Type &_q_value, T expectedValue, T newValue)
    {
        return _q_value.compare_exchange_strong(expectedValue, newValue, std::memory_order_acq_rel);
    }

    static inline bool isFetchAndStoreNative() { return false; }
    static inline bool isFetchAndStoreWaitFree() { return false; }

    static T fetchAndStoreRelaxed(Type &_q_value, T newValue)
    {
        return _q_value.exchange(newValue, std::memory_order_relaxed);
    }

    static T fetchAndStoreAcquire(Type &_q_value, T newValue)
    {
        return _q_value.exchange(newValue, std::memory_order_acquire);
    }

    static T fetchAndStoreRelease(Type &_q_value, T newValue)
    {
        return _q_value.exchange(newValue, std::memory_order_release);
    }

    static T fetchAndStoreOrdered(Type &_q_value, T newValue)
    {
        return _q_value.exchange(newValue, std::memory_order_acq_rel);
    }

    static inline bool isFetchAndAddNative() { return false; }
    static inline bool isFetchAndAddWaitFree() { return false; }

    static
    T fetchAndAddRelaxed(Type &_q_value, _AdditiveType valueToAdd)
    {
        return _q_value.fetch_add(valueToAdd * AddScale,
                                  std::memory_order_relaxed);
    }

    static
    T fetchAndAddAcquire(Type &_q_value, _AdditiveType valueToAdd)
    {
        return _q_value.fetch_add(valueToAdd * AddScale,
                                  std::memory_order_acquire);
    }

    static
    T fetchAndAddRelease(Type &_q_value, _AdditiveType valueToAdd)
    {
        return _q_value.fetch_add(valueToAdd * AddScale,
                                  std::memory_order_release);
    }

    static
    T fetchAndAddOrdered(Type &_q_value, _AdditiveType valueToAdd)
    {
        return _q_value.fetch_add(valueToAdd * AddScale,
                                  std::memory_order_acq_rel);
    }
};

#ifdef ATOMIC_VAR_INIT
# define Q_BASIC_ATOMIC_INITIALIZER(a)   { ATOMIC_VAR_INIT(a) }
#else
# define Q_BASIC_ATOMIC_INITIALIZER(a)   { {a} }
#endif

QT_END_NAMESPACE
QT_END_HEADER

#endif // QATOMIC_CXX0X_H
