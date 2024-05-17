// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <qlocale.h>
#include <qcollator.h>
#include <private/qglobal_p.h>
#include <QScopeGuard>

#include <cstring>
#include <iostream>

class tst_QCollator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void basics();
    void moveSemantics();

    void compare_data();
    void compare();

    void state();
};

static bool dpointer_is_null(QCollator &c)
{
    char mem[sizeof c];
    using namespace std;
    memcpy(mem, &c, sizeof c);
    for (size_t i = 0; i < sizeof c; ++i)
        if (mem[i])
            return false;
    return true;
}

void tst_QCollator::basics()
{
    const QLocale de_AT(QLocale::German, QLocale::Austria);

    QCollator c1(de_AT);
    QCOMPARE(c1.locale(), de_AT);

    QCollator c2(c1);
    QCOMPARE(c2.locale(), de_AT);

    QCollator c3;
    // Test copy assignment
    c3 = c2;
    QCOMPARE(c3.locale(), de_AT);

    // posix implementation supports only C and default locale,
    // so update it for Android and INTEGRITY builds
#if !QT_CONFIG(icu) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    c3.setLocale(QLocale());
#endif
    QCollatorSortKey key1 = c3.sortKey("test");

    QCollatorSortKey key2(key1);
    QCOMPARE(key1.compare(key2), 0);

    QCollatorSortKey key3 = c3.sortKey("abc");
    // Test copy assignment
    key3 = key2;
    QCOMPARE(key1.compare(key3), 0);
}

void tst_QCollator::moveSemantics()
{
    const QLocale de_AT(QLocale::German, QLocale::Austria);

    QCollator c1(de_AT);
    QCOMPARE(c1.locale(), de_AT);

    QCollator c2(std::move(c1));
    QCOMPARE(c2.locale(), de_AT);
    QVERIFY(dpointer_is_null(c1));

    QCollator c3(c1);
    QVERIFY(dpointer_is_null(c3));

    c1 = std::move(c2);
    QCOMPARE(c1.locale(), de_AT);
    QVERIFY(dpointer_is_null(c2));

    // test QCollatorSortKey move assignment
    // posix implementation supports only C and default locale,
    // so update it for Android and INTEGRITY builds
#if !QT_CONFIG(icu) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    c1.setLocale(QLocale());
#endif
    QCollatorSortKey key1 = c1.sortKey("1");
    QCollatorSortKey key2 = c1.sortKey("2");
    QVERIFY(key1.compare(key2) < 0);

    QCollatorSortKey key3 = c1.sortKey("a");
    // test move assignment
    key3 = std::move(key2);
    QVERIFY(key1.compare(key3) < 0);
}


