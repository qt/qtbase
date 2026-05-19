// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QLocale>
#include <QTest>

using namespace Qt::StringLiterals;

class tst_QLocale : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fromString_data();
    void fromString();
    void fromTags_data();
    void fromTags();
    void fromLangScript_data();
    void fromLangScript();
    void fromLangLand_data();
    void fromLangLand();
    void fromScriptLand_data();
    void fromScriptLand();
    void fromLang_data();
    void fromLang();
    void fromScript_data();
    void fromScript();
    void fromLand_data();
    void fromLand();
    void toUpper_QLocale_1();
    void toUpper_QLocale_2();
    void toUpper_QString();
    void number_QString();
    void toLongLong_data();
    void toLongLong();
    void toULongLong_data();
    void toULongLong();
    void toDouble_data();
    void toDouble();
    void toDate_data();
    void toDate();
    void toTime_data();
    void toTime();
    void toDateTime_data();
    void toDateTime();
};

static QString data()
{
    return QStringLiteral("/qt-5/qtbase/tests/benchmarks/corelib/tools/qlocale");
}

// Make individual cycles O(a few) msecs, rather than tiny fractions thereof:
#define LOOP(s) for (int i = 0; i < 5000; ++i) { s; }

void tst_QLocale::fromString_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("C") << QStringLiteral("C");
#define ROW(name) QTest::newRow(name) << QStringLiteral(name)
    ROW("en-Latn-DE");
    ROW("sd-Deva-IN");
    ROW("az-Cyrl-AZ");
    ROW("az-Latn-AZ");
    ROW("bs-Cyrl-BA");
    ROW("bs-Latn-BA");
    ROW("ff-Latn-LR");
    ROW("ff-Latn-MR");
    ROW("pa-Arab-PK");
    ROW("pa-Guru-IN");
    ROW("shi-Latn-MA");
    ROW("shi-Tfng-MA");
    ROW("sr-Cyrl-BA");
    ROW("sr-Cyrl-RS");
    ROW("sr-Latn-BA");
    ROW("sr-Latn-ME");
    ROW("uz-Arab-AF");
    ROW("uz-Cyrl-UZ");
    ROW("uz-Latn-UZ");
    ROW("vai-Latn-LR");
    ROW("vai-Vaii-LR");
    ROW("yue-Hans-CN");
    ROW("yue-Hant-HK");
    ROW("zh-Hans-CN");
    ROW("zh-Hans-HK");
    ROW("zh-Hans-SG");
    ROW("zh-Hant-HK");
    ROW("zh-Hant-TW");
#undef ROW
}

void tst_QLocale::fromString()
{
    QFETCH(const QString, name);
    QBENCHMARK { LOOP(QLocale loc(name)) }
}

void tst_QLocale::fromTags_data()
{
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<QLocale::Script>("script");
    QTest::addColumn<QLocale::Territory>("territory");

#define ROW(name, lang, text, land) \
        QTest::newRow(name) << QLocale::lang << QLocale::text << QLocale::land
    ROW("C", C, AnyScript, AnyTerritory);
    ROW("en-Latn-DE", English, LatinScript, Germany);
    ROW("sd-Deva-IN", Sindhi, DevanagariScript, India);
    ROW("az-Cyrl-AZ", Azerbaijani, CyrillicScript, Azerbaijan);
    ROW("az-Latn-AZ", Azerbaijani, LatinScript, Azerbaijan);
    ROW("bs-Cyrl-BA", Bosnian, CyrillicScript, BosniaAndHerzegowina);
    ROW("bs-Latn-BA", Bosnian, LatinScript, BosniaAndHerzegowina);
    ROW("ff-Latn-LR", Fulah, LatinScript, Liberia);
    ROW("ff-Latn-MR", Fulah, LatinScript, Mauritania);
    ROW("pa-Arab-PK", Punjabi, ArabicScript, Pakistan);
    ROW("pa-Guru-IN", Punjabi, GurmukhiScript, India);
    ROW("shi-Latn-MA", Tachelhit, LatinScript, Morocco);
    ROW("shi-Tfng-MA", Tachelhit, TifinaghScript, Morocco);
    ROW("sr-Cyrl-BA", Serbian, CyrillicScript, BosniaAndHerzegowina);
    ROW("sr-Cyrl-RS", Serbian, CyrillicScript, Serbia);
    ROW("sr-Latn-BA", Serbian, LatinScript, BosniaAndHerzegowina);
    ROW("sr-Latn-ME", Serbian, LatinScript, Montenegro);
    ROW("uz-Arab-AF", Uzbek, ArabicScript, Afghanistan);
    ROW("uz-Cyrl-UZ", Uzbek, CyrillicScript, Uzbekistan);
    ROW("uz-Latn-UZ", Uzbek, LatinScript, Uzbekistan);
    ROW("vai-Latn-LR", Vai, LatinScript, Liberia);
    ROW("vai-Vaii-LR", Vai, VaiScript, Liberia);
    ROW("yue-Hans-CN", Cantonese, SimplifiedHanScript, China);
    ROW("yue-Hant-HK", Cantonese, TraditionalHanScript, HongKong);
    ROW("zh-Hans-CN", Chinese, SimplifiedHanScript, China);
    ROW("zh-Hans-HK", Chinese, SimplifiedHanScript, HongKong);
    ROW("zh-Hans-SG", Chinese, SimplifiedHanScript, Singapore);
    ROW("zh-Hant-HK", Chinese, TraditionalHanScript, HongKong);
    ROW("zh-Hant-TW", Chinese, TraditionalHanScript, Taiwan);
#undef ROW
}

void tst_QLocale::fromTags()
{
    QFETCH(const QLocale::Language, language);
    QFETCH(const QLocale::Script, script);
    QFETCH(const QLocale::Territory, territory);
    QBENCHMARK { LOOP(QLocale loc(language, script, territory)) }
}

void tst_QLocale::fromLangScript_data()
{
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<QLocale::Script>("script");

#define ROW(name, lang, text) \
        QTest::newRow(name) << QLocale::lang << QLocale::text
    ROW("C", C, AnyScript);
    ROW("en-Latn", English, LatinScript);
    ROW("sd-Deva", Sindhi, DevanagariScript);
    ROW("az-Cyrl", Azerbaijani, CyrillicScript);
    ROW("az-Latn", Azerbaijani, LatinScript);
    ROW("bs-Cyrl", Bosnian, CyrillicScript);
    ROW("bs-Latn", Bosnian, LatinScript);
    ROW("ff-Latn", Fulah, LatinScript);
    ROW("pa-Arab", Punjabi, ArabicScript);
    ROW("pa-Guru", Punjabi, GurmukhiScript);
    ROW("shi-Latn", Tachelhit, LatinScript);
    ROW("shi-Tfng", Tachelhit, TifinaghScript);
    ROW("sr-Cyrl", Serbian, CyrillicScript);
    ROW("sr-Latn", Serbian, LatinScript);
    ROW("uz-Arab", Uzbek, ArabicScript);
    ROW("uz-Cyrl", Uzbek, CyrillicScript);
    ROW("uz-Latn", Uzbek, LatinScript);
    ROW("vai-Latn", Vai, LatinScript);
    ROW("vai-Vaii", Vai, VaiScript);
    ROW("yue-Hans", Cantonese, SimplifiedHanScript);
    ROW("yue-Hant", Cantonese, TraditionalHanScript);
    ROW("zh-Hans", Chinese, SimplifiedHanScript);
    ROW("zh-Hant", Chinese, TraditionalHanScript);
#undef ROW
}

