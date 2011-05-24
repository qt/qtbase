/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <QtTest/QtTest>

#include <QAtomicInt>
#include <QCoreApplication>

#include <limits.h>

//TESTED_CLASS=
//TESTED_FILES=

class tst_QAtomicInt : public QObject
{
    Q_OBJECT

public:
    tst_QAtomicInt();
    ~tst_QAtomicInt();

private slots:
    void warningFree();

    // QAtomicInt members
    void constructor_data();
    void constructor();
    void copy_constructor_data();
    void copy_constructor();
    void equality_operator_data();
    void equality_operator();
    void inequality_operator_data();
    void inequality_operator();
    void not_operator_data();
    void not_operator();
    void cast_operator_data();
    void cast_operator();
    void assignment_operator_data();
    void assignment_operator();

    void isReferenceCountingNative();
    void isReferenceCountingWaitFree();
    void ref_data();
    void ref();
    void deref_data();
    void deref();

    void isTestAndSetNative();
    void isTestAndSetWaitFree();
    void testAndSet_data();
    void testAndSet();

    void isFetchAndStoreNative();
    void isFetchAndStoreWaitFree();
    void fetchAndStore_data();
    void fetchAndStore();

    void isFetchAndAddNative();
    void isFetchAndAddWaitFree();
    void fetchAndAdd_data();
    void fetchAndAdd();

    // stress tests
    void testAndSet_loop();
    void fetchAndAdd_loop();
    void fetchAndAdd_threadedLoop();

private:
    static void warningFreeHelper();
};

tst_QAtomicInt::tst_QAtomicInt()
{ }

tst_QAtomicInt::~tst_QAtomicInt()
{ }

void tst_QAtomicInt::warningFreeHelper()
{
    qFatal("This code is bogus, and shouldn't be run. We're looking for compiler warnings only.");

    QBasicAtomicInt i = Q_BASIC_ATOMIC_INITIALIZER(0);

    int expectedValue = 0;
    int newValue = 0;
    int valueToAdd = 0;

    i.ref();
    i.deref();

    i.testAndSetRelaxed(expectedValue, newValue);
    i.testAndSetAcquire(expectedValue, newValue);
    i.testAndSetRelease(expectedValue, newValue);
    i.testAndSetOrdered(expectedValue, newValue);

    i.fetchAndStoreRelaxed(newValue);
    i.fetchAndStoreAcquire(newValue);
    i.fetchAndStoreRelease(newValue);
    i.fetchAndStoreOrdered(newValue);

    i.fetchAndAddRelaxed(valueToAdd);
    i.fetchAndAddAcquire(valueToAdd);
    i.fetchAndAddRelease(valueToAdd);
    i.fetchAndAddOrdered(valueToAdd);
}

void tst_QAtomicInt::warningFree()
{
    // This is a compile time check for warnings.
    // No need for actual work here.

    void (*foo)() = &warningFreeHelper;
    (void)foo;
}

void tst_QAtomicInt::constructor_data()
{
    QTest::addColumn<int>("value");

    QTest::newRow("0") << 31337;
    QTest::newRow("1") << 0;
    QTest::newRow("2") << 1;
    QTest::newRow("3") << -1;
    QTest::newRow("4") << 2;
    QTest::newRow("5") << -2;
    QTest::newRow("6") << 3;
    QTest::newRow("7") << -3;
    QTest::newRow("8") << INT_MAX;
    QTest::newRow("9") << INT_MIN+1;
}

void tst_QAtomicInt::constructor()
{
    QFETCH(int, value);
    QAtomicInt atomic1(value);
    QCOMPARE(int(atomic1), value);
    QAtomicInt atomic2 = value;
    QCOMPARE(int(atomic2), value);
}

void tst_QAtomicInt::copy_constructor_data()
{ constructor_data(); }

void tst_QAtomicInt::copy_constructor()
{
    QFETCH(int, value);
    QAtomicInt atomic1(value);
    QCOMPARE(int(atomic1), value);

    QAtomicInt atomic2(atomic1);
    QCOMPARE(int(atomic2), value);
    QAtomicInt atomic3 = atomic1;
    QCOMPARE(int(atomic3), value);
    QAtomicInt atomic4(atomic2);
    QCOMPARE(int(atomic4), value);
    QAtomicInt atomic5 = atomic2;
    QCOMPARE(int(atomic5), value);
}

