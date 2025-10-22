// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <qtimezone.h>
#include <private/qtimezoneprivate_p.h>

#include <qlocale.h>
#include <qscopeguard.h>

#if defined(Q_OS_WIN) && !QT_CONFIG(icu) && !QT_CONFIG(timezone_tzdb)
#  define USING_WIN_TZ
#endif

// Enable to test exhaustively - this is slow and expensive.
// It is also quite likely to trip over problems that we can't necessarily fix.
// #define EXHAUSTIVE_ZONE_DISPLAY

using namespace Qt::StringLiterals;

class tst_QTimeZoneBackend : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Tests of QTZP functionality:
    void isValidId_data();
    void isValidId();
    void roundtripDisplayNames_data();
    void roundtripDisplayNames();
    void findOffsetPrefix_data();
    void findOffsetPrefix();

    // Tests of specific backends (see also ../qtimezone/):
    void icuTest();
    void tzTest();
    void macTest();
    void winTest();

private:
    // Generic tests of privates, called by implementation-specific private tests:
    void testCetPrivate(const QTimeZonePrivate &tzp);
    void testEpochTranPrivate(const QTimeZonePrivate &tzp);
    // Set to true to print debug output, test Display Names and run long stress tests
    static constexpr bool debug = false;
};

void tst_QTimeZoneBackend::isValidId_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<bool>("valid");

    // a-z, A-Z, 0-9, '.', '-', '_' are valid chars
    // Can't start with '-'
    // Parts separated by '/', each part min 1 and max of 14 chars
    // (Android has parts with lengths up to 17, so tolerates this as a special case.)
#define TESTSET(name, section, valid) \
    QTest::newRow(name " front")  << QByteArray(section "/xyz/xyz")    << valid; \
    QTest::newRow(name " middle") << QByteArray("xyz/" section "/xyz") << valid; \
    QTest::newRow(name " back")   << QByteArray("xyz/xyz/" section)    << valid

    // a-z, A-Z, 0-9, '.', '-', '_' are valid chars
    // Can't start with '-'
    // Parts separated by '/', each part min 1 and max of 14 chars
    TESTSET("empty", "", false);
    TESTSET("minimal", "m", true);
#if (defined(Q_OS_ANDROID) || QT_CONFIG(icu)) && !QT_CONFIG(timezone_tzdb)
    TESTSET("maximal", "East-Saskatchewan", true); // Android actually uses this
    TESTSET("too long", "North-Saskatchewan", false); // ... but thankfully not this.
#else
    TESTSET("maximal", "12345678901234", true);
    TESTSET("maximal twice", "12345678901234/12345678901234", true);
    TESTSET("too long", "123456789012345", false);
    TESTSET("too-long/maximal", "123456789012345/12345678901234", false);
    TESTSET("maximal/too-long", "12345678901234/123456789012345", false);
#endif

    TESTSET("bad hyphen", "-hyphen", false);
    TESTSET("good hyphen", "hy-phen", true);

    TESTSET("valid char _", "_", true);
    TESTSET("valid char .", ".", true);
    TESTSET("valid char :", ":", true);
    TESTSET("valid char +", "+", true);
    TESTSET("valid char A", "A", true);
    TESTSET("valid char Z", "Z", true);
    TESTSET("valid char a", "a", true);
    TESTSET("valid char z", "z", true);
    TESTSET("valid char 0", "0", true);
    TESTSET("valid char 9", "9", true);

    TESTSET("valid pair az", "az", true);
    TESTSET("valid pair AZ", "AZ", true);
    TESTSET("valid pair 09", "09", true);
    TESTSET("valid pair .z", ".z", true);
    TESTSET("valid pair _z", "_z", true);
    TESTSET("invalid pair -z", "-z", false);

    TESTSET("valid triple a/z", "a/z", true);
    TESTSET("valid triple a.z", "a.z", true);
    TESTSET("valid triple a-z", "a-z", true);
    TESTSET("valid triple a_z", "a_z", true);
    TESTSET("invalid triple a z", "a z", false);
    TESTSET("invalid triple a\\z", "a\\z", false);
    TESTSET("invalid triple a,z", "a,z", false);

    TESTSET("invalid space", " ", false);
    TESTSET("invalid char ^", "^", false);
    TESTSET("invalid char \"", "\"", false);
    TESTSET("invalid char $", "$", false);
    TESTSET("invalid char %", "%", false);
    TESTSET("invalid char &", "&", false);
    TESTSET("invalid char (", "(", false);
    TESTSET("invalid char )", ")", false);
    TESTSET("invalid char =", "=", false);
    TESTSET("invalid char -", "-", false);
    TESTSET("invalid char ?", "?", false);
    TESTSET("invalid char ß", "ß", false);
    TESTSET("invalid char \\x01", "\x01", false);
    TESTSET("invalid char ' '", " ", false);

#undef TESTSET

    QTest::newRow("az alone") << QByteArray("az") << true;
    QTest::newRow("AZ alone") << QByteArray("AZ") << true;
    QTest::newRow("09 alone") << QByteArray("09") << true;
    QTest::newRow("a/z alone") << QByteArray("a/z") << true;
    QTest::newRow("a.z alone") << QByteArray("a.z") << true;
    QTest::newRow("a-z alone") << QByteArray("a-z") << true;
    QTest::newRow("a_z alone") << QByteArray("a_z") << true;
    QTest::newRow(".z alone") << QByteArray(".z") << true;
    QTest::newRow("_z alone") << QByteArray("_z") << true;
    QTest::newRow("a z alone") << QByteArray("a z") << false;
    QTest::newRow("a\\z alone") << QByteArray("a\\z") << false;
    QTest::newRow("a,z alone") << QByteArray("a,z") << false;
    QTest::newRow("/z alone") << QByteArray("/z") << false;
    QTest::newRow("-z alone") << QByteArray("-z") << false;
#if (defined(Q_OS_ANDROID) || QT_CONFIG(icu)) && !QT_CONFIG(timezone_tzdb)
    QTest::newRow("long alone") << QByteArray("12345678901234567") << true;
    QTest::newRow("over-long alone") << QByteArray("123456789012345678") << false;
