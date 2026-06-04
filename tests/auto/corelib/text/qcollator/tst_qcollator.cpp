// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <qlocale.h>
#include <qcollator.h>
#include <private/qglobal_p.h>
#include <QScopeGuard>

#include <string.h>

using namespace Qt::StringLiterals;

class tst_QCollator : public QObject
{
    Q_OBJECT

private:
    using Opt = QCollator::CollationOption;
    using Opts = QCollator::CollationOptions;

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
    QCOMPARE(c1, QCollator());

    QCollator c3(c1);
    QVERIFY(dpointer_is_null(c3));

    c1 = std::move(c2);
    QCOMPARE(c1.locale(), de_AT);
    QVERIFY(dpointer_is_null(c2));
    QCOMPARE(c2, QCollator());

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
    QTest::addColumn<Opts>("options");
    QTest::addColumn<int>("result");

    /*
        It's hard to test English, because it's treated differently
        on different platforms. For example, on Linux, it uses the
        iso14651_t1 template file, which happens to provide good
        defaults for Swedish. OS X seems to do a pure bytewise
        comparison of Latin-1 values, although I'm not sure. So I
        just test digits to make sure that it's not totally broken.
    */
    QTest::newRow("en-5:4") << u"en_US"_s << u"5"_s << u"4"_s << Opts{} << 1;
    QTest::newRow("en-5:4-nocase")
            << u"en_US"_s << u"5"_s << u"4"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("en-4:6") << u"en_US"_s << u"4"_s << u"6"_s << Opts{} << -1;
    QTest::newRow("en-4:6-nocase")
            << u"en_US"_s << u"4"_s << u"6"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("en-5:6") << u"en_US"_s << u"5"_s << u"6"_s << Opts{} << -1;
    QTest::newRow("en-5:6-nocase")
            << u"en_US"_s << u"5"_s << u"6"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("en-a:b") << u"en_US"_s << u"a"_s << u"b"_s << Opts{} << -1;
    QTest::newRow("en-a:b-nocase")
            << u"en_US"_s << u"a"_s << u"b"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("en-é:e") << u"en_US"_s << u"é"_s << u"e"_s << Opts{} << 1;
    QTest::newRow("en-é:e-nodiacr")
            << u"en_US"_s << u"é"_s << u"e"_s << Opts{Opt::DiacriticInsensitive} << 0;
#if QT_CONFIG(icu) || !defined(Q_OS_WIN)
    // Windows applies ligature folding implicitly, so these compare as
    // equal even without the flag
    QTest::newRow("en-æ:ae") << u"en_US"_s << u"æ"_s << u"ae"_s << Opts{} << 1;
    QTest::newRow("en-ẞ:SS") << u"en_US"_s << u"ẞ"_s << u"SS"_s << Opts{} << 1;
#endif
#if QT_CONFIG(icu) || defined(Q_OS_WIN)
    // Ligature folding: ICU and Windows only, not supported on macOS backend
    QTest::newRow("en-ẞ:SS-nodiacr")
            << u"en_US"_s << u"ẞ"_s << u"SS"_s << Opts{Opt::DiacriticInsensitive} << 0;
    QTest::newRow("en-æ:ae-nodiacr")
            << u"en_US"_s << u"æ"_s << u"ae"_s << Opts{Opt::DiacriticInsensitive} << 0;
#endif
    QTest::newRow("en-9:19-numsort")
            << u"en_US"_s << u"test 9"_s << u"test 19"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("en-9:19-nocase")
            << u"en_US"_s << u"test 9"_s << u"test 19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("en-9:_19-numsort")
            << u"en_US"_s << u"test 9"_s << u"test_19"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("en-9:_19-nocase")
            << u"en_US"_s << u"test 9"_s << u"test_19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("en-9:_19-nopun")
            << u"en_US"_s << u"test 9"_s << u"test_19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << -1;
    QTest::newRow("en-_19:19-numsort")
            << u"en_US"_s << u"test_19"_s << u"test 19"_s << Opts{Opt::NumericSort} << 1;
    QTest::newRow("en-_19:19-nocase")
            << u"en_US"_s << u"test_19"_s << u"test 19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("en-.19:,19-numsort")
            << u"en_US"_s << u"test.19"_s << u"test,19"_s << Opts{Opt::NumericSort} << 1;
    QTest::newRow("en-.19:,19-nocase")
            << u"en_US"_s << u"test.19"_s << u"test,19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("en-.19:,19-nopun")
            << u"en_US"_s << u"test.19"_s << u"test,19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << 0;
    QTest::newRow("en-empty-word") << u"en_US"_s << QString() << u"non-empty"_s << Opts{} << -1;
    QTest::newRow("en-empty-word-nocase")
            << u"en_US"_s << QString() << u"non-empty"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("en-empty-word-nopun")
            << u"en_US"_s << QString() << u"non-empty"_s
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << -1;
    QTest::newRow("en-empty-number-numsort")
            << u"en_US"_s << QString() << u"42"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("en-empty-number-nocase")
            << u"en_US"_s << QString() << u"42"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("en-empty-number-nopun")
            << u"en_US"_s << QString() << u"42"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << -1;
    QTest::newRow("en-word-empty") << u"en_US"_s << u"non-empty"_s << QString() << Opts{} << 1;
    QTest::newRow("en-word-empty-nocase")
            << u"en_US"_s << u"non-empty"_s << QString() << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("en-word-empty-nopun")
            << u"en_US"_s << u"non-empty"_s << QString()
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 1;
    QTest::newRow("en-number-empty-numsort")
            << u"en_US"_s << u"42"_s << QString() << Opts{Opt::NumericSort} << 1;
    QTest::newRow("en-number-empty-nocase")
            << u"en_US"_s << u"42"_s << QString() << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("en-number-empty-nopun")
            << u"en_US"_s << u"42"_s << QString()
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << 1;
    QTest::newRow("en-empty-empty") << u"en_US"_s << QString() << QString() << Opts{} << 0;
    QTest::newRow("en-empty-empty-nocase")
            << u"en_US"_s << QString() << QString() << Opts{Opt::CaseInsensitive} << 0;
    QTest::newRow("en-empty-empty-nopun")
            << u"en_US"_s << QString() << QString()
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 0;

