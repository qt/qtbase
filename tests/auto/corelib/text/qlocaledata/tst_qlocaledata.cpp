// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>

#include <QLocale>
#include <private/qlocale_p.h>

#include <QDebug>
#include <private/qlocale_tools_p.h>
#include <qnumeric.h>

#ifndef QT_BUILD_INTERNAL
#error "This subdirectory is for internal (developer-build) tests only"
#endif // QT_BUILD_INTERNAL

using namespace Qt::StringLiterals;

class tst_QLocaleData : public QObject
{
    Q_OBJECT

private slots:
    void testNames_data();
    void testNames();
    void signsNeverCompareEqualToNullCharacter_data() { testNames_data(); }
    void signsNeverCompareEqualToNullCharacter();

    void numericData_data();
    void numericData();
    void numericDataDigits_data();
    void numericDataDigits();
    void suzhouDigits_data();
    void suzhouDigits();

    void strtod_data();
    void strtod();

    void digitSequence_data();
    void digitSequence();

#ifndef QT_NO_SYSTEMLOCALE
    void mySystemLocale_data();
    void mySystemLocale();
    void systemGrouping_data();
    void systemGrouping();
#endif
};

void tst_QLocaleData::testNames_data()
{
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<QLocale::Territory>("country");

    bool ok = QLocaleData::allLocaleDataRows([](qsizetype index, const QLocaleData &item) {
        const QByteArray lang =
                QLocale::languageToString(QLocale::Language(item.m_language_id)).toUtf8();
        const QByteArray land =
                QLocale::territoryToString(QLocale::Territory(item.m_territory_id)).toUtf8();

        QTest::addRow("data_%d (%s/%s)", int(index), lang.constData(), land.constData())
                << QLocale::Language(item.m_language_id) << QLocale::Territory(item.m_territory_id);
        return true;
    });
    QVERIFY(ok);
}

void tst_QLocaleData::testNames()
{
    QFETCH(QLocale::Language, language);
    QFETCH(const QLocale::Territory, country);

    const QLocale l1(language, country);
    if (language == QLocale::AnyLanguage && country == QLocale::AnyTerritory)
        language = QLocale::C;
    QCOMPARE(l1.language(), language);
    QCOMPARE(l1.territory(), country);

    const QString name = l1.name();

    const QLocale l2(name);
    QCOMPARE(l2.language(), language);
    QCOMPARE(l2.territory(), country);
    QCOMPARE(l2.name(), name);

    const QLocale l3(name + QLatin1String("@foo"));
    QCOMPARE(l3.language(), language);
    QCOMPARE(l3.territory(), country);
    QCOMPARE(l3.name(), name);

    const QLocale l4(name + QLatin1String(".foo"));
    QCOMPARE(l4.language(), language);
    QCOMPARE(l4.territory(), country);
    QCOMPARE(l4.name(), name);

    if (language != QLocale::C) {
        const int idx = name.indexOf(QLatin1Char('_'));
        QCOMPARE_NE(idx, -1);
        const QString lang = name.left(idx);

        QCOMPARE(QLocale(lang).language(), language);
        QCOMPARE(QLocale(lang + QLatin1String("@foo")).language(), language);
        QCOMPARE(QLocale(lang + QLatin1String(".foo")).language(), language);
    }
}

void tst_QLocaleData::signsNeverCompareEqualToNullCharacter() // otherwise QTextStream has a problem
{
    QFETCH(QLocale::Language, language);
    QFETCH(const QLocale::Territory, country);

    if (language == QLocale::AnyLanguage && country == QLocale::AnyTerritory)
        language = QLocale::C;

    const QLocale loc(language, country);
    QCOMPARE_NE(loc.negativeSign(), QChar());
    QCOMPARE_NE(loc.positiveSign(), QChar());
}

#define LOCALE_DATA_PTR(lang, script, terr) QLocaleData::dataForLocaleIndex( \
        QLocaleData::findLocaleIndex({ QLocale::lang, QLocale::script, QLocale::terr }))

