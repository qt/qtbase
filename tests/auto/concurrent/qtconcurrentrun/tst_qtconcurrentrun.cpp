/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <qtconcurrentrun.h>
#include <qfuture.h>
#include <QString>
#include <QtTest/QtTest>

using namespace QtConcurrent;

class tst_QtConcurrentRun: public QObject
{
    Q_OBJECT
private slots:
    void runLightFunction();
    void runHeavyFunction();
    void returnValue();
    void functionObject();
    void memberFunctions();
    void implicitConvertibleTypes();
    void runWaitLoop();
    void pollForIsFinished();
    void recursive();
#ifndef QT_NO_EXCEPTIONS
    void exceptions();
#endif
#ifdef Q_COMPILER_DECLTYPE
    void functor();
#endif
#ifdef Q_COMPILER_LAMBDA
    void lambda();
#endif
};

void light()
{
    qDebug("in function");
    qDebug("done function");
}

void heavy()
{
    qDebug("in function");
    QString str;
    for (int i = 0; i < 1000000; ++i)
        str.append("a");
    qDebug("done function");
}


void tst_QtConcurrentRun::runLightFunction()
{
    qDebug("starting function");
    QFuture<void> future = run(light);
    qDebug("waiting");
    future.waitForFinished();
    qDebug("done");
}

void tst_QtConcurrentRun::runHeavyFunction()
{
    QThreadPool pool;
    qDebug("starting function");
    QFuture<void> future = run(&pool, heavy);
    qDebug("waiting");
    future.waitForFinished();
    qDebug("done");
}

int returnInt0()
{
    return 10;
}

int returnInt1(int i)
{
    return i;
}

class A
{
public:
    int member0() { return 10; }
    int member1(int in) { return in; }

    typedef int result_type;
    int operator()() { return 10; }
    int operator()(int in) { return in; }
};

class AConst
{
public:
    int member0() const { return 10; }
    int member1(int in) const { return in; }

    typedef int result_type;
    int operator()() const { return 10; }
    int operator()(int in) const { return in; }
};

void tst_QtConcurrentRun::returnValue()
{
    QThreadPool pool;
    QFuture<int> f;

    f = run(returnInt0);
    QCOMPARE(f.result(), 10);
    f = run(&pool, returnInt0);
    QCOMPARE(f.result(), 10);

    A a;
    f = run(&a, &A::member0);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &a, &A::member0);
    QCOMPARE(f.result(), 10);

    f = run(&a, &A::member1, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &a, &A::member1, 20);
    QCOMPARE(f.result(), 20);

    f = run(a, &A::member0);
    QCOMPARE(f.result(), 10);
    f = run(&pool, a, &A::member0);
    QCOMPARE(f.result(), 10);

    f = run(a, &A::member1, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, a, &A::member1, 20);
    QCOMPARE(f.result(), 20);

    f = run(a);
    QCOMPARE(f.result(), 10);
    f = run(&pool, a);
    QCOMPARE(f.result(), 10);

    f = run(&a);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &a);
    QCOMPARE(f.result(), 10);

    f = run(a, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, a, 20);
    QCOMPARE(f.result(), 20);

    f = run(&a, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &a, 20);
    QCOMPARE(f.result(), 20);

    const AConst aConst = AConst();
    f = run(&aConst, &AConst::member0);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &aConst, &AConst::member0);
    QCOMPARE(f.result(), 10);

    f = run(&aConst, &AConst::member1, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &aConst, &AConst::member1, 20);
    QCOMPARE(f.result(), 20);

    f = run(aConst, &AConst::member0);
    QCOMPARE(f.result(), 10);
    f = run(&pool, aConst, &AConst::member0);
    QCOMPARE(f.result(), 10);

    f = run(aConst, &AConst::member1, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, aConst, &AConst::member1, 20);
    QCOMPARE(f.result(), 20);

    f = run(aConst);
    QCOMPARE(f.result(), 10);
    f = run(&pool, aConst);
    QCOMPARE(f.result(), 10);

    f = run(&aConst);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &aConst);
    QCOMPARE(f.result(), 10);

    f = run(aConst, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, aConst, 20);
    QCOMPARE(f.result(), 20);

    f = run(&aConst, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &aConst, 20);
    QCOMPARE(f.result(), 20);
}

