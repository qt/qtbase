/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qtimezone.h>
#include <private/qtimezoneprivate_p.h>
#include <qlocale.h>

class tst_QTimeZone : public QObject
{
    Q_OBJECT

public:
    tst_QTimeZone();

private slots:
    // Public class default system tests
    void createTest();
    void nullTest();
    void dataStreamTest();
    void isTimeZoneIdAvailable();
    void availableTimeZoneIds();
    void stressTest();
    void windowsId();
    void isValidId_data();
    void isValidId();
    // Backend tests
    void utcTest();
    void icuTest();
    void tzTest();
    void macTest();
    void winTest();

private:
    void printTimeZone(const QTimeZone tz);
#ifdef QT_BUILD_INTERNAL
    void testCetPrivate(const QTimeZonePrivate &tzp);
#endif // QT_BUILD_INTERNAL
    bool debug;
};

tst_QTimeZone::tst_QTimeZone()
{
    // Set to true to print debug output, test Display Names and run long stress tests
    debug = false;
}

void tst_QTimeZone::printTimeZone(const QTimeZone tz)
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime jan = QDateTime(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime jun = QDateTime(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC);
    qDebug() << "";
    qDebug() << "Time Zone               = " << tz;
    qDebug() << "";
    qDebug() << "Is Valid                = " << tz.isValid();
    qDebug() << "";
    qDebug() << "Zone ID                 = " << tz.id();
    qDebug() << "Country                 = " << QLocale::countryToString(tz.country());
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
    QTimeZone tz("Pacific/Auckland");

    if (debug)
        printTimeZone(tz);

    // If the tz is not valid then skip as is probably using the UTC backend which is tested later
    if (!tz.isValid())
        return;

    // Validity tests
    QCOMPARE(tz.isValid(), true);

    // Comparison tests
    QTimeZone tz2("Pacific/Auckland");
    QTimeZone tz3("Australia/Sydney");
    QCOMPARE((tz == tz2), true);
    QCOMPARE((tz != tz2), false);
    QCOMPARE((tz == tz3), false);
    QCOMPARE((tz != tz3), true);

    QCOMPARE(tz.country(), QLocale::NewZealand);

    QDateTime jan = QDateTime(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime jun = QDateTime(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime janPrev = QDateTime(QDate(2011, 1, 1), QTime(0, 0, 0), Qt::UTC);

    QCOMPARE(tz.offsetFromUtc(jan), 46800);
    QCOMPARE(tz.offsetFromUtc(jun), 43200);

    QCOMPARE(tz.standardTimeOffset(jan), 43200);
    QCOMPARE(tz.standardTimeOffset(jun), 43200);

    QCOMPARE(tz.daylightTimeOffset(jan), 3600);
    QCOMPARE(tz.daylightTimeOffset(jun), 0);

    QCOMPARE(tz.hasDaylightTime(), true);
    QCOMPARE(tz.isDaylightTime(jan), true);
    QCOMPARE(tz.isDaylightTime(jun), false);

    // Only test transitions if host system supports them
    if (tz.hasTransitions()) {
        QTimeZone::OffsetData tran = tz.nextTransition(jan);
        QCOMPARE(tran.atUtc.toMSecsSinceEpoch(), (qint64)1333202400000);
        QCOMPARE(tran.offsetFromUtc, 43200);
        QCOMPARE(tran.standardTimeOffset, 43200);
        QCOMPARE(tran.daylightTimeOffset, 0);

        tran = tz.nextTransition(jun);
        QCOMPARE(tran.atUtc.toMSecsSinceEpoch(), (qint64)1348927200000);
        QCOMPARE(tran.offsetFromUtc, 46800);
        QCOMPARE(tran.standardTimeOffset, 43200);
        QCOMPARE(tran.daylightTimeOffset, 3600);

        tran = tz.previousTransition(jan);
        QCOMPARE(tran.atUtc.toMSecsSinceEpoch(), (qint64)1316872800000);
        QCOMPARE(tran.offsetFromUtc, 46800);
        QCOMPARE(tran.standardTimeOffset, 43200);
        QCOMPARE(tran.daylightTimeOffset, 3600);

        tran = tz.previousTransition(jun);
        QCOMPARE(tran.atUtc.toMSecsSinceEpoch(), (qint64)1333202400000);
        QCOMPARE(tran.offsetFromUtc, 43200);
        QCOMPARE(tran.standardTimeOffset, 43200);
        QCOMPARE(tran.daylightTimeOffset, 0);

        QTimeZone::OffsetDataList expected;
        tran.atUtc = QDateTime::fromMSecsSinceEpoch(1301752800000, Qt::UTC);
        tran.offsetFromUtc = 46800;
        tran.standardTimeOffset = 43200;
        tran.daylightTimeOffset = 3600;
        expected << tran;
        tran.atUtc = QDateTime::fromMSecsSinceEpoch(1316872800000, Qt::UTC);
        tran.offsetFromUtc = 43200;
        tran.standardTimeOffset = 43200;
        tran.daylightTimeOffset = 0;
        expected << tran;
        QTimeZone::OffsetDataList result = tz.transitions(janPrev, jan);
        QCOMPARE(result.count(), expected.count());
        for (int i = 0; i > expected.count(); ++i) {
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
    QCOMPARE(nullTz1.country(), QLocale::AnyCountry);
    QCOMPARE(nullTz1.comment(), QString());

    QDateTime jan = QDateTime(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime jun = QDateTime(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime janPrev = QDateTime(QDate(2011, 1, 1), QTime(0, 0, 0), Qt::UTC);

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
    QCOMPARE(data.atUtc, QDateTime());
    QCOMPARE(data.offsetFromUtc, std::numeric_limits<int>::min());
    QCOMPARE(data.standardTimeOffset, std::numeric_limits<int>::min());
    QCOMPARE(data.daylightTimeOffset, std::numeric_limits<int>::min());

    QCOMPARE(nullTz1.hasTransitions(), false);

    data = nullTz1.nextTransition(jan);
    QCOMPARE(data.atUtc, QDateTime());
    QCOMPARE(data.offsetFromUtc, std::numeric_limits<int>::min());
    QCOMPARE(data.standardTimeOffset, std::numeric_limits<int>::min());
    QCOMPARE(data.daylightTimeOffset, std::numeric_limits<int>::min());

    data = nullTz1.previousTransition(jan);
    QCOMPARE(data.atUtc, QDateTime());
    QCOMPARE(data.offsetFromUtc, std::numeric_limits<int>::min());
    QCOMPARE(data.standardTimeOffset, std::numeric_limits<int>::min());
    QCOMPARE(data.daylightTimeOffset, std::numeric_limits<int>::min());
}

void tst_QTimeZone::dataStreamTest()
{
    // Test the OffsetFromUtc backend serialization
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
    QCOMPARE(tz2.country(), QLocale::Norway);
    QCOMPARE(tz2.abbreviation(QDateTime::currentDateTime()), QString("QST"));
    QCOMPARE(tz2.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QString()),
             QString("Qt Standard Time"));
    QCOMPARE(tz2.displayName(QTimeZone::DaylightTime, QTimeZone::LongName, QString()),
             QString("Qt Standard Time"));
    QCOMPARE(tz2.offsetFromUtc(QDateTime::currentDateTime()), 123456);

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
}

void tst_QTimeZone::isTimeZoneIdAvailable()
{
    QList<QByteArray> available = QTimeZone::availableTimeZoneIds();
    foreach (const QByteArray &id, available)
        QVERIFY(QTimeZone::isTimeZoneIdAvailable(id));

    // a-z, A-Z, 0-9, '.', '-', '_' are valid chars
    // Can't start with '-'
    // Parts separated by '/', each part min 1 and max of 14 chars
    QCOMPARE(QTimeZonePrivate::isValidId("az"), true);
    QCOMPARE(QTimeZonePrivate::isValidId("AZ"), true);
    QCOMPARE(QTimeZonePrivate::isValidId("09"), true);
    QCOMPARE(QTimeZonePrivate::isValidId("a/z"), true);
    QCOMPARE(QTimeZonePrivate::isValidId("a.z"), true);
    QCOMPARE(QTimeZonePrivate::isValidId("a-z"), true);
    QCOMPARE(QTimeZonePrivate::isValidId("a_z"), true);
    QCOMPARE(QTimeZonePrivate::isValidId(".z"), true);
    QCOMPARE(QTimeZonePrivate::isValidId("_z"), true);
    QCOMPARE(QTimeZonePrivate::isValidId("12345678901234"), true);
    QCOMPARE(QTimeZonePrivate::isValidId("12345678901234/12345678901234"), true);
    QCOMPARE(QTimeZonePrivate::isValidId("a z"), false);
    QCOMPARE(QTimeZonePrivate::isValidId("a\\z"), false);
    QCOMPARE(QTimeZonePrivate::isValidId("a,z"), false);
    QCOMPARE(QTimeZonePrivate::isValidId("/z"), false);
    QCOMPARE(QTimeZonePrivate::isValidId("-z"), false);
    QCOMPARE(QTimeZonePrivate::isValidId("123456789012345"), false);
    QCOMPARE(QTimeZonePrivate::isValidId("123456789012345/12345678901234"), false);
    QCOMPARE(QTimeZonePrivate::isValidId("12345678901234/123456789012345"), false);
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
    QList<QByteArray> idList = QTimeZone::availableTimeZoneIds();
    foreach (const QByteArray &id, idList) {
        QTimeZone testZone = QTimeZone(id);
        QCOMPARE(testZone.isValid(), true);
        QCOMPARE(testZone.id(), id);
        QDateTime testDate = QDateTime(QDate(2015, 1, 1), QTime(0, 0, 0), Qt::UTC);
        testZone.country();
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
        QDateTime lowDate1 = QDateTime(QDate(1800, 1, 1), QTime(0, 0, 0), Qt::UTC);
        QDateTime lowDate2 = QDateTime(QDate(1800, 6, 1), QTime(0, 0, 0), Qt::UTC);
        QDateTime highDate1 = QDateTime(QDate(2200, 1, 1), QTime(0, 0, 0), Qt::UTC);
        QDateTime highDate2 = QDateTime(QDate(2200, 6, 1), QTime(0, 0, 0), Qt::UTC);
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
    AnyCountry  "CST6CDT"
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
    QCOMPARE(QTimeZone::windowsIdToDefaultIanaId("Central Standard Time", QLocale::AnyCountry),
             QByteArray("CST6CDT"));
    QCOMPARE(QTimeZone::windowsIdToDefaultIanaId(QByteArray()), QByteArray());

    // No country is sorted list of all zones
    QList<QByteArray> list;
    list << "America/Chicago" << "America/Indiana/Knox" << "America/Indiana/Tell_City"
         << "America/Matamoros" << "America/Menominee" << "America/North_Dakota/Beulah"
         << "America/North_Dakota/Center" << "America/North_Dakota/New_Salem"
         << "America/Rainy_River" << "America/Rankin_Inlet" << "America/Resolute"
         << "America/Winnipeg" << "CST6CDT";
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
    list << "America/Matamoros";
    QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::Mexico), list);

    list.clear();
    list << "America/Chicago" << "America/Indiana/Knox" << "America/Indiana/Tell_City"
         << "America/Menominee" << "America/North_Dakota/Beulah" << "America/North_Dakota/Center"
         << "America/North_Dakota/New_Salem";
    QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::UnitedStates),
             list);

    list.clear();
    list << "CST6CDT";
    QCOMPARE(QTimeZone::windowsIdToIanaIds("Central Standard Time", QLocale::AnyCountry),
             list);

    // Check no windowsId return empty
    list.clear();
    QCOMPARE(QTimeZone::windowsIdToIanaIds(QByteArray()), list);
    QCOMPARE(QTimeZone::windowsIdToIanaIds(QByteArray(), QLocale::AnyCountry), list);
}

void tst_QTimeZone::isValidId_data()
{
#ifdef QT_BUILD_INTERNAL
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<bool>("valid");

#define TESTSET(name, section, valid) \
    QTest::newRow(name " front")  << QByteArray(section "/xyz/xyz")    << valid; \
    QTest::newRow(name " middle") << QByteArray("xyz/" section "/xyz") << valid; \
    QTest::newRow(name " back")   << QByteArray("xyz/xyz/" section)    << valid

    TESTSET("empty", "", false);
    TESTSET("minimal", "m", true);
    TESTSET("maximal", "12345678901234", true);
    TESTSET("too long", "123456789012345", false);

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

    TESTSET("invalid char ^", "^", false);
    TESTSET("invalid char \"", "\"", false);
    TESTSET("invalid char $", "$", false);
    TESTSET("invalid char %", "%", false);
    TESTSET("invalid char &", "&", false);
    TESTSET("invalid char (", "(", false);
    TESTSET("invalid char )", ")", false);
    TESTSET("invalid char =", "=", false);
    TESTSET("invalid char ?", "?", false);
    TESTSET("invalid char ß", "ß", false);
    TESTSET("invalid char \\x01", "\x01", false);
    TESTSET("invalid char ' '", " ", false);

#undef TESTSET
#endif // QT_BUILD_INTERNAL
}

void tst_QTimeZone::isValidId()
{
#ifdef QT_BUILD_INTERNAL
    QFETCH(QByteArray, input);
    QFETCH(bool, valid);

    QCOMPARE(QTimeZonePrivate::isValidId(input), valid);
#else
    QSKIP("This test requires a Qt -developer-build.");
#endif
}

void tst_QTimeZone::utcTest()
{
#ifdef QT_BUILD_INTERNAL
    // Test default UTC constructor
    QUtcTimeZonePrivate tzp;
    QCOMPARE(tzp.isValid(),   true);
    QCOMPARE(tzp.id(), QByteArray("UTC"));
    QCOMPARE(tzp.country(), QLocale::AnyCountry);
    QCOMPARE(tzp.abbreviation(0), QString("UTC"));
    QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QString()), QString("UTC"));
    QCOMPARE(tzp.offsetFromUtc(0), 0);
    QCOMPARE(tzp.standardTimeOffset(0), 0);
    QCOMPARE(tzp.daylightTimeOffset(0), 0);
    QCOMPARE(tzp.hasDaylightTime(), false);
    QCOMPARE(tzp.hasTransitions(), false);

    // Test create from UTC Offset
    QDateTime now = QDateTime::currentDateTime();
    QTimeZone tz(36000);
    QCOMPARE(tz.isValid(),   true);
    QCOMPARE(tz.id(), QByteArray("UTC+10:00"));
    QCOMPARE(tz.offsetFromUtc(now), 36000);
    QCOMPARE(tz.standardTimeOffset(now), 36000);
    QCOMPARE(tz.daylightTimeOffset(now), 0);

    // Test invalid UTC offset, must be in range -14 to +14 hours
    int min = -14*60*60;
    int max = 14*60*60;
    QCOMPARE(QTimeZone(min - 1).isValid(), false);
    QCOMPARE(QTimeZone(min).isValid(), true);
    QCOMPARE(QTimeZone(min + 1).isValid(), true);
    QCOMPARE(QTimeZone(max - 1).isValid(), true);
    QCOMPARE(QTimeZone(max).isValid(), true);
    QCOMPARE(QTimeZone(max + 1).isValid(), false);

    // Test create from standard name
    tz = QTimeZone("UTC+10:00");
    QCOMPARE(tz.isValid(),   true);
    QCOMPARE(tz.id(), QByteArray("UTC+10:00"));
    QCOMPARE(tz.offsetFromUtc(now), 36000);
    QCOMPARE(tz.standardTimeOffset(now), 36000);
    QCOMPARE(tz.daylightTimeOffset(now), 0);

    // Test invalid UTC ID, must be in available list
    tz = QTimeZone("UTC+00:01");
    QCOMPARE(tz.isValid(),   false);

    // Test create custom zone
    tz = QTimeZone("QST", 123456, "Qt Standard Time", "QST", QLocale::Norway, "Qt Testing");
    QCOMPARE(tz.isValid(),   true);
    QCOMPARE(tz.id(), QByteArray("QST"));
    QCOMPARE(tz.comment(), QString("Qt Testing"));
    QCOMPARE(tz.country(), QLocale::Norway);
    QCOMPARE(tz.abbreviation(now), QString("QST"));
    QCOMPARE(tz.displayName(QTimeZone::StandardTime, QTimeZone::LongName, QString()),
             QString("Qt Standard Time"));
    QCOMPARE(tz.offsetFromUtc(now), 123456);
    QCOMPARE(tz.standardTimeOffset(now), 123456);
    QCOMPARE(tz.daylightTimeOffset(now), 0);
#endif // QT_BUILD_INTERNAL
}

