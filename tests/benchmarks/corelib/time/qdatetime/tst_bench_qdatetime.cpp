// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QDateTime>
#include <QTimeZone>
#include <QTest>
#include <QList>
#include <qdebug.h>
#include <QtCore/private/qdatetime_p.h>

class tst_QDateTime : public QObject
{
    Q_OBJECT

    static QList<QDateTime> daily(qint64 start, qint64 end);
#if QT_CONFIG(timezone)
    static QList<QDateTime> norse(qint64 start, qint64 end);
#endif
    void decade_data();

private Q_SLOTS:
    void create_data() { decade_data(); }
    void create();
    void isNull();
    void isValid();
    void date();
    void time();
    void timeSpec();
    void offsetFromUtc();
    void timeZoneAbbreviation();
    void toMSecsSinceEpoch_data() { decade_data(); }
    void toMSecsSinceEpoch();
#if QT_CONFIG(timezone) 
    void toMSecsSinceEpochTz_data() { decade_data(); }
    void toMSecsSinceEpochTz();
#endif
    void setDate();
    void setTime();
#if QT_DEPRECATED_SINCE(6, 9)
    void setTimeSpec();
    void setOffsetFromUtc();
#endif
    void setMSecsSinceEpoch();
#if QT_CONFIG(timezone)
    void setMSecsSinceEpochTz();
#endif
    void toString();
    void toStringTextFormat();
    void toStringIsoFormat();
    void addDays();
#if QT_CONFIG(timezone)
    void addDaysTz();
    void addMSecsTz();
#endif
    void addMSecs();
#if QT_DEPRECATED_SINCE(6, 9)
    void toTimeSpec();
    void toOffsetFromUtc();
#endif
    void daysTo();
    void msecsTo();
    void equivalent();
    void equivalentUtc();
    void lessThan();
    void lessThanUtc();
    void currentDateTime();
    void currentDate();
    void currentTime();
    void currentDateTimeUtc();
    void currentMSecsSinceEpoch();
    void fromString();
    void fromString_data();
    void fromStringText();
    void fromStringIso();
    void fromMSecsSinceEpoch();
    void fromMSecsSinceEpochUtc();
#if QT_CONFIG(timezone)
    void fromMSecsSinceEpochTz();
#endif
};

using namespace QtPrivate::DateTimeConstants;
constexpr qint64 JULIAN_DAY_1 = 1721426;
constexpr qint64 JULIAN_DAY_11 = 1725078;
constexpr qint64 JULIAN_DAY_1890 = 2411369;
constexpr qint64 JULIAN_DAY_1900 = 2415021;
constexpr qint64 JULIAN_DAY_1950 = 2433283;
constexpr qint64 JULIAN_DAY_1960 = 2436935;
constexpr qint64 JULIAN_DAY_1970 = 2440588; // Epoch
constexpr qint64 JULIAN_DAY_2010 = 2455198;
constexpr qint64 JULIAN_DAY_2011 = 2455563;
constexpr qint64 JULIAN_DAY_2020 = 2458850;
constexpr qint64 JULIAN_DAY_2050 = 2469808;
constexpr qint64 JULIAN_DAY_2060 = 2473460;

void tst_QDateTime::decade_data()
{
    QTest::addColumn<qint64>("startJd");
    QTest::addColumn<qint64>("stopJd");

    QTest::newRow("first-decade-CE") << JULIAN_DAY_1 << JULIAN_DAY_11;
    QTest::newRow("1890s") << JULIAN_DAY_1890 << JULIAN_DAY_1900;
    QTest::newRow("1950s") << JULIAN_DAY_1950 << JULIAN_DAY_1960;
    QTest::newRow("2010s") << JULIAN_DAY_2010 << JULIAN_DAY_2020;
    QTest::newRow("2050s") << JULIAN_DAY_2050 << JULIAN_DAY_2060;
}

QList<QDateTime> tst_QDateTime::daily(qint64 start, qint64 end)
{
    QList<QDateTime> list;
    list.reserve(end - start);
    for (int jd = start; jd < end; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd).startOfDay()));
    return list;
}
#if QT_CONFIG(timezone)
QList<QDateTime> tst_QDateTime::norse(qint64 start, qint64 end)
{
    const QTimeZone cet("Europe/Oslo");
    QList<QDateTime> list;
    list.reserve(end - start);
    for (int jd = start; jd < end; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd).startOfDay(cet)));
    return list;
}
#endif
void tst_QDateTime::create()
{
    QFETCH(const qint64, startJd);
    QFETCH(const qint64, stopJd);
    const QTime noon = QTime::fromMSecsSinceStartOfDay(43200 * 1000);
    QBENCHMARK {
        for (int jd = startJd; jd < stopJd; ++jd) {
            QDateTime test(QDate::fromJulianDay(jd), noon);
            Q_UNUSED(test);
        }
    }
}