void tst_QLocale::fromLangScript()
{
    QFETCH(const QLocale::Language, language);
    QFETCH(const QLocale::Script, script);
    QBENCHMARK { LOOP(QLocale loc(language, script, QLocale::AnyTerritory)) }
}

void tst_QLocale::fromLangLand_data()
{
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<QLocale::Territory>("territory");

#define ROW(name, lang, land) \
        QTest::newRow(name) << QLocale::lang << QLocale::land
    ROW("C", C, AnyTerritory);
    ROW("en-DE", English, Germany);
    ROW("sd-IN", Sindhi, India);
    ROW("az-AZ", Azerbaijani, Azerbaijan);
    ROW("bs-BA", Bosnian, BosniaAndHerzegowina);
    ROW("ff-LR", Fulah, Liberia);
    ROW("ff-MR", Fulah, Mauritania);
    ROW("pa-PK", Punjabi, Pakistan);
    ROW("pa-IN", Punjabi, India);
    ROW("shi-MA", Tachelhit, Morocco);
    ROW("sr-BA", Serbian, BosniaAndHerzegowina);
    ROW("sr-RS", Serbian, Serbia);
    ROW("sr-ME", Serbian, Montenegro);
    ROW("uz-AF", Uzbek, Afghanistan);
    ROW("uz-UZ", Uzbek, Uzbekistan);
    ROW("vai-LR", Vai, Liberia);
    ROW("yue-CN", Cantonese, China);
    ROW("yue-HK", Cantonese, HongKong);
    ROW("zh-CN", Chinese, China);
    ROW("zh-HK", Chinese, HongKong);
    ROW("zh-SG", Chinese, Singapore);
    ROW("zh-TW", Chinese, Taiwan);
#undef ROW
}

void tst_QLocale::fromLangLand()
{
    QFETCH(const QLocale::Language, language);
    QFETCH(const QLocale::Territory, territory);
    QBENCHMARK { LOOP(QLocale loc(language, territory)) }
}

void tst_QLocale::fromScriptLand_data()
{
    QTest::addColumn<QLocale::Script>("script");
    QTest::addColumn<QLocale::Territory>("territory");

#define ROW(name, text, land) \
        QTest::newRow(name) << QLocale::text << QLocale::land
    ROW("Any", AnyScript, AnyTerritory);
    ROW("Latn-DE", LatinScript, Germany);
    ROW("Deva-IN", DevanagariScript, India);
    ROW("Cyrl-AZ", CyrillicScript, Azerbaijan);
    ROW("Latn-AZ", LatinScript, Azerbaijan);
    ROW("Cyrl-BA", CyrillicScript, BosniaAndHerzegowina);
    ROW("Latn-BA", LatinScript, BosniaAndHerzegowina);
    ROW("Latn-LR", LatinScript, Liberia);
    ROW("Latn-MR", LatinScript, Mauritania);
    ROW("Arab-PK", ArabicScript, Pakistan);
    ROW("Guru-IN", GurmukhiScript, India);
    ROW("Latn-MA", LatinScript, Morocco);
    ROW("Tfng-MA", TifinaghScript, Morocco);
    ROW("Cyrl-RS", CyrillicScript, Serbia);
    ROW("Latn-ME", LatinScript, Montenegro);
    ROW("Arab-AF", ArabicScript, Afghanistan);
    ROW("Cyrl-UZ", CyrillicScript, Uzbekistan);
    ROW("Latn-UZ", LatinScript, Uzbekistan);
    ROW("Vaii-LR", VaiScript, Liberia);
    ROW("Hans-CN", SimplifiedHanScript, China);
    ROW("Hant-HK", TraditionalHanScript, HongKong);
    ROW("Hans-HK", SimplifiedHanScript, HongKong);
    ROW("Hans-SG", SimplifiedHanScript, Singapore);
    ROW("Hant-TW", TraditionalHanScript, Taiwan);
#undef ROW
}

void tst_QLocale::fromScriptLand()
{
    QFETCH(const QLocale::Script, script);
    QFETCH(const QLocale::Territory, territory);
    QBENCHMARK { LOOP(QLocale loc(QLocale::AnyLanguage, script, territory)) }
}

void tst_QLocale::fromLang_data()
{
    QTest::addColumn<QLocale::Language>("language");

#define ROW(name, lang) \
        QTest::newRow(name) << QLocale::lang
    ROW("C", C);
    ROW("en", English);
    ROW("sd", Sindhi);
    ROW("az", Azerbaijani);
    ROW("bs", Bosnian);
    ROW("ff", Fulah);
    ROW("pa", Punjabi);
    ROW("shi", Tachelhit);
    ROW("sr", Serbian);
    ROW("uz", Uzbek);
    ROW("vai", Vai);
    ROW("yue", Cantonese);
    ROW("zh", Chinese);
#undef ROW
}

void tst_QLocale::fromLang()
{
    QFETCH(const QLocale::Language, language);
    QBENCHMARK { LOOP(QLocale loc(language)) }
}

void tst_QLocale::fromScript_data()
{
    QTest::addColumn<QLocale::Script>("script");

#define ROW(name, text) \
        QTest::newRow(name) << QLocale::text
    ROW("Any", AnyScript);
    ROW("Latn", LatinScript);
    ROW("Deva", DevanagariScript);
    ROW("Cyrl", CyrillicScript);
    ROW("Arab", ArabicScript);
    ROW("Guru", GurmukhiScript);
    ROW("Tfng", TifinaghScript);
    ROW("Vaii", VaiScript);
    ROW("Hans", SimplifiedHanScript);
    ROW("Hant", TraditionalHanScript);
#undef ROW
}

void tst_QLocale::fromScript()
{
    QFETCH(const QLocale::Script, script);
    QBENCHMARK { LOOP(QLocale loc(QLocale::AnyLanguage, script, QLocale::AnyTerritory)) }
}

void tst_QLocale::fromLand_data()
{
    QTest::addColumn<QLocale::Territory>("territory");

#define ROW(name, land) \
        QTest::newRow(name) << QLocale::land
    ROW("Any", AnyTerritory);
    ROW("DE", Germany);
    ROW("IN", India);
    ROW("AZ", Azerbaijan);
    ROW("BA", BosniaAndHerzegowina);
    ROW("LR", Liberia);
    ROW("MR", Mauritania);
    ROW("PK", Pakistan);
    ROW("MA", Morocco);
    ROW("RS", Serbia);
    ROW("ME", Montenegro);
    ROW("AF", Afghanistan);
    ROW("UZ", Uzbekistan);
    ROW("CN", China);
    ROW("HK", HongKong);
    ROW("SG", Singapore);
    ROW("TW", Taiwan);
#undef ROW
}

