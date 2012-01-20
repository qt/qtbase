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

#ifndef QBASICATOMIC_H
#define QBASICATOMIC_H

#include <QtCore/qglobal.h>

#  define QT_OLD_ATOMICS

#ifdef QT_OLD_ATOMICS
# include "qoldbasicatomic.h"
# undef QT_OLD_ATOMICS
#else

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#if 0
#pragma qt_no_master_include
#pragma qt_sync_stop_processing
#endif

// New atomics

template <typename T>
class QBasicAtomicInteger
{
public:
    typedef QAtomicOps<T> Ops;
    // static check that this is a valid integer
    typedef char PermittedIntegerType[QAtomicIntegerTraits<T>::IsInteger ? 1 : -1];

    typename Ops::Type _q_value;

    // Non-atomic API
    T load() const { return Ops::load(_q_value); }
    void store(T newValue) { Ops::store(_q_value, newValue); }

    // Atomic API, implemented in qatomic_XXX.h

    T loadAcquire() { return Ops::loadAcquire(_q_value); }
    void storeRelease(T newValue) { Ops::storeRelease(_q_value, newValue); }

    static bool isReferenceCountingNative() { return Ops::isReferenceCountingNative(); }
    static bool isReferenceCountingWaitFree() { return Ops::isReferenceCountingWaitFree(); }

    bool ref() { return Ops::ref(_q_value); }
    bool deref() { return Ops::deref(_q_value); }

    static bool isTestAndSetNative() { return Ops::isTestAndSetNative(); }
    static bool isTestAndSetWaitFree() { return Ops::isTestAndSetWaitFree(); }

    bool testAndSetRelaxed(T expectedValue, T newValue)
    { return Ops::testAndSetRelaxed(_q_value, expectedValue, newValue); }
    bool testAndSetAcquire(T expectedValue, T newValue)
    { return Ops::testAndSetAcquire(_q_value, expectedValue, newValue); }
    bool testAndSetRelease(T expectedValue, T newValue)
    { return Ops::testAndSetRelease(_q_value, expectedValue, newValue); }
    bool testAndSetOrdered(T expectedValue, T newValue)
    { return Ops::testAndSetOrdered(_q_value, expectedValue, newValue); }

    static bool isFetchAndStoreNative() { return Ops::isFetchAndStoreNative(); }
    static bool isFetchAndStoreWaitFree() { return Ops::isFetchAndStoreWaitFree(); }

    T fetchAndStoreRelaxed(T newValue)
    { return Ops::fetchAndStoreRelaxed(_q_value, newValue); }
    T fetchAndStoreAcquire(T newValue)
    { return Ops::fetchAndStoreAcquire(_q_value, newValue); }
    T fetchAndStoreRelease(T newValue)
    { return Ops::fetchAndStoreRelease(_q_value, newValue); }
    T fetchAndStoreOrdered(T newValue)
    { return Ops::fetchAndStoreOrdered(_q_value, newValue); }

    static bool isFetchAndAddNative() { return Ops::isFetchAndAddNative(); }
    static bool isFetchAndAddWaitFree() { return Ops::isFetchAndAddWaitFree(); }

    T fetchAndAddRelaxed(T valueToAdd)
    { return Ops::fetchAndAddRelaxed(_q_value, valueToAdd); }
    T fetchAndAddAcquire(T valueToAdd)
    { return Ops::fetchAndAddAcquire(_q_value, valueToAdd); }
    T fetchAndAddRelease(T valueToAdd)
    { return Ops::fetchAndAddRelease(_q_value, valueToAdd); }
    T fetchAndAddOrdered(T valueToAdd)
    { return Ops::fetchAndAddOrdered(_q_value, valueToAdd); }

#if defined(Q_COMPILER_CONSTEXPR) && defined(Q_COMPILER_DEFAULT_DELETE_MEMBERS)
    QBasicAtomicInteger() = default;
    constexpr QBasicAtomicInteger(T value) : _q_value(value) {}
    QBasicAtomicInteger(const QBasicAtomicInteger &) = delete;
    QBasicAtomicInteger &operator=(const QBasicAtomicInteger &) = delete;
    QBasicAtomicInteger &operator=(const QBasicAtomicInteger &) volatile = delete;
#endif
};
typedef QBasicAtomicInteger<int> QBasicAtomicInt;

