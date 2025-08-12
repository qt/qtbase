// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>
#include <QLocale>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <qnumeric.h>
#if QT_CONFIG(process)
#  include <QProcess>
#endif
#include <QScopedArrayPointer>
#include <QTimeZone>

#include <private/qlocale_p.h>
#include <private/qlocale_tools_p.h>
#include "../../../../shared/localechange.h"

#include <float.h>
#include <math.h>
#include <fenv.h>

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
#    include <stdlib.h>
#endif

using namespace Qt::StringLiterals;

Q_DECLARE_METATYPE(QLocale::FormatType)

// These platforms implement a locale-dependent case conversion
// (the others fall back to QString::toUpper/toLower()):
#if QT_CONFIG(icu) || defined(Q_OS_WIN) || defined(Q_OS_APPLE)
#  define QT_HAS_LOCALE_CASE_CONVERSION 1
#else
#  define QT_HAS_LOCALE_CASE_CONVERSION 0
#endif

class tst_QLocale : public QObject
{
    Q_OBJECT

public:
    tst_QLocale();

private slots:
    void initTestCase();
    void compareCompiles();
    void compareWithLanguage();
#if defined(Q_OS_WIN)
    void windowsDefaultLocale();
#endif
#ifdef Q_OS_DARWIN
    void macDefaultLocale();
#endif

    void ctor_data();
    void ctor();
    void ctor_match_land();
    void systemLocale_data();
    void systemLocale();
    void consistentC();
    void matchingLocales();

    void stringToDouble_data();
    void stringToDouble();
    void stringToFloat_data();
    void stringToFloat();
    void doubleToString_data();
    void doubleToString();
    void longlongToString_data();
    void longlongToString();
    void qulonglongToString_data();
    void qulonglongToString();
    void strtod_data();
    void strtod();
    void long_long_conversion_data();
    void long_long_conversion();
    void long_long_conversion_extra();
    void infNaN();
    void fpExceptions();
    void negativeZero_data();
    void negativeZero();

    void signsNeverCompareEqualToNullCharacter_data() { testNames_data(); }
    void signsNeverCompareEqualToNullCharacter();

    void dayOfWeek();
    void dayOfWeek_data();
    void formatDate();
    void formatDate_data();
    void formatTime();
    void formatTime_data();
    void formatDateTime();
    void formatDateTime_data();
    void formatTimeZone();
    void toDateTime_data();
    void toDateTime();
    void roundtripDateTimeFormat_data();
    void roundtripDateTimeFormat();
    void toDate_data();
    void toDate();
    void toTime_data();
    void toTime();

    void doubleRoundTrip_data();
    void doubleRoundTrip();
    void integerRoundTrip_data();
    void integerRoundTrip();
    void negativeNumbers();
    void numberOptions();
    void dayName_data();
    void dayName();
    void standaloneDayName_data();
    void standaloneDayName();
    void underflowOverflow();

    void dateFormat();
    void timeFormat();
    void dateTimeFormat();
    void monthName();
    void standaloneMonthName();

    void languageToString_data();
    void languageToString();
    void scriptToString_data();
    void scriptToString();
    void territoryToString_data();
    void territoryToString();
    void endonym_data();
    void endonym();

    void defaultNumberingSystem_data();
    void defaultNumberingSystem();

    void ampm_data();
    void ampm();
    void currency();
    void quoteString();
    void uiLanguages_data();
    void uiLanguages();
    void weekendDays();
    void listPatterns();

    void measurementSystems_data();
    void measurementSystems();
    void QTBUG_26035_positivesign();

    void textDirection_data();
    void textDirection();

    void formattedDataSize_data();
    void formattedDataSize();
    void bcp47Name_data();
    void bcp47Name();

#ifndef QT_NO_SYSTEMLOCALE
#  ifdef QT_BUILD_INTERNAL
    void mySystemLocale_data();
    void mySystemLocale();
    void systemGrouping_data();
    void systemGrouping();
#  endif

    void systemLocaleDayAndMonthNames_data();
    void systemLocaleDayAndMonthNames();
#endif

    void numberGrouping_data();
    void numberGrouping();
    void numberGroupingIndia();
    void numberFormatChakma();

    void lcsToCode();
    void codeToLcs();
    void codeToLang_data();
    void codeToLang();

#if QT_HAS_LOCALE_CASE_CONVERSION
    void toLowerUpper_data();
    void toLowerUpper();

    void toLowerUpperEszett();
#endif
    void toLowerUpperFinalSigma_data();
    void toLowerUpperFinalSigma();

    void defaulted_ctor();
    void legacyNames();
    void unixLocaleName_data();
    void unixLocaleName();
    void testNames_data();
    void testNames();
    void debugOutput();
private:
    QString m_decimal, m_thousand, m_sdate, m_ldate, m_time;
    QString m_sysapp;
    QStringList cleanEnv;
    const bool europeanTimeZone;
    void toReal_data();

    using TransientLocale = QTestLocaleChange::TransientLocale;
};

tst_QLocale::tst_QLocale()
    // Some tests are specific to CET, test if it applies:
    : europeanTimeZone(
        QDate(2013, 1, 1).startOfDay().offsetFromUtc() == 3600
        && QDate(2013, 6, 1).startOfDay().offsetFromUtc() == 7200
        // ICU in a zone not currently doing DST may ignore any historical DST
        // excursions in its display-names (Africa/Tripoli).
        && QDate(QDate::currentDate().year(), 1, 1).startOfDay().offsetFromUtc() == 3600
        && QDate(QDate::currentDate().year(), 7, 1).startOfDay().offsetFromUtc() == 7200)
{
    qRegisterMetaType<QLocale::FormatType>("QLocale::FormatType");
}

void tst_QLocale::initTestCase()
{
#ifdef Q_OS_ANDROID
    // We can't start a QProcess on Android, and we anyway skip the test
    // that uses m_sysapp. So no need to initialize it properly.
    return;
#elif QT_CONFIG(process)
    const QString syslocaleapp_dir = QFINDTESTDATA("syslocaleapp");
    QVERIFY2(!syslocaleapp_dir.isEmpty(),
            qPrintable(QStringLiteral("Cannot find 'syslocaleapp' starting from ")
                       + QDir::toNativeSeparators(QDir::currentPath())));
    m_sysapp = syslocaleapp_dir + QStringLiteral("/syslocaleapp");
#ifdef Q_OS_WIN
    m_sysapp += QStringLiteral(".exe");
#endif
    const QFileInfo fi(m_sysapp);
    QVERIFY2(fi.exists() && fi.isExecutable(),
             qPrintable(QDir::toNativeSeparators(m_sysapp)
                        + QStringLiteral(" does not exist or is not executable.")));

    // Get an environment free of any locale-related variables
    cleanEnv.clear();
    const QStringList sysenv = QProcess::systemEnvironment();
    for (const QString &entry : sysenv) {
        if (entry.startsWith("LANG=") || entry.startsWith("LC_") || entry.startsWith("LANGUAGE="))
            continue;
        cleanEnv << entry;
    }
#endif // QT_CONFIG(process)
}

void tst_QLocale::compareCompiles()
{
    QTestPrivate::testEqualityOperatorsCompile<QLocale>();
    QTestPrivate::testEqualityOperatorsCompile<QLocale, QLocale::Language>();
}

void tst_QLocale::compareWithLanguage()
{
    QLocale de(QLocale::German);
    QT_TEST_EQUALITY_OPS(de, QLocale::German, true);
    QT_TEST_EQUALITY_OPS(de, QLocale::English, false);

    QLocale en_DE(QLocale::English, QLocale::Germany);
    QCOMPARE_EQ(en_DE.language(), QLocale::English);
    QCOMPARE_EQ(en_DE.territory(), QLocale::Germany);
    // Territory won't match
    QT_TEST_EQUALITY_OPS(en_DE, QLocale::English, false);
}

void tst_QLocale::ctor_data()
{
    QTest::addColumn<QLocale::Language>("reqLang");
    QTest::addColumn<QLocale::Script>("reqText");
    QTest::addColumn<QLocale::Territory>("reqLand");
    QTest::addColumn<QLocale::Language>("expLang");
    QTest::addColumn<QLocale::Script>("expText");
    QTest::addColumn<QLocale::Territory>("expLand");

    // Exact match
#define ECHO(name, lang, text, land) \
    QTest::newRow(name) \
        << QLocale::lang << QLocale::text << QLocale::land \
        << QLocale::lang << QLocale::text << QLocale::land

    ECHO("zh_Hans_CN", Chinese, SimplifiedHanScript, China);
    ECHO("zh_Hant_TW", Chinese, TraditionalHanScript, Taiwan);
    ECHO("zh_Hant_HK", Chinese, TraditionalHanScript, HongKong);
    ECHO("en_POSIX", C, AnyScript, AnyTerritory);
#undef ECHO

    // Determine territory from language and script:
#define WHATLAND(name, lang, text, land)         \
    QTest::newRow(name) \
        << QLocale::lang << QLocale::text << QLocale::AnyTerritory \
        << QLocale::lang << QLocale::text << QLocale::land

    WHATLAND("zh_Hans", Chinese, SimplifiedHanScript, China);
    WHATLAND("zh_Hant", Chinese, TraditionalHanScript, Taiwan);
#undef WHATLAND

    // Determine script from language and territory:
#define WHATTEXT(name, lang, text, land) \
    QTest::newRow(name) \
        << QLocale::lang << QLocale::AnyScript << QLocale::land \
        << QLocale::lang << QLocale::text << QLocale::land

    WHATTEXT("zh_CN", Chinese, SimplifiedHanScript, China);
    WHATTEXT("zh_TW", Chinese, TraditionalHanScript, Taiwan);
    WHATTEXT("zh_HK", Chinese, TraditionalHanScript, HongKong);
#undef WHATTEXT

    // No exact match, fix by change of territory:
#define FIXLAND(name, lang, text, land, fixed) \
    QTest::newRow(name) \
        << QLocale::lang << QLocale::text << QLocale::land \
        << QLocale::lang << QLocale::text << QLocale::fixed

    FIXLAND("zh_Hans_TW", Chinese, SimplifiedHanScript, Taiwan, China);
    FIXLAND("zh_Hans_US", Chinese, SimplifiedHanScript, UnitedStates, China);
    FIXLAND("zh_Hant_CN", Chinese, TraditionalHanScript, China, Taiwan);
    FIXLAND("zh_Hant_US", Chinese, TraditionalHanScript, UnitedStates, Taiwan);
#undef FIXLAND

    // No exact match, fix by change of script:
#define FIXTEXT(name, lang, text, land, fixed) \
    QTest::newRow(name) \
        << QLocale::lang << QLocale::text << QLocale::land \
        << QLocale::lang << QLocale::fixed << QLocale::land

    FIXTEXT("zh_Taml_CN", Chinese, TamilScript, China, SimplifiedHanScript);
    FIXTEXT("zh_Taml_TW", Chinese, TamilScript, Taiwan, TraditionalHanScript);
#undef FIXTEXT

    // No exact match, preserve language:
#define KEEPLANG(name, lang, text, land, fixtext, fixland)  \
    QTest::newRow(name) \
        << QLocale::lang << QLocale::text << QLocale::land \
        << QLocale::lang << QLocale::fixtext << QLocale::fixland

    KEEPLANG("zh_US", Chinese, AnyScript, UnitedStates, SimplifiedHanScript, China);
    KEEPLANG("zh_Taml_US", Chinese, TamilScript, UnitedStates, SimplifiedHanScript, China);
#undef KEEPLANG

    // Only territory - likely subtags imply language and script:
#define LANDFILL(name, lang, text, land) \
    QTest::newRow(name) \
        << QLocale::AnyLanguage << QLocale::AnyScript << QLocale::land \
        << QLocale::lang << QLocale::text << QLocale::land

    LANDFILL("und_CN", Chinese, SimplifiedHanScript, China);
    LANDFILL("und_TW", Chinese, TraditionalHanScript, Taiwan);
    LANDFILL("und_CA", English, LatinScript, Canada);
    LANDFILL("und_US", English, LatinScript, UnitedStates);
    LANDFILL("und_GB", English, LatinScript, UnitedKingdom);
    LANDFILL("und_POSIX", C, AnyScript, AnyTerritory);
#undef LANDFILL
}

void tst_QLocale::ctor()
{
    QFETCH(const QLocale::Language, reqLang);
    QFETCH(const QLocale::Script, reqText);
    QFETCH(const QLocale::Territory, reqLand);

    const QLatin1String request(QTest::currentDataTag());
    if (request != "und_POSIX"_L1) {
        const QLocale l(reqLang, reqText, reqLand);
        QTEST(l.language(), "expLang");
        QTEST(l.script(), "expText");
        QTEST(l.territory(), "expLand");
    }
    if (!request.startsWith("und_"_L1) || request.endsWith("_POSIX"_L1)) {
        const QLocale l(request);
        QTEST(l.language(), "expLang");
        QTEST(l.script(), "expText");
        QTEST(l.territory(), "expLand");
    }
}

void tst_QLocale::ctor_match_land()
{
    // QTBUG-64940: QLocale(Any, Any, land).territory() should normally be land:
    constexpr QLocale::Territory exceptions[] = {
        // There are, however, some exceptions:
        QLocale::AmericanSamoa,
        QLocale::Antarctica,
        QLocale::AscensionIsland,
        QLocale::BouvetIsland,
        QLocale::CaribbeanNetherlands,
        QLocale::ClippertonIsland,
        QLocale::Curacao,
        QLocale::Europe,
        QLocale::EuropeanUnion,
        QLocale::FrenchSouthernTerritories,
        QLocale::Haiti,
        QLocale::HeardAndMcDonaldIslands,
        QLocale::OutlyingOceania,
        QLocale::Palau,
        QLocale::Samoa,
        QLocale::SouthGeorgiaAndSouthSandwichIslands,
        QLocale::TokelauTerritory,
        QLocale::TristanDaCunha,
        QLocale::TuvaluTerritory,
        QLocale::Vanuatu
    };
    for (int i = int(QLocale::AnyTerritory) + 1; i <= int(QLocale::LastTerritory); ++i) {
        const auto land = QLocale::Territory(i);
        if (std::find(std::begin(exceptions), std::end(exceptions), land) != std::end(exceptions))
            continue;
        QCOMPARE(QLocale(QLocale::AnyLanguage, QLocale::AnyScript, land).territory(), land);
    }
}

void tst_QLocale::defaulted_ctor()
{
    QLocale priorDefault;
    const auto restoreDefault = qScopeGuard([priorDefault]() {
        QLocale::setDefault(priorDefault);
    });
    QLocale::Language defaultLanguage = priorDefault.language();
    QLocale::Territory defaultTerritory = priorDefault.territory();

    qDebug("Default: %s/%s", QLocale::languageToString(defaultLanguage).toUtf8().constData(),
            QLocale::territoryToString(defaultTerritory).toUtf8().constData());

    {
        QLocale l;
        QCOMPARE(l.language(), defaultLanguage);
        QCOMPARE(l.territory(), defaultTerritory);
    }

    {
        QLocale l(QLocale::C, QLocale::AnyTerritory);
        QCOMPARE(l.language(), QLocale::C);
        QCOMPARE(l.territory(), QLocale::AnyTerritory);
    }

#define CHECK_DEFAULT(lang, terr) \
    do { \
        const QLocale l; \
        QCOMPARE(l.language(), lang); \
        QCOMPARE(l.territory(), terr); \
    } while (false)

#define TEST_CTOR(req_lang, req_country, exp_lang, exp_country) \
    do { \
        const QLocale l(QLocale::req_lang, QLocale::req_country); \
        QCOMPARE(l.language(), exp_lang); \
        QCOMPARE(l.territory(), exp_country); \
    } while (false)

    TEST_CTOR(AnyLanguage, AnyTerritory, QLocale::English, QLocale::UnitedStates);
    TEST_CTOR(C, AnyTerritory, QLocale::C, QLocale::AnyTerritory);
    TEST_CTOR(Aymara, AnyTerritory, defaultLanguage, defaultTerritory);
    TEST_CTOR(Aymara, France, defaultLanguage, defaultTerritory);

    TEST_CTOR(English, AnyTerritory, QLocale::English, QLocale::UnitedStates);
    TEST_CTOR(English, UnitedStates, QLocale::English, QLocale::UnitedStates);
    TEST_CTOR(English, France, QLocale::English, QLocale::France);
    TEST_CTOR(English, UnitedKingdom, QLocale::English, QLocale::UnitedKingdom);
    // Used in tests below to check we pick the likely-best substitute consistently:
    TEST_CTOR(Arabic, UnitedStates, QLocale::Arabic, QLocale::Egypt);

    TEST_CTOR(French, France, QLocale::French, QLocale::France);
    TEST_CTOR(C, France, QLocale::C, QLocale::AnyTerritory);
    TEST_CTOR(Spanish, LatinAmerica, QLocale::Spanish,
              QLocale::LatinAmerica);

    QLocale::setDefault(QLocale(QLocale::Arabic, QLocale::UnitedStates));
    CHECK_DEFAULT(QLocale::Arabic, QLocale::Egypt);

    TEST_CTOR(French, France, QLocale::French, QLocale::France);
    TEST_CTOR(English, UnitedKingdom, QLocale::English, QLocale::UnitedKingdom);

    TEST_CTOR(French, France, QLocale::French, QLocale::France);
    TEST_CTOR(C, AnyTerritory, QLocale::C, QLocale::AnyTerritory);
    TEST_CTOR(C, France, QLocale::C, QLocale::AnyTerritory);
    TEST_CTOR(Aymara, AnyTerritory, QLocale::Arabic, QLocale::Egypt);

    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedKingdom));
    CHECK_DEFAULT(QLocale::English, QLocale::UnitedKingdom);

    TEST_CTOR(French, France, QLocale::French, QLocale::France);
    TEST_CTOR(English, UnitedKingdom, QLocale::English, QLocale::UnitedKingdom);

    TEST_CTOR(C, AnyTerritory, QLocale::C, QLocale::AnyTerritory);
    TEST_CTOR(C, France, QLocale::C, QLocale::AnyTerritory);

    QLocale::setDefault(QLocale(QLocale::Aymara, QLocale::France));
    CHECK_DEFAULT(QLocale::English, QLocale::UnitedKingdom);

    TEST_CTOR(Aymara, AnyTerritory, QLocale::English, QLocale::UnitedKingdom);
    TEST_CTOR(Aymara, France, QLocale::English, QLocale::UnitedKingdom);

    TEST_CTOR(English, AnyTerritory, QLocale::English, QLocale::UnitedStates);
    TEST_CTOR(English, UnitedStates, QLocale::English, QLocale::UnitedStates);
    TEST_CTOR(English, France, QLocale::English, QLocale::France);
    TEST_CTOR(English, UnitedKingdom, QLocale::English, QLocale::UnitedKingdom);
    TEST_CTOR(Arabic, UnitedStates, QLocale::Arabic, QLocale::Egypt);

    TEST_CTOR(French, France, QLocale::French, QLocale::France);
    TEST_CTOR(C, AnyTerritory, QLocale::C, QLocale::AnyTerritory);
    TEST_CTOR(C, France, QLocale::C, QLocale::AnyTerritory);

    QLocale::setDefault(QLocale(QLocale::Aymara, QLocale::AnyTerritory));
    CHECK_DEFAULT(QLocale::English, QLocale::UnitedKingdom);

    TEST_CTOR(Aymara, AnyTerritory, QLocale::English, QLocale::UnitedKingdom);
    TEST_CTOR(Aymara, France, QLocale::English, QLocale::UnitedKingdom);

    TEST_CTOR(English, AnyTerritory, QLocale::English, QLocale::UnitedStates);
    TEST_CTOR(English, UnitedStates, QLocale::English, QLocale::UnitedStates);
    TEST_CTOR(English, France, QLocale::English, QLocale::France);
    TEST_CTOR(English, UnitedKingdom, QLocale::English, QLocale::UnitedKingdom);
    TEST_CTOR(Arabic, UnitedStates, QLocale::Arabic, QLocale::Egypt);

    TEST_CTOR(French, France, QLocale::French, QLocale::France);
    TEST_CTOR(C, AnyTerritory, QLocale::C, QLocale::AnyTerritory);
    TEST_CTOR(C, France, QLocale::C, QLocale::AnyTerritory);

    TEST_CTOR(Arabic, AnyTerritory, QLocale::Arabic, QLocale::Egypt);
    TEST_CTOR(Dutch, AnyTerritory, QLocale::Dutch, QLocale::Netherlands);
    TEST_CTOR(German, AnyTerritory, QLocale::German, QLocale::Germany);
    TEST_CTOR(Greek, AnyTerritory, QLocale::Greek, QLocale::Greece);
    TEST_CTOR(Malay, AnyTerritory, QLocale::Malay, QLocale::Malaysia);
    TEST_CTOR(Persian, AnyTerritory, QLocale::Persian, QLocale::Iran);
    TEST_CTOR(Portuguese, AnyTerritory, QLocale::Portuguese, QLocale::Brazil);
    TEST_CTOR(Serbian, AnyTerritory, QLocale::Serbian, QLocale::Serbia);
    TEST_CTOR(Somali, AnyTerritory, QLocale::Somali, QLocale::Somalia);
    TEST_CTOR(Spanish, AnyTerritory, QLocale::Spanish, QLocale::Spain);
    TEST_CTOR(Swedish, AnyTerritory, QLocale::Swedish, QLocale::Sweden);
    TEST_CTOR(Uzbek, AnyTerritory, QLocale::Uzbek, QLocale::Uzbekistan);

#undef CHECK_DEFAULT
#undef TEST_CTOR
#define TEST_CTOR(req_lc, exp_lang, exp_country) \
    do { \
        const QLocale l(req_lc); \
        QCOMPARE(l.language(), QLocale::exp_lang); \
        QCOMPARE(l.territory(), QLocale::exp_country); \
        const QLocale m(QLocale::exp_lang, QLocale::exp_country); \
        QCOMPARE(l, m); \
        QCOMPARE(qHash(l), qHash(m)); \
    } while (false)

    const QString empty;

    TEST_CTOR("C", C, AnyTerritory);
    TEST_CTOR("bla", C, AnyTerritory);
    TEST_CTOR("zz", C, AnyTerritory);
    TEST_CTOR("zz_zz", C, AnyTerritory);
    TEST_CTOR("zz...", C, AnyTerritory);
    TEST_CTOR("", C, AnyTerritory);
    TEST_CTOR("en/", C, AnyTerritory);
    TEST_CTOR(empty, C, AnyTerritory);
    TEST_CTOR("en", English, UnitedStates);
    TEST_CTOR("en", English, UnitedStates);
    TEST_CTOR("en.", English, UnitedStates);
    TEST_CTOR("en@", English, UnitedStates);
    TEST_CTOR("en.@", English, UnitedStates);
    TEST_CTOR("en_", English, UnitedStates);
    TEST_CTOR("en_U", English, UnitedStates);
    TEST_CTOR("en_.", English, UnitedStates);
    TEST_CTOR("en_.@", English, UnitedStates);
    TEST_CTOR("en.bla", English, UnitedStates);
    TEST_CTOR("en@bla", English, UnitedStates);
    TEST_CTOR("en_blaaa", English, UnitedStates);
    TEST_CTOR("en_zz", English, UnitedStates);
    TEST_CTOR("en_GB", English, UnitedKingdom);
    TEST_CTOR("en_GB.bla", English, UnitedKingdom);
    TEST_CTOR("en_GB@.bla", English, UnitedKingdom);
    TEST_CTOR("en_GB@bla", English, UnitedKingdom);
    TEST_CTOR("en-GB", English, UnitedKingdom);
    TEST_CTOR("en-GB@bla", English, UnitedKingdom);
    TEST_CTOR("eo", Esperanto, World);
    TEST_CTOR("yi", Yiddish, Ukraine);

    TEST_CTOR("no", NorwegianBokmal, Norway);
    TEST_CTOR("nb", NorwegianBokmal, Norway);
    TEST_CTOR("nn", NorwegianNynorsk, Norway);
    TEST_CTOR("no_NO", NorwegianBokmal, Norway);
    TEST_CTOR("nb_NO", NorwegianBokmal, Norway);
    TEST_CTOR("nn_NO", NorwegianNynorsk, Norway);
    TEST_CTOR("es_ES", Spanish, Spain);
    TEST_CTOR("es_419", Spanish, LatinAmerica);
    TEST_CTOR("es-419", Spanish, LatinAmerica);
    TEST_CTOR("fr_MA", French, Morocco);

    // test default countries for languages
    TEST_CTOR("zh", Chinese, China);
    TEST_CTOR("zh-Hans", Chinese, China);
    TEST_CTOR("ne", Nepali, Nepal);

#undef TEST_CTOR
#define TEST_CTOR(req_lc, exp_lang, exp_script, exp_country) \
    do { \
        const QLocale l(req_lc); \
        QCOMPARE(l.language(), QLocale::exp_lang); \
        QCOMPARE(l.script(), QLocale::exp_script); \
        QCOMPARE(l.territory(), QLocale::exp_country); \
    } while (false)

    TEST_CTOR("zh_CN", Chinese, SimplifiedHanScript, China);
    TEST_CTOR("zh_Hans_CN", Chinese, SimplifiedHanScript, China);
    TEST_CTOR("zh_Hans", Chinese, SimplifiedHanScript, China);
    TEST_CTOR("zh_Hant", Chinese, TraditionalHanScript, Taiwan);
    TEST_CTOR("zh_Hans_MO", Chinese, SimplifiedHanScript, Macau);
    TEST_CTOR("zh_Hant_MO", Chinese, TraditionalHanScript, Macau);
    TEST_CTOR("az_Latn_AZ", Azerbaijani, LatinScript, Azerbaijan);
    TEST_CTOR("ha_NG", Hausa, LatinScript, Nigeria);

    TEST_CTOR("ru", Russian, CyrillicScript, RussianFederation);
    TEST_CTOR("ru_Cyrl", Russian, CyrillicScript, RussianFederation);

#undef TEST_CTOR
}

#if QT_CONFIG(process)
static inline bool runSysApp(const QString &binary,
                             const QStringList &args,
                             const QStringList &env,
                             QString *output,
                             QString *errorMessage)
{
    output->clear();
    errorMessage->clear();
    QProcess process;
    process.setEnvironment(env);
    process.start(binary, args);
    process.closeWriteChannel();
    if (!process.waitForStarted()) {
        *errorMessage = QLatin1String("Cannot start '") + binary
            + QLatin1String("': ") + process.errorString();
        return false;
    }
    if (!process.waitForFinished()) {
        process.kill();
        *errorMessage = QStringLiteral("Timeout waiting for ") + binary;
        return false;
    }
    *output = QString::fromLocal8Bit(process.readAllStandardOutput());
    return true;
}

static inline bool runSysAppTest(const QString &binary,
                                 QStringList baseEnv,
                                 const QString &requestedLocale,
                                 const QString &expectedOutput,
                                 QString *errorMessage)
{
    QString output;
    QStringList args;
#ifdef Q_OS_MACOS
    args << "-AppleLocale" << requestedLocale;
#endif
    baseEnv.append(QStringLiteral("LANG=") + requestedLocale);
    if (!runSysApp(binary, args, baseEnv, &output, errorMessage))
        return false;

    if (output.isEmpty()) {
        *errorMessage += QLatin1String("Empty output received for requested '") + requestedLocale
            + QLatin1String("' (expected '") + expectedOutput + QLatin1String("')");
        return false;
    }
    if (output != expectedOutput) {
        *errorMessage += QLatin1String("Output mismatch for requested '") + requestedLocale
            + QLatin1String("': Expected '") + expectedOutput + QLatin1String("', got '")
            + output + QLatin1String("'");
        return false;
    }
    return true;
}
#endif

void tst_QLocale::systemLocale_data()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support");
#endif
#ifdef Q_OS_ANDROID
    QSKIP("Can't start QProcess to run a custom user binary on Android");
#endif

    QTest::addColumn<QString>("expected");

#if QT_CONFIG(jalalicalendar)
#define ADD_CTOR_TEST(input, localePart, monthName) \
       QTest::newRow(input) << QStringLiteral(localePart) + " "_L1 + QStringLiteral(monthName);
#else
#define ADD_CTOR_TEST(input, localePart, monthName) \
       QTest::newRow(input) << QStringLiteral(localePart);
#endif

    // For format and meaning, see:
    // http://pubs.opengroup.org/onlinepubs/7908799/xbd/envvar.html
    // Note that the accepted values for fields are implementation-dependent;
    // the template is language[_territory][.codeset][@modifier]

    // "Ordibehesht" is the name (as adapted to English, German or Norsk) of the
    // second month of the Jalali calendar. If you see anything in Arabic,
    // setDefault(Persian) has interfered with the system locale setup.

    // Vanilla:
    ADD_CTOR_TEST("C", "C", "Ordibehesht");

    // Standard forms:
    ADD_CTOR_TEST("en", "en_US", "Ordibehesht");
    ADD_CTOR_TEST("en_GB", "en_GB", "Ordibehesht");
    ADD_CTOR_TEST("de", "de_DE", "Ordibehescht");
    // Norsk has some quirks:
    ADD_CTOR_TEST("no", "nb_NO", "ordibehesht");
    ADD_CTOR_TEST("nb", "nb_NO", "ordibehesht");
    ADD_CTOR_TEST("nn", "nn_NO", "ordibehesht");
    ADD_CTOR_TEST("no_NO", "nb_NO",  "ordibehesht");
    ADD_CTOR_TEST("nb_NO", "nb_NO", "ordibehesht");
    ADD_CTOR_TEST("nn_NO", "nn_NO", "ordibehesht");

    // Not too fussy about case:
    ADD_CTOR_TEST("DE", "de_DE", "Ordibehescht");
    ADD_CTOR_TEST("EN", "en_US", "Ordibehesht");

    // Invalid fields
    ADD_CTOR_TEST("bla", "C", "Ordibehesht");
    ADD_CTOR_TEST("zz", "C", "Ordibehesht");
    ADD_CTOR_TEST("zz_zz", "C", "Ordibehesht");
    ADD_CTOR_TEST("zz...", "C", "Ordibehesht");
    ADD_CTOR_TEST("en.bla", "en_US", "Ordibehesht");
    ADD_CTOR_TEST("en@bla", "en_US", "Ordibehesht");
    ADD_CTOR_TEST("en_blaaa", "en_US", "Ordibehesht");
    ADD_CTOR_TEST("en_zz", "en_US", "Ordibehesht");
    ADD_CTOR_TEST("en_GB.bla", "en_GB", "Ordibehesht");
    ADD_CTOR_TEST("en_GB@.bla", "en_GB", "Ordibehesht");
    ADD_CTOR_TEST("en_GB@bla", "en_GB", "Ordibehesht");

    // Empty optional fields, but with punctuators supplied
    ADD_CTOR_TEST("en.", "en_US", "Ordibehesht");
    ADD_CTOR_TEST("en@", "en_US", "Ordibehesht");
    ADD_CTOR_TEST("en.@", "en_US", "Ordibehesht");
    ADD_CTOR_TEST("en_", "en_US", "Ordibehesht");
    ADD_CTOR_TEST("en_.", "en_US", "Ordibehesht");
    ADD_CTOR_TEST("en_.@", "en_US", "Ordibehesht");
#undef ADD_CTOR_TEST

#if QT_CONFIG(process) // for runSysApp
    // Get default locale.
    QString defaultLoc;
    QString errorMessage;
    if (runSysApp(m_sysapp, QStringList(), cleanEnv, &defaultLoc, &errorMessage)) {
#if defined(Q_OS_MACOS)
#if QT_CONFIG(jalalicalendar)
        QString localeForInvalidLocale = "C Ordibehesht";
#else
        QString localeForInvalidLocale = "C";
#endif // QT_CONFIG(jalalicalendar)
#else
        QString localeForInvalidLocale = defaultLoc;
#endif
#define ADD_CTOR_TEST(give) QTest::newRow(give) << localeForInvalidLocale;
        ADD_CTOR_TEST("en/");
        ADD_CTOR_TEST("asdfghj");
        ADD_CTOR_TEST("123456");
#undef ADD_CTOR_TEST
    } else {
        qDebug() << "Skipping tests based on default locale" << qPrintable(errorMessage);
    }
#endif // process
}

void tst_QLocale::systemLocale()
{
#if QT_CONFIG(process) // for runSysAppTest
    QLatin1String request(QTest::currentDataTag());
    QFETCH(QString, expected);

    // Test constructor without arguments (see syslocaleapp/syslocaleapp.cpp)
    // Needs separate process because of caching of the system locale.
    QString errorMessage;
    QVERIFY2(runSysAppTest(m_sysapp, cleanEnv, request, expected, &errorMessage),
             qPrintable(errorMessage));

#else
    // This won't be called, as _data() skipped out early.
#endif // process
}

void tst_QLocale::legacyNames()
{
#define TEST_CTOR(req_lc, exp_lang, exp_country) \
    do { \
        const QLocale l(req_lc); \
        QCOMPARE(l.language(), QLocale::exp_lang); \
        QCOMPARE(l.territory(), QLocale::exp_country); \
    } while (false)

    TEST_CTOR("mo_MD", Romanian, Moldova);
    TEST_CTOR("no", NorwegianBokmal, Norway);
    TEST_CTOR("sh_ME", Serbian, Montenegro);
    TEST_CTOR("tl", Filipino, Philippines);
    TEST_CTOR("iw", Hebrew, Israel);
    TEST_CTOR("in", Indonesian, Indonesia);
#undef TEST_CTOR
}

void tst_QLocale::consistentC()
{
    const QLocale c(QLocale::C);
    QT_TEST_EQUALITY_OPS(c, QLocale::c(), true);
    QT_TEST_EQUALITY_OPS(c, QLocale(QLocale::C, QLocale::AnyScript, QLocale::AnyTerritory), true);
    QVERIFY(QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript,
                                     QLocale::AnyTerritory).contains(c));
}

void tst_QLocale::matchingLocales()
{
    const QLocale c(QLocale::C);
    const QLocale ru_RU(QLocale::Russian, QLocale::Russia);
    QT_TEST_EQUALITY_OPS(c, ru_RU, false);

    QList<QLocale> locales = QLocale::matchingLocales(QLocale::C, QLocale::AnyScript, QLocale::AnyTerritory);
    QCOMPARE(locales.size(), 1);
    QVERIFY(locales.contains(c));

    locales = QLocale::matchingLocales(QLocale::Russian, QLocale::CyrillicScript, QLocale::Russia);
    QCOMPARE(locales.size(), 1);
    QVERIFY(locales.contains(ru_RU));

    locales = QLocale::matchingLocales(QLocale::Russian, QLocale::AnyScript, QLocale::AnyTerritory);
    QVERIFY(!locales.isEmpty());
    QVERIFY(!locales.contains(c));
    QVERIFY(locales.contains(ru_RU));

    locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::CyrillicScript, QLocale::AnyTerritory);
    QVERIFY(!locales.isEmpty());
    QVERIFY(!locales.contains(c));
    QVERIFY(locales.contains(ru_RU));

    locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::Russia);
    QVERIFY(!locales.isEmpty());
    QVERIFY(!locales.contains(c));
    QVERIFY(locales.contains(ru_RU));

    // Regression check for assertion failure when no locales match:
    locales = QLocale::matchingLocales(QLocale::Abkhazian, QLocale::AnyScript, QLocale::AnyTerritory);
    // Empty in CLDR v39, but don't require that.
    QVERIFY(!locales.contains(c));
    QVERIFY(!locales.contains(ru_RU));
}

void tst_QLocale::unixLocaleName_data()
{
    QTest::addColumn<QLocale::Language>("lang");
    QTest::addColumn<QLocale::Territory>("land");
    QTest::addColumn<QString>("expect");

#define ADDROW(nom, lang, land, name) \
    QTest::newRow(nom) << QLocale::lang << QLocale::land << QStringLiteral(name)

    ADDROW("C_any", C, AnyTerritory, "C");
    ADDROW("en_any", English, AnyTerritory, "en_US");
    ADDROW("en_GB", English, UnitedKingdom, "en_GB");
#undef ADDROW
    QTest::newRow("ay_GB") << QLocale::Aymara << QLocale::UnitedKingdom << QLocale().name();
}

