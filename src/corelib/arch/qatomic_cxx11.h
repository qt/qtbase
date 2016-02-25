/****************************************************************************
**
** Copyright (C) 2011 Thiago Macieira <thiago@kde.org>
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

#ifndef QATOMIC_CXX11_H
#define QATOMIC_CXX11_H

#include <QtCore/qgenericatomic.h>
#include <atomic>

QT_BEGIN_NAMESPACE

#if 0
// silence syncqt warnings
QT_END_NAMESPACE
#pragma qt_sync_skip_header_check
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

template<> struct QAtomicOpsSupport<1> { enum { IsSupported = 1 }; };
template<> struct QAtomicOpsSupport<2> { enum { IsSupported = 1 }; };
template<> struct QAtomicOpsSupport<8> { enum { IsSupported = 1 }; };

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

#ifndef QT_NO_STD_ATOMIC64
#  define Q_ATOMIC_INT64_IS_SUPPORTED
#  define Q_ATOMIC_INT64_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define Q_ATOMIC_INT64_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define Q_ATOMIC_INT64_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define Q_ATOMIC_INT64_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#endif

template <typename X> struct QAtomicOps
{
    typedef std::atomic<X> Type;

    template <typename T> static inline
    T load(const std::atomic<T> &_q_value) Q_DECL_NOTHROW
    {
        return _q_value.load(std::memory_order_relaxed);
    }

    template <typename T> static inline
    T load(const volatile std::atomic<T> &_q_value) Q_DECL_NOTHROW
    {
        return _q_value.load(std::memory_order_relaxed);
    }

    template <typename T> static inline
    T loadAcquire(const std::atomic<T> &_q_value) Q_DECL_NOTHROW
    {
        return _q_value.load(std::memory_order_acquire);
    }

    template <typename T> static inline
    T loadAcquire(const volatile std::atomic<T> &_q_value) Q_DECL_NOTHROW
    {
        return _q_value.load(std::memory_order_acquire);
    }

    template <typename T> static inline
    void store(std::atomic<T> &_q_value, T newValue) Q_DECL_NOTHROW
    {
        _q_value.store(newValue, std::memory_order_relaxed);
    }

    template <typename T> static inline
    void storeRelease(std::atomic<T> &_q_value, T newValue) Q_DECL_NOTHROW
    {
        _q_value.store(newValue, std::memory_order_release);
    }

    static inline Q_DECL_CONSTEXPR bool isReferenceCountingNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isReferenceCountingWaitFree() Q_DECL_NOTHROW { return false; }
    template <typename T>
    static inline bool ref(std::atomic<T> &_q_value)
    {
        return ++_q_value != 0;
    }

    template <typename T>
    static inline bool deref(std::atomic<T> &_q_value) Q_DECL_NOTHROW
    {
        return --_q_value != 0;
    }

    static inline Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return false; }
    static inline Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return false; }

    template <typename T>
    static bool testAndSetRelaxed(std::atomic<T> &_q_value, T expectedValue, T newValue, T *currentValue = Q_NULLPTR) Q_DECL_NOTHROW
    {
        bool tmp = _q_value.compare_exchange_strong(expectedValue, newValue, std::memory_order_relaxed);
        if (currentValue)
            *currentValue = expectedValue;
        return tmp;
    }

    template <typename T>
    static bool testAndSetAcquire(std::atomic<T> &_q_value, T expectedValue, T newValue, T *currentValue = Q_NULLPTR) Q_DECL_NOTHROW
    {
        bool tmp = _q_value.compare_exchange_strong(expectedValue, newValue, std::memory_order_acquire);
        if (currentValue)
            *currentValue = expectedValue;
        return tmp;
    }

    template <typename T>
    static bool testAndSetRelease(std::atomic<T> &_q_value, T expectedValue, T newValue, T *currentValue = Q_NULLPTR) Q_DECL_NOTHROW
    {
        bool tmp = _q_value.compare_exchange_strong(expectedValue, newValue, std::memory_order_release);
        if (currentValue)
            *currentValue = expectedValue;
        return tmp;
    }

    template <typename T>
    static bool testAndSetOrdered(std::atomic<T> &_q_value, T expectedValue, T newValue, T *currentValue = Q_NULLPTR) Q_DECL_NOTHROW
    {
        bool tmp = _q_value.compare_exchange_strong(expectedValue, newValue, std::memory_order_acq_rel);
        if (currentValue)
            *currentValue = expectedValue;
        return tmp;
    }

    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreNative() Q_DECL_NOTHROW { return false; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree() Q_DECL_NOTHROW { return false; }

    template <typename T>
    static T fetchAndStoreRelaxed(std::atomic<T> &_q_value, T newValue) Q_DECL_NOTHROW
    {
        return _q_value.exchange(newValue, std::memory_order_relaxed);
    }

    template <typename T>
    static T fetchAndStoreAcquire(std::atomic<T> &_q_value, T newValue) Q_DECL_NOTHROW
    {
        return _q_value.exchange(newValue, std::memory_order_acquire);
    }

    template <typename T>
    static T fetchAndStoreRelease(std::atomic<T> &_q_value, T newValue) Q_DECL_NOTHROW
    {
        return _q_value.exchange(newValue, std::memory_order_release);
    }

    template <typename T>
    static T fetchAndStoreOrdered(std::atomic<T> &_q_value, T newValue) Q_DECL_NOTHROW
    {
        return _q_value.exchange(newValue, std::memory_order_acq_rel);
    }

    static inline Q_DECL_CONSTEXPR bool isFetchAndAddNative() Q_DECL_NOTHROW { return false; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree() Q_DECL_NOTHROW { return false; }

    template <typename T> static inline
    T fetchAndAddRelaxed(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_add(valueToAdd, std::memory_order_relaxed);
    }

    template <typename T> static inline
    T fetchAndAddAcquire(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_add(valueToAdd, std::memory_order_acquire);
    }

    template <typename T> static inline
    T fetchAndAddRelease(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_add(valueToAdd, std::memory_order_release);
    }

    template <typename T> static inline
    T fetchAndAddOrdered(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_add(valueToAdd, std::memory_order_acq_rel);
    }

    template <typename T> static inline
    T fetchAndSubRelaxed(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_sub(valueToAdd, std::memory_order_relaxed);
    }

    template <typename T> static inline
    T fetchAndSubAcquire(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_sub(valueToAdd, std::memory_order_acquire);
    }

    template <typename T> static inline
    T fetchAndSubRelease(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_sub(valueToAdd, std::memory_order_release);
    }

    template <typename T> static inline
    T fetchAndSubOrdered(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_sub(valueToAdd, std::memory_order_acq_rel);
    }

    template <typename T> static inline
    T fetchAndAndRelaxed(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_and(valueToAdd, std::memory_order_relaxed);
    }

    template <typename T> static inline
    T fetchAndAndAcquire(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_and(valueToAdd, std::memory_order_acquire);
    }

    template <typename T> static inline
    T fetchAndAndRelease(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_and(valueToAdd, std::memory_order_release);
    }

    template <typename T> static inline
    T fetchAndAndOrdered(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_and(valueToAdd, std::memory_order_acq_rel);
    }

    template <typename T> static inline
    T fetchAndOrRelaxed(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_or(valueToAdd, std::memory_order_relaxed);
    }

    template <typename T> static inline
    T fetchAndOrAcquire(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_or(valueToAdd, std::memory_order_acquire);
    }

    template <typename T> static inline
    T fetchAndOrRelease(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_or(valueToAdd, std::memory_order_release);
    }

    template <typename T> static inline
    T fetchAndOrOrdered(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_or(valueToAdd, std::memory_order_acq_rel);
    }

    template <typename T> static inline
    T fetchAndXorRelaxed(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_xor(valueToAdd, std::memory_order_relaxed);
    }

    template <typename T> static inline
    T fetchAndXorAcquire(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_xor(valueToAdd, std::memory_order_acquire);
    }

    template <typename T> static inline
    T fetchAndXorRelease(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_xor(valueToAdd, std::memory_order_release);
    }

    template <typename T> static inline
    T fetchAndXorOrdered(std::atomic<T> &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return _q_value.fetch_xor(valueToAdd, std::memory_order_acq_rel);
    }
};

#if defined(Q_COMPILER_CONSTEXPR) && defined(Q_COMPILER_DEFAULT_MEMBERS) && defined(Q_COMPILER_DELETE_MEMBERS)
#  define Q_BASIC_ATOMIC_INITIALIZER(a)     { a }
#else
#  define Q_BASIC_ATOMIC_INITIALIZER(a)     { ATOMIC_VAR_INIT(a) }
#endif

QT_END_NAMESPACE

#endif // QATOMIC_CXX0X_H
