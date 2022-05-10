// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

using namespace Qt::StringLiterals;

//! [0]
QMimeDatabase db;
QMimeType mime = db.mimeTypeForFile(fileName);
if (mime.inherits("text/plain")) {
    // The file is plain text, we can display it in a QTextEdit
}
//! [0]

//! [1]
QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "mime/packages"_L1,
                          QStandardPaths::LocateDirectory);
//! [1]

//! [2]
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="application/vnd.qt.qmakeprofile">
    <comment xml:lang="en">Qt qmake Profile</comment>
    <glob pattern="*.pro" weight="50"/>
  </mime-type>
</mime-info>
//! [2]
