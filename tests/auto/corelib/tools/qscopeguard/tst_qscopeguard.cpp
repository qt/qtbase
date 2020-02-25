/****************************************************************************
**
** Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sérgio Martins <sergio.martins@kdab.com>
** Copyright (C) 2019 The Qt Company Ltd.
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
#include <QtCore/QScopeGuard>

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

Q_REQUIRED_RESULT int noDiscardFunc()
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
#ifdef __cpp_deduction_guides
    QScopeGuard fromLambda([] { });
    QScopeGuard fromFunction(func);
    QScopeGuard fromFunctionPointer(&func);
    QScopeGuard fromNonVoidFunction(intFunc);
    QScopeGuard fromNoDiscardFunction(noDiscardFunc);
    QScopeGuard fromStdFunction{std::function<void()>(func)};
    std::function<void()> stdFunction(func);
    QScopeGuard fromNamedStdFunction(stdFunction);
#else
    QSKIP("This test requires C++17 Class Template Argument Deduction support enabled in the compiler.");
#endif
}

void tst_QScopeGuard::constructionFromLvalue()
{
#ifdef __cpp_deduction_guides
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
#else
    QSKIP("This test requires C++17 Class Template Argument Deduction support enabled in the compiler.");
#endif
}

void tst_QScopeGuard::constructionFromRvalue()
{
#ifdef __cpp_deduction_guides
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
#else
    QSKIP("This test requires C++17 Class Template Argument Deduction support enabled in the compiler.");
#endif
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