void tst_QAtomicInt::equality_operator_data()
{
    QTest::addColumn<int>("value1");
    QTest::addColumn<int>("value2");
    QTest::addColumn<int>("result");

    QTest::newRow("success0") <<  1 <<  1 << 1;
    QTest::newRow("success1") << -1 << -1 << 1;
    QTest::newRow("failure0") <<  0 <<  1 << 0;
    QTest::newRow("failure1") <<  1 <<  0 << 0;
    QTest::newRow("failure2") <<  0 << -1 << 0;
    QTest::newRow("failure3") << -1 <<  0 << 0;
}

void tst_QAtomicInt::equality_operator()
{
    QFETCH(int, value1);
    QFETCH(int, value2);
    QAtomicInt x = value1;
    QTEST(x == value2 ? 1 : 0, "result");
}

void tst_QAtomicInt::inequality_operator_data()
{
    QTest::addColumn<int>("value1");
    QTest::addColumn<int>("value2");
    QTest::addColumn<int>("result");

    QTest::newRow("failure0") <<  1 <<  1 << 0;
    QTest::newRow("failure1") << -1 << -1 << 0;
    QTest::newRow("success0") <<  0 <<  1 << 1;
    QTest::newRow("success1") <<  1 <<  0 << 1;
    QTest::newRow("success2") <<  0 << -1 << 1;
    QTest::newRow("success3") << -1 <<  0 << 1;
}

void tst_QAtomicInt::inequality_operator()
{
    QFETCH(int, value1);
    QFETCH(int, value2);
    QAtomicInt x = value1;
    QTEST(x != value2 ? 1 : 0, "result");
}

void tst_QAtomicInt::not_operator_data()
{ constructor_data(); }

void tst_QAtomicInt::not_operator()
{
    QFETCH(int, value);
    QAtomicInt atomic = value;
    QCOMPARE(!atomic, !value);
}

void tst_QAtomicInt::cast_operator_data()
{ constructor_data(); }

void tst_QAtomicInt::cast_operator()
{
    QFETCH(int, value);
    QAtomicInt atomic = value;
    int copy = atomic;
    QCOMPARE(copy, value);
}

void tst_QAtomicInt::assignment_operator_data()
{
    QTest::addColumn<int>("value");
    QTest::addColumn<int>("newval");

    QTest::newRow("value0") <<  0 <<  1;
    QTest::newRow("value1") <<  1 <<  0;
    QTest::newRow("value2") <<  0 << -1;
    QTest::newRow("value3") << -1 <<  0;
    QTest::newRow("value4") << -1 <<  1;
    QTest::newRow("value5") <<  1 << -1;
}

void tst_QAtomicInt::assignment_operator()
{
    QFETCH(int, value);
    QFETCH(int, newval);

    {
        QAtomicInt atomic1 = value;
        atomic1 = newval;
        QCOMPARE(int(atomic1), newval);
        atomic1 = value;
        QCOMPARE(int(atomic1), value);
        QAtomicInt atomic2 = newval;
        atomic1 = atomic2;
        QCOMPARE(atomic1, atomic2);
    }
}

void tst_QAtomicInt::isReferenceCountingNative()
{
#if defined(Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicInt::isReferenceCountingNative());

#  if (defined(Q_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE)     \
       || defined(Q_ATOMIC_INT_REFERENCE_COUNTING_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_REFERENCE_COUNTING_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE)
    // could be either, just want to make sure the function is implemented
    QVERIFY(QAtomicInt::isReferenceCountingNative() || !QAtomicInt::isReferenceCountingNative());

#  if (defined(Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_INT_REFERENCE_COUNTING_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_REFERENCE_COUNTING_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_INT_REFERENCE_COUNTING_IS_NOT_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(!QAtomicInt::isReferenceCountingNative());

#  if (defined(Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_REFERENCE_COUNTING_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#else
#  error "Q_ATOMIC_INT_REFERENCE_COUNTING_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE is not defined"
#endif
}