    /*
        In Swedish, a with ring above (å) comes before a with
        diaresis (ä), which comes before o diaresis (ö), which
        all come after z.
    */
    QTest::newRow("sv-å:ä") << u"sv_SE"_s << u"å"_s << u"ä"_s << Opts{} << -1;
    QTest::newRow("sv-å:ä-nocase")
            << u"sv_SE"_s << u"å"_s << u"ä"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("sv-ä:ö") << u"sv_SE"_s << u"ä"_s << u"ö"_s << Opts{} << -1;
    QTest::newRow("sv-ä:ö-nocase")
            << u"sv_SE"_s << u"ä"_s << u"ö"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("sv-å:ö") << u"sv_SE"_s << u"å"_s << u"ö"_s << Opts{} << -1;
    QTest::newRow("sv-å:ö-nocase")
            << u"sv_SE"_s << u"å"_s << u"ö"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("sv-z:å") << u"sv_SE"_s << u"z"_s << u"å"_s << Opts{} << -1;
    QTest::newRow("sv-z:å-nocase")
            << u"sv_SE"_s << u"z"_s << u"å"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("sv-å:a")
            << u"sv_SE"_s << u"å"_s << u"a"_s << Opts{} << 1;
    QTest::newRow("sv-å:a-nodiacr")
            << u"sv_SE"_s << u"å"_s << u"a"_s << Opts{Opt::DiacriticInsensitive} << 1;
    QTest::newRow("sv-ä:a")
            << u"sv_SE"_s << u"ä"_s << u"a"_s << Opts{} << 1;
    QTest::newRow("sv-ä:a-nodiacr")
            << u"sv_SE"_s << u"ä"_s << u"a"_s << Opts{Opt::DiacriticInsensitive} << 1;
    QTest::newRow("sv-ö:o")
            << u"sv_SE"_s << u"ö"_s << u"o"_s << Opts{} << 1;
    QTest::newRow("sv-ö:o-nodiacr")
            << u"sv_SE"_s << u"ö"_s << u"o"_s << Opts{Opt::DiacriticInsensitive} << 1;
    QTest::newRow("sv-9:19-numsort")
            << u"sv_SE"_s << u"9"_s << u"19"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("sv-9:19-nocase")
            << u"sv_SE"_s << u"9"_s << u"19"_s << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("sv-9:_19-numsort")
            << u"sv_SE"_s << u"Test 9"_s << u"Test_19"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("sv-9:_19-nocase")
            << u"sv_SE"_s << u"Test 9"_s << u"Test_19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("sv-9:_19-nopun")
            << u"sv_SE"_s << u"Test 9"_s << u"Test_19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << -1;
    QTest::newRow("sv-_19:19-numsort")
            << u"sv_SE"_s << u"test_19"_s << u"test 19"_s << Opts{Opt::NumericSort} << 1;
    QTest::newRow("sv-_19:19-nocase")
            << u"sv_SE"_s << u"test_19"_s << u"test 19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("sv-.19:,19-numsort")
            << u"sv_SE"_s << u"test.19"_s << u"test,19"_s << Opts{Opt::NumericSort} << 1;
    QTest::newRow("sv-.19:,19-nocase")
            << u"sv_SE"_s << u"test.19"_s << u"test,19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("sv-.19:,19-nopun")
            << u"sv_SE"_s << u"test.19"_s << u"test,19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << 0;
    QTest::newRow("sv-empty-word") << u"sv_SE"_s << QString() << u"mett"_s << Opts{} << -1;
    QTest::newRow("sv-empty-word-nocase")
            << u"sv_SE"_s << QString() << u"mett"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("sv-empty-word-nopun")
            << u"sv_SE"_s << QString() << u"mett"_s
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << -1;
    QTest::newRow("sv-empty-number-numsort")
            << u"sv_SE"_s << QString() << u"42"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("sv-empty-number-nocase")
            << u"sv_SE"_s << QString() << u"42"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("sv-empty-number-nopun")
            << u"sv_SE"_s << QString() << u"42"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << -1;
    QTest::newRow("sv-word-empty") << u"sv_SE"_s << u"mett"_s << QString() << Opts{} << 1;
    QTest::newRow("sv-word-empty-nocase")
            << u"sv_SE"_s << u"mett"_s << QString() << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("sv-word-empty-nopun")
            << u"sv_SE"_s << u"mett"_s << QString()
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 1;
    QTest::newRow("sv-number-empty-numsort")
            << u"sv_SE"_s << u"42"_s << QString() << Opts{Opt::NumericSort} << 1;
    QTest::newRow("sv-number-empty-nocase")
            << u"sv_SE"_s << u"42"_s << QString() << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("sv-number-empty-nopun")
            << u"sv_SE"_s << u"42"_s << QString()
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << 1;
    QTest::newRow("sv-empty-empty") << u"sv_SE"_s << QString() << QString() << Opts{} << 0;
    QTest::newRow("sv-empty-empty-nocase")
            << u"sv_SE"_s << QString() << QString() << Opts{Opt::CaseInsensitive} << 0;
    QTest::newRow("sv-empty-empty-nopun")
            << u"sv_SE"_s << QString() << QString()
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 0;

