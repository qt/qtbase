/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#ifndef Q_OS_WINCE
#include <time.h>
#endif

#include <qdatetime.h>
#include <private/qdatetime_p.h>

#ifdef Q_OS_WIN
# include <windows.h>
#endif

class tst_QDateTime : public QObject
{
    Q_OBJECT

public:
    tst_QDateTime();

    static QString str( int y, int month, int d, int h, int min, int s );
    static QDateTime dt( const QString& str );
public slots:
    void init();
private slots:
    void ctor();
    void operator_eq();
    void isNull();
    void isValid();
    void date();
    void time();
    void timeSpec();
    void toTime_t_data();
    void toTime_t();
    void daylightSavingsTimeChange();
    void setDate();
    void setTime_data();
    void setTime();
    void setTimeSpec_data();
    void setTimeSpec();
    void setTime_t();
    void setMSecsSinceEpoch_data();
    void setMSecsSinceEpoch();
    void fromMSecsSinceEpoch_data();
    void fromMSecsSinceEpoch();
    void toString_isoDate_data();
    void toString_isoDate();
    void toString_enumformat();
    void toString_strformat_data();
    void toString_strformat();
    void addDays();
    void addMonths();
    void addMonths_data();
    void addYears();
    void addYears_data();
    void addSecs_data();
    void addSecs();
    void addMSecs_data();
    void addMSecs();
    void toTimeSpec_data();
    void toTimeSpec();
    void toLocalTime_data();
    void toLocalTime();
    void toUTC_data();
    void toUTC();
    void daysTo();
    void secsTo_data();
    void secsTo();
    void msecsTo_data();
    void msecsTo();
    void operator_eqeq_data();
    void operator_eqeq();
#ifndef Q_OS_WINCE
    void operator_insert_extract_data();
    void operator_insert_extract();
#endif
    void currentDateTime();
    void currentDateTimeUtc();
    void currentDateTimeUtc2();
    void fromStringDateFormat_data();
    void fromStringDateFormat();
    void fromStringStringFormat_data();
    void fromStringStringFormat();
#ifdef Q_OS_WIN
    void fromString_LOCALE_ILDATE();
#endif
    void fromStringToStringLocale_data();
    void fromStringToStringLocale();

    void utcOffset();
    void setUtcOffset();

    void getDate();

    void fewDigitsInYear() const;
    void printNegativeYear() const;
    void roundtripGermanLocale() const;
    void utcOffsetLessThan() const;

private:
    bool europeanTimeZone;
    QDate defDate() const { return QDate(1900, 1, 1); }
    QTime defTime() const { return QTime(0, 0, 0); }
    QDateTime defDateTime() const { return QDateTime(defDate(), defTime()); }
    QDateTime invalidDateTime() const { return QDateTime(invalidDate(), invalidTime()); }
    QDate invalidDate() const { return QDate(); }
    QTime invalidTime() const { return QTime(-1, -1, -1); }
    qint64 minJd() const { return QDateTimePrivate::minJd(); }
    qint64 maxJd() const { return QDateTimePrivate::maxJd(); }
};

Q_DECLARE_METATYPE(Qt::TimeSpec)
Q_DECLARE_METATYPE(Qt::DateFormat)

tst_QDateTime::tst_QDateTime()
{
    uint x1 = QDateTime(QDate(1990, 1, 1), QTime()).toTime_t();
    uint x2 = QDateTime(QDate(1990, 6, 1), QTime()).toTime_t();
    europeanTimeZone = (x1 == 631148400 && x2 == 644191200);
}

