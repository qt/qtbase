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
    using namespace QtParseTemporal;

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
        << u"The quIck brOWN fox JUMped over the LAZY Dogs."_s
        << Fields{ Field{ u"The quick brown fox jumped over the lazy dogs."_s,
                          0, Flags{ Flag::IgnoreCase }, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 46 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    // Case-insensitive takes precedence over upper and lower:
    QTest::newRow("The quIck brOWN fox JUMped over the LAZY Dogs./literal-allcase/C/greg/0")
        << u"The quIck brOWN fox JUMped over the LAZY Dogs."_s
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

    // TimeZone: taken care of by ../qtparsetimezone/.

    // Time:
    // MillisecondInDay (when we get round to implementing it)
    // SecondFraction:
    QTest::newRow("999/millis:3/C/greg/0")
        << u"999"_s << Fields{ Field{ empty, 3, Flags{}, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << 999 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("999/millis+num:3/C/greg/0")
        << u"999"_s << Fields{ Field{ empty, 3, Flag::Numeric, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << 999 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("99/centis:2/C/greg/0")
        << u"99"_s << Fields{ Field{ empty, 2, Flags{}, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << 990 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("99/centis+num:2/C/greg/0")
        << u"99"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << 990 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("99/centis+0pad:2/C/greg/0")
        << u"99"_s << Fields{ Field{ empty, 2, Flag::ZeroPad, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << 990 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("99/centis+0pad:3/C/greg/0")
        << u"99"_s << Fields{ Field{ empty, 3, Flag::ZeroPad, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("99/centis+num:3/C/greg/0")
        << u"99"_s << Fields{ Field{ empty, 3, Flag::Numeric, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << 990 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("9/decis:2/C/greg/0")
        << u"9"_s << Fields{ Field{ empty, 2, Flags{}, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << 900 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("9/decis+0pad:1/C/greg/0")
        << u"9"_s << Fields{ Field{ empty, 1, Flag::ZeroPad, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << 900 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("9/decis+num:1/C/greg/0")
        << u"9"_s << Fields{ Field{ empty, 1, Flag::Numeric, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << 900 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("9/decis+0pad:2/C/greg/0")
        << u"9"_s << Fields{ Field{ empty, 2, Flag::ZeroPad, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("9/decis+num:2/C/greg/0")
        << u"9"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << 900 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("9/decis+0pad:3/C/greg/0")
        << u"9"_s << Fields{ Field{ empty, 3, Flag::ZeroPad, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("9/decis+num:3/C/greg/0")
        << u"9"_s << Fields{ Field{ empty, 3, Flag::Numeric, Cat::SecondFraction } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << 900 << -1 << -1 << -1 << 0 << 0 << 0 << 0;

    // Second:
    QTest::newRow("59/second/C/greg/0")
        << u"59"_s << Fields{ Field{ empty, 2, Flags{}, Cat::Second } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << 59 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("59/second+num/C/greg/0")
        << u"59"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::Second } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << 59 << -1 << -1 << 0 << 0 << 0 << 0;
    // For digital complications, see Minute; no point duplicating.

    // MinuteFraction (when we get round to implementing it)
    // Minute:
    QTest::newRow("59/minute+num/C/greg/0")
        << u"59"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::Minute } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << 59 << -1 << 0 << 0 << 0 << 0;
    // Always numeric, so the flag is redundant:
    QTest::newRow("59/minute:2/C/greg/0")
        << u"59"_s << Fields{ Field{ empty, 2, Flags{}, Cat::Minute } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << 59 << -1 << 0 << 0 << 0 << 0;
    // Will read two digits even if you only ask for one:
    QTest::newRow("59/minute:1/C/greg/0")
        << u"59"_s << Fields{ Field{ empty, 1, Flags{}, Cat::Minute } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << 59 << -1 << 0 << 0 << 0 << 0;
    // Number of digits is restricted to two:
    QTest::newRow("159/minute:1/C/greg/0")
        << u"159"_s << Fields{ Field{ empty, 1, Flags{}, Cat::Minute } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << 15 << -1 << 0 << 0 << 0 << 0;
    // ... unless we override (which we can):
    QTest::newRow("059/minute:3/C/greg/0")
        << u"059"_s << Fields{ Field{ empty, 3, Flags{}, Cat::Minute } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << 59 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("059/minute+0pad:3/C/greg/0")
        << u"059"_s << Fields{ Field{ empty, 3, Flag::ZeroPad, Cat::Minute } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << 59 << -1 << 0 << 0 << 0 << 0;
    // ... and, if we don't, a leading zero counts against an over-width field,
    // but not enough to overcome the preference for longer up to two digits:
    QTest::newRow("059/minute:1/C/greg/0")
        << u"059"_s << Fields{ Field{ empty, 1, Flags{}, Cat::Minute } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << 5 << -1 << 0 << 0 << 0 << 0;
    // ... unless ZeroPad was asked for, when it's tolerated:
    QTest::newRow("059/minute+0pad:1/C/greg/0")
        << u"059"_s << Fields{ Field{ empty, 1, Flag::ZeroPad, Cat::Minute } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << 5 << -1 << 0 << 0 << 0 << 0;
    // Value is limited to 0 through 59:
    QTest::newRow("60/minute:1/C/greg/0")
        << u"60"_s << Fields{ Field{ empty, 1, Flags{}, Cat::Minute } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << 6 << -1 << 0 << 0 << 0 << 0;
    // ... and a single digit is acceptable for width 2:
    QTest::newRow("60/minute:2/C/greg/0")
        << u"60"_s << Fields{ Field{ empty, 2, Flags{}, Cat::Minute } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << 6 << -1 << 0 << 0 << 0 << 0;
    // ... unless ZeroPad is set:
    QTest::newRow("60/minute+0pad:2/C/greg/0")
        << u"60"_s << Fields{ Field{ empty, 2, Flag::ZeroPad, Cat::Minute } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;

    // PeriodInDay:
    // Successful match (with no hour info) of AM => hour = -12; of PM => hour = 36
    QTest::newRow("AM/daypart/C/greg/0")
        << u"AM"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        // until == 2 (as well as the value of hour) reveals that it was matched:
        << 2 << wall << -1 << -1 << -1 << -12 << 0 << 0 << 0 << 0;
    QTest::newRow("PM/daypart/C/greg/0")
        << u"PM"_s << Fields{ Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 36 << 0 << 0 << 0 << 0;
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
        << 2 << wall << -1 << -1 << -1 << -12 << 0 << 0 << 0 << 0;
    QTest::newRow("PM/daypart-case/C/greg/0")
        << u"PM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 36 << 0 << 0 << 0 << 0;
    QTest::newRow("Am/daypart-case/C/greg/0")
        << u"Am"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -12 << 0 << 0 << 0 << 0;
    QTest::newRow("Pm/daypart-case/C/greg/0")
        << u"Pm"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 36 << 0 << 0 << 0 << 0;
    QTest::newRow("aM/daypart-case/C/greg/0")
        << u"aM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -12 << 0 << 0 << 0 << 0;
    QTest::newRow("pM/daypart-case/C/greg/0")
        << u"pM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 36 << 0 << 0 << 0 << 0;
    QTest::newRow("am/daypart-case/C/greg/0")
        << u"am"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -12 << 0 << 0 << 0 << 0;
    QTest::newRow("pm/daypart-case/C/greg/0")
        << u"pm"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::IgnoreCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 36 << 0 << 0 << 0 << 0;

    // Upper case (happens to coincide with locale):
    QTest::newRow("AM/daypart-upper/C/greg/0")
        << u"AM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::UpperCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -12 << 0 << 0 << 0 << 0;
    QTest::newRow("PM/daypart-upper/C/greg/0")
        << u"PM"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::UpperCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 36 << 0 << 0 << 0 << 0;
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
        << 2 << wall << -1 << -1 << -1 << -12 << 0 << 0 << 0 << 0;
    QTest::newRow("pm/daypart-lower/C/greg/0")
        << u"pm"_s << Fields{ Field{ empty, 0, Flag::Verbal | Flag::LowerCase, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 36 << 0 << 0 << 0 << 0;

    // HourMod12 - when no PeriodInDay is specified, its value is used directly as hour:
    QTest::newRow("12/hr%12:2/C/greg/0")
        << u"12"_s << Fields{ Field{ empty, 2, Flags{}, Cat::HourMod12 } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        // until == 2 reveals that it was parsed, in addition to setting hour:
        << 2 << wall << -1 << -1 << -1 << 12 << 0 << 0 << 0 << 0;
    QTest::newRow("12/hr%12:1/C/greg/0")
        << u"12"_s << Fields{ Field{ empty, 1, Flags{}, Cat::HourMod12 } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 12 << 0 << 0 << 0 << 0;
    QTest::newRow("1/hr%12:2/C/greg/0")
        << u"1"_s << Fields{ Field{ empty, 2, Flags{}, Cat::HourMod12 } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << 1 << 0 << 0 << 0 << 0;
    QTest::newRow("1/hr%12+0pad:2/C/greg/0")
        << u"1"_s << Fields{ Field{ empty, 2, Flag::ZeroPad, Cat::HourMod12 } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("1/hr%12:1/C/greg/0")
        << u"1"_s << Fields{ Field{ empty, 1, Flags{}, Cat::HourMod12 } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << 1 << 0 << 0 << 0 << 0;
    QTest::newRow("12/hr%12+num:2/C/greg/0")
        << u"12"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::HourMod12 } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 12 << 0 << 0 << 0 << 0;
    QTest::newRow("12/hr%12+num:1/C/greg/0")
        << u"12"_s << Fields{ Field{ empty, 1, Flag::Numeric, Cat::HourMod12 } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 12 << 0 << 0 << 0 << 0;
    QTest::newRow("1/hr%12+num:2/C/greg/0")
        << u"1"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::HourMod12 } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << 1 << 0 << 0 << 0 << 0;
    QTest::newRow("1/hr%12+num+0pad:2/C/greg/0")
        << u"1"_s << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::HourMod12 } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("1/hr%12+num:1/C/greg/0")
        << u"1"_s << Fields{ Field{ empty, 1, Flag::Numeric, Cat::HourMod12 } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << 1 << 0 << 0 << 0 << 0;

    // HourMod12 + PeriodInDay:
    QTest::newRow("12AM/hr%12/C/greg/0")
        << u"12AM"_s
        << Fields{ Field{ empty, 2, Flags{}, Cat::HourMod12 },
            Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 4 << wall << -1 << -1 << -1 << 0 << 0 << 0 << 0 << 0;
    QTest::newRow("12PM/hr%12/C/greg/0")
        << u"12PM"_s
        << Fields{ Field{ empty, 2, Flags{}, Cat::HourMod12 },
            Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 4 << wall << -1 << -1 << -1 << 12 << 0 << 0 << 0 << 0;
    QTest::newRow("12AM/hr%12+num/C/greg/0")
        << u"12AM"_s
        << Fields{ Field{ empty, 2, Flag::Numeric, Cat::HourMod12 },
            Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 4 << wall << -1 << -1 << -1 << 0 << 0 << 0 << 0 << 0;
    QTest::newRow("12PM/hr%12+num/C/greg/0")
        << u"12PM"_s
        << Fields{ Field{ empty, 2, Flag::Numeric, Cat::HourMod12 },
            Field{ empty, 0, Flag::Verbal, Cat::PeriodInDay } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 4 << wall << -1 << -1 << -1 << 12 << 0 << 0 << 0 << 0;

    // HourFraction (when we get round to implementing it)
    // Hour:
    QTest::newRow("23/hour/C/greg/0")
        << u"23"_s << Fields{ Field{ empty, 2, Flags{}, Cat::Hour } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 23 << 0 << 0 << 0 << 0;
    QTest::newRow("23/hour+num/C/greg/0")
        << u"23"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::Hour } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << 23 << 0 << 0 << 0 << 0;

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

    // DayOfMonth:
    QTest::newRow("31/mday+num/C/greg/0")
        << u"31"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 31 << 0 << 0;
    // Always numeric, so the flag is redundant:
    QTest::newRow("31/mday:2/C/greg/0")
        << u"31"_s << Fields{ Field{ empty, 2, Flags{}, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 31 << 0 << 0;
    // Will read two digits even if you only ask for one:
    QTest::newRow("31/mday:1/C/greg/0")
        << u"31"_s << Fields{ Field{ empty, 1, Flags{}, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 31 << 0 << 0;
    // Number of digits is restricted to two:
    QTest::newRow("031/mday:1/C/greg/0")
        << u"031"_s << Fields{ Field{ empty, 1, Flags{}, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 3 << 0 << 0;
    // ... unless we override (which we can):
    QTest::newRow("031/mday:3/C/greg/0")
        << u"031"_s << Fields{ Field{ empty, 3, Flags{}, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 31 << 0 << 0;
    // Zero isn't a valid value
    QTest::newRow("0031/mday:1/C/greg/0")
        << u"0031"_s << Fields{ Field{ empty, 1, Flags{}, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    // ... but overriding width can pull in later digits:
    QTest::newRow("0031/mday:3/C/greg/0")
        << u"0031"_s << Fields{ Field{ empty, 3, Flags{}, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 3 << wall << -1 << -1 << -1 << -1 << 0 << 3 << 0 << 0;
    QTest::newRow("0031/mday:4/C/greg/0")
        << u"0031"_s << Fields{ Field{ empty, 4, Flags{}, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 4 << wall << -1 << -1 << -1 << -1 << 0 << 31 << 0 << 0;
    // Value is limited to calendar's max days in month,
    QTest::newRow("32/mday/C/greg/0")
        << u"32"_s << Fields{ Field{ empty, 1, Flags{}, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 3 << 0 << 0;
    // ... which may limit digits:
    QTest::newRow("301/mday:1/C/greg/0")
        << u"301"_s << Fields{ Field{ empty, 1, Flags{}, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 30 << 0 << 0;
    // ... even when width tries to override:
    QTest::newRow("301/mday:3/C/greg/0")
        << u"301"_s << Fields{ Field{ empty, 3, Flags{}, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 30 << 0 << 0;

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
    QTest::newRow("10/month-wide/C/greg/0") // Read as numeric
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 10 << 0;
    QTest::newRow("10/month-short/C/greg/0") // Read as numeric
        << u"10"_s << Fields{ Field{ empty, 0, Flag::Short, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 10 << 0;
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

    // Numeric (ignores Wide, used here to exclude Verbal | Narrow):
    QTest::newRow("7/month+wide:0/C/greg/0")
        << u"7"_s << Fields{ Field{ empty, 0, Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 7 << 0;
    QTest::newRow("7/month+wide:1/C/greg/0")
        << u"7"_s << Fields{ Field{ empty, 1, Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 7 << 0;
    QTest::newRow("7/month+wide:2/C/greg/0")
        << u"7"_s << Fields{ Field{ empty, 2, Flag::Wide, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 7 << 0;
    QTest::newRow("7/month+wide+0pad:2/C/greg/0") // Must meet minimum width
        << u"7"_s << Fields{ Field{ empty, 2, Flag::Wide | Flag::ZeroPad, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("7/month+num:0/C/greg/0")
        << u"7"_s << Fields{ Field{ empty, 0, Flag::Numeric, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 7 << 0;
    QTest::newRow("7/month+num:1/C/greg/0")
        << u"7"_s << Fields{ Field{ empty, 1, Flag::Numeric, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 7 << 0;
    QTest::newRow("7/month+num:2/C/greg/0")
        << u"7"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 7 << 0;
    QTest::newRow("7/month+num+0pad:1/C/greg/0")
        << u"7"_s << Fields{ Field{ empty, 1, Flag::Numeric | Flag::ZeroPad, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 1 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 7 << 0;
    QTest::newRow("7/month+num+0pad:2/C/greg/0") // Must meet minimum width
        << u"7"_s << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;

    // Quarter (when we get round to implementing it)
    // YearWithinCentury
    QTest::newRow("42/yr%100/C/greg/0")
        << u"42"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::YearWithinCentury } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    // Sets year, to baseYear through baseYear+99, when baseYear is provided:
    QTest::newRow("42/yr%100:1900/C/greg/0")
        << u"42"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::YearWithinCentury } }
        << QLocale::c() << QCalendar::System::Gregorian << 1900 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 1942;
    QTest::newRow("42/yr%100:1941/C/greg/0")
        << u"42"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::YearWithinCentury } }
        << QLocale::c() << QCalendar::System::Gregorian << 1941 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 1942;
    QTest::newRow("42/yr%100:1942/C/greg/0")
        << u"42"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::YearWithinCentury } }
        << QLocale::c() << QCalendar::System::Gregorian << 1942 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 1942;
    QTest::newRow("42/yr%100:1943/C/greg/0")
        << u"42"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::YearWithinCentury } }
        << QLocale::c() << QCalendar::System::Gregorian << 1943 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 2042;
    QTest::newRow("42/yr%100:2000/C/greg/0")
        << u"42"_s << Fields{ Field{ empty, 2, Flag::Numeric, Cat::YearWithinCentury } }
        << QLocale::c() << QCalendar::System::Gregorian << 2000 << 0
        << 2 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 2042;
    // Combinations with other fields may also affect resolution, see below, after Era.

    // RelatedGregorianYear (when we get round to implementing it)
    // Year:
    QTest::newRow("1942/year:0/C/greg/0")
        << u"1942"_s << Fields{ Field{ empty, 0, Flags{}, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 4 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 1942;
    QTest::newRow("-212/year:0/C/greg/0")
        << u"-212"_s << Fields{ Field{ empty, 0, Flags{}, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 4 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << -212;
    QTest::newRow("1942/year:4/C/greg/0")
        << u"1942"_s << Fields{ Field{ empty, 4, Flags{}, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 4 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 1942;
    QTest::newRow("1942/year+num/C/greg/0")
        << u"1942"_s << Fields{ Field{ empty, 4, Flag::Numeric, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 4 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 1942;
    using Bound = std::numeric_limits<qint32>;
    QTest::newRow("2147483647/year:10/C/greg/0")
        << u"2147483647"_s << Fields{ Field{ empty, 10, Flags{}, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 10 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << Bound::max();
    QTest::newRow("-2147483648/year:10/C/greg/0") // sign doesn't count towards width
        << u"-2147483648"_s << Fields{ Field{ empty, 10, Flags{}, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 11 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << Bound::min();
    QTest::newRow("+2147483647/year:10/C/greg/0")
        << u"+2147483647"_s << Fields{ Field{ empty, 10, Flags{}, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 11 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << Bound::max();
    QTest::newRow("2147483647/year:4/C/greg/0")
        << u"2147483647"_s << Fields{ Field{ empty, 4, Flags{}, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 10 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << Bound::max();
    QTest::newRow("+2147483647/year:4/C/greg/0")
        << u"+2147483647"_s << Fields{ Field{ empty, 4, Flags{}, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 11 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << Bound::max();
    QTest::newRow("-2147483648/year:4/C/greg/0")
        << u"-2147483648"_s << Fields{ Field{ empty, 4, Flags{}, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 11 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << Bound::min();

    // Century (when we get round to implementing it)
    // Era (when we get round to implementing it)

    // Composite tests, combining various fields:

    // Date with day of week and of month, month and two-digit year can confirm
    // or adjust from a baseYear's century to an adjacent one:
    QTest::newRow("260211Wed/date:1800/C/greg/0") // Ambiguous, picks forward option:
        << u"260211Wed"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ empty, 0, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 1800 << 0
        << 9 << wall << -1 << -1 << -1 << -1 << 3 << 11 << 2 << 2026;
    QTest::newRow("260211Wed/date:1900/C/greg/0")
        << u"260211Wed"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ empty, 0, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 1900 << 0
        << 9 << wall << -1 << -1 << -1 << -1 << 3 << 11 << 2 << 2026;
    QTest::newRow("260211Wed/date:2000/C/greg/0")
        << u"260211Wed"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ empty, 0, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2000 << 0
        << 9 << wall << -1 << -1 << -1 << -1 << 3 << 11 << 2 << 2026;
    QTest::newRow("260211Wed/date:2100/C/greg/0")
        << u"260211Wed"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ empty, 0, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2100 << 0
        << 9 << wall << -1 << -1 << -1 << -1 << 3 << 11 << 2 << 2026;
    QTest::newRow("260211Wed/date:2200/C/greg/0") // Ambiguous, picks forward option:
        << u"260211Wed"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ empty, 0, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2200 << 0
        << 9 << wall << -1 << -1 << -1 << -1 << 3 << 11 << 2 << 2426;
    QTest::newRow("260211Thu/date:2000/C/greg/0") // Can shunt out of the 21st century, too.
        << u"260211Thu"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ empty, 0, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2000 << 0
        << 9 << wall << -1 << -1 << -1 << -1 << 4 << 11 << 2 << 1926;
    QTest::newRow("260211Mon/date:2000/C/greg/0")
        << u"260211Mon"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ empty, 0, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2000 << 0
        << 9 << wall << -1 << -1 << -1 << -1 << 1 << 11 << 2 << 2126;

    // Similar, with variations in the form of the week-day name:
    QTest::newRow("26 April 8, Wednesday/date+wide-dow:1900/C/greg/0")
        << u"26 April 8, Wednesday"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 1900 << 0
        << 21 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;
    QTest::newRow("26 April 8, Wednesday/date+dow:1900/C/greg/0") // greedy by default
        << u"26 April 8, Wednesday"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 1900 << 0
        << 21 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;
    QTest::newRow("26 April 8, Wednesday/date+short-dow:1900/C/greg/0") // ignores nesday
        << u"26 April 8, Wednesday"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 1900 << 0
        << 15 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;
    QTest::newRow("26 April 8, Wedding/date+short-dow:1900/C/greg/0") // ignores ding
        << u"26 April 8, Wedding"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 1900 << 0
        << 15 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;
    // Now with correct century:
    QTest::newRow("26 April 8, Wednesday/date+wide-dow:2000/C/greg/0")
        << u"26 April 8, Wednesday"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2000 << 0
        << 21 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;
    QTest::newRow("26 April 8, Wednesday/date+dow:2000/C/greg/0") // greedy by default
        << u"26 April 8, Wednesday"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2000 << 0
        << 21 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;
    QTest::newRow("26 April 8, Wednesday/date+short-dow:2000/C/greg/0") // ignores nesday
        << u"26 April 8, Wednesday"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2000 << 0
        << 15 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;
    QTest::newRow("26 April 8, Wedding/date+short-dow:2000/C/greg/0") // ignores ding
        << u"26 April 8, Wedding"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2000 << 0
        << 15 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;
    // ... and future century:
    QTest::newRow("26 April 8, Wednesday/date+wide-dow:2100/C/greg/0")
        << u"26 April 8, Wednesday"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2100 << 0
        << 21 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;
    QTest::newRow("26 April 8, Wednesday/date+dow:2100/C/greg/0") // greedy by default
        << u"26 April 8, Wednesday"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2100 << 0
        << 21 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;
    QTest::newRow("26 April 8, Wednesday/date+short-dow:2100/C/greg/0") // ignores nesday
        << u"26 April 8, Wednesday"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2100 << 0
        << 15 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;
    QTest::newRow("26 April 8, Wedding/date+short-dow:2100/C/greg/0") // ignores ding
        << u"26 April 8, Wedding"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 2100 << 0
        << 15 << wall << -1 << -1 << -1 << -1 << 3 << 8 << 4 << 2026;

    // Whole datetime with zone abbreviation (various combinations of fields):
    QTest::newRow("Wed, 11 Feb 15:10:22 CET 2026/null/C/greg/0")
        << u"Wed, 11 Feb 15:10:22 CET 2026"_s << Fields{}
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("Wed, 11 Feb 15:10:22 CET 2026/date/C/greg/0")
        << u"Wed, 11 Feb 15:10:22 CET 2026"_s
        << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::Month },
            Field{ u" 15:10:22 CET "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 29 << wall << -1 << -1 << -1 << -1 << 3 << 11 << 2 << 2026;
    QTest::newRow("Wed, 11 Feb 15:10:22 CET 2026/time/C/greg/0")
        << u"Wed, 11 Feb 15:10:22 CET 2026"_s
        << Fields{ Field{ u"Wed, 11 Feb "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ u":"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ u":"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u" CET 2026"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 29 << wall << -1 << 22 << 10 << 15 << 0 << 0 << 0 << 0;
    QTest::newRow("Wed, 11 Feb 15:10:22 CET 2026/time/C/greg/11")
        << u"Wed, 11 Feb 15:10:22 CET 2026"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ u":"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ u":"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 12
        << 20 << wall << -1 << 22 << 10 << 15 << 0 << 0 << 0 << 0;
    QTest::newRow("Wed, 11 Feb 15:10:22 CET 2026/date+time/C/greg/0")
        << u"Wed, 11 Feb 15:10:22 CET 2026"_s
        << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek },
            Field{ u", "_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ u":"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ u":"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u" CET "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 29 << wall << -1 << 22 << 10 << 15 << 3 << 11 << 2 << 2026;

    const QTimeZone cet("Europe/Oslo");
    // Variations on how to express the zone:
    QTest::newRow("Wed, 11 Feb 15:10:22 CET 2026/date+time+abbr/C/greg/0")
        << u"Wed, 11 Feb 15:10:22 CET 2026"_s
        << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek },
            Field{ u", "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ u":"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ u":"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            // Unsupported at present:
            Field{ empty, 0, Flag::LocalizedZone | Flag::Verbal | Flag::Abbreviated,
                   Cat::TimeZone },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 29 << cet << -1 << 22 << 10 << 15 << 3 << 11 << 2 << 2026;
    QTest::newRow("Wed, 11 Feb 15:10:22 Europe/Oslo 2026/date+time+iana/C/greg/0")
        << u"Wed, 11 Feb 15:10:22 Europe/Oslo 2026"_s
        << Fields{ Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ u":"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ u":"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 0, Flag::Standalone | Flag::Short, Cat::TimeZone },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 37 << cet << -1 << 22 << 10 << 15 << 3 << 11 << 2 << 2026;
    // See tst_qtpaarsetimezone for further permutations.

    // ISO-style formats (with juxtaposed numeric fields):
    QTest::newRow("20260211T151022+0100/date/C/greg/0")
        << u"20260211T151022+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T151022+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 20 << wall << -1 << -1 << -1 << -1 << 0 << 11 << 2 << 2026;
    QTest::newRow("20260211/date/C/greg/0")
        << u"20260211T151022+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 8 << wall << -1 << -1 << -1 << -1 << 0 << 11 << 2 << 2026;
    QTest::newRow("20260211T151022+0100/time/C/greg/0")
        << u"20260211T151022+0100"_s
        << Fields{ Field{ u"20260211T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u"+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 20 << wall << -1 << 22 << 10 << 15 << 0 << 0 << 0 << 0;
    QTest::newRow("20260211T151022+0100/time/C/greg/9")
        << u"20260211T151022+0100"_s
        << Fields{ Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 9
        << 15 << wall << -1 << 22 << 10 << 15 << 0 << 0 << 0 << 0;
    QTest::newRow("20260211T151022+0100/date+time/C/greg/0")
        << u"20260211T151022+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u"+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 20 << wall << -1 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    QTest::newRow("20260211T151022+0100/date+time+offset/C/greg/0")
        << u"20260211T151022+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 20 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << -1 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;

    // Now let's try that without ZeroPad:
    QTest::newRow("202621T132+0100/date/C/greg/0")
        << u"202621T132+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric, Cat::Year },
            Field{ empty, 1, Flag::Numeric, Cat::Month },
            Field{ empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u"T132+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 15 << wall << -1 << -1 << -1 << -1 << 0 << 1 << 2 << 2026;
    QTest::newRow("202621/date/C/greg/0")
        << u"202621T132+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric, Cat::Year },
            Field{ empty, 1, Flag::Numeric, Cat::Month },
            Field{ empty, 1, Flag::Numeric, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 6 << wall << -1 << -1 << -1 << -1 << 0 << 1 << 2 << 2026;
    QTest::newRow("202621T132+0100/time/C/greg/0")
        << u"202621T132+0100"_s
        << Fields{ Field{ u"202621T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 1, Flag::Numeric, Cat::Hour },
            Field{ empty, 1, Flag::Numeric, Cat::Minute },
            Field{ empty, 1, Flag::Numeric, Cat::Second },
            Field{ u"+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 15 << wall << -1 << 2 << 3 << 1 << 0 << 0 << 0 << 0;
    QTest::newRow("202621T132+0100/time/C/greg/7")
        << u"202621T132+0100"_s
        << Fields{ Field{ empty, 1, Flag::Numeric, Cat::Hour },
            Field{ empty, 1, Flag::Numeric, Cat::Minute },
            Field{ empty, 1, Flag::Numeric, Cat::Second } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 7
        << 10 << wall << -1 << 2 << 3 << 1 << 0 << 0 << 0 << 0;
    QTest::newRow("202621T132+0100/date+time/C/greg/0")
        << u"202621T132+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric, Cat::Year },
            Field{ empty, 1, Flag::Numeric, Cat::Month },
            Field{ empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 1, Flag::Numeric, Cat::Hour },
            Field{ empty, 1, Flag::Numeric, Cat::Minute },
            Field{ empty, 1, Flag::Numeric, Cat::Second },
            Field{ u"+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 15 << wall << -1 << 2 << 3 << 1 << 0 << 1 << 2 << 2026;
    QTest::newRow("202621T132+0100/date+time+offset/C/greg/0")
        << u"202621T132+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric, Cat::Year },
            Field{ empty, 1, Flag::Numeric, Cat::Month },
            Field{ empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 1, Flag::Numeric, Cat::Hour },
            Field{ empty, 1, Flag::Numeric, Cat::Minute },
            Field{ empty, 1, Flag::Numeric, Cat::Second },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 15 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << -1 << 2 << 3 << 1 << 0 << 1 << 2 << 2026;

    // ... and again, but now with some ambiguities
    QTest::newRow("2026102T132+0100/date/C/greg/0")
        << u"2026102T132+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric, Cat::Year },
            Field{ empty, 1, Flag::Numeric, Cat::Month },
            Field{ empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u"T132+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        // Ambiguous, 10 2 vs 1 02, but the latter zero-pads when we didn't ask to.
        << 16 << wall << -1 << -1 << -1 << -1 << 0 << 2 << 10 << 2026;
    QTest::newRow("2026112T132+0100/date/C/greg/0")
        << u"2026112T132+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric, Cat::Year },
            Field{ empty, 1, Flag::Numeric, Cat::Month },
            Field{ empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u"T132+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        // Ambiguous: 112 could be (Nov) 11 2 or (Jan) 1 12;
        // resolved by letting the earlier field be greedy.
        << 16 << wall << -1 << -1 << -1 << -1 << 0 << 2 << 11 << 2026;
    QTest::newRow("2026102/date/C/greg/0")
        << u"2026102T132+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric, Cat::Year },
            Field{ empty, 1, Flag::Numeric, Cat::Month },
            Field{ empty, 1, Flag::Numeric, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 7 << wall << -1 << -1 << -1 << -1 << 0 << 2 << 10 << 2026;
    QTest::newRow("2026112/date/C/greg/0")
        << u"2026112T132+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric, Cat::Year },
            Field{ empty, 1, Flag::Numeric, Cat::Month },
            Field{ empty, 1, Flag::Numeric, Cat::DayOfMonth } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 7 << wall << -1 << -1 << -1 << -1 << 0 << 2 << 11 << 2026;

    QTest::newRow("202621T1032+0100/time/C/greg/0")
        << u"202621T1032+0100"_s
        << Fields{ Field{ u"202621T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 1, Flag::Numeric, Cat::Hour },
            Field{ empty, 1, Flag::Numeric, Cat::Minute },
            Field{ empty, 1, Flag::Numeric, Cat::Second },
            Field{ u"+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        // Ambiguous but no zero-pad asked for so prefer resolution with no
        // leading zeros (10 3 2 vs 1 03 2) and let earlier field be greedy
        // (10 3 2 vs 1 0 32).
        << 16 << wall << -1 << 2 << 3 << 10 << 0 << 0 << 0 << 0;
    QTest::newRow("202621T1132+0100/time/C/greg/0")
        << u"202621T1132+0100"_s
        << Fields{ Field{ u"202621T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 1, Flag::Numeric, Cat::Hour },
            Field{ empty, 1, Flag::Numeric, Cat::Minute },
            Field{ empty, 1, Flag::Numeric, Cat::Second },
            Field{ u"+0100"_s, 0, Flags{}, Cat::Literal } }
        // Ambiguous: let earlier field's greed win.
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 16 << wall << -1 << 2 << 3 << 11 << 0 << 0 << 0 << 0;
    // There is no minute 73 in an hour:
    QTest::newRow("202621T0732+0100/time/C/greg/0")
        << u"202621T0732+0100"_s
        << Fields{ Field{ u"202621T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 1, Flag::Numeric, Cat::Hour },
            Field{ empty, 1, Flag::Numeric, Cat::Minute },
            Field{ empty, 1, Flag::Numeric, Cat::Second },
            Field{ u"+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        // 07 3 2 vs 0 7 32: prefer not zero-padded
        << 16 << wall << -1 << 32 << 7 << 0 << 0 << 0 << 0 << 0;
    QTest::newRow("202621T07325+0100/time/C/greg/0")
        << u"202621T07325+0100"_s
        << Fields{ Field{ u"202621T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 1, Flag::Numeric, Cat::Hour },
            Field{ empty, 1, Flag::Numeric, Cat::Minute },
            Field{ empty, 1, Flag::Numeric, Cat::Second },
            Field{ u"+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        // 07 3 25 vs 07 32 5: prefer earlier greed
        << 17 << wall << -1 << 5 << 32 << 7 << 0 << 0 << 0 << 0;
    // But there is a minute 13:
    QTest::newRow("202621T01325+0100/time/C/greg/0")
        << u"202621T01325+0100"_s
        << Fields{ Field{ u"202621T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 1, Flag::Numeric, Cat::Hour },
            Field{ empty, 1, Flag::Numeric, Cat::Minute },
            Field{ empty, 1, Flag::Numeric, Cat::Second },
            Field{ u"+0100"_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        // 0 13 25 vs 01 3 25 vs 01 32 5: not zero-padded
        << 17 << wall << -1 << 25 << 13 << 0 << 0 << 0 << 0 << 0;

    // Millisecond fields
    QTest::newRow("20260211T151022.765+0100/date/C/greg/0")
        << u"20260211T151022.765+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u"."_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 3, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 24 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 765 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    // Centiseconds:
    QTest::newRow("20260211T151022.76+0100/date/C/greg/0")
        << u"20260211T151022.76+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u"."_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 23 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 760 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    // Deciseconds:
    QTest::newRow("20260211T151022.7+0100/date/C/greg/0")
        << u"20260211T151022.7+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u"."_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 1, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 22 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 700 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    // Surplus precision:
    QTest::newRow("20260211T151022.76543210+0100/date/C/greg/0")
        << u"20260211T151022.76543210+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u"."_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 8, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 29 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 765 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    // Rounding up:
    QTest::newRow("20260211T151022.876543210+0100/date/C/greg/0")
        << u"20260211T151022.876543210+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u"."_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 9, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 30 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 877 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    // Overflow (controversial) - don't round up past 999:
    QTest::newRow("20260211T151022.9999+0100/date/C/greg/0")
        << u"20260211T151022.9999+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ u"."_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 4, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 25 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 999 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    // Without a separator:
    QTest::newRow("20260211T151022765+0100/date/C/greg/0")
        << u"20260211T151022765+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ empty, 3, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 23 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 765 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    // Centiseconds:
    QTest::newRow("20260211T15102276+0100/date/C/greg/0")
        << u"20260211T15102276+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ empty, 2, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 22 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 760 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    // Deciseconds:
    QTest::newRow("20260211T1510227+0100/date/C/greg/0")
        << u"20260211T1510227+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ empty, 1, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 21 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 700 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    // Surplus precision:
    QTest::newRow("20260211T15102276543210+0100/date/C/greg/0")
        << u"20260211T15102276543210+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ empty, 8, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 28 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 765 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    // Rounding up:
    QTest::newRow("20260211T151022876543210+0100/date/C/greg/0")
        << u"20260211T151022876543210+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ empty, 9, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 29 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 877 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;
    // Overflow (controversial) - don't round up past 999:
    QTest::newRow("20260211T1510229999+0100/date/C/greg/0")
        << u"20260211T1510229999+0100"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Month },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth },
            Field{ u"T"_s, 0, Flags{}, Cat::Literal },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute },
            Field{ empty, 2, Flag::Numeric | Flag::ZeroPad, Cat::Second },
            Field{ empty, 4, Flag::Numeric, Cat::SecondFraction },
            Field{ empty, 0, Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad,
                   Cat::TimeZone } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 24 << QTimeZone::fromSecondsAheadOfUtc(3600)
        << 999 << 22 << 10 << 15 << 0 << 11 << 2 << 2026;

    // Inconsistent fields
    QTest::newRow("November 10/month-name/C/greg/0") // ignoring 10
        << u"November 10"_s
        << Fields{ Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 9 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 11 << 0;
    QTest::newRow("November 10/month/C/greg/0") // 11 != 10
        << u"November 10"_s
        << Fields{ Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("2017 42/full-year/C/greg/o") // Ignoring 42
        << u"2017 42"_s
        << Fields{ Field { empty, 0, Flags{}, Cat::Year },
            Field{ u" "_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 5 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 2017;
    QTest::newRow("2017 42/year/C/greg/o") // 2017 % 100 != 42
        << u"2017 42"_s
        << Fields{ Field { empty, 0, Flags{}, Cat::Year },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::YearWithinCentury } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;

    // Conflicting month fields:
    QTest::newRow("March 7/month-only/C/greg/0") // ignoring 7
        << u"March 7"_s
        << Fields{ Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 6 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 3 << 0;
    QTest::newRow("March 7/month/C/greg/0") // 3 != 7
        << u"March 7"_s
        << Fields{ Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::Month } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;

    // Cross-talk between date fields:
    QTest::newRow("2026 April 8, Thursday/date-dow/C/greg/0") // Ignoring Thursday
        << u"2026 April 8, Thursday"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 14 << wall << -1 << -1 << -1 << -1 << 0 << 8 << 4 << 2026;
    QTest::newRow("2026 April 8, Thursday/date+short-dow/C/greg/0") // It's a Wednesday
        << u"2026 April 8, Thursday"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Short, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("2026 April 8, Thursday/date+dow/C/greg/0") // It's a Wednesday
        << u"2026 April 8, Thursday"_s
        << Fields{ Field{ empty, 4, Flag::Numeric | Flag::ZeroPad, Cat::Year },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 0, Flag::Verbal, Cat::Month },
            Field{ u" "_s, 0, Flags{}, Cat::Literal },
            Field { empty, 1, Flag::Numeric, Cat::DayOfMonth },
            Field{ u","_s, 0, Flags{ Flag::SpacePad }, Cat::Literal },
            Field{ empty, 0, Flag::Verbal | Flag::Wide, Cat::DayOfWeek } }
        << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
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
        QEXPECT_FAIL("Wed, 11 Feb 15:10:22 CET 2026/date+time+abbr/C/greg/0",
                     "Zone abbreviations not yet supported", Abort);
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
