/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <qatomic.h>
#include <limits.h>

class tst_QAtomicPointer : public QObject
{
    Q_OBJECT
private slots:
    void warningFree();
    void alignment();

    void constructor();
    void copy_constructor();
    void assignment_operator();

    void isTestAndSetNative();
    void isTestAndSetWaitFree();
    void testAndSet();

    void isFetchAndStoreNative();
    void isFetchAndStoreWaitFree();
    void fetchAndStore();

    void isFetchAndAddNative();
    void isFetchAndAddWaitFree();
    void fetchAndAdd_data();
    void fetchAndAdd();

    void constAndVolatile();
    void forwardDeclared();

    void operators();
private:
    static void warningFreeHelper();
};

struct WFHC
{
    void bar() {}
};

void tst_QAtomicPointer::warningFreeHelper()
{
    qFatal("This code is bogus, and shouldn't be run. We're looking for compiler warnings only.");

    QBasicAtomicPointer<WFHC> p = Q_BASIC_ATOMIC_INITIALIZER(0);

    p.load()->bar();

    WFHC *expectedValue = 0;
    WFHC *newValue = 0;
    qptrdiff valueToAdd = 0;

    p.testAndSetRelaxed(expectedValue, newValue);
    p.testAndSetAcquire(expectedValue, newValue);
    p.testAndSetRelease(expectedValue, newValue);
    p.testAndSetOrdered(expectedValue, newValue);

    p.fetchAndStoreRelaxed(newValue);
    p.fetchAndStoreAcquire(newValue);
    p.fetchAndStoreRelease(newValue);
    p.fetchAndStoreOrdered(newValue);

    p.fetchAndAddRelaxed(valueToAdd);
    p.fetchAndAddAcquire(valueToAdd);
    p.fetchAndAddRelease(valueToAdd);
    p.fetchAndAddOrdered(valueToAdd);
}

void tst_QAtomicPointer::warningFree()
{
    // This is a compile time check for warnings.
    // No need for actual work here.

    void (*foo)() = &warningFreeHelper;
    (void)foo;
}

void tst_QAtomicPointer::alignment()
{
#ifdef Q_ALIGNOF
    // this will cause a build error if the alignment isn't the same
    char dummy[Q_ALIGNOF(QBasicAtomicPointer<void>) == Q_ALIGNOF(void*) ? 1 : -1];
    (void)dummy;
#endif
}

void tst_QAtomicPointer::constructor()
{
    void *one = this;
    QAtomicPointer<void> atomic1 = one;
    QCOMPARE(atomic1.load(), one);

    void *two = &one;
    QAtomicPointer<void> atomic2 = two;
    QCOMPARE(atomic2.load(), two);

    void *three = &two;
    QAtomicPointer<void> atomic3 = three;
    QCOMPARE(atomic3.load(), three);
}

void tst_QAtomicPointer::copy_constructor()
{
    void *one = this;
    QAtomicPointer<void> atomic1 = one;
    QAtomicPointer<void> atomic1_copy = atomic1;
    QCOMPARE(atomic1_copy.load(), one);
    QCOMPARE(atomic1_copy.load(), atomic1.load());

    void *two = &one;
    QAtomicPointer<void> atomic2 = two;
    QAtomicPointer<void> atomic2_copy = atomic2;
    QCOMPARE(atomic2_copy.load(), two);
    QCOMPARE(atomic2_copy.load(), atomic2.load());

    void *three = &two;
    QAtomicPointer<void> atomic3 = three;
    QAtomicPointer<void> atomic3_copy = atomic3;
    QCOMPARE(atomic3_copy.load(), three);
    QCOMPARE(atomic3_copy.load(), atomic3.load());
}

void tst_QAtomicPointer::assignment_operator()
{
    void *one = this;
    void *two = &one;
    void *three = &two;

    QAtomicPointer<void> atomic1 = one;
    QAtomicPointer<void> atomic2 = two;
    QAtomicPointer<void> atomic3 = three;

    QCOMPARE(atomic1.load(), one);
    QCOMPARE(atomic2.load(), two);
    QCOMPARE(atomic3.load(), three);

    atomic1 = two;
    atomic2 = three;
    atomic3 = one;

    QCOMPARE(atomic1.load(), two);
    QCOMPARE(atomic2.load(), three);
    QCOMPARE(atomic3.load(), one);
}