void tst_QDateTime::init()
{
#if defined(Q_OS_WINCE)
    SetUserDefaultLCID(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
#elif defined(Q_OS_WIN)
    SetThreadLocale(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
#endif
}

QString tst_QDateTime::str( int y, int month, int d, int h, int min, int s )
{
    return QDateTime( QDate(y, month, d), QTime(h, min, s) ).toString( Qt::ISODate );
}

QDateTime tst_QDateTime::dt( const QString& str )
{
    if ( str == "INVALID" ) {
        return QDateTime();
    } else {
        return QDateTime::fromString( str, Qt::ISODate );
    }
}

void tst_QDateTime::ctor()
{
    QDateTime dt1(QDate(2004, 1, 2), QTime(1, 2, 3));
    QDateTime dt2(QDate(2004, 1, 2), QTime(1, 2, 3), Qt::LocalTime);
    QDateTime dt3(QDate(2004, 1, 2), QTime(1, 2, 3), Qt::UTC);

    QVERIFY(dt1 == dt2);
    if (europeanTimeZone) {
        QVERIFY(dt1 != dt3);
        QVERIFY(dt1 < dt3);
        QVERIFY(dt1.addSecs(3600).toUTC() == dt3);
    }
}

void tst_QDateTime::operator_eq()
{
    QDateTime dt1(QDate(2004, 3, 24), QTime(23, 45, 57), Qt::UTC);
    QDateTime dt2(QDate(2005, 3, 11), QTime(), Qt::UTC);
    dt2 = dt1;
    QVERIFY(dt1 == dt2);
}

void tst_QDateTime::isNull()
{
    QDateTime dt1;
    QVERIFY(dt1.isNull());
    dt1.setDate(QDate());
    QVERIFY(dt1.isNull());
    dt1.setTime(QTime());
    QVERIFY(dt1.isNull());
    dt1.setTimeSpec(Qt::UTC);
    QVERIFY(dt1.isNull());   // maybe it should return false?

    dt1.setDate(QDate(2004, 1, 2));
    QVERIFY(!dt1.isNull());
    dt1.setTime(QTime(12, 34, 56));
    QVERIFY(!dt1.isNull());
    dt1.setTime(QTime());
    QVERIFY(!dt1.isNull());
}

void tst_QDateTime::isValid()
{
    QDateTime dt1;
    QVERIFY(!dt1.isValid());
    dt1.setDate(QDate());
    QVERIFY(!dt1.isValid());
    dt1.setTime(QTime());
    QVERIFY(!dt1.isValid());
    dt1.setTimeSpec(Qt::UTC);
    QVERIFY(!dt1.isValid());

    dt1.setDate(QDate(2004, 1, 2));
    QVERIFY(dt1.isValid());
    dt1.setDate(QDate());
    QVERIFY(!dt1.isValid());
    dt1.setTime(QTime(12, 34, 56));
    QVERIFY(!dt1.isValid());
    dt1.setTime(QTime());
    QVERIFY(!dt1.isValid());
}

void tst_QDateTime::date()
{
    QDateTime dt1(QDate(2004, 3, 24), QTime(23, 45, 57), Qt::LocalTime);
    QCOMPARE(dt1.date(), QDate(2004, 3, 24));

    QDateTime dt2(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::LocalTime);
    QCOMPARE(dt2.date(), QDate(2004, 3, 25));

    QDateTime dt3(QDate(2004, 3, 24), QTime(23, 45, 57), Qt::UTC);
    QCOMPARE(dt3.date(), QDate(2004, 3, 24));

    QDateTime dt4(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC);
    QCOMPARE(dt4.date(), QDate(2004, 3, 25));
}

void tst_QDateTime::time()
{
    QDateTime dt1(QDate(2004, 3, 24), QTime(23, 45, 57), Qt::LocalTime);
    QCOMPARE(dt1.time(), QTime(23, 45, 57));

    QDateTime dt2(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::LocalTime);
    QCOMPARE(dt2.time(), QTime(0, 45, 57));

    QDateTime dt3(QDate(2004, 3, 24), QTime(23, 45, 57), Qt::UTC);
    QCOMPARE(dt3.time(), QTime(23, 45, 57));

    QDateTime dt4(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC);
    QCOMPARE(dt4.time(), QTime(0, 45, 57));
}

void tst_QDateTime::timeSpec()
{
    QDateTime dt1(QDate(2004, 1, 24), QTime(23, 45, 57));
    QCOMPARE(dt1.timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addDays(0).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addMonths(0).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addMonths(6).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addYears(0).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addSecs(0).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addSecs(86400 * 185).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.toTimeSpec(Qt::LocalTime).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.toTimeSpec(Qt::UTC).timeSpec(), Qt::UTC);

    QDateTime dt2(QDate(2004, 1, 24), QTime(23, 45, 57), Qt::LocalTime);
    QCOMPARE(dt2.timeSpec(), Qt::LocalTime);

    QDateTime dt3(QDate(2004, 1, 25), QTime(0, 45, 57), Qt::UTC);
    QCOMPARE(dt3.timeSpec(), Qt::UTC);

    QDateTime dt4 = QDateTime::currentDateTime();
    QCOMPARE(dt4.timeSpec(), Qt::LocalTime);
}

void tst_QDateTime::setDate()
{
    QDateTime dt1(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC);
    dt1.setDate(QDate(2004, 6, 25));
    QCOMPARE(dt1.date(), QDate(2004, 6, 25));
    QCOMPARE(dt1.time(), QTime(0, 45, 57));
    QCOMPARE(dt1.timeSpec(), Qt::UTC);

    QDateTime dt2(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::LocalTime);
    dt2.setDate(QDate(2004, 6, 25));
    QCOMPARE(dt2.date(), QDate(2004, 6, 25));
    QCOMPARE(dt2.time(), QTime(0, 45, 57));
    QCOMPARE(dt2.timeSpec(), Qt::LocalTime);

    QDateTime dt3(QDate(4004, 3, 25), QTime(0, 45, 57), Qt::UTC);
    dt3.setDate(QDate(4004, 6, 25));
    QCOMPARE(dt3.date(), QDate(4004, 6, 25));
    QCOMPARE(dt3.time(), QTime(0, 45, 57));
    QCOMPARE(dt3.timeSpec(), Qt::UTC);

    QDateTime dt4(QDate(4004, 3, 25), QTime(0, 45, 57), Qt::LocalTime);
    dt4.setDate(QDate(4004, 6, 25));
    QCOMPARE(dt4.date(), QDate(4004, 6, 25));
    QCOMPARE(dt4.time(), QTime(0, 45, 57));
    QCOMPARE(dt4.timeSpec(), Qt::LocalTime);

    QDateTime dt5(QDate(1760, 3, 25), QTime(0, 45, 57), Qt::UTC);
    dt5.setDate(QDate(1760, 6, 25));
    QCOMPARE(dt5.date(), QDate(1760, 6, 25));
    QCOMPARE(dt5.time(), QTime(0, 45, 57));
    QCOMPARE(dt5.timeSpec(), Qt::UTC);

    QDateTime dt6(QDate(1760, 3, 25), QTime(0, 45, 57), Qt::LocalTime);
    dt6.setDate(QDate(1760, 6, 25));
    QCOMPARE(dt6.date(), QDate(1760, 6, 25));
    QCOMPARE(dt6.time(), QTime(0, 45, 57));
    QCOMPARE(dt6.timeSpec(), Qt::LocalTime);
}

void tst_QDateTime::setTime_data()
{
    QTest::addColumn<QDateTime>("dateTime");
    QTest::addColumn<QTime>("newTime");

    QTest::newRow("data0") << QDateTime(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC) << QTime(23, 11, 22);
    QTest::newRow("data1") << QDateTime(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::LocalTime) << QTime(23, 11, 22);
    QTest::newRow("data2") << QDateTime(QDate(4004, 3, 25), QTime(0, 45, 57), Qt::UTC) << QTime(23, 11, 22);
    QTest::newRow("data3") << QDateTime(QDate(4004, 3, 25), QTime(0, 45, 57), Qt::LocalTime) << QTime(23, 11, 22);
    QTest::newRow("data4") << QDateTime(QDate(1760, 3, 25), QTime(0, 45, 57), Qt::UTC) << QTime(23, 11, 22);
    QTest::newRow("data5") << QDateTime(QDate(1760, 3, 25), QTime(0, 45, 57), Qt::LocalTime) << QTime(23, 11, 22);

    QTest::newRow("set on std/dst") << QDateTime::currentDateTime() << QTime(23, 11, 22);
}

void tst_QDateTime::setTime()
{
    QFETCH(QDateTime, dateTime);
    QFETCH(QTime, newTime);

    const QDate expectedDate(dateTime.date());
    const Qt::TimeSpec expectedTimeSpec(dateTime.timeSpec());

    dateTime.setTime(newTime);

    QCOMPARE(dateTime.date(), expectedDate);
    QCOMPARE(dateTime.time(), newTime);
    QCOMPARE(dateTime.timeSpec(), expectedTimeSpec);
}

void tst_QDateTime::setTimeSpec_data()
{
    QTest::addColumn<QDateTime>("dateTime");
    QTest::addColumn<Qt::TimeSpec>("newTimeSpec");

    QTest::newRow("UTC => UTC") << QDateTime(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC) << Qt::UTC;
    QTest::newRow("UTC => LocalTime") << QDateTime(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC) << Qt::LocalTime;
    QTest::newRow("UTC => OffsetFromUTC") << QDateTime(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC) << Qt::OffsetFromUTC;
}

void tst_QDateTime::setTimeSpec()
{
    QFETCH(QDateTime, dateTime);
    QFETCH(Qt::TimeSpec, newTimeSpec);

    const QDate expectedDate(dateTime.date());
    const QTime expectedTime(dateTime.time());

    dateTime.setTimeSpec(newTimeSpec);

    QCOMPARE(dateTime.date(), expectedDate);
    QCOMPARE(dateTime.time(), expectedTime);
    QCOMPARE(dateTime.timeSpec(), newTimeSpec);
}

void tst_QDateTime::setTime_t()
{
    QDateTime dt1;
    dt1.setTime_t(0);
    QCOMPARE(dt1.toUTC(), QDateTime(QDate(1970, 1, 1), QTime(), Qt::UTC));

    dt1.setTimeSpec(Qt::UTC);
    dt1.setTime_t(0);
    QCOMPARE(dt1, QDateTime(QDate(1970, 1, 1), QTime(), Qt::UTC));

    dt1.setTime_t(123456);
    QCOMPARE(dt1, QDateTime(QDate(1970, 1, 2), QTime(10, 17, 36), Qt::UTC));
    if (europeanTimeZone) {
        QDateTime dt2;
        dt2.setTime_t(123456);
        QCOMPARE(dt2, QDateTime(QDate(1970, 1, 2), QTime(11, 17, 36), Qt::LocalTime));
    }

    dt1.setTime_t((uint)(quint32)-123456);
    QCOMPARE(dt1, QDateTime(QDate(2106, 2, 5), QTime(20, 10, 40), Qt::UTC));
    if (europeanTimeZone) {
        QDateTime dt2;
        dt2.setTime_t((uint)(quint32)-123456);
        QCOMPARE(dt2, QDateTime(QDate(2106, 2, 5), QTime(21, 10, 40), Qt::LocalTime));
    }

    dt1.setTime_t(1214567890);
    QCOMPARE(dt1, QDateTime(QDate(2008, 6, 27), QTime(11, 58, 10), Qt::UTC));
    if (europeanTimeZone) {
        QDateTime dt2;
        dt2.setTime_t(1214567890);
        QCOMPARE(dt2, QDateTime(QDate(2008, 6, 27), QTime(13, 58, 10), Qt::LocalTime));
    }

    dt1.setTime_t(0x7FFFFFFF);
    QCOMPARE(dt1, QDateTime(QDate(2038, 1, 19), QTime(3, 14, 7), Qt::UTC));
    if (europeanTimeZone) {
        QDateTime dt2;
        dt2.setTime_t(0x7FFFFFFF);
        QCOMPARE(dt2, QDateTime(QDate(2038, 1, 19), QTime(4, 14, 7), Qt::LocalTime));
    }
}

void tst_QDateTime::setMSecsSinceEpoch_data()
{
    QTest::addColumn<qint64>("msecs");
    QTest::addColumn<QDateTime>("utc");
    QTest::addColumn<QDateTime>("european");

    QTest::newRow("zero")
            << Q_INT64_C(0)
            << QDateTime(QDate(1970, 1, 1), QTime(), Qt::UTC)
            << QDateTime(QDate(1970, 1, 1), QTime(1, 0));
    QTest::newRow("-1")
            << Q_INT64_C(-1)
            << QDateTime(QDate(1969, 12, 31), QTime(23, 59, 59, 999), Qt::UTC)
            << QDateTime(QDate(1970, 1, 1), QTime(0, 59, 59, 999));
    QTest::newRow("123456789")
            << Q_INT64_C(123456789)
            << QDateTime(QDate(1970, 1, 2), QTime(10, 17, 36, 789), Qt::UTC)
            << QDateTime(QDate(1970, 1, 2), QTime(11, 17, 36, 789), Qt::LocalTime);
    QTest::newRow("-123456789")
            << Q_INT64_C(-123456789)
            << QDateTime(QDate(1969, 12, 30), QTime(13, 42, 23, 211), Qt::UTC)
            << QDateTime(QDate(1969, 12, 30), QTime(14, 42, 23, 211), Qt::LocalTime);
    QTest::newRow("non-time_t")
            << (Q_INT64_C(1000) << 32)
            << QDateTime(QDate(2106, 2, 7), QTime(6, 28, 16), Qt::UTC)
            << QDateTime(QDate(2106, 2, 7), QTime(7, 28, 16));
    QTest::newRow("very-large")
            << (Q_INT64_C(123456) << 32)
            << QDateTime(QDate(18772, 8, 15), QTime(1, 8, 14, 976), Qt::UTC)
            << QDateTime(QDate(18772, 8, 15), QTime(3, 8, 14, 976));
    QTest::newRow("old min (Tue Nov 25 00:00:00 -4714)")
            << Q_INT64_C(-210866716800000)
            << QDateTime(QDate::fromJulianDay(1), QTime(), Qt::UTC)
            << QDateTime(QDate::fromJulianDay(1), QTime(1, 0));
    QTest::newRow("old max (Tue Jun 3 21:59:59 5874898)")
            << Q_INT64_C(185331720376799999)
            << QDateTime(QDate::fromJulianDay(0x7fffffff), QTime(21, 59, 59, 999), Qt::UTC)
            << QDateTime(QDate::fromJulianDay(0x7fffffff), QTime(23, 59, 59, 999));
    QTest::newRow("min")
            // + 1 because, in the reference check below, calling addMSecs(qint64min)
            // will internally apply unary minus to -qint64min, resulting in a
            // positive value 1 too big for qint64max, causing an overflow.
            << std::numeric_limits<qint64>::min() + 1
            << QDateTime(QDate(-292275056, 5, 16), QTime(16, 47, 4, 193), Qt::UTC)
#ifdef Q_OS_WIN
            // Windows applies Daylight Time to dates before 1980, Olsen does not
            << QDateTime(QDate(-292275056, 5, 16), QTime(18, 47, 4, 193), Qt::LocalTime);
#else
            << QDateTime(QDate(-292275056, 5, 16), QTime(17, 47, 4, 193), Qt::LocalTime);
#endif
    QTest::newRow("max")
            << std::numeric_limits<qint64>::max()
            << QDateTime(QDate(292278994, 8, 17), QTime(7, 12, 55, 807), Qt::UTC)
            << QDateTime(QDate(292278994, 8, 17), QTime(9, 12, 55, 807), Qt::LocalTime);
}

void tst_QDateTime::setMSecsSinceEpoch()
{
    QFETCH(qint64, msecs);
    QFETCH(QDateTime, utc);
    QFETCH(QDateTime, european);

    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setMSecsSinceEpoch(msecs);

    QCOMPARE(dt, utc);
    if (europeanTimeZone) {
        QCOMPARE(dt.toLocalTime(), european);

        // Test converting from LocalTime to UTC back to LocalTime.
        QDateTime localDt;
        localDt.setTimeSpec(Qt::LocalTime);
        localDt.setMSecsSinceEpoch(msecs);

        QCOMPARE(localDt, utc);
    }

    QCOMPARE(dt.toMSecsSinceEpoch(), msecs);

    if (quint64(msecs / 1000) < 0xFFFFFFFF) {
        QCOMPARE(qint64(dt.toTime_t()), msecs / 1000);
    }

    QDateTime reference(QDate(1970, 1, 1), QTime(), Qt::UTC);
    QCOMPARE(dt, reference.addMSecs(msecs));
}

void tst_QDateTime::fromMSecsSinceEpoch_data()
{
    setMSecsSinceEpoch_data();
}

void tst_QDateTime::fromMSecsSinceEpoch()
{
    QFETCH(qint64, msecs);
    QFETCH(QDateTime, utc);
    QFETCH(QDateTime, european);

    QDateTime dt(QDateTime::fromMSecsSinceEpoch(msecs));

    QCOMPARE(dt, utc);
    if (europeanTimeZone)
        QCOMPARE(dt.toLocalTime(), european);

    QCOMPARE(dt.toMSecsSinceEpoch(), msecs);

    if (quint64(msecs / 1000) < 0xFFFFFFFF) {
        QCOMPARE(qint64(dt.toTime_t()), msecs / 1000);
    }

    QDateTime reference(QDate(1970, 1, 1), QTime(), Qt::UTC);
    QCOMPARE(dt, reference.addMSecs(msecs));
}

void tst_QDateTime::toString_isoDate_data()
{
    QTest::addColumn<QDateTime>("dt");
    QTest::addColumn<QString>("formatted");

    QTest::newRow("localtime")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34))
            << QString("1978-11-09T13:28:34");
    QTest::newRow("UTC")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34), Qt::UTC)
            << QString("1978-11-09T13:28:34Z");
    QDateTime dt(QDate(1978, 11, 9), QTime(13, 28, 34));
    dt.setUtcOffset(19800);
    QTest::newRow("positive OffsetFromUTC")
            << dt
            << QString("1978-11-09T13:28:34+05:30");
    dt.setUtcOffset(-7200);
    QTest::newRow("negative OffsetFromUTC")
            << dt
            << QString("1978-11-09T13:28:34-02:00");
    QTest::newRow("invalid")
            << QDateTime(QDate(-1, 11, 9), QTime(13, 28, 34), Qt::UTC)
            << QString();
}

void tst_QDateTime::toString_isoDate()
{
    QFETCH(QDateTime, dt);
    QFETCH(QString, formatted);

    QCOMPARE(dt.toString(Qt::ISODate), formatted);
}

void tst_QDateTime::toString_enumformat()
{
    QDateTime dt1(QDate(1995, 5, 20), QTime(12, 34, 56));


    QString str1 = dt1.toString(Qt::TextDate);
    QVERIFY(!str1.isEmpty()); // It's locale dependent everywhere

    QString str2 = dt1.toString(Qt::ISODate);
    QCOMPARE(str2, QString("1995-05-20T12:34:56"));

    QString str3 = dt1.toString(Qt::LocalDate);
    QVERIFY(!str3.isEmpty());
    //check for date/time components in any order
    //year may be 2 or 4 digits
    QVERIFY(str3.contains("95"));
    //day and month may be in numeric or word form
    QVERIFY(str3.contains("12"));
    QVERIFY(str3.contains("34"));
    //seconds may be absent
}

