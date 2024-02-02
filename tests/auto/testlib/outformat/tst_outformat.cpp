// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

class tst_OutFormat : public QObject
{
    Q_OBJECT
private slots:
    void toHex_data() const;
    void toHex() const;
    // other formats of interest ?
};

void tst_OutFormat::toHex_data() const
{
    QTest::addColumn<QByteArray>("raw");
    QTest::addColumn<QByteArray>("hex");

    QTest::newRow("empty") << QByteArray("") << QByteArray("");
    QTest::newRow("long")
        << QByteArray("Truncates in ellipsis when more than fifty characters long")
        << QByteArray("54 72 75 6E 63 61 74 65 73 20 69 6E 20 65 6C 6C "
                      "69 70 73 69 73 20 77 68 65 6E 20 6D 6F 72 65 20 "
                      "74 68 61 6E 20 66 69 66 74 79 20 63 68 61 72 61 "
                      "63 74 ...");
    QTest::newRow("spaces")
        << QByteArray(" \t\n\v\f\r") << QByteArray("20 09 0A 0B 0C 0D");
    QTest::newRow("ASCII-escapes")
        << QByteArray("\a\b\\\"'\177") << QByteArray("07 08 5C 22 27 7F");
    // These are the ISO Latin-15 &nbsp;, pound, Euro, ..., y-umlaut
    QTest::newRow("8-bit-sampler")
        << QByteArray("\240\243\244\261\327\360\377") << QByteArray("A0 A3 A4 B1 D7 F0 FF");
}

void tst_OutFormat::toHex() const
{
    QFETCH(QByteArray, raw);
    QFETCH(QByteArray, hex);
    QScopedArrayPointer<char> repr(QTest::toHexRepresentation(raw.constData(), raw.size()));
    QCOMPARE(repr.data(), hex);
}

QTEST_APPLESS_MAIN(tst_OutFormat)

#include "tst_outformat.moc"