void tst_QAtomicPointer::isTestAndSetNative()
{
#if defined(Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicPointer<void>::isTestAndSetNative());

#  if (defined(Q_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE)     \
       || defined(Q_ATOMIC_POINTER_TEST_AND_SET_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_POINTER_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE)
    // could be either, just want to make sure the function is implemented
    QVERIFY(QAtomicPointer<void>::isTestAndSetNative()
            || !QAtomicPointer<void>::isTestAndSetNative());

#  if (defined(Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_POINTER_TEST_AND_SET_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_POINTER_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_POINTER_TEST_AND_SET_IS_NOT_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(!QAtomicPointer<void>::isTestAndSetNative());

#  if (defined(Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE))
#    error "Define only one of Q_ATOMIC_POINTER_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#else
#  error "Q_ATOMIC_POINTER_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE is not defined"
#endif
}

void tst_QAtomicPointer::isTestAndSetWaitFree()
{
#if defined(Q_ATOMIC_POINTER_TEST_AND_SET_IS_WAIT_FREE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicPointer<void>::isTestAndSetWaitFree());

    // enforce some invariants
    QVERIFY(QAtomicPointer<void>::isTestAndSetNative());
#  if defined(Q_ATOMIC_POINTER_TEST_AND_SET_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
    // the runtime test should say the same thing
    QVERIFY(!QAtomicPointer<void>::isTestAndSetWaitFree());
#endif
}

void tst_QAtomicPointer::testAndSet()
{
    void *one = this;
    void *two = &one;
    void *three = &two;

    {
        QAtomicPointer<void> atomic1 = one;
        QAtomicPointer<void> atomic2 = two;
        QAtomicPointer<void> atomic3 = three;

        QCOMPARE(atomic1.load(), one);
        QCOMPARE(atomic2.load(), two);
        QCOMPARE(atomic3.load(), three);

        QVERIFY(atomic1.testAndSetRelaxed(one, two));
        QVERIFY(atomic2.testAndSetRelaxed(two, three));
        QVERIFY(atomic3.testAndSetRelaxed(three, one));

        QCOMPARE(atomic1.load(), two);
        QCOMPARE(atomic2.load(), three);
        QCOMPARE(atomic3.load(), one);
    }

    {
        QAtomicPointer<void> atomic1 = one;
        QAtomicPointer<void> atomic2 = two;
        QAtomicPointer<void> atomic3 = three;

        QCOMPARE(atomic1.load(), one);
        QCOMPARE(atomic2.load(), two);
        QCOMPARE(atomic3.load(), three);

        QVERIFY(atomic1.testAndSetAcquire(one, two));
        QVERIFY(atomic2.testAndSetAcquire(two, three));
        QVERIFY(atomic3.testAndSetAcquire(three, one));

        QCOMPARE(atomic1.load(), two);
        QCOMPARE(atomic2.load(), three);
        QCOMPARE(atomic3.load(), one);
    }

    {
        QAtomicPointer<void> atomic1 = one;
        QAtomicPointer<void> atomic2 = two;
        QAtomicPointer<void> atomic3 = three;

        QCOMPARE(atomic1.load(), one);
        QCOMPARE(atomic2.load(), two);
        QCOMPARE(atomic3.load(), three);

        QVERIFY(atomic1.testAndSetRelease(one, two));
        QVERIFY(atomic2.testAndSetRelease(two, three));
        QVERIFY(atomic3.testAndSetRelease(three, one));

        QCOMPARE(atomic1.load(), two);
        QCOMPARE(atomic2.load(), three);
        QCOMPARE(atomic3.load(), one);
    }

    {
        QAtomicPointer<void> atomic1 = one;
        QAtomicPointer<void> atomic2 = two;
        QAtomicPointer<void> atomic3 = three;

        QCOMPARE(atomic1.load(), one);
        QCOMPARE(atomic2.load(), two);
        QCOMPARE(atomic3.load(), three);

        QVERIFY(atomic1.testAndSetOrdered(one, two));
        QVERIFY(atomic2.testAndSetOrdered(two, three));
        QVERIFY(atomic3.testAndSetOrdered(three, one));

        QCOMPARE(atomic1.load(), two);
        QCOMPARE(atomic2.load(), three);
        QCOMPARE(atomic3.load(), one);
    }
}

