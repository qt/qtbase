// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    MyObject obj;
    obj.startup();
    QTest::qWaitFor([&]() {
        return obj.isReady();
    }, 3000);
//! [0]