void tst_QAtomicInt::isReferenceCountingWaitFree()
{
#if defined(Q_ATOMIC_INT_REFERENCE_COUNTING_IS_WAIT_FREE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicInt::isReferenceCountingWaitFree());

    // enforce some invariants
    QVERIFY(QAtomicInt::isReferenceCountingNative());
#  if defined(Q_ATOMIC_INT_REFERENCE_COUNTING_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
    // the runtime test should say the same thing
    QVERIFY(!QAtomicInt::isReferenceCountingWaitFree());
#endif
}

void tst_QAtomicInt::ref_data()
{
    QTest::addColumn<int>("value");
    QTest::addColumn<int>("result");
    QTest::addColumn<int>("expected");

    QTest::newRow("data0") <<  0 << 1 << 1;
    QTest::newRow("data1") << -1 << 0 << 0;
    QTest::newRow("data2") <<  1 << 1 << 2;
}

void tst_QAtomicInt::ref()
{
    QFETCH(int, value);
    QAtomicInt x = value;
    QTEST(x.ref() ? 1 : 0, "result");
    QTEST(int(x), "expected");
}

void tst_QAtomicInt::deref_data()
{
    QTest::addColumn<int>("value");
    QTest::addColumn<int>("result");
    QTest::addColumn<int>("expected");

    QTest::newRow("data0") <<  0 << 1 << -1;
    QTest::newRow("data1") <<  1 << 0 <<  0;
    QTest::newRow("data2") <<  2 << 1 <<  1;
}

void tst_QAtomicInt::deref()
{
    QFETCH(int, value);
    QAtomicInt x = value;
    QTEST(x.deref() ? 1 : 0, "result");
    QTEST(int(x), "expected");
}

void tst_QAtomicInt::isTestAndSetNative()
{
#if defined(Q_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicInt::isTestAndSetNative());

#  if (defined(Q_ATOMIC_INT_TEST_AND_SET_IS_SOMETIMES_NATIVE)     \
       || defined(Q_ATOMIC_INT_TEST_AND_SET_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_INT_TEST_AND_SET_IS_SOMETIMES_NATIVE)
    // could be either, just want to make sure the function is implemented
    QVERIFY(QAtomicInt::isTestAndSetNative() || !QAtomicInt::isTestAndSetNative());

#  if (defined(Q_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_INT_TEST_AND_SET_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_INT_TEST_AND_SET_IS_NOT_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(!QAtomicInt::isTestAndSetNative());

#  if (defined(Q_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_INT_TEST_AND_SET_IS_SOMETIMES_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#else
#  error "Q_ATOMIC_INT_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE is not defined"
#endif
}

void tst_QAtomicInt::isTestAndSetWaitFree()
{
#if defined(Q_ATOMIC_INT_TEST_AND_SET_IS_WAIT_FREE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicInt::isTestAndSetWaitFree());

    // enforce some invariants
    QVERIFY(QAtomicInt::isTestAndSetNative());
#  if defined(Q_ATOMIC_INT_TEST_AND_SET_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
    // the runtime test should say the same thing
    QVERIFY(!QAtomicInt::isTestAndSetWaitFree());
#endif
}