void tst_QAtomicPointer::isFetchAndStoreNative()
{
#if defined(Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicPointer<void>::isFetchAndStoreNative());

#  if (defined(Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_SOMETIMES_NATIVE)     \
       || defined(Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_SOMETIMES_NATIVE)
    // could be either, just want to make sure the function is implemented
    QVERIFY(QAtomicPointer<void>::isFetchAndStoreNative()
            || !QAtomicPointer<void>::isFetchAndStoreNative());

#  if (defined(Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_NOT_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(!QAtomicPointer<void>::isFetchAndStoreNative());

#  if (defined(Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_SOMETIMES_NATIVE))
#    error "Define only one of Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#else
#  error "Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE is not defined"
#endif
}

void tst_QAtomicPointer::isFetchAndStoreWaitFree()
{
#if defined(Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_WAIT_FREE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicPointer<void>::isFetchAndStoreWaitFree());

    // enforce some invariants
    QVERIFY(QAtomicPointer<void>::isFetchAndStoreNative());
#  if defined(Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
    // the runtime test should say the same thing
    QVERIFY(!QAtomicPointer<void>::isFetchAndStoreWaitFree());
#endif
}

void tst_QAtomicPointer::fetchAndStore()
{
    void *one = this;
    void *two = &one;
    void *three = &two;

    {
        QAtomicPointer<void> atomic1 = one;
        QAtomicPointer<void> atomic2 = two;
        QAtomicPointer<void> atomic3 = three;

        QCOMPARE(atomic1.load(), one);
        QCOMPARE(atomic2.load(), two);
        QCOMPARE(atomic3.load(), three);

        QCOMPARE(atomic1.fetchAndStoreRelaxed(two), one);
        QCOMPARE(atomic2.fetchAndStoreRelaxed(three), two);
        QCOMPARE(atomic3.fetchAndStoreRelaxed(one), three);

        QCOMPARE(atomic1.load(), two);
        QCOMPARE(atomic2.load(), three);
        QCOMPARE(atomic3.load(), one);
    }

    {
        QAtomicPointer<void> atomic1 = one;
        QAtomicPointer<void> atomic2 = two;
        QAtomicPointer<void> atomic3 = three;

        QCOMPARE(atomic1.load(), one);
        QCOMPARE(atomic2.load(), two);
        QCOMPARE(atomic3.load(), three);

        QCOMPARE(atomic1.fetchAndStoreAcquire(two), one);
        QCOMPARE(atomic2.fetchAndStoreAcquire(three), two);
        QCOMPARE(atomic3.fetchAndStoreAcquire(one), three);

        QCOMPARE(atomic1.load(), two);
        QCOMPARE(atomic2.load(), three);
        QCOMPARE(atomic3.load(), one);
    }

    {
        QAtomicPointer<void> atomic1 = one;
        QAtomicPointer<void> atomic2 = two;
        QAtomicPointer<void> atomic3 = three;

        QCOMPARE(atomic1.load(), one);
        QCOMPARE(atomic2.load(), two);
        QCOMPARE(atomic3.load(), three);

        QCOMPARE(atomic1.fetchAndStoreRelease(two), one);
        QCOMPARE(atomic2.fetchAndStoreRelease(three), two);
        QCOMPARE(atomic3.fetchAndStoreRelease(one), three);

        QCOMPARE(atomic1.load(), two);
        QCOMPARE(atomic2.load(), three);
        QCOMPARE(atomic3.load(), one);
    }

    {
        QAtomicPointer<void> atomic1 = one;
        QAtomicPointer<void> atomic2 = two;
        QAtomicPointer<void> atomic3 = three;

        QCOMPARE(atomic1.load(), one);
        QCOMPARE(atomic2.load(), two);
        QCOMPARE(atomic3.load(), three);

        QCOMPARE(atomic1.fetchAndStoreOrdered(two), one);
        QCOMPARE(atomic2.fetchAndStoreOrdered(three), two);
        QCOMPARE(atomic3.fetchAndStoreOrdered(one), three);

        QCOMPARE(atomic1.load(), two);
        QCOMPARE(atomic2.load(), three);
        QCOMPARE(atomic3.load(), one);
    }
}