    /*
        In Norwegian, ae (æ) comes before o with stroke (ø), which
        comes before a with ring above (å).
    */
    QTest::newRow("no-æ:Ø") << u"no_NO"_s << u"æ"_s << u"Ø"_s << Opts{} << -1;
    QTest::newRow("no-æ:Ø-nocase")
            << u"no_NO"_s << u"æ"_s << u"Ø"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("no-Ø:å") << u"no_NO"_s << u"Ø"_s << u"å"_s << Opts{} << -1;
    QTest::newRow("no-Ø:å-nocase")
            << u"no_NO"_s << u"Ø"_s << u"å"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("no-æ:å") << u"no_NO"_s << u"æ"_s << u"å"_s << Opts{} << -1;
    QTest::newRow("no-æ:å-nocase")
            << u"no_NO"_s << u"æ"_s << u"å"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("no-å:aa")
            << u"no_NO"_s << u"å"_s << u"aa"_s << Opts{} << -1;
    QTest::newRow("no-å:aa-nodiacr")
            << u"no_NO"_s << u"å"_s << u"aa"_s << Opts{Opt::DiacriticInsensitive} << 0;
    QTest::newRow("no-å:AA-nocase")
            << u"no_NO"_s << u"å"_s << u"AA"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("no-å:AA-nodiacr")
            << u"no_NO"_s << u"å"_s << u"AA"_s << Opts{Opt::DiacriticInsensitive}
            << -1;
    QTest::newRow("no-å:AA-nocase-nodiacr")
            << u"no_NO"_s << u"å"_s << u"AA"_s << (Opt::CaseInsensitive | Opt::DiacriticInsensitive)
            << 0;
    QTest::newRow("no-å:a")
            << u"no_NO"_s << u"å"_s << u"a"_s << Opts{} << 1;
    QTest::newRow("no-å:a-nodiacr")
            << u"no_NO"_s << u"å"_s << u"a"_s << Opts{Opt::DiacriticInsensitive} << 1;
    QTest::newRow("no-å:a-nocase-nodiacr")
            << u"no_NO"_s << u"å"_s << u"a"_s << (Opt::CaseInsensitive | Opt::DiacriticInsensitive)
            << 1;
    QTest::newRow("no-ø:o") << u"no_NO"_s << u"ø"_s << u"o"_s << Opts{} << 1;
    QTest::newRow("no-ø:o-nodiacr")
            << u"no_NO"_s << u"ø"_s << u"o"_s << Opts{Opt::DiacriticInsensitive} << 1;
    QTest::newRow("no-ø:o-nocase-nodiacr")
            << u"no_NO"_s << u"ø"_s << u"o"_s << (Opt::CaseInsensitive | Opt::DiacriticInsensitive)
            << 1;
    QTest::newRow("no-Ø:O")
            << u"no_NO"_s << u"Ø"_s << u"O"_s << Opts{} << 1;
    QTest::newRow("no-Ø:O-nocase")
            << u"no_NO"_s << u"Ø"_s << u"O"_s << Opts{Opt::DiacriticInsensitive} << 1;
    QTest::newRow("no-Ø:O-nocase-nodiacr")
            << u"no_NO"_s << u"Ø"_s << u"O"_s << (Opt::CaseInsensitive | Opt::DiacriticInsensitive)
            << 1;
    QTest::newRow("no-æ:ae")
            << u"no_NO"_s << u"æ"_s << u"ae"_s << Opts{} << 1;
    QTest::newRow("no-æ:ae-nodiacr")
            << u"no_NO"_s << u"æ"_s << u"ae"_s << Opts{Opt::DiacriticInsensitive} << 1;
    QTest::newRow("no-Æ:AE") << u"no_NO"_s << u"Æ"_s << u"AE"_s << Opts{} << 1;
    QTest::newRow("no-Æ:AE-nodiacr")
            << u"no_NO"_s << u"Æ"_s << u"AE"_s << Opts{Opt::DiacriticInsensitive} << 1;
    QTest::newRow("no-ü:u") << u"no_NO"_s << u"ü"_s << u"u"_s << Opts{} << 1;
    QTest::newRow("no-ü:u-nodiacr")
            << u"no_NO"_s << u"ü"_s << u"u"_s << Opts{Opt::DiacriticInsensitive} << 1;
    QTest::newRow("no-ü:y") << u"no_NO"_s << u"ü"_s << u"y"_s << Opts{} << 1;
    QTest::newRow("no-ü:y-nodiacr")
            << u"no_NO"_s << u"ü"_s << u"y"_s << Opts{Opt::DiacriticInsensitive} << 0;
    QTest::newRow("no-9:19-numsort")
            << u"no_NO"_s << u"9"_s << u"19"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("no-9:19-nocase")
            << u"no_NO"_s << u"9"_s << u"19"_s << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("no-9:_19-numsort")
            << u"no_NO"_s << u"Test 9"_s << u"Test_19"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("no-9:_19-nocase")
            << u"no_NO"_s << u"Test 9"_s << u"Test_19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("no-9:_19-nopun")
            << u"no_NO"_s << u"Test 9"_s << u"Test_19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << -1;
    QTest::newRow("no-_19:19-numsort")
            << u"no_NO"_s << u"test_19"_s << u"test 19"_s << Opts{Opt::NumericSort} << 1;
    QTest::newRow("no-_19:19-nocase")
            << u"no_NO"_s << u"test_19"_s << u"test 19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("no-.19:,19-numsort")
            << u"no_NO"_s << u"test.19"_s << u"test,19"_s << Opts{Opt::NumericSort} << 1;
    QTest::newRow("no-.19:,19-nocase")
            << u"no_NO"_s << u"test.19"_s << u"test,19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("no-.19:,19-nopun")
            << u"no_NO"_s << u"test.19"_s << u"test,19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << 0;
    QTest::newRow("nb-empty-word") << u"nb_NO"_s << QString() << u"mett"_s << Opts{} << -1;
    QTest::newRow("nb-empty-word-nocase")
            << u"nb_NO"_s << QString() << u"mett"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("nb-empty-word-nopun")
            << u"nb_NO"_s << QString() << u"mett"_s
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << -1;
    QTest::newRow("nb-empty-number-numsort")
            << u"nb_NO"_s << QString() << u"42"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("nb-empty-number-nocase")
            << u"nb_NO"_s << QString() << u"42"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("nb-empty-number-nopun")
            << u"nb_NO"_s << QString() << u"42"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << -1;
    QTest::newRow("nb-word-empty") << u"nb_NO"_s << u"mett"_s << QString() << Opts{} << 1;
    QTest::newRow("nb-word-empty-nocase")
            << u"nb_NO"_s << u"mett"_s << QString() << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("nb-word-empty-nopun")
            << u"nb_NO"_s << u"mett"_s << QString()
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 1;
    QTest::newRow("nb-number-empty-numsort")
            << u"nb_NO"_s << u"42"_s << QString() << Opts{Opt::NumericSort} << 1;
    QTest::newRow("nb-number-empty-nocase")
            << u"nb_NO"_s << u"42"_s << QString() << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("nb-number-empty-nopun")
            << u"nb_NO"_s << u"42"_s << QString()
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << 1;
    QTest::newRow("nb-empty-empty") << u"nb_NO"_s << QString() << QString() << Opts{} << 0;
    QTest::newRow("nb-empty-empty-nocase")
            << u"nb_NO"_s << QString() << QString() << Opts{Opt::CaseInsensitive} << 0;
    QTest::newRow("nb-empty-empty-nopun")
            << u"nb_NO"_s << QString() << QString()
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 0;