void tst_QTimeZone::icuTest()
{
#if defined(QT_BUILD_INTERNAL) && defined(QT_USE_ICU)
    // Known datetimes
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();

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
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::LongName, enUS),
                QString("Central European Standard Time"));
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::ShortName, enUS),
                QString("GMT+01:00"));
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::OffsetName, enUS),
                QString("UTC+01:00"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::LongName, enUS),
                QString("Central European Summer Time"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::ShortName, enUS),
                QString("GMT+02:00"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::OffsetName, enUS),
                QString("UTC+02:00"));
        // ICU C api does not support Generic Time yet, C++ api does
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::LongName, enUS),
                QString("Central European Standard Time"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::ShortName, enUS),
                QString("GMT+01:00"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::OffsetName, enUS),
                QString("UTC+01:00"));

        // Test Abbreviations
        QCOMPARE(tzp.abbreviation(std), QString("CET"));
        QCOMPARE(tzp.abbreviation(dst), QString("CEST"));
    }

    testCetPrivate(tzp);
#endif // QT_USE_ICU
}

void tst_QTimeZone::tzTest()
{
#if defined QT_BUILD_INTERNAL && defined Q_OS_UNIX && !defined Q_OS_MAC
    // Known datetimes
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();

    // Test default constructor
    QTzTimeZonePrivate tzpd;
    QVERIFY(tzpd.isValid());

    // Test invalid constructor
    QTzTimeZonePrivate tzpi("Gondwana/Erewhon");
    QCOMPARE(tzpi.isValid(), false);

    // Test named constructor
    QTzTimeZonePrivate tzp("Europe/Berlin");
    QVERIFY(tzp.isValid());

    // Test display names by type, either ICU or abbreviation only
    QLocale enUS("en_US");
#ifdef QT_USE_ICU
    // Only test names in debug mode, names used can vary by ICU version installed
    if (debug) {
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::LongName, enUS),
                QString("Central European Standard Time"));
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::ShortName, enUS),
                QString("GMT+01:00"));
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::OffsetName, enUS),
                QString("UTC+01:00"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::LongName, enUS),
                QString("Central European Summer Time"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::ShortName, enUS),
                QString("GMT+02:00"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::OffsetName, enUS),
                QString("UTC+02:00"));
        // ICU C api does not support Generic Time yet, C++ api does
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::LongName, enUS),
                QString("Central European Standard Time"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::ShortName, enUS),
                QString("GMT+01:00"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::OffsetName, enUS),
                QString("UTC+01:00"));
    }