void tst_QLocaleData::numericData_data()
{
    // Inputs:
    QTest::addColumn<const QLocaleData *>("data");
    QTest::addColumn<QLocaleData::NumberMode>("mode");
    // Outputs:
    QTest::addColumn<QString>("decimal");
    QTest::addColumn<QString>("group"); // (*Whether* to group is a QLocalePrivate question.)
    QTest::addColumn<QString>("minus");
    QTest::addColumn<QString>("plus");
    QTest::addColumn<QString>("exponent");
    QTest::addColumn<QLocaleData::GroupSizes>("groupSizes");
#define GS(f, h, e) QLocaleData::GroupSizes{f, h, e}
    QTest::addColumn<char32_t>("zero");
    QTest::addColumn<bool>("cyril");
    // isC: handled by inspecting test-row name.

    // Doesn't set any field of NumericData except isC and grouping:
    QTest::newRow("C/exp")
        << QLocaleData::c() << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"e"_s << GS(1, 3, 3) << U'0' << false;

    const QLocaleData *enUS = LOCALE_DATA_PTR(English, LatinScript, UnitedStates);
    // Check mode controls which fields are set:
    QTest::newRow("en-Latn-US/int")
        << enUS << QLocaleData::IntegerMode
        << u""_s << u","_s << u"-"_s << u"+"_s << u""_s << GS(1, 3, 3) << U'0' << false;
    QTest::newRow("en-Latn-US/frac")
        << enUS << QLocaleData::DoubleStandardMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u""_s << GS(1, 3, 3) << U'0' << false;
    QTest::newRow("en-Latn-US/exp")
        << enUS << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"E"_s << GS(1, 3, 3) << U'0' << false;
    QTest::newRow("en-US/exp")
        << LOCALE_DATA_PTR(English, AnyScript, UnitedStates) << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"E"_s << GS(1, 3, 3) << U'0' << false;
    QTest::newRow("en/exp")
        << LOCALE_DATA_PTR(English, AnyScript, AnyTerritory) << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"E"_s << GS(1, 3, 3) << U'0' << false;
    QTest::newRow("en-Latn/exp")
        << LOCALE_DATA_PTR(English, LatinScript, AnyTerritory) << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"E"_s << GS(1, 3, 3) << U'0' << false;

    // Check for Cyrillic special case:
    QTest::newRow("uk-Cyrl-UA/frac")
        << LOCALE_DATA_PTR(Ukrainian, CyrillicScript, Ukraine) << QLocaleData::DoubleStandardMode
        << u","_s << u"\u00A0"_s << u"-"_s << u"+"_s << QString()
        << GS(1, 3, 3) << U'0' << false; // Only applies when exponent is included.
    QTest::newRow("uk-Cyrl-UA/exp")
        << LOCALE_DATA_PTR(Ukrainian, CyrillicScript, Ukraine) << QLocaleData::DoubleScientificMode
        << u","_s << u"\u00A0"_s << u"-"_s << u"+"_s << u"\u0415"_s
        << GS(1, 3, 3) << U'0' << true;

    // Check Arabic:
    QTest::newRow("ar-EG/int") // U+061C (Arabic Letter Mark) before signs.
        << LOCALE_DATA_PTR(Arabic, ArabicScript, Egypt) << QLocaleData::IntegerMode
        << QString() << u"\u066C"_s << u"\u061C-"_s << u"\u061C+"_s << QString()
        << GS(1, 3, 3) << U'\u0660' << false;
    QTest::newRow("ar-EG/frac")
        << LOCALE_DATA_PTR(Arabic, ArabicScript, Egypt) << QLocaleData::DoubleStandardMode
        << u"\u066B"_s << u"\u066C"_s << u"\u061C-"_s << u"\u061C+"_s << QString()
        << GS(1, 3, 3) << U'\u0660' << false;
    QTest::newRow("ar-EG/exp")
        << LOCALE_DATA_PTR(Arabic, ArabicScript, Egypt) << QLocaleData::DoubleScientificMode
        << u"\u066B"_s << u"\u066C"_s << u"\u061C-"_s << u"\u061C+"_s << u"\u0623\u0633"_s
        << GS(1, 3, 3) << U'\u0660' << false;

    // Variations on zero digit:
    QTest::newRow("pa-Arab-PK/exp") // L-to-R mark both before and after sign
        << LOCALE_DATA_PTR(Punjabi, ArabicScript, Pakistan) << QLocaleData::DoubleScientificMode
        << u"\u066B"_s << u"\u066C"_s << u"\u200E-\u200E"_s << u"\u200E+\u200E"_s
        << u"\u00D7\u06F1\u06F0^"_s << GS(1, 3, 3) << U'\u06F0' << false;

    QTest::newRow("ne-Deva-NP/exp")
        << LOCALE_DATA_PTR(Nepali, DevanagariScript, Nepal) << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"E"_s << GS(1, 2, 3) << U'\u0966' << false;

    QTest::newRow("mni-Beng-IN/exp")
        << LOCALE_DATA_PTR(Manipuri, BanglaScript, India) << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"E"_s << GS(1, 3, 3) << U'\u09E6' << false;

    QTest::newRow("mni-Mtei-IN/exp")
        << LOCALE_DATA_PTR(Manipuri, MeiteiMayekScript, India) << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"E"_s << GS(1, 3, 3) << U'\uABF0' << false;

    QTest::newRow("nqo-Nkoo-GN/exp")
        << LOCALE_DATA_PTR(Nko, NkoScript, Guinea) << QLocaleData::DoubleScientificMode
        << u"."_s << u"\u060C"_s << u"-"_s << u"+"_s << u"E"_s
        << GS(1, 3, 3) << U'\u07C0' << false;

    QTest::newRow("ff-Adlm-GN/exp")
        << LOCALE_DATA_PTR(Fulah, AdlamScript, Guinea) << QLocaleData::DoubleScientificMode
        << u"."_s << u"\u2E41"_s << u"-"_s << u"+"_s << u"\U0001E909"_s <<
        GS(1, 3, 3) << U'\U0001E950' << false;

    QTest::newRow("ccp-Cakm-BD/exp")
        << LOCALE_DATA_PTR(Chakma, ChakmaScript, Bangladesh) << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"E"_s
        << GS(1, 2, 3) << U'\U00011136' << false;

    QTest::newRow("dz-Tibt-BT/exp")
        << LOCALE_DATA_PTR(Dzongkha, TibetanScript, Bhutan) << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"E"_s << GS(1, 2, 3) << U'\u0F20' << false;

    QTest::newRow("my-Mimr-MM/exp")
        << LOCALE_DATA_PTR(Burmese, MyanmarScript, Myanmar) << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"E"_s << GS(1, 3, 3) << U'\u1040' << false;

    QTest::newRow("sat-Olck-IN/exp")
        << LOCALE_DATA_PTR(Santali, OlChikiScript, India) << QLocaleData::DoubleScientificMode
        << u"."_s << u","_s << u"-"_s << u"+"_s << u"E"_s << GS(1, 3, 3) << U'\u1C50' << false;

    // Variations on exponent separator (where not already covered)
    QTest::newRow("se-Latn-NO/exp")
        << LOCALE_DATA_PTR(NorthernSami, LatinScript, Norway) << QLocaleData::DoubleScientificMode
        << u","_s << u"\u00A0"_s << u"\u2212"_s << u"+"_s << u"\u00B7" "10^"_s
        << GS(1, 3, 3) << U'0' << false;

    QTest::newRow("sv-Latn-SE/exp")
        << LOCALE_DATA_PTR(Swedish, LatinScript, Sweden) << QLocaleData::DoubleScientificMode
        << u","_s << u"\u00A0"_s << u"\u2212"_s << u"+"_s << u"\u00D7" "10^"_s
        << GS(1, 3, 3) << U'0' << false;

    // Central and Southern Kurdish share their exponent with Sindhi.
    // Central Kurdish also has an unusual variant on minus sign.
    QTest::newRow("ckb-Arab-IQ/exp") // R-to-L mark before sign:
        << LOCALE_DATA_PTR(CentralKurdish, ArabicScript, Iraq) << QLocaleData::DoubleScientificMode
        << u"\u066B"_s << u"\u066C"_s << u"\u200F-"_s << u"\u200F+"_s << u"\u0627\u0633"_s
        << GS(1, 3, 3) << U'\u0660' << false;

    // Sign variants:
    QTest::newRow("ar-Arab-TN/exp") // L-to-R mark only before sign:
        << LOCALE_DATA_PTR(Arabic, ArabicScript, Tunisia) << QLocaleData::DoubleScientificMode
        << u","_s << u"."_s << u"\u200E-"_s << u"\u200E+"_s << u"E"_s
        << GS(1, 3, 3) << U'0' << false;

    QTest::newRow("fa-Arab-IR/exp")
        << LOCALE_DATA_PTR(Persian, ArabicScript, Iran) << QLocaleData::DoubleScientificMode
        << u"\u066B"_s << u"\u066C"_s << u"\u200E\u2212"_s << u"\u200E+"_s
        << u"\u00D7\u06F1\u06F0^"_s << GS(1, 3, 3) << U'\u06F0' << false;

    // Grouping separator variants:
    QTest::newRow("gsw-Latn-CH/exp") // Uses apostrophe for grouping (matching C++):
        << LOCALE_DATA_PTR(SwissGerman, LatinScript, Switzerland)
        << QLocaleData::DoubleScientificMode
        << u"."_s << u"'"_s << u"\u2212"_s << u"+"_s << u"E"_s
        << GS(1, 3, 3) << U'0' << false;

    QTest::newRow("fr-Latn-FR/exp") // Narrow non-breaking space (as in BIPM) for grouping:
        << LOCALE_DATA_PTR(French, LatinScript, France) << QLocaleData::DoubleScientificMode
        << u","_s << u"\u202F"_s << u"-"_s << u"+"_s << u"E"_s
        << GS(1, 3, 3) << U'0' << false;

    QTest::newRow("gez-Ethi-ET/exp") // U+12C8 (Ethiopic Symbol WA) as grouping separator:
        << LOCALE_DATA_PTR(Geez, EthiopicScript, Ethiopia) << QLocaleData::DoubleScientificMode
        << u"."_s << u"\u12C8"_s << u"-"_s << u"+"_s << u"E"_s
        << GS(1, 3, 3) << U'0' << false;
#undef GS
}

void tst_QLocaleData::numericData()
{
    QFETCH(const QLocaleData *, data);
    QFETCH(const QLocaleData::NumberMode, mode);
    QFETCH(const QString, decimal);
    QFETCH(const QString, group);
    QFETCH(const QString, minus);
    QFETCH(const QString, plus);
    QFETCH(const QString, exponent);
    QFETCH(const QLocaleData::GroupSizes, groupSizes);
    QFETCH(const char32_t, zero);
    QFETCH(const bool, cyril);
    const bool isC = QByteArrayView(QTest::currentDataTag()).startsWith("C/");

    QLocaleData::NumericData numeric(data, mode);
    if (isC || mode == QLocaleData::IntegerMode)
        QVERIFY(numeric.decimal.isNull());
    else
        QCOMPARE(numeric.decimal, decimal);
    if (isC)
        QVERIFY(numeric.group.isNull());
    else
        QCOMPARE(numeric.group, group);

    if (isC)
        QVERIFY(numeric.minus.isNull());
    else
        QCOMPARE(numeric.minus, minus);
    if (isC)
        QVERIFY(numeric.plus.isNull());
    else
        QCOMPARE(numeric.plus, plus);

    if (isC || mode != QLocaleData::DoubleScientificMode)
        QVERIFY(numeric.exponent.isNull());
    else
        QCOMPARE(numeric.exponent, exponent);
    QCOMPARE(numeric.isC, isC);
    QCOMPARE(numeric.grouping.first, groupSizes.first);
    QCOMPARE(numeric.grouping.higher, groupSizes.higher);
    QCOMPARE(numeric.grouping.least, groupSizes.least);
    QCOMPARE(numeric.zeroUcs, isC ? char32_t(0) : zero);
    QCOMPARE(numeric.zeroLen, isC ? 0 : QChar::requiresSurrogates(zero) ? 2 : 1);
    QCOMPARE(numeric.exponentCyrillic, cyril);
}

