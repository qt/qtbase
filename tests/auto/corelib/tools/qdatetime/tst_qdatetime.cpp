/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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
#include <time.h>
#include <qdatetime.h>
#include <private/qdatetime_p.h>

#ifdef Q_OS_WIN
#   include <qt_windows.h>
#  if defined(Q_OS_WINRT)
#    define tzset()
#  endif
#endif

class tst_QDateTime : public QObject
{
    Q_OBJECT

public:
    tst_QDateTime();

    static QString str( int y, int month, int d, int h, int min, int s );
    static QDateTime dt( const QString& str );
public slots:
    void initTestCase();
    void init();
private slots:
    void ctor();
    void operator_eq();
    void isNull();
    void isValid();
    void date();
    void time();
    void timeSpec();
    void toSecsSinceEpoch_data();
    void toSecsSinceEpoch();
    void daylightSavingsTimeChange_data();
    void daylightSavingsTimeChange();
    void springForward_data();
    void springForward();
    void setDate();
    void setTime_data();
    void setTime();
    void setTimeSpec_data();
    void setTimeSpec();
    void setSecsSinceEpoch();
    void setMSecsSinceEpoch_data();
    void setMSecsSinceEpoch();
    void fromMSecsSinceEpoch_data();
    void fromMSecsSinceEpoch();
    void toString_isoDate_data();
    void toString_isoDate();
    void toString_textDate_data();
    void toString_textDate();
    void toString_rfcDate_data();
    void toString_rfcDate();
    void toString_enumformat();
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
    void operator_insert_extract_data();
    void operator_insert_extract();
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

    void offsetFromUtc();
    void setOffsetFromUtc();
    void toOffsetFromUtc();

    void timeZoneAbbreviation();

    void getDate();

    void fewDigitsInYear() const;
    void printNegativeYear() const;
    void roundtripGermanLocale() const;
    void utcOffsetLessThan() const;

    void isDaylightTime() const;
    void daylightTransitions() const;
    void timeZones() const;
#if defined(Q_OS_UNIX)
    void systemTimeZoneChange() const;
#endif

    void invalid() const;

    void macTypes();

private:
    enum { LocalTimeIsUtc = 0, LocalTimeAheadOfUtc = 1, LocalTimeBehindUtc = -1} localTimeType;
    bool zoneIsCET;
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
    /*
      Due to some jurisdictions changing their zones and rules, it's possible
      for a non-CET zone to accidentally match CET at a few tested moments but
      be different a few years later or earlier.  This would lead to tests
      failing if run in the partially-aliasing zone (e.g. Algeria, Lybia).  So
      test thoroughly; ideally at every mid-winter or mid-summer in whose
      half-year any test below assumes zoneIsCET means what it says.  (Tests at
      or near a DST transition implicate both of the half-years that meet
      there.)  Years outside the 1970--2038 range, however, are likely not
      properly handled by the TZ-database; and QDateTime explicitly handles them
      differently, so don't probe them here.
    */
    const uint day = 24 * 3600; // in seconds
    zoneIsCET = (QDateTime(QDate(2038, 1, 19), QTime(4, 14, 7)).toSecsSinceEpoch() == 0x7fffffff
                 // Entries a year apart robustly differ by multiples of day.
                 && QDateTime(QDate(2015, 7, 1), QTime()).toSecsSinceEpoch() == 1435701600
                 && QDateTime(QDate(2015, 1, 1), QTime()).toSecsSinceEpoch() == 1420066800
                 && QDateTime(QDate(2013, 7, 1), QTime()).toSecsSinceEpoch() == 1372629600
                 && QDateTime(QDate(2013, 1, 1), QTime()).toSecsSinceEpoch() == 1356994800
                 && QDateTime(QDate(2012, 7, 1), QTime()).toSecsSinceEpoch() == 1341093600
                 && QDateTime(QDate(2012, 1, 1), QTime()).toSecsSinceEpoch() == 1325372400
                 && QDateTime(QDate(2008, 7, 1), QTime()).toSecsSinceEpoch() == 1214863200
                 && QDateTime(QDate(2004, 1, 1), QTime()).toSecsSinceEpoch() == 1072911600
                 && QDateTime(QDate(2000, 1, 1), QTime()).toSecsSinceEpoch() == 946681200
                 && QDateTime(QDate(1990, 7, 1), QTime()).toSecsSinceEpoch() == 646783200
                 && QDateTime(QDate(1990, 1, 1), QTime()).toSecsSinceEpoch() == 631148400
                 && QDateTime(QDate(1979, 1, 1), QTime()).toSecsSinceEpoch() == 283993200
                 // .toSecsSinceEpoch() returns -1 for everything before this:
                 && QDateTime(QDate(1970, 1, 1), QTime(1, 0, 0)).toSecsSinceEpoch() == 0);
    // Use .toMSecsSinceEpoch() if you really need to test anything earlier.

    /*
      Again, rule changes can cause a TZ to look like UTC at some sample dates
      but deviate at some date relevant to a test using localTimeType.  These
      tests mostly use years outside the 1970--2038 range for which TZ data is
      credible, so we can't helpfully be exhaustive.  So scan a sample of years'
      starts and middles.
    */
    const int sampled = 3;
    // UTC starts of months in 2004, 2038 and 1970:
    qint64 jans[sampled] = { 12418 * day, 24837 * day, 0 };
    qint64 juls[sampled] = { 12600 * day, 25018 * day, 181 * day };
    localTimeType = LocalTimeIsUtc;
    for (int i = sampled; i-- > 0; ) {
        QDateTime jan = QDateTime::fromSecsSinceEpoch(jans[i]);
        QDateTime jul = QDateTime::fromSecsSinceEpoch(juls[i]);
        if (jan.date().year() < 1970 || jul.date().month() < 7) {
            localTimeType = LocalTimeBehindUtc;
            break;
        } else if (jan.time().hour() > 0 || jul.time().hour() > 0
                   || jan.date().day() > 1 || jul.date().day() > 1) {
            localTimeType = LocalTimeAheadOfUtc;
            break;
        }
    }
    /*
      Even so, TZ=Africa/Algiers will fail fromMSecsSinceEpoch(-1) because it
      switched from WET without DST (i.e. UTC) in the late 1960s to WET with DST
      for all of 1970 - so they had a DST transition *on the epoch*.  They've
      since switched to CET with no DST, making life simple; but our tests for
      mistakes around the epoch can't tell the difference between what Algeria
      really did and the symptoms we can believe a bug might produce: there's
      not much we can do about that, that wouldn't hide real bugs.
    */
}

void tst_QDateTime::initTestCase()
{
    // Never construct a message like this in an i18n context...
    const char *typemsg1 = "exactly";
    const char *typemsg2 = "and therefore not";
    switch (localTimeType) {
    case LocalTimeIsUtc:
        break;
    case LocalTimeBehindUtc:
        typemsg1 = "behind";
        break;
    case LocalTimeAheadOfUtc:
        typemsg1 = "ahead of";
        typemsg2 = zoneIsCET ? "and is" : "but isn't";
        break;
    }

    qDebug() << "Current local time detected to be"
             << typemsg1
             << "UTC"
             << typemsg2
             << "the Central European timezone";
}

