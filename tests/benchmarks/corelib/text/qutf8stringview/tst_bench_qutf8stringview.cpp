// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#include <qbytearray.h>
#include <qdebug.h>
#include <qstring.h>
#include <qtest.h>
#include <qutf8stringview.h>

class tst_QUtf8StringView : public QObject
{
    Q_OBJECT

private slots:
    void equalStringsLatin1_data() { equalStrings_data(); }
    void equalStringsLatin1();
    void equalStringsUtf16_data() { equalStrings_data(); }
    void equalStringsUtf16();
    void equalStringsUtf8_data() { equalStrings_data(); }
    void equalStringsUtf8();

    void compareStringsCaseSensitiveLatin1_data() { compareStringsCaseSensitive_data(); }
    void compareStringsCaseSensitiveLatin1() { compareStringsLatin1(true); }
    void compareStringsCaseSensitiveUtf16_data() { compareStringsCaseSensitive_data(); }
    void compareStringsCaseSensitiveUtf16() { compareStringsUtf16(true); }
    void compareStringsCaseSensitiveUtf8_data() { compareStringsCaseSensitive_data(); }
    void compareStringsCaseSensitiveUtf8() { compareStringsUtf8(true); }

    void compareStringsCaseInsensitiveLatin1_data() { compareStringsCaseInsensitive_data(); }
    void compareStringsCaseInsensitiveLatin1() { compareStringsLatin1(false); }
    void compareStringsCaseInsensitiveUtf16_data() { compareStringsCaseInsensitive_data(); }
    void compareStringsCaseInsensitiveUtf16() { compareStringsUtf16(false); }
    void compareStringsCaseInsensitiveUtf8_data() { compareStringsCaseInsensitive_data(); }
    void compareStringsCaseInsensitiveUtf8() { compareStringsUtf8(false); }

    void compareStringsWithErrors_data();
    void compareStringsWithErrors();

private:
    void equalStrings_data();
    void compareStringsCaseSensitive_data();
    void compareStringsCaseInsensitive_data();
    void compareStringsLatin1(bool caseSensitive);
    void compareStringsUtf16(bool caseSensitive);
    void compareStringsUtf8(bool caseSensitive);
};

void tst_QUtf8StringView::equalStrings_data()
{
    QTest::addColumn<QString>("lhs");
    QTest::addColumn<QString>("rhs");
    QTest::addColumn<bool>("isEqual");

    QTest::newRow("EqualStrings") << "Test"
                                  << "Test" << true;
    QTest::newRow("EqualStringsLong")
            << "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
            << "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" << true;
    QTest::newRow("DifferentCase") << "test"
                                   << "Test" << false;
    QTest::newRow("DifferentCaseLong")
            << "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
            << "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz" << false;
    QTest::newRow("ReverseStrings") << "Test"
                                    << "tseT" << false;
    QTest::newRow("Latin1RangeCharacter") << u8"B\u00d8" << u8"B\u00d8" << true;
    QTest::newRow("Latin1RangeCharacterDifferentCase") << u8"B\u00d8" << u8"B\u00f8" << false;
}

void tst_QUtf8StringView::equalStringsLatin1()
{
    QFETCH(QString, lhs);
    QFETCH(QString, rhs);
    QFETCH(bool, isEqual);
    QByteArray left = lhs.toUtf8();
    QByteArray right = rhs.toLatin1();
    QBasicUtf8StringView<false> lhv(left);
    QLatin1StringView rhv(right);
    bool result;

    QBENCHMARK {
        result = QtPrivate::equalStrings(lhv, rhv);
    };
    QCOMPARE(result, isEqual);
}

void tst_QUtf8StringView::equalStringsUtf16()
{
    QFETCH(QString, lhs);
    QFETCH(QString, rhs);
    QFETCH(bool, isEqual);

    QByteArray left = lhs.toUtf8();
    QBasicUtf8StringView<false> lhv(left);
    QStringView rhv(rhs);
    bool result;

    QBENCHMARK {
        result = QtPrivate::equalStrings(lhv, rhv);
    };
    QCOMPARE(result, isEqual);
}

void tst_QUtf8StringView::equalStringsUtf8()
{
    QFETCH(QString, lhs);
    QFETCH(QString, rhs);
    QFETCH(bool, isEqual);

    QByteArray left = lhs.toUtf8();
    QByteArray right = rhs.toUtf8();
    QBasicUtf8StringView<false> lhv(left);
    QBasicUtf8StringView<false> rhv(right);
    bool result;

    QBENCHMARK {
        result = QtPrivate::equalStrings(lhv, rhv);
    };
    QCOMPARE(result, isEqual);
}

void tst_QUtf8StringView::compareStringsCaseSensitive_data()
{
    QTest::addColumn<QString>("lhs");
    QTest::addColumn<QString>("rhs");
    QTest::addColumn<int>("compareValue");

    QTest::newRow("EqualStrings") << "Test"
                                  << "Test" << 0;
    QTest::newRow("EqualStringsLong") << "abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVXYZ"
                                      << "abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVXYZ" << 0;
    QTest::newRow("DifferentCase") << "test"
                                   << "Test" << 32;
    QTest::newRow("DifferentCaseLong")
            << "abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVXYZ"
            << "abcdefghijklmnopqrstuvxyzabcdefghijklmnopqrstuvxyz" << -32;
    QTest::newRow("ReverseStrings") << "Test"
                                    << "tseT" << -32;
    QTest::newRow("Different Strings") << "Testing and testing"
                                       << "Testing and resting" << 2;
    QTest::newRow("Latin1RangeCharacter") << u8"B\u00d8" << u8"B\u00d8" << 0;
    QTest::newRow("Latin1RangeCharacterDifferentCase") << u8"B\u00d8" << u8"B\u00f8" << -32;
}

