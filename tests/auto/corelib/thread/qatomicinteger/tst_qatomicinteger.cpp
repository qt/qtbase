/****************************************************************************
**
** Copyright (C) 2013 Intel Corporation
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest>
#include <QAtomicInt>

#include <limits>
#include <limits.h>
#include <wchar.h>

#if !defined(Q_ATOMIC_INT32_IS_SUPPORTED)
#  error "QAtomicInteger for 32-bit types must be supported!"
#endif
#if QT_POINTER_SIZE == 8 && !defined(Q_ATOMIC_INT64_IS_SUPPORTED)
#  error "QAtomicInteger for 64-bit types must be supported on 64-bit builds!"
#endif

// always supported types:
#define TYPE_SUPPORTED_int          1
#define TYPE_SUPPORTED_uint         1
#define TYPE_SUPPORTED_long         1
#define TYPE_SUPPORTED_ulong        1
#define TYPE_SUPPORTED_qptrdiff     1
#define TYPE_SUPPORTED_quintptr     1
#if (defined(__SIZEOF_WCHAR_T__) && (__SIZEOF_WCHAR_T__-0) > 2) \
    || (defined(WCHAR_MAX) && (WCHAR_MAX-0 > 0x10000))
#  define TYPE_SUPPORTED_wchar_t    1
#endif
#ifdef Q_COMPILER_UNICODE_STRINGS
#  define TYPE_SUPPORTED_char32_t   1
#endif

#ifdef Q_ATOMIC_INT8_IS_SUPPORTED
#  define TYPE_SUPPORTED_char       1
#  define TYPE_SUPPORTED_uchar      1
#  define TYPE_SUPPORTED_schar      1
#endif
#ifdef Q_ATOMIC_INT16_IS_SUPPORTED
#  define TYPE_SUPPORTED_short      1
#  define TYPE_SUPPORTED_ushort     1
#  ifdef Q_COMPILER_UNICODE_STRINGS
#    define TYPE_SUPPORTED_char16_t 1
#  endif
#  ifndef TYPE_SUPPORTED_wchar_t
#    define TYPE_SUPPORTED_wchar_t  1
#  endif
#endif
#ifdef Q_ATOMIC_INT64_IS_SUPPORTED
#  define TYPE_SUPPORTED_qlonglong  1
#  define TYPE_SUPPORTED_qulonglong 1
#endif

#ifdef Q_MOC_RUN
#  define QATOMIC_TYPE_SUPPORTED(type)      1
#else
#  define QATOMIC_TYPE_SUPPORTED2(type)     TYPE_SUPPORTED_ ## type
#  define QATOMIC_TYPE_SUPPORTED(type)      QATOMIC_TYPE_SUPPORTED2(type)
#endif // Q_MOC_RUN

#if QATOMIC_TYPE_SUPPORTED(QATOMIC_TEST_TYPE)
#  define TEST_TYPE QATOMIC_TEST_TYPE
#else
#  define TEST_TYPE int
#  define QATOMIC_TEST_NOT_SUPPORTED
#endif

#if defined(Q_CC_GNU) && !defined(Q_CC_INTEL)
#  pragma GCC diagnostic ignored "-Wtype-limits"
#  pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#if defined(Q_CC_CLANG) && !defined(Q_CC_INTEL)
#  pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#endif

typedef signed char schar;

typedef TEST_TYPE Type;
typedef Type T; // shorthand
enum {
    TypeIsUnsigned = Type(-1) > Type(0),
    TypeIsSigned = !TypeIsUnsigned
};

template <bool> struct LargeIntTemplate;
template <>     struct LargeIntTemplate<true>  { typedef quint64 Type; };
template <>     struct LargeIntTemplate<false> { typedef qint64 Type; };
typedef LargeIntTemplate<TypeIsUnsigned>::Type LargeInt;

class tst_QAtomicIntegerXX : public QObject
{
    Q_OBJECT

    void addData();

private Q_SLOTS:
    void initTestCase();
    void static_checks();

    void constructor_data() { addData(); }
    void constructor();

    void copy_data() { addData(); }
    void copy();

    void assign_data() { addData(); }
    void assign();

    void operatorInteger_data() { addData(); }
    void operatorInteger();

    void loadAcquireStoreRelease_data() { addData(); }
    void loadAcquireStoreRelease();

    void refDeref_data() { addData(); }
    void refDeref();

    void testAndSet_data() { addData(); }
    void testAndSet();

    void testAndSet3_data() { addData(); }
    void testAndSet3();

    void fetchAndStore_data() { addData(); }
    void fetchAndStore();

    void fetchAndAdd_data() { addData(); }
    void fetchAndAdd();

    void fetchAndSub_data() { addData(); }
    void fetchAndSub();

    void addSub_data() { addData(); }
    void addSub();

    void fetchAndOr_data() { addData(); }
    void fetchAndOr();

    void fetchAndAnd_data() { addData(); }
    void fetchAndAnd();

    void fetchAndXor_data() { addData(); }
    void fetchAndXor();
};

template <bool> inline void booleanHelper() { }
struct TypeInStruct { TEST_TYPE type; };

void tst_QAtomicIntegerXX::static_checks()
{
    Q_STATIC_ASSERT(sizeof(QAtomicInteger<T>) == sizeof(T));
    Q_STATIC_ASSERT(Q_ALIGNOF(QAtomicInteger<T>) == Q_ALIGNOF(TypeInStruct));

    // statements with no effect
    (void) QAtomicInteger<T>::isReferenceCountingNative();
    (void) QAtomicInteger<T>::isReferenceCountingWaitFree();
    (void) QAtomicInteger<T>::isTestAndSetNative();
    (void) QAtomicInteger<T>::isTestAndSetWaitFree();
    (void) QAtomicInteger<T>::isFetchAndStoreNative();
    (void) QAtomicInteger<T>::isFetchAndStoreWaitFree();
    (void) QAtomicInteger<T>::isFetchAndAddNative();
    (void) QAtomicInteger<T>::isFetchAndAddWaitFree();

#ifdef Q_COMPILER_CONSTEXPR
    // this is a compile-time test only
    booleanHelper<QAtomicInteger<T>::isReferenceCountingNative()>();
    booleanHelper<QAtomicInteger<T>::isReferenceCountingWaitFree()>();
    booleanHelper<QAtomicInteger<T>::isTestAndSetNative()>();
    booleanHelper<QAtomicInteger<T>::isTestAndSetWaitFree()>();
    booleanHelper<QAtomicInteger<T>::isFetchAndStoreNative()>();
    booleanHelper<QAtomicInteger<T>::isFetchAndStoreWaitFree()>();
    booleanHelper<QAtomicInteger<T>::isFetchAndAddNative()>();
    booleanHelper<QAtomicInteger<T>::isFetchAndAddWaitFree()>();
#endif
}

void tst_QAtomicIntegerXX::addData()
{
    typedef std::numeric_limits<T> Limits;
    QTest::addColumn<LargeInt>("value");
    QTest::newRow("0") << LargeInt(0);
    QTest::newRow("+1") << LargeInt(1);
    QTest::newRow("42") << LargeInt(42);
    if (TypeIsSigned) {
        QTest::newRow("-1") << qint64(-1);
        QTest::newRow("-47") << qint64(-47);
    }

    // exercise bits
    if (TypeIsSigned && Limits::min() < qint64(SCHAR_MIN))
        QTest::newRow("int8_min") << qint64(SCHAR_MIN);
    if (Limits::max() > LargeInt(SCHAR_MAX))
        QTest::newRow("int8_max") << LargeInt(SCHAR_MAX);
    if (Limits::max() > LargeInt(UCHAR_MAX))
        QTest::newRow("uint8_max") << LargeInt(UCHAR_MAX);
    if (TypeIsSigned && Limits::min() < -qint64(UCHAR_MAX))
        QTest::newRow("-uint8_max") << -qint64(UCHAR_MAX);
    if (Limits::max() > LargeInt(SHRT_MAX))
        QTest::newRow("int16_max") << LargeInt(SHRT_MAX);
    if (TypeIsSigned && Limits::min() < qint64(SHRT_MIN))
        QTest::newRow("int16_min") << qint64(SHRT_MIN);
    if (Limits::max() > LargeInt(USHRT_MAX))
        QTest::newRow("uint16_max") << LargeInt(USHRT_MAX);
    if (TypeIsSigned && Limits::min() < -qint64(USHRT_MAX))
        QTest::newRow("-uint16_max") << -qint64(USHRT_MAX);
    if (Limits::max() > LargeInt(INT_MAX))
        QTest::newRow("int32_max") << LargeInt(INT_MAX);
    if (TypeIsSigned && Limits::min() < qint64(INT_MIN))
        QTest::newRow("int32_min") << qint64(INT_MIN);
    if (Limits::max() > LargeInt(UINT_MAX))
        QTest::newRow("uint32_max") << LargeInt(UINT_MAX);
    if (Limits::max() > LargeInt(std::numeric_limits<qint64>::max()))
        QTest::newRow("int64_max") << LargeInt(std::numeric_limits<qint64>::max());
    if (TypeIsSigned && Limits::min() < -qint64(UINT_MAX))
        QTest::newRow("-uint32_max") << -qint64(UINT_MAX);

    if (TypeIsSigned)
        QTest::newRow(QT_STRINGIFY(QATOMIC_TEST_TYPE) "_min") << qint64(Limits::min());
    QTest::newRow(QT_STRINGIFY(QATOMIC_TEST_TYPE) "_max") << LargeInt(Limits::max());
}

void tst_QAtomicIntegerXX::initTestCase()
{
#ifdef QATOMIC_TEST_NOT_SUPPORTED
    QSKIP("QAtomicInteger<" QT_STRINGIFY(QATOMIC_TEST_TYPE) "> is not supported on this platform");
#endif
}

void tst_QAtomicIntegerXX::constructor()
{
    QFETCH(LargeInt, value);

    QAtomicInteger<T> atomic(value);
    QCOMPARE(atomic.load(), T(value));

    QAtomicInteger<T> atomic2 = value;
    QCOMPARE(atomic2.load(), T(value));

    QVERIFY(atomic.load() >= std::numeric_limits<T>::min());
    QVERIFY(atomic.load() <= std::numeric_limits<T>::max());
}

void tst_QAtomicIntegerXX::copy()
{
    QFETCH(LargeInt, value);

    QAtomicInteger<T> atomic(value);
    QAtomicInteger<T> copy(atomic);
    QCOMPARE(copy.load(), atomic.load());

    QAtomicInteger<T> copy2 = atomic;
    QCOMPARE(copy2.load(), atomic.load());

    // move
    QAtomicInteger<T> copy3(qMove(copy));
    QCOMPARE(copy3.load(), atomic.load());

    QAtomicInteger<T> copy4 = qMove(copy2);
    QCOMPARE(copy4.load(), atomic.load());
}

void tst_QAtomicIntegerXX::assign()
{
    QFETCH(LargeInt, value);

    QAtomicInteger<T> atomic(value);
    QAtomicInteger<T> copy;
    copy = atomic;
    QCOMPARE(copy.load(), atomic.load());

    QAtomicInteger<T> copy2;
    copy2 = atomic;  // operator=(const QAtomicInteger &)
    QCOMPARE(copy2.load(), atomic.load());

    QAtomicInteger<T> copy2bis;
    copy2bis = atomic.load(); // operator=(T)
    QCOMPARE(copy2bis.load(), atomic.load());

    // move
    QAtomicInteger<T> copy3;
    copy3 = qMove(copy);
    QCOMPARE(copy3.load(), atomic.load());

    QAtomicInteger<T> copy4;
    copy4 = qMove(copy2);
    QCOMPARE(copy4.load(), atomic.load());
}

void tst_QAtomicIntegerXX::operatorInteger()
{
    QFETCH(LargeInt, value);

    QAtomicInteger<T> atomic(value);
    T val2 = atomic;
    QCOMPARE(val2, atomic.load());
    QCOMPARE(val2, T(value));
}

void tst_QAtomicIntegerXX::loadAcquireStoreRelease()
{
    QFETCH(LargeInt, value);

    QAtomicInteger<T> atomic(value);
    QCOMPARE(atomic.loadAcquire(), T(value));

    atomic.storeRelease(~value);
    QCOMPARE(atomic.loadAcquire(), T(~value));

    atomic.storeRelease(value);
    QCOMPARE(atomic.load(), T(value));
}

void tst_QAtomicIntegerXX::refDeref()
{
    QFETCH(LargeInt, value);
    T nextValue = T(value + 1);
    T prevValue = T(value - 1);

    QAtomicInteger<T> atomic(value);
    QCOMPARE(atomic.ref(), (nextValue != 0));
    QCOMPARE(atomic.load(), nextValue);
    QCOMPARE(atomic.deref(), (value != 0));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.deref(), (prevValue != 0));
    QCOMPARE(atomic.load(), prevValue);
    QCOMPARE(atomic.ref(), (value != 0));
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(++atomic, nextValue);
    QCOMPARE(--atomic, T(value));
    QCOMPARE(--atomic, prevValue);
    QCOMPARE(++atomic, T(value));

    QCOMPARE(atomic++, T(value));
    QCOMPARE(atomic--, nextValue);
    QCOMPARE(atomic--, T(value));
    QCOMPARE(atomic++, prevValue);
    QCOMPARE(atomic.load(), T(value));
}

void tst_QAtomicIntegerXX::testAndSet()
{
    QFETCH(LargeInt, value);
    T newValue = ~T(value);
    QAtomicInteger<T> atomic(value);

    QVERIFY(atomic.testAndSetRelaxed(value, newValue));
    QCOMPARE(atomic.load(), newValue);
    QVERIFY(!atomic.testAndSetRelaxed(value, newValue));
    QVERIFY(atomic.testAndSetRelaxed(newValue, value));
    QCOMPARE(atomic.load(), T(value));

    QVERIFY(atomic.testAndSetAcquire(value, newValue));
    QCOMPARE(atomic.load(), newValue);
    QVERIFY(!atomic.testAndSetAcquire(value, newValue));
    QVERIFY(atomic.testAndSetAcquire(newValue, value));
    QCOMPARE(atomic.load(), T(value));

    QVERIFY(atomic.testAndSetRelease(value, newValue));
    QCOMPARE(atomic.loadAcquire(), newValue);
    QVERIFY(!atomic.testAndSetRelease(value, newValue));
    QVERIFY(atomic.testAndSetRelease(newValue, value));
    QCOMPARE(atomic.loadAcquire(), T(value));

    QVERIFY(atomic.testAndSetOrdered(value, newValue));
    QCOMPARE(atomic.loadAcquire(), newValue);
    QVERIFY(!atomic.testAndSetOrdered(value, newValue));
    QVERIFY(atomic.testAndSetOrdered(newValue, value));
    QCOMPARE(atomic.loadAcquire(), T(value));
}

void tst_QAtomicIntegerXX::testAndSet3()
{
    QFETCH(LargeInt, value);
    T newValue = ~T(value);
    T oldValue;
    QAtomicInteger<T> atomic(value);

    QVERIFY(atomic.testAndSetRelaxed(value, newValue, oldValue));
    QCOMPARE(atomic.load(), newValue);
    QVERIFY(!atomic.testAndSetRelaxed(value, newValue, oldValue));
    QCOMPARE(oldValue, newValue);
    QVERIFY(atomic.testAndSetRelaxed(newValue, value, oldValue));
    QCOMPARE(atomic.load(), T(value));

    QVERIFY(atomic.testAndSetAcquire(value, newValue, oldValue));
    QCOMPARE(atomic.load(), newValue);
    QVERIFY(!atomic.testAndSetAcquire(value, newValue, oldValue));
    QCOMPARE(oldValue, newValue);
    QVERIFY(atomic.testAndSetAcquire(newValue, value, oldValue));
    QCOMPARE(atomic.load(), T(value));

    QVERIFY(atomic.testAndSetRelease(value, newValue, oldValue));
    QCOMPARE(atomic.loadAcquire(), newValue);
    QVERIFY(!atomic.testAndSetRelease(value, newValue, oldValue));
    QCOMPARE(oldValue, newValue);
    QVERIFY(atomic.testAndSetRelease(newValue, value, oldValue));
    QCOMPARE(atomic.loadAcquire(), T(value));

    QVERIFY(atomic.testAndSetOrdered(value, newValue, oldValue));
    QCOMPARE(atomic.loadAcquire(), newValue);
    QVERIFY(!atomic.testAndSetOrdered(value, newValue, oldValue));
    QCOMPARE(oldValue, newValue);
    QVERIFY(atomic.testAndSetOrdered(newValue, value, oldValue));
    QCOMPARE(atomic.loadAcquire(), T(value));
}

void tst_QAtomicIntegerXX::fetchAndStore()
{
    QFETCH(LargeInt, value);
    T newValue = ~T(value);
    QAtomicInteger<T> atomic(value);

    QCOMPARE(atomic.fetchAndStoreRelaxed(newValue), T(value));
    QCOMPARE(atomic.load(), newValue);
    QCOMPARE(atomic.fetchAndStoreRelaxed(value), newValue);
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndStoreAcquire(newValue), T(value));
    QCOMPARE(atomic.load(), newValue);
    QCOMPARE(atomic.fetchAndStoreAcquire(value), newValue);
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndStoreRelease(newValue), T(value));
    QCOMPARE(atomic.loadAcquire(), newValue);
    QCOMPARE(atomic.fetchAndStoreRelease(value), newValue);
    QCOMPARE(atomic.loadAcquire(), T(value));

    QCOMPARE(atomic.fetchAndStoreOrdered(newValue), T(value));
    QCOMPARE(atomic.loadAcquire(), newValue);
    QCOMPARE(atomic.fetchAndStoreOrdered(value), newValue);
    QCOMPARE(atomic.loadAcquire(), T(value));
}

void tst_QAtomicIntegerXX::fetchAndAdd()
{
    QFETCH(LargeInt, value);
    QAtomicInteger<T> atomic(value);

    // note: this test has undefined behavior for signed max and min
    T parcel1 = 42;
    T parcel2 = T(0-parcel1);
    T newValue1 = T(value) + parcel1;
    T newValue2 = T(value) + parcel2;

    QCOMPARE(atomic.fetchAndAddRelaxed(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndAddRelaxed(parcel2), newValue1);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndAddRelaxed(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndAddRelaxed(parcel1), newValue2);
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndAddAcquire(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndAddAcquire(parcel2), newValue1);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndAddAcquire(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndAddAcquire(parcel1), newValue2);
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndAddRelease(parcel1), T(value));
    QCOMPARE(atomic.loadAcquire(), newValue1);
    QCOMPARE(atomic.fetchAndAddRelease(parcel2), newValue1);
    QCOMPARE(atomic.loadAcquire(), T(value));
    QCOMPARE(atomic.fetchAndAddRelease(parcel2), T(value));
    QCOMPARE(atomic.loadAcquire(), newValue2);
    QCOMPARE(atomic.fetchAndAddRelease(parcel1), newValue2);
    QCOMPARE(atomic.loadAcquire(), T(value));

    QCOMPARE(atomic.fetchAndAddOrdered(parcel1), T(value));
    QCOMPARE(atomic.loadAcquire(), newValue1);
    QCOMPARE(atomic.fetchAndAddOrdered(parcel2), newValue1);
    QCOMPARE(atomic.loadAcquire(), T(value));
    QCOMPARE(atomic.fetchAndAddOrdered(parcel2), T(value));
    QCOMPARE(atomic.loadAcquire(), newValue2);
    QCOMPARE(atomic.fetchAndAddOrdered(parcel1), newValue2);
    QCOMPARE(atomic.loadAcquire(), T(value));

    // operator+=
    QCOMPARE(atomic += parcel1, newValue1);
    QCOMPARE(atomic += parcel2, T(value));
    QCOMPARE(atomic += parcel2, newValue2);
    QCOMPARE(atomic += parcel1, T(value));
}

void tst_QAtomicIntegerXX::fetchAndSub()
{
    QFETCH(LargeInt, value);
    QAtomicInteger<T> atomic(value);

    // note: this test has undefined behavior for signed max and min
    T parcel1 = 42;
    T parcel2 = T(0-parcel1);
    T newValue1 = T(value) - parcel1;
    T newValue2 = T(value) - parcel2;

    QCOMPARE(atomic.fetchAndSubRelaxed(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndSubRelaxed(parcel2), newValue1);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndSubRelaxed(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndSubRelaxed(parcel1), newValue2);
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndSubAcquire(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndSubAcquire(parcel2), newValue1);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndSubAcquire(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndSubAcquire(parcel1), newValue2);
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndSubRelease(parcel1), T(value));
    QCOMPARE(atomic.loadAcquire(), newValue1);
    QCOMPARE(atomic.fetchAndSubRelease(parcel2), newValue1);
    QCOMPARE(atomic.loadAcquire(), T(value));
    QCOMPARE(atomic.fetchAndSubRelease(parcel2), T(value));
    QCOMPARE(atomic.loadAcquire(), newValue2);
    QCOMPARE(atomic.fetchAndSubRelease(parcel1), newValue2);
    QCOMPARE(atomic.loadAcquire(), T(value));

    QCOMPARE(atomic.fetchAndSubOrdered(parcel1), T(value));
    QCOMPARE(atomic.loadAcquire(), newValue1);
    QCOMPARE(atomic.fetchAndSubOrdered(parcel2), newValue1);
    QCOMPARE(atomic.loadAcquire(), T(value));
    QCOMPARE(atomic.fetchAndSubOrdered(parcel2), T(value));
    QCOMPARE(atomic.loadAcquire(), newValue2);
    QCOMPARE(atomic.fetchAndSubOrdered(parcel1), newValue2);
    QCOMPARE(atomic.loadAcquire(), T(value));

    // operator-=
    QCOMPARE(atomic -= parcel1, newValue1);
    QCOMPARE(atomic -= parcel2, T(value));
    QCOMPARE(atomic -= parcel2, newValue2);
    QCOMPARE(atomic -= parcel1, T(value));
}

void tst_QAtomicIntegerXX::addSub()
{
    QFETCH(LargeInt, value);
    QAtomicInteger<T> atomic(value);

    // note: this test has undefined behavior for signed max and min
    T parcel1 = 42;
    T parcel2 = T(0-parcel1);
    T newValue1 = T(value) + parcel1;
    T newValue2 = T(value) - parcel1;

    QCOMPARE(atomic.fetchAndAddRelaxed(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndSubRelaxed(parcel1), newValue1);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndSubRelaxed(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndAddRelaxed(parcel1), newValue2);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndAddRelaxed(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndSubRelaxed(parcel2), newValue2);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndSubRelaxed(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndAddRelaxed(parcel2), newValue1);
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndAddAcquire(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndSubAcquire(parcel1), newValue1);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndSubAcquire(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndAddAcquire(parcel1), newValue2);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndAddAcquire(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndSubAcquire(parcel2), newValue2);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndSubAcquire(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndAddAcquire(parcel2), newValue1);
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndAddRelease(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndSubRelease(parcel1), newValue1);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndSubRelease(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndAddRelease(parcel1), newValue2);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndAddRelease(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndSubRelease(parcel2), newValue2);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndSubRelease(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndAddRelease(parcel2), newValue1);
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndAddOrdered(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndSubOrdered(parcel1), newValue1);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndSubOrdered(parcel1), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndAddOrdered(parcel1), newValue2);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndAddOrdered(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue2);
    QCOMPARE(atomic.fetchAndSubOrdered(parcel2), newValue2);
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndSubOrdered(parcel2), T(value));
    QCOMPARE(atomic.load(), newValue1);
    QCOMPARE(atomic.fetchAndAddOrdered(parcel2), newValue1);
    QCOMPARE(atomic.load(), T(value));

    // operator+= and operator-=
    QCOMPARE(atomic += parcel1, newValue1);
    QCOMPARE(atomic -= parcel1, T(value));
    QCOMPARE(atomic -= parcel1, newValue2);
    QCOMPARE(atomic += parcel1, T(value));
    QCOMPARE(atomic += parcel2, newValue2);
    QCOMPARE(atomic -= parcel2, T(value));
    QCOMPARE(atomic -= parcel2, newValue1);
    QCOMPARE(atomic += parcel2, T(value));
}

void tst_QAtomicIntegerXX::fetchAndOr()
{
    QFETCH(LargeInt, value);
    QAtomicInteger<T> atomic(value);

    T zero = 0;
    T one = 1;
    T minusOne = T(~0);

    QCOMPARE(atomic.fetchAndOrRelaxed(zero), T(value));
    QCOMPARE(atomic.fetchAndOrRelaxed(one), T(value));
    QCOMPARE(atomic.load(), T(value | 1));
    QCOMPARE(atomic.fetchAndOrRelaxed(minusOne), T(value | 1));
    QCOMPARE(atomic.load(), minusOne);

    atomic.store(value);
    QCOMPARE(atomic.fetchAndOrAcquire(zero), T(value));
    QCOMPARE(atomic.fetchAndOrAcquire(one), T(value));
    QCOMPARE(atomic.load(), T(value | 1));
    QCOMPARE(atomic.fetchAndOrAcquire(minusOne), T(value | 1));
    QCOMPARE(atomic.load(), minusOne);

    atomic.store(value);
    QCOMPARE(atomic.fetchAndOrRelease(zero), T(value));
    QCOMPARE(atomic.fetchAndOrRelease(one), T(value));
    QCOMPARE(atomic.load(), T(value | 1));
    QCOMPARE(atomic.fetchAndOrRelease(minusOne), T(value | 1));
    QCOMPARE(atomic.load(), minusOne);

    atomic.store(value);
    QCOMPARE(atomic.fetchAndOrOrdered(zero), T(value));
    QCOMPARE(atomic.fetchAndOrOrdered(one), T(value));
    QCOMPARE(atomic.load(), T(value | 1));
    QCOMPARE(atomic.fetchAndOrOrdered(minusOne), T(value | 1));
    QCOMPARE(atomic.load(), minusOne);

    atomic.store(value);
    QCOMPARE(atomic |= zero, T(value));
    QCOMPARE(atomic |= one, T(value | 1));
    QCOMPARE(atomic |= minusOne, minusOne);
}

void tst_QAtomicIntegerXX::fetchAndAnd()
{
    QFETCH(LargeInt, value);
    QAtomicInteger<T> atomic(value);

    T zero = 0;
    T f = 0xf;
    T minusOne = T(~0);

    QCOMPARE(atomic.fetchAndAndRelaxed(minusOne), T(value));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndAndRelaxed(f), T(value));
    QCOMPARE(atomic.load(), T(value & 0xf));
    QCOMPARE(atomic.fetchAndAndRelaxed(zero), T(value & 0xf));
    QCOMPARE(atomic.load(), zero);

    atomic.store(value);
    QCOMPARE(atomic.fetchAndAndAcquire(minusOne), T(value));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndAndAcquire(f), T(value));
    QCOMPARE(atomic.load(), T(value & 0xf));
    QCOMPARE(atomic.fetchAndAndAcquire(zero), T(value & 0xf));
    QCOMPARE(atomic.load(), zero);

    atomic.store(value);
    QCOMPARE(atomic.fetchAndAndRelease(minusOne), T(value));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndAndRelease(f), T(value));
    QCOMPARE(atomic.load(), T(value & 0xf));
    QCOMPARE(atomic.fetchAndAndRelease(zero), T(value & 0xf));
    QCOMPARE(atomic.load(), zero);

    atomic.store(value);
    QCOMPARE(atomic.fetchAndAndOrdered(minusOne), T(value));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndAndOrdered(f), T(value));
    QCOMPARE(atomic.load(), T(value & 0xf));
    QCOMPARE(atomic.fetchAndAndOrdered(zero), T(value & 0xf));
    QCOMPARE(atomic.load(), zero);

    atomic.store(value);
    QCOMPARE(atomic &= minusOne, T(value));
    QCOMPARE(atomic &= f, T(value & 0xf));
    QCOMPARE(atomic &= zero, zero);
}

void tst_QAtomicIntegerXX::fetchAndXor()
{
    QFETCH(LargeInt, value);
    QAtomicInteger<T> atomic(value);

    T zero = 0;
    T pattern = T(Q_UINT64_C(0xcccccccccccccccc));
    T minusOne = T(~0);

    QCOMPARE(atomic.fetchAndXorRelaxed(zero), T(value));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndXorRelaxed(pattern), T(value));
    QCOMPARE(atomic.load(), T(value ^ pattern));
    QCOMPARE(atomic.fetchAndXorRelaxed(pattern), T(value ^ pattern));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndXorRelaxed(minusOne), T(value));
    QCOMPARE(atomic.load(), T(~value));
    QCOMPARE(atomic.fetchAndXorRelaxed(minusOne), T(~value));
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndXorAcquire(zero), T(value));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndXorAcquire(pattern), T(value));
    QCOMPARE(atomic.load(), T(value ^ pattern));
    QCOMPARE(atomic.fetchAndXorAcquire(pattern), T(value ^ pattern));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndXorAcquire(minusOne), T(value));
    QCOMPARE(atomic.load(), T(~value));
    QCOMPARE(atomic.fetchAndXorAcquire(minusOne), T(~value));
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndXorRelease(zero), T(value));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndXorRelease(pattern), T(value));
    QCOMPARE(atomic.load(), T(value ^ pattern));
    QCOMPARE(atomic.fetchAndXorRelease(pattern), T(value ^ pattern));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndXorRelease(minusOne), T(value));
    QCOMPARE(atomic.load(), T(~value));
    QCOMPARE(atomic.fetchAndXorRelease(minusOne), T(~value));
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic.fetchAndXorOrdered(zero), T(value));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndXorOrdered(pattern), T(value));
    QCOMPARE(atomic.load(), T(value ^ pattern));
    QCOMPARE(atomic.fetchAndXorOrdered(pattern), T(value ^ pattern));
    QCOMPARE(atomic.load(), T(value));
    QCOMPARE(atomic.fetchAndXorOrdered(minusOne), T(value));
    QCOMPARE(atomic.load(), T(~value));
    QCOMPARE(atomic.fetchAndXorOrdered(minusOne), T(~value));
    QCOMPARE(atomic.load(), T(value));

    QCOMPARE(atomic ^= zero, T(value));
    QCOMPARE(atomic ^= pattern, T(value ^ pattern));
    QCOMPARE(atomic ^= pattern, T(value));
    QCOMPARE(atomic ^= minusOne, T(~value));
    QCOMPARE(atomic ^= minusOne, T(value));
}

#include "tst_qatomicinteger.moc"

QTEST_APPLESS_MAIN(tst_QAtomicIntegerXX)