void tst_QDateTime::isNull()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.isNull();
    }
}

void tst_QDateTime::isValid()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.isValid();
    }
}

void tst_QDateTime::date()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.date();
    }
}

void tst_QDateTime::time()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.time();
    }
}

void tst_QDateTime::timeSpec()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.timeSpec();
    }
}

void tst_QDateTime::offsetFromUtc()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.offsetFromUtc();
    }
}

void tst_QDateTime::timeZoneAbbreviation()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.timeZoneAbbreviation();
    }
}

void tst_QDateTime::toMSecsSinceEpoch()
{
    QFETCH(const qint64, startJd);
    QFETCH(const qint64, stopJd);
    const auto list = daily(startJd, stopJd);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.toMSecsSinceEpoch();
    }
}
#if QT_CONFIG(timezone)
void tst_QDateTime::toMSecsSinceEpochTz()
{
    QFETCH(const qint64, startJd);
    QFETCH(const qint64, stopJd);
    const auto list = norse(startJd, stopJd);

    qint64 result;
    QBENCHMARK {
        for (const QDateTime &test : list)
            result = test.toMSecsSinceEpoch();
    }
    Q_UNUSED(result);
}
#endif
void tst_QDateTime::setDate()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (QDateTime test : list)
            test.setDate(QDate::fromJulianDay(JULIAN_DAY_2010));
    }
}

void tst_QDateTime::setTime()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (QDateTime test : list)
            test.setTime(QTime(12, 0, 0));
    }
}

#if QT_DEPRECATED_SINCE(6, 9)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
void tst_QDateTime::setTimeSpec()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (QDateTime test : list)
            test.setTimeSpec(Qt::UTC);
    }
}

void tst_QDateTime::setOffsetFromUtc()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (QDateTime test : list)
            test.setOffsetFromUtc(3600);
    }
}
QT_WARNING_POP
#endif // 6.9 deprecation

void tst_QDateTime::setMSecsSinceEpoch()
{
    qint64 msecs = qint64(JULIAN_DAY_2010 - JULIAN_DAY_1970 + 180) * MSECS_PER_DAY;
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (QDateTime test : list)
            test.setMSecsSinceEpoch(msecs);
    }
}
#if QT_CONFIG(timezone)
void tst_QDateTime::setMSecsSinceEpochTz()
{
    const qint64 msecs = qint64(JULIAN_DAY_2010 - JULIAN_DAY_1970 + 180) * MSECS_PER_DAY;
    const auto list = norse(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (QDateTime test : list)
            test.setMSecsSinceEpoch(msecs);
    }
}
#endif
void tst_QDateTime::toString()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2011);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.toString(QStringLiteral("yyy-MM-dd hh:mm:ss.zzz t"));
    }
}

void tst_QDateTime::toStringTextFormat()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2011);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.toString(Qt::TextDate);
    }
}

void tst_QDateTime::toStringIsoFormat()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2011);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.toString(Qt::ISODate);
    }
}

void tst_QDateTime::addDays()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QDateTime next;
    QBENCHMARK {
        for (const QDateTime &test : list)
            next = test.addDays(1);
    }
    Q_UNUSED(next);
}
#if QT_CONFIG(timezone)
void tst_QDateTime::addDaysTz()
{
    const auto list = norse(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            QDateTime result = test.addDays(1);
    }
}
#endif
void tst_QDateTime::addMSecs()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QDateTime next;
    QBENCHMARK {
        for (const QDateTime &test : list)
            next = test.addMSecs(1);
    }
    Q_UNUSED(next);
}
#if QT_CONFIG(timezone)
void tst_QDateTime::addMSecsTz()
{
    const auto list = norse(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            QDateTime result = test.addMSecs(1);
    }
}
#endif
#if QT_DEPRECATED_SINCE(6, 9)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
void tst_QDateTime::toTimeSpec()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.toTimeSpec(Qt::UTC);
    }
}

void tst_QDateTime::toOffsetFromUtc()
{
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.toOffsetFromUtc(3600);
    }
}
QT_WARNING_POP
#endif

