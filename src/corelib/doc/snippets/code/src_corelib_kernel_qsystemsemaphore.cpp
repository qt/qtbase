// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QSystemSemaphore sem(QSystemSemaphore::platformSafeKey("market"), 3, QSystemSemaphore::Create);
                             // resources available == 3
sem.acquire();               // resources available == 2
sem.acquire();               // resources available == 1
sem.acquire();               // resources available == 0
sem.release();               // resources available == 1
sem.release(2);              // resources available == 3
//! [0]


//! [1]
QSystemSemaphore sem(QSystemSemaphore::platformSafeKey("market"), 5, QSystemSemaphore::Create);
for (int i = 0; i < 5; ++i)  // acquire all 5 resources
    sem.acquire();
sem.release(5);              // release the 5 resources
//! [1]


//! [2]
sem.release(10);          // "create" 10 new resources
//! [2]
