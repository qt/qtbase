// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sérgio Martins <sergio.martins@kdab.com>
// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/QScopeGuard>

#include <optional>

/*!
 \class tst_QScopeGuard
 \internal
 \since 5.11
 \brief Tests class QScopeGuard and function qScopeGuard

 */
class tst_QScopeGuard : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void construction();
    void constructionFromLvalue();
    void constructionFromRvalue();
    void optionalGuard();
    void leavingScope();
    void exceptions();
};

void func()
{
}

int intFunc()
{
    return 0;
}

[[nodiscard]] int noDiscardFunc()
{
    return 0;
}

struct Callable
{
    Callable() { }
    Callable(const Callable &other)
    {
        Q_UNUSED(other);
        ++copied;
    }
    Callable(Callable &&other)
    {
        Q_UNUSED(other);
        ++moved;
    }
    void operator()() { }

    static int copied;
    static int moved;
    static void resetCounts()
    {
        copied = 0;
        moved = 0;
    }
};

int Callable::copied = 0;
int Callable::moved = 0;

static int s_globalState = 0;

void tst_QScopeGuard::construction()
{
    QScopeGuard fromLambda([] { });
    QScopeGuard fromFunction(func);
    QScopeGuard fromFunctionPointer(&func);
    QScopeGuard fromNonVoidFunction(intFunc);
    QScopeGuard fromNoDiscardFunction(noDiscardFunc);
#ifndef __apple_build_version__
    QScopeGuard fromStdFunction{std::function<void()>(func)};
    std::function<void()> stdFunction(func);
    QScopeGuard fromNamedStdFunction(stdFunction);
#endif
}

void tst_QScopeGuard::constructionFromLvalue()
{
    Callable::resetCounts();
    {
        Callable callable;
        QScopeGuard guard(callable);
    }
    QCOMPARE(Callable::copied, 1);
    QCOMPARE(Callable::moved, 0);
    Callable::resetCounts();
    {
        Callable callable;
        auto guard = qScopeGuard(callable);
    }
    QCOMPARE(Callable::copied, 1);
    QCOMPARE(Callable::moved, 0);
}

void tst_QScopeGuard::constructionFromRvalue()
{
    Callable::resetCounts();
    {
        Callable callable;
        QScopeGuard guard(std::move(callable));
    }
    QCOMPARE(Callable::copied, 0);
    QCOMPARE(Callable::moved, 1);
    Callable::resetCounts();
    {
        Callable callable;
        auto guard = qScopeGuard(std::move(callable));
    }
    QCOMPARE(Callable::copied, 0);
    QCOMPARE(Callable::moved, 1);
}

void tst_QScopeGuard::optionalGuard()
{
    int i = 0;
    auto lambda = [&] { ++i; };
    std::optional sg = false ? std::optional{qScopeGuard(lambda)} : std::nullopt;
    QVERIFY(!sg);
    QCOMPARE(i, 0);
    sg.emplace(qScopeGuard(lambda));
    QVERIFY(sg);
    sg->dismiss();
    sg.reset();
    QCOMPARE(i, 0);
    sg.emplace(qScopeGuard(lambda));
    QCOMPARE(i, 0);
    sg.reset();
    QCOMPARE(i, 1);
}

void tst_QScopeGuard::leavingScope()
{
    auto cleanup = qScopeGuard([] { s_globalState++; QCOMPARE(s_globalState, 3); });
    QCOMPARE(s_globalState, 0);

    {
        auto cleanup = qScopeGuard([] { s_globalState++; });
        QCOMPARE(s_globalState, 0);
    }

    QCOMPARE(s_globalState, 1);
    s_globalState++;
}

void tst_QScopeGuard::exceptions()
{
    s_globalState = 0;
    bool caught = false;
    QT_TRY
    {
        auto cleanup = qScopeGuard([] { s_globalState++; });
        QT_THROW(std::bad_alloc()); //if Qt compiled without exceptions this is noop
        s_globalState = 100;
    }
    QT_CATCH(...)
    {
        caught = true;
        QCOMPARE(s_globalState, 1);
    }

    QVERIFY((caught && s_globalState == 1) || (!caught && s_globalState == 101));

}

QTEST_MAIN(tst_QScopeGuard)
#include "tst_qscopeguard.moc"
