// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

{
//! [0]
    // Within a function/method...

    QTemporaryFile file;
    if (file.open()) {
        // file.fileName() returns the unique file name
    }

    // The QTemporaryFile destructor removes the temporary file
    // as it goes out of scope.
//! [0]
}

//! [1]
  QFile f(":/resources/file.txt");
  QTemporaryFile::createNativeFile(f); // Returns a pointer to a temporary file

  QFile f("/users/qt/file.txt");
  QTemporaryFile::createNativeFile(f); // Returns 0
//! [1]
