// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
extern void aFunction();
QFuture<void> future = QtConcurrent::run(aFunction);
//! [0]


//! [explicit-pool-0]
extern void aFunction();
QThreadPool pool;
QFuture<void> future = QtConcurrent::run(&pool, aFunction);
//! [explicit-pool-0]


//! [1]
extern void aFunctionWithArguments(int arg1, double arg2, const QString &string);

int integer = ...;
double floatingPoint = ...;
QString string = ...;

QFuture<void> future = QtConcurrent::run(aFunctionWithArguments, integer, floatingPoint, string);
//! [1]


//! [2]
extern QString functionReturningAString();
QFuture<QString> future = QtConcurrent::run(functionReturningAString);
...
QString result = future.result();
//! [2]


//! [3]
extern QString someFunction(const QByteArray &input);

QByteArray bytearray = ...;

QFuture<QString> future = QtConcurrent::run(someFunction, bytearray);
...
QString result = future.result();
//! [3]

//! [4]
// call 'QList<QByteArray>  QByteArray::split(char sep) const' in a separate thread
QByteArray bytearray = "hello world";
QFuture<QList<QByteArray> > future = QtConcurrent::run(&QByteArray::split, bytearray, ' ');
...
QList<QByteArray> result = future.result();
//! [4]

//! [5]
// call 'void QImage::invertPixels(InvertMode mode)' in a separate thread
QImage image = ...;
QFuture<void> future = QtConcurrent::run(&QImage::invertPixels, &image, QImage::InvertRgba);
...
future.waitForFinished();
// At this point, the pixels in 'image' have been inverted
//! [5]

//! [6]
QFuture<void> future = QtConcurrent::run([=]() {
    // Code in this block will run in another thread
});
...
//! [6]

//! [7]
static void addOne(int &n) { ++n; }
...
int n = 42;
QtConcurrent::run(&addOne, std::ref(n)).waitForFinished(); // n == 43
//! [7]

//! [8]
struct TestClass
{
    void operator()(int s1) { s = s1; }
    int s = 42;
};

...

TestClass o;

// Modify original object
QtConcurrent::run(std::ref(o), 15).waitForFinished(); // o.s == 15

// Modify a copy of the original object
QtConcurrent::run(o, 42).waitForFinished(); // o.s == 15

// Use a temporary object
QtConcurrent::run(TestClass(), 42).waitForFinished();

// Ill-formed
QtConcurrent::run(&o, 42).waitForFinished(); // compilation error
//! [8]

//! [9]
extern void aFunction(QPromise<void> &promise);
QFuture<void> future = QtConcurrent::run(aFunction);
//! [9]

//! [10]
extern void aFunction(QPromise<void> &promise, int arg1, const QString &arg2);

int integer = ...;
QString string = ...;

QFuture<void> future = QtConcurrent::run(aFunction, integer, string);
//! [10]

//! [11]
void helloWorldFunction(QPromise<QString> &promise)
{
    promise.addResult("Hello");
    promise.addResult("world");
}

QFuture<QString> future = QtConcurrent::run(helloWorldFunction);
...
QList<QString> results = future.results();
//! [11]

//! [12]
void aFunction(QPromise<int> &promise)
{
    for (int i = 0; i < 100; ++i) {
        promise.suspendIfRequested();
        if (promise.isCanceled())
            return;

        // computes the next result, may be time consuming like 1 second
        const int res = ... ;
        promise.addResult(res);
    }
}

QFuture<int> future = QtConcurrent::run(aFunction);

... // user pressed a pause button after 10 seconds
future.suspend();

... // user pressed a resume button after 10 seconds
future.resume();

... // user pressed a cancel button after 10 seconds
future.cancel();
//! [12]

//! [13]
void aFunction(QPromise<int> &promise)
{
    promise.setProgressRange(0, 100);
    int result = 0;
    for (int i = 0; i < 100; ++i) {
        // computes some part of the task
        const int part = ... ;
        result += part;
        promise.setProgressValue(i);
    }
    promise.addResult(result);
}

QFutureWatcher<int> watcher;
QObject::connect(&watcher, &QFutureWatcher::progressValueChanged, [](int progress){
    ... ; // update GUI with a progress
    qDebug() << "current progress:" << progress;
});
watcher.setFuture(QtConcurrent::run(aFunction));
//! [13]

//! [14]
struct Functor {
    void operator()(QPromise<int> &) { }
    void operator()(QPromise<double> &) { }
};

Functor f;
run<double>(f); // this will select the 2nd overload
// run(f);      // error, both candidate overloads potentially match
//! [14]

//! [15]
void foo(int arg);
void foo(int arg1, int arg2);
...
QFuture<void> future = QtConcurrent::run(foo, 42);
//! [15]

//! [16]
QFuture<void> future = QtConcurrent::run([] { foo(42); });
//! [16]

//! [17]
QFuture<void> future = QtConcurrent::run(static_cast<void(*)(int)>(foo), 42);
//! [17]

//! [18]
QFuture<void> future = QtConcurrent::run(qOverload<int>(foo), 42);
//! [18]
