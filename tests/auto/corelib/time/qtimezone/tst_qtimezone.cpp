// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <qtimezone.h>
#include <private/qtimezoneprivate_p.h>

#include <qlocale.h>

#if defined(Q_OS_WIN)
#include <QOperatingSystemVersion>
#endif

#if defined(Q_OS_WIN) && !QT_CONFIG(icu)
#  define USING_WIN_TZ
#endif

class tst_QTimeZone : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Public class default system tests
    void createTest();
    void nullTest();
    void assign();
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
    void specificTransition_data();
    void specificTransition();
    void transitionEachZone_data();
    void transitionEachZone();
    void checkOffset_data();
    void checkOffset();
    void stressTest();
    void windowsId();
    void isValidId_data();
    void isValidId();
    void serialize();
    void malformed();
    // Backend tests
    void utcTest();
    void icuTest();
    void tzTest();
    void macTest();
    void darwinTypes();
    void winTest();
    void localeSpecificDisplayName_data();
    void localeSpecificDisplayName();
    void stdCompatibility_data();
    void stdCompatibility();
#endif // timezone backends

private:
    void printTimeZone(const QTimeZone &tz);
#if defined(QT_BUILD_INTERNAL) && QT_CONFIG(timezone)
    // Generic tests of privates, called by implementation-specific private tests:
    void testCetPrivate(const QTimeZonePrivate &tzp);
    void testEpochTranPrivate(const QTimeZonePrivate &tzp);
#endif // QT_BUILD_INTERNAL && timezone backends
    // Set to true to print debug output, test Display Names and run long stress tests
    static constexpr bool debug = false;
};

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
    QLocale locale = QLocale(QStringLiteral("de_DE"));
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

void tst_QTimeZone::createTest()
{
    const QTimeZone tz("Pacific/Auckland");

    if (debug)
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
}

void tst_QTimeZone::nullTest()
{
    QTimeZone nullTz1;
    QTimeZone nullTz2;
    QTimeZone utc("UTC");

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

void tst_QTimeZone::compare()
{
    const QTimeZone local;
    const QTimeZone utc(QTimeZone::UTC);
    const auto secondEast = QTimeZone::fromSecondsAheadOfUtc(1);

    QCOMPARE_NE(local, utc);
    QCOMPARE_NE(utc, secondEast);
    QCOMPARE_NE(secondEast, local);

    QCOMPARE(local, QTimeZone());
    QCOMPARE(utc, QTimeZone::fromSecondsAheadOfUtc(0));
    QCOMPARE(secondEast, QTimeZone::fromDurationAheadOfUtc(std::chrono::seconds{1}));
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
#ifndef QT_NO_DATASTREAM
    // Test the OffsetFromUtc backend serialization. First with a custom timezone:
    QTimeZone tz1("QST", 123456, "Qt Standard Time", "QST", QLocale::Norway, "Qt Testing");
    QByteArray tmp;
    {
        QDataStream ds(&tmp, QIODevice::WriteOnly);
        ds << tz1;
    }
    QTimeZone tz2("UTC");
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> tz2;
    }
    QCOMPARE(tz2.id(), QByteArray("QST"));
    QCOMPARE(tz2.comment(), QString("Qt Testing"));
    QCOMPARE(tz2.territory(), QLocale::Norway);
    QCOMPARE(tz2.abbreviation(QDateTime::currentDateTime()), QString("QST"));
    QCOMPARE(tz2.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QLocale()),
             QString("Qt Standard Time"));
    QCOMPARE(tz2.displayName(QTimeZone::DaylightTime, QTimeZone::LongName, QLocale()),
             QString("Qt Standard Time"));
    QCOMPARE(tz2.offsetFromUtc(QDateTime::currentDateTime()), 123456);

    // And then with a standard IANA timezone (QTBUG-60595):
    tz1 = QTimeZone("UTC");
    QCOMPARE(tz1.isValid(), true);
    {
        QDataStream ds(&tmp, QIODevice::WriteOnly);
        ds << tz1;
    }
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> tz2;
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
    }
    tz2 = QTimeZone("UTC");
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> tz2;
    }
    QCOMPARE(tz2.id(), tz1.id());
