// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <qtimezone.h>
#if QT_CONFIG(timezone)
#  include <private/qtimezoneprivate_p.h>
#endif
#include <private/qcomparisontesthelper_p.h>
#include <private/qtools_p.h>

#include <qlocale.h>
#include <qscopeguard.h>

#if defined(Q_OS_WIN)
#include <QOperatingSystemVersion>
#endif

#if defined(Q_OS_WIN) && !QT_CONFIG(icu) && !QT_CONFIG(timezone_tzdb)
#  define USING_WIN_TZ
#endif

using namespace Qt::StringLiterals;

class tst_QTimeZone : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Public class default system tests
    void createTest();
    void nullTest();
    void assign();
    void compareCompiles();
    void compare_data();
    void compare();
    void timespec();
    void offset();
    void dataStreamTest();
#if QT_CONFIG(timezone)
    void asBackendZone();
    void systemZone();
    void isTimeZoneIdAvailable();
    void availableTimeZoneIds();
    void utcOffsetId_data();
    void utcOffsetId();
    void hasAlternativeName_data();
    void hasAlternativeName();
    void specificTransition_data();
    void specificTransition();
    void transitionEachZone_data();
    void transitionEachZone();
    void checkOffset_data();
    void checkOffset();
    void stressTest();
    void windowsId();
    void serialize();
    void malformed();
    // Backend tests (see also ../qtimezonebackend/)
    void utcTest();
    void darwinTypes();
    void wasmTypes();
    void localeSpecificDisplayName_data();
    void localeSpecificDisplayName();
    // Compatibility with std::chrono::tzdb:
    void stdCompatibility_data();
    void stdCompatibility();
#endif // timezone backends exist

private:
#if QT_CONFIG(timezone)
    void printTimeZone(const QTimeZone &tz);
    // Where tzdb contains a link between zones in different territories, CLDR
    // doesn't treat those as aliases for one another. For details see "Links in
    // the tz database" at:
    // https://www.unicode.org/reports/tr35/#time-zone-identifiers
    // Some of these could be identified as equivalent by looking at metazone
    // histories but, for now, we stick with CLDR's notion of alias.
    const std::set<QByteArrayView> unAliasedLinks = {
        // By continent:
        "America/Kralendijk", "America/Lower_Princes", "America/Marigot", "America/St_Barthelemy",
        "Antarctica/South_Pole",
        "Arctic/Longyearbyen",
        "Asia/Choibalsan",
        "Atlantic/Jan_Mayen",
        "Europe/Bratislava", "Europe/Busingen", "Europe/Mariehamn",
        "Europe/Podgorica", "Europe/San_Marino", "Europe/Vatican",
        // Assorted legacy abbreviations and POSIX zones:
        "CET", "EET", "EST", "HST", "MET", "MST", "WET",
        "CST6CDT", "EST5EDT", "MST7MDT", "PST8PDT",
    };
#endif // timezone backends
    // Set to true to print debug output, test Display Names and run long stress tests
    static constexpr bool debug = false;
};

#if QT_CONFIG(timezone)
void tst_QTimeZone::printTimeZone(const QTimeZone &tz)
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime jan = QDateTime(QDate(2012, 1, 1), QTime(0, 0), QTimeZone::UTC);
    QDateTime jun = QDateTime(QDate(2012, 6, 1), QTime(0, 0), QTimeZone::UTC);
    qDebug() << "";
    qDebug() << "Time Zone               = " << tz;
    qDebug() << "";
    qDebug() << "Is Valid                = " << tz.isValid();
    qDebug() << "";
    qDebug() << "Zone ID                 = " << tz.id();
    qDebug() << "Territory               = " << QLocale::territoryToString(tz.territory());
    qDebug() << "Comment                 = " << tz.comment();
    qDebug() << "";
    qDebug() << "Locale                  = " << QLocale().name();
    qDebug() << "Name Long               = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::LongName);
    qDebug() << "Name Short              = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::ShortName);
    qDebug() << "Name Offset             = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::OffsetName);
    qDebug() << "Name Long DST           = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::LongName);
    qDebug() << "Name Short DST          = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::ShortName);
    qDebug() << "Name Offset DST         = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::OffsetName);
    qDebug() << "Name Long Generic       = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::LongName);
    qDebug() << "Name Short Generic      = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::ShortName);
    qDebug() << "Name Offset Generic     = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::OffsetName);
    qDebug() << "";
    QLocale locale = QLocale(u"de_DE");
    qDebug() << "Locale                  = " << locale.name();
    qDebug() << "Name Long               = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::LongName, locale);
    qDebug() << "Name Short              = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::ShortName, locale);
    qDebug() << "Name Offset             = " << tz.displayName(QTimeZone::StandardTime, QTimeZone::OffsetName, locale);
    qDebug() << "Name Long DST           = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::LongName,locale);
    qDebug() << "Name Short DST          = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::ShortName, locale);
    qDebug() << "Name Offset DST         = " << tz.displayName(QTimeZone::DaylightTime, QTimeZone::OffsetName, locale);
    qDebug() << "Name Long Generic       = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::LongName, locale);
    qDebug() << "Name Short Generic      = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::ShortName, locale);
    qDebug() << "Name Offset Generic     = " << tz.displayName(QTimeZone::GenericTime, QTimeZone::OffsetName, locale);
    qDebug() << "";
    qDebug() << "Abbreviation Now        = " << tz.abbreviation(now);
    qDebug() << "Abbreviation on 1 Jan   = " << tz.abbreviation(jan);
    qDebug() << "Abbreviation on 1 June  = " << tz.abbreviation(jun);
    qDebug() << "";
    qDebug() << "Offset on 1 January     = " << tz.offsetFromUtc(jan);
    qDebug() << "Offset on 1 June        = " << tz.offsetFromUtc(jun);
    qDebug() << "Offset Now              = " << tz.offsetFromUtc(now);
    qDebug() << "";
    qDebug() << "UTC Offset Now          = " << tz.standardTimeOffset(now);
    qDebug() << "UTC Offset on 1 January = " << tz.standardTimeOffset(jan);
    qDebug() << "UTC Offset on 1 June    = " << tz.standardTimeOffset(jun);
    qDebug() << "";
    qDebug() << "DST Offset on 1 January = " << tz.daylightTimeOffset(jan);
    qDebug() << "DST Offset on 1 June    = " << tz.daylightTimeOffset(jun);
    qDebug() << "DST Offset Now          = " << tz.daylightTimeOffset(now);
    qDebug() << "";
    qDebug() << "Has DST                 = " << tz.hasDaylightTime();
    qDebug() << "Is DST Now              = " << tz.isDaylightTime(now);
    qDebug() << "Is DST on 1 January     = " << tz.isDaylightTime(jan);
    qDebug() << "Is DST on 1 June        = " << tz.isDaylightTime(jun);
    qDebug() << "";
    qDebug() << "Has Transitions         = " << tz.hasTransitions();
    qDebug() << "Transition after 1 Jan  = " << tz.nextTransition(jan).atUtc;
    qDebug() << "Transition after 1 Jun  = " << tz.nextTransition(jun).atUtc;
    qDebug() << "Transition before 1 Jan = " << tz.previousTransition(jan).atUtc;
    qDebug() << "Transition before 1 Jun = " << tz.previousTransition(jun).atUtc;
    qDebug() << "";
}
#endif // feature timezone