void tst_QLocaleData::numericDataDigits_data()
{
    QTest::addColumn<const QLocaleData *>("data");
    QTest::addColumn<char32_t>("character");
    QTest::addColumn<int>("value");

    // All but Suzhou digits come in contiguous blocks.
    const auto dataFor = [](const QLocaleData *data) {
        const char32_t zero = data->zeroUcs();
        const bool wide = QChar::requiresSurrogates(zero);
        const QByteArray name = data->id().name();
        const char *nom = name.data();
        for (int i = 0; i < 10; ++i) {
            QTest::addRow("%s/%d", nom, i) << data << char32_t(zero + i) << i;
            QTest::addRow("%s/ASCII-%d", nom, i) << data << char32_t(U'0' + i) << (wide ? -1 : i);
        }
        QTest::addRow("%s/early", nom) << data << char32_t(zero - 1) << -1;
        QTest::addRow("%s/late", nom) << data << char32_t(zero + 10) << -1;
        QTest::addRow("%s/ASCII-early", nom) << data << U'/' << -1; // U'0' - 1
        QTest::addRow("%s/ASCII-late", nom) << std::move(data) << U':' << -1; // U'9' + 1
    };
    dataFor(QLocaleData::c());
    dataFor(LOCALE_DATA_PTR(English, LatinScript, UnitedStates));
    dataFor(LOCALE_DATA_PTR(Ukrainian, CyrillicScript, Ukraine));
    dataFor(LOCALE_DATA_PTR(Arabic, ArabicScript, Egypt));
    dataFor(LOCALE_DATA_PTR(Punjabi, ArabicScript, Pakistan));
    dataFor(LOCALE_DATA_PTR(Nepali, DevanagariScript, Nepal));
    dataFor(LOCALE_DATA_PTR(Manipuri, BanglaScript, India));
    dataFor(LOCALE_DATA_PTR(Manipuri, MeiteiMayekScript, India));
    dataFor(LOCALE_DATA_PTR(Nko, NkoScript, Guinea));
    dataFor(LOCALE_DATA_PTR(Fulah, AdlamScript, Guinea));
    dataFor(LOCALE_DATA_PTR(Chakma, ChakmaScript, Bangladesh));
    dataFor(LOCALE_DATA_PTR(Dzongkha, TibetanScript, Bhutan));
    dataFor(LOCALE_DATA_PTR(Burmese, MyanmarScript, Myanmar));
    dataFor(LOCALE_DATA_PTR(Santali, OlChikiScript, India));
    dataFor(LOCALE_DATA_PTR(NorthernSami, LatinScript, Norway));
    dataFor(LOCALE_DATA_PTR(Swedish, LatinScript, Sweden));
    dataFor(LOCALE_DATA_PTR(CentralKurdish, ArabicScript, Iraq));
    dataFor(LOCALE_DATA_PTR(Arabic, ArabicScript, Tunisia));
    dataFor(LOCALE_DATA_PTR(Persian, ArabicScript, Iran));
    dataFor(LOCALE_DATA_PTR(SwissGerman, LatinScript, Switzerland));
    dataFor(LOCALE_DATA_PTR(French, LatinScript, France));
    dataFor(LOCALE_DATA_PTR(Geez, EthiopicScript, Ethiopia));

    // No CLDR locale uses Suzhou digits: see suzhouDigits() below.
}

void tst_QLocaleData::numericDataDigits()
{
    QFETCH(const QLocaleData *, data);
    QFETCH(const char32_t, character);
    QFETCH(const int, value);

    // digitValue() doesn't depend on mode
    QLocaleData::NumericData numeric(data, QLocaleData::IntegerMode);
    QCOMPARE(numeric.digitValue(character), qint8(value));
}

void tst_QLocaleData::suzhouDigits_data()
{
    QTest::addColumn<char32_t>("character");
    QTest::addColumn<int>("value");

    QTest::newRow("0") << U'\u3007' << 0;
    for (int i = 1; i < 10; ++i) {
        QTest::addRow("%d", i) << char32_t(U'\u3020' + i) << i;
        QTest::addRow("ASCII-%d", i) << char32_t(U'0' + i) << i;
    }

    // 25 == 0x20 - 0x07
    for (uint i = 10; i <= 25; ++i)
        QTest::addRow("gap-%x", i - 10) << char32_t(U'\u3007' + i) << -1;

    QTest::newRow("early") << U'\u3006' << -1;
    QTest::newRow("late") << U'\u302a' << -1;
    QTest::newRow("ASCII-early") << U'/' << -1;
    QTest::newRow("ASCII-late") << U':' << -1;
}

void tst_QLocaleData::suzhouDigits()
{
    QFETCH(const char32_t, character);
    QFETCH(const int, value);

    // Fake up a suzhou-using locale's numeric data:
    const QLocaleData *zhHantCN = QLocaleData::dataForLocaleIndex(QLocaleData::findLocaleIndex({
                QLocale::Chinese, QLocale::TraditionalHanScript, QLocale::China}));
    QLocaleData::NumericData suzhou(zhHantCN, QLocaleData::IntegerMode);
    suzhou.setZero(u"\u3007");

    QCOMPARE(suzhou.digitValue(character), qint8(value));
}

void tst_QLocaleData::strtod_data()
{
    QTest::addColumn<QString>("num_str");
    QTest::addColumn<double>("num");
    QTest::addColumn<int>("processed");
    QTest::addColumn<bool>("ok");

    // plain numbers, success
    QTest::newRow("0")               << QString("0")               << 0.0           << 1  << true;
    QTest::newRow("0.")              << QString("0.")              << 0.0           << 2  << true;
    QTest::newRow("0.0")             << QString("0.0")             << 0.0           << 3  << true;
    QTest::newRow("0e+0")            << QString("0e+0")            << 0.0           << 4  << true;
    QTest::newRow("0e-0")            << QString("0e-0")            << 0.0           << 4  << true;
    QTest::newRow("0e+1")            << QString("0e+1")            << 0.0           << 4  << true;
    QTest::newRow("0e-1")            << QString("0e-1")            << 0.0           << 4  << true;
    QTest::newRow("0E+0")            << QString("0E+0")            << 0.0           << 4  << true;
    QTest::newRow("0E-0")            << QString("0E-0")            << 0.0           << 4  << true;
    QTest::newRow("0E+1")            << QString("0E+1")            << 0.0           << 4  << true;
    QTest::newRow("0E-1")            << QString("0E-1")            << 0.0           << 4  << true;
    QTest::newRow("3.4")             << QString("3.4")             << 3.4           << 3  << true;
    QTest::newRow("0.035003945")     << QString("0.035003945")     << 0.035003945   << 11 << true;
    QTest::newRow("3.5003945e-2")    << QString("3.5003945e-2")    << 0.035003945   << 12 << true;
    QTest::newRow("0.000003945")     << QString("0.000003945")     << 0.000003945   << 11 << true;
    QTest::newRow("3.945e-6")        << QString("3.945e-6")        << 0.000003945   << 8  << true;
    QTest::newRow("12456789012")     << QString("12456789012")     << 12456789012.0 << 11 << true;
    QTest::newRow("1.2456789012e10") << QString("1.2456789012e10") << 12456789012.0 << 15 << true;

    // Overflow - fails but reports right length:
    QTest::newRow("1e2000")          << QString("1e2000")          << qInf()        << 6  << false;
    QTest::newRow("-1e2000")         << QString("-1e2000")         << -qInf()       << 7  << false;

    // Underflow - fails but reports right length:
    QTest::newRow("1e-2000")         << QString("1e-2000")         << 0.0           << 7  << false;
    QTest::newRow("-1e-2000")        << QString("-1e-2000")        << 0.0           << 8  << false;

    // starts with junk, fails
    QTest::newRow("a0")               << QString("a0")               << 0.0 << 0 << false;
    QTest::newRow("a0.")              << QString("a0.")              << 0.0 << 0 << false;
    QTest::newRow("a0.0")             << QString("a0.0")             << 0.0 << 0 << false;
    QTest::newRow("a3.4")             << QString("a3.4")             << 0.0 << 0 << false;
    QTest::newRow("b0.035003945")     << QString("b0.035003945")     << 0.0 << 0 << false;
    QTest::newRow("c3.5003945e-2")    << QString("c3.5003945e-2")    << 0.0 << 0 << false;
    QTest::newRow("d0.000003945")     << QString("d0.000003945")     << 0.0 << 0 << false;
    QTest::newRow("e3.945e-6")        << QString("e3.945e-6")        << 0.0 << 0 << false;
    QTest::newRow("f12456789012")     << QString("f12456789012")     << 0.0 << 0 << false;
    QTest::newRow("g1.2456789012e10") << QString("g1.2456789012e10") << 0.0 << 0 << false;

    // ends with junk, success
    QTest::newRow("0a")               << QString("0a")               << 0.0           << 1  << true;
    QTest::newRow("0.a")              << QString("0.a")              << 0.0           << 2  << true;
    QTest::newRow("0.0a")             << QString("0.0a")             << 0.0           << 3  << true;
    QTest::newRow("0e+0a")            << QString("0e+0a")            << 0.0           << 4  << true;
    QTest::newRow("0e-0a")            << QString("0e-0a")            << 0.0           << 4  << true;
    QTest::newRow("0e+1a")            << QString("0e+1a")            << 0.0           << 4  << true;
    QTest::newRow("0e-1a")            << QString("0e-1a")            << 0.0           << 4  << true;
    QTest::newRow("0E+0a")            << QString("0E+0a")            << 0.0           << 4  << true;
    QTest::newRow("0E-0a")            << QString("0E-0a")            << 0.0           << 4  << true;
    QTest::newRow("0E+1a")            << QString("0E+1a")            << 0.0           << 4  << true;
    QTest::newRow("0E-1a")            << QString("0E-1a")            << 0.0           << 4  << true;
    QTest::newRow("0.035003945b")     << QString("0.035003945b")     << 0.035003945   << 11 << true;
    QTest::newRow("3.5003945e-2c")    << QString("3.5003945e-2c")    << 0.035003945   << 12 << true;
    QTest::newRow("0.000003945d")     << QString("0.000003945d")     << 0.000003945   << 11 << true;
    QTest::newRow("3.945e-6e")        << QString("3.945e-6e")        << 0.000003945   << 8  << true;
    QTest::newRow("12456789012f")     << QString("12456789012f")     << 12456789012.0 << 11 << true;
    QTest::newRow("1.2456789012e10g") << QString("1.2456789012e10g") << 12456789012.0 << 15 << true;

    // Overflow, ends with cruft - fails but reports right length:
    QTest::newRow("1e2000 cruft")     << QString("1e2000 cruft")     << qInf()        << 6  << false;
    QTest::newRow("-1e2000 cruft")    << QString("-1e2000 cruft")    << -qInf()       << 7  << false;

    // NaN and nan
    QTest::newRow("NaN") << QString("NaN") << qQNaN() << 3 << true;
    QTest::newRow("nan") << QString("nan") << qQNaN() << 3 << true;

    // Underflow, ends with cruft - fails but reports right length:
    QTest::newRow("1e-2000 cruft")    << QString("1e-2000 cruft")    << 0.0           << 7  << false;
    QTest::newRow("-1e-2000 cruft")   << QString("-1e-2000 cruft")   << 0.0           << 8  << false;

    // "0x" prefix, success but only for the "0" before "x"
    QTest::newRow("0x0")               << QString("0x0")               << 0.0 << 1 << true;
    QTest::newRow("0x0.")              << QString("0x0.")              << 0.0 << 1 << true;
    QTest::newRow("0x0.0")             << QString("0x0.0")             << 0.0 << 1 << true;
    QTest::newRow("0x3.4")             << QString("0x3.4")             << 0.0 << 1 << true;
    QTest::newRow("0x0.035003945")     << QString("0x0.035003945")     << 0.0 << 1 << true;
    QTest::newRow("0x3.5003945e-2")    << QString("0x3.5003945e-2")    << 0.0 << 1 << true;
    QTest::newRow("0x0.000003945")     << QString("0x0.000003945")     << 0.0 << 1 << true;
    QTest::newRow("0x3.945e-6")        << QString("0x3.945e-6")        << 0.0 << 1 << true;
    QTest::newRow("0x12456789012")     << QString("0x12456789012")     << 0.0 << 1 << true;
    QTest::newRow("0x1.2456789012e10") << QString("0x1.2456789012e10") << 0.0 << 1 << true;

    // hexfloat is not supported (yet)
    QTest::newRow("0x1.921fb5p+1")     << QString("0x1.921fb5p+1")     << 0.0 << 1 << true;
}