void tst_QDateTime::addDays()
{
    for (int pass = 0; pass < 2; ++pass) {
        QDateTime dt(QDate(2004, 1, 1), QTime(12, 34, 56), pass == 0 ? Qt::LocalTime : Qt::UTC);
        dt = dt.addDays(185);
        QVERIFY(dt.date().year() == 2004 && dt.date().month() == 7 && dt.date().day() == 4);
        QVERIFY(dt.time().hour() == 12 && dt.time().minute() == 34 && dt.time().second() == 56
               && dt.time().msec() == 0);
        QCOMPARE(dt.timeSpec(), (pass == 0 ? Qt::LocalTime : Qt::UTC));

        dt = dt.addDays(-185);
        QCOMPARE(dt.date(), QDate(2004, 1, 1));
        QCOMPARE(dt.time(), QTime(12, 34, 56));
    }

    QDateTime dt(QDate(1752, 9, 14), QTime(12, 34, 56));
    while (dt.date().year() < 8000) {
        int year = dt.date().year();
        if (QDate::isLeapYear(year + 1))
            dt = dt.addDays(366);
        else
            dt = dt.addDays(365);
        QCOMPARE(dt.date(), QDate(year + 1, 9, 14));
        QCOMPARE(dt.time(), QTime(12, 34, 56));
    }

    // ### test invalid QDateTime()
}


void tst_QDateTime::addMonths_data()
{
    QTest::addColumn<int>("months");
    QTest::addColumn<QDate>("dt");

    QTest::newRow("-15") << -15 << QDate(2002, 10, 31);
    QTest::newRow("-14") << -14 << QDate(2002, 11, 30);
    QTest::newRow("-13") << -13 << QDate(2002, 12, 31);
    QTest::newRow("-12") << -12 << QDate(2003, 1, 31);

    QTest::newRow("-11") << -11 << QDate(2003, 2, 28);
    QTest::newRow("-10") << -10 << QDate(2003, 3, 31);
    QTest::newRow("-9") << -9 << QDate(2003, 4, 30);
    QTest::newRow("-8") << -8 << QDate(2003, 5, 31);
    QTest::newRow("-7") << -7 << QDate(2003, 6, 30);
    QTest::newRow("-6") << -6 << QDate(2003, 7, 31);
    QTest::newRow("-5") << -5 << QDate(2003, 8, 31);
    QTest::newRow("-4") << -4 << QDate(2003, 9, 30);
    QTest::newRow("-3") << -3 << QDate(2003, 10, 31);
    QTest::newRow("-2") << -2 << QDate(2003, 11, 30);
    QTest::newRow("-1") << -1 << QDate(2003, 12, 31);
    QTest::newRow("0") << 0 << QDate(2004, 1, 31);
    QTest::newRow("1") << 1 << QDate(2004, 2, 29);
    QTest::newRow("2") << 2 << QDate(2004, 3, 31);
    QTest::newRow("3") << 3 << QDate(2004, 4, 30);
    QTest::newRow("4") << 4 << QDate(2004, 5, 31);
    QTest::newRow("5") << 5 << QDate(2004, 6, 30);
    QTest::newRow("6") << 6 << QDate(2004, 7, 31);
    QTest::newRow("7") << 7 << QDate(2004, 8, 31);
    QTest::newRow("8") << 8 << QDate(2004, 9, 30);
    QTest::newRow("9") << 9 << QDate(2004, 10, 31);
    QTest::newRow("10") << 10 << QDate(2004, 11, 30);
    QTest::newRow("11") << 11 << QDate(2004, 12, 31);
    QTest::newRow("12") << 12 << QDate(2005, 1, 31);
    QTest::newRow("13") << 13 << QDate(2005, 2, 28);
    QTest::newRow("14") << 14 << QDate(2005, 3, 31);
    QTest::newRow("15") << 15 << QDate(2005, 4, 30);
}

void tst_QDateTime::addMonths()
{
    QFETCH(QDate, dt);
    QFETCH(int, months);

    QDateTime start(QDate(2004, 1, 31), QTime(12, 34, 56));
    QCOMPARE(start.addMonths(months).date(), dt);
    QCOMPARE(start.addMonths(months).time(), QTime(12, 34, 56));
}

void tst_QDateTime::addYears_data()
{
    QTest::addColumn<int>("years1");
    QTest::addColumn<int>("years2");
    QTest::addColumn<QDate>("start");
    QTest::addColumn<QDate>("dt");

    QTest::newRow("0") << 0 << 0 << QDate(1752, 9, 14) << QDate(1752, 9, 14);
    QTest::newRow("4000 - 4000") << 4000 << -4000 << QDate(1752, 9, 14) << QDate(1752, 9, 14);
    QTest::newRow("10") << 10 << 0 << QDate(1752, 9, 14) << QDate(1762, 9, 14);
    QTest::newRow("0 leap year") << 0 << 0 << QDate(1760, 2, 29) << QDate(1760, 2, 29);
    QTest::newRow("1 leap year") << 1 << 0 << QDate(1760, 2, 29) << QDate(1761, 2, 28);
    QTest::newRow("2 leap year") << 2 << 0 << QDate(1760, 2, 29) << QDate(1762, 2, 28);
    QTest::newRow("3 leap year") << 3 << 0 << QDate(1760, 2, 29) << QDate(1763, 2, 28);
    QTest::newRow("4 leap year") << 4 << 0 << QDate(1760, 2, 29) << QDate(1764, 2, 29);

    QTest::newRow("toNegative1") << -2000 << 0 << QDate(1752, 9, 14) << QDate(-249, 9, 14);
    QTest::newRow("toNegative2") << -1752 << 0 << QDate(1752, 9, 14) << QDate(-1, 9, 14);
    QTest::newRow("toNegative3") << -1751 << 0 << QDate(1752, 9, 14) << QDate(1, 9, 14);
    QTest::newRow("toPositive1") << 2000 << 0 << QDate(-1752, 9, 14) << QDate(249, 9, 14);
    QTest::newRow("toPositive2") << 1752 << 0 << QDate(-1752, 9, 14) << QDate(1, 9, 14);
    QTest::newRow("toPositive3") << 1751 << 0 << QDate(-1752, 9, 14) << QDate(-1, 9, 14);
}

void tst_QDateTime::addYears()
{
    QFETCH(int, years1);
    QFETCH(int, years2);
    QFETCH(QDate, start);
    QFETCH(QDate, dt);

    QDateTime startdt(start, QTime(14, 25, 36));
    QCOMPARE(startdt.addYears(years1).addYears(years2).date(), dt);
    QCOMPARE(startdt.addYears(years1).addYears(years2).time(), QTime(14, 25, 36));
}

void tst_QDateTime::addSecs_data()
{
    QTest::addColumn<QDateTime>("dt");
    QTest::addColumn<int>("nsecs");
    QTest::addColumn<QDateTime>("result");

    QTime standardTime(12, 34, 56);
    QTime daylightTime(13, 34, 56);

    QTest::newRow("utc0") << QDateTime(QDate(2004, 1, 1), standardTime, Qt::UTC) << 86400
                       << QDateTime(QDate(2004, 1, 2), standardTime, Qt::UTC);
    QTest::newRow("utc1") << QDateTime(QDate(2004, 1, 1), standardTime, Qt::UTC) << (86400 * 185)
                       << QDateTime(QDate(2004, 7, 4), standardTime, Qt::UTC);
    QTest::newRow("utc2") << QDateTime(QDate(2004, 1, 1), standardTime, Qt::UTC) << (86400 * 366)
                       << QDateTime(QDate(2005, 1, 1), standardTime, Qt::UTC);
    QTest::newRow("utc3") << QDateTime(QDate(1760, 1, 1), standardTime, Qt::UTC) << 86400
                       << QDateTime(QDate(1760, 1, 2), standardTime, Qt::UTC);
    QTest::newRow("utc4") << QDateTime(QDate(1760, 1, 1), standardTime, Qt::UTC) << (86400 * 185)
                       << QDateTime(QDate(1760, 7, 4), standardTime, Qt::UTC);
    QTest::newRow("utc5") << QDateTime(QDate(1760, 1, 1), standardTime, Qt::UTC) << (86400 * 366)
                       << QDateTime(QDate(1761, 1, 1), standardTime, Qt::UTC);
    QTest::newRow("utc6") << QDateTime(QDate(4000, 1, 1), standardTime, Qt::UTC) << 86400
                       << QDateTime(QDate(4000, 1, 2), standardTime, Qt::UTC);
    QTest::newRow("utc7") << QDateTime(QDate(4000, 1, 1), standardTime, Qt::UTC) << (86400 * 185)
                       << QDateTime(QDate(4000, 7, 4), standardTime, Qt::UTC);
    QTest::newRow("utc8") << QDateTime(QDate(4000, 1, 1), standardTime, Qt::UTC) << (86400 * 366)
                       << QDateTime(QDate(4001, 1, 1), standardTime, Qt::UTC);
    QTest::newRow("utc9") << QDateTime(QDate(4000, 1, 1), standardTime, Qt::UTC) << 0
                       << QDateTime(QDate(4000, 1, 1), standardTime, Qt::UTC);

    if (europeanTimeZone) {
        QTest::newRow("cet0") << QDateTime(QDate(2004, 1, 1), standardTime, Qt::LocalTime) << 86400
                           << QDateTime(QDate(2004, 1, 2), standardTime, Qt::LocalTime);
        QTest::newRow("cet1") << QDateTime(QDate(2004, 1, 1), standardTime, Qt::LocalTime) << (86400 * 185)
                           << QDateTime(QDate(2004, 7, 4), daylightTime, Qt::LocalTime);
        QTest::newRow("cet2") << QDateTime(QDate(2004, 1, 1), standardTime, Qt::LocalTime) << (86400 * 366)
                           << QDateTime(QDate(2005, 1, 1), standardTime, Qt::LocalTime);
        QTest::newRow("cet3") << QDateTime(QDate(1760, 1, 1), standardTime, Qt::LocalTime) << 86400
                           << QDateTime(QDate(1760, 1, 2), standardTime, Qt::LocalTime);
#ifdef Q_OS_WIN
        // QDateTime uses 1980 on Windows, which did have daylight savings in July
        QTest::newRow("cet4") << QDateTime(QDate(1760, 1, 1), standardTime, Qt::LocalTime) << (86400 * 185)
                           << QDateTime(QDate(1760, 7, 4), daylightTime, Qt::LocalTime);
#else
        // QDateTime uses 1970 everywhere else, which did NOT have daylight savings in July
        QTest::newRow("cet4") << QDateTime(QDate(1760, 1, 1), standardTime, Qt::LocalTime) << (86400 * 185)
                           << QDateTime(QDate(1760, 7, 4), standardTime, Qt::LocalTime);
#endif
        QTest::newRow("cet5") << QDateTime(QDate(1760, 1, 1), standardTime, Qt::LocalTime) << (86400 * 366)
                           << QDateTime(QDate(1761, 1, 1), standardTime, Qt::LocalTime);
        QTest::newRow("cet6") << QDateTime(QDate(4000, 1, 1), standardTime, Qt::LocalTime) << 86400
                           << QDateTime(QDate(4000, 1, 2), standardTime, Qt::LocalTime);
        QTest::newRow("cet7") << QDateTime(QDate(4000, 1, 1), standardTime, Qt::LocalTime) << (86400 * 185)
                           << QDateTime(QDate(4000, 7, 4), daylightTime, Qt::LocalTime);
        QTest::newRow("cet8") << QDateTime(QDate(4000, 1, 1), standardTime, Qt::LocalTime) << (86400 * 366)
                           << QDateTime(QDate(4001, 1, 1), standardTime, Qt::LocalTime);
        QTest::newRow("cet9") << QDateTime(QDate(4000, 1, 1), standardTime, Qt::LocalTime) << 0
                           << QDateTime(QDate(4000, 1, 1), standardTime, Qt::LocalTime);
    }

    // Year sign change
    QTest::newRow("toNegative") << QDateTime(QDate(1, 1, 1), QTime(0, 0, 0), Qt::UTC)
                                << -1
                                << QDateTime(QDate(-1, 12, 31), QTime(23, 59, 59), Qt::UTC);
    QTest::newRow("toPositive") << QDateTime(QDate(-1, 12, 31), QTime(23, 59, 59), Qt::UTC)
                                << 1
                                << QDateTime(QDate(1, 1, 1), QTime(0, 0, 0), Qt::UTC);

    QTest::newRow("invalid") << invalidDateTime() << 1 << invalidDateTime();
}

