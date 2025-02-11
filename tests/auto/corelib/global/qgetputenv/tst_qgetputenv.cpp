// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qdebug.h>
#include <QTest>

#include <qglobal.h>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

class tst_QGetPutEnv : public QObject
{
Q_OBJECT
private slots:
    void getSetCheck();
    void encoding();
    void intValue_data();
    void intValue();
};

void tst_QGetPutEnv::getSetCheck()
{
    const char varName[] = "should_not_exist";

    bool ok;

    QVERIFY(!qEnvironmentVariableIsSet(varName));
    QVERIFY(qEnvironmentVariableIsEmpty(varName));
    ok = true;
    QCOMPARE(qEnvironmentVariableIntValue(varName), 0);
    QCOMPARE(qEnvironmentVariableIntValue(varName, &ok), 0);
    QVERIFY(!ok);
    QByteArray result = qgetenv(varName);
    QVERIFY(result.isNull());
    QString sresult = qEnvironmentVariable(varName);
    QVERIFY(sresult.isNull());
    sresult = qEnvironmentVariable(varName, "hello");
    QCOMPARE(sresult, QString("hello"));

#ifndef Q_OS_WIN
    QVERIFY(qputenv(varName, "")); // deletes varName instead of making it empty, on Windows

    QVERIFY(qEnvironmentVariableIsSet(varName));
    QVERIFY(qEnvironmentVariableIsEmpty(varName));
    ok = true;
    QCOMPARE(qEnvironmentVariableIntValue(varName), 0);
    QCOMPARE(qEnvironmentVariableIntValue(varName, &ok), 0);
    QVERIFY(!ok);

    result = qgetenv(varName);
    QVERIFY(!result.isNull());
    QCOMPARE(result, QByteArray());
    sresult = qEnvironmentVariable(varName);
    QVERIFY(!sresult.isNull());
    QCOMPARE(sresult, QString());
    sresult = qEnvironmentVariable(varName, "hello");
    QVERIFY(!sresult.isNull());
    QCOMPARE(sresult, QString());
#endif

    constexpr char varValueFullString[] = "supervalue123";
    const auto varValueQBA = QByteArray::fromRawData(varValueFullString, sizeof varValueFullString - 4);
    QCOMPARE_EQ(varValueQBA, "supervalue");

    QVERIFY(qputenv(varName, varValueQBA));

    QVERIFY(qEnvironmentVariableIsSet(varName));
    QVERIFY(!qEnvironmentVariableIsEmpty(varName));
    ok = true;
    QCOMPARE(qEnvironmentVariableIntValue(varName), 0);
    QCOMPARE(qEnvironmentVariableIntValue(varName, &ok), 0);
    QVERIFY(!ok);
    result = qgetenv(varName);
    QCOMPARE(result, QByteArrayLiteral("supervalue"));
    sresult = qEnvironmentVariable(varName);
    QCOMPARE(sresult, QString("supervalue"));
    sresult = qEnvironmentVariable(varName, "hello");
    QCOMPARE(sresult, QString("supervalue"));

    qputenv(varName,QByteArray());

    // Now test qunsetenv
    QVERIFY(qunsetenv(varName));
    QVERIFY(!qEnvironmentVariableIsSet(varName)); // note: might fail on some systems!
    QVERIFY(qEnvironmentVariableIsEmpty(varName));
    ok = true;
    QCOMPARE(qEnvironmentVariableIntValue(varName), 0);
    QCOMPARE(qEnvironmentVariableIntValue(varName, &ok), 0);
    QVERIFY(!ok);

    result = qgetenv(varName);
    QVERIFY(result.isNull());
    sresult = qEnvironmentVariable(varName);
    QVERIFY(sresult.isNull());
    sresult = qEnvironmentVariable(varName, "hello");
    QCOMPARE(sresult, QString("hello"));
}

