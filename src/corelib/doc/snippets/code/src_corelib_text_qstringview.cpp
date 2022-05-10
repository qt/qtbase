// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    void myfun1(QStringView sv);        // preferred
    void myfun2(const QStringView &sv); // compiles and works, but slower
//! [0]

//! [1]
    void fun(QChar ch) { fun(QStringView(&ch, 1)); }
//! [1]