void tst_QDateTime::init()
{
#if defined(Q_OS_WIN32)
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
    QCOMPARE(dt1.timeSpec(), Qt::LocalTime);
    QDateTime dt2(QDate(2004, 1, 2), QTime(1, 2, 3), Qt::LocalTime);
    QCOMPARE(dt2.timeSpec(), Qt::LocalTime);
    QDateTime dt3(QDate(2004, 1, 2), QTime(1, 2, 3), Qt::UTC);
    QCOMPARE(dt3.timeSpec(), Qt::UTC);

    QVERIFY(dt1 == dt2);
    if (zoneIsCET) {
        QVERIFY(dt1 != dt3);
        QVERIFY(dt1 < dt3);
        QVERIFY(dt1.addSecs(3600).toUTC() == dt3);
    }

    // Test OffsetFromUTC constructors
    QDate offsetDate(2013, 1, 1);
    QTime offsetTime(1, 2, 3);

    QDateTime offset1(offsetDate, offsetTime, Qt::OffsetFromUTC);
    QCOMPARE(offset1.timeSpec(), Qt::UTC);
    QCOMPARE(offset1.offsetFromUtc(), 0);
    QCOMPARE(offset1.date(), offsetDate);
    QCOMPARE(offset1.time(), offsetTime);

    QDateTime offset2(offsetDate, offsetTime, Qt::OffsetFromUTC, 0);
    QCOMPARE(offset2.timeSpec(), Qt::UTC);
    QCOMPARE(offset2.offsetFromUtc(), 0);
    QCOMPARE(offset2.date(), offsetDate);
    QCOMPARE(offset2.time(), offsetTime);

    QDateTime offset3(offsetDate, offsetTime, Qt::OffsetFromUTC, 60 * 60);
    QCOMPARE(offset3.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(offset3.offsetFromUtc(), 60 * 60);
    QCOMPARE(offset3.date(), offsetDate);
    QCOMPARE(offset3.time(), offsetTime);

    QDateTime offset4(offsetDate, QTime(), Qt::OffsetFromUTC, 60 * 60);
    QCOMPARE(offset4.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(offset4.offsetFromUtc(), 60 * 60);
    QCOMPARE(offset4.date(), offsetDate);
    QCOMPARE(offset4.time(), QTime(0, 0, 0));
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
    if (newTimeSpec == Qt::OffsetFromUTC)
        QCOMPARE(dateTime.timeSpec(), Qt::UTC);
    else
        QCOMPARE(dateTime.timeSpec(), newTimeSpec);
}

void tst_QDateTime::setSecsSinceEpoch()
{
    QDateTime dt1;
    dt1.setSecsSinceEpoch(0);
    QCOMPARE(dt1.toUTC(), QDateTime(QDate(1970, 1, 1), QTime(), Qt::UTC));
    QCOMPARE(dt1.timeSpec(), Qt::LocalTime);

    dt1.setTimeSpec(Qt::UTC);
    dt1.setSecsSinceEpoch(0);
    QCOMPARE(dt1, QDateTime(QDate(1970, 1, 1), QTime(), Qt::UTC));
    QCOMPARE(dt1.timeSpec(), Qt::UTC);

    dt1.setSecsSinceEpoch(123456);
    QCOMPARE(dt1, QDateTime(QDate(1970, 1, 2), QTime(10, 17, 36), Qt::UTC));
    if (zoneIsCET) {
        QDateTime dt2;
        dt2.setSecsSinceEpoch(123456);
        QCOMPARE(dt2, QDateTime(QDate(1970, 1, 2), QTime(11, 17, 36), Qt::LocalTime));
    }

    dt1.setSecsSinceEpoch((uint)(quint32)-123456);
    QCOMPARE(dt1, QDateTime(QDate(2106, 2, 5), QTime(20, 10, 40), Qt::UTC));
    if (zoneIsCET) {
        QDateTime dt2;
        dt2.setSecsSinceEpoch((uint)(quint32)-123456);
        QCOMPARE(dt2, QDateTime(QDate(2106, 2, 5), QTime(21, 10, 40), Qt::LocalTime));
    }

    dt1.setSecsSinceEpoch(1214567890);
    QCOMPARE(dt1, QDateTime(QDate(2008, 6, 27), QTime(11, 58, 10), Qt::UTC));
    if (zoneIsCET) {
        QDateTime dt2;
        dt2.setSecsSinceEpoch(1214567890);
        QCOMPARE(dt2, QDateTime(QDate(2008, 6, 27), QTime(13, 58, 10), Qt::LocalTime));
    }

    dt1.setSecsSinceEpoch(0x7FFFFFFF);
    QCOMPARE(dt1, QDateTime(QDate(2038, 1, 19), QTime(3, 14, 7), Qt::UTC));
    if (zoneIsCET) {
        QDateTime dt2;
        dt2.setSecsSinceEpoch(0x7FFFFFFF);
        QCOMPARE(dt2, QDateTime(QDate(2038, 1, 19), QTime(4, 14, 7), Qt::LocalTime));
    }

    dt1 = QDateTime(QDate(2013, 1, 1), QTime(0, 0, 0), Qt::OffsetFromUTC, 60 * 60);
    dt1.setSecsSinceEpoch(123456);
    QCOMPARE(dt1, QDateTime(QDate(1970, 1, 2), QTime(10, 17, 36), Qt::UTC));
    QCOMPARE(dt1.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt1.offsetFromUtc(), 60 * 60);
}

void tst_QDateTime::setMSecsSinceEpoch_data()
{
    QTest::addColumn<qint64>("msecs");
    QTest::addColumn<QDateTime>("utc");
    QTest::addColumn<QDateTime>("cet");

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
            << QDateTime(QDate(-292275056, 5, 16), QTime(17, 47, 4, 193), Qt::LocalTime);
    QTest::newRow("max")
            << std::numeric_limits<qint64>::max()
            << QDateTime(QDate(292278994, 8, 17), QTime(7, 12, 55, 807), Qt::UTC)
            << QDateTime(QDate(292278994, 8, 17), QTime(9, 12, 55, 807), Qt::LocalTime);
}

void tst_QDateTime::setMSecsSinceEpoch()
{
    QFETCH(qint64, msecs);
    QFETCH(QDateTime, utc);
    QFETCH(QDateTime, cet);

    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setMSecsSinceEpoch(msecs);

    QCOMPARE(dt, utc);
    QCOMPARE(dt.date(), utc.date());
    QCOMPARE(dt.time(), utc.time());
    QCOMPARE(dt.timeSpec(), Qt::UTC);

    if (zoneIsCET) {
        QCOMPARE(dt.toLocalTime(), cet);

        // Test converting from LocalTime to UTC back to LocalTime.
        QDateTime localDt;
        localDt.setTimeSpec(Qt::LocalTime);
        localDt.setMSecsSinceEpoch(msecs);

        // LocalTime will overflow for max
        if (msecs != std::numeric_limits<qint64>::max())
            QCOMPARE(localDt, utc);
        QCOMPARE(localDt.timeSpec(), Qt::LocalTime);

        // Compare result for LocalTime to TimeZone
        QTimeZone europe("Europe/Oslo");
        QDateTime dt2;
        dt2.setTimeZone(europe);
        dt2.setMSecsSinceEpoch(msecs);
        QCOMPARE(dt2.date(), cet.date());

        // don't compare the time if the date is too early or too late: prior
        // to 1916, timezones in Europe were not standardised and some OS APIs
        // have hard limits. Let's restrict it to the 32-bit Unix range
        if (dt2.date().year() >= 1970 && dt2.date().year() <= 2037)
            QCOMPARE(dt2.time(), cet.time());
        QCOMPARE(dt2.timeSpec(), Qt::TimeZone);
        QCOMPARE(dt2.timeZone(), europe);
    }

    QCOMPARE(dt.toMSecsSinceEpoch(), msecs);

    if (quint64(msecs / 1000) < 0xFFFFFFFF) {
        QCOMPARE(qint64(dt.toSecsSinceEpoch()), msecs / 1000);
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
    QFETCH(QDateTime, cet);

    QDateTime dtLocal = QDateTime::fromMSecsSinceEpoch(msecs, Qt::LocalTime);
    QDateTime dtUtc = QDateTime::fromMSecsSinceEpoch(msecs, Qt::UTC);
    QDateTime dtOffset = QDateTime::fromMSecsSinceEpoch(msecs, Qt::OffsetFromUTC, 60*60);

    // LocalTime will overflow for min or max, depending on whether you're
    // East or West of Greenwich. The test passes at GMT.
    if (localTimeType == LocalTimeIsUtc
            || msecs != std::numeric_limits<qint64>::max() * localTimeType)
        QCOMPARE(dtLocal, utc);

    QCOMPARE(dtUtc, utc);
    QCOMPARE(dtUtc.date(), utc.date());
    QCOMPARE(dtUtc.time(), utc.time());

    QCOMPARE(dtOffset, utc);
    QCOMPARE(dtOffset.offsetFromUtc(), 60*60);
    // // OffsetFromUTC will overflow for max
    if (msecs != std::numeric_limits<qint64>::max())
        QCOMPARE(dtOffset.time(), utc.time().addMSecs(60*60*1000));

    if (zoneIsCET) {
        QCOMPARE(dtLocal.toLocalTime(), cet);
        QCOMPARE(dtUtc.toLocalTime(), cet);
        QCOMPARE(dtOffset.toLocalTime(), cet);
    }

    // LocalTime will overflow for max
    if (msecs != std::numeric_limits<qint64>::max())
        QCOMPARE(dtLocal.toMSecsSinceEpoch(), msecs);
    QCOMPARE(dtUtc.toMSecsSinceEpoch(), msecs);
    QCOMPARE(dtOffset.toMSecsSinceEpoch(), msecs);

    if (quint64(msecs / 1000) < 0xFFFFFFFF) {
        QCOMPARE(qint64(dtLocal.toSecsSinceEpoch()), msecs / 1000);
        QCOMPARE(qint64(dtUtc.toSecsSinceEpoch()), msecs / 1000);
        QCOMPARE(qint64(dtOffset.toSecsSinceEpoch()), msecs / 1000);
    }

    QDateTime reference(QDate(1970, 1, 1), QTime(), Qt::UTC);
    // LocalTime will overflow for max
    if (msecs != std::numeric_limits<qint64>::max())
        QCOMPARE(dtLocal, reference.addMSecs(msecs));
    QCOMPARE(dtUtc, reference.addMSecs(msecs));
    QCOMPARE(dtOffset, reference.addMSecs(msecs));
}

void tst_QDateTime::toString_isoDate_data()
{
    QTest::addColumn<QDateTime>("datetime");
    QTest::addColumn<Qt::DateFormat>("format");
    QTest::addColumn<QString>("expected");

    QTest::newRow("localtime")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34))
            << Qt::ISODate << QString("1978-11-09T13:28:34");
    QTest::newRow("UTC")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34), Qt::UTC)
            << Qt::ISODate << QString("1978-11-09T13:28:34Z");
    QDateTime dt(QDate(1978, 11, 9), QTime(13, 28, 34));
    dt.setOffsetFromUtc(19800);
    QTest::newRow("positive OffsetFromUTC")
            << dt << Qt::ISODate
            << QString("1978-11-09T13:28:34+05:30");
    dt.setUtcOffset(-7200);
    QTest::newRow("negative OffsetFromUTC")
            << dt << Qt::ISODate
            << QString("1978-11-09T13:28:34-02:00");
    dt.setUtcOffset(-900);
    QTest::newRow("negative non-integral OffsetFromUTC")
            << dt << Qt::ISODate
            << QString("1978-11-09T13:28:34-00:15");
    QTest::newRow("invalid")
            << QDateTime(QDate(-1, 11, 9), QTime(13, 28, 34), Qt::UTC)
            << Qt::ISODate << QString();
    QTest::newRow("without-ms")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34, 20))
            << Qt::ISODate << QString("1978-11-09T13:28:34");
    QTest::newRow("with-ms")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34, 20))
            << Qt::ISODateWithMs << QString("1978-11-09T13:28:34.020");
}

void tst_QDateTime::toString_isoDate()
{
    QFETCH(QDateTime, datetime);
    QFETCH(Qt::DateFormat, format);
    QFETCH(QString, expected);

    QLocale oldLocale;
    QLocale::setDefault(QLocale("en_US"));

    QString result = datetime.toString(format);
    QCOMPARE(result, expected);

    QDateTime resultDatetime = QDateTime::fromString(result, format);
    // If expecting invalid result the datetime may still be valid, i.e. year < 0 or > 9999
    if (!expected.isEmpty()) {
        QEXPECT_FAIL("without-ms", "Qt::ISODate truncates milliseconds (QTBUG-56552)", Abort);

        QCOMPARE(resultDatetime, datetime);
        QCOMPARE(resultDatetime.date(), datetime.date());
        QCOMPARE(resultDatetime.time(), datetime.time());
        QCOMPARE(resultDatetime.timeSpec(), datetime.timeSpec());
        QCOMPARE(resultDatetime.utcOffset(), datetime.utcOffset());
    } else {
        QCOMPARE(resultDatetime, QDateTime());
    }

    QLocale::setDefault(oldLocale);
}

void tst_QDateTime::toString_textDate_data()
{
    QTest::addColumn<QDateTime>("datetime");
    QTest::addColumn<QString>("expected");

    QString wednesdayJanuary = QDate::shortDayName(3) + ' ' + QDate::shortMonthName(1);

    QTest::newRow("localtime")  << QDateTime(QDate(2013, 1, 2), QTime(1, 2, 3), Qt::LocalTime)
                                << wednesdayJanuary + QString(" 2 01:02:03 2013");
    QTest::newRow("utc")        << QDateTime(QDate(2013, 1, 2), QTime(1, 2, 3), Qt::UTC)
                                << wednesdayJanuary + QString(" 2 01:02:03 2013 GMT");
    QTest::newRow("offset+")    << QDateTime(QDate(2013, 1, 2), QTime(1, 2, 3), Qt::OffsetFromUTC,
                                             10 * 60 * 60)
                                << wednesdayJanuary + QString(" 2 01:02:03 2013 GMT+1000");
    QTest::newRow("offset-")    << QDateTime(QDate(2013, 1, 2), QTime(1, 2, 3), Qt::OffsetFromUTC,
                                             -10 * 60 * 60)
                                << wednesdayJanuary + QString(" 2 01:02:03 2013 GMT-1000");
    QTest::newRow("invalid")    << QDateTime()
                                << QString("");
}

void tst_QDateTime::toString_textDate()
{
    QFETCH(QDateTime, datetime);
    QFETCH(QString, expected);

    QString result = datetime.toString(Qt::TextDate);
    QCOMPARE(result, expected);

    QDateTime resultDatetime = QDateTime::fromString(result, Qt::TextDate);
    QCOMPARE(resultDatetime, datetime);
    QCOMPARE(resultDatetime.date(), datetime.date());
    QCOMPARE(resultDatetime.time(), datetime.time());
    QCOMPARE(resultDatetime.timeSpec(), datetime.timeSpec());
    QCOMPARE(resultDatetime.utcOffset(), datetime.utcOffset());
}

void tst_QDateTime::toString_rfcDate_data()
{
    QTest::addColumn<QDateTime>("dt");
    QTest::addColumn<QString>("formatted");

    if (zoneIsCET) {
        QTest::newRow("localtime")
                << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34))
                << QString("09 Nov 1978 13:28:34 +0100");
    }
    QTest::newRow("UTC")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34), Qt::UTC)
            << QString("09 Nov 1978 13:28:34 +0000");
    QDateTime dt(QDate(1978, 11, 9), QTime(13, 28, 34));
    dt.setUtcOffset(19800);
    QTest::newRow("positive OffsetFromUTC")
            << dt
            << QString("09 Nov 1978 13:28:34 +0530");
    dt.setUtcOffset(-7200);
    QTest::newRow("negative OffsetFromUTC")
            << dt
            << QString("09 Nov 1978 13:28:34 -0200");
    QTest::newRow("invalid")
            << QDateTime(QDate(1978, 13, 9), QTime(13, 28, 34), Qt::UTC)
            << QString();
    QTest::newRow("999 milliseconds UTC")
            << QDateTime(QDate(2000, 1, 1), QTime(13, 28, 34, 999), Qt::UTC)
            << QString("01 Jan 2000 13:28:34 +0000");
}