void tst_QLocaleData::strtod()
{
    QFETCH(QString, num_str);
    QFETCH(double, num);
    QFETCH(int, processed);
    QFETCH(bool, ok);

    QByteArray numData = num_str.toUtf8();
    const char *end = nullptr;
    bool actualOk = false;
    double result = qstrtod(numData.constData(), &end, &actualOk);

    QCOMPARE(result, num);
    QCOMPARE(actualOk, ok);
    QCOMPARE(static_cast<int>(end - numData.constData()), processed);

    // Make sure QByteArray, QString and QLocale also work.
    // (They don't support incomplete parsing, and give 0 for overflow.)
    if (ok && (processed == num_str.size() || processed == 0)) {
        actualOk = false;
        QCOMPARE(num_str.toDouble(&actualOk), num);
        QCOMPARE(actualOk, ok);

        actualOk = false;
        QCOMPARE(numData.toDouble(&actualOk), num);
        QCOMPARE(actualOk, ok);

        actualOk = false;
        QCOMPARE(QLocale::c().toDouble(num_str, &actualOk), num);
        QCOMPARE(actualOk, ok);
    }

    // and QStringView, but we can limit the length without allocating memory
    QStringView num_strref = QStringView{ num_str }.mid(0, processed);
    actualOk = false;
    QCOMPARE(QLocale::c().toDouble(num_strref, &actualOk), num);
    QCOMPARE(actualOk, ok);
}