void tst_QTimeZone::createTest()
{
#if QT_CONFIG(timezone)
    const QTimeZone tz("Pacific/Auckland");

    if constexpr (debug)
        printTimeZone(tz);

    // If the tz is not valid then skip as is probably using the UTC backend which is tested later
    if (!tz.isValid())
        QSKIP("System lacks zone used for test"); // This returns.

    QCOMPARE(tz.id(), "Pacific/Auckland");
    // Comparison tests:
    const QTimeZone same("Pacific/Auckland");
    QCOMPARE((tz == same), true);
    QCOMPARE((tz != same), false);
    const QTimeZone other("Australia/Sydney");
    QCOMPARE((tz == other), false);
    QCOMPARE((tz != other), true);

    QCOMPARE(tz.territory(), QLocale::NewZealand);

    QDateTime jan = QDateTime(QDate(2012, 1, 1), QTime(0, 0), QTimeZone::UTC);
    QDateTime jun = QDateTime(QDate(2012, 6, 1), QTime(0, 0), QTimeZone::UTC);
    QDateTime janPrev = QDateTime(QDate(2011, 1, 1), QTime(0, 0), QTimeZone::UTC);

    QCOMPARE(tz.offsetFromUtc(jan), 13 * 3600);
    QCOMPARE(tz.offsetFromUtc(jun), 12 * 3600);

    QCOMPARE(tz.standardTimeOffset(jan), 12 * 3600);
    QCOMPARE(tz.standardTimeOffset(jun), 12 * 3600);

    QCOMPARE(tz.daylightTimeOffset(jan), 3600);
    QCOMPARE(tz.daylightTimeOffset(jun), 0);

    QCOMPARE(tz.hasDaylightTime(), true);
    QCOMPARE(tz.isDaylightTime(jan), true);
    QCOMPARE(tz.isDaylightTime(jun), false);

    // Only test transitions if host system supports them
    if (tz.hasTransitions()) {
        QTimeZone::OffsetData tran = tz.nextTransition(jan);
        // 2012-04-01 03:00 NZDT, +13 -> +12
        QCOMPARE(tran.atUtc,
                 QDateTime(QDate(2012, 4, 1), QTime(3, 0),
                           QTimeZone::fromSecondsAheadOfUtc(13 * 3600)));
        QCOMPARE(tran.offsetFromUtc, 12 * 3600);
        QCOMPARE(tran.standardTimeOffset, 12 * 3600);
        QCOMPARE(tran.daylightTimeOffset, 0);

        tran = tz.nextTransition(jun);
        // 2012-09-30 02:00 NZST, +12 -> +13
        QCOMPARE(tran.atUtc,
                 QDateTime(QDate(2012, 9, 30), QTime(2, 0),
                           QTimeZone::fromSecondsAheadOfUtc(12 * 3600)));
        QCOMPARE(tran.offsetFromUtc, 13 * 3600);
        QCOMPARE(tran.standardTimeOffset, 12 * 3600);
        QCOMPARE(tran.daylightTimeOffset, 3600);

        tran = tz.previousTransition(jan);
        // 2011-09-25 02:00 NZST, +12 -> +13
        QCOMPARE(tran.atUtc,
                 QDateTime(QDate(2011, 9, 25), QTime(2, 0),
                           QTimeZone::fromSecondsAheadOfUtc(12 * 3600)));
        QCOMPARE(tran.offsetFromUtc, 13 * 3600);
        QCOMPARE(tran.standardTimeOffset, 12 * 3600);
        QCOMPARE(tran.daylightTimeOffset, 3600);

        tran = tz.previousTransition(jun);
        // 2012-04-01 03:00 NZDT, +13 -> +12 (again)
        QCOMPARE(tran.atUtc,
                 QDateTime(QDate(2012, 4, 1), QTime(3, 0),
                           QTimeZone::fromSecondsAheadOfUtc(13 * 3600)));
        QCOMPARE(tran.offsetFromUtc, 12 * 3600);
        QCOMPARE(tran.standardTimeOffset, 12 * 3600);
        QCOMPARE(tran.daylightTimeOffset, 0);

        QTimeZone::OffsetDataList expected;
        // Reuse 2012's fall-back data for 2011-04-03:
        tran.atUtc = QDateTime(QDate(2011, 4, 3), QTime(3, 0),
                               QTimeZone::fromSecondsAheadOfUtc(13 * 3600));
        expected << tran;
        // 2011's spring-forward:
        tran.atUtc = QDateTime(QDate(2011, 9, 25), QTime(2, 0),
                               QTimeZone::fromSecondsAheadOfUtc(12 * 3600));
        tran.offsetFromUtc = 13 * 3600;
        tran.daylightTimeOffset = 3600;
        expected << tran;
        QTimeZone::OffsetDataList result = tz.transitions(janPrev, jan);
        QCOMPARE(result.size(), expected.size());
        for (int i = 0; i < expected.size(); ++i) {
            QCOMPARE(result.at(i).atUtc, expected.at(i).atUtc);
            QCOMPARE(result.at(i).offsetFromUtc, expected.at(i).offsetFromUtc);
            QCOMPARE(result.at(i).standardTimeOffset, expected.at(i).standardTimeOffset);
            QCOMPARE(result.at(i).daylightTimeOffset, expected.at(i).daylightTimeOffset);
        }
    }
#else
    QSKIP("Test depends on backends, enabled by feature timezone");
#endif // feature timezone
}

void tst_QTimeZone::nullTest()
{
    QTimeZone nullTz1;
    QTimeZone nullTz2;
    QTimeZone utc(QTimeZone::UTC);

    // Validity tests
    QCOMPARE(nullTz1.isValid(), false);
    QCOMPARE(nullTz2.isValid(), false);
    QCOMPARE(utc.isValid(), true);

    // Comparison tests
    QCOMPARE((nullTz1 == nullTz2), true);
    QCOMPARE((nullTz1 != nullTz2), false);
    QCOMPARE((nullTz1 == utc), false);
    QCOMPARE((nullTz1 != utc), true);

    // Assignment tests
    nullTz2 = utc;
    QCOMPARE(nullTz2.isValid(), true);
    utc = nullTz1;
    QCOMPARE(utc.isValid(), false);

#if QT_CONFIG(timezone)
    QCOMPARE(nullTz1.id(), QByteArray());
    QCOMPARE(nullTz1.territory(), QLocale::AnyTerritory);
    QCOMPARE(nullTz1.comment(), QString());

    QDateTime jan = QDateTime(QDate(2012, 1, 1), QTime(0, 0), QTimeZone::UTC);
    QDateTime jun = QDateTime(QDate(2012, 6, 1), QTime(0, 0), QTimeZone::UTC);
    QDateTime janPrev = QDateTime(QDate(2011, 1, 1), QTime(0, 0), QTimeZone::UTC);

    QCOMPARE(nullTz1.abbreviation(jan), QString());
    QCOMPARE(nullTz1.displayName(jan), QString());
    QCOMPARE(nullTz1.displayName(QTimeZone::StandardTime), QString());

    QCOMPARE(nullTz1.offsetFromUtc(jan), 0);
    QCOMPARE(nullTz1.offsetFromUtc(jun), 0);

    QCOMPARE(nullTz1.standardTimeOffset(jan), 0);
    QCOMPARE(nullTz1.standardTimeOffset(jun), 0);

    QCOMPARE(nullTz1.daylightTimeOffset(jan), 0);
    QCOMPARE(nullTz1.daylightTimeOffset(jun), 0);

    QCOMPARE(nullTz1.hasDaylightTime(), false);
    QCOMPARE(nullTz1.isDaylightTime(jan), false);
    QCOMPARE(nullTz1.isDaylightTime(jun), false);

    QTimeZone::OffsetData data = nullTz1.offsetData(jan);
    constexpr auto invalidOffset = std::numeric_limits<int>::min();
    QCOMPARE(data.atUtc, QDateTime());
    QCOMPARE(data.offsetFromUtc, invalidOffset);
    QCOMPARE(data.standardTimeOffset, invalidOffset);
    QCOMPARE(data.daylightTimeOffset, invalidOffset);

    QCOMPARE(nullTz1.hasTransitions(), false);

    data = nullTz1.nextTransition(jan);
    QCOMPARE(data.atUtc, QDateTime());
    QCOMPARE(data.offsetFromUtc, invalidOffset);
    QCOMPARE(data.standardTimeOffset, invalidOffset);
    QCOMPARE(data.daylightTimeOffset, invalidOffset);

    data = nullTz1.previousTransition(jan);
    QCOMPARE(data.atUtc, QDateTime());
    QCOMPARE(data.offsetFromUtc, invalidOffset);
    QCOMPARE(data.standardTimeOffset, invalidOffset);
    QCOMPARE(data.daylightTimeOffset, invalidOffset);
#endif // feature timezone
}

void tst_QTimeZone::assign()
{
    QTimeZone assignee;
    QCOMPARE(assignee.timeSpec(), Qt::TimeZone);
    assignee = QTimeZone();
    QCOMPARE(assignee.timeSpec(), Qt::TimeZone);
    assignee = QTimeZone::UTC;
    QCOMPARE(assignee.timeSpec(), Qt::UTC);
    assignee = QTimeZone::LocalTime;
    QCOMPARE(assignee.timeSpec(), Qt::LocalTime);
    assignee = QTimeZone();
    QCOMPARE(assignee.timeSpec(), Qt::TimeZone);
    assignee = QTimeZone::fromSecondsAheadOfUtc(1);
    QCOMPARE(assignee.timeSpec(), Qt::OffsetFromUTC);
    assignee = QTimeZone::fromSecondsAheadOfUtc(0);
    QCOMPARE(assignee.timeSpec(), Qt::UTC);
#if QT_CONFIG(timezone)
    {
        const QTimeZone cet("Europe/Oslo");
        assignee = cet;
        QCOMPARE(assignee.timeSpec(), Qt::TimeZone);
    }
#endif
}

