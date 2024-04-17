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

    QTest::newRow("yyyyMMdd") << u"20240412"_s << u"yyyyMMdd"_s << 2000;
    QTest::newRow("yyyy-MM-dd") << u"2024-04-12"_s << u"yyyy-MM-dd"_s << 2000;
    QTest::newRow("YYYYMMDD") << u"20240412"_s << u"YYYYMMDD"_s << 2000; // Invalid, QTBUG-124465.
}

void tst_QDate::fromString()
{
    QFETCH(const QString, string);
    QFETCH(const QString, format);
    QFETCH(const int, baseYear);
    QDate date;
    QBENCHMARK {
        date = QDate::fromString(string, format, baseYear);
    }
    Q_UNUSED(date);
}

QTEST_MAIN(tst_QDate)
#include "tst_bench_qdate.moc"
