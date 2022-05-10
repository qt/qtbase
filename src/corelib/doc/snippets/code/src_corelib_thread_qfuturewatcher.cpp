// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
// Instantiate the objects and connect to the finished signal.
MyClass myObject;
QFutureWatcher<int> watcher;
connect(&watcher, &QFutureWatcher<int>::finished, &myObject, &MyClass::handleFinished);

// Start the computation.
QFuture<int> future = QtConcurrent::run(...);
watcher.setFuture(future);
//! [0]