#else
    if (debug) {
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::LongName, enUS),
                QString("CET"));
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::ShortName, enUS),
                QString("CET"));
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::OffsetName, enUS),
                QString("CET"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::LongName, enUS),
                QString("CEST"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::ShortName, enUS),
                QString("CEST"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::OffsetName, enUS),
                QString("CEST"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::LongName, enUS),
                QString("CET"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::ShortName, enUS),
                QString("CET"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::OffsetName, enUS),
                QString("CET"));
    }
#endif // QT_USE_ICU

    if (debug) {
        // Test Abbreviations
        QCOMPARE(tzp.abbreviation(std), QString("CET"));
        QCOMPARE(tzp.abbreviation(dst), QString("CEST"));
    }

    testCetPrivate(tzp);

    // Test first and last transition rule
    // Warning: This could vary depending on age of TZ file!

    // Test low date uses first rule found
    // Note: Depending on the OS in question, the database may be carrying the
    // Local Mean Time. which for Berlin is 0:53:28
    QTimeZonePrivate::Data dat = tzp.data(-9999999999999);
    QCOMPARE(dat.atMSecsSinceEpoch, (qint64)-9999999999999);
    QCOMPARE(dat.daylightTimeOffset, 0);
    if (dat.abbreviation == "LMT") {
        QCOMPARE(dat.standardTimeOffset, 3208);
    } else {
        QCOMPARE(dat.standardTimeOffset, 3600);

        // Test previous to low value is invalid
        dat = tzp.previousTransition(-9999999999999);
        QCOMPARE(dat.atMSecsSinceEpoch, std::numeric_limits<qint64>::min());
        QCOMPARE(dat.standardTimeOffset, std::numeric_limits<int>::min());
        QCOMPARE(dat.daylightTimeOffset, std::numeric_limits<int>::min());
    }

    dat = tzp.nextTransition(-9999999999999);
    QCOMPARE(dat.atMSecsSinceEpoch, (qint64)-2422054408000);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 0);

    // Known high datetimes
    qint64 stdHi = QDateTime(QDate(2100, 1, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();
    qint64 dstHi = QDateTime(QDate(2100, 6, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();

    // Tets high dates use the POSIX rule
    dat = tzp.data(stdHi);
    QCOMPARE(dat.atMSecsSinceEpoch, (qint64)stdHi);
    QCOMPARE(dat.offsetFromUtc, 3600);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 0);

    dat = tzp.data(dstHi);
    QCOMPARE(dat.atMSecsSinceEpoch, (qint64)dstHi);
    QCOMPARE(dat.offsetFromUtc, 7200);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 3600);

    dat = tzp.previousTransition(stdHi);
    QCOMPARE(dat.atMSecsSinceEpoch, (qint64)4096659600000);
    QCOMPARE(dat.offsetFromUtc, 3600);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 0);

    dat = tzp.previousTransition(dstHi);
    QCOMPARE(dat.atMSecsSinceEpoch, (qint64)4109965200000);
    QCOMPARE(dat.offsetFromUtc, 7200);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 3600);

    dat = tzp.nextTransition(stdHi);
    QCOMPARE(dat.atMSecsSinceEpoch, (qint64)4109965200000);
    QCOMPARE(dat.offsetFromUtc, 7200);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 3600);

    dat = tzp.nextTransition(dstHi);
    QCOMPARE(dat.atMSecsSinceEpoch, (qint64)4128109200000);
    QCOMPARE(dat.offsetFromUtc, 3600);
    QCOMPARE(dat.standardTimeOffset, 3600);
    QCOMPARE(dat.daylightTimeOffset, 0);

    // Test TZ timezone vs UTC timezone for fractionary negative offset
    QTzTimeZonePrivate  tztz1("America/Caracas");
    QUtcTimeZonePrivate tzutc1("UTC-04:30");
    QVERIFY(tztz1.isValid());
    QVERIFY(tzutc1.isValid());
    QTzTimeZonePrivate::Data datatz1 = tztz1.data(std);
    QTzTimeZonePrivate::Data datautc1 = tzutc1.data(std);
    QCOMPARE(datatz1.offsetFromUtc, datautc1.offsetFromUtc);

    // Test TZ timezone vs UTC timezone for fractionary positive offset
    QTzTimeZonePrivate  tztz2("Asia/Calcutta");
    QUtcTimeZonePrivate tzutc2("UTC+05:30");
    QVERIFY(tztz2.isValid());
    QVERIFY(tzutc2.isValid());
    QTzTimeZonePrivate::Data datatz2 = tztz2.data(std);
    QTzTimeZonePrivate::Data datautc2 = tzutc2.data(std);
    QCOMPARE(datatz2.offsetFromUtc, datautc2.offsetFromUtc);