    /*
        In German, z comes *after* a with diaresis (ä),
        which comes before o diaresis (ö).
    */
    QTest::newRow("de-a:ä") << u"de_DE"_s << u"a"_s << u"ä"_s << Opts{} << -1;
    QTest::newRow("de-a:ä-nocase")
            << u"de_DE"_s << u"a"_s << u"ä"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("de-b:ä") << u"de_DE"_s << u"b"_s << u"ä"_s << Opts{} << 1;
    QTest::newRow("de-b:ä-nocase")
            << u"de_DE"_s << u"b"_s << u"ä"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("de-z:ä") << u"de_DE"_s << u"z"_s << u"ä"_s << Opts{} << 1;
    QTest::newRow("de-z:ä-nocase")
            << u"de_DE"_s << u"z"_s << u"ä"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("de-ä:ö") << u"de_DE"_s << u"ä"_s << u"ö"_s << Opts{} << -1;
    QTest::newRow("de-ä:ö-nocase")
            << u"de_DE"_s << u"ä"_s << u"ö"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("de-z:ö") << u"de_DE"_s << u"z"_s << u"ö"_s << Opts{} << 1;
    QTest::newRow("de-z:ö-nocase")
            << u"de_DE"_s << u"z"_s << u"ö"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("de-À:à") << u"de_DE"_s << u"À"_s << u"à"_s << Opts{} << 1;
    QTest::newRow("de-À:à-nocase")
            << u"de_DE"_s << u"À"_s << u"à"_s << Opts{Opt::CaseInsensitive} << 0;
    QTest::newRow("de-Ö:ö") << u"de_DE"_s << u"Ö"_s << u"ö"_s << Opts{} << 1;
    QTest::newRow("de-Ö:ö-nocase")
            << u"de_DE"_s << u"Ö"_s << u"ö"_s << Opts{Opt::CaseInsensitive} << 0;
    QTest::newRow("de-oe:ö") << u"de_DE"_s << u"oe"_s << u"ö"_s << Opts{} << 1;
    QTest::newRow("de-oe:ö-nocase")
            << u"de_DE"_s << u"oe"_s << u"ö"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("de-A:a") << u"de_DE"_s << u"A"_s << u"a"_s << Opts{} << 1;
    QTest::newRow("de-A:a-nocase")
            << u"de_DE"_s << u"A"_s << u"a"_s << Opts{Opt::CaseInsensitive} << 0;
    QTest::newRow("de-ö:o")
            << u"de_DE"_s << u"ö"_s << u"o"_s << Opts{} << 1;
    QTest::newRow("de-ö:o-nodiacr")
            << u"de_DE"_s << u"ö"_s << u"o"_s << Opts{Opt::DiacriticInsensitive} << 0;
    QTest::newRow("de-ö:O-nocase")
            << u"de_DE"_s << u"ö"_s << u"O"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("de-ö:O-nodiacr")
            << u"de_DE"_s << u"ö"_s << u"O"_s << Opts{Opt::DiacriticInsensitive} << -1;
    QTest::newRow("de-ö:O-nocase-nodiacr")
            << u"de_DE"_s << u"ö"_s << u"O"_s << (Opt::CaseInsensitive | Opt::DiacriticInsensitive)
            << 0;
    QTest::newRow("de-ä:a")
            << u"de_DE"_s << u"ä"_s << u"a"_s << Opts{} << 1;
    QTest::newRow("de-ä:a-nodiacr")
            << u"de_DE"_s << u"ä"_s << u"a"_s << Opts{Opt::DiacriticInsensitive} << 0;
    QTest::newRow("de-ä:A-nocase")
            << u"de_DE"_s << u"ä"_s << u"A"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("de-ä:A-nodiacr")
            << u"de_DE"_s << u"ä"_s << u"A"_s << Opts{Opt::DiacriticInsensitive} << -1;
    QTest::newRow("de-ä:a-nocase-nodiacr")
            << u"de_DE"_s << u"ä"_s << u"A"_s << (Opt::CaseInsensitive | Opt::DiacriticInsensitive)
            << 0;
    QTest::newRow("de-9:19-numsort")
            << u"de_DE"_s << u"9"_s << u"19"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("de-9:19-nocase")
            << u"de_DE"_s << u"9"_s << u"19"_s << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("de-9:_19-numsort")
            << u"de_DE"_s << u"Test 9"_s << u"Test_19"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("de-9:_19-nocase")
            << u"de_DE"_s << u"Test 9"_s << u"Test_19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("de-9:_19-nopun")
            << u"de_DE"_s << u"Test 9"_s << u"Test_19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << -1;
    QTest::newRow("de-_19:19-numsort")
            << u"de_DE"_s << u"test_19"_s << u"test 19"_s << Opts{Opt::NumericSort} << 1;
    QTest::newRow("de-_19:19-nocase")
            << u"de_DE"_s << u"test_19"_s << u"test 19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("de-.19:,19-numsort")
            << u"de_DE"_s << u"test.19"_s << u"test,19"_s << Opts{Opt::NumericSort} << 1;
    QTest::newRow("de-.19:,19-nocase")
            << u"de_DE"_s << u"test.19"_s << u"test,19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("de-.19:,19-nopun")
            << u"de_DE"_s << u"test.19"_s << u"test,19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << 0;
    QTest::newRow("de-empty-word") << u"de_DE"_s << QString() << u"satt"_s << Opts{} << -1;
    QTest::newRow("de-empty-word-nocase")
            << u"de_DE"_s << QString() << u"satt"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("de-empty-word-nopun")
            << u"de_DE"_s << QString() << u"satt"_s
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << -1;
    QTest::newRow("de-empty-number-numsort")
            << u"de_DE"_s << QString() << u"42"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("de-empty-number-nocase")
            << u"de_DE"_s << QString() << u"42"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("de-empty-number-nopun")
            << u"de_DE"_s << QString() << u"42"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << -1;
    QTest::newRow("de-word-empty") << u"de_DE"_s << u"satt"_s << QString() << Opts{} << 1;
    QTest::newRow("de-word-empty-nocase")
            << u"de_DE"_s << u"satt"_s << QString() << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("de-word-empty-nopun")
            << u"de_DE"_s << u"satt"_s << QString()
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 1;
    QTest::newRow("de-number-empty-numsort")
            << u"de_DE"_s << u"42"_s << QString() << Opts{Opt::NumericSort} << 1;
    QTest::newRow("de-number-empty-nocase")
            << u"de_DE"_s << u"42"_s << QString() << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("de-number-empty-nopun")
            << u"de_DE"_s << u"42"_s << QString()
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << 1;
    QTest::newRow("de-empty-empty") << u"de_DE"_s << QString() << QString() << Opts{} << 0;
    QTest::newRow("de-empty-empty-nocase")
            << u"de_DE"_s << QString() << QString() << Opts{Opt::CaseInsensitive} << 0;
    QTest::newRow("de-empty-empty-nopun")
            << u"de_DE"_s << QString() << QString()
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 0;

