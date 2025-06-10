// Copyright (C) 2025 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QtCore/qcheckedint_impl.h>
#include <QtCore/qscopedvaluerollback.h>

#include <climits>
#include <type_traits>

class tst_QCheckedInt : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void unary();

    void addition();
    void subtraction();
    void multiplication();
    void division();
};

static bool checkedIntErrorDetected = false;

struct TestPolicy
{
    static void check(bool ok, const char *, const char *)
    {
        if (checkedIntErrorDetected)
            qFatal("Logical error in the test, did you reset the flag?");

        if (!ok)
            checkedIntErrorDetected = true;
    }
};

void tst_QCheckedInt::init()
{
    checkedIntErrorDetected = false;
}

using CheckedInt =
    QtPrivate::QCheckedIntegers::QCheckedInt<
        int,
        QtPrivate::QCheckedIntegers::SafeCheckImpl<int>,
        TestPolicy>;

void tst_QCheckedInt::unary()
{
    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(10);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 10);

        ++a;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 11);

        a.setValue(INT_MAX);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MAX);

        --a;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MAX - 1);

        a++;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MAX);

        ++a;
        QVERIFY(checkedIntErrorDetected);
    }

    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(INT_MIN);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MIN);

        ++a;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MIN + 1);

        --a;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MIN);

        --a;
        QVERIFY(checkedIntErrorDetected);
    }

    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(0);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 0);

        a = -a;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 0);

        a.setValue(10);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 10);

        a = -a;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), -10);

        a = +a;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), -10);

        a = -a;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 10);

        a.setValue(INT_MAX);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MAX);

        a = -a;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), -INT_MAX);

        a.setValue(INT_MIN);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MIN);

        a = -a;
        QVERIFY(checkedIntErrorDetected);
    }
}

void tst_QCheckedInt::addition()
{
    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(10);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 10);

        a += 10;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 20);

        a = a + 10;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 30);
    }

    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(42);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 42);

        a += 10;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 52);

        a += INT_MAX;
        QVERIFY(checkedIntErrorDetected);
    }

    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(5);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 5);

        a += -10;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), -5);

        a += INT_MIN;
        QVERIFY(checkedIntErrorDetected);
    }
}

void tst_QCheckedInt::subtraction()
{
    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(42);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 42);

        a -= 10;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 32);

        a = a - 10;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 22);
    }

    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(0);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 0);

        a -= INT_MIN;
        QVERIFY(checkedIntErrorDetected);
    }

    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(INT_MIN);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MIN);

        a -= 1;
        QVERIFY(checkedIntErrorDetected);
    }

    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(INT_MAX);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MAX);

        a -= -1;
        QVERIFY(checkedIntErrorDetected);
    }
}

void tst_QCheckedInt::multiplication()
{
    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(10);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 10);

        a *= 10;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 100);

        a *= -10;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), -1000);
    }

    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(INT_MAX / 4);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MAX / 4);

        a *= 2;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MAX / 2 - 1);

        a *= 2;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MAX - 3);

        a *= 2;
        QVERIFY(checkedIntErrorDetected);
    }

    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(INT_MAX - 1);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MAX - 1);

        a *= INT_MAX;
        QVERIFY(checkedIntErrorDetected);
    }

    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(INT_MIN / 4);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MIN / 4);

        a *= 2;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MIN / 2);

        a *= 2;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MIN);

        a *= 2;
        QVERIFY(checkedIntErrorDetected);
    }

    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(INT_MIN);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), INT_MIN);

        a *= -1;
        QVERIFY(checkedIntErrorDetected);
    }
}

void tst_QCheckedInt::division()
{
    {
        QScopedValueRollback rb(checkedIntErrorDetected);

        CheckedInt a(100);
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 100);

        a /= -4;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), -25);

        a = a / -10;
        QVERIFY(!checkedIntErrorDetected);
        QCOMPARE(a.value(), 2);

        a = a / 0;
        QVERIFY(checkedIntErrorDetected);
    }
}


// compile-time tests

// This causes an internal compiler error on MSVC, so skipping it there
// Integrity's compiler says this code isn't constexpr.
#if (!defined(Q_CC_MSVC) || Q_CC_MSVC > 1944) && !defined(Q_CC_GHS)

template <typename T>
constexpr bool checkedIntTypeProperties()
{
    using Int = QtPrivate::QCheckedIntegers::QCheckedInt<T>;
    static_assert(sizeof(Int) == sizeof(T));
    static_assert(alignof(Int) == alignof(T));
    static_assert(std::is_trivially_constructible_v<Int>);
    static_assert(std::is_trivially_copy_constructible_v<Int>);
    static_assert(std::is_trivially_move_constructible_v<Int>);
    static_assert(std::is_trivially_copy_assignable_v<Int>);
    static_assert(std::is_trivially_move_assignable_v<Int>);
    static_assert(std::is_trivially_destructible_v<Int>);

    Int i(T(10));
    i += T(10);
    i *= T(5);
    i -= T(200);
    i = -i;
    i /= T(5);

    return i == i;
}

static_assert(checkedIntTypeProperties<int>());
static_assert(checkedIntTypeProperties<long>());
static_assert(checkedIntTypeProperties<long long>());

#endif // MSVC / GHS

QTEST_APPLESS_MAIN(tst_QCheckedInt)
#include "tst_qcheckedint.moc"