void tst_QCollator::compare_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("s1");
    QTest::addColumn<QString>("s2");
    QTest::addColumn<int>("result");
    QTest::addColumn<int>("caseInsensitiveResult");
    QTest::addColumn<bool>("numericMode");
    QTest::addColumn<bool>("ignorePunctuation");
    QTest::addColumn<int>("punctuationResult"); // Test ignores punctuation *and case*

    /*
        It's hard to test English, because it's treated differently
        on different platforms. For example, on Linux, it uses the
        iso14651_t1 template file, which happens to provide good
        defaults for Swedish. OS X seems to do a pure bytewise
        comparison of Latin-1 values, although I'm not sure. So I
        just test digits to make sure that it's not totally broken.
    */
    QTest::newRow("english1") << QString("en_US") << QString("5") << QString("4") << 1 << 1 << false << false << 1;
    QTest::newRow("english2") << QString("en_US") << QString("4") << QString("6") << -1 << -1 << false << false << -1;
    QTest::newRow("english3") << QString("en_US") << QString("5") << QString("6") << -1 << -1 << false << false << -1;
    QTest::newRow("english4") << QString("en_US") << QString("a") << QString("b") << -1 << -1 << false << false << -1;
    QTest::newRow("english5") << QString("en_US") << QString("test 9") << QString("test 19") << -1 << -1 << true << false << -1;
    QTest::newRow("english6") << QString("en_US") << QString("test 9") << QString("test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("english7") << QString("en_US") << QString("test_19") << QString("test 19") << 1 << 1 << true << false << 1;
    QTest::newRow("english8") << QString("en_US") << QString("test.19") << QString("test,19") << 1 << 1 << true << true << 0;
    QTest::newRow("en-empty-word") << QString("en_US") << QString() << QString("non-empty") << -1 << -1 << false << true << -1;
    QTest::newRow("en-empty-number") << QString("en_US") << QString() << QString("42") << -1 << -1 << true << true << -1;
    QTest::newRow("en-word-empty") << QString("en_US") << QString("non-empty") << QString() << 1
                                   << 1 << false << true << 1;
    QTest::newRow("en-number-empty")
            << QString("en_US") << QString("42") << QString() << 1 << 1 << true << true << 1;
    QTest::newRow("en-empty-empty")
            << QString("en_US") << QString() << QString() << 0 << 0 << false << true << 0;

    /*
        In Swedish, a with ring above (E5) comes before a with
        diaresis (E4), which comes before o diaresis (F6), which
        all come after z.
    */
    QTest::newRow("swedish1") << QString("sv_SE") << QString::fromLatin1("\xe5") << QString::fromLatin1("\xe4") << -1 << -1 << false << false << -1;
    QTest::newRow("swedish2") << QString("sv_SE") << QString::fromLatin1("\xe4") << QString::fromLatin1("\xf6") << -1 << -1 << false << false << -1;
    QTest::newRow("swedish3") << QString("sv_SE") << QString::fromLatin1("\xe5") << QString::fromLatin1("\xf6") << -1 << -1 << false << false << -1;
    QTest::newRow("swedish4") << QString("sv_SE") << QString::fromLatin1("z") << QString::fromLatin1("\xe5") << -1 << -1 << false << false << -1;
    QTest::newRow("swedish5") << QString("sv_SE") << QString("9") << QString("19") << -1 << -1 << true << false << -1;
    QTest::newRow("swedish6") << QString("sv_SE") << QString("Test 9") << QString("Test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("swedish7") << QString("sv_SE") << QString("test_19") << QString("test 19") << 1 << 1 << true << false << 1;
    QTest::newRow("swedish8") << QString("sv_SE") << QString("test.19") << QString("test,19") << 1 << 1 << true << true << 0;
    QTest::newRow("sv-empty-word") << QString("sv_SE") << QString() << QString("mett") << -1 << -1 << false << true << -1;
    QTest::newRow("sv-empty-number") << QString("sv_SE") << QString() << QString("42") << -1 << -1 << true << true << -1;
    QTest::newRow("sv-word-empty")
            << QString("sv_SE") << QString("mett") << QString() << 1 << 1 << false << true << 1;
    QTest::newRow("sv-number-empty")
            << QString("sv_SE") << QString("42") << QString() << 1 << 1 << true << true << 1;
    QTest::newRow("sv-empty-empty")
            << QString("sv_SE") << QString() << QString() << 0 << 0 << false << true << 0;

    /*
        In Norwegian, ae (E6) comes before o with stroke (D8), which
        comes before a with ring above (E5).
    */
    QTest::newRow("norwegian1") << QString("no_NO") << QString::fromLatin1("\xe6") << QString::fromLatin1("\xd8") << -1 << -1 << false << false << -1;
    QTest::newRow("norwegian2") << QString("no_NO") << QString::fromLatin1("\xd8") << QString::fromLatin1("\xe5") << -1 << -1 << false << false << -1;
    QTest::newRow("norwegian3") << QString("no_NO") << QString::fromLatin1("\xe6") << QString::fromLatin1("\xe5") << -1 << -1 << false << false << -1;
    QTest::newRow("norwegian4") << QString("no_NO") << QString("9") << QString("19") << -1 << -1 << true << false << -1;
    QTest::newRow("norwegian5") << QString("no_NO") << QString("Test 9") << QString("Test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("norwegian6") << QString("no_NO") << QString("Test 9") << QString("Test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("norwegian7") << QString("no_NO") << QString("test_19") << QString("test 19") << 1 << 1 << true << false << 1;
    QTest::newRow("norwegian8") << QString("no_NO") << QString("test.19") << QString("test,19") << 1 << 1 << true << true << 0;
    QTest::newRow("nb-empty-word") << QString("nb_NO") << QString() << QString("mett") << -1 << -1 << false << true << -1;
    QTest::newRow("nb-empty-number") << QString("nb_NO") << QString() << QString("42") << -1 << -1 << true << true << -1;
    QTest::newRow("nb-word-empty")
            << QString("nb_NO") << QString("mett") << QString() << 1 << 1 << false << true << 1;
    QTest::newRow("nb-number-empty")
            << QString("nb_NO") << QString("42") << QString() << 1 << 1 << true << true << 1;
    QTest::newRow("nb-empty-empty")
            << QString("nb_NO") << QString() << QString() << 0 << 0 << false << true << 0;

    /*
        In German, z comes *after* a with diaresis (E4),
        which comes before o diaresis (F6).
    */
    QTest::newRow("german1") << QString("de_DE") << QString::fromLatin1("a") << QString::fromLatin1("\xe4") << -1 << -1 << false << false << -1;
    QTest::newRow("german2") << QString("de_DE") << QString::fromLatin1("b") << QString::fromLatin1("\xe4") << 1 << 1 << false << false << 1;
    QTest::newRow("german3") << QString("de_DE") << QString::fromLatin1("z") << QString::fromLatin1("\xe4") << 1 << 1 << false << false << 1;
    QTest::newRow("german4") << QString("de_DE") << QString::fromLatin1("\xe4") << QString::fromLatin1("\xf6") << -1 << -1 << false << false << -1;
    QTest::newRow("german5") << QString("de_DE") << QString::fromLatin1("z") << QString::fromLatin1("\xf6") << 1 << 1 << false << false << 1;
    QTest::newRow("german6") << QString("de_DE") << QString::fromLatin1("\xc0") << QString::fromLatin1("\xe0") << 1 << 0 << false << false << 0;
    QTest::newRow("german7") << QString("de_DE") << QString::fromLatin1("\xd6") << QString::fromLatin1("\xf6") << 1 << 0 << false << false << 0;
    QTest::newRow("german8") << QString("de_DE") << QString::fromLatin1("oe") << QString::fromLatin1("\xf6") << 1 << 1 << false << false << 1;
    QTest::newRow("german9") << QString("de_DE") << QString("A") << QString("a") << 1 << 0 << false << false << 0;
    QTest::newRow("german10") << QString("de_DE") << QString("9") << QString("19") << -1 << -1 << true << false << -1;
    QTest::newRow("german11") << QString("de_DE") << QString("Test 9") << QString("Test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("german12") << QString("de_DE") << QString("test_19") << QString("test 19") << 1 << 1 << true << false << 1;
    QTest::newRow("german13") << QString("de_DE") << QString("test.19") << QString("test,19") << 1 << 1 << true << true << 0;
    QTest::newRow("de-empty-word") << QString("de_DE") << QString() << QString("satt") << -1 << -1 << false << true << -1;
    QTest::newRow("de-empty-number") << QString("de_DE") << QString() << QString("42") << -1 << -1 << true << true << -1;
    QTest::newRow("de-word-empty")
            << QString("de_DE") << QString("satt") << QString() << 1 << 1 << false << true << 1;
    QTest::newRow("de-number-empty")
            << QString("de_DE") << QString("42") << QString() << 1 << 1 << true << true << 1;
    QTest::newRow("de-empty-empty")
            << QString("de_DE") << QString() << QString() << 0 << 0 << false << true << 0;

    /*
        French sorting of e and e with acute accent
    */
    QTest::newRow("french1") << QString("fr_FR") << QString::fromLatin1("\xe9") << QString::fromLatin1("e") << 1 << 1 << false << false << 1;
    QTest::newRow("french2") << QString("fr_FR") << QString::fromLatin1("\xe9t") << QString::fromLatin1("et") << 1 << 1 << false << false << 1;
    QTest::newRow("french3") << QString("fr_FR") << QString::fromLatin1("\xe9") << QString::fromLatin1("d") << 1 << 1 << false << false << 1;
    QTest::newRow("french4") << QString("fr_FR") << QString::fromLatin1("\xe9") << QString::fromLatin1("f") << -1 << -1 << false << false << -1;
    QTest::newRow("french5") << QString("fr_FR") << QString("9") << QString("19") << -1 << -1 << true << false << -1;
    QTest::newRow("french6") << QString("fr_FR") << QString("Test 9") << QString("Test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("french7") << QString("fr_FR") << QString("test_19") << QString("test 19") << 1 << 1 << true << false << 1;
    QTest::newRow("french8") << QString("fr_FR") << QString("test.19") << QString("test,19") << 1 << 1 << true << true << 0;
    QTest::newRow("fr-empty-word") << QString("fr_FR") << QString() << QString("plein") << -1 << -1 << false << true << -1;
    QTest::newRow("fr-empty-number") << QString("fr_FR") << QString() << QString("42") << -1 << -1 << true << true << -1;
    QTest::newRow("fr-word-empty")
            << QString("fr_FR") << QString("plein") << QString() << 1 << 1 << false << true << 1;
    QTest::newRow("fr-number-empty")
            << QString("fr_FR") << QString("42") << QString() << 1 << 1 << true << true << 1;
    QTest::newRow("fr-empty-empty")
            << QString("fr_FR") << QString() << QString() << 0 << 0 << false << true << 0;

    // C locale: case sensitive [A-Z] < [a-z] but case insensitive [Aa] < [Bb] <...< [Zz]
    const QString C = QStringLiteral("C");
    QTest::newRow("C:ABBA:AaaA") << C << QStringLiteral("ABBA") << QStringLiteral("AaaA") << -1 << 1 << false << false << 1;
    QTest::newRow("C:AZa:aAZ") << C << QStringLiteral("AZa") << QStringLiteral("aAZ") << -1 << 1 << false << false << 1;
    QTest::newRow("C-empty-word") << QString(C) << QString() << QString("non-empty") << -1 << -1 << false << true << -1;
    QTest::newRow("C-empty-number") << QString(C) << QString() << QString("42") << -1 << -1 << true << true << -1;
    QTest::newRow("C-word-empty") << QString(C) << QString("non-empty") << QString() << 1 << 1
                                  << false << true << 1;
    QTest::newRow("C-number-empty")
            << QString(C) << QString("42") << QString() << 1 << 1 << true << true << 1;
    QTest::newRow("C-empty-empty")
            << QString(C) << QString() << QString() << 0 << 0 << false << true << 0;
}

void tst_QCollator::compare()
{
    QFETCH(QString, locale);
    QFETCH(QString, s1);
    QFETCH(QString, s2);
    QFETCH(int, result);
    QFETCH(int, caseInsensitiveResult);
    QFETCH(bool, numericMode);
    QFETCH(bool, ignorePunctuation);
    QFETCH(int, punctuationResult);

    QCollator collator((QLocale(locale)));

    // AFTER the QCollator initialization
    auto localechanger = qScopeGuard([original = QLocale()] {
        QLocale::setDefault(original);  // reset back to what it was
    });
    QLocale::setDefault(QLocale(locale));

    // Need to canonicalize sign to -1, 0 or 1, as .compare() can produce any -ve for <, any +ve for >.
    auto asSign = [](int compared) {
        return compared < 0 ? -1 : compared > 0 ? 1 : 0;
    };
#if defined(Q_OS_WASM)
    if (strcmp(QTest::currentDataTag(), "english5") == 0
        || strcmp(QTest::currentDataTag(), "english8") == 0)
        QSKIP("Some en-us locale tests have issues on WASM");
#endif // Q_OS_WASM
#if !QT_CONFIG(icu) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    if (collator.locale() != QLocale::c() && collator.locale() != QLocale::system().collation())
        QSKIP("POSIX implementation of collation only supports C and system collation locales");
#endif

    if (numericMode)
        collator.setNumericMode(true);

    [[maybe_unused]] int keyCompareResult = result;
    [[maybe_unused]] int keyCompareCaseInsensitiveResult = caseInsensitiveResult;
    [[maybe_unused]] int keyComparePunctuationResultResult = punctuationResult;

    // trying to deal with special behavior of different OS-dependent collators
    if (collator.locale() == QLocale("C")) {
#if !QT_CONFIG(icu) && defined(Q_OS_MACOS)
        // for MACOS C-locale is not supported, always providing empty string for sortKey()
        keyCompareResult = 0;
        keyCompareCaseInsensitiveResult = 0;
        keyComparePunctuationResultResult = 0;
#else
        // for other platforms C-locale strings are not modified by sortKey() anyhow
        keyCompareCaseInsensitiveResult = keyCompareResult;
        keyComparePunctuationResultResult = keyCompareResult;
#endif
    }

    // NOTE: currently QCollatorSortKey::compare is not working
    // properly without icu: see QTBUG-88704 for details
    QCOMPARE(asSign(collator.compare(s1, s2)), result);
    if (!numericMode)
        QCOMPARE(asSign(QCollator::defaultCompare(s1, s2)), result);
#if QT_CONFIG(icu)
    auto key1 = collator.sortKey(s1);
    auto key2 = collator.sortKey(s2);
    QCOMPARE(asSign(key1.compare(key2)), keyCompareResult);

    key1 = QCollator::defaultSortKey(s1);
    key2 = QCollator::defaultSortKey(s2);
    if (!numericMode)
        QCOMPARE(asSign(key1.compare(key2)), keyCompareResult);
#endif
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    QCOMPARE(asSign(collator.compare(s1, s2)), caseInsensitiveResult);
#if QT_CONFIG(icu)
    key1 = collator.sortKey(s1);
    key2 = collator.sortKey(s2);
    QCOMPARE(asSign(key1.compare(key2)), keyCompareCaseInsensitiveResult);
#endif
    collator.setIgnorePunctuation(ignorePunctuation);
    QCOMPARE(asSign(collator.compare(s1, s2)), punctuationResult);
#if QT_CONFIG(icu)
    key1 = collator.sortKey(s1);
    key2 = collator.sortKey(s2);
    QCOMPARE(asSign(key1.compare(key2)), keyComparePunctuationResultResult);
#endif
}


void tst_QCollator::state()
{
    QCollator c;
    c.setCaseSensitivity(Qt::CaseInsensitive);
    c.setLocale(QLocale::German);

    c.compare(QString("a"), QString("b"));

    QCOMPARE(c.caseSensitivity(), Qt::CaseInsensitive);
    QCOMPARE(c.locale(), QLocale(QLocale::German));

    c.setLocale(QLocale::French);
    c.setNumericMode(true);
    c.setIgnorePunctuation(true);
    c.setLocale(QLocale::NorwegianBokmal);

    QCOMPARE(c.caseSensitivity(), Qt::CaseInsensitive);
    QCOMPARE(c.numericMode(), true);
    QCOMPARE(c.ignorePunctuation(), true);
    QCOMPARE(c.locale(), QLocale(QLocale::NorwegianBokmal));
}

QTEST_APPLESS_MAIN(tst_QCollator)

#include "tst_qcollator.moc"
