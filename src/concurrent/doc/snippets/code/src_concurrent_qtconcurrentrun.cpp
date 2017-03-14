/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
QFuture<QList<QByteArray> > future = QtConcurrent::run(bytearray, &QByteArray::split, ',');
...
QList<QByteArray> result = future.result();
//! [4]

//! [5]
// call 'void QImage::invertPixels(InvertMode mode)' in a separate thread
QImage image = ...;
QFuture<void> future = QtConcurrent::run(&image, &QImage::invertPixels, QImage::InvertRgba);
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