#else
    QTest::newRow("long alone") << QByteArray("12345678901234") << true;
    QTest::newRow("over-long alone") << QByteArray("123456789012345") << false;
#endif
}

void tst_QTimeZoneBackend::isValidId()
{
    QFETCH(QByteArray, input);
    QFETCH(bool, valid);

    QCOMPARE(QTimeZonePrivate::isValidId(input), valid);
}

void tst_QTimeZoneBackend::roundtripDisplayNames_data()
{
    QTest::addColumn<QTimeZone>("zone");
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QTimeZone::TimeType>("type");

    constexpr QTimeZone::TimeType types[] = {
        QTimeZone::GenericTime, QTimeZone::StandardTime, QTimeZone::DaylightTime
    };
    const auto typeName = [](QTimeZone::TimeType type) {
        switch (type) {
        case QTimeZone::GenericTime: return "Gen";
        case QTimeZone::StandardTime: return "Std";
        case QTimeZone::DaylightTime: return "DST";
        }
        Q_UNREACHABLE_RETURN("Unrecognised");
    };
    const QList<QByteArray> allList = (QTimeZone::availableTimeZoneIds() << "Vulcan/ShiKahr"_ba);
#ifdef EXHAUSTIVE_ZONE_DISPLAY
    const QList<QByteArray> idList = allList;
#else
    const QList<QByteArray> idList = {
        "Africa/Casablanca"_ba, "Africa/Lagos"_ba, "Africa/Tunis"_ba,
        "America/Caracas"_ba, "America/Coyhaique"_ba,
        "America/Indiana/Tell_City"_ba, "America/Managua"_ba,
        "Asia/Bangkok"_ba, "Asia/Colombo"_ba, "Asia/Tokyo"_ba,
        "Atlantic/Bermuda"_ba, "Atlantic/Faroe"_ba, "Atlantic/Madeira"_ba,
        "Australia/Broken_Hill"_ba, "Australia/NSW"_ba, "Australia/Tasmania"_ba,
        "Brazil/Acre"_ba, "Canada/Atlantic"_ba, "Chile/EasterIsland"_ba,
        "CST6CDT"_ba, "Etc/Greenwich"_ba, "Etc/Universal"_ba,
        "Europe/Guernsey"_ba, "Europe/Kaliningrad"_ba, "Europe/Kyiv"_ba,
        "Europe/Prague"_ba, "Europe/Vatican"_ba,
        "Indian/Comoro"_ba, "Mexico/BajaSur"_ba,
        "Pacific/Bougainville"_ba, "Pacific/Midway"_ba, "Pacific/Wallis"_ba,
        "US/Aleutian"_ba,
        "UTC"_ba,
        // Those named overtly in tst_QDateTime - special cases first:
        "UTC-02:00"_ba, "UTC+02:00"_ba, "UTC+12:00"_ba,
        "Etc/GMT+3"_ba, "GMT-0"_ba, "GMT"_ba,
        // ... then ordinary names in alphabetic order:
        "America/Anchorage"_ba, "America/Metlakatla"_ba, "America/New_York"_ba,
        "America/Sao_Paulo"_ba, "America/Toronto"_ba, "America/Vancouver"_ba,
        "Asia/Kathmandu"_ba, "Asia/Manila"_ba, "Asia/Singapore"_ba,
        "Australia/Brisbane"_ba, "Australia/Eucla"_ba, "Australia/Sydney"_ba,
        "Europe/Berlin"_ba, "Europe/Helsinki"_ba, "Europe/Lisbon"_ba, "Europe/Oslo"_ba,
        "Europe/Rome"_ba,
        "Pacific/Apia"_ba, "Pacific/Auckland"_ba, "Pacific/Kiritimati"_ba,
        "Vulcan/ShiKahr"_ba // Invalid: also worth testing.
    };
    // Some valid zones in that list may be absent from the platform's
    // availableTimeZoneIds(), yet in fact work when used as it's asked to
    // instantiate them (e.g. Etc/Universal on macOS). This can give them a
    // displayName() that we fail to decode, without timezone_locale, due to
    // only trying the availableTimeZoneIds() in findLongNamePrefix(). So we
    // have to filter on membership of allList when creating rows.
#endif // Exhaustive
    const QLocale fr(QLocale::French, QLocale::France);
    const QLocale hi(QLocale::Hindi, QLocale::India);
    const QLocale ff(QLocale::Fulah, QLocale::AdlamScript); // Digits are surrogate pairs.
    for (const QByteArray &id : idList) {
        if (id == "localtime"_ba || id == "posixrules"_ba || !allList.contains(id))
            continue;
        QTimeZone zone = QTimeZone(id);
        if (!zone.isValid())
            continue;
        for (const auto type : types) {
            if (type == QTimeZone::DaylightTime && !zone.hasDaylightTime())
                continue;
            QTest::addRow("%s@fr_FR/%s", id.constData(), typeName(type))
                << zone << fr << type;
            QTest::addRow("%s@hi_IN/%s", id.constData(), typeName(type))
                << zone << hi << type;
            QTest::addRow("%s@ff_Adlm/%s", id.constData(), typeName(type))
                << zone << ff << type;
        }
    }
}

