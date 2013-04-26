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

#include <QtCore/qatomic.h>

#ifndef QBASICATOMIC_H
#define QBASICATOMIC_H

#if defined(QT_BOOTSTRAPPED)
#  include <QtCore/qatomic_bootstrap.h>

// Compiler dependent implementation
#elif defined(Q_CC_MSVC)
#  include <QtCore/qatomic_msvc.h>

// Operating system dependent implementation
#elif defined(Q_OS_INTEGRITY)
#  include "QtCore/qatomic_integrity.h"
#elif defined(Q_OS_VXWORKS)
#  include "QtCore/qatomic_vxworks.h"

// Processor dependent implementation
#elif defined(Q_PROCESSOR_ALPHA)
#  include "QtCore/qatomic_alpha.h"
#elif defined(Q_PROCESSOR_ARM_V7)
# include "QtCore/qatomic_armv7.h"
#elif defined(Q_PROCESSOR_ARM_V6)
# include "QtCore/qatomic_armv6.h"
#elif defined(Q_PROCESSOR_ARM_V5)
# include "QtCore/qatomic_armv5.h"
#elif defined(Q_PROCESSOR_BFIN)
#  include "QtCore/qatomic_bfin.h"
#elif defined(Q_PROCESSOR_IA64)
#  include "QtCore/qatomic_ia64.h"
#elif defined(Q_PROCESSOR_MIPS)
#  include "QtCore/qatomic_mips.h"
#elif defined(Q_PROCESSOR_POWER)
#  include "QtCore/qatomic_power.h"
#elif defined(Q_PROCESSOR_S390)
#  include "QtCore/qatomic_s390.h"
#elif defined(Q_PROCESSOR_SH4A)
#  include "QtCore/qatomic_sh4a.h"
#elif defined(Q_PROCESSOR_SPARC)
#  include "QtCore/qatomic_sparc.h"
#elif defined(Q_PROCESSOR_X86)
#  include <QtCore/qatomic_x86.h>

// Fallback compiler dependent implementation
#elif defined(Q_COMPILER_ATOMICS) && defined(Q_COMPILER_CONSTEXPR)
#  include <QtCore/qatomic_cxx11.h>
#elif defined(Q_CC_GNU)
#  include <QtCore/qatomic_gcc.h>

// Fallback operating system dependent implementation
#elif defined(Q_OS_UNIX)
#  include <QtCore/qatomic_unix.h>

// No fallback
#else
#  error "Qt has not been ported to this platform"
#endif

// Only include if the implementation has been ported to QAtomicOps
#ifndef QOLDBASICATOMIC_H

QT_BEGIN_NAMESPACE

#if 0
// silence syncqt warnings
QT_END_NAMESPACE
#pragma qt_no_master_include
#pragma qt_sync_stop_processing
#endif

// New atomics

#if defined(Q_COMPILER_CONSTEXPR) && defined(Q_COMPILER_DEFAULT_MEMBERS) && defined(Q_COMPILER_DELETE_MEMBERS)
# if defined(Q_CC_CLANG) && ((((__clang_major__ * 100) + __clang_minor__) < 303) \
                             || defined(__apple_build_version__) \
                            )
   /* Do not define QT_BASIC_ATOMIC_HAS_CONSTRUCTORS for "stock" clang before version 3.3.
      Apple's version has different (higher!) version numbers, so disable it for all of them for now.
      (The only way to distinguish between them seems to be a check for __apple_build_version__ .)

      For details about the bug: see http://llvm.org/bugs/show_bug.cgi?id=12670
    */
# else
#  define QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
# endif
#endif

template <typename T>
class QBasicAtomicInteger
{
public:
    typedef QAtomicOps<T> Ops;
    // static check that this is a valid integer
    typedef char PermittedIntegerType[QAtomicIntegerTraits<T>::IsInteger ? 1 : -1];

    typename Ops::Type _q_value;