#endif
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
    QVERIFY2(zone.isValid(), "Invalid system zone setting, tests are doomed.");
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
        QVERIFY2(QTimeZone(id).isValid(), id);
    }
    for (qint32 offset = QTimeZone::MinUtcOffsetSecs;
         offset <= QTimeZone::MinUtcOffsetSecs; ++offset) {
        const QByteArray id = QTimeZone(offset).id();
        QVERIFY2(QTimeZone::isTimeZoneIdAvailable(id), id);
        QVERIFY2(QTimeZone(id).isValid(), id);
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

    // See qtbase/util/locale_database/cldr2qtimezone.py for source
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
    ROW("UTC-02", true, -7200);
    ROW("UTC+12", true, 43200);
    ROW("UTC+13", true, 46800);
    // Encountered in bug reports:
    ROW("UTC+10", true, 36000); // QTBUG-77738

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
        QCOMPARE(zone.id(), id);
    }
}

void tst_QTimeZone::specificTransition_data()
{
    QTest::addColumn<QByteArray>("zone");
    QTest::addColumn<QDate>("start");
    QTest::addColumn<QDate>("stop");
    QTest::addColumn<int>("count");
    QTest::addColumn<QDateTime>("atUtc");
    // In seconds:
    QTest::addColumn<int>("offset");
    QTest::addColumn<int>("stdoff");
    QTest::addColumn<int>("dstoff");
#ifdef Q_OS_ANDROID
    if (!QTimeZone("Europe/Moscow").hasTransitions())
        QSKIP("Android time-zone back-end has no transition data");
#endif

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
    if (debug) {
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
        //Just test the calls work, we cannot know what any test machine has available
        QList<QByteArray> listAll = QTimeZone::availableTimeZoneIds();
        QList<QByteArray> listUs = QTimeZone::availableTimeZoneIds(QLocale::UnitedStates);
        QList<QByteArray> listZero = QTimeZone::availableTimeZoneIds(0);
    }
}

void tst_QTimeZone::stressTest()
{
    const auto UTC = QTimeZone::UTC;
    const QList<QByteArray> idList = QTimeZone::availableTimeZoneIds();
    for (const QByteArray &id : idList) {
        QTimeZone testZone = QTimeZone(id);
        QCOMPARE(testZone.isValid(), true);
        QCOMPARE(testZone.id(), id);
        QDateTime testDate = QDateTime(QDate(2015, 1, 1), QTime(0, 0), UTC);
        testZone.territory();
        testZone.comment();
        testZone.displayName(testDate);
        testZone.displayName(QTimeZone::DaylightTime);
        testZone.displayName(QTimeZone::StandardTime);
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
        if (debug) {
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
    Default     "America/Chicago"
    Canada      "America/Winnipeg America/Rainy_River America/Rankin_Inlet America/Resolute"
    Mexico      "America/Matamoros"
    USA         "America/Chicago America/Indiana/Knox America/Indiana/Tell_City America/Menominee"
                "America/North_Dakota/Beulah America/North_Dakota/Center"
                "America/North_Dakota/New_Salem"
    AnyTerritory  "CST6CDT"
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
    QCOMPARE(QTimeZone::windowsIdToDefaultIanaId("Central Standard Time", QLocale::Canada),
             QByteArray("America/Winnipeg"));
    QCOMPARE(QTimeZone::windowsIdToDefaultIanaId("Central Standard Time", QLocale::AnyTerritory),
             QByteArray("CST6CDT"));
    QCOMPARE(QTimeZone::windowsIdToDefaultIanaId(QByteArray()), QByteArray());

    // No country is sorted list of all zones
    QList<QByteArray> list;
    list << "America/Chicago" << "America/Indiana/Knox" << "America/Indiana/Tell_City"
         << "America/Matamoros" << "America/Menominee" << "America/North_Dakota/Beulah"
         << "America/North_Dakota/Center" << "America/North_Dakota/New_Salem"
         << "America/Ojinaga" << "America/Rainy_River" << "America/Rankin_Inlet"
         << "America/Resolute" << "America/Winnipeg" << "CST6CDT";
    QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time"), list);

    // Check country with no match returns empty list
    list.clear();
    QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::NewZealand),
             list);

    // Check valid country returns list in preference order
    list.clear();
    list << "America/Winnipeg" << "America/Rainy_River" << "America/Rankin_Inlet"
         << "America/Resolute";
    QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::Canada), list);

    list.clear();
    list << "America/Matamoros" << "America/Ojinaga";
    QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::Mexico), list);

    list.clear();
    list << "America/Chicago" << "America/Indiana/Knox" << "America/Indiana/Tell_City"
         << "America/Menominee" << "America/North_Dakota/Beulah" << "America/North_Dakota/Center"
         << "America/North_Dakota/New_Salem";
    QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::UnitedStates),
             list);

    list.clear();
    list << "CST6CDT";
    QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::AnyTerritory),
             list);

    // Check no windowsId return empty
    list.clear();
    QCOMPARE(QTimeZone::windowsIdToIanaIds(QByteArray()), list);
    QCOMPARE(QTimeZone::windowsIdToIanaIds(QByteArray(), QLocale::AnyTerritory), list);
}

