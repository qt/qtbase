// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    QString s;

    s = "a";
    qDebug().noquote() << s;    // prints: a
    qDebug() << s;              // prints: "a"

    s = "\"a\r\n\"";
    qDebug() << s;              // prints: "\"a\r\n\""

    s = "\033";                 // escape character
    qDebug() << s;              // prints: "\u001B"

    s = "\u00AD";               // SOFT HYPHEN
    qDebug() << s;              // prints: "\u00AD"

    s = "\u00E1";               // LATIN SMALL LETTER A WITH ACUTE
    qDebug() << s;              // prints: "á"

    s = "a\u0301";              // "a" followed by COMBINING ACUTE ACCENT
    qDebug() << s;              // prints: "á";

    s = "\u0430\u0301";         // CYRILLIC SMALL LETTER A followed by COMBINING ACUTE ACCENT
    qDebug() << s;              // prints: "а́"
//! [0]

//! [1]
    QByteArray ba;

    ba = "a";
    qDebug().noquote() << ba;    // prints: a
    qDebug() << ba;              // prints: "a"

    ba = "\"a\r\n\"";
    qDebug() << ba;              // prints: "\"a\r\n\""

    ba = "\033";                 // escape character
    qDebug() << ba;              // prints: "\x1B"

    ba = "\xC3\xA1";
    qDebug() << ba;              // prints: "\xC3\xA1"

    ba = QByteArray("a\0b", 3);
    qDebug() << ba               // prints: "\a\x00""b"
//! [1]

//! [toString]
    QTRY_VERIFY2(list.isEmpty(), qPrintable(QString::fromLatin1(
        "Expected list to be empty, but it has the following items: %1")).arg(QDebug::toString(list)));
//! [toString]