void tst_QTimeZoneBackend::roundtripDisplayNames()
{
    QFETCH(const QTimeZone, zone);
    QFETCH(const QLocale, locale);
    QFETCH(const QTimeZone::TimeType, type);
    static const QDateTime jan = QDateTime(QDate(2015, 1, 1), QTime(12, 0), QTimeZone::UTC);
    static const QDateTime jul = QDateTime(QDate(2015, 7, 1), QTime(12, 0), QTimeZone::UTC);
    const QDateTime dt = zone.isDaylightTime(jul) == (type == QTimeZone::DaylightTime) ? jul : jan;

    // Some zones exercise region format.
    const QString name = zone.displayName(type, QTimeZone::LongName, locale);
    if (!name.isEmpty()) {
        const auto tran = QTimeZonePrivate::extractPrivate(zone)->data(type);
        const qint64 when = tran.atMSecsSinceEpoch == QTimeZonePrivate::invalidMSecs()
            ? dt.toMSecsSinceEpoch() : tran.atMSecsSinceEpoch;
        const QString extended = name + "some spurious cruft"_L1;
        auto match =
            QTimeZonePrivate::findLongNamePrefix(extended, locale, when);
        if (!match)
            match = QTimeZonePrivate::findLongNamePrefix(extended, locale);
        if (!match)
            match = QTimeZonePrivate::findNarrowOffsetPrefix(extended, locale);
        if (!match)
            match = QTimeZonePrivate::findLongUtcPrefix(extended);
        auto report = qScopeGuard([=]() {
            qDebug() << "At" << QDateTime::fromMSecsSinceEpoch(when, QTimeZone::UTC)
                     << "via" << name;
        });
        QCOMPARE(match.nameLength, name.size());
        report.dismiss();
#if 0
        if (match.ianaId != zone.id()) {
            const QTimeZone found = QTimeZone(match.ianaId);
            if (QTimeZonePrivate::extractPrivate(found)->offsetFromUtc(when)
                != QTimeZonePrivate::extractPrivate(zone)->offsetFromUtc(when)) {
                // For DST, some zones haven't done it in ages, so tran may be ancient.
                // Meanwhile, match.ianaId is typically the canonical zone for a metazone.
                // That, in turn, may not have been doing DST when zone was.
                // So we can't rely on a match, but can report the mismatches.
                qDebug() << "Long name" << name << "on"
                         << QTimeZonePrivate::extractPrivate(zone)->offsetFromUtc(when)
                         << "at" << QDateTime::fromMSecsSinceEpoch(when, QTimeZone::UTC)
                         << "got" << match.ianaId << "on"
                         << QTimeZonePrivate::extractPrivate(found)->offsetFromUtc(when);
                // There are also some absurdly over-generic names, that lead to
                // ambiguities, e.g. "heure : West"
            }
        }
#endif // Debug code
    } else if (type != QTimeZone::DaylightTime) { /* Zones with no DST have no DST-name */
        qDebug("Empty display name");
    }
}

static void addOffsetPrefixCase(int mins, QByteArrayView name, const QLocale &loc)
{
    constexpr QTimeZone::TimeType seasons[] = {
        QTimeZone::GenericTime, QTimeZone::StandardTime, QTimeZone::DaylightTime
    };
    const auto typeTag = [](QTimeZone::TimeType type) {
        switch (type) {
        case QTimeZone::GenericTime: return "gen";
        case QTimeZone::StandardTime: return "std";
        case QTimeZone::DaylightTime: return "dst";
        }
        Q_UNREACHABLE_RETURN("<unknown season>");
    };
    const auto flagsFor = [](QTimeZone::TimeType season) {
        using Flag = QtTemporalPattern::TemporalFieldFlag;
        switch (season) {
        case QTimeZone::GenericTime:
            return Flag::Numeric | Flag::GenericTime;
        case QTimeZone::StandardTime:
            return Flag::Numeric | Flag::StandardTime;
        case QTimeZone::DaylightTime:
            return Flag::Numeric | Flag::DaylightSavingTime;
        }
        Q_UNREACHABLE_RETURN(QtTemporalPattern::TemporalFieldFlags{});
    };
    constexpr QtTemporalPattern::TemporalFieldFlags NoFlags = {};
    constexpr auto AllSeasons = QtTemporalPattern::TemporalFieldFlag::GenericTime
        | QtTemporalPattern::TemporalFieldFlag::StandardTime
        | QtTemporalPattern::TemporalFieldFlag::DaylightSavingTime
        | QtTemporalPattern::TemporalFieldFlag::Numeric;
    const char sign = mins < 0 ? '-' : '+';
    const int absMin = qAbs(mins);
    const int hour = absMin / 60;
    const int min = absMin % 60;
    for (const auto type : seasons) {
        QTest::addRow("%s/%c%02d:%02d/%s/only", name.data(),
                      sign, hour, min, typeTag(type))
            << loc << mins << type << flagsFor(type);
        QTest::addRow("%s/%c%02d:%02d/%s/none", name.data(),
                      sign, hour, min, typeTag(type))
            << loc << mins << type << NoFlags;
        QTest::addRow("%s/%c%02d:%02d/%s/all", name.data(),
                      sign, hour, min, typeTag(type))
            << loc << mins << type << AllSeasons;
    }
}

void tst_QTimeZoneBackend::findOffsetPrefix_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<int>("offsetMinutes");
    QTest::addColumn<QTimeZone::TimeType>("season");
    QTest::addColumn<QtTemporalPattern::TemporalFieldFlags>("flags");

#ifdef EXHAUSTIVE_ZONE_DISPLAY
    bool ok = QLocaleData::allLocaleDataRows([](qsizetype, const QLocaleData &item) {
        const QLocale loc(QLocale::Language(item.m_language_id),
                          QLocale::Script(item.m_script_id),
                          QLocale::Territory(item.m_territory_id));
        const QByteArray name = item.id().name();
        // We only consider whole numbers of minutes, as locales usually only
        // support HH:mm offsets.
        for (int mins = QTimeZone::MinUtcOffsetSecs / 60;
             mins <= QTimeZone::MaxUtcOffsetSecs / 60; ++mins) {
            addOffsetPrefixCase(mins, name, loc);
        }
        return true;
    });
    Q_ASSERT(ok); // Function called for each locale always returns true.