void tst_QTimeZone::isValidId_data()
{
#ifdef QT_BUILD_INTERNAL
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
#if defined(Q_OS_ANDROID) || QT_CONFIG(icu)
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
#if defined(Q_OS_ANDROID) || QT_CONFIG(icu)
    QTest::newRow("long alone") << QByteArray("12345678901234567") << true;
    QTest::newRow("over-long alone") << QByteArray("123456789012345678") << false;
#else
    QTest::newRow("long alone") << QByteArray("12345678901234") << true;
    QTest::newRow("over-long alone") << QByteArray("123456789012345") << false;
#endif

#else
    QSKIP("This test requires a Qt -developer-build.");
#endif // QT_BUILD_INTERNAL
}

void tst_QTimeZone::isValidId()
{
#ifdef QT_BUILD_INTERNAL
    QFETCH(QByteArray, input);
    QFETCH(bool, valid);

    QCOMPARE(QTimeZonePrivate::isValidId(input), valid);
#endif
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
#ifdef QT_BUILD_INTERNAL
    // Test default UTC constructor
    QUtcTimeZonePrivate tzp;
    QCOMPARE(tzp.isValid(),   true);
    QCOMPARE(tzp.id(), QByteArray("UTC"));
    QCOMPARE(tzp.territory(), QLocale::AnyTerritory);
    QCOMPARE(tzp.abbreviation(0), QString("UTC"));
    QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QLocale()), QString("UTC"));
    QCOMPARE(tzp.offsetFromUtc(0), 0);
    QCOMPARE(tzp.standardTimeOffset(0), 0);
    QCOMPARE(tzp.daylightTimeOffset(0), 0);
    QCOMPARE(tzp.hasDaylightTime(), false);
    QCOMPARE(tzp.hasTransitions(), false);

    // Test create from UTC Offset (uses minimal id, skipping minutes if 0)
    QDateTime now = QDateTime::currentDateTime();
    QTimeZone tz(36000);
    QVERIFY(tz.isValid());
    QCOMPARE(tz.id(), QByteArray("UTC+10"));
    QCOMPARE(tz.offsetFromUtc(now), 36000);
    QCOMPARE(tz.standardTimeOffset(now), 36000);
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
    QCOMPARE(tz.id(), QByteArray("UTC+10:00"));
    QCOMPARE(tz.offsetFromUtc(now), 36000);
    QCOMPARE(tz.standardTimeOffset(now), 36000);
    QCOMPARE(tz.daylightTimeOffset(now), 0);

    // Test create custom zone
    tz = QTimeZone("QST", 123456, "Qt Standard Time", "QST", QLocale::Norway, "Qt Testing");
    QCOMPARE(tz.isValid(),   true);
    QCOMPARE(tz.id(), QByteArray("QST"));
    QCOMPARE(tz.comment(), QString("Qt Testing"));
    QCOMPARE(tz.territory(), QLocale::Norway);
    QCOMPARE(tz.abbreviation(now), QString("QST"));
    QCOMPARE(tz.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QLocale()),
             QString("Qt Standard Time"));
    QCOMPARE(tz.offsetFromUtc(now), 123456);
    QCOMPARE(tz.standardTimeOffset(now), 123456);
    QCOMPARE(tz.daylightTimeOffset(now), 0);