struct TestClass
{
    void foo() { }
    typedef void result_type;
    void operator()() { }
    void operator()(int) { }
    void fooInt(int){ };
};

struct TestConstClass
{
    void foo() const { }
    typedef void result_type;
    void operator()() const { }
    void operator()(int) const { }
    void fooInt(int) const { };
};

void tst_QtConcurrentRun::functionObject()
{
    QThreadPool pool;
    QFuture<void> f;
    TestClass c;

    f = run(c);
    f = run(&c);
    f = run(c, 10);
    f = run(&c, 10);

    f = run(&pool, c);
    f = run(&pool, &c);
    f = run(&pool, c, 10);
    f = run(&pool, &c, 10);

    const TestConstClass cc = TestConstClass();
    f = run(cc);
    f = run(&cc);
    f = run(cc, 10);
    f = run(&cc, 10);

    f = run(&pool, cc);
    f = run(&pool, &cc);
    f = run(&pool, cc, 10);
    f = run(&pool, &cc, 10);
}


void tst_QtConcurrentRun::memberFunctions()
{
    QThreadPool pool;

    TestClass c;

    run(c, &TestClass::foo).waitForFinished();
    run(&c, &TestClass::foo).waitForFinished();
    run(c, &TestClass::fooInt, 10).waitForFinished();
    run(&c, &TestClass::fooInt, 10).waitForFinished();

    run(&pool, c, &TestClass::foo).waitForFinished();
    run(&pool, &c, &TestClass::foo).waitForFinished();
    run(&pool, c, &TestClass::fooInt, 10).waitForFinished();
    run(&pool, &c, &TestClass::fooInt, 10).waitForFinished();

    const TestConstClass cc = TestConstClass();
    run(cc, &TestConstClass::foo).waitForFinished();
    run(&cc, &TestConstClass::foo).waitForFinished();
    run(cc, &TestConstClass::fooInt, 10).waitForFinished();
    run(&cc, &TestConstClass::fooInt, 10).waitForFinished();

    run(&pool, cc, &TestConstClass::foo).waitForFinished();
    run(&pool, &cc, &TestConstClass::foo).waitForFinished();
    run(&pool, cc, &TestConstClass::fooInt, 10).waitForFinished();
    run(&pool, &cc, &TestConstClass::fooInt, 10).waitForFinished();
}


void doubleFunction(double)
{

}

void stringConstRefFunction(const QString &)
{

}

void stringRefFunction(QString &)
{

}

void stringFunction(QString)
{

}

void stringIntFunction(QString)
{

}


void tst_QtConcurrentRun::implicitConvertibleTypes()
{
    QThreadPool pool;

    double d;
    run(doubleFunction, d).waitForFinished();
    run(&pool, doubleFunction, d).waitForFinished();
    int i;
    run(doubleFunction, d).waitForFinished();
    run(&pool, doubleFunction, d).waitForFinished();
    run(doubleFunction, i).waitForFinished();
    run(&pool, doubleFunction, i).waitForFinished();
    run(doubleFunction, 10).waitForFinished();
    run(&pool, doubleFunction, 10).waitForFinished();
    run(stringFunction, QLatin1String("Foo")).waitForFinished();
    run(&pool, stringFunction, QLatin1String("Foo")).waitForFinished();
    run(stringConstRefFunction, QLatin1String("Foo")).waitForFinished();
    run(&pool, stringConstRefFunction, QLatin1String("Foo")).waitForFinished();
    QString string;
    run(stringRefFunction, string).waitForFinished();
    run(&pool, stringRefFunction, string).waitForFinished();
}

void fn() { }

void tst_QtConcurrentRun::runWaitLoop()
{
    for (int i = 0; i < 1000; ++i)
        run(fn).waitForFinished();
}