#else
    addOffsetPrefixCase(-120, "kl-GL", QLocale(QLocale::Kalaallisut, QLocale::Greenland));
    addOffsetPrefixCase(-30, "en-GB", QLocale(QLocale::English, QLocale::UnitedKingdom));
    addOffsetPrefixCase(0, "en-GB", QLocale(QLocale::English, QLocale::UnitedKingdom));
    addOffsetPrefixCase(30, "en-GB", QLocale(QLocale::English, QLocale::UnitedKingdom));
    addOffsetPrefixCase(-16 * 60, "en", QLocale(QLocale::English));
    addOffsetPrefixCase(-15 * 60, "en", QLocale(QLocale::English));
    addOffsetPrefixCase(-11 * 60, "en", QLocale(QLocale::English));
    addOffsetPrefixCase(-4 * 60, "en", QLocale(QLocale::English));
    addOffsetPrefixCase(15 * 60, "en", QLocale(QLocale::English));
    addOffsetPrefixCase(16 * 60, "en", QLocale(QLocale::English));
    addOffsetPrefixCase(120, "ar-SD", QLocale(QLocale::Arabic, QLocale::Sudan));
    addOffsetPrefixCase(180, "am-ET", QLocale(QLocale::Amharic, QLocale::Ethiopia));
    addOffsetPrefixCase(-30, "eu-ES", QLocale(QLocale::Basque, QLocale::Spain));
    addOffsetPrefixCase(-30, "ee-GH", QLocale(QLocale::Ewe, QLocale::Ghana));
    addOffsetPrefixCase(+30, "ee-TG", QLocale(QLocale::Ewe, QLocale::Togo));
    addOffsetPrefixCase(180, "he-IL", QLocale(QLocale::Hebrew, QLocale::Israel));
    addOffsetPrefixCase(120, "se-FI", QLocale(QLocale::Swedish, QLocale::Finland));
    addOffsetPrefixCase(210, "fa-IR", QLocale(QLocale::Persian, QLocale::Iran));
    addOffsetPrefixCase(270, "fa-AF", QLocale(QLocale::Persian, QLocale::Afghanistan));
    addOffsetPrefixCase(60, "blo-BJ", QLocale(QLocale::Anii, QLocale::Benin));
#endif
}

void tst_QTimeZoneBackend::findOffsetPrefix()
{
    // Round-trip test
    QFETCH(const QLocale, locale);
    QFETCH(const int, offsetMinutes);
    QFETCH(const QTimeZone::TimeType, season);
    QFETCH(const QtTemporalPattern::TemporalFieldFlags, flags);

    const QTimeZone zone(offsetMinutes * 60);
    const QString text = zone.displayName(season, QTimeZone::OffsetName, locale);
    auto parsed = QTimeZonePrivate::findOffsetPrefix(text, locale, flags);
    {
        auto report = qScopeGuard([text]() { qDebug() << "Offset name was:" << text; });
        QVERIFY2(parsed, "Failed to round-trip parsing of offset zone");
        // We may parse std or dst as generic, since fixed-offset zones are
        // typically the same for all three.
        if (parsed.timeType != QTimeZone::GenericTime)
            QCOMPARE(parsed.timeType, season);
        QCOMPARE(parsed.nameLength, text.size());
        QCOMPARE(parsed.ianaId, zone.id());
        report.dismiss();
    }

    {
        QString extended = text + " s0me+dang1ing-cruft"_L1;
        auto report = qScopeGuard([extended]() { qDebug() << "Extended name was:" << extended; });
        parsed = QTimeZonePrivate::findOffsetPrefix(extended, locale, flags);
        QVERIFY2(parsed, "Round-trip parsing of offset zone ran afoul of dangling cruft");
        if (parsed.timeType != QTimeZone::GenericTime)
            QCOMPARE(parsed.timeType, season);
        QCOMPARE(parsed.nameLength, text.size());
        QCOMPARE(parsed.ianaId, zone.id());
        report.dismiss();
    }
}

// Relies on local variable names: zone tzp and locale enUS.
#define ZONE_DNAME_CHECK(type, name, val) \
        QCOMPARE(tzp.displayName(QTimeZone::type, QTimeZone::name, enUS), val);

void tst_QTimeZoneBackend::icuTest()
{
#if QT_CONFIG(icu) && !QT_CONFIG(timezone_tzdb) && (defined(Q_OS_VXWORKS) || defined(Q_OS_OHOS) || !defined(Q_OS_UNIX))
    // Known datetimes
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();

    // Test default constructor
    QIcuTimeZonePrivate tzpd;
    QVERIFY(tzpd.isValid());

    // Test invalid is not available:
    QVERIFY(!tzpd.isTimeZoneIdAvailable("Gondwana/Erewhon"));
    // and construction gives an invalid result:
    QIcuTimeZonePrivate tzpi("Gondwana/Erewhon");
    QCOMPARE(tzpi.isValid(), false);

    // Test named constructor
    QIcuTimeZonePrivate tzp("Europe/Berlin");
    QVERIFY(tzp.isValid());

    // Only test names in debug mode, names used can vary by ICU version installed
    if constexpr (debug) {
        // Test display names by type
        QLocale enUS(u"en_US");
        ZONE_DNAME_CHECK(StandardTime, LongName, u"Central European Standard Time");
        ZONE_DNAME_CHECK(StandardTime, ShortName, u"GMT+01:00");
        ZONE_DNAME_CHECK(StandardTime, OffsetName, u"UTC+01:00");
        ZONE_DNAME_CHECK(DaylightTime, LongName, u"Central European Summer Time");
        ZONE_DNAME_CHECK(DaylightTime, ShortName, u"GMT+02:00");
        ZONE_DNAME_CHECK(DaylightTime, OffsetName, u"UTC+02:00");
        // ICU C api does not support Generic Time yet, C++ api does
        ZONE_DNAME_CHECK(GenericTime, LongName, u"Central European Standard Time");
        ZONE_DNAME_CHECK(GenericTime, ShortName, u"GMT+01:00");
        ZONE_DNAME_CHECK(GenericTime, OffsetName, u"UTC+01:00");

        // Test Abbreviations
        QCOMPARE(tzp.abbreviation(std), u"CET");
        QCOMPARE(tzp.abbreviation(dst), u"CEST");
    }

    testCetPrivate(tzp);
    if (QTest::currentTestFailed())
        return;
    testEpochTranPrivate(QIcuTimeZonePrivate("America/Toronto"));
#endif // ICU without tzdb, on VxWorks or not on Unix
}