void tst_QLocale::fromLand()
{
    QFETCH(const QLocale::Territory, territory);
    QBENCHMARK { LOOP(QLocale loc(QLocale::AnyLanguage, territory)) }
}

void tst_QLocale::toUpper_QLocale_1()
{
    QString s = data();
    QBENCHMARK { LOOP(QString t(QLocale().toUpper(s))) }
}

void tst_QLocale::toUpper_QLocale_2()
{
    QString s = data();
    QLocale l;
    QBENCHMARK { LOOP(QString t(l.toUpper(s))) }
}

void tst_QLocale::toUpper_QString()
{
    QString s = data();
    QBENCHMARK { LOOP(QString t(s.toUpper())) }
}

void tst_QLocale::number_QString()
{
    QString s;
    QBENCHMARK {
        s = QString::number(12345678);
    }
}

template <typename Integer>
void toWholeCommon_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("locale");
    QTest::addColumn<bool>("good");
    QTest::addColumn<Integer>("expected");

    QTest::newRow("C: empty") << u""_s << u"C"_s << false << Integer(0ull);
    QTest::newRow("C: spaces") << u" \t\n\r\f\v\u00A0\u202F "_s << u"C"_s << false << Integer(0ull);
    QTest::newRow("C: 0") << u"0"_s << u"C"_s << true << Integer(0ull);
    QTest::newRow("C:  0 ") << u" 0 "_s << u"C"_s << true << Integer(0ull);
    QTest::newRow("C: spaces0spaces") << u" \t\n\r\f\v\u00A0\u202F 0 \t\n\r\f\v\u00A0\u202F "_s
                                      << u"C"_s << true << Integer(0ull);
    QTest::newRow("C: 1234") << u"1234"_s << u"C"_s << true << Integer(1234ull);
    QTest::newRow("C:     1234") << u"    1234"_s << u"C"_s << true << Integer(1234ull);
    QTest::newRow("C: 1234    ") << u"1234    "_s << u"C"_s << true << Integer(1234ull);
    // C locale omits grouping, but doesn't reject it.
    QTest::newRow("C: 1,234") << u"1,234"_s << u"C"_s << true << Integer(1234ull);
    QTest::newRow("C: 123456789")
            << u"123456789"_s << u"C"_s << true << Integer(123456789ull);
    QTest::newRow("C: 123,456,789")
            << u"123,456,789"_s << u"C"_s << true << Integer(123456789ull);

    QTest::newRow("en: empty") << u""_s << u"en"_s << false << Integer(0ull);
    QTest::newRow("en: spaces") << u" \t\n\r\f\v "_s << u"en"_s << false << Integer(0ull);
    QTest::newRow("en: 0") << u"0"_s << u"en"_s << true << Integer(0ull);
    QTest::newRow("en:  0") << u" 0"_s << u"en"_s << true << Integer(0ull);
    QTest::newRow("en: 0 ") << u"0 "_s << u"en"_s << true << Integer(0ull);
    QTest::newRow("en: spaces0spaces") << u" \t\n\r\f\v\u00A0\u202F 0 \t\n\r\f\v\u00A0\u202F "_s
                                       << u"en"_s << true << Integer(0ull);
    QTest::newRow("en: 1234") << u"1234"_s << u"en"_s << true << Integer(1234ull);
    QTest::newRow("en:     1234") << u"    1234"_s << u"en"_s << true << Integer(1234ull);
    QTest::newRow("en: 1234    ") << u"1234    "_s << u"en"_s << true << Integer(1234ull);
    QTest::newRow("en: 1,234") << u"1,234"_s << u"en"_s << true << Integer(1234ull);
    QTest::newRow("en: 123,456,789")
            << u"123,456,789"_s << u"en"_s << true << Integer(123456789ull);
    QTest::newRow("en: 123456789")
            << u"123456789"_s << u"en"_s << true << Integer(123456789ull);

    QTest::newRow("de: empty") << u""_s << u"de"_s << false << Integer(0ull);
    QTest::newRow("de: spaces") << u" \t\n\r\f\v "_s << u"de"_s << false << Integer(0ull);
    QTest::newRow("de: 0") << u"0"_s << u"de"_s << true << Integer(0ull);
    QTest::newRow("de:  0") << u" 0"_s << u"de"_s << true << Integer(0ull);
    QTest::newRow("de: 0 ") << u"0 "_s << u"de"_s << true << Integer(0ull);
    QTest::newRow("de: spaces0spaces") << u" \t\n\r\f\v\u00A0\u202F 0 \t\n\r\f\v\u00A0\u202F"_s
                                       << u"de"_s << true << Integer(0ull);
    QTest::newRow("de: 1234") << u"1234"_s << u"de"_s << true << Integer(1234ull);
    QTest::newRow("de:     1234") << u"    1234"_s << u"de"_s << true << Integer(1234ull);
    QTest::newRow("de: 1234    ") << u"1234    "_s << u"de"_s << true << Integer(1234ull);
    QTest::newRow("de: 1.234") << u"1.234"_s << u"de"_s << true << Integer(1234ull);
    QTest::newRow("de: 123.456.789")
            << u"123.456.789"_s << u"de"_s << true << Integer(123456789ull);
    QTest::newRow("de: 123456789")
            << u"123456789"_s << u"de"_s << true << Integer(123456789ull);

    // Locales with non-single-character signs:
    QTest::newRow("ar_EG: +403") // Arabic, Egypt
            << u"\u061c+\u0664\u0660\u0663"_s << u"ar_EG"_s << true << Integer(403ull);
    QTest::newRow("ar_EG: !403") // Arabic Letter Mark in place of sign
            << u"\u061c\u0664\u0660\u0663"_s << u"ar_EG"_s << true << Integer(403ull);
    QTest::newRow("fa_IR: +403") // Farsi, Iran
            << u"\u200e+\u06f4\u06f0\u06f3"_s << u"fa_IR"_s << true << Integer(403ull);
    QTest::newRow("fa_IR: !403") // L-to-R mark in place of sign
            << u"\u200e\u06f4\u06f0\u06f3"_s << u"fa_IR"_s << true << Integer(403ull);
}

void tst_QLocale::toLongLong_data()
{
    toWholeCommon_data<qlonglong>();

    QTest::newRow("C: -1234") << u"-1234"_s << u"C"_s << true << -1234ll;
    QTest::newRow("C:     -1234") << u"    -1234"_s << u"C"_s << true << -1234ll;
    QTest::newRow("C: -1234    ") << u"-1234    "_s << u"C"_s << true << -1234ll;
    QTest::newRow("C: -123456789") << u"-123456789"_s << u"C"_s << true << -123456789ll;
    QTest::newRow("C: qlonglong-max")
            << u"9223372036854775807"_s << u"C"_s << true
            << std::numeric_limits<qlonglong>::max();
    QTest::newRow("C: qlonglong-min")
            << u"-9223372036854775808"_s << u"C"_s << true
            << std::numeric_limits<qlonglong>::min();

    // Locales with multi-character signs:
    QTest::newRow("ar_EG: -403") // Arabic, Egypt
            << u"\u061c-\u0664\u0660\u0663"_s << u"ar_EG"_s << true << -403ll;
    QTest::newRow("fa_IR: -403") // Farsi, Iran
            << u"\u200e\u2212\u06f4\u06f0\u06f3"_s << u"fa_IR"_s << true << -403ll;
}