void tst_QDateTime::toString_rfcDate()
{
    QFETCH(QDateTime, dt);
    QFETCH(QString, formatted);

    // Set to non-English locale to confirm still uses English
    QLocale oldLocale;
    QLocale::setDefault(QLocale("de_DE"));
    QCOMPARE(dt.toString(Qt::RFC2822Date), formatted);
    QLocale::setDefault(oldLocale);
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

    // Test preserves TimeSpec
    QDateTime dt1(QDate(2013, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime dt2 = dt1.addDays(2);
    QCOMPARE(dt2.date(), QDate(2013, 1, 3));
    QCOMPARE(dt2.time(), QTime(0, 0, 0));
    QCOMPARE(dt2.timeSpec(), Qt::UTC);

    dt1 = QDateTime(QDate(2013, 1, 1), QTime(0, 0, 0), Qt::LocalTime);
    dt2 = dt1.addDays(2);
    QCOMPARE(dt2.date(), QDate(2013, 1, 3));
    QCOMPARE(dt2.time(), QTime(0, 0, 0));
    QCOMPARE(dt2.timeSpec(), Qt::LocalTime);

    dt1 = QDateTime(QDate(2013, 1, 1), QTime(0, 0, 0), Qt::OffsetFromUTC, 60*60);
    dt2 = dt1.addDays(2);
    QCOMPARE(dt2.date(), QDate(2013, 1, 3));
    QCOMPARE(dt2.time(), QTime(0, 0, 0));
    QCOMPARE(dt2.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt2.offsetFromUtc(), 60 * 60);

    // ### test invalid QDateTime()
}


void tst_QDateTime::addMonths_data()
{
    QTest::addColumn<int>("months");
    QTest::addColumn<QDate>("resultDate");

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
    QFETCH(int, months);
    QFETCH(QDate, resultDate);

    QDate testDate(2004, 1, 31);
    QTime testTime(12, 34, 56);
    QDateTime start(testDate, testTime);
    QDateTime end = start.addMonths(months);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::LocalTime);

    start = QDateTime(testDate, testTime, Qt::UTC);
    end = start.addMonths(months);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::UTC);

    start = QDateTime(testDate, testTime, Qt::OffsetFromUTC, 60 * 60);
    end = start.addMonths(months);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(end.offsetFromUtc(), 60 * 60);
}

void tst_QDateTime::addYears_data()
{
    QTest::addColumn<int>("years1");
    QTest::addColumn<int>("years2");
    QTest::addColumn<QDate>("startDate");
    QTest::addColumn<QDate>("resultDate");

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
    QFETCH(QDate, startDate);
    QFETCH(QDate, resultDate);

    QTime testTime(14, 25, 36);
    QDateTime start(startDate, testTime);
    QDateTime end = start.addYears(years1).addYears(years2);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::LocalTime);

    start = QDateTime(startDate, testTime, Qt::UTC);
    end = start.addYears(years1).addYears(years2);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::UTC);

    start = QDateTime(startDate, testTime, Qt::OffsetFromUTC, 60 * 60);
    end = start.addYears(years1).addYears(years2);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(end.offsetFromUtc(), 60 * 60);
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

    if (zoneIsCET) {
        QTest::newRow("cet0") << QDateTime(QDate(2004, 1, 1), standardTime, Qt::LocalTime) << 86400
                           << QDateTime(QDate(2004, 1, 2), standardTime, Qt::LocalTime);
        QTest::newRow("cet1") << QDateTime(QDate(2004, 1, 1), standardTime, Qt::LocalTime) << (86400 * 185)
                           << QDateTime(QDate(2004, 7, 4), daylightTime, Qt::LocalTime);
        QTest::newRow("cet2") << QDateTime(QDate(2004, 1, 1), standardTime, Qt::LocalTime) << (86400 * 366)
                           << QDateTime(QDate(2005, 1, 1), standardTime, Qt::LocalTime);
        QTest::newRow("cet3") << QDateTime(QDate(1760, 1, 1), standardTime, Qt::LocalTime) << 86400
                           << QDateTime(QDate(1760, 1, 2), standardTime, Qt::LocalTime);
        QTest::newRow("cet4") << QDateTime(QDate(1760, 1, 1), standardTime, Qt::LocalTime) << (86400 * 185)
                           << QDateTime(QDate(1760, 7, 4), standardTime, Qt::LocalTime);
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

    // Check Offset details are preserved
    QTest::newRow("offset0") << QDateTime(QDate(2013, 1, 1), QTime(1, 2, 3),
                                          Qt::OffsetFromUTC, 60 * 60)
                             << 60 * 60
                             << QDateTime(QDate(2013, 1, 1), QTime(2, 2, 3),
                                          Qt::OffsetFromUTC, 60 * 60);
}

void tst_QDateTime::addSecs()
{
    QFETCH(QDateTime, dt);
    QFETCH(int, nsecs);
    QFETCH(QDateTime, result);
#ifdef Q_OS_IRIX
    QEXPECT_FAIL("cet4", "IRIX databases say 1970 had DST", Abort);
#endif
    QDateTime test = dt.addSecs(nsecs);
    QCOMPARE(test, result);
    QCOMPARE(test.timeSpec(), dt.timeSpec());
    if (test.timeSpec() == Qt::OffsetFromUTC)
        QCOMPARE(test.offsetFromUtc(), dt.offsetFromUtc());
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
    QDateTime test = dt.addMSecs(qint64(nsecs) * 1000);
    QCOMPARE(test, result);
    QCOMPARE(test.timeSpec(), dt.timeSpec());
    if (test.timeSpec() == Qt::OffsetFromUTC)
        QCOMPARE(test.offsetFromUtc(), dt.offsetFromUtc());
    QCOMPARE(result.addMSecs(qint64(-nsecs) * 1000), dt);
}