void tst_QUtf8StringView::compareStringsCaseInsensitive_data()
{
    QTest::addColumn<QString>("lhs");
    QTest::addColumn<QString>("rhs");
    QTest::addColumn<int>("compareValue");

    QTest::newRow("EqualStrings") << "Test"
                                  << "Test" << 0;
    QTest::newRow("EqualStringsLong") << "abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVXYZ"
                                      << "abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVXYZ" << 0;
    QTest::newRow("DifferentCase") << "test"
                                   << "Test" << 0;
    QTest::newRow("DifferentCaseLong") << "abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVXYZ"
                                       << "abcdefghijklmnopqrstuvxyzabcdefghijklmnopqrstuvxyz" << 0;
    QTest::newRow("ReverseStrings") << "Test"
                                    << "tseT" << -14;
    QTest::newRow("Different Strings") << "Testing and testing"
                                       << "Testing and resting" << 2;
    QTest::newRow("Latin1RangeCharacter") << u8"B\u00d8" << u8"B\u00d8" << 0;
    QTest::newRow("Latin1RangeCharacterDifferentCase") << u8"B\u00d8" << u8"B\u00f8" << 0;
}

void tst_QUtf8StringView::compareStringsLatin1(bool caseSensitive)
{
    QFETCH(QString, lhs);
    QFETCH(QString, rhs);
    QFETCH(int, compareValue);
    QByteArray left = lhs.toUtf8();
    QByteArray right = rhs.toLatin1();
    QBasicUtf8StringView<false> lhv(left);
    QLatin1StringView rhv(right);
    Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int result;

    QBENCHMARK {
        result = lhv.compare(rhv, cs);
    };
    QCOMPARE(result, compareValue);
}

void tst_QUtf8StringView::compareStringsUtf16(bool caseSensitive)
{
    QFETCH(QString, lhs);
    QFETCH(QString, rhs);
    QFETCH(int, compareValue);

    QByteArray left = lhs.toUtf8();
    QBasicUtf8StringView<false> lhv(left);
    QStringView rhv(rhs);
    Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int result;

    QBENCHMARK {
        result = lhv.compare(rhv, cs);
    };
    QCOMPARE(result, compareValue);
}

void tst_QUtf8StringView::compareStringsUtf8(bool caseSensitive)
{
    QFETCH(QString, lhs);
    QFETCH(QString, rhs);
    QFETCH(int, compareValue);

    QByteArray left = lhs.toUtf8();
    QByteArray right = rhs.toUtf8();
    QBasicUtf8StringView<false> lhv(left);
    QBasicUtf8StringView<false> rhv(right);
    Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int result;

    QBENCHMARK {
        result = lhv.compare(rhv, cs);
    };
    QCOMPARE(result, compareValue);
}

void tst_QUtf8StringView::compareStringsWithErrors_data()
{
    QTest::addColumn<QByteArray>("lhs");
    QTest::addColumn<QByteArray>("rhs");
    QTest::addColumn<int>("compare");
    QTest::addColumn<bool>("caseSensitive");

    QTest::newRow("Compare errors 0xfe vs 0xff case-insensitive")
            << QByteArray("\xfe") << QByteArray("\xff") << 0 << false;
    QTest::newRow("Compare errors 0xff vs 0xff case-insensitive")
            << QByteArray("\xff") << QByteArray("\xff") << 0 << false;
    QTest::newRow("Compare 'a' with error 0xff case-insensitive")
            << QByteArray("a") << QByteArray("\xff") << -65436 << false;
    QTest::newRow("Compare errors 0xfe vs 0xff case-sensitive")
            << QByteArray("\xfe") << QByteArray("\xff") << -1 << true;
    QTest::newRow("Compare errors 0xff vs 0xff case-sensitive")
            << QByteArray("\xff") << QByteArray("\xff") << 0 << true;
    QTest::newRow("Compare 'a' with error 0xff case-sensitive")
            << QByteArray("a") << QByteArray("\xff") << -158 << true;
}

void tst_QUtf8StringView::compareStringsWithErrors()
{
    QFETCH(QByteArray, lhs);
    QFETCH(QByteArray, rhs);
    QFETCH(int, compare);
    QFETCH(bool, caseSensitive);
    QBasicUtf8StringView<false> lhv(lhs);
    QBasicUtf8StringView<false> rhv(rhs);
    Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int result;

    QBENCHMARK {
        result = lhv.compare(rhv, cs);
    };
    QCOMPARE(result, compare);
    QCOMPARE(-result, rhv.compare(lhv, cs));
}

QTEST_MAIN(tst_QUtf8StringView)

#include "tst_bench_qutf8stringview.moc"