void tst_QLocale::toLongLong()
{
    QFETCH(QString, text);
    QFETCH(QString, locale);
    QFETCH(bool, good);
    QFETCH(qlonglong, expected);

    const QLocale loc(locale);
    qlonglong actual = expected;
    bool ok = false;
    QBENCHMARK {
        actual = loc.toLongLong(text, &ok);
    }
    QCOMPARE(ok, good);
    QCOMPARE(actual, expected);
}

void tst_QLocale::toULongLong_data()
{
    toWholeCommon_data<qulonglong>();

    QTest::newRow("C: qlonglong-max + 1")
            << u"9223372036854775808"_s << u"C"_s << true
            << (qulonglong(std::numeric_limits<qlonglong>::max()) + 1);
    QTest::newRow("C: qulonglong-max")
            << u"18446744073709551615"_s << u"C"_s << true
            << std::numeric_limits<qulonglong>::max();
}

void tst_QLocale::toULongLong()
{
    QFETCH(QString, text);
    QFETCH(QString, locale);
    QFETCH(bool, good);
    QFETCH(qulonglong, expected);

    const QLocale loc(locale);
    qulonglong actual = expected;
    bool ok = false;
    QBENCHMARK {
        actual = loc.toULongLong(text, &ok);
    }
    QCOMPARE(ok, good);
    QCOMPARE(actual, expected);
}


void tst_QLocale::toDouble_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("locale");
    QTest::addColumn<bool>("good");
    QTest::addColumn<double>("expected");

    QTest::newRow("C: empty") << u""_s << u"C"_s << false << 0.0;
    QTest::newRow("C: spaces") << u" \t\n\r\f\v "_s << u"C"_s << false << 0.0;
    QTest::newRow("C: 0") << u"0"_s << u"C"_s << true << 0.0;
    QTest::newRow("C:  0") << u" 0"_s << u"C"_s << true << 0.0;
    QTest::newRow("C: 0 ") << u"0 "_s << u"C"_s << true << 0.0;
    QTest::newRow("C: spaces0spaces") << u" \t\n\r\f\v\u00A0\u202F 0 \t\n\r\f\v\u00A0\u202F"_s
                                      << u"C"_s << true << 0.0;
    QTest::newRow("C: 0.12340") << u"0.12340"_s << u"C"_s << true << 0.12340;
    QTest::newRow("C:     0.12340") << u"    0.12340"_s << u"C"_s << true << 0.12340;
    QTest::newRow("C: 0.12340    ") << u"0.12340    "_s << u"C"_s << true << 0.12340;
    QTest::newRow("C: -0.12340") << u"-0.12340"_s << u"C"_s << true << -0.12340;
    QTest::newRow("C:     -0.12340") << u"    -0.12340"_s << u"C"_s << true << -0.12340;
    QTest::newRow("C: -0.12340    ") << u"-0.12340    "_s << u"C"_s << true << -0.12340;
    QTest::newRow("C: &minus;0.12340") << u"\u2212" "0.12340"_s << u"C"_s << true << -0.12340;
    QTest::newRow("C: 1.0e-4") << u"1.0e-4"_s << u"C"_s << true << 1.0e-4;
    QTest::newRow("C: 1.0e&minus;4") << u"1.0e\u2212" "4"_s << u"C"_s << true << 1.0e-4;
    QTest::newRow("C: 1.0e+4") << u"1.0e+4"_s << u"C"_s << true << 1.0e+4;
    QTest::newRow("C: 10.e+3") << u"10.e+3"_s << u"C"_s << true << 1.0e+4;
    QTest::newRow("C: 10e+3.") << u"10e+3."_s << u"C"_s << false << 0.0; // exp...dot
    QTest::newRow("C: 1e4") << u"1e4"_s << u"C"_s << true << 1.0e+4;

    // NaN and infinity:
    QTest::newRow("C: nan") << u"nan"_s << u"C"_s << true << qQNaN();
    QTest::newRow("C: NaN") << u"NaN"_s << u"C"_s << true << qQNaN();
    QTest::newRow("C: -nan") << u"-nan"_s << u"C"_s << false << 0.0;
    QTest::newRow("C: +nan") << u"+nan"_s << u"C"_s << false << 0.0;
    QTest::newRow("C: inf") << u"inf"_s << u"C"_s << true << qInf();
    QTest::newRow("C: Inf") << u"Inf"_s << u"C"_s << true << qInf();
    QTest::newRow("C: +inf") << u"+inf"_s << u"C"_s << true << qInf();
    QTest::newRow("C: -inf") << u"-inf"_s << u"C"_s << true << -qInf();

    // Wantonly long-form representations, with trailing and leading zeros:
    QTest::newRow("C: 1e-64 long-form")
            << (u"0."_s + QString(63, u'0') + u'1' + QString(962, u'0')) << u"C"_s << true << 1e-64;
    QTest::newRow("C: 1e+64 long-form")
            << (QString(961, u'0') + u'1' + QString(64, u'0') + u".0"_s) << u"C"_s << true << 1e+64;
    QTest::newRow("C: long-form 1 via e+64")
            << (u"0."_s + QString(63, u'0') + u'1' + QString(962, u'0') + u"e+64"_s)
            << u"C"_s << true << 1.0;
    QTest::newRow("C: long-form 1 via e-64")
            << (QString(961, u'0') + u'1' + QString(64, u'0') + u".0e-64"_s)
            << u"C"_s << true << 1.0;
    QTest::newRow("C: 12345678.9") << u"12345678.9"_s << u"C"_s << true << 12345678.9;

    // With and without grouping, en vs de for flipped separators:
    QTest::newRow("en: 12345678.9") << u"12345678.9"_s << u"en"_s << true << 12345678.9;
    QTest::newRow("en: 12,345,678.9") << u"12,345,678.9"_s << u"en"_s << true << 12'345'678.9;
    QTest::newRow("de: 12345678,9") << u"12345678,9"_s << u"de"_s << true << 12345678.9;
    QTest::newRow("de: 12.345.678,9") << u"12.345.678,9"_s << u"de"_s << true << 12'345'678.9;

    // NaN and infinity are locale-independent (for now - QTBUG-95460)
    QTest::newRow("cy: nan") << u"nan"_s << u"cy"_s << true << qQNaN();
    QTest::newRow("cy: NaN") << u"NaN"_s << u"cy"_s << true << qQNaN();
    QTest::newRow("cy: -nan") << u"-nan"_s << u"cy"_s << false << 0.0;
    QTest::newRow("cy: +nan") << u"+nan"_s << u"cy"_s << false << 0.0;
    QTest::newRow("cy: inf") << u"inf"_s << u"cy"_s << true << qInf();
    QTest::newRow("cy: Inf") << u"Inf"_s << u"cy"_s << true << qInf();
    QTest::newRow("cy: +inf") << u"+inf"_s << u"cy"_s << true << qInf();
    QTest::newRow("cy: -inf") << u"-inf"_s << u"cy"_s << true << -qInf();
    // Samples ready for QTBUG-95460:
    QTest::newRow("en: &infin;") << u"\u221e"_s << u"en"_s << true << qInf();
    QTest::newRow("ga: Nuimh") << u"Nuimh"_s << u"ga"_s << true << qQNaN();

    // Locales with multi-character exponents:
    QTest::newRow("sv_SE: 4e-3") // Swedish, Sweden
            << u"4\u00d7" "10^\u2212" "03"_s << u"sv_SE"_s << true << 4e-3;
    QTest::newRow("sv_SE: 4x-3") // Only first character of exponent
            << u"4\u00d7\u2212" "03"_s << u"sv_SE"_s << false << 0.0;
    QTest::newRow("se_NO: 4e-3") // Northern Sami, Norway
            << u"4\u00b7" "10^\u2212" "03"_s << u"se_NO"_s << true << 4e-3;
    QTest::newRow("se_NO: 4x-3") // Only first character of exponent
            << u"4\u00b7\u2212" "03"_s << u"se_NO"_s << false << 0.0;
    QTest::newRow("ar_EG: 4e-3") // Arabic, Egypt
            << u"\u0664\u0623\u0633\u061c-\u0660\u0663"_s << u"ar_EG"_s << true << 4e-3;
    QTest::newRow("ar_EG: 4x-3") // Only first character of exponent
            << u"\u0664\u0623\u061c-\u0660\u0663"_s << u"ar_EG"_s << false << 0.0;
    QTest::newRow("ar_EG: 4e!3") // Arabic Letter Mark in place of sign
            << u"\u0664\u0623\u0633\u061c\u0660\u0663"_s << u"ar_EG"_s << true << 4e3;
    QTest::newRow("ar_EG: 4x!3") // Only first character of sign and exponent
            << u"\u0664\u0623\u061c\u0660\u0663"_s << u"ar_EG"_s << false << 0.0;
}