void tst_QLocale::unixLocaleName()
{
    QFETCH(const QLocale::Language, lang);
    QFETCH(const QLocale::Territory, land);
    QFETCH(const QString, expect);
    const auto expected = [expect](QChar ch) {
        // Kludge around QString::replace() not being const.
        QString copy = expect;
        return copy.replace(u'_', ch);
    };

    const QLocale locale(lang, land);
    QCOMPARE(locale.name(), expect);
    QCOMPARE(locale.name(QLocale::TagSeparator::Dash), expected(u'-'));
    QCOMPARE(locale.name(QLocale::TagSeparator{'|'}), expected(u'|'));
    QTest::ignoreMessage(QtWarningMsg, "QLocale::name(): "
                         "Using non-ASCII separator '\u00ff' (ff) is unsupported");
    QCOMPARE(locale.name(QLocale::TagSeparator{'\xff'}), QString());
}

void tst_QLocale::toReal_data()
{
    QTest::addColumn<QString>("locale_name");
    QTest::addColumn<QString>("num_str");
    QTest::addColumn<bool>("good");
    QTest::addColumn<double>("num");

    QTest::newRow("C 1")               << QString("C") << QString("1")               << true  << 1.0;
    QTest::newRow("C 1.0")             << QString("C") << QString("1.0")             << true  << 1.0;
    QTest::newRow("C 1.234")           << QString("C") << QString("1.234")           << true  << 1.234;
    QTest::newRow("C 1.234e-10")       << QString("C") << QString("1.234e-10")       << true  << 1.234e-10;
    QTest::newRow("C 1.234E10")        << QString("C") << QString("1.234E10")        << true  << 1.234e10;
    QTest::newRow("C 1e10")            << QString("C") << QString("1e10")            << true  << 1.0e10;
    QTest::newRow("C 1e310")           << QString("C") << QString("1e310")           << false << std::numeric_limits<double>::infinity();
    QTest::newRow("C 1E310")           << QString("C") << QString("1E310")           << false << std::numeric_limits<double>::infinity();
    QTest::newRow("C  1")              << QString("C") << QString(" 1")              << true  << 1.0;
    QTest::newRow("C   1")             << QString("C") << QString("  1")             << true  << 1.0;
    QTest::newRow("C 1 ")              << QString("C") << QString("1 ")              << true  << 1.0;
    QTest::newRow("C 1  ")             << QString("C") << QString("1  ")             << true  << 1.0;

    QTest::newRow("C 1,")              << QString("C") << QString("1,")              << false << 0.0;
    QTest::newRow("C 1,2")             << QString("C") << QString("1,2")             << false << 0.0;
    QTest::newRow("C 1,23")            << QString("C") << QString("1,23")            << false << 0.0;
    QTest::newRow("C 1,234")           << QString("C") << QString("1,234")           << true  << 1234.0;
    QTest::newRow("C 1,234,")          << QString("C") << QString("1,234,")          << false << 0.0;
    QTest::newRow("C 1,234,5")         << QString("C") << QString("1,234,5")         << false << 0.0;
    QTest::newRow("C 1,234,56")        << QString("C") << QString("1,234,56")        << false << 0.0;
    QTest::newRow("C 1,234,567")       << QString("C") << QString("1,234,567")       << true  << 1234567.0;
    QTest::newRow("C 1,234,567.")      << QString("C") << QString("1,234,567.")      << true  << 1234567.0;
    QTest::newRow("C 1,234,567.8")     << QString("C") << QString("1,234,567.8")     << true  << 1234567.8;
    QTest::newRow("C 1,234567.8")      << QString("C") << QString("1,234567.8")      << false << 0.0;
    QTest::newRow("C 12,34567.8")      << QString("C") << QString("12,34567.8")      << false << 0.0;
    QTest::newRow("C 1234,567.8")      << QString("C") << QString("1234,567.8")      << false << 0.0;
    QTest::newRow("C 1234567.8")       << QString("C") << QString("1234567.8")       << true  << 1234567.8;
    QTest::newRow("C ,")               << QString("C") << QString(",")               << false << 0.0;
    QTest::newRow("C ,123")            << QString("C") << QString(",123")            << false << 0.0;
    QTest::newRow("C ,3")              << QString("C") << QString(",3")              << false << 0.0;
    QTest::newRow("C , 3")             << QString("C") << QString(", 3")             << false << 0.0;
    QTest::newRow("C ,  3")            << QString("C") << QString(",  3")            << false << 0.0;
    QTest::newRow("C ,  3.2")          << QString("C") << QString(",  3.2")          << false << 0.0;
    QTest::newRow("C ,  3.2e2")        << QString("C") << QString(",  3.2e2")        << false << 0.0;
    QTest::newRow("C ,  e2")           << QString("C") << QString(",  e2")           << false << 0.0;
    QTest::newRow("C 1,,234")          << QString("C") << QString("1,,234")          << false << 0.0;

    QTest::newRow("C empty")           << QString("C") << QString("")                << false << 0.0;
    QTest::newRow("C null")            << QString("C") << QString()                  << false << 0.0;
    QTest::newRow("C .")               << QString("C") << QString(".")               << false << 0.0;
    QTest::newRow("C 1e")              << QString("C") << QString("1e")              << false << 0.0;
    QTest::newRow("C 1,0")             << QString("C") << QString("1,0")             << false << 0.0;
    QTest::newRow("C 1,000")           << QString("C") << QString("1,000")           << true  << 1000.0;
    QTest::newRow("C 1,000e-6")        << QString("C") << QString("1,000e-6")        << true  << 1000.0e-6;
    QTest::newRow("C 1e1.0")           << QString("C") << QString("1e1.0")           << false << 0.0;
    QTest::newRow("C 1e+")             << QString("C") << QString("1e+")             << false << 0.0;
    QTest::newRow("C 1e-")             << QString("C") << QString("1e-")             << false << 0.0;

    QTest::newRow("C .1")              << QString("C") << QString(".1")              << true  << 0.1;
    QTest::newRow("C -.1")             << QString("C") << QString("-.1")             << true  << -0.1;
    QTest::newRow("C 1.")              << QString("C") << QString("1.")              << true  << 1.0;
    QTest::newRow("C 1.E10")           << QString("C") << QString("1.E10")           << true  << 1.0e10;
    QTest::newRow("C 1e+10")           << QString("C") << QString("1e+10")           << true  << 1.0e+10;
    QTest::newRow("C e+10")            << QString("C") << QString("e+10")            << false << 0.0;
    QTest::newRow("C .e+10")           << QString("C") << QString(".e+10")           << false << 0.0;
    QTest::newRow("C 1e+2e+10")        << QString("C") << QString("1e+2e+10")        << false << 0.0;

    QTest::newRow("de_DE 1.")          << QString("de_DE") << QString("1.")          << false << 0.0;
    QTest::newRow("de_DE 1.2")         << QString("de_DE") << QString("1.2")         << false << 0.0;
    QTest::newRow("de_DE 1.23")        << QString("de_DE") << QString("1.23")        << false << 0.0;
    QTest::newRow("de_DE 1.234")       << QString("de_DE") << QString("1.234")       << true  << 1234.0;
    QTest::newRow("de_DE 1.234,")      << QString("de_DE") << QString("1.234.")      << false << 0.0;
    QTest::newRow("de_DE 1.234.5")     << QString("de_DE") << QString("1.234.5")     << false << 0.0;
    QTest::newRow("de_DE 1.234.56")    << QString("de_DE") << QString("1.234.56")    << false << 0.0;
    QTest::newRow("de_DE 1.234.567")   << QString("de_DE") << QString("1.234.567")   << true  << 1234567.0;
    QTest::newRow("de_DE 1.234.567,")  << QString("de_DE") << QString("1.234.567,")  << true  << 1234567.0;
    QTest::newRow("de_DE 1.234.567,8") << QString("de_DE") << QString("1.234.567,8") << true  << 1234567.8;
    QTest::newRow("de_DE 1.234567,8")  << QString("de_DE") << QString("1.234567,8")  << false << 0.0;
    QTest::newRow("de_DE 12.34567,8")  << QString("de_DE") << QString("12.34567,8")  << false << 0.0;
    QTest::newRow("de_DE 1234.567,8")  << QString("de_DE") << QString("1234.567,8")  << false << 0.0;
    QTest::newRow("de_DE 1234567,8")   << QString("de_DE") << QString("1234567,8")   << true  << 1234567.8;
    QTest::newRow("de_DE .123")        << QString("de_DE") << QString(".123")        << false << 0.0;
    QTest::newRow("de_DE .3")          << QString("de_DE") << QString(".3")          << false << 0.0;
    QTest::newRow("de_DE . 3")         << QString("de_DE") << QString(". 3")         << false << 0.0;
    QTest::newRow("de_DE .  3")        << QString("de_DE") << QString(".  3")        << false << 0.0;
    QTest::newRow("de_DE .  3,2")      << QString("de_DE") << QString(".  3,2")      << false << 0.0;
    QTest::newRow("de_DE .  3,2e2")    << QString("de_DE") << QString(".  3,2e2")    << false << 0.0;
    QTest::newRow("de_DE .  e2")       << QString("de_DE") << QString(".  e2")       << false << 0.0;
    QTest::newRow("de_DE 1..234")      << QString("de_DE") << QString("1..234")      << false << 0.0;

    QTest::newRow("de_DE 1")           << QString("de_DE") << QString("1")           << true  << 1.0;
    QTest::newRow("de_DE 1.0")         << QString("de_DE") << QString("1.0")         << false << 0.0;
    QTest::newRow("de_DE 1.234e-10")   << QString("de_DE") << QString("1.234e-10")   << true  << 1234.0e-10;
    QTest::newRow("de_DE 1.234E10")    << QString("de_DE") << QString("1.234E10")    << true  << 1234.0e10;
    QTest::newRow("de_DE 1e10")        << QString("de_DE") << QString("1e10")        << true  << 1.0e10;
    QTest::newRow("de_DE .1")          << QString("de_DE") << QString(".1")          << false << 0.0;
    QTest::newRow("de_DE -.1")         << QString("de_DE") << QString("-.1")         << false << 0.0;
    QTest::newRow("de_DE 1.E10")       << QString("de_DE") << QString("1.E10")       << false << 0.0;
    QTest::newRow("de_DE 1e+10")       << QString("de_DE") << QString("1e+10")       << true  << 1.0e+10;

    QTest::newRow("de_DE 1,0")         << QString("de_DE") << QString("1,0")         << true  << 1.0;
    QTest::newRow("de_DE 1,234")       << QString("de_DE") << QString("1,234")       << true  << 1.234;
    QTest::newRow("de_DE 1,234e-10")   << QString("de_DE") << QString("1,234e-10")   << true  << 1.234e-10;
    QTest::newRow("de_DE 1,234E10")    << QString("de_DE") << QString("1,234E10")    << true  << 1.234e10;
    QTest::newRow("de_DE ,1")          << QString("de_DE") << QString(",1")          << true  << 0.1;
    QTest::newRow("de_DE -,1")         << QString("de_DE") << QString("-,1")         << true  << -0.1;
    QTest::newRow("de_DE 1,")          << QString("de_DE") << QString("1,")          << true  << 1.0;
    QTest::newRow("de_DE 1,E10")       << QString("de_DE") << QString("1,E10")       << true  << 1.0e10;

    QTest::newRow("de_DE empty")       << QString("de_DE") << QString("")            << false << 0.0;
    QTest::newRow("de_DE null")        << QString("de_DE") << QString()              << false << 0.0;
    QTest::newRow("de_DE .")           << QString("de_DE") << QString(".")           << false << 0.0;
    QTest::newRow("de_DE 1e")          << QString("de_DE") << QString("1e")          << false << 0.0;
    QTest::newRow("de_DE 1e1.0")       << QString("de_DE") << QString("1e1.0")       << false << 0.0;
    QTest::newRow("de_DE 1e+")         << QString("de_DE") << QString("1e+")         << false << 0.0;
    QTest::newRow("de_DE 1e-")         << QString("de_DE") << QString("1e-")         << false << 0.0;

    QTest::newRow("C 9,876543")        << QString("C") << QString("9,876543")        << false << 0.0;
    QTest::newRow("C 9,876543.2")      << QString("C") << QString("9,876543.2")      << false << 0.0;
    QTest::newRow("C 9,876543e-2")     << QString("C") << QString("9,876543e-2")     << false << 0.0;
    QTest::newRow("C 9,876543.0e-2")   << QString("C") << QString("9,876543.0e-2")   << false << 0.0;

    QTest::newRow("de_DE 9.876543")      << QString("de_DE") << QString("9876.543")        << false << 0.0;
    QTest::newRow("de_DE 9.876543,2")    << QString("de_DE") << QString("9.876543,2")      << false << 0.0;
    QTest::newRow("de_DE 9.876543e-2")   << QString("de_DE") << QString("9.876543e-2")     << false << 0.0;
    QTest::newRow("de_DE 9.876543,0e-2") << QString("de_DE") << QString("9.876543,0e-2")   << false << 0.0;
    QTest::newRow("de_DE 9.876543e--2")   << QString("de_DE") << QString("9.876543e")+QChar(8722)+QString("2")     << false << 0.0;
    QTest::newRow("de_DE 9.876543,0e--2") << QString("de_DE") << QString("9.876543,0e")+QChar(8722)+QString("2")   << false << 0.0;

    // Signs and exponent separator aren't single characters:
    QTest::newRow("sv_SE 4e-3") // Swedish, Sweden
        << u"sv_SE"_s << u"4\u00d7" "10^\u2212" "03"_s << true << 4e-3;
    QTest::newRow("sv_SE 4x-3") // Only first character of exponent
        << u"sv_SE"_s << u"4\u00d7\u2212" "03"_s << false << 0.0;
    QTest::newRow("se_NO 4e-3") // Northern Sami, Norway
        << u"se_NO"_s << u"4\u00b7" "10^\u2212" "03"_s << true << 4e-3;
    QTest::newRow("se_NO 4x-3") // Only first character of exponent
        << u"se_NO"_s << u"4\u00b7\u2212" "03"_s << false << 0.0;
    QTest::newRow("ar_EG 4e-3") // Arabic, Egypt
        << u"ar_EG"_s << u"\u0664\u0623\u0633\u061c-\u0660\u0663"_s << true << 4e-3;
    QTest::newRow("ar_EG 4e!3") // Only first character of sign:
        << u"ar_EG"_s << u"\u0664\u0623\u0633\u061c\u0660\u0663"_s << false << 0.0;
    QTest::newRow("ar_EG 4x-3") // Only first character of exponent
        << u"ar_EG"_s << u"\u0664\u0623\u061c-\u0660\u0663"_s << false << 0.0;
    QTest::newRow("ar_EG 4x!3") // Only first character of exponent and sign
        << u"ar_EG"_s << u"\u0664\u0623\u061c\u0660\u0663"_s << false << 0.0;
    QTest::newRow("fa_IR 4e-3") // Farsi, Iran
        << u"fa_IR"_s << u"\u06f4\u00d7\u06f1\u06f0^\u200e\u2212\u06f0\u06f3"_s << true << 4e-3;
    QTest::newRow("fa_IR 4e!3") // Only first character of sign:
        << u"fa_IR"_s << u"\u06f4\u00d7\u06f1\u06f0^\u200e\u06f0\u06f3"_s << false << 0.0;
    QTest::newRow("fa_IR 4x-3") // Only first character of exponent
        << u"fa_IR"_s << u"\u06f4\u00d7\u200e\u2212\u06f0\u06f3"_s << false << 0.0;
    QTest::newRow("fa_IR 4x!3") // Only first character of exponent and sign
        << u"fa_IR"_s << u"\u06f4\u00d7\u200e\u06f0\u06f3"_s << false << 0.0;

    // Cyrillic has its own E; only officially used by Ukrainian as exponent,
    // with other Cyrillic locales using the Latin E. QLocale allows that there
    // may be some cross-over between these.
    QTest::newRow("uk_UA Cyrillic E") << u"uk_UA"_s << u"4\u0415-3"_s << true << 4e-3; // Official
    QTest::newRow("uk_UA Latin E") << u"uk_UA"_s << u"4E-3"_s << true << 4e-3;
    QTest::newRow("uk_UA Cyrilic e") << u"uk_UA"_s << u"4\u0435-3"_s << true << 4e-3;
    QTest::newRow("uk_UA Latin e") << u"uk_UA"_s << u"4e-3"_s << true << 4e-3;
    QTest::newRow("ru_RU Latin E") << u"ru_RU"_s << u"4E-3"_s << true << 4e-3; // Official
    QTest::newRow("ru_RU Cyrillic E") << u"ru_RU"_s << u"4\u0415-3"_s << true << 4e-3;
    QTest::newRow("ru_RU Latin e") << u"ru_RU"_s << u"4e-3"_s << true << 4e-3;
    QTest::newRow("ru_RU Cyrilic e") << u"ru_RU"_s << u"4\u0435-3"_s << true << 4e-3;
}

void tst_QLocale::stringToDouble_data()
{
    toReal_data();
    if (std::numeric_limits<double>::has_infinity) {
        double huge = std::numeric_limits<double>::infinity();
        QTest::newRow("C inf") << QString("C") << QString("inf") << true << huge;
        QTest::newRow("C +inf") << QString("C") << QString("+inf") << true << +huge;
        QTest::newRow("C -inf") << QString("C") << QString("-inf") << true << -huge;
        // Overflow:
        QTest::newRow("C huge") << QString("C") << QString("2e308") << false << huge;
        QTest::newRow("C -huge") << QString("C") << QString("-2e308") << false << -huge;
    }
    if (std::numeric_limits<double>::has_quiet_NaN)
        QTest::newRow("C qnan") << QString("C") << QString("NaN") << true << std::numeric_limits<double>::quiet_NaN();

    // Malformed
    QTest::newRow("infe10") << QString("C") << QString("infe10") << false << 0.;
    QTest::newRow("inf.10") << QString("C") << QString("inf.10") << false << 0.;
    QTest::newRow("i1n0f") << QString("C") << QString("i1n0f") << false << 0.;
    QTest::newRow("inf,000") << QString("en_US") << QString("inf,000") << false << 0.;
    QTest::newRow("1,inf") << QString("en_US") << QString("1,inf") << false << 0.;
    QTest::newRow("NaNe10") << QString("C") << QString("NaNe10") << false << 0.;
    QTest::newRow("NaN.10") << QString("C") << QString("NaN.10") << false << 0.;
    QTest::newRow("N1a0N") << QString("C") << QString("N1a0N") << false << 0.;
    QTest::newRow("NaN,000") << QString("en_US") << QString("NaN,000") << false << 0.;
    QTest::newRow("1,NaN") << QString("en_US") << QString("1,NaN") << false << 0.;

    // In range (but outside float's range):
    QTest::newRow("C big") << QString("C") << QString("3.5e38") << true << 3.5e38;
    QTest::newRow("C -big") << QString("C") << QString("-3.5e38") << true << -3.5e38;
    QTest::newRow("C small") << QString("C") << QString("1e-45") << true << 1e-45;
    QTest::newRow("C -small") << QString("C") << QString("-1e-45") << true << -1e-45;

    // Underflow:
    QTest::newRow("C tiny") << QString("C") << QString("2e-324") << false << 0.;
    QTest::newRow("C -tiny") << QString("C") << QString("-2e-324") << false << 0.;

    // Test a tiny fraction (well beyond denomal) with a huge exponent:
    const QString zeros(500, '0');
    QTest::newRow("C tiny fraction, huge exponent")
        << u"C"_s << u"0."_s + zeros + u"123e501"_s << true << 1.23;
    QTest::newRow("uk_UA tiny fraction, huge exponent")
        << u"uk_UA"_s << u"0,"_s + zeros + u"123\u0415" "501"_s << true << 1.23;
}

void tst_QLocale::stringToDouble()
{
#define MY_DOUBLE_EPSILON (2.22045e-16) // 1/2^{52}; double has a 53-bit mantissa

    QFETCH(QString, locale_name);
    QFETCH(QString, num_str);
    QFETCH(bool, good);
    QFETCH(double, num);
    QStringView num_strRef{ num_str };

    QLocale locale(locale_name);
    QCOMPARE(locale.name(), locale_name);

    bool ok;
    double d = locale.toDouble(num_str, &ok);
    QCOMPARE(ok, good);

    {
        // Make sure result is independent of locale:
        TransientLocale ignoreme(LC_ALL, "ar_SA.UTF-8");
        QCOMPARE(locale.toDouble(num_str, &ok), d);
        QCOMPARE(ok, good);
    }

    if (ok || std::isinf(num)) {
        // First use fuzzy-compare, then a more precise check:
        QCOMPARE(d, num);
        if (std::isfinite(num)) {
            double diff = d > num ? d - num : num - d;
            QCOMPARE_LE(diff, MY_DOUBLE_EPSILON);
        }
    }

    d = locale.toDouble(num_strRef, &ok);
    QCOMPARE(ok, good);

    if (ok || std::isinf(num)) {
        QCOMPARE(d, num);
        if (std::isfinite(num)) {
            double diff = d > num ? d - num : num - d;
            QCOMPARE_LE(diff, MY_DOUBLE_EPSILON);
        }
    }
#undef MY_DOUBLE_EPSILON
}

void tst_QLocale::stringToFloat_data()
{
    using Bounds = std::numeric_limits<float>;
    toReal_data();
    const QString C(QStringLiteral("C"));
    if (Bounds::has_infinity) {
        double huge = Bounds::infinity();
        QTest::newRow("C inf") << C << QString("inf") << true << huge;
        QTest::newRow("C +inf") << C << QString("+inf") << true << +huge;
        QTest::newRow("C -inf") << C << QString("-inf") << true << -huge;
        // Overflow float, but not double:
        QTest::newRow("C big") << C << QString("3.5e38") << false << huge;
        QTest::newRow("C -big") << C << QString("-3.5e38") << false << -huge;
        // Overflow double, too:
        QTest::newRow("C huge") << C << QString("2e308") << false << huge;
        QTest::newRow("C -huge") << C << QString("-2e308") << false << -huge;
    }
    if (Bounds::has_quiet_NaN)
        QTest::newRow("C qnan") << C << QString("NaN") << true << double(Bounds::quiet_NaN());

    // Minimal float: shouldn't underflow
    QTest::newRow("C float min")
        << C << QLocale::c().toString(Bounds::denorm_min()) << true << double(Bounds::denorm_min());
    QTest::newRow("C float -min")
        << C << QLocale::c().toString(-Bounds::denorm_min()) << true << -double(Bounds::denorm_min());

    // Underflow float, but not double:
    QTest::newRow("C small") << C << QString("7e-46") << false << 0.;
    QTest::newRow("C -small") << C << QString("-7e-46") << false << 0.;
    using Double = std::numeric_limits<double>;
    QTest::newRow("C double min")
        << C << QLocale::c().toString(Double::denorm_min()) << false << 0.0;
    QTest::newRow("C double -min")
        << C << QLocale::c().toString(-Double::denorm_min()) << false << 0.0;

    // Underflow double, too:
    QTest::newRow("C tiny") << C << QString("2e-324") << false << 0.;
    QTest::newRow("C -tiny") << C << QString("-2e-324") << false << 0.;

    // Test a small fraction (well beyond denomal) with a big exponent:
    const QString zeros(80, '0');
    QTest::newRow("C small fraction, big exponent")
        << u"C"_s << u"0."_s + zeros + u"123e81"_s << true << 1.23;
    QTest::newRow("uk_UA small fraction, big exponent")
        << u"uk_UA"_s << u"0,"_s + zeros + u"123\u0415" "81"_s << true << 1.23;
}

void tst_QLocale::stringToFloat()
{
#define MY_FLOAT_EPSILON (2.384e-7) // 1/2^{22}; float has a 23-bit mantissa

    QFETCH(QString, locale_name);
    QFETCH(QString, num_str);
    QFETCH(bool, good);
    QFETCH(double, num);
    QStringView num_strRef{ num_str };
    float fnum = num;

    QLocale locale(locale_name);
    QCOMPARE(locale.name(), locale_name);

    QT_IGNORE_DEPRECATIONS(constexpr bool float_has_denorm = std::numeric_limits<float>::has_denorm != std::denorm_present;)
    if constexpr (float_has_denorm) {
        if (qstrcmp(QTest::currentDataTag(), "C float -min") == 0
                || qstrcmp(QTest::currentDataTag(), "C float min") == 0)
            QSKIP("Skipping 'denorm' as this type lacks denormals on this system");
    }
    bool ok;
    float f = locale.toFloat(num_str, &ok);
    QCOMPARE(ok, good);

    QT_IGNORE_DEPRECATIONS(constexpr bool double_has_denorm = std::numeric_limits<double>::has_denorm != std::denorm_present;)
    if constexpr (double_has_denorm) {
        if (qstrcmp(QTest::currentDataTag(), "C double min") == 0
                || qstrcmp(QTest::currentDataTag(), "C double -min") == 0
                || qstrcmp(QTest::currentDataTag(), "C tiny") == 0
                || qstrcmp(QTest::currentDataTag(), "C -tiny") == 0) {
            QSKIP("Skipping 'denorm' as this type lacks denormals on this system");
        }
    }

    {
        // Make sure result is independent of locale:
        TransientLocale ignoreme(LC_ALL, "ar_SA.UTF-8");
        QCOMPARE(locale.toFloat(num_str, &ok), f);
        QCOMPARE(ok, good);
    }

    if (ok || std::isinf(fnum)) {
        // First use fuzzy-compare, then a more precise check:
        QCOMPARE(f, fnum);
        if (std::isfinite(fnum)) {
            float diff = f > fnum ? f - fnum : fnum - f;
            QCOMPARE_LE(diff, MY_FLOAT_EPSILON);
        }
    }

    f = locale.toFloat(num_strRef, &ok);
    QCOMPARE(ok, good);

    if (ok || std::isinf(fnum)) {
        QCOMPARE(f, fnum);
        if (std::isfinite(fnum)) {
            float diff = f > fnum ? f - fnum : fnum - f;
            QCOMPARE_LE(diff, MY_FLOAT_EPSILON);
        }
    }
#undef MY_FLOAT_EPSILON
}

void tst_QLocale::doubleToString_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QString>("numStr");
    QTest::addColumn<double>("num");
    QTest::addColumn<char>("mode");
    QTest::addColumn<int>("precision");

    int shortest = QLocale::FloatingPointShortest;

    QTest::newRow("C 0 f 0") << QString("C") << QString("0")        << 0.0 << 'f' << 0;
    QTest::newRow("C 0 f 5") << QString("C") << QString("0.00000")  << 0.0 << 'f' << 5;
    QTest::newRow("C 0 f -") << QString("C") << QString("0")        << 0.0 << 'f' << shortest;
    QTest::newRow("C 0 e 0") << QString("C") << QString("0e+00")       << 0.0 << 'e' << 0;
    QTest::newRow("C 0 e 5") << QString("C") << QString("0.00000e+00") << 0.0 << 'e' << 5;
    QTest::newRow("C 0 e -") << QString("C") << QString("0e+00")       << 0.0 << 'e' << shortest;
    QTest::newRow("C 0 g 0") << QString("C") << QString("0")        << 0.0 << 'g' << 0;
    QTest::newRow("C 0 g 5") << QString("C") << QString("0")        << 0.0 << 'g' << 5;
    QTest::newRow("C 0 g -") << QString("C") << QString("0")        << 0.0 << 'g' << shortest;

    double d = std::numeric_limits<double>::max();
    static const char doublemaxfixed[] =
            "1797693134862315708145274237317043567980705675258449965989174768031572607800285387605"
            "8955863276687817154045895351438246423432132688946418276846754670353751698604991057655"
            "1282076245490090389328944075868508455133942304583236903222948165808559332123348274797"
            "826204144723168738177180919299881250404026184124858368";

    QTest::newRow("C max f 0") << QString("C") << QString(doublemaxfixed) << d << 'f' << 0;
    QTest::newRow("C max f 5") << QString("C") << doublemaxfixed + QString(".00000")  << d << 'f' << 5;
    QTest::newRow("C max e 0") << QString("C") << QString("2e+308")       << d << 'e' << 0;
    QTest::newRow("C max g 0") << QString("C") << QString("2e+308")       << d << 'g' << 0;
    QTest::newRow("C max e 5") << QString("C") << QString("1.79769e+308") << d << 'e' << 5;
    QTest::newRow("C max g 5") << QString("C") << QString("1.7977e+308")  << d << 'g' << 5;
#if QT_CONFIG(doubleconversion)
    QTest::newRow("C max e -") << QString("C") << QString("1.7976931348623157e+308") << d << 'e' << shortest;
    QTest::newRow("C max g -") << QString("C") << QString("1.7976931348623157e+308") << d << 'g' << shortest;
    QTest::newRow("C max f -") << QString("C")
                               << QString("%1").arg("17976931348623157", -int(strlen(doublemaxfixed)), u'0')
                               << d << 'f' << shortest;
#endif

    d = std::numeric_limits<double>::min();
    QTest::newRow("C min f 0") << QString("C") << QString("0")            << d << 'f' << 0;
    QTest::newRow("C min f 5") << QString("C") << QString("0.00000")      << d << 'f' << 5;
    QTest::newRow("C min e 0") << QString("C") << QString("2e-308")       << d << 'e' << 0;
    QTest::newRow("C min g 0") << QString("C") << QString("2e-308")       << d << 'g' << 0;
    QTest::newRow("C min e 5") << QString("C") << QString("2.22507e-308") << d << 'e' << 5;
    QTest::newRow("C min g 5") << QString("C") << QString("2.2251e-308")  << d << 'g' << 5;
#if QT_CONFIG(doubleconversion)
    QTest::newRow("C min e -") << QString("C") << QString("2.2250738585072014e-308") << d << 'e' << shortest;
    QTest::newRow("C min f -") << QString("C")
                               << QString("0.%1").arg("22250738585072014", 308 - 1 + std::numeric_limits<double>::max_digits10, u'0')
                               << d << 'f' << shortest;
    QTest::newRow("C min g -") << QString("C") << QString("2.2250738585072014e-308") << d << 'g' << shortest;