void tst_QTimeZone::compareCompiles()
{
    QTestPrivate::testEqualityOperatorsCompile<QTimeZone>();
}

void tst_QTimeZone::compare_data()
{
    QTest::addColumn<QTimeZone>("left");
    QTest::addColumn<QTimeZone>("right");
    QTest::addColumn<bool>("expectedEqual");

    const QTimeZone local;
    const QTimeZone utc(QTimeZone::UTC);
    const auto secondEast = QTimeZone::fromSecondsAheadOfUtc(1);
    const auto zeroOffset = QTimeZone::fromSecondsAheadOfUtc(0);
    const auto durationEast = QTimeZone::fromDurationAheadOfUtc(std::chrono::seconds{1});

    QTest::newRow("local vs default-constructed") << local << QTimeZone() << true;
    QTest::newRow("local vs UTC") << local << utc << false;
    QTest::newRow("local vs secondEast") << local << secondEast << false;
    QTest::newRow("secondEast vs UTC") << secondEast << utc << false;
    QTest::newRow("UTC vs zeroOffset") << utc << zeroOffset << true;
    QTest::newRow("secondEast vs durationEast") << secondEast << durationEast << true;
}

void tst_QTimeZone::compare()
{
    QFETCH(QTimeZone, left);
    QFETCH(QTimeZone, right);
    QFETCH(bool, expectedEqual);

    QT_TEST_EQUALITY_OPS(left, right, expectedEqual);
}