void tst_QDateTime::toTimeSpec_data()
{
    QTest::addColumn<QDateTime>("fromUtc");
    QTest::addColumn<QDateTime>("fromLocal");

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
        << QDateTime(QDate(-271821, 4, 20), QTime(1, 0, 0), Qt::LocalTime);
    QTest::newRow("-271821/4/20 23:00 UTC (JavaScript min date, end of day)")
        << QDateTime(QDate(-271821, 4, 20), QTime(23, 0, 0), Qt::UTC)
        << QDateTime(QDate(-271821, 4, 21), QTime(0, 0, 0), Qt::LocalTime);

    if (zoneIsCET) {
        QTest::newRow("summer1") << QDateTime(QDate(2004, 6, 30), utcTime, Qt::UTC)
                                 << QDateTime(QDate(2004, 6, 30), localDaylightTime, Qt::LocalTime);
        QTest::newRow("summer2") << QDateTime(QDate(1760, 6, 30), utcTime, Qt::UTC)
                                 << QDateTime(QDate(1760, 6, 30), localStandardTime, Qt::LocalTime);
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
    if (zoneIsCET) {
        QFETCH(QDateTime, fromUtc);
        QFETCH(QDateTime, fromLocal);

        QDateTime utcToUtc = fromUtc.toTimeSpec(Qt::UTC);
        QDateTime localToLocal = fromLocal.toTimeSpec(Qt::LocalTime);
        QDateTime utcToLocal = fromUtc.toTimeSpec(Qt::LocalTime);
        QDateTime localToUtc = fromLocal.toTimeSpec(Qt::UTC);
        QDateTime utcToOffset = fromUtc.toTimeSpec(Qt::OffsetFromUTC);
        QDateTime localToOffset = fromLocal.toTimeSpec(Qt::OffsetFromUTC);

        QCOMPARE(utcToUtc, fromUtc);
        QCOMPARE(utcToUtc.date(), fromUtc.date());
        QCOMPARE(utcToUtc.time(), fromUtc.time());
        QCOMPARE(utcToUtc.timeSpec(), Qt::UTC);

        QCOMPARE(localToLocal, fromLocal);
        QCOMPARE(localToLocal.date(), fromLocal.date());
        QCOMPARE(localToLocal.time(), fromLocal.time());
        QCOMPARE(localToLocal.timeSpec(), Qt::LocalTime);

#ifdef Q_OS_IRIX
        QEXPECT_FAIL("summer2", "IRIX databases say 1970 had DST", Abort);
#endif
        QCOMPARE(utcToLocal, fromLocal);
        QCOMPARE(utcToLocal.date(), fromLocal.date());
        QCOMPARE(utcToLocal.time(), fromLocal.time());
        QCOMPARE(utcToLocal.timeSpec(), Qt::LocalTime);

        QCOMPARE(localToUtc, fromUtc);
        QCOMPARE(localToUtc.date(), fromUtc.date());
        QCOMPARE(localToUtc.time(), fromUtc.time());
        QCOMPARE(localToUtc.timeSpec(), Qt::UTC);

        QCOMPARE(utcToUtc, localToUtc);
        QCOMPARE(utcToUtc.date(), localToUtc.date());
        QCOMPARE(utcToUtc.time(), localToUtc.time());
        QCOMPARE(utcToUtc.timeSpec(), Qt::UTC);

        QCOMPARE(utcToLocal, localToLocal);
        QCOMPARE(utcToLocal.date(), localToLocal.date());
        QCOMPARE(utcToLocal.time(), localToLocal.time());
        QCOMPARE(utcToLocal.timeSpec(), Qt::LocalTime);

        // OffsetToUTC becomes UTC
        QCOMPARE(utcToOffset, fromUtc);
        QCOMPARE(utcToOffset.date(), fromUtc.date());
        QCOMPARE(utcToOffset.time(), fromUtc.time());
        QCOMPARE(utcToOffset.timeSpec(), Qt::UTC);

        QCOMPARE(localToOffset, fromUtc);
        QCOMPARE(localToOffset.date(), fromUtc.date());
        QCOMPARE(localToOffset.time(), fromUtc.time());
        QCOMPARE(localToOffset.timeSpec(), Qt::UTC);
    } else {
        QSKIP("Not tested with timezone other than Central European (CET/CEST)");
    }
}

void tst_QDateTime::toLocalTime_data()
{
    toTimeSpec_data();
}

void tst_QDateTime::toLocalTime()
{
    if (zoneIsCET) {
        QFETCH(QDateTime, fromUtc);
        QFETCH(QDateTime, fromLocal);

        QCOMPARE(fromLocal.toLocalTime(), fromLocal);
#ifdef Q_OS_IRIX
        QEXPECT_FAIL("summer2", "IRIX databases say 1970 had DST", Abort);
#endif
        QCOMPARE(fromUtc.toLocalTime(), fromLocal);
        QCOMPARE(fromUtc.toLocalTime(), fromLocal.toLocalTime());
    } else {
        QSKIP("Not tested with timezone other than Central European (CET/CEST)");
    }
}

void tst_QDateTime::toUTC_data()
{
    toTimeSpec_data();
}

void tst_QDateTime::toUTC()
{
    if (zoneIsCET) {
        QFETCH(QDateTime, fromUtc);
        QFETCH(QDateTime, fromLocal);

        QCOMPARE(fromUtc.toUTC(), fromUtc);
#ifdef Q_OS_IRIX
        QEXPECT_FAIL("summer2", "IRIX databases say 1970 had DST", Abort);
#endif
        QCOMPARE(fromLocal.toUTC(), fromUtc);
        QCOMPARE(fromUtc.toUTC(), fromLocal.toUTC());
    } else {
        QSKIP("Not tested with timezone other than Central European (CET/CEST)");
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
    time_t buf1, buf2;
    ::time(&buf1);
    QDateTime lowerBound;
    lowerBound.setSecsSinceEpoch(buf1);

    QDateTime dt1 = QDateTime::currentDateTime();
    QDateTime dt2 = QDateTime::currentDateTime().toLocalTime();
    QDateTime dt3 = QDateTime::currentDateTime().toUTC();

    ::time(&buf2);

    QDateTime upperBound;
    upperBound.setSecsSinceEpoch(buf2);
    // Note we must add 2 seconds here because time() may return up to
    // 1 second difference from the more accurate method used by QDateTime::currentDateTime()
    upperBound = upperBound.addSecs(2);

    QString details = QString("\n"
        "lowerBound: %1\n"
        "dt1:        %2\n"
        "dt2:        %3\n"
        "dt3:        %4\n"
        "upperBound: %5\n")
        .arg(lowerBound.toSecsSinceEpoch())
        .arg(dt1.toSecsSinceEpoch())
        .arg(dt2.toSecsSinceEpoch())
        .arg(dt3.toSecsSinceEpoch())
        .arg(upperBound.toSecsSinceEpoch());

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
    time_t buf1, buf2;
    ::time(&buf1);

    QDateTime lowerBound;
    lowerBound.setSecsSinceEpoch(buf1);

    QDateTime dt1 = QDateTime::currentDateTimeUtc();
    QDateTime dt2 = QDateTime::currentDateTimeUtc().toLocalTime();
    QDateTime dt3 = QDateTime::currentDateTimeUtc().toUTC();

    ::time(&buf2);

    QDateTime upperBound;
    upperBound.setSecsSinceEpoch(buf2);
    // Note we must add 2 seconds here because time() may return up to
    // 1 second difference from the more accurate method used by QDateTime::currentDateTime()
    upperBound = upperBound.addSecs(2);

    QString details = QString("\n"
        "lowerBound: %1\n"
        "dt1:        %2\n"
        "dt2:        %3\n"
        "dt3:        %4\n"
        "upperBound: %5\n")
        .arg(lowerBound.toSecsSinceEpoch())
        .arg(dt1.toSecsSinceEpoch())
        .arg(dt2.toSecsSinceEpoch())
        .arg(dt3.toSecsSinceEpoch())
        .arg(upperBound.toSecsSinceEpoch());

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

    // and finally, the SecsSinceEpoch should equal our number
    QCOMPARE(qint64(utc.toSecsSinceEpoch()), msec / 1000);
    QCOMPARE(qint64(local.toSecsSinceEpoch()), msec / 1000);
    QCOMPARE(utc.toMSecsSinceEpoch(), msec);
    QCOMPARE(local.toMSecsSinceEpoch(), msec);
}

void tst_QDateTime::toSecsSinceEpoch_data()
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

void tst_QDateTime::toSecsSinceEpoch()
{
    QFETCH( QString, dateTimeStr );
    QDateTime datetime = dt( dateTimeStr );

    qint64 asSecsSinceEpoch = datetime.toSecsSinceEpoch();
    uint asTime_t = datetime.toTime_t();
    QFETCH( bool, res );
    if (res) {
        QVERIFY( asTime_t != (uint)-1 );
    } else {
        QVERIFY( asTime_t == (uint)-1 );
    }
    QCOMPARE(asSecsSinceEpoch, datetime.toMSecsSinceEpoch() / 1000);

    if ( asTime_t != (uint) -1 ) {
        QDateTime datetime2 = QDateTime::fromTime_t( asTime_t );
        QCOMPARE(datetime, datetime2);
    }
    QDateTime datetime2 = QDateTime::fromSecsSinceEpoch(asSecsSinceEpoch);
    QCOMPARE(datetime, datetime2);
}

void tst_QDateTime::daylightSavingsTimeChange_data()
{
    QTest::addColumn<QDate>("inDST");
    QTest::addColumn<QDate>("outDST");
    QTest::addColumn<int>("days"); // from in to out; -ve if reversed
    QTest::addColumn<int>("months");

    QTest::newRow("Autumn") << QDate(2006, 8, 1) << QDate(2006, 12, 1)
                            << 122 << 4;

    QTest::newRow("Spring") << QDate(2006, 5, 1) << QDate(2006, 2, 1)
                            << -89 << -3;
}

void tst_QDateTime::daylightSavingsTimeChange()
{
    // This has grown from a regression test for an old bug where starting with
    // a date in DST and then moving to a date outside it (or vice-versa) caused
    // 1-hour jumps in time when addSecs() was called.
    //
    // The bug was caused by QDateTime knowing more than it lets show.
    // Internally, if it knows, QDateTime stores a flag indicating if the time is
    // DST or not. If it doesn't, it sets to "LocalUnknown".  The problem happened
    // because some functions did not reset the flag when moving in or out of DST.

    // WARNING: This only tests anything if there's a Daylight Savings Time change
    // in the current locale between inDST and outDST.
    // This is true for Central European Time and may be elsewhere.

    QFETCH(QDate, inDST);
    QFETCH(QDate, outDST);
    QFETCH(int, days);
    QFETCH(int, months);

    // First with simple construction
    QDateTime dt = QDateTime(outDST, QTime(0, 0, 0), Qt::LocalTime);
    int outDSTsecs = dt.toSecsSinceEpoch();

    dt.setDate(inDST);
    dt = dt.addSecs(1);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 0, 1)));

    // now using addDays:
    dt = dt.addDays(days).addSecs(1);
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 0, 2)));

    // ... and back again:
    dt = dt.addDays(-days).addSecs(1);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 0, 3)));

    // now using addMonths:
    dt = dt.addMonths(months).addSecs(1);
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 0, 4)));

    // ... and back again:
    dt = dt.addMonths(-months).addSecs(1);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 0, 5)));

    // now using fromSecsSinceEpoch
    dt = QDateTime::fromSecsSinceEpoch(outDSTsecs);
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 0, 0)));

    dt.setDate(inDST);
    dt = dt.addSecs(60);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 1, 0)));

    // using addMonths:
    dt = dt.addMonths(months).addSecs(60);
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 2, 0)));
    // back again:
    dt = dt.addMonths(-months).addSecs(60);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 3, 0)));

    // using addDays:
    dt = dt.addDays(days).addSecs(60);
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 4, 0)));
    // back again:
    dt = dt.addDays(-days).addSecs(60);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 5, 0)));

    // Now use the result of a UTC -> LocalTime conversion
    dt = QDateTime(outDST, QTime(0, 0, 0), Qt::LocalTime).toUTC();
    dt = QDateTime(dt.date(), dt.time(), Qt::UTC).toLocalTime();
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 0, 0)));

    // using addDays:
    dt = dt.addDays(-days).addSecs(3600);
    QCOMPARE(dt, QDateTime(inDST, QTime(1, 0, 0)));
    // back again
    dt = dt.addDays(days).addSecs(3600);
    QCOMPARE(dt, QDateTime(outDST, QTime(2, 0, 0)));

    // using addMonths:
    dt = dt.addMonths(-months).addSecs(3600);
    QCOMPARE(dt, QDateTime(inDST, QTime(3, 0, 0)));
    // back again:
    dt = dt.addMonths(months).addSecs(3600);
    QCOMPARE(dt, QDateTime(outDST, QTime(4, 0, 0)));

    // using setDate:
    dt.setDate(inDST);
    dt = dt.addSecs(3600);
    QCOMPARE(dt, QDateTime(inDST, QTime(5, 0, 0)));
}

void tst_QDateTime::springForward_data()
{
    QTest::addColumn<QDate>("day"); // day of DST transition
    QTest::addColumn<QTime>("time"); // in the "missing hour"
    QTest::addColumn<int>("step"); // days to step; +ve from before, -ve from after
    QTest::addColumn<int>("adjust"); // minutes ahead of UTC on day stepped from

    /*
      Zone tests compare a summer and winter moment's SecsSinceEpoch to known values.
      This could in principle be flawed (two DST-using zones in the same
      hemisphere with the same DST and standard times but different transition
      times) but no actual example is known where this is a problem.  Please
      document any such conflicts, if discovered.

      See http://www.timeanddate.com/time/zones/ for data on more candidates to
      test.
     */

    uint winter = QDateTime(QDate(2015, 1, 1), QTime()).toSecsSinceEpoch();
    uint summer = QDateTime(QDate(2015, 7, 1), QTime()).toSecsSinceEpoch();

    if (winter == 1420066800 && summer == 1435701600) {
        QTest::newRow("CET from day before") << QDate(2015, 3, 29) << QTime(2, 30, 0) << 1 << 60;
        QTest::newRow("CET from day after") << QDate(2015, 3, 29) << QTime(2, 30, 0) << -1 << 120;
    } else if (winter == 1420063200 && summer == 1435698000) {
        // e.g. Finland, where our CI runs ...
        QTest::newRow("EET from day before") << QDate(2015, 3, 29) << QTime(3, 30, 0) << 1 << 120;
        QTest::newRow("EET from day after") << QDate(2015, 3, 29) << QTime(3, 30, 0) << -1 << 180;
    } else if (winter == 1420070400 && summer == 1435705200) {
        // Western European Time, WET/WEST; a.k.a. GMT/BST
        QTest::newRow("WET from day before") << QDate(2015, 3, 29) << QTime(1, 30, 0) << 1 << 0;
        QTest::newRow("WET from day after") << QDate(2015, 3, 29) << QTime(1, 30, 0) << -1 << 60;
    } else if (winter == 1420099200 && summer == 1435734000) {
        // Western USA, Canada: Pacific Time (e.g. US/Pacific)
        QTest::newRow("PT from day before") << QDate(2015, 3, 8) << QTime(2, 30, 0) << 1 << -480;
        QTest::newRow("PT from day after") << QDate(2015, 3, 8) << QTime(2, 30, 0) << -1 << -420;
    } else if (winter == 1420088400 && summer == 1435723200) {
        // Eastern USA, Canada: Eastern Time (e.g. US/Eastern)
        QTest::newRow("ET from day before") << QDate(2015, 3, 8) << QTime(2, 30, 0) << 1 << -300;
        QTest::newRow("ET from day after") << QDate(2015, 3, 8) << QTime(2, 30, 0) << -1 << -240;
    } else {
        // Includes the numbers you need to test for your zone, as above:
        QString msg(QString::fromLatin1("No spring forward test data for this TZ (%1, %2)"
                        ).arg(winter).arg(summer));
        QSKIP(qPrintable(msg));
    }
}

void tst_QDateTime::springForward()
{
    QFETCH(QDate, day);
    QFETCH(QTime, time);
    QFETCH(int, step);
    QFETCH(int, adjust);

    QDateTime direct = QDateTime(day.addDays(-step), time, Qt::LocalTime).addDays(step);
    QCOMPARE(direct.date(), day);
    QCOMPARE(direct.time().minute(), time.minute());
    QCOMPARE(direct.time().second(), time.second());
    int off = direct.time().hour() - time.hour();
    QVERIFY(off == 1 || off == -1);
    // Note: function doc claims always +1, but this should be reviewed !

    // Repeat, but getting there via .toLocalTime():
    QDateTime detour = QDateTime(day.addDays(-step),
                                 time.addSecs(-60 * adjust),
                                 Qt::UTC).toLocalTime();
    QCOMPARE(detour.time(), time);
    detour = detour.addDays(step);
    QCOMPARE(detour, direct); // Insist on consistency.
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
    dateTime3c.setOffsetFromUtc(3600);
    QDateTime dateTime3d(dateTime3.addSecs(-3600));
    dateTime3d.setOffsetFromUtc(-3600);
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
    QTest::newRow("data13") << dateTime3 << dateTime3e
                            << (localTimeType == LocalTimeIsUtc) << false;
    QTest::newRow("invalid == invalid") << invalidDateTime() << invalidDateTime() << true << false;
    QTest::newRow("invalid == valid #1") << invalidDateTime() << dateTime1 << false << false;

    if (zoneIsCET) {
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

    if (checkEuro && zoneIsCET) {
        QVERIFY(dt1.toUTC() == dt2);
        QVERIFY(dt1 == dt2.toLocalTime());
    }
}

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
        const QByteArray vN = QByteArray::number(dataStreamVersion);
        const QByteArray pY = positiveYear.toString().toLatin1();
        QTest::newRow(('v' + vN + " WA => HAWAII " + pY).constData())
            << positiveYear << westernAustralia << hawaii << dataStreamVersion;
        QTest::newRow(('v' + vN + " WA => WA " + pY).constData())
            << positiveYear << westernAustralia << westernAustralia << dataStreamVersion;
        QTest::newRow(('v' + vN + " HAWAII => WA " + negativeYear.toString().toLatin1()).constData())
            << negativeYear << hawaii << westernAustralia << dataStreamVersion;
        QTest::newRow(('v' + vN + " HAWAII => HAWAII " + pY).constData())
            << positiveYear << hawaii << hawaii << dataStreamVersion;
    }
}