void tst_QTimeZoneBackend::tzTest()
{
#if defined(Q_OS_UNIX) && !(QT_CONFIG(timezone_tzdb) || defined(Q_OS_DARWIN) \
                            || defined(Q_OS_ANDROID) || defined(Q_OS_VXWORKS) || defined(Q_OS_OHOS))
    const auto UTC = QTimeZone::UTC;
    // Known datetimes
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0), UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0), UTC).toMSecsSinceEpoch();

    // Test default constructor
    QTzTimeZonePrivate tzpd;
    QVERIFY(tzpd.isValid());

    // Test invalid constructor
    QTzTimeZonePrivate tzpi("Gondwana/Erewhon");
    QVERIFY(!tzpi.isValid());

    // Test named constructor
    QTzTimeZonePrivate tzp("Europe/Berlin");
    QVERIFY(tzp.isValid());

    // Test POSIX-format value for $TZ:
    QTimeZone tzposix("MET-1METDST-2,M3.5.0/02:00:00,M10.5.0/03:00:00");
    QVERIFY(tzposix.isValid());
    QVERIFY(tzposix.hasDaylightTime());

    // Cope with stray space at start of value (QTBUG-135109):
    QTimeZone syd(" AEST-10AEDT,M10.1.0,M4.1.0/3");
    QVERIFY(syd.isValid());
    QVERIFY(syd.hasDaylightTime());

    // RHEL has been seen with this as Africa/Casablanca's POSIX rule:
    QTzTimeZonePrivate permaDst("<+00>0<+01>,0/0,J365/25");
    const QTimeZone utcP1("UTC+01:00"); // Should always have same offset as permaDst
    QVERIFY(permaDst.isValid());
    QVERIFY(permaDst.hasDaylightTime());
    QVERIFY(permaDst.isDaylightTime(QDate(2020, 1, 1).startOfDay(utcP1).toMSecsSinceEpoch()));
    QVERIFY(permaDst.isDaylightTime(QDate(2020, 12, 31).endOfDay(utcP1).toMSecsSinceEpoch()));
    // Note that the final /25 could be misunderstood as putting a fall-back at
    // 1am on the next year's Jan 1st; check we don't do that:
    QVERIFY(permaDst.isDaylightTime(
                QDateTime(QDate(2020, 1, 1), QTime(1, 30), utcP1).toMSecsSinceEpoch()));
    // It shouldn't have any transitions. QTimeZone::hasTransitions() only says
    // whether the backend supports them, so ask for transitions in a wide
    // enough interval that one would show up, if there are any:
    QVERIFY(permaDst.transitions(QDate(2015, 1, 1).startOfDay(UTC).toMSecsSinceEpoch(),
                                 QDate(2020, 1, 1).startOfDay(UTC).toMSecsSinceEpoch()
                                ).isEmpty());

    QTimeZone tzBrazil("BRT+3"); // parts of Northern Brazil, as a POSIX rule
    QVERIFY(tzBrazil.isValid());
    QCOMPARE(tzBrazil.offsetFromUtc(QDateTime(QDate(1111, 11, 11).startOfDay())), -10800);

    // Test display names by type, either ICU or abbreviation only
    QLocale enUS(u"en_US");
    // Only test names in debug mode, names used can vary by ICU version installed
    if constexpr (debug) {
#if QT_CONFIG(icu)
        ZONE_DNAME_CHECK(StandardTime, LongName, "Central European Standard Time");
        ZONE_DNAME_CHECK(StandardTime, ShortName, "GMT+01:00");
        ZONE_DNAME_CHECK(StandardTime, OffsetName, "UTC+01:00");
        ZONE_DNAME_CHECK(DaylightTime, LongName, "Central European Summer Time");
        ZONE_DNAME_CHECK(DaylightTime, ShortName, "GMT+02:00");
        ZONE_DNAME_CHECK(DaylightTime, OffsetName, "UTC+02:00");
        // ICU C api does not support Generic Time yet, C++ api does
        ZONE_DNAME_CHECK(GenericTime, LongName, "Central European Standard Time");
        ZONE_DNAME_CHECK(GenericTime, ShortName, "GMT+01:00");
        ZONE_DNAME_CHECK(GenericTime, OffsetName, "UTC+01:00");
#else
        ZONE_DNAME_CHECK(StandardTime, LongName, "CET");
        ZONE_DNAME_CHECK(StandardTime, ShortName, "CET");
        ZONE_DNAME_CHECK(StandardTime, OffsetName, "CET");
        ZONE_DNAME_CHECK(DaylightTime, LongName, "CEST");
        ZONE_DNAME_CHECK(DaylightTime, ShortName, "CEST");
        ZONE_DNAME_CHECK(DaylightTime, OffsetName, "CEST");
        ZONE_DNAME_CHECK(GenericTime, LongName, "CET");
        ZONE_DNAME_CHECK(GenericTime, ShortName, "CET");
        ZONE_DNAME_CHECK(GenericTime, OffsetName, "CET");
#endif // icu

        // Test Abbreviations
        QCOMPARE(tzp.abbreviation(std), u"CET");
        QCOMPARE(tzp.abbreviation(dst), u"CEST");
    }

    testCetPrivate(tzp);
    if (QTest::currentTestFailed())
        return;
    testEpochTranPrivate(QTzTimeZonePrivate("America/Toronto"));
    if (QTest::currentTestFailed())
        return;

    // Test first and last transition rule
    // Warning: This could vary depending on age of TZ file!

    // Test low date uses first rule found
    constexpr qint64 ancient = -Q_INT64_C(9999999999999);
    // Note: Depending on the OS in question, the database may be carrying the
    // Local Mean Time. which for Berlin is 0:53:28
    QTimeZonePrivate::Data dat = tzp.data(ancient);
    QCOMPARE(dat.atMSecsSinceEpoch, ancient);
    QCOMPARE(dat.daylightTimeOffset, 0);
    if (dat.abbreviation == u"LMT") {
        QCOMPARE(dat.standardTimeOffset, 3208);
    } else {
        QCOMPARE(dat.standardTimeOffset, 3600);

        constexpr qint64 invalidTime = std::numeric_limits<qint64>::min();
        constexpr int invalidOffset = std::numeric_limits<int>::min();
        // Test previous to low value is invalid
        dat = tzp.previousTransition(ancient);
        QCOMPARE(dat.atMSecsSinceEpoch, invalidTime);
        QCOMPARE(dat.standardTimeOffset, invalidOffset);
        QCOMPARE(dat.daylightTimeOffset, invalidOffset);
    }

    dat = tzp.nextTransition(ancient);
    QCOMPARE(QDateTime::fromMSecsSinceEpoch(dat.atMSecsSinceEpoch,
                                            QTimeZone::fromSecondsAheadOfUtc(3600)),
             QDateTime(QDate(1893, 4, 1), QTime(0, 6, 32),
                       QTimeZone::fromSecondsAheadOfUtc(3600)));
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 0);

    // Date-times late enough to exercise POSIX rules:
    qint64 stdHi = QDate(2100, 1, 1).startOfDay(UTC).toMSecsSinceEpoch();
    qint64 dstHi = QDate(2100, 6, 1).startOfDay(UTC).toMSecsSinceEpoch();
    // Relevant last Sundays in October and March:
    QCOMPARE(Qt::DayOfWeek(QDate(2099, 10, 25).dayOfWeek()), Qt::Sunday);
    QCOMPARE(Qt::DayOfWeek(QDate(2100, 3, 28).dayOfWeek()), Qt::Sunday);
    QCOMPARE(Qt::DayOfWeek(QDate(2100, 10, 31).dayOfWeek()), Qt::Sunday);

    dat = tzp.data(stdHi);
    QCOMPARE(dat.atMSecsSinceEpoch - stdHi, qint64(0));
    QCOMPARE(dat.offsetFromUtc, 3600);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 0);

    dat = tzp.data(dstHi);
    QCOMPARE(dat.atMSecsSinceEpoch - dstHi, qint64(0));
    QCOMPARE(dat.offsetFromUtc, 7200);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 3600);

    dat = tzp.previousTransition(stdHi);
    QCOMPARE(dat.abbreviation, u"CET");
    QCOMPARE(QDateTime::fromMSecsSinceEpoch(dat.atMSecsSinceEpoch, UTC),
             QDateTime(QDate(2099, 10, 25), QTime(3, 0), QTimeZone::fromSecondsAheadOfUtc(7200)));
    QCOMPARE(dat.offsetFromUtc, 3600);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 0);

    dat = tzp.previousTransition(dstHi);
    QCOMPARE(dat.abbreviation, u"CEST");
    QCOMPARE(QDateTime::fromMSecsSinceEpoch(dat.atMSecsSinceEpoch, UTC),
             QDateTime(QDate(2100, 3, 28), QTime(2, 0), QTimeZone::fromSecondsAheadOfUtc(3600)));
    QCOMPARE(dat.offsetFromUtc, 7200);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 3600);

    dat = tzp.nextTransition(stdHi);
    QCOMPARE(dat.abbreviation, u"CEST");
    QCOMPARE(QDateTime::fromMSecsSinceEpoch(dat.atMSecsSinceEpoch, UTC),
             QDateTime(QDate(2100, 3, 28), QTime(2, 0), QTimeZone::fromSecondsAheadOfUtc(3600)));
    QCOMPARE(dat.offsetFromUtc, 7200);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 3600);

    dat = tzp.nextTransition(dstHi);
    QCOMPARE(dat.abbreviation, u"CET");
    QCOMPARE(QDateTime::fromMSecsSinceEpoch(dat.atMSecsSinceEpoch,
                                            QTimeZone::fromSecondsAheadOfUtc(3600)),
             QDateTime(QDate(2100, 10, 31), QTime(3, 0), QTimeZone::fromSecondsAheadOfUtc(7200)));
    QCOMPARE(dat.offsetFromUtc, 3600);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 0);

    // Test TZ timezone vs UTC timezone for non-whole-hour negative offset:
    QTzTimeZonePrivate  tztz1("America/Caracas");
    QUtcTimeZonePrivate tzutc1("UTC-04:30");
    QVERIFY(tztz1.isValid());
    QVERIFY(tzutc1.isValid());
    QTzTimeZonePrivate::Data datatz1 = tztz1.data(std);
    QTzTimeZonePrivate::Data datautc1 = tzutc1.data(std);
    QCOMPARE(datatz1.offsetFromUtc, datautc1.offsetFromUtc);

    // Test TZ timezone vs UTC timezone for non-whole-hour positive offset:
    QTzTimeZonePrivate tztz2k("Asia/Kolkata"); // New name
    QTzTimeZonePrivate tztz2c("Asia/Calcutta"); // Legacy name
    // Can't assign QtzTZP, so use a reference; prefer new name.
    QTzTimeZonePrivate &tztz2 = tztz2k.isValid() ? tztz2k : tztz2c;
    QUtcTimeZonePrivate tzutc2("UTC+05:30");
    QVERIFY2(tztz2.isValid(), tztz2.id().constData());
    QVERIFY(tzutc2.isValid());
    QTzTimeZonePrivate::Data datatz2 = tztz2.data(std);
    QTzTimeZonePrivate::Data datautc2 = tzutc2.data(std);
    QCOMPARE(datatz2.offsetFromUtc, datautc2.offsetFromUtc);

    // Test a timezone with an abbreviation that isn't all letters:
    QTzTimeZonePrivate tzBarnaul("Asia/Barnaul");
    if (tzBarnaul.isValid()) {
        QCOMPARE(tzBarnaul.data(std).abbreviation, u"+07");

        // first full day of the new rule (tzdata2016b)
        QDateTime dt(QDate(2016, 3, 28), QTime(0, 0), UTC);
        QCOMPARE(tzBarnaul.data(dt.toMSecsSinceEpoch()).abbreviation, u"+07");
    }