template <typename X>
class QBasicAtomicPointer
{
public:
    typedef X *Type;
    typedef QAtomicOps<Type> Ops;
    typedef typename Ops::Type AtomicType;

    AtomicType _q_value;

    // Non-atomic API
    Type load() const { return _q_value; }
    void store(Type newValue) { _q_value = newValue; }

    // Atomic API, implemented in qatomic_XXX.h
    Type loadAcquire() { return Ops::loadAcquire(_q_value); }
    void storeRelease(Type newValue) { Ops::storeRelease(_q_value, newValue); }

    static bool isTestAndSetNative() { return Ops::isTestAndSetNative(); }
    static bool isTestAndSetWaitFree() { return Ops::isTestAndSetWaitFree(); }

    bool testAndSetRelaxed(Type expectedValue, Type newValue)
    { return Ops::testAndSetRelaxed(_q_value, expectedValue, newValue); }
    bool testAndSetAcquire(Type expectedValue, Type newValue)
    { return Ops::testAndSetAcquire(_q_value, expectedValue, newValue); }
    bool testAndSetRelease(Type expectedValue, Type newValue)
    { return Ops::testAndSetRelease(_q_value, expectedValue, newValue); }
    bool testAndSetOrdered(Type expectedValue, Type newValue)
    { return Ops::testAndSetOrdered(_q_value, expectedValue, newValue); }

    static bool isFetchAndStoreNative() { return Ops::isFetchAndStoreNative(); }
    static bool isFetchAndStoreWaitFree() { return Ops::isFetchAndStoreWaitFree(); }

    Type fetchAndStoreRelaxed(Type newValue)
    { return Ops::fetchAndStoreRelaxed(_q_value, newValue); }
    Type fetchAndStoreAcquire(Type newValue)
    { return Ops::fetchAndStoreAcquire(_q_value, newValue); }
    Type fetchAndStoreRelease(Type newValue)
    { return Ops::fetchAndStoreRelease(_q_value, newValue); }
    Type fetchAndStoreOrdered(Type newValue)
    { return Ops::fetchAndStoreOrdered(_q_value, newValue); }

    static bool isFetchAndAddNative() { return Ops::isFetchAndAddNative(); }
    static bool isFetchAndAddWaitFree() { return Ops::isFetchAndAddWaitFree(); }

    Type fetchAndAddRelaxed(qptrdiff valueToAdd)
    { return Ops::fetchAndAddRelaxed(_q_value, valueToAdd); }
    Type fetchAndAddAcquire(qptrdiff valueToAdd)
    { return Ops::fetchAndAddAcquire(_q_value, valueToAdd); }
    Type fetchAndAddRelease(qptrdiff valueToAdd)
    { return Ops::fetchAndAddRelease(_q_value, valueToAdd); }
    Type fetchAndAddOrdered(qptrdiff valueToAdd)
    { return Ops::fetchAndAddOrdered(_q_value, valueToAdd); }

#if defined(Q_COMPILER_CONSTEXPR) && defined(Q_COMPILER_DEFAULT_DELETE_MEMBERS)
    QBasicAtomicPointer() = default;
    constexpr QBasicAtomicPointer(Type value) : _q_value(value) {}
    QBasicAtomicPointer(const QBasicAtomicPointer &) = delete;
    QBasicAtomicPointer &operator=(const QBasicAtomicPointer &) = delete;
    QBasicAtomicPointer &operator=(const QBasicAtomicPointer &) volatile = delete;
#endif
};

#ifndef Q_BASIC_ATOMIC_INITIALIZER
#  define Q_BASIC_ATOMIC_INITIALIZER(a) { (a) }
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_OLD_ATOMICS

#endif // QBASIC_ATOMIC
