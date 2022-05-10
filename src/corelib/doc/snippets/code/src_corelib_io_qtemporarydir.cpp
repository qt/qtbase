// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

{
//! [0]
    // Within a function/method...

    QTemporaryDir dir;
    if (dir.isValid()) {
        // dir.path() returns the unique directory path
    }

    // The QTemporaryDir destructor removes the temporary directory
    // as it goes out of scope.
//! [0]
}