#endif // Q_OS_UNIX
}

void tst_QTimeZone::macTest()
{
#if defined(QT_BUILD_INTERNAL) && defined (Q_OS_MAC)
    // Known datetimes
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();

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
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::LongName, enUS),
                QString("Central European Standard Time"));
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::ShortName, enUS),
                QString("GMT+01:00"));
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::OffsetName, enUS),
                QString("UTC+01:00"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::LongName, enUS),
                QString("Central European Summer Time"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::ShortName, enUS),
                QString("GMT+02:00"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::OffsetName, enUS),
                QString("UTC+02:00"));
        // ICU C api does not support Generic Time yet, C++ api does
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::LongName, enUS),
                QString("Central European Time"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::ShortName, enUS),
                QString("Germany Time"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::OffsetName, enUS),
                QString("UTC+01:00"));

        // Test Abbreviations
        QCOMPARE(tzp.abbreviation(std), QString("CET"));
        QCOMPARE(tzp.abbreviation(dst), QString("CEST"));
    }

    testCetPrivate(tzp);
#endif // Q_OS_MAC
}

void tst_QTimeZone::winTest()
{
#if defined(QT_BUILD_INTERNAL) && defined(Q_OS_WIN)
    // Known datetimes
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();

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
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::LongName, enUS),
                QString("W. Europe Standard Time"));
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::ShortName, enUS),
                QString("W. Europe Standard Time"));
        QCOMPARE(tzp.displayName(QTimeZone::StandardTime, QTimeZone::OffsetName, enUS),
                QString("UTC+01:00"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::LongName, enUS),
                QString("W. Europe Daylight Time"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::ShortName, enUS),
                QString("W. Europe Daylight Time"));
        QCOMPARE(tzp.displayName(QTimeZone::DaylightTime, QTimeZone::OffsetName, enUS),
                QString("UTC+02:00"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::LongName, enUS),
                QString("(UTC+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::ShortName, enUS),
                QString("(UTC+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna"));
        QCOMPARE(tzp.displayName(QTimeZone::GenericTime, QTimeZone::OffsetName, enUS),
                QString("UTC+01:00"));

        // Test Abbreviations
        QCOMPARE(tzp.abbreviation(std), QString("W. Europe Standard Time"));
        QCOMPARE(tzp.abbreviation(dst), QString("W. Europe Daylight Time"));
    }

    testCetPrivate(tzp);