#endif // Unix && !(timezone_tzdb || Darwin || Android || VxWorks || OHOS)
}

void tst_QTimeZoneBackend::macTest()
{
#if defined(Q_OS_DARWIN) && !QT_CONFIG(timezone_tzdb)
    // Known datetimes
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();

    // Test default constructor
    QMacTimeZonePrivate tzpd;
    QVERIFY(tzpd.isValid());

    // Test invalid constructor
    QMacTimeZonePrivate tzpi("Gondwana/Erewhon");
    QCOMPARE(tzpi.isValid(), false);

    // Test named constructor
    QMacTimeZonePrivate tzp("Europe/Berlin");
    QVERIFY(tzp.isValid());

    // Only test names in debug mode, names used can vary by version
    if constexpr (debug) {
        // Test display names by type
        QLocale enUS(u"en_US");
        ZONE_DNAME_CHECK(StandardTime, LongName, "Central European Standard Time");
        ZONE_DNAME_CHECK(StandardTime, ShortName, "GMT+01:00");
        ZONE_DNAME_CHECK(StandardTime, OffsetName, "UTC+01:00");
        ZONE_DNAME_CHECK(DaylightTime, LongName, "Central European Summer Time");
        ZONE_DNAME_CHECK(DaylightTime, ShortName, "GMT+02:00");
        ZONE_DNAME_CHECK(DaylightTime, OffsetName, "UTC+02:00");
        // ICU C api does not support Generic Time yet, C++ api does
        ZONE_DNAME_CHECK(GenericTime, LongName, "Central European Time");
        ZONE_DNAME_CHECK(GenericTime, ShortName, "Germany Time");
        ZONE_DNAME_CHECK(GenericTime, OffsetName, "UTC+01:00");

        // Test Abbreviations
        QCOMPARE(tzp.abbreviation(std), u"CET");
        QCOMPARE(tzp.abbreviation(dst), u"CEST");
    }

    testCetPrivate(tzp);
    if (QTest::currentTestFailed())
        return;
    testEpochTranPrivate(QMacTimeZonePrivate("America/Toronto"));
#endif // Q_OS_DARWIN without tzdb
}