void tst_QDateTime::daysTo()
{
    const QDateTime other = QDateTime::fromMSecsSinceEpoch(
        qint64(JULIAN_DAY_2010 - JULIAN_DAY_1970) * MSECS_PER_DAY);
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.daysTo(other);
    }
}

void tst_QDateTime::msecsTo()
{
    const QDateTime other = QDateTime::fromMSecsSinceEpoch(
        qint64(JULIAN_DAY_2010 - JULIAN_DAY_1970) * MSECS_PER_DAY);
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            test.msecsTo(other);
    }
}

void tst_QDateTime::equivalent()
{
    bool result;
    const QDateTime other = QDateTime::fromMSecsSinceEpoch(
        qint64(JULIAN_DAY_2010 - JULIAN_DAY_1970) * MSECS_PER_DAY);
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            result = (test == other);
    }
    Q_UNUSED(result);
}

void tst_QDateTime::equivalentUtc()
{
    bool result = false;
    const QDateTime other = QDateTime::fromMSecsSinceEpoch(
        qint64(JULIAN_DAY_2010 - JULIAN_DAY_1970) * MSECS_PER_DAY, QTimeZone::UTC);
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            result = (test == other);
    }
    Q_UNUSED(result);
}

void tst_QDateTime::lessThan()
{
    bool result = false;
    const QDateTime other = QDateTime::fromMSecsSinceEpoch(
        qint64(JULIAN_DAY_2010 - JULIAN_DAY_1970) * MSECS_PER_DAY);
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            result = (test < other);
    }
    Q_UNUSED(result);
}

void tst_QDateTime::lessThanUtc()
{
    bool result = false;
    const QDateTime other = QDateTime::fromMSecsSinceEpoch(
        qint64(JULIAN_DAY_2010 - JULIAN_DAY_1970) * MSECS_PER_DAY, QTimeZone::UTC);
    const auto list = daily(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            result = (test < other);
    }
    Q_UNUSED(result);
}

void tst_QDateTime::currentDateTime()
{
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::currentDateTime();
    }
}

void tst_QDateTime::currentDate()
{
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDate::currentDate();
    }
}

void tst_QDateTime::currentTime()
{
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QTime::currentTime();
    }
}

void tst_QDateTime::currentDateTimeUtc()
{
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::currentDateTimeUtc();
    }
}

void tst_QDateTime::currentMSecsSinceEpoch()
{
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::currentMSecsSinceEpoch();
    }
}