#endif // Q_OS_WIN
}

#ifdef QT_BUILD_INTERNAL
// Test each private produces the same basic results for CET
void tst_QTimeZone::testCetPrivate(const QTimeZonePrivate &tzp)
{
    // Known datetimes
    qint64 std = QDateTime(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();
    qint64 dst = QDateTime(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();
    qint64 prev = QDateTime(QDate(2011, 1, 1), QTime(0, 0, 0), Qt::UTC).toMSecsSinceEpoch();

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
        QCOMPARE(tran.atMSecsSinceEpoch, (qint64)1332637200000);
        QCOMPARE(tran.offsetFromUtc, 7200);
        QCOMPARE(tran.standardTimeOffset, 3600);
        QCOMPARE(tran.daylightTimeOffset, 3600);

        tran = tzp.nextTransition(dst);
        QCOMPARE(tran.atMSecsSinceEpoch, (qint64)1351386000000);
        QCOMPARE(tran.offsetFromUtc, 3600);
        QCOMPARE(tran.standardTimeOffset, 3600);
        QCOMPARE(tran.daylightTimeOffset, 0);

        tran = tzp.previousTransition(std);
        QCOMPARE(tran.atMSecsSinceEpoch, (qint64)1319936400000);
        QCOMPARE(tran.offsetFromUtc, 3600);
        QCOMPARE(tran.standardTimeOffset, 3600);
        QCOMPARE(tran.daylightTimeOffset, 0);

        tran = tzp.previousTransition(dst);
        QCOMPARE(tran.atMSecsSinceEpoch, (qint64)1332637200000);
        QCOMPARE(tran.offsetFromUtc, 7200);
        QCOMPARE(tran.standardTimeOffset, 3600);
        QCOMPARE(tran.daylightTimeOffset, 3600);

        QTimeZonePrivate::DataList expected;
        tran.atMSecsSinceEpoch = (qint64)1301752800000;
        tran.offsetFromUtc = 7200;
        tran.standardTimeOffset = 3600;
        tran.daylightTimeOffset = 3600;
        expected << tran;
        tran.atMSecsSinceEpoch = (qint64)1316872800000;
        tran.offsetFromUtc = 3600;
        tran.standardTimeOffset = 3600;
        tran.daylightTimeOffset = 0;
        expected << tran;
        QTimeZonePrivate::DataList result = tzp.transitions(prev, std);
        QCOMPARE(result.count(), expected.count());
        for (int i = 0; i > expected.count(); ++i) {
            QCOMPARE(result.at(i).atMSecsSinceEpoch, expected.at(i).atMSecsSinceEpoch);
            QCOMPARE(result.at(i).offsetFromUtc, expected.at(i).offsetFromUtc);
            QCOMPARE(result.at(i).standardTimeOffset, expected.at(i).standardTimeOffset);
            QCOMPARE(result.at(i).daylightTimeOffset, expected.at(i).daylightTimeOffset);
        }
    }
}
#endif // QT_BUILD_INTERNAL

QTEST_APPLESS_MAIN(tst_QTimeZone)
#include "tst_qtimezone.moc"