void tst_QAtomicInt::testAndSet_data()
{
    QTest::addColumn<int>("value");
    QTest::addColumn<int>("expected");
    QTest::addColumn<int>("newval");
    QTest::addColumn<int>("result");

    // these should succeed
    QTest::newRow("success0") <<         0 <<         0 <<         0 << 1;
    QTest::newRow("success1") <<         0 <<         0 <<         1 << 1;
    QTest::newRow("success2") <<         0 <<         0 <<        -1 << 1;
    QTest::newRow("success3") <<         1 <<         1 <<         0 << 1;
    QTest::newRow("success4") <<         1 <<         1 <<         1 << 1;
    QTest::newRow("success5") <<         1 <<         1 <<        -1 << 1;
    QTest::newRow("success6") <<        -1 <<        -1 <<         0 << 1;
    QTest::newRow("success7") <<        -1 <<        -1 <<         1 << 1;
    QTest::newRow("success8") <<        -1 <<        -1 <<        -1 << 1;
    QTest::newRow("success9") << INT_MIN+1 << INT_MIN+1 << INT_MIN+1 << 1;
    QTest::newRow("successA") << INT_MIN+1 << INT_MIN+1 <<         1 << 1;
    QTest::newRow("successB") << INT_MIN+1 << INT_MIN+1 <<        -1 << 1;
    QTest::newRow("successC") << INT_MAX   << INT_MAX   << INT_MAX   << 1;
    QTest::newRow("successD") << INT_MAX   << INT_MAX   <<         1 << 1;
    QTest::newRow("successE") << INT_MAX   << INT_MAX   <<        -1 << 1;

    // these should fail
    QTest::newRow("failure0") <<       0   <<       1   <<        ~0 << 0;
    QTest::newRow("failure1") <<       0   <<      -1   <<        ~0 << 0;
    QTest::newRow("failure2") <<       1   <<       0   <<        ~0 << 0;
    QTest::newRow("failure3") <<      -1   <<       0   <<        ~0 << 0;
    QTest::newRow("failure4") <<       1   <<      -1   <<        ~0 << 0;
    QTest::newRow("failure5") <<      -1   <<       1   <<        ~0 << 0;
    QTest::newRow("failure6") << INT_MIN+1 << INT_MAX   <<        ~0 << 0;
    QTest::newRow("failure7") << INT_MAX   << INT_MIN+1 <<        ~0 << 0;
}

void tst_QAtomicInt::testAndSet()
{
    QFETCH(int, value);
    QFETCH(int, expected);
    QFETCH(int, newval);

    {
        QAtomicInt atomic = value;
        QTEST(atomic.testAndSetRelaxed(expected, newval) ? 1 : 0, "result");
    }

    {
        QAtomicInt atomic = value;
        QTEST(atomic.testAndSetAcquire(expected, newval) ? 1 : 0, "result");
    }

    {
        QAtomicInt atomic = value;
        QTEST(atomic.testAndSetRelease(expected, newval) ? 1 : 0, "result");
    }

    {
        QAtomicInt atomic = value;
        QTEST(atomic.testAndSetOrdered(expected, newval) ? 1 : 0, "result");
    }
}

void tst_QAtomicInt::isFetchAndStoreNative()
{
#if defined(Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicInt::isFetchAndStoreNative());

#  if (defined(Q_ATOMIC_INT_FETCH_AND_STORE_IS_SOMETIMES_NATIVE)     \
       || defined(Q_ATOMIC_INT_FETCH_AND_STORE_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_INT_FETCH_AND_STORE_IS_SOMETIMES_NATIVE)
    // could be either, just want to make sure the function is implemented
    QVERIFY(QAtomicInt::isFetchAndStoreNative() || !QAtomicInt::isFetchAndStoreNative());

#  if (defined(Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_INT_FETCH_AND_STORE_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_INT_FETCH_AND_STORE_IS_NOT_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(!QAtomicInt::isFetchAndStoreNative());

#  if (defined(Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_INT_FETCH_AND_STORE_IS_SOMETIMES_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#else
#  error "Q_ATOMIC_INT_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE is not defined"
#endif
}

void tst_QAtomicInt::isFetchAndStoreWaitFree()
{
#if defined(Q_ATOMIC_INT_FETCH_AND_STORE_IS_WAIT_FREE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicInt::isFetchAndStoreWaitFree());

    // enforce some invariants
    QVERIFY(QAtomicInt::isFetchAndStoreNative());
#  if defined(Q_ATOMIC_INT_FETCH_AND_STORE_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
    // the runtime test should say the same thing
    QVERIFY(!QAtomicInt::isFetchAndStoreWaitFree());
#endif
}

void tst_QAtomicInt::fetchAndStore_data()
{
    QTest::addColumn<int>("value");
    QTest::addColumn<int>("newval");

    QTest::newRow("data0") << 0 << 1;
    QTest::newRow("data1") << 1 << 2;
    QTest::newRow("data2") << 3 << 8;
}