#endif

    QTest::newRow("C 3.4 f 5") << QString("C") << QString("3.40000")     << 3.4 << 'f' << 5;
    QTest::newRow("C 3.4 f 0") << QString("C") << QString("3")           << 3.4 << 'f' << 0;
    QTest::newRow("C 3.4 e 5") << QString("C") << QString("3.40000e+00") << 3.4 << 'e' << 5;
    QTest::newRow("C 3.4 e 0") << QString("C") << QString("3e+00")       << 3.4 << 'e' << 0;
    QTest::newRow("C 3.4 g 5") << QString("C") << QString("3.4")         << 3.4 << 'g' << 5;
    QTest::newRow("C 3.4 g 1") << QString("C") << QString("3")           << 3.4 << 'g' << 1;

    QTest::newRow("C 3.4 f 1") << QString("C") << QString("3.4")     << 3.4 << 'f' << 1;
    QTest::newRow("C 3.4 f -") << QString("C") << QString("3.4")     << 3.4 << 'f' << shortest;
    QTest::newRow("C 3.4 e 1") << QString("C") << QString("3.4e+00") << 3.4 << 'e' << 1;
    QTest::newRow("C 3.4 e -") << QString("C") << QString("3.4e+00") << 3.4 << 'e' << shortest;
    QTest::newRow("C 3.4 g 2") << QString("C") << QString("3.4")     << 3.4 << 'g' << 2;
    QTest::newRow("C 3.4 g -") << QString("C") << QString("3.4")     << 3.4 << 'g' << shortest;

    QTest::newRow("de_DE 3,4 f 1") << QString("de_DE") << QString("3,4")     << 3.4 << 'f' << 1;
    QTest::newRow("de_DE 3,4 f -") << QString("de_DE") << QString("3,4")     << 3.4 << 'f' << shortest;
    QTest::newRow("de_DE 3,4 e 1") << QString("de_DE") << QString("3,4e+00") << 3.4 << 'e' << 1;
    QTest::newRow("de_DE 3,4 E 1") << QString("de_DE") << QString("3,4E+00") << 3.4 << 'E' << 1;
    QTest::newRow("de_DE 3,4 e -") << QString("de_DE") << QString("3,4e+00") << 3.4 << 'e' << shortest;
    QTest::newRow("de_DE 3,4 E -") << QString("de_DE") << QString("3,4E+00") << 3.4 << 'E' << shortest;
    QTest::newRow("de_DE 3,4 g 2") << QString("de_DE") << QString("3,4")     << 3.4 << 'g' << 2;
    QTest::newRow("de_DE 3,4 g -") << QString("de_DE") << QString("3,4")     << 3.4 << 'g' << shortest;

    QTest::newRow("C 0.035003945 f 12") << QString("C") << QString("0.035003945000")   << 0.035003945 << 'f' << 12;
    QTest::newRow("C 0.035003945 f 6")  << QString("C") << QString("0.035004")         << 0.035003945 << 'f' << 6;
    QTest::newRow("C 0.035003945 e 10") << QString("C") << QString("3.5003945000e-02") << 0.035003945 << 'e' << 10;
    QTest::newRow("C 0.035003945 e 4")  << QString("C") << QString("3.5004e-02")       << 0.035003945 << 'e' << 4;
    QTest::newRow("C 0.035003945 g 11") << QString("C") << QString("0.035003945")      << 0.035003945 << 'g' << 11;
    QTest::newRow("C 0.035003945 g 5")  << QString("C") << QString("0.035004")         << 0.035003945 << 'g' << 5;

    QTest::newRow("C 0.035003945 f 9") << QString("C") << QString("0.035003945")   << 0.035003945 << 'f' << 9;
    QTest::newRow("C 0.035003945 f -") << QString("C") << QString("0.035003945")   << 0.035003945 << 'f' << shortest;
    QTest::newRow("C 0.035003945 e 7") << QString("C") << QString("3.5003945e-02") << 0.035003945 << 'e' << 7;
    QTest::newRow("C 0.035003945 e -") << QString("C") << QString("3.5003945e-02") << 0.035003945 << 'e' << shortest;
    QTest::newRow("C 0.035003945 g 8") << QString("C") << QString("0.035003945")   << 0.035003945 << 'g' << 8;
    QTest::newRow("C 0.035003945 g -") << QString("C") << QString("0.035003945")   << 0.035003945 << 'g' << shortest;

    QTest::newRow("de_DE 0,035003945 f 9") << QString("de_DE") << QString("0,035003945")   << 0.035003945 << 'f' << 9;
    QTest::newRow("de_DE 0,035003945 f -") << QString("de_DE") << QString("0,035003945")   << 0.035003945 << 'f' << shortest;
    QTest::newRow("de_DE 0,035003945 e 7") << QString("de_DE") << QString("3,5003945e-02") << 0.035003945 << 'e' << 7;
    QTest::newRow("de_DE 0,035003945 E 7") << QString("de_DE") << QString("3,5003945E-02") << 0.035003945 << 'E' << 7;
    QTest::newRow("de_DE 0,035003945 e -") << QString("de_DE") << QString("3,5003945e-02") << 0.035003945 << 'e' << shortest;
    QTest::newRow("de_DE 0,035003945 E -") << QString("de_DE") << QString("3,5003945E-02") << 0.035003945 << 'E' << shortest;
    QTest::newRow("de_DE 0,035003945 g 8") << QString("de_DE") << QString("0,035003945")   << 0.035003945 << 'g' << 8;
    QTest::newRow("de_DE 0,035003945 g -") << QString("de_DE") << QString("0,035003945")   << 0.035003945 << 'g' << shortest;
    // Check 'f/F' iff (adjusted) precision > exponent >= -4:
    QTest::newRow("de_DE 12345 g 4") << QString("de_DE") << QString("1,235e+04") << 12345. << 'g' << 4;
    QTest::newRow("de_DE 12345 G 4") << QString("de_DE") << QString("1,235E+04") << 12345. << 'G' << 4;
    QTest::newRow("de_DE 1e7 g 8")   << QString("de_DE") << QString("10.000.000") << 1e7 << 'g' << 8;
    QTest::newRow("de_DE 1e8 g 8")   << QString("de_DE") << QString("1e+08") << 1e8  << 'g' << 8;
    QTest::newRow("de_DE 1e8 G 8")   << QString("de_DE") << QString("1E+08") << 1e8  << 'G' << 8;
    QTest::newRow("de_DE 10.0 g 1")  << QString("de_DE") << QString("1e+01") << 10.0  << 'g' << 1;
    QTest::newRow("de_DE 10.0 g 0")  << QString("de_DE") << QString("1e+01") << 10.0  << 'g' << 0;
    QTest::newRow("de_DE 1.0 g 0")   << QString("de_DE") << QString("1") << 1.0  << 'g' << 0;
    QTest::newRow("de_DE 0.0001 g 0")  << QString("de_DE") << QString("0,0001") << 0.0001  << 'g' << 0;
    QTest::newRow("de_DE 0.00001 g 0") << QString("de_DE") << QString("1e-05") << 0.00001 << 'g' << 0;
    // Check transition to exponent form:
    QTest::newRow("de_DE 1245678900 g -")  << QString("de_DE") << QString("1.245.678.900") << 12456789e2 << 'g' << shortest;
    QTest::newRow("de_DE 12456789100 g -") << QString("de_DE") << QString("12.456.789.100") << 124567891e2 << 'g' << shortest;
    QTest::newRow("de_DE 12456789000 g -") << QString("de_DE") << QString("1,2456789e+10")  << 12456789e3 << 'g' << shortest;
    QTest::newRow("de_DE 12000 g -")
        << QString("de_DE") << QString("12.000") << 12e3 << 'g' << shortest;
    // 12e4 has "120.000" and "1.2e+05" of equal length; which shortest picks is unspecified.
    QTest::newRow("de_DE 1200000 g -") << QString("de_DE") << QString("1,2e+06") << 12e5 << 'g' << shortest;
    QTest::newRow("de_DE 1000 g -")  << QString("de_DE") << QString("1.000") << 1e3 << 'g' << shortest;
    QTest::newRow("de_DE 10000 g -") << QString("de_DE") << QString("1e+04") << 1e4 << 'g' << shortest;

    QTest::newRow("C 0.000003945 f 12") << QString("C") << QString("0.000003945000") << 0.000003945 << 'f' << 12;
    QTest::newRow("C 0.000003945 f 6")  << QString("C") << QString("0.000004")       << 0.000003945 << 'f' << 6;
    QTest::newRow("C 0.000003945 e 6")  << QString("C") << QString("3.945000e-06")   << 0.000003945 << 'e' << 6;
    QTest::newRow("C 0.000003945 e 0")  << QString("C") << QString("4e-06")          << 0.000003945 << 'e' << 0;
    QTest::newRow("C 0.000003945 g 7")  << QString("C") << QString("3.945e-06")      << 0.000003945 << 'g' << 7;
    QTest::newRow("C 0.000003945 g 1")  << QString("C") << QString("4e-06")          << 0.000003945 << 'g' << 1;
    QTest::newRow("sv_SE 0.000003945 g 1") // Swedish, Sweden (among others)
        << u"sv_SE"_s << u"4\u00d7" "10^\u2212" "06"_s << 0.000003945 << 'g' << 1;
    QTest::newRow("sv_SE 3945e3 g 1")
        << u"sv_SE"_s << u"4\u00d7" "10^+06"_s << 3945e3 << 'g' << 1;
    QTest::newRow("se 0.000003945 g 1") // Northern Sami
        << u"se"_s << u"4\u00b7" "10^\u2212" "06"_s << 0.000003945 << 'g' << 1;
    QTest::newRow("ar_EG 0.000003945 g 1") // Arabic, Egypt (among others)
        << u"ar_EG"_s << u"\u0664\u0623\u0633\u061c-\u0660\u0666"_s << 0.000003945 << 'g' << 1;
    QTest::newRow("ar_EG 3945e3 g 1")
        << u"ar_EG"_s << u"\u0664\u0623\u0633\u061c+\u0660\u0666"_s << 3945e3 << 'g' << 1;
    QTest::newRow("fa_IR 0.000003945 g 1") // Farsi, Iran (same for Afghanistan)
        << u"fa_IR"_s << u"\u06f4\u00d7\u06f1\u06f0^\u200e\u2212\u06f0\u06f6"_s
        << 0.000003945 << 'g' << 1;

    QTest::newRow("C 0.000003945 f 9") << QString("C") << QString("0.000003945") << 0.000003945 << 'f' << 9;
    QTest::newRow("C 0.000003945 f -") << QString("C") << QString("0.000003945") << 0.000003945 << 'f' << shortest;
    QTest::newRow("C 0.000003945 e 3") << QString("C") << QString("3.945e-06")   << 0.000003945 << 'e' << 3;
    QTest::newRow("C 0.000003945 e -") << QString("C") << QString("3.945e-06")   << 0.000003945 << 'e' << shortest;
    QTest::newRow("C 0.000003945 g 4") << QString("C") << QString("3.945e-06")   << 0.000003945 << 'g' << 4;
    QTest::newRow("C 0.000003945 g -") << QString("C") << QString("3.945e-06")   << 0.000003945 << 'g' << shortest;

    QTest::newRow("de_DE 0,000003945 f 9") << QString("de_DE") << QString("0,000003945") << 0.000003945 << 'f' << 9;
    QTest::newRow("de_DE 0,000003945 f -") << QString("de_DE") << QString("0,000003945") << 0.000003945 << 'f' << shortest;
    QTest::newRow("de_DE 0,000003945 e 3") << QString("de_DE") << QString("3,945e-06")   << 0.000003945 << 'e' << 3;
    QTest::newRow("de_DE 0,000003945 E 3") << QString("de_DE") << QString("3,945E-06")   << 0.000003945 << 'E' << 3;
    QTest::newRow("de_DE 0,000003945 e -") << QString("de_DE") << QString("3,945e-06")   << 0.000003945 << 'e' << shortest;
    QTest::newRow("de_DE 0,000003945 E -") << QString("de_DE") << QString("3,945E-06")   << 0.000003945 << 'E' << shortest;
    QTest::newRow("de_DE 0,000003945 g 4") << QString("de_DE") << QString("3,945e-06")   << 0.000003945 << 'g' << 4;
    QTest::newRow("de_DE 0,000003945 G 4") << QString("de_DE") << QString("3,945E-06")   << 0.000003945 << 'G' << 4;
    QTest::newRow("de_DE 0,000003945 g -") << QString("de_DE") << QString("3,945e-06")   << 0.000003945 << 'g' << shortest;
    QTest::newRow("de_DE 0,000003945 G -") << QString("de_DE") << QString("3,945E-06")   << 0.000003945 << 'G' << shortest;

    QTest::newRow("C 12456789012 f 3")  << QString("C") << QString("12456789012.000")     << 12456789012.0 << 'f' << 3;
    QTest::newRow("C 12456789012 e 13") << QString("C") << QString("1.2456789012000e+10") << 12456789012.0 << 'e' << 13;
    QTest::newRow("C 12456789012 e 7")  << QString("C") << QString("1.2456789e+10")       << 12456789012.0 << 'e' << 7;
    QTest::newRow("C 12456789012 g 14") << QString("C") << QString("12456789012")         << 12456789012.0 << 'g' << 14;
    QTest::newRow("C 12456789012 g 8")  << QString("C") << QString("1.2456789e+10")       << 12456789012.0 << 'g' << 8;
    // Check 'f/F' iff (adjusted) precision > exponent >= -4:
    QTest::newRow("C 12345 g 4") << QString("C") << QString("1.235e+04") << 12345. << 'g' << 4;
    QTest::newRow("C 1e7 g 8")   << QString("C") << QString("10000000") << 1e7 << 'g' << 8;
    QTest::newRow("C 1e8 g 8")   << QString("C") << QString("1e+08") << 1e8  << 'g' << 8;
    QTest::newRow("C 10.0 g 1")  << QString("C") << QString("1e+01") << 10.0  << 'g' << 1;
    QTest::newRow("C 10.0 g 0")  << QString("C") << QString("1e+01") << 10.0  << 'g' << 0;
    QTest::newRow("C 1.0 g 0")   << QString("C") << QString("1") << 1.0  << 'g' << 0;
    QTest::newRow("C 0.0001 g 0")  << QString("C") << QString("0.0001") << 0.0001  << 'g' << 0;
    QTest::newRow("C 0.00001 g 0") << QString("C") << QString("1e-05") << 0.00001 << 'g' << 0;
    // Check transition to exponent form:
    QTest::newRow("C 1245678900000 g -")  << QString("C") << QString("1245678900000")     << 1245678900000.0 << 'g' << shortest;
    QTest::newRow("C 12456789100000 g -") << QString("C") << QString("12456789100000")    << 12456789100000.0 << 'g' << shortest;
    QTest::newRow("C 12456789000000 g -") << QString("C") << QString("1.2456789e+13")     << 12456789000000.0 << 'g' << shortest;
    QTest::newRow("C 1200000 g -")  << QString("C") << QString("1200000") << 12e5 << 'g' << shortest;
    QTest::newRow("C 12000000 g -") << QString("C") << QString("1.2e+07") << 12e6 << 'g' << shortest;
    QTest::newRow("C 10000 g -")   << QString("C") << QString("10000") << 1e4 << 'g' << shortest;
    QTest::newRow("C 100000 g -")  << QString("C") << QString("1e+05") << 1e5 << 'g' << shortest;

    QTest::newRow("C 12456789012 f 0")  << QString("C") << QString("12456789012")      << 12456789012.0 << 'f' << 0;
    QTest::newRow("C 12456789012 f -")  << QString("C") << QString("12456789012")      << 12456789012.0 << 'f' << shortest;
    QTest::newRow("C 12456789012 e 10") << QString("C") << QString("1.2456789012e+10") << 12456789012.0 << 'e' << 10;
    QTest::newRow("C 12456789012 e -")  << QString("C") << QString("1.2456789012e+10") << 12456789012.0 << 'e' << shortest;
    QTest::newRow("C 12456789012 g 11") << QString("C") << QString("12456789012")      << 12456789012.0 << 'g' << 11;
    QTest::newRow("C 12456789012 g -")  << QString("C") << QString("12456789012")      << 12456789012.0 << 'g' << shortest;

    QTest::newRow("de_DE 12456789012 f 0")  << QString("de_DE") << QString("12.456.789.012")   << 12456789012.0 << 'f' << 0;
    QTest::newRow("de_DE 12456789012 f -")  << QString("de_DE") << QString("12.456.789.012")   << 12456789012.0 << 'f' << shortest;
    QTest::newRow("de_DE 12456789012 e 10") << QString("de_DE") << QString("1,2456789012e+10") << 12456789012.0 << 'e' << 10;
    QTest::newRow("de_DE 12456789012 e -")  << QString("de_DE") << QString("1,2456789012e+10") << 12456789012.0 << 'e' << shortest;
    QTest::newRow("de_DE 12456789012 g 11") << QString("de_DE") << QString("12.456.789.012")   << 12456789012.0 << 'g' << 11;
    QTest::newRow("de_DE 12456789012 g -")  << QString("de_DE") << QString("12.456.789.012")   << 12456789012.0 << 'g' << shortest;
}

void tst_QLocale::doubleToString()
{
    QFETCH(QString, localeName);
    QFETCH(QString, numStr);
    QFETCH(double, num);
    QFETCH(char, mode);
    QFETCH(int, precision);

#ifdef QT_NO_DOUBLECONVERSION
    if (precision == QLocale::FloatingPointShortest)
        QSKIP("'Shortest' double conversion is not that short without libdouble-conversion");
#endif

    const QLocale locale(localeName);
    QCOMPARE(locale.toString(num, mode, precision), numStr);

    // System locale is irrelevant here:
    TransientLocale ignoreme(LC_ALL, "de_DE.UTF-8");
    QCOMPARE(locale.toString(num, mode, precision), numStr);
}

void tst_QLocale::longlongToString_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<qlonglong>("number");
    QTest::addColumn<int>("fieldWidth");
    QTest::addColumn<char32_t>("fillChar");
    QTest::addColumn<bool>("grouped");
    QTest::addColumn<QString>("numStr");

    QTest::newRow("C 0 0 'x' t")
        << u"C"_s << qlonglong(0) << 0  << U'x' << true << u"0"_s;
    QTest::newRow("C 0 0 'x' f")
        << u"C"_s << qlonglong(0) << 0  << U'x' << false << u"0"_s;
    QTest::newRow("en_US 0 0 'x' t")
        << u"en_US"_s << qlonglong(0) << 0  << U'x' << true << u"0"_s;
    QTest::newRow("en_US 0 0 'x' f")
        << u"en_US"_s << qlonglong(0) << 0  << U'x' << false << u"0"_s;

    QTest::newRow("pl_PL 23500 0 x f")
            << u"pl_PL"_s << qlonglong(23500)  << 0   << U'x' << false << u"23500"_s;
    QTest::newRow("pl_PL 23500 0 x t")
            << u"pl_PL"_s << qlonglong(23500)  << 0   << U'x' << true  << u"23\u00A0500"_s;
    QTest::newRow("pl_PL 23500 10 x f")
            << u"pl_PL"_s << qlonglong(23500)  << 10  << U'x' << false << u"xxxxx23500"_s;
    QTest::newRow("pl_PL 23500 10 x t")
            << u"pl_PL"_s << qlonglong(23500)  << 10  << U'x' << true  << u"xxxx23\u00A0500"_s;
    QTest::newRow("pl_PL 23500 -10 x f")
            << u"pl_PL"_s << qlonglong(23500)  << -10 << U'x' << false << u"23500xxxxx"_s;
    QTest::newRow("nb_NO 23500 -10 x t")
            << u"nb_NO"_s << qlonglong(23500)  << -10 << U'x' << true  << u"23\u00A0500xxxx"_s;
    QTest::newRow("pl_PL -23500 10 x f")
            << u"pl_PL"_s << qlonglong(-23500) << 10  << U'x' << false << u"xxxx-23500"_s;
    QTest::newRow("pl_PL -23500 10 x t")
            << u"pl_PL"_s << qlonglong(-23500) << 10  << U'x' << true  << u"xxx-23\u00A0500"_s;
    QTest::newRow("pl_PL -23500 -10 x f")
            << u"pl_PL"_s << qlonglong(-23500) << -10 << U'x' << false << u"-23500xxxx"_s;
    QTest::newRow("nb_NO -23500 -10 x t")
            << u"nb_NO"_s << qlonglong(-23500) << -10 << U'x' << true  << u"\u221223\u00A0500xxx"_s;

    QTest::newRow("pl_PL 23500 0 \u0020 f")
            << u"pl_PL"_s << qlonglong(23500)  << 0   << U' ' << false << u"23500"_s;
    QTest::newRow("pl_PL 23500 0 \u0020 t")
            << u"pl_PL"_s << qlonglong(23500)  << 0   << U' ' << true  << u"23\u00A0500"_s;
    QTest::newRow("pl_PL 23500 10 \u0020 f")
            << u"pl_PL"_s << qlonglong(23500)  << 10  << U' ' << false << u"     23500"_s;
    QTest::newRow("pl_PL 23500 10 \u0020 t")
            << u"pl_PL"_s << qlonglong(23500)  << 10  << U' ' << true  << u"    23\u00A0500"_s;
    QTest::newRow("pl_PL 23500 -10 \u0020 f")
            << u"pl_PL"_s << qlonglong(23500)  << -10 << U' ' << false << u"23500     "_s;
    QTest::newRow("nb_NO 23500 -10 \u0020 t")
            << u"nb_NO"_s << qlonglong(23500)  << -10 << U' ' << true  << u"23\u00A0500    "_s;
    QTest::newRow("pl_PL -23500 10 \u0020 f")
            << u"pl_PL"_s << qlonglong(-23500) << 10  << U' ' << false << u"    -23500"_s;
    QTest::newRow("pl_PL -23500 10 \u0020 t")
            << u"pl_PL"_s << qlonglong(-23500) << 10  << U' ' << true  << u"   -23\u00A0500"_s;
    QTest::newRow("pl_PL -23500 -10 \u0020 f")
            << u"pl_PL"_s << qlonglong(-23500) << -10 << U' ' << false << u"-23500    "_s;
    QTest::newRow("nb_NO -23500 -10 \u0020 t")
            << u"nb_NO"_s << qlonglong(-23500) << -10 << U' ' << true  << u"\u221223\u00A0500   "_s;

    QTest::newRow("pl_PL 23000000 0 0 t")
            << u"pl_PL"_s << qlonglong(23000000)  << 0   << U'0' << true  << u"23\u00A0000\u00A0000"_s;
    QTest::newRow("pl_PL 23000000 0 0 f")
            << u"pl_PL"_s << qlonglong(23000000)  << 0   << U'0' << false << u"23000000"_s;
    QTest::newRow("pl_PL 23000000 15 0 t")
            << u"pl_PL"_s << qlonglong(23000000)  << 15  << U'0' << true  << u"0000023\u00A0000\u00A0000"_s;
    QTest::newRow("pl_PL 23000000 15 0 f")
            << u"pl_PL"_s << qlonglong(23000000)  << 15  << U'0' << false << u"000000023000000"_s;
    QTest::newRow("pl_PL 23000000 -15 0 t")
            << u"pl_PL"_s << qlonglong(23000000)  << -15 << U'0' << true  << u"23\u00A0000\u00A000000000"_s;
    QTest::newRow("pl_PL 23000000 -15 0 f")
            << u"pl_PL"_s << qlonglong(23000000)  << -15 << U'0' << false << u"230000000000000"_s;
    QTest::newRow("ja_JP -23000000 -15 0 t")
            << u"ja_JP"_s << qlonglong(-23000000) << -15 << U'0' << true  << u"-23,000,0000000"_s;
    QTest::newRow("ja_JP -23000000 -15 0 f")
            << u"ja_JP"_s << qlonglong(-23000000) << -15 << U'0' << false << u"-23000000000000"_s;
    QTest::newRow("ja_JP -23000000 15 0 t")
            << u"ja_JP"_s << qlonglong(-23000000) << 15  << U'0' << true  << u"-000023,000,000"_s;
    QTest::newRow("ja_JP -23000000 15 0 f")
            << u"ja_JP"_s << qlonglong(-23000000) << 15  << U'0' << false << u"-00000023000000"_s;

    QTest::newRow("hi_IN 23500 0 0 f")
            << u"hi_IN"_s << qlonglong(23500)  << 0   << U'0' << false << u"23500"_s;
    QTest::newRow("hi_IN 23500 0 0 t")
            << u"hi_IN"_s << qlonglong(23500)  << 0   << U'0' << true  << u"23,500"_s;
    QTest::newRow("hi_IN 23500 10 0 f")
            << u"hi_IN"_s << qlonglong(23500)  << 10  << U'0' << false << u"0000023500"_s;
    QTest::newRow("hi_IN 23500 10 0 t")
            << u"hi_IN"_s << qlonglong(23500)  << 10  << U'0' << true  << u"000023,500"_s;
    QTest::newRow("hi_IN 23500 -10 0 f")
            << u"hi_IN"_s << qlonglong(23500)  << -10 << U'0' << false << u"2350000000"_s;
    QTest::newRow("hi_IN 23500 -10 0 t")
            << u"hi_IN"_s << qlonglong(23500)  << -10 << U'0' << true  << u"23,5000000"_s;
    QTest::newRow("hi_IN -23500 10 0 f")
            << u"hi_IN"_s << qlonglong(-23500) << 10  << U'0' << false << u"-000023500"_s;
    QTest::newRow("hi_IN -23500 10 0 t")
            << u"hi_IN"_s << qlonglong(-23500) << 10  << U'0' << true  << u"-00023,500"_s;
    QTest::newRow("hi_IN -23500 -10 0 f")
            << u"hi_IN"_s << qlonglong(-23500) << -10 << U'0' << false << u"-235000000"_s;
    QTest::newRow("hi_IN -23500 -10 0 t")
            << u"hi_IN"_s << qlonglong(-23500) << -10 << U'0' << true  << u"-23,500000"_s;

    QTest::newRow("hi_IN 23000000 0 0 t")
            << u"hi_IN"_s << qlonglong(23000000)  << 0   << U'0' << true  << u"2,30,00,000"_s;
    QTest::newRow("hi_IN 23000000 0 0 f")
            << u"hi_IN"_s << qlonglong(23000000)  << 0   << U'0' << false << u"23000000"_s;
    QTest::newRow("hi_IN 23000000 15 0 t")
            << u"hi_IN"_s << qlonglong(23000000)  << 15  << U'0' << true  << u"00002,30,00,000"_s;
    QTest::newRow("hi_IN 23000000 15 0 f")
            << u"hi_IN"_s << qlonglong(23000000)  << 15  << U'0' << false << u"000000023000000"_s;
    QTest::newRow("hi_IN 23000000 -15 0 t")
            << u"hi_IN"_s << qlonglong(23000000)  << -15 << U'0' << true  << u"2,30,00,0000000"_s;
    QTest::newRow("hi_IN 23000000 -15 0 f")
            << u"hi_IN"_s << qlonglong(23000000)  << -15 << U'0' << false << u"230000000000000"_s;
    QTest::newRow("hi_IN -23000000 -15 0 t")
            << u"hi_IN"_s << qlonglong(-23000000) << -15 << U'0' << true  << u"-2,30,00,000000"_s;
    QTest::newRow("hi_IN -23000000 -15 0 f")
            << u"hi_IN"_s << qlonglong(-23000000) << -15 << U'0' << false << u"-23000000000000"_s;
    QTest::newRow("hi_IN -23000000 15 0 t")
            << u"hi_IN"_s << qlonglong(-23000000) << 15  << U'0' << true  << u"-0002,30,00,000"_s;
    QTest::newRow("hi_IN -23000000 15 0 f")
            << u"hi_IN"_s << qlonglong(-23000000) << 15  << U'0' << false << u"-00000023000000"_s;

    QTest::newRow("emoji -2300 7 😀 f")
        << u"en_US"_s << qlonglong(-23000) << 7  << U'😀' << false << u"😀-23000"_s;
    QTest::newRow("emoji -2300 -7 😀 f")
        << u"en_US"_s << qlonglong(-23000) << -7 << U'😀' << false << u"-23000😀"_s;
    QTest::newRow("emoji -2300 8 😀 t")
        << u"en_US"_s << qlonglong(-23000) << 8  << U'😀' << true  << u"😀-23,000"_s;
    QTest::newRow("emoji -2300 -8 😀 t")
        << u"en_US"_s << qlonglong(-23000) << -8 << U'😀' << true  << u"-23,000😀"_s;

    QTest::newRow("ar_EG 0 0 x f")
        << u"ar_EG"_s << qlonglong(0) << 0 << U'x' << false << u"\u0660"_s;
    QTest::newRow("ar_EG 0 0 x t")
        << u"ar_EG"_s << qlonglong(0) << 0 << U'x' << true << u"\u0660"_s;

    QTest::newRow("ccp_BD 0 0 𑄃 t")
        << u"ccp_BD"_s << qlonglong(0) << 0  << U'𑄃' << false << u"𑄶"_s;
    QTest::newRow("ccp_BD 0 0 𑄃 f")
        << u"ccp_BD"_s << qlonglong(0) << 0  << U'𑄃' << true << u"𑄶"_s;
    QTest::newRow("ccp_BD -2300 6 𑄃 f")
        << u"ccp_BD"_s << qlonglong(-2300) << 6  << U'𑄃' << false << u"𑄃-𑄸𑄹𑄶𑄶"_s;
    QTest::newRow("ccp_BD -2300 -6 𑄃 f")
        << u"ccp_BD"_s << qlonglong(-2300) << -6 << U'𑄃' << false << u"-𑄸𑄹𑄶𑄶𑄃"_s;
    QTest::newRow("ccp_BD -2300 7 𑄃 t")
        << u"ccp_BD"_s << qlonglong(-2300) << 7  << U'𑄃' << true  << u"𑄃-𑄸,𑄹𑄶𑄶"_s;
    QTest::newRow("ccp_BD -2300 -7 𑄃 t")
        << u"ccp_BD"_s << qlonglong(-2300) << -7 << U'𑄃' << true  << u"-𑄸,𑄹𑄶𑄶𑄃"_s;
}

void tst_QLocale::longlongToString()
{
    QFETCH(QString, localeName);
    QFETCH(qlonglong, number);
    QFETCH(int, fieldWidth);
    QFETCH(char32_t, fillChar);
    QFETCH(bool, grouped);
    QFETCH(QString, numStr);

    QLocale locale(localeName);
    auto toCompare = locale.toString(number, fieldWidth, fillChar);
    if (grouped) {
        QCOMPARE(toCompare, numStr);
    } else {
        locale.setNumberOptions(QLocale::OmitGroupSeparator);
        QCOMPARE(locale.toString(number, fieldWidth, fillChar), numStr);
    }
}

void tst_QLocale::qulonglongToString_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<qulonglong>("number");
    QTest::addColumn<int>("fieldWidth");
    QTest::addColumn<char32_t>("fillChar");
    QTest::addColumn<bool>("grouped");
    QTest::addColumn<QString>("numStr");

    QTest::newRow("C 0 0 x f")
            << u"C"_s << qulonglong(0)  << 0   << U'x' << false << u"0"_s;
    QTest::newRow("C 0 0 x t")
            << u"C"_s << qulonglong(0)  << 0   << U'x' << true << u"0"_s;
    QTest::newRow("en_US 0 0 x f")
            << u"en_US"_s << qulonglong(0)  << 0   << U'x' << false << u"0"_s;
    QTest::newRow("en_US 0 0 x t")
            << u"en_US"_s << qulonglong(0)  << 0   << U'x' << true << u"0"_s;

    QTest::newRow("pl_PL 23500 0 x f")
            << u"pl_PL"_s << qulonglong(23500)  << 0   << U'x' << false << u"23500"_s;
    QTest::newRow("pl_PL 23500 0 x t")
            << u"pl_PL"_s << qulonglong(23500)  << 0   << U'x' << true  << u"23\u00A0500"_s;
    QTest::newRow("pl_PL 23500 10 x f")
            << u"pl_PL"_s << qulonglong(23500)  << 10  << U'x' << false << u"xxxxx23500"_s;
    QTest::newRow("pl_PL 23500 10 x t")
            << u"pl_PL"_s << qulonglong(23500)  << 10  << U'x' << true  << u"xxxx23\u00A0500"_s;
    QTest::newRow("pl_PL 23500 -10 x f")
            << u"pl_PL"_s << qulonglong(23500)  << -10 << U'x' << false << u"23500xxxxx"_s;
    QTest::newRow("nb_NO 23500 -10 x t")
            << u"nb_NO"_s << qulonglong(23500)  << -10 << U'x' << true  << u"23\u00A0500xxxx"_s;

    QTest::newRow("pl_PL 23000000 0 \u0020 t")
            << u"pl_PL"_s << qulonglong(23000000) << 0   << U' ' << true  << u"23\u00A0000\u00A0000"_s;
    QTest::newRow("pl_PL 23000000 0 \u0020 f")
            << u"pl_PL"_s << qulonglong(23000000) << 0   << U' ' << false << u"23000000"_s;
    QTest::newRow("nb_NO 23000000 15 \u0020 t")
            << u"nb_NO"_s << qulonglong(23000000) << 15  << U' ' << true  << u"     23\u00A0000\u00A0000"_s;
    QTest::newRow("nb_NO 23000000 15 \u0020 f")
            << u"nb_NO"_s << qulonglong(23000000) << 15  << U' ' << false << u"       23000000"_s;
    QTest::newRow("ja_JP 23000000 -15 \u0020 t")
            << u"ja_JP"_s << qulonglong(23000000) << -15 << U' ' << true  << u"23,000,000     "_s;
    QTest::newRow("ja_JP 23000000 -15 \u0020 f")
            << u"ja_JP"_s << qulonglong(23000000) << -15 << U' ' << false << u"23000000       "_s;

    QTest::newRow("ja_JP 23500 0 0 f")
            << u"ja_JP"_s << qulonglong(23500) << 0   << U'0' << false << u"23500"_s;
    QTest::newRow("ja_JP 23500 0 0 t")
            << u"ja_JP"_s << qulonglong(23500) << 0   << U'0' << true  << u"23,500"_s;
    QTest::newRow("nb_NO 23500 10 0 f")
            << u"nb_NO"_s << qulonglong(23500) << 10  << U'0' << false << u"0000023500"_s;
    QTest::newRow("nb_NO 23500 10 0 t")
            << u"nb_NO"_s << qulonglong(23500) << 10  << U'0' << true  << u"000023\u00A0500"_s;
    QTest::newRow("pl_PL 23500 -10 0 f")
            << u"pl_PL"_s << qulonglong(23500) << -10 << U'0' << false << u"2350000000"_s;
    QTest::newRow("pl_PL 23500 -10 0 t")
            << u"pl_PL"_s << qulonglong(23500) << -10 << U'0' << true  << u"23\u00A05000000"_s;

    QTest::newRow("pl_PL 23000000 0 0 t")
            << u"pl_PL"_s << qulonglong(23000000) << 0   << U'0' << true  << u"23\u00A0000\u00A0000"_s;
    QTest::newRow("pl_PL 23000000 0 0 f")
            << u"pl_PL"_s << qulonglong(23000000) << 0   << U'0' << false << u"23000000"_s;
    QTest::newRow("nb_NO 23000000 15 0 t")
            << u"nb_NO"_s << qulonglong(23000000) << 15  << U'0' << true  << u"0000023\u00A0000\u00A0000"_s;
    QTest::newRow("nb_NO 23000000 15 0 f")
            << u"nb_NO"_s << qulonglong(23000000) << 15  << U'0' << false << u"000000023000000"_s;
    QTest::newRow("ja_JP 23000000 -15 0 t")
            << u"ja_JP"_s << qulonglong(23000000) << -15 << U'0' << true  << u"23,000,00000000"_s;
    QTest::newRow("ja_JP 23000000 -15 0 f")
            << u"ja_JP"_s << qulonglong(23000000) << -15 << U'0' << false << u"230000000000000"_s;

    QTest::newRow("hi_IN 23500 0 0 f")
            << u"hi_IN"_s << qulonglong(23500) << 0   << U'0' << false << u"23500"_s;
    QTest::newRow("hi_IN 23500 0 0 t")
            << u"hi_IN"_s << qulonglong(23500) << 0   << U'0' << true  << u"23,500"_s;
    QTest::newRow("hi_IN 23500 10 0 f")
            << u"hi_IN"_s << qulonglong(23500) << 10  << U'0' << false << u"0000023500"_s;
    QTest::newRow("hi_IN 23500 10 0 t")
            << u"hi_IN"_s << qulonglong(23500) << 10  << U'0' << true  << u"000023,500"_s;
    QTest::newRow("hi_IN 23500 -10 0 f")
            << u"hi_IN"_s << qulonglong(23500) << -10 << U'0' << false << u"2350000000"_s;
    QTest::newRow("hi_IN 23500 -10 0 t")
            << u"hi_IN"_s << qulonglong(23500) << -10 << U'0' << true  << u"23,5000000"_s;

    QTest::newRow("hi_IN 23000000 0 0 t")
            << u"hi_IN"_s << qulonglong(23000000) << 0   << U'0' << true  << u"2,30,00,000"_s;
    QTest::newRow("hi_IN 23000000 0 0 f")
            << u"hi_IN"_s << qulonglong(23000000) << 0   << U'0' << false << u"23000000"_s;
    QTest::newRow("hi_IN 23000000 15 0 t")
            << u"hi_IN"_s << qulonglong(23000000) << 15  << U'0' << true  << u"00002,30,00,000"_s;
    QTest::newRow("hi_IN 23000000 15 0 f")
            << u"hi_IN"_s << qulonglong(23000000) << 15  << U'0' << false << u"000000023000000"_s;
    QTest::newRow("hi_IN 23000000 -15 0 t")
            << u"hi_IN"_s << qulonglong(23000000) << -15 << U'0' << true  << u"2,30,00,0000000"_s;
    QTest::newRow("hi_IN 23000000 -15 0 f")
            << u"hi_IN"_s << qulonglong(23000000) << -15 << U'0' << false << u"230000000000000"_s;

    QTest::newRow("emoji 2300 6 😀 f")
        << u"en_US"_s << qulonglong(23000) << 6  << U'😀' << false << u"😀23000"_s;
    QTest::newRow("emoji 2300 -6 😀 f")
        << u"en_US"_s << qulonglong(23000) << -6 << U'😀' << false << u"23000😀"_s;
    QTest::newRow("emoji 2300 7 😀 t")
        << u"en_US"_s << qulonglong(23000) << 7  << U'😀' << true  << u"😀23,000"_s;
    QTest::newRow("emoji 2300 -7 😀 t")
        << u"en_US"_s << qulonglong(23000) << -7 << U'😀' << true  << u"23,000😀"_s;

    QTest::newRow("ar_EG 0 0 x f")
        << u"ar_EG"_s << qulonglong(0) << 0 << U'x' << false << u"\u0660"_s;
    QTest::newRow("ar_EG 0 0 x t")
        << u"ar_EG"_s << qulonglong(0) << 0 << U'x' << true << u"\u0660"_s;

    QTest::newRow("ccp_BD 0 0 𑄃 t")
        << u"ccp_BD"_s << qulonglong(0) << 0  << U'𑄃' << false << u"𑄶"_s;
    QTest::newRow("ccp_BD 0 0 𑄃 f")
        << u"ccp_BD"_s << qulonglong(0) << 0  << U'𑄃' << true << u"𑄶"_s;
    QTest::newRow("ccp_BD 2300 5 𑄃 f")
        << u"ccp_BD"_s << qulonglong(2300) << 5  << U'𑄃' << false << u"𑄃𑄸𑄹𑄶𑄶"_s;
    QTest::newRow("ccp_BD 2300 -5 𑄃 f")
        << u"ccp_BD"_s << qulonglong(2300) << -5 << U'𑄃' << false << u"𑄸𑄹𑄶𑄶𑄃"_s;
    QTest::newRow("ccp_BD 2300 6 𑄃 t")
        << u"ccp_BD"_s << qulonglong(2300) << 6  << U'𑄃' << true  << u"𑄃𑄸,𑄹𑄶𑄶"_s;
    QTest::newRow("ccp_BD 2300 -6 𑄃 t")
        << u"ccp_BD"_s << qulonglong(2300) << -6 << U'𑄃' << true  << u"𑄸,𑄹𑄶𑄶𑄃"_s;
}

void tst_QLocale::qulonglongToString()
{
    QFETCH(QString, localeName);
    QFETCH(QString, numStr);
    QFETCH(qulonglong, number);
    QFETCH(int, fieldWidth);
    QFETCH(char32_t, fillChar);
    QFETCH(bool, grouped);

    QLocale locale(localeName);
    if (grouped) {
        QCOMPARE(locale.toString(number, fieldWidth, fillChar), numStr);
    } else {
        locale.setNumberOptions(QLocale::OmitGroupSeparator);
        QCOMPARE(locale.toString(number, fieldWidth, fillChar), numStr);
    }
}

void tst_QLocale::strtod_data()
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

void tst_QLocale::strtod()
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