static bool allFinished(const QList<QFuture<void> > &futures)
{
    auto hasNotFinished = [](const QFuture<void> &future) { return !future.isFinished(); };
    return std::find_if(futures.cbegin(), futures.cend(), hasNotFinished)
        == futures.constEnd();
}

static void runFunction()
{
    QEventLoop loop;
    QTimer::singleShot(20, &loop, &QEventLoop::quit);
    loop.exec();
}

void tst_QtConcurrentRun::pollForIsFinished()
{
    const int numThreads = std::max(4, 2 * QThread::idealThreadCount());
    QThreadPool::globalInstance()->setMaxThreadCount(numThreads);

    QFutureSynchronizer<void> synchronizer;
    for (int i = 0; i < numThreads; ++i)
        synchronizer.addFuture(QtConcurrent::run(&runFunction));

    // same as synchronizer.waitForFinished() but with a timeout
    QTRY_VERIFY(allFinished(synchronizer.futures()));
}


QAtomicInt count;

void recursiveRun(int level)
{
    count.ref();
    if (--level > 0) {
        QFuture<void> f1 = run(recursiveRun, level);
        QFuture<void> f2 = run(recursiveRun, level);
        f1.waitForFinished();
        f2.waitForFinished();
    }
}

int recursiveResult(int level)
{
    count.ref();
    if (--level > 0) {
        QFuture<int> f1 = run(recursiveResult, level);
        QFuture<int> f2 = run(recursiveResult, level);
        return f1.result() + f2.result();
    }
    return 1;
}

void tst_QtConcurrentRun::recursive()
{
    int levels = 15;

    for (int i = 0; i < QThread::idealThreadCount(); ++i) {
        count.store(0);
        QThreadPool::globalInstance()->setMaxThreadCount(i);
        recursiveRun(levels);
        QCOMPARE(count.load(), (int)std::pow(2.0, levels) - 1);
    }

    for (int i = 0; i < QThread::idealThreadCount(); ++i) {
        count.store(0);
        QThreadPool::globalInstance()->setMaxThreadCount(i);
        recursiveResult(levels);
        QCOMPARE(count.load(), (int)std::pow(2.0, levels) - 1);
    }
}

int e;
void vfn0()
{
    ++e;
}

int fn0()
{
    return 1;
}

void vfn1(double)
{
    ++e;
}

int fn1(int)
{
    return 1;
}

void vfn2(double, int *)
{
    ++e;
}

int fn2(double, int *)
{
    return 1;
}


#ifndef QT_NO_EXCEPTIONS
void throwFunction()
{
    throw QException();
}

int throwFunctionReturn()
{
    throw QException();
    return 0;
}

void tst_QtConcurrentRun::exceptions()
{
    QThreadPool pool;
    bool caught;

    caught = false;
    try  {
        QtConcurrent::run(throwFunction).waitForFinished();
    } catch (QException &) {
        caught = true;
    }
    if (!caught)
        QFAIL("did not get exception");

    caught = false;
    try  {
        QtConcurrent::run(&pool, throwFunction).waitForFinished();
    } catch (QException &) {
        caught = true;
    }
    if (!caught)
        QFAIL("did not get exception");

    caught = false;
    try  {
        QtConcurrent::run(throwFunctionReturn).waitForFinished();
    } catch (QException &) {
        caught = true;
    }
    if (!caught)
        QFAIL("did not get exception");

    caught = false;
    try  {
        QtConcurrent::run(&pool, throwFunctionReturn).waitForFinished();
    } catch (QException &) {
        caught = true;
    }
    if (!caught)
        QFAIL("did not get exception");
}
#endif

#ifdef Q_COMPILER_DECLTYPE
// Compiler supports decltype
struct Functor {
    int operator()() { return 42; }
    double operator()(double a, double b) { return a/b; }
    int operator()(int a, int b) { return a/b; }
    void operator()(int) { }
    void operator()(int, int, int) { }
    void operator()(int, int, int, int) { }
    void operator()(int, int, int, int, int) { }
    void operator()(int, int, int, int, int, int) { }
};