#endif // QT_BUILD_INTERNAL
}

// Relies on local variable names: zone tzp and locale enUS.
#define ZONE_DNAME_CHECK(type, name, val) \
        QCOMPARE(tzp.displayName(QTimeZone::type, QTimeZone::name, enUS), \
                 QStringLiteral(val));

void tst_QTimeZone::icuTest()
{
#if defined(QT_BUILD_INTERNAL) && QT_CONFIG(icu)
    // Known datetimes
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();

    // Test default constructor
    QIcuTimeZonePrivate tzpd;
    QVERIFY(tzpd.isValid());

    // Test invalid constructor
    QIcuTimeZonePrivate tzpi("Gondwana/Erewhon");
    QCOMPARE(tzpi.isValid(), false);

    // Test named constructor
    QIcuTimeZonePrivate tzp("Europe/Berlin");
    QVERIFY(tzp.isValid());

    // Only test names in debug mode, names used can vary by ICU version installed
    if (debug) {
        // Test display names by type
        QLocale enUS("en_US");
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

        // Test Abbreviations
        QCOMPARE(tzp.abbreviation(std), QString("CET"));
        QCOMPARE(tzp.abbreviation(dst), QString("CEST"));
    }

    testCetPrivate(tzp);
    if (QTest::currentTestFailed())
        return;
    testEpochTranPrivate(QIcuTimeZonePrivate("America/Toronto"));
#endif // icu
}

void tst_QTimeZone::tzTest()
{
#if defined QT_BUILD_INTERNAL && defined Q_OS_UNIX && !defined Q_OS_DARWIN && !defined Q_OS_ANDROID
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
    QLocale enUS("en_US");
    // Only test names in debug mode, names used can vary by ICU version installed
    if (debug) {
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
        QCOMPARE(tzp.abbreviation(std), QString("CET"));
        QCOMPARE(tzp.abbreviation(dst), QString("CEST"));
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
    if (dat.abbreviation == "LMT") {
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
    QCOMPARE(dat.abbreviation, QStringLiteral("CET"));
    QCOMPARE(QDateTime::fromMSecsSinceEpoch(dat.atMSecsSinceEpoch, UTC),
             QDateTime(QDate(2099, 10, 25), QTime(3, 0), QTimeZone::fromSecondsAheadOfUtc(7200)));
    QCOMPARE(dat.offsetFromUtc, 3600);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 0);

    dat = tzp.previousTransition(dstHi);
    QCOMPARE(dat.abbreviation, QStringLiteral("CEST"));
    QCOMPARE(QDateTime::fromMSecsSinceEpoch(dat.atMSecsSinceEpoch, UTC),
             QDateTime(QDate(2100, 3, 28), QTime(2, 0), QTimeZone::fromSecondsAheadOfUtc(3600)));
    QCOMPARE(dat.offsetFromUtc, 7200);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 3600);

    dat = tzp.nextTransition(stdHi);
    QCOMPARE(dat.abbreviation, QStringLiteral("CEST"));
    QCOMPARE(QDateTime::fromMSecsSinceEpoch(dat.atMSecsSinceEpoch, UTC),
             QDateTime(QDate(2100, 3, 28), QTime(2, 0), QTimeZone::fromSecondsAheadOfUtc(3600)));
    QCOMPARE(dat.offsetFromUtc, 7200);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 3600);

    dat = tzp.nextTransition(dstHi);
    QCOMPARE(dat.abbreviation, QStringLiteral("CET"));
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
        QCOMPARE(tzBarnaul.data(std).abbreviation, QString("+07"));

        // first full day of the new rule (tzdata2016b)
        QDateTime dt(QDate(2016, 3, 28), QTime(0, 0), UTC);
        QCOMPARE(tzBarnaul.data(dt.toMSecsSinceEpoch()).abbreviation, QString("+07"));
    }
#endif // QT_BUILD_INTERNAL && Q_OS_UNIX && !Q_OS_DARWIN
}