void tst_QLocale::toDouble()
{
    QFETCH(QString, text);
    QFETCH(QString, locale);
    QFETCH(bool, good);
    QFETCH(double, expected);

    const QLocale loc(locale);
    double actual = expected;
    bool ok = false;
    QBENCHMARK {
        actual = loc.toDouble(text, &ok);
    }
    QEXPECT_FAIL("en: &infin;", "Localized infinity support missing: QTBUG-95460", Abort);
    QEXPECT_FAIL("ga: Nuimh", "Localized NaN support missing: QTBUG-95460", Abort);
    QCOMPARE(ok, good);
    QCOMPARE(actual, expected);
}

void tst_QLocale::toDate_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QCalendar>("calendar");
    QTest::addColumn<int>("baseYear");
    QTest::addColumn<QString>("locale");

    const QCalendar greg(QCalendar::System::Gregorian);

    QTest::newRow("d.M.yy+Gregorian+C")
        << "1.9.24" << "d.M.yy" << greg << 2000 << "C";
    QTest::newRow("dd/MM/yyyy+Gregorian+C")
        << "01/09/2024" << "dd/MM/yyyy" << greg << 2000 << "C";
    QTest::newRow("ddd d-MMM-yy+Gregorian+C")
        << "Sun 1-Sep-24" << "ddd d-MMM-yy" << greg << 2000 << "C";
    QTest::newRow("dddd dd MMMM yyyy+Gregorian+C")
        << "Sunday 01 September 2024" << "dddd dd MMMM yyyy" << greg << 2000 << "C";
    QTest::newRow("yy.M.d+Gregorian+C")
        << "24.9.1" << "yy.M.d" << greg << 2000 << "C";
    QTest::newRow("yyyy/MM/dd+Gregorian+C")
        << "2024/09/01" << "yyyy/MM/dd" << greg << 2000 << "C";
    QTest::newRow("ddd yy-MMM-d+Gregorian+C")
        << "Sun 24-Sep-1" << "ddd yy-MMM-d" << greg << 2000 << "C";
    QTest::newRow("dddd yyyy MMMM dd+Gregorian+C")
        << "Sunday 2024 September 01" << "dddd yyyy MMMM dd" << greg << 2000 << "C";

    QTest::newRow("d.M.yy+Gregorian+de-DE")
        << "1.9.24" << "d.M.yy" << greg << 2000 << "de-DE";
    QTest::newRow("dd/MM/yyyy+Gregorian+de-DE")
        << "01/09/2024" << "dd/MM/yyyy" << greg << 2000 << "de-DE";
    QTest::newRow("ddd d-MMM-yy+Gregorian+de-DE")
        << "So. 1-Sept.-24" << "ddd d-MMM-yy" << greg << 2000 << "de-DE";
    QTest::newRow("dddd dd MMMM yyyy+Gregorian+de-DE")
        << "Sonntag 01 September 2024" << "dddd dd MMMM yyyy" << greg << 2000 << "de-DE";
    QTest::newRow("yy.M.d+Gregorian+de-DE")
        << "24.9.1" << "yy.M.d" << greg << 2000 << "de-DE";
    QTest::newRow("yyyy/MM/dd+Gregorian+de-DE")
        << "2024/09/01" << "yyyy/MM/dd" << greg << 2000 << "de-DE";
    QTest::newRow("ddd yy-MMM-d+Gregorian+de-DE")
        << "So. 24-Sept.-1" << "ddd yy-MMM-d" << greg << 2000 << "de-DE";
    QTest::newRow("dddd yyyy MMMM dd+Gregorian+de-DE")
        << "Sonntag 2024 September 01" << "dddd yyyy MMMM dd" << greg << 2000 << "de-DE";