void tst_QDateTime::addSecs()
{
    QFETCH(QDateTime, dt);
    QFETCH(int, nsecs);
    QFETCH(QDateTime, result);

#ifdef Q_OS_IRIX
    QEXPECT_FAIL("cet4", "IRIX databases say 1970 had DST", Abort);
#endif
    QCOMPARE(dt.addSecs(nsecs), result);
    QCOMPARE(result.addSecs(-nsecs), dt);
}

void tst_QDateTime::addMSecs_data()
{
    addSecs_data();
}

void tst_QDateTime::addMSecs()
{
    QFETCH(QDateTime, dt);
    QFETCH(int, nsecs);
    QFETCH(QDateTime, result);

#ifdef Q_OS_IRIX
    QEXPECT_FAIL("cet4", "IRIX databases say 1970 had DST", Abort);
#endif
    QCOMPARE(dt.addMSecs(qint64(nsecs) * 1000), result);
    QCOMPARE(result.addMSecs(qint64(-nsecs) * 1000), dt);
}

void tst_QDateTime::toTimeSpec_data()
{
    QTest::addColumn<QDateTime>("utc");
    QTest::addColumn<QDateTime>("local");

    QTime utcTime(4, 20, 30);
    QTime localStandardTime(5, 20, 30);
    QTime localDaylightTime(6, 20, 30);

    QTest::newRow("winter1") << QDateTime(QDate(2004, 1, 1), utcTime, Qt::UTC)
                          << QDateTime(QDate(2004, 1, 1), localStandardTime, Qt::LocalTime);
    QTest::newRow("winter2") << QDateTime(QDate(2004, 2, 29), utcTime, Qt::UTC)
                          << QDateTime(QDate(2004, 2, 29), localStandardTime, Qt::LocalTime);
    QTest::newRow("winter3") << QDateTime(QDate(1760, 2, 29), utcTime, Qt::UTC)
                          << QDateTime(QDate(1760, 2, 29), localStandardTime, Qt::LocalTime);
    QTest::newRow("winter4") << QDateTime(QDate(6000, 2, 29), utcTime, Qt::UTC)
                          << QDateTime(QDate(6000, 2, 29), localStandardTime, Qt::LocalTime);

    // Test mktime boundaries (1970 - 2038) and adjustDate().
    QTest::newRow("1969/12/31 23:00 UTC")
        << QDateTime(QDate(1969, 12, 31), QTime(23, 0, 0), Qt::UTC)
        << QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0), Qt::LocalTime);
    QTest::newRow("2037/12/31 23:00 UTC")
        << QDateTime(QDate(2037, 12, 31), QTime(23, 0, 0), Qt::UTC)
        << QDateTime(QDate(2038, 1, 1), QTime(0, 0, 0), Qt::LocalTime);

    QTest::newRow("-271821/4/20 00:00 UTC (JavaScript min date, start of day)")
        << QDateTime(QDate(-271821, 4, 20), QTime(0, 0, 0), Qt::UTC)
#ifdef Q_OS_WIN
        // Windows applies Daylight Time to dates before 1980, Olsen does not
        << QDateTime(QDate(-271821, 4, 20), QTime(2, 0, 0), Qt::LocalTime);
#else
        << QDateTime(QDate(-271821, 4, 20), QTime(1, 0, 0), Qt::LocalTime);
#endif
    QTest::newRow("-271821/4/20 23:00 UTC (JavaScript min date, end of day)")
        << QDateTime(QDate(-271821, 4, 20), QTime(23, 0, 0), Qt::UTC)
#ifdef Q_OS_WIN
        // Windows applies Daylight Time to dates before 1980, Olsen does not
        << QDateTime(QDate(-271821, 4, 21), QTime(1, 0, 0), Qt::LocalTime);
#else
        << QDateTime(QDate(-271821, 4, 21), QTime(0, 0, 0), Qt::LocalTime);
#endif

    QTest::newRow("QDate min")
        << QDateTime(QDate::fromJulianDay(minJd()), QTime(0, 0, 0), Qt::UTC)
        << QDateTime(QDate::fromJulianDay(minJd()), QTime(1, 0, 0), Qt::LocalTime);

    QTest::newRow("QDate max")
        << QDateTime(QDate::fromJulianDay(maxJd()), QTime(22, 59, 59), Qt::UTC)
        << QDateTime(QDate::fromJulianDay(maxJd()), QTime(23, 59, 59), Qt::LocalTime);

    if (europeanTimeZone) {
        QTest::newRow("summer1") << QDateTime(QDate(2004, 6, 30), utcTime, Qt::UTC)
                                 << QDateTime(QDate(2004, 6, 30), localDaylightTime, Qt::LocalTime);
#ifdef Q_OS_WIN
        // QDateTime uses 1980 on Windows, which did have daylight savings in July
        QTest::newRow("summer2") << QDateTime(QDate(1760, 6, 30), utcTime, Qt::UTC)
                                 << QDateTime(QDate(1760, 6, 30), localDaylightTime, Qt::LocalTime);
#else
        // QDateTime uses 1970 everywhere else, which did NOT have daylight savings in July
        QTest::newRow("summer2") << QDateTime(QDate(1760, 6, 30), utcTime, Qt::UTC)
                                 << QDateTime(QDate(1760, 6, 30), localStandardTime, Qt::LocalTime);
#endif
        QTest::newRow("summer3") << QDateTime(QDate(4000, 6, 30), utcTime, Qt::UTC)
                                 << QDateTime(QDate(4000, 6, 30), localDaylightTime, Qt::LocalTime);

        QTest::newRow("275760/9/23 00:00 UTC (JavaScript max date, start of day)")
            << QDateTime(QDate(275760, 9, 23), QTime(0, 0, 0), Qt::UTC)
            << QDateTime(QDate(275760, 9, 23), QTime(2, 0, 0), Qt::LocalTime);

        QTest::newRow("275760/9/23 22:00 UTC (JavaScript max date, end of day)")
            << QDateTime(QDate(275760, 9, 23), QTime(22, 0, 0), Qt::UTC)
            << QDateTime(QDate(275760, 9, 24), QTime(0, 0, 0), Qt::LocalTime);
    }

    QTest::newRow("msec") << QDateTime(QDate(4000, 6, 30), utcTime.addMSecs(1), Qt::UTC)
                       << QDateTime(QDate(4000, 6, 30), localDaylightTime.addMSecs(1), Qt::LocalTime);
}

void tst_QDateTime::toTimeSpec()
{
    if (europeanTimeZone) {
        QFETCH(QDateTime, utc);
        QFETCH(QDateTime, local);

        QCOMPARE(utc.toTimeSpec(Qt::UTC), utc);
        QCOMPARE(local.toTimeSpec(Qt::LocalTime), local);
#ifdef Q_OS_IRIX
        QEXPECT_FAIL("summer2", "IRIX databases say 1970 had DST", Abort);
#endif
        QCOMPARE(utc.toTimeSpec(Qt::LocalTime), local);
        QCOMPARE(local.toTimeSpec(Qt::UTC), utc);
        QCOMPARE(utc.toTimeSpec(Qt::UTC), local.toTimeSpec(Qt::UTC));
        QCOMPARE(utc.toTimeSpec(Qt::LocalTime), local.toTimeSpec(Qt::LocalTime));
    } else {
        QSKIP("Not tested with timezone other than Central European (CET/CST)");
    }
}

void tst_QDateTime::toLocalTime_data()
{
    toTimeSpec_data();
}

void tst_QDateTime::toLocalTime()
{
    if (europeanTimeZone) {
        QFETCH(QDateTime, utc);
        QFETCH(QDateTime, local);

        QCOMPARE(local.toLocalTime(), local);
#ifdef Q_OS_IRIX
        QEXPECT_FAIL("summer2", "IRIX databases say 1970 had DST", Abort);
#endif
        QCOMPARE(utc.toLocalTime(), local);
        QCOMPARE(utc.toLocalTime(), local.toLocalTime());
    } else {
        QSKIP("Not tested with timezone other than Central European (CET/CST)");
    }
}

void tst_QDateTime::toUTC_data()
{
    toTimeSpec_data();
}

void tst_QDateTime::toUTC()
{
    if (europeanTimeZone) {
        QFETCH(QDateTime, utc);
        QFETCH(QDateTime, local);

        QCOMPARE(utc.toUTC(), utc);
#ifdef Q_OS_IRIX
        QEXPECT_FAIL("summer2", "IRIX databases say 1970 had DST", Abort);
#endif
        QCOMPARE(local.toUTC(), utc);
        QCOMPARE(utc.toUTC(), local.toUTC());
    } else {
        QSKIP("Not tested with timezone other than Central European (CET/CST)");
    }

    QDateTime dt = QDateTime::currentDateTime();
    if(dt.time().msec() == 0){
        dt.setTime(dt.time().addMSecs(1));
    }
    QString s = dt.toString("zzz");
    QString t = dt.toUTC().toString("zzz");
    QCOMPARE(s, t);
}

void tst_QDateTime::daysTo()
{
    QDateTime dt1(QDate(1760, 1, 2), QTime());
    QDateTime dt2(QDate(1760, 2, 2), QTime());
    QDateTime dt3(QDate(1760, 3, 2), QTime());

    QCOMPARE(dt1.daysTo(dt2), (qint64) 31);
    QCOMPARE(dt1.addDays(31), dt2);

    QCOMPARE(dt2.daysTo(dt3), (qint64) 29);
    QCOMPARE(dt2.addDays(29), dt3);

    QCOMPARE(dt1.daysTo(dt3), (qint64) 60);
    QCOMPARE(dt1.addDays(60), dt3);

    QCOMPARE(dt2.daysTo(dt1), (qint64) -31);
    QCOMPARE(dt2.addDays(-31), dt1);

    QCOMPARE(dt3.daysTo(dt2), (qint64) -29);
    QCOMPARE(dt3.addDays(-29), dt2);

    QCOMPARE(dt3.daysTo(dt1), (qint64) -60);
    QCOMPARE(dt3.addDays(-60), dt1);
}

void tst_QDateTime::secsTo_data()
{
    addSecs_data();

    QTest::newRow("disregard milliseconds #1")
        << QDateTime(QDate(2012, 3, 7), QTime(0, 58, 0, 0)) << 60
        << QDateTime(QDate(2012, 3, 7), QTime(0, 59, 0, 400));

    QTest::newRow("disregard milliseconds #2")
        << QDateTime(QDate(2012, 3, 7), QTime(0, 59, 0, 0)) << 60
        << QDateTime(QDate(2012, 3, 7), QTime(1, 0, 0, 400));
}

