// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qatomicscopedvaluerollback.h>

#include <QTest>

#include <chrono>

using namespace std::chrono_literals;

class tst_QAtomicScopedValueRollback : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void leavingScope();
    void leavingScopeAfterCommit();
    void rollbackToPreviousCommit();
    void exceptions();
    void earlyExitScope();
    void mixedTypes();
private:
    void earlyExitScope_helper(int exitpoint, std::atomic<int> &member);
};

void tst_QAtomicScopedValueRollback::leavingScope()
{
    QAtomicInt i = 0;
    QBasicAtomicInteger<bool> b = false;
    std::atomic<bool> b2 = false;
    int x = 0, y = 42;
    QBasicAtomicPointer<int> p = &x;

    //test rollback on going out of scope
    {
        QAtomicScopedValueRollback ri(i);
        QAtomicScopedValueRollback rb(b);
        QAtomicScopedValueRollback rb2(b2, true);
        QAtomicScopedValueRollback rp(p);
        QCOMPARE(b.loadRelaxed(), false);
        QCOMPARE(b2, true);
        QCOMPARE(i.loadRelaxed(), 0);
        QCOMPARE(p.loadRelaxed(), &x);
        b.storeRelaxed(true);
        i.storeRelaxed(1);
        p.storeRelaxed(&y);
        QCOMPARE(b.loadRelaxed(), true);
        QCOMPARE(i.loadRelaxed(), 1);
        QCOMPARE(p.loadRelaxed(), &y);
    }
    QCOMPARE(b.loadRelaxed(), false);
    QCOMPARE(b2, false);
    QCOMPARE(i.loadRelaxed(), 0);
    QCOMPARE(p.loadRelaxed(), &x);
}

void tst_QAtomicScopedValueRollback::leavingScopeAfterCommit()
{
    std::atomic<int> i = 0;
    QAtomicInteger<bool> b = false;

    //test rollback on going out of scope
    {
        QAtomicScopedValueRollback ri(i);
        QAtomicScopedValueRollback rb(b);
        QCOMPARE(b.loadRelaxed(), false);
        QCOMPARE(i, 0);
        b.storeRelaxed(true);
        i = 1;
        QCOMPARE(b.loadRelaxed(), true);
        QCOMPARE(i, 1);
        ri.commit();
        rb.commit();
    }
    QCOMPARE(b.loadRelaxed(), true);
    QCOMPARE(i, 1);
}

void tst_QAtomicScopedValueRollback::rollbackToPreviousCommit()
{
    QBasicAtomicInt i = 0;
    {
        QAtomicScopedValueRollback ri(i);
        i++;
        ri.commit();
        i++;
    }
    QCOMPARE(i.loadRelaxed(), 1);
    {
        QAtomicScopedValueRollback ri1(i);
        i++;
        ri1.commit();
        i++;
        ri1.commit();
        i++;
    }
    QCOMPARE(i.loadRelaxed(), 3);
}

void tst_QAtomicScopedValueRollback::exceptions()
{
    std::atomic<bool> b = false;
    bool caught = false;
    QT_TRY
    {
        QAtomicScopedValueRollback rb(b);
        b = true;
        QT_THROW(std::bad_alloc()); //if Qt compiled without exceptions this is noop
        rb.commit(); //if Qt compiled without exceptions, true is committed
    }
    QT_CATCH(...)
    {
        caught = true;
    }
    QCOMPARE(b, !caught); //expect false if exception was thrown, true otherwise
}

void tst_QAtomicScopedValueRollback::earlyExitScope()
{
    QAtomicInt ai = 0;
    std::atomic<int> aj = 0;
    while (true) {
        QAtomicScopedValueRollback ri(ai);
        ++ai;
        aj = ai.loadRelaxed();
        if (ai.loadRelaxed() > 8) break;
        ri.commit();
    }
    QCOMPARE(ai.loadRelaxed(), 8);
    QCOMPARE(aj.load(), 9);

    for (int i = 0; i < 5; ++i) {
        aj = 1;
        earlyExitScope_helper(i, aj);
        QCOMPARE(aj.load(), 1 << i);
    }
}