#if QT_CONFIG(islamiccivilcalendar)
    const QCalendar isci(QCalendar::System::IslamicCivil);

    QTest::newRow("d.M.yy+IslamicCivil+C")
        << "1.9.24" << "d.M.yy" << isci << 2000 << "C";
    QTest::newRow("dd/MM/yyyy+IslamicCivil+C")
        << "01/09/2024" << "dd/MM/yyyy" << isci << 2000 << "C";
    QTest::newRow("ddd d-MMM-yy+IslamicCivil+C")
        << "Wed 1-Ram.-24" << "ddd d-MMM-yy" << isci << 2000 << "C";
    QTest::newRow("dddd dd MMMM yyyy+IslamicCivil+C")
        << "Wednesday 01 Ramadan 2024" << "dddd dd MMMM yyyy" << isci << 2000 << "C";
    QTest::newRow("yy.M.d+IslamicCivil+C")
        << "24.9.1" << "yy.M.d" << isci << 2000 << "C";
    QTest::newRow("yyyy/MM/dd+IslamicCivil+C")
        << "2024/09/01" << "yyyy/MM/dd" << isci << 2000 << "C";
    QTest::newRow("ddd yy-MMM-d+IslamicCivil+C")
        << "Wed 24-Ram.-1" << "ddd yy-MMM-d" << isci << 2000 << "C";
    QTest::newRow("dddd yyyy MMMM dd+IslamicCivil+C")
        << "Wednesday 2024 Ramadan 01" << "dddd yyyy MMMM dd" << isci << 2000 << "C";

    QTest::newRow("d.M.yy+IslamicCivil+de-DE")
        << "1.9.24" << "d.M.yy" << isci << 2000 << "de-DE";
    QTest::newRow("dd/MM/yyyy+IslamicCivil+de-DE")
        << "01/09/2024" << "dd/MM/yyyy" << isci << 2000 << "de-DE";
    QTest::newRow("ddd d-MMM-yy+IslamicCivil+de-DE")
        << "Mi. 1-Ram.-24" << "ddd d-MMM-yy" << isci << 2000 << "de-DE";
    QTest::newRow("dddd dd MMMM yyyy+IslamicCivil+de-DE")
        << "Mittwoch 01 Ramadan 2024" << "dddd dd MMMM yyyy" << isci << 2000 << "de-DE";
    QTest::newRow("yy.M.d+IslamicCivil+de-DE")
        << "24.9.1" << "yy.M.d" << isci << 2000 << "de-DE";
    QTest::newRow("yyyy/MM/dd+IslamicCivil+de-DE")
        << "2024/09/01" << "yyyy/MM/dd" << isci << 2000 << "de-DE";
    QTest::newRow("ddd yy-MMM-d+IslamicCivil+de-DE")
        << "Mi. 24-Ram.-1" << "ddd yy-MMM-d" << isci << 2000 << "de-DE";
    QTest::newRow("dddd yyyy MMMM dd+IslamicCivil+de-DE")
        << "Mittwoch 2024 Ramadan 01" << "dddd yyyy MMMM dd" << isci << 2000 << "de-DE";
#endif

#if QT_CONFIG(jalalicalendar)
    const QCalendar jali(QCalendar::System::Jalali);

    QTest::newRow("d.M.yy+Jalali+C")
        << "1.9.24" << "d.M.yy" << jali << 2000 << "C";
    QTest::newRow("dd/MM/yyyy+Jalali+C")
        << "01/09/2024" << "dd/MM/yyyy" << jali << 2000 << "C";
    QTest::newRow("ddd d-MMM-yy+Jalali+C")
        << "Sat 1-Aza-24" << "ddd d-MMM-yy" << jali << 2000 << "C";
    QTest::newRow("dddd dd MMMM yyyy+Jalali+C")
        << "Saturday 01 Azar 2024" << "dddd dd MMMM yyyy" << jali << 2000 << "C";
    QTest::newRow("yy.M.d+Jalali+C")
        << "24.9.1" << "yy.M.d" << jali << 2000 << "C";
    QTest::newRow("yyyy/MM/dd+Jalali+C")
        << "2024/09/01" << "yyyy/MM/dd" << jali << 2000 << "C";
    QTest::newRow("ddd yy-MMM-d+Jalali+C")
        << "Sat 24-Aza-1" << "ddd yy-MMM-d" << jali << 2000 << "C";
    QTest::newRow("dddd yyyy MMMM dd+Jalali+C")
        << "Saturday 2024 Azar 01" << "dddd yyyy MMMM dd" << jali << 2000 << "C";

    QTest::newRow("d.M.yy+Jalali+de-DE")
        << "1.9.24" << "d.M.yy" << jali << 2000 << "de-DE";
    QTest::newRow("dd/MM/yyyy+Jalali+de-DE")
        << "01/09/2024" << "dd/MM/yyyy" << jali << 2000 << "de-DE";
    QTest::newRow("ddd d-MMM-yy+Jalali+de-DE")
        << "Sa. 1-Āsar-24" << "ddd d-MMM-yy" << jali << 2000 << "de-DE";
    QTest::newRow("dddd dd MMMM yyyy+Jalali+de-DE")
        << "Samstag 01 Āsar 2024" << "dddd dd MMMM yyyy" << jali << 2000 << "de-DE";
    QTest::newRow("yy.M.d+Jalali+de-DE")
        << "24.9.1" << "yy.M.d" << jali << 2000 << "de-DE";
    QTest::newRow("yyyy/MM/dd+Jalali+de-DE")
        << "2024/09/01" << "yyyy/MM/dd" << jali << 2000 << "de-DE";
    QTest::newRow("ddd yy-MMM-d+Jalali+de-DE")
        << "Sa. 24-Āsar-1" << "ddd yy-MMM-d" << jali << 2000 << "de-DE";
    QTest::newRow("dddd yyyy MMMM dd+Jalali+de-DE")
        << "Samstag 2024 Āsar 01" << "dddd yyyy MMMM dd" << jali << 2000 << "de-DE";
#endif
}

void tst_QLocale::toDate()
{
    QFETCH(QString, input);
    QFETCH(QString, format);
    QFETCH(QCalendar, calendar);
    QFETCH(int, baseYear);
    QFETCH(QString, locale);

    QLocale loc(locale);

    QDate d;
    QBENCHMARK {
        d = loc.toDate(input, format, calendar, baseYear);
    }
    QVERIFY(d.isValid());
}

void tst_QLocale::toTime_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("locale");

    // C locale
    QTest::newRow("h:m:s.z AP(C)") << "1:2:3.4 PM" << "h:m:s.z AP" << "C";
    QTest::newRow("hh&mm&ss.zz A(C)") << "01&02&03.44 PM" << "hh&mm&ss.zz A" << "C";
    QTest::newRow("HmszzzAp(C)") << "1323444pm" << "HmszzzAp" << "C";
    QTest::newRow("HH/mm/ss.zzz a(C)") << "13/02/03.444 pm" << "HH/mm/ss.zzz a" << "C";
    QTest::newRow("hh mm aP(C)") << "11 59 pm" << "hh mm aP" << "C";
    QTest::newRow("HH-mm-ss ap(C)") << "23-59-58 pm" << "HH-mm-ss ap" << "C";

    // de-DE locale
    QTest::newRow("h:m:s.z AP(de-DE)") << "1:2:3.4 PM" << "h:m:s.z AP" << "de-DE";
    QTest::newRow("hh&mm&ss.zz A(de-DE)") << "01&02&03.44 PM" << "hh&mm&ss.zz A" << "de-DE";
    QTest::newRow("HmszzzAp(de-DE)") << "1323444pm" << "HmszzzAp" << "de-DE";
    QTest::newRow("HH/mm/ss.zzz a(de-DE)") << "13/02/03.444 pm" << "HH/mm/ss.zzz a" << "de-DE";
    QTest::newRow("hh mm aP(de-DE)") << "11 59 pm" << "hh mm aP" << "de-DE";
    QTest::newRow("HH-mm-ss ap(de-DE)") << "23-59-58 pm" << "HH-mm-ss ap" << "de-DE";
}