void tst_QTimeZone::timespec()
{
    using namespace std::chrono_literals;
    QCOMPARE(QTimeZone().timeSpec(), Qt::TimeZone);
    QCOMPARE(QTimeZone(QTimeZone::UTC).timeSpec(), Qt::UTC);
    QCOMPARE(QTimeZone(QTimeZone::LocalTime).timeSpec(), Qt::LocalTime);
    QCOMPARE(QTimeZone::fromSecondsAheadOfUtc(0).timeSpec(), Qt::UTC);
    QCOMPARE(QTimeZone::fromDurationAheadOfUtc(0s).timeSpec(), Qt::UTC);
    QCOMPARE(QTimeZone::fromDurationAheadOfUtc(0min).timeSpec(), Qt::UTC);
    QCOMPARE(QTimeZone::fromDurationAheadOfUtc(0h).timeSpec(), Qt::UTC);
    QCOMPARE(QTimeZone::fromSecondsAheadOfUtc(1).timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(QTimeZone::fromSecondsAheadOfUtc(-1).timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(QTimeZone::fromSecondsAheadOfUtc(36000).timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(QTimeZone::fromSecondsAheadOfUtc(-36000).timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(QTimeZone::fromDurationAheadOfUtc(3h - 20min +17s).timeSpec(), Qt::OffsetFromUTC);
    {
        const QTimeZone zone;
        QCOMPARE(zone.timeSpec(), Qt::TimeZone);
    }
    {
        const QTimeZone zone = { QTimeZone::UTC };
        QCOMPARE(zone.timeSpec(), Qt::UTC);
    }
    {
        const QTimeZone zone = { QTimeZone::LocalTime };
        QCOMPARE(zone.timeSpec(), Qt::LocalTime);
    }
    {
        const auto zone = QTimeZone::fromSecondsAheadOfUtc(0);
        QCOMPARE(zone.timeSpec(), Qt::UTC);
    }
    {
        const auto zone = QTimeZone::fromDurationAheadOfUtc(0s);
        QCOMPARE(zone.timeSpec(), Qt::UTC);
    }
    {
        const auto zone = QTimeZone::fromSecondsAheadOfUtc(1);
        QCOMPARE(zone.timeSpec(), Qt::OffsetFromUTC);
    }
    {
        const auto zone = QTimeZone::fromDurationAheadOfUtc(1s);
        QCOMPARE(zone.timeSpec(), Qt::OffsetFromUTC);
    }
#if QT_CONFIG(timezone)
    QCOMPARE(QTimeZone("Europe/Oslo").timeSpec(), Qt::TimeZone);
#endif
}

void tst_QTimeZone::offset()
{
    QCOMPARE(QTimeZone().fixedSecondsAheadOfUtc(), 0);
    QCOMPARE(QTimeZone(QTimeZone::UTC).fixedSecondsAheadOfUtc(), 0);
    QCOMPARE(QTimeZone::fromSecondsAheadOfUtc(0).fixedSecondsAheadOfUtc(), 0);
    QCOMPARE(QTimeZone::fromDurationAheadOfUtc(std::chrono::seconds{}).fixedSecondsAheadOfUtc(), 0);
    QCOMPARE(QTimeZone::fromDurationAheadOfUtc(std::chrono::minutes{}).fixedSecondsAheadOfUtc(), 0);
    QCOMPARE(QTimeZone::fromDurationAheadOfUtc(std::chrono::hours{}).fixedSecondsAheadOfUtc(), 0);
    QCOMPARE(QTimeZone::fromSecondsAheadOfUtc(1).fixedSecondsAheadOfUtc(), 1);
    QCOMPARE(QTimeZone::fromSecondsAheadOfUtc(-1).fixedSecondsAheadOfUtc(), -1);
    QCOMPARE(QTimeZone::fromSecondsAheadOfUtc(36000).fixedSecondsAheadOfUtc(), 36000);
    QCOMPARE(QTimeZone::fromSecondsAheadOfUtc(-36000).fixedSecondsAheadOfUtc(), -36000);
    {
        const QTimeZone zone;
        QCOMPARE(zone.fixedSecondsAheadOfUtc(), 0);
    }
    {
        const QTimeZone zone = { QTimeZone::UTC };
        QCOMPARE(zone.fixedSecondsAheadOfUtc(), 0);
    }
    {
        const auto zone = QTimeZone::fromSecondsAheadOfUtc(0);
        QCOMPARE(zone.fixedSecondsAheadOfUtc(), 0);
    }
    {
        const auto zone = QTimeZone::fromDurationAheadOfUtc(std::chrono::seconds{});
        QCOMPARE(zone.fixedSecondsAheadOfUtc(), 0);
    }
    {
        const auto zone = QTimeZone::fromSecondsAheadOfUtc(1);
        QCOMPARE(zone.fixedSecondsAheadOfUtc(), 1);
    }
    {
        const auto zone = QTimeZone::fromDurationAheadOfUtc(std::chrono::seconds{1});
        QCOMPARE(zone.fixedSecondsAheadOfUtc(), 1);
    }
#if QT_CONFIG(timezone)
    QCOMPARE(QTimeZone("Europe/Oslo").fixedSecondsAheadOfUtc(), 0);
#endif
}

void tst_QTimeZone::dataStreamTest()
{
#if QT_CONFIG(timezone) && !defined(QT_NO_DATASTREAM)
    // Test the OffsetFromUtc backend serialization. First with a custom timezone:
    QTimeZone tz1("QST"_ba, 23456,
                  u"Qt Standard Time"_s, u"QST"_s, QLocale::Norway, u"Qt Testing"_s);
    QByteArray tmp;
    {
        QDataStream ds(&tmp, QIODevice::WriteOnly);
        ds << tz1;
        QCOMPARE(ds.status(), QDataStream::Ok);
    }
    QTimeZone tz2("UTC-12:00"); // Shall be over-written.
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> tz2;
        QCOMPARE(ds.status(), QDataStream::Ok);
    }
    QCOMPARE(tz2.id(), "QST"_ba);
    QCOMPARE(tz2.comment(), u"Qt Testing"_s);
    QCOMPARE(tz2.territory(), QLocale::Norway);
    QCOMPARE(tz2.abbreviation(QDateTime::currentDateTime()), u"QST"_s);
    QCOMPARE(tz2.displayName(QTimeZone::StandardTime, QTimeZone::LongName), u"Qt Standard Time"_s);
    QCOMPARE(tz2.displayName(QTimeZone::DaylightTime, QTimeZone::LongName), u"Qt Standard Time"_s);
    QCOMPARE(tz2.offsetFromUtc(QDateTime::currentDateTime()), 23456);

    // And then with a standard IANA timezone (QTBUG-60595):
    tz1 = QTimeZone("UTC");
    QCOMPARE(tz1.isValid(), true);
    {
        QDataStream ds(&tmp, QIODevice::WriteOnly);
        ds << tz1;
        QCOMPARE(ds.status(), QDataStream::Ok);
    }
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> tz2;
        QCOMPARE(ds.status(), QDataStream::Ok);
    }
    QCOMPARE(tz2.isValid(), true);
    QCOMPARE(tz2.id(), tz1.id());

    // Test the system backend serialization
    tz1 = QTimeZone("Pacific/Auckland");

    // If not valid then probably using the UTC system backend so skip
    if (!tz1.isValid())
        return;

    {
        QDataStream ds(&tmp, QIODevice::WriteOnly);
        ds << tz1;
        QCOMPARE(ds.status(), QDataStream::Ok);
    }
    tz2 = QTimeZone("UTC");
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> tz2;
        QCOMPARE(ds.status(), QDataStream::Ok);
    }
    QCOMPARE(tz2.id(), tz1.id());
#endif // feature timezone and enabled datastream
}

#if QT_CONFIG(timezone)
void tst_QTimeZone::asBackendZone()
{
    QCOMPARE(QTimeZone(QTimeZone::LocalTime).asBackendZone(), QTimeZone::systemTimeZone());
    QCOMPARE(QTimeZone(QTimeZone::UTC).asBackendZone(), QTimeZone::utc());
    QCOMPARE(QTimeZone::fromSecondsAheadOfUtc(-300).asBackendZone(), QTimeZone(-300));
    QTimeZone cet("Europe/Oslo");
    QCOMPARE(cet.asBackendZone(), cet);
}

void tst_QTimeZone::systemZone()
{
    const QTimeZone zone = QTimeZone::systemTimeZone();
    QVERIFY2(zone.isValid(),
             "Invalid system zone setting, tests are doomed on misconfigured system.");
    // This may fail on Windows if CLDR data doesn't map system MS ID to IANA ID:
    QCOMPARE(zone.id(), QTimeZone::systemTimeZoneId());
    QCOMPARE(zone, QTimeZone(QTimeZone::systemTimeZoneId()));
    // Check it behaves the same as local-time:
    const QDate dates[] = {
        QDate::fromJulianDay(0), // far in the distant past (LMT)
        QDate(1625, 6, 8), // Before time-zones (date of Cassini's birth)
        QDate(1901, 12, 13), // Last day before 32-bit time_t's range
        QDate(1969, 12, 31), // Last day before the epoch
        QDate(1970, 0, 0), // Start of epoch
        QDate(2000, 2, 29), // An anomalous leap day
        QDate(2038, 1, 20) // First day after 32-bit time_t's range
    };
    for (const auto &date : dates)
        QCOMPARE(date.startOfDay(QTimeZone::LocalTime), date.startOfDay(zone));

#if __cpp_lib_chrono >= 201907L
    const std::chrono::time_zone *currentTimeZone = std::chrono::current_zone();
    QCOMPARE(QByteArrayView(currentTimeZone->name()), QByteArrayView(zone.id()));
#endif
}

void tst_QTimeZone::isTimeZoneIdAvailable()
{
    const QList<QByteArray> available = QTimeZone::availableTimeZoneIds();
    for (const QByteArray &id : available) {
        QVERIFY2(QTimeZone::isTimeZoneIdAvailable(id), id);
        const QTimeZone zone(id);
        QVERIFY2(zone.isValid(), id);
        if (unAliasedLinks.find(id) == unAliasedLinks.end())
            QVERIFY2(zone.hasAlternativeName(id), zone.id() + " != " + id);
    }
    // availableTimeZoneIds() doesn't list all possible offset IDs, but
    // isTimeZoneIdAvailable() should accept them.
    for (qint32 offset = QTimeZone::MinUtcOffsetSecs;
         offset <= QTimeZone::MinUtcOffsetSecs; ++offset) {
        const QByteArray id = QTimeZone(offset).id();
        QVERIFY2(QTimeZone::isTimeZoneIdAvailable(id), id);
        QVERIFY2(QTimeZone(id).isValid(), id);
        QCOMPARE(QTimeZone(id).id(), id);
    }
}

void tst_QTimeZone::utcOffsetId_data()
{
    QTest::addColumn<QByteArray>("id");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<int>("offset"); // ignored unless valid

    // Some of these are actual CLDR zone IDs, some are known Windows IDs; the
    // rest rely on parsing the offset. Since CLDR and Windows may add to their
    // known IDs, which fall in which category may vary. Only the CLDR and
    // Windows ones are known to isTimeZoneAvailable() or listed in
    // availableTimeZoneIds().
#define ROW(name, valid, offset) \
    QTest::newRow(name) << QByteArray(name) << valid << offset

    // See qtbase/util/locale_database/zonedata.py for source
    // CLDR v35.1 IDs:
    ROW("UTC", true, 0);
    ROW("UTC-14:00", true, -50400);
    ROW("UTC-13:00", true, -46800);
    ROW("UTC-12:00", true, -43200);
    ROW("UTC-11:00", true, -39600);
    ROW("UTC-10:00", true, -36000);
    ROW("UTC-09:00", true, -32400);
    ROW("UTC-08:00", true, -28800);
    ROW("UTC-07:00", true, -25200);
    ROW("UTC-06:00", true, -21600);
    ROW("UTC-05:00", true, -18000);
    ROW("UTC-04:30", true, -16200);
    ROW("UTC-04:00", true, -14400);
    ROW("UTC-03:30", true, -12600);
    ROW("UTC-03:00", true, -10800);
    ROW("UTC-02:00", true, -7200);
    ROW("UTC-01:00", true, -3600);
    ROW("UTC-00:00", true, 0);
    ROW("UTC+00:00", true, 0);
    ROW("UTC+01:00", true, 3600);
    ROW("UTC+02:00", true, 7200);
    ROW("UTC+03:00", true, 10800);
    ROW("UTC+03:30", true, 12600);
    ROW("UTC+04:00", true, 14400);
    ROW("UTC+04:30", true, 16200);
    ROW("UTC+05:00", true, 18000);
    ROW("UTC+05:30", true, 19800);
    ROW("UTC+05:45", true, 20700);
    ROW("UTC+06:00", true, 21600);
    ROW("UTC+06:30", true, 23400);
    ROW("UTC+07:00", true, 25200);
    ROW("UTC+08:00", true, 28800);
    ROW("UTC+08:30", true, 30600);
    ROW("UTC+09:00", true, 32400);
    ROW("UTC+09:30", true, 34200);
    ROW("UTC+10:00", true, 36000);
    ROW("UTC+11:00", true, 39600);
    ROW("UTC+12:00", true, 43200);
    ROW("UTC+13:00", true, 46800);
    ROW("UTC+14:00", true, 50400);

    // Windows IDs known to CLDR v35.1:
    ROW("UTC-11", true, -39600);
    ROW("UTC-09", true, -32400);
    ROW("UTC-08", true, -28800);
    ROW("UTC-8", true, -28800);
    ROW("UTC-02", true, -7200);
    ROW("UTC+2", true, 7200);
    ROW("UTC+12", true, 43200);
    ROW("UTC+13", true, 46800);
    // Encountered in bug reports:
    ROW("UTC+10", true, 36000); // QTBUG-77738

    // Minutes, if present, require two digits, but hour doesn't:
    ROW("UTC+2:5", false, 0);
    ROW("UTC+02:5", false, 0);
    ROW("UTC+2:05", true, 7500);
    ROW("UTC+02:05", true, 7500);
    ROW("UTC-2:5", false, 0);
    ROW("UTC-02:5", false, 0);
    ROW("UTC-2:05", true, -7500);
    ROW("UTC-02:05", true, -7500);

    // Bounds:
    ROW("UTC+23", true, 82800);
    ROW("UTC-23", true, -82800);
    ROW("UTC+23:59", true, 86340);
    ROW("UTC-23:59", true, -86340);
    ROW("UTC+23:59:59", true, 86399);
    ROW("UTC-23:59:59", true, -86399);

    // Out of range
    ROW("UTC+24:0:0", false, 0);
    ROW("UTC-24:0:0", false, 0);
    ROW("UTC+0:60:0", false, 0);
    ROW("UTC-0:60:0", false, 0);
    ROW("UTC+0:0:60", false, 0);
    ROW("UTC-0:0:60", false, 0);

    // Malformed
    ROW("UTC+", false, 0);
    ROW("UTC-", false, 0);
    ROW("UTC10", false, 0);
    ROW("UTC:10", false, 0);
    ROW("UTC+cabbage", false, 0);
    ROW("UTC+10:rice", false, 0);
    ROW("UTC+9:3:oat", false, 0);
    ROW("UTC+9+3", false, 0);
    ROW("UTC+9-3", false, 0);
    ROW("UTC+9:3-4", false, 0);
    ROW("UTC+9:3:4:more", false, 0);
    ROW("UTC+9:3:4:5", false, 0);
}

void tst_QTimeZone::utcOffsetId()
{
    QFETCH(QByteArray, id);
    QFETCH(bool, valid);
    QTimeZone zone(id);
    QCOMPARE(zone.isValid(), valid);
    if (valid) {
        QDateTime epoch(QDate(1970, 1, 1), QTime(0, 0), QTimeZone::UTC);
        QFETCH(int, offset);
        QCOMPARE(zone.offsetFromUtc(epoch), offset);
        QVERIFY(!zone.hasDaylightTime());

        if (zone.id() != id && zone.id().contains(':')) {
            // If the backend handles the offset-form, it uses the ID passed to
            // QTimeZone as its id(). Otherwise, it's picked up by the QTZ
            // constructor's final fallback, which constructs a UTC-backend from
            // the parsed offset, which canonicalises the ID. That'll include a
            // zero minutes field if the original was offset by a whole number
            // of hours from UTC. It'll also zero-pad a single-digit hour to two
            // digits. (Single-digit minutes are not valid.)
            qsizetype cut = id.indexOf(':');
            if (cut < 0) {
                cut = id.size();
                id += ":00";
            }
            if (cut == 5)
                id.insert(4, '0');
            // else: the discrepancy makes no sense, let the QCOMPARE() fail.
        }

        QCOMPARE(zone.id(), id);
    }
}

void tst_QTimeZone::hasAlternativeName_data()
{
    QTest::addColumn<QByteArray>("iana");
    QTest::addColumn<QByteArray>("alias");

    QTest::newRow("Montreal=Toronto") << "America/Toronto"_ba << "America/Montreal"_ba;
    QTest::newRow("Asmera=Asmara") << "Africa/Asmara"_ba << "Africa/Asmera"_ba;
    QTest::newRow("Argentina/Catamarca")
        << "America/Argentina/Catamarca"_ba << "America/Catamarca"_ba;
    QTest::newRow("Godthab=Nuuk") << "America/Nuuk"_ba << "America/Godthab"_ba;
    QTest::newRow("Indiana/Indianapolis")
        << "America/Indiana/Indianapolis"_ba << "America/Indianapolis"_ba;
    QTest::newRow("Kentucky/Louisville")
        << "America/Kentucky/Louisville"_ba << "America/Louisville"_ba;
    QTest::newRow("Calcutta=Kolkata") << "Asia/Kolkata"_ba << "Asia/Calcutta"_ba;
    QTest::newRow("Katmandu=Kathmandu") << "Asia/Kathmandu"_ba << "Asia/Katmandu"_ba;
    QTest::newRow("Rangoon=Yangon") << "Asia/Yangon"_ba << "Asia/Rangoon"_ba;
    QTest::newRow("Saigon=Ho_Chi_Minh") << "Asia/Ho_Chi_Minh"_ba << "Asia/Saigon"_ba;
    QTest::newRow("Faeroe=Faroe") << "Atlantic/Faroe"_ba << "Atlantic/Faeroe"_ba;
    QTest::newRow("Currie=Hobart") << "Australia/Hobart"_ba << "Australia/Currie"_ba;
    QTest::newRow("Kiev=Kyiv") << "Europe/Kyiv"_ba << "Europe/Kiev"_ba;
    QTest::newRow("Uzhgorod=Kyiv") << "Europe/Kyiv"_ba << "Europe/Uzhgorod"_ba;
    QTest::newRow("Zaporozhye=Kyiv") << "Europe/Kyiv"_ba << "Europe/Zaporozhye"_ba;
    QTest::newRow("Fiji=Fiji") << "Pacific/Fiji"_ba << "Pacific/Fiji"_ba;
    QTest::newRow("Enderbury=Enderbury") << "Pacific/Enderbury"_ba << "Pacific/Enderbury"_ba;
}

void tst_QTimeZone::hasAlternativeName()
{
    QFETCH(const QByteArray, iana);
    QFETCH(const QByteArray, alias);
    const QTimeZone zone(iana);
    const QTimeZone peer(alias);
    if (!zone.isValid())
        QSKIP("Backend doesn't support IANA ID");

    auto report = qScopeGuard([zone, peer]() {
        const QByteArray zid = zone.id(), pid = peer.id();
        qDebug("Using %s and %s", zid.constData(), pid.constData());
    });
    QVERIFY2(peer.isValid(), "Construction should have fallen back on IANA ID");
    QVERIFY(zone.hasAlternativeName(zone.id()));
    QVERIFY(zone.hasAlternativeName(iana));
    QVERIFY(peer.hasAlternativeName(peer.id()));
    QVERIFY(peer.hasAlternativeName(alias));
    QVERIFY(zone.hasAlternativeName(peer.id()));
    QVERIFY(zone.hasAlternativeName(alias));
    QVERIFY(peer.hasAlternativeName(zone.id()));
    QVERIFY(peer.hasAlternativeName(iana));
    report.dismiss();
}

void tst_QTimeZone::specificTransition_data()
{
#if QT_CONFIG(timezone_tzdb) && defined(__GLIBCXX__)
    QSKIP("libstdc++'s C++20 misreads the IANA DB for Moscow's transitions (among others).");
#endif
#if defined Q_OS_ANDROID && !QT_CONFIG(timezone_tzdb)
    if (!QTimeZone("Europe/Moscow").hasTransitions())
        QSKIP("Android time-zone back-end has no transition data");
#endif
    QTest::addColumn<QByteArray>("zone");
    QTest::addColumn<QDate>("start");
    QTest::addColumn<QDate>("stop");
    QTest::addColumn<int>("count");
    QTest::addColumn<QDateTime>("atUtc");
    // In seconds:
    QTest::addColumn<int>("offset");
    QTest::addColumn<int>("stdoff");
    QTest::addColumn<int>("dstoff");

    // Moscow ditched DST on 2010-10-31 but has since changed standard offset twice.
#ifdef USING_WIN_TZ
    // Win7 is too old to know about this transition:
    if (QOperatingSystemVersion::current() > QOperatingSystemVersion::Windows7)
#endif
    {
        QTest::newRow("Moscow/2014") // From original bug-report
            << QByteArray("Europe/Moscow")
            << QDate(2011, 4, 1) << QDate(2021, 12, 31) << 1
            << QDateTime(QDate(2014, 10, 26), QTime(2, 0),
                         QTimeZone::fromSecondsAheadOfUtc(4 * 3600)).toUTC()
            << 3 * 3600 << 3 * 3600 << 0;
    }
    QTest::newRow("Moscow/2011") // Transition on 2011-03-27
        << QByteArray("Europe/Moscow")
        << QDate(2010, 11, 1) << QDate(2014, 10, 25) << 1
        << QDateTime(QDate(2011, 3, 27), QTime(2, 0),
                     QTimeZone::fromSecondsAheadOfUtc(3 * 3600)).toUTC()
        << 4 * 3600 << 4 * 3600 << 0;
}

void tst_QTimeZone::specificTransition()
{
    // Regression test for QTBUG-42021 (on MS-Win)
    QFETCH(QByteArray, zone);
    QFETCH(QDate, start);
    QFETCH(QDate, stop);
    QFETCH(int, count);
    // No attempt to check abbreviations; to much cross-platform variation.
    QFETCH(QDateTime, atUtc);
    QFETCH(int, offset);
    QFETCH(int, stdoff);
    QFETCH(int, dstoff);

    QTimeZone timeZone(zone);
    if (!timeZone.isValid())
        QSKIP("Missing time-zone data");
    QTimeZone::OffsetDataList transits =
        timeZone.transitions(start.startOfDay(timeZone), stop.endOfDay(timeZone));
    QCOMPARE(transits.size(), count);
    if (count) {
        const QTimeZone::OffsetData &transition = transits.at(0);
        QCOMPARE(transition.offsetFromUtc, offset);
        QCOMPARE(transition.standardTimeOffset, stdoff);
        QCOMPARE(transition.daylightTimeOffset, dstoff);
        QCOMPARE(transition.atUtc, atUtc);
    }
}

void tst_QTimeZone::transitionEachZone_data()
{
    QTest::addColumn<QByteArray>("zone");
    QTest::addColumn<qint64>("secs");
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("stop");

    const struct {
        qint64 baseSecs;
        int start, stop;
        int year;
    } table[] = {
        { 1288488600, -4, 8, 2010 }, // 2010-10-31 01:30 UTC; Europe, Russia
        { 25666200, 3, 12, 1970 },   // 1970-10-25 01:30 UTC; North America
    };

    const auto zones = QTimeZone::availableTimeZoneIds();
    for (const auto &entry : table) {
        for (const QByteArray &zone : zones) {
            QTest::addRow("%s@%d", zone.constData(), entry.year)
                << zone << entry.baseSecs << entry.start << entry.stop;
        }
    }
}

void tst_QTimeZone::transitionEachZone()
{
    // Regression test: round-trip fromMsecs/toMSecs should be idempotent; but
    // various zones failed during fall-back transitions.
    QFETCH(const QByteArray, zone);
    QFETCH(const qint64, secs);
    QFETCH(const int, start);
    QFETCH(const int, stop);
    const QTimeZone named(zone);
    if (!named.isValid())
        QSKIP("Supposedly available zone is not valid");
    if (named.id() != zone)
        QSKIP("Supposedly available zone's id does not match");

    for (int i = start; i < stop; i++) {
#ifdef USING_WIN_TZ
        // See QTBUG-64985: MS's TZ APIs' misdescription of Europe/Samara leads
        // to mis-disambiguation of its fall-back here.
        if (zone == "Europe/Samara" && i == -3)
            continue;
#endif
        const qint64 here = secs + i * 3600;
        const QDateTime when = QDateTime::fromSecsSinceEpoch(here, named);
        const qint64 stamp = when.toMSecsSinceEpoch();
        if (here * 1000 != stamp) {
            // (The +1 is due to using _1_:30 as baseSecs.)
            qDebug("Failing at half past %d UTC (offset %d in %s)", i + 1, when.offsetFromUtc(),
                   QLocale::territoryToString(named.territory()).toUtf8().constData());
        }
        QCOMPARE(stamp % 1000, 0);
        QCOMPARE(here - stamp / 1000, 0);
    }
}

void tst_QTimeZone::checkOffset_data()
{
    QTest::addColumn<QTimeZone>("zone");
    QTest::addColumn<QDateTime>("when");
    QTest::addColumn<int>("netOffset");
    QTest::addColumn<int>("stdOffset");
    QTest::addColumn<int>("dstOffset");

    const QTimeZone UTC = QTimeZone::UTC;
    QTest::addRow("UTC")
        << UTC << QDate(1970, 1, 1).startOfDay(UTC) << 0 << 0 << 0;
    const auto east = QTimeZone::fromSecondsAheadOfUtc(28'800); // 8 hours
    QTest::addRow("UTC+8")
        << east << QDate(2000, 2, 29).startOfDay(east) << 28'800 << 28'800 << 0;
    const auto west = QTimeZone::fromDurationAheadOfUtc(std::chrono::hours{-8});
    QTest::addRow("UTC-8")
        << west << QDate(2100, 2, 28).startOfDay(west) << -28'800 << -28'800 << 0;

    struct {
        const char *zone, *nick;
        int year, month, day, hour, min, sec;
        int std, dst;
    } table[] = {
        // Exercise the UTC-backend:
        { "UTC", "epoch", 1970, 1, 1, 0, 0, 0, 0, 0 },
        // Zone with no transitions (QTBUG-74614, QTBUG-74666, when TZ backend uses minimal data)
        { "Etc/UTC", "epoch", 1970, 1, 1, 0, 0, 0, 0, 0 },
        { "Etc/UTC", "pre_int32", 1901, 12, 13, 20, 45, 51, 0, 0 },
        { "Etc/UTC", "post_int32", 2038, 1, 19, 3, 14, 9, 0, 0 },
        { "Etc/UTC", "post_uint32", 2106, 2, 7, 6, 28, 17, 0, 0 },
        { "Etc/UTC", "initial", -292275056, 5, 16, 16, 47, 5, 0, 0 },
        { "Etc/UTC", "final", 292278994, 8, 17, 7, 12, 55, 0, 0 },
        // Kyiv: regression test for QTBUG-64122 (on MS):
        { "Europe/Kyiv", "summer", 2017, 10, 27, 12, 0, 0, 2 * 3600, 3600 },
        { "Europe/Kyiv", "winter", 2017, 10, 29, 12, 0, 0, 2 * 3600, 0 }
    };
    for (const auto &entry : table) {
        QTimeZone zone(entry.zone);
        if (zone.isValid()) {
            QTest::addRow("%s@%s", entry.zone, entry.nick)
                << zone
                << QDateTime(QDate(entry.year, entry.month, entry.day),
                             QTime(entry.hour, entry.min, entry.sec), zone)
                << entry.dst + entry.std << entry.std << entry.dst;
        } else {
            qWarning("Skipping %s@%s test as zone is invalid", entry.zone, entry.nick);
        }
    }
}

void tst_QTimeZone::checkOffset()
{
    QFETCH(QTimeZone, zone);
    QFETCH(QDateTime, when);
    QFETCH(int, netOffset);
    QFETCH(int, stdOffset);
    QFETCH(int, dstOffset);

    QVERIFY(zone.isValid()); // It was when _data() added the row !
    QCOMPARE(zone.offsetFromUtc(when), netOffset);
    QCOMPARE(zone.standardTimeOffset(when), stdOffset);
    QCOMPARE(zone.daylightTimeOffset(when), dstOffset);
    QCOMPARE(zone.isDaylightTime(when), dstOffset != 0);

    // Also test offsetData(), which gets all this data in one go:
    const auto data = zone.offsetData(when);
    QCOMPARE(data.atUtc, when);
    QCOMPARE(data.offsetFromUtc, netOffset);
    QCOMPARE(data.standardTimeOffset, stdOffset);
    QCOMPARE(data.daylightTimeOffset, dstOffset);
}

void tst_QTimeZone::availableTimeZoneIds()
{
    if constexpr (debug) {
        qDebug() << "";
        qDebug() << "Available Time Zones" ;
        qDebug() << QTimeZone::availableTimeZoneIds();
        qDebug() << "";
        qDebug() << "Available Time Zones in the US";
        qDebug() << QTimeZone::availableTimeZoneIds(QLocale::UnitedStates);
        qDebug() << "";
        qDebug() << "Available Time Zones with UTC Offset 0";
        qDebug() << QTimeZone::availableTimeZoneIds(0);
        qDebug() << "";
    } else {
        // Test the calls work:
        QList<QByteArray> listAll = QTimeZone::availableTimeZoneIds();
        QList<QByteArray> list001 = QTimeZone::availableTimeZoneIds(QLocale::World);
        QList<QByteArray> listUsa = QTimeZone::availableTimeZoneIds(QLocale::UnitedStates);
        QList<QByteArray> listGmt = QTimeZone::availableTimeZoneIds(0);
        // We cannot know what any test machine has available, so can't test contents.
        // But we can do a consistency check:
        QCOMPARE_LT(list001.size(), listAll.size());
        QCOMPARE_LT(listUsa.size(), listAll.size());
        QCOMPARE_LT(listGmt.size(), listAll.size());
        // And we do know CLDR data supplies some entries to each:
        QCOMPARE_GT(listAll.size(), 0);
        QCOMPARE_GT(list001.size(), 0);
        QCOMPARE_GT(listUsa.size(), 0);
        QCOMPARE_GT(listGmt.size(), 0);
    }
}

void tst_QTimeZone::stressTest()
{
    constexpr auto UTC = QTimeZone::UTC;
    const QList<QByteArray> idList = QTimeZone::availableTimeZoneIds();
    for (const QByteArray &id : idList) {
        QTimeZone testZone = QTimeZone(id);
        QCOMPARE(testZone.isValid(), true);
        if (unAliasedLinks.find(id) == unAliasedLinks.end())
            QVERIFY2(testZone.hasAlternativeName(id), testZone.id() + " != " + id);
        QDateTime testDate = QDateTime(QDate(2015, 1, 1), QTime(0, 0), UTC);
        testZone.territory();
        testZone.comment();
        testZone.displayName(testDate);
        (void)testZone.displayName(QTimeZone::GenericTime);
        (void)testZone.displayName(QTimeZone::StandardTime);
        (void)testZone.displayName(QTimeZone::DaylightTime);
        testZone.abbreviation(testDate);
        testZone.offsetFromUtc(testDate);
        testZone.standardTimeOffset(testDate);
        testZone.daylightTimeOffset(testDate);
        testZone.hasDaylightTime();
        testZone.isDaylightTime(testDate);
        testZone.offsetData(testDate);
        testZone.hasTransitions();
        testZone.nextTransition(testDate);
        testZone.previousTransition(testDate);
        // Dates known to be outside possible tz file pre-calculated rules range
        QDateTime lowDate1 = QDateTime(QDate(1800, 1, 1), QTime(0, 0), UTC);
        QDateTime lowDate2 = QDateTime(QDate(1800, 6, 1), QTime(0, 0), UTC);
        QDateTime highDate1 = QDateTime(QDate(2200, 1, 1), QTime(0, 0), UTC);
        QDateTime highDate2 = QDateTime(QDate(2200, 6, 1), QTime(0, 0), UTC);
        testZone.nextTransition(lowDate1);
        testZone.nextTransition(lowDate2);
        testZone.previousTransition(lowDate2);
        testZone.previousTransition(lowDate2);
        testZone.nextTransition(highDate1);
        testZone.nextTransition(highDate2);
        testZone.previousTransition(highDate1);
        testZone.previousTransition(highDate2);
        if constexpr (debug) {
            // This could take a long time, depending on platform and database
            qDebug() << "Stress test calculating transistions for" << testZone.id();
            testZone.transitions(lowDate1, highDate1);
        }
        testDate.setTimeZone(testZone);
        testDate.isValid();
        testDate.offsetFromUtc();
        testDate.timeZoneAbbreviation();
    }
}

void tst_QTimeZone::windowsId()
{
/*
    Current Windows zones for "Central Standard Time":
    Region      IANA Id(s)
    World       "America/Chicago" (the default)
    Canada      "America/Winnipeg America/Rankin_Inlet America/Resolute"
    Mexico      "America/Matamoros America/Ojinaga"
    USA         "America/Chicago America/Indiana/Knox America/Indiana/Tell_City America/Menominee"
                "America/North_Dakota/Beulah America/North_Dakota/Center"
                "America/North_Dakota/New_Salem"
*/
    QCOMPARE(QTimeZone::ianaIdToWindowsId("America/Chicago"),
             QByteArray("Central Standard Time"));
    QCOMPARE(QTimeZone::ianaIdToWindowsId("America/Resolute"),
             QByteArray("Central Standard Time"));

    // Partials shouldn't match
    QCOMPARE(QTimeZone::ianaIdToWindowsId("America/Chi"), QByteArray());
    QCOMPARE(QTimeZone::ianaIdToWindowsId("InvalidZone"), QByteArray());
    QCOMPARE(QTimeZone::ianaIdToWindowsId(QByteArray()), QByteArray());

    // Check default value
    QCOMPARE(QTimeZone::windowsIdToDefaultIanaId("Central Standard Time"),
             QByteArray("America/Chicago"));
    QCOMPARE(QTimeZone::windowsIdToDefaultIanaId("Central Standard Time", QLocale::World),
             QByteArray("America/Chicago"));
    QCOMPARE(QTimeZone::windowsIdToDefaultIanaId("Central Standard Time", QLocale::Canada),
             QByteArray("America/Winnipeg"));
    QCOMPARE(QTimeZone::windowsIdToDefaultIanaId("Central Standard Time", QLocale::AnyTerritory),
             QByteArray());
    QCOMPARE(QTimeZone::windowsIdToDefaultIanaId(QByteArray()), QByteArray());

    {
        // With no country, expect sorted list of all zones for ID
        const QList<QByteArray> list = {
            "America/Chicago", "America/Indiana/Knox", "America/Indiana/Tell_City",
            "America/Matamoros", "America/Menominee", "America/North_Dakota/Beulah",
            "America/North_Dakota/Center", "America/North_Dakota/New_Salem",
            "America/Ojinaga", "America/Rankin_Inlet", "America/Resolute",
            "America/Winnipeg"
        };
        QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time"), list);
    }
    {
        const QList<QByteArray> list = { "America/Chicago" };
        QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::World),
                 list);
    }
    {
        // Check country with no match returns empty list
        const QList<QByteArray> empty;
        QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::NewZealand),
                 empty);
    }
    {
        // Check valid country returns list in preference order
        const QList<QByteArray> list = {
            "America/Winnipeg", "America/Rankin_Inlet", "America/Resolute"
        };
        QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::Canada), list);
    }
    {
        const QList<QByteArray> list = { "America/Matamoros", "America/Ojinaga" };
        QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::Mexico), list);
    }
    {
        const QList<QByteArray> list = {
            "America/Chicago", "America/Indiana/Knox", "America/Indiana/Tell_City",
            "America/Menominee", "America/North_Dakota/Beulah", "America/North_Dakota/Center",
            "America/North_Dakota/New_Salem"
        };
        QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::UnitedStates),
                 list);
    }
    {
        const QList<QByteArray> list;
        QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::AnyTerritory),
                 list);
    }
    {
        // Check empty if given no windowsId:
        const QList<QByteArray> empty;
        QCOMPARE(QTimeZone::windowsIdToIanaIds(QByteArray()), empty);
        QCOMPARE(QTimeZone::windowsIdToIanaIds(QByteArray(), QLocale::AnyTerritory), empty);
    }
}