template <typename T>
struct Wrap {
#if __cpp_deduction_guides < 201907L // no CTAD for aggregates
    Q_IMPLICIT Wrap(T &t) : t{t} {}
    Q_IMPLICIT Wrap(T &&t) : t{std::move(t)} {}
#endif
    T t;
    Q_IMPLICIT operator T() const { return t; }
};

void tst_QAtomicScopedValueRollback::mixedTypes()
{
    const auto relaxed = std::memory_order_relaxed;
    {
        std::atomic a{10'000ms};
        {
            QAtomicScopedValueRollback rb(a, 5s);
            QCOMPARE(a.load(relaxed), 5s);
        }
        QCOMPARE(a.load(relaxed), 10s);
        {
            QAtomicScopedValueRollback rb(a, 5s, relaxed);
            QCOMPARE(a.load(relaxed), 5s);
        }
        QCOMPARE(a.load(relaxed), 10s);
    }
    {
        QBasicAtomicInteger a{10'000};
        {
            QAtomicScopedValueRollback rb(a, Wrap{5'000});
            QCOMPARE(a.loadRelaxed(), 5'000);
        }
        QCOMPARE(a.loadRelaxed(), 10'000);
        {
            QAtomicScopedValueRollback rb(a, Wrap{5'000}, relaxed);
            QCOMPARE(a.loadRelaxed(), 5'000);
        }
        QCOMPARE(a.loadRelaxed(), 10'000);
    }
    {
        QAtomicInteger a{10'000};
        {
            QAtomicScopedValueRollback rb(a, Wrap{5'000});
            QCOMPARE(a.loadRelaxed(), 5'000);
        }
        QCOMPARE(a.loadRelaxed(), 10'000);
        {
            QAtomicScopedValueRollback rb(a, Wrap{5'000}, relaxed);
            QCOMPARE(a.loadRelaxed(), 5'000);
        }
        QCOMPARE(a.loadRelaxed(), 10'000);
    }
    {
        int i = 10'000;
        int j =  5'000;
        QBasicAtomicPointer a{&i};
        {
            QAtomicScopedValueRollback rb(a, Wrap{&j});
            QCOMPARE(a.loadRelaxed(), &j);
        }
        QCOMPARE(a.loadRelaxed(), &i);
        {
            QAtomicScopedValueRollback rb(a, Wrap{&j}, relaxed);
            QCOMPARE(a.loadRelaxed(), &j);
        }
        QCOMPARE(a.loadRelaxed(), &i);
    }
    {
        int i = 10'000;
        int j =  5'000;
        QAtomicPointer a{&i};
        {
            QAtomicScopedValueRollback rb(a, Wrap{&j});
            QCOMPARE(a.loadRelaxed(), &j);
        }
        QCOMPARE(a.loadRelaxed(), &i);
        {
            QAtomicScopedValueRollback rb(a, Wrap{&j}, relaxed);
            QCOMPARE(a.loadRelaxed(), &j);
        }
        QCOMPARE(a.loadRelaxed(), &i);
    }
}

static void operator*=(std::atomic<int> &lhs, int rhs)
{
    int expected = lhs.load();
    while (!lhs.compare_exchange_weak(expected, expected * rhs))
        ;
}

void tst_QAtomicScopedValueRollback::earlyExitScope_helper(int exitpoint, std::atomic<int>& member)
{
    QAtomicScopedValueRollback r(member);
    member *= 2;
    if (exitpoint == 0)
        return;
    r.commit();
    member *= 2;
    if (exitpoint == 1)
        return;
    r.commit();
    member *= 2;
    if (exitpoint == 2)
        return;
    r.commit();
    member *= 2;
    if (exitpoint == 3)
        return;
    r.commit();
}

QTEST_MAIN(tst_QAtomicScopedValueRollback)
#include "tst_qatomicscopedvaluerollback.moc"
