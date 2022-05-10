// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [2]
if (qstrcmp(v.typeName(), "PGresult*") == 0) {
    PGresult *handle = *static_cast<PGresult **>(v.data());
    if (handle) {
        // ...
        }
}

if (qstrcmp(v.typeName(), "MYSQL_STMT*") == 0) {
    MYSQL_STMT *handle = *static_cast<MYSQL_STMT **>(v.data());
    if (handle) {
        // ...
        }
    }
//! [2]