void tst_QLocale::toTime()
{
    QFETCH(QString, input);
    QFETCH(QString, format);
    QFETCH(QString, locale);

    QLocale loc(locale);

    QTime t;
    QBENCHMARK {
        t = loc.toTime(input, format);
    }
    QVERIFY(t.isValid());
}

void tst_QLocale::toDateTime_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QCalendar>("calendar");
    QTest::addColumn<int>("baseYear");
    QTest::addColumn<QString>("locale");

    const QCalendar greg(QCalendar::System::Gregorian);

    // Gregorian(C)
    QTest::newRow("d.M.yy h:m:s.z AP+Gregorian+C")
        << "1.9.24 1:2:3.4 PM" << "d.M.yy h:m:s.z AP"
        << greg << 2000 << "C";
    QTest::newRow("hh&mm&ss.zz A dd/MM/yyyy t+Gregorian+C")
        << "01&02&03.44 PM 01/09/2024 UTC+02:00" << "hh&mm&ss.zz A dd/MM/yyyy t"
        << greg << 2000 << "C";
    QTest::newRow("ddd d-MMM-yy HmszzzAp tt+Gregorian+C")
        << "Sun 1-Sep-24 1323444pm +0200" << "ddd d-MMM-yy HmszzzAp tt"
        << greg << 2000 << "C";
    QTest::newRow("HH/mm/ss.zzz a dddd dd MMMM yyyy ttt+Gregorian+C")
        << "13/02/03.444 pm Sunday 01 September 2024 +02:00"
        << "HH/mm/ss.zzz a dddd dd MMMM yyyy ttt"
        << greg << 2000 << "C";
    QTest::newRow("yy.M.d hh mm aP tttt+Gregorian+C")
        << "24.9.1 11 59 pm Europe/Berlin" << "yy.M.d hh mm aP tttt"
        << greg << 2000 << "C";
    QTest::newRow("yyyy/MM/dd HH-mm-ss ap+Gregorian+C")
        << "2024/09/01 23-59-58 pm" << "yyyy/MM/dd HH-mm-ss ap"
        << greg << 2000 << "C";
    QTest::newRow("HH/mm/ss.zzz a ddd yy-MMM-d+Gregorian+C")
        << "13/02/03.444 pm Sun 24-Sep-1" << "HH/mm/ss.zzz a ddd yy-MMM-d"
        << greg << 2000 << "C";
    QTest::newRow("dddd yyyy MMMM dd HH-mm-ss ap+Gregorian+C")
        << "Sunday 2024 September 01 23-59-58 pm" << "dddd yyyy MMMM dd HH-mm-ss ap"
        << greg << 2000 << "C";

    // Gregorian(de-DE)
    QTest::newRow("d.M.yy h:m:s.z AP t+Gregorian+de-DE")
        << "1.9.24 1:2:3.4 PM UTC+02:00" << "d.M.yy h:m:s.z AP t"
        << greg << 2000 << "de-DE";
    QTest::newRow("hh&mm&ss.zz A dd/MM/yyyy+Gregorian+de-DE")
        << "01&02&03.44 PM 01/09/2024" << "hh&mm&ss.zz A dd/MM/yyyy"
        << greg << 2000 << "de-DE";
    QTest::newRow("ddd d-MMM-yy HmszzzAp tt+Gregorian+de-DE")
        << "So. 1-Sept.-24 1323444pm +0200" << "ddd d-MMM-yy HmszzzAp tt"
        << greg << 2000 << "de-DE";
    QTest::newRow("HH/mm/ss.zzz a dddd dd MMMM yyyy ttt+Gregorian+de-DE")
        << "13/02/03.444 pm Sonntag 01 September 2024 +02:00"
        << "HH/mm/ss.zzz a dddd dd MMMM yyyy ttt"
        << greg << 2000 << "de-DE";
    QTest::newRow("yy.M.d hh mm aP tttt+Gregorian+de-DE")
        << "24.9.1 11 59 pm Europe/Berlin" << "yy.M.d hh mm aP tttt"
        << greg << 2000 << "de-DE";
    QTest::newRow("yyyy/MM/dd HH-mm-ss ap+Gregorian+de-DE")
        << "2024/09/01 23-59-58 pm" << "yyyy/MM/dd HH-mm-ss ap"
        << greg << 2000 << "de-DE";
    QTest::newRow("HH/mm/ss.zzz a ddd yy-MMM-d+Gregorian+de-DE")
        << "13/02/03.444 pm So. 24-Sept.-1" << "HH/mm/ss.zzz a ddd yy-MMM-d"
        << greg << 2000 << "de-DE";
    QTest::newRow("dddd yyyy MMMM dd HH-mm-ss ap+Gregorian+de-DE")
        << "Sonntag 2024 September 01 23-59-58 pm" << "dddd yyyy MMMM dd HH-mm-ss ap"
        << greg << 2000 << "de-DE";

#if QT_CONFIG(islamiccivilcalendar)
    // IslamicCivil(C)

    const QCalendar isci(QCalendar::System::IslamicCivil);

    QTest::newRow("d.M.yy h:m:s.z AP t+IslamicCivi+C")
        << "1.9.24 1:2:3.4 PM UTC+02:00" << "d.M.yy h:m:s.z AP t"
        << isci << 2000 << "C";
    QTest::newRow("hh&mm&ss.zz A dd/MM/yyyy+IslamicCivi+C")
        << "01&02&03.44 PM 01/09/2024" << "hh&mm&ss.zz A dd/MM/yyyy"
        << isci << 2000 << "C";
    QTest::newRow("ddd d-MMM-yy HmszzzAp tt+IslamicCivi+C")
        << "Wed 1-Ram.-24 1323444pm +0200" << "ddd d-MMM-yy HmszzzAp tt"
        << isci << 2000 << "C";
    QTest::newRow("HH/mm/ss.zzz a dddd dd MMMM yyyy ttt+IslamicCivi+C")
        << "13/02/03.444 pm Wednesday 01 Ramadan 2024 +02:00"
        << "HH/mm/ss.zzz a dddd dd MMMM yyyy ttt"
        << isci << 2000 << "C";
    QTest::newRow("yy.M.d hh mm aP tttt+IslamicCivi+C")
        << "24.9.1 11 59 pm Europe/Berlin" << "yy.M.d hh mm aP tttt"
        << isci << 2000 << "C";
    QTest::newRow("yyyy/MM/dd HH-mm-ss ap+IslamicCivi+C")
        << "2024/09/01 23-59-58 pm" << "yyyy/MM/dd HH-mm-ss ap"
        << isci << 2000 << "C";
    QTest::newRow("HH/mm/ss.zzz a ddd yy-MMM-d+IslamicCivi+C")
        << "13/02/03.444 pm Wed 24-Ram.-1" << "HH/mm/ss.zzz a ddd yy-MMM-d"
        << isci << 2000 << "C";
    QTest::newRow("dddd yyyy MMMM dd HH-mm-ss ap+IslamicCivi+C")
        << "Wednesday 2024 Ramadan 01 23-59-58 pm" << "dddd yyyy MMMM dd HH-mm-ss ap"
        << isci << 2000 << "C";

    // IslamicCivil(de-DE)
    QTest::newRow("d.M.yy h:m:s.z AP t+IslamicCivil+de-DE")
        << "1.9.24 1:2:3.4 PM UTC+02:00" << "d.M.yy h:m:s.z AP t"
        << isci << 2000 << "de-DE";
    QTest::newRow("hh&mm&ss.zz A dd/MM/yyyy+IslamicCivil+de-DE")
        << "01&02&03.44 PM 01/09/2024" << "hh&mm&ss.zz A dd/MM/yyyy"
        << isci << 2000 << "de-DE";
    QTest::newRow("ddd d-MMM-yy HmszzzAp tt+IslamicCivil+de-DE")
        << "Mi. 1-Ram.-24 1323444pm +0200" << "ddd d-MMM-yy HmszzzAp tt"
        << isci << 2000 << "de-DE";
    QTest::newRow("HH/mm/ss.zzz a dddd dd MMMM yyyy ttt+IslamicCivil+de-DE")
        << "13/02/03.444 pm Mittwoch 01 Ramadan 2024 +02:00"
        << "HH/mm/ss.zzz a dddd dd MMMM yyyy ttt"
        << isci << 2000 << "de-DE";
    QTest::newRow("yy.M.d hh mm aP tttt+IslamicCivil+de-DE")
        << "24.9.1 11 59 pm Europe/Berlin" << "yy.M.d hh mm aP tttt"
        << isci << 2000 << "de-DE";
    QTest::newRow("yyyy/MM/dd HH-mm-ss ap+IslamicCivil+de-DE")
        << "2024/09/01 23-59-58 pm" << "yyyy/MM/dd HH-mm-ss ap"
        << isci << 2000 << "de-DE";
    QTest::newRow("HH/mm/ss.zzz a ddd yy-MMM-d+IslamicCivil+de-DE")
        << "13/02/03.444 pm Mi. 24-Ram.-1" << "HH/mm/ss.zzz a ddd yy-MMM-d"
        << isci << 2000 << "de-DE";
    QTest::newRow("dddd yyyy MMMM dd HH-mm-ss ap+IslamicCivil+de-DE")
        << "Mittwoch 2024 Ramadan 01 23-59-58 pm" << "dddd yyyy MMMM dd HH-mm-ss ap"
        << isci << 2000 << "de-DE";