void tst_QTimeZone::serialize()
{
    int parts = 0;
#ifndef QT_NO_DEBUG_STREAM
    QTest::ignoreMessage(QtDebugMsg, "QTimeZone(\"\")");
    qDebug() << QTimeZone(); // to verify no crash
    parts++;
#endif
#ifndef QT_NO_DATASTREAM
    QByteArray blob;
    {
        QDataStream stream(&blob, QIODevice::WriteOnly);
        stream << QTimeZone("Europe/Oslo") << QTimeZone(420) << QTimeZone() << qint64(-1);
    }
    QDataStream stream(&blob, QIODevice::ReadOnly);
    QTimeZone invalid, offset, oslo;
    qint64 minusone;
    stream >> oslo >> offset >> invalid >> minusone;
    QCOMPARE(oslo, QTimeZone("Europe/Oslo"));
    QCOMPARE(offset, QTimeZone(420));
    QVERIFY(!invalid.isValid());
    QCOMPARE(minusone, qint64(-1));
    parts++;
#endif
    if (!parts)
        QSKIP("No serialization enabled");
}

void tst_QTimeZone::malformed()
{
    // Regression test for QTBUG-92808
    // Strings that look enough like a POSIX zone specifier that the constructor
    // accepts them, but the specifier is invalid.
    // Must not crash or trigger assertions when calling offsetFromUtc()
    const QDateTime now = QDateTime::currentDateTime();
    QTimeZone barf("QUT4tCZ0 , /");
    if (barf.isValid())
        QCOMPARE(barf.offsetFromUtc(now), 0);
    barf = QTimeZone("QtC+09,,MA");
    if (barf.isValid())
        QCOMPARE(barf.offsetFromUtc(now), 0);
    barf = QTimeZone("UTCC+14:00,-,");
    if (barf.isValid())
        QCOMPARE(barf.daylightTimeOffset(now), -14 * 3600);
}

