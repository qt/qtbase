/****************************************************************************
**
** Copyright (C) 2011 Thiago Macieira <thiago@kde.org>
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

#ifndef QATOMIC_GCC_H
#define QATOMIC_GCC_H

#include <QtCore/qgenericatomic.h>

QT_BEGIN_NAMESPACE

#if 0
// silence syncqt warnings
QT_END_NAMESPACE
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

template<> struct QAtomicIntegerTraits<int> { enum { IsInteger = 1 }; };
template<> struct QAtomicIntegerTraits<unsigned int> { enum { IsInteger = 1 }; };
#ifdef Q_COMPILER_UNICODE_STRINGS
template<> struct QAtomicIntegerTraits<char32_t> { enum { IsInteger = 1 }; };
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

template <typename T> struct QAtomicOps: QGenericAtomicOps<QAtomicOps<T> >
{
    // The GCC intrinsics all have fully-ordered memory semantics, so we define
    // only the xxxRelaxed functions. The exception is __sync_lock_and_test,
    // which has acquire semantics, so we need to define the Release and
    // Ordered versions too.

    typedef T Type;

#ifndef __ia64__
    static T loadAcquire(const T &_q_value) Q_DECL_NOTHROW
    {
        T tmp = _q_value;
        __sync_synchronize();
        return tmp;
    }

    static void storeRelease(T &_q_value, T newValue) Q_DECL_NOTHROW
    {
        __sync_synchronize();
        _q_value = newValue;
    }
#endif

    static Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return false; }
    static Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return false; }
    static bool testAndSetRelaxed(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
    {
        return __sync_bool_compare_and_swap(&_q_value, expectedValue, newValue);
    }

    static T fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW
    {
        return __sync_lock_test_and_set(&_q_value, newValue);
    }

    static T fetchAndStoreRelease(T &_q_value, T newValue) Q_DECL_NOTHROW
    {
        __sync_synchronize();
        return __sync_lock_test_and_set(&_q_value, newValue);
    }

    static T fetchAndStoreOrdered(T &_q_value, T newValue) Q_DECL_NOTHROW
    {
        return fetchAndStoreRelease(_q_value, newValue);
    }

    static
    T fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
    {
        return __sync_fetch_and_add(&_q_value, valueToAdd * QAtomicAdditiveType<T>::AddScale);
    }
};

QT_END_NAMESPACE
#endif // QATOMIC_GCC_H
