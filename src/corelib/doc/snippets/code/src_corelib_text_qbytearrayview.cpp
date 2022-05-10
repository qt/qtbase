// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtCore/qbytearrayview.h>

//! [0]
    void myfun1(QByteArrayView bv);        // preferred
    void myfun2(const QByteArrayView &bv); // compiles and works, but slower
//! [0]

//! [1]
    void fun(QByteArrayView bv);
    void fun(char ch) { fun(QByteArrayView(&ch, 1)); }
//! [1]

//! [2]
QByteArrayView str("FF");
bool ok;
int hex = str.toInt(&ok, 16);     // hex == 255, ok == true
int dec = str.toInt(&ok, 10);     // dec == 0, ok == false
//! [2]

//! [3]
QByteArrayView str("FF");
bool ok;
long hex = str.toLong(&ok, 16);   // hex == 255, ok == true
long dec = str.toLong(&ok, 10);   // dec == 0, ok == false
//! [3]

//! [4]
QByteArrayView string("1234.56 Volt");
bool ok;
float a = str.toFloat(&ok);       // a == 0, ok == false
a = string.first(7).toFloat(&ok); // a == 1234.56, ok == true
//! [4]