void tst_QTimeZone::utcTest()
{
#if QT_CONFIG(icu) // || hopefully various other cases, eventually
    const QString utcLongName = u"Coordinated Universal Time"_s;
#else
    const QString utcLongName = u"UTC"_s;
#endif
#ifdef QT_BUILD_INTERNAL
    // Test default UTC constructor
    QUtcTimeZonePrivate tzp;
    QCOMPARE(tzp.isValid(),   true);
    QCOMPARE(tzp.id(), "UTC");
    QCOMPARE(tzp.territory(), QLocale::AnyTerritory);
    QCOMPARE(tzp.abbreviation(0), u"UTC");
    QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QLocale()), utcLongName);
    QCOMPARE(tzp.offsetFromUtc(0), 0);
    QCOMPARE(tzp.standardTimeOffset(0), 0);
    QCOMPARE(tzp.daylightTimeOffset(0), 0);
    QCOMPARE(tzp.hasDaylightTime(), false);
    QCOMPARE(tzp.hasTransitions(), false);
#endif // QT_BUILD_INTERNAL

    // Test UTC accessor
    const QDateTime now = QDateTime::currentDateTime();
    auto tz = QTimeZone::utc();
    QCOMPARE(tz.isValid(), true);
    QCOMPARE(tz.id(), "UTC");
    QCOMPARE(tz.territory(), QLocale::AnyTerritory);
    QCOMPARE(tz.abbreviation(now), u"UTC");
    QCOMPARE(tz.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QLocale()), utcLongName);
    QCOMPARE(tz.offsetFromUtc(now), 0);
    QCOMPARE(tz.standardTimeOffset(now), 0);
    QCOMPARE(tz.daylightTimeOffset(now), 0);
    QCOMPARE(tz.hasDaylightTime(), false);
    QCOMPARE(tz.hasTransitions(), false);

    // Test create from UTC Offset:
    tz = QTimeZone(36000);
    QVERIFY(tz.isValid());
    QCOMPARE(tz.id(), "UTC+10:00");
    QCOMPARE(tz.offsetFromUtc(now), 36000);
    QCOMPARE(tz.standardTimeOffset(now), 36000);
    QCOMPARE(tz.daylightTimeOffset(now), 0);

    tz = QTimeZone(15 * 3600); // no IANA ID, so uses minimal id, skipping :00 minutes
    QVERIFY(tz.isValid());
    QCOMPARE(tz.id(), "UTC+15:00");
    QCOMPARE(tz.offsetFromUtc(now), 15 * 3600);
    QCOMPARE(tz.standardTimeOffset(now), 15 * 3600);
    QCOMPARE(tz.daylightTimeOffset(now), 0);

    // Test validity range of UTC offsets:
    int min = QTimeZone::MinUtcOffsetSecs;
    int max = QTimeZone::MaxUtcOffsetSecs;
    QCOMPARE(QTimeZone(min - 1).isValid(), false);
    QCOMPARE(QTimeZone(min).isValid(), true);
    QCOMPARE(QTimeZone(min + 1).isValid(), true);
    QCOMPARE(QTimeZone(max - 1).isValid(), true);
    QCOMPARE(QTimeZone(max).isValid(), true);
    QCOMPARE(QTimeZone(max + 1).isValid(), false);

    // Test create from standard name (preserves :00 for minutes in id):
    tz = QTimeZone("UTC+10:00");
    QVERIFY(tz.isValid());
    QCOMPARE(tz.id(), "UTC+10:00");
    QCOMPARE(tz.offsetFromUtc(now), 36000);
    QCOMPARE(tz.standardTimeOffset(now), 36000);
    QCOMPARE(tz.daylightTimeOffset(now), 0);

    // Test create custom zone
    tz = QTimeZone("QST", 23456,
                   u"Qt Standard Time"_s, u"QST"_s, QLocale::Norway, u"Qt Testing"_s);
    QCOMPARE(tz.isValid(),   true);
    QCOMPARE(tz.id(), "QST");
    QCOMPARE(tz.comment(), u"Qt Testing");
    QCOMPARE(tz.territory(), QLocale::Norway);
    QCOMPARE(tz.abbreviation(now), u"QST");
    QCOMPARE(tz.displayName(QTimeZone::StandardTime, QTimeZone::LongName), u"Qt Standard Time");
    QCOMPARE(tz.offsetFromUtc(now), 23456);
    QCOMPARE(tz.standardTimeOffset(now), 23456);
    QCOMPARE(tz.daylightTimeOffset(now), 0);
}