void tst_QDateTime::secsTo()
{
    QFETCH(QDateTime, dt);
    QFETCH(int, nsecs);
    QFETCH(QDateTime, result);

    if (dt.isValid()) {
    #ifdef Q_OS_IRIX
        QEXPECT_FAIL("cet4", "IRIX databases say 1970 had DST", Abort);
    #endif
        QCOMPARE(dt.secsTo(result), (qint64)nsecs);
        QCOMPARE(result.secsTo(dt), (qint64)-nsecs);
        QVERIFY((dt == result) == (0 == nsecs));
        QVERIFY((dt != result) == (0 != nsecs));
        QVERIFY((dt < result) == (0 < nsecs));
        QVERIFY((dt <= result) == (0 <= nsecs));
        QVERIFY((dt > result) == (0 > nsecs));
        QVERIFY((dt >= result) == (0 >= nsecs));
    } else {
        QVERIFY(dt.secsTo(result) == 0);
        QVERIFY(result.secsTo(dt) == 0);
    }
}

void tst_QDateTime::msecsTo_data()
{
    addMSecs_data();
}

void tst_QDateTime::msecsTo()
{
    QFETCH(QDateTime, dt);
    QFETCH(int, nsecs);
    QFETCH(QDateTime, result);

    if (dt.isValid()) {
    #ifdef Q_OS_IRIX
        QEXPECT_FAIL("cet4", "IRIX databases say 1970 had DST", Abort);
    #endif
        QCOMPARE(dt.msecsTo(result), qint64(nsecs) * 1000);
        QCOMPARE(result.msecsTo(dt), -qint64(nsecs) * 1000);
        QVERIFY((dt == result) == (0 == (qint64(nsecs) * 1000)));
        QVERIFY((dt != result) == (0 != (qint64(nsecs) * 1000)));
        QVERIFY((dt < result) == (0 < (qint64(nsecs) * 1000)));
        QVERIFY((dt <= result) == (0 <= (qint64(nsecs) * 1000)));
        QVERIFY((dt > result) == (0 > (qint64(nsecs) * 1000)));
        QVERIFY((dt >= result) == (0 >= (qint64(nsecs) * 1000)));
    } else {
        QVERIFY(dt.msecsTo(result) == 0);
        QVERIFY(result.msecsTo(dt) == 0);
    }
}

void tst_QDateTime::currentDateTime()
{
#if defined(Q_OS_WINCE)
    __time64_t buf1, buf2;
    ::_time64(&buf1);
#else
    time_t buf1, buf2;
    ::time(&buf1);
#endif
    QDateTime lowerBound;
    lowerBound.setTime_t(buf1);

    QDateTime dt1 = QDateTime::currentDateTime();
    QDateTime dt2 = QDateTime::currentDateTime().toLocalTime();
    QDateTime dt3 = QDateTime::currentDateTime().toUTC();

#if defined(Q_OS_WINCE)
    ::_time64(&buf2);
#else
    ::time(&buf2);
#endif
    QDateTime upperBound;
    upperBound.setTime_t(buf2);
    // Note we must add 2 seconds here because time() may return up to
    // 1 second difference from the more accurate method used by QDateTime::currentDateTime()
    upperBound = upperBound.addSecs(2);

    QString details = QString("\n"
        "lowerBound: %1\n"
        "dt1:        %2\n"
        "dt2:        %3\n"
        "dt3:        %4\n"
        "upperBound: %5\n")
        .arg(lowerBound.toTime_t())
        .arg(dt1.toTime_t())
        .arg(dt2.toTime_t())
        .arg(dt3.toTime_t())
        .arg(upperBound.toTime_t());

    QVERIFY2(lowerBound < upperBound, qPrintable(details));

    QVERIFY2(lowerBound <= dt1, qPrintable(details));
    QVERIFY2(dt1 < upperBound, qPrintable(details));
    QVERIFY2(lowerBound <= dt2, qPrintable(details));
    QVERIFY2(dt2 < upperBound, qPrintable(details));
    QVERIFY2(lowerBound <= dt3, qPrintable(details));
    QVERIFY2(dt3 < upperBound, qPrintable(details));

    QVERIFY(dt1.timeSpec() == Qt::LocalTime);
    QVERIFY(dt2.timeSpec() == Qt::LocalTime);
    QVERIFY(dt3.timeSpec() == Qt::UTC);
}

void tst_QDateTime::currentDateTimeUtc()
{
#if defined(Q_OS_WINCE)
    __time64_t buf1, buf2;
    ::_time64(&buf1);
#else
    time_t buf1, buf2;
    ::time(&buf1);
#endif
    QDateTime lowerBound;
    lowerBound.setTime_t(buf1);

    QDateTime dt1 = QDateTime::currentDateTimeUtc();
    QDateTime dt2 = QDateTime::currentDateTimeUtc().toLocalTime();
    QDateTime dt3 = QDateTime::currentDateTimeUtc().toUTC();

#if defined(Q_OS_WINCE)
    ::_time64(&buf2);
#else
    ::time(&buf2);
#endif
    QDateTime upperBound;
    upperBound.setTime_t(buf2);
    // Note we must add 2 seconds here because time() may return up to
    // 1 second difference from the more accurate method used by QDateTime::currentDateTime()
    upperBound = upperBound.addSecs(2);

    QString details = QString("\n"
        "lowerBound: %1\n"
        "dt1:        %2\n"
        "dt2:        %3\n"
        "dt3:        %4\n"
        "upperBound: %5\n")
        .arg(lowerBound.toTime_t())
        .arg(dt1.toTime_t())
        .arg(dt2.toTime_t())
        .arg(dt3.toTime_t())
        .arg(upperBound.toTime_t());

    QVERIFY2(lowerBound < upperBound, qPrintable(details));

    QVERIFY2(lowerBound <= dt1, qPrintable(details));
    QVERIFY2(dt1 < upperBound, qPrintable(details));
    QVERIFY2(lowerBound <= dt2, qPrintable(details));
    QVERIFY2(dt2 < upperBound, qPrintable(details));
    QVERIFY2(lowerBound <= dt3, qPrintable(details));
    QVERIFY2(dt3 < upperBound, qPrintable(details));

    QVERIFY(dt1.timeSpec() == Qt::UTC);
    QVERIFY(dt2.timeSpec() == Qt::LocalTime);
    QVERIFY(dt3.timeSpec() == Qt::UTC);
}

void tst_QDateTime::currentDateTimeUtc2()
{
    QDateTime local, utc;
    qint64 msec;

    // check that we got all down to the same milliseconds
    int i = 20;
    bool ok = false;
    do {
        local = QDateTime::currentDateTime();
        utc = QDateTime::currentDateTimeUtc();
        msec = QDateTime::currentMSecsSinceEpoch();
        ok = local.time().msec() == utc.time().msec()
            && utc.time().msec() == (msec % 1000);
    } while (--i && !ok);

    if (!i)
        QSKIP("Failed to get the dates within 1 ms of each other");

    // seconds and milliseconds should be the same:
    QCOMPARE(utc.time().second(), local.time().second());
    QCOMPARE(utc.time().msec(), local.time().msec());
    QCOMPARE(msec % 1000, qint64(local.time().msec()));
    QCOMPARE(msec / 1000 % 60, qint64(local.time().second()));

    // the two dates should be equal, actually
    QCOMPARE(local.toUTC(), utc);
    QCOMPARE(utc.toLocalTime(), local);

    // and finally, the time_t should equal our number
    QCOMPARE(qint64(utc.toTime_t()), msec / 1000);
    QCOMPARE(qint64(local.toTime_t()), msec / 1000);
    QCOMPARE(utc.toMSecsSinceEpoch(), msec);
    QCOMPARE(local.toMSecsSinceEpoch(), msec);
}

void tst_QDateTime::toTime_t_data()
{
    QTest::addColumn<QString>("dateTimeStr");
    QTest::addColumn<bool>("res");

    QTest::newRow( "data1" ) << str( 1800, 1, 1, 12, 0, 0 ) << false;
    QTest::newRow( "data2" ) << str( 1969, 1, 1, 12, 0, 0 ) << false;
    QTest::newRow( "data3" ) << str( 2002, 1, 1, 12, 0, 0 ) << true;
    QTest::newRow( "data4" ) << str( 2002, 6, 1, 12, 0, 0 ) << true;
    QTest::newRow( "data5" ) << QString("INVALID") << false;
    QTest::newRow( "data6" ) << str( 2038, 1, 1, 12, 0, 0 ) << true;
    QTest::newRow( "data7" ) << str( 2063, 4, 5, 12, 0, 0 ) << true; // the day of First Contact
    QTest::newRow( "data8" ) << str( 2107, 1, 1, 12, 0, 0 )
                          << bool( sizeof(uint) > 32 && sizeof(time_t) > 32 );
}

void tst_QDateTime::toTime_t()
{
    QFETCH( QString, dateTimeStr );
    QDateTime datetime = dt( dateTimeStr );

    uint asTime_t = datetime.toTime_t();
    QFETCH( bool, res );
    if (res) {
        QVERIFY( asTime_t != (uint)-1 );
    } else {
        QVERIFY( asTime_t == (uint)-1 );
    }

    if ( asTime_t != (uint) -1 ) {
        QDateTime datetime2 = QDateTime::fromTime_t( asTime_t );
        QCOMPARE(datetime, datetime2);
    }
}

void tst_QDateTime::daylightSavingsTimeChange()
{
    // This is a regression test for an old bug where starting with a date in
    // DST and then moving to a date outside it (or vice-versa) caused 1-hour
    // jumps in time when addSecs() was called.
    //
    // The bug was caused by QDateTime knowing more than it lets show.
    // Internally, if it knows, QDateTime stores a flag indicating if the time is
    // DST or not. If it doesn't, it sets to "LocalUnknown".  The problem happened
    // because some functions did not reset the flag when moving in or out of DST.

    // WARNING: This test only works if there's a Daylight Savings Time change
    // in the current locale between 2006-11-06 and 2006-10-16
    // This is true for Central European Time

    if (!europeanTimeZone)
        QSKIP("Not tested with timezone other than Central European (CET/CEST)");

    QDateTime dt = QDateTime(QDate(2006, 11, 6), QTime(0, 0, 0), Qt::LocalTime);
    dt.setDate(QDate(2006, 10, 16));
    dt = dt.addSecs(1);
    QCOMPARE(dt.date(), QDate(2006, 10, 16));
    QCOMPARE(dt.time(), QTime(0, 0, 1));

    // now using fromTime_t
    dt = QDateTime::fromTime_t(1162767600); // 2006-11-06 00:00:00 +0100
    dt.setDate(QDate(2006, 10, 16));
    dt = dt.addSecs (1);
    QCOMPARE(dt.date(), QDate(2006, 10, 16));
    QCOMPARE(dt.time(), QTime(0, 0, 1));
}

