// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QDate>
#include <QTest>
#include <QList>
using namespace Qt::StringLiterals;

class tst_QDate : public QObject
{
    Q_OBJECT

    enum : qint64
    {
        JULIAN_DAY_2010 = 2455198,
        JULIAN_DAY_2011 = 2455563,
        JULIAN_DAY_2020 = 2458850,
    };

    static QList<QDate> daily(qint64 start, qint64 end);
    static QList<QDate> yearly(qint32 first, qint32 last);

private Q_SLOTS:
    void create();
    void year();
    void month();
    void day();
    void dayOfWeek();
    void dayOfYear();
    void monthLengths(); // isValid() and daysInMonth()
    void daysInYear();
    void isLeapYear();
    void getSetDate();
    void addDays();
    void addMonths();
    void addYears();

    void fromString_data();
    void fromString();
};

QList<QDate> tst_QDate::daily(qint64 start, qint64 end)
{
    QList<QDate> list;
    list.reserve(end - start);
    for (qint64 jd = start; jd < end; ++jd)
        list.append(QDate::fromJulianDay(jd));
    return list;
}

QList<QDate> tst_QDate::yearly(qint32 first, qint32 last)
{
    QList<QDate> list;
    list.reserve(last + 1 - first);
    for (qint32 year = first; year <= last; ++year)
        list.append(QDate(year, 3, 21));
    return list;
}

void tst_QDate::create()
{
    QDate test;
    QBENCHMARK {
        for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
            test = QDate::fromJulianDay(jd);
    }
    Q_UNUSED(test);
}

void tst_QDate::year()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDate &test : list)
            test.year();
    }
}

void tst_QDate::month()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDate &test : list)
            test.month();
    }
}

void tst_QDate::day()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDate &test : list)
            test.day();
    }
}

void tst_QDate::dayOfWeek()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDate &test : list)
            test.dayOfWeek();
    }
}

void tst_QDate::dayOfYear()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDate &test : list)
            test.dayOfYear();
    }
}

void tst_QDate::monthLengths()
{
    bool check = true;
    QBENCHMARK {
        for (int year = 1900; year <= 2100; year++) {
            for (int month = 1; month <= 12; month++)
                check = QDate::isValid(year, month, QDate(year, month, 1).daysInMonth());
        }
    }
    Q_UNUSED(check);
}

void tst_QDate::daysInYear()
{
    const auto list = yearly(1601, 2401);
    QBENCHMARK {
        for (const QDate &date : list)
            date.daysInYear();
    }
}

void tst_QDate::isLeapYear()
{
    QBENCHMARK {
        for (qint32 year = 1601; year <= 2401; year++)
            QDate::isLeapYear(year);
    }
}

void tst_QDate::getSetDate()
{
    QDate store;
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const auto &test : list) {
            int year, month, day;
            test.getDate(&year, &month, &day);
            store.setDate(year, month, day);
        }
    }
    Q_UNUSED(store);
}

void tst_QDate::addDays()
{
    QDate store;
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const auto &test : list)
            store = test.addDays(17);
    }
    Q_UNUSED(store);
}

void tst_QDate::addMonths()
{
    QDate store;
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const auto &test : list)
            store = test.addMonths(17);
    }
    Q_UNUSED(store);
}

void tst_QDate::addYears()
{
    QDate store;
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const auto &test : list)
            store = test.addYears(17);
    }
    Q_UNUSED(store);
}