// This tests functor without result_type; decltype need to be supported by the compiler.
void tst_QtConcurrentRun::functor()
{
    Functor f;
    {
        QFuture<int> fut = QtConcurrent::run(f);
        QCOMPARE(fut.result(), 42);
    }
    {
        QFuture<double> fut = QtConcurrent::run(f, 8.5, 1.8);
        QCOMPARE(fut.result(), (8.5/1.8));
    }
    {
        QFuture<int> fut = QtConcurrent::run(f, 19, 3);
        QCOMPARE(fut.result(), int(19/3));
    }
    {
        QtConcurrent::run(f, 1).waitForFinished();
        QtConcurrent::run(f, 1,2).waitForFinished();
        QtConcurrent::run(f, 1,2,3).waitForFinished();
        QtConcurrent::run(f, 1,2,3,4).waitForFinished();
        QtConcurrent::run(f, 1,2,3,4,5).waitForFinished();
    }
    // and now with explicit pool:
    QThreadPool pool;
    {
        QFuture<int> fut = QtConcurrent::run(&pool, f);
        QCOMPARE(fut.result(), 42);
    }
    {
        QFuture<double> fut = QtConcurrent::run(&pool, f, 8.5, 1.8);
        QCOMPARE(fut.result(), (8.5/1.8));
    }
    {
        QFuture<int> fut = QtConcurrent::run(&pool, f, 19, 3);
        QCOMPARE(fut.result(), int(19/3));
    }
    {
        QtConcurrent::run(&pool, f, 1).waitForFinished();
        QtConcurrent::run(&pool, f, 1,2).waitForFinished();
        QtConcurrent::run(&pool, f, 1,2,3).waitForFinished();
        QtConcurrent::run(&pool, f, 1,2,3,4).waitForFinished();
        QtConcurrent::run(&pool, f, 1,2,3,4,5).waitForFinished();
    }
}
#endif

#ifdef Q_COMPILER_LAMBDA
// Compiler supports lambda
void tst_QtConcurrentRun::lambda()
{
    QCOMPARE(QtConcurrent::run([](){ return 45; }).result(), 45);
    QCOMPARE(QtConcurrent::run([](int a){ return a+15; }, 12).result(), 12+15);
    QCOMPARE(QtConcurrent::run([](int a, double b){ return a + b; }, 12, 15).result(), double(12+15));
    QCOMPARE(QtConcurrent::run([](int a , int, int, int, int b){ return a + b; }, 1, 2, 3, 4, 5).result(), 1 + 5);

#ifdef Q_COMPILER_INITIALIZER_LISTS
    {
        QString str { "Hello World Foo" };
        QFuture<QStringList> f1 = QtConcurrent::run([&](){ return str.split(' '); });
        auto r = f1.result();
        QCOMPARE(r, QStringList({"Hello", "World", "Foo"}));
    }
#endif

    // and now with explicit pool:
    QThreadPool pool;
    QCOMPARE(QtConcurrent::run(&pool, [](){ return 45; }).result(), 45);
    QCOMPARE(QtConcurrent::run(&pool, [](int a){ return a+15; }, 12).result(), 12+15);
    QCOMPARE(QtConcurrent::run(&pool, [](int a, double b){ return a + b; }, 12, 15).result(), double(12+15));
    QCOMPARE(QtConcurrent::run(&pool, [](int a , int, int, int, int b){ return a + b; }, 1, 2, 3, 4, 5).result(), 1 + 5);

#ifdef Q_COMPILER_INITIALIZER_LISTS
    {
        QString str { "Hello World Foo" };
        QFuture<QStringList> f1 = QtConcurrent::run(&pool, [&](){ return str.split(' '); });
        auto r = f1.result();
        QCOMPARE(r, QStringList({"Hello", "World", "Foo"}));
    }
#endif
}
#endif

QTEST_MAIN(tst_QtConcurrentRun)
#include "tst_qtconcurrentrun.moc"