void tst_QDateTime::operator_eqeq_data()
{
    QTest::addColumn<QDateTime>("dt1");
    QTest::addColumn<QDateTime>("dt2");
    QTest::addColumn<bool>("expectEqual");
    QTest::addColumn<bool>("checkEuro");

    QDateTime dateTime1(QDate(2012, 6, 20), QTime(14, 33, 2, 500));
    QDateTime dateTime1a = dateTime1.addMSecs(1);
    QDateTime dateTime2(QDate(2012, 20, 6), QTime(14, 33, 2, 500));
    QDateTime dateTime2a = dateTime2.addMSecs(-1);
    QDateTime dateTime3(QDate(1970, 1, 1), QTime(0, 0, 0, 0), Qt::UTC);
    QDateTime dateTime3a = dateTime3.addDays(1);
    QDateTime dateTime3b = dateTime3.addDays(-1);
    // Ensure that different times may be equal when considering timezone.
    QDateTime dateTime3c(dateTime3.addSecs(3600));
    dateTime3c.setUtcOffset(3600);
    QDateTime dateTime3d(dateTime3.addSecs(-3600));
    dateTime3d.setUtcOffset(-3600);
    // Convert from UTC to local.
    QDateTime dateTime3e(dateTime3.date(), dateTime3.time());

    QTest::newRow("data0") << dateTime1 << dateTime1 << true << false;
    QTest::newRow("data1") << dateTime2 << dateTime2 << true << false;
    QTest::newRow("data2") << dateTime1a << dateTime1a << true << false;
    QTest::newRow("data3") << dateTime1 << dateTime2 << false << false;
    QTest::newRow("data4") << dateTime1 << dateTime1a << false << false;
    QTest::newRow("data5") << dateTime2 << dateTime2a << false << false;
    QTest::newRow("data6") << dateTime2 << dateTime3 << false << false;
    QTest::newRow("data7") << dateTime3 << dateTime3a << false << false;
    QTest::newRow("data8") << dateTime3 << dateTime3b << false << false;
    QTest::newRow("data9") << dateTime3a << dateTime3b << false << false;
    QTest::newRow("data10") << dateTime3 << dateTime3c << true << false;
    QTest::newRow("data11") << dateTime3 << dateTime3d << true << false;
    QTest::newRow("data12") << dateTime3c << dateTime3d << true << false;
    QTest::newRow("data13") << dateTime3 << dateTime3e << false << false;
    QTest::newRow("invalid == invalid") << invalidDateTime() << invalidDateTime() << true << false;
    QTest::newRow("invalid == valid #1") << invalidDateTime() << dateTime1 << false << false;

    if (europeanTimeZone) {
        QTest::newRow("data14") << QDateTime(QDate(2004, 1, 2), QTime(2, 2, 3), Qt::LocalTime)
             << QDateTime(QDate(2004, 1, 2), QTime(1, 2, 3), Qt::UTC) << true << true;
    }
}

void tst_QDateTime::operator_eqeq()
{
    QFETCH(QDateTime, dt1);
    QFETCH(QDateTime, dt2);
    QFETCH(bool, expectEqual);
    QFETCH(bool, checkEuro);

    QVERIFY(dt1 == dt1);
    QVERIFY(!(dt1 != dt1));

    QVERIFY(dt2 == dt2);
    QVERIFY(!(dt2 != dt2));

    QVERIFY(dt1 != QDateTime::currentDateTime());
    QVERIFY(dt2 != QDateTime::currentDateTime());

    QVERIFY(dt1.toUTC() == dt1.toUTC());

    bool equal = dt1 == dt2;
    QCOMPARE(equal, expectEqual);
    bool notEqual = dt1 != dt2;
    QCOMPARE(notEqual, !expectEqual);

    if (equal)
        QVERIFY(qHash(dt1) == qHash(dt2));

    if (checkEuro && europeanTimeZone) {
        QVERIFY(dt1.toUTC() == dt2);
        QVERIFY(dt1 == dt2.toLocalTime());
    }
}

#ifndef Q_OS_WINCE
Q_DECLARE_METATYPE(QDataStream::Version)

void tst_QDateTime::operator_insert_extract_data()
{
    QTest::addColumn<QDateTime>("dateTime");
    QTest::addColumn<QString>("serialiseAs");
    QTest::addColumn<QString>("deserialiseAs");
    QTest::addColumn<QDataStream::Version>("dataStreamVersion");

    const QDateTime positiveYear(QDateTime(QDate(2012, 8, 14), QTime(8, 0, 0), Qt::LocalTime));
    const QDateTime negativeYear(QDateTime(QDate(-2012, 8, 14), QTime(8, 0, 0), Qt::LocalTime));

    const QString westernAustralia(QString::fromLatin1("AWST-8AWDT-9,M10.5.0,M3.5.0/03:00:00"));
    const QString hawaii(QString::fromLatin1("HAW10"));

    const QDataStream tmpDataStream;
    const int thisVersion = tmpDataStream.version();
    for (int version = QDataStream::Qt_1_0; version <= thisVersion; ++version) {
        const QDataStream::Version dataStreamVersion = static_cast<QDataStream::Version>(version);
        QTest::newRow(QString::fromLatin1("v%1 WA => HAWAII %2").arg(dataStreamVersion).arg(positiveYear.toString()).toLocal8Bit().constData())
            << positiveYear << westernAustralia << hawaii << dataStreamVersion;
        QTest::newRow(QString::fromLatin1("v%1 WA => WA %2").arg(dataStreamVersion).arg(positiveYear.toString()).toLocal8Bit().constData())
            << positiveYear << westernAustralia << westernAustralia << dataStreamVersion;
        QTest::newRow(QString::fromLatin1("v%1 HAWAII => WA %2").arg(dataStreamVersion).arg(negativeYear.toString()).toLocal8Bit().constData())
            << negativeYear << hawaii << westernAustralia << dataStreamVersion;
        QTest::newRow(QString::fromLatin1("v%1 HAWAII => HAWAII %2").arg(dataStreamVersion).arg(positiveYear.toString()).toLocal8Bit().constData())
            << positiveYear << hawaii << hawaii << dataStreamVersion;
    }
}

void tst_QDateTime::operator_insert_extract()
{
    QFETCH(QDateTime, dateTime);
    QFETCH(QString, serialiseAs);
    QFETCH(QString, deserialiseAs);
    QFETCH(QDataStream::Version, dataStreamVersion);

    // Save the previous timezone so we can restore it afterwards, just in case.
    QString previousTimeZone = qgetenv("TZ");
    // Start off in a certain timezone.
    qputenv("TZ", serialiseAs.toLocal8Bit().constData());
    tzset();
    QDateTime dateTimeAsUTC(dateTime.toUTC());

    QByteArray byteArray;
    {
        QDataStream dataStream(&byteArray, QIODevice::WriteOnly);
        dataStream.setVersion(dataStreamVersion);
        if (dataStreamVersion == QDataStream::Qt_5_0) {
            // Qt 5 serialises as UTC and converts back to the stored timeSpec when
            // deserialising; we don't need to do it ourselves...
            dataStream << dateTime << dateTime;
        } else {
            // ... but other versions don't, so we have to here.
            dataStream << dateTimeAsUTC << dateTimeAsUTC;
            // We'll also make sure that a deserialised local datetime is the same
            // time of day (potentially different UTC time), regardless of which
            // timezone it was serialised in. E.g.: Tue Aug 14 08:00:00 2012
            // serialised in WST should be deserialised as Tue Aug 14 08:00:00 2012
            // HST.
            dataStream << dateTime;
        }
    }

    // Ensure that a change in timezone between serialisation and deserialisation
    // still results in identical UTC-converted datetimes.
    qputenv("TZ", deserialiseAs.toLocal8Bit().constData());
    tzset();
    QDateTime expectedLocalTime(dateTimeAsUTC.toLocalTime());
    {
        // Deserialise whole QDateTime at once.
        QDataStream dataStream(&byteArray, QIODevice::ReadOnly);
        dataStream.setVersion(dataStreamVersion);
        QDateTime deserialised;
        dataStream >> deserialised;

        if (dataStreamVersion == QDataStream::Qt_5_0) {
            // Ensure local time is still correct. Again, Qt 5 handles the timeSpec
            // conversion (in this case, UTC => LocalTime) for us when deserialising.
            QCOMPARE(deserialised, expectedLocalTime);
        } else {
            if (dataStreamVersion < QDataStream::Qt_4_0) {
                // Versions lower than Qt 4 don't serialise the timeSpec, instead
                // assuming that everything is LocalTime.
                deserialised.setTimeSpec(Qt::UTC);
            }
            // Qt 4.* versions do serialise the timeSpec, so we only need to convert from UTC here.
            deserialised = deserialised.toLocalTime();

            QCOMPARE(deserialised, expectedLocalTime);
        }
        // Sanity check UTC times (operator== already converts its operands to UTC before comparing).
        QCOMPARE(deserialised.toUTC(), expectedLocalTime.toUTC());

        // Deserialise each component individually.
        QDate deserialisedDate;
        dataStream >> deserialisedDate;
        QTime deserialisedTime;
        dataStream >> deserialisedTime;
        qint8 deserialisedSpec;
        if (dataStreamVersion >= QDataStream::Qt_4_0)
            dataStream >> deserialisedSpec;
        deserialised = QDateTime(deserialisedDate, deserialisedTime, Qt::UTC);
        if (dataStreamVersion >= QDataStream::Qt_4_0)
            deserialised = deserialised.toTimeSpec(static_cast<Qt::TimeSpec>(deserialisedSpec));
        // Ensure local time is still correct.
        QCOMPARE(deserialised, expectedLocalTime);
        // Sanity check UTC times.
        QCOMPARE(deserialised.toUTC(), expectedLocalTime.toUTC());

        if (dataStreamVersion != QDataStream::Qt_5_0) {
            // Deserialised local datetime should be the same time of day,
            // regardless of which timezone it was serialised in.
            QDateTime localDeserialized;
            dataStream >> localDeserialized;
            QCOMPARE(localDeserialized, dateTime);
        }
    }

    qputenv("TZ", previousTimeZone.toLocal8Bit().constData());
    tzset();
}
#endif

void tst_QDateTime::toString_strformat_data()
{
    QTest::addColumn<QDateTime>("dt");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("str");

    QTest::newRow( "datetime0" ) << QDateTime() << QString("dd-MM-yyyy hh:mm:ss") << QString();
    QTest::newRow( "datetime1" ) << QDateTime(QDate(1999, 12, 31), QTime(23, 59, 59, 999))
                                 << QString("dd-'mmddyy'MM-yyyy hh:mm:ss.zzz")
                                 << QString("31-mmddyy12-1999 23:59:59.999");
    QTest::newRow( "datetime2" ) << QDateTime(QDate(1999, 12, 31), QTime(23, 59, 59, 999))
                                 << QString("dd-'apAP'MM-yyyy hh:mm:ss.zzz")
                                 << QString("31-apAP12-1999 23:59:59.999");
    QTest::newRow( "datetime3" ) << QDateTime(QDate(1999, 12, 31), QTime(23, 59, 59, 999))
                                 << QString("Apdd-MM-yyyy hh:mm:ss.zzz")
                                 << QString("PMp31-12-1999 11:59:59.999");
    QTest::newRow( "datetime4" ) << QDateTime(QDate(1999, 12, 31), QTime(23, 59, 59, 999))
                                 << QString("'ap'apdd-MM-yyyy 'AP'hh:mm:ss.zzz")
                                 << QString("appm31-12-1999 AP11:59:59.999");
    QTest::newRow( "datetime5" ) << QDateTime(QDate(1999, 12, 31), QTime(23, 59, 59, 999))
                                 << QString("'''") << QString("'");
    QTest::newRow( "datetime6" ) << QDateTime(QDate(1999, 12, 31), QTime(23, 59, 59, 999))
                                 << QString("'ap") << QString("ap");
    QTest::newRow( "datetime7" ) << QDateTime(QDate(1999, 12, 31), QTime(23, 59, 59, 999))
                                 << QString("' ' 'hh' hh") << QString("  hh 23");
    QTest::newRow( "datetime8" ) << QDateTime(QDate(1999, 12, 31), QTime(23, 59, 59, 999))
                                 << QString("d'foobar'") << QString("31foobar");
    QTest::newRow( "datetime9" ) << QDateTime(QDate(1999, 12, 31), QTime(3, 59, 59, 999))
                                 << QString("hhhhh") << QString("03033");
    QTest::newRow( "datetime11" ) << QDateTime(QDate(1999, 12, 31), QTime(23, 59, 59, 999))
                                 << QString("HHHhhhAaAPap") << QString("23231111PMpmPMpm");
    QTest::newRow( "datetime12" ) << QDateTime(QDate(1999, 12, 31), QTime(3, 59, 59, 999))
                                 << QString("HHHhhhAaAPap") << QString("033033AMamAMam");
    QTest::newRow( "datetime13" ) << QDateTime(QDate(1974, 12, 1), QTime(14, 14, 20))
                                 << QString("hh''mm''ss dd''MM''yyyy")
                                 << QString("14'14'20 01'12'1974");
    QTest::newRow( "single, 0 => 12 AM" ) << QDateTime(QDate(1999, 12, 31), QTime(0, 59, 59, 999))
        << QString("hAP") << QString("12AM");
    QTest::newRow( "double, 0 => 12 AM" ) << QDateTime(QDate(1999, 12, 31), QTime(0, 59, 59, 999))
        << QString("hhAP") << QString("12AM");
    QTest::newRow( "dddd" ) << QDateTime(QDate(1999, 12, 31), QTime(0, 59, 59, 999))
        << QString("dddd") << QString("Friday");
    QTest::newRow( "ddd" ) << QDateTime(QDate(1999, 12, 31), QTime(0, 59, 59, 999))
        << QString("ddd") << QString("Fri");
    QTest::newRow( "MMMM" ) << QDateTime(QDate(1999, 12, 31), QTime(0, 59, 59, 999))
        << QString("MMMM") << QString("December");
    QTest::newRow( "MMM" ) << QDateTime(QDate(1999, 12, 31), QTime(0, 59, 59, 999))
        << QString("MMM") << QString("Dec");
    QTest::newRow( "emtpy" ) << QDateTime(QDate(1999, 12, 31), QTime(0, 59, 59, 999))
        << QString("") << QString("");
}