#endif

#if QT_CONFIG(jalalicalendar)
    // Jalali(C)

    const QCalendar jali(QCalendar::System::Jalali);

    QTest::newRow("d.M.yy h:m:s.z AP t+Jalali+C") << "1.9.24 1:2:3.4 PM UTC+02:00"
        << "d.M.yy h:m:s.z AP t"
        << jali << 2000 << "C";

    QTest::newRow("hh&mm&ss.zz A dd/MM/yyyy+Jalali+C") << "01&02&03.44 PM 01/09/2024"
        << "hh&mm&ss.zz A dd/MM/yyyy"
        << jali << 2000 << "C";

    QTest::newRow("ddd d-MMM-yy HmszzzAp tt+Jalali+C") << "Sat 1-Aza-24 1323444pm +0200"
        << "ddd d-MMM-yy HmszzzAp tt"
        << jali << 2000 << "C";

    QTest::newRow("dddd dd MMMM yyyy ttt+Jalali+C") << "Saturday 01 Azar 2024 +02:00"
        << "dddd dd MMMM yyyy ttt"
        << jali << 2000 << "C";

    QTest::newRow("yy.M.d hh mm aP tttt+Jalali+C") << "24.9.1 11 59 pm Europe/Berlin"
        << "yy.M.d hh mm aP tttt"
        << jali << 2000 << "C";

    QTest::newRow("yyyy/MM/dd HH-mm-ss ap+Jalali+C") << "2024/09/01 23-59-58 pm"
        << "yyyy/MM/dd HH-mm-ss ap"
        << jali << 2000 << "C";

    QTest::newRow("HH/mm/ss.zzz a ddd yy-MMM-d+Jalali+C") << "13/02/03.444 pm Sat 24-Aza-1"
        << "HH/mm/ss.zzz a ddd yy-MMM-d"
        << jali << 2000 << "C";

    QTest::newRow("dddd yyyy MMMM dd HH-mm-ss ap+Jalali+C")
        << "Saturday 2024 Azar 01 23-59-58 pm"
        << "dddd yyyy MMMM dd HH-mm-ss ap"
        << jali << 2000 << "C";

    // Jalali(de-DE)
    QTest::newRow("d.M.yy h:m:s.z AP t+Jalali+de-DE")
        << "1.9.24 1:2:3.4 PM UTC+02:00" << "d.M.yy h:m:s.z AP t"
        << jali << 2000 << "de-DE";

    QTest::newRow("hh&mm&ss.zz A dd/MM/yyyy+Jalali+de-DE")
        << "01&02&03.44 PM 01/09/2024" << "hh&mm&ss.zz A dd/MM/yyyy"
        << jali << 2000 << "de-DE";

    QTest::newRow("ddd d-MMM-yy HmszzzAp tt+Jalali+de-DE")
        << "Sa. 1-Āsar-24 1323444pm +0200" << "ddd d-MMM-yy HmszzzAp tt"
        << jali << 2000 << "de-DE";

    QTest::newRow("dddd dd MMMM yyyy ttt+Jalali+de-DE")
        << "Samstag 01 Āsar 2024 +02:00" << "dddd dd MMMM yyyy ttt"
        << jali << 2000 << "de-DE";

    QTest::newRow("yy.M.d hh mm aP tttt+Jalali+de-DE")
        << "24.9.1 11 59 pm Europe/Berlin" << "yy.M.d hh mm aP tttt"
        << jali << 2000 << "de-DE";

    QTest::newRow("yyyy/MM/dd HH-mm-ss ap+Jalali+de-DE")
        << "2024/09/01 23-59-58 pm" << "yyyy/MM/dd HH-mm-ss ap"
        << jali << 2000 << "de-DE";

    QTest::newRow("HH/mm/ss.zzz a ddd yy-MMM-d+Jalali+de-DE")
        << "13/02/03.444 pm Sa. 24-Āsar-1" << "HH/mm/ss.zzz a ddd yy-MMM-d"
        << jali << 2000 << "de-DE";

    QTest::newRow("dddd yyyy MMMM dd HH-mm-ss ap+Jalali+de-DE")
        << "Samstag 2024 Āsar 01 23-59-58 pm" << "dddd yyyy MMMM dd HH-mm-ss ap"
        << jali << 2000 << "de-DE";
#endif
}

void tst_QLocale::toDateTime()
{
    QFETCH(QString, input);
    QFETCH(QString, format);
    QFETCH(QCalendar, calendar);
    QFETCH(int, baseYear);
    QFETCH(QString, locale);

    QLocale loc(locale);

    QDateTime dt;

    QBENCHMARK {
        dt = loc.toDateTime(input, format, calendar, baseYear);
    }
    QVERIFY(dt.isValid());
}


QTEST_MAIN(tst_QLocale)

#include "tst_bench_qlocale.moc"
