// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtconcurrentrun.h>
#include <QFuture>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QWaitCondition>
#include <QTest>
#include <QTimer>
#include <QFutureSynchronizer>

#include <QtTest/private/qemulationdetector_p.h>

using namespace QtConcurrent;

class tst_QtConcurrentRun: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void runLightFunction();
    void runHeavyFunction();
    void returnValue();
    void reportValueWithPromise();
    void functionObject();
    void memberFunctions();
    void implicitConvertibleTypes();
    void runWaitLoop();
    void pollForIsFinished();
    void recursive();
#ifndef QT_NO_EXCEPTIONS
    void exceptions();
    void unhandledException();
#endif
    void functor();
    void lambda();
    void callableObjectWithState();
    void withPromise();
    void withPromiseInThreadPool();
    void withPromiseAndThen();
    void moveOnlyType();
    void crefFunction();
    void customPromise();
    void nonDefaultConstructibleValue();
    void nullThreadPool();
    void nullThreadPoolNoLeak();
};

void light()
{
    qDebug("in function");
    qDebug("done function");
}

void lightOverloaded()
{
    qDebug("in function");
    qDebug("done function");
}

void lightOverloaded(int)
{
    qDebug("in function with arg");
    qDebug("done function");
}

void lightOverloaded(QPromise<int> &)
{
    qDebug("in function with promise");
    qDebug("done function");
}