void tst_QLocaleData::digitSequence_data()
{
    using O = QLocaleData::DigitSequence::Option;
    using Os = QLocaleData::DigitSequence::Options;
    QTest::addColumn<const QLocaleData *>("data");
    QTest::addColumn<QString>("numStr");
    QTest::addColumn<Os>("flags");
    QTest::addColumn<qsizetype>("from");
    QTest::addColumn<qsizetype>("endIndex");
    QTest::addColumn<QByteArray>("parsed");
    const auto addRow = [](const char *format, const char *name,
                           const QLocaleData *locale, QString &&numStr,
                           Os opt, qsizetype from, qsizetype end,
                           QByteArray &&expect) {
        QTest::addRow(format, name) << locale << numStr << opt << from << end << expect;
    };

    // Test both for C (which is special-cased) and en-US (which should be the same as it)
    for (int i = 0; i < 2; ++i) {
        const QLocaleData *loc = i == 0 ? QLocaleData::c()
            : LOCALE_DATA_PTR(English, LatinScript, UnitedStates);
        const char *name = i == 0 ? "C" : "en-US";

        addRow("null/%s", name, loc, QString(), O::Default, 0, 0, ""_ba);
        addRow("null/%s/sign", name, loc, QString(), O::AllowSign, 0, 0, ""_ba);
        addRow("empty/%s", name, loc, u""_s, O::Default, 0, 0, ""_ba);
        addRow("empty/%s/sign", name, loc, u""_s, O::AllowSign, 0, 0, ""_ba);
        // Sign on its own does get read:
        addRow("-/%s", name, loc, u"-"_s, O::Default, 0, 0, ""_ba);
        addRow("+/%s", name, loc, u"+"_s, O::Default, 0, 0, ""_ba);
        addRow("-/%s/sign", name, loc, u"-"_s, O::AllowSign, 0, 1, "-"_ba);
        addRow("+/%s/sign", name, loc, u"+"_s, O::AllowSign, 0, 1, "+"_ba);
        // Extra sign is ignored:
        addRow("--/%s", name, loc, u"-"_s, O::Default, 0, 0, ""_ba);
        addRow("++/%s", name, loc, u"+"_s, O::Default, 0, 0, ""_ba);
        addRow("--/%s/sign", name, loc, u"-"_s, O::AllowSign, 0, 1, "-"_ba);
        addRow("++/%s/sign", name, loc, u"+"_s, O::AllowSign, 0, 1, "+"_ba);
        // Dangling cruft gets ignored:
        addRow("-cruft/%s/sign", name, loc, u"-cruft"_s, O::AllowSign, 0, 1, "-"_ba);
        addRow("+cruft/%s/sign", name, loc, u"+cruft"_s, O::AllowSign, 0, 1, "+"_ba);

        // Digits get read:
        addRow("0/%s", name, loc, u"0"_s, O::Default, 0, 1, "0"_ba);
        // But must be at the start:
        addRow(" 0/%s", name, loc, u" 0"_s, O::Default, 0, 0, ""_ba);
        // ... or start at from:
        addRow("0|after /%s", name, loc, u" 0"_s, O::Default, 1, 2, "0"_ba);
        // Dangling cruft makes no difference:
        addRow("0cruft/%s", name, loc, u"0cruft"_s, O::Default, 0, 1, "0"_ba);
        addRow(" 0cruft/%s", name, loc, u" 0cruft"_s, O::Default, 0, 0, ""_ba);
        addRow("0cruft|after /%s", name, loc, u" 0cruft"_s, O::Default, 1, 2, "0"_ba);
        // Sign is not digit:
        addRow("+0/%s", name, loc, u"+0"_s, O::Default, 0, 0, ""_ba);
        addRow("-0/%s", name, loc, u"-0"_s, O::Default, 0, 0, ""_ba);
        // Digit after sign can be read if from points at it:
        addRow("0|after+/%s", name, loc, u"+0"_s, O::Default, 1, 2, "0"_ba);
        addRow("0|after-/%s", name, loc, u"-0"_s, O::Default, 1, 2, "0"_ba);

        // Sign and digit can be read together:
        addRow("+0/%s/sign", name, loc, u"+0"_s, O::AllowSign, 0, 2, "+0"_ba);
        addRow("-0/%s/sign", name, loc, u"-0"_s, O::AllowSign, 0, 2, "-0"_ba);
        // Can still skip sign by setting from:
        addRow("0|after+/%s/sign", name, loc, u"+0"_s, O::AllowSign, 1, 2, "0"_ba);
        addRow("0|after-/%s/sign", name, loc, u"-0"_s, O::AllowSign, 1, 2, "0"_ba);
        // Duplicate sign blocks reading:
        addRow("++0/%s/sign", name, loc, u"++0"_s, O::AllowSign, 0, 1, "+"_ba);
        addRow("--0/%s/sign", name, loc, u"--0"_s, O::AllowSign, 0, 1, "-"_ba);
        // Unless from points after it:
        addRow("+0|after+/%s/sign", name, loc, u"++0"_s, O::AllowSign, 1, 3, "+0"_ba);
        addRow("-0|after-/%s/sign", name, loc, u"--0"_s, O::AllowSign, 1, 3, "-0"_ba);
        // Cruft makes no difference:
        addRow("+0cruft/%s/sign", name, loc, u"+0cruft"_s, O::AllowSign, 0, 2, "+0"_ba);
        addRow("-0cruft/%s/sign", name, loc, u"-0cruft"_s, O::AllowSign, 0, 2, "-0"_ba);
        addRow("0cruft|after+/%s/sign", name, loc, u"+0cruft"_s,
               O::AllowSign, 1, 2, "0"_ba);
        addRow("0cruft|after-/%s/sign", name, loc, u"-0cruft"_s,
               O::AllowSign, 1, 2, "0"_ba);
        addRow("++0cruft/%s/sign", name, loc, u"++0cruft"_s,
               O::AllowSign, 0, 1, "+"_ba);
        addRow("--0cruft/%s/sign", name, loc, u"--0cruft"_s,
               O::AllowSign, 0, 1, "-"_ba);
        addRow("+0cruft|after+/%s/sign", name, loc, u"++0cruft"_s,
               O::AllowSign, 1, 3, "+0"_ba);
        addRow("-0cruft|after-/%s/sign", name, loc, u"--0cruft"_s,
               O::AllowSign, 1, 3, "-0"_ba);

        // We can read many digits:
        addRow("0123456789/%s", name, loc, u"0123456789"_s,
               O::Default, 0, 10, "0123456789"_ba);
        addRow("9012345678/%s/sign", name, loc, u"9012345678"_s,
               O::AllowSign, 0, 10, "9012345678"_ba);
        addRow("8901234567|after+/%s", name, loc, u"+8901234567"_s,
               O::Default, 1, 11, "8901234567"_ba);
        addRow("7890123456|after-/%s", name, loc, u"-7890123456"_s,
               O::Default, 1, 11, "7890123456"_ba);
        addRow("+6789012345/%s/sign", name, loc, u"+6789012345"_s,
               O::AllowSign, 0, 11, "+6789012345"_ba);
        addRow("-5678901234/%s/sign", name, loc, u"-5678901234"_s,
               O::AllowSign, 0, 11, "-5678901234"_ba);
        // Cruft makes no difference:
        addRow("4567890123cruft/%s", name, loc, u"4567890123cruft"_s,
               O::Default, 0, 10, "4567890123"_ba);
        addRow("3456789012cruft/%s/sign", name, loc, u"3456789012cruft"_s,
               O::AllowSign, 0, 10, "3456789012"_ba);
        addRow("2345678901cruft|after+/%s", name, loc, u"+2345678901cruft"_s,
               O::Default, 1, 11, "2345678901"_ba);
        addRow("1234567890cruft|after-/%s", name, loc, u"-1234567890cruft"_s,
               O::Default, 1, 11, "1234567890"_ba);
        addRow("+0123456789cruft/%s/sign", name, loc, u"+0123456789cruft"_s,
               O::AllowSign, 0, 11, "+0123456789"_ba);
        addRow("-0123456789cruft/%s/sign", name, loc, u"-0123456789cruft"_s,
               O::AllowSign, 0, 11, "-0123456789"_ba);

        // Grouping not supported:
        addRow("0,123,456,789/%s", name, loc, u"0,123,456,789"_s,
               O::Default, 0, 1, "0"_ba);
    }

    const QLocaleData *svensk = LOCALE_DATA_PTR(Swedish, LatinScript, Sweden);
    addRow("0123456789/%s", "sv-Latn-SE", svensk, u"0123456789"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("\u2212" "0123456789/%s/sign", "sv-Latn-SE", svensk, u"\u2212" "0123456789"_s,
           O::AllowSign, 0, 11, "-0123456789"_ba);

    const QLocaleData *spanish = LOCALE_DATA_PTR(Spanish, LatinScript, Spain);
    addRow("0123456789/%s", "es-Latn-ES", spanish, u"0123456789"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "es-Latn-ES", spanish, u"\u2212" "0123456789"_s,
           O::AllowSign, 0, 11, "-0123456789"_ba);

    const QLocaleData *tunisian = LOCALE_DATA_PTR(Arabic, ArabicScript, Tunisia);
    addRow("0123456789/%s", "ar-Arab-TN", tunisian, u"0123456789"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "ar-Arab-TN", tunisian, u"\u200E-0123456789"_s,
           O::AllowSign, 0, 12, "-0123456789"_ba);

    const QLocaleData *arabic = LOCALE_DATA_PTR(Arabic, ArabicScript, Egypt);
    addRow("0123456789/%s", "ar-EG", arabic,
           u"\u0660\u0661\u0662\u0663\u0664\u0665\u0666\u0667\u0668\u0669"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "ar-EG", arabic,
           u"\u061C-\u0660\u0661\u0662\u0663\u0664\u0665\u0666\u0667\u0668\u0669"_s,
           O::AllowSign, 0, 12, "-0123456789"_ba);

    const QLocaleData *punjabi = LOCALE_DATA_PTR(Punjabi, ArabicScript, Pakistan);
    addRow("0123456789/%s", "pa-Arab-PK", punjabi,
           u"\u06F0\u06F1\u06F2\u06F3\u06F4\u06F5\u06F6\u06F7\u06F8\u06F9"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "pa-Arab-PK", punjabi,
           u"\u200E-\u200E\u06F0\u06F1\u06F2\u06F3\u06F4\u06F5\u06F6\u06F7\u06F8\u06F9"_s,
           O::AllowSign, 0, 13, "-0123456789"_ba);

    const QLocaleData *persian = LOCALE_DATA_PTR(Persian, ArabicScript, Iran);
    addRow("0123456789/%s", "fa-Arab-IR", persian,
           u"\u06F0\u06F1\u06F2\u06F3\u06F4\u06F5\u06F6\u06F7\u06F8\u06F9"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "fa-Arab-IR", persian,
           u"\u200E\u2212\u06F0\u06F1\u06F2\u06F3\u06F4\u06F5\u06F6\u06F7\u06F8\u06F9"_s,
           O::AllowSign, 0, 12, "-0123456789"_ba);

    const QLocaleData *deva = LOCALE_DATA_PTR(Nepali, DevanagariScript, Nepal);
    addRow("0123456789/%s", "ne-Deva-NP", deva,
           u"\u0966\u0967\u0968\u0969\u096a\u096b\u096c\u096d\u096e\u096f"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "ne-Deva-NP", deva,
           u"-\u0966\u0967\u0968\u0969\u096a\u096b\u096c\u096d\u096e\u096f"_s,
           O::AllowSign, 0, 11, "-0123456789"_ba);

    const QLocaleData *beng = LOCALE_DATA_PTR(Manipuri, BanglaScript, India);
    addRow("0123456789/%s", "mni-Beng-IN", beng,
           u"\u09E6\u09E7\u09E8\u09E9\u09EA\u09EB\u09EC\u09ED\u09EE\u09EF"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "mni-Beng-IN", beng,
           u"-\u09E6\u09E7\u09E8\u09E9\u09EA\u09EB\u09EC\u09ED\u09EE\u09EF"_s,
           O::AllowSign, 0, 11, "-0123456789"_ba);

    const QLocaleData *meitei = LOCALE_DATA_PTR(Manipuri, MeiteiMayekScript, India);
    addRow("0123456789/%s", "mni-Mtei-IN", meitei,
           u"\uABF0\uABF1\uABF2\uABF3\uABF4\uABF5\uABF6\uABF7\uABF8\uABF9"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "mni-Mtei-IN", meitei,
           u"-\uABF0\uABF1\uABF2\uABF3\uABF4\uABF5\uABF6\uABF7\uABF8\uABF9"_s,
           O::AllowSign, 0, 11, "-0123456789"_ba);

    const QLocaleData *nkoo = LOCALE_DATA_PTR(Nko, NkoScript, Guinea);
    addRow("0123456789/%s", "nqo-Nkoo-GN", nkoo,
           u"\u07C0\u07C1\u07C2\u07C3\u07C4\u07C5\u07C6\u07C7\u07C8\u07C9"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "nqo-Nkoo-GN", nkoo,
           u"-\u07C0\u07C1\u07C2\u07C3\u07C4\u07C5\u07C6\u07C7\u07C8\u07C9"_s,
           O::AllowSign, 0, 11, "-0123456789"_ba);

    const QLocaleData *adlam = LOCALE_DATA_PTR(Fulah, AdlamScript, Guinea);
    addRow("0123456789/%s", "ff-Adlm-GN", adlam,
           u"\U0001E950\U0001E951\U0001E952\U0001E953\U0001E954"
           "\U0001E955\U0001E956\U0001E957\U0001E958\U0001E959"_s,
           O::Default, 0, 20, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "ff-Adlm-GN", adlam,
           u"-\U0001E950\U0001E951\U0001E952\U0001E953\U0001E954"
           "\U0001E955\U0001E956\U0001E957\U0001E958\U0001E959"_s,
           O::AllowSign, 0, 21, "-0123456789"_ba);

    const QLocaleData *chakma = LOCALE_DATA_PTR(Chakma, ChakmaScript, Bangladesh);
    addRow("0123456789/%s", "ccp-Cakm-BD", chakma,
           u"\U00011136\U00011137\U00011138\U00011139\U0001113a"
           "\U0001113b\U0001113c\U0001113d\U0001113e\U0001113f"_s,
           O::Default, 0, 20, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "ccp-Cakm-BD", chakma,
           u"-\U00011136\U00011137\U00011138\U00011139\U0001113a"
           "\U0001113b\U0001113c\U0001113d\U0001113e\U0001113f"_s,
           O::AllowSign, 0, 21, "-0123456789"_ba);

    const QLocaleData *tibetan = LOCALE_DATA_PTR(Dzongkha, TibetanScript, Bhutan);
    addRow("0123456789/%s", "dz-Tibt-BT", tibetan,
           u"\u0F20\u0F21\u0F22\u0F23\u0F24\u0F25\u0F26\u0F27\u0F28\u0F29"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "dz-Tibt-BT", tibetan,
           u"-\u0F20\u0F21\u0F22\u0F23\u0F24\u0F25\u0F26\u0F27\u0F28\u0F29"_s,
           O::AllowSign, 0, 11, "-0123456789"_ba);

    const QLocaleData *burmese = LOCALE_DATA_PTR(Burmese, MyanmarScript, Myanmar);
    addRow("0123456789/%s", "my-Mimr-MM", burmese,
           u"\u1040\u1041\u1042\u1043\u1044\u1045\u1046\u1047\u1048\u1049"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "my-Mimr-MM", burmese,
           u"-\u1040\u1041\u1042\u1043\u1044\u1045\u1046\u1047\u1048\u1049"_s,
           O::AllowSign, 0, 11, "-0123456789"_ba);

    const QLocaleData *olchiki = LOCALE_DATA_PTR(Santali, OlChikiScript, India);
    addRow("0123456789/%s", "sat-Olck-IN", olchiki,
           u"\u1C50\u1C51\u1C52\u1C53\u1C54\u1C55\u1C56\u1C57\u1C58\u1C59"_s,
           O::Default, 0, 10, "0123456789"_ba);
    addRow("-0123456789/%s/sign", "sat-Olck-IN", olchiki,
           u"-\u1C50\u1C51\u1C52\u1C53\u1C54\u1C55\u1C56\u1C57\u1C58\u1C59"_s,
           O::AllowSign, 0, 11, "-0123456789"_ba);
}
#undef LOCALE_DATA_PTR

void tst_QLocaleData::digitSequence()
{
    QFETCH(const QLocaleData *, data);
    QFETCH(const QString, numStr);
    QFETCH(const QLocaleData::DigitSequence::Options, flags);
    QFETCH(const qsizetype, from);
    QFETCH(const qsizetype, endIndex);
    QFETCH(const QByteArray, parsed);
    const char sign = parsed.startsWith('-') || parsed.startsWith('+') ? parsed[0] : '\0';

    QLocaleData::DigitSequence result = data->digitSequence(numStr, flags, from);
    QCOMPARE(result.size(), parsed.size());
    QCOMPARE(result.endIndex(), endIndex);
    QCOMPARE(result.hasSign(), sign != '\0');
    QCOMPARE(result.sign, sign);
    QCOMPARE(result.used(numStr, from), numStr.first(endIndex).sliced(from));
    QCOMPARE(result.digits.size(), parsed.size() - (sign == '\0' ? 0 : 1));

    QLocaleData::CharBuff buffer;
    result.transcribeTo(&buffer);
    QByteArrayView actual(buffer);

    QCOMPARE(actual, parsed);
}

#ifndef QT_NO_SYSTEMLOCALE
class MySystemLocale : public QSystemLocale
{
    Q_DISABLE_COPY_MOVE(MySystemLocale)
public:
    MySystemLocale(const QString &locale)
    : m_name(locale), m_id(QLocaleId::fromName(locale)), m_locale(locale)
    {
    }

    QVariant query(QueryType type, QVariant &&/*in*/) const override
    {
        switch (type) {
        case UILanguages:
            if (m_name == u"en-Latn")
                return QVariant(QStringList{u"en-NO"_s});
            if (m_name == u"en-DE") // QTBUG-104930: simulate macOS's list not including m_name.
                return QVariant(QStringList{u"en-GB"_s, u"de-DE"_s});
            if (m_name == u"en-Dsrt-GB")
                return QVariant(QStringList{u"en-Dsrt-GB"_s, u"en-GB"_s});
            if (m_name == u"en-FO") { // Nominally Faroe Islands, used for en-mixed test
                return QVariant(QStringList{u"en-DK"_s, u"en-GB"_s, u"fo-FO"_s,
                                            u"da-FO"_s, u"da-DK"_s});
            }
            if (m_name == u"en-NL") // Anglophone in Netherlands:
                return QVariant(QStringList{u"en-NL"_s, u"nl-NL"_s});
            if (m_name == u"en-NL-GB") // Netherlander at work for a GB-ish employer:
                return QVariant(QStringList{u"en-NL"_s, u"nl-NL"_s, u"en-GB"_s});
            if (m_name == u"de-CA") { // Imagine a 2nd generation Canadian of de-AT ancestry ...
                return QVariant(QStringList{u"en-CA"_s, u"fr-CA"_s, u"de-AT"_s,
                                            u"en-GB"_s, u"fr-FR"_s});
            }
            if (m_name == u"pa-Arab-GB") // Pakistani Punjabi in Britain
                return QVariant(QStringList{u"pa-PK"_s, u"en-GB"_s});
            if (m_name == u"no") // QTBUG-131127
                return QVariant(QStringList{u"no"_s, u"en-US"_s, u"nb"_s});
            if (m_name == u"no-US") // Empty query result:
                return QVariant(QStringList{});
            return QVariant(QStringList{m_name});
        case LanguageId:
            return m_id.language_id;
        case TerritoryId:
            return m_id.territory_id;
        case ScriptId:
            return m_id.script_id;
        case Grouping:
            if (m_name == u"en-ES") // CLDR: 1,3,3
                return QVariant::fromValue(QLocaleData::GroupSizes{2,3,3});
            if (m_name == u"en-BD") // CLDR: 1,3,3
                return QVariant::fromValue(QLocaleData::GroupSizes{1,2,3});
            if (m_name == u"ccp") // CLDR: 1,3,3
                return QVariant::fromValue(QLocaleData::GroupSizes{2,2,3});
            if (m_name == u"en-BT") // CLDR: 1,3,3
                return QVariant::fromValue(QLocaleData::GroupSizes{0,2,3});
            if (m_name == u"en-NP") // CLDR: 1,3,3
                return QVariant::fromValue(QLocaleData::GroupSizes{0,2,0});
            // See GroupSeparator:
            if (m_name == u"en-MN") // (same as CLDR en)
                return QVariant::fromValue(QLocaleData::GroupSizes{1,3,3});
            if (m_name == u"es-MN") // (same as CLDR es-ES)
                return QVariant::fromValue(QLocaleData::GroupSizes{2,3,3});
            if (m_name == u"ccp-MN") // (same as CLDR ccp-IN)
                return QVariant::fromValue(QLocaleData::GroupSizes{2,2,3});
            break;
        case GroupSeparator:
        case DecimalPoint:
            // CLDR v43 through v45 had the same group and fractional-part
            // separator for mn_Mong_MN. A user might also misconfigure their
            // system. Use made-up hybrids *-MN for that.
            if (m_name.endsWith(u"-MN"))
                return QVariant(u"."_s);
            break;
        default:
            break;
        }
        return QVariant();
    }

    QLocale fallbackLocale() const override
    {
        return m_locale;
    }

private:
    const QString m_name;
    const QLocaleId m_id;
    const QLocale m_locale;
};

void tst_QLocaleData::mySystemLocale_data()
{
    // Test uses MySystemLocale, so is platform-independent.
    QTest::addColumn<QString>("name");
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<QStringList>("uiLanguages");

    QTest::addRow("empty")
        << u"no-US"_s << QLocale::NorwegianBokmal
        << QStringList{u"nb-Latn-US"_s, u"nb-US"_s,
                       u"nb-Latn-NO"_s, u"nb-NO"_s, u"nb-Latn"_s, u"nb"_s};
    QTest::addRow("no") // QTBUG-131127
        << u"no"_s << QLocale::NorwegianBokmal
        << QStringList{u"no"_s, u"nb-Latn-NO"_s, u"nb-NO"_s, u"nb-Latn"_s,
                       u"en-Latn-US"_s, u"en-US"_s, u"en-Latn"_s, u"en"_s,
                       u"nb"_s};
    QTest::addRow("en-Latn") // Android crash
        << u"en-Latn"_s << QLocale::English
        << QStringList{u"en-Latn-NO"_s, u"en-NO"_s,
                       u"en-Latn-US"_s, u"en-US"_s, u"en-Latn"_s, u"en"_s};

    QTest::addRow("anglo-dutch") // QTBUG-131894
        << u"en-NL"_s << QLocale::English
        << QStringList{u"en-Latn-NL"_s, u"en-NL"_s,
                       // No later en-Latn-* or en-* in the list, so include truncations now:
                       u"en-Latn"_s, u"en"_s,
                       u"nl-Latn-NL"_s, u"nl-NL"_s, u"nl-Latn"_s, u"nl"_s};
    QTest::addRow("anglo-dutch-GB")
        << u"en-NL-GB"_s << QLocale::English
        << QStringList{u"en-Latn-NL"_s, u"en-NL"_s,
                       u"nl-Latn-NL"_s, u"nl-NL"_s, u"nl-Latn"_s, u"nl"_s,
                       u"en-Latn-GB"_s, u"en-GB"_s, u"en-Latn"_s, u"en"_s};

    QTest::addRow("catalan")
        << u"ca"_s << QLocale::Catalan
        << QStringList{u"ca-Latn-ES"_s, u"ca-ES"_s, u"ca-Latn"_s, u"ca"_s};
    QTest::addRow("catalan-spain")
        << u"ca-ES"_s << QLocale::Catalan
        << QStringList{u"ca-Latn-ES"_s, u"ca-ES"_s, u"ca-Latn"_s, u"ca"_s};
    QTest::addRow("catalan-latin")
        << u"ca-Latn"_s << QLocale::Catalan
        << QStringList{u"ca-Latn-ES"_s, u"ca-ES"_s, u"ca-Latn"_s, u"ca"_s};
    QTest::addRow("ukrainian")
        << u"uk"_s << QLocale::Ukrainian
        << QStringList{u"uk-Cyrl-UA"_s, u"uk-UA"_s, u"uk-Cyrl"_s, u"uk"_s};

    QTest::addRow("english-germany")
        << u"en-DE"_s << QLocale::English
        // First two were missed out before fix to QTBUG-104930:
        << QStringList{u"en-Latn-GB"_s, u"en-GB"_s,
                       u"en-Latn-DE"_s, u"en-DE"_s,
                       u"de-Latn-DE"_s, u"de-DE"_s, u"de-Latn"_s, u"de"_s,
                       // Fallbacks implied by those:
                       u"en-Latn"_s, u"en"_s};

    QTest::addRow("german")
        << u"de"_s << QLocale::German
        << QStringList{u"de-Latn-DE"_s, u"de-DE"_s, u"de-Latn"_s, u"de"_s};
    QTest::addRow("german-britain")
        << u"de-GB"_s << QLocale::German
        << QStringList{u"de-Latn-GB"_s, u"de-GB"_s, u"de-Latn"_s, u"de"_s};
    QTest::addRow("chinese-min")
        << u"zh"_s << QLocale::Chinese
        << QStringList{u"zh-Hans-CN"_s, u"zh-CN"_s, u"zh-Hans"_s, u"zh"_s};
    QTest::addRow("chinese-full")
        << u"zh-Hans-CN"_s << QLocale::Chinese
        << QStringList{u"zh-Hans-CN"_s, u"zh-CN"_s, u"zh-Hans"_s, u"zh"_s};
    QTest::addRow("chinese-taiwan")
        << u"zh-TW"_s << QLocale::Chinese
        << QStringList{u"zh-Hant-TW"_s, u"zh-TW"_s, u"zh-Hant"_s, u"zh"_s};
    QTest::addRow("chinese-trad")
        << u"zh-Hant"_s << QLocale::Chinese
        << QStringList{u"zh-Hant-TW"_s, u"zh-TW"_s, u"zh-Hant"_s, u"zh"_s};

    // For C, it should preserve what the system gave us but only add "C", never anything more:
    QTest::addRow("C") << u"C"_s << QLocale::C << QStringList{u"C"_s};
    QTest::addRow("C-Latn") << u"C-Latn"_s << QLocale::C << QStringList{u"C-Latn"_s, u"C"_s};
    QTest::addRow("C-US") << u"C-US"_s << QLocale::C << QStringList{u"C-US"_s, u"C"_s};
    QTest::addRow("C-Latn-US")
        << u"C-Latn-US"_s << QLocale::C << QStringList{u"C-Latn-US"_s, u"C"_s};
    QTest::addRow("C-Hans") << u"C-Hans"_s << QLocale::C << QStringList{u"C-Hans"_s, u"C"_s};
    QTest::addRow("C-CN") << u"C-CN"_s << QLocale::C << QStringList{u"C-CN"_s, u"C"_s};
    QTest::addRow("C-Hans-CN")
        << u"C-Hans-CN"_s << QLocale::C << QStringList{u"C-Hans-CN"_s, u"C"_s};

    QTest::addRow("pa-Arab-GB")
        << u"pa-Arab-GB"_s << QLocale::Punjabi
        << QStringList{u"pa-Arab-PK"_s, u"pa-PK"_s, u"pa-Arab"_s,
            u"pa-Arab-GB"_s,
            u"en-Latn-GB"_s, u"en-GB"_s,
            // Truncations:
            u"en-Latn"_s, u"en"_s,
            // Last because its implied script, Guru, doesn't match the Arab
            // implied by the entry that it's derived from, pa-PK - in contrast
            // to en-Latn and en.
            u"pa"_s};

    QTest::newRow("en-Dsrt-GB")
        << u"en-Dsrt-GB"_s << QLocale::English
        << QStringList{u"en-Dsrt-GB"_s, u"en-Dsrt"_s,
                       u"en-Latn-GB"_s, u"en-GB"_s, u"en-Latn"_s, u"en"_s};
    QTest::newRow("en-mixed")
        << u"en-FO"_s << QLocale::English
        << QStringList{u"en-Latn-DK"_s, u"en-DK"_s,
                       u"en-Latn-GB"_s, u"en-GB"_s,
                       u"en-Latn-FO"_s, u"en-FO"_s,
                       u"fo-Latn-FO"_s, u"fo-FO"_s, u"fo-Latn"_s, u"fo"_s,
                       u"da-Latn-FO"_s, u"da-FO"_s,
                       u"da-Latn-DK"_s, u"da-DK"_s, u"da-Latn"_s, u"da"_s,
                       // Fallbacks implied by those:
                       u"en-Latn"_s, u"en"_s};
    QTest::newRow("polylingual-CA")
        << u"de-CA"_s << QLocale::German
        << QStringList{u"en-Latn-CA"_s, u"en-CA"_s, u"fr-Latn-CA"_s, u"fr-CA"_s,
                       u"de-Latn-AT"_s, u"de-AT"_s, u"de-Latn-CA"_s, u"de-CA"_s,
                       u"en-Latn-GB"_s, u"en-GB"_s,
                       u"fr-Latn-FR"_s, u"fr-FR"_s, u"fr-Latn"_s, u"fr"_s,
                       // Fallbacks:
                       u"en-Latn"_s, u"en"_s, u"de-Latn"_s, u"de"_s};

    QTest::newRow("und-US")
        << u"und-US"_s << QLocale::C
        << QStringList{u"und-US"_s, u"C"_s};
    QTest::newRow("und-Latn")
        << u"und-Latn"_s << QLocale::C
        << QStringList{u"und-Latn"_s, u"C"_s};

    // TODO: test actual system backends correctly handle locales with
    // script-specificity (script listed first is the default, in CLDR v40):
    // az_{Latn,Cyrl}_AZ, bs_{Latn,Cyrl}_BA, sr_{Cyrl,Latn}_{BA,RS,XK,UZ},
    // sr_{Latn,Cyrl}_ME, ff_{Latn,Adlm}_{BF,CM,GH,GM,GN,GW,LR,MR,NE,NG,SL,SN},
    // shi_{Tfng,Latn}_MA, vai_{Vaii,Latn}_LR, zh_{Hant,Hans}_{MO,HK}
}

void tst_QLocaleData::mySystemLocale()
{
    // Compare uiLanguages(), which tests this for CLDR-derived locales.
    QLocale originalLocale;
    QLocale originalSystemLocale = QLocale::system();

    QFETCH(QString, name);
    QFETCH(QLocale::Language, language);
    QFETCH(QStringList, uiLanguages);

    const QLocale::NumberOptions eno
        = language == QLocale::C ? QLocale::OmitGroupSeparator : QLocale::DefaultNumberOptions;

    {
        MySystemLocale sLocale(name);
        QCOMPARE(QLocale().language(), language);
        const QLocale sys = QLocale::system();
        QCOMPARE(sys.language(), language);
        auto reporter = qScopeGuard([]() {
            qDebug("Actual entries:\n\t%s",
                   qPrintable(QLocale::system().uiLanguages().join(u"\n\t")));
        });
        QCOMPARE(sys.uiLanguages(), uiLanguages);
        QCOMPARE(sys.uiLanguages(QLocale::TagSeparator::Underscore),
                 uiLanguages.replaceInStrings(u"-", u"_"));
        QCOMPARE(sys.numberOptions(), eno);
        reporter.dismiss();
    }

    // Verify MySystemLocale tidy-up restored prior state:
    QT_TEST_EQUALITY_OPS(QLocale(), originalLocale, true);
    QT_TEST_EQUALITY_OPS(QLocale::system(), originalSystemLocale, true);
}

void tst_QLocaleData::systemGrouping_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("separator");
    QTest::addColumn<QString>("zeroDigit");
    QTest::addColumn<int>("whole");
    QTest::addColumn<QString>("formattedWhole");
    QTest::addColumn<double>("real");
    QTest::addColumn<QString>("formattedReal");
    QTest::addColumn<int>("precision");

    // Testing locales with non-{1, 3, 3} groupe sizes, plus some locales
    // that return invalid group sizes to test that we fallback to CLDR data.
    QTest::newRow("en-ES") // {2,3,3}
            << u"en-ES"_s << u"."_s << u"0"_s
            << 1234 << u"1234"_s << 1234.567 << u"1234,567"_s << 3;
    QTest::newRow("en-ES-grouped")
            << u"en-ES"_s << u"."_s << u"0"_s
            << 12345 << u"12.345"_s << 12345.678 << u"12.345,678"_s << 3;
    QTest::newRow("en-ES-long")
            << u"en-ES"_s << u"."_s << u"0"_s << 1234567 << u"1.234.567"_s
            << 1234567.089 << u"1.234.567,089"_s << 3;;
    QTest::newRow("en-BD") // {1,2,3}
            << u"en-BD"_s << u","_s << u"0"_s
            << 123456789 << u"12,34,56,789"_s << 1234567.089 << u"12,34,567.089"_s << 3;
    // Filling in the blanks where sys gives a zero:
    QTest::newRow("en-BT") // {1,2,3}
            << u"en-BT"_s << u","_s << u"0"_s
            << 123456789 << u"12,34,56,789"_s << 1.234 << u"1.234"_s << 3;
    QTest::newRow("en-NP") // {1,2,3}
            << u"en-NP"_s << u","_s << u"0"_s
            << 123456789 << u"12,34,56,789"_s << 1.234 << u"1.234"_s << 3;
    // Test a locale in which fractional-part and group separators coincide.
    // Floating-point handling in this scenario is in general ambiguous.
    // When one reading violates grouping rules, use the other:
    QTest::newRow("en-MN") // {1,3,3}
            << u"en-MN"_s << u"."_s << u"0"_s << 1234 << u"1.234"_s
            << 0.003 << u"0.003"_s << 3; // QTBUG-134913
    QTest::newRow("es-MN") // {2,3,3},
            << u"es-MN"_s << u"."_s << u"0"_s << 123456789 << u"123.456.789"_s
            << 12345.6789 << u"12.345.6789"_s << 4; // long last group => fractional part
    QTest::newRow("es-MN-short")
            << u"es-MN"_s << u"."_s << u"0"_s << 1234 << u"1234"_s
            << 1.234 << u"1.234"_s << 3; // short first "group" => not a group
    QTest::newRow("es-MN-split")
            << u"es-MN"_s << u"."_s << u"0"_s << 1234567 << u"1.234.567"_s
            << 1234.567 << u"1234.567"_s << 3; // long first "group" => rest is fraction
    QTest::newRow("es-MN-whole")
            << u"es-MN"_s << u"."_s << u"0"_s << 1234567 << u"1.234.567"_s
            << 1234567. << u"1.234.567"_s << 0; // short first group => later group separator
    // Test the code's best guesses do match our intentions:
    QTest::newRow("es-MN-plain")
            << u"es-MN"_s << u"."_s << u"0"_s << 12345 << u"12.345"_s
            << 12.345 << u"12.345"_s << 3; // Ambiguous, best guess
    QTest::newRow("es-MN-long")
            << u"es-MN"_s << u"."_s << u"0"_s << 1234567089 << u"1.234.567.089"_s
            << 1234567.089 << u"1.234.567.089"_s << 3; // Ambiguous, best guess
    // This last could equally be argued to be whole, based on "The two earlier
    // separators were grouping, so read the last one the same way."

    // Test handling of surrogates (non-BMP digits) in Chakma (ccp):
    const char32_t zeroVal = 0x11136; // Chakma zero
    const QChar data[] = {
        QChar::highSurrogate(zeroVal), QChar::lowSurrogate(zeroVal),
        QChar::highSurrogate(zeroVal + 1), QChar::lowSurrogate(zeroVal + 1),
        QChar::highSurrogate(zeroVal + 2), QChar::lowSurrogate(zeroVal + 2),
        QChar::highSurrogate(zeroVal + 3), QChar::lowSurrogate(zeroVal + 3),
        QChar::highSurrogate(zeroVal + 4), QChar::lowSurrogate(zeroVal + 4),
        QChar::highSurrogate(zeroVal + 5), QChar::lowSurrogate(zeroVal + 5),
        QChar::highSurrogate(zeroVal + 6), QChar::lowSurrogate(zeroVal + 6),
        QChar::highSurrogate(zeroVal + 7), QChar::lowSurrogate(zeroVal + 7),
        QChar::highSurrogate(zeroVal + 8), QChar::lowSurrogate(zeroVal + 8),
        // QChar::highSurrogate(zeroVal + 9), QChar::lowSurrogate(zeroVal + 9),
    };
    const QChar separator(QLatin1Char(',')); // Group separator for the Chakma locale
    const QChar fractional(QLatin1Char('.')); // Fractional-part (and group for ccp-MN)

    const QString
        // Copy zero so it persists through QFETCH(), after data falls off the stack.
        zero = QString(data, 2),
        one = QString::fromRawData(data + 2, 2),
        two = QString::fromRawData(data + 4, 2),
        three = QString::fromRawData(data + 6, 2),
        four = QString::fromRawData(data + 8, 2),
        five = QString::fromRawData(data + 10, 2),
        six = QString::fromRawData(data + 12, 2),
        seven = QString::fromRawData(data + 14, 2),
        eight = QString::fromRawData(data + 16, 2);
    QString fourDigit = one + two + three + four;
    QString fiveDigit = one + two  + separator + three + four + five;
    // Leading group can have single digit as long as there's a later separator:
    QString sixDigit =  one + separator  + two + three + separator + four + five + six;

    QString fourFloat = one + fractional + two + three + four;
    QString fiveFloat = one + two + fractional + three + four + five;
    QString sevenFloat = one + two + three + four + fractional + five + six + seven;

    QTest::newRow("Chakma-short") // {2,2,3}
            << u"ccp"_s << QString(separator) << zero
            << 1234 << fourDigit << 1.234 << fourFloat << 3;
    QTest::newRow("Chakma")
            << u"ccp"_s << QString(separator) << zero
            << 12345 << fiveDigit << 12.345 << fiveFloat << 3;
    QTest::newRow("Chakma-long")
            << u"ccp"_s << QString(separator) << zero
            << 123456 << sixDigit << 1234.567 << sevenFloat << 3;

    // Floating-point forms for ccp-MN, whose group separator is the fractional-part separator:
    // Leading "group" of four means too short to group, so rest is fractional part:
    QTest::newRow("ccp-MN-short")
            << u"ccp-MN"_s << QString(fractional) << zero
            << 1234 << fourDigit << 1234.567 << sevenFloat << 3;
    // Penultimate group of three implies final group must be fractional part:
    QString groupFloat = one + two + fractional + three + four + five
                         + fractional + six + seven + eight;
    QTest::newRow("ccp-MN")
            << u"ccp-MN"_s << QString(fractional) << zero
            << 12345 << fiveFloat << 12345.678 << groupFloat << 3;

    // Penultimate group of two implies rest must be grouping within the whole part:
    QString eightDigit = one + fractional + two + three + fractional + four + five
                         + fractional + six + seven + eight;
    QTest::newRow("ccp-MN-long")
            << u"ccp-MN"_s << QString(fractional) << zero
            << 12345678 << eightDigit << 12345678. << eightDigit << 0;
}

void tst_QLocaleData::systemGrouping()
{
    QFETCH(QString, name);
    QFETCH(QString, separator);
    QFETCH(QString, zeroDigit);
    QFETCH(int, whole);
    QFETCH(QString, formattedWhole);
    QFETCH(double, real);
    QFETCH(QString, formattedReal);
    QFETCH(int, precision);

    {
        MySystemLocale sLocale(name);
        QLocale sys = QLocale::system();
        QCOMPARE(sys.groupSeparator(), separator);
        QCOMPARE(sys.zeroDigit(), zeroDigit);

        QCOMPARE(sys.toString(whole), formattedWhole);
        bool ok;
        int count = sys.toInt(formattedWhole, &ok);
        QVERIFY2(ok, "Integer didn't round-trip");
        QCOMPARE(count, whole);

        QCOMPARE(sys.toString(real, 'f', precision), formattedReal);
        double apparent = sys.toDouble(formattedReal, &ok);
        QVERIFY2(ok, "Floating-precision didn't round-trip");
        QCOMPARE(apparent, real);
    }
}

#endif // QT_NO_SYSTEMLOCALE

QTEST_MAIN(tst_QLocaleData)
#include "tst_qlocaledata.moc"