void tst_QDateTime::toString_strformat()
{
    QFETCH( QDateTime, dt );
    QFETCH( QString, format );
    QFETCH( QString, str );
    QCOMPARE( dt.toString( format ), str );
}

void tst_QDateTime::fromStringDateFormat_data()
{
    QTest::addColumn<QString>("dateTimeStr");
    QTest::addColumn<Qt::DateFormat>("dateFormat");
    QTest::addColumn<QDateTime>("expected");

    // Test Qt::TextDate format.
    QTest::newRow("text date") << QString::fromLatin1("Tue Jun 17 08:00:10 2003")
        << Qt::TextDate << QDateTime(QDate(2003, 6, 17), QTime(8, 0, 10, 0), Qt::LocalTime);
    QTest::newRow("text date Year 0999") << QString::fromLatin1("Tue Jun 17 08:00:10 0999")
        << Qt::TextDate << QDateTime(QDate(999, 6, 17), QTime(8, 0, 10, 0), Qt::LocalTime);
    QTest::newRow("text date Year 999") << QString::fromLatin1("Tue Jun 17 08:00:10 999")
        << Qt::TextDate << QDateTime(QDate(999, 6, 17), QTime(8, 0, 10, 0), Qt::LocalTime);
    QTest::newRow("text date Year 12345") << QString::fromLatin1("Tue Jun 17 08:00:10 12345")
        << Qt::TextDate << QDateTime(QDate(12345, 6, 17), QTime(8, 0, 10, 0), Qt::LocalTime);
    QTest::newRow("text date Year -4712") << QString::fromLatin1("Tue Jan 1 00:01:02 -4712")
        << Qt::TextDate << QDateTime(QDate(-4712, 1, 1), QTime(0, 1, 2, 0), Qt::LocalTime);
    QTest::newRow("text data0") << QString::fromLatin1("Thu Jan 1 00:00:00 1970")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0), Qt::LocalTime);
    QTest::newRow("text data1") << QString::fromLatin1("Thu Jan 2 12:34 1970")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 2), QTime(12, 34, 0), Qt::LocalTime);
    QTest::newRow("text data2") << QString::fromLatin1("Thu Jan 1 00 1970")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text data3") << QString::fromLatin1("Thu Jan 1 00:00:00:00 1970")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text data4") << QString::fromLatin1("Thu 1. Jan 00:00:00 1970")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 1), QTime(0, 0), Qt::LocalTime);
    QTest::newRow("text data5") << QString::fromLatin1(" Thu   Jan   1    00:00:00    1970  ")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0), Qt::LocalTime);
    QTest::newRow("text data6") << QString::fromLatin1("Thu Jan 1 00:00:00")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text data7") << QString::fromLatin1("Thu Jan 1 1970 00:00:00")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0), Qt::LocalTime);
    QTest::newRow("text data8") << QString::fromLatin1("Thu Jan 1 00:12:34 1970 GMT+foo")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text data9") << QString::fromLatin1("Thu Jan 1 00:12:34 1970 GMT")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("text data10") << QString::fromLatin1("Thu Jan 1 00:12:34 1970 GMT-0300")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 1), QTime(3, 12, 34), Qt::UTC);
    QTest::newRow("text data11") << QString::fromLatin1("Thu Jan 1 00:12:34 1970 GMT+0300")
        << Qt::TextDate << QDateTime(QDate(1969, 12, 31), QTime(21, 12, 34), Qt::UTC);
    QTest::newRow("text data12") << QString::fromLatin1("Thu Jan 1 00:12:34 1970 gmt")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("text data13") << QString::fromLatin1("Thu Jan 1 1970 00:12:34 GMT+0100")
        << Qt::TextDate << QDateTime(QDate(1969, 12, 31), QTime(23, 12, 34), Qt::UTC);
    QTest::newRow("text empty") << QString::fromLatin1("")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text too many parts") << QString::fromLatin1("Thu Jan 1 00:12:34 1970 gmt +0100")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid month name") << QString::fromLatin1("Thu Jaz 1 1970 00:12:34")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid date") << QString::fromLatin1("Thu Jan 32 1970 00:12:34")
        << Qt::TextDate << QDateTime(invalidDate(), QTime(0, 12, 34), Qt::LocalTime);
    QTest::newRow("text invalid day #1") << QString::fromLatin1("Thu Jan XX 1970 00:12:34")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid day #2") << QString::fromLatin1("Thu X. Jan 00:00:00 1970")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid day #3") << QString::fromLatin1("Thu 1 Jan 00:00:00 1970")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid year #1") << QString::fromLatin1("Thu 1. Jan 00:00:00 19X0")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid year #2") << QString::fromLatin1("Thu 1. Jan 19X0 00:00:00")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid hour") << QString::fromLatin1("Thu 1. Jan 1970 0X:00:00")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid minute") << QString::fromLatin1("Thu 1. Jan 1970 00:0X:00")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid second") << QString::fromLatin1("Thu 1. Jan 1970 00:00:0X")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid gmt specifier #1") << QString::fromLatin1("Thu 1. Jan 1970 00:00:00 DMT")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid gmt specifier #2") << QString::fromLatin1("Thu 1. Jan 1970 00:00:00 GMTx0200")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid gmt hour") << QString::fromLatin1("Thu 1. Jan 1970 00:00:00 GMT+0X00")
        << Qt::TextDate << invalidDateTime();
    QTest::newRow("text invalid gmt minute") << QString::fromLatin1("Thu 1. Jan 1970 00:00:00 GMT+000X")
        << Qt::TextDate << invalidDateTime();

    // Test Qt::ISODate format.
    QTest::newRow("ISO +01:00") << QString::fromLatin1("1987-02-13T13:24:51+01:00")
        << Qt::ISODate << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("ISO -01:00") << QString::fromLatin1("1987-02-13T13:24:51-01:00")
        << Qt::ISODate << QDateTime(QDate(1987, 2, 13), QTime(14, 24, 51), Qt::UTC);
    // Not sure about these two... it will currently be created as LocalTime, but it
    // should probably be UTC according to the ISO 8601 spec (see 4.2.5.1).
    QTest::newRow("ISO +0000") << QString::fromLatin1("1970-01-01T00:12:34+0000")
        << Qt::ISODate << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::LocalTime);
    QTest::newRow("ISO +00:00") << QString::fromLatin1("1970-01-01T00:12:34+00:00")
        << Qt::ISODate << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::LocalTime);
    // No time specified - defaults to Qt::LocalTime.
    QTest::newRow("ISO data3") << QString::fromLatin1("2002-10-01")
        << Qt::ISODate << QDateTime(QDate(2002, 10, 1), QTime(0, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO") << QString::fromLatin1("2005-06-28T07:57:30.0010000000Z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 1), Qt::UTC);
    QTest::newRow("ISO with comma 1") << QString::fromLatin1("2005-06-28T07:57:30,0040000000Z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 4), Qt::UTC);
    QTest::newRow("ISO with comma 2") << QString::fromLatin1("2005-06-28T07:57:30,0015Z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 2), Qt::UTC);
    QTest::newRow("ISO with comma 3") << QString::fromLatin1("2005-06-28T07:57:30,0014Z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 1), Qt::UTC);
    QTest::newRow("ISO with comma 4") << QString::fromLatin1("2005-06-28T07:57:30,1Z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 100), Qt::UTC);
    QTest::newRow("ISO with comma 5") << QString::fromLatin1("2005-06-28T07:57:30,11")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 110), Qt::LocalTime);
    // 24:00:00 Should be next day according to ISO 8601 section 4.2.3.
    QTest::newRow("ISO 24:00") << QString::fromLatin1("2012-06-04T24:00:00")
        << Qt::ISODate << QDateTime(QDate(2012, 6, 5), QTime(0, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO 24:00 end of month") << QString::fromLatin1("2012-06-30T24:00:00")
        << Qt::ISODate << QDateTime(QDate(2012, 7, 1), QTime(0, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO 24:00 end of year") << QString::fromLatin1("2012-12-31T24:00:00")
        << Qt::ISODate << QDateTime(QDate(2013, 1, 1), QTime(0, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO 24:00, fract ms") << QString::fromLatin1("2012-01-01T24:00:00.000")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 2), QTime(0, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO 24:00 end of year, fract ms") << QString::fromLatin1("2012-12-31T24:00:00.000")
        << Qt::ISODate << QDateTime(QDate(2013, 1, 1), QTime(0, 0, 0, 0), Qt::LocalTime);
    // Test fractional seconds.
    QTest::newRow("ISO .0 of a second (period)") << QString::fromLatin1("2012-01-01T08:00:00.0")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO .00 of a second (period)") << QString::fromLatin1("2012-01-01T08:00:00.00")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO .000 of a second (period)") << QString::fromLatin1("2012-01-01T08:00:00.000")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO .1 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,1")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 100), Qt::LocalTime);
    QTest::newRow("ISO .99 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,99")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 990), Qt::LocalTime);
    QTest::newRow("ISO .998 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,998")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 998), Qt::LocalTime);
    QTest::newRow("ISO .999 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,999")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 999), Qt::LocalTime);
    QTest::newRow("ISO .3335 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,3335")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 334), Qt::LocalTime);
    QTest::newRow("ISO .333333 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,333333")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 333), Qt::LocalTime);
    QTest::newRow("ISO .00009 of a second (period)") << QString::fromLatin1("2012-01-01T08:00:00.00009")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO no fract specified") << QString::fromLatin1("2012-01-01T08:00:00.")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    // Test invalid characters (should ignore invalid characters at end of string).
    QTest::newRow("ISO invalid character at end") << QString::fromLatin1("2012-01-01T08:00:00!")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO invalid character at front") << QString::fromLatin1("!2012-01-01T08:00:00")
        << Qt::ISODate << invalidDateTime();
    QTest::newRow("ISO invalid character both ends") << QString::fromLatin1("!2012-01-01T08:00:00!")
        << Qt::ISODate << invalidDateTime();
    QTest::newRow("ISO invalid character at front, 2 at back") << QString::fromLatin1("!2012-01-01T08:00:00..")
        << Qt::ISODate << invalidDateTime();
    QTest::newRow("ISO invalid character 2 at front") << QString::fromLatin1("!!2012-01-01T08:00:00")
        << Qt::ISODate << invalidDateTime();
    // Test fractional minutes.
    QTest::newRow("ISO .0 of a minute (period)") << QString::fromLatin1("2012-01-01T08:00.0")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO .8 of a minute (period)") << QString::fromLatin1("2012-01-01T08:00.8")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 48, 0), Qt::LocalTime);
    QTest::newRow("ISO .99999 of a minute (period)") << QString::fromLatin1("2012-01-01T08:00.99999")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 59, 999), Qt::LocalTime);
    QTest::newRow("ISO .0 of a minute (comma)") << QString::fromLatin1("2012-01-01T08:00,0")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO .8 of a minute (comma)") << QString::fromLatin1("2012-01-01T08:00,8")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 48, 0), Qt::LocalTime);
    QTest::newRow("ISO .99999 of a minute (comma)") << QString::fromLatin1("2012-01-01T08:00,99999")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 59, 999), Qt::LocalTime);
    QTest::newRow("ISO empty") << QString::fromLatin1("") << Qt::ISODate << invalidDateTime();
}