void tst_QDateTime::fromString_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QCalendar>("calendar");
    QTest::addColumn<int>("baseYear");

    const QCalendar greg(QCalendar::System::Gregorian);

    QTest::newRow("h:m:s.z d.M.yy(Gregorian)")
            << "07:01:04.8 1.9.24" << "h:m:s.z d.M.yy" << greg << 2000;

    QTest::newRow("dd/MM/yyyy hh:mm:ss.zzz(Gregorian)")
            << "01/09/2024 07:01:04.783" << "dd/MM/yyyy hh:mm:ss.zzz" << greg << 2000;

    QTest::newRow("H:m:s.zz ddd d-MMM-yyyy(Gregorian)")
            << "7:1:4.78 Sun 1-Sep-2024" << "H:m:s.zz ddd d-MMM-yyyy" << greg << 2000;

    QTest::newRow("dddd dd/MMMM/yy HH:mm:ss.z(Gregorian)")
            << "Sunday 01/September/24 07:01:04.8" << "dddd dd/MMMM/yy HH:mm:ss.z" << greg << 2000;

    QTest::newRow("h:m A M/d/yy(Gregorian)")
            << "7:1 AM 9/1/24" << "h:m AP M/d/yy" << greg << 2000;

    QTest::newRow("MM-dd-yyyy hh:mm:ss ap(Gregorian)")
            << "09-01-2024 07:01:04 pm" << "MM-dd-yyyy hh:mm:ss a" << greg << 2000;

    QTest::newRow("HH:mm:ss.zz yyyy.MM.dd(Gregorian)")
            << "07:01:04.78 2024.09.01" << "HH:mm:ss.zz yyyy.MM.dd" << greg << 2000;

    QTest::newRow("yyyy MM dd hh:mm:ss.z aP(Gregorian)")
            << "2024 09 01 07:01:04.8 PM" << "yyyy MM dd hh:mm:ss.z aP" << greg << 2000;

    QTest::newRow("hh:mm:ss.zzz dd.MMM.yyyy(Gregorian)")
            << "07:01:04.783 01.Sep.2024" << "hh:mm:ss.zzz dd.MMM.yyyy" << greg << 2000;

    QTest::newRow("dddd, dd MMMM yyyy H:m:s(Gregorian)")
            << "Sunday, 01 September 2024 7:1:4" << "dddd, dd MMMM yyyy H:m:s" << greg << 2000;

    QTest::newRow("hh:mm:ss.z yyyy-MM-dd t(Gregorian)")
            << "07:01:04.8 2024-09-01 UTC+02:00" << "hh:mm:ss.z yyyy-MM-dd t" << greg << 2000;

    QTest::newRow("yyyy/MM/dd HH:mm:ss.zz tt(Gregorian)")
            << "2024/09/01 07:01:04.78 +0200" << "yyyy/MM/dd HH:mm:ss.zz tt" << greg << 2000;

    QTest::newRow("H:m:s.zzz yyyy.MM.dd ttt(Gregorian)")
            << "7:1:4.783 2024.09.01 +02:00" << "H:m:s.zzz yyyy.MM.dd ttt" << greg << 2000;

    QTest::newRow("yyyy MM dd HH mm ss.z tttt(Gregorian)")
            << "2024 09 01 07 01 04.8 Europe/Berlin" << "yyyy MM dd HH mm ss.z tttt"
            << greg << 2000;

    QTest::newRow("hh:mm AP ddd dd MMM yyyy(Gregorian)")
            << "07:01 AM Sun 01 Sep 2024" << "hh:mm AP ddd dd MMM yyyy" << greg << 2000;

    QTest::newRow("dddd MMMM dd yy HH:mm:ss ap(Gregorian)")
            << "Sunday September 01 24 19:01:04 pm" << "dddd MMMM dd yy HH:mm:ss ap"
            << greg << 2000;

    QTest::newRow("HH:mm:ss.z d/M/yyyy(Gregorian)")
            << "07:01:04.8 1/9/2024" << "HH:mm:ss.z d/M/yyyy" << greg << 2000;

    QTest::newRow("yyyy-MM-dd h:m:s AP(Gregorian)")
            << "2024-09-01 7:1:4 AM" << "yyyy-MM-dd h:m:s Ap" << greg << 2000;

    QTest::newRow("H:m:s.z dddd dd MMMM yyyy(Gregorian)")
            << "7:1:4.8 Sunday 01 September 2024" << "H:m:s.z dddd dd MMMM yyyy" << greg << 2000;

    QTest::newRow("ddd dd MMM yy hh:mm:ss.zzz(Gregorian)")
            << "Sun 01 Sep 24 07:01:04.783" << "ddd dd MMM yy hh:mm:ss.zzz" << greg << 2000;

    QTest::newRow("hh:mm:ss t d-M-yy(Gregorian)")
            << "07:01:04 UTC+02:00 1-9-24" << "hh:mm:ss t d-M-yy" << greg << 2000;

    QTest::newRow("yyyy/MM/dd HH:mm:ss.ztt(Gregorian)")
            << "2024/09/01 07:01:04.8+0200" << "yyyy/MM/dd HH:mm:ss.ztt" << greg << 2000;

    QTest::newRow("H:m:s.zz dd.MM.yyyy(Gregorian)")
            << "7:1:4.78 01.09.2024" << "H:m:s.zz dd.MM.yyyy" << greg << 2000;

    QTest::newRow("MMMM dd yyyy hh:mm:ss AP(Gregorian)")
            << "September 01 2024 07:01:04 AM" << "MMMM dd yyyy hh:mm:ss AP" << greg << 2000;

    QTest::newRow("hh:mm:ss.zz ddd, dd MMM yyyy(Gregorian)")
            << "07:01:04.78 Sun, 01 Sep 2024" << "hh:mm:ss.zz ddd, dd MMM yyyy" << greg << 2000;

    QTest::newRow("dddd, MMMM dd yy H:m:s.z(Gregorian)")
            << "Sunday, September 01 24 7:1:4.8" << "dddd, MMMM dd yy H:m:s.z" << greg << 2000;

    QTest::newRow("yyyy-MM-ddTHH:mm:ss.zzz(Gregorian)")
            << "2024-09-01T07:01:04.783" << "yyyy-MM-ddTHH:mm:ss.zzz" << greg << 2000;

    QTest::newRow("HH:mm:ss.zzz yyyy-MM-ddT(Gregorian)")
            << "07:01:04.783 2024-09-01T" << "HH:mm:ss.zzz yyyy-MM-ddT" << greg << 2000;

    QTest::newRow("h:m:s d M yy(Gregorian)")
            << "7:1:4 1 9 24" << "h:m:s d M yy" << greg << 2000;

    QTest::newRow("yyyy MM dd HH:mm:ss.zzz(Gregorian)")
            << "2024 09 01 07:01:04.783" << "yyyy MM dd HH:mm:ss.zzz" << greg << 2000;