void tst_QDate::fromString_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("format");
    QTest::addColumn<int>("baseYear");
    QTest::addColumn<QCalendar>("calendar");

    const QCalendar greg(QCalendar::System::Gregorian);

    QTest::newRow("yyyy-MM-dd") << u"2024-04-12"_s << u"yyyy-MM-dd"_s << 2000 << greg;
    QTest::newRow("yyyy/MM/dd") << u"2024/04/12"_s << u"yyyy/MM/dd"_s << 2000 << greg;
    QTest::newRow("yyyy MM dd") << u"2024 04 12"_s << u"yyyy MM dd"_s << 2000 << greg;
    QTest::newRow("yyyy MM dd, ddd")
        << u"2024 04 12, Fri"_s << u"yyyy MM dd, ddd"_s << 2000 << greg;
    QTest::newRow("yyyy.MM.dd, ddd")
        << u"2024.04.12, Fri"_s << u"yyyy.MM.dd, ddd"_s << 2000 << greg;
    QTest::newRow("ddd, dd MM yyyy")
        << u"Fri, 12 04 2024"_s << u"ddd, dd MM yyyy"_s << 2000 << greg;
    QTest::newRow("ddd, d-MMM-yyyy")
        << u"Fri, 12-Apr-2024"_s << u"ddd, d-MMM-yyyy"_s << 2000 << greg;
    QTest::newRow("dddd, dd MMMM yyyy")
        << u"Friday, 12 April 2024"_s << u"dddd, dd MMMM yyyy"_s << 2000 << greg;

    struct YearInput
    {
        const char format[5];
        std::string input;
    };

    const YearInput years[] = {
        YearInput{"yyyy", "2024"},
        YearInput{"yy", "24"}
    };

    struct MonthInput
    {
        const char format[5];
        std::string input;
    };

    struct DayInput
    {
        const char format[5];
        std::string input;
    };

    struct CalendarInput
    {
        std::string name;
        QCalendar::System system;
    };

    const CalendarInput calendars[] = {
        CalendarInput{"Gregorian", QCalendar::System::Gregorian},
#if QT_CONFIG(islamiccivilcalendar)
        CalendarInput{"IslamicCivil", QCalendar::System::IslamicCivil},
#endif
#if QT_CONFIG(jalalicalendar)
        CalendarInput{"Jalali", QCalendar::System::Jalali}
#endif
    };

    const QLocale cLocale = QLocale::c();

    for (const auto &calSys : calendars) {
        const QCalendar calendar(calSys.system);

        const MonthInput months[] = {
            MonthInput{"M", "4"},
            MonthInput{"MM", "04"},
            MonthInput{"MMM",
                       calendar.monthName(cLocale, 4, 2024, QLocale::ShortFormat).toStdString()},
            MonthInput{"MMMM",
                       calendar.monthName(cLocale, 4, 2024, QLocale::LongFormat).toStdString()}
        };

        // Construct a Gregorian QDate from the expected calendar-specific date
        const int dayOfWeek = calendar.dayOfWeek(calendar.dateFromParts({2024, 4, 12}));

        const DayInput days[] = {
            DayInput{"d", "12"},
            DayInput{"dd", "12"},
            DayInput{"ddd",
                     calendar.weekDayName(cLocale, dayOfWeek, QLocale::ShortFormat).toStdString()},
            DayInput{"dddd",
                     calendar.weekDayName(cLocale, dayOfWeek, QLocale::LongFormat).toStdString()}
        };

        for (const auto &year : years) {
            for (const auto &month : months) {
                for (const auto &day : days) {
                    QTest::addRow("%s%s%s_%s", year.format, month.format,
                                  day.format, calSys.name.data())
                        << QString::asprintf("%s%s%s", year.input.data(), month.input.data(),
                                             day.input.data())
                        << QString::asprintf("%s%s%s", year.format, month.format, day.format)
                        << 2000 << calendar;
                }

                QTest::addRow("ddd, d%s%s_%s", month.format, year.format, calSys.name.data())
                    << QString::asprintf("%s, 12%s%s", days[2].input.data(), month.input.data(),
                                         year.input.data())
                    << QString::asprintf("ddd, d%s%s", month.format, year.format)
                    << 2000 << calendar;

                QTest::addRow("dddd, dd%s%s_%s", month.format, year.format, calSys.name.data())
                    << QString::asprintf("%s, 12%s%s", days[3].input.data(), month.input.data(),
                                         year.input.data())
                    << QString::asprintf("dddd, dd%s%s", month.format, year.format)
                    << 2000 << calendar;
            }
        }
    }
}

void tst_QDate::fromString()
{
    QFETCH(const QString, string);
    QFETCH(const QString, format);
    QFETCH(const int, baseYear);
    QFETCH(const QCalendar, calendar);

    QDate date;
    QBENCHMARK {
        date = QDate::fromString(string, format, baseYear, calendar);
    }
    QVERIFY(date.isValid());
}

QTEST_MAIN(tst_QDate)
#include "tst_bench_qdate.moc"