void tst_QTimeZoneBackend::winTest()
{
#if defined(USING_WIN_TZ)
    // Known datetimes
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();

    // Test default constructor
    QWinTimeZonePrivate tzpd;
    if constexpr (debug)
        qDebug() << "System ID = " << tzpd.id()
                 << tzpd.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QLocale())
                 << tzpd.displayName(QTimeZone::GenericTime, QTimeZone::LongName, QLocale());
    QVERIFY(tzpd.isValid());

    // Test invalid constructor
    QWinTimeZonePrivate tzpi("Gondwana/Erewhon");
    QCOMPARE(tzpi.isValid(), false);

    // Test named constructor
    QWinTimeZonePrivate tzp("Europe/Berlin");
    QVERIFY(tzp.isValid());

    // Only test names in debug mode, names used can vary by version
    if constexpr (debug) {
        // Test display names by type
        QLocale enUS(u"en_US");
        ZONE_DNAME_CHECK(StandardTime, LongName, "W. Europe Standard Time");
        ZONE_DNAME_CHECK(StandardTime, ShortName, "W. Europe Standard Time");
        ZONE_DNAME_CHECK(StandardTime, OffsetName, "UTC+01:00");
        ZONE_DNAME_CHECK(DaylightTime, LongName, "W. Europe Daylight Time");
        ZONE_DNAME_CHECK(DaylightTime, ShortName, "W. Europe Daylight Time");
        ZONE_DNAME_CHECK(DaylightTime, OffsetName, "UTC+02:00");
        ZONE_DNAME_CHECK(GenericTime, LongName,
                         "(UTC+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna");
        ZONE_DNAME_CHECK(GenericTime, ShortName,
                         "(UTC+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna");
        ZONE_DNAME_CHECK(GenericTime, OffsetName, "UTC+01:00");

        // Test Abbreviations
        QCOMPARE(tzp.abbreviation(std), u"CET");
        QCOMPARE(tzp.abbreviation(dst), u"CEST");
    }

    testCetPrivate(tzp);
    if (QTest::currentTestFailed())
        return;
    testEpochTranPrivate(QWinTimeZonePrivate("America/Toronto"));
#endif // TZ backend
}

#undef ZONE_DNAME_CHECK