void tst_QDateTime::operator_insert_extract()
{
    QFETCH(QDateTime, dateTime);
    QFETCH(QString, serialiseAs);
    QFETCH(QString, deserialiseAs);
    QFETCH(QDataStream::Version, dataStreamVersion);

    // Save the previous timezone so we can restore it afterwards, otherwise later tests will break
    QByteArray previousTimeZone = qgetenv("TZ");

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

    if (previousTimeZone.isNull())
        qunsetenv("TZ");
    else
        qputenv("TZ", previousTimeZone.constData());
    tzset();
}

void tst_QDateTime::toString_strformat()
{
    // Most tests are in QLocale, just test that the api works.
    QDate testDate(2013, 1, 1);
    QTime testTime(1, 2, 3);
    QDateTime testDateTime(testDate, testTime, Qt::UTC);
    QCOMPARE(testDate.toString("yyyy-MM-dd"), QString("2013-01-01"));
    QCOMPARE(testTime.toString("hh:mm:ss"), QString("01:02:03"));
    QCOMPARE(testDateTime.toString("yyyy-MM-dd hh:mm:ss t"), QString("2013-01-01 01:02:03 UTC"));
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
        << Qt::TextDate << invalidDateTime();
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
    QTest::newRow("text second fraction") << QString::fromLatin1("Mon 6. May 2013 01:02:03.456")
        << Qt::TextDate << QDateTime(QDate(2013, 5, 6), QTime(1, 2, 3, 456));

    // Test Qt::ISODate format.
    QTest::newRow("ISO +01:00") << QString::fromLatin1("1987-02-13T13:24:51+01:00")
        << Qt::ISODate << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("ISO +00:01") << QString::fromLatin1("1987-02-13T13:24:51+00:01")
        << Qt::ISODate << QDateTime(QDate(1987, 2, 13), QTime(13, 23, 51), Qt::UTC);
    QTest::newRow("ISO -01:00") << QString::fromLatin1("1987-02-13T13:24:51-01:00")
        << Qt::ISODate << QDateTime(QDate(1987, 2, 13), QTime(14, 24, 51), Qt::UTC);
    QTest::newRow("ISO -00:01") << QString::fromLatin1("1987-02-13T13:24:51-00:01")
        << Qt::ISODate << QDateTime(QDate(1987, 2, 13), QTime(13, 25, 51), Qt::UTC);
    QTest::newRow("ISO +0000") << QString::fromLatin1("1970-01-01T00:12:34+0000")
        << Qt::ISODate << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("ISO +00:00") << QString::fromLatin1("1970-01-01T00:12:34+00:00")
        << Qt::ISODate << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("ISO -03") << QString::fromLatin1("2014-12-15T12:37:09-03")
        << Qt::ISODate << QDateTime(QDate(2014, 12, 15), QTime(15, 37, 9), Qt::UTC);
    QTest::newRow("ISO zzz-03") << QString::fromLatin1("2014-12-15T12:37:09.745-03")
        << Qt::ISODate << QDateTime(QDate(2014, 12, 15), QTime(15, 37, 9, 745), Qt::UTC);
    QTest::newRow("ISO -3") << QString::fromLatin1("2014-12-15T12:37:09-3")
        << Qt::ISODate << QDateTime(QDate(2014, 12, 15), QTime(15, 37, 9), Qt::UTC);
    QTest::newRow("ISO zzz-3") << QString::fromLatin1("2014-12-15T12:37:09.745-3")
        << Qt::ISODate << QDateTime(QDate(2014, 12, 15), QTime(15, 37, 9, 745), Qt::UTC);
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

    // Test Qt::RFC2822Date format (RFC 2822).
    QTest::newRow("RFC 2822 +0100") << QString::fromLatin1("13 Feb 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("RFC 2822 with day +0100") << QString::fromLatin1("Fri, 13 Feb 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("RFC 2822 -0100") << QString::fromLatin1("13 Feb 1987 13:24:51 -0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(14, 24, 51), Qt::UTC);
    QTest::newRow("RFC 2822 with day -0100") << QString::fromLatin1("Fri, 13 Feb 1987 13:24:51 -0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(14, 24, 51), Qt::UTC);
    QTest::newRow("RFC 2822 +0000") << QString::fromLatin1("01 Jan 1970 00:12:34 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("RFC 2822 with day +0000") << QString::fromLatin1("Thu, 01 Jan 1970 00:12:34 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("RFC 2822 +0000") << QString::fromLatin1("01 Jan 1970 00:12:34 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("RFC 2822 with day +0000") << QString::fromLatin1("Thu, 01 Jan 1970 00:12:34 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    // No timezone assume UTC
    QTest::newRow("RFC 2822 no timezone") << QString::fromLatin1("01 Jan 1970 00:12:34")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    // No time specified
    QTest::newRow("RFC 2822 date only") << QString::fromLatin1("01 Nov 2002")
        << Qt::RFC2822Date << invalidDateTime();
    QTest::newRow("RFC 2822 with day date only") << QString::fromLatin1("Fri, 01 Nov 2002")
        << Qt::RFC2822Date << invalidDateTime();
    // Test invalid month, day, year
    QTest::newRow("RFC 2822 invalid month name") << QString::fromLatin1("13 Fev 1987 13:24:51 +0100")
        << Qt::RFC2822Date << invalidDateTime();
    QTest::newRow("RFC 2822 invalid day") << QString::fromLatin1("36 Fev 1987 13:24:51 +0100")
        << Qt::RFC2822Date << invalidDateTime();
    QTest::newRow("RFC 2822 invalid year") << QString::fromLatin1("13 Fev 0000 13:24:51 +0100")
        << Qt::RFC2822Date << invalidDateTime();
    // Test invalid characters (should ignore invalid characters at end of string).
    QTest::newRow("RFC 2822 invalid character at end") << QString::fromLatin1("01 Jan 2012 08:00:00 +0100!")
        << Qt::RFC2822Date << QDateTime(QDate(2012, 1, 1), QTime(7, 0, 0, 0), Qt::UTC);
    QTest::newRow("RFC 2822 invalid character at front") << QString::fromLatin1("!01 Jan 2012 08:00:00 +0000")
        << Qt::RFC2822Date << invalidDateTime();
    QTest::newRow("RFC 2822 invalid character both ends") << QString::fromLatin1("!01 Jan 2012 08:00:00 +0000!")
        << Qt::RFC2822Date << invalidDateTime();
    QTest::newRow("RFC 2822 invalid character at front, 2 at back") << QString::fromLatin1("!01 Jan 2012 08:00:00 +0000..")
        << Qt::RFC2822Date << invalidDateTime();
    QTest::newRow("RFC 2822 invalid character 2 at front") << QString::fromLatin1("!!01 Jan 2012 08:00:00 +0000")
        << Qt::RFC2822Date << invalidDateTime();

    // Test Qt::RFC2822Date format (RFC 850 and 1036).
    QTest::newRow("RFC 850 and 1036 +0100") << QString::fromLatin1("Fri Feb 13 13:24:51 1987 +0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("RFC 850 and 1036 -0100") << QString::fromLatin1("Fri Feb 13 13:24:51 1987 -0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(14, 24, 51), Qt::UTC);
    QTest::newRow("RFC 850 and 1036 +0000") << QString::fromLatin1("Thu Jan 01 00:12:34 1970 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("RFC 850 and 1036 +0000") << QString::fromLatin1("Thu Jan 01 00:12:34 1970 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    // No timezone assume UTC
    QTest::newRow("RFC 850 and 1036 no timezone") << QString::fromLatin1("Thu Jan 01 00:12:34 1970")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    // No time specified
    QTest::newRow("RFC 850 and 1036 date only") << QString::fromLatin1("Fri Nov 01 2002")
        << Qt::RFC2822Date << invalidDateTime();
    // Test invalid characters (should ignore invalid characters at end of string).
    QTest::newRow("RFC 850 and 1036 invalid character at end") << QString::fromLatin1("Sun Jan 01 08:00:00 2012 +0100!")
        << Qt::RFC2822Date << QDateTime(QDate(2012, 1, 1), QTime(7, 0, 0, 0), Qt::UTC);
    QTest::newRow("RFC 850 and 1036 invalid character at front") << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0000")
        << Qt::RFC2822Date << invalidDateTime();
    QTest::newRow("RFC 850 and 1036 invalid character both ends") << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0000!")
        << Qt::RFC2822Date << invalidDateTime();
    QTest::newRow("RFC 850 and 1036 invalid character at front, 2 at back") << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0000..")
        << Qt::RFC2822Date << invalidDateTime();
    QTest::newRow("RFC 850 and 1036 invalid character 2 at front") << QString::fromLatin1("!!Sun Jan 01 08:00:00 2012 +0000")
        << Qt::RFC2822Date << invalidDateTime();

    QTest::newRow("RFC empty") << QString::fromLatin1("") << Qt::RFC2822Date << invalidDateTime();
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
    // Day non-conflict should not hide earlier year conflict (1963-03-01 was a
    // Friday; asking for Thursday moves this, without conflict, to the 7th):
    QTest::newRow("data8") << QString("77 03 1963 " + thu) << QString("yy MM yyyy ddd") << invalidDateTime();
    QTest::newRow("data9") << QString("101010") << QString("dMyy") << QDateTime(QDate(1910, 10, 10), QTime());
    QTest::newRow("data10") << QString("101010") << QString("dMyy") << QDateTime(QDate(1910, 10, 10), QTime());
    QTest::newRow("data11") << date << QString("dd MMM yy") << QDateTime(QDate(1910, 10, 10), QTime());
    date = fri + QLatin1Char(' ') + december + " 3 2004";
    QTest::newRow("data12") << date << QString("ddd MMMM d yyyy") << QDateTime(QDate(2004, 12, 3), QTime());
    QTest::newRow("data13") << QString("30.02.2004") << QString("dd.MM.yyyy") << invalidDateTime();
    QTest::newRow("data14") << QString("32.01.2004") << QString("dd.MM.yyyy") << invalidDateTime();
    date = thu + QLatin1Char(' ') + january + " 2004";
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

void tst_QDateTime::offsetFromUtc()
{
    /* Check default value. */
    QCOMPARE(QDateTime().offsetFromUtc(), 0);

    // Offset constructor
    QDateTime dt1(QDate(2013, 1, 1), QTime(1, 0, 0), Qt::OffsetFromUTC, 60 * 60);
    QCOMPARE(dt1.offsetFromUtc(), 60 * 60);
    QVERIFY(dt1.timeZone().isValid());
    dt1 = QDateTime(QDate(2013, 1, 1), QTime(1, 0, 0), Qt::OffsetFromUTC, -60 * 60);
    QCOMPARE(dt1.offsetFromUtc(), -60 * 60);

    // UTC should be 0 offset
    QDateTime dt2(QDate(2013, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QCOMPARE(dt2.offsetFromUtc(), 0);

    // LocalTime should vary
    if (zoneIsCET) {
        // Time definitely in Standard Time so 1 hour ahead
        QDateTime dt3(QDate(2013, 1, 1), QTime(0, 0, 0), Qt::LocalTime);
        QCOMPARE(dt3.offsetFromUtc(), 1 * 60 * 60);
        // Time definitely in Daylight Time so 2 hours ahead
        QDateTime dt4(QDate(2013, 6, 1), QTime(0, 0, 0), Qt::LocalTime);
        QCOMPARE(dt4.offsetFromUtc(), 2 * 60 * 60);
     } else {
         QSKIP("You must test using Central European (CET/CEST) time zone, e.g. TZ=Europe/Oslo");
     }

    QDateTime dt5(QDate(2013, 1, 1), QTime(0, 0, 0), QTimeZone("Pacific/Auckland"));
    QCOMPARE(dt5.offsetFromUtc(), 46800);

    QDateTime dt6(QDate(2013, 6, 1), QTime(0, 0, 0), QTimeZone("Pacific/Auckland"));
    QCOMPARE(dt6.offsetFromUtc(), 43200);
}

void tst_QDateTime::setOffsetFromUtc()
{
    /* Basic tests. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        dt.setTimeSpec(Qt::LocalTime);

        dt.setOffsetFromUtc(0);
        QCOMPARE(dt.offsetFromUtc(), 0);
        QCOMPARE(dt.timeSpec(), Qt::UTC);

        dt.setOffsetFromUtc(-100);
        QCOMPARE(dt.offsetFromUtc(), -100);
        QCOMPARE(dt.timeSpec(), Qt::OffsetFromUTC);
    }

    /* Test detaching. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        QDateTime dt2(dt);
        int offset2 = dt2.offsetFromUtc();

        dt.setOffsetFromUtc(501);

        QCOMPARE(dt.offsetFromUtc(), 501);
        QCOMPARE(dt2.offsetFromUtc(), offset2);
    }

    /* Check copying. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        dt.setOffsetFromUtc(502);
        QCOMPARE(dt.offsetFromUtc(), 502);

        QDateTime dt2(dt);
        QCOMPARE(dt2.offsetFromUtc(), 502);
    }

    /* Check assignment. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        dt.setOffsetFromUtc(502);
        QDateTime dt2;
        dt2 = dt;

        QCOMPARE(dt2.offsetFromUtc(), 502);
    }

    // Check spec persists
    QDateTime dt1(QDate(2013, 1, 1), QTime(0, 0, 0), Qt::OffsetFromUTC, 60 * 60);
    dt1.setMSecsSinceEpoch(123456789);
    QCOMPARE(dt1.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt1.offsetFromUtc(), 60 * 60);
    dt1.setSecsSinceEpoch(123456789);
    QCOMPARE(dt1.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt1.offsetFromUtc(), 60 * 60);

    // Check datastream serialises the offset seconds
    QByteArray tmp;
    {
        QDataStream ds(&tmp, QIODevice::WriteOnly);
        ds << dt1;
    }
    QDateTime dt2;
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> dt2;
    }
    QCOMPARE(dt2, dt1);
    QCOMPARE(dt2.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt2.offsetFromUtc(), 60 * 60);
}

void tst_QDateTime::toOffsetFromUtc()
{
    QDateTime dt1(QDate(2013, 1, 1), QTime(0, 0, 0), Qt::UTC);

    QDateTime dt2 = dt1.toOffsetFromUtc(60 * 60);
    QCOMPARE(dt2, dt1);
    QCOMPARE(dt2.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt2.date(), QDate(2013, 1, 1));
    QCOMPARE(dt2.time(), QTime(1, 0, 0));

    dt2 = dt1.toOffsetFromUtc(0);
    QCOMPARE(dt2, dt1);
    QCOMPARE(dt2.timeSpec(), Qt::UTC);
    QCOMPARE(dt2.date(), QDate(2013, 1, 1));
    QCOMPARE(dt2.time(), QTime(0, 0, 0));

    dt2 = dt1.toTimeSpec(Qt::OffsetFromUTC);
    QCOMPARE(dt2, dt1);
    QCOMPARE(dt2.timeSpec(), Qt::UTC);
    QCOMPARE(dt2.date(), QDate(2013, 1, 1));
    QCOMPARE(dt2.time(), QTime(0, 0, 0));
}

void tst_QDateTime::timeZoneAbbreviation()
{
    QDateTime dt1(QDate(2013, 1, 1), QTime(1, 0, 0), Qt::OffsetFromUTC, 60 * 60);
    QCOMPARE(dt1.timeZoneAbbreviation(), QString("UTC+01:00"));
    QDateTime dt2(QDate(2013, 1, 1), QTime(1, 0, 0), Qt::OffsetFromUTC, -60 * 60);
    QCOMPARE(dt2.timeZoneAbbreviation(), QString("UTC-01:00"));

    QDateTime dt3(QDate(2013, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QCOMPARE(dt3.timeZoneAbbreviation(), QString("UTC"));

    // LocalTime should vary
    if (zoneIsCET) {
        // Time definitely in Standard Time
        QDateTime dt4(QDate(2013, 1, 1), QTime(0, 0, 0), Qt::LocalTime);
#ifdef Q_OS_WIN
        QEXPECT_FAIL("", "Windows only returns long name (QTBUG-32759)", Continue);
#endif // Q_OS_WIN
        QCOMPARE(dt4.timeZoneAbbreviation(), QString("CET"));
        // Time definitely in Daylight Time
        QDateTime dt5(QDate(2013, 6, 1), QTime(0, 0, 0), Qt::LocalTime);
#ifdef Q_OS_WIN
        QEXPECT_FAIL("", "Windows only returns long name (QTBUG-32759)", Continue);
#endif // Q_OS_WIN
        QCOMPARE(dt5.timeZoneAbbreviation(), QString("CEST"));
    } else {
        QSKIP("You must test using Central European (CET/CEST) time zone, e.g. TZ=Europe/Oslo");
    }

    QDateTime dt5(QDate(2013, 1, 1), QTime(0, 0, 0), QTimeZone("Europe/Berlin"));
#ifdef Q_OS_WIN
    QEXPECT_FAIL("", "QTimeZone windows backend only returns long name", Continue);
#endif
    QCOMPARE(dt5.timeZoneAbbreviation(), QString("CET"));
    QDateTime dt6(QDate(2013, 6, 1), QTime(0, 0, 0), QTimeZone("Europe/Berlin"));
#ifdef Q_OS_WIN
    QEXPECT_FAIL("", "QTimeZone windows backend only returns long name", Continue);
#endif
    QCOMPARE(dt6.timeZoneAbbreviation(), QString("CEST"));
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

    dt1.setOffsetFromUtc(-(2 * 60 * 60)); // Minus two hours.
    dt2.setOffsetFromUtc(-(3 * 60 * 60)); // Minus three hours.

    QVERIFY(dt1 != dt2);
    QVERIFY(!(dt1 == dt2));
    QVERIFY(dt1 < dt2);
    QVERIFY(!(dt2 < dt1));
}

void tst_QDateTime::isDaylightTime() const
{
    QDateTime utc1(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QVERIFY(!utc1.isDaylightTime());
    QDateTime utc2(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC);
    QVERIFY(!utc2.isDaylightTime());

    QDateTime offset1(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::OffsetFromUTC, 1 * 60 * 60);
    QVERIFY(!offset1.isDaylightTime());
    QDateTime offset2(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::OffsetFromUTC, 1 * 60 * 60);
    QVERIFY(!offset2.isDaylightTime());

    if (zoneIsCET) {
        QDateTime cet1(QDate(2012, 1, 1), QTime(0, 0, 0));
        QVERIFY(!cet1.isDaylightTime());
        QDateTime cet2(QDate(2012, 6, 1), QTime(0, 0, 0));
        QVERIFY(cet2.isDaylightTime());
    } else {
        QSKIP("You must test using Central European (CET/CEST) time zone, e.g. TZ=Europe/Oslo");
    }
}

void tst_QDateTime::daylightTransitions() const
{
    if (zoneIsCET) {
        // CET transitions occur at 01:00:00 UTC on last Sunday in March and October
        // 2011-03-27 02:00:00 CET  became 03:00:00 CEST at msecs = 1301187600000
        // 2011-10-30 03:00:00 CEST became 02:00:00 CET  at msecs = 1319936400000
        // 2012-03-25 02:00:00 CET  became 03:00:00 CEST at msecs = 1332637200000
        // 2012-10-28 03:00:00 CEST became 02:00:00 CET  at msecs = 1351386000000
        const qint64 daylight2012 = 1332637200000;
        const qint64 standard2012 = 1351386000000;
        const qint64 msecsOneHour = 3600000;

        // Test for correct behviour for StandardTime -> DaylightTime transition, i.e. missing hour

        // Test setting date, time in missing hour will be invalid

        QDateTime before(QDate(2012, 3, 25), QTime(1, 59, 59, 999));
        QVERIFY(before.isValid());
        QCOMPARE(before.date(), QDate(2012, 3, 25));
        QCOMPARE(before.time(), QTime(1, 59, 59, 999));
        QCOMPARE(before.toMSecsSinceEpoch(), daylight2012 - 1);

        QDateTime missing(QDate(2012, 3, 25), QTime(2, 0, 0));
        QVERIFY(!missing.isValid());
        QCOMPARE(missing.date(), QDate(2012, 3, 25));
        QCOMPARE(missing.time(), QTime(2, 0, 0));

        QDateTime after(QDate(2012, 3, 25), QTime(3, 0, 0));
        QVERIFY(after.isValid());
        QCOMPARE(after.date(), QDate(2012, 3, 25));
        QCOMPARE(after.time(), QTime(3, 0, 0));
        QCOMPARE(after.toMSecsSinceEpoch(), daylight2012);

        // Test round-tripping of msecs

        before.setMSecsSinceEpoch(daylight2012 - 1);
        QVERIFY(before.isValid());
        QCOMPARE(before.date(), QDate(2012, 3, 25));
        QCOMPARE(before.time(), QTime(1, 59, 59, 999));
        QCOMPARE(before.toMSecsSinceEpoch(), daylight2012 -1);

        after.setMSecsSinceEpoch(daylight2012);
        QVERIFY(after.isValid());
        QCOMPARE(after.date(), QDate(2012, 3, 25));
        QCOMPARE(after.time(), QTime(3, 0, 0));
        QCOMPARE(after.toMSecsSinceEpoch(), daylight2012);

        // Test changing time spec re-validates the date/time

        QDateTime utc(QDate(2012, 3, 25), QTime(2, 00, 0), Qt::UTC);
        QVERIFY(utc.isValid());
        QCOMPARE(utc.date(), QDate(2012, 3, 25));
        QCOMPARE(utc.time(), QTime(2, 0, 0));
        utc.setTimeSpec(Qt::LocalTime);
        QVERIFY(!utc.isValid());
        QCOMPARE(utc.date(), QDate(2012, 3, 25));
        QCOMPARE(utc.time(), QTime(2, 0, 0));
        utc.setTimeSpec(Qt::UTC);
        QVERIFY(utc.isValid());
        QCOMPARE(utc.date(), QDate(2012, 3, 25));
        QCOMPARE(utc.time(), QTime(2, 0, 0));

        // Test date maths, if result falls in missing hour then becomes next hour

        QDateTime test(QDate(2011, 3, 25), QTime(2, 0, 0));
        QVERIFY(test.isValid());
        test = test.addYears(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 3, 25));
        QCOMPARE(test.time(), QTime(3, 0, 0));

        test = QDateTime(QDate(2012, 2, 25), QTime(2, 0, 0));
        QVERIFY(test.isValid());
        test = test.addMonths(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 3, 25));
        QCOMPARE(test.time(), QTime(3, 0, 0));

        test = QDateTime(QDate(2012, 3, 24), QTime(2, 0, 0));
        QVERIFY(test.isValid());
        test = test.addDays(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 3, 25));
        QCOMPARE(test.time(), QTime(3, 0, 0));

        test = QDateTime(QDate(2012, 3, 25), QTime(1, 0, 0));
        QVERIFY(test.isValid());
        QCOMPARE(test.toMSecsSinceEpoch(), daylight2012 - msecsOneHour);
        test = test.addMSecs(msecsOneHour);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 3, 25));
        QCOMPARE(test.time(), QTime(3, 0, 0));
        QCOMPARE(test.toMSecsSinceEpoch(), daylight2012);


        // Test for correct behviour for DaylightTime -> StandardTime transition, i.e. second occurrence

        // Test setting date and time in first and second occurrence will be valid

        // 1 hour before transition is 2:00:00 FirstOccurrence
        QDateTime hourBefore(QDate(2012, 10, 28), QTime(2, 0, 0));
        QVERIFY(hourBefore.isValid());
        QCOMPARE(hourBefore.date(), QDate(2012, 10, 28));
        QCOMPARE(hourBefore.time(), QTime(2, 0, 0));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(hourBefore.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // 1 msec before transition is 2:59:59.999 FirstOccurrence
        QDateTime msecBefore(QDate(2012, 10, 28), QTime(2, 59, 59, 999));
        QVERIFY(msecBefore.isValid());
        QCOMPARE(msecBefore.date(), QDate(2012, 10, 28));
        QCOMPARE(msecBefore.time(), QTime(2, 59, 59, 999));
#if defined(Q_OS_MAC) || defined(Q_OS_WIN) || defined(Q_OS_QNX)
        // Win and Mac uses SecondOccurrence here
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_MAC
        QCOMPARE(msecBefore.toMSecsSinceEpoch(), standard2012 - 1);

        // At transition is 2:00:00 SecondOccurrence
        QDateTime atTran(QDate(2012, 10, 28), QTime(2, 0, 0));
        QVERIFY(atTran.isValid());
        QCOMPARE(atTran.date(), QDate(2012, 10, 28));
        QCOMPARE(atTran.time(), QTime(2, 0, 0));
#ifndef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(atTran.toMSecsSinceEpoch(), standard2012);

        // 59:59.999 after transition is 2:59:59.999 SecondOccurrence
        QDateTime afterTran(QDate(2012, 10, 28), QTime(2, 59, 59, 999));
        QVERIFY(afterTran.isValid());
        QCOMPARE(afterTran.date(), QDate(2012, 10, 28));
        QCOMPARE(afterTran.time(), QTime(2, 59, 59, 999));
#if defined (Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_QNX)
        // Linux mktime bug uses last calculation
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_UNIX
        QCOMPARE(afterTran.toMSecsSinceEpoch(), standard2012 + msecsOneHour - 1);

        // 1 hour after transition is 3:00:00 FirstOccurrence
        QDateTime hourAfter(QDate(2012, 10, 28), QTime(3, 0, 0));
        QVERIFY(hourAfter.isValid());
        QCOMPARE(hourAfter.date(), QDate(2012, 10, 28));
        QCOMPARE(hourAfter.time(), QTime(3, 0, 0));
        QCOMPARE(hourAfter.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

        // Test round-tripping of msecs

        // 1 hour before transition is 2:00:00 FirstOccurrence
        hourBefore.setMSecsSinceEpoch(standard2012 - msecsOneHour);
        QVERIFY(hourBefore.isValid());
        QCOMPARE(hourBefore.date(), QDate(2012, 10, 28));
        QCOMPARE(hourBefore.time(), QTime(2, 0, 0));
        QCOMPARE(hourBefore.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // 1 msec before transition is 2:59:59.999 FirstOccurrence
        msecBefore.setMSecsSinceEpoch(standard2012 - 1);
        QVERIFY(msecBefore.isValid());
        QCOMPARE(msecBefore.date(), QDate(2012, 10, 28));
        QCOMPARE(msecBefore.time(), QTime(2, 59, 59, 999));
        QCOMPARE(msecBefore.toMSecsSinceEpoch(), standard2012 - 1);

        // At transition is 2:00:00 SecondOccurrence
        atTran.setMSecsSinceEpoch(standard2012);
        QVERIFY(atTran.isValid());
        QCOMPARE(atTran.date(), QDate(2012, 10, 28));
        QCOMPARE(atTran.time(), QTime(2, 0, 0));
        QCOMPARE(atTran.toMSecsSinceEpoch(), standard2012);

        // 59:59.999 after transition is 2:59:59.999 SecondOccurrence
        afterTran.setMSecsSinceEpoch(standard2012 + msecsOneHour - 1);
        QVERIFY(afterTran.isValid());
        QCOMPARE(afterTran.date(), QDate(2012, 10, 28));
        QCOMPARE(afterTran.time(), QTime(2, 59, 59, 999));
        QCOMPARE(afterTran.toMSecsSinceEpoch(), standard2012 + msecsOneHour - 1);

        // 1 hour after transition is 3:00:00 FirstOccurrence
        hourAfter.setMSecsSinceEpoch(standard2012 + msecsOneHour);
        QVERIFY(hourAfter.isValid());
        QCOMPARE(hourAfter.date(), QDate(2012, 10, 28));
        QCOMPARE(hourAfter.time(), QTime(3, 0, 0));
        QCOMPARE(hourAfter.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

        // Test date maths, result is always FirstOccurrence

        // Add year to get to tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 28), QTime(2, 0, 0));
        test = test.addYears(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(2, 0, 0));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // Add year to get to after tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 28), QTime(3, 0, 0));
        test = test.addYears(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(3, 0, 0));
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

        // Add year to tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0, 0));
        test = test.addYears(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 30));
        QCOMPARE(test.time(), QTime(2, 0, 0));

        // Add year to tran SecondOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0, 0)); // TODO SecondOccurrence
        test = test.addYears(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 30));
        QCOMPARE(test.time(), QTime(2, 0, 0));

        // Add year to after tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(3, 0, 0));
        test = test.addYears(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 30));
        QCOMPARE(test.time(), QTime(3, 0, 0));


        // Add month to get to tran FirstOccurrence
        test = QDateTime(QDate(2012, 9, 28), QTime(2, 0, 0));
        test = test.addMonths(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(2, 0, 0));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // Add month to get to after tran FirstOccurrence
        test = QDateTime(QDate(2012, 9, 28), QTime(3, 0, 0));
        test = test.addMonths(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(3, 0, 0));
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

        // Add month to tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0, 0));
        test = test.addMonths(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 11, 30));
        QCOMPARE(test.time(), QTime(2, 0, 0));

        // Add month to tran SecondOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0, 0)); // TODO SecondOccurrence
        test = test.addMonths(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 11, 30));
        QCOMPARE(test.time(), QTime(2, 0, 0));

        // Add month to after tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(3, 0, 0));
        test = test.addMonths(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 11, 30));
        QCOMPARE(test.time(), QTime(3, 0, 0));


        // Add day to get to tran FirstOccurrence
        test = QDateTime(QDate(2012, 10, 27), QTime(2, 0, 0));
        test = test.addDays(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(2, 0, 0));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // Add day to get to after tran FirstOccurrence
        test = QDateTime(QDate(2012, 10, 27), QTime(3, 0, 0));
        test = test.addDays(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(3, 0, 0));
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

        // Add day to tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0, 0));
        test = test.addDays(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 10, 31));
        QCOMPARE(test.time(), QTime(2, 0, 0));

        // Add day to tran SecondOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0, 0)); // TODO SecondOccurrence
        test = test.addDays(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 10, 31));
        QCOMPARE(test.time(), QTime(2, 0, 0));

        // Add day to after tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(3, 0, 0));
        test = test.addDays(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 10, 31));
        QCOMPARE(test.time(), QTime(3, 0, 0));


        // Add hour to get to tran FirstOccurrence
        test = QDateTime(QDate(2012, 10, 28), QTime(1, 0, 0));
        test = test.addMSecs(msecsOneHour);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(2, 0, 0));
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // Add hour to tran FirstOccurrence to get to tran SecondOccurrence
        test = QDateTime(QDate(2012, 10, 28), QTime(2, 0, 0));
        test = test.addMSecs(msecsOneHour);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.time(), QTime(2, 0, 0));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012);

        // Add hour to tran SecondOccurrence to get to after tran FirstOccurrence
        test = QDateTime(QDate(2012, 10, 28), QTime(2, 0, 0)); // TODO SecondOccurrence
        test = test.addMSecs(msecsOneHour);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
#if defined(Q_OS_MAC) || defined(Q_OS_QNX)
        // Mac uses FirstOccurrence, Windows uses SecondOccurrence, Linux uses last calculation
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.time(), QTime(3, 0, 0));
#if defined(Q_OS_MAC) || defined(Q_OS_QNX)
        // Mac uses FirstOccurrence, Windows uses SecondOccurrence, Linux uses last calculation
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

    } else {
        QSKIP("You must test using Central European (CET/CEST) time zone, e.g. TZ=Europe/Oslo");
    }
}

void tst_QDateTime::timeZones() const
{
    QTimeZone invalidTz = QTimeZone("Vulcan/ShiKahr");
    QCOMPARE(invalidTz.isValid(), false);
    QDateTime invalidDateTime = QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0), invalidTz);
    QCOMPARE(invalidDateTime.isValid(), false);
    QCOMPARE(invalidDateTime.date(), QDate(2000, 1, 1));
    QCOMPARE(invalidDateTime.time(), QTime(0, 0, 0));

    QTimeZone nzTz = QTimeZone("Pacific/Auckland");
    QTimeZone nzTzOffset = QTimeZone(12 * 3600);

    // During Standard Time NZ is +12:00
    QDateTime utcStd(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime nzStd(QDate(2012, 6, 1), QTime(12, 0, 0), nzTz);
    QDateTime nzStdOffset(QDate(2012, 6, 1), QTime(12, 0, 0), nzTzOffset);

    QCOMPARE(nzStd.isValid(), true);
    QCOMPARE(nzStd.timeSpec(), Qt::TimeZone);
    QCOMPARE(nzStd.date(), QDate(2012, 6, 1));
    QCOMPARE(nzStd.time(), QTime(12, 0, 0));
    QVERIFY(nzStd.timeZone() == nzTz);
    QCOMPARE(nzStd.timeZone().id(), QByteArray("Pacific/Auckland"));
    QCOMPARE(nzStd.offsetFromUtc(), 43200);
    QCOMPARE(nzStd.isDaylightTime(), false);
    QCOMPARE(nzStd.toMSecsSinceEpoch(), utcStd.toMSecsSinceEpoch());

    QCOMPARE(nzStdOffset.isValid(), true);
    QCOMPARE(nzStdOffset.timeSpec(), Qt::TimeZone);
    QCOMPARE(nzStdOffset.date(), QDate(2012, 6, 1));
    QCOMPARE(nzStdOffset.time(), QTime(12, 0, 0));
    QVERIFY(nzStdOffset.timeZone() == nzTzOffset);
    QCOMPARE(nzStdOffset.timeZone().id(), QByteArray("UTC+12:00"));
    QCOMPARE(nzStdOffset.offsetFromUtc(), 43200);
    QCOMPARE(nzStdOffset.isDaylightTime(), false);
    QCOMPARE(nzStdOffset.toMSecsSinceEpoch(), utcStd.toMSecsSinceEpoch());

    // During Daylight Time NZ is +13:00
    QDateTime utcDst(QDate(2012, 1, 1), QTime(0, 0, 0), Qt::UTC);
    QDateTime nzDst(QDate(2012, 1, 1), QTime(13, 0, 0), nzTz);

    QCOMPARE(nzDst.isValid(), true);
    QCOMPARE(nzDst.date(), QDate(2012, 1, 1));
    QCOMPARE(nzDst.time(), QTime(13, 0, 0));
    QCOMPARE(nzDst.offsetFromUtc(), 46800);
    QCOMPARE(nzDst.isDaylightTime(), true);
    QCOMPARE(nzDst.toMSecsSinceEpoch(), utcDst.toMSecsSinceEpoch());

    QDateTime utc = nzStd.toUTC();
    QCOMPARE(utc.date(), utcStd.date());
    QCOMPARE(utc.time(), utcStd.time());

    utc = nzDst.toUTC();
    QCOMPARE(utc.date(), utcDst.date());
    QCOMPARE(utc.time(), utcDst.time());

    // Sydney is 2 hours behind New Zealand
    QTimeZone ausTz = QTimeZone("Australia/Sydney");
    QDateTime aus = nzStd.toTimeZone(ausTz);
    QCOMPARE(aus.date(), QDate(2012, 6, 1));
    QCOMPARE(aus.time(), QTime(10, 0, 0));

    QDateTime dt1(QDate(2012, 6, 1), QTime(0, 0, 0), Qt::UTC);
    QCOMPARE(dt1.timeSpec(), Qt::UTC);
    dt1.setTimeZone(nzTz);
    QCOMPARE(dt1.timeSpec(), Qt::TimeZone);
    QCOMPARE(dt1.date(), QDate(2012, 6, 1));
    QCOMPARE(dt1.time(), QTime(0, 0, 0));
    QCOMPARE(dt1.timeZone(), nzTz);

    QDateTime dt2 = QDateTime::fromSecsSinceEpoch(1338465600, nzTz);
    QCOMPARE(dt2.date(), dt1.date());
    QCOMPARE(dt2.time(), dt1.time());
    QCOMPARE(dt2.timeSpec(), dt1.timeSpec());
    QCOMPARE(dt2.timeZone(), dt1.timeZone());

    QDateTime dt3 = QDateTime::fromMSecsSinceEpoch(1338465600000, nzTz);
    QCOMPARE(dt3.date(), dt1.date());
    QCOMPARE(dt3.time(), dt1.time());
    QCOMPARE(dt3.timeSpec(), dt1.timeSpec());
    QCOMPARE(dt3.timeZone(), dt1.timeZone());

    // Check datastream serialises the time zone
    QByteArray tmp;
    {
        QDataStream ds(&tmp, QIODevice::WriteOnly);
        ds << dt1;
    }
    QDateTime dt4;
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> dt4;
    }
    QCOMPARE(dt4, dt1);
    QCOMPARE(dt4.timeSpec(), Qt::TimeZone);
    QCOMPARE(dt4.timeZone(), nzTz);

    // Check handling of transition times
    QTimeZone cet("Europe/Oslo");

    // Standard Time to Daylight Time 2013 on 2013-03-31 is 2:00 local time / 1:00 UTC
    qint64 stdToDstMSecs = 1364691600000;

    // Test MSecs to local
    // - Test 1 msec before tran = 01:59:59.999
    QDateTime beforeDst = QDateTime::fromMSecsSinceEpoch(stdToDstMSecs - 1, cet);
    QCOMPARE(beforeDst.date(), QDate(2013, 3, 31));
    QCOMPARE(beforeDst.time(), QTime(1, 59, 59, 999));
    // - Test at tran = 03:00:00
    QDateTime atDst = QDateTime::fromMSecsSinceEpoch(stdToDstMSecs, cet);
    QCOMPARE(atDst.date(), QDate(2013, 3, 31));
    QCOMPARE(atDst.time(), QTime(3, 0, 0));

    // Test local to MSecs
    // - Test 1 msec before tran = 01:59:59.999
    beforeDst = QDateTime(QDate(2013, 3, 31), QTime(1, 59, 59, 999), cet);
    QCOMPARE(beforeDst.toMSecsSinceEpoch(), stdToDstMSecs - 1);
    // - Test at tran = 03:00:00
    atDst = QDateTime(QDate(2013, 3, 31), QTime(3, 0, 0), cet);
    QCOMPARE(atDst.toMSecsSinceEpoch(), stdToDstMSecs);
    // - Test transition hole, setting 03:00:00 is valid
    atDst = QDateTime(QDate(2013, 3, 31), QTime(3, 0, 0), cet);
    QVERIFY(atDst.isValid());
    QCOMPARE(atDst.date(), QDate(2013, 3, 31));
    QCOMPARE(atDst.time(), QTime(3, 0, 0));
    QCOMPARE(atDst.toMSecsSinceEpoch(), stdToDstMSecs);
    // - Test transition hole, setting 02:00:00 is invalid
    atDst = QDateTime(QDate(2013, 3, 31), QTime(2, 0, 0), cet);
    QVERIFY(!atDst.isValid());
    QCOMPARE(atDst.date(), QDate(2013, 3, 31));
    QCOMPARE(atDst.time(), QTime(2, 0, 0));
    // - Test transition hole, setting 02:59:59.999 is invalid
    atDst = QDateTime(QDate(2013, 3, 31), QTime(2, 59, 59, 999), cet);
    QVERIFY(!atDst.isValid());
    QCOMPARE(atDst.date(), QDate(2013, 3, 31));
    QCOMPARE(atDst.time(), QTime(2, 59, 59, 999));

    // Standard Time to Daylight Time 2013 on 2013-10-27 is 3:00 local time / 1:00 UTC
    qint64 dstToStdMSecs = 1382835600000;

    // Test MSecs to local
    // - Test 1 hour before tran = 02:00:00 local first occurrence
    QDateTime hourBeforeStd = QDateTime::fromMSecsSinceEpoch(dstToStdMSecs - 3600000, cet);
    QCOMPARE(hourBeforeStd.date(), QDate(2013, 10, 27));
    QCOMPARE(hourBeforeStd.time(), QTime(2, 0, 0));
    // - Test 1 msec before tran = 02:59:59.999 local first occurrence
    QDateTime msecBeforeStd = QDateTime::fromMSecsSinceEpoch(dstToStdMSecs - 1, cet);
    QCOMPARE(msecBeforeStd.date(), QDate(2013, 10, 27));
    QCOMPARE(msecBeforeStd.time(), QTime(2, 59, 59, 999));
    // - Test at tran = 03:00:00 local becomes 02:00:00 local second occurrence
    QDateTime atStd = QDateTime::fromMSecsSinceEpoch(dstToStdMSecs, cet);
    QCOMPARE(atStd.date(), QDate(2013, 10, 27));
    QCOMPARE(atStd.time(), QTime(2, 0, 0));
    // - Test 59 mins after tran = 02:59:59.999 local second occurrence
    QDateTime afterStd = QDateTime::fromMSecsSinceEpoch(dstToStdMSecs + 3600000 -1, cet);
    QCOMPARE(afterStd.date(), QDate(2013, 10, 27));
    QCOMPARE(afterStd.time(), QTime(2, 59, 59, 999));
    // - Test 1 hour after tran = 03:00:00 local
    QDateTime hourAfterStd = QDateTime::fromMSecsSinceEpoch(dstToStdMSecs + 3600000, cet);
    QCOMPARE(hourAfterStd.date(), QDate(2013, 10, 27));
    QCOMPARE(hourAfterStd.time(), QTime(3, 00, 00));

    // Test local to MSecs
    // - Test first occurrence 02:00:00 = 1 hour before tran
    hourBeforeStd = QDateTime(QDate(2013, 10, 27), QTime(2, 0, 0), cet);
    QCOMPARE(hourBeforeStd.toMSecsSinceEpoch(), dstToStdMSecs - 3600000);
    // - Test first occurrence 02:59:59.999 = 1 msec before tran
    msecBeforeStd = QDateTime(QDate(2013, 10, 27), QTime(2, 59, 59, 999), cet);
    QCOMPARE(msecBeforeStd.toMSecsSinceEpoch(), dstToStdMSecs - 1);
    // - Test second occurrence 02:00:00 = at tran
    atStd = QDateTime(QDate(2013, 10, 27), QTime(2, 0, 0), cet);
    QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
    QCOMPARE(atStd.toMSecsSinceEpoch(), dstToStdMSecs);
    // - Test second occurrence 03:00:00 = 59 mins after tran
    afterStd = QDateTime(QDate(2013, 10, 27), QTime(2, 59, 59, 999), cet);
    QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
    QCOMPARE(afterStd.toMSecsSinceEpoch(), dstToStdMSecs + 3600000 - 1);
    // - Test 03:00:00 = 1 hour after tran
    hourAfterStd = QDateTime(QDate(2013, 10, 27), QTime(3, 0, 0), cet);
    QCOMPARE(hourAfterStd.toMSecsSinceEpoch(), dstToStdMSecs + 3600000);

    // Test Time Zone that has transitions but no future transitions afer a given date
    QTimeZone sgt("Asia/Singapore");
    QDateTime future(QDate(2015, 1, 1), QTime(0, 0, 0), sgt);
    QVERIFY(future.isValid());
    QCOMPARE(future.offsetFromUtc(), 28800);
}

#if defined(Q_OS_UNIX)
// Currently disabled on Windows as adjusting the timezone
// requires additional privileges that aren't normally
// enabled for a process. This can be achieved by calling
// AdjustTokenPrivileges() and then SetTimeZoneInformation(),
// which will require linking to a different library to access that API.
static void setTimeZone(const QByteArray &tz)
{
    qputenv("TZ", tz);
    ::tzset();

// following left for future reference, see comment above
// #if defined(Q_OS_WIN32)
//     ::_tzset();
// #endif
}

void tst_QDateTime::systemTimeZoneChange() const
{
    struct ResetTZ {
        QByteArray original;
        ResetTZ() : original(qgetenv("TZ")) {}
        ~ResetTZ() { setTimeZone(original); }
    } scopedReset;

    // Set the timezone to Brisbane time
    setTimeZone(QByteArray("AEST-10:00"));

    QDateTime localDate = QDateTime(QDate(2012, 6, 1), QTime(2, 15, 30), Qt::LocalTime);
    QDateTime utcDate = QDateTime(QDate(2012, 6, 1), QTime(2, 15, 30), Qt::UTC);
    QDateTime tzDate = QDateTime(QDate(2012, 6, 1), QTime(2, 15, 30), QTimeZone("Australia/Brisbane"));
    qint64 localMsecs = localDate.toMSecsSinceEpoch();
    qint64 utcMsecs = utcDate.toMSecsSinceEpoch();
    qint64 tzMsecs = tzDate.toMSecsSinceEpoch();

    // check that Australia/Brisbane is known
    QVERIFY(tzDate.timeZone().isValid());

    // Change to Indian time
    setTimeZone(QByteArray("IST-05:30"));

    QCOMPARE(localDate, QDateTime(QDate(2012, 6, 1), QTime(2, 15, 30), Qt::LocalTime));
    QVERIFY(localMsecs != localDate.toMSecsSinceEpoch());
    QCOMPARE(utcDate, QDateTime(QDate(2012, 6, 1), QTime(2, 15, 30), Qt::UTC));
    QCOMPARE(utcDate.toMSecsSinceEpoch(), utcMsecs);
    QCOMPARE(tzDate, QDateTime(QDate(2012, 6, 1), QTime(2, 15, 30), QTimeZone("Australia/Brisbane")));
    QCOMPARE(tzDate.toMSecsSinceEpoch(), tzMsecs);
}
#endif

void tst_QDateTime::invalid() const
{
    QDateTime invalidDate = QDateTime(QDate(0, 0, 0), QTime(-1, -1, -1));
    QCOMPARE(invalidDate.isValid(), false);
    QCOMPARE(invalidDate.timeSpec(), Qt::LocalTime);

    QDateTime utcDate = invalidDate.toUTC();
    QCOMPARE(utcDate.isValid(), false);
    QCOMPARE(utcDate.timeSpec(), Qt::UTC);

    QDateTime offsetDate = invalidDate.toOffsetFromUtc(3600);
    QCOMPARE(offsetDate.isValid(), false);
    QCOMPARE(offsetDate.timeSpec(), Qt::OffsetFromUTC);

    QDateTime tzDate = invalidDate.toTimeZone(QTimeZone("Europe/Oslo"));
    QCOMPARE(tzDate.isValid(), false);
    QCOMPARE(tzDate.timeSpec(), Qt::TimeZone);
}

void tst_QDateTime::macTypes()
{
#ifndef Q_OS_MAC
    QSKIP("This is a Apple-only test");
#else
    extern void tst_QDateTime_macTypes(); // in qdatetime_mac.mm
    tst_QDateTime_macTypes();
#endif
}

QTEST_APPLESS_MAIN(tst_QDateTime)
#include "tst_qdatetime.moc"