#if QT_CONFIG(islamiccivilcalendar)
    const QCalendar isci(QCalendar::System::IslamicCivil);

    QTest::newRow("h:m:s.z d.M.yy(IslamicCivil)")
            << "07:01:04.8 1.9.24" << "h:m:s.z d.M.yy" << isci << 2000;

    QTest::newRow("dd/MM/yyyy hh:mm:ss.zzz(IslamicCivil)")
            << "01/09/2024 07:01:04.783" << "dd/MM/yyyy hh:mm:ss.zzz" << isci << 2000;

    QTest::newRow("H:m:s.zz ddd d-MMM-yyyy(IslamicCivil)")
            << "7:1:4.78 Wed 1-Ram.-2024" << "H:m:s.zz ddd d-MMM-yyyy" << isci << 2000;

    QTest::newRow("dddd dd/MMMM/yy HH:mm:ss.z(IslamicCivil)")
            << "Wednesday 01/Ramadan/24 07:01:04.8" << "dddd dd/MMMM/yy HH:mm:ss.z"
            << isci << 2000;

    QTest::newRow("h:m AP M/d/yy(IslamicCivil)")
            << "7:1 AM 9/1/24" << "h:m A M/d/yy" << isci << 2000;

    QTest::newRow("MM-dd-yyyy hh:mm:ss ap(IslamicCivil)")
            << "09-01-2024 07:01:04 pm" << "MM-dd-yyyy hh:mm:ss a" << isci << 2000;

    QTest::newRow("HH:mm:ss.zz yyyy.MM.dd(IslamicCivil)")
            << "07:01:04.78 2024.09.01" << "HH:mm:ss.zz yyyy.MM.dd" << isci << 2000;

    QTest::newRow("yyyy MM dd hh:mm:ss.z aP(IslamicCivil)")
            << "2024 09 01 07:01:04.8 PM" << "yyyy MM dd hh:mm:ss.z aP" << isci << 2000;

    QTest::newRow("hh:mm:ss.zzz dd.MMM.yyyy(IslamicCivil)")
            << "07:01:04.783 01.Ram..2024" << "hh:mm:ss.zzz dd.MMM.yyyy" << isci << 2000;

    QTest::newRow("dddd, dd MMMM yyyy H:m:s(IslamicCivil)")
            << "Wednesday, 01 Ramadan 2024 7:1:4" << "dddd, dd MMMM yyyy H:m:s" << isci << 2000;

    QTest::newRow("hh:mm:ss.z yyyy-MM-dd t(IslamicCivil)")
            << "07:01:04.8 2024-09-01 UTC+02:00" << "hh:mm:ss.z yyyy-MM-dd t" << isci << 2000;

    QTest::newRow("yyyy/MM/dd HH:mm:ss.zz tt(IslamicCivil)")
            << "2024/09/01 07:01:04.78 +0200" << "yyyy/MM/dd HH:mm:ss.zz tt" << isci << 2000;

    QTest::newRow("H:m:s.zzz yyyy.MM.dd ttt(IslamicCivil)")
            << "7:1:4.783 2024.09.01 +02:00" << "H:m:s.zzz yyyy.MM.dd ttt" << isci << 2000;

    QTest::newRow("yyyy MM dd HH mm ss.z tttt(IslamicCivil)")
            << "2024 09 01 07 01 04.8 Europe/Berlin" << "yyyy MM dd HH mm ss.z tttt"
            << isci << 2000;

    QTest::newRow("hh:mm AP ddd dd MMM yyyy(IslamicCivil)")
            << "07:01 AM Wed 01 Ram. 2024" << "hh:mm Ap ddd dd MMM yyyy" << isci << 2000;

    QTest::newRow("dddd MMMM dd yy HH:mm:ss ap(IslamicCivil)")
            << "Wednesday Ramadan 01 24 19:01:04 pm" << "dddd MMMM dd yy HH:mm:ss ap"
            << isci << 2000;

    QTest::newRow("HH:mm:ss.z d/M/yyyy(IslamicCivil)")
            << "07:01:04.8 1/9/2024" << "HH:mm:ss.z d/M/yyyy" << isci << 2000;

    QTest::newRow("yyyy-MM-dd h:m:s AP(IslamicCivil)")
            << "2024-09-01 7:1:4 AM" << "yyyy-MM-dd h:m:s AP" << isci << 2000;

    QTest::newRow("H:m:s.z dddd dd MMMM yyyy(IslamicCivil)")
            << "7:1:4.8 Wednesday 01 Ramadan 2024" << "H:m:s.z dddd dd MMMM yyyy" << isci << 2000;

    QTest::newRow("ddd dd MMM yy hh:mm:ss.zzz(IslamicCivil)")
            << "Wed 01 Ram. 24 07:01:04.783" << "ddd dd MMM yy hh:mm:ss.zzz" << isci << 2000;

    QTest::newRow("hh:mm:ss t d-M-yy(IslamicCivil)")
            << "07:01:04 UTC+02:00 1-9-24" << "hh:mm:ss t d-M-yy" << isci << 2000;

    QTest::newRow("yyyy/MM/dd HH:mm:ss.ztt(IslamicCivil)")
            << "2024/09/01 07:01:04.8+0200" << "yyyy/MM/dd HH:mm:ss.ztt" << isci << 2000;

    QTest::newRow("H:m:s.zz dd.MM.yyyy(IslamicCivil)")
            << "7:1:4.78 01.09.2024" << "H:m:s.zz dd.MM.yyyy" << isci << 2000;

    QTest::newRow("MMMM dd yyyy hh:mm:ss AP(IslamicCivil)")
            << "Ramadan 01 2024 07:01:04 AM" << "MMMM dd yyyy hh:mm:ss AP" << isci << 2000;

    QTest::newRow("hh:mm:ss.zz ddd, dd MMM yyyy(IslamicCivil)")
            << "07:01:04.78 Wed, 01 Ram. 2024" << "hh:mm:ss.zz ddd, dd MMM yyyy" << isci << 2000;

    QTest::newRow("dddd, MMMM dd yy H:m:s.z(IslamicCivil)")
            << "Wednesday, Ramadan 01 24 7:1:4.8" << "dddd, MMMM dd yy H:m:s.z" << isci << 2000;

    QTest::newRow("yyyy-MM-ddTHH:mm:ss.zzz(IslamicCivil)")
            << "2024-09-01T07:01:04.783" << "yyyy-MM-ddTHH:mm:ss.zzz" << isci << 2000;

    QTest::newRow("HH:mm:ss.zzz yyyy-MM-ddT(IslamicCivil)")
            << "07:01:04.783 2024-09-01T" << "HH:mm:ss.zzz yyyy-MM-ddT" << isci << 2000;

    QTest::newRow("h:m:s d M yy(IslamicCivil)")
            << "7:1:4 1 9 24" << "h:m:s d M yy" << isci << 2000;

    QTest::newRow("yyyy MM dd HH:mm:ss.zzz(IslamicCivil)")
            << "2024 09 01 07:01:04.783" << "yyyy MM dd HH:mm:ss.zzz" << isci << 2000;