void tst_QTimeZone::darwinTypes()
{
#ifndef Q_OS_DARWIN
    QSKIP("This is an Apple-only test");
#else
    extern void tst_QTimeZone_darwinTypes(); // in tst_qtimezone_darwin.mm
    tst_QTimeZone_darwinTypes();
#endif
}

void tst_QTimeZone::wasmTypes()
{
#ifndef Q_OS_WASM
    QSKIP("This is a WebAssembly-only test");
#else
    const QByteArray sysId = QTimeZone::systemTimeZoneId();
    QVERIFY(!sysId.isEmpty());
    QVERIFY(sysId == "UTC" || sysId.startsWith("UTC+") || sysId.startsWith("UTC-"));

    const QDateTime now = QDateTime::currentDateTimeUtc();

    const QTimeZone utcPlus2(7200); // UTC+02:00
    QVERIFY(utcPlus2.isValid());
    QCOMPARE(utcPlus2.id(), QByteArray("UTC+02:00"));
    QCOMPARE(utcPlus2.offsetFromUtc(now), 7200);

    const QTimeZone local(QTimeZone::LocalTime);
    const QTimeZone localBackend = local.asBackendZone();
    QCOMPARE(local.offsetFromUtc(now), localBackend.offsetFromUtc(now));
#endif
}

