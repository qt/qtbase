// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <qstringmatcher.h>

class tst_QStringMatcher : public QObject
{
    Q_OBJECT

private slots:
    void qstringmatcher();
    void caseSensitivity();
    void indexIn_data();
    void indexIn();
    void setCaseSensitivity_data();
    void setCaseSensitivity();
    void assignOperator();
};

void tst_QStringMatcher::qstringmatcher()
{
    QStringMatcher matcher;
    QCOMPARE(matcher.caseSensitivity(), Qt::CaseSensitive);
    QCOMPARE(matcher.indexIn("foo", 1), 1);
    QCOMPARE(matcher.pattern(), QString());
    QCOMPARE(matcher.patternView(), QStringView());
}

// public Qt::CaseSensitivity caseSensitivity() const
void tst_QStringMatcher::caseSensitivity()
{
    const QString haystack = QStringLiteral("foobarFoo");
    const QStringView needle = QStringView{ haystack }.right(3); // "Foo"
    QStringMatcher matcher(needle.data(), needle.size());

    QCOMPARE(matcher.caseSensitivity(), Qt::CaseSensitive);
    QCOMPARE(matcher.indexIn(haystack), 6);

    matcher.setCaseSensitivity(Qt::CaseInsensitive);

    QCOMPARE(matcher.caseSensitivity(), Qt::CaseInsensitive);
    QCOMPARE(matcher.indexIn(haystack), 0);

    matcher.setCaseSensitivity(Qt::CaseSensitive);
    QCOMPARE(matcher.caseSensitivity(), Qt::CaseSensitive);
    QCOMPARE(matcher.indexIn(haystack), 6);
}

void tst_QStringMatcher::indexIn_data()
{
    QTest::addColumn<QString>("needle");
    QTest::addColumn<QString>("haystack");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("indexIn");
    QTest::newRow("empty-1") << QString() << QString("foo") << 0 << 0;
    QTest::newRow("empty-2") << QString() << QString("foo") << 10 << -1;
    QTest::newRow("empty-3") << QString() << QString("foo") << -10 << 0;

    QTest::newRow("simple-1") << QString("a") << QString("foo") << 0 << -1;
    QTest::newRow("simple-2") << QString("a") << QString("bar") << 0 << 1;
    QTest::newRow("harder-1") << QString("foo") << QString("slkdf sldkjf slakjf lskd ffools ldjf") << 0 << 26;
    QTest::newRow("harder-2") << QString("foo") << QString("slkdf sldkjf slakjf lskd ffools ldjf") << 20 << 26;
    QTest::newRow("harder-3") << QString("foo") << QString("slkdf sldkjf slakjf lskd ffools ldjf") << 26 << 26;
    QTest::newRow("harder-4") << QString("foo") << QString("slkdf sldkjf slakjf lskd ffools ldjf") << 27 << -1;
}

void tst_QStringMatcher::indexIn()
{
    QFETCH(QString, needle);
    QFETCH(QString, haystack);
    QFETCH(int, from);
    QFETCH(int, indexIn);

    QStringMatcher matcher;
    matcher.setPattern(needle);

    QCOMPARE(matcher.indexIn(haystack, from), indexIn);

    const auto needleSV = QStringView(needle);
    QStringMatcher matcherSV(needleSV);

    QCOMPARE(matcherSV.indexIn(QStringView(haystack), from), indexIn);
}

void tst_QStringMatcher::setCaseSensitivity_data()
{
    QTest::addColumn<QString>("needle");
    QTest::addColumn<QString>("haystack");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("indexIn");
    QTest::addColumn<int>("cs");

    QTest::newRow("overshot") << QString("foo") << QString("baFooz foo bar") << 14 << -1 << (int) Qt::CaseSensitive;
    QTest::newRow("sensitive") << QString("foo") << QString("baFooz foo bar") << 1 << 7 << (int) Qt::CaseSensitive;
    QTest::newRow("insensitive-1")
            << QString("foo") << QString("baFooz foo bar") << 0 << 2 << (int)Qt::CaseInsensitive;
    QTest::newRow("insensitive-2")
            << QString("foo") << QString("baFooz foo bar") << 1 << 2 << (int)Qt::CaseInsensitive;
    QTest::newRow("insensitive-3")
            << QString("foo") << QString("baFooz foo bar") << 4 << 7 << (int)Qt::CaseInsensitive;
    QTest::newRow("insensitive-4")
            << QString("foogabooga") << QString("baFooGaBooga foogabooga bar") << 1 << 2
            << (int)Qt::CaseInsensitive;
    QTest::newRow("insensitive-5")
            << QString("foogabooga") << QString("baFooGaBooga foogabooga bar") << 3 << 13
            << (int)Qt::CaseInsensitive;
    QTest::newRow("insensitive-6") << QString("foogabooga") << QString("GaBoogaFoogaBooga bar") << 0
                                   << 7 << (int)Qt::CaseInsensitive;
    QTest::newRow("insensitive-7") << QString("foogabooga") << QString("FoGaBoogaFoogaBooga") << 9
                                   << 9 << (int)Qt::CaseInsensitive;
    QTest::newRow("insensitive-8") << QString("foogaBooga") << QString("zzzzaazzffoogaBooga") << 0
                                   << 9 << (int)Qt::CaseInsensitive;
    QString stringOf32("abcdefghijklmnopqrstuvwxyz123456");
    Q_ASSERT(stringOf32.size() == 32);
    QString stringOf128 = stringOf32 + stringOf32 + stringOf32 + stringOf32;
    QString needle = stringOf128 + stringOf128 + "CAse";
    QString haystack = stringOf128 + stringOf128 + "caSE";
    QTest::newRow("insensitive-9") << needle << haystack << 0 << 0 << (int)Qt::CaseInsensitive;
}

void tst_QStringMatcher::setCaseSensitivity()
{
    QFETCH(QString, needle);
    QFETCH(QString, haystack);
    QFETCH(int, from);
    QFETCH(int, indexIn);
    QFETCH(int, cs);

    QStringMatcher matcher;
    matcher.setPattern(needle);
    matcher.setCaseSensitivity(static_cast<Qt::CaseSensitivity> (cs));

    QCOMPARE(matcher.indexIn(haystack, from), indexIn);
    QCOMPARE(matcher.indexIn(QStringView(haystack), from), indexIn);
}

void tst_QStringMatcher::assignOperator()
{
    QString needle("d");
    QString hayStack("abcdef");
    QStringMatcher m1(needle);
    QCOMPARE(m1.indexIn(hayStack), 3);

    QStringMatcher m2 = m1;
    QCOMPARE(m2.pattern(), needle);
    QCOMPARE(m2.patternView(), needle);
    QCOMPARE(m2.indexIn(hayStack), 3);
}

QTEST_MAIN(tst_QStringMatcher)
#include "tst_qstringmatcher.moc"