void tst_QLocale::long_long_conversion_data()
{
    QTest::addColumn<QString>("locale_name");
    QTest::addColumn<QString>("num_str");
    QTest::addColumn<bool>("good");
    QTest::addColumn<qlonglong>("num");

    QTest::newRow("C null")                   << QString("C")     << QString()       << false << (qlonglong) 0;
    QTest::newRow("C empty")                  << QString("C")     << QString("")     << false << (qlonglong) 0;
    QTest::newRow("C 0")                      << QString("C")     << "0"             << true  << (qlonglong) 0;
    QTest::newRow("C 0,")                     << QString("C")     << "0,"            << false << (qlonglong) 0;
    QTest::newRow("C 1")                      << QString("C")     << "1"             << true  << (qlonglong) 1;
    QTest::newRow("C 1,")                     << QString("C")     << "1,"            << false << (qlonglong) 0;
    QTest::newRow("C 1,2")                    << QString("C")     << "1,2"           << false << (qlonglong) 0;
    QTest::newRow("C 1,23")                   << QString("C")     << "1,23"          << false << (qlonglong) 0;
    QTest::newRow("C 1,234")                  << QString("C")     << "1,234"         << true  << (qlonglong) 1234;
    QTest::newRow("C 1234567")                << QString("C")     << "1234567"       << true  << (qlonglong) 1234567;
    QTest::newRow("C 1,234567")               << QString("C")     << "1,234567"      << false << (qlonglong) 0;
    QTest::newRow("C 12,34567")               << QString("C")     << "12,34567"      << false << (qlonglong) 0;
    QTest::newRow("C 123,4567")               << QString("C")     << "123,4567"      << false << (qlonglong) 0;
    QTest::newRow("C 1234,567")               << QString("C")     << "1234,567"      << false << (qlonglong) 0;
    QTest::newRow("C 12345,67")               << QString("C")     << "12345,67"      << false << (qlonglong) 0;
    QTest::newRow("C 123456,7")               << QString("C")     << "123456,7"      << false << (qlonglong) 0;
    QTest::newRow("C 1,234,567")              << QString("C")     << "1,234,567"     << true  << (qlonglong) 1234567;
    using LL = std::numeric_limits<qlonglong>;
    QTest::newRow("C LLONG_MIN") << QString("C") << QString::number(LL::min()) << true << LL::min();
    QTest::newRow("C LLONG_MAX") << QString("C") << QString::number(LL::max()) << true << LL::max();

    QTest::newRow("de_DE 1")                  << QString("de_DE") << "1"             << true  << (qlonglong) 1;
    QTest::newRow("de_DE 1.")                 << QString("de_DE") << "1."            << false << (qlonglong) 0;
    QTest::newRow("de_DE 1.2")                << QString("de_DE") << "1.2"           << false << (qlonglong) 0;
    QTest::newRow("de_DE 1.23")               << QString("de_DE") << "1.23"          << false << (qlonglong) 0;
    QTest::newRow("de_DE 1.234")              << QString("de_DE") << "1.234"         << true  << (qlonglong) 1234;
    QTest::newRow("de_DE 1234567")            << QString("de_DE") << "1234567"       << true  << (qlonglong) 1234567;
    QTest::newRow("de_DE 1.234567")           << QString("de_DE") << "1.234567"      << false << (qlonglong) 0;
    QTest::newRow("de_DE 12.34567")           << QString("de_DE") << "12.34567"      << false << (qlonglong) 0;
    QTest::newRow("de_DE 123.4567")           << QString("de_DE") << "123.4567"      << false << (qlonglong) 0;
    QTest::newRow("de_DE 1234.567")           << QString("de_DE") << "1234.567"      << false << (qlonglong) 0;
    QTest::newRow("de_DE 12345.67")           << QString("de_DE") << "12345.67"      << false << (qlonglong) 0;
    QTest::newRow("de_DE 123456.7")           << QString("de_DE") << "123456.7"      << false << (qlonglong) 0;
    QTest::newRow("de_DE 1.234.567")          << QString("de_DE") << "1.234.567"     << true  << (qlonglong) 1234567;
    QTest::newRow("de_DE 1.234.567 ldspcs")   << QString("de_DE") << "  1.234.567"   << true  << (qlonglong) 1234567;
    QTest::newRow("de_DE 1.234.567 trspcs")   << QString("de_DE") << "1.234.567  "   << true  << (qlonglong) 1234567;
    QTest::newRow("de_DE 1.234.567 ldtrspcs") << QString("de_DE") << "  1.234.567  " << true  << (qlonglong) 1234567;

    // test that space is also accepted whenever QLocale::groupSeparator() == 0xa0 (which looks like space).
    QTest::newRow("nb_NO 123 groupsep")       << QString("nb_NO") << QString("1")+QChar(0xa0)+QString("234") << true  << (qlonglong) 1234;
    QTest::newRow("nb_NO 123 groupsep_space") << QString("nb_NO") << QString("1")+QChar(0x20)+QString("234") << true  << (qlonglong) 1234;

    QTest::newRow("nb_NO 123 ldspcs")         << QString("nb_NO") << "  123"         << true  << (qlonglong) 123;
    QTest::newRow("nb_NO 123 trspcs")         << QString("nb_NO") << "123  "         << true  << (qlonglong) 123;
    QTest::newRow("nb_NO 123 ldtrspcs")       << QString("nb_NO") << "  123  "       << true  << (qlonglong) 123;

    QTest::newRow("C   1234")                 << QString("C")     << "  1234"        << true  << (qlonglong) 1234;
    QTest::newRow("C 1234  ")                 << QString("C")     << "1234  "        << true  << (qlonglong) 1234;
    QTest::newRow("C   1234  ")               << QString("C")     << "  1234  "      << true  << (qlonglong) 1234;
}

void tst_QLocale::long_long_conversion()
{
    QFETCH(QString, locale_name);
    QFETCH(QString, num_str);
    QFETCH(bool, good);
    QFETCH(qlonglong, num);
    QStringView num_strRef{ num_str };

    QLocale locale(locale_name);
    QCOMPARE(locale.name(), locale_name);

    bool ok;
    qlonglong l = locale.toLongLong(num_str, &ok);
    QCOMPARE(ok, good);

    if (ok)
        QCOMPARE(l, num);

    l = locale.toLongLong(num_strRef, &ok);
    QCOMPARE(ok, good);

    if (ok)
        QCOMPARE(l, num);

    if (num >= 0) {
        qulonglong ull = locale.toULongLong(num_str, &ok);
        QCOMPARE(ok, good);

        if (ok)
            QCOMPARE(ull, num);

        ull = locale.toULongLong(num_strRef, &ok);
        QCOMPARE(ok, good);

        if (ok)
            QCOMPARE(ull, num);
    }
}

void tst_QLocale::long_long_conversion_extra()
{
    QLocale l(QLocale::C);
    l.setNumberOptions({ });
    QCOMPARE(l.toString((qlonglong)1), QString("1"));
    QCOMPARE(l.toString((qlonglong)12), QString("12"));
    QCOMPARE(l.toString((qlonglong)123), QString("123"));
    QCOMPARE(l.toString((qlonglong)1234), QString("1,234"));
    QCOMPARE(l.toString((qlonglong)12345), QString("12,345"));
    QCOMPARE(l.toString((qlonglong)-1), QString("-1"));
    QCOMPARE(l.toString((qlonglong)-12), QString("-12"));
    QCOMPARE(l.toString((qlonglong)-123), QString("-123"));
    QCOMPARE(l.toString((qlonglong)-1234), QString("-1,234"));
    QCOMPARE(l.toString((qlonglong)-12345), QString("-12,345"));
    QCOMPARE(l.toString((qulonglong)1), QString("1"));
    QCOMPARE(l.toString((qulonglong)12), QString("12"));
    QCOMPARE(l.toString((qulonglong)123), QString("123"));
    QCOMPARE(l.toString((qulonglong)1234), QString("1,234"));
    QCOMPARE(l.toString((qulonglong)12345), QString("12,345"));
}