void tst_QTimeZone::localeSpecificDisplayName_data()
{
    QTest::addColumn<QByteArray>("zoneName");
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QTimeZone::TimeType>("timeType");
    QTest::addColumn<QString>("expectedName");
    QTest::addColumn<QDateTime>("when");

    QStringList names;
    QLocale locale;
    // Pick a non-system locale; German or French
    if (QLocale::system().language() != QLocale::German) {
        locale = QLocale(QLocale::German);
        names << u"Mitteleurop\u00e4ische Normalzeit"_s
              << u"Mitteleurop\u00e4ische Sommerzeit"_s;
    } else {
        locale = QLocale(QLocale::French);
        names << u"heure normale d\u2019Europe centrale"_s
              << u"heure d\u2019\u00E9t\u00E9 d\u2019Europe centrale"_s;
    }

    qsizetype index = 0;
    QTest::newRow("Berlin, standard time")
            << "Europe/Berlin"_ba << locale << QTimeZone::StandardTime << names.at(index++)
            << QDateTime(QDate(2024, 1, 1), QTime(12, 0));

    QTest::newRow("Berlin, summer time")
            << "Europe/Berlin"_ba << locale << QTimeZone::DaylightTime << names.at(index++)
            << QDateTime(QDate(2024, 7, 1), QTime(12, 0));
}

void tst_QTimeZone::localeSpecificDisplayName()
{
    // This test checks that QTimeZone::displayName() correctly uses the
    // specified locale, NOT the system locale (see QTBUG-101460).
    QFETCH(QByteArray, zoneName);
    QFETCH(QLocale, locale);
    QFETCH(QTimeZone::TimeType, timeType);
    QFETCH(QString, expectedName);

    const QTimeZone zone(zoneName);
    QVERIFY(zone.isValid());

    const QString localeName = zone.displayName(timeType, QTimeZone::LongName, locale);
    QCOMPARE(localeName, expectedName);
#ifdef QT_BUILD_INTERNAL
    QFETCH(QDateTime, when);
    // Check that round-trips:
    auto match = QTimeZonePrivate::findLongNamePrefix(localeName, locale, when.toMSecsSinceEpoch());
    QCOMPARE(match.nameLength, localeName.size());
    auto report = qScopeGuard([=]() {
        auto typeName = [](QTimeZone::TimeType type) {
            return (type == QTimeZone::StandardTime ? "std"
                    : type == QTimeZone::GenericTime ? "gen" : "dst");
        };
        qDebug("Long name round-tripped %s (%s) to %s (%s) via %s",
               zoneName.constData(), typeName(timeType),
               match.ianaId.constData(), typeName(match.timeType),
               localeName.toUtf8().constData());
    });
    // We may have found a different zone in the same metazone.
    // Ideally prefer canonical, but the ICU-based version doesn't.
    // At least check offsets match:
    const QTimeZone actual(match.ianaId);
    if (when.isValid() && actual.isValid())
        QCOMPARE(actual.offsetFromUtc(when), zone.offsetFromUtc(when));
    // GenericTime gets preferred and may be a synonym for StandardTime:
    if (timeType != QTimeZone::StandardTime || match.timeType != QTimeZone::GenericTime)
        QCOMPARE(match.timeType, timeType);

    // Let report happen when names don't match:
    if (match.ianaId == zoneName)
        report.dismiss();
#endif
}

#if __cpp_lib_chrono >= 201907L
Q_DECLARE_METATYPE(const std::chrono::time_zone *);
#endif

void tst_QTimeZone::stdCompatibility_data()
{
#if __cpp_lib_chrono >= 201907L
    QTest::addColumn<const std::chrono::time_zone *>("timeZone");
    const std::chrono::tzdb &tzdb = std::chrono::get_tzdb();
    qDebug() << "Using tzdb version:" << QByteArrayView(tzdb.version);

    for (const std::chrono::time_zone &zone : tzdb.zones)
        QTest::addRow("%s", zone.name().data()) << &zone;
#else
    QSKIP("This test requires C++20's <chrono>.");
#endif
}

void tst_QTimeZone::stdCompatibility()
{
#if __cpp_lib_chrono >= 201907L
    QFETCH(const std::chrono::time_zone *, timeZone);
    QByteArrayView zoneName = QByteArrayView(timeZone->name());
    QTimeZone tz = QTimeZone::fromStdTimeZonePtr(timeZone);
    if (tz.isValid())
        QVERIFY2(tz.hasAlternativeName(zoneName), tz.id().constData());
    else
        QVERIFY(!QTimeZone::isTimeZoneIdAvailable(zoneName));
#else
    QSKIP("This test requires C++20's <chrono>.");
#endif
}
#endif // timezone backends

QTEST_APPLESS_MAIN(tst_QTimeZone)
#include "tst_qtimezone.moc"
