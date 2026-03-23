// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <private/qtparsetemporal_p.h>

#include <QtCore/qcalendar.h>
#include <QtCore/qlocale.h>
#include <QtCore/qstring.h>

QT_REQUIRE_CONFIG(datetimeparser);

using namespace Qt::StringLiterals;
using namespace QtTemporalPattern;

class tst_QtParseTemporal : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void prefix_data();
    void prefix();
};

void tst_QtParseTemporal::prefix_data()
{
    using Cat = QtTemporalPattern::TemporalFieldCategory;
    using Flag = QtTemporalPattern::TemporalFieldFlag;
    using Flags = QtTemporalPattern::TemporalFieldFlags;
    using Field = QtTemporalPattern::TemporalField;
    using Fields = QList<Field>;
    // Inputs
    QTest::addColumn<QString>("text");
    QTest::addColumn<Fields>("fields");
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QCalendar::System>("cal");
    QTest::addColumn<int>("baseYear"); // 0 invalid so map to std::nullopt
    QTest::addColumn<int>("from");
    // Outputs expected (when until > 0):
    QTest::addColumn<int>("until"); // 0 => fail to parse
    QTest::addColumn<QTimeZone>("zone");
    QTest::addColumn<int>("millis");
    QTest::addColumn<int>("second");
    QTest::addColumn<int>("minute");
    QTest::addColumn<int>("hour");
    QTest::addColumn<int>("dayOfWeek");
    QTest::addColumn<int>("dayOfMonth");
    QTest::addColumn<int>("month");
    QTest::addColumn<int>("year");

    // Parsing treats "no zone found" as local time (a.k.a. wall-clock time)
    // since a timestamp with no zone indicator is assumed to mean that.
    const QTimeZone wall(QTimeZone::LocalTime);
    const QString empty;

    // Mainly to check we don't crash or trigger assertions:
    QTest::newRow("null/null/C/greg/0")
        << QString() << Fields() << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("empty/empty/C/greg/0")
        << empty << Fields{} << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    // Those are technically successful parses of the no fields asked for.

    // Single field tests:
    // Literal:
    QTest::newRow("The quick brown fox jumped over the lazy dogs./literal/C/greg/0")
        << u"The quick brown fox jumped over the lazy dogs."_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 46 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    // Case-sensitive:
    QTest::newRow("The quIck brOWN fox JUMped over the LAZY Dogs./literal/C/greg/0")
        << u"The quIck brOWN fox JUMped over the LAZY Dogs."_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("THE QUICK BROWN FOX JUMPED OVER THE LAZY DOGS./literal/C/greg/0")
        << u"THE QUICK BROWN FOX JUMPED OVER THE LAZY DOGS."_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("the quick brown fox jumped over the lazy dogs./literal/C/greg/0")
        << u"the quick brown fox jumped over the lazy dogs."_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    // Case-insensitive:
    QTest::newRow("The quIck brOWN fox JUMped over the LAZY Dogs./literal-case/C/greg/0")
        << u"the quick brown fox jumped over the lazy dogs."_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{ Flag::IgnoreCase }, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 46 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    // Case-insensitive takes precedence over upper and lower:
    QTest::newRow("The quIck brOWN fox JUMped over the LAZY Dogs./literal-allcase/C/greg/0")
        << u"the quick brown fox jumped over the lazy dogs."_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{ Flag::IgnoreCase | Flag::UpperCase | Flag::LowerCase },
                          Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 46 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    // Case-specific - upper:
    QTest::newRow("The quick brown fox jumped over the lazy dogs./literal+upper/C/greg/0")
        << u"The quick brown fox jumped over the lazy dogs."_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{ Flag::UpperCase }, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("THE QUICK BROWN FOX JUMPED OVER THE LAZY DOGS./literal+upper/C/greg/0")
        << u"THE QUICK BROWN FOX JUMPED OVER THE LAZY DOGS."_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{ Flag::UpperCase }, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 46 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    // Case-specific - lower:
    QTest::newRow("The quick brown fox jumped over the lazy dogs./literal+lower/C/greg/0")
        << u"The quick brown fox jumped over the lazy dogs."_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{ Flag::LowerCase }, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("the quick brown fox jumped over the lazy dogs./literal+lower/C/greg/0")
        << u"the quick brown fox jumped over the lazy dogs."_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{ Flag::LowerCase }, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 46 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    // Space-padding:
    QTest::newRow(" The quick brown fox jumped over the lazy dogs. /literal/C/greg/0")
        << u" The quick brown fox jumped over the lazy dogs. "_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow(" The quick brown fox jumped over the lazy dogs. /literal+space/C/greg/0")
        << u" The quick brown fox jumped over the lazy dogs. "_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{ Flag::SpacePad }, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 48 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;

    // Time:
    // PeriodInDay:
    QTest::newRow("AM/daypart/C/greg/0")
        << u"AM"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        // Not returned directly by prefix(), but until == 2 reveals that it was matched:
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("PM/daypart/C/greg/0")
        << u"PM"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Am/daypart/C/greg/0")
        << u"Am"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Pm/daypart/C/greg/0")
        << u"Pm"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("aM/daypart/C/greg/0")
        << u"aM"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("pM/daypart/C/greg/0")
        << u"pM"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("am/daypart/C/greg/0")
        << u"am"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("pm/daypart/C/greg/0")
        << u"pm"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;

    // Ignoring case:
    QTest::newRow("AM/daypart-case/C/greg/0")
        << u"AM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("PM/daypart-case/C/greg/0")
        << u"PM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Am/daypart-case/C/greg/0")
        << u"Am"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Pm/daypart-case/C/greg/0")
        << u"Pm"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("aM/daypart-case/C/greg/0")
        << u"aM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("pM/daypart-case/C/greg/0")
        << u"pM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("am/daypart-case/C/greg/0")
        << u"am"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("pm/daypart-case/C/greg/0")
        << u"pm"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;

    // Upper case (happens to coincide with locale):
    QTest::newRow("AM/daypart-upper/C/greg/0")
        << u"AM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::UpperCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("PM/daypart-upper/C/greg/0")
        << u"PM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::UpperCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Am/daypart-upper/C/greg/0")
        << u"Am"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::UpperCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Pm/daypart-upper/C/greg/0")
        << u"Pm"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::UpperCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("aM/daypart-upper/C/greg/0")
        << u"aM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::UpperCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("pM/daypart-upper/C/greg/0")
        << u"pM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::UpperCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("am/daypart-upper/C/greg/0")
        << u"am"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::UpperCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("pm/daypart-upper/C/greg/0")
        << u"pm"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::UpperCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;

    // Lower case:
    QTest::newRow("AM/daypart-lower/C/greg/0")
        << u"AM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::LowerCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("PM/daypart-lower/C/greg/0")
        << u"PM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::LowerCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Am/daypart-lower/C/greg/0")
        << u"Am"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::LowerCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Pm/daypart-lower/C/greg/0")
        << u"Pm"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::LowerCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("aM/daypart-lower/C/greg/0")
        << u"aM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::LowerCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("pM/daypart-lower/C/greg/0")
        << u"pM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::LowerCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("am/daypart-lower/C/greg/0")
        << u"am"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::LowerCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("pm/daypart-lower/C/greg/0")
        << u"pm"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::LowerCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;


    // Date:
    // DayOfWeek:
    QTest::newRow("Sunday/wday/C/greg/0")
        << u"Sunday"_s << Fields{ Field{ empty, 0, Flags{}, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 6 << wall << -1 << -1 << -1 << -1 << 7 << 0 << 0 << 0;
    QTest::newRow("Sunday/wday-wide/C/greg/0")
        << u"Sunday"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 6 << wall << -1 << -1 << -1 << -1 << 7 << 0 << 0 << 0;
    QTest::newRow("Sunday/wday-short/C/greg/0") // ignores "day"
        << u"Sunday"_s << Fields{ Field{ empty, 0, Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 7 << 0 << 0 << 0;
    QTest::newRow("Sunday/wday-abbr/C/greg/0") // ignores "day"
        << u"Sunday"_s << Fields{ Field{ empty, 0, Flag::Abbreviated, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 7 << 0 << 0 << 0;
    QTest::newRow("Sunday/wday-narrow/C/greg/0") // ignores "unday", reads "S" as "Saturday"
        << u"Sunday"_s << Fields{ Field{ empty, 0, Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 6 << 0 << 0 << 0;
    QTest::newRow("Sat/wday/C/greg/0")
        << u"Sat"_s << Fields{ Field{ empty, 0, Flags{}, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 6 << 0 << 0 << 0;
    QTest::newRow("Sat/wday-wide/C/greg/0")
        << u"Sat"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Sat/wday-short/C/greg/0")
        << u"Sat"_s << Fields{ Field{ empty, 0, Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 6 << 0 << 0 << 0;
    QTest::newRow("Sat/wday-narrow/C/greg/0") // ignores "at"
        << u"Sat"_s << Fields{ Field{ empty, 0, Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 6 << 0 << 0 << 0;
    QTest::newRow("5/wday/C/greg/0")
        << u"5"_s << Fields{ Field{ empty, 0, Flags{}, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 5 << 0 << 0 << 0;
    QTest::newRow("5/wday-wide/C/greg/0")
        << u"5"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("5/wday-short/C/greg/0")
        << u"5"_s << Fields{ Field{ empty, 0, Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("5/wday-narrow/C/greg/0")
        << u"5"_s << Fields{ Field{ empty, 0, Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 5 << 0 << 0 << 0;
    QTest::newRow("W/wday/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flags{}, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 3 << 0 << 0 << 0;
    QTest::newRow("W/wday-wide/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("W/wday-short/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("W/wday-narrow/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 3 << 0 << 0 << 0;

    // Verbal
    QTest::newRow("Saturday/wday-verb/C/greg/0")
        << u"Saturday"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 8 << wall << -1 << -1 << -1 << -1 << 6 << 0 << 0 << 0;
    QTest::newRow("Saturday/wday-verb-wide/C/greg/0")
        << u"Saturday"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 8 << wall << -1 << -1 << -1 << -1 << 6 << 0 << 0 << 0;
    QTest::newRow("Saturday/wday-verb-short/C/greg/0") // ignores "urday"
        << u"Saturday"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 6 << 0 << 0 << 0;
    QTest::newRow("Saturday/wday-verb-abbr/C/greg/0") // ignores "urday"
        << u"Saturday"_s << Fields{ Field{ empty, 0,
                                           Flag::Verbal | Flag::Abbreviated, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 6 << 0 << 0 << 0;
    QTest::newRow("Saturday/wday-verb-narrow/C/greg/0")
        << u"Saturday"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Fri/wday-verb/C/greg/0")
        << u"Fri"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 5 << 0 << 0 << 0;
    QTest::newRow("Fri/wday-verb-wide/C/greg/0")
        << u"Fri"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Fri/wday-verb-short/C/greg/0")
        << u"Fri"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 5 << 0 << 0 << 0;
    QTest::newRow("Fri/wday-verb-narrow/C/greg/0")
        << u"Fri"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("4/wday-verb/C/greg/0")
        << u"4"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 4 << 0 << 0 << 0;
    QTest::newRow("4/wday-verb-wide/C/greg/0")
        << u"4"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("4/wday-verb-short/C/greg/0")
        << u"4"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("4/wday-verb-narrow/C/greg/0")
        << u"4"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 4 << 0 << 0 << 0;
    QTest::newRow("W/wday-verb/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("W/wday-verb-wide/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("W/wday-verb-short/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("W/wday-verb-narrow/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;

    // Standalone
    QTest::newRow("Friday/wday-lone/C/greg/0")
        << u"Friday"_s << Fields{ Field{ empty, 0, Flag::Standalone, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 6 << wall << -1 << -1 << -1 << -1 << 5 << 0 << 0 << 0;
    QTest::newRow("Friday/wday-lone-wide/C/greg/0")
        << u"Friday"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 6 << wall << -1 << -1 << -1 << -1 << 5 << 0 << 0 << 0;
    QTest::newRow("Friday/wday-lone-short/C/greg/0") // ignores "day"
        << u"Friday"_s << Fields{ Field{ empty, 0,
                                         Flag::Standalone | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 5 << 0 << 0 << 0;
    QTest::newRow("Friday/wday-lone-abbr/C/greg/0") // ignores "day"
        << u"Friday"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Abbreviated,
                                         Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 5 << 0 << 0 << 0;
    QTest::newRow("Friday/wday-lone-narrow/C/greg/0")
        << u"Friday"_s << Fields{ Field{ empty, 0,
                                         Flag::Standalone | Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 5 << 0 << 0 << 0;
    QTest::newRow("Thu/wday-lone/C/greg/0")
        << u"Thu"_s << Fields{ Field{ empty, 0, Flag::Standalone, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 4 << 0 << 0 << 0;
    QTest::newRow("Thu/wday-lone-wide/C/greg/0")
        << u"Thu"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Thu/wday-lone-short/C/greg/0")
        << u"Thu"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 4 << 0 << 0 << 0;
    QTest::newRow("Thu/wday-lone-narrow/C/greg/0") // Ignores "hu", reads "T" as Tuesday
        << u"Thu"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 2 << 0 << 0 << 0;
    QTest::newRow("3/wday-lone/C/greg/0")
        << u"3"_s << Fields{ Field{ empty, 0, Flag::Standalone, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("3/wday-lone-narrow/C/greg/0")
        << u"3"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("W/wday-lone/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flag::Standalone, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 3 << 0 << 0 << 0;
    QTest::newRow("W/wday-lone-wide/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("W/wday-lone-short/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("W/wday-lone-narrow/C/greg/0")
        << u"W"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Narrow, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 3 << 0 << 0 << 0;

    // DayOfYear (when we get round to implementing it)
    // JulianDay (when we get round to implementing it)
    // WeekOfMonth (when we get round to implementing it)
    // WeekOfYear (when we get round to implementing it)
    // Month:
    QTest::newRow("December/month/C/greg/0")
        << u"December"_s << Fields{ Field{ empty, 0, Flags{}, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 8 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-wide/C/greg/0")
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 8 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-short/C/greg/0") // ignores "ember"
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-abbr/C/greg/0") // ignores "ember"
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Abbreviated, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-narrow/C/greg/0") // ignores "ecember"
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("Nov/month/C/greg/0")
        << u"Nov"_s << Fields{ Field{ empty, 0, Flags{}, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 11 << 0;
    QTest::newRow("Nov/month-wide/C/greg/0")
        << u"Nov"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Nov/month-short/C/greg/0")
        << u"Nov"_s << Fields{ Field{ empty, 0, Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 11 << 0;
    QTest::newRow("Nov/month-narrow/C/greg/0") // ignores "ov"
        << u"Nov"_s << Fields{ Field{ empty, 0, Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 11 << 0;
    QTest::newRow("10/month/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flags{}, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 10 << 0;
    QTest::newRow("10/month-wide/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("10/month-short/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("10/month-narrow/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 10 << 0;
    QTest::newRow("S/month/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flags{}, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 9 << 0;
    QTest::newRow("S/month-wide/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("S/month-short/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("S/month-narrow/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 9 << 0;
#if QT_CONFIG(islamiccivilcalendar)
    // See comments on TemporalFieldMatcher::monthNameExtend() about month names
    // that are prefixes of others. These tests expect a greedy parse or a parse
    // in which a trailing 'I' literal forces the less greedy parse to win out.
    QTest::newRow("Jum. II/month-short/C/islam/0")
        << u"Jum. II"_s << Fields{ Field{ empty, 0, Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::IslamicCivil << 0 << 0
        << 7 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 6 << 0;
    QTest::newRow("Jum. I'I'/month-short/C/islam/0")
        << u"Jum. II"_s << Fields{ Field{ empty, 0, Flag::Short, Cat::Month },
            Field{ u"I"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::IslamicCivil << 0 << 0
        << 7 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 5 << 0;
    QTest::newRow("Rabiʻ II/month-wide/C/islam/0")
        << u"Rabiʻ II"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::IslamicCivil << 0 << 0
        << 8 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 4 << 0;
    QTest::newRow("Rabiʻ I'I'/month-wide/C/islam/0")
        << u"Rabiʻ II"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::Month },
            Field{ u"I"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::IslamicCivil << 0 << 0
        << 8 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 3 << 0;
#endif // Islamic Civil Calendar support

    // Verbal (Narrow is number)
    QTest::newRow("December/month-verb/C/greg/0")
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 8 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-verb-wide/C/greg/0")
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 8 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-verb-short/C/greg/0") // ignores "ember"
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-verb-abbr/C/greg/0") // ignores "ember"
        << u"December"_s << Fields{ Field{ empty, 0,
                                           Flag::Verbal | Flag::Abbreviated, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-verb-narrow/C/greg/0")
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Nov/month-verb/C/greg/0")
        << u"Nov"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 11 << 0;
    QTest::newRow("Nov/month-verb-wide/C/greg/0")
        << u"Nov"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Nov/month-verb-short/C/greg/0")
        << u"Nov"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 11 << 0;
    QTest::newRow("Nov/month-verb-narrow/C/greg/0")
        << u"Nov"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("10/month-verb/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 10 << 0;
    QTest::newRow("10/month-verb-wide/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("10/month-verb-short/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("10/month-verb-narrow/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 10 << 0;
    QTest::newRow("S/month-verb/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("S/month-verb-wide/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("S/month-verb-short/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("S/month-verb-narrow/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;

    // Standalone (Narrow is first letter)
    QTest::newRow("December/month-lone/C/greg/0")
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Standalone, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 8 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-lone-wide/C/greg/0")
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 8 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-lone-short/C/greg/0") // ignores "ember"
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-lone-abbr/C/greg/0") // ignores "ember"
        << u"December"_s << Fields{ Field{ empty, 0,
                                           Flag::Standalone | Flag::Abbreviated, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("December/month-lone-narrow/C/greg/0") // ignores "ecember"
        << u"December"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 12 << 0;
    QTest::newRow("Nov/month-lone/C/greg/0")
        << u"Nov"_s << Fields{ Field{ empty, 0, Flag::Standalone, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 11 << 0;
    QTest::newRow("Nov/month-lone-wide/C/greg/0")
        << u"Nov"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Nov/month-lone-short/C/greg/0")
        << u"Nov"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 11 << 0;
    QTest::newRow("Nov/month-lone-narrow/C/greg/0") // ignores "ov"
        << u"Nov"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 11 << 0;
    QTest::newRow("10/month-lone/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Standalone, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("10/month-lone-wide/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("10/month-lone-short/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("10/month-lone-narrow/C/greg/0")
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("S/month-lone/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flag::Standalone, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 9 << 0;
    QTest::newRow("S/month-lone-wide/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("S/month-lone-short/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("S/month-lone-narrow/C/greg/0")
        << u"S"_s << Fields{ Field{ empty, 0, Flag::Standalone | Flag::Narrow, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 9 << 0;
}

void tst_QtParseTemporal::prefix()
{
    QFETCH(const QString, text);
    QFETCH(const QList<QtTemporalPattern::TemporalField>, fields);
    QFETCH(const QLocale, locale);
    QFETCH(const QCalendar::System, cal);
    QFETCH(const int, baseYear);
    QFETCH(const int, from);
    QFETCH(const int, until);

    const QCalendar calendar(cal);
    std::optional<int> century;
    if (baseYear)
        century = baseYear;
    const auto parsed = QtParseTemporal::prefix(text, fields, locale, calendar, century, from);
    if (until) {
        QVERIFY(!parsed.isEmpty());
        QCOMPARE(parsed.startIndex, from);
        QCOMPARE(parsed.endIndex, until);
        QTEST(parsed.zone, "zone");
        QTEST(parsed.millis, "millis");
        QTEST(parsed.second, "second");
        QTEST(parsed.minute, "minute");
        QTEST(parsed.hour, "hour");
        QTEST(parsed.dayOfWeek, "dayOfWeek");
        QTEST(parsed.dayOfMonth, "dayOfMonth");
        QTEST(parsed.month, "month");
        // Test uses year 0 as unspecified, but parser results can't because
        // some proleptic calendars have a year zero.
        QFETCH(const int, year);
        if (year) {
            QVERIFY2(parsed.year, "Year wasn't set but should have been");
            QCOMPARE(*parsed.year, year);
        } else {
            QVERIFY2(!parsed.year, "Year field should not have been set");
        }
    } else {
        QVERIFY(parsed.isEmpty());
    }
}

QTEST_APPLESS_MAIN(tst_QtParseTemporal)
#include "tst_qtparsetemporal.moc"