void tst_QAtomicInt::fetchAndStore()
{
    QFETCH(int, value);
    QFETCH(int, newval);

    {
        QAtomicInt atomic = value;
        QCOMPARE(atomic.fetchAndStoreRelaxed(newval), value);
        QCOMPARE(int(atomic), newval);
    }

    {
        QAtomicInt atomic = value;
        QCOMPARE(atomic.fetchAndStoreAcquire(newval), value);
        QCOMPARE(int(atomic), newval);
    }

    {
        QAtomicInt atomic = value;
        QCOMPARE(atomic.fetchAndStoreRelease(newval), value);
        QCOMPARE(int(atomic), newval);
    }

    {
        QAtomicInt atomic = value;
        QCOMPARE(atomic.fetchAndStoreOrdered(newval), value);
        QCOMPARE(int(atomic), newval);
    }
}

void tst_QAtomicInt::isFetchAndAddNative()
{
#if defined(Q_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicInt::isFetchAndAddNative());

#  if (defined(Q_ATOMIC_INT_FETCH_AND_ADD_IS_SOMETIMES_NATIVE)     \
       || defined(Q_ATOMIC_INT_FETCH_AND_ADD_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_INT_FETCH_AND_ADD_IS_SOMETIMES_NATIVE)
    // could be either, just want to make sure the function is implemented
    QVERIFY(QAtomicInt::isFetchAndAddNative() || !QAtomicInt::isFetchAndAddNative());

#  if (defined(Q_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_INT_FETCH_AND_ADD_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_INT_FETCH_AND_ADD_IS_NOT_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(!QAtomicInt::isFetchAndAddNative());

#  if (defined(Q_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_INT_FETCH_AND_ADD_IS_SOMETIMES_NATIVE))
#    error "Define only one of Q_ATOMIC_INT_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#else
#  error "Q_ATOMIC_INT_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE is not defined"
#endif
}

void tst_QAtomicInt::isFetchAndAddWaitFree()
{
#if defined(Q_ATOMIC_INT_FETCH_AND_ADD_IS_WAIT_FREE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicInt::isFetchAndAddWaitFree());

    // enforce some invariants
    QVERIFY(QAtomicInt::isFetchAndAddNative());
#  if defined(Q_ATOMIC_INT_FETCH_AND_ADD_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
    // the runtime test should say the same thing
    QVERIFY(!QAtomicInt::isFetchAndAddWaitFree());
#endif
}

void tst_QAtomicInt::fetchAndAdd_data()
{
    QTest::addColumn<int>("value1");
    QTest::addColumn<int>("value2");

    QTest::newRow("0+1") << 0 << 1;
    QTest::newRow("1+0") << 1 << 0;
    QTest::newRow("1+2") << 1 << 2;
    QTest::newRow("2+1") << 2 << 1;
    QTest::newRow("10+21") << 10 << 21;
    QTest::newRow("31+40") << 31 << 40;
    QTest::newRow("51+62") << 51 << 62;
    QTest::newRow("72+81") << 72 << 81;
    QTest::newRow("810+721") << 810 << 721;
    QTest::newRow("631+540") << 631 << 540;
    QTest::newRow("451+362") << 451 << 362;
    QTest::newRow("272+181") << 272 << 181;
    QTest::newRow("1810+8721") << 1810 << 8721;
    QTest::newRow("3631+6540") << 3631 << 6540;
    QTest::newRow("5451+4362") << 5451 << 4362;
    QTest::newRow("7272+2181") << 7272 << 2181;

    QTest::newRow("0+-1") << 0 << -1;
    QTest::newRow("1+0") << 1 << 0;
    QTest::newRow("1+-2") << 1 << -2;
    QTest::newRow("2+-1") << 2 << -1;
    QTest::newRow("10+-21") << 10 << -21;
    QTest::newRow("31+-40") << 31 << -40;
    QTest::newRow("51+-62") << 51 << -62;
    QTest::newRow("72+-81") << 72 << -81;
    QTest::newRow("810+-721") << 810 << -721;
    QTest::newRow("631+-540") << 631 << -540;
    QTest::newRow("451+-362") << 451 << -362;
    QTest::newRow("272+-181") << 272 << -181;
    QTest::newRow("1810+-8721") << 1810 << -8721;
    QTest::newRow("3631+-6540") << 3631 << -6540;
    QTest::newRow("5451+-4362") << 5451 << -4362;
    QTest::newRow("7272+-2181") << 7272 << -2181;

    QTest::newRow("0+1") << 0 << 1;
    QTest::newRow("-1+0") << -1 << 0;
    QTest::newRow("-1+2") << -1 << 2;
    QTest::newRow("-2+1") << -2 << 1;
    QTest::newRow("-10+21") << -10 << 21;
    QTest::newRow("-31+40") << -31 << 40;
    QTest::newRow("-51+62") << -51 << 62;
    QTest::newRow("-72+81") << -72 << 81;
    QTest::newRow("-810+721") << -810 << 721;
    QTest::newRow("-631+540") << -631 << 540;
    QTest::newRow("-451+362") << -451 << 362;
    QTest::newRow("-272+181") << -272 << 181;
    QTest::newRow("-1810+8721") << -1810 << 8721;
    QTest::newRow("-3631+6540") << -3631 << 6540;
    QTest::newRow("-5451+4362") << -5451 << 4362;
    QTest::newRow("-7272+2181") << -7272 << 2181;
}