    /*
        French sorting of e and e with acute accent (é)
    */
    QTest::newRow("fr-é:e") << u"fr_FR"_s << u"é"_s << u"e"_s << Opts{} << 1;
    QTest::newRow("fr-é:e-nocase")
            << u"fr_FR"_s << u"é"_s << u"e"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("fr-é:E-nodiacr")
            << u"fr_FR"_s << u"é"_s << u"E"_s << Opts{Opt::DiacriticInsensitive}
            << -1;
    QTest::newRow("fr-é:e-nodiacr")
            << u"fr_FR"_s << u"é"_s << u"e"_s << Opts{Opt::DiacriticInsensitive} << 0;
    QTest::newRow("fr-é:E-nocase-nodiacr")
            << u"fr_FR"_s << u"é"_s << u"E"_s << (Opt::CaseInsensitive | Opt::DiacriticInsensitive)
            << 0;
    QTest::newRow("fr-ét:et") << u"fr_FR"_s << u"ét"_s << u"et"_s << Opts{} << 1;
    QTest::newRow("fr-ét:et-nocase")
            << u"fr_FR"_s << u"ét"_s << u"et"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("fr-é:d") << u"fr_FR"_s << u"é"_s << u"d"_s << Opts{} << 1;
    QTest::newRow("fr-é:d-nocase")
            << u"fr_FR"_s << u"é"_s << u"d"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("fr-é:f") << u"fr_FR"_s << u"é"_s << u"f"_s << Opts{} << -1;
    QTest::newRow("fr-é:f-nocase")
            << u"fr_FR"_s << u"é"_s << u"f"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("fr-9:19-numsort")
            << u"fr_FR"_s << u"9"_s << u"19"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("fr-9:19-nocase")
            << u"fr_FR"_s << u"9"_s << u"19"_s << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("fr-9:_19-numsort")
            << u"fr_FR"_s << u"Test 9"_s << u"Test_19"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("fr-9:_19-nocase")
            << u"fr_FR"_s << u"Test 9"_s << u"Test_19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("fr-9:_19-nopun")
            << u"fr_FR"_s << u"Test 9"_s << u"Test_19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << -1;
    QTest::newRow("fr-_19:19-numsort")
            << u"fr_FR"_s << u"test_19"_s << u"test 19"_s << Opts{Opt::NumericSort} << 1;
    QTest::newRow("fr-_19:19-nocase")
            << u"fr_FR"_s << u"test_19"_s << u"test 19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("fr-.19:,19-numsort")
            << u"fr_FR"_s << u"test.19"_s << u"test,19"_s << Opts{Opt::NumericSort} << 1;
    QTest::newRow("fr-.19:,19-nocase")
            << u"fr_FR"_s << u"test.19"_s << u"test,19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("fr-.19:,19-nopun")
            << u"fr_FR"_s << u"test.19"_s << u"test,19"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << 0;
    QTest::newRow("fr-empty-word") << u"fr_FR"_s << QString() << u"plein"_s << Opts{} << -1;
    QTest::newRow("fr-empty-word-nocase")
            << u"fr_FR"_s << QString() << u"plein"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("fr-empty-word-nopun")
            << u"fr_FR"_s << QString() << u"plein"_s
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << -1;
    QTest::newRow("fr-empty-number-numsort")
            << u"fr_FR"_s << QString() << u"42"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("fr-empty-number-nocase")
            << u"fr_FR"_s << QString() << u"42"_s
            << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("fr-empty-number-nopun")
            << u"fr_FR"_s << QString() << u"42"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << -1;
    QTest::newRow("fr-word-empty") << u"fr_FR"_s << u"plein"_s << QString() << Opts{} << 1;
    QTest::newRow("fr-word-empty-nocase")
            << u"fr_FR"_s << u"plein"_s << QString() << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("fr-word-empty-nopun")
            << u"fr_FR"_s << u"plein"_s << QString()
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 1;
    QTest::newRow("fr-number-empty-numsort")
            << u"fr_FR"_s << u"42"_s << QString() << Opts{Opt::NumericSort} << 1;
    QTest::newRow("fr-number-empty-nocase")
            << u"fr_FR"_s << u"42"_s << QString() << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("fr-number-empty-nopun")
            << u"fr_FR"_s << u"42"_s << QString()
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << 1;
    QTest::newRow("fr-empty-empty") << u"fr_FR"_s << QString() << QString() << Opts{} << 0;
    QTest::newRow("fr-empty-empty-nocase")
            << u"fr_FR"_s << QString() << QString() << Opts{Opt::CaseInsensitive} << 0;
    QTest::newRow("fr-empty-empty-nopun")
            << u"fr_FR"_s << QString() << QString()
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 0;