void tst_QTimeZone::macTest()
{
#if defined(QT_BUILD_INTERNAL) && defined(Q_OS_DARWIN)
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
    if (debug) {
        // Test display names by type
        QLocale enUS("en_US");
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
        QCOMPARE(tzp.abbreviation(std), QString("CET"));
        QCOMPARE(tzp.abbreviation(dst), QString("CEST"));
    }

    testCetPrivate(tzp);
    if (QTest::currentTestFailed())
        return;
    testEpochTranPrivate(QMacTimeZonePrivate("America/Toronto"));
#endif // QT_BUILD_INTERNAL && Q_OS_DARWIN
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

void tst_QTimeZone::winTest()
{
#if defined(QT_BUILD_INTERNAL) && defined(USING_WIN_TZ)
    // Known datetimes
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();

    // Test default constructor
    QWinTimeZonePrivate tzpd;
    if (debug)
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
    if (debug) {
        // Test display names by type
        QLocale enUS("en_US");
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
        QCOMPARE(tzp.abbreviation(std), QString("W. Europe Standard Time"));
        QCOMPARE(tzp.abbreviation(dst), QString("W. Europe Daylight Time"));
    }

    testCetPrivate(tzp);
    if (QTest::currentTestFailed())
        return;
    testEpochTranPrivate(QWinTimeZonePrivate("America/Toronto"));
#endif // QT_BUILD_INTERNAL && USING_WIN_TZ
}

#undef ZONE_DNAME_CHECK

void tst_QTimeZone::localeSpecificDisplayName_data()
{
#ifdef USING_WIN_TZ
    QSKIP("MS backend does not use locale parameter");
#endif
    QTest::addColumn<QByteArray>("zoneName");
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QTimeZone::TimeType>("timeType");
    QTest::addColumn<QString>("expectedName");

    QStringList names;
    QLocale locale;
    // Pick a non-system locale; German or French
    if (QLocale::system().language() != QLocale::German) {
        locale = QLocale(QLocale::German);
        names << QString("Mitteleurop\u00e4ische Normalzeit")
              << QString("Mitteleurop\u00e4ische Sommerzeit");
    } else {
        locale = QLocale(QLocale::French);
        names << QString("heure normale d\u2019Europe centrale")
              << QString("heure d\u2019\u00E9t\u00E9 d\u2019Europe centrale");
    }

    qsizetype index = 0;
    QTest::newRow("Berlin, standard time")
            << QByteArray("Europe/Berlin") << locale << QTimeZone::StandardTime
            << names.at(index++);

    QTest::newRow("Berlin, summer time")
            << QByteArray("Europe/Berlin") << locale << QTimeZone::DaylightTime
            << names.at(index++);
}

void tst_QTimeZone::localeSpecificDisplayName()
{
    // This test checks that QTimeZone::displayName() correctly uses the
    // specified locale, NOT the system locale (see QTBUG-101460).
    QFETCH(QByteArray, zoneName);
    QFETCH(QLocale, locale);
    QFETCH(QTimeZone::TimeType, timeType);
    QFETCH(QString, expectedName);

    QTimeZone zone(zoneName);
    QVERIFY(zone.isValid());

    const QString localeName = zone.displayName(timeType, QTimeZone::LongName, locale);
    QCOMPARE(localeName, expectedName);
}

#ifdef QT_BUILD_INTERNAL
// Test each private produces the same basic results for CET
void tst_QTimeZone::testCetPrivate(const QTimeZonePrivate &tzp)
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
void tst_QTimeZone::testEpochTranPrivate(const QTimeZonePrivate &tzp)
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
#endif // QT_BUILD_INTERNAL

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
        QTest::addRow(zone.name().data()) << &zone;
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
    if (tz.isValid()) {
        QCOMPARE(tz.id(), zoneName);
    } else {
        // QTBUG-102187: a few timezones reported by tzdb might not be
        // recognized by QTimeZone. This happens for instance on Windows, where
        // tzdb is using ICU, whose database does not match QTimeZone's.
        const bool isKnownUnknown =
                !zoneName.contains('/')
                || zoneName == "Antarctica/Troll"
                || zoneName.startsWith("SystemV/");
        QVERIFY(isKnownUnknown);
    }
#else
    QSKIP("This test requires C++20's <chrono>.");
#endif
}
#endif // timezone backends

QTEST_APPLESS_MAIN(tst_QTimeZone)
#include "tst_qtimezone.moc"