void lightOverloaded(QPromise<double> &, int)
{
    qDebug("in function with promise and with arg");
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

void tst_QtConcurrentRun::initTestCase()
{
    // proxy check for QEMU; catches slightly more though
    if (QTestPrivate::isRunningArmOnX86())
        QSKIP("Runs into spurious crashes on QEMU -- QTBUG-106906");
}

void tst_QtConcurrentRun::runLightFunction()
{
    qDebug("starting function");
    QFuture<void> future = run(light);
    qDebug("waiting");
    future.waitForFinished();
    qDebug("done");

    void (*f1)() = lightOverloaded;
    qDebug("starting function");
    QFuture<void> future1 = run(f1);
    qDebug("waiting");
    future1.waitForFinished();
    qDebug("done");

    void (*f2)(int) = lightOverloaded;
    qDebug("starting function with arg");
    QFuture<void> future2 = run(f2, 2);
    qDebug("waiting");
    future2.waitForFinished();
    qDebug("done");

    void (*f3)(QPromise<int> &) = lightOverloaded;
    qDebug("starting function with promise");
    QFuture<int> future3 = run(f3);
    qDebug("waiting");
    future3.waitForFinished();
    qDebug("done");

    void (*f4)(QPromise<double> &, int v) = lightOverloaded;
    qDebug("starting function with promise and with arg");
    QFuture<double> future4 = run(f4, 2);
    qDebug("waiting");
    future4.waitForFinished();
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

    int operator()() { return 10; }
    int operator()(int in) { return in; }
};

class AConst
{
public:
    int member0() const { return 10; }
    int member1(int in) const { return in; }

    int operator()() const { return 10; }
    int operator()(int in) const { return in; }
};

class ANoExcept
{
public:
    int member0() noexcept { return 10; }
    int member1(int in) noexcept { return in; }

    int operator()() noexcept { return 10; }
    int operator()(int in) noexcept { return in; }
};

class AConstNoExcept
{
public:
    int member0() const noexcept { return 10; }
    int member1(int in) const noexcept { return in; }

    int operator()() const noexcept { return 10; }
    int operator()(int in) const noexcept { return in; }
};

void tst_QtConcurrentRun::returnValue()
{
    QThreadPool pool;
    QFuture<int> f;

    f = run(returnInt0);
    QCOMPARE(f.result(), 10);
    f = run(&pool, returnInt0);
    QCOMPARE(f.result(), 10);
    f = run(returnInt1, 4);
    QCOMPARE(f.result(), 4);
    f = run(&pool, returnInt1, 4);
    QCOMPARE(f.result(), 4);


    A a;
    f = run(&A::member0, &a);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &A::member0, &a);
    QCOMPARE(f.result(), 10);

    f = run(&A::member1, &a, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &A::member1, &a, 20);
    QCOMPARE(f.result(), 20);

    f = run(&A::member0, a);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &A::member0, a);
    QCOMPARE(f.result(), 10);

    f = run(&A::member1, a, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &A::member1, a, 20);
    QCOMPARE(f.result(), 20);

    f = run(a);
    QCOMPARE(f.result(), 10);
    f = run(&pool, a);
    QCOMPARE(f.result(), 10);

    f = run(a);
    QCOMPARE(f.result(), 10);
    f = run(&pool, std::ref(a));
    QCOMPARE(f.result(), 10);

    f = run(a, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, a, 20);
    QCOMPARE(f.result(), 20);

    f = run(std::ref(a), 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, std::ref(a), 20);
    QCOMPARE(f.result(), 20);


    const AConst aConst = AConst();
    f = run(&AConst::member0, &aConst);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &AConst::member0, &aConst);
    QCOMPARE(f.result(), 10);

    f = run(&AConst::member1, &aConst, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &AConst::member1, &aConst, 20);
    QCOMPARE(f.result(), 20);

    f = run(&AConst::member0, aConst);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &AConst::member0, aConst);
    QCOMPARE(f.result(), 10);

    f = run(&AConst::member1, aConst, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &AConst::member1, aConst, 20);
    QCOMPARE(f.result(), 20);

    f = run(aConst);
    QCOMPARE(f.result(), 10);
    f = run(&pool, aConst);
    QCOMPARE(f.result(), 10);

    f = run(std::ref(a));
    QCOMPARE(f.result(), 10);
    f = run(&pool, std::ref(a));
    QCOMPARE(f.result(), 10);

    f = run(aConst, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, aConst, 20);
    QCOMPARE(f.result(), 20);

    f = run(std::ref(aConst), 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, std::ref(aConst), 20);
    QCOMPARE(f.result(), 20);


    ANoExcept aNoExcept;
    f = run(&ANoExcept::member0, &aNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &ANoExcept::member0, &aNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(&ANoExcept::member1, &aNoExcept, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &ANoExcept::member1, &aNoExcept, 20);
    QCOMPARE(f.result(), 20);

    f = run(&ANoExcept::member0, aNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &ANoExcept::member0, aNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(&ANoExcept::member1, aNoExcept, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &ANoExcept::member1, aNoExcept, 20);
    QCOMPARE(f.result(), 20);

    f = run(aNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, aNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(std::ref(aNoExcept));
    QCOMPARE(f.result(), 10);
    f = run(&pool, std::ref(aNoExcept));
    QCOMPARE(f.result(), 10);

    f = run(aNoExcept, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, aNoExcept, 20);
    QCOMPARE(f.result(), 20);

    f = run(std::ref(aNoExcept), 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, std::ref(aNoExcept), 20);
    QCOMPARE(f.result(), 20);


    const AConstNoExcept aConstNoExcept = AConstNoExcept();
    f = run(&AConstNoExcept::member0, &aConstNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &AConstNoExcept::member0, &aConstNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(&AConstNoExcept::member1, &aConstNoExcept, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &AConstNoExcept::member1, &aConstNoExcept, 20);
    QCOMPARE(f.result(), 20);

    f = run(&AConstNoExcept::member0, aConstNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &AConstNoExcept::member0, aConstNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(&AConstNoExcept::member1, aConstNoExcept, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &AConstNoExcept::member1, aConstNoExcept, 20);
    QCOMPARE(f.result(), 20);

    f = run(aConstNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, aConstNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(std::ref(aConstNoExcept));
    QCOMPARE(f.result(), 10);
    f = run(&pool, std::ref(aConstNoExcept));
    QCOMPARE(f.result(), 10);

    f = run(aConstNoExcept, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, aConstNoExcept, 20);
    QCOMPARE(f.result(), 20);

    f = run(std::ref(aConstNoExcept), 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, std::ref(aConstNoExcept), 20);
    QCOMPARE(f.result(), 20);
}

void reportInt0(QPromise<int> &promise)
{
    promise.addResult(0);
}

void reportIntPlusOne(QPromise<int> &promise, int i)
{
    promise.addResult(i + 1);
}

class AWithPromise
{
public:
    void member0(QPromise<int> &promise) { promise.addResult(10); }
    void member1(QPromise<int> &promise, int in) { promise.addResult(in); }

    void operator()(QPromise<int> &promise) { promise.addResult(10); }
};

class AConstWithPromise
{
public:
    void member0(QPromise<int> &promise) const { promise.addResult(10); }
    void member1(QPromise<int> &promise, int in) const { promise.addResult(in); }

    void operator()(QPromise<int> &promise) const { promise.addResult(10); }
};

class ANoExceptWithPromise
{
public:
    void member0(QPromise<int> &promise) noexcept { promise.addResult(10); }
    void member1(QPromise<int> &promise, int in) noexcept { promise.addResult(in); }

    void operator()(QPromise<int> &promise) noexcept { promise.addResult(10); }
};

class AConstNoExceptWithPromise
{
public:
    void member0(QPromise<int> &promise) const noexcept { promise.addResult(10); }
    void member1(QPromise<int> &promise, int in) const noexcept { promise.addResult(in); }

    void operator()(QPromise<int> &promise) const noexcept { promise.addResult(10); }
};

void tst_QtConcurrentRun::reportValueWithPromise()
{
    QThreadPool pool;
    QFuture<int> f;

    f = run(reportInt0);
    QCOMPARE(f.result(), 0);
    f = run(&pool, reportInt0);
    QCOMPARE(f.result(), 0);
    f = run(reportIntPlusOne, 5);
    QCOMPARE(f.result(), 6);
    f = run(&pool, reportIntPlusOne, 5);
    QCOMPARE(f.result(), 6);


    AWithPromise a;
    f = run(&AWithPromise::member0, &a);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &AWithPromise::member0, &a);
    QCOMPARE(f.result(), 10);

    f = run(&AWithPromise::member1, &a, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &AWithPromise::member1, &a, 20);
    QCOMPARE(f.result(), 20);

    f = run(&AWithPromise::member0, a);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &AWithPromise::member0, a);
    QCOMPARE(f.result(), 10);

    f = run(&AWithPromise::member1, a, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &AWithPromise::member1, a, 20);
    QCOMPARE(f.result(), 20);

    f = run(a);
    QCOMPARE(f.result(), 10);
    f = run(&pool, a);
    QCOMPARE(f.result(), 10);

    f = run(std::ref(a));
    QCOMPARE(f.result(), 10);
    f = run(&pool, std::ref(a));
    QCOMPARE(f.result(), 10);


    const AConstWithPromise aConst = AConstWithPromise();
    f = run(&AConstWithPromise::member0, &aConst);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &AConstWithPromise::member0, &aConst);
    QCOMPARE(f.result(), 10);

    f = run(&AConstWithPromise::member1, &aConst, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &AConstWithPromise::member1, &aConst, 20);
    QCOMPARE(f.result(), 20);

    f = run(&AConstWithPromise::member0, aConst);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &AConstWithPromise::member0, aConst);
    QCOMPARE(f.result(), 10);

    f = run(&AConstWithPromise::member1, aConst, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &AConstWithPromise::member1, aConst, 20);
    QCOMPARE(f.result(), 20);

    f = run(aConst);
    QCOMPARE(f.result(), 10);
    f = run(&pool, aConst);
    QCOMPARE(f.result(), 10);

    f = run(std::ref(a));
    QCOMPARE(f.result(), 10);
    f = run(&pool, std::ref(a));
    QCOMPARE(f.result(), 10);


    ANoExceptWithPromise aNoExcept;
    f = run(&ANoExceptWithPromise::member0, &aNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &ANoExceptWithPromise::member0, &aNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(&ANoExceptWithPromise::member1, &aNoExcept, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &ANoExceptWithPromise::member1, &aNoExcept, 20);
    QCOMPARE(f.result(), 20);

    f = run(&ANoExceptWithPromise::member0, aNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &ANoExceptWithPromise::member0, aNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(&ANoExceptWithPromise::member1, aNoExcept, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &ANoExceptWithPromise::member1, aNoExcept, 20);
    QCOMPARE(f.result(), 20);

    f = run(aNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, aNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(std::ref(aNoExcept));
    QCOMPARE(f.result(), 10);
    f = run(&pool, std::ref(aNoExcept));
    QCOMPARE(f.result(), 10);


    const AConstNoExceptWithPromise aConstNoExcept = AConstNoExceptWithPromise();
    f = run(&AConstNoExceptWithPromise::member0, &aConstNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &AConstNoExceptWithPromise::member0, &aConstNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(&AConstNoExceptWithPromise::member1, &aConstNoExcept, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &AConstNoExceptWithPromise::member1, &aConstNoExcept, 20);
    QCOMPARE(f.result(), 20);

    f = run(&AConstNoExceptWithPromise::member0, aConstNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, &AConstNoExceptWithPromise::member0, aConstNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(&AConstNoExceptWithPromise::member1, aConstNoExcept, 20);
    QCOMPARE(f.result(), 20);
    f = run(&pool, &AConstNoExceptWithPromise::member1, aConstNoExcept, 20);
    QCOMPARE(f.result(), 20);

    f = run(aConstNoExcept);
    QCOMPARE(f.result(), 10);
    f = run(&pool, aConstNoExcept);
    QCOMPARE(f.result(), 10);

    f = run(std::ref(aConstNoExcept));
    QCOMPARE(f.result(), 10);
    f = run(&pool, std::ref(aConstNoExcept));
    QCOMPARE(f.result(), 10);
}

struct TestClass
{
    void foo() { }
    void operator()() { }
    void operator()(int) { }
    void fooInt(int){ };
};

struct TestConstClass
{
    void foo() const { }
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
    f = run(std::ref(c));
    f = run(c, 10);
    f = run(std::ref(c), 10);

    f = run(&pool, c);
    f = run(&pool, std::ref(c));
    f = run(&pool, c, 10);
    f = run(&pool, std::ref(c), 10);

    const TestConstClass cc = TestConstClass();
    f = run(cc);
    f = run(std::ref(c));
    f = run(cc, 10);
    f = run(std::ref(c), 10);

    f = run(&pool, cc);
    f = run(&pool, std::ref(c));
    f = run(&pool, cc, 10);
    f = run(&pool, std::ref(c), 10);
}


void tst_QtConcurrentRun::memberFunctions()
{
    QThreadPool pool;

    TestClass c;

    run(&TestClass::foo, c).waitForFinished();
    run(&TestClass::foo, &c).waitForFinished();
    run(&TestClass::fooInt, c, 10).waitForFinished();
    run(&TestClass::fooInt, &c, 10).waitForFinished();

    run(&pool, &TestClass::foo, c).waitForFinished();
    run(&pool, &TestClass::foo, &c).waitForFinished();
    run(&pool, &TestClass::fooInt, c, 10).waitForFinished();
    run(&pool, &TestClass::fooInt, &c, 10).waitForFinished();

    const TestConstClass cc = TestConstClass();
    run(&TestConstClass::foo, cc).waitForFinished();
    run(&TestConstClass::foo, &cc).waitForFinished();
    run(&TestConstClass::fooInt, cc, 10).waitForFinished();
    run(&TestConstClass::fooInt, &cc, 10).waitForFinished();

    run(&pool, &TestConstClass::foo, cc).waitForFinished();
    run(&pool, &TestConstClass::foo, &cc).waitForFinished();
    run(&pool, &TestConstClass::fooInt, cc, 10).waitForFinished();
    run(&pool, &TestConstClass::fooInt, &cc, 10).waitForFinished();
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

    double d = 0.0;
    run(doubleFunction, d).waitForFinished();
    run(&pool, doubleFunction, d).waitForFinished();
    int i = 0;
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
    run(stringRefFunction, std::ref(string)).waitForFinished();
    run(&pool, stringRefFunction, std::ref(string)).waitForFinished();
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
        count.storeRelaxed(0);
        QThreadPool::globalInstance()->setMaxThreadCount(i);
        recursiveRun(levels);
        QCOMPARE(count.loadRelaxed(), (int)std::pow(2.0, levels) - 1);
    }

    for (int i = 0; i < QThread::idealThreadCount(); ++i) {
        count.storeRelaxed(0);
        QThreadPool::globalInstance()->setMaxThreadCount(i);
        recursiveResult(levels);
        QCOMPARE(count.loadRelaxed(), (int)std::pow(2.0, levels) - 1);
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

class SlowTask : public QRunnable
{
public:
    static QAtomicInt cancel;
    void run() override {
        int iter = 60;
        while (--iter && !cancel.loadRelaxed())
            QThread::currentThread()->sleep(std::chrono::milliseconds{25});
    }
};

QAtomicInt SlowTask::cancel;

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

    caught = false;
    try  {
        QtConcurrent::run(&pool, throwFunctionReturn).result();
    } catch (QException &) {
        caught = true;
    }
    QVERIFY2(caught, "did not get exception");

    // Force the task to be run on this thread.
    caught = false;
    QThreadPool shortPool;
    shortPool.setMaxThreadCount(1);
    SlowTask *st = new SlowTask();
    try  {
        shortPool.start(st);
        QtConcurrent::run(&shortPool, throwFunctionReturn).result();
    } catch (QException &) {
        caught = true;
    }

    SlowTask::cancel.storeRelaxed(true);

    QVERIFY2(caught, "did not get exception");
}

void tst_QtConcurrentRun::unhandledException()
{
    struct Exception {};
    bool caught = false;
    try {
        auto f = QtConcurrent::run([] { throw Exception {}; });
        f.waitForFinished();
    } catch (const QUnhandledException &e) {
        try {
            if (e.exception())
                std::rethrow_exception(e.exception());
        } catch (const Exception &) {
            caught = true;
        }
    }

    QVERIFY(caught);
}
#endif

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

struct FunctorWithPromise {
    void operator()(QPromise<int> &, double) { }
};

struct OverloadedFunctorWithPromise {
    void operator()(QPromise<int> &) { }
    void operator()(QPromise<double> &) { }
    void operator()(QPromise<int> &, int) { }
    void operator()(QPromise<double> &, int) { }
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
    FunctorWithPromise fWithPromise;
    {
        QtConcurrent::run(fWithPromise, 1.5).waitForFinished();
    }
    OverloadedFunctorWithPromise ofWithPromise;
    {
        QtConcurrent::run<int>(ofWithPromise).waitForFinished();
        QtConcurrent::run<double>(ofWithPromise).waitForFinished();
        QtConcurrent::run<int>(ofWithPromise, 1).waitForFinished();
        QtConcurrent::run<double>(ofWithPromise, 1).waitForFinished();
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

// Compiler supports lambda
void tst_QtConcurrentRun::lambda()
{
    QCOMPARE(QtConcurrent::run([]() { return 45; }).result(), 45);
    QCOMPARE(QtConcurrent::run([](int a) { return a+15; }, 12).result(), 12+15);
    QCOMPARE(QtConcurrent::run([](int a, double b) { return a + b; }, 12, 15).result(),
             double(12+15));
    QCOMPARE(QtConcurrent::run([](int a , int, int, int, int b)
             { return a + b; }, 1, 2, 3, 4, 5).result(), 1 + 5);

    QCOMPARE(QtConcurrent::run([](QPromise<int> &promise)
             { promise.addResult(45); }).result(), 45);
    QCOMPARE(QtConcurrent::run([](QPromise<int> &promise, double input)
             { promise.addResult(input / 2.0); }, 15.0).result(), 7);

    {
        QString str { "Hello World Foo" };
        QFuture<QStringList> f1 = QtConcurrent::run([&](){ return str.split(' '); });
        auto r = f1.result();
        QCOMPARE(r, QStringList({"Hello", "World", "Foo"}));
    }

    // and now with explicit pool:
    QThreadPool pool;
    QCOMPARE(QtConcurrent::run(&pool, []() { return 45; }).result(), 45);
    QCOMPARE(QtConcurrent::run(&pool, [](int a) { return a + 15; }, 12).result(), 12 + 15);
    QCOMPARE(QtConcurrent::run(&pool, [](int a, double b)
             { return a + b; }, 12, 15).result(), double(12 + 15));
    QCOMPARE(QtConcurrent::run(&pool, [](int a , int, int, int, int b)
             { return a + b; }, 1, 2, 3, 4, 5).result(), 1 + 5);

    {
        QString str { "Hello World Foo" };
        QFuture<QStringList> f1 = QtConcurrent::run(&pool, [&](){ return str.split(' '); });
        auto r = f1.result();
        QCOMPARE(r, QStringList({"Hello", "World", "Foo"}));
    }
}

struct CallableWithState
{
    void setNewState(int newState) { state = newState; }
    int operator()(int newState) { return (state = newState); }

    static constexpr int defaultState() { return 42; }
    int state = defaultState();
};

struct CallableWithStateWithPromise
{
    void setNewState(QPromise<int> &, int newState) { state = newState; }
    void operator()(QPromise<int> &promise, int newState) { state = newState; promise.addResult(newState); }

    static constexpr int defaultState() { return 42; }
    int state = defaultState();
};

void tst_QtConcurrentRun::callableObjectWithState()
{
    CallableWithState o;

    // Run method setNewState explicitly
    run(&CallableWithState::setNewState, &o, CallableWithState::defaultState() + 1).waitForFinished();
    QCOMPARE(o.state, CallableWithState::defaultState() + 1);

    // Run operator()(int) explicitly
    run(std::ref(o), CallableWithState::defaultState() + 2).waitForFinished();
    QCOMPARE(o.state, CallableWithState::defaultState() + 2);

    // Run on a copy of object (original object remains unchanged)
    run(o, CallableWithState::defaultState() + 3).waitForFinished();
    QCOMPARE(o.state, CallableWithState::defaultState() + 2);

    // Explicitly run on a temporary object
    QCOMPARE(run(CallableWithState(), 15).result(), 15);

    CallableWithStateWithPromise oWithPromise;

    // Run method setNewState explicitly
    run(&CallableWithStateWithPromise::setNewState, &oWithPromise,
        CallableWithStateWithPromise::defaultState() + 1).waitForFinished();
    QCOMPARE(oWithPromise.state, CallableWithStateWithPromise::defaultState() + 1);

    // Run operator()(int) explicitly
    run(std::ref(oWithPromise), CallableWithStateWithPromise::defaultState() + 2).waitForFinished();
    QCOMPARE(oWithPromise.state, CallableWithStateWithPromise::defaultState() + 2);

    // Run on a copy of object (original object remains unchanged)
    run(oWithPromise, CallableWithStateWithPromise::defaultState() + 3).waitForFinished();
    QCOMPARE(oWithPromise.state, CallableWithStateWithPromise::defaultState() + 2);

    // Explicitly run on a temporary object
    QCOMPARE(run(CallableWithStateWithPromise(), 15).result(), 15);
}

void report3(QPromise<int> &promise)
{
    promise.addResult(0);
    promise.addResult(2);
    promise.addResult(1);
}

void reportN(QPromise<double> &promise, int n)
{
    for (int i = 0; i < n; ++i)
        promise.addResult(0);
}

void reportString1(QPromise<QString> &promise, const QString &s)
{
    promise.addResult(s);
}

void reportString2(QPromise<QString> &promise, QString s)
{
    promise.addResult(s);
}

class Callable {
public:
    void operator()(QPromise<double> &promise, int n) const
    {
        for (int i = 0; i < n; ++i)
            promise.addResult(0);
    }
};

class MyObject {
public:
    static void staticMember0(QPromise<double> &promise)
    {
        promise.addResult(0);
        promise.addResult(2);
        promise.addResult(1);
    }

    static void staticMember1(QPromise<double> &promise, int n)
    {
        for (int i = 0; i < n; ++i)
            promise.addResult(0);
    }

    void member0(QPromise<double> &promise) const
    {
        promise.addResult(0);
        promise.addResult(2);
        promise.addResult(1);
    }

    void member1(QPromise<double> &promise, int n) const
    {
        for (int i = 0; i < n; ++i)
            promise.addResult(0);
    }

    void memberString1(QPromise<QString> &promise, const QString &s) const
    {
        promise.addResult(s);
    }

    void memberString2(QPromise<QString> &promise, QString s) const
    {
        promise.addResult(s);
    }

    void nonConstMember(QPromise<double> &promise)
    {
        promise.addResult(0);
        promise.addResult(2);
        promise.addResult(1);
    }
};

void tst_QtConcurrentRun::withPromise()
{
    // free function pointer
    QCOMPARE(run(&report3).results(),
             QList<int>({0, 2, 1}));
    QCOMPARE(run(report3).results(),
             QList<int>({0, 2, 1}));

    QCOMPARE(run(reportN, 4).results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(run(reportN, 2).results(),
             QList<double>({0, 0}));

    QString s = QLatin1String("string");
    const QString &crs = QLatin1String("cr string");
    const QString cs = QLatin1String("c string");

    QCOMPARE(run(reportString1, s).results(),
             QList<QString>({s}));
    QCOMPARE(run(reportString1, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(run(reportString1, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(run(reportString1, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    QCOMPARE(run(reportString2, s).results(),
             QList<QString>({s}));
    QCOMPARE(run(reportString2, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(run(reportString2, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(run(reportString2, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    // lambda
    QCOMPARE(run([](QPromise<double> &promise, int n) {
                 for (int i = 0; i < n; ++i)
                     promise.addResult(0);
             }, 3).results(),
             QList<double>({0, 0, 0}));

    // std::function
    const std::function<void(QPromise<double> &, int)> fun = [](QPromise<double> &promise, int n) {
        for (int i = 0; i < n; ++i)
            promise.addResult(0);
    };
    QCOMPARE(run(fun, 2).results(),
             QList<double>({0, 0}));

    // operator()
    QCOMPARE(run(Callable(), 3).results(),
             QList<double>({0, 0, 0}));
    const Callable c{};
    QCOMPARE(run(c, 2).results(),
             QList<double>({0, 0}));

    // static member functions
    QCOMPARE(run(&MyObject::staticMember0).results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(run(&MyObject::staticMember1, 2).results(),
             QList<double>({0, 0}));

    // member functions
    const MyObject obj{};
    QCOMPARE(run(&MyObject::member0, &obj).results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(run(&MyObject::member1, &obj, 4).results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(run(&MyObject::memberString1, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(run(&MyObject::memberString1, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(run(&MyObject::memberString1, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(run(&MyObject::memberString1, &obj, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    QCOMPARE(run(&MyObject::memberString2, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(run(&MyObject::memberString2, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(run(&MyObject::memberString2, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(run(&MyObject::memberString2, &obj, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    MyObject nonConstObj{};
    QCOMPARE(run(&MyObject::nonConstMember, &nonConstObj).results(),
             QList<double>({0, 2, 1}));
}

void tst_QtConcurrentRun::withPromiseInThreadPool()
{
    QScopedPointer<QThreadPool> pool(new QThreadPool);
    // free function pointer
    QCOMPARE(run(pool.data(), &report3).results(),
             QList<int>({0, 2, 1}));
    QCOMPARE(run(pool.data(), report3).results(),
             QList<int>({0, 2, 1}));

    QCOMPARE(run(pool.data(), reportN, 4).results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(run(pool.data(), reportN, 2).results(),
             QList<double>({0, 0}));

    QString s = QLatin1String("string");
    const QString &crs = QLatin1String("cr string");
    const QString cs = QLatin1String("c string");

    QCOMPARE(run(pool.data(), reportString1, s).results(),
             QList<QString>({s}));
    QCOMPARE(run(pool.data(), reportString1, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(run(pool.data(), reportString1, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(run(pool.data(), reportString1, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    QCOMPARE(run(pool.data(), reportString2, s).results(),
             QList<QString>({s}));
    QCOMPARE(run(pool.data(), reportString2, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(run(pool.data(), reportString2, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(run(pool.data(), reportString2, QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));

    // lambda
    QCOMPARE(run(pool.data(), [](QPromise<double> &promise, int n) {
                 for (int i = 0; i < n; ++i)
                     promise.addResult(0);
             }, 3).results(),
             QList<double>({0, 0, 0}));

    // std::function
    const std::function<void(QPromise<double> &, int)> fun = [](QPromise<double> &promise, int n) {
        for (int i = 0; i < n; ++i)
            promise.addResult(0);
    };
    QCOMPARE(run(pool.data(), fun, 2).results(),
             QList<double>({0, 0}));

    // operator()
    QCOMPARE(run(pool.data(), Callable(), 3).results(),
             QList<double>({0, 0, 0}));
    const Callable c{};
    QCOMPARE(run(pool.data(), c, 2).results(),
             QList<double>({0, 0}));

    // static member functions
    QCOMPARE(run(pool.data(), &MyObject::staticMember0).results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(run(pool.data(), &MyObject::staticMember1, 2).results(),
             QList<double>({0, 0}));

    // member functions
    const MyObject obj{};
    QCOMPARE(run(pool.data(), &MyObject::member0, &obj).results(),
             QList<double>({0, 2, 1}));
    QCOMPARE(run(pool.data(), &MyObject::member1, &obj, 4).results(),
             QList<double>({0, 0, 0, 0}));
    QCOMPARE(run(pool.data(), &MyObject::memberString1, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(run(pool.data(), &MyObject::memberString1, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(run(pool.data(), &MyObject::memberString1, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(run(pool.data(), &MyObject::memberString1, &obj,
                 QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
    QCOMPARE(run(pool.data(), &MyObject::memberString2, &obj, s).results(),
             QList<QString>({s}));
    QCOMPARE(run(pool.data(), &MyObject::memberString2, &obj, crs).results(),
             QList<QString>({crs}));
    QCOMPARE(run(pool.data(), &MyObject::memberString2, &obj, cs).results(),
             QList<QString>({cs}));
    QCOMPARE(run(pool.data(), &MyObject::memberString2, &obj,
                 QString(QLatin1String("rvalue"))).results(),
             QList<QString>({QString(QLatin1String("rvalue"))}));
}

void tst_QtConcurrentRun::withPromiseAndThen()
{
    bool runExecuted = false;
    bool cancelReceivedBeforeSync = false;
    bool cancelReceivedAfterSync = false;

    bool syncBegin = false;
    bool syncEnd = false;

    QMutex mutex;
    QWaitCondition condition;

    auto reset = [&]() {
        runExecuted = false;
        cancelReceivedBeforeSync = false;
        cancelReceivedAfterSync = false;
        syncBegin = false;
        syncEnd = false;
    };

    auto setFlag = [&mutex, &condition] (bool &flag) {
        QMutexLocker locker(&mutex);
        flag = true;
        condition.wakeOne();
    };

    auto waitForFlag = [&mutex, &condition] (const bool &flag) {
        QMutexLocker locker(&mutex);
        while (!flag)
            condition.wait(&mutex);
    };

    auto report1WithCancel = [&](QPromise<int> &promise) {
        runExecuted = true;
        cancelReceivedBeforeSync = promise.isCanceled();

        setFlag(syncBegin);
        waitForFlag(syncEnd);

        cancelReceivedAfterSync = promise.isCanceled();
        if (cancelReceivedAfterSync)
            return;
        promise.addResult(1);
    };

    {
        auto future = run(report1WithCancel);

        waitForFlag(syncBegin);
        future.cancel();
        setFlag(syncEnd);

        future.waitForFinished();
        QCOMPARE(future.results().size(), 0);
        QVERIFY(runExecuted);
        QVERIFY(!cancelReceivedBeforeSync);
        QVERIFY(cancelReceivedAfterSync);
    }

    reset();

    {
        bool thenExecuted = false;
        bool cancelExecuted = false;
        auto future = run(report1WithCancel);
        auto resultFuture = future.then(QtFuture::Launch::Async, [&](int) { thenExecuted = true; })
                            .onCanceled([&]() { cancelExecuted = true; });

        waitForFlag(syncBegin);
        // no cancel this time
        setFlag(syncEnd);

        resultFuture.waitForFinished();
        QCOMPARE(future.results().size(), 1);
        QCOMPARE(future.result(), 1);
        QVERIFY(runExecuted);
        QVERIFY(thenExecuted);
        QVERIFY(!cancelExecuted);
        QVERIFY(!cancelReceivedBeforeSync);
        QVERIFY(!cancelReceivedAfterSync);
    }

    reset();

    {
        bool thenExecuted = false;
        bool cancelExecuted = false;
        auto future = run(report1WithCancel);
        auto resultFuture = future.then(QtFuture::Launch::Async, [&](int) { thenExecuted = true; })
                            .onCanceled([&]() { cancelExecuted = true; });

        waitForFlag(syncBegin);
        future.cancel();
        setFlag(syncEnd);

        resultFuture.waitForFinished();
        QCOMPARE(future.results().size(), 0);
        QVERIFY(runExecuted);
        QVERIFY(!thenExecuted);
        QVERIFY(cancelExecuted);
        QVERIFY(!cancelReceivedBeforeSync);
        QVERIFY(cancelReceivedAfterSync);
    }
}

class MoveOnlyType
{
public:
    MoveOnlyType() = default;
    MoveOnlyType(const MoveOnlyType &) = delete;
    MoveOnlyType(MoveOnlyType &&) = default;
    MoveOnlyType &operator=(const MoveOnlyType &) = delete;
    MoveOnlyType &operator=(MoveOnlyType &&) = default;
};

class MoveOnlyCallable : public MoveOnlyType
{
public:
    void operator()(QPromise<int> &promise, const MoveOnlyType &)
    {
        promise.addResult(1);
    }
};

void tst_QtConcurrentRun::moveOnlyType()
{
    QCOMPARE(run(MoveOnlyCallable(), MoveOnlyType()).results(),
             QList<int>({1}));
}

void tst_QtConcurrentRun::crefFunction()
{
    {
        // free function pointer with promise
        auto fun = &returnInt0;
        QCOMPARE(run(std::cref(fun)).result(), 10);

        // lambda with promise
        auto lambda = [](int n) {
            return 2 * n;
        };
        QCOMPARE(run(std::cref(lambda), 3).result(), 6);

        // std::function with promise
        const std::function<int(int)> funObj = [](int n) {
            return 2 * n;
        };
        QCOMPARE(run(std::cref(funObj), 2).result(), 4);

        // callable with promise
        const AConst c{};
        QCOMPARE(run(std::cref(c), 2).result(), 2);

        // member functions with promise
        auto member = &AConst::member0;
        const AConst obj{};
        QCOMPARE(run(std::cref(member), &obj).result(), 10);
    }

    {
        // free function pointer with promise
        auto fun = &report3;
        QCOMPARE(run(std::cref(fun)).results(),
                 QList<int>({0, 2, 1}));

        // lambda with promise
        auto lambda = [](QPromise<double> &promise, int n) {
            for (int i = 0; i < n; ++i)
                promise.addResult(0);
        };
        QCOMPARE(run(std::cref(lambda), 3).results(),
                 QList<double>({0, 0, 0}));

        // std::function with promise
        const std::function<void(QPromise<double> &, int)> funObj =
                [](QPromise<double> &promise, int n) {
            for (int i = 0; i < n; ++i)
                promise.addResult(0);
        };
        QCOMPARE(run(std::cref(funObj), 2).results(),
                 QList<double>({0, 0}));

        // callable with promise
        const Callable c{};
        QCOMPARE(run(std::cref(c), 2).results(),
                 QList<double>({0, 0}));

        // member functions with promise
        auto member = &MyObject::member0;
        const MyObject obj{};
        QCOMPARE(run(std::cref(member), &obj).results(),
                 QList<double>({0, 2, 1}));
    }
}

int explicitPromise(QPromise<int> &promise, int &i)
{
    promise.setProgressRange(-10, 10);
    ++i;
    return i * 2;
}

void tst_QtConcurrentRun::customPromise()
{
    QPromise<int> p;
    int i = 4;
    QCOMPARE(QtConcurrent::run(explicitPromise, std::ref(p), std::ref(i)).result(), 10);
    QCOMPARE(i, 5);
    QCOMPARE(p.future().progressMinimum(), -10);
    QCOMPARE(p.future().progressMaximum(), 10);
}

void tst_QtConcurrentRun::nonDefaultConstructibleValue()
{
    struct NonDefaultConstructible
    {
        explicit NonDefaultConstructible(int v) : value(v) { }
        int value = 0;
    };

    auto future = QtConcurrent::run([] { return NonDefaultConstructible(42); });
    QCOMPARE(future.result().value, 42);
}

// QTBUG-98901
void tst_QtConcurrentRun::nullThreadPool()
{
    QThreadPool *pool = nullptr;
    std::atomic<bool> isInvoked = false;
    auto future = run(pool, [&] { isInvoked = true; });
    future.waitForFinished();
    QVERIFY(future.isCanceled());
    QVERIFY(!isInvoked);
}

struct LifetimeChecker
{
    LifetimeChecker() { ++count; }
    LifetimeChecker(const LifetimeChecker &) { ++count; }
    ~LifetimeChecker() { --count; }

    void operator()() { }

    static std::atomic<int> count;
};
std::atomic<int> LifetimeChecker::count = 0;

void tst_QtConcurrentRun::nullThreadPoolNoLeak()
{
    {
        QThreadPool *pool = nullptr;
        auto future = run(pool, LifetimeChecker());
        future.waitForFinished();
    }
    QCOMPARE(LifetimeChecker::count, 0);
}

QTEST_MAIN(tst_QtConcurrentRun)
#include "tst_qtconcurrentrun.moc"