#endif

#if QT_CONFIG(jalalicalendar)
    const QCalendar jali(QCalendar::System::Jalali);

    QTest::newRow("h:m:s.z d.M.yy(Jalali)")
            << "07:01:04.8 1.9.24" << "h:m:s.z d.M.yy" << jali << 2000;

    QTest::newRow("dd/MM/yyyy hh:mm:ss.zzz(Jalali)")
            << "01/09/2024 07:01:04.783" << "dd/MM/yyyy hh:mm:ss.zzz" << jali << 2000;

    QTest::newRow("H:m:s.zz ddd d-MMM-yyyy(Jalali)")
            << "7:1:4.78 Sat 1-Aza-2024" << "H:m:s.zz ddd d-MMM-yyyy" << jali << 2000;

    QTest::newRow("dddd dd/MMMM/yy HH:mm:ss.z(Jalali)")
            << "Saturday 01/Azar/24 07:01:04.8" << "dddd dd/MMMM/yy HH:mm:ss.z" << jali << 2000;

    QTest::newRow("h:m AP M/d/yy(Jalali)")
            << "7:1 AM 9/1/24" << "h:m A M/d/yy" << jali << 2000;

    QTest::newRow("MM-dd-yyyy hh:mm:ss ap(Jalali)")
            << "09-01-2024 07:01:04 pm" << "MM-dd-yyyy hh:mm:ss a" << jali << 2000;

    QTest::newRow("HH:mm:ss.zz yyyy.MM.dd(Jalali)")
            << "07:01:04.78 2024.09.01" << "HH:mm:ss.zz yyyy.MM.dd" << jali << 2000;

    QTest::newRow("yyyy MM dd hh:mm:ss.z aP(Jalali)")
            << "2024 09 01 07:01:04.8 PM" << "yyyy MM dd hh:mm:ss.z aP" << jali << 2000;

    QTest::newRow("hh:mm:ss.zzz dd.MMM.yyyy(Jalali)")
            << "07:01:04.783 01.Aza.2024" << "hh:mm:ss.zzz dd.MMM.yyyy" << jali << 2000;

    QTest::newRow("dddd, dd MMMM yyyy H:m:s(Jalali)")
            << "Saturday, 01 Azar 2024 7:1:4" << "dddd, dd MMMM yyyy H:m:s" << jali << 2000;

    QTest::newRow("hh:mm:ss.z yyyy-MM-dd t(Jalali)")
            << "07:01:04.8 2024-09-01 UTC+02:00" << "hh:mm:ss.z yyyy-MM-dd t" << jali << 2000;

    QTest::newRow("yyyy/MM/dd HH:mm:ss.zz tt(Jalali)")
            << "2024/09/01 07:01:04.78 +0200" << "yyyy/MM/dd HH:mm:ss.zz tt" << jali << 2000;

    QTest::newRow("H:m:s.zzz yyyy.MM.dd ttt(Jalali)")
            << "7:1:4.783 2024.09.01 +02:00" << "H:m:s.zzz yyyy.MM.dd ttt" << jali << 2000;

    QTest::newRow("yyyy MM dd HH mm ss.z tttt(Jalali)")
            << "2024 09 01 07 01 04.8 Europe/Berlin" << "yyyy MM dd HH mm ss.z tttt"
            << jali << 2000;

    QTest::newRow("hh:mm AP ddd dd MMM yyyy(Jalali)")
            << "07:01 AM Sat 01 Aza 2024" << "hh:mm Ap ddd dd MMM yyyy" << jali << 2000;

    QTest::newRow("dddd MMMM dd yy HH:mm:ss ap(Jalali)")
            << "Saturday Azar 01 24 19:01:04 pm" << "dddd MMMM dd yy HH:mm:ss ap" << jali << 2000;

    QTest::newRow("HH:mm:ss.z d/M/yyyy(Jalali)")
            << "07:01:04.8 1/9/2024" << "HH:mm:ss.z d/M/yyyy" << jali << 2000;

    QTest::newRow("yyyy-MM-dd h:m:s AP(Jalali)")
            << "2024-09-01 7:1:4 AM" << "yyyy-MM-dd h:m:s AP" << jali << 2000;

    QTest::newRow("H:m:s.z dddd dd MMMM yyyy(Jalali)")
            << "7:1:4.8 Saturday 01 Azar 2024" << "H:m:s.z dddd dd MMMM yyyy" << jali << 2000;

    QTest::newRow("ddd dd MMM yy hh:mm:ss.zzz(Jalali)")
            << "Sat 01 Aza 24 07:01:04.783" << "ddd dd MMM yy hh:mm:ss.zzz" << jali << 2000;

    QTest::newRow("hh:mm:ss t d-M-yy(Jalali)")
            << "07:01:04 UTC+02:00 1-9-24" << "hh:mm:ss t d-M-yy" << jali << 2000;

    QTest::newRow("yyyy/MM/dd HH:mm:ss.ztt(Jalali)")
            << "2024/09/01 07:01:04.8+0200" << "yyyy/MM/dd HH:mm:ss.ztt" << jali << 2000;

    QTest::newRow("H:m:s.zz dd.MM.yyyy(Jalali)")
            << "7:1:4.78 01.09.2024" << "H:m:s.zz dd.MM.yyyy" << jali << 2000;

    QTest::newRow("MMMM dd yyyy hh:mm:ss AP(Jalali)")
            << "Azar 01 2024 07:01:04 AM" << "MMMM dd yyyy hh:mm:ss AP" << jali << 2000;

    QTest::newRow("hh:mm:ss.zz ddd, dd MMM yyyy(Jalali)")
            << "07:01:04.78 Sat, 01 Aza 2024" << "hh:mm:ss.zz ddd, dd MMM yyyy" << jali << 2000;

    QTest::newRow("dddd, MMMM dd yy H:m:s.z(Jalali)")
            << "Saturday, Azar 01 24 7:1:4.8" << "dddd, MMMM dd yy H:m:s.z" << jali << 2000;

    QTest::newRow("yyyy-MM-ddTHH:mm:ss.zzz(Jalali)")
            << "2024-09-01T07:01:04.783" << "yyyy-MM-ddTHH:mm:ss.zzz" << jali << 2000;

    QTest::newRow("HH:mm:ss.zzz yyyy-MM-ddT(Jalali)")
            << "07:01:04.783 2024-09-01T" << "HH:mm:ss.zzz yyyy-MM-ddT" << jali << 2000;

    QTest::newRow("h:m:s d M yy(Jalali)")
            << "7:1:4 1 9 24" << "h:m:s d M yy" << jali << 2000;

    QTest::newRow("yyyy MM dd HH:mm:ss.zzz(Jalali)")
            << "2024 09 01 07:01:04.783" << "yyyy MM dd HH:mm:ss.zzz" << jali << 2000;