void tst_QAtomicPointer::isFetchAndAddNative()
{
#if defined(Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicPointer<void>::isFetchAndAddNative());

#  if (defined(Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_SOMETIMES_NATIVE)     \
       || defined(Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_SOMETIMES_NATIVE)
    // could be either, just want to make sure the function is implemented
    QVERIFY(QAtomicPointer<void>::isFetchAndAddNative()
            || !QAtomicPointer<void>::isFetchAndAddNative());

#  if (defined(Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_NOT_NATIVE))
#    error "Define only one of Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#elif defined(Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_NOT_NATIVE)
    // the runtime test should say the same thing
    QVERIFY(!QAtomicPointer<void>::isFetchAndAddNative());

#  if (defined(Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE) \
       || defined(Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_SOMETIMES_NATIVE))
#    error "Define only one of Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE"
#  endif
#else
#  error "Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NOT}_NATIVE is not defined"
#endif
}

void tst_QAtomicPointer::isFetchAndAddWaitFree()
{
#if defined(Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_WAIT_FREE)
    // the runtime test should say the same thing
    QVERIFY(QAtomicPointer<void>::isFetchAndAddWaitFree());

    // enforce some invariants
    QVERIFY(QAtomicPointer<void>::isFetchAndAddNative());
#  if defined(Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
    // the runtime test should say the same thing
    QVERIFY(!QAtomicPointer<void>::isFetchAndAddWaitFree());
#endif
}

void tst_QAtomicPointer::fetchAndAdd_data()
{
    QTest::addColumn<int>("valueToAdd");

    QTest::newRow("0") << 0;
    QTest::newRow("1") << 1;
    QTest::newRow("2") << 2;
    QTest::newRow("10") << 10;
    QTest::newRow("31") << 31;
    QTest::newRow("51") << 51;
    QTest::newRow("72") << 72;
    QTest::newRow("810") << 810;
    QTest::newRow("631") << 631;
    QTest::newRow("451") << 451;
    QTest::newRow("272") << 272;
    QTest::newRow("1810") << 1810;
    QTest::newRow("3631") << 3631;
    QTest::newRow("5451") << 5451;
    QTest::newRow("7272") << 7272;
    QTest::newRow("-1") << -1;
    QTest::newRow("-2") << -2;
    QTest::newRow("-10") << -10;
    QTest::newRow("-31") << -31;
    QTest::newRow("-51") << -51;
    QTest::newRow("-72") << -72;
    QTest::newRow("-810") << -810;
    QTest::newRow("-631") << -631;
    QTest::newRow("-451") << -451;
    QTest::newRow("-272") << -272;
    QTest::newRow("-1810") << -1810;
    QTest::newRow("-3631") << -3631;
    QTest::newRow("-5451") << -5451;
    QTest::newRow("-7272") << -7272;
}