    // C locale: case sensitive [A-Z] < [a-z] but case insensitive [Aa] < [Bb] <...< [Zz]
    const QString C = u"C"_s;
    QTest::newRow("C:ABBA:AaaA") << C << u"ABBA"_s << u"AaaA"_s << Opts{} << -1;
    QTest::newRow("C:ABBA:AaaA-nocase")
            << C << u"ABBA"_s << u"AaaA"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("C:AZa:aAZ") << C << u"AZa"_s << u"aAZ"_s << Opts{} << -1;
    QTest::newRow("C:AZa:aAZ-nocase")
            << C << u"AZa"_s << u"aAZ"_s << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("C-é:e") << C << u"é"_s << u"e"_s << Opts{} << 1;
    QTest::newRow("C-é:e-nodiacr") << C << u"é"_s << u"e"_s << Opts{Opt::DiacriticInsensitive} << 1;
    QTest::newRow("C-9:10")
            << C << u"file9"_s << u"file10"_s << Opts{} << 1;
    QTest::newRow("C-9:10-numsort")
            << C << u"file9"_s << u"file10"_s << Opts{Opt::NumericSort} << 1;
    QTest::newRow("C-a_b:ab")
            << C << u"a_b"_s << u"ab"_s << Opts{} << -1;
    QTest::newRow("C-a_b:ab-nopun")
            << C << u"a_b"_s << u"ab"_s << Opts{Opt::IgnorePunctuation} << -1;
    QTest::newRow("C-empty-word") << C << QString() << u"non-empty"_s << Opts{} << -1;
    QTest::newRow("C-empty-word-nocase")
            << C << QString() << u"non-empty"_s << Opts{Opt::CaseInsensitive} << -1;
    QTest::newRow("C-empty-word-nopun")
            << C << QString() << u"non-empty"_s
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << -1;
    QTest::newRow("C-empty-number-numsort")
            << C << QString() << u"42"_s << Opts{Opt::NumericSort} << -1;
    QTest::newRow("C-empty-number-nocase")
            << C << QString() << u"42"_s << (Opt::CaseInsensitive | Opt::NumericSort) << -1;
    QTest::newRow("C-empty-number-nopun")
            << C << QString() << u"42"_s
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << -1;
    QTest::newRow("C-word-empty") << C << u"non-empty"_s << QString() << Opts{} << 1;
    QTest::newRow("C-word-empty-nocase")
            << C << u"non-empty"_s << QString() << Opts{Opt::CaseInsensitive} << 1;
    QTest::newRow("C-word-empty-nopun")
            << C << u"non-empty"_s << QString()
            << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 1;
    QTest::newRow("C-number-empty-numsort")
            << C << u"42"_s << QString() << Opts{Opt::NumericSort} << 1;
    QTest::newRow("C-number-empty-nocase")
            << C << u"42"_s << QString() << (Opt::CaseInsensitive | Opt::NumericSort) << 1;
    QTest::newRow("C-number-empty-nopun")
            << C << u"42"_s << QString()
            << (Opt::CaseInsensitive | Opt::NumericSort | Opt::IgnorePunctuation) << 1;
    QTest::newRow("C-empty-empty") << C << QString() << QString() << Opts{} << 0;
    QTest::newRow("C-empty-empty-nocase")
            << C << QString() << QString() << Opts{Opt::CaseInsensitive} << 0;
    QTest::newRow("C-empty-empty-nopun")
            << C << QString() << QString() << (Opt::CaseInsensitive | Opt::IgnorePunctuation) << 0;
}