void tst_QGetPutEnv::encoding()
{
    // The test string is:
    //  U+0061      LATIN SMALL LETTER A
    //  U+00E1      LATIN SMALL LETTER A WITH ACUTE
    //  U+03B1      GREEK SMALL LETTER ALPHA
    //  U+0430      CYRILLIC SMALL LETTER A
    // This has letters in three different scripts, so no locale besides
    // UTF-8 is able handle them all.
    // The LATIN SMALL LETTER A WITH ACUTE is NFC for NFD:
    //  U+0061 U+0301   LATIN SMALL LETTER A + COMBINING ACUTE ACCENT

    const char varName[] = "should_not_exist";
    static const wchar_t rawvalue[] = { 'a', 0x00E1, 0x03B1, 0x0430, 0 };
    QString value = QString::fromWCharArray(rawvalue);

#if defined(Q_OS_WIN)
    const wchar_t wvarName[] = L"should_not_exist";
    _wputenv_s(wvarName, rawvalue);
#else
    // confirm the locale is UTF-8
    if (value.toLocal8Bit() != "a\xc3\xa1\xce\xb1\xd0\xb0")
        QSKIP("Locale is not UTF-8, cannot test");

    qputenv(varName, QFile::encodeName(value));
#endif

    QVERIFY(qEnvironmentVariableIsSet(varName));
    QCOMPARE(qEnvironmentVariable(varName), value);
}

void tst_QGetPutEnv::intValue_data()
{
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<int>("expected");
    QTest::addColumn<bool>("ok");

    // some repetition from what is tested in getSetCheck()
    QTest::newRow("empty") << QByteArray() << 0 << false;
    QTest::newRow("spaces-heading") << QByteArray(" \n\r\t1") << 1 << true;
    QTest::newRow("spaces-trailing") << QByteArray("1 \n\r\t") << 1 << true;
    QTest::newRow("junk-heading") << QByteArray("x1") << 0 << false;
    QTest::newRow("junk-trailing") << QByteArray("1x") << 0 << false;

    auto addRow = [](const char *text, int expected, bool ok) {
        QTest::newRow(text) << QByteArray(text) << expected << ok;
    };
    addRow("auto", 0, false);
    addRow("1auto", 0, false);
    addRow("0", 0, true);
    addRow("+0", 0, true);
    addRow("1", 1, true);
    addRow("+1", 1, true);
    addRow("09", 0, false);
    addRow("010", 8, true);
    addRow("0x10", 16, true);
    addRow("0x", 0, false);
    addRow("0xg", 0, false);
    addRow("0x1g", 0, false);
    addRow("000000000000000000000000000000000000000000000000001", 0, false);
    addRow("+000000000000000000000000000000000000000000000000001", 0, false);
    addRow("000000000000000000000000000000000000000000000000001g", 0, false);
    addRow("-0", 0, true);
    addRow("-1", -1, true);
    addRow("-010", -8, true);
    addRow("-000000000000000000000000000000000000000000000000001", 0, false);
    // addRow("0xffffffff", -1, true); // could be expected, but not how QByteArray::toInt() works
    addRow("0xffffffff", 0, false);

    auto addNumWithBase = [](qlonglong num, int base) {
        QByteArray text;
        {
            QTextStream s(&text);
            s.setIntegerBase(base);
            s << Qt::showbase << num;
        }
        QTestData &row = QTest::addRow("%s", text.constData()) << text;
        if (num == int(num))
            row << int(num) << true;
        else
            row << 0 << false;
    };
    for (int base : {10, 8, 16}) {
        addNumWithBase(INT_MAX, base);
        addNumWithBase(qlonglong(INT_MAX) + 1, base);
        addNumWithBase(INT_MIN, base);
        addNumWithBase(qlonglong(INT_MIN) - 1 , base);
    };
}

void tst_QGetPutEnv::intValue()
{
    const int maxlen = (sizeof(int) * CHAR_BIT + 2) / 3;
    const char varName[] = "should_not_exist";

    QFETCH(QByteArray, value);
    QFETCH(int, expected);
    QFETCH(bool, ok);

    bool actualOk = !ok;

    // Self-test: confirm that it was like the docs said it should be
    if (value.size() < maxlen) {
        QCOMPARE(value.toInt(&actualOk, 0), expected);
        QCOMPARE(actualOk, ok);
    }

    actualOk = !ok;
    QVERIFY(qputenv(varName, value));
    QCOMPARE(qEnvironmentVariableIntValue(varName), expected);
    QCOMPARE(qEnvironmentVariableIntValue(varName, &actualOk), expected);
    QCOMPARE(actualOk, ok);
}

QTEST_MAIN(tst_QGetPutEnv)
#include "tst_qgetputenv.moc"