void tst_QAtomicPointer::fetchAndAdd()
{
    QFETCH(int, valueToAdd);

    char c;
    char *pc = &c;
    short s;
    short *ps = &s;
    int i;
    int *pi = &i;

    {
        QAtomicPointer<char> pointer1 = pc;
        // cast to void* in order to avoid QCOMPARE to compare string content of the char*
        QCOMPARE(static_cast<void*>(pointer1.fetchAndAddRelaxed(valueToAdd)), static_cast<void*>(pc));
        QCOMPARE(static_cast<void*>(pointer1.fetchAndAddRelaxed(-valueToAdd)), static_cast<void*>(pc + valueToAdd));
        QCOMPARE(static_cast<void*>(pointer1.load()), static_cast<void*>(pc));
        QAtomicPointer<short> pointer2 = ps;
        QCOMPARE(pointer2.fetchAndAddRelaxed(valueToAdd), ps);
        QCOMPARE(pointer2.fetchAndAddRelaxed(-valueToAdd), ps + valueToAdd);
        QCOMPARE(pointer2.load(), ps);
        QAtomicPointer<int> pointer3 = pi;
        QCOMPARE(pointer3.fetchAndAddRelaxed(valueToAdd), pi);
        QCOMPARE(pointer3.fetchAndAddRelaxed(-valueToAdd), pi + valueToAdd);
        QCOMPARE(pointer3.load(), pi);
    }

    {
        QAtomicPointer<char> pointer1 = pc;
        QCOMPARE(static_cast<void*>(pointer1.fetchAndAddAcquire(valueToAdd)), static_cast<void*>(pc));
        QCOMPARE(static_cast<void*>(pointer1.fetchAndAddAcquire(-valueToAdd)), static_cast<void*>(pc + valueToAdd));
        QCOMPARE(static_cast<void*>(pointer1.load()), static_cast<void*>(pc));
        QAtomicPointer<short> pointer2 = ps;
        QCOMPARE(pointer2.fetchAndAddAcquire(valueToAdd), ps);
        QCOMPARE(pointer2.fetchAndAddAcquire(-valueToAdd), ps + valueToAdd);
        QCOMPARE(pointer2.load(), ps);
        QAtomicPointer<int> pointer3 = pi;
        QCOMPARE(pointer3.fetchAndAddAcquire(valueToAdd), pi);
        QCOMPARE(pointer3.fetchAndAddAcquire(-valueToAdd), pi + valueToAdd);
        QCOMPARE(pointer3.load(), pi);
    }

    {
        QAtomicPointer<char> pointer1 = pc;
        QCOMPARE(static_cast<void*>(pointer1.fetchAndAddRelease(valueToAdd)), static_cast<void*>(pc));
        QCOMPARE(static_cast<void*>(pointer1.fetchAndAddRelease(-valueToAdd)), static_cast<void*>(pc + valueToAdd));
        QCOMPARE(static_cast<void*>(pointer1.load()), static_cast<void*>(pc));
        QAtomicPointer<short> pointer2 = ps;
        QCOMPARE(pointer2.fetchAndAddRelease(valueToAdd), ps);
        QCOMPARE(pointer2.fetchAndAddRelease(-valueToAdd), ps + valueToAdd);
        QCOMPARE(pointer2.load(), ps);
        QAtomicPointer<int> pointer3 = pi;
        QCOMPARE(pointer3.fetchAndAddRelease(valueToAdd), pi);
        QCOMPARE(pointer3.fetchAndAddRelease(-valueToAdd), pi + valueToAdd);
        QCOMPARE(pointer3.load(), pi);
    }

    {
        QAtomicPointer<char> pointer1 = pc;
        QCOMPARE(static_cast<void*>(pointer1.fetchAndAddOrdered(valueToAdd)), static_cast<void*>(pc));
        QCOMPARE(static_cast<void*>(pointer1.fetchAndAddOrdered(-valueToAdd)), static_cast<void*>(pc + valueToAdd));
        QCOMPARE(static_cast<void*>(pointer1.load()), static_cast<void*>(pc));
        QAtomicPointer<short> pointer2 = ps;
        QCOMPARE(pointer2.fetchAndAddOrdered(valueToAdd), ps);
        QCOMPARE(pointer2.fetchAndAddOrdered(-valueToAdd), ps + valueToAdd);
        QCOMPARE(pointer2.load(), ps);
        QAtomicPointer<int> pointer3 = pi;
        QCOMPARE(pointer3.fetchAndAddOrdered(valueToAdd), pi);
        QCOMPARE(pointer3.fetchAndAddOrdered(-valueToAdd), pi + valueToAdd);
        QCOMPARE(pointer3.load(), pi);
    }
}