#endif

    // After 2038
    QTest::newRow("post-2038")
            << "2040-01-01 00:00:00" << "yyyy-MM-dd hh:mm:ss" << greg << 2000;

    // Before 1900
    QTest::newRow("pre-1900")
            << "1850-06-15 10:20:30" << "yyyy-MM-dd hh:mm:ss" << greg << 2000;

    // After 3000
    QTest::newRow("post-3000")
            << "3500-01-01 08:00:00" << "yyyy-MM-dd hh:mm:ss" << greg << 2000;

    // DST spring-forward gap (nonexistent local time)
    QTest::newRow("dst-spring-forward")
            << "2024-03-31 02:30:00" << "yyyy-MM-dd hh:mm:ss" << greg << 2000;

    // DST fall-back ambiguous time
    QTest::newRow("dst-fall-back")
            << "2024-10-27 02:30:00" << "yyyy-MM-dd hh:mm:ss" << greg << 1900;
}

void tst_QDateTime::fromString()
{
    QFETCH(QString, input);
    QFETCH(QString, format);
    QFETCH(QCalendar, calendar);
    QFETCH(int, baseYear);

    QDateTime dt;
    QBENCHMARK {
        dt = QDateTime::fromString(input, format, baseYear, calendar);
    }
    QVERIFY(dt.isValid());
}

