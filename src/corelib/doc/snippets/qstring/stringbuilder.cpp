// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

using namespace Qt::StringLiterals;

//! [0]
    QString foo;
    QString type = "long";

    foo = "vector<"_L1 + type + ">::iterator"_L1;

    if (foo.startsWith("(" + type + ") 0x"))
        ...
//! [0]

//! [5]
    #include <QStringBuilder>

    QString hello("hello");
    QStringView el = QStringView{ hello }.mid(2, 3);
    QLatin1StringView world("world");
    QString message =  hello % el % world % QChar('!');
//! [5]