// Test each private produces the same basic results for CET
void tst_QTimeZoneBackend::testCetPrivate(const QTimeZonePrivate &tzp)
{
    // Known datetimes
    const auto UTC = QTimeZone::UTC;
    const auto eastOneHour = QTimeZone::fromSecondsAheadOfUtc(3600);
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0), UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0), UTC).toMSecsSinceEpoch();
    qint64 prev = QDateTime(QDate(2011, 1, 1), QTime(0, 0), UTC).toMSecsSinceEpoch();

    QCOMPARE(tzp.offsetFromUtc(std), 3600);
    QCOMPARE(tzp.offsetFromUtc(dst), 7200);

    QCOMPARE(tzp.standardTimeOffset(std), 3600);
    QCOMPARE(tzp.standardTimeOffset(dst), 3600);

    QCOMPARE(tzp.daylightTimeOffset(std), 0);
    QCOMPARE(tzp.daylightTimeOffset(dst), 3600);

    QCOMPARE(tzp.hasDaylightTime(), true);
    QCOMPARE(tzp.isDaylightTime(std), false);
    QCOMPARE(tzp.isDaylightTime(dst), true);

    QTimeZonePrivate::Data dat = tzp.data(std);
    QCOMPARE(dat.atMSecsSinceEpoch, std);
    QCOMPARE(dat.offsetFromUtc, 3600);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 0);
    QCOMPARE(dat.abbreviation, tzp.abbreviation(std));

    dat = tzp.data(dst);
    QCOMPARE(dat.atMSecsSinceEpoch, dst);
    QCOMPARE(dat.offsetFromUtc, 7200);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 3600);
    QCOMPARE(dat.abbreviation, tzp.abbreviation(dst));

    // Only test transitions if host system supports them
    if (tzp.hasTransitions()) {
        QTimeZonePrivate::Data tran = tzp.nextTransition(std);
        // 2012-03-25 02:00 CET, +1 -> +2
        QCOMPARE(QDateTime::fromMSecsSinceEpoch(tran.atMSecsSinceEpoch, UTC),
                 QDateTime(QDate(2012, 3, 25), QTime(2, 0), eastOneHour));
        QCOMPARE(tran.offsetFromUtc, 7200);
        QCOMPARE(tran.standardTimeOffset, 3600);
        QCOMPARE(tran.daylightTimeOffset, 3600);

        tran = tzp.nextTransition(dst);
        // 2012-10-28 03:00 CEST, +2 -> +1
        QCOMPARE(QDateTime::fromMSecsSinceEpoch(tran.atMSecsSinceEpoch, UTC),
                 QDateTime(QDate(2012, 10, 28), QTime(3, 0),
                           QTimeZone::fromSecondsAheadOfUtc(2 * 3600)));
        QCOMPARE(tran.offsetFromUtc, 3600);
        QCOMPARE(tran.standardTimeOffset, 3600);
        QCOMPARE(tran.daylightTimeOffset, 0);

        tran = tzp.previousTransition(std);
        // 2011-10-30 03:00 CEST, +2 -> +1
        QCOMPARE(QDateTime::fromMSecsSinceEpoch(tran.atMSecsSinceEpoch, UTC),
                 QDateTime(QDate(2011, 10, 30), QTime(3, 0),
                           QTimeZone::fromSecondsAheadOfUtc(2 * 3600)));
        QCOMPARE(tran.offsetFromUtc, 3600);
        QCOMPARE(tran.standardTimeOffset, 3600);
        QCOMPARE(tran.daylightTimeOffset, 0);

        tran = tzp.previousTransition(dst);
        // 2012-03-25 02:00 CET, +1 -> +2 (again)
        QCOMPARE(QDateTime::fromMSecsSinceEpoch(tran.atMSecsSinceEpoch, UTC),
                 QDateTime(QDate(2012, 3, 25), QTime(2, 0), eastOneHour));
        QCOMPARE(tran.offsetFromUtc, 7200);
        QCOMPARE(tran.standardTimeOffset, 3600);
        QCOMPARE(tran.daylightTimeOffset, 3600);

        QTimeZonePrivate::DataList expected;
        // 2011-03-27 02:00 CET, +1 -> +2
        tran.atMSecsSinceEpoch = QDateTime(QDate(2011, 3, 27), QTime(2, 0),
                                           eastOneHour).toMSecsSinceEpoch();
        tran.offsetFromUtc = 7200;
        tran.standardTimeOffset = 3600;
        tran.daylightTimeOffset = 3600;
        expected << tran;
        // 2011-10-30 03:00 CEST, +2 -> +1
        tran.atMSecsSinceEpoch = QDateTime(QDate(2011, 10, 30), QTime(3, 0),
                                           QTimeZone::fromSecondsAheadOfUtc(2 * 3600)
                                           ).toMSecsSinceEpoch();
        tran.offsetFromUtc = 3600;
        tran.standardTimeOffset = 3600;
        tran.daylightTimeOffset = 0;
        expected << tran;
        QTimeZonePrivate::DataList result = tzp.transitions(prev, std);
        QCOMPARE(result.size(), expected.size());
        for (int i = 0; i < expected.size(); ++i) {
            QCOMPARE(QDateTime::fromMSecsSinceEpoch(result.at(i).atMSecsSinceEpoch, eastOneHour),
                     QDateTime::fromMSecsSinceEpoch(expected.at(i).atMSecsSinceEpoch, eastOneHour));
            QCOMPARE(result.at(i).offsetFromUtc, expected.at(i).offsetFromUtc);
            QCOMPARE(result.at(i).standardTimeOffset, expected.at(i).standardTimeOffset);
            QCOMPARE(result.at(i).daylightTimeOffset, expected.at(i).daylightTimeOffset);
        }
    }
}

// Needs a zone with DST around the epoch; currently America/Toronto (EST5EDT)
void tst_QTimeZoneBackend::testEpochTranPrivate(const QTimeZonePrivate &tzp)
{
    if (!tzp.hasTransitions())
        return; // test only viable for transitions

    const auto UTC = QTimeZone::UTC;
    const auto hour = std::chrono::hours{1};
    QTimeZonePrivate::Data tran = tzp.nextTransition(0); // i.e. first after epoch
    // 1970-04-26 02:00 EST, -5 -> -4
    const QDateTime after = QDateTime(QDate(1970, 4, 26), QTime(2, 0),
                                      QTimeZone::fromDurationAheadOfUtc(-5 * hour));
    const QDateTime found = QDateTime::fromMSecsSinceEpoch(tran.atMSecsSinceEpoch, UTC);
#ifdef USING_WIN_TZ // MS gets the date wrong: 5th April instead of 26th.
    QCOMPARE(found.toOffsetFromUtc(-5 * 3600).time(), after.time());
#else
    QCOMPARE(found, after);
#endif
    QCOMPARE(tran.offsetFromUtc, -4 * 3600);
    QCOMPARE(tran.standardTimeOffset, -5 * 3600);
    QCOMPARE(tran.daylightTimeOffset, 3600);

    // Pre-epoch time-zones might not be supported at all:
    tran = tzp.nextTransition(QDateTime(QDate(1601, 1, 1), QTime(0, 0), UTC).toMSecsSinceEpoch());
    if (tran.atMSecsSinceEpoch != QTimeZonePrivate::invalidMSecs()
        // Toronto *did* have a transition before 1970 (DST since 1918):
        && tran.atMSecsSinceEpoch < 0) {
        // ... but, if they are, we should be able to search back to them:
        tran = tzp.previousTransition(0); // i.e. last before epoch
        // 1969-10-26 02:00 EDT, -4 -> -5
        QCOMPARE(QDateTime::fromMSecsSinceEpoch(tran.atMSecsSinceEpoch, UTC),
                 QDateTime(QDate(1969, 10, 26), QTime(2, 0),
                           QTimeZone::fromDurationAheadOfUtc(-4 * hour)));
        QCOMPARE(tran.offsetFromUtc, -5 * 3600);
        QCOMPARE(tran.standardTimeOffset, -5 * 3600);
        QCOMPARE(tran.daylightTimeOffset, 0);
    } else {
        // Do not use QSKIP(): that would discard the rest of this sub-test's caller.
        qDebug() << "No support for pre-epoch time-zone transitions";
    }
}

QTEST_APPLESS_MAIN(tst_QTimeZoneBackend)
#include "tst_qtimezonebackend.moc"