    // Non-atomic API
    T load() const Q_DECL_NOTHROW { return Ops::load(_q_value); }
    void store(T newValue) Q_DECL_NOTHROW { Ops::store(_q_value, newValue); }

    // Atomic API, implemented in qatomic_XXX.h

    T loadAcquire() const Q_DECL_NOTHROW { return Ops::loadAcquire(_q_value); }
    void storeRelease(T newValue) Q_DECL_NOTHROW { Ops::storeRelease(_q_value, newValue); }

    static Q_DECL_CONSTEXPR bool isReferenceCountingNative() Q_DECL_NOTHROW { return Ops::isReferenceCountingNative(); }
    static Q_DECL_CONSTEXPR bool isReferenceCountingWaitFree() Q_DECL_NOTHROW { return Ops::isReferenceCountingWaitFree(); }

    bool ref() Q_DECL_NOTHROW { return Ops::ref(_q_value); }
    bool deref() Q_DECL_NOTHROW { return Ops::deref(_q_value); }

    static Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return Ops::isTestAndSetNative(); }
    static Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return Ops::isTestAndSetWaitFree(); }

    bool testAndSetRelaxed(T expectedValue, T newValue) Q_DECL_NOTHROW
    { return Ops::testAndSetRelaxed(_q_value, expectedValue, newValue); }
    bool testAndSetAcquire(T expectedValue, T newValue) Q_DECL_NOTHROW
    { return Ops::testAndSetAcquire(_q_value, expectedValue, newValue); }
    bool testAndSetRelease(T expectedValue, T newValue) Q_DECL_NOTHROW
    { return Ops::testAndSetRelease(_q_value, expectedValue, newValue); }
    bool testAndSetOrdered(T expectedValue, T newValue) Q_DECL_NOTHROW
    { return Ops::testAndSetOrdered(_q_value, expectedValue, newValue); }

    static Q_DECL_CONSTEXPR bool isFetchAndStoreNative() Q_DECL_NOTHROW { return Ops::isFetchAndStoreNative(); }
    static Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree() Q_DECL_NOTHROW { return Ops::isFetchAndStoreWaitFree(); }

    T fetchAndStoreRelaxed(T newValue) Q_DECL_NOTHROW
    { return Ops::fetchAndStoreRelaxed(_q_value, newValue); }
    T fetchAndStoreAcquire(T newValue) Q_DECL_NOTHROW
    { return Ops::fetchAndStoreAcquire(_q_value, newValue); }
    T fetchAndStoreRelease(T newValue) Q_DECL_NOTHROW
    { return Ops::fetchAndStoreRelease(_q_value, newValue); }
    T fetchAndStoreOrdered(T newValue) Q_DECL_NOTHROW
    { return Ops::fetchAndStoreOrdered(_q_value, newValue); }

    static Q_DECL_CONSTEXPR bool isFetchAndAddNative() Q_DECL_NOTHROW { return Ops::isFetchAndAddNative(); }
    static Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree() Q_DECL_NOTHROW { return Ops::isFetchAndAddWaitFree(); }

    T fetchAndAddRelaxed(T valueToAdd) Q_DECL_NOTHROW
    { return Ops::fetchAndAddRelaxed(_q_value, valueToAdd); }
    T fetchAndAddAcquire(T valueToAdd) Q_DECL_NOTHROW
    { return Ops::fetchAndAddAcquire(_q_value, valueToAdd); }
    T fetchAndAddRelease(T valueToAdd) Q_DECL_NOTHROW
    { return Ops::fetchAndAddRelease(_q_value, valueToAdd); }
    T fetchAndAddOrdered(T valueToAdd) Q_DECL_NOTHROW
    { return Ops::fetchAndAddOrdered(_q_value, valueToAdd); }

