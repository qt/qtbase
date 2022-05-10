// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
void someFunction()
{
    QFutureSynchronizer<void> synchronizer;

    ...

    synchronizer.addFuture(QtConcurrent::run(anotherFunction));
    synchronizer.addFuture(QtConcurrent::map(list, mapFunction));

    return; // QFutureSynchronizer waits for all futures to finish
}
//! [0]