void tst_QDateTime::fromStringDateFormat()
{
    QFETCH(QString, dateTimeStr);
    QFETCH(Qt::DateFormat, dateFormat);
    QFETCH(QDateTime, expected);

    QDateTime dateTime = QDateTime::fromString(dateTimeStr, dateFormat);
    QCOMPARE(dateTime, expected);
}

void tst_QDateTime::fromStringStringFormat_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QDateTime>("expected");

    QString january = QDate::longMonthName(1);
    QString oct = QDate::shortMonthName(10);
    QString december = QDate::longMonthName(12);
    QString thu = QDate::shortDayName(4);
    QString fri = QDate::shortDayName(5);
    QString date = "10 " + oct + " 10";

    QTest::newRow("data0") << QString("101010") << QString("dMyy") << QDateTime(QDate(1910, 10, 10), QTime());
    QTest::newRow("data1") << QString("1020") << QString("sss") << invalidDateTime();
    QTest::newRow("data2") << QString("1010") << QString("sss") << QDateTime(defDate(), QTime(0, 0, 10));
    QTest::newRow("data3") << QString("10hello20") << QString("ss'hello'ss") << invalidDateTime();
    QTest::newRow("data4") << QString("10") << QString("''") << invalidDateTime();
    QTest::newRow("data5") << QString("10") << QString("'") << invalidDateTime();
    QTest::newRow("data6") << QString("pm") << QString("ap") << QDateTime(defDate(), QTime(12, 0, 0));
    QTest::newRow("data7") << QString("foo") << QString("ap") << invalidDateTime();
    QTest::newRow("data8") << QString("101010") << QString("dMyy") << QDateTime(QDate(1910, 10, 10), QTime());
    QTest::newRow("data9") << QString("101010") << QString("dMyy") << QDateTime(QDate(1910, 10, 10), QTime());
    QTest::newRow("data10") << QString("101010") << QString("dMyy") << QDateTime(QDate(1910, 10, 10), QTime());
    QTest::newRow("data11") << date << QString("dd MMM yy") << QDateTime(QDate(1910, 10, 10), QTime());
    date = fri + " " + december + " 3 2004";
    QTest::newRow("data12") << date << QString("ddd MMMM d yyyy") << QDateTime(QDate(2004, 12, 3), QTime());
    QTest::newRow("data13") << QString("30.02.2004") << QString("dd.MM.yyyy") << invalidDateTime();
    QTest::newRow("data14") << QString("32.01.2004") << QString("dd.MM.yyyy") << invalidDateTime();
    date = thu + " " + january + " 2004";
    QTest::newRow("data15") << date << QString("ddd MMMM yyyy") << QDateTime(QDate(2004, 1, 1), QTime());
    QTest::newRow("data16") << QString("2005-06-28T07:57:30.001Z")
                            << QString("yyyy-MM-ddThh:mm:ss.zZ")
                            << QDateTime(QDate(2005, 06, 28), QTime(07, 57, 30, 1));
}

void tst_QDateTime::fromStringStringFormat()
{
    QFETCH(QString, string);
    QFETCH(QString, format);
    QFETCH(QDateTime, expected);

    QDateTime dt = QDateTime::fromString(string, format);

    QCOMPARE(dt, expected);
}

#ifdef Q_OS_WIN
// Windows only
void tst_QDateTime::fromString_LOCALE_ILDATE()
{
    QString date1 = QLatin1String("Sun 1. Dec 13:02:00 1974");
    QString date2 = QLatin1String("Sun Dec 1 13:02:00 1974");

    QDateTime ref(QDate(1974, 12, 1), QTime(13, 2));
    QCOMPARE(ref, QDateTime::fromString(date2, Qt::TextDate));
    QCOMPARE(ref, QDateTime::fromString(date1, Qt::TextDate));
}
#endif

void tst_QDateTime::fromStringToStringLocale_data()
{
    QTest::addColumn<QDateTime>("dateTime");

    QTest::newRow("data0") << QDateTime(QDate(1999, 1, 18), QTime(11, 49, 00));
}

void tst_QDateTime::fromStringToStringLocale()
{
    QFETCH(QDateTime, dateTime);

    QLocale def;
    QLocale::setDefault(QLocale(QLocale::French, QLocale::France));

    QCOMPARE(QDateTime::fromString(dateTime.toString(Qt::DefaultLocaleShortDate), Qt::DefaultLocaleShortDate), dateTime);
    QCOMPARE(QDateTime::fromString(dateTime.toString(Qt::SystemLocaleShortDate), Qt::SystemLocaleShortDate), dateTime);

    // obsolete
    QCOMPARE(QDateTime::fromString(dateTime.toString(Qt::SystemLocaleDate), Qt::SystemLocaleDate), dateTime);
    QCOMPARE(QDateTime::fromString(dateTime.toString(Qt::LocaleDate), Qt::LocaleDate), dateTime);

    QEXPECT_FAIL("data0", "This format is apparently failing because of a bug in the datetime parser. (QTBUG-22833)", Continue);
    QCOMPARE(QDateTime::fromString(dateTime.toString(Qt::DefaultLocaleLongDate), Qt::DefaultLocaleLongDate), dateTime);
#ifndef Q_OS_WIN
    QEXPECT_FAIL("data0", "This format is apparently failing because of a bug in the datetime parser. (QTBUG-22833)", Continue);
#endif
    QCOMPARE(QDateTime::fromString(dateTime.toString(Qt::SystemLocaleLongDate), Qt::SystemLocaleLongDate), dateTime);

    QLocale::setDefault(def);
}

void tst_QDateTime::utcOffset()
{
    /* Check default value. */
    QCOMPARE(QDateTime().utcOffset(), 0);
}

void tst_QDateTime::setUtcOffset()
{
    /* Basic tests. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        dt.setTimeSpec(Qt::LocalTime);

        dt.setUtcOffset(0);
        QCOMPARE(dt.utcOffset(), 0);
        QCOMPARE(dt.timeSpec(), Qt::UTC);

        dt.setUtcOffset(-100);
        QCOMPARE(dt.utcOffset(), -100);
        QCOMPARE(dt.timeSpec(), Qt::OffsetFromUTC);
    }

    /* Test detaching. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        QDateTime dt2(dt);

        dt.setUtcOffset(501);

        QCOMPARE(dt.utcOffset(), 501);
        QCOMPARE(dt2.utcOffset(), 0);
    }

    /* Check copying. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        dt.setUtcOffset(502);
        QCOMPARE(dt.utcOffset(), 502);

        QDateTime dt2(dt);
        QCOMPARE(dt2.utcOffset(), 502);
    }

    /* Check assignment. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        dt.setUtcOffset(502);
        QDateTime dt2;
        dt2 = dt;

        QCOMPARE(dt2.utcOffset(), 502);
    }
}

void tst_QDateTime::getDate()
{
    {
    int y = -33, m = -44, d = -55;
    QDate date;
    date.getDate(&y, &m, &d);
    QVERIFY(date.year() == y);
    QVERIFY(date.month() == m);
    QVERIFY(date.day() == d);

    date.getDate(0, 0, 0);
    }

    {
    int y = -33, m = -44, d = -55;
    QDate date(1998, 5, 24);
    date.getDate(0, &m, 0);
    date.getDate(&y, 0, 0);
    date.getDate(0, 0, &d);

    QVERIFY(date.year() == y);
    QVERIFY(date.month() == m);
    QVERIFY(date.day() == d);
    }
}

void tst_QDateTime::fewDigitsInYear() const
{
    const QDateTime three(QDate(300, 10, 11), QTime());
    QCOMPARE(three.toString(QLatin1String("yyyy-MM-dd")), QString::fromLatin1("0300-10-11"));

    const QDateTime two(QDate(20, 10, 11), QTime());
    QCOMPARE(two.toString(QLatin1String("yyyy-MM-dd")), QString::fromLatin1("0020-10-11"));

    const QDateTime yyTwo(QDate(30, 10, 11), QTime());
    QCOMPARE(yyTwo.toString(QLatin1String("yy-MM-dd")), QString::fromLatin1("30-10-11"));

    const QDateTime yyOne(QDate(4, 10, 11), QTime());
    QCOMPARE(yyOne.toString(QLatin1String("yy-MM-dd")), QString::fromLatin1("04-10-11"));
}

void tst_QDateTime::printNegativeYear() const
{
    {
        QDateTime date(QDate(-20, 10, 11));
        QVERIFY(date.isValid());
        QCOMPARE(date.toString(QLatin1String("yyyy")), QString::fromLatin1("-0020"));
    }

    {
        QDateTime date(QDate(-3, 10, 11));
        QVERIFY(date.isValid());
        QCOMPARE(date.toString(QLatin1String("yyyy")), QString::fromLatin1("-0003"));
    }

    {
        QDateTime date(QDate(-400, 10, 11));
        QVERIFY(date.isValid());
        QCOMPARE(date.toString(QLatin1String("yyyy")), QString::fromLatin1("-0400"));
    }
}

void tst_QDateTime::roundtripGermanLocale() const
{
    /* This code path should not result in warnings. */
    const QDateTime theDateTime(QDateTime::currentDateTime());
    theDateTime.fromString(theDateTime.toString(Qt::TextDate), Qt::TextDate);
}

void tst_QDateTime::utcOffsetLessThan() const
{
    QDateTime dt1(QDate(2002, 10, 10), QTime(0, 0, 0));
    QDateTime dt2(dt1);

    dt1.setUtcOffset(-(2 * 60 * 60)); // Minus two hours.
    dt2.setUtcOffset(-(3 * 60 * 60)); // Minus three hours.

    QVERIFY(dt1 != dt2);
    QVERIFY(!(dt1 == dt2));
    QVERIFY(dt1 < dt2);
    QVERIFY(!(dt2 < dt1));
}

QTEST_APPLESS_MAIN(tst_QDateTime)
#include "tst_qdatetime.moc"