void tst_QLocale::infNaN()
{
    // TODO: QTBUG-95460 -- could support localized forms of inf/NaN
    const QLocale c(QLocale::C);

    QT_TEST_EQUALITY_OPS(QLocale(), QLocale(QLocale::C), false);
    QT_TEST_EQUALITY_OPS(QLocale(), QLocale(), true);
    QT_TEST_EQUALITY_OPS(QLocale(QLocale::C), c, true);

    QCOMPARE(c.toString(qQNaN()), u"nan");
    QCOMPARE(c.toString(qQNaN(), 'e'), u"nan");
    QCOMPARE(c.toString(qQNaN(), 'f'), u"nan");
    QCOMPARE(c.toString(qQNaN(), 'g'), u"nan");
    QCOMPARE(c.toString(qQNaN(), 'E'), u"NAN");
    QCOMPARE(c.toString(qQNaN(), 'F'), u"NAN");
    QCOMPARE(c.toString(qQNaN(), 'G'), u"NAN");

    QCOMPARE(c.toString(qInf()), u"inf");
    QCOMPARE(c.toString(qInf(), 'e'), u"inf");
    QCOMPARE(c.toString(qInf(), 'f'), u"inf");
    QCOMPARE(c.toString(qInf(), 'g'), u"inf");
    QCOMPARE(c.toString(qInf(), 'E'), u"INF");
    QCOMPARE(c.toString(qInf(), 'F'), u"INF");
    QCOMPARE(c.toString(qInf(), 'G'), u"INF");

    // Precision is ignored for inf and NaN:
    QCOMPARE(c.toString(qQNaN(), 'g', 42), u"nan");
    QCOMPARE(c.toString(qQNaN(), 'G', 42), u"NAN");
    QCOMPARE(c.toString(qInf(), 'g', 42), u"inf");
    QCOMPARE(c.toString(qInf(), 'G', 42), u"INF");

    // Case is ignored when parsing inf and NaN:
    bool ok = false;
    QCOMPARE(c.toDouble("inf", &ok), qInf());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("INF", &ok), qInf());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("Inf", &ok), qInf());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("+inf", &ok), qInf());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("+INF", &ok), qInf());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("+inF", &ok), qInf());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("-inf", &ok), -qInf());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("-INF", &ok), -qInf());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("-iNf", &ok), -qInf());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("nan", &ok), qQNaN());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("NaN", &ok), qQNaN());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("NAN", &ok), qQNaN());
    QVERIFY(ok);
    QCOMPARE(c.toDouble("nAn", &ok), qQNaN());
    QVERIFY(ok);
    // Sign is invalid for NaN:
    QCOMPARE(c.toDouble("-nan", &ok), 0.0);
    QVERIFY(!ok);
    QCOMPARE(c.toDouble("+nan", &ok), 0.0);
    QVERIFY(!ok);


    // Case is ignored when parsing inf and NaN:
    QCOMPARE(c.toFloat("inf", &ok), float(qInf()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("INF", &ok), float(qInf()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("Inf", &ok), float(qInf()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("+inf", &ok), float(qInf()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("+INF", &ok), float(qInf()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("+inF", &ok), float(qInf()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("-inf", &ok), -float(qInf()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("-INF", &ok), -float(qInf()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("-iNf", &ok), -float(qInf()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("nan", &ok), float(qQNaN()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("NaN", &ok), float(qQNaN()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("NAN", &ok), float(qQNaN()));
    QVERIFY(ok);
    QCOMPARE(c.toFloat("nAn", &ok), float(qQNaN()));
    QVERIFY(ok);
    // Sign is invalid for NaN:
    QCOMPARE(c.toFloat("-nan", &ok), 0.0f);
    QVERIFY(!ok);
    QCOMPARE(c.toFloat("+nan", &ok), 0.0f);
    QVERIFY(!ok);
}

void tst_QLocale::fpExceptions()
{
#if defined(FE_ALL_EXCEPT) && FE_ALL_EXCEPT != 0
    // Check that double-to-string conversion doesn't throw floating point
    // exceptions when they are enabled.
    fenv_t envp;
    fegetenv(&envp);
    feclearexcept(FE_ALL_EXCEPT);

    QString::number(1000.1245);
    QString::number(1.1);
    QString::number(0.0);

    QVERIFY(true);

    QCOMPARE(fetestexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW | FE_INVALID), 0);
    fesetenv(&envp);
#endif
}

void tst_QLocale::negativeZero_data()
{
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<QLocale::Script>("script");
    QTest::addColumn<QLocale::Territory>("territory");
    QTest::addColumn<QStringView>("expect");

    QTest::newRow("C")
        << QLocale::C << QLocale::AnyScript << QLocale::AnyTerritory
        << QStringView(u"0");
    QTest::newRow("Arabic")
        << QLocale::Arabic << QLocale::ArabicScript << QLocale::AnyTerritory
        << QStringView(u"\u0660");
    QTest::newRow("Chakma")
        << QLocale::Chakma << QLocale::ChakmaScript << QLocale::AnyTerritory
        << QStringView(u"\xD804\xDD36"); // A surrogate pair.
}

void tst_QLocale::negativeZero()
{
    QFETCH(QLocale::Language, language);
    QFETCH(QLocale::Script, script);
    QFETCH(QLocale::Territory, territory);
    QFETCH(QStringView, expect);
    QLocale locale(language, script, territory);
    QCOMPARE(locale.toString(std::copysign(0.0, -1.0)), expect);
}

void tst_QLocale::signsNeverCompareEqualToNullCharacter() // otherwise QTextStream has a problem
{
    QFETCH(QLocale::Language, language);
    QFETCH(const QLocale::Territory, country);

    if (language == QLocale::AnyLanguage && country == QLocale::AnyTerritory)
        language = QLocale::C;

    const QLocale loc(language, country);
    QCOMPARE_NE(loc.negativeSign(), QChar());
    QCOMPARE_NE(loc.positiveSign(), QChar());
}

void tst_QLocale::dayOfWeek_data()
{
    QTest::addColumn<QDate>("date");
    QTest::addColumn<QString>("shortName");
    QTest::addColumn<QString>("longName");

    QTest::newRow("Sun") << QDate(2006, 1, 1) << "Sun" << "Sunday";
    QTest::newRow("Mon") << QDate(2006, 1, 2) << "Mon" << "Monday";
    QTest::newRow("Tue") << QDate(2006, 1, 3) << "Tue" << "Tuesday";
    QTest::newRow("Wed") << QDate(2006, 1, 4) << "Wed" << "Wednesday";
    QTest::newRow("Thu") << QDate(2006, 1, 5) << "Thu" << "Thursday";
    QTest::newRow("Fri") << QDate(2006, 1, 6) << "Fri" << "Friday";
    QTest::newRow("Sat") << QDate(2006, 1, 7) << "Sat" << "Saturday";
}

void tst_QLocale::dayOfWeek()
{
    QFETCH(QDate, date);
    QFETCH(QString, shortName);
    QFETCH(QString, longName);

    QCOMPARE(QLocale::c().toString(date, "ddd"), shortName);
    QCOMPARE(QLocale::c().toString(date, "dddd"), longName);

    QCOMPARE(QLocale::c().toString(date, u"ddd"), shortName);
    QCOMPARE(QLocale::c().toString(date, u"dddd"), longName);
}

void tst_QLocale::formatDate_data()
{
    QTest::addColumn<QDate>("date");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("result");

    QTest::newRow("1") << QDate(1974, 12, 1) << "d/M/yyyy" << "1/12/1974";
    QTest::newRow("2") << QDate(1974, 12, 1) << "d/M/yyyyy" << "1/12/1974y";
    QTest::newRow("4") << QDate(1974, 1, 1) << "d/M/yyyy" << "1/1/1974";
    QTest::newRow("5") << QDate(1974, 1, 1) << "dd/MM/yyy" << "01/01/74y";
    QTest::newRow("6") << QDate(1974, 12, 1) << "ddd/MMM/yy" << "Sun/Dec/74";
    QTest::newRow("7") << QDate(1974, 12, 1) << "dddd/MMMM/y" << "Sunday/December/y";
    QTest::newRow("8") << QDate(1974, 12, 1) << "ddddd/MMMMM/yy" << "Sunday1/December12/74";
    QTest::newRow("9") << QDate(1974, 12, 1) << "'dddd'/MMMM/yy" << "dddd/December/74";
    QTest::newRow("10") << QDate(1974, 12, 1) << "d'dd'd/MMMM/yyy" << "1dd1/December/74y";
    QTest::newRow("11") << QDate(1974, 12, 1) << "d'dd'd/MMM'M'/yy" << "1dd1/DecM/74";
    QTest::newRow("12") << QDate(1974, 12, 1) << "d'd'dd/M/yy" << "1d01/12/74";

    QTest::newRow("20") << QDate(1974, 12, 1) << "foo" << "foo";
    QTest::newRow("21") << QDate(1974, 12, 1) << "'" << "";
    QTest::newRow("22") << QDate(1974, 12, 1) << "''" << "'";
    QTest::newRow("23") << QDate(1974, 12, 1) << "'''" << "'";
    QTest::newRow("24") << QDate(1974, 12, 1) << "\"" << "\"";
    QTest::newRow("25") << QDate(1974, 12, 1) << "\"\"" << "\"\"";
    QTest::newRow("26") << QDate(1974, 12, 1) << "\"yy\"" << "\"74\"";
    QTest::newRow("27") << QDate(1974, 12, 1) << "'\"yy\"'" << "\"yy\"";
    QTest::newRow("28") << QDate() << "'\"yy\"'" << "";
    QTest::newRow("29")
        << QDate(1974, 12, 1) << "hh:mm:ss.zzz ap d'd'dd/M/yy" << "hh:mm:ss.zzz ap 1d01/12/74";

    QTest::newRow("dd MMMM yyyy") << QDate(1, 1, 1) << "dd MMMM yyyy" << "01 January 0001";

    // Test unicode handling.
    QTest::newRow("unicode in format string") << QDate(1, 1, 1)
            << u8"🔴😤🌼😫dd☀😥🤤👉💃MM💛🙂💓🤩yyyy😴"
            << u8"🔴😤🌼😫01☀😥🤤👉💃01💛🙂💓🤩0001😴";
}

void tst_QLocale::formatDate()
{
    QFETCH(QDate, date);
    QFETCH(QString, format);
    QFETCH(QString, result);

    QLocale l(QLocale::C);
    QCOMPARE(l.toString(date, format), result);
    QCOMPARE(l.toString(date, QStringView(format)), result);
}

void tst_QLocale::formatTime_data()
{
    QTest::addColumn<QTime>("time");
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("result");

    QTest::newRow("C-h:m:s-am") << QTime(1, 2, 3) << "C" << "h:m:s" << "1:2:3";
    QTest::newRow("C-H:m:s-am") << QTime(1, 2, 3) << "C" << "H:m:s" << "1:2:3";
    QTest::newRow("C-hh:mm:ss-am") << QTime(1, 2, 3) << "C" << "hh:mm:ss" << "01:02:03";
    QTest::newRow("C-HH:mm:ss-am") << QTime(1, 2, 3) << "C" << "HH:mm:ss" << "01:02:03";
    QTest::newRow("C-hhh:mmm:sss-am") << QTime(1, 2, 3) << "C" << "hhh:mmm:sss" << "011:022:033";

    QTest::newRow("C-h:m:s-pm") << QTime(14, 2, 3) << "C" << "h:m:s" << "14:2:3";
    QTest::newRow("C-H:m:s-pm") << QTime(14, 2, 3) << "C" << "H:m:s" << "14:2:3";
    QTest::newRow("C-hh:mm:ss-pm") << QTime(14, 2, 3) << "C" << "hh:mm:ss" << "14:02:03";
    QTest::newRow("C-HH:mm:ss-pm") << QTime(14, 2, 3) << "C" << "HH:mm:ss" << "14:02:03";
    QTest::newRow("C-hhh:mmm:sss-pm") << QTime(14, 2, 3) << "C" << "hhh:mmm:sss" << "1414:022:033";

    QTest::newRow("C-h:m:s+ap-pm") << QTime(14, 2, 3) << "C" << "h:m:s ap" << "2:2:3 pm";
    QTest::newRow("C-H:m:s+AP-pm") << QTime(14, 2, 3) << "C" << "H:m:s AP" << "14:2:3 PM";
    QTest::newRow("C-hh:mm:ss+aap-pm")
        << QTime(14, 2, 3) << "C" << "hh:mm:ss aap" << "02:02:03 pmpm";
    QTest::newRow("C-HH:mm:ss+AP+aa-pm")
        << QTime(14, 2, 3) << "C" << "HH:mm:ss AP aa" << "14:02:03 PM pmpm";

    QTest::newRow("C-h:m:s+ap-am") << QTime(1, 2, 3) << "C" << "h:m:s ap" << "1:2:3 am";
    QTest::newRow("C-H:m:s+AP-am") << QTime(1, 2, 3) << "C" << "H:m:s AP" << "1:2:3 AM";

    QTest::newRow("C-foo") << QTime(1, 2, 3) << "C" << "foo" << "foo";
    QTest::newRow("C-quote") << QTime(1, 2, 3) << "C" << "'" << "";
    QTest::newRow("C-quote*2") << QTime(1, 2, 3) << "C" << "''" << "'";
    QTest::newRow("C-quote*3") << QTime(1, 2, 3) << "C" << "'''" << "'";
    QTest::newRow("C-dquote") << QTime(1, 2, 3) << "C" << "\"" << "\"";
    QTest::newRow("C-dquote*2") << QTime(1, 2, 3) << "C" << "\"\"" << "\"\"";
    QTest::newRow("C-dquote-H") << QTime(1, 2, 3) << "C" << "\"H\"" << "\"1\"";
    QTest::newRow("C-quote-dquote-H") << QTime(1, 2, 3) << "C" << "'\"H\"'" << "\"H\"";

    QTest::newRow("C-H:m:s.z") << QTime(1, 2, 3, 456) << "C" << "H:m:s.z" << "1:2:3.456";
    QTest::newRow("C-H:m:s.zz") << QTime(1, 2, 3, 456) << "C" << "H:m:s.zz" << "1:2:3.456";
    QTest::newRow("C-H:m:s.zzz") << QTime(1, 2, 3, 456) << "C" << "H:m:s.zzz" << "1:2:3.456";
    QTest::newRow("C-H:m:s.z=400") << QTime(1, 2, 3, 400) << "C" << "H:m:s.z" << "1:2:3.4";
    QTest::newRow("C-H:m:s.zzz=400") << QTime(1, 2, 3, 400) << "C" << "H:m:s.zzz" << "1:2:3.400";
    QTest::newRow("C-H:m:s.z=004") << QTime(1, 2, 3, 4) << "C" << "H:m:s.z" << "1:2:3.004";
    QTest::newRow("C-H:m:s.zzz=004") << QTime(1, 2, 3, 4) << "C" << "H:m:s.zzz" << "1:2:3.004";

    QTest::newRow("C-invalid") << QTime() << "C" << "H:m:s.zzz" << "";
    QTest::newRow("C-date-time")
        << QTime(1, 2, 3, 4) << "C" << "dd MM yyyy H:m:s.zzz" << "dd MM yyyy 1:2:3.004";

    // Test unicode handling.
    QTest::newRow("C-emoji")
        << QTime(17, 22, 05, 18) << "C" << u8"m📌ss📢H.zzz" << u8"22📌05📢17.018";

    // Test-cases related to QTBUG-95790 (case of localized am/pm indicators):
    QTest::newRow("en_US-h:m:s+ap-pm")
        << QTime(14, 2, 3) << "en_US" << "h:m:s ap" << "2:2:3 pm";
    QTest::newRow("en_US-H:m:s+AP-pm")
        << QTime(14, 2, 3) << "en_US" << "H:m:s AP" << "14:2:3 PM";
    QTest::newRow("en_US-H:m:s+Ap-pm")
        << QTime(14, 2, 3) << "en_US" << "H:m:s Ap" << "14:2:3 PM";
    QTest::newRow("en_US-h:m:s+ap-am")
        << QTime(1, 2, 3) << "en_US" << "h:m:s ap" << "1:2:3 am";
    QTest::newRow("en_US-H:m:s+AP-am")
        << QTime(1, 2, 3) << "en_US" << "H:m:s AP" << "1:2:3 AM";
    QTest::newRow("en_US-H:m:s+aP-am")
        << QTime(1, 2, 3) << "en_US" << "H:m:s aP" << "1:2:3 AM";

    QTest::newRow("cs_CZ-h:m:s+ap-pm")
        << QTime(14, 2, 3) << "cs_CZ" << "h:m:s ap" << "2:2:3 odp.";
    QTest::newRow("cs_CZ-h:m:s+AP-pm")
        << QTime(14, 2, 3) << "cs_CZ" << "h:m:s AP" << "2:2:3 ODP.";
    QTest::newRow("cs_CZ-h:m:s+Ap-pm")
        << QTime(14, 2, 3) << "cs_CZ" << "h:m:s Ap" << "2:2:3 odp.";
    QTest::newRow("cs_CZ-h:m:s+ap-am")
        << QTime(1, 2, 3) << "cs_CZ" << "h:m:s ap" << "1:2:3 dop.";
    QTest::newRow("cs_CZ-h:m:s+AP-am")
        << QTime(1, 2, 3) << "cs_CZ" << "h:m:s AP" << "1:2:3 DOP.";
    QTest::newRow("cs_CZ-h:m:s+aP-am")
        << QTime(1, 2, 3) << "cs_CZ" << "h:m:s aP" << "1:2:3 dop.";
}

void tst_QLocale::formatTime()
{
    QFETCH(const QTime, time);
    QFETCH(const QString, locale);
    QFETCH(const QString, format);
    QFETCH(const QString, result);

    QLocale l(locale);
    QCOMPARE(l.toString(time, format), result);
    QCOMPARE(l.toString(time, QStringView(format)), result);
}


void tst_QLocale::formatDateTime_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QDateTime>("dateTime");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("result");

    QTest::newRow("1C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(5, 14, 13))
                        << "d/M/yyyy hh:h:mm" << "1/12/1974 05:5:14";
    QTest::newRow("2C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                        << "d/M/yyyyy h" << "1/12/1974y 15";
    QTest::newRow("4C") << "C" << QDateTime(QDate(1974, 1, 1), QTime(15, 14, 13))
                        << "d/M/yyyy zzz" << "1/1/1974 000";
    QTest::newRow("5C") << "C" << QDateTime(QDate(1974, 1, 1), QTime(15, 14, 13))
                        << "dd/MM/yyy z" << "01/01/74y 0";
    QTest::newRow("6C") << "C" << QDateTime(QDate(1974, 12, 2), QTime(15, 14, 13))
                        << "ddd/MMM/yy AP" << "Mon/Dec/74 PM";
    QTest::newRow("7C") << "C" << QDateTime(QDate(1974, 12, 2), QTime(15, 14, 13))
                        << "dddd/MMMM/y apa" << "Monday/December/y pmpm";
    QTest::newRow("8C") << "C" << QDateTime(QDate(1974, 12, 2), QTime(15, 14, 13))
                        << "ddddd/MMMMM/yy ss" << "Monday2/December12/74 13";
    QTest::newRow("9C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                        << "'dddd'/MMMM/yy s" << "dddd/December/74 13";
    QTest::newRow("10C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 4, 13))
                         << "d'dd'd/MMMM/yyy m'm'mm" << "1dd1/December/74y 4m04";
    QTest::newRow("11C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 3))
                         << "d'dd'd/MMM'M'/yysss" << "1dd1/DecM/74033";
    QTest::newRow("12C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                         << "d'd'dd/M/yyh" << "1d01/12/7415";

    QTest::newRow("dd MMMM yyyy, hh:mm:ss") << "C" << QDateTime(QDate(1, 1, 1), QTime(12, 00, 00))
                         << "dd MMMM yyyy, hh:mm:ss" << "01 January 0001, 12:00:00";

    QTest::newRow("20C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                         << "foo" << "foo";
    QTest::newRow("21C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                         << "'" << "";
    QTest::newRow("22C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                         << "''" << "'";
    QTest::newRow("23C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                         << "'''" << "'";
    QTest::newRow("24C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                         << "\"" << "\"";
    QTest::newRow("25C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                         << "\"\"" << "\"\"";
    QTest::newRow("26C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                         << "\"yymm\"" << "\"7414\"";
    QTest::newRow("27C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                         << "'\"yymm\"'" << "\"yymm\"";
    QTest::newRow("28C") << "C" << QDateTime()
                         << "'\"yymm\"'" << "";

    QTest::newRow("1no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(5, 14, 13))
                            << "d/M/yyyy hh:h:mm" << "1/12/1974 05:5:14";
    QTest::newRow("2no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                            << "d/M/yyyyy h" << "1/12/1974y 15";
    QTest::newRow("4no_NO") << "no_NO" << QDateTime(QDate(1974, 1, 1), QTime(15, 14, 13))
                            << "d/M/yyyy zzz" << "1/1/1974 000";
    QTest::newRow("5no_NO") << "no_NO" << QDateTime(QDate(1974, 1, 1), QTime(15, 14, 13))
                            << "dd/MM/yyy z" << "01/01/74y 0";
    QTest::newRow("6no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 2), QTime(15, 14, 13))
                            << "ddd/MMM/yy AP" << "man./des./74 P.M.";
    QTest::newRow("7no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 2), QTime(15, 14, 13))
                            << "dddd/MMMM/y apa" << "mandag/desember/y p.m.p.m.";
    QTest::newRow("8no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 2), QTime(15, 14, 13))
                            << "ddddd/MMMMM/yy ss" << "mandag2/desember12/74 13";
    QTest::newRow("9no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                            << "'dddd'/MMMM/yy s" << "dddd/desember/74 13";
    QTest::newRow("10no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 4, 13))
                             << "d'dd'd/MMMM/yyy m'm'mm" << "1dd1/desember/74y 4m04";
    QTest::newRow("11no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 3))
                             << "d'dd'd/MMM'M'/yysss" << "1dd1/des.M/74033";
    QTest::newRow("12no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                             << "d'd'dd/M/yyh" << "1d01/12/7415";

    QTest::newRow("20no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                             << "foo" << "foo";
    QTest::newRow("21no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                             << "'" << "";
    QTest::newRow("22no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                             << "''" << "'";
    QTest::newRow("23no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                             << "'''" << "'";
    QTest::newRow("24no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                             << "\"" << "\"";
    QTest::newRow("25no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                             << "\"\"" << "\"\"";
    QTest::newRow("26no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                             << "\"yymm\"" << "\"7414\"";
    QTest::newRow("27no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 14, 13))
                             << "'\"yymm\"'" << "\"yymm\"";
    QTest::newRow("28no_NO") << "no_NO" << QDateTime()
                             << "'\"yymm\"'" << "";

    QDateTime testLongHour(QDate(1999, 12, 31), QTime(23, 59, 59, 999));
    QDateTime testShortHour(QDate(1999, 12, 31), QTime(3, 59, 59, 999));
    QDateTime testZeroHour(QDate(1999, 12, 31), QTime(0, 59, 59, 999));

    QTest::newRow("datetime0")      << "en_US" << QDateTime()
                                    << QString("dd-MM-yyyy hh:mm:ss") << QString();
    QTest::newRow("datetime1")      << "en_US" << testLongHour
                                    << QString("dd-'mmddyy'MM-yyyy hh:mm:ss.zzz")
                                    << QString("31-mmddyy12-1999 23:59:59.999");
    QTest::newRow("datetime2")      << "en_US" << testLongHour
                                    << QString("dd-'apAP'MM-yyyy hh:mm:ss.zzz")
                                    << QString("31-apAP12-1999 23:59:59.999");
    QTest::newRow("datetime3")      << "en_US" << testLongHour
                                    << QString("Apdd-MM-yyyy hh:mm:ss.zzz")
                                    << QString("PM31-12-1999 11:59:59.999");
    QTest::newRow("datetime4")      << "en_US" << testLongHour
                                    << QString("'ap'apdd-MM-yyyy 'AP'hh:mm:ss.zzz")
                                    << QString("appm31-12-1999 AP11:59:59.999");
    QTest::newRow("datetime5")      << "en_US" << testLongHour
                                    << QString("'''") << QString("'");
    QTest::newRow("datetime6")      << "en_US" << testLongHour
                                    << QString("'ap") << QString("ap");
    QTest::newRow("datetime7")      << "en_US" << testLongHour
                                    << QString("' ' 'hh' hh") << QString("  hh 23");
    QTest::newRow("datetime8")      << "en_US" << testLongHour
                                    << QString("d'foobar'") << QString("31foobar");
    QTest::newRow("datetime9")      << "en_US" << testShortHour
                                    << QString("hhhhh") << QString("03033");
    QTest::newRow("datetime11")     << "en_US" << testLongHour
                                    << QString("HHHhhhAaAPap") << QString("23231111PMpmPMpm");
    QTest::newRow("datetime12")     << "en_US" << testShortHour
                                    << QString("HHHhhhAaAPap") << QString("033033AMamAMam");
    QTest::newRow("datetime13")     << "en_US" << QDateTime(QDate(1974, 12, 1), QTime(14, 14, 20))
                                    << QString("hh''mm''ss dd''MM''yyyy")
                                    << QString("14'14'20 01'12'1974");
    QTest::newRow("AM no p")        << "en_US" << testZeroHour
                                    << QString("hhAX") << QString("12AMX");
    QTest::newRow("AM no p, x 2")   << "en_US" << testShortHour
                                    << QString("hhhhhaA") << QString("03033amAM");
    QTest::newRow("am 0 hour")      << "en_US" << testZeroHour
                                    << QString("hAP") << QString("12AM");
    QTest::newRow("AM zero hour")   << "en_US" << testZeroHour
                                    << QString("hhAP") << QString("12AM");
    QTest::newRow("dddd")           << "en_US" << testZeroHour
                                    << QString("dddd") << QString("Friday");
    QTest::newRow("ddd")            << "en_US" << testZeroHour
                                    << QString("ddd") << QString("Fri");
    QTest::newRow("MMMM")           << "en_US" << testZeroHour
                                    << QString("MMMM") << QString("December");
    QTest::newRow("MMM")            << "en_US" << testZeroHour
                                    << QString("MMM") << QString("Dec");
    QTest::newRow("empty")          << "en_US" << testZeroHour
                                    << QString("") << QString("");

    // Test unicode handling.
    QTest::newRow("emoji in format string")
        << "en_US" << QDateTime(QDate(1980, 2, 1), QTime(17, 12))
        << QString(u8"💖yyyy💖MM💖dd hh💖mm") << u8"💖1980💖02💖01 17💖12";
}

void tst_QLocale::formatDateTime()
{
    QFETCH(QString, localeName);
    QFETCH(QDateTime, dateTime);
    QFETCH(QString, format);
    QFETCH(QString, result);

    QLocale l(localeName);
    QCOMPARE(l.toString(dateTime, format), result);
    QCOMPARE(l.toString(dateTime, QStringView(format)), result);
}

void tst_QLocale::formatTimeZone()
{
    QLocale enUS("en_US");

    QDateTime dt1(QDate(2013, 1, 1), QTime(1, 0), QTimeZone::fromSecondsAheadOfUtc(60 * 60));
    QCOMPARE(enUS.toString(dt1, "t"), QLatin1String("UTC+01:00"));

    QDateTime dt2(QDate(2013, 1, 1), QTime(1, 0), QTimeZone::fromSecondsAheadOfUtc(-60 * 60));
    QCOMPARE(enUS.toString(dt2, "t"), QLatin1String("UTC-01:00"));

    QDateTime dt3(QDate(2013, 1, 1), QTime(0, 0), QTimeZone::UTC);
    QCOMPARE(enUS.toString(dt3, "t"), QLatin1String("UTC"));

    // LocalTime should vary
    if (europeanTimeZone) {
        // Time definitely in Standard Time
        const QStringList knownCETus = {
            u"GMT+1"_s, // ICU
            u"CET"_s // Standard abbreviation
        };
        const QString cet = enUS.toString(QDate(2013, 1, 1).startOfDay(), u"t");
        QVERIFY2(knownCETus.contains(cet), cet.isEmpty() ? "[empty]" : qPrintable(cet));

        // Time definitely in Daylight Time
        const QStringList knownCESTus = {
            u"GMT+2"_s, // ICU
            u"CEST"_s // Standard abbreviation
        };
        const QString cest = enUS.toString(QDate(2013, 6, 1).startOfDay(), u"t");
        QVERIFY2(knownCESTus.contains(cest), cest.isEmpty() ? "[empty]" : qPrintable(cest));
    } else {
        qDebug("(Skipped some CET-only tests)");
    }

#if QT_CONFIG(timezone)
    const QTimeZone berlin("Europe/Berlin");
    const QDateTime jan(QDate(2010, 1, 1).startOfDay(berlin));
    const QDateTime jul(QDate(2010, 7, 1).startOfDay(berlin));

    QCOMPARE(enUS.toString(jan, "t"), berlin.displayName(jan, QTimeZone::ShortName, enUS));
    QCOMPARE(enUS.toString(jul, "t"), berlin.displayName(jul, QTimeZone::ShortName, enUS));
#endif

    // Current datetime should use current zone's abbreviation:
    const auto now = QDateTime::currentDateTime();
    QString zone;
#if QT_CONFIG(timezone) // Match logic in QDTP's startsWithLocalTimeZone() helper.
    zone = now.timeRepresentation().displayName(now, QTimeZone::ShortName, enUS);
    if (zone.isEmpty()) // Fall back to unlocalized from when no timezone backend:
#endif
        zone = now.timeZoneAbbreviation();
    QCOMPARE(enUS.toString(now, "t"), zone);

    // Time on its own will always use the current local time zone:
    QCOMPARE(enUS.toString(now.time(), "t"), zone);
}

void tst_QLocale::toDateTime_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QDateTime>("result");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("string");
    // No non-format letters in format string, no time-zone (t format):
    QTest::addColumn<bool>("clean");

    QTest::newRow("1C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(5, 14, 0))
                        << "d/M/yyyy hh:h:mm" << "1/12/1974 05:5:14" << true;
    QTest::newRow("2C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 0, 0))
                        << "d/M/yyyyy h" << "1/12/1974y 15" << false;
    QTest::newRow("4C") << "C" << QDateTime(QDate(1974, 1, 1), QTime(0, 0, 0, 1))
                        << "d/M/yyyy zzz" << "1/1/1974 001" << true;
    QTest::newRow("5C") << "C" << QDateTime(QDate(1974, 1, 1), QTime(0, 0, 0, 1))
                        << "dd/MM/yyy z" << "01/01/74y 001" << false;
    QTest::newRow("6C") << "C" << QDateTime(QDate(1974, 1, 1), QTime(0, 0, 0, 100))
                        << "dd/MM/yyy z" << "01/01/74y 1" << false;
    QTest::newRow("8C") << "C" << QDateTime(QDate(1974, 12, 2), QTime(0, 0, 13))
                        << "ddddd/MMMMM/yy ss" << "Monday2/December12/74 13" << true;
    QTest::newRow("9C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(0, 0, 13))
                        << "'dddd'/MMMM/yy s" << "dddd/December/74 13" << false;
    QTest::newRow("10C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(0, 4, 0))
                         << "d'dd'd/MMMM/yyy m'm'mm" << "1dd1/December/74y 4m04" << false;
    QTest::newRow("11C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(0, 0, 3))
                         << "d'dd'd/MMM'M'/yysss" << "1dd1/DecM/74033" << false;
    QTest::newRow("12C") << "C" << QDateTime(QDate(1974, 12, 1), QTime(15, 0, 0))
                         << "d'd'dd/M/yyh" << "1d01/12/7415" << false;
    // Unpadded value for fixed-width field is wrong:
    QTest::newRow("bad-day-C") << "C" << QDateTime() << "dd-MMM-yy" << "4-Jun-11" << true;
    QTest::newRow("bad-month-C") << "C" << QDateTime() << "d-MM-yy" << "4-6-11" << true;
    QTest::newRow("bad-year-C") << "C" << QDateTime() << "d-MMM-yyyy" << "4-Jun-11" << true;
    QTest::newRow("bad-hour-C") << "C" << QDateTime() << "d-MMM-yy hh:m" << "4-Jun-11 1:2" << true;
    QTest::newRow("bad-min-C") << "C" << QDateTime() << "d-MMM-yy h:mm" << "4-Jun-11 1:2" << true;
    QTest::newRow("bad-sec-C")
        << "C" << QDateTime() << "d-MMM-yy h:m:ss" << "4-Jun-11 1:2:3" << true;
    QTest::newRow("bad-milli-C")
        << "C" << QDateTime() << "d-MMM-yy h:m:s.zzz" << "4-Jun-11 1:2:3.4" << true;
    QTest::newRow("ok-C") << "C" << QDateTime(QDate(1911, 6, 4), QTime(1, 2, 3, 400))
                          << "d-MMM-yy h:m:s.z" << "4-Jun-11 1:2:3.4" << true;

    QTest::newRow("1no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(5, 14, 0))
                            << "d/M/yyyy hh:h:mm" << "1/12/1974 05:5:14" << true;
    QTest::newRow("2no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 0, 0))
                            << "d/M/yyyyy h" << "1/12/1974y 15" << false;
    QTest::newRow("4no_NO") << "no_NO" << QDateTime(QDate(1974, 1, 1), QTime(0, 0, 0))
                            << "d/M/yyyy zzz" << "1/1/1974 000" << true;
    QTest::newRow("5no_NO") << "no_NO" << QDateTime(QDate(1974, 1, 1), QTime(0, 0, 0))
                            << "dd/MM/yyy z" << "01/01/74y 0" << false;
    QTest::newRow("8no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 2), QTime(0, 0, 13))
                            << "ddddd/MMMMM/yy ss" << "mandag2/desember12/74 13" << true;
    QTest::newRow("9no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(0, 0, 13))
                            << "'dddd'/MMMM/yy s" << "dddd/desember/74 13" << false;
    QTest::newRow("10no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(0, 4, 0))
                             << "d'dd'd/MMMM/yyy m'm'mm" << "1dd1/desember/74y 4m04" << false;
    QTest::newRow("11no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(0, 0, 3))
                             << "d'dd'd/MMM'M'/yysss" << "1dd1/des.M/74033" << false;
    QTest::newRow("12no_NO") << "no_NO" << QDateTime(QDate(1974, 12, 1), QTime(15, 0, 0))
                             << "d'd'dd/M/yyh" << "1d01/12/7415" << false;

    QTest::newRow("short-ss") // QTBUG-102199: trips over an assert in CET
        << "C" << QDateTime() // Single-digit seconds does not match ss format.
        << u"ddd, d MMM yyyy HH:mm:ss"_s << u"Sun, 29 Mar 2020 02:26:3"_s << true;

    QTest::newRow("short-ss-Z") // Same, but with a valid date-time:
        << "C" << QDateTime()
        << u"ddd, d MMM yyyy HH:mm:ss t"_s << u"Sun, 29 Mar 2020 02:26:3 Z"_s << false;

    QTest::newRow("s-Z") // Same, but with a format that accepts the single digit:
        << "C" << QDateTime(QDate(2020, 3, 29), QTime(2, 26, 3), QTimeZone::UTC)
        << u"ddd, d MMM yyyy HH:mm:s t"_s << u"Sun, 29 Mar 2020 02:26:3 Z"_s << false;

    QTest::newRow("RFC-1123")
        << "C" << QDateTime(QDate(2007, 11, 1), QTime(18, 8, 30))
        << "ddd, dd MMM yyyy hh:mm:ss 'GMT'" << "Thu, 01 Nov 2007 18:08:30 GMT" << false;

    QTest::newRow("longFormat")
        << "en_US" << QDateTime(QDate(2009, 1, 5), QTime(11, 48, 32))
        << "dddd, MMMM d, yyyy h:mm:ss AP " << "Monday, January 5, 2009 11:48:32 AM " << true;

    // Parsing am/pm indicators case-insensitively:
    QTest::newRow("am-cs_CZ")
        << "cs_CZ" << QDateTime(QDate(1945, 8, 6), QTime(8, 15, 44, 400))
        << "yyyy-MM-dd hh:mm:ss.z aP" << "1945-08-06 08:15:44.4 dOp." << true;
    QTest::newRow("pm-cs_CZ")
        << "cs_CZ" << QDateTime(QDate(1945, 8, 15), QTime(12, 0))
        << "yyyy-MM-dd hh:mm aP" << "1945-08-15 12:00 OdP." << true;

    const QDateTime dt(QDate(2017, 02, 25), QTime(17, 21, 25));
    // These formats correspond to the locale formats, with the timezone removed.
    // We hardcode them in case an update to the locale DB changes them.

    QTest::newRow("C:long") << "C" << dt << "dddd, d MMMM yyyy HH:mm:ss"
                            << "Saturday, 25 February 2017 17:21:25" << true;
    QTest::newRow("C:short")
        << "C" << dt << "d MMM yyyy HH:mm:ss" << "25 Feb 2017 17:21:25" << true;
    QTest::newRow("C:narrow")
        << "C" << dt << "d MMM yyyy HH:mm:ss" << "25 Feb 2017 17:21:25" << true;

    // Test the same again with unicode and emoji.
    QTest::newRow("C:long with emoji") << "C" << dt << u8"dddd, d💪MMMM yyyy HH:mm:ss"
        << u8"Saturday, 25💪February 2017 17:21:25" << true;
    QTest::newRow("C:short with emoji")
        << "C" << dt << u8"d MMM yyyy HH📞mm📞ss" << u8"25 Feb 2017 17📞21📞25" << true;
    QTest::newRow("C:narrow with emoji")
        << "C" << dt << u8"🇬🇧d MMM yyyy HH:mm:ss🇬🇧" << u8"🇬🇧25 Feb 2017 17:21:25🇬🇧" << true;

    QTest::newRow("fr:long") << "fr" << dt << "dddd d MMMM yyyy HH:mm:ss"
                             << "Samedi 25 février 2017 17:21:25" << true;
    QTest::newRow("fr:short")
        << "fr" << dt.addSecs(-25) << "dd/MM/yyyy HH:mm" << "25/02/2017 17:21" << true;

    // In Turkish, the word for Friday ("Cuma") is a prefix for the word for
    // Saturday ("Cumartesi")
    QTest::newRow("tr:long")
        << "tr" << dt << "d MMMM yyyy dddd HH:mm:ss" << "25 Şubat 2017 Cumartesi 17:21:25" << true;
    QTest::newRow("tr:long2") << "tr" << dt.addDays(-1) << "d MMMM yyyy dddd HH:mm:ss"
                              << "24 Şubat 2017 Cuma 17:21:25" << true;
    QTest::newRow("tr:mashed")
        << "tr" << dt << "d MMMMyyyy ddddHH:mm:ss" << "25 Şubat2017 Cumartesi17:21:25" << true;
    QTest::newRow("tr:mashed2") << "tr" << dt.addDays(-1) << "d MMMMyyyy ddddHH:mm:ss"
                                << "24 Şubat2017 Cuma17:21:25" << true;
    QTest::newRow("tr:short")
        << "tr" << dt.addSecs(-25) << "d.MM.yyyy HH:mm" << "25.02.2017 17:21" << true;

    QTest::newRow("ccp:short")
        << "ccp" << dt << "dd/M/yy h:mm AP"
        // "𑄸𑄻/𑄸/𑄷𑄽 𑄻:𑄸𑄷 PM"
        << QString::fromUcs4(U"\U00011138\U0001113b/\U00011138/\U00011137\U0001113d \U0001113b:"
                             U"\U00011138\U00011137 PM") << true;
    QTest::newRow("ccp:long")
        << "ccp" << dt << "dddd, d MMMM, yyyy h:mm:ss AP"
        // "𑄥𑄧𑄚𑄨𑄝𑄢𑄴, 𑄸𑄻 𑄜𑄬𑄛𑄴𑄝𑄳𑄢𑄪𑄠𑄢𑄨, 𑄸𑄶𑄷𑄽 𑄻:𑄸𑄷:𑄸𑄻 PM"
        << QString::fromUcs4(U"\U00011125\U00011127\U0001111a\U00011128\U0001111d\U00011122"
                             U"\U00011134, \U00011138\U0001113b \U0001111c\U0001112c\U0001111b"
                             U"\U00011134\U0001111d\U00011133\U00011122\U0001112a\U00011120"
                             U"\U00011122\U00011128, \U00011138\U00011136\U00011137\U0001113d "
                             U"\U0001113b:\U00011138\U00011137:\U00011138\U0001113b PM") << true;
}

void tst_QLocale::toDateTime()
{
    QFETCH(QString, localeName);
    QFETCH(QDateTime, result);
    QFETCH(QString, format);
    QFETCH(QString, string);
    QFETCH(bool, clean);

    QLocale l(localeName);
    QEXPECT_FAIL("ccp:short", "QTBUG-87111: Handling of code points outside BMP is broken", Abort);
    QEXPECT_FAIL("ccp:long", "QTBUG-87111: Handling of code points outside BMP is broken", Abort);
    QCOMPARE(l.toDateTime(string, format), result);
    if (clean) {
        QCOMPARE(l.toDateTime(string.toLower(), format), result);
        QCOMPARE(l.toDateTime(string.toUpper(), format), result);
    }

    if (l.dateTimeFormat(QLocale::LongFormat) == format)
        QCOMPARE(l.toDateTime(string, QLocale::LongFormat), result);
    if (l.dateTimeFormat(QLocale::ShortFormat) == format)
        QCOMPARE(l.toDateTime(string, QLocale::ShortFormat), result);
}

void tst_QLocale::roundtripDateTimeFormat_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QDateTime>("when");
    QTest::addColumn<QCalendar>("cal");
    QTest::addColumn<QLocale::FormatType>("format");
    QTest::addColumn<int>("baseYear");
    const QCalendar greg;

#if QT_CONFIG(timezone)
    qsizetype count = 0;
    const QTimeZone westOz("Australia/Perth");
    if (westOz.isValid()) {
        QTest::newRow("de_DE/LongFormat/2024-05-06T12:34/AWT") // QTBUG-130278
            << QLocale(QLocale::German, QLocale::Germany)
            << QDateTime(QDate(2024, 5, 6, greg), QTime(12, 34), westOz)
            << greg << QLocale::LongFormat << 2000;
        ++count;
    }

    const QTimeZone nepal("Asia/Katmandu");
    if (nepal.isValid()) {
        // Triggers the region-format code-path:
        QTest::newRow("en_US/LongFormat/2025-02-06T20:20/Katmandu")
            << QLocale(QLocale::English, QLocale::UnitedStates)
            << QDateTime(QDate(2025, 2, 6, greg), QTime(20, 20), nepal)
            << greg << QLocale::LongFormat << 2000;
        ++count;
    }

    if (!count)
        QSKIP("Missing zones for both test-cases");
#else
    QSKIP("The only test-case depends on feature timezone");
#endif
}

void tst_QLocale::roundtripDateTimeFormat()
{
    QFETCH(const QLocale, locale);
    QFETCH(const QDateTime, when);
    QFETCH(const QCalendar, cal);
    QFETCH(const QLocale::FormatType, format);
    QFETCH(const int, baseYear);

    const QString text = locale.toString(when, format, cal);
    auto report = qScopeGuard([=]() {
        qDebug() << "Went via:" << text;
        qDebug() << "Used format:" << locale.dateTimeFormat(format);
        QDateTime parsed = locale.toDateTime(text, format, cal, baseYear);
        if (parsed.isValid()) {
            switch (parsed.timeSpec()) {
#if QT_CONFIG(timezone)
            case Qt::TimeZone:
                qDebug() << "Used zone:" << parsed.timeZone().id();
                break;
#endif
            case Qt::OffsetFromUTC:
                qDebug() << "Used fixed UTC offset:" << parsed.offsetFromUtc();
                break;
            case Qt::LocalTime:
                qDebug("Used local time");
                break;
            case Qt::UTC:
                qDebug("Used plain UTC");
                break;
            }
        }
    });
    QCOMPARE(locale.toDateTime(text, format, cal, baseYear), when);
    report.dismiss();
}

void tst_QLocale::toDate_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QDate>("result");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("string");
    // No non-format letters in format string:
    QTest::addColumn<bool>("clean");

    const auto C = QLocale::c();
    QTest::newRow("C-d/M/yyyy")
        << C << QDate(1974, 12, 1) << u"d/M/yyyy"_s << u"1/12/1974"_s << true;
    QTest::newRow("C-d/M/yyyyy")
        << C << QDate(1974, 12, 1) << u"d/M/yyyyy"_s << u"1/12/1974y"_s << false;
    QTest::newRow("C-dd/MM/yyy")
        << C << QDate(1974, 1, 1) << u"dd/MM/yyy"_s << u"01/01/74y"_s << false;
    QTest::newRow("C-ddddd/MMMMM/yy")
        << C << QDate(1974, 12, 2) << u"ddddd/MMMMM/yy"_s << u"Monday2/December12/74"_s
        << true;
    QTest::newRow("C-'dddd'/MMMM/yy")
        << C << QDate(1974, 12, 1) << u"'dddd'/MMMM/yy"_s << u"dddd/December/74"_s << false;
    QTest::newRow("C-d'dd'd/MMMM/yyy")
        << C << QDate(1974, 12, 1) << u"d'dd'd/MMMM/yyy"_s << u"1dd1/December/74y"_s << false;
    QTest::newRow("C-d'dd'd/MMM'M'/yy")
        << C << QDate(1974, 12, 1) << u"d'dd'd/MMM'M'/yy"_s << u"1dd1/DecM/74"_s << false;
    QTest::newRow("C-d'd'dd/M/yy")
        << C << QDate(1974, 12, 1) << u"d'd'dd/M/yy"_s << u"1d01/12/74"_s << false;
    // Unpadded value for fixed-width field is wrong:
    QTest::newRow("bad-day-C")
        << C << QDate() << u"dd-MMM-yy"_s << u"4-Jun-11"_s << true;
    QTest::newRow("bad-month-C")
        << C << QDate() << u"d-MM-yy"_s << u"4-6-11"_s << true;
    QTest::newRow("bad-year-C")
        << C << QDate() << u"d-MMM-yyyy"_s << u"4-Jun-11"_s << true;
    QTest::newRow("ok-C")
        << C << QDate(1911, 6, 4) << u"d-MMM-yy"_s << u"4-Jun-11"_s << true;

    // Locale-specific details frozen to avoid CLDR update breakage.
    // However, updating to match CLDR from time to time would be constructive.
    const QLocale norsk{QLocale::NorwegianBokmal, QLocale::Norway};
    QTest::newRow("no_NO-d/M/yyyy")
        << norsk << QDate(1974, 12, 1) << u"d/M/yyyy"_s << u"1/12/1974"_s << true;
    QTest::newRow("no_NO-d/M/yyyyy")
        << norsk << QDate(1974, 12, 1) << u"d/M/yyyyy"_s << u"1/12/1974y"_s << false;
    QTest::newRow("no_NO-dd/MM/yyy")
        << norsk << QDate(1974, 1, 1) << u"dd/MM/yyy"_s << u"01/01/74y"_s << false;
    QTest::newRow("no_NO-ddddd/MMMMM/yy")
        << norsk << QDate(1974, 12, 2) << u"ddddd/MMMMM/yy"_s << u"mandag2/desember12/74"_s
        << true;
    QTest::newRow("no_NO-'dddd'/MMMM/yy")
        << norsk << QDate(1974, 12, 1) << u"'dddd'/MMMM/yy"_s << u"dddd/desember/74"_s
        << false;
    QTest::newRow("no_NO-d'dd'd/MMMM/yyy")
        << norsk << QDate(1974, 12, 1) << u"d'dd'd/MMMM/yyy"_s << u"1dd1/desember/74y"_s
        << false;
    QTest::newRow("no_NO-d'dd'd/MMM'M'/yy")
        << norsk << QDate(1974, 12, 1) << u"d'dd'd/MMM'M'/yy"_s << u"1dd1/des.M/74"_s
        << false;
    QTest::newRow("no_NO-d'd'dd/M/yy")
        << norsk << QDate(1974, 12, 1) << u"d'd'dd/M/yy"_s << u"1d01/12/74"_s << false;

    QTest::newRow("RFC-1123")
        << C << QDate(2007, 11, 1) << u"ddd, dd MMM yyyy 'GMT'"_s << u"Thu, 01 Nov 2007 GMT"_s
        << false;

    const QLocale usa{QLocale::English, QLocale::UnitedStates};
    QTest::newRow("longFormat")
        << usa << QDate(2009, 1, 5) << u"dddd, MMMM d, yyyy"_s
        << u"Monday, January 5, 2009"_s << true;
    QTest::newRow("shortFormat") // Use of two-digit year considered harmful.
        << usa << QDate(1909, 1, 5) << u"M/d/yy"_s << u"1/5/09"_s << true;

    const QDate date(2017, 02, 25);
    QTest::newRow("C:long")
        << C << date << "dddd, d MMMM yyyy" << u"Saturday, 25 February 2017"_s << true;
    QTest::newRow("C:short")
        << C << date << u"d MMM yyyy"_s << u"25 Feb 2017"_s << true;
    QTest::newRow("C:narrow")
        << C << date << u"d MMM yyyy"_s << u"25 Feb 2017"_s << true;

    // Test the same again with unicode and emoji.
    QTest::newRow("C:long with emoji")
        << C << date << u8"dddd, d💪MMMM yyyy" << u8"Saturday, 25💪February 2017" << true;
    QTest::newRow("C:short with emoji")
        << C << date << u8"d📞MMM📞yyyy" << u8"25📞Feb📞2017" << true;
    QTest::newRow("C:narrow with emoji")
        << C << date << u8"🇬🇧d MMM yyyy🇬🇧"
        << u8"🇬🇧25 Feb 2017🇬🇧" << true;

    const QLocale fr{QLocale::French};
    QTest::newRow("fr:long")
        << fr << date << "dddd d MMMM yyyy" << u"Samedi 25 février 2017"_s << true;
    QTest::newRow("fr:short")
        << fr << date << u"dd/MM/yyyy"_s << u"25/02/2017"_s << true;

    // In Turkish, the word for Friday ("Cuma") is a prefix for the word for
    // Saturday ("Cumartesi")
    const QLocale turk(QLocale::Turkish);
    QTest::newRow("tr:long-Cumartesi")
        << turk << date << u"d MMMM yyyy dddd"_s << u"25 Şubat 2017 Cumartesi"_s << true;
    QTest::newRow("tr:long-Cuma")
        << turk << date.addDays(-1) << "d MMMM yyyy dddd" << u"24 Şubat 2017 Cuma"_s << true;
    QTest::newRow("tr:mashed-Cumartesi")
        << turk << date << u"d MMMMyyyydddd"_s << u"25 Şubat2017Cumartesi"_s << true;
    QTest::newRow("tr:mashed-Cuma")
        << turk << date.addDays(-1) << "ddddd MMMMyyyy" << u"Cuma24 Şubat2017"_s << true;
    QTest::newRow("tr:short")
        << turk << date << u"d.MM.yyyy"_s << u"25.02.2017"_s << true;

    const QLocale chakma{QLocale::Chakma};
    QTest::newRow("ccp:short")
        << chakma << date << "dd/M/yy"
        // "𑄸𑄻/𑄸/𑄷𑄽"
        << QString::fromUcs4(U"\U00011138\U0001113b/\U00011138/\U00011137\U0001113d") << true;
    QTest::newRow("ccp:long")
        << chakma << date << "dddd, d MMMM, yyyy"
        // "𑄥𑄧𑄚𑄨𑄝𑄢𑄴, 𑄸𑄻 𑄜𑄬𑄛𑄴𑄝𑄳𑄢𑄪𑄠𑄢𑄨, 𑄸𑄶𑄷𑄽"
        << QString::fromUcs4(U"\U00011125\U00011127\U0001111a\U00011128\U0001111d\U00011122"
                             U"\U00011134, \U00011138\U0001113b \U0001111c\U0001112c\U0001111b"
                             U"\U00011134\U0001111d\U00011133\U00011122\U0001112a\U00011120"
                             U"\U00011122\U00011128, \U00011138\U00011136\U00011137\U0001113d")
        << true;
}

void tst_QLocale::toDate()
{
    QFETCH(const QLocale, locale);
    QFETCH(const QDate, result);
    QFETCH(const QString, format);
    QFETCH(const QString, string);
    QFETCH(const bool, clean);

    QEXPECT_FAIL("ccp:short", "QTBUG-87111: Handling of code points outside BMP is broken", Abort);
    QEXPECT_FAIL("ccp:long", "QTBUG-87111: Handling of code points outside BMP is broken", Abort);
    QCOMPARE(locale.toDate(string, format), result);
    if (clean) {
        QCOMPARE(locale.toDate(string.toLower(), format), result);
        QCOMPARE(locale.toDate(string.toUpper(), format), result);
    }

    if (locale.dateFormat(QLocale::LongFormat) == format)
        QCOMPARE(locale.toDate(string, QLocale::LongFormat), result);
    if (locale.dateFormat(QLocale::ShortFormat) == format)
        QCOMPARE(locale.toDate(string, QLocale::ShortFormat), result);
}

void tst_QLocale::toTime_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QTime>("result");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("string");
    // No non-format letters in format string:
    QTest::addColumn<bool>("clean");

    const auto C = QLocale::c();
    QTest::newRow("C-hh:h:mm")
        << C << QTime(5, 14) << u"hh:h:mm"_s << u"05:5:14"_s << true;
    QTest::newRow("C-h")
        << C << QTime(15, 0) << u"h"_s << u"15"_s << true;
    QTest::newRow("C-zzz")
        << C << QTime(0, 0, 0, 1) << u"zzz"_s << u"001"_s << true;
    QTest::newRow("C-z/001")
        << C << QTime(0, 0, 0, 1) << u"z"_s << u"001"_s << true;
    QTest::newRow("C-z/1")
        << C << QTime(0, 0, 0, 100) << u"z"_s << u"1"_s << true;
    QTest::newRow("C-ss")
        << C << QTime(0, 0, 13) << u"ss"_s << u"13"_s << true;
    QTest::newRow("C-s")
        << C << QTime(0, 0, 13) << u"s"_s << u"13"_s << true;
    QTest::newRow("C-m'm'mm")
        << C << QTime(0, 4) << u"m'm'mm"_s << u"4m04"_s << false;
    QTest::newRow("C-hhmmsss")
        << C << QTime(0, 0, 3) << u"hhmmsss"_s << u"0000033"_s << true;
    // Unpadded value for fixed-width field is wrong:
    QTest::newRow("bad-hour-C")
        << C << QTime() << u"hh:m"_s << u"1:2"_s << true;
    QTest::newRow("bad-min-C")
        << C << QTime() << u"h:mm"_s << u"1:2"_s << true;
    QTest::newRow("bad-sec-C")
        << C << QTime() << u"d-MMM-yy h:m:ss"_s << u"4-Jun-11 1:2:3"_s << true;
    QTest::newRow("bad-milli-C")
        << C << QTime() << u"h:m:s.zzz"_s << u"1:2:3.4"_s << true;
    QTest::newRow("ok-C")
        << C << QTime(1, 2, 3, 400) << u"h:m:s.z"_s << u"1:2:3.4"_s << true;

    // Locale-specific details frozen to avoid CLDR update breakage.
    // However, updating to match CLDR from time to time would be constructive.
    const QLocale norsk{QLocale::NorwegianBokmal, QLocale::Norway};
    QTest::newRow("nb_NO-hh:h:mm")
        << norsk << QTime(5, 14) << u"hh:h:mm"_s << u"05:5:14"_s << true;
    QTest::newRow("nb_NO-h")
        <<norsk << QTime(15, 0) << u"h"_s << u"15"_s << true;
    QTest::newRow("nb_NO-zzz")
        <<norsk << QTime(0, 0) << u"zzz"_s << u"000"_s << true;
    QTest::newRow("nb_NO-z")
        <<norsk << QTime(0, 0) << u"z"_s << u"0"_s << true;
    QTest::newRow("nb_NO-ss")
        <<norsk << QTime(0, 0, 13) << u"ss"_s << u"13"_s << true;
    QTest::newRow("nb_NO-s")
        <<norsk << QTime(0, 0, 13) << u"s"_s << u"13"_s << true;
    QTest::newRow("nb_NO-m'm'mm")
        <<norsk << QTime(0, 4) << u"m'm'mm"_s << u"4m04"_s << false;
    QTest::newRow("nb_NO-hhmmsss")
        <<norsk << QTime(0, 0, 3) << u"hhmmsss"_s << u"0000033"_s << true;

    QTest::newRow("short-ss") // Single-digit seconds does not match ss format.
        << C << QTime() << u"HH:mm:ss"_s << u"02:26:3"_s << true;
    QTest::newRow("RFC-1123")
        << C << QTime(18, 8, 30) << u"hh:mm:ss 'GMT'"_s << u"18:08:30 GMT"_s << false;

    const QLocale usa{QLocale::English, QLocale::UnitedStates};
    QTest::newRow("longFormat-AM")
        << usa << QTime(4, 43, 32) << u"h:mm:ss AP "_s << u"4:43:32 AM "_s << true;
    QTest::newRow("shortFormat-AM")
        << usa << QTime(4, 43) << u"h:mm AP "_s << u"4:43 AM "_s << true;
    QTest::newRow("longFormat-PM")
        << usa << QTime(16, 43, 32) << u"h:mm:ss AP "_s << u"4:43:32 PM "_s << true;
    QTest::newRow("shortFormat-PM")
        << usa << QTime(16, 43) << u"h:mm AP "_s << u"4:43 PM "_s << true;
    // Some locales use a narrow non-breaking space as separator, but
    // the user can't see the difference from a space (QTBUG-114909):
    QTest::newRow("shortFormat-AM-mixspace")
        << usa << QTime(4, 43) << u"h:mm\u202F" "AP "_s << u"4:43 AM "_s << true;

    // Parsing am/pm indicators case-insensitively:
    const QLocale czech{QLocale::Czech, QLocale::Czechia};
    QTest::newRow("am-cs_CZ")
        << czech << QTime(8, 15, 44, 400) << u"hh:mm:ss.z aP"_s << u"08:15:44.4 dOp."_s
        << true;
    QTest::newRow("pm-cs_CZ")
        << czech << QTime(12, 0) << u"hh:mm aP"_s << u"12:00 OdP."_s << true;

    const QTime time(17, 21, 25);
    QTest::newRow("C:long")
        << C << time << "HH:mm:ss" << u"17:21:25"_s << true;
    QTest::newRow("C:short")
        << C << time << u"HH:mm:ss"_s << u"17:21:25"_s << true;
    QTest::newRow("C:narrow")
        << C << time << u"HH:mm:ss"_s << u"17:21:25"_s << true;

    // Test the same again with unicode and emoji.
    QTest::newRow("C:long with emoji")
        << C << time << u8"HH💪mm💪ss" << u8"17💪21💪25" << true;
    QTest::newRow("C:short with emoji")
        << C << time << u8"HH📞mm📞ss" << u8"17📞21📞25" << true;
    QTest::newRow("C:narrow with emoji")
        << C << time << u8"🇬🇧HH:mm:ss🇬🇧"
        << u8"🇬🇧17:21:25🇬🇧" << true;

    const QLocale fr{QLocale::French};
    QTest::newRow("fr:long")
        << fr << time << "HH:mm:ss" << u"17:21:25"_s << true;
    QTest::newRow("fr:short")
        << fr << time.addSecs(-25) << u"HH:mm"_s << u"17:21"_s << true;
    QTest::newRow("tr:short")
        << QLocale(QLocale::Turkish) << time.addSecs(-25) << u"HH:mm"_s << u"17:21"_s << true;

    const QLocale chakma{QLocale::Chakma};
    QTest::newRow("ccp:short")
        << chakma << time << "h:mm AP"
        // "𑄸𑄻/𑄸/𑄷𑄽 𑄻:𑄸𑄷 PM"
        << QString::fromUcs4(U"\U0001113b:\U00011138\U00011137 PM") << true;
    QTest::newRow("ccp:long")
        << chakma << time << "h:mm:ss AP"
        // "𑄻:𑄸𑄷:𑄸𑄻 PM"
        << QString::fromUcs4(U"\U0001113b:\U00011138\U00011137:\U00011138\U0001113b PM") << true;
}

void tst_QLocale::toTime()
{
    QFETCH(const QLocale, locale);
    QFETCH(const QTime, result);
    QFETCH(const QString, format);
    QFETCH(const QString, string);
    QFETCH(const bool, clean);

    QEXPECT_FAIL("ccp:short", "QTBUG-87111: Handling of code points outside BMP is broken", Abort);
    QEXPECT_FAIL("ccp:long", "QTBUG-87111: Handling of code points outside BMP is broken", Abort);
    QCOMPARE(locale.toTime(string, format), result);
    if (clean) {
        QCOMPARE(locale.toTime(string.toLower(), format), result);
        QCOMPARE(locale.toTime(string.toUpper(), format), result);
    }

    if (locale.timeFormat(QLocale::LongFormat) == format)
        QCOMPARE(locale.toTime(string, QLocale::LongFormat), result);
    if (locale.timeFormat(QLocale::ShortFormat) == format)
        QCOMPARE(locale.toTime(string, QLocale::ShortFormat), result);
}

void tst_QLocale::doubleRoundTrip_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QString>("numberText");
    QTest::addColumn<char>("numberFormat");

    // Signs and exponent separator aren't single characters:
    QTest::newRow("sv_SE 4e-06 g") // Swedish, Sweden
        << u"sv_SE"_s << u"4\u00d7" "10^\u2212" "06"_s << 'g';
    QTest::newRow("se_NO 4e-06 g") // Northern Sami, Norway
        << u"se_NO"_s << u"4\u00b7" "10^\u2212" "06"_s << 'g';
    QTest::newRow("ar_EG 4e-06 g") // Arabic, Egypt
        << u"ar_EG"_s << u"\u0664\u0623\u0633\u061c-\u0660\u0666"_s << 'g';
    QTest::newRow("fa_IR 4e-06 g") // Farsi, Iran
        << u"fa_IR"_s << u"\u06f4\u00d7\u06f1\u06f0^\u200e\u2212\u06f0\u06f6"_s << 'g';
}

void tst_QLocale::doubleRoundTrip()
{
    QFETCH(QString, localeName);
    QFETCH(QString, numberText);
    QFETCH(char, numberFormat);

    QLocale locale(localeName);
    bool ok;

    double number = locale.toDouble(numberText, &ok);
    QVERIFY(ok);
    QCOMPARE(locale.toString(number, numberFormat), numberText);
}

void tst_QLocale::integerRoundTrip_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QString>("numberText");

    // Two-character signs:
    // Arabic, Egypt
    QTest::newRow("ar_EG -406") << u"ar_EG"_s << u"\u061c-\u0664\u0660\u0666"_s;
    // Farsi, Iran
    QTest::newRow("fa_IR -406") << u"fa_IR"_s << u"\u200e\u2212\u06f4\u06f0\u06f6"_s;
}

void tst_QLocale::integerRoundTrip()
{
    QFETCH(QString, localeName);
    QFETCH(QString, numberText);

    QLocale locale(localeName);
    bool ok;

    qlonglong number = locale.toLongLong(numberText, &ok);
    QVERIFY(ok);
    QCOMPARE(locale.toString(number), numberText);
}

#ifdef Q_OS_DARWIN

// Format number string according to system locale settings.
// Expected in format is US "1,234.56".
QString systemLocaleFormatNumber(QString &&numberString)
{
    QLocale locale = QLocale::system();
    QString numberStringMunged =
        numberString.replace(QChar(','), QChar('G')).replace(QChar('.'), QChar('D'));
    if (locale.numberOptions() & QLocale::OmitGroupSeparator)
        numberStringMunged.remove(QLatin1Char('G'));
    else
        numberStringMunged.replace(QChar('G'), locale.groupSeparator());
    return numberStringMunged.replace(QChar('D'), locale.decimalPoint());
}

void tst_QLocale::macDefaultLocale()
{
    QLocale locale = QLocale::system();

    if (locale.name() != QLatin1String("en_US"))
        QSKIP("This test only tests for en_US");

    QTime invalidTime;
    QDate invalidDate;
    QCOMPARE(locale.toString(invalidTime, QLocale::ShortFormat), QString());
    QCOMPARE(locale.toString(invalidDate, QLocale::ShortFormat), QString());
    QCOMPARE(locale.toString(invalidTime, QLocale::NarrowFormat), QString());
    QCOMPARE(locale.toString(invalidDate, QLocale::NarrowFormat), QString());
    QCOMPARE(locale.toString(invalidTime, QLocale::LongFormat), QString());
    QCOMPARE(locale.toString(invalidDate, QLocale::LongFormat), QString());

    // On OS X the decimal point and group separator are configurable
    // independently of the locale. Verify that they have one of the
    // allowed values and are not the same.
    QVERIFY(locale.decimalPoint() == QStringView(u".")
            || locale.decimalPoint() == QStringView(u","));
    QVERIFY(locale.groupSeparator() == QStringView(u",")
         || locale.groupSeparator() == QStringView(u".")
         || locale.groupSeparator() == QStringView(u"\xA0") // no-breaking space
         || locale.groupSeparator() == QStringView(u"'")
         || locale.groupSeparator().isEmpty());
    QCOMPARE_NE(locale.decimalPoint(), locale.groupSeparator());

    // make sure we are using the system to parse them
    QCOMPARE(locale.toString(1234.56), systemLocaleFormatNumber(QString("1,234.56")));

    QTime testTime = QTime(1, 2, 3);
    QTime utcTime = QDateTime(QDate::currentDate(), testTime).toUTC().time();
    int diff = testTime.hour() - utcTime.hour();

    // Check if local time and utc time are on opposite sides of the 24-hour wrap-around.
    if (diff < -12)
        diff += 24;
    if (diff > 12)
        diff -= 24;

    const QString timeString = locale.toString(testTime, QLocale::LongFormat);
    QVERIFY(timeString.contains(QString("1:02:03")));

    // To run this test make sure "Curreny" is US Dollar in System Preferences->Language & Region->Advanced.
    if (locale.currencySymbol() == QString("$")) {
        QCOMPARE(locale.toCurrencyString(qulonglong(1234)),
                 systemLocaleFormatNumber(QString("$1,234.00")));
        QCOMPARE(locale.toCurrencyString(double(1234.56)),
                 systemLocaleFormatNumber(QString("$1,234.56")));
    }

    // Depending on the configured time zone, the time string might not
    // contain a GMT specifier. (Sometimes it just names the zone, like "CEST")
    QLatin1String gmt("GMT");
    if (timeString.contains(gmt) && diff) {
        QLatin1Char sign(diff < 0 ? '-' : '+');
        QString number(QString::number(qAbs(diff)));
        const QString expect = gmt + sign + number;

        if (diff < 10) {
            const QString zeroed = gmt + sign + QLatin1Char('0') + number;

            QVERIFY2(timeString.contains(expect) || timeString.contains(zeroed),
                     qPrintable(QString("timeString `%1', expected GMT specifier `%2' or `%3'")
                                .arg(timeString).arg(expect).arg(zeroed)));
        } else {
            QVERIFY2(timeString.contains(expect),
                     qPrintable(QString("timeString `%1', expected GMT specifier `%2'")
                                .arg(timeString).arg(expect)));
        }
    }
    QCOMPARE(locale.dayName(1), QString("Monday"));
    QCOMPARE(locale.dayName(7), QString("Sunday"));
    QCOMPARE(locale.monthName(1), QString("January"));
    QCOMPARE(locale.monthName(12), QString("December"));
    QCOMPARE(locale.quoteString("string"),
             QString::fromUtf8("\xe2\x80\x9c" "string" "\xe2\x80\x9d"));
    QCOMPARE(locale.quoteString("string", QLocale::AlternateQuotation),
             QString::fromUtf8("\xe2\x80\x98" "string" "\xe2\x80\x99"));

    QList<Qt::DayOfWeek> days;
    days << Qt::Monday << Qt::Tuesday << Qt::Wednesday << Qt::Thursday << Qt::Friday;
    QCOMPARE(locale.weekdays(), days);

}
#endif // Q_OS_DARWIN

#if defined(Q_OS_WIN)
#include <qt_windows.h>

static QString getWinLocaleInfo(LCTYPE type)
{
    LCID id = GetThreadLocale();
    int cnt = GetLocaleInfo(id, type, 0, 0) * 2;

    if (cnt == 0) {
        qWarning().nospace() << "QLocale: empty windows locale info (" <<  type << ')';
        return QString();
    }
    cnt /= sizeof(wchar_t);
    QScopedArrayPointer<wchar_t> buf(new wchar_t[cnt]);
    cnt = GetLocaleInfo(id, type, buf.data(), cnt);

    if (cnt == 0) {
        qWarning().nospace() << "QLocale: empty windows locale info (" << type << ')';
        return QString();
    }
    return QString::fromWCharArray(buf.data());
}

static void setWinLocaleInfo(LCTYPE type, QStringView value)
{
    LCID id = GetThreadLocale();
    SetLocaleInfo(id, type, reinterpret_cast<const wchar_t*>(value.utf16()));
}

#ifndef LOCALE_SSHORTTIME
#  define LOCALE_SSHORTTIME 0x00000079
#endif

class RestoreLocaleHelper
{
public:
    RestoreLocaleHelper()
    {
        m_decimal = getWinLocaleInfo(LOCALE_SDECIMAL);
        m_thousand = getWinLocaleInfo(LOCALE_STHOUSAND);
        m_sdate = getWinLocaleInfo(LOCALE_SSHORTDATE);
        m_ldate = getWinLocaleInfo(LOCALE_SLONGDATE);
        m_stime = getWinLocaleInfo(LOCALE_SSHORTTIME);
        m_ltime = getWinLocaleInfo(LOCALE_STIMEFORMAT);
        m_digits = getWinLocaleInfo(LOCALE_SNATIVEDIGITS);
        m_subst = getWinLocaleInfo(LOCALE_IDIGITSUBSTITUTION);
    }

    ~RestoreLocaleHelper()
    {
        // restore these, or the user will get a surprise
        setWinLocaleInfo(LOCALE_SDECIMAL, m_decimal);
        setWinLocaleInfo(LOCALE_STHOUSAND, m_thousand);
        setWinLocaleInfo(LOCALE_SSHORTDATE, m_sdate);
        setWinLocaleInfo(LOCALE_SLONGDATE, m_ldate);
        setWinLocaleInfo(LOCALE_SSHORTTIME, m_stime);
        setWinLocaleInfo(LOCALE_STIMEFORMAT, m_ltime);
        setWinLocaleInfo(LOCALE_SNATIVEDIGITS, m_digits);
        setWinLocaleInfo(LOCALE_IDIGITSUBSTITUTION, m_subst);

        QTestLocaleChange::resetSystemLocale();
    }

    QString m_decimal, m_thousand, m_sdate, m_ldate, m_stime, m_ltime, m_digits, m_subst;
};

void tst_QLocale::windowsDefaultLocale()
{
    RestoreLocaleHelper systemLocale;
    // set weird system defaults and make sure we're using them
    setWinLocaleInfo(LOCALE_SDECIMAL, u"@");
    setWinLocaleInfo(LOCALE_STHOUSAND, u"?");
    const QString shortDateFormat = QStringLiteral("d*M*yyyy");
    setWinLocaleInfo(LOCALE_SSHORTDATE, shortDateFormat);
    const QString longDateFormat = QStringLiteral("d@M@yyyy");
    setWinLocaleInfo(LOCALE_SLONGDATE, longDateFormat);
    const QString shortTimeFormat = QStringLiteral("h^m^s");
    setWinLocaleInfo(LOCALE_SSHORTTIME, shortTimeFormat);
    const QString longTimeFormat = QStringLiteral("HH%mm%ss");
    setWinLocaleInfo(LOCALE_STIMEFORMAT, longTimeFormat);
    // Suzhou numerals (QTBUG-85409):
    const QStringView digits = u"\u3007\u3021\u3022\u3023\u3024\u3025\u3026\u3027\u3028\u3029";
    setWinLocaleInfo(LOCALE_SNATIVEDIGITS, digits);
    setWinLocaleInfo(LOCALE_IDIGITSUBSTITUTION, u"2");
    // NB: when adding to the system things being set, be sure to update RestoreLocaleHelper, too.

    QLocale locale = QTestLocaleChange::resetSystemLocale();

    // Make sure we are seeing the system's format strings
    QCOMPARE(locale.zeroDigit(), QStringView(u"\u3007"));
    QCOMPARE(locale.decimalPoint(), QStringView(u"@"));
    QCOMPARE(locale.groupSeparator(), QStringView(u"?"));
    QCOMPARE(locale.dateFormat(QLocale::ShortFormat), shortDateFormat);
    QCOMPARE(locale.dateFormat(QLocale::LongFormat), longDateFormat);
    QCOMPARE(locale.timeFormat(QLocale::ShortFormat), shortTimeFormat);
    QCOMPARE(locale.dateTimeFormat(QLocale::ShortFormat),
             shortDateFormat + QLatin1Char(' ') + shortTimeFormat);
    const QString expectedLongDateTimeFormat
        = longDateFormat + QLatin1Char(' ') + longTimeFormat;
    QCOMPARE(locale.dateTimeFormat(QLocale::LongFormat), expectedLongDateTimeFormat);

    // make sure we are using the system to parse them
    QCOMPARE(locale.toString(1234.56), QStringView(u"\u3021?\u3022\u3023\u3024@\u3025\u3026"));
    QCOMPARE(locale.toString(QDate(1974, 12, 1), QLocale::ShortFormat),
             QStringView(u"\u3021*\u3021\u3022*\u3021\u3029\u3027\u3024"));
    QCOMPARE(locale.toString(QDate(1974, 12, 1), QLocale::NarrowFormat),
             locale.toString(QDate(1974, 12, 1), QLocale::ShortFormat));
    QCOMPARE(locale.toString(QDate(1974, 12, 1), QLocale::LongFormat),
             QStringView(u"\u3021@\u3021\u3022@\u3021\u3029\u3027\u3024"));
    const QString expectedFormattedShortTime = QStringView(u"\u3021^\u3022^\u3023").toString();
    QCOMPARE(locale.toString(QTime(1,2,3), QLocale::ShortFormat), expectedFormattedShortTime);
    QCOMPARE(locale.toString(QTime(1,2,3), QLocale::NarrowFormat),
             locale.toString(QTime(1,2,3), QLocale::ShortFormat));
    const QString expectedFormattedLongTime
        = QStringView(u"\u3007\u3021%\u3007\u3022%\u3007\u3023").toString();
    QCOMPARE(locale.toString(QTime(1,2,3), QLocale::LongFormat), expectedFormattedLongTime);
    QCOMPARE(locale.toString(QDateTime(QDate(1974, 12, 1), QTime(1,2,3)), QLocale::ShortFormat),
             QStringView(u"\u3021*\u3021\u3022*\u3021\u3029\u3027\u3024 ").toString()
             + expectedFormattedShortTime);
    QCOMPARE(locale.toString(QDateTime(QDate(1974, 12, 1), QTime(1,2,3)), QLocale::NarrowFormat),
             locale.toString(QDateTime(QDate(1974, 12, 1), QTime(1,2,3)), QLocale::ShortFormat));
    QCOMPARE(locale.toString(QDateTime(QDate(1974, 12, 1), QTime(1,2,3)), QLocale::LongFormat),
             QStringView(u"\u3021@\u3021\u3022@\u3021\u3029\u3027\u3024 ").toString()
             + expectedFormattedLongTime);
}
#endif // Q_OS_WIN

void tst_QLocale::numberOptions()
{
    bool ok;

    QLocale locale(QLocale::C);
    QCOMPARE(locale.numberOptions(), QLocale::OmitGroupSeparator);
    QCOMPARE(locale.toInt(QString("12345"), &ok), 12345);
    QVERIFY(ok);
    QCOMPARE(locale.toInt(QString("12345"), &ok), 12345);
    QVERIFY(ok);
    QCOMPARE(locale.toString(12345), QString("12345"));

    locale.setNumberOptions({ });
    QCOMPARE(locale.numberOptions(), 0);
    QCOMPARE(locale.toInt(QString("12,345"), &ok), 12345);
    QVERIFY(ok);
    QCOMPARE(locale.toInt(QString("12345"), &ok), 12345);
    QVERIFY(ok);
    QCOMPARE(locale.toString(12345), QString("12,345"));

    locale.setNumberOptions(QLocale::RejectGroupSeparator);
    QCOMPARE(locale.numberOptions(), QLocale::RejectGroupSeparator);
    locale.toInt(QString("12,345"), &ok);
    QVERIFY(!ok);
    QCOMPARE(locale.toInt(QString("12345"), &ok), 12345);
    QVERIFY(ok);
    QCOMPARE(locale.toString(12345), QString("12,345"));

    QLocale locale2 = locale;
    QCOMPARE(locale2.numberOptions(), QLocale::RejectGroupSeparator);

    QCOMPARE(locale.toString(12.4, 'e', 2), QString("1.24e+01"));
    locale.setNumberOptions(QLocale::OmitLeadingZeroInExponent);
    QCOMPARE(locale.numberOptions(), QLocale::OmitLeadingZeroInExponent);
    QCOMPARE(locale.toString(12.4, 'e', 2), QString("1.24e+1"));

    locale.toDouble(QString("1.24e+01"), &ok);
    QVERIFY(ok);
    locale.setNumberOptions(QLocale::RejectLeadingZeroInExponent);
    QCOMPARE(locale.numberOptions(), QLocale::RejectLeadingZeroInExponent);
    locale.toDouble(QString("1.24e+1"), &ok);
    QVERIFY(ok);
    locale.toDouble(QString("1.24e+01"), &ok);
    QVERIFY(!ok);

    QCOMPARE(locale.toString(12.4, 'g', 5), QString("12.4"));
    locale.setNumberOptions(QLocale::IncludeTrailingZeroesAfterDot);
    QCOMPARE(locale.numberOptions(), QLocale::IncludeTrailingZeroesAfterDot);
    QCOMPARE(locale.toString(12.4, 'g', 5), QString("12.400"));

    locale.toDouble(QString("1.24e+01"), &ok);
    QVERIFY(ok);
    locale.toDouble(QString("1.2400e+01"), &ok);
    QVERIFY(ok);
    locale.toDouble(QString("12.4"), &ok);
    QVERIFY(ok);
    locale.toDouble(QString("12.400"), &ok);
    QVERIFY(ok);
    locale.setNumberOptions(QLocale::RejectTrailingZeroesAfterDot);
    QCOMPARE(locale.numberOptions(), QLocale::RejectTrailingZeroesAfterDot);
    locale.toDouble(QString("1.24e+01"), &ok);
    QVERIFY(ok);
    locale.toDouble(QString("1.2400e+01"), &ok);
    QVERIFY(!ok);
    locale.toDouble(QString("12.4"), &ok);
    QVERIFY(ok);
    locale.toDouble(QString("12.400"), &ok);
    QVERIFY(!ok);
    QT_TEST_EQUALITY_OPS(locale, locale2, false);
}

void tst_QLocale::negativeNumbers()
{
    QLocale locale(QLocale::C);

    bool ok;
    int i;

    i = locale.toInt(QLatin1String("-100"), &ok);
    QVERIFY(ok);
    QCOMPARE(i, -100);

    i = locale.toInt(QLatin1String("-1,000"), &ok);
    QVERIFY(ok);
    QCOMPARE(i, -1000);

    i = locale.toInt(QLatin1String("-1000"), &ok);
    QVERIFY(ok);
    QCOMPARE(i, -1000);

    i = locale.toInt(QLatin1String("-10,000"), &ok);
    QVERIFY(ok);
    QCOMPARE(i, -10000);

    i = locale.toInt(QLatin1String("-10000"), &ok);
    QVERIFY(ok);
    QCOMPARE(i, -10000);

    i = locale.toInt(QLatin1String("-100,000"), &ok);
    QVERIFY(ok);
    QCOMPARE(i, -100000);

    i = locale.toInt(QLatin1String("-100000"), &ok);
    QVERIFY(ok);
    QCOMPARE(i, -100000);

    i = locale.toInt(QLatin1String("-1,000,000"), &ok);
    QVERIFY(ok);
    QCOMPARE(i, -1000000);

    i = locale.toInt(QLatin1String("-1000000"), &ok);
    QVERIFY(ok);
    QCOMPARE(i, -1000000);

    // Several Arabic locales have an invisible script-marker before their signs:
    const QLocale egypt(QLocale::Arabic, QLocale::Egypt);
    QCOMPARE(egypt.toString(-403), u"\u061c-\u0664\u0660\u0663"_s);
    i = egypt.toInt(u"\u061c-\u0664\u0660\u0663"_s, &ok);
    QVERIFY(ok);
    QCOMPARE(i, -403);
    i = egypt.toInt(u"\u061c+\u0664\u0660\u0663"_s, &ok);
    QVERIFY(ok);
    QCOMPARE(i, 403);

    // Likewise Farsi:
    const QLocale farsi(QLocale::Persian, QLocale::Iran);
    QCOMPARE(farsi.toString(-403), u"\u200e\u2212\u06f4\u06f0\u06f3"_s);
    i = farsi.toInt(u"\u200e\u2212\u06f4\u06f0\u06f3"_s, &ok);
    QVERIFY(ok);
    QCOMPARE(i, -403);
    i = farsi.toInt(u"\u200e+\u06f4\u06f0\u06f3"_s, &ok);
    QVERIFY(ok);
    QCOMPARE(i, 403);
    QT_TEST_EQUALITY_OPS(egypt, farsi, false);
}

#ifdef QT_BUILD_INTERNAL
#include <private/qlocale_p.h>
#endif

void tst_QLocale::testNames_data()
{
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<QLocale::Territory>("country");

#ifdef QT_BUILD_INTERNAL
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
#else
    QSKIP("Only internal builds can access the data to set up this test");
#endif // QT_BUILD_INTERNAL
}

void tst_QLocale::testNames()
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

void tst_QLocale::debugOutput()
{
    // Test operator<<(QDebug, const QLocale &) works as intended:
    QTest::failOnWarning();
    {
        const QLocale en(QLocale::English, QLocale::LatinScript, QLocale::UnitedStates);
        QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                             "QLocale(English, Latin, United States)");
        qWarning() << en;
    }
    {
        const auto params = [](const QLocale &loc) {
            return (QLocale::languageToString(loc.language())
                    + u", " + QLocale::scriptToString(loc.script())
                    + u", " + QLocale::territoryToString(loc.territory())).toUtf8();
        };
        const QLocale sys = QLocale::system();
        QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                             ("QLocale::system()/* " + params(sys) + " */").constData());
        // QTBUG-133922: system and its CLDR counterpart should differ.
        qWarning() << sys;
        const QLocale match(sys.language(), sys.script(), sys.territory());
        QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                             ("QLocale(" + params(match) + ')').constData());
        qWarning() << match;
    }
}

void tst_QLocale::dayName_data()
{
    QTest::addColumn<QString>("locale_name");
    QTest::addColumn<QString>("dayName");
    QTest::addColumn<int>("day");
    QTest::addColumn<QLocale::FormatType>("format");

    QTest::newRow("no_NO") << QString("no_NO") << QString("tirsdag") << 2 << QLocale::LongFormat;
    QTest::newRow("nb_NO") << QString("nb_NO") << QString("tirsdag") << 2 << QLocale::LongFormat;
    QTest::newRow("nn_NO") << QString("nn_NO") << QString("tysdag") << 2 << QLocale::LongFormat;

    QTest::newRow("C long") << QString("C") << QString("Sunday") << 7 << QLocale::LongFormat;
    QTest::newRow("C short") << QString("C") << QString("Sun") << 7 << QLocale::ShortFormat;
    QTest::newRow("C narrow") << QString("C") << QString("7") << 7 << QLocale::NarrowFormat;

    QTest::newRow("ru_RU long")
        << QString("ru_RU")
        << QString::fromUtf8("\320\262\320\276\321\201\320\272\321\200\320"
                             "\265\321\201\320\265\320\275\321\214\320\265")
        << 7 << QLocale::LongFormat;
    QTest::newRow("ru_RU short")
        << QString("ru_RU") << QString::fromUtf8("\320\262\321\201") << 7 << QLocale::ShortFormat;
    QTest::newRow("ru_RU narrow")
        << QString("ru_RU") << u"\u0412"_s << 7 << QLocale::NarrowFormat;

    QTest::newRow("ga_IE/Mon") << QString("ga_IE") << QString("Luan") << 1 << QLocale::ShortFormat;
    QTest::newRow("ga_IE/Sun") << QString("ga_IE") << QString("Domh") << 7 << QLocale::ShortFormat;
    QTest::newRow("el_GR/Tue")
        << QString("el_GR") << QString::fromUtf8("\316\244\317\201\316\257")
        << 2 << QLocale::ShortFormat;
    QTest::newRow("el_GR/Thu")
        << QString("el_GR") << QString::fromUtf8("\316\240\316\255\316\274")
        << 4 << QLocale::ShortFormat;
    QTest::newRow("el_GR/Sat")
        << QString("el_GR") << QString::fromUtf8("\316\243\316\254\316\262")
        << 6 << QLocale::ShortFormat;

    // Main concern is that short != narrow, for the benefit of QTBUG-10506, QTBUG-84877.
    QTest::newRow("zh long")
        << QString("zh") << QString::fromUtf8("\u661F\u671F\u56DB") << 4 << QLocale::LongFormat;
    QTest::newRow("zh short")
        << QString("zh") << QString::fromUtf8("\u5468\u56DB") << 4 << QLocale::ShortFormat;
    QTest::newRow("zh narrow")
        << QString("zh") << QString::fromUtf8("\u56DB") << 4 << QLocale::NarrowFormat;
}

void tst_QLocale::dayName()
{
    QFETCH(QString, locale_name);
    QFETCH(int, day);
    QFETCH(QLocale::FormatType, format);

    QLocale l(locale_name);
    QTEST(l.dayName(day, format), "dayName");
}

void tst_QLocale::standaloneDayName_data()
{
    QTest::addColumn<QString>("locale_name");
    QTest::addColumn<QString>("dayName");
    QTest::addColumn<int>("day");
    QTest::addColumn<QLocale::FormatType>("format");

    QTest::newRow("no_NO") << QString("no_NO") << QString("tirsdag") << 2 << QLocale::LongFormat;
    QTest::newRow("nb_NO") << QString("nb_NO") << QString("tirsdag") << 2 << QLocale::LongFormat;
    QTest::newRow("nn_NO") << QString("nn_NO") << QString("tysdag") << 2 << QLocale::LongFormat;

    QTest::newRow("C invalid: 0 long") << QString("C") << QString() << 0 << QLocale::LongFormat;
    QTest::newRow("C invalid: 0 short") << QString("C") << QString() << 0 << QLocale::ShortFormat;
    QTest::newRow("C invalid: 0 narrow") << QString("C") << QString() << 0 << QLocale::NarrowFormat;
    QTest::newRow("C invalid: 8 long") << QString("C") << QString() << 8 << QLocale::LongFormat;
    QTest::newRow("C invalid: 8 short") << QString("C") << QString() << 8 << QLocale::ShortFormat;
    QTest::newRow("C invalid: 8 narrow") << QString("C") << QString() << 8 << QLocale::NarrowFormat;

    QTest::newRow("C long") << QString("C") << QString("Sunday") << 7 << QLocale::LongFormat;
    QTest::newRow("C short") << QString("C") << QString("Sun") << 7 << QLocale::ShortFormat;
    QTest::newRow("C narrow") << QString("C") << QString("S") << 7 << QLocale::NarrowFormat;

    QTest::newRow("ru_RU long")
        << QString("ru_RU")
        << QString::fromUtf8("\320\262\320\276\321\201\320\272\321\200\320"
                             "\265\321\201\320\265\320\275\321\214\320\265")
        << 7 << QLocale::LongFormat;
    QTest::newRow("ru_RU short")
        << QString("ru_RU") << QString::fromUtf8("\320\262\321\201") << 7 << QLocale::ShortFormat;
    QTest::newRow("ru_RU narrow")
        << QString("ru_RU") << QString::fromUtf8("\320\222") << 7 << QLocale::NarrowFormat;

    // Main concern is that short != narrow, for the benefit of QTBUG-10506, QTBUG-84877.
    QTest::newRow("zh long")
        << QString("zh") << QString::fromUtf8("\u661F\u671F\u56DB") << 4 << QLocale::LongFormat;
    QTest::newRow("zh short")
        << QString("zh") << QString::fromUtf8("\u5468\u56DB") << 4 << QLocale::ShortFormat;
    QTest::newRow("zh narrow")
        << QString("zh") << QString::fromUtf8("\u56DB") << 4 << QLocale::NarrowFormat;
}

void tst_QLocale::standaloneDayName()
{
    QFETCH(QString, locale_name);
    QFETCH(int, day);
    QFETCH(QLocale::FormatType, format);

    QLocale l(locale_name);
    QTEST(l.standaloneDayName(day, format), "dayName");
}

void tst_QLocale::underflowOverflow()
{
    QString a(QLatin1String("0.") + QString(546, QLatin1Char('0')) + QLatin1String("1e10"));
    bool ok = false;
    double d = a.toDouble(&ok);
    QVERIFY(!ok);
    QCOMPARE(d, 0.0);

    a = QLatin1String("1e600");
    ok = false;
    d = a.toDouble(&ok);
    QVERIFY(!ok); // detectable overflow
    QVERIFY(qIsInf(d));

    a = QLatin1String("-1e600");
    ok = false;
    d = a.toDouble(&ok);
    QVERIFY(!ok); // detectable underflow
    QVERIFY(qIsInf(-d));

    a = QLatin1String("1e-600");
    ok = false;
    d = a.toDouble(&ok);
    QVERIFY(!ok);
    QCOMPARE(d, 0.0);

    a = QLatin1String("-9223372036854775809");
    a.toLongLong(&ok);
    QVERIFY(!ok);
}

void tst_QLocale::defaultNumberingSystem_data()
{
    QTest::addColumn<QString>("expect");

    QTest::newRow("sk_SK") << QStringLiteral("123");
    QTest::newRow("ta_IN") << QStringLiteral("123");
    QTest::newRow("te_IN") << QStringLiteral("123");
    QTest::newRow("hi_IN") << QStringLiteral("123");
    QTest::newRow("gu_IN") << QStringLiteral("123");
    QTest::newRow("kn_IN") << QStringLiteral("123");
    QTest::newRow("pa_IN") << QStringLiteral("123");
    QTest::newRow("ne_IN") << QString::fromUtf8("१२३");
    QTest::newRow("mr_IN") << QString::fromUtf8("१२३");
    QTest::newRow("ml_IN") << QStringLiteral("123");
    QTest::newRow("kok_IN") << QStringLiteral("123");
}

void tst_QLocale::defaultNumberingSystem()
{
    QLatin1String name(QTest::currentDataTag());
    QLocale locale(name);
    QTEST(locale.toString(123), "expect");
}

void tst_QLocale::ampm_data()
{
    QTest::addColumn<QString>("morn");
    QTest::addColumn<QString>("even");

    QTest::newRow("C") << QStringLiteral("AM") << QStringLiteral("PM");
    QTest::newRow("de_DE") << QStringLiteral("AM") << QStringLiteral("PM");
    QTest::newRow("sv_SE") << QStringLiteral("fm") << QStringLiteral("em");
    QTest::newRow("nl_NL") << QStringLiteral("a.m.") << QStringLiteral("p.m.");
    QTest::newRow("uk_UA") << QString::fromUtf8("\320\264\320\277")
                           << QString::fromUtf8("\320\277\320\277");
    QTest::newRow("tr_TR") << QString::fromUtf8("\303\226\303\226")
                           << QString::fromUtf8("\303\226\123");
    QTest::newRow("id_ID") << QStringLiteral("AM") << QStringLiteral("PM");
    QTest::newRow("ta_LK") << QString::fromUtf8("AM") << QString::fromUtf8("PM");
}

void tst_QLocale::ampm()
{
    QLatin1String name(QTest::currentDataTag());
    QLocale locale(name == QLatin1String("C") ? QLocale(QLocale::C) : QLocale(name));
    QTEST(locale.amText(), "morn");
    QTEST(locale.pmText(), "even");
}

void tst_QLocale::dateFormat()
{
    const QLocale c(QLocale::C);
    // check that the NarrowFormat is the same as ShortFormat.
    QCOMPARE(c.dateFormat(QLocale::NarrowFormat), c.dateFormat(QLocale::ShortFormat));

    const QLocale no("no_NO");
    QCOMPARE(no.dateFormat(QLocale::NarrowFormat), QLatin1String("dd.MM.yyyy"));
    QCOMPARE(no.dateFormat(QLocale::ShortFormat), QLatin1String("dd.MM.yyyy"));
    QCOMPARE(no.dateFormat(QLocale::LongFormat), QLatin1String("dddd d. MMMM yyyy"));

    const QLocale ca("en_CA");
    QCOMPARE(ca.dateFormat(QLocale::ShortFormat), QLatin1String("yyyy-MM-dd"));
    QCOMPARE(ca.dateFormat(QLocale::LongFormat), QLatin1String("dddd, MMMM d, yyyy"));

    const QLocale ja("ja_JP");
    QCOMPARE(ja.dateFormat(QLocale::ShortFormat), QLatin1String("yyyy/MM/dd"));

    const QLocale ir("ga_IE");
    QCOMPARE(ir.dateFormat(QLocale::ShortFormat), QLatin1String("dd/MM/yyyy"));

    QT_TEST_EQUALITY_OPS(c, no, false);
    QT_TEST_EQUALITY_OPS(ca, ja, false);
    QT_TEST_EQUALITY_OPS(ca, ir, false);
    QT_TEST_EQUALITY_OPS(ir, ja, false);

    const auto sys = QLocale::system(); // QTBUG-92018, ru_RU on MS
    const QDate date(2021, 3, 17);
    QCOMPARE(sys.toString(date, sys.dateFormat(QLocale::LongFormat)), sys.toString(date));

    // Check that system locale can format a date with year < 1601 (MS cut-off):
    QString old = sys.toString(QDate(1564, 2, 15), QLocale::LongFormat);
    QVERIFY(!old.isEmpty());
    QVERIFY2(old.contains(u"1564"), qPrintable(old + QLatin1String(" for locale ") + sys.name()));
    old = sys.toString(QDate(1564, 2, 15), QLocale::ShortFormat);
    QVERIFY(!old.isEmpty());
    QVERIFY2(old.contains(u"64"), qPrintable(old + QLatin1String(" for locale ") + sys.name()));

    // Including one with year % 100 < 12 (lest we substitute year for month or day)
    old = sys.toString(QDate(1511, 11, 11), QLocale::LongFormat);
    QVERIFY(!old.isEmpty());
    QVERIFY2(old.contains(u"1511"), qPrintable(old + QLatin1String(" for locale ") + sys.name()));
    old = sys.toString(QDate(1511, 11, 11), QLocale::ShortFormat);
    QVERIFY(!old.isEmpty());
    QVERIFY2(old.contains(u"11"), qPrintable(old + QLatin1String(" for locale ") + sys.name()));

    // And, indeed, one for a negative year:
    old = sys.toString(QDate(-1173, 5, 1), QLocale::LongFormat);
    QVERIFY(!old.isEmpty());
    qsizetype yearDigitStart = old.indexOf(u"1173");
    QVERIFY2(yearDigitStart != -1, qPrintable(old + QLatin1String(" for locale ") + sys.name()));
    QStringView before = QStringView(old).first(yearDigitStart);
    QVERIFY2(before.endsWith(QChar('-')) || before.endsWith(QChar(0x2212)),
             qPrintable(old + QLatin1String(" has no minus sign for locale ") + sys.name()));
}

void tst_QLocale::timeFormat()
{
    const QLocale c(QLocale::C);
    // check that the NarrowFormat is the same as ShortFormat.
    QCOMPARE(c.timeFormat(QLocale::NarrowFormat), c.timeFormat(QLocale::ShortFormat));

    const QLocale no("no_NO");
    QCOMPARE(no.timeFormat(QLocale::NarrowFormat), QLatin1String("HH:mm"));
    QCOMPARE(no.timeFormat(QLocale::ShortFormat), QLatin1String("HH:mm"));
    QCOMPARE(no.timeFormat(QLocale::LongFormat), "HH:mm:ss tttt"_L1);

    const QLocale id("id_ID");
    QCOMPARE(id.timeFormat(QLocale::ShortFormat), QLatin1String("HH.mm"));
    QCOMPARE(id.timeFormat(QLocale::LongFormat), "HH.mm.ss tttt"_L1);

    const QLocale cat("ca_ES");
    QCOMPARE(cat.timeFormat(QLocale::ShortFormat), QLatin1String("H:mm"));
    QCOMPARE(cat.timeFormat(QLocale::LongFormat), "H:mm:ss (tttt)"_L1);

    const QLocale bra("pt_BR");
    QCOMPARE(bra.timeFormat(QLocale::ShortFormat), QLatin1String("HH:mm"));
    QCOMPARE(bra.timeFormat(QLocale::LongFormat), "HH:mm:ss tttt"_L1);

    // QTBUG-123872 - we kludge CLDR's B to Ap:
    const QLocale tw("zh_TW");
    QCOMPARE(tw.timeFormat(QLocale::ShortFormat), "Aph:mm"_L1);
    QCOMPARE(tw.timeFormat(QLocale::LongFormat), "Aph:mm:ss [tttt]"_L1);

    QT_TEST_EQUALITY_OPS(c, no, false);
    QT_TEST_EQUALITY_OPS(id, no, false);
    QT_TEST_EQUALITY_OPS(c, cat, false);
    QT_TEST_EQUALITY_OPS(bra, no, false);
}

void tst_QLocale::dateTimeFormat()
{
    const QLocale c(QLocale::C);
    // check that the NarrowFormat is the same as ShortFormat.
    QCOMPARE(c.dateTimeFormat(QLocale::NarrowFormat), c.dateTimeFormat(QLocale::ShortFormat));

    const QLocale no("no_NO");
    QCOMPARE(no.dateTimeFormat(QLocale::NarrowFormat), QLatin1String("dd.MM.yyyy HH:mm"));
    QCOMPARE(no.dateTimeFormat(QLocale::ShortFormat), QLatin1String("dd.MM.yyyy HH:mm"));
    QCOMPARE(no.dateTimeFormat(QLocale::LongFormat), "dddd d. MMMM yyyy HH:mm:ss tttt"_L1);

    QT_TEST_EQUALITY_OPS(c, no, false);
}

void tst_QLocale::monthName()
{
    const QLocale c(QLocale::C);
    QCOMPARE(c.monthName(0, QLocale::ShortFormat), QString());
    QCOMPARE(c.monthName(0, QLocale::LongFormat), QString());
    QCOMPARE(c.monthName(0, QLocale::NarrowFormat), QString());
    QCOMPARE(c.monthName(13, QLocale::ShortFormat), QString());
    QCOMPARE(c.monthName(13, QLocale::LongFormat), QString());
    QCOMPARE(c.monthName(13, QLocale::NarrowFormat), QString());

    QCOMPARE(c.monthName(1, QLocale::LongFormat), QLatin1String("January"));
    QCOMPARE(c.monthName(1, QLocale::ShortFormat), QLatin1String("Jan"));
    QCOMPARE(c.monthName(1, QLocale::NarrowFormat), QLatin1String("1"));

    const QLocale de("de_DE");
    QCOMPARE(de.monthName(12, QLocale::LongFormat), QLatin1String("Dezember"));
    QCOMPARE(de.monthName(12, QLocale::ShortFormat), QLatin1String("Dez."));
    // 'de' locale doesn't have narrow month name
    QCOMPARE(de.monthName(12, QLocale::NarrowFormat), QLatin1String("D"));

    const QLocale ru("ru_RU");
    QCOMPARE(ru.monthName(1, QLocale::LongFormat),
             QString::fromUtf8("\321\217\320\275\320\262\320\260\321\200\321\217"));
    QCOMPARE(ru.monthName(1, QLocale::ShortFormat),
             QString::fromUtf8("\321\217\320\275\320\262\56"));
    QCOMPARE(ru.monthName(1, QLocale::NarrowFormat), QString::fromUtf8("\320\257"));
    const auto sys = QLocale::system();
    if (sys.language() == QLocale::Russian) // QTBUG-92018
        QCOMPARE_NE(sys.monthName(3), sys.standaloneMonthName(3));

    const QLocale ir("ga_IE");
    QCOMPARE(ir.monthName(1, QLocale::ShortFormat), QLatin1String("Ean"));
    QCOMPARE(ir.monthName(12, QLocale::ShortFormat), QLatin1String("Noll"));

    const QLocale cz("cs_CZ");
    QCOMPARE(cz.monthName(1, QLocale::ShortFormat), QLatin1String("led"));
    QCOMPARE(cz.monthName(12, QLocale::ShortFormat), QLatin1String("pro"));

    // For the benefit of QTBUG-10506, QTBUG-84877.
    const QLocale cn(QLocale::Chinese);
    QCOMPARE_NE(cn.monthName(3, QLocale::NarrowFormat), cn.monthName(3, QLocale::ShortFormat));
    if (sys.language() == QLocale::Chinese) {
        QCOMPARE_NE(sys.monthName(3, QLocale::NarrowFormat),
                    sys.monthName(3, QLocale::ShortFormat));
    }
}

void tst_QLocale::standaloneMonthName()
{
    const QLocale c(QLocale::C);
    QCOMPARE(c.monthName(0, QLocale::ShortFormat), QString());
    QCOMPARE(c.monthName(0, QLocale::LongFormat), QString());
    QCOMPARE(c.monthName(0, QLocale::NarrowFormat), QString());
    QCOMPARE(c.monthName(13, QLocale::ShortFormat), QString());
    QCOMPARE(c.monthName(13, QLocale::LongFormat), QString());
    QCOMPARE(c.monthName(13, QLocale::NarrowFormat), QString());

    QCOMPARE(c.standaloneMonthName(1, QLocale::LongFormat), QLatin1String("January"));
    QCOMPARE(c.standaloneMonthName(1, QLocale::ShortFormat), QLatin1String("Jan"));

    const QLocale de("de_DE");
    // For de_DE locale Unicode CLDR database doesn't contain standalone long months
    // so just checking if the return value is the same as in monthName().
    QCOMPARE(de.standaloneMonthName(12, QLocale::LongFormat), QLatin1String("Dezember"));
    QCOMPARE(de.standaloneMonthName(12, QLocale::LongFormat),
             de.monthName(12, QLocale::LongFormat));
    QCOMPARE(de.standaloneMonthName(12, QLocale::ShortFormat), QLatin1String("Dez"));
    QCOMPARE(de.standaloneMonthName(12, QLocale::NarrowFormat), QLatin1String("D"));

    QLocale ru("ru_RU");
    QCOMPARE(ru.standaloneMonthName(1, QLocale::LongFormat),
             QString::fromUtf8("\xd1\x8f\xd0\xbd\xd0\xb2\xd0\xb0\xd1\x80\xd1\x8c"));
    QCOMPARE(ru.standaloneMonthName(1, QLocale::ShortFormat),
             QString::fromUtf8("\xd1\x8f\xd0\xbd\xd0\xb2."));
    QCOMPARE(ru.standaloneMonthName(1, QLocale::NarrowFormat), QString::fromUtf8("\xd0\xaf"));

    // For the benefit of QTBUG-10506, QTBUG-84877.
    const QLocale cn(QLocale::Chinese);
    QCOMPARE_NE(cn.standaloneMonthName(3, QLocale::NarrowFormat),
                cn.standaloneMonthName(3, QLocale::ShortFormat));
    const auto sys = QLocale::system();
    if (sys.language() == QLocale::Chinese) {
        QCOMPARE_NE(sys.standaloneMonthName(3, QLocale::NarrowFormat),
                    sys.standaloneMonthName(3, QLocale::ShortFormat));
    }
}

void tst_QLocale::languageToString_data()
{
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<QString>("name");

    // Prone to change at CLDR updates.
    QTest::newRow("cu") << QLocale::Church << u"Church Slavic"_s;
    QTest::newRow("dyo") << QLocale::JolaFonyi << u"Jola-Fonyi"_s;
    QTest::newRow("ff") << QLocale::Fulah << u"Fula"_s;
    QTest::newRow("gd") << QLocale::Gaelic << u"Scottish Gaelic"_s;
    QTest::newRow("ht") << QLocale::Haitian << u"Haitian Creole"_s;
    QTest::newRow("lu") << QLocale::LubaKatanga << u"Luba-Katanga"_s;
    QTest::newRow("mgh") << QLocale::MakhuwaMeetto << u"Makhuwa-Meetto"_s;
    QTest::newRow("mgo") << QLocale::Meta << u"Meta\u02bc"_s;
    QTest::newRow("mi") << QLocale::Maori << u"M\u0101" "ori"_s;
    QTest::newRow("nb") << QLocale::NorwegianBokmal << u"Norwegian Bokm\u00e5" "l"_s;
    QTest::newRow("nqo") << QLocale::Nko << u"N\u2019" "Ko"_s;
    QTest::newRow("quc") << QLocale::Kiche << u"K\u02bc" "iche\u02bc"_s;
    QTest::newRow("sah") << QLocale::Sakha << u"Yakut"_s;
    QTest::newRow("vo") << QLocale::Volapuk << u"Volap\u00fc" "k"_s;
}

void tst_QLocale::languageToString()
{
    QFETCH(const QLocale::Language, language);
    QTEST(QLocale::languageToString(language), "name");
}

void tst_QLocale::scriptToString_data()
{
    QTest::addColumn<QLocale::Script>("script");
    QTest::addColumn<QString>("name");

    // Prone to change at CLDR updates.
    QTest::newRow("Cans")
        << QLocale::CanadianAboriginalScript << u"Unified Canadian Aboriginal Syllabics"_s;
    QTest::newRow("Dupl") << QLocale::DuployanScript << u"Duployan shorthand"_s;
    QTest::newRow("Egyp") << QLocale::EgyptianHieroglyphsScript << u"Egyptian hieroglyphs"_s;
    QTest::newRow("Nkoo") << QLocale::NkoScript << u"N\u2019" "Ko"_s;
    QTest::newRow("Phag") << QLocale::PhagsPaScript << u"Phags-pa"_s;
    QTest::newRow("Rohg") << QLocale::HanifiScript << u"Hanifi Rohingya"_s;
    QTest::newRow("Sgnw") << QLocale::SignWritingScript << u"SignWriting"_s;
    QTest::newRow("Xsux") << QLocale::CuneiformScript << u"Sumero-Akkadian Cuneiform"_s;
}

void tst_QLocale::scriptToString()
{
    QFETCH(const QLocale::Script, script);
    QTEST(QLocale::scriptToString(script), "name");
}

void tst_QLocale::territoryToString_data()
{
    QTest::addColumn<QLocale::Territory>("territory");
    QTest::addColumn<QString>("name");
    // Prone to change at CLDR updates.

    QTest::newRow("AX") << QLocale::AlandIslands << u"\u00c5" "land Islands"_s;
    QTest::newRow("AG") << QLocale::AntiguaAndBarbuda << u"Antigua & Barbuda"_s;
    QTest::newRow("BA") << QLocale::BosniaAndHerzegovina << u"Bosnia & Herzegovina"_s;
    QTest::newRow("BL") << QLocale::SaintBarthelemy << u"St. Barth\u00e9" "lemy"_s;
    QTest::newRow("CC") << QLocale::CocosIslands << u"Cocos (Keeling) Islands"_s;
    QTest::newRow("CD") << QLocale::CongoKinshasa << u"Congo - Kinshasa"_s;
    QTest::newRow("CG") << QLocale::CongoBrazzaville << u"Congo - Brazzaville"_s;
    QTest::newRow("CI") << QLocale::IvoryCoast << u"C\u00f4" "te d\u2019" "Ivoire"_s;
    QTest::newRow("CW") << QLocale::Curacao << u"Cura\u00e7" "ao"_s;
    QTest::newRow("EA") << QLocale::CeutaAndMelilla << u"Ceuta & Melilla"_s;
    QTest::newRow("GS")
        << QLocale::SouthGeorgiaAndSouthSandwichIslands
        << u"South Georgia & South Sandwich Islands"_s;
    QTest::newRow("GW") << QLocale::GuineaBissau << u"Guinea-Bissau"_s;
    QTest::newRow("HM") << QLocale::HeardAndMcDonaldIslands << u"Heard & McDonald Islands"_s;
    QTest::newRow("IM") << QLocale::IsleOfMan << u"Isle of Man"_s;
    QTest::newRow("KN") << QLocale::SaintKittsAndNevis << u"St. Kitts & Nevis"_s;
    QTest::newRow("LC") << QLocale::SaintLucia << u"St. Lucia"_s;
    QTest::newRow("MF") << QLocale::SaintMartin << u"St. Martin"_s;
    QTest::newRow("MK") << QLocale::Macedonia << u"North Macedonia"_s;
    QTest::newRow("MM") << QLocale::Myanmar << u"Myanmar (Burma)"_s;
    QTest::newRow("MO") << QLocale::Macao << u"Macao SAR China"_s;
    QTest::newRow("PM") << QLocale::SaintPierreAndMiquelon << u"St. Pierre & Miquelon"_s;
    QTest::newRow("PN") << QLocale::Pitcairn << u"Pitcairn Islands"_s;
    QTest::newRow("RE") << QLocale::Reunion << u"R\u00e9" "union"_s;
    QTest::newRow("SH") << QLocale::SaintHelena << u"St. Helena"_s;
    QTest::newRow("SJ") << QLocale::SvalbardAndJanMayen << u"Svalbard & Jan Mayen"_s;
    QTest::newRow("ST")
        << QLocale::SaoTomeAndPrincipe << u"S\u00e3" "o Tom\u00e9" " & Pr\u00ed" "ncipe"_s;
    QTest::newRow("TA") << QLocale::TristanDaCunha << u"Tristan da Cunha"_s;
    QTest::newRow("TC") << QLocale::TurksAndCaicosIslands << u"Turks & Caicos Islands"_s;
    QTest::newRow("TR") << QLocale::Turkey << u"T\u00fc" "rkiye"_s;
    QTest::newRow("TT") << QLocale::TrinidadAndTobago << u"Trinidad & Tobago"_s;
    QTest::newRow("UM") << QLocale::UnitedStatesOutlyingIslands << u"U.S. Outlying Islands"_s;
    QTest::newRow("VC") << QLocale::SaintVincentAndGrenadines << u"St. Vincent & Grenadines"_s;
    QTest::newRow("VI") << QLocale::UnitedStatesVirginIslands << u"U.S. Virgin Islands"_s;
    QTest::newRow("WF") << QLocale::WallisAndFutuna << u"Wallis & Futuna"_s;
    QTest::newRow("001") << QLocale::World << u"world"_s;
}

void tst_QLocale::territoryToString()
{
    QFETCH(const QLocale::Territory, territory);
    QTEST(QLocale::territoryToString(territory), "name");
}

void tst_QLocale::endonym_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("territory");

    QTest::newRow("en")
        << QLocale(QLocale::English, QLocale::UnitedStates)
        << u"American English"_s << u"United States"_s;
    QTest::newRow("en_GB")
        << QLocale(QLocale::English, QLocale::UnitedKingdom)
        << u"British English"_s << u"United Kingdom"_s; // So inaccurate
}

void tst_QLocale::endonym()
{
    QFETCH(const QLocale, locale);

    auto report = qScopeGuard([locale]() {
        qDebug()
            << "Failed for" << locale.name()
            << "with language" << QLocale::languageToString(locale.language())
            << "for territory" << QLocale::territoryToString(locale.territory())
            << "in script" << QLocale::scriptToString(locale.script());
    });

    QTEST(locale.nativeLanguageName(), "language");
    QTEST(locale.nativeTerritoryName(), "territory");
    report.dismiss();
}

void tst_QLocale::currency()
{
    const QLocale c(QLocale::C);
    QCOMPARE(c.toCurrencyString(qulonglong(1234)), QString("1234"));
    QCOMPARE(c.toCurrencyString(qlonglong(-1234)), QString("-1234"));
    QCOMPARE(c.toCurrencyString(double(1234.56)), QString("1234.56"));
    QCOMPARE(c.toCurrencyString(double(-1234.56)), QString("-1234.56"));
    QCOMPARE(c.toCurrencyString(double(-1234.5678)), QString("-1234.57"));
    QCOMPARE(c.toCurrencyString(double(-1234.5678), NULL, 4), QString("-1234.5678"));
    QCOMPARE(c.toCurrencyString(double(-1234.56), NULL, 4), QString("-1234.5600"));

    const QLocale en_US("en_US");
    QCOMPARE(en_US.toCurrencyString(qulonglong(1234)), QString("$1,234"));
    QCOMPARE(en_US.toCurrencyString(qlonglong(-1234)), QString("($1,234)"));
    QCOMPARE(en_US.toCurrencyString(double(1234.56)), QString("$1,234.56"));
    QCOMPARE(en_US.toCurrencyString(double(-1234.56)), QString("($1,234.56)"));
    QCOMPARE(en_US.toCurrencyString(double(-1234.5678)), QString("($1,234.57)"));
    QCOMPARE(en_US.toCurrencyString(double(-1234.5678), NULL, 4), QString("($1,234.5678)"));
    QCOMPARE(en_US.toCurrencyString(double(-1234.56), NULL, 4), QString("($1,234.5600)"));

    const QLocale ru_RU("ru_RU");
    QCOMPARE(ru_RU.toCurrencyString(qulonglong(1234)),
             QString::fromUtf8("1" "\xc2\xa0" "234\xc2\xa0\xe2\x82\xbd"));
    QCOMPARE(ru_RU.toCurrencyString(qlonglong(-1234)),
             QString::fromUtf8("-1" "\xc2\xa0" "234\xc2\xa0\xe2\x82\xbd"));
    QCOMPARE(ru_RU.toCurrencyString(double(1234.56)),
             QString::fromUtf8("1" "\xc2\xa0" "234,56\xc2\xa0\xe2\x82\xbd"));
    QCOMPARE(ru_RU.toCurrencyString(double(-1234.56)),
             QString::fromUtf8("-1" "\xc2\xa0" "234,56\xc2\xa0\xe2\x82\xbd"));

    const QLocale de_DE("de_DE");
    QCOMPARE(de_DE.toCurrencyString(qulonglong(1234)),
             QString::fromUtf8("1.234\xc2\xa0\xe2\x82\xac"));
    QCOMPARE(de_DE.toCurrencyString(qulonglong(1234), QLatin1String("BAZ")),
             QString::fromUtf8("1.234\xc2\xa0" "BAZ"));
    QCOMPARE(de_DE.toCurrencyString(qlonglong(-1234)),
             QString::fromUtf8("-1.234\xc2\xa0\xe2\x82\xac"));
    QCOMPARE(de_DE.toCurrencyString(qlonglong(-1234), QLatin1String("BAZ")),
             QString::fromUtf8("-1.234\xc2\xa0" "BAZ"));
    QCOMPARE(de_DE.toCurrencyString(double(1234.56)),
             QString::fromUtf8("1.234,56\xc2\xa0\xe2\x82\xac"));
    QCOMPARE(de_DE.toCurrencyString(double(-1234.56)),
             QString::fromUtf8("-1.234,56\xc2\xa0\xe2\x82\xac"));
    QCOMPARE(de_DE.toCurrencyString(double(-1234.56), QLatin1String("BAZ")),
             QString::fromUtf8("-1.234,56\xc2\xa0" "BAZ"));

    const QLocale es_CR(QLocale::Spanish, QLocale::CostaRica);
    QCOMPARE(es_CR.toCurrencyString(double(1565.25)),
             QString::fromUtf8("\xE2\x82\xA1" "1\xC2\xA0" "565,25"));
    QCOMPARE(es_CR.toCurrencyString(double(12565.25)),
             QString::fromUtf8("\xE2\x82\xA1" "12\xC2\xA0" "565,25"));

    const QLocale system = QLocale::system();
    QVERIFY(system.toCurrencyString(1, QLatin1String("FOO")).contains(QLatin1String("FOO")));
    QT_TEST_EQUALITY_OPS(system, es_CR, false);
}

void tst_QLocale::quoteString()
{
    const QString someText("text");
    const QLocale c(QLocale::C);
    QCOMPARE(c.quoteString(someText), QString::fromUtf8("\x22" "text" "\x22"));
    QCOMPARE(c.quoteString(someText, QLocale::AlternateQuotation),
             QString::fromUtf8("\x27" "text" "\x27"));

    const QLocale de_CH("de_CH");
    QCOMPARE(de_CH.quoteString(someText), QString::fromUtf8("\xe2\x80\x9e" "text" "\xe2\x80\x9c"));
    QCOMPARE(de_CH.quoteString(someText, QLocale::AlternateQuotation),
             QString::fromUtf8("\xe2\x80\x9a" "text" "\xe2\x80\x98"));
    QT_TEST_EQUALITY_OPS(de_CH, c, false);
}

void tst_QLocale::uiLanguages_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QStringList>("all");

    QTest::newRow("C") << QLocale::c() << QStringList{u"C"_s};

    QTest::newRow("en_US")
        << QLocale("en_US") << QStringList{u"en-Latn-US"_s, u"en-US"_s, u"en-Latn"_s, u"en"_s};
    QTest::newRow("en_Latn_US")
        << QLocale("en_Latn_US") // Specifying the default script makes no difference
        << QStringList{u"en-Latn-US"_s, u"en-US"_s, u"en-Latn"_s, u"en"_s};

    QTest::newRow("en_GB")
        << QLocale("en_GB") << QStringList{u"en-Latn-GB"_s, u"en-GB"_s, u"en-Latn"_s, u"en"_s};
    QTest::newRow("en_Dsrt_US")
        << QLocale("en_Dsrt_US") << QStringList{u"en-Dsrt-US"_s, u"en-Dsrt"_s, u"en"_s};

    QTest::newRow("ru_RU")
        << QLocale("ru_RU") << QStringList{u"ru-Cyrl-RU"_s, u"ru-RU"_s, u"ru-Cyrl"_s, u"ru"_s};

    QTest::newRow("zh_Hant")
        << QLocale("zh_Hant")
        << QStringList{u"zh-Hant-TW"_s, u"zh-TW"_s, u"zh-Hant"_s, u"zh"_s};
    QTest::newRow("zh_TW")
        << QLocale("zh_TW")
        << QStringList{u"zh-Hant-TW"_s, u"zh-TW"_s, u"zh-Hant"_s, u"zh"_s};

    QTest::newRow("zh_Hans_CN")
        << QLocale(QLocale::Chinese, QLocale::SimplifiedHanScript, QLocale::China)
        << QStringList{u"zh-Hans-CN"_s, u"zh-CN"_s, u"zh-Hans"_s, u"zh"_s};

    QTest::newRow("pa_IN")
        << QLocale("pa_IN") << QStringList{u"pa-Guru-IN"_s, u"pa-IN"_s, u"pa-Guru"_s, u"pa"_s};
    QTest::newRow("pa_Guru")
        << QLocale("pa_Guru") << QStringList{u"pa-Guru-IN"_s, u"pa-IN"_s, u"pa-Guru"_s, u"pa"_s};
    QTest::newRow("pa_PK")
        << QLocale("pa_PK") << QStringList{u"pa-Arab-PK"_s, u"pa-PK"_s, u"pa-Arab"_s, u"pa"_s};
    QTest::newRow("pa_Arab")
        << QLocale("pa_Arab") << QStringList{u"pa-Arab-PK"_s, u"pa-PK"_s, u"pa-Arab"_s, u"pa"_s};
    // GB has no native Punjabi locales, so GB is eliminated by likely subtag rules:
    QTest::newRow("pa_GB")
        << QLocale("pa_GB") << QStringList{u"pa-Guru-IN"_s, u"pa-IN"_s, u"pa-Guru"_s, u"pa"_s};
    QTest::newRow("pa_Arab_GB")
        << QLocale("pa_Arab_GB") << QStringList{u"pa-Arab-PK"_s, u"pa-PK"_s, u"pa-Arab"_s, u"pa"_s};

    // We presently map und (or any other unrecognized language) to C, ignoring
    // what a sub-tag lookup would surely find us.
    QTest::newRow("und_US") << QLocale("und_US") << QStringList{u"C"_s};
    QTest::newRow("und_Latn") << QLocale("und_Latn") << QStringList{u"C"_s};
}

void tst_QLocale::uiLanguages()
{
    // Compare mySystemLocale(), which tests the same for a custom system locale.
    QFETCH(const QLocale, locale);
    QFETCH(const QStringList, all);
    const auto expected = [all](QChar sep) {
        QStringList adjusted;
        for (QString name : all)
            adjusted << name.replace(u'-', sep);
        return adjusted;
    };

    {
        // By default tags are joined with a dash:
        const QStringList actual = locale.uiLanguages();
        auto reporter = qScopeGuard([&actual]() {
            qDebug("\n\t%ls", qUtf16Printable(actual.join("\n\t"_L1)));
        });
        QCOMPARE(actual, all);
        reporter.dismiss();
    }
    {
        // We also support joining with an underscore:
        const QStringList actual = locale.uiLanguages(QLocale::TagSeparator::Underscore);
        auto reporter = qScopeGuard([&actual]() {
            qDebug("\n\t%ls", qUtf16Printable(actual.join("\n\t"_L1)));
        });
        QCOMPARE(actual, expected(u'_'));
        reporter.dismiss();
    }
    {
        // Or, in fact, any ASCII character:
        const QStringList actual = locale.uiLanguages(QLocale::TagSeparator{'|'});
        auto reporter = qScopeGuard([&actual]() {
            qDebug("\n\t%ls", qUtf16Printable(actual.join("\n\t"_L1)));
        });
        QCOMPARE(actual, expected(u'|'));
        reporter.dismiss();
    }
    {
        // Non-ASCII separator (here, y-umlaut) is unsupported.
        QTest::ignoreMessage(QtWarningMsg, "QLocale::uiLanguages(): "
                             "Using non-ASCII separator '\u00ff' (ff) is unsupported");
        const QStringList actual = locale.uiLanguages(QLocale::TagSeparator{'\xff'});
        auto reporter = qScopeGuard([&actual]() {
            qDebug("\n\t%ls", qUtf16Printable(actual.join("\n\t"_L1)));
        });
        QCOMPARE(actual, QStringList{});
        reporter.dismiss();
    }
}

void tst_QLocale::weekendDays()
{
    const QLocale c(QLocale::C);
    QList<Qt::DayOfWeek> days;
    days << Qt::Monday << Qt::Tuesday << Qt::Wednesday << Qt::Thursday << Qt::Friday;
    QCOMPARE(c.weekdays(), days);
}

void tst_QLocale::listPatterns()
{
    QStringList sl1;
    QStringList sl2;
    sl2 << "aaa";
    QStringList sl3;
    sl3 << "aaa" << "bbb";
    QStringList sl4;
    sl4 << "aaa" << "bbb" << "ccc";
    QStringList sl5;
    sl5 << "aaa" << "bbb" << "ccc" << "ddd";

    const QLocale c(QLocale::C);
    QCOMPARE(c.createSeparatedList(sl1), QString(""));
    QCOMPARE(c.createSeparatedList(sl2), QString("aaa"));
    QCOMPARE(c.createSeparatedList(sl3), QString("aaa, bbb"));
    QCOMPARE(c.createSeparatedList(sl4), QString("aaa, bbb, ccc"));
    QCOMPARE(c.createSeparatedList(sl5), QString("aaa, bbb, ccc, ddd"));

    const QLocale en_US("en_US");
    QCOMPARE(en_US.createSeparatedList(sl1), QString(""));
    QCOMPARE(en_US.createSeparatedList(sl2), QString("aaa"));
    QCOMPARE(en_US.createSeparatedList(sl3), QString("aaa and bbb"));
    QCOMPARE(en_US.createSeparatedList(sl4), QString("aaa, bbb, and ccc"));
    QCOMPARE(en_US.createSeparatedList(sl5), QString("aaa, bbb, ccc, and ddd"));

    const QLocale zh_CN("zh_CN");
    QCOMPARE(zh_CN.createSeparatedList(sl1), QString(""));
    QCOMPARE(zh_CN.createSeparatedList(sl2), QString("aaa"));
    QCOMPARE(zh_CN.createSeparatedList(sl3), QString::fromUtf8("aaa" "\xe5\x92\x8c" "bbb"));
    QCOMPARE(zh_CN.createSeparatedList(sl4),
             QString::fromUtf8("aaa" "\xe3\x80\x81" "bbb" "\xe5\x92\x8c" "ccc"));
    QCOMPARE(zh_CN.createSeparatedList(sl5),
             QString::fromUtf8("aaa" "\xe3\x80\x81" "bbb" "\xe3\x80\x81"
                               "ccc" "\xe5\x92\x8c" "ddd"));
}

void tst_QLocale::measurementSystems_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QLocale::MeasurementSystem>("system");
    QTest::newRow("en_US") << QLocale(QLocale::English, QLocale::UnitedStates) << QLocale::ImperialUSSystem;
    QTest::newRow("en_GB") << QLocale(QLocale::English, QLocale::UnitedKingdom) << QLocale::ImperialUKSystem;
    QTest::newRow("en_AU") << QLocale(QLocale::English, QLocale::Australia) << QLocale::MetricSystem;
    QTest::newRow("de") << QLocale(QLocale::German) << QLocale::MetricSystem;
}

void tst_QLocale::measurementSystems()
{
    QFETCH(QLocale, locale);
    QTEST(locale.measurementSystem(), "system");
}

void tst_QLocale::QTBUG_26035_positivesign()
{
    QLocale locale(QLocale::C);
    bool ok (false);
    QCOMPARE(locale.toInt(QString("+100,000"), &ok), 100000);
    QVERIFY(ok);
    ok = false;
    QCOMPARE(locale.toInt(QString("+100,000,000"), &ok), 100000000);
    QVERIFY(ok);
    ok = false;
    QCOMPARE(locale.toLongLong(QString("+100,000"), &ok), (qlonglong)100000);
    QVERIFY(ok);
    ok = false;
    QCOMPARE(locale.toLongLong(QString("+100,000,000"), &ok), (qlonglong)100000000);
    QVERIFY(ok);
}

void tst_QLocale::textDirection_data()
{
    QTest::addColumn<int>("language");
    QTest::addColumn<int>("script");
    QTest::addColumn<bool>("rightToLeft");

    for (int language = QLocale::C; language <= QLocale::LastLanguage; ++language) {
        bool rightToLeft = false;
        switch (language) {
        // based on likelySubtags for RTL scripts
        case QLocale::AncientGreek:
        case QLocale::Arabic:
        case QLocale::Aramaic:
        case QLocale::Avestan:
        case QLocale::Baluchi:
        case QLocale::CentralKurdish:
        case QLocale::Divehi:
//        case QLocale::Fulah:
//        case QLocale::Hausa:
        case QLocale::Hebrew:
//        case QLocale::Hungarian:
        case QLocale::Kashmiri:
//        case QLocale::Kurdish:
        case QLocale::Mandingo:
        case QLocale::Mazanderani:
        case QLocale::Mende:
        case QLocale::Nko:
        case QLocale::NorthernLuri:
        case QLocale::Pahlavi:
        case QLocale::Pashto:
        case QLocale::Persian:
        case QLocale::Phoenician:
        case QLocale::Sindhi:
        case QLocale::SouthernKurdish:
        case QLocale::Syriac:
        case QLocale::Torwali:
        case QLocale::Uighur:
        case QLocale::Urdu:
        case QLocale::WesternBalochi:
        case QLocale::Yiddish:
            // false if there is no locale data for language:
            rightToLeft = (QLocale(QLocale::Language(language)).language()
                           == QLocale::Language(language));
            break;
        default:
            break;
        }
        const QString testName = QLocale::languageToCode(QLocale::Language(language));
        QTest::newRow(qPrintable(testName)) << language << int(QLocale::AnyScript) << rightToLeft;
    }
    QTest::newRow("pa_Arab") << int(QLocale::Punjabi) << int(QLocale::ArabicScript) << true;
    QTest::newRow("uz_Arab") << int(QLocale::Uzbek) << int(QLocale::ArabicScript) << true;
}

void tst_QLocale::textDirection()
{
    QFETCH(int, language);
    QFETCH(int, script);

    QLocale locale(QLocale::Language(language), QLocale::Script(script), QLocale::AnyTerritory);
    QTEST(locale.textDirection() == Qt::RightToLeft, "rightToLeft");
}

void tst_QLocale::formattedDataSize_data()
{
    QTest::addColumn<QLocale::Language>("language");
    QTest::addColumn<int>("decimalPlaces");
    QTest::addColumn<QLocale::DataSizeFormats>("units");
    QTest::addColumn<qint64>("bytes");
    QTest::addColumn<QString>("output");

    struct {
        const char *name;
        QLocale::Language lang;
        const char *bytes;
        const char abbrev;
        const char sep; // decimal separator
    } data[] = {
        { "English", QLocale::English, "bytes", 'B', '.' },
        { "French", QLocale::French, "octets", 'o', ',' },
        { "C", QLocale::C, "bytes", 'B', '.' }
    };

    constexpr auto min64 = (std::numeric_limits<qint64>::min)();
    constexpr auto max64 = (std::numeric_limits<qint64>::max)();

    for (const auto row : data) {
#define ROWB(id, deci, num, text)                 \
        QTest::addRow("%s-%s", row.name, id)      \
            << row.lang << deci << format         \
            << qint64{num} << (QString(text) + QChar(' ') + QString(row.bytes))
#define ROWQ(id, deci, num, head, tail)           \
        QTest::addRow("%s-%s", row.name, id)      \
            << row.lang << deci << format         \
            << qint64{num} << (QString(head) + QChar(row.sep) + QString(tail) + QChar(row.abbrev))

        // Metatype system fails to handle raw enum members as format; needs variable
        {
            const QLocale::DataSizeFormats format = QLocale::DataSizeIecFormat;
            ROWB("IEC-0", 2, 0, "0");
            ROWB("IEC-10", 2, 10, "10");
            ROWB("IEC--10", 2, -10, "-10");
            ROWQ("IEC-12Ki", 2, 12345, "12", "06 Ki");
            ROWQ("IEC-16Ki", 2, 16384, "16", "00 Ki");
            ROWQ("IEC--16Ki", 2, -16384, "-16", "00 Ki");
            ROWQ("IEC-1235k", 2, 1234567, "1", "18 Mi");
            ROWQ("IEC-1374k", 2, 1374744, "1", "31 Mi");
            ROWQ("IEC-1234M", 2, 1234567890, "1", "15 Gi");
            ROWQ("IEC-min", 2, min64, "-8", "00 Ei");
            ROWQ("IEC-max", 2, max64, "8", "00 Ei");
        }
        {
            const QLocale::DataSizeFormats format = QLocale::DataSizeTraditionalFormat;
            ROWB("Trad-0", 2, 0, "0");
            ROWB("Trad-10", 2, 10, "10");
            ROWB("Trad--10", 2, -10, "-10");
            ROWQ("Trad-12Ki", 2, 12345, "12", "06 k");
            ROWQ("Trad-16Ki", 2, 16384, "16", "00 k");
            ROWQ("Trad-1235k", 2, 1234567, "1", "18 M");
            ROWQ("Trad--1235k", 2, -1234567, "-1", "18 M");
            ROWQ("Trad-1374k", 2, 1374744, "1", "31 M");
            ROWQ("Trad-1234M", 2, 1234567890, "1", "15 G");
            ROWQ("Trad-min", 2, min64, "-8", "00 E");
            ROWQ("Trad-max", 2, max64, "8", "00 E");
        }
        {
            const QLocale::DataSizeFormats format = QLocale::DataSizeSIFormat;
            ROWB("Decimal-0", 2, 0, "0");
            ROWB("Decimal-10", 2, 10, "10");
            ROWB("Decimal--10", 2, -10, "-10");
            ROWQ("Decimal-16Ki", 2, 16384, "16", "38 k");
            ROWQ("Decimal-1234k", 2, 1234567, "1", "23 M");
            ROWQ("Decimal-1374k", 2, 1374744, "1", "37 M");
            ROWQ("Decimal-1234M", 2, 1234567890, "1", "23 G");
            ROWQ("Decimal--1234M", 2, -1234567890, "-1", "23 G");
            ROWQ("Decimal-min", 2, min64, "-9", "22 E");
            ROWQ("Decimal-max", 2, max64, "9", "22 E");
        }
#undef ROWQ
#undef ROWB
    }

    // Languages which don't use a Latin alphabet

    const QLocale::DataSizeFormats iecFormat = QLocale::DataSizeIecFormat;
    const QLocale::DataSizeFormats traditionalFormat = QLocale::DataSizeTraditionalFormat;
    const QLocale::DataSizeFormats siFormat = QLocale::DataSizeSIFormat;
    const QLocale::Language lang = QLocale::Russian;

    QTest::newRow("Russian-IEC-0") << lang << 2 << iecFormat << 0LL << QString("0 \u0431\u0430\u0439\u0442\u044B");
    QTest::newRow("Russian-IEC-10") << lang << 2 << iecFormat << 10LL << QString("10 \u0431\u0430\u0439\u0442\u044B");
    // CLDR doesn't provide IEC prefixes (yet?) so they aren't getting translated
    QTest::newRow("Russian-IEC-12Ki") << lang << 2 << iecFormat << 12345LL << QString("12,06 KiB");
    QTest::newRow("Russian-IEC-16Ki") << lang << 2 << iecFormat << 16384LL << QString("16,00 KiB");
    QTest::newRow("Russian-IEC-1235k") << lang << 2 << iecFormat << 1234567LL << QString("1,18 MiB");
    QTest::newRow("Russian-IEC-1374k") << lang << 2 << iecFormat << 1374744LL << QString("1,31 MiB");
    QTest::newRow("Russian-IEC-1234M") << lang << 2 << iecFormat << 1234567890LL << QString("1,15 GiB");

    QTest::newRow("Russian-Trad-0") << lang << 2 << traditionalFormat << 0LL << QString("0 \u0431\u0430\u0439\u0442\u044B");
    QTest::newRow("Russian-Trad-10") << lang << 2 << traditionalFormat << 10LL << QString("10 \u0431\u0430\u0439\u0442\u044B");
    QTest::newRow("Russian-Trad-12Ki") << lang << 2 << traditionalFormat << 12345LL << QString("12,06 \u043A\u0411");
    QTest::newRow("Russian-Trad-16Ki") << lang << 2 << traditionalFormat << 16384LL << QString("16,00 \u043A\u0411");
    QTest::newRow("Russian-Trad-1235k") << lang << 2 << traditionalFormat << 1234567LL << QString("1,18 \u041C\u0411");
    QTest::newRow("Russian-Trad-1374k") << lang << 2 << traditionalFormat << 1374744LL << QString("1,31 \u041C\u0411");
    QTest::newRow("Russian-Trad-1234M") << lang << 2 << traditionalFormat << 1234567890LL << QString("1,15 \u0413\u0411");

    QTest::newRow("Russian-Decimal-0") << lang << 2 << siFormat << 0LL << QString("0 \u0431\u0430\u0439\u0442\u044B");
    QTest::newRow("Russian-Decimal-10") << lang << 2 << siFormat << 10LL << QString("10 \u0431\u0430\u0439\u0442\u044B");
    QTest::newRow("Russian-Decimal-16Ki") << lang << 2 << siFormat << 16384LL << QString("16,38 \u043A\u0411");
    QTest::newRow("Russian-Decimal-1234k") << lang << 2 << siFormat << 1234567LL << QString("1,23 \u041C\u0411");
    QTest::newRow("Russian-Decimal-1374k") << lang << 2 << siFormat << 1374744LL << QString("1,37 \u041C\u0411");
    QTest::newRow("Russian-Decimal-1234M") << lang << 2 << siFormat << 1234567890LL << QString("1,23 \u0413\u0411");
}

void tst_QLocale::formattedDataSize()
{
    QFETCH(QLocale::Language, language);
    QFETCH(int, decimalPlaces);
    QFETCH(QLocale::DataSizeFormats, units);
    QFETCH(const qint64, bytes);

    QTEST(QLocale(language).formattedDataSize(bytes, decimalPlaces, units), "output");
}

void tst_QLocale::bcp47Name_data()
{
    QTest::addColumn<QString>("expect");

    QTest::newRow("C") << QStringLiteral("en-POSIX");
    QTest::newRow("en") << QStringLiteral("en");
    QTest::newRow("en_US") << QStringLiteral("en");
    QTest::newRow("en_GB") << QStringLiteral("en-GB");
    QTest::newRow("en_DE") << QStringLiteral("en-DE");
    QTest::newRow("de_DE") << QStringLiteral("de");
    QTest::newRow("sr_RS") << QStringLiteral("sr");
    QTest::newRow("sr_Cyrl_RS") << QStringLiteral("sr");
    QTest::newRow("sr_Latn_RS") << QStringLiteral("sr-Latn");
    QTest::newRow("sr_ME") << QStringLiteral("sr-ME");
    QTest::newRow("sr_Cyrl_ME") << QStringLiteral("sr-Cyrl-ME");
    QTest::newRow("sr_Latn_ME") << QStringLiteral("sr-ME");

    // Fall back to defaults when country isn't in CLDR for this language:
    QTest::newRow("sr_HR") << QStringLiteral("sr");
    QTest::newRow("sr_Cyrl_HR") << QStringLiteral("sr");
    QTest::newRow("sr_Latn_HR") << QStringLiteral("sr-Latn");
}

void tst_QLocale::bcp47Name()
{
    QFETCH(const QString, expect);
    const auto expected = [expect](QChar ch) {
        // Kludge around QString::replace() not being const.
        QString copy = expect;
        return copy.replace(u'-', ch);
    };

    const auto locale = QLocale(QLatin1String(QTest::currentDataTag()));
    QCOMPARE(locale.bcp47Name(), expect);
    QCOMPARE(locale.bcp47Name(QLocale::TagSeparator::Underscore), expected(u'_'));
    QCOMPARE(locale.bcp47Name(QLocale::TagSeparator{'|'}), expected(u'|'));
    QTest::ignoreMessage(QtWarningMsg, "QLocale::bcp47Name(): "
                         "Using non-ASCII separator '\u00ff' (ff) is unsupported");
    QCOMPARE(locale.bcp47Name(QLocale::TagSeparator{'\xff'}), QString());
    QT_TEST_EQUALITY_OPS(locale, QLocale(QLatin1String(QTest::currentDataTag())), true);
}

#ifndef QT_NO_SYSTEMLOCALE
#  ifdef QT_BUILD_INTERNAL
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

void tst_QLocale::mySystemLocale_data()
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

void tst_QLocale::mySystemLocale()
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

void tst_QLocale::systemGrouping_data()
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

void tst_QLocale::systemGrouping()
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

#  endif // QT_BUILD_INTERNAL

void tst_QLocale::systemLocaleDayAndMonthNames_data()
{
    QTest::addColumn<QByteArray>("locale");
    QTest::addColumn<QDate>("date");
    QTest::addColumn<QLocale::FormatType>("format");
    QTest::addColumn<QString>("month");
    QTest::addColumn<QString>("standaloneMonth");
    QTest::addColumn<QString>("day");
    QTest::addColumn<QString>("standaloneDay");

    // en_US and de_DE locale outputs for ICU and macOS are similar.
    // ru_RU are different.
    // Windows has its own representation for all of the locales

#if QT_CONFIG(icu)
    // августа, август, понедельник, понедельник
    QTest::newRow("ru_RU 30.08.2021 long")
            << QByteArray("ru_RU") << QDate(2021, 8, 30) << QLocale::LongFormat
            << QString("\u0430\u0432\u0433\u0443\u0441\u0442\u0430")
            << QString("\u0430\u0432\u0433\u0443\u0441\u0442")
            << QString("\u043f\u043e\u043d\u0435\u0434\u0435\u043b\u044c\u043d\u0438\u043a")
            << QString("\u043f\u043e\u043d\u0435\u0434\u0435\u043b\u044c\u043d\u0438\u043a");
    // авг., авг., пн, пн
    QTest::newRow("ru_RU 30.08.2021 short")
            << QByteArray("ru_RU") << QDate(2021, 8, 30) << QLocale::ShortFormat
            << QString("\u0430\u0432\u0433.") << QString("\u0430\u0432\u0433.")
            << QString("\u043f\u043d") << QString("\u043f\u043d");
    // А, А, П, П
    QTest::newRow("ru_RU 30.08.2021 narrow")
            << QByteArray("ru_RU") << QDate(2021, 8, 30) << QLocale::NarrowFormat
            << QString("\u0410") << QString("\u0410") << QString("\u041f")
            << QString("\u041f");
#elif defined(Q_OS_DARWIN)
    // августа, август, понедельник, понедельник
    QTest::newRow("ru_RU 30.08.2021 long")
            << QByteArray("ru_RU") << QDate(2021, 8, 30) << QLocale::LongFormat
            << QString("\u0430\u0432\u0433\u0443\u0441\u0442\u0430")
            << QString("\u0430\u0432\u0433\u0443\u0441\u0442")
            << QString("\u043f\u043e\u043d\u0435\u0434\u0435\u043b\u044c\u043d\u0438\u043a")
            << QString("\u043f\u043e\u043d\u0435\u0434\u0435\u043b\u044c\u043d\u0438\u043a");
    // авг., авг., Пн, Пн
    QTest::newRow("ru_RU 30.08.2021 short")
            << QByteArray("ru_RU") << QDate(2021, 8, 30) << QLocale::ShortFormat
            << QString("\u0430\u0432\u0433.") << QString("\u0430\u0432\u0433.")
            << QString("\u041f\u043d") << QString("\u041f\u043d");
    // А, А, Пн, П
    QTest::newRow("ru_RU 30.08.2021 narrow")
            << QByteArray("ru_RU") << QDate(2021, 8, 30) << QLocale::NarrowFormat
            << QString("\u0410") << QString("\u0410") << QString("\u041f\u043d")
            << QString("\u041f");
#endif

#if QT_CONFIG(icu) || defined(Q_OS_DARWIN)
    QTest::newRow("en_US 30.08.2021 long")
            << QByteArray("en_US") << QDate(2021, 8, 30) << QLocale::LongFormat
            << "August" << "August" << "Monday" << "Monday";
    QTest::newRow("en_US 30.08.2021 short")
            << QByteArray("en_US") << QDate(2021, 8, 30) << QLocale::ShortFormat
            << "Aug" << "Aug" << "Mon" << "Mon";
    QTest::newRow("en_US 30.08.2021 narrow")
            << QByteArray("en_US") << QDate(2021, 8, 30) << QLocale::NarrowFormat
            << "A" << "A" << "M" << "M";

    QTest::newRow("de_DE 30.08.2021 long")
            << QByteArray("de_DE") << QDate(2021, 8, 30) << QLocale::LongFormat
            << "August" << "August" << "Montag" << "Montag";
    QTest::newRow("de_DE 30.08.2021 short")
            << QByteArray("de_DE") << QDate(2021, 8, 30) << QLocale::ShortFormat
            << "Aug." << "Aug" << "Mo." << "Mo";
    QTest::newRow("de_DE 30.08.2021 narrow")
            << QByteArray("de_DE") << QDate(2021, 8, 30) << QLocale::NarrowFormat
            << "A" << "A" << "M" << "M";
#elif defined(Q_OS_WIN)
    // августа, Август, понедельник, понедельник
    QTest::newRow("ru_RU 30.08.2021 long")
            << QByteArray("ru_RU") << QDate(2021, 8, 30) << QLocale::LongFormat
            << QString("\u0430\u0432\u0433\u0443\u0441\u0442\u0430")
            << QString("\u0410\u0432\u0433\u0443\u0441\u0442")
            << QString("\u043f\u043e\u043d\u0435\u0434\u0435\u043b\u044c\u043d\u0438\u043a")
            << QString("\u043f\u043e\u043d\u0435\u0434\u0435\u043b\u044c\u043d\u0438\u043a");
    // авг, авг, Пн, пн
    QTest::newRow("ru_RU 30.08.2021 short")
            << QByteArray("ru_RU") << QDate(2021, 8, 30) << QLocale::ShortFormat
            << QString("\u0430\u0432\u0433") << QString("\u0430\u0432\u0433")
            << QString("\u041f\u043d") << QString("\u043f\u043d");
    // А, А, Пн, П
    QTest::newRow("ru_RU 30.08.2021 narrow")
            << QByteArray("ru_RU") << QDate(2021, 8, 30) << QLocale::NarrowFormat
            << QString("\u0410") << QString("\u0410") << QString("\u041f\u043d")
            << QString("\u041f");

    QTest::newRow("en_US 30.08.2021 long")
            << QByteArray("en_US") << QDate(2021, 8, 30) << QLocale::LongFormat
            << "August" << "August" << "Monday" << "Monday";
    QTest::newRow("en_US 30.08.2021 short")
            << QByteArray("en_US") << QDate(2021, 8, 30) << QLocale::ShortFormat
            << "Aug" << "Aug" << "Mon" << "Mon";
    QTest::newRow("en_US 30.08.2021 narrow")
            << QByteArray("en_US") << QDate(2021, 8, 30) << QLocale::NarrowFormat
            << "A" << "A" << "Mo" << "M";

    QTest::newRow("de_DE 30.08.2021 long")
            << QByteArray("de_DE") << QDate(2021, 8, 30) << QLocale::LongFormat
            << "August" << "August" << "Montag" << "Montag";
    QTest::newRow("de_DE 30.08.2021 short")
            << QByteArray("de_DE") << QDate(2021, 8, 30) << QLocale::ShortFormat
            << "Aug" << "Aug" << "Mo" << "Mo";
    QTest::newRow("de_DE 30.08.2021 narrow")
            << QByteArray("de_DE") << QDate(2021, 8, 30) << QLocale::NarrowFormat
            << "A" << "A" << "Mo" << "M";
#else
    QSKIP("This test can't run on this OS");
#endif
}

void tst_QLocale::systemLocaleDayAndMonthNames()
{
    QFETCH(QByteArray, locale);
    QFETCH(QDate, date);
    QFETCH(QLocale::FormatType, format);
    locale += ".UTF-8"; // So we don't have to repeat it on every data row !

    const TransientLocale tested(LC_ALL, locale.constData());

    QLocale sys = QLocale::system();
#if !QT_CONFIG(icu)
    // setlocale() does not really change locale on Windows and macOS, we
    // need to actually set the locale manually to run the test
    if (!locale.startsWith(sys.name().toUtf8()))
        QSKIP(("Set locale to " + locale + " manually to run this test.").constData());
#endif

    const int m = date.month();
    QTEST(sys.monthName(m, format), "month");
    QTEST(sys.standaloneMonthName(m, format), "standaloneMonth");

    const int d = date.dayOfWeek();
    QTEST(sys.dayName(d, format), "day");
    QTEST(sys.standaloneDayName(d, format), "standaloneDay");
}

#endif // QT_NO_SYSTEMLOCALE

void tst_QLocale::numberGrouping_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<int>("number");
    QTest::addColumn<QString>("string");
    // Number options set here are expected to be default, but set for the
    // avoidance of uncertainty or susceptibility to changed defaults.

    QLocale c(QLocale::C); // English-style, without separators.
    c.setNumberOptions(c.numberOptions() | QLocale::OmitGroupSeparator);
    QTest::newRow("c:1") << c << 1 << u"1"_s;
    QTest::newRow("c:12") << c << 12 << u"12"_s;
    QTest::newRow("c:123") << c << 123 << u"123"_s;
    QTest::newRow("c:1234") << c << 1234 << u"1234"_s;
    QTest::newRow("c:12345") << c << 12345 << u"12345"_s;
    QTest::newRow("c:123456") << c << 123456 << u"123456"_s;
    QTest::newRow("c:1234567") << c << 1234567 << u"1234567"_s;
    QTest::newRow("c:12345678") << c << 12345678 << u"12345678"_s;
    QTest::newRow("c:123456789") << c << 123456789 << u"123456789"_s;
    QTest::newRow("c:1234567890") << c << 1234567890 << u"1234567890"_s;

    QLocale en(QLocale::English); // English-style, with separators:
    en.setNumberOptions(en.numberOptions() & ~QLocale::OmitGroupSeparator);
    QTest::newRow("en:1") << en << 1 << u"1"_s;
    QTest::newRow("en:12") << en << 12 << u"12"_s;
    QTest::newRow("en:123") << en << 123 << u"123"_s;
    QTest::newRow("en:1,234") << en << 1234 << u"1,234"_s;
    QTest::newRow("en:12,345") << en << 12345 << u"12,345"_s;
    QTest::newRow("en:123,456") << en << 123456 << u"123,456"_s;
    QTest::newRow("en:1,234,567") << en << 1234567 << u"1,234,567"_s;
    QTest::newRow("en:12,345,678") << en << 12345678 << u"12,345,678"_s;
    QTest::newRow("en:123,456,789") << en << 123456789 << u"123,456,789"_s;
    QTest::newRow("en:1,234,567,890") << en << 1234567890 << u"1,234,567,890"_s;

    QLocale es(QLocale::Spanish); // Spanish-style, with separators
    es.setNumberOptions(es.numberOptions() & ~QLocale::OmitGroupSeparator);
    QTest::newRow("es:1") << es << 1 << u"1"_s;
    QTest::newRow("es:12") << es << 12 << u"12"_s;
    QTest::newRow("es:123") << es << 123 << u"123"_s;
    // First split doesn't happen unless first group has at least two digits:
    QTest::newRow("es:1234") << es << 1234 << u"1234"_s;
    QTest::newRow("es:12.345") << es << 12345 << u"12.345"_s;
    QTest::newRow("es:123.456") << es << 123456 << u"123.456"_s;
    // Later splits aren't limited to two digits (QTBUG-115740):
    QTest::newRow("es:1.234.567") << es << 1234567 << u"1.234.567"_s;
    QTest::newRow("es:12.345.678") << es << 12345678 << u"12.345.678"_s;
    QTest::newRow("es:123.456.789") << es << 123456789 << u"123.456.789"_s;
    QTest::newRow("es:1.234.567.890") << es << 1234567890 << u"1.234.567.890"_s;

    QLocale hi(QLocale::Hindi, QLocale::India);
    hi.setNumberOptions(hi.numberOptions() & ~QLocale::OmitGroupSeparator);
    QTest::newRow("hi:1") << hi << 1 << u"1"_s;
    QTest::newRow("hi:12") << hi << 12 << u"12"_s;
    QTest::newRow("hi:123") << hi << 123 << u"123"_s;
    QTest::newRow("hi:1,234") << hi << 1234 << u"1,234"_s;
    QTest::newRow("hi:12,345") << hi << 12345 << u"12,345"_s;
    QTest::newRow("hi:1,23,456") << hi << 123456 << u"1,23,456"_s;
    QTest::newRow("hi:12,34,567") << hi << 1234567 << u"12,34,567"_s;
    QTest::newRow("hi:1,23,45,678") << hi << 12345678 << u"1,23,45,678"_s;
    QTest::newRow("hi:12,34,56,789") << hi << 123456789 << u"12,34,56,789"_s;
    QTest::newRow("hi:1,23,45,67,890") << hi << 1234567890 << u"1,23,45,67,890"_s;
}

void tst_QLocale::numberGrouping()
{
    QFETCH(const QLocale, locale);
    QFETCH(const int, number);
    QFETCH(const QString, string);

    QCOMPARE(locale.toString(number), string);
    QLocale sys = QLocale::system();
    if (sys.language() == locale.language()
            && sys.script() == locale.script()
            && sys.territory() == locale.territory()) {
        sys.setNumberOptions(locale.numberOptions());

        QCOMPARE(sys.toString(number), string);
        if (QLocale() == sys) { // This normally should be the case.
            QCOMPARE(u"%L1"_s.arg(number), string);
            QCOMPARE(u"%L1"_s.arg(double(number), 0, 'f', 0), string);
        }
    }
    // Check round-trip via toInt():
    bool ok;
    int actual = locale.toInt(string, &ok);
    QVERIFY(ok);
    QCOMPARE(actual, number);
}

void tst_QLocale::numberGroupingIndia()
{
    const QLocale indian(QLocale::Hindi, QLocale::India);

    // A 7-bit value (fits in signed 8-bit):
    const QString strResult8("120");
    const qint8 int8 = 120;
    QCOMPARE(indian.toString(int8), strResult8);
    QCOMPARE(indian.toShort(strResult8), short(int8));

    const quint8 uint8 = 120u;
    QCOMPARE(indian.toString(uint8), strResult8);
    QCOMPARE(indian.toShort(strResult8), short(uint8));

    // Boundary case for needing a first separator:
    const QString strResultSep("3,210");
    const short shortSep = 3210;
    QCOMPARE(indian.toString(shortSep), strResultSep);
    QCOMPARE(indian.toShort(strResultSep), shortSep);

    const ushort uShortSep = 3210u;
    QCOMPARE(indian.toString(uShortSep), strResultSep);
    QCOMPARE(indian.toUShort(strResultSep), uShortSep);

    // 15-bit:
    const QString strResult16("24,310");
    const short short16 = 24310;
    QCOMPARE(indian.toString(short16), strResult16);
    QCOMPARE(indian.toShort(strResult16), short16);

    const ushort uShort16 = 24310u;
    QCOMPARE(indian.toString(uShort16), strResult16);
    QCOMPARE(indian.toUShort(strResult16), uShort16);

    // 31-bit
    const QString strResult32("2,03,04,05,010");
    const int integer32 = 2030405010;
    QCOMPARE(indian.toString(integer32), strResult32);
    QCOMPARE(indian.toInt(strResult32), integer32);

    const uint uInteger32 = 2030405010u;
    QCOMPARE(indian.toString(uInteger32), strResult32);
    QCOMPARE(indian.toUInt(strResult32), uInteger32);

    bool ok = false;
    QCOMPARE(indian.toInt(u"1,23,45,678"_s, &ok), 12345678);
    QVERIFY(ok);
    // Malformed (bad grouping):
    QCOMPARE(indian.toInt(u"123,45,678"_s, &ok), 0);
    QVERIFY(!ok);

    // 63-bit:
    const QString strResult64("60,05,00,40,03,00,20,01,000");
    const qint64 int64 = Q_INT64_C(6005004003002001000);
    QCOMPARE(indian.toString(int64), strResult64);
    QCOMPARE(indian.toLongLong(strResult64), int64);

    const quint64 uint64 = Q_UINT64_C(6005004003002001000);
    QCOMPARE(indian.toString(uint64), strResult64);
    QCOMPARE(indian.toULongLong(strResult64), uint64);
}

void tst_QLocale::numberFormatChakma()
{
    const QLocale chakma(QLocale::Chakma, QLocale::ChakmaScript, QLocale::Bangladesh);
    const uint zeroVal = 0x11136; // Unicode's representation of Chakma zero
    const QChar data[] = {
        QChar::highSurrogate(zeroVal), QChar::lowSurrogate(zeroVal),
        QChar::highSurrogate(zeroVal + 1), QChar::lowSurrogate(zeroVal + 1),
        QChar::highSurrogate(zeroVal + 2), QChar::lowSurrogate(zeroVal + 2),
        QChar::highSurrogate(zeroVal + 3), QChar::lowSurrogate(zeroVal + 3),
        QChar::highSurrogate(zeroVal + 4), QChar::lowSurrogate(zeroVal + 4),
        QChar::highSurrogate(zeroVal + 5), QChar::lowSurrogate(zeroVal + 5),
        QChar::highSurrogate(zeroVal + 6), QChar::lowSurrogate(zeroVal + 6),
    };
    const QChar separator(QLatin1Char(','));
    const QString
        zero = QString::fromRawData(data, 2),
        one = QString::fromRawData(data + 2, 2),
        two = QString::fromRawData(data + 4, 2),
        three = QString::fromRawData(data + 6, 2),
        four = QString::fromRawData(data + 8, 2),
        five = QString::fromRawData(data + 10, 2),
        six = QString::fromRawData(data + 12, 2);

    // A 7-bit value (fits in signed 8-bit):
    const QString strResult8 = one + two + zero;
    const qint8 int8 = 120;
    QCOMPARE(chakma.toString(int8), strResult8);
    QCOMPARE(chakma.toShort(strResult8), short(int8));

    const quint8 uint8 = 120;
    QCOMPARE(chakma.toString(uint8), strResult8);
    QCOMPARE(chakma.toShort(strResult8), short(uint8));

    // Boundary case for needing a first separator:
    const QString strResultSep = three + separator + two + one + zero;
    const short shortSep = 3210;
    QCOMPARE(chakma.toString(shortSep), strResultSep);
    QCOMPARE(chakma.toShort(strResultSep), shortSep);

    const ushort uShortSep = 3210u;
    QCOMPARE(chakma.toString(uShortSep), strResultSep);
    QCOMPARE(chakma.toUShort(strResultSep), uShortSep);

    // Fifteen-bit value:
    const QString strResult16 = two + four + separator + three + one + zero;
    const short short16 = 24310;
    QCOMPARE(chakma.toString(short16), strResult16);
    QCOMPARE(chakma.toShort(strResult16), short16);

    const ushort uShort16 = 24310u;
    QCOMPARE(chakma.toString(uShort16), strResult16);
    QCOMPARE(chakma.toUShort(strResult16), uShort16);

    // 31-bit
    const QString strResult32 =
        two + separator + zero + three + separator + zero + four
        + separator + zero + five + separator + zero + one + zero;
    const int integer32 = 2030405010;
    QCOMPARE(chakma.toString(integer32), strResult32);
    QCOMPARE(chakma.toInt(strResult32), integer32);

    bool ok = false; // Lakh is the Hindi name for 1,00,000
    const QString goodLakh = one + separator + two + three + separator + four + five + six;
    QCOMPARE(chakma.toInt(goodLakh, &ok), 123456);
    QVERIFY(ok);
    const QString longThousand = one + two + three + separator + four + five + six;
    QCOMPARE(chakma.toInt(longThousand, &ok), 0);
    QVERIFY(!ok);
    const QString goodCrore // Crore is Hindi for 1,00,00,000
        = one + separator + two + three + separator + zero + zero + separator + four + five + six;
    QCOMPARE(chakma.toInt(goodCrore, &ok), 12300456);
    QVERIFY(ok);
    // Officially should be grouped, but we tolerate a complete lack of grouping
    // for backwards-compatibility reasons.
    const QString badLakh
        = one + two + three + four + five + six;
    QCOMPARE(chakma.toInt(badLakh, &ok), 123456);
    QVERIFY(ok);
    // However, even one group separator requires all to be correctly placed:
    const QString longLakh
        = one + two + three + separator + zero + zero + separator + four + five + six;
    QCOMPARE(chakma.toInt(longLakh, &ok), 0);
    QVERIFY(!ok);

    const uint uInteger32 = 2030405010u;
    QCOMPARE(chakma.toString(uInteger32), strResult32);
    QCOMPARE(chakma.toUInt(strResult32), uInteger32);

    // 63-bit:
    const QString strResult64 =
        six + zero + separator + zero + five + separator + zero + zero + separator
        + four + zero + separator + zero + three + separator + zero + zero + separator
        + two + zero + separator + zero + one + separator + zero + zero + zero;
    const qint64 int64 = Q_INT64_C(6005004003002001000);
    QCOMPARE(chakma.toString(int64), strResult64);
    QCOMPARE(chakma.toLongLong(strResult64), int64);

    const quint64 uint64 = Q_UINT64_C(6005004003002001000);
    QCOMPARE(chakma.toString(uint64), strResult64);
    QCOMPARE(chakma.toULongLong(strResult64), uint64);
}

void tst_QLocale::lcsToCode()
{
    QCOMPARE(QLocale::languageToCode(QLocale::AnyLanguage), QString());
    QCOMPARE(QLocale::languageToCode(QLocale::C), QString("C"));
    QCOMPARE(QLocale::languageToCode(QLocale::English), QString("en"));
    QCOMPARE(QLocale::languageToCode(QLocale::Albanian), u"sq"_s);
    QCOMPARE(QLocale::languageToCode(QLocale::Albanian, QLocale::ISO639Part1), u"sq"_s);
    QCOMPARE(QLocale::languageToCode(QLocale::Albanian, QLocale::ISO639Part2B), u"alb"_s);
    QCOMPARE(QLocale::languageToCode(QLocale::Albanian, QLocale::ISO639Part2T), u"sqi"_s);
    QCOMPARE(QLocale::languageToCode(QLocale::Albanian, QLocale::ISO639Part3), u"sqi"_s);

    QCOMPARE(QLocale::languageToCode(QLocale::Taita), u"dav"_s);
    QCOMPARE(QLocale::languageToCode(QLocale::Taita,
                                     QLocale::ISO639Part1 | QLocale::ISO639Part2B
                                             | QLocale::ISO639Part2T),
             QString());
    QCOMPARE(QLocale::languageToCode(QLocale::Taita, QLocale::ISO639Part3), u"dav"_s);
    QCOMPARE(QLocale::languageToCode(QLocale::English, QLocale::LanguageCodeTypes {}), QString());

    // Legacy codes can only be used to convert them to Language values, not other way around.
    QCOMPARE(QLocale::languageToCode(QLocale::NorwegianBokmal, QLocale::LegacyLanguageCode),
             QString());

    QCOMPARE(QLocale::territoryToCode(QLocale::AnyTerritory), QString());
    QCOMPARE(QLocale::territoryToCode(QLocale::UnitedStates), QString("US"));
    QCOMPARE(QLocale::territoryToCode(QLocale::EuropeanUnion), QString("EU"));

    QCOMPARE(QLocale::scriptToCode(QLocale::AnyScript), QString());
    QCOMPARE(QLocale::scriptToCode(QLocale::SimplifiedHanScript), QString("Hans"));
}

static constexpr auto AnyLanguageCode = QLocale::LanguageCodeType::AnyLanguageCode;

void tst_QLocale::codeToLang_data()
{
    QTest::addColumn<QStringView>("input");
    QTest::addColumn<QLocale::LanguageCodeTypes>("options");
    QTest::addColumn<QLocale::Language>("expected");

    auto row = [](const char *tag, QStringView in, QLocale::Language expected,
                  QLocale::LanguageCodeTypes options = AnyLanguageCode)
    {
        QTest::addRow("%s", tag) << in << options << expected;
    };
    auto invalid = [](const char *tag, QStringView in,
                      QLocale::LanguageCodeTypes options = AnyLanguageCode)
    {
        constexpr auto InvalidScript = QLocale::Language::AnyLanguage;
        QTest::addRow("invalid:%s", tag) << in << options << InvalidScript;
    };

    constexpr bool QTBUG_138562 = QT_VERSION >= QT_VERSION_CHECK(6,6,0);
    // (introduced by 3dcd6b7ec98b2edf9654bcefdb83134c4c3d2a38, to be precise)

    invalid("null", nullptr);
    invalid("empty", u"");
    invalid("1*SP", u" ");
    if constexpr (!QTBUG_138562) {
    invalid("2*SP", u"  ");
    invalid("3*SP", u"   ");
    }
    invalid("4*SP", u"    ");
    invalid("und", u"und"); // does not exist
    invalid("e", u"e");     // too short
    row("en", u"en", QLocale::English);
    row("eN", u"eN", QLocale::English);
    row("EN", u"EN", QLocale::English);
    row("En", u"En", QLocale::English);
    row("eng", u"eng", QLocale::English);
    row("Eng", u"Eng", QLocale::English);
    row("eNg", u"eNg", QLocale::English);
    row("enG", u"enG", QLocale::English);
    row("ha", u"ha", QLocale::Hausa);
    invalid("ha/alpha3", u"ha", QLocale::ISO639Alpha3);
    row("haw", u"haw", QLocale::Hawaiian);
    invalid("haw/alpha2", u"haw", QLocale::ISO639Alpha2);

    row("sq", u"sq", QLocale::Albanian);
    row("alb", u"alb", QLocale::Albanian);
    row("sqi", u"sqi", QLocale::Albanian);
    row("sq/part1", u"sq", QLocale::Albanian, QLocale::ISO639Part1);
    invalid("sq/part3", u"sq", QLocale::ISO639Part3);
    row("alb/part2b", u"alb", QLocale::Albanian, QLocale::ISO639Part2B);
    invalid("alb/part2t/part3", u"alb", QLocale::ISO639Part2T | QLocale::ISO639Part3);
    row("sqi/part2t", u"sqi", QLocale::Albanian, QLocale::ISO639Part2T);
    row("sqi/part3", u"sqi", QLocale::Albanian, QLocale::ISO639Part3);
    invalid("sqi/part1/part2b", u"sqi", QLocale::ISO639Part1 | QLocale::ISO639Part2B);

    // Legacy code
    row("no", u"no", QLocale::NorwegianBokmal);
    invalid("no (part 1)", u"no", QLocale::ISO639Part1);

    invalid("aaaa", u"aaaa"); // too long
    invalid("2*NUL", QStringView(u"\0\0", 2));
    invalid("3*NUL", QStringView(u"\0\0\0", 3));

    // Codes with invalid characters:

    invalid("1", u"1"); // numeric
    if constexpr (!QTBUG_138562) {
    invalid("11", u"11");
    invalid("111", u"111");
    }
    invalid("1111", u"1111");
    if constexpr (!QTBUG_138562) {
    invalid("a1", u"a1");
    invalid("aa1", u"aa1");
    }
    invalid("aaa1", u"aaa1");

    invalid("1*AUML", u"ä"); // non-ASCII
    if constexpr (!QTBUG_138562) {
    invalid("2*AUML", u"ää");
    invalid("3*AUML", u"äää");
    }
    invalid("4*AUML", u"ääää");
    if constexpr (!QTBUG_138562) {
    invalid("1*a+AUML", u"aä");
    invalid("2*a+AUML", u"aaä");
    }
    invalid("3*a+AUML", u"aaaä");

    invalid("ar_1", u"١"); // Arabic 1...1234 (non-L1)
    if constexpr (!QTBUG_138562) {
    invalid("ar_12", u"١٢");
    invalid("ar_123", u"١٢٣");
    }
    invalid("ar_1234", u"١٢٣٤");
    if constexpr (!QTBUG_138562) {
    invalid("ar_a1", u"a١"); // a...aaa + Arabic 1
    invalid("ar_aa1", u"aa١");
    }
    invalid("ar_aaa1", u"aaa١");

    if constexpr (!QTBUG_138562) {
    invalid("hier-A042", u"𓀰"); // EGYPTIAN HIEROGLYPH A042 U13030 (non-BMP)
    invalid("a+hier-A042", u"a𓀰");
    }

    // valid codes with invalid characters at the end should not match valid codes:

    invalid("de+null", QStringView(u"de\0", 3));
    invalid("de+space", u"de ");     // character below [A-z]
    invalid("de1", u"de1");          // numeric character
    invalid("de^", u"de^");          // character between [A-Z] and [a-z]
    invalid("de~", u"de~");          // character above [A-z]
    invalid("de+0x80", u"de\u0080"); // negative character (if char is signed)
    invalid("de+0xff", u"de\u00ff"); // UCHAR_MAX (if char is signed)
    invalid("de+non-L1", u"de١");      // Arabic 1
}

void tst_QLocale::codeToLang()
{
    QFETCH(const QStringView, input);
    QFETCH(const QLocale::LanguageCodeTypes, options);
    QFETCH(const QLocale::Language, expected);

    QEXPECT_FAIL("invalid:de+null", "This should probably be rejected, too", Abort);
    QCOMPARE(QLocale::codeToLanguage(input, options), expected);
}

void tst_QLocale::codeToLcs()
{
    QCOMPARE(QLocale::codeToTerritory(QString()), QLocale::AnyTerritory);
    QCOMPARE(QLocale::codeToTerritory(QString("ZZ")), QLocale::AnyTerritory);
    QCOMPARE(QLocale::codeToTerritory(QString("US")), QLocale::UnitedStates);
    QCOMPARE(QLocale::codeToTerritory(QString("us")), QLocale::UnitedStates);
    QCOMPARE(QLocale::codeToTerritory(QString("USA")), QLocale::AnyTerritory);
    QCOMPARE(QLocale::codeToTerritory(QString("EU")), QLocale::EuropeanUnion);
    QCOMPARE(QLocale::codeToTerritory(QString("001")), QLocale::World);
    QCOMPARE(QLocale::codeToTerritory(QString("150")), QLocale::Europe);

    QCOMPARE(QLocale::codeToScript(QString()), QLocale::AnyScript);
    QCOMPARE(QLocale::codeToScript(QString("Zzzz")), QLocale::AnyScript);
    QCOMPARE(QLocale::codeToScript(QString("Hans")), QLocale::SimplifiedHanScript);
    // ensure we can find the last script, too:
    QCOMPARE(QLocale::codeToScript(QLocale::scriptToCode(QLocale::LastScript)),
             QLocale::LastScript);
}

#if QT_HAS_LOCALE_CASE_CONVERSION
void tst_QLocale::toLowerUpper_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QString>("lower");
    QTest::addColumn<QString>("upper");

    const QLocale germanLocale = QLocale(u"de_DE"_s);
    const QLocale turkishLocale = QLocale(u"tr_TR"_s);

    QTest::newRow("null string default locale") << QLocale() << QString() << QString();
    QTest::newRow("null string Turkish") << turkishLocale << QString() << QString();
    QTest::newRow("null string German") << germanLocale << QString() << QString();

    QTest::newRow("empty string default locale") << QLocale() << u""_s << u""_s;
    QTest::newRow("empty string Turkish") << turkishLocale << u""_s << u""_s;
    QTest::newRow("empty string German") << germanLocale << u""_s << u""_s;

    QTest::newRow("ASCII i Turkish") << turkishLocale << u"i"_s << u"İ"_s;
    QTest::newRow("ASCII i German") << germanLocale << u"i"_s << u"I"_s;
    QTest::newRow("ASCII ı Turkish") << turkishLocale << u"ı"_s << u"I"_s;

    QTest::newRow("Latin1 default locale") << QLocale() << u"é"_s << u"É"_s;
    QTest::newRow("Latin1 Turkish") << turkishLocale << u"é"_s << u"É"_s;
    QTest::newRow("Latin1 German") << germanLocale << u"é"_s << u"É"_s;

    QTest::newRow("Replacement default locale") << QLocale() << u"\uFFFD"_s << u"\uFFFD"_s;
    QTest::newRow("Replacement Turkish") << turkishLocale << u"\uFFFD"_s << u"\uFFFD"_s;
    QTest::newRow("Replacement German") << germanLocale << u"\uFFFD"_s << u"\uFFFD"_s;

    QTest::newRow("non-Latin1 default locale") << QLocale() << u"δ"_s << u"Δ"_s;
    QTest::newRow("non-Latin1 Turkish") << turkishLocale << u"δ"_s << u"Δ"_s;
    QTest::newRow("non-Latin1 German") << germanLocale << u"δ"_s << u"Δ"_s;
    // Vithkuqi a/A:
    QTest::newRow("non-BMP default locale") << QLocale() << u"\u10597"_s << u"\u10570"_s;
    QTest::newRow("non-BMP Turkish") << turkishLocale << u"\u10597"_s << u"\u10570"_s;
    QTest::newRow("non-BMP German") << germanLocale << u"\u10597"_s << u"\u10570"_s;

    const QString pLowerString = QString(16, QChar('p'));
    const QString pUpperString = QString(16, QChar('P'));
    QTest::newRow("16 letters default locale") << QLocale() << pLowerString << pUpperString;
    QTest::newRow("16 letters Turkish") << turkishLocale << pLowerString << pUpperString;
    QTest::newRow("16 letters German") << germanLocale << pLowerString << pUpperString;

    const QString zLowerString = QString(4096, QChar('z'));
    const QString zUpperString = QString(4096, QChar('Z'));
    QTest::newRow("4096 letters default locale") << QLocale() << zLowerString << zUpperString;
    QTest::newRow("4096 letters Turkish") << turkishLocale << zLowerString << zUpperString;
    QTest::newRow("4096 letters German") << germanLocale << zLowerString << zUpperString;

    const QString iLowerString = QString(4096, u'i');
    const QString iUpperString = QString(4096, u'İ');
    QTest::newRow("4096 Turkiye dotted i") << turkishLocale << iLowerString << iUpperString;

    const QString ILowerString = QString(4096, u'ı');
    const QString IUpperString = QString(4096, u'I');
    QTest::newRow("4096 Turkiye undotted I") << turkishLocale << ILowerString << IUpperString;
}

void tst_QLocale::toLowerUpper()
{
    QFETCH(QLocale, locale);
    QFETCH(QString, lower);
    QFETCH(QString, upper);

    QEXPECT_FAIL("non-BMP default locale",
                 "QTBUG-131489: Handling of code points outside BMP is broken",
                 Abort);
    QEXPECT_FAIL("non-BMP Turkish",
                 "QTBUG-131489: Handling of code points outside BMP is broken",
                 Abort);
    QEXPECT_FAIL("non-BMP German",
                 "QTBUG-131489: Handling of code points outside BMP is broken",
                 Abort);

    QCOMPARE(locale.toLower(upper), lower);
    QCOMPARE(locale.toUpper(lower), upper);
}

void tst_QLocale::toLowerUpperEszett()
{
    const QString eszettLowerString = u"\u00DF"_s;
    const QString eszettUpperString = u"SS"_s;

#if defined(Q_OS_WIN)
    QEXPECT_FAIL("",
                 "Conversion of \u00DF currently returns \u00DF instead of SS or \u1E9E with "
                 "Windows internal API",
                 Abort);
#endif

    QCOMPARE(QLocale().toUpper(eszettLowerString), eszettUpperString);
    QCOMPARE(QLocale(u"de_DE"_s).toUpper(eszettLowerString), eszettUpperString);
    QCOMPARE(QLocale(u"tr_TR"_s).toUpper(eszettLowerString), eszettUpperString);
}
#endif // QT_HAS_LOCALE_CASE_CONVERSION

void tst_QLocale::toLowerUpperFinalSigma_data()
{
    QTest::addColumn<QString>("lower");
    QTest::addColumn<QString>("upper");

    QTest::addRow("logos") << u"λογος"_s
    //                  final sigma ↕
                           << u"ΛΟΓΟΣ"_s;
    QTest::addRow("music") << u"μουσικη"_s
    //              "medial" sigma ↕
                           << u"ΜΟΥΣΙΚΗ"_s;

    //
    // Now the same with "tonos" (stess marker):
    //
    //  Modern Greek uses them on lower-case, but not on upper-case words, but
    //  Unicode/CLDR doesn't/can't have rules for adding or removing them when
    //  case-converting.

    QTest::addRow("logos+tonos") << u"λόγος"_s
    //                        final sigma ↕
                                 << u"ΛΌΓΟΣ"_s;
    QTest::addRow("music+tonos") << u"μουσική"_s
    //                    "medial" sigma ↕
                                 << u"ΜΟΥΣΙΚΉ"_s;
}

void tst_QLocale::toLowerUpperFinalSigma()
{
    QFETCH(const QString, lower);
    QFETCH(const QString, upper);

    static const QLocale gr("gr_GR"_L1);
    if constexpr (!QT_HAS_LOCALE_CASE_CONVERSION) {
        // these fall back to QString::toLower/Upper(), so inherit QTBUG-2163
        QEXPECT_FAIL("logos", "QTBUG-2163", Continue);
        QEXPECT_FAIL("logos+tonos", "QTBUG-2163", Continue);
    }
#ifdef Q_OS_WIN
    QEXPECT_FAIL("logos", "QTBUG-138705", Continue);
    QEXPECT_FAIL("logos+tonos", "QTBUG-138705", Continue);
#endif
    QCOMPARE(gr.toLower(upper), lower);
    QCOMPARE(gr.toUpper(lower), upper);

    // This ought to be a property of the script, so locale-independent:
    static const QLocale c("C"_L1);
    if constexpr (!QT_HAS_LOCALE_CASE_CONVERSION) {
        // these fall back to QString::toLower/Upper(), so inherit QTBUG-2163
        QEXPECT_FAIL("logos", "QTBUG-2163", Continue);
        QEXPECT_FAIL("logos+tonos", "QTBUG-2163", Continue);
    }
#ifdef Q_OS_WIN
    QEXPECT_FAIL("logos", "QTBUG-138705", Continue);
    QEXPECT_FAIL("logos+tonos", "QTBUG-138705", Continue);
#endif
    QCOMPARE(c.toLower(upper), lower);
    QCOMPARE(c.toUpper(lower), upper);

    // For comparison: locale-independent QString::toUpper/Lower():
    // Qt's own implementation does it wrong:
    QEXPECT_FAIL("logos", "QTBUG-2163", Continue);
    QEXPECT_FAIL("logos+tonos", "QTBUG-2163", Continue);
    QCOMPARE(upper.toLower(), lower);
    QCOMPARE(lower.toUpper(), upper);
}

QTEST_MAIN(tst_QLocale)
#include "tst_qlocale.moc"