void tst_QCollator::compare()
{
    QFETCH(QString, locale);
    QFETCH(QString, s1);
    QFETCH(QString, s2);
    QFETCH(Opts, options);
    QFETCH(int, result);

    QCollator collator((QLocale(locale)));
    collator.setOptions(options);

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
    const QByteArrayView tag = QTest::currentDataTag();
    if (tag.startsWith("en-9:19") || tag.startsWith("en-.19:,19"))
        QSKIP("Some en-us locale tests have issues on WASM");
#endif // Q_OS_WASM
#if !QT_CONFIG(icu) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    if (collator.locale() != QLocale::c() && collator.locale() != QLocale::system().collation())
        QSKIP("POSIX implementation of collation only supports C and system collation locales");
#elif QT_CONFIG(icu) || defined(Q_OS_WIN)
#  define SORTKEY_WORKS
#endif

    [[maybe_unused]] int keyCompareResult = result;

    // trying to deal with special behavior of different OS-dependent collators
    if (collator.locale() == QLocale("C")) {
#if !QT_CONFIG(icu) && defined(Q_OS_MACOS)
        // for MACOS C-locale is not supported, always providing empty string for sortKey()
        keyCompareResult = 0;
#else
        if (options.testFlag(Opt::CaseInsensitive)) {
            // C locale sort keys ignore CaseInsensitive
            collator.setOptions(options & ~Opts(Opt::CaseInsensitive));
            keyCompareResult = asSign(collator.compare(s1, s2));
            collator.setOptions(options);
        }
#endif
    }

    QCOMPARE(asSign(collator.compare(s1, s2)), result);
    if (!options)
        QCOMPARE(asSign(QCollator::defaultCompare(s1, s2)), result);
#ifdef SORTKEY_WORKS
    auto key1 = collator.sortKey(s1);
    auto key2 = collator.sortKey(s2);
    QCOMPARE(asSign(key1.compare(key2)), keyCompareResult);

    if (!options) {
        key1 = QCollator::defaultSortKey(s1);
        key2 = QCollator::defaultSortKey(s2);
        QCOMPARE(asSign(key1.compare(key2)), keyCompareResult);
    }
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
