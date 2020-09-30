/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
