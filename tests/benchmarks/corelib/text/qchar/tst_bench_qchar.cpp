// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QTest>
#include <QChar>

class tst_QChar: public QObject
{
    Q_OBJECT
private slots:
    void isUpper_data();
    void isUpper();
    void isLower_data();
    void isLower();
    void isLetter_data();
    void isLetter();
    void isDigit_data();
    void isDigit();
    void isLetterOrNumber_data();
    void isLetterOrNumber();
    void isSpace_data();
    void isSpace();
};

void tst_QChar::isUpper_data()
{
    QTest::addColumn<QChar>("c");

    QTest::newRow("k") << QChar('k');
    QTest::newRow("K") << QChar('K');
    QTest::newRow("5") << QChar('5');
    QTest::newRow("\\0") << QChar();
    QTest::newRow("space") << QChar(' ');
    QTest::newRow("\\u3C20") << QChar(0x3C20);
}

void tst_QChar::isUpper()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isUpper();
    }
}

void tst_QChar::isLower_data()
{
    isUpper_data();
}

void tst_QChar::isLower()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isLower();
    }
}

void tst_QChar::isLetter_data()
{
    isUpper_data();
}

void tst_QChar::isLetter()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isLetter();
    }
}

void tst_QChar::isDigit_data()
{
    isUpper_data();
}

void tst_QChar::isDigit()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isDigit();
    }
}

void tst_QChar::isLetterOrNumber_data()
{
    isUpper_data();
}

void tst_QChar::isLetterOrNumber()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isLetterOrNumber();
    }
}

void tst_QChar::isSpace_data()
{
    isUpper_data();
}

void tst_QChar::isSpace()
{
    QFETCH(QChar, c);
    QBENCHMARK {
        c.isSpace();
    }
}

QTEST_MAIN(tst_QChar)

#include "tst_bench_qchar.moc"