void tst_QAtomicInt::fetchAndAdd()
{
    QFETCH(int, value1);
    QFETCH(int, value2);
    int result;

    {
        QAtomicInt atomic = value1;
        result = atomic.fetchAndAddRelaxed(value2);
        QCOMPARE(result, value1);
        QCOMPARE(int(atomic), value1 + value2);
    }

    {
        QAtomicInt atomic = value1;
        result = atomic.fetchAndAddAcquire(value2);
        QCOMPARE(result, value1);
        QCOMPARE(int(atomic), value1 + value2);
    }

    {
        QAtomicInt atomic = value1;
        result = atomic.fetchAndAddRelease(value2);
        QCOMPARE(result, value1);
        QCOMPARE(int(atomic), value1 + value2);
    }

    {
        QAtomicInt atomic = value1;
        result = atomic.fetchAndAddOrdered(value2);
        QCOMPARE(result, value1);
        QCOMPARE(int(atomic), value1 + value2);
    }
}

void tst_QAtomicInt::testAndSet_loop()
{
    QTime stopWatch;
    stopWatch.start();

    int iterations = 10000000;

    QAtomicInt val=0;
    for (int i = 0; i < iterations; ++i) {
        QVERIFY(val.testAndSetRelaxed(val, val+1));
        if ((i % 1000) == 999) {
            if (stopWatch.elapsed() > 60 * 1000) {
                // This test shouldn't run for more than two minutes.
                qDebug("Interrupted test after %d iterations (%.2f iterations/sec)",
                       i, (i * 1000.0) / double(stopWatch.elapsed()));
                break;
            }
        }
    }
}

void tst_QAtomicInt::fetchAndAdd_loop()
{
    int iterations = 10000000;
#if defined (Q_OS_HPUX)
    iterations = 1000000;
#endif

    QAtomicInt val=0;
    for (int i = 0; i < iterations; ++i) {
        const int prev = val.fetchAndAddRelaxed(1);
        QCOMPARE(prev, int(val) -1);
    }
}

class FetchAndAddThread : public QThread
{
public:
    void run()
    {

        for (int i = 0; i < iterations; ++i)
            val->fetchAndAddAcquire(1);

        for (int i = 0; i < iterations; ++i)
            val->fetchAndAddAcquire(-1);

    }
QAtomicInt *val;
int iterations;
};


void tst_QAtomicInt::fetchAndAdd_threadedLoop()
{
    QAtomicInt val;
    FetchAndAddThread t1;
    t1.val = &val;
    t1.iterations = 1000000;

    FetchAndAddThread t2;
    t2.val = &val;
    t2.iterations = 2000000;

    t1.start();
    t2.start();
    t1.wait();
    t2.wait();

    QCOMPARE(int(val), 0);
}

QTEST_MAIN(tst_QAtomicInt)
#include "tst_qatomicint.moc"