template <typename T> void constAndVolatile_helper()
{
    T *one = 0;
    T *two = &one;
    T *three = &two;

    {
        QAtomicPointer<T> atomic1 = one;
        QAtomicPointer<T> atomic2 = two;
        QAtomicPointer<T> atomic3 = three;

        QVERIFY(atomic1.load() == one);
        QVERIFY(atomic2.load() == two);
        QVERIFY(atomic3.load() == three);

        QVERIFY(atomic1.fetchAndStoreRelaxed(two) == one);
        QVERIFY(atomic2.fetchAndStoreRelaxed(three) == two);
        QVERIFY(atomic3.fetchAndStoreRelaxed(one) == three);

        QVERIFY(atomic1.load() == two);
        QVERIFY(atomic2.load() == three);
        QVERIFY(atomic3.load() == one);
    }
    {
        QAtomicPointer<T> atomic1 = one;
        QAtomicPointer<T> atomic2 = two;
        QAtomicPointer<T> atomic3 = three;

        QVERIFY(atomic1.load() == one);
        QVERIFY(atomic2.load() == two);
        QVERIFY(atomic3.load() == three);

        QVERIFY(atomic1.testAndSetRelaxed(one, two));
        QVERIFY(atomic2.testAndSetRelaxed(two, three));
        QVERIFY(atomic3.testAndSetRelaxed(three, one));

        QVERIFY(atomic1.load() == two);
        QVERIFY(atomic2.load() == three);
        QVERIFY(atomic3.load() == one);
    }

}

void tst_QAtomicPointer::constAndVolatile()
{
    constAndVolatile_helper<void>();
    constAndVolatile_helper<const void>();
    constAndVolatile_helper<volatile void>();
    constAndVolatile_helper<const volatile void>();
}

struct ForwardDeclared;
struct ContainsForwardDeclared
{
    QAtomicPointer<ForwardDeclared> ptr;
};

void tst_QAtomicPointer::forwardDeclared()
{
    // this is just a compilation test
    QAtomicPointer<ForwardDeclared> ptr;
    ContainsForwardDeclared cfd;
    Q_UNUSED(ptr);
    Q_UNUSED(cfd);
    QVERIFY(true);
}

template <typename T> static void operators_helper()
{
    typedef T *Ptr;
    T array[3] = {};
    Ptr zero = array;
    Ptr one = array + 1;
    Ptr two = array + 2;

    {
        // Test that QBasicAtomicPointer also has operator= and cast operators
        // We've been using them for QAtomicPointer<T> elsewhere
        QBasicAtomicPointer<T> atomic = Q_BASIC_ATOMIC_INITIALIZER(0);
        atomic = one;
        QCOMPARE(Ptr(atomic), one);
    }

    QAtomicPointer<T> atomic = zero;
    Ptr x = ++atomic;
    QCOMPARE(Ptr(atomic), x);
    QCOMPARE(Ptr(atomic), one);

    x = atomic++;
    QCOMPARE(Ptr(atomic), x + 1);
    QCOMPARE(Ptr(atomic), two);

    x = atomic--;
    QCOMPARE(Ptr(atomic), x - 1);
    QCOMPARE(Ptr(atomic), one);

    x = --atomic;
    QCOMPARE(Ptr(atomic), x);
    QCOMPARE(Ptr(atomic), zero);

    x = (atomic += 1);
    QCOMPARE(Ptr(atomic), x);
    QCOMPARE(Ptr(atomic), one);

    x = (atomic -= 1);
    QCOMPARE(Ptr(atomic), x);
    QCOMPARE(Ptr(atomic), zero);
}

struct Big { double d[10]; };
void tst_QAtomicPointer::operators()
{
    operators_helper<char>();
    operators_helper<int>();
    operators_helper<double>();
    operators_helper<Big>();
}

QTEST_APPLESS_MAIN(tst_QAtomicPointer)
#include "tst_qatomicpointer.moc"
