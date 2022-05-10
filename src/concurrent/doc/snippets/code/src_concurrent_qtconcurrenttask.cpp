// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QtConcurrent::task([]{ qDebug("Hello, world!"); }).spawn();
//! [0]

//! [1]
auto task = [](const QString &s){ qDebug() << ("Hello, " + s); };
QtConcurrent::task(std::move(task))
    .withArguments("world!")
    .spawn();
//! [1]

//! [2]
QString s("Hello, ");
QtConcurrent::task([](QString &s){ s.append("world!"); })
    .withArguments(std::ref(s))
    .spawn();
//! [2]

//! [3]
auto future = QtConcurrent::task([]{ return 42; }).spawn();
auto result = future.result(); // result == 42
//! [3]

//! [4]
std::is_invocable_v<std::decay_t<Task>, std::decay_t<Args>...>
//! [4]

//! [5]
QVariant value(42);
auto result = QtConcurrent::task(&qvariant_cast<int>)
                  .withArguments(value)
                  .spawn()
                  .result(); // result == 42
//! [5]

//! [6]
QString result("Hello, world!");

QtConcurrent::task(&QString::chop)
    .withArguments(&result, 8)
    .spawn()
    .waitForFinished(); // result == "Hello"
//! [6]

//! [7]
auto result = QtConcurrent::task(std::plus<int>())
                  .withArguments(40, 2)
                  .spawn()
                  .result() // result == 42
//! [7]

//! [8]
struct CallableWithState
{
    void operator()(int newState) { state = newState; }

    // ...
};

// ...

CallableWithState object;

QtConcurrent::task(std::ref(object))
   .withArguments(42)
   .spawn()
   .waitForFinished(); // The object's state is set to 42
//! [8]

//! [9]
QThreadPool pool;
QtConcurrent::task([]{ return 42; }).onThreadPool(pool).spawn();
//! [9]

//! [10]
QtConcurrent::task([]{ return 42; }).withPriority(10).spawn();
//! [10]

//! [11]
QtConcurrent::task([]{ qDebug("Hello, world!"); }).spawn(FutureResult::Ignore);
//! [11]

//! [12]
void increment(QPromise<int> &promise, int i)
{
    promise.addResult(i + 1);
}

int result = QtConcurrent::task(&increment).withArguments(10).spawn().result(); // result == 11
//! [12]