void tst_QDateTime::fromStringText()
{
    QString input = "Wed Jan 2 01:02:03.000 2013 GMT";
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::fromString(input, Qt::TextDate);
    }
}

void tst_QDateTime::fromStringIso()
{
    QString input = "2010-01-01T13:28:34.999Z";
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::fromString(input, Qt::ISODate);
    }
}

void tst_QDateTime::fromMSecsSinceEpoch()
{
    const int start = JULIAN_DAY_2010 - JULIAN_DAY_1970;
    const int end = JULIAN_DAY_2020 - JULIAN_DAY_1970;
    QBENCHMARK {
        for (int jd = start; jd < end; ++jd)
            QDateTime::fromMSecsSinceEpoch(jd * MSECS_PER_DAY);
    }
}

void tst_QDateTime::fromMSecsSinceEpochUtc()
{
    const int start = JULIAN_DAY_2010 - JULIAN_DAY_1970;
    const int end = JULIAN_DAY_2020 - JULIAN_DAY_1970;
    QBENCHMARK {
        for (int jd = start; jd < end; ++jd)
            QDateTime::fromMSecsSinceEpoch(jd * MSECS_PER_DAY, QTimeZone::UTC);
    }
}
#if QT_CONFIG(timezone)
void tst_QDateTime::fromMSecsSinceEpochTz()
{
    const int start = JULIAN_DAY_2010 - JULIAN_DAY_1970;
    const int end = JULIAN_DAY_2020 - JULIAN_DAY_1970;
    const QTimeZone cet("Europe/Oslo");
    QBENCHMARK {
        for (int jd = start; jd < end; ++jd)
            QDateTime test = QDateTime::fromMSecsSinceEpoch(jd * MSECS_PER_DAY, cet);
    }
}
#endif

QTEST_MAIN(tst_QDateTime)

#include "tst_bench_qdatetime.moc"
