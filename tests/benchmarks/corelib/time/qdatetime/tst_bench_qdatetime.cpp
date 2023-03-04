// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    static QList<QDateTime> norse(qint64 start, qint64 end);
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
    void toMSecsSinceEpochTz_data() { decade_data(); }
    void toMSecsSinceEpochTz();
    void setDate();
    void setTime();
#if QT_DEPRECATED_SINCE(6, 9)
    void setTimeSpec();
    void setOffsetFromUtc();
#endif
    void setMSecsSinceEpoch();
    void setMSecsSinceEpochTz();
    void toString();
    void toStringTextFormat();
    void toStringIsoFormat();
    void addDays();
    void addDaysTz();
    void addMSecs();
    void addMSecsTz();
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
    void fromStringText();
    void fromStringIso();
    void fromMSecsSinceEpoch();
    void fromMSecsSinceEpochUtc();
    void fromMSecsSinceEpochTz();
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

QList<QDateTime> tst_QDateTime::norse(qint64 start, qint64 end)
{
    const QTimeZone cet("Europe/Oslo");
    QList<QDateTime> list;
    list.reserve(end - start);
    for (int jd = start; jd < end; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd).startOfDay(cet)));
    return list;
}

void tst_QDateTime::create()
{
    QFETCH(const qint64, startJd);
    QFETCH(const qint64, stopJd);
    const QTime noon = QTime::fromMSecsSinceStartOfDay(43200);
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

void tst_QDateTime::setMSecsSinceEpochTz()
{
    const qint64 msecs = qint64(JULIAN_DAY_2010 - JULIAN_DAY_1970 + 180) * MSECS_PER_DAY;
    const auto list = norse(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (QDateTime test : list)
            test.setMSecsSinceEpoch(msecs);
    }
}

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

void tst_QDateTime::addDaysTz()
{
    const auto list = norse(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            QDateTime result = test.addDays(1);
    }
}

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

void tst_QDateTime::addMSecsTz()
{
    const auto list = norse(JULIAN_DAY_2010, JULIAN_DAY_2020);
    QBENCHMARK {
        for (const QDateTime &test : list)
            QDateTime result = test.addMSecs(1);
    }
}

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

void tst_QDateTime::fromString()
{
    QString format = "yyyy-MM-dd hh:mm:ss.zzz";
    QString input = "2010-01-01 13:12:11.999";
    QVERIFY(QDateTime::fromString(input, format).isValid());
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::fromString(input, format);
    }
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

QTEST_MAIN(tst_QDateTime)

#include "tst_bench_qdatetime.moc"
