// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

using namespace Qt::StringLiterals;

//! [1]
QString url = "https://www.unicode.org/"_L1;
//! [1]


//! [2]
double d = 12.34;
QString str = QString("delta: %1").arg(d, 0, 'E', 3);
// str == "delta: 1.234E+01"
//! [2]


//! [3]
if (str == "auto" || str == "extern"
        || str == "static" || str == "register") {
    ...
}
//! [3]


//! [4]
if (str == QString("auto") || str == QString("extern")
        || str == QString("static") || str == QString("register")) {
    ...
}
//! [4]

//! [4bis]
str.append("Hello ").append("World");
//! [4bis]

//! [5]
if (str == "auto"_L1
        || str == "extern"_L1
        || str == "static"_L1
        || str == "register"_L1 {
    ...
}
//! [5]


//! [6]
QLabel *label = new QLabel("MOD"_L1, this);
//! [6]


//! [7]
QString plain = "#include <QtCore>"
QString html = plain.toHtmlEscaped();
// html == "#include &lt;QtCore&gt;"
//! [7]

//! [8]
QString str("ab");
str.repeated(4);            // returns "abababab"
//! [8]

//! [9]
// hasAttribute takes a QString argument
if (node.hasAttribute("http-contents-length")) //...
//! [9]

//! [10]
if (node.hasAttribute(QStringLiteral(u"http-contents-length"))) //...
//! [10]

//! [11]
if (attribute.name() == "http-contents-length"_L1) //...
//! [11]

//! [qUtf8Printable]
qWarning("%s: %s", qUtf8Printable(key), qUtf8Printable(value));
//! [qUtf8Printable]

//! [qUtf16Printable]
qWarning("%ls: %ls", qUtf16Printable(key), qUtf16Printable(value));
//! [qUtf16Printable]
