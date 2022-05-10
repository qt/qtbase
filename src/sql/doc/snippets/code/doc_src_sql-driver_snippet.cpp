// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [36]
QSqlQuery query;
QVariant v;
query.setForwardOnly(true);
query.exec("SELECT * FROM table");
while (query.next()) {
    // Handle changes in every iteration of the loop
    v = query.result()->handle();

    if (qstrcmp(v.typeName(), "PGresult*") == 0) {
        PGresult *handle = *static_cast<PGresult **>(v.data());
        if (handle) {
            // Do something...
        }
    }
}
//! [36]