#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    QBasicAtomicInteger() = default;
    constexpr QBasicAtomicInteger(T value) Q_DECL_NOTHROW : _q_value(value) {}
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
    Type load() const Q_DECL_NOTHROW { return _q_value; }
    void store(Type newValue) Q_DECL_NOTHROW { _q_value = newValue; }

    // Atomic API, implemented in qatomic_XXX.h
    Type loadAcquire() const Q_DECL_NOTHROW { return Ops::loadAcquire(_q_value); }
    void storeRelease(Type newValue) Q_DECL_NOTHROW { Ops::storeRelease(_q_value, newValue); }

    static Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return Ops::isTestAndSetNative(); }
    static Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return Ops::isTestAndSetWaitFree(); }

    bool testAndSetRelaxed(Type expectedValue, Type newValue) Q_DECL_NOTHROW
    { return Ops::testAndSetRelaxed(_q_value, expectedValue, newValue); }
    bool testAndSetAcquire(Type expectedValue, Type newValue) Q_DECL_NOTHROW
    { return Ops::testAndSetAcquire(_q_value, expectedValue, newValue); }
    bool testAndSetRelease(Type expectedValue, Type newValue) Q_DECL_NOTHROW
    { return Ops::testAndSetRelease(_q_value, expectedValue, newValue); }
    bool testAndSetOrdered(Type expectedValue, Type newValue) Q_DECL_NOTHROW
    { return Ops::testAndSetOrdered(_q_value, expectedValue, newValue); }

    static Q_DECL_CONSTEXPR bool isFetchAndStoreNative() Q_DECL_NOTHROW { return Ops::isFetchAndStoreNative(); }
    static Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree() Q_DECL_NOTHROW { return Ops::isFetchAndStoreWaitFree(); }

    Type fetchAndStoreRelaxed(Type newValue) Q_DECL_NOTHROW
    { return Ops::fetchAndStoreRelaxed(_q_value, newValue); }
    Type fetchAndStoreAcquire(Type newValue) Q_DECL_NOTHROW
    { return Ops::fetchAndStoreAcquire(_q_value, newValue); }
    Type fetchAndStoreRelease(Type newValue) Q_DECL_NOTHROW
    { return Ops::fetchAndStoreRelease(_q_value, newValue); }
    Type fetchAndStoreOrdered(Type newValue) Q_DECL_NOTHROW
    { return Ops::fetchAndStoreOrdered(_q_value, newValue); }

    static Q_DECL_CONSTEXPR bool isFetchAndAddNative() Q_DECL_NOTHROW { return Ops::isFetchAndAddNative(); }
    static Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree() Q_DECL_NOTHROW { return Ops::isFetchAndAddWaitFree(); }

    Type fetchAndAddRelaxed(qptrdiff valueToAdd) Q_DECL_NOTHROW
    { return Ops::fetchAndAddRelaxed(_q_value, valueToAdd); }
    Type fetchAndAddAcquire(qptrdiff valueToAdd) Q_DECL_NOTHROW
    { return Ops::fetchAndAddAcquire(_q_value, valueToAdd); }
    Type fetchAndAddRelease(qptrdiff valueToAdd) Q_DECL_NOTHROW
    { return Ops::fetchAndAddRelease(_q_value, valueToAdd); }
    Type fetchAndAddOrdered(qptrdiff valueToAdd) Q_DECL_NOTHROW
    { return Ops::fetchAndAddOrdered(_q_value, valueToAdd); }

#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    QBasicAtomicPointer() = default;
    constexpr QBasicAtomicPointer(Type value) Q_DECL_NOTHROW : _q_value(value) {}
    QBasicAtomicPointer(const QBasicAtomicPointer &) = delete;
    QBasicAtomicPointer &operator=(const QBasicAtomicPointer &) = delete;
    QBasicAtomicPointer &operator=(const QBasicAtomicPointer &) volatile = delete;
#endif
};

#ifndef Q_BASIC_ATOMIC_INITIALIZER
#  define Q_BASIC_ATOMIC_INITIALIZER(a) { (a) }
#endif

QT_END_NAMESPACE

#endif // QOLDBASICATOMIC_H

#endif // QBASICATOMIC_H
