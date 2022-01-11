/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2021 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformdefs.h"
#include "qdatetime.h"

#include "qcalendar.h"
#include "qdatastream.h"
#include "qdebug.h"
#include "qset.h"
#include "qlocale.h"

#include "private/qdatetime_p.h"
#if QT_CONFIG(datetimeparser)
#include "private/qdatetimeparser_p.h"
#endif
#ifdef Q_OS_DARWIN
#include "private/qcore_mac_p.h"
#endif
#include "private/qgregoriancalendar_p.h"
#include "private/qnumeric_p.h"
#include "private/qstringiterator_p.h"
#if QT_CONFIG(timezone)
#include "private/qtimezoneprivate_p.h"
#endif

#include <cmath>
#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif
#include <time.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  Date/Time Constants
 *****************************************************************************/

enum : qint64 {
    SECS_PER_DAY = 86400,
    MSECS_PER_DAY = 86400000,
    SECS_PER_HOUR = 3600,
    MSECS_PER_HOUR = 3600000,
    SECS_PER_MIN = 60,
    MSECS_PER_MIN = 60000,
    MSECS_PER_SEC = 1000,
    TIME_T_MAX = std::numeric_limits<time_t>::max(),
    JULIAN_DAY_FOR_EPOCH = 2440588 // result of julianDayFromDate(1970, 1, 1)
};

/*****************************************************************************
  QDate static helper functions
 *****************************************************************************/

static inline QDate fixedDate(QCalendar::YearMonthDay &&parts, QCalendar cal)
{
    if ((parts.year < 0 && !cal.isProleptic()) || (parts.year == 0 && !cal.hasYearZero()))
        return QDate();

    parts.day = qMin(parts.day, cal.daysInMonth(parts.month, parts.year));
    return cal.dateFromParts(parts);
}

static inline QDate fixedDate(QCalendar::YearMonthDay &&parts)
{
    if (parts.year) {
        parts.day = qMin(parts.day, QGregorianCalendar::monthLength(parts.month, parts.year));
        qint64 jd;
        if (QGregorianCalendar::julianFromParts(parts.year, parts.month, parts.day, &jd))
            return QDate::fromJulianDay(jd);
    }
    return QDate();
}

/*****************************************************************************
  Date/Time formatting helper functions
 *****************************************************************************/

#if QT_CONFIG(textdate)
static const char qt_shortMonthNames[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int fromShortMonthName(QStringView monthName)
{
    for (unsigned int i = 0; i < sizeof(qt_shortMonthNames) / sizeof(qt_shortMonthNames[0]); ++i) {
        if (monthName == QLatin1String(qt_shortMonthNames[i], 3))
            return i + 1;
    }
    return -1;
}
#endif // textdate

#if QT_CONFIG(datestring) // depends on, so implies, textdate
struct ParsedRfcDateTime {
    QDate date;
    QTime time;
    int utcOffset;
};

static int shortDayFromName(QStringView name)
{
    const char16_t shortDayNames[] = u"MonTueWedThuFriSatSun";
    for (int i = 0; i < 7; i++) {
        if (name == QStringView(shortDayNames + 3 * i, 3))
            return i + 1;
    }
    return 0;
}

static ParsedRfcDateTime rfcDateImpl(QStringView s)
{
    // Matches "[ddd,] dd MMM yyyy[ hh:mm[:ss]] [±hhmm]" - correct RFC 822, 2822, 5322 format -
    // or           "ddd MMM dd[ hh:mm:ss] yyyy [±hhmm]" - permissive RFC 850, 1036 (read only)
    ParsedRfcDateTime result;

    auto words = QStringView{s}.split(u' ', Qt::SkipEmptyParts);
    if (words.size() < 3 || words.size() > 6)
        return result;
    const QChar colon(u':');
    bool ok = true;
    QDate date;

    const auto isShortName = [](QStringView name) {
        return (name.length() == 3 && name[0].isUpper()
                && name[1].isLower() && name[2].isLower());
    };

    /* Reject entirely (return) if the string is malformed; however, if the date
     * is merely invalid, (break, so as to) go on to parsing of the time.
     */
    int yearIndex;
    do { // "loop" so that we can use break on merely invalid, but "right shape" date.
        QStringView dayName;
        bool rfcX22 = true;
        if (words.at(0).endsWith(u',')) {
            dayName = words.takeFirst().chopped(1);
        } else if (!words.at(0)[0].isDigit()) {
            dayName = words.takeFirst();
            rfcX22 = false;
        } // else: dayName is not specified (so we can only be RFC *22)
        if (words.size() < 3 || words.size() > 5)
            return result;

        // Don't break before setting yearIndex.
        int dayIndex, monthIndex;
        if (rfcX22) {
            // dd MMM yyyy [hh:mm[:ss]] [±hhmm]
            dayIndex = 0;
            monthIndex = 1;
            yearIndex = 2;
        } else {
            // MMM dd[ hh:mm:ss] yyyy [±hhmm]
            dayIndex = 1;
            monthIndex = 0;
            yearIndex = words.size() > 3 && words.at(2).contains(colon) ? 3 : 2;
        }

        int dayOfWeek = 0;
        if (!dayName.isEmpty()) {
            if (!isShortName(dayName))
                return result;
            dayOfWeek = shortDayFromName(dayName);
            if (!dayOfWeek)
                break;
        }

        const int day = words.at(dayIndex).toInt(&ok);
        if (!ok)
            return result;
        const int year = words.at(yearIndex).toInt(&ok);
        if (!ok)
            return result;
        const QStringView monthName = words.at(monthIndex);
        if (!isShortName(monthName))
            return result;
        int month = fromShortMonthName(monthName);
        if (month < 0)
            break;

        date = QDate(year, month, day);
        if (dayOfWeek && date.dayOfWeek() != dayOfWeek)
            date = QDate();
    } while (false);
    words.remove(yearIndex);
    words.remove(0, 2); // month and day-of-month, in some order

    // Time: [hh:mm[:ss]]
    QTime time;
    if (words.size() && words.at(0).contains(colon)) {
        const QStringView when = words.takeFirst();
        if (when.size() < 5 || when[2] != colon
            || (when.size() == 8 ? when[5] != colon : when.size() > 5)) {
            return result;
        }
        const int hour = when.first(2).toInt(&ok);
        if (!ok)
            return result;
        const int minute = when.sliced(3, 2).toInt(&ok);
        if (!ok)
            return result;
        const auto secs = when.size() == 8 ? when.last(2).toInt(&ok) : 0;
        if (!ok)
            return result;
        time = QTime(hour, minute, secs);
    }

    // Offset: [±hh[mm]]
    int offset = 0;
    if (words.size()) {
        const QStringView zone = words.takeFirst();
        if (words.size() || !(zone.size() == 3 || zone.size() == 5))
            return result;
        bool negate = false;
        if (zone[0] == u'-')
            negate = true;
        else if (zone[0] != u'+')
            return result;
        const int hour = zone.sliced(1, 2).toInt(&ok);
        if (!ok)
            return result;
        const auto minute = zone.size() == 5 ? zone.last(2).toInt(&ok) : 0;
        if (!ok)
            return result;
        offset = (hour * 60 + minute) * 60;
        if (negate)
            offset = -offset;
    }

    result.date = date;
    result.time = time;
    result.utcOffset = offset;
    return result;
}
#endif // datestring

// Return offset in [+-]HH:mm format
static QString toOffsetString(Qt::DateFormat format, int offset)
{
    return QString::asprintf("%c%02d%s%02d",
                             offset >= 0 ? '+' : '-',
                             qAbs(offset) / int(SECS_PER_HOUR),
                             // Qt::ISODate puts : between the hours and minutes, but Qt:TextDate does not:
                             format == Qt::TextDate ? "" : ":",
                             (qAbs(offset) / 60) % 60);
}

#if QT_CONFIG(datestring)
// Parse offset in [+-]HH[[:]mm] format
static int fromOffsetString(QStringView offsetString, bool *valid) noexcept
{
    *valid = false;

    const int size = offsetString.size();
    if (size < 2 || size > 6)
        return 0;

    // sign will be +1 for a positive and -1 for a negative offset
    int sign;

    // First char must be + or -
    const QChar signChar = offsetString[0];
    if (signChar == u'+')
        sign = 1;
    else if (signChar == u'-')
        sign = -1;
    else
        return 0;

    // Split the hour and minute parts
    const QStringView time = offsetString.sliced(1);
    qsizetype hhLen = time.indexOf(u':');
    qsizetype mmIndex;
    if (hhLen == -1)
        mmIndex = hhLen = 2; // [+-]HHmm or [+-]HH format
    else
        mmIndex = hhLen + 1;

    const QStringView hhRef = time.first(qMin(hhLen, time.size()));
    bool ok = false;
    const int hour = hhRef.toInt(&ok);
    if (!ok || hour > 23) // More generous than QTimeZone::MaxUtcOffsetSecs
        return 0;

    const QStringView mmRef = time.sliced(qMin(mmIndex, time.size()));
    const int minute = mmRef.isEmpty() ? 0 : mmRef.toInt(&ok);
    if (!ok || minute < 0 || minute > 59)
        return 0;

    *valid = true;
    return sign * ((hour * 60) + minute) * 60;
}
#endif // datestring

/*****************************************************************************
  QDate member functions
 *****************************************************************************/

/*!
    \class QDate
    \inmodule QtCore
    \reentrant
    \brief The QDate class provides date functions.

    A QDate object represents a particular day, regardless of calendar, locale
    or other settings used when creating it or supplied by the system.  It can
    report the year, month and day of the month that represent the day with
    respect to the proleptic Gregorian calendar or any calendar supplied as a
    QCalendar object. QDate objects should be passed by value rather than by
    reference to const; they simply package \c qint64.

    A QDate object is typically created by giving the year, month, and day
    numbers explicitly. Note that QDate interprets year numbers less than 100 as
    presented, i.e., as years 1 through 99, without adding any offset. The
    static function currentDate() creates a QDate object containing the date
    read from the system clock. An explicit date can also be set using
    setDate(). The fromString() function returns a QDate given a string and a
    date format which is used to interpret the date within the string.

    The year(), month(), and day() functions provide access to the year, month,
    and day numbers. When more than one of these values is needed, it is more
    efficient to call QCalendar::partsFromDate(), to save repeating (potentially
    expensive) calendrical calculations.

    Also, dayOfWeek() and dayOfYear() functions are provided. The same
    information is provided in textual format by toString(). QLocale can map the
    day numbers to names, QCalendar can map month numbers to names.

    QDate provides a full set of operators to compare two QDate
    objects where smaller means earlier, and larger means later.

    You can increment (or decrement) a date by a given number of days
    using addDays(). Similarly you can use addMonths() and addYears().
    The daysTo() function returns the number of days between two
    dates.

    The daysInMonth() and daysInYear() functions return how many days there are
    in this date's month and year, respectively. The isLeapYear() function
    indicates whether a date is in a leap year. QCalendar can also supply this
    information, in some cases more conveniently.

    \section1 Remarks

    \note All conversion to and from string formats is done using the C locale.
    For localized conversions, see QLocale.

    In the Gregorian calendar, there is no year 0. Dates in that year are
    considered invalid. The year -1 is the year "1 before Christ" or "1 before
    common era." The day before 1 January 1 CE, QDate(1, 1, 1), is 31 December
    1 BCE, QDate(-1, 12, 31). Various other calendars behave similarly; see
    QCalendar::hasYearZero().

    \section2 Range of Valid Dates

    Dates are stored internally as a Julian Day number, an integer count of
    every day in a contiguous range, with 24 November 4714 BCE in the Gregorian
    calendar being Julian Day 0 (1 January 4713 BCE in the Julian calendar).
    As well as being an efficient and accurate way of storing an absolute date,
    it is suitable for converting a date into other calendar systems such as
    Hebrew, Islamic or Chinese. The Julian Day number can be obtained using
    QDate::toJulianDay() and can be set using QDate::fromJulianDay().

    The range of Julian Day numbers that QDate can represent is, for technical
    reasons, limited to between -784350574879 and 784354017364, which means from
    before 2 billion BCE to after 2 billion CE. This is more than seven times as
    wide as the range of dates a QDateTime can represent.

    \sa QTime, QDateTime, QCalendar, QDateTime::YearRange, QDateEdit, QDateTimeEdit, QCalendarWidget
*/

/*!
    \fn QDate::QDate()

    Constructs a null date. Null dates are invalid.

    \sa isNull(), isValid()
*/

/*!
    Constructs a date with year \a y, month \a m and day \a d.

    The date is understood in terms of the Gregorian calendar. If the specified
    date is invalid, the date is not set and isValid() returns \c false.

    \warning Years 1 to 99 are interpreted as is. Year 0 is invalid.

    \sa isValid(), QCalendar::dateFromParts()
*/

QDate::QDate(int y, int m, int d)
{
    if (!QGregorianCalendar::julianFromParts(y, m, d, &jd))
        jd = nullJd();
}

QDate::QDate(int y, int m, int d, QCalendar cal)
{
    *this = cal.dateFromParts(y, m, d);
}

/*!
    \fn bool QDate::isNull() const

    Returns \c true if the date is null; otherwise returns \c false. A null
    date is invalid.

    \note The behavior of this function is equivalent to isValid().

    \sa isValid()
*/

/*!
    \fn bool QDate::isValid() const

    Returns \c true if this date is valid; otherwise returns \c false.

    \sa isNull(), QCalendar::isDateValid()
*/

/*!
    Returns the year of this date.

    Uses \a cal as calendar, if supplied, else the Gregorian calendar.

    Returns 0 if the date is invalid. For some calendars, dates before their
    first year may all be invalid.

    If using a calendar which has a year 0, check using isValid() if the return
    is 0. Such calendars use negative year numbers in the obvious way, with
    year 1 preceded by year 0, in turn preceded by year -1 and so on.

    Some calendars, despite having no year 0, have a conventional numbering of
    the years before their first year, counting backwards from 1. For example,
    in the proleptic Gregorian calendar, successive years before 1 CE (the first
    year) are identified as 1 BCE, 2 BCE, 3 BCE and so on. For such calendars,
    negative year numbers are used to indicate these years before year 1, with
    -1 indicating the year before 1.

    \sa month(), day(), QCalendar::hasYearZero(), QCalendar::isProleptic(), QCalendar::partsFromDate()
*/

int QDate::year(QCalendar cal) const
{
    if (isValid()) {
        const auto parts = cal.partsFromDate(*this);
        if (parts.isValid())
            return parts.year;
    }
    return 0;
}

/*!
  \overload
 */

int QDate::year() const
{
    if (isValid()) {
        const auto parts = QGregorianCalendar::partsFromJulian(jd);
        if (parts.isValid())
            return parts.year;
    }
    return 0;
}

/*!
    Returns the month-number for the date.

    Numbers the months of the year starting with 1 for the first. Uses \a cal
    as calendar if supplied, else the Gregorian calendar, for which the month
    numbering is as follows:

    \list
    \li 1 = "January"
    \li 2 = "February"
    \li 3 = "March"
    \li 4 = "April"
    \li 5 = "May"
    \li 6 = "June"
    \li 7 = "July"
    \li 8 = "August"
    \li 9 = "September"
    \li 10 = "October"
    \li 11 = "November"
    \li 12 = "December"
    \endlist

    Returns 0 if the date is invalid. Note that some calendars may have more
    than 12 months in some years.

    \sa year(), day(), QCalendar::partsFromDate()
*/

int QDate::month(QCalendar cal) const
{
    if (isValid()) {
        const auto parts = cal.partsFromDate(*this);
        if (parts.isValid())
            return parts.month;
    }
    return 0;
}

/*!
  \overload
 */

int QDate::month() const
{
    if (isValid()) {
        const auto parts = QGregorianCalendar::partsFromJulian(jd);
        if (parts.isValid())
            return parts.month;
    }
    return 0;
}

/*!
    Returns the day of the month for this date.

    Uses \a cal as calendar if supplied, else the Gregorian calendar (for which
    the return ranges from 1 to 31). Returns 0 if the date is invalid.

    \sa year(), month(), dayOfWeek(), QCalendar::partsFromDate()
*/

int QDate::day(QCalendar cal) const
{
    if (isValid()) {
        const auto parts = cal.partsFromDate(*this);
        if (parts.isValid())
            return parts.day;
    }
    return 0;
}

/*!
  \overload
 */

int QDate::day() const
{
    if (isValid()) {
        const auto parts = QGregorianCalendar::partsFromJulian(jd);
        if (parts.isValid())
            return parts.day;
    }
    return 0;
}

/*!
    Returns the weekday (1 = Monday to 7 = Sunday) for this date.

    Uses \a cal as calendar if supplied, else the Gregorian calendar. Returns 0
    if the date is invalid. Some calendars may give special meaning
    (e.g. intercallary days) to values greater than 7.

    \sa day(), dayOfYear(), QCalendar::dayOfWeek(), Qt::DayOfWeek
*/

int QDate::dayOfWeek(QCalendar cal) const
{
    if (isNull())
        return 0;

    return cal.dayOfWeek(*this);
}

/*!
  \overload
 */

int QDate::dayOfWeek() const
{
    return isValid() ? QGregorianCalendar::weekDayOfJulian(jd) : 0;
}

/*!
    Returns the day of the year (1 for the first day) for this date.

    Uses \a cal as calendar if supplied, else the Gregorian calendar.
    Returns 0 if either the date or the first day of its year is invalid.

    \sa day(), dayOfWeek(), QCalendar::daysInYear()
*/

int QDate::dayOfYear(QCalendar cal) const
{
    if (isValid()) {
        QDate firstDay = cal.dateFromParts(year(cal), 1, 1);
        if (firstDay.isValid())
            return firstDay.daysTo(*this) + 1;
    }
    return 0;
}

/*!
  \overload
 */

int QDate::dayOfYear() const
{
    if (isValid()) {
        qint64 first;
        if (QGregorianCalendar::julianFromParts(year(), 1, 1, &first))
            return jd - first + 1;
    }
    return 0;
}

/*!
    Returns the number of days in the month for this date.

    Uses \a cal as calendar if supplied, else the Gregorian calendar (for which
    the result ranges from 28 to 31). Returns 0 if the date is invalid.

    \sa day(), daysInYear(), QCalendar::daysInMonth(),
        QCalendar::maximumDaysInMonth(), QCalendar::minimumDaysInMonth()
*/

int QDate::daysInMonth(QCalendar cal) const
{
    if (isValid()) {
        const auto parts = cal.partsFromDate(*this);
        if (parts.isValid())
            return cal.daysInMonth(parts.month, parts.year);
    }
    return 0;
}

/*!
  \overload
 */

int QDate::daysInMonth() const
{
    if (isValid()) {
        const auto parts = QGregorianCalendar::partsFromJulian(jd);
        if (parts.isValid())
            return QGregorianCalendar::monthLength(parts.month, parts.year);
    }
    return 0;
}

/*!
    Returns the number of days in the year for this date.

    Uses \a cal as calendar if supplied, else the Gregorian calendar (for which
    the result is 365 or 366). Returns 0 if the date is invalid.

    \sa day(), daysInMonth(), QCalendar::daysInYear(), QCalendar::maximumMonthsInYear()
*/

int QDate::daysInYear(QCalendar cal) const
{
    if (isNull())
        return 0;

    return cal.daysInYear(year(cal));
}

/*!
  \overload
 */

int QDate::daysInYear() const
{
    return isValid() ? QGregorianCalendar::leapTest(year()) ? 366 : 365 : 0;
}

/*!
    Returns the ISO 8601 week number (1 to 53).

    Returns 0 if the date is invalid. Otherwise, returns the week number for the
    date. If \a yearNumber is not \nullptr (its default), stores the year as
    *\a{yearNumber}.

    In accordance with ISO 8601, each week falls in the year to which most of
    its days belong, in the Gregorian calendar. As ISO 8601's week starts on
    Monday, this is the year in which the week's Thursday falls. Most years have
    52 weeks, but some have 53.

    \note *\a{yearNumber} is not always the same as year(). For example, 1
    January 2000 has week number 52 in the year 1999, and 31 December
    2002 has week number 1 in the year 2003.

    \sa isValid()
*/

int QDate::weekNumber(int *yearNumber) const
{
    if (!isValid())
        return 0;

    // This could be replaced by use of QIso8601Calendar, once we implement it.
    // The Thursday of the same week determines our answer:
    const QDate thursday(addDays(4 - dayOfWeek()));
    if (yearNumber)
        *yearNumber = thursday.year();

    // Week n's Thurs's DOY has 1 <= DOY - 7*(n-1) < 8, so 0 <= DOY + 6 - 7*n < 7:
    return (thursday.dayOfYear() + 6) / 7;
}

static bool inDateTimeRange(qint64 jd, bool start)
{
    using Bounds = std::numeric_limits<qint64>;
    if (jd < Bounds::min() + JULIAN_DAY_FOR_EPOCH)
        return false;
    jd -= JULIAN_DAY_FOR_EPOCH;
    const qint64 maxDay = Bounds::max() / MSECS_PER_DAY;
    const qint64 minDay = Bounds::min() / MSECS_PER_DAY - 1;
    // (Divisions rounded towards zero, as MSECS_PER_DAY has factors other than two.)
    // Range includes start of last day and end of first:
    if (start)
        return jd > minDay && jd <= maxDay;
    return jd >= minDay && jd < maxDay;
}

static QDateTime toEarliest(QDate day, const QDateTime &form)
{
    const Qt::TimeSpec spec = form.timeSpec();
    const int offset = (spec == Qt::OffsetFromUTC) ? form.offsetFromUtc() : 0;
#if QT_CONFIG(timezone)
    QTimeZone zone;
    if (spec == Qt::TimeZone)
        zone = form.timeZone();
#endif
    auto moment = [=](QTime time) {
        switch (spec) {
        case Qt::OffsetFromUTC: return QDateTime(day, time, spec, offset);
#if QT_CONFIG(timezone)
        case Qt::TimeZone: return QDateTime(day, time, zone);
#endif
        default: return QDateTime(day, time, spec);
        }
    };
    // Longest routine time-zone transition is 2 hours:
    QDateTime when = moment(QTime(2, 0));
    if (!when.isValid()) {
        // Noon should be safe ...
        when = moment(QTime(12, 0));
        if (!when.isValid()) {
            // ... unless it's a 24-hour jump (moving the date-line)
            when = moment(QTime(23, 59, 59, 999));
            if (!when.isValid())
                return QDateTime();
        }
    }
    int high = when.time().msecsSinceStartOfDay() / 60000;
    int low = 0;
    // Binary chop to the right minute
    while (high > low + 1) {
        int mid = (high + low) / 2;
        QDateTime probe = moment(QTime(mid / 60, mid % 60));
        if (probe.isValid() && probe.date() == day) {
            high = mid;
            when = probe;
        } else {
            low = mid;
        }
    }
    return when;
}

/*!
    \since 5.14
    \fn QDateTime QDate::startOfDay(Qt::TimeSpec spec, int offsetSeconds) const
    \fn QDateTime QDate::startOfDay(const QTimeZone &zone) const

    Returns the start-moment of the day.  Usually, this shall be midnight at the
    start of the day: however, if a time-zone transition causes the given date
    to skip over that midnight (e.g. a DST spring-forward skipping from the end
    of the previous day to 01:00 of the new day), the actual earliest time in
    the day is returned.  This can only arise when the start-moment is specified
    in terms of a time-zone (by passing its QTimeZone as \a zone) or in terms of
    local time (by passing Qt::LocalTime as \a spec; this is its default).

    The \a offsetSeconds is ignored unless \a spec is Qt::OffsetFromUTC, when it
    gives the implied zone's offset from UTC.  As UTC and such zones have no
    transitions, the start of the day is QTime(0, 0) in these cases.

    In the rare case of a date that was entirely skipped (this happens when a
    zone east of the international date-line switches to being west of it), the
    return shall be invalid.  Passing Qt::TimeZone as \a spec (instead of
    passing a QTimeZone) or passing an invalid time-zone as \a zone will also
    produce an invalid result, as shall dates that start outside the range
    representable by QDateTime.

    \sa endOfDay()
*/
QDateTime QDate::startOfDay(Qt::TimeSpec spec, int offsetSeconds) const
{
    if (!inDateTimeRange(jd, true))
        return QDateTime();

    switch (spec) {
    case Qt::TimeZone: // should pass a QTimeZone instead of Qt::TimeZone
        qWarning() << "Called QDate::startOfDay(Qt::TimeZone) on" << *this;
        return QDateTime();
    case Qt::OffsetFromUTC:
    case Qt::UTC:
        return QDateTime(*this, QTime(0, 0), spec, offsetSeconds);

    case Qt::LocalTime:
        if (offsetSeconds)
            qWarning("Ignoring offset (%d seconds) passed with Qt::LocalTime", offsetSeconds);
        break;
    }
    QDateTime when(*this, QTime(0, 0), spec);
    if (!when.isValid())
        when = toEarliest(*this, when);

    return when.isValid() ? when : QDateTime();
}

#if QT_CONFIG(timezone)
/*!
  \overload
  \since 5.14
*/
QDateTime QDate::startOfDay(const QTimeZone &zone) const
{
    if (!inDateTimeRange(jd, true) || !zone.isValid())
        return QDateTime();

    QDateTime when(*this, QTime(0, 0), zone);
    if (when.isValid())
        return when;

    // The start of the day must have fallen in a spring-forward's gap; find the spring-forward:
    if (zone.hasTransitions()) {
        QTimeZone::OffsetData tran
            // There's unlikely to be another transition before noon tomorrow.
            // However, the whole of today may have been skipped !
            = zone.previousTransition(QDateTime(addDays(1), QTime(12, 0), zone));
        const QDateTime &at = tran.atUtc.toTimeZone(zone);
        if (at.isValid() && at.date() == *this)
            return at;
    }

    when = toEarliest(*this, when);
    return when.isValid() ? when : QDateTime();
}
#endif // timezone

static QDateTime toLatest(QDate day, const QDateTime &form)
{
    const Qt::TimeSpec spec = form.timeSpec();
    const int offset = (spec == Qt::OffsetFromUTC) ? form.offsetFromUtc() : 0;
#if QT_CONFIG(timezone)
    QTimeZone zone;
    if (spec == Qt::TimeZone)
        zone = form.timeZone();
#endif
    auto moment = [=](QTime time) {
        switch (spec) {
        case Qt::OffsetFromUTC: return QDateTime(day, time, spec, offset);
#if QT_CONFIG(timezone)
        case Qt::TimeZone: return QDateTime(day, time, zone);
#endif
        default: return QDateTime(day, time, spec);
        }
    };
    // Longest routine time-zone transition is 2 hours:
    QDateTime when = moment(QTime(21, 59, 59, 999));
    if (!when.isValid()) {
        // Noon should be safe ...
        when = moment(QTime(12, 0));
        if (!when.isValid()) {
            // ... unless it's a 24-hour jump (moving the date-line)
            when = moment(QTime(0, 0));
            if (!when.isValid())
                return QDateTime();
        }
    }
    int high = 24 * 60;
    int low = when.time().msecsSinceStartOfDay() / 60000;
    // Binary chop to the right minute
    while (high > low + 1) {
        int mid = (high + low) / 2;
        QDateTime probe = moment(QTime(mid / 60, mid % 60, 59, 999));
        if (probe.isValid() && probe.date() == day) {
            low = mid;
            when = probe;
        } else {
            high = mid;
        }
    }
    return when;
}

/*!
    \since 5.14
    \fn QDateTime QDate::endOfDay(Qt::TimeSpec spec, int offsetSeconds) const
    \fn QDateTime QDate::endOfDay(const QTimeZone &zone) const

    Returns the end-moment of the day.  Usually, this is one millisecond before
    the midnight at the end of the day: however, if a time-zone transition
    causes the given date to skip over that midnight (e.g. a DST spring-forward
    skipping from just before 23:00 to the start of the next day), the actual
    latest time in the day is returned.  This can only arise when the
    start-moment is specified in terms of a time-zone (by passing its QTimeZone
    as \a zone) or in terms of local time (by passing Qt::LocalTime as \a spec;
    this is its default).

    The \a offsetSeconds is ignored unless \a spec is Qt::OffsetFromUTC, when it
    gives the implied zone's offset from UTC.  As UTC and such zones have no
    transitions, the end of the day is QTime(23, 59, 59, 999) in these cases.

    In the rare case of a date that was entirely skipped (this happens when a
    zone east of the international date-line switches to being west of it), the
    return shall be invalid.  Passing Qt::TimeZone as \a spec (instead of
    passing a QTimeZone) will also produce an invalid result, as shall dates
    that end outside the range representable by QDateTime.

    \sa startOfDay()
*/
QDateTime QDate::endOfDay(Qt::TimeSpec spec, int offsetSeconds) const
{
    if (!inDateTimeRange(jd, false))
        return QDateTime();

    switch (spec) {
    case Qt::TimeZone: // should pass a QTimeZone instead of Qt::TimeZone
        qWarning() << "Called QDate::endOfDay(Qt::TimeZone) on" << *this;
        return QDateTime();
    case Qt::UTC:
    case Qt::OffsetFromUTC:
        return QDateTime(*this, QTime(23, 59, 59, 999), spec, offsetSeconds);

    case Qt::LocalTime:
        if (offsetSeconds)
            qWarning("Ignoring offset (%d seconds) passed with Qt::LocalTime", offsetSeconds);
        break;
    }
    QDateTime when(*this, QTime(23, 59, 59, 999), spec);
    if (!when.isValid())
        when = toLatest(*this, when);
    return when.isValid() ? when : QDateTime();
}

#if QT_CONFIG(timezone)
/*!
  \overload
  \since 5.14
*/
QDateTime QDate::endOfDay(const QTimeZone &zone) const
{
    if (!inDateTimeRange(jd, false) || !zone.isValid())
        return QDateTime();

    QDateTime when(*this, QTime(23, 59, 59, 999), zone);
    if (when.isValid())
        return when;

    // The end of the day must have fallen in a spring-forward's gap; find the spring-forward:
    if (zone.hasTransitions()) {
        QTimeZone::OffsetData tran
            // It's unlikely there's been another transition since yesterday noon.
            // However, the whole of today may have been skipped !
            = zone.nextTransition(QDateTime(addDays(-1), QTime(12, 0), zone));
        const QDateTime &at = tran.atUtc.toTimeZone(zone);
        if (at.isValid() && at.date() == *this)
            return at;
    }

    when = toLatest(*this, when);
    return when.isValid() ? when : QDateTime();
}
#endif // timezone

#if QT_CONFIG(datestring) // depends on, so implies, textdate

static QString toStringTextDate(QDate date)
{
    if (date.isValid()) {
        QCalendar cal; // Always Gregorian
        const auto parts = cal.partsFromDate(date);
        if (parts.isValid()) {
            const QLatin1Char sp(' ');
            return QLocale::c().dayName(cal.dayOfWeek(date), QLocale::ShortFormat) + sp
                + cal.monthName(QLocale::c(), parts.month, parts.year, QLocale::ShortFormat)
                // Documented to use 4-digit year
                + sp + QString::asprintf("%d %04d", parts.day, parts.year);
        }
    }
    return QString();
}

static QString toStringIsoDate(QDate date)
{
    const auto parts = QCalendar().partsFromDate(date);
    if (parts.isValid() && parts.year >= 0 && parts.year <= 9999)
        return QString::asprintf("%04d-%02d-%02d", parts.year, parts.month, parts.day);
    return QString();
}

/*!
    \overload

    Returns the date as a string. The \a format parameter determines the format
    of the string.

    If the \a format is Qt::TextDate, the string is formatted in the default
    way. The day and month names will be in English. An example of this
    formatting is "Sat May 20 1995". For localized formatting, see
    \l{QLocale::toString()}.

    If the \a format is Qt::ISODate, the string format corresponds
    to the ISO 8601 extended specification for representations of
    dates and times, taking the form yyyy-MM-dd, where yyyy is the
    year, MM is the month of the year (between 01 and 12), and dd is
    the day of the month between 01 and 31.

    If the \a format is Qt::RFC2822Date, the string is formatted in
    an \l{RFC 2822} compatible way. An example of this formatting is
    "20 May 1995".

    If the date is invalid, an empty string will be returned.

    \warning The Qt::ISODate format is only valid for years in the
    range 0 to 9999.

    \sa fromString(), QLocale::toString()
*/
QString QDate::toString(Qt::DateFormat format) const
{
    if (!isValid())
        return QString();

    switch (format) {
    case Qt::RFC2822Date:
        return QLocale::c().toString(*this, u"dd MMM yyyy");
    default:
    case Qt::TextDate:
        return toStringTextDate(*this);
    case Qt::ISODate:
    case Qt::ISODateWithMs:
        // No calendar dependence
        return toStringIsoDate(*this);
    }
}

/*!
    \fn QString QDate::toString(const QString &format, QCalendar cal) const
    \fn QString QDate::toString(QStringView format, QCalendar cal) const

    Returns the date as a string. The \a format parameter determines the format
    of the result string. If \a cal is supplied, it determines the calendar used
    to represent the date; it defaults to Gregorian.

    These expressions may be used:

    \table
    \header \li Expression \li Output
    \row \li d \li The day as a number without a leading zero (1 to 31)
    \row \li dd \li The day as a number with a leading zero (01 to 31)
    \row \li ddd \li The abbreviated day name ('Mon' to 'Sun').
    \row \li dddd \li The long day name ('Monday' to 'Sunday').
    \row \li M \li The month as a number without a leading zero (1 to 12)
    \row \li MM \li The month as a number with a leading zero (01 to 12)
    \row \li MMM \li The abbreviated month name ('Jan' to 'Dec').
    \row \li MMMM \li The long month name ('January' to 'December').
    \row \li yy \li The year as a two digit number (00 to 99)
    \row \li yyyy \li The year as a four digit number. If the year is negative,
            a minus sign is prepended, making five characters.
    \endtable

    Any sequence of characters enclosed in single quotes will be included
    verbatim in the output string (stripped of the quotes), even if it contains
    formatting characters. Two consecutive single quotes ("''") are replaced by
    a single quote in the output. All other characters in the format string are
    included verbatim in the output string.

    Formats without separators (e.g. "ddMM") are supported but must be used with
    care, as the resulting strings aren't always reliably readable (e.g. if "dM"
    produces "212" it could mean either the 2nd of December or the 21st of
    February).

    Example format strings (assuming that the QDate is the 20 July
    1969):

    \table
    \header \li Format            \li Result
    \row    \li dd.MM.yyyy        \li 20.07.1969
    \row    \li ddd MMMM d yy     \li Sun July 20 69
    \row    \li 'The day is' dddd \li The day is Sunday
    \endtable

    If the datetime is invalid, an empty string will be returned.

    \note Day and month names are given in English (C locale).
    If localized month and day names are desired, use
    QLocale::system().toString().

    \sa fromString(), QDateTime::toString(), QTime::toString(), QLocale::toString()

*/
QString QDate::toString(QStringView format, QCalendar cal) const
{
    return QLocale::c().toString(*this, format, cal);
}
#endif // datestring

/*!
    \since 4.2

    Sets this to represent the date, in the Gregorian calendar, with the given
    \a year, \a month and \a day numbers. Returns true if the resulting date is
    valid, otherwise it sets this to represent an invalid date and returns
    false.

    \sa isValid(), QCalendar::dateFromParts()
*/
bool QDate::setDate(int year, int month, int day)
{
    if (QGregorianCalendar::julianFromParts(year, month, day, &jd))
        return true;

    jd = nullJd();
    return false;
}

/*!
    \since 5.14

    Sets this to represent the date, in the given calendar \a cal, with the
    given \a year, \a month and \a day numbers. Returns true if the resulting
    date is valid, otherwise it sets this to represent an invalid date and
    returns false.

    \sa isValid(), QCalendar::dateFromParts()
*/

bool QDate::setDate(int year, int month, int day, QCalendar cal)
{
    *this = QDate(year, month, day, cal);
    return isValid();
}

/*!
    \since 4.5

    Extracts the date's year, month, and day, and assigns them to
    *\a year, *\a month, and *\a day. The pointers may be null.

    Returns 0 if the date is invalid.

    \note In Qt versions prior to 5.7, this function is marked as non-\c{const}.

    \sa year(), month(), day(), isValid(), QCalendar::partsFromDate()
*/
void QDate::getDate(int *year, int *month, int *day) const
{
    QCalendar::YearMonthDay parts; // invalid by default
    if (isValid())
        parts = QGregorianCalendar::partsFromJulian(jd);

    const bool ok = parts.isValid();
    if (year)
        *year = ok ? parts.year : 0;
    if (month)
        *month = ok ? parts.month : 0;
    if (day)
        *day = ok ? parts.day : 0;
}

/*!
    Returns a QDate object containing a date \a ndays later than the
    date of this object (or earlier if \a ndays is negative).

    Returns a null date if the current date is invalid or the new date is
    out of range.

    \sa addMonths(), addYears(), daysTo()
*/

QDate QDate::addDays(qint64 ndays) const
{
    if (isNull())
        return QDate();

    if (qint64 r; Q_UNLIKELY(qAddOverflow(jd, ndays, &r)))
        return QDate();
    else
        return fromJulianDay(r);
}

/*!
    Returns a QDate object containing a date \a nmonths later than the
    date of this object (or earlier if \a nmonths is negative).

    Uses \a cal as calendar, if supplied, else the Gregorian calendar.

    \note If the ending day/month combination does not exist in the resulting
    month/year, this function will return a date that is the latest valid date
    in the selected month.

    \sa addDays(), addYears()
*/

QDate QDate::addMonths(int nmonths, QCalendar cal) const
{
    if (!isValid())
        return QDate();

    if (nmonths == 0)
        return *this;

    auto parts = cal.partsFromDate(*this);

    if (!parts.isValid())
        return QDate();
    Q_ASSERT(parts.year || cal.hasYearZero());

    parts.month += nmonths;
    while (parts.month <= 0) {
        if (--parts.year || cal.hasYearZero())
            parts.month += cal.monthsInYear(parts.year);
    }
    int count = cal.monthsInYear(parts.year);
    while (parts.month > count) {
        parts.month -= count;
        count = (++parts.year || cal.hasYearZero()) ? cal.monthsInYear(parts.year) : 0;
    }

    return fixedDate(std::move(parts), cal);
}

/*!
  \overload
*/

QDate QDate::addMonths(int nmonths) const
{
    if (isNull())
        return QDate();

    if (nmonths == 0)
        return *this;

    auto parts = QGregorianCalendar::partsFromJulian(jd);

    if (!parts.isValid())
        return QDate();
    Q_ASSERT(parts.year);

    parts.month += nmonths;
    while (parts.month <= 0) {
        if (--parts.year) // skip over year 0
            parts.month += 12;
    }
    while (parts.month > 12) {
        parts.month -= 12;
        if (!++parts.year) // skip over year 0
            ++parts.year;
    }

    return fixedDate(std::move(parts));
}

/*!
    Returns a QDate object containing a date \a nyears later than the
    date of this object (or earlier if \a nyears is negative).

    Uses \a cal as calendar, if supplied, else the Gregorian calendar.

    \note If the ending day/month combination does not exist in the resulting
    year (e.g., for the Gregorian calendar, if the date was Feb 29 and the final
    year is not a leap year), this function will return a date that is the
    latest valid date in the given month (in the example, Feb 28).

    \sa addDays(), addMonths()
*/

QDate QDate::addYears(int nyears, QCalendar cal) const
{
    if (!isValid())
        return QDate();

    auto parts = cal.partsFromDate(*this);
    if (!parts.isValid())
        return QDate();

    int old_y = parts.year;
    parts.year += nyears;

    // If we just crossed (or hit) a missing year zero, adjust year by +/- 1:
    if (!cal.hasYearZero() && ((old_y > 0) != (parts.year > 0) || !parts.year))
        parts.year += nyears > 0 ? +1 : -1;

    return fixedDate(std::move(parts), cal);
}

/*!
    \overload
*/

QDate QDate::addYears(int nyears) const
{
    if (isNull())
        return QDate();

    auto parts = QGregorianCalendar::partsFromJulian(jd);
    if (!parts.isValid())
        return QDate();

    int old_y = parts.year;
    parts.year += nyears;

    // If we just crossed (or hit) a missing year zero, adjust year by +/- 1:
    if ((old_y > 0) != (parts.year > 0) || !parts.year)
        parts.year += nyears > 0 ? +1 : -1;

    return fixedDate(std::move(parts));
}

/*!
    Returns the number of days from this date to \a d (which is
    negative if \a d is earlier than this date).

    Returns 0 if either date is invalid.

    Example:
    \snippet code/src_corelib_time_qdatetime.cpp 0

    \sa addDays()
*/

qint64 QDate::daysTo(QDate d) const
{
    if (isNull() || d.isNull())
        return 0;

    // Due to limits on minJd() and maxJd() we know this will never overflow
    return d.jd - jd;
}


/*!
    \fn bool QDate::operator==(QDate lhs, QDate rhs)

    Returns \c true if \a lhs and \a rhs represent the same day, otherwise
    \c false.
*/

/*!
    \fn bool QDate::operator!=(QDate lhs, QDate rhs)

    Returns \c true if \a lhs and \a rhs represent distinct days; otherwise
    returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QDate::operator<(QDate lhs, QDate rhs)

    Returns \c true if \a lhs is earlier than \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QDate::operator<=(QDate lhs, QDate rhs)

    Returns \c true if \a lhs is earlier than or equal to \a rhs;
    otherwise returns \c false.
*/

/*!
    \fn bool QDate::operator>(QDate lhs, QDate rhs)

    Returns \c true if \a lhs is later than \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QDate::operator>=(QDate lhs, QDate rhs)

    Returns \c true if \a lhs is later than or equal to \a rhs;
    otherwise returns \c false.
*/

/*!
    \fn QDate::currentDate()
    Returns the current date, as reported by the system clock.

    \sa QTime::currentTime(), QDateTime::currentDateTime()
*/

#if QT_CONFIG(datestring) // depends on, so implies, textdate
namespace {

struct ParsedInt { qulonglong value = 0; bool ok = false; };

/*
    /internal

    Read a whole number that must be the whole text.  QStringView::toULongLong()
    will happily ignore spaces and accept signs; but various date formats'
    fields (e.g. all in ISO) should not.
*/
ParsedInt readInt(QStringView text)
{
    ParsedInt result;
    for (QStringIterator it(text); it.hasNext();) {
        if (!QChar::isDigit(it.next()))
            return result;
    }
    result.value = text.toULongLong(&result.ok);
    return result;
}

}

/*!
    \fn QDate QDate::fromString(const QString &string, Qt::DateFormat format)

    Returns the QDate represented by the \a string, using the
    \a format given, or an invalid date if the string cannot be
    parsed.

    Note for Qt::TextDate: only English month names (e.g. "Jan" in short form or
    "January" in long form) are recognized.

    \sa toString(), QLocale::toDate()
*/

/*!
    \overload
    \since 6.0
*/
QDate QDate::fromString(QStringView string, Qt::DateFormat format)
{
    if (string.isEmpty())
        return QDate();

    switch (format) {
    case Qt::RFC2822Date:
        return rfcDateImpl(string).date;
    default:
    case Qt::TextDate: {
        // Documented as "ddd MMM d yyyy"
        auto parts = string.split(u' ', Qt::SkipEmptyParts);

        if (parts.count() != 4)
            return QDate();

        bool ok = false;
        int year = parts.at(3).toInt(&ok);
        int day = ok ? parts.at(2).toInt(&ok) : 0;
        if (!ok || !day)
            return QDate();

        const int month = fromShortMonthName(parts.at(1));
        if (month == -1) // Month name matches no English or localised name.
            return QDate();

        return QDate(year, month, day);
        }
    case Qt::ISODate:
        // Semi-strict parsing, must be long enough and have punctuators as separators
        if (string.size() >= 10 && string.at(4).isPunct() && string.at(7).isPunct()
                && (string.size() == 10 || !string.at(10).isDigit())) {
            const ParsedInt year = readInt(string.first(4));
            const ParsedInt month = readInt(string.sliced(5, 2));
            const ParsedInt day = readInt(string.sliced(8, 2));
            if (year.ok && year.value > 0 && year.value <= 9999 && month.ok && day.ok)
                return QDate(year.value, month.value, day.value);
        }
        break;
    }
    return QDate();
}

/*!
    \fn QDate QDate::fromString(const QString &string, const QString &format, QCalendar cal)

    Returns the QDate represented by the \a string, using the \a
    format given, or an invalid date if the string cannot be parsed.

    Uses \a cal as calendar if supplied, else the Gregorian calendar. Ranges of
    values in the format descriptions below are for the latter; they may be
    different for other calendars.

    These expressions may be used for the format:

    \table
    \header \li Expression \li Output
    \row \li d \li The day as a number without a leading zero (1 to 31)
    \row \li dd \li The day as a number with a leading zero (01 to 31)
    \row \li ddd \li The abbreviated day name ('Mon' to 'Sun').
    \row \li dddd \li The long day name ('Monday' to 'Sunday').
    \row \li M \li The month as a number without a leading zero (1 to 12)
    \row \li MM \li The month as a number with a leading zero (01 to 12)
    \row \li MMM \li The abbreviated month name ('Jan' to 'Dec').
    \row \li MMMM \li The long month name ('January' to 'December').
    \row \li yy \li The year as a two digit number (00 to 99)
    \row \li yyyy \li The year as a four digit number, possibly plus a leading
             minus sign for negative years.
    \endtable

    \note Day and month names must be given in English (C locale).
    If localized month and day names are used, use
    QLocale::system().toDate().

    All other input characters will be treated as text. Any non-empty sequence
    of characters enclosed in single quotes will also be treated (stripped of
    the quotes) as text and not be interpreted as expressions. For example:

    \snippet code/src_corelib_time_qdatetime.cpp 1

    If the format is not satisfied, an invalid QDate is returned. The
    expressions that don't expect leading zeroes (d, M) will be
    greedy. This means that they will use two digits even if this
    will put them outside the accepted range of values and leaves too
    few digits for other sections. For example, the following format
    string could have meant January 30 but the M will grab two
    digits, resulting in an invalid date:

    \snippet code/src_corelib_time_qdatetime.cpp 2

    For any field that is not represented in the format the following
    defaults are used:

    \table
    \header \li Field  \li Default value
    \row    \li Year   \li 1900
    \row    \li Month  \li 1 (January)
    \row    \li Day    \li 1
    \endtable

    The following examples demonstrate the default values:

    \snippet code/src_corelib_time_qdatetime.cpp 3

    \sa toString(), QDateTime::fromString(), QTime::fromString(),
        QLocale::toDate()
*/

/*!
    \fn QDate QDate::fromString(QStringView string, QStringView format, QCalendar cal)
    \overload
    \since 6.0
*/

/*!
    \overload
    \since 6.0
*/
QDate QDate::fromString(const QString &string, QStringView format, QCalendar cal)
{
    QDate date;
#if QT_CONFIG(datetimeparser)
    QDateTimeParser dt(QMetaType::QDate, QDateTimeParser::FromString, cal);
    dt.setDefaultLocale(QLocale::c());
    if (dt.parseFormat(format))
        dt.fromString(string, &date, nullptr);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
    Q_UNUSED(cal);
#endif
    return date;
}
#endif // datestring

/*!
    \overload

    Returns \c true if the specified date (\a year, \a month, and \a day) is
    valid in the Gregorian calendar; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_time_qdatetime.cpp 4

    \sa isNull(), setDate(), QCalendar::isDateValid()
*/

bool QDate::isValid(int year, int month, int day)
{
    return QGregorianCalendar::validParts(year, month, day);
}

/*!
    \fn bool QDate::isLeapYear(int year)

    Returns \c true if the specified \a year is a leap year in the Gregorian
    calendar; otherwise returns \c false.

    \sa QCalendar::isLeapYear()
*/

bool QDate::isLeapYear(int y)
{
    return QGregorianCalendar::leapTest(y);
}

/*! \fn static QDate QDate::fromJulianDay(qint64 jd)

    Converts the Julian day \a jd to a QDate.

    \sa toJulianDay()
*/

/*! \fn int QDate::toJulianDay() const

    Converts the date to a Julian day.

    \sa fromJulianDay()
*/

/*****************************************************************************
  QTime member functions
 *****************************************************************************/

/*!
    \class QTime
    \inmodule QtCore
    \reentrant

    \brief The QTime class provides clock time functions.

    A QTime object contains a clock time, which it can express as the numbers of
    hours, minutes, seconds, and milliseconds since midnight. It provides
    functions for comparing times and for manipulating a time by adding a number
    of milliseconds. QTime objects should be passed by value rather than by
    reference to const; they simply package \c int.

    QTime uses the 24-hour clock format; it has no concept of AM/PM.
    Unlike QDateTime, QTime knows nothing about time zones or
    daylight-saving time (DST).

    A QTime object is typically created either by giving the number of hours,
    minutes, seconds, and milliseconds explicitly, or by using the static
    function currentTime(), which creates a QTime object that represents the
    system's local time.

    The hour(), minute(), second(), and msec() functions provide
    access to the number of hours, minutes, seconds, and milliseconds
    of the time. The same information is provided in textual format by
    the toString() function.

    The addSecs() and addMSecs() functions provide the time a given
    number of seconds or milliseconds later than a given time.
    Correspondingly, the number of seconds or milliseconds
    between two times can be found using secsTo() or msecsTo().

    QTime provides a full set of operators to compare two QTime
    objects; an earlier time is considered smaller than a later one;
    if A.msecsTo(B) is positive, then A < B.

    QTime objects can also be created from a text representation using
    fromString() and converted to a string representation using toString(). All
    conversion to and from string formats is done using the C locale.  For
    localized conversions, see QLocale.

    \sa QDate, QDateTime
*/

/*!
    \fn QTime::QTime()

    Constructs a null time object. For a null time, isNull() returns \c true and
    isValid() returns \c false. If you need a zero time, use QTime(0, 0).  For
    the start of a day, see QDate::startOfDay().

    \sa isNull(), isValid()
*/

/*!
    Constructs a time with hour \a h, minute \a m, seconds \a s and
    milliseconds \a ms.

    \a h must be in the range 0 to 23, \a m and \a s must be in the
    range 0 to 59, and \a ms must be in the range 0 to 999.

    \sa isValid()
*/

QTime::QTime(int h, int m, int s, int ms)
{
    setHMS(h, m, s, ms);
}


/*!
    \fn bool QTime::isNull() const

    Returns \c true if the time is null (i.e., the QTime object was
    constructed using the default constructor); otherwise returns
    false. A null time is also an invalid time.

    \sa isValid()
*/

/*!
    Returns \c true if the time is valid; otherwise returns \c false. For example,
    the time 23:30:55.746 is valid, but 24:12:30 is invalid.

    \sa isNull()
*/

bool QTime::isValid() const
{
    return mds > NullTime && mds < MSECS_PER_DAY;
}


/*!
    Returns the hour part (0 to 23) of the time.

    Returns -1 if the time is invalid.

    \sa minute(), second(), msec()
*/

int QTime::hour() const
{
    if (!isValid())
        return -1;

    return ds() / MSECS_PER_HOUR;
}

/*!
    Returns the minute part (0 to 59) of the time.

    Returns -1 if the time is invalid.

    \sa hour(), second(), msec()
*/

int QTime::minute() const
{
    if (!isValid())
        return -1;

    return (ds() % MSECS_PER_HOUR) / MSECS_PER_MIN;
}

/*!
    Returns the second part (0 to 59) of the time.

    Returns -1 if the time is invalid.

    \sa hour(), minute(), msec()
*/

int QTime::second() const
{
    if (!isValid())
        return -1;

    return (ds() / MSECS_PER_SEC) % SECS_PER_MIN;
}

/*!
    Returns the millisecond part (0 to 999) of the time.

    Returns -1 if the time is invalid.

    \sa hour(), minute(), second()
*/

int QTime::msec() const
{
    if (!isValid())
        return -1;

    return ds() % MSECS_PER_SEC;
}

#if QT_CONFIG(datestring) // depends on, so implies, textdate
/*!
    \overload

    Returns the time as a string. The \a format parameter determines
    the format of the string.

    If \a format is Qt::TextDate, the string format is HH:mm:ss;
    e.g. 1 second before midnight would be "23:59:59".

    If \a format is Qt::ISODate, the string format corresponds to the
    ISO 8601 extended specification for representations of dates,
    represented by HH:mm:ss. To include milliseconds in the ISO 8601
    date, use the \a format Qt::ISODateWithMs, which corresponds to
    HH:mm:ss.zzz.

    If the \a format is Qt::RFC2822Date, the string is formatted in
    an \l{RFC 2822} compatible way. An example of this formatting is
    "23:59:20".

    If the time is invalid, an empty string will be returned.

    \sa fromString(), QDate::toString(), QDateTime::toString(), QLocale::toString()
*/

QString QTime::toString(Qt::DateFormat format) const
{
    if (!isValid())
        return QString();

    switch (format) {
    case Qt::ISODateWithMs:
        return QString::asprintf("%02d:%02d:%02d.%03d", hour(), minute(), second(), msec());
    case Qt::RFC2822Date:
    case Qt::ISODate:
    case Qt::TextDate:
    default:
        return QString::asprintf("%02d:%02d:%02d", hour(), minute(), second());
    }
}

/*!
    \fn QString QTime::toString(const QString &format) const
    \fn QString QTime::toString(QStringView format) const

    Returns the time as a string. The \a format parameter determines
    the format of the result string.

    These expressions may be used:

    \table
    \header \li Expression \li Output
    \row \li h
         \li The hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
    \row \li hh
         \li The hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)
    \row \li H
         \li The hour without a leading zero (0 to 23, even with AM/PM display)
    \row \li HH
         \li The hour with a leading zero (00 to 23, even with AM/PM display)
    \row \li m \li The minute without a leading zero (0 to 59)
    \row \li mm \li The minute with a leading zero (00 to 59)
    \row \li s \li The whole second, without any leading zero (0 to 59)
    \row \li ss \li The whole second, with a leading zero where applicable (00 to 59)
    \row \li z \li The fractional part of the second, to go after a decimal
                point, without trailing zeroes (0 to 999).  Thus "\c{s.z}"
                reports the seconds to full available (millisecond) precision
                without trailing zeroes.
    \row \li zzz \li The fractional part of the second, to millisecond
                precision, including trailing zeroes where applicable (000 to 999).
    \row \li AP or A
         \li Use AM/PM display. \e A/AP will be replaced by 'AM' or 'PM'
    \row \li ap or a
         \li Use am/pm display. \e a/ap will be replaced by 'am' or 'pm'
    \row \li t \li The timezone (for example "CEST")
    \endtable

    Any non-empty sequence of characters enclosed in single quotes will be
    included verbatim in the output string (stripped of the quotes), even if it
    contains formatting characters. Two consecutive single quotes ("''") are
    replaced by a single quote in the output. All other characters in the format
    string are included verbatim in the output string.

    Formats without separators (e.g. "ddMM") are supported but must be used with
    care, as the resulting strings aren't always reliably readable (e.g. if "dM"
    produces "212" it could mean either the 2nd of December or the 21st of
    February).

    Example format strings (assuming that the QTime is 14:13:09.042 and the system
    locale is \c{en_US})

    \table
    \header \li Format \li Result
    \row \li hh:mm:ss.zzz \li 14:13:09.042
    \row \li h:m:s ap     \li 2:13:9 pm
    \row \li H:m:s a      \li 14:13:9 pm
    \endtable

    If the time is invalid, an empty string will be returned.

    \note If localized forms of am or pm (the AP, ap, A or a formats) are
    desired, please use QLocale::system().toString().

    \sa fromString(), QDate::toString(), QDateTime::toString(), QLocale::toString()
*/
QString QTime::toString(QStringView format) const
{
    return QLocale::c().toString(*this, format);
}
#endif // datestring

/*!
    Sets the time to hour \a h, minute \a m, seconds \a s and
    milliseconds \a ms.

    \a h must be in the range 0 to 23, \a m and \a s must be in the
    range 0 to 59, and \a ms must be in the range 0 to 999.
    Returns \c true if the set time is valid; otherwise returns \c false.

    \sa isValid()
*/

bool QTime::setHMS(int h, int m, int s, int ms)
{
    if (!isValid(h,m,s,ms)) {
        mds = NullTime;                // make this invalid
        return false;
    }
    mds = (h * SECS_PER_HOUR + m * SECS_PER_MIN + s) * MSECS_PER_SEC + ms;
    return true;
}

/*!
    Returns a QTime object containing a time \a s seconds later
    than the time of this object (or earlier if \a s is negative).

    Note that the time will wrap if it passes midnight.

    Returns a null time if this time is invalid.

    Example:

    \snippet code/src_corelib_time_qdatetime.cpp 5

    \sa addMSecs(), secsTo(), QDateTime::addSecs()
*/

QTime QTime::addSecs(int s) const
{
    s %= SECS_PER_DAY;
    return addMSecs(s * MSECS_PER_SEC);
}

/*!
    Returns the number of seconds from this time to \a t.
    If \a t is earlier than this time, the number of seconds returned
    is negative.

    Because QTime measures time within a day and there are 86400
    seconds in a day, the result is always between -86400 and 86400.

    secsTo() does not take into account any milliseconds.

    Returns 0 if either time is invalid.

    \sa addSecs(), QDateTime::secsTo()
*/

int QTime::secsTo(QTime t) const
{
    if (!isValid() || !t.isValid())
        return 0;

    // Truncate milliseconds as we do not want to consider them.
    int ourSeconds = ds() / MSECS_PER_SEC;
    int theirSeconds = t.ds() / MSECS_PER_SEC;
    return theirSeconds - ourSeconds;
}

/*!
    Returns a QTime object containing a time \a ms milliseconds later
    than the time of this object (or earlier if \a ms is negative).

    Note that the time will wrap if it passes midnight. See addSecs()
    for an example.

    Returns a null time if this time is invalid.

    \sa addSecs(), msecsTo(), QDateTime::addMSecs()
*/

QTime QTime::addMSecs(int ms) const
{
    QTime t;
    if (isValid()) {
        if (ms < 0) {
            // %,/ not well-defined for -ve, so always work with +ve.
            int negdays = (MSECS_PER_DAY - ms) / MSECS_PER_DAY;
            t.mds = (ds() + ms + negdays * MSECS_PER_DAY) % MSECS_PER_DAY;
        } else {
            t.mds = (ds() + ms) % MSECS_PER_DAY;
        }
    }
    return t;
}

/*!
    Returns the number of milliseconds from this time to \a t.
    If \a t is earlier than this time, the number of milliseconds returned
    is negative.

    Because QTime measures time within a day and there are 86400
    seconds in a day, the result is always between -86400000 and
    86400000 ms.

    Returns 0 if either time is invalid.

    \sa secsTo(), addMSecs(), QDateTime::msecsTo()
*/

int QTime::msecsTo(QTime t) const
{
    if (!isValid() || !t.isValid())
        return 0;
    return t.ds() - ds();
}


/*!
    \fn bool QTime::operator==(QTime lhs, QTime rhs)

    Returns \c true if \a lhs is equal to \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator!=(QTime lhs, QTime rhs)

    Returns \c true if \a lhs is different from \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator<(QTime lhs, QTime rhs)

    Returns \c true if \a lhs is earlier than \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator<=(QTime lhs, QTime rhs)

    Returns \c true if \a lhs is earlier than or equal to \a rhs;
    otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator>(QTime lhs, QTime rhs)

    Returns \c true if \a lhs is later than \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator>=(QTime lhs, QTime rhs)

    Returns \c true if \a lhs is later than or equal to \a rhs;
    otherwise returns \c false.
*/

/*!
    \fn QTime QTime::fromMSecsSinceStartOfDay(int msecs)

    Returns a new QTime instance with the time set to the number of \a msecs
    since the start of the day, i.e. since 00:00:00.

    If \a msecs falls outside the valid range an invalid QTime will be returned.

    \sa msecsSinceStartOfDay()
*/

/*!
    \fn int QTime::msecsSinceStartOfDay() const

    Returns the number of msecs since the start of the day, i.e. since 00:00:00.

    \sa fromMSecsSinceStartOfDay()
*/

/*!
    \fn QTime::currentTime()

    Returns the current time as reported by the system clock.

    Note that the accuracy depends on the accuracy of the underlying
    operating system; not all systems provide 1-millisecond accuracy.

    Furthermore, currentTime() only increases within each day; it shall drop by
    24 hours each time midnight passes; and, beside this, changes in it may not
    correspond to elapsed time, if a daylight-saving transition intervenes.

    \sa QDateTime::currentDateTime(), QDateTime::currentDateTimeUtc()
*/

#if QT_CONFIG(datestring) // depends on, so implies, textdate

static QTime fromIsoTimeString(QStringView string, Qt::DateFormat format, bool *isMidnight24)
{
    Q_ASSERT(format == Qt::TextDate || format == Qt::ISODate || format == Qt::ISODateWithMs);
    if (isMidnight24)
        *isMidnight24 = false;
    // Match /\d\d(:\d\d(:\d\d)?)?([,.]\d+)?/ as "HH[:mm[:ss]][.zzz]"
    // The fractional part, if present, is in the same units as the field it follows.
    // TextDate restricts fractional parts to the seconds field.

    QStringView tail;
    const int dot = string.indexOf(u'.'), comma = string.indexOf(u',');
    if (dot != -1) {
        tail = string.sliced(dot + 1);
        if (tail.indexOf(u'.') != -1) // Forbid second dot:
            return QTime();
        string = string.first(dot);
    } else if (comma != -1) {
        tail = string.sliced(comma + 1);
        string = string.first(comma);
    }
    if (tail.indexOf(u',') != -1) // Forbid comma after first dot-or-comma:
        return QTime();

    const ParsedInt frac = readInt(tail);
    // There must be *some* digits in a fractional part; and it must be all digits:
    if (tail.isEmpty() ? dot != -1 || comma != -1 : !frac.ok)
        return QTime();
    Q_ASSERT(frac.ok ^ tail.isEmpty());
    double fraction = frac.ok ? frac.value * std::pow(0.1, tail.size()) : 0.0;

    const int size = string.size();
    if (size < 2 || size > 8)
        return QTime();

    ParsedInt hour = readInt(string.first(2));
    if (!hour.ok || hour.value > (format == Qt::TextDate ? 23 : 24))
        return QTime();

    ParsedInt minute;
    if (string.size() > 2) {
        if (string[2] == u':' && string.size() > 4)
            minute = readInt(string.sliced(3, 2));
        if (!minute.ok || minute.value >= 60)
            return QTime();
    } else if (format == Qt::TextDate) { // Requires minutes
        return QTime();
    } else if (frac.ok) {
        Q_ASSERT(!(fraction < 0.0) && fraction < 1.0);
        fraction *= 60;
        minute.value = qulonglong(fraction);
        fraction -= minute.value;
    }

    ParsedInt second;
    if (string.size() > 5) {
        if (string[5] == u':' && string.size() == 8)
            second = readInt(string.sliced(6, 2));
        if (!second.ok || second.value >= 60)
            return QTime();
    } else if (frac.ok) {
        if (format == Qt::TextDate) // Doesn't allow fraction of minutes
            return QTime();
        Q_ASSERT(!(fraction < 0.0) && fraction < 1.0);
        fraction *= 60;
        second.value = qulonglong(fraction);
        fraction -= second.value;
    }

    Q_ASSERT(!(fraction < 0.0) && fraction < 1.0);
    // Round millis to nearest (unlike minutes and seconds, rounded down):
    int msec = frac.ok ? qRound(MSECS_PER_SEC * fraction) : 0;
    // But handle overflow gracefully:
    if (msec == MSECS_PER_SEC) {
        // If we can (when data were otherwise valid) validly propagate overflow
        // into other fields, do so:
        if (isMidnight24 || hour.value < 23 || minute.value < 59 || second.value < 59) {
            msec = 0;
            if (++second.value == 60) {
                second.value = 0;
                if (++minute.value == 60) {
                    minute.value = 0;
                    ++hour.value;
                    // May need to propagate further via isMidnight24, see below
                }
            }
        } else {
            // QTime::fromString() or Qt::TextDate: rounding up would cause
            // 23:59:59.999... to become invalid; clip to 999 ms instead:
            msec = MSECS_PER_SEC - 1;
        }
    }

    // For ISO date format, 24:0:0 means 0:0:0 on the next day:
    if (hour.value == 24 && minute.value == 0 && second.value == 0 && msec == 0) {
        Q_ASSERT(format != Qt::TextDate); // It clipped hour at 23, above.
        if (isMidnight24)
            *isMidnight24 = true;
        hour.value = 0;
    }

    return QTime(hour.value, minute.value, second.value, msec);
}

/*!
    \fn QTime QTime::fromString(const QString &string, Qt::DateFormat format)

    Returns the time represented in the \a string as a QTime using the
    \a format given, or an invalid time if this is not possible.

    \sa toString(), QLocale::toTime()
*/

/*!
    \overload
    \since 6.0
*/
QTime QTime::fromString(QStringView string, Qt::DateFormat format)
{
    if (string.isEmpty())
        return QTime();

    switch (format) {
    case Qt::RFC2822Date:
        return rfcDateImpl(string).time;
    case Qt::ISODate:
    case Qt::ISODateWithMs:
    case Qt::TextDate:
    default:
        return fromIsoTimeString(string, format, nullptr);
    }
}

/*!
    \fn QTime QTime::fromString(const QString &string, const QString &format)

    Returns the QTime represented by the \a string, using the \a
    format given, or an invalid time if the string cannot be parsed.

    These expressions may be used for the format:

    \table
    \header \li Expression \li Output
    \row \li h
         \li The hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
    \row \li hh
         \li The hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)
    \row \li H
         \li The hour without a leading zero (0 to 23, even with AM/PM display)
    \row \li HH
         \li The hour with a leading zero (00 to 23, even with AM/PM display)
    \row \li m \li The minute without a leading zero (0 to 59)
    \row \li mm \li The minute with a leading zero (00 to 59)
    \row \li s \li The whole second, without any leading zero (0 to 59)
    \row \li ss \li The whole second, with a leading zero where applicable (00 to 59)
    \row \li z \li The fractional part of the second, to go after a decimal
                point, without trailing zeroes (0 to 999).  Thus "\c{s.z}"
                reports the seconds to full available (millisecond) precision
                without trailing zeroes.
    \row \li zzz \li The fractional part of the second, to millisecond
                precision, including trailing zeroes where applicable (000 to 999).
    \row \li AP or A
         \li Interpret as an AM/PM time. \e A/AP will match 'AM' or 'PM'.
    \row \li ap or a
         \li Interpret as an am/pm time. \e a/ap will match 'am' or 'pm'.
    \endtable

    All other input characters will be treated as text. Any non-empty sequence
    of characters enclosed in single quotes will also be treated (stripped of
    the quotes) as text and not be interpreted as expressions.

    \snippet code/src_corelib_time_qdatetime.cpp 6

    If the format is not satisfied, an invalid QTime is returned.
    Expressions that do not expect leading zeroes to be given (h, m, s
    and z) are greedy. This means that they will use two digits (or three, for z) even if
    this puts them outside the range of accepted values and leaves too
    few digits for other sections. For example, the following string
    could have meant 00:07:10, but the m will grab two digits, resulting
    in an invalid time:

    \snippet code/src_corelib_time_qdatetime.cpp 7

    Any field that is not represented in the format will be set to zero.
    For example:

    \snippet code/src_corelib_time_qdatetime.cpp 8

    \note If localized forms of am or pm (the AP, ap, A or a formats) are used,
    please use QLocale::system().toTime().

    \sa toString(), QDateTime::fromString(), QDate::fromString(),
    QLocale::toTime()
*/

/*!
    \fn QTime QTime::fromString(QStringView string, QStringView format)
    \overload
    \since 6.0
*/

/*!
    \overload
    \since 6.0
*/
QTime QTime::fromString(const QString &string, QStringView format)
{
    QTime time;
#if QT_CONFIG(datetimeparser)
    QDateTimeParser dt(QMetaType::QTime, QDateTimeParser::FromString, QCalendar());
    dt.setDefaultLocale(QLocale::c());
    if (dt.parseFormat(format))
        dt.fromString(string, nullptr, &time);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
#endif
    return time;
}
#endif // datestring


/*!
    \overload

    Returns \c true if the specified time is valid; otherwise returns
    false.

    The time is valid if \a h is in the range 0 to 23, \a m and
    \a s are in the range 0 to 59, and \a ms is in the range 0 to 999.

    Example:

    \snippet code/src_corelib_time_qdatetime.cpp 9
*/

bool QTime::isValid(int h, int m, int s, int ms)
{
    return uint(h) < 24 && uint(m) < 60 && uint(s) < SECS_PER_MIN && uint(ms) < MSECS_PER_SEC;
}

/*****************************************************************************
  QDateTime static helper functions
 *****************************************************************************/

// get the types from QDateTime (through QDateTimePrivate)
typedef QDateTimePrivate::QDateTimeShortData ShortData;
typedef QDateTimePrivate::QDateTimeData QDateTimeData;

// Returns the tzname, assume tzset has been called already
static QString qt_tzname(QDateTimePrivate::DaylightStatus daylightStatus)
{
    int isDst = (daylightStatus == QDateTimePrivate::DaylightTime) ? 1 : 0;
#if defined(Q_CC_MSVC)
    size_t s = 0;
    char name[512];
    if (_get_tzname(&s, name, 512, isDst))
        return QString();
    return QString::fromLocal8Bit(name);
#else
    return QString::fromLocal8Bit(tzname[isDst]);
#endif // Q_OS_WIN
}

#if QT_CONFIG(datetimeparser)
/*
  \internal
  Implemented here to share qt_tzname()
*/
int QDateTimeParser::startsWithLocalTimeZone(QStringView name)
{
    QDateTimePrivate::DaylightStatus zones[2] = {
        QDateTimePrivate::StandardTime,
        QDateTimePrivate::DaylightTime
    };
    for (const auto z : zones) {
        QString zone(qt_tzname(z));
        if (name.startsWith(zone))
            return zone.size();
    }
    return 0;
}
#endif // datetimeparser

/*
    Qt represents n BCE as -n, whereas struct tm's tm_year field represents a
    year by the number of years after (negative for before) 1900, so that 1+m
    BCE is -1900 -m; so treating 1 BCE as 0 CE. We thus shift by different
    offsets depending on whether the year is BCE or CE.
*/
static constexpr int tmYearFromQYear(int year) { return year - (year < 0 ? 1899 : 1900); }
static constexpr int qYearFromTmYear(int year) { return year + (year < -1899 ? 1899 : 1900); }

// Calls the platform variant of mktime for the given date, time and
// daylightStatus, and updates the date, time, daylightStatus and abbreviation
// with the returned values. If the date falls outside the time_t range
// supported by mktime, then date/time will not be updated and *ok is set false.
static qint64 qt_mktime(QDate *date, QTime *time, QDateTimePrivate::DaylightStatus *daylightStatus,
                        QString *abbreviation, bool *ok)
{
    Q_ASSERT(ok);
    qint64 msec = time->msec();
    int yy, mm, dd;
    date->getDate(&yy, &mm, &dd);

    // All other platforms provide standard C library time functions
    tm local = {};
    local.tm_sec = time->second();
    local.tm_min = time->minute();
    local.tm_hour = time->hour();
    local.tm_mday = dd;
    local.tm_mon = mm - 1;
    local.tm_year = tmYearFromQYear(yy);
    local.tm_isdst = daylightStatus ? int(*daylightStatus) : -1;

#if defined(Q_OS_WIN)
    int hh = local.tm_hour;
#endif // Q_OS_WIN
    time_t secsSinceEpoch = qMkTime(&local);
    // That can fail if we thought we knew DST-ness, but were wrong:
    if (secsSinceEpoch == time_t(-1) && local.tm_isdst >= 0) {
        local.tm_isdst = -1;
        secsSinceEpoch = qMkTime(&local);
    }

    if (secsSinceEpoch != time_t(-1)) {
        *date = QDate(qYearFromTmYear(local.tm_year), local.tm_mon + 1, local.tm_mday);
        *time = QTime(local.tm_hour, local.tm_min, local.tm_sec, msec);
#if defined(Q_OS_WIN)
        // Windows mktime for the missing hour subtracts 1 hour from the time
        // instead of adding 1 hour.  If time differs and is standard time then
        // this has happened, so add 2 hours to the time and 1 hour to the msecs
        if (local.tm_isdst == 0 && local.tm_hour != hh) {
            if (time->hour() >= 22)
                *date = date->addDays(1);
            *time = time->addSecs(2 * SECS_PER_HOUR);
            secsSinceEpoch += SECS_PER_HOUR;
            local.tm_isdst = 1;
        }
#endif // Q_OS_WIN
        if (local.tm_isdst > 0) {
            if (daylightStatus)
                *daylightStatus = QDateTimePrivate::DaylightTime;
            if (abbreviation)
                *abbreviation = qt_tzname(QDateTimePrivate::DaylightTime);
        } else {
            if (daylightStatus) {
                *daylightStatus = (local.tm_isdst == 0
                                   ? QDateTimePrivate::StandardTime
                                   : QDateTimePrivate::UnknownDaylightTime);
            }
            if (abbreviation)
                *abbreviation = qt_tzname(QDateTimePrivate::StandardTime);
        }
    } else if (yy == 1969 && mm == 12 && dd == 31
               && QTime(0, 0).secsTo(*time) == SECS_PER_DAY - 1) {
        // There was, of course, a last second in 1969, at time_t(-1); we won't
        // rescue it if it's not in normalised form, and we don't know its DST
        // status (unless we did already), but let's not wantonly declare it
        // invalid.
    } else {
        *date = QDate();
        *time = QTime();
        if (daylightStatus)
            *daylightStatus = QDateTimePrivate::UnknownDaylightTime;
        if (abbreviation)
            *abbreviation = QString();
        *ok = false;
        return 0;
    }
    if (secsSinceEpoch < 0 && msec > 0) {
        secsSinceEpoch++;
        msec -= MSECS_PER_SEC;
    }
    qint64 millis;
    const bool overflow =
        mul_overflow(qint64(secsSinceEpoch),
                     std::integral_constant<qint64, MSECS_PER_SEC>(), &millis)
        || add_overflow(millis, msec, &msec);
    *ok = !overflow;

    return msec;
}

// Calls the platform variant of localtime for the given msecs, and updates
// the date, time, and DST status with the returned values.
static bool qt_localtime(qint64 msecsSinceEpoch, QDate *localDate, QTime *localTime,
                         QDateTimePrivate::DaylightStatus *daylightStatus)
{
    const int signFix = msecsSinceEpoch % MSECS_PER_SEC && msecsSinceEpoch < 0 ? 1 : 0;
    const time_t secsSinceEpoch = msecsSinceEpoch / MSECS_PER_SEC - signFix;
    const int msec = msecsSinceEpoch % MSECS_PER_SEC + signFix * MSECS_PER_SEC;
    Q_ASSERT(msec >= 0 && msec < MSECS_PER_SEC);

    tm local;
    bool valid = false;

    // localtime() is specified to work as if it called tzset().
    // localtime_r() does not have this constraint, so make an explicit call.
    // The explicit call should also request the timezone info be re-parsed.
    qTzSet();
    if (qint64(secsSinceEpoch) * MSECS_PER_SEC + msec == msecsSinceEpoch) {
#if QT_CONFIG(thread) && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
        // Use the reentrant version of localtime() where available
        // as is thread-safe and doesn't use a shared static data area
        if (tm *res = localtime_r(&secsSinceEpoch, &local)) {
            Q_ASSERT(res == &local);
            valid = true;
        }
#elif defined(Q_CC_MSVC)
        if (!_localtime64_s(&local, &secsSinceEpoch))
            valid = true;
#else
        // Returns shared static data which may be overwritten at any time
        // So copy the result asap
        if (tm *res = localtime(&secsSinceEpoch)) {
            local = *res;
            valid = true;
        }
#endif
    }
    if (valid) {
        *localDate = QDate(qYearFromTmYear(local.tm_year), local.tm_mon + 1, local.tm_mday);
        *localTime = QTime(local.tm_hour, local.tm_min, local.tm_sec, msec);
        if (daylightStatus) {
            if (local.tm_isdst > 0)
                *daylightStatus = QDateTimePrivate::DaylightTime;
            else if (local.tm_isdst < 0)
                *daylightStatus = QDateTimePrivate::UnknownDaylightTime;
            else
                *daylightStatus = QDateTimePrivate::StandardTime;
        }
        return true;
    } else {
        *localDate = QDate();
        *localTime = QTime();
        if (daylightStatus)
            *daylightStatus = QDateTimePrivate::UnknownDaylightTime;
        return false;
    }
}

// Converts an msecs value into a date and time
static void msecsToTime(qint64 msecs, QDate *date, QTime *time)
{
    qint64 jd = JULIAN_DAY_FOR_EPOCH;
    qint64 ds = 0;

    if (msecs >= MSECS_PER_DAY || msecs <= -MSECS_PER_DAY) {
        jd += msecs / MSECS_PER_DAY;
        msecs %= MSECS_PER_DAY;
    }

    if (msecs < 0) {
        ds = MSECS_PER_DAY - msecs - 1;
        jd -= ds / MSECS_PER_DAY;
        ds = ds % MSECS_PER_DAY;
        ds = MSECS_PER_DAY - ds - 1;
    } else {
        ds = msecs;
    }

    if (date)
        *date = QDate::fromJulianDay(jd);
    if (time)
        *time = QTime::fromMSecsSinceStartOfDay(ds);
}

// Converts a date/time value into msecs
static qint64 timeToMSecs(QDate date, QTime time)
{
    return ((date.toJulianDay() - JULIAN_DAY_FOR_EPOCH) * MSECS_PER_DAY)
           + time.msecsSinceStartOfDay();
}

/*!
    \internal
    Determine the range of the system time_t functions.

    On MS-systems (where time_t is 64-bit by default), the start-point is the
    epoch, the end-point is the end of the year 3000 (for mktime(); for
    _localtime64_s it's 18 days later, but we ignore that here). Darwin's range
    runs from the beginning of 1900 to the end of its 64-bit time_t and Linux
    uses the full range of time_t (but this might still be 32-bit on some
    embedded systems).

    (One potential constraint might appear to be the range of struct tm's int
    tm_year, only allowing time_t to represent times from the start of year
    1900+INT_MIN to the end of year INT_MAX. The 26-bit number of seconds in a
    year means that a 64-bit time_t can indeed represent times outside the range
    of 32-bit years, by a factor of 32 - but the range of representable
    milliseconds needs ten more bits than that of seconds, so can't reach the
    ends of the 32-bit year range.)

    Given the diversity of ranges, we conservatively estimate the actual
    supported range by experiment on the first call to millisInSystemRange() by
    exploration among the known candidates, converting the result to
    milliseconds and flagging whether each end is the qint64 range's bound (so
    millisInSystemRange will know not to try to pad beyond those bounds). The
    probed date-times are somewhat inside the range, but close enough to the
    relevant bound that we can be fairly sure the bound is reached, if the probe
    succeeds.
*/
static auto computeSystemMillisRange()
{
    struct R { qint64 min, max; bool minClip, maxClip; };
    using Bounds = std::numeric_limits<qint64>;
    constexpr bool isNarrow = Bounds::max() / MSECS_PER_SEC > TIME_T_MAX;
    if constexpr (isNarrow) {
        const qint64 msecsMax = quint64(TIME_T_MAX) * MSECS_PER_SEC - 1 + MSECS_PER_SEC;
        const qint64 msecsMin = -1 - msecsMax; // TIME_T_MIN is -1 - TIME_T_MAX
        // If we reach back to msecsMin, use it; otherwise, assume 1970 cut-off (MS).
        struct tm local = {};
        local.tm_year = tmYearFromQYear(1901);
        local.tm_mon = 11;
        local.tm_mday = 15; // A day and a bit after the start of 32-bit time_t:
        return R{qMkTime(&local) == -1 ? 0 : msecsMin, msecsMax, false, false};
    } else {
        const struct { int year; qint64 millis; } starts[] = {
            { int(QDateTime::YearRange::First) + 1, Bounds::min() },
            // Beginning of the Common Era:
            { 1, -Q_INT64_C(62135596800000) },
            // Invention of the Gregorian calendar:
            { 1582, -Q_INT64_C(12244089600000) },
            // Its adoption by the anglophone world:
            { 1752, -Q_INT64_C(6879427200000) },
            // Before this, struct tm's tm_year is negative (Darwin):
            { 1900, -Q_INT64_C(2208988800000) },
        }, ends[] = {
            { int(QDateTime::YearRange::Last) - 1, Bounds::max() },
            // MS's end-of-range, end of year 3000:
            { 3000, Q_INT64_C(32535215999999) },
        };
        // Assume we do at least reach the end of 32-bit time_t:
        qint64 stop = quint64(TIME_T_MAX) * MSECS_PER_SEC - 1 + MSECS_PER_SEC;
        // Cleared if first pass round loop fails:
        bool stopMax = true;
        for (const auto c : ends) {
            struct tm local = {};
            local.tm_year = tmYearFromQYear(c.year);
            local.tm_mon = 11;
            local.tm_mday = 31;
            local.tm_hour = 23;
            local.tm_min = local.tm_sec = 59;
            if (qMkTime(&local) != -1) {
                stop = c.millis;
                break;
            }
            stopMax = false;
        }
        bool startMin = true;
        for (const auto c : starts) {
            struct tm local {};
            local.tm_year = tmYearFromQYear(c.year);
            local.tm_mon = 1;
            local.tm_mday = 1;
            if (qMkTime(&local) != -1)
                return R{c.millis, stop, startMin, stopMax};
            startMin = false;
        }
        return R{0, stop, false, stopMax};
    }
}

/*!
    \internal
    Tests whether system functions can handle a given time.

    The range of milliseconds for which the time_t-based functions work depends
    somewhat on platform (see computeSystemMillisRange() for details). This
    function tests whether the UTC time \a millis milliseconds from the epoch is
    in the supported range.

    To test a local time, pass an upper bound on the magnitude of time-zone
    correction potentially needed as \a slack: in this case the range is
    extended by this many milliseconds at each end (where applicable). The
    function then returns true precisely if \a millis is within this (possibly)
    widened range. This doesn't guarantee that the time_t functions can handle
    the time, so check their returns to be sure. Values for which the function
    returns false should be assumed unrepresentable.
*/
static inline bool millisInSystemRange(qint64 millis, qint64 slack = 0)
{
    static const auto bounds = computeSystemMillisRange();
    return (bounds.minClip || millis >= bounds.min - slack)
        && (bounds.maxClip || millis <= bounds.max + slack);
}

// First year for which system functions give useful answers, when earlier times
// aren't handled by those functions (see millisInSystemRange):
#ifdef Q_OS_WIN
constexpr int firstSystemTimeYear = 1970;
#else // First year fully in 32-bit time_t range:
constexpr int firstSystemTimeYear = 1902;
#endif

// Convert an MSecs Since Epoch into Local Time
bool QDateTimePrivate::epochMSecsToLocalTime(qint64 msecs, QDate *localDate, QTime *localTime,
                                             QDateTimePrivate::DaylightStatus *daylightStatus)
{
    if (!millisInSystemRange(msecs)) {
        // Docs state any LocalTime after 2038-01-18 *will* have any DST applied.
        // When this falls outside the supported range, we need to fake it.
#if QT_CONFIG(timezone)
        // Use the system time-zone.
        const auto sys = QTimeZone::systemTimeZone();
        if (sys.isValid()) {
            if (daylightStatus) {
                *daylightStatus = sys.d->isDaylightTime(msecs)
                    ? QDateTimePrivate::DaylightTime
                    : QDateTimePrivate::StandardTime;
            }

            // NB: cast to qint64 here is important to make sure a matching
            // add_overflow is found, GCC 7.5.0 fails without this cast
            if (add_overflow(msecs, qint64(sys.d->offsetFromUtc(msecs)) * MSECS_PER_SEC, &msecs))
                return false;
            msecsToTime(msecs, localDate, localTime);
            return true;
        }
#endif // timezone
        // Kludge
        // Use existing method to fake the conversion (this is deeply flawed
        // as it may apply the conversion from the wrong day number, e.g. if
        // rule is last Sunday of month).
        QDate utcDate;
        QTime utcTime;
        msecsToTime(msecs, &utcDate, &utcTime);
        int year, month, day;
        utcDate.getDate(&year, &month, &day);
        // No boundary year is a leap year, so make sure date isn't Feb 29
        if (month == 2 && day == 29)
            --day;
        QDate fakeDate(year < 1970 ? firstSystemTimeYear : 2037, month, day);
        qint64 fakeMsecs = QDateTime(fakeDate, utcTime, Qt::UTC).toMSecsSinceEpoch();
        bool res = qt_localtime(fakeMsecs, localDate, localTime, daylightStatus);
        *localDate = localDate->addDays(fakeDate.daysTo(utcDate));
        return res;
    }

    // Falls inside time_t supported range so localtime can handle it:
    return qt_localtime(msecs, localDate, localTime, daylightStatus);
}

// Convert a LocalTime expressed in local msecs encoding and the corresponding
// DST status into a UTC epoch msecs. Optionally populate the returned
// values from mktime for the adjusted local date and time.
qint64 QDateTimePrivate::localMSecsToEpochMSecs(qint64 localMsecs,
                                                QDateTimePrivate::DaylightStatus *daylightStatus,
                                                QDate *localDate, QTime *localTime,
                                                QString *abbreviation)
{
    // First, if localMsecs is within +/- 1 day of viable range, try mktime() in
    // case it does fall in the range and gets proper DST conversion:
    if (millisInSystemRange(localMsecs, MSECS_PER_DAY)) {
        bool valid;
        QDate dt;
        QTime tm;
        msecsToTime(localMsecs, &dt, &tm);
        const qint64 utcMsecs = qt_mktime(&dt, &tm, daylightStatus, abbreviation, &valid);
        if (valid && millisInSystemRange(utcMsecs)) {
            // mktime worked and falls in valid range, so use it
            if (localDate)
                *localDate = dt;
            if (localTime)
                *localTime = tm;
            return utcMsecs;
        }
    }

    // Otherwise, outside the system range.
#if QT_CONFIG(timezone)
    // Use the system zone:
    const auto sys = QTimeZone::systemTimeZone();
    if (sys.isValid()) {
        return QDateTimePrivate::zoneMSecsToEpochMSecs(localMsecs, sys, daylightStatus,
                                                       localDate, localTime, abbreviation);
    }
#endif // timezone
    // Kludge
    // Use existing method to fake the conversion (this is deeply flawed as it
    // may apply the conversion from the wrong day number, e.g. if rule is last
    // Sunday of month).
    QDate dt;
    QTime tm;
    msecsToTime(localMsecs, &dt, &tm);
    int year, month, day;
    dt.getDate(&year, &month, &day);
    // No boundary year is a leap year, so make sure date isn't Feb 29
    if (month == 2 && day == 29)
        --day;
    bool ok;
    QDate fakeDate(year < 1970 ? firstSystemTimeYear : 2037, month, day);
    const qint64 fakeDiff = fakeDate.daysTo(dt);
    const qint64 utcMsecs = qt_mktime(&fakeDate, &tm, daylightStatus, abbreviation, &ok);
    Q_ASSERT(ok);
    if (localDate)
        *localDate = fakeDate.addDays(fakeDiff);
    if (localTime)
        *localTime = tm;
    QDate utcDate;
    QTime utcTime;
    msecsToTime(utcMsecs, &utcDate, &utcTime);
    return timeToMSecs(utcDate.addDays(fakeDiff), utcTime);
}

static inline bool specCanBeSmall(Qt::TimeSpec spec)
{
    return spec == Qt::LocalTime || spec == Qt::UTC;
}

static inline bool msecsCanBeSmall(qint64 msecs)
{
    if (!QDateTimeData::CanBeSmall)
        return false;

    ShortData sd;
    sd.msecs = qintptr(msecs);
    return sd.msecs == msecs;
}

static constexpr inline
QDateTimePrivate::StatusFlags mergeSpec(QDateTimePrivate::StatusFlags status, Qt::TimeSpec spec)
{
    status &= ~QDateTimePrivate::TimeSpecMask;
    status |= QDateTimePrivate::StatusFlags::fromInt(int(spec) << QDateTimePrivate::TimeSpecShift);
    return status;
}

static constexpr inline Qt::TimeSpec extractSpec(QDateTimePrivate::StatusFlags status)
{
    return Qt::TimeSpec((status & QDateTimePrivate::TimeSpecMask).toInt() >> QDateTimePrivate::TimeSpecShift);
}

// Set the Daylight Status if LocalTime set via msecs
static constexpr inline QDateTimePrivate::StatusFlags
mergeDaylightStatus(QDateTimePrivate::StatusFlags sf, QDateTimePrivate::DaylightStatus status)
{
    sf &= ~QDateTimePrivate::DaylightMask;
    if (status == QDateTimePrivate::DaylightTime) {
        sf |= QDateTimePrivate::SetToDaylightTime;
    } else if (status == QDateTimePrivate::StandardTime) {
        sf |= QDateTimePrivate::SetToStandardTime;
    }
    return sf;
}

// Get the DST Status if LocalTime set via msecs
static constexpr inline
QDateTimePrivate::DaylightStatus extractDaylightStatus(QDateTimePrivate::StatusFlags status)
{
    if (status & QDateTimePrivate::SetToDaylightTime)
        return QDateTimePrivate::DaylightTime;
    if (status & QDateTimePrivate::SetToStandardTime)
        return QDateTimePrivate::StandardTime;
    return QDateTimePrivate::UnknownDaylightTime;
}

static inline qint64 getMSecs(const QDateTimeData &d)
{
    if (d.isShort()) {
        // same as, but producing better code
        //return d.data.msecs;
        return qintptr(d.d) >> 8;
    }
    return d->m_msecs;
}

static inline QDateTimePrivate::StatusFlags getStatus(const QDateTimeData &d)
{
    if (d.isShort()) {
        // same as, but producing better code
        //return StatusFlag(d.data.status);
        return QDateTimePrivate::StatusFlag(qintptr(d.d) & 0xFF);
    }
    return d->m_status;
}

static inline Qt::TimeSpec getSpec(const QDateTimeData &d)
{
    return extractSpec(getStatus(d));
}

/* True if we *can cheaply determine* that a and b use the same offset.
   If they use different offsets or it would be expensive to find out, false.
   Calls to toMSecsSinceEpoch() are expensive, for these purposes.
   See QDateTime's comparison operators.
*/
static inline bool usesSameOffset(const QDateTimeData &a, const QDateTimeData &b)
{
    const auto status = getStatus(a);
    if (status != getStatus(b))
        return false;
    // Status includes DST-ness, so we now know they match in it.

    switch (extractSpec(status)) {
    case Qt::LocalTime:
    case Qt::UTC:
        return true;

    case Qt::TimeZone:
        /* TimeZone always determines its offset during construction of the
           private data. Even if we're in different zones, what matters is the
           offset actually in effect at the specific time. (DST can cause things
           with the same time-zone to use different offsets, but we already
           checked their DSTs match.) */
    case Qt::OffsetFromUTC: // always knows its offset, which is all that matters.
        Q_ASSERT(!a.isShort() && !b.isShort());
        return a->m_offsetFromUtc == b->m_offsetFromUtc;
    }
    Q_UNREACHABLE();
    return false;
}

// Refresh the LocalTime or TimeZone validity and offset
static void refreshZonedDateTime(QDateTimeData &d, Qt::TimeSpec spec)
{
    Q_ASSERT(spec == Qt::TimeZone || spec == Qt::LocalTime);
    auto status = getStatus(d);
    Q_ASSERT(extractSpec(status) == spec);
    int offsetFromUtc = 0;

    // If not valid date and time then is invalid
    if (!(status & QDateTimePrivate::ValidDate) || !(status & QDateTimePrivate::ValidTime)) {
        status &= ~QDateTimePrivate::ValidDateTime;
    } else {
        // We have a valid date and time and a Qt::LocalTime or Qt::TimeZone that needs calculating
        // LocalTime and TimeZone might fall into a "missing" DST transition hour
        // Calling toEpochMSecs will adjust the returned date/time if it does
        const qint64 msecs = getMSecs(d);
        qint64 epochMSecs = 0;
        QDate testDate;
        QTime testTime;
        auto dstStatus = extractDaylightStatus(status);
        if (spec == Qt::LocalTime) {
            epochMSecs =
                QDateTimePrivate::localMSecsToEpochMSecs(msecs, &dstStatus, &testDate, &testTime);
#if QT_CONFIG(timezone)
        // else spec == Qt::TimeZone, so check zone is valid:
        } else if (d->m_timeZone.isValid()) {
            epochMSecs = QDateTimePrivate::zoneMSecsToEpochMSecs(
                msecs, d->m_timeZone, &dstStatus, &testDate, &testTime);
#endif // timezone
        } // else: testDate, testTime haven't been set, so are invalid.
        const bool ok = testDate.isValid() && testTime.isValid();
        // Cache the offset to use in offsetFromUtc() &c., even if the next
        // check marks invalid; this lets fromMSecsSinceEpoch() give a useful
        // fallback for times in spring-forward gaps.
        if (ok)
            offsetFromUtc = (msecs - epochMSecs) / MSECS_PER_SEC;
        Q_ASSERT(offsetFromUtc >= -SECS_PER_DAY && offsetFromUtc <= SECS_PER_DAY);
        if (ok && timeToMSecs(testDate, testTime) == msecs) {
            status = mergeDaylightStatus(status, dstStatus);
            status |= QDateTimePrivate::ValidDateTime;
        } else {
            status &= ~QDateTimePrivate::ValidDateTime;
        }
    }

    if (status & QDateTimePrivate::ShortData) {
        d.data.status = status.toInt();
    } else {
        d->m_status = status;
        d->m_offsetFromUtc = offsetFromUtc;
    }
}

// Check the UTC / offsetFromUTC validity
static void refreshSimpleDateTime(QDateTimeData &d)
{
    auto status = getStatus(d);
    Q_ASSERT(extractSpec(status) == Qt::UTC || extractSpec(status) == Qt::OffsetFromUTC);
    if ((status & QDateTimePrivate::ValidDate) && (status & QDateTimePrivate::ValidTime))
        status |= QDateTimePrivate::ValidDateTime;
    else
        status &= ~QDateTimePrivate::ValidDateTime;

    if (status & QDateTimePrivate::ShortData)
        d.data.status = status.toInt();
    else
        d->m_status = status;
}

// Clean up and set status after assorted set-up or reworking:
static void checkValidDateTime(QDateTimeData &d)
{
    auto status = getStatus(d);
    auto spec = extractSpec(status);
    switch (spec) {
    case Qt::OffsetFromUTC:
    case Qt::UTC:
        // for these, a valid date and a valid time imply a valid QDateTime
        refreshSimpleDateTime(d);
        break;
    case Qt::TimeZone:
    case Qt::LocalTime:
        // for these, we need to check whether the timezone is valid and whether
        // the time is valid in that timezone. Expensive, but no other option.
        refreshZonedDateTime(d, spec);
        break;
    }
}

// Caller needs to refresh after calling this
static void setTimeSpec(QDateTimeData &d, Qt::TimeSpec spec, int offsetSeconds)
{
    auto status = getStatus(d);
    status &= ~(QDateTimePrivate::ValidDateTime | QDateTimePrivate::DaylightMask |
                QDateTimePrivate::TimeSpecMask);

    switch (spec) {
    case Qt::OffsetFromUTC:
        if (offsetSeconds == 0)
            spec = Qt::UTC;
        break;
    case Qt::TimeZone:
        qWarning("Using TimeZone in setTimeSpec() is unsupported"); // Use system time zone instead
        spec = Qt::LocalTime;
        Q_FALLTHROUGH();
    case Qt::UTC:
    case Qt::LocalTime:
        offsetSeconds = 0;
        break;
    }

    status = mergeSpec(status, spec);
    if (d.isShort() && offsetSeconds == 0) {
        d.data.status = status.toInt();
    } else {
        d.detach();
        d->m_status = status & ~QDateTimePrivate::ShortData;
        d->m_offsetFromUtc = offsetSeconds;
#if QT_CONFIG(timezone)
        d->m_timeZone = QTimeZone();
#endif // timezone
    }
}

static void setDateTime(QDateTimeData &d, QDate date, QTime time)
{
    // If the date is valid and the time is not we set time to 00:00:00
    QTime useTime = time;
    if (!useTime.isValid() && date.isValid())
        useTime = QTime::fromMSecsSinceStartOfDay(0);

    QDateTimePrivate::StatusFlags newStatus = { };

    // Set date value and status
    qint64 days = 0;
    if (date.isValid()) {
        days = date.toJulianDay() - JULIAN_DAY_FOR_EPOCH;
        newStatus = QDateTimePrivate::ValidDate;
    }

    // Set time value and status
    int ds = 0;
    if (useTime.isValid()) {
        ds = useTime.msecsSinceStartOfDay();
        newStatus |= QDateTimePrivate::ValidTime;
    }
    Q_ASSERT(ds < MSECS_PER_DAY);
    // Only the later parts of the very first day are representable - its start
    // would overflow - so get ds the same side of 0 as days:
    if (days < 0 && ds > 0) {
        days++;
        ds -= MSECS_PER_DAY;
    }

    // Check in representable range:
    qint64 msecs = 0;
    if (mul_overflow(days, std::integral_constant<qint64, MSECS_PER_DAY>(), &msecs)
        || add_overflow(msecs, qint64(ds), &msecs)) {
        newStatus = QDateTimePrivate::StatusFlags{};
        msecs = 0;
    }
    if (d.isShort()) {
        // let's see if we can keep this short
        if (msecsCanBeSmall(msecs)) {
            // yes, we can
            d.data.msecs = qintptr(msecs);
            d.data.status &= ~(QDateTimePrivate::ValidityMask | QDateTimePrivate::DaylightMask).toInt();
            d.data.status |= newStatus.toInt();
        } else {
            // nope...
            d.detach();
        }
    }
    if (!d.isShort()) {
        d.detach();
        d->m_msecs = msecs;
        d->m_status &= ~(QDateTimePrivate::ValidityMask | QDateTimePrivate::DaylightMask);
        d->m_status |= newStatus;
    }
}

static QPair<QDate, QTime> getDateTime(const QDateTimeData &d)
{
    QPair<QDate, QTime> result;
    qint64 msecs = getMSecs(d);
    auto status = getStatus(d);
    msecsToTime(msecs, &result.first, &result.second);

    if (!status.testFlag(QDateTimePrivate::ValidDate))
        result.first = QDate();

    if (!status.testFlag(QDateTimePrivate::ValidTime))
        result.second = QTime();

    return result;
}

/*****************************************************************************
  QDateTime::Data member functions
 *****************************************************************************/

inline QDateTime::Data::Data() noexcept
{
    // default-constructed data has a special exception:
    // it can be small even if CanBeSmall == false
    // (optimization so we don't allocate memory in the default constructor)
    quintptr value = mergeSpec(QDateTimePrivate::ShortData, Qt::LocalTime).toInt();
    d = reinterpret_cast<QDateTimePrivate *>(value);
}

inline QDateTime::Data::Data(Qt::TimeSpec spec)
{
    if (CanBeSmall && Q_LIKELY(specCanBeSmall(spec))) {
        d = reinterpret_cast<QDateTimePrivate *>(quintptr(mergeSpec(QDateTimePrivate::ShortData, spec).toInt()));
    } else {
        // the structure is too small, we need to detach
        d = new QDateTimePrivate;
        d->ref.ref();
        d->m_status = mergeSpec({}, spec);
    }
}

inline QDateTime::Data::Data(const Data &other)
    : d(other.d)
{
    if (!isShort()) {
        // check if we could shrink
        if (specCanBeSmall(extractSpec(d->m_status)) && msecsCanBeSmall(d->m_msecs)) {
            ShortData sd;
            sd.msecs = qintptr(d->m_msecs);
            sd.status = (d->m_status | QDateTimePrivate::ShortData).toInt();
            data = sd;
        } else {
            // no, have to keep it big
            d->ref.ref();
        }
    }
}

inline QDateTime::Data::Data(Data &&other)
    : d(other.d)
{
    // reset the other to a short state
    Data dummy;
    Q_ASSERT(dummy.isShort());
    other.d = dummy.d;
}

inline QDateTime::Data &QDateTime::Data::operator=(const Data &other)
{
    if (d == other.d)
        return *this;

    auto x = d;
    d = other.d;
    if (!other.isShort()) {
        // check if we could shrink
        if (specCanBeSmall(extractSpec(other.d->m_status)) && msecsCanBeSmall(other.d->m_msecs)) {
            ShortData sd;
            sd.msecs = qintptr(other.d->m_msecs);
            sd.status = (other.d->m_status | QDateTimePrivate::ShortData).toInt();
            data = sd;
        } else {
            // no, have to keep it big
            other.d->ref.ref();
        }
    }

    if (!(quintptr(x) & QDateTimePrivate::ShortData) && !x->ref.deref())
        delete x;
    return *this;
}

inline QDateTime::Data::~Data()
{
    if (!isShort() && !d->ref.deref())
        delete d;
}

inline bool QDateTime::Data::isShort() const
{
    bool b = quintptr(d) & QDateTimePrivate::ShortData;

    // sanity check:
    Q_ASSERT(b || (d->m_status & QDateTimePrivate::ShortData) == 0);

    // even if CanBeSmall = false, we have short data for a default-constructed
    // QDateTime object. But it's unlikely.
    if (CanBeSmall)
        return Q_LIKELY(b);
    return Q_UNLIKELY(b);
}

inline void QDateTime::Data::detach()
{
    QDateTimePrivate *x;
    bool wasShort = isShort();
    if (wasShort) {
        // force enlarging
        x = new QDateTimePrivate;
        x->m_status = QDateTimePrivate::StatusFlags::fromInt(data.status) & ~QDateTimePrivate::ShortData;
        x->m_msecs = data.msecs;
    } else {
        if (d->ref.loadRelaxed() == 1)
            return;

        x = new QDateTimePrivate(*d);
    }

    x->ref.storeRelaxed(1);
    if (!wasShort && !d->ref.deref())
        delete d;
    d = x;
}

inline const QDateTimePrivate *QDateTime::Data::operator->() const
{
    Q_ASSERT(!isShort());
    return d;
}

inline QDateTimePrivate *QDateTime::Data::operator->()
{
    // should we attempt to detach here?
    Q_ASSERT(!isShort());
    Q_ASSERT(d->ref.loadRelaxed() == 1);
    return d;
}

/*****************************************************************************
  QDateTimePrivate member functions
 *****************************************************************************/

Q_NEVER_INLINE
QDateTime::Data QDateTimePrivate::create(QDate toDate, QTime toTime, Qt::TimeSpec toSpec,
                                         int offsetSeconds)
{
    QDateTime::Data result(toSpec);
    setTimeSpec(result, toSpec, offsetSeconds);
    setDateTime(result, toDate, toTime);
    if (toSpec == Qt::OffsetFromUTC || toSpec == Qt::UTC)
        refreshSimpleDateTime(result);
    else
        refreshZonedDateTime(result, Qt::LocalTime);
    return result;
}

#if QT_CONFIG(timezone)
inline QDateTime::Data QDateTimePrivate::create(QDate toDate, QTime toTime,
                                                const QTimeZone &toTimeZone)
{
    QDateTime::Data result(Qt::TimeZone);
    Q_ASSERT(!result.isShort());

    result.d->m_status = mergeSpec(result.d->m_status, Qt::TimeZone);
    result.d->m_timeZone = toTimeZone;
    setDateTime(result, toDate, toTime);
    refreshZonedDateTime(result, Qt::TimeZone);
    return result;
}

// Convert a TimeZone time expressed in zone msecs encoding into a UTC epoch msecs
// DST transitions are disambiguated by hint.
inline qint64 QDateTimePrivate::zoneMSecsToEpochMSecs(qint64 zoneMSecs, const QTimeZone &zone,
                                                      DaylightStatus *hint,
                                                      QDate *zoneDate, QTime *zoneTime,
                                                      QString *abbreviation)
{
    Q_ASSERT(zone.isValid());
    // Get the effective data from QTimeZone
    DaylightStatus dst = hint ? *hint : UnknownDaylightTime;
    QTimeZonePrivate::Data data = zone.d->dataForLocalTime(zoneMSecs, int(dst));
    if (data.offsetFromUtc == QTimeZonePrivate::invalidSeconds()) {
        if (hint)
            *hint = QDateTimePrivate::UnknownDaylightTime;
        if (abbreviation)
            *abbreviation = QString();
        if (zoneDate)
            *zoneDate = QDate();
        if (zoneTime)
            *zoneTime = QTime();
    } else {
        Q_ASSERT(zone.d->offsetFromUtc(data.atMSecsSinceEpoch) == data.offsetFromUtc);
        msecsToTime(data.atMSecsSinceEpoch + data.offsetFromUtc * MSECS_PER_SEC,
                    zoneDate, zoneTime);
        if (hint) {
            *hint = data.daylightTimeOffset
                ? QDateTimePrivate::DaylightTime
                : QDateTimePrivate::StandardTime;
        }
        if (abbreviation)
            *abbreviation = data.abbreviation;
    }
    return data.atMSecsSinceEpoch;
}
#endif // timezone

/*****************************************************************************
  QDateTime member functions
 *****************************************************************************/

/*!
    \class QDateTime
    \inmodule QtCore
    \ingroup shared
    \reentrant
    \brief The QDateTime class provides date and time functions.


    A QDateTime object encodes a calendar date and a clock time (a
    "datetime"). It combines features of the QDate and QTime classes.
    It can read the current datetime from the system clock. It
    provides functions for comparing datetimes and for manipulating a
    datetime by adding a number of seconds, days, months, or years.

    QDateTime can describe datetimes with respect to \l{Qt::LocalTime}{local
    time}, to \l{Qt::UTC}{UTC}, to a specified \l{Qt::OffsetFromUTC}{offset from
    UTC} or to a specified \l{Qt::TimeZone}{time zone}, in conjunction with the
    QTimeZone class. For example, a time zone of "Europe/Berlin" will apply the
    daylight-saving rules as used in Germany. In contrast, an offset from UTC of
    +3600 seconds is one hour ahead of UTC (usually written in ISO standard
    notation as "UTC+01:00"), with no daylight-saving offset or changes. When
    using either local time or a specified time zone, time-zone transitions such
    as the starts and ends of daylight-saving time (DST; but see below) are
    taken into account. The choice of system used to represent a datetime is
    described as its "timespec".

    A QDateTime object is typically created either by giving a date and time
    explicitly in the constructor, or by using a static function such as
    currentDateTime() or fromMSecsSinceEpoch(). The date and time can be changed
    with setDate() and setTime(). A datetime can also be set using the
    setMSecsSinceEpoch() function that takes the time, in milliseconds, since
    00:00:00 on January 1, 1970. The fromString() function returns a QDateTime,
    given a string and a date format used to interpret the date within the
    string.

    QDateTime::currentDateTime() returns a QDateTime that expresses the current
    time with respect to local time. QDateTime::currentDateTimeUtc() returns a
    QDateTime that expresses the current time with respect to UTC.

    The date() and time() functions provide access to the date and
    time parts of the datetime. The same information is provided in
    textual format by the toString() function.

    QDateTime provides a full set of operators to compare two
    QDateTime objects, where smaller means earlier and larger means
    later.

    You can increment (or decrement) a datetime by a given number of
    milliseconds using addMSecs(), seconds using addSecs(), or days using
    addDays(). Similarly, you can use addMonths() and addYears(). The daysTo()
    function returns the number of days between two datetimes, secsTo() returns
    the number of seconds between two datetimes, and msecsTo() returns the
    number of milliseconds between two datetimes. These operations are aware of
    daylight-saving time (DST) and other time-zone transitions, where
    applicable.

    Use toTimeSpec() to express a datetime in local time or UTC,
    toOffsetFromUtc() to express in terms of an offset from UTC, or toTimeZone()
    to express it with respect to a general time zone. You can use timeSpec() to
    find out what time-spec a QDateTime object stores its time relative to. When
    that is Qt::TimeZone, you can use timeZone() to find out which zone it is
    using.

    \note QDateTime does not account for leap seconds.

    \section1 Remarks

    \note All conversion to and from string formats is done using the C locale.
    For localized conversions, see QLocale.

    \note There is no year 0 in the Gregorian calendar. Dates in that year are
    considered invalid. The year -1 is the year "1 before Christ" or "1 before
    common era." The day before 1 January 1 CE is 31 December 1 BCE.

    \section2 Range of Valid Dates

    The range of values that QDateTime can represent is dependent on the
    internal storage implementation. QDateTime is currently stored in a qint64
    as a serial msecs value encoding the date and time. This restricts the date
    range to about +/- 292 million years, compared to the QDate range of +/- 2
    billion years. Care must be taken when creating a QDateTime with extreme
    values that you do not overflow the storage. The exact range of supported
    values varies depending on the Qt::TimeSpec and time zone.

    \section2 Use of Timezones

    QDateTime uses the system's time zone information to determine the current
    local time zone and its offset from UTC. If the system is not configured
    correctly or not up-to-date, QDateTime will give wrong results.

    QDateTime likewise uses system-provided information to determine the offsets
    of other timezones from UTC. If this information is incomplete or out of
    date, QDateTime will give wrong results. See the QTimeZone documentation for
    more details.

    On modern Unix systems, this means QDateTime usually has accurate
    information about historical transitions (including DST, see below) whenever
    possible. On Windows, where the system doesn't support historical timezone
    data, historical accuracy is not maintained with respect to timezone
    transitions, notably including DST.

    \section2 Daylight-Saving Time (DST)

    QDateTime takes into account transitions between Standard Time and
    Daylight-Saving Time. For example, if the transition is at 2am and the clock
    goes forward to 3am, then there is a "missing" hour from 02:00:00 to
    02:59:59.999 which QDateTime considers to be invalid. Any date arithmetic
    performed will take this missing hour into account and return a valid
    result. For example, adding one minute to 01:59:59 will get 03:00:00.

    For date-times that the system \c time_t can represent (from 1901-12-14 to
    2038-01-18 on systems with 32-bit \c time_t; for the full range QDateTime
    can represent if the type is 64-bit), the standard system APIs are used to
    determine local time's offset from UTC. For date-times not handled by these
    system APIs, QTimeZone::systemTimeZone() is used. In either case, the offset
    information used depends on the system and may be incomplete or, for past
    times, historically inaccurate. In any case, for future dates, the local
    time zone's offsets and DST rules may change before that date comes around.

    \section2 Offsets From UTC

    There is no explicit size restriction on an offset from UTC, but there is an
    implicit limit imposed when using the toString() and fromString() methods
    which use a [+|-]hh:mm format, effectively limiting the range to +/- 99
    hours and 59 minutes and whole minutes only. Note that currently no time
    zone has an offset outside the range of ±14 hours and all known offsets are
    multiples of five minutes.

    \sa QDate, QTime, QDateTimeEdit, QTimeZone
*/

/*!
    \since 5.14
    \enum QDateTime::YearRange

    This enumerated type describes the range of years (in the Gregorian
    calendar) representable by QDateTime:

    \value First The later parts of this year are representable
    \value Last The earlier parts of this year are representable

    All dates strictly between these two years are also representable.
    Note, however, that the Gregorian Calendar has no year zero.

    \note QDate can describe dates in a wider range of years.  For most
    purposes, this makes little difference, as the range of years that QDateTime
    can support reaches 292 million years either side of 1970.

    \sa isValid(), QDate
*/

/*!
    Constructs a null datetime.

    A null datetime is invalid, since its date and time are invalid.

    \sa isValid()
*/
QDateTime::QDateTime() noexcept
{
#if QT_VERSION >= QT_VERSION_CHECK(7,0,0) || QT_POINTER_SIZE == 8
    static_assert(sizeof(ShortData) == sizeof(qint64));
    static_assert(sizeof(Data) == sizeof(qint64));
#endif
}

/*!
    Constructs a datetime with the given \a date and \a time, using
    the time specification defined by \a spec and \a offsetSeconds seconds.

    If \a date is valid and \a time is not, the time will be set to midnight.

    If the \a spec is not Qt::OffsetFromUTC then \a offsetSeconds will be ignored.

    If the \a spec is Qt::OffsetFromUTC and \a offsetSeconds is 0 then the
    timeSpec() will be set to Qt::UTC, i.e. an offset of 0 seconds.

    If \a spec is Qt::TimeZone then the spec will be set to Qt::LocalTime,
    i.e. the current system time zone.  To create a Qt::TimeZone datetime
    use the correct constructor.
*/

QDateTime::QDateTime(QDate date, QTime time, Qt::TimeSpec spec, int offsetSeconds)
         : d(QDateTimePrivate::create(date, time, spec, offsetSeconds))
{
}

#if QT_CONFIG(timezone)
/*!
    \since 5.2

    Constructs a datetime with the given \a date and \a time, using
    the Time Zone specified by \a timeZone.

    If \a date is valid and \a time is not, the time will be set to 00:00:00.

    If \a timeZone is invalid then the datetime will be invalid.
*/

QDateTime::QDateTime(QDate date, QTime time, const QTimeZone &timeZone)
    : d(QDateTimePrivate::create(date, time, timeZone))
{
}
#endif // timezone

/*!
    Constructs a copy of the \a other datetime.
*/
QDateTime::QDateTime(const QDateTime &other) noexcept
    : d(other.d)
{
}

/*!
    \since 5.8
    Moves the content of the temporary \a other datetime to this object and
    leaves \a other in an unspecified (but proper) state.
*/
QDateTime::QDateTime(QDateTime &&other) noexcept
    : d(std::move(other.d))
{
}

/*!
    Destroys the datetime.
*/
QDateTime::~QDateTime()
{
}

/*!
    Makes a copy of the \a other datetime and returns a reference to the
    copy.
*/

QDateTime &QDateTime::operator=(const QDateTime &other) noexcept
{
    d = other.d;
    return *this;
}
/*!
    \fn void QDateTime::swap(QDateTime &other)
    \since 5.0

    Swaps this datetime with \a other. This operation is very fast
    and never fails.
*/

/*!
    Returns \c true if both the date and the time are null; otherwise
    returns \c false. A null datetime is invalid.

    \sa QDate::isNull(), QTime::isNull(), isValid()
*/

bool QDateTime::isNull() const
{
    auto status = getStatus(d);
    return !status.testFlag(QDateTimePrivate::ValidDate) &&
            !status.testFlag(QDateTimePrivate::ValidTime);
}

/*!
    Returns \c true if both the date and the time are valid and they are valid in
    the current Qt::TimeSpec, otherwise returns \c false.

    If the timeSpec() is Qt::LocalTime or Qt::TimeZone and this object
    represents a time that was skipped by a forward transition, then it is
    invalid.  For example, if DST ends at 2am with the clock advancing to 3am,
    then date-times from 02:00:00 to 02:59:59.999 on that day are considered
    invalid.

    \sa QDateTime::YearRange, QDate::isValid(), QTime::isValid()
*/

bool QDateTime::isValid() const
{
    auto status = getStatus(d);
    return status.testAnyFlag(QDateTimePrivate::ValidDateTime);
}

/*!
    Returns the date part of the datetime.

    \sa setDate(), time(), timeSpec()
*/

QDate QDateTime::date() const
{
    auto status = getStatus(d);
    if (!status.testFlag(QDateTimePrivate::ValidDate))
        return QDate();
    QDate dt;
    msecsToTime(getMSecs(d), &dt, nullptr);
    return dt;
}

/*!
    Returns the time part of the datetime.

    \sa setTime(), date(), timeSpec()
*/

QTime QDateTime::time() const
{
    auto status = getStatus(d);
    if (!status.testFlag(QDateTimePrivate::ValidTime))
        return QTime();
    QTime tm;
    msecsToTime(getMSecs(d), nullptr, &tm);
    return tm;
}

/*!
    Returns the time specification of the datetime.

    \sa setTimeSpec(), date(), time(), Qt::TimeSpec
*/

Qt::TimeSpec QDateTime::timeSpec() const
{
    return getSpec(d);
}

#if QT_CONFIG(timezone)
/*!
    \since 5.2

    Returns the time zone of the datetime.

    If the timeSpec() is Qt::LocalTime then an instance of the current system
    time zone will be returned. Note however that if you copy this time zone
    the instance will not remain in sync if the system time zone changes.

    \sa setTimeZone(), Qt::TimeSpec
*/

QTimeZone QDateTime::timeZone() const
{
    switch (getSpec(d)) {
    case Qt::UTC:
        return QTimeZone::utc();
    case Qt::OffsetFromUTC:
        return QTimeZone(d->m_offsetFromUtc);
    case Qt::TimeZone:
        if (d->m_timeZone.isValid())
            return d->m_timeZone;
        break;
    case Qt::LocalTime:
        return QTimeZone::systemTimeZone();
    }
    return QTimeZone();
}
#endif // timezone

/*!
    \since 5.2

    Returns this date-time's Offset From UTC in seconds.

    The result depends on timeSpec():
    \list
    \li \c Qt::UTC The offset is 0.
    \li \c Qt::OffsetFromUTC The offset is the value originally set.
    \li \c Qt::LocalTime The local time's offset from UTC is returned.
    \li \c Qt::TimeZone The offset used by the time-zone is returned.
    \endlist

    For the last two, the offset at this date and time will be returned, taking
    account of Daylight-Saving Offset. The offset is the difference between the
    local time or time in the given time-zone and UTC time; it is positive in
    time-zones ahead of UTC (East of The Prime Meridian), negative for those
    behind UTC (West of The Prime Meridian).

    \sa setOffsetFromUtc()
*/

int QDateTime::offsetFromUtc() const
{
    if (!d.isShort())
        return d->m_offsetFromUtc;
    if (!isValid())
        return 0;

    auto spec = getSpec(d);
    if (spec == Qt::LocalTime) {
        // we didn't cache the value, so we need to calculate it now...
        qint64 msecs = getMSecs(d);
        return (msecs - toMSecsSinceEpoch()) / MSECS_PER_SEC;
    }

    Q_ASSERT(spec == Qt::UTC);
    return 0;
}

/*!
    \since 5.2

    Returns the Time Zone Abbreviation for this datetime.

    The returned string depends on timeSpec():

    \list
    \li For Qt::UTC it is "UTC".
    \li For Qt::OffsetFromUTC it will be in the format "UTC[+-]00:00".
    \li For Qt::LocalTime, the host system is queried.
    \li For Qt::TimeZone, the associated QTimeZone object is queried.
    \endlist

    \note The abbreviation is not guaranteed to be unique, i.e. different time
    zones may have the same abbreviation. For Qt::LocalTime and Qt::TimeZone,
    when returned by the host system, the abbreviation may be localized.

    \sa timeSpec(), QTimeZone::abbreviation()
*/

QString QDateTime::timeZoneAbbreviation() const
{
    if (!isValid())
        return QString();

    switch (getSpec(d)) {
    case Qt::UTC:
        return QLatin1String("UTC");
    case Qt::OffsetFromUTC:
        return QLatin1String("UTC") + toOffsetString(Qt::ISODate, d->m_offsetFromUtc);
    case Qt::TimeZone:
#if !QT_CONFIG(timezone)
        break;
#else
        Q_ASSERT(d->m_timeZone.isValid());
        return d->m_timeZone.abbreviation(*this);
#endif // timezone
    case Qt::LocalTime:  {
        QString abbrev;
        auto status = extractDaylightStatus(getStatus(d));
        QDateTimePrivate::localMSecsToEpochMSecs(getMSecs(d), &status, nullptr, nullptr, &abbrev);
        return abbrev;
        }
    }
    return QString();
}

/*!
    \since 5.2

    Returns if this datetime falls in Daylight-Saving Time.

    If the Qt::TimeSpec is not Qt::LocalTime or Qt::TimeZone then will always
    return false.

    \sa timeSpec()
*/

bool QDateTime::isDaylightTime() const
{
    if (!isValid())
        return false;

    switch (getSpec(d)) {
    case Qt::UTC:
    case Qt::OffsetFromUTC:
        return false;
    case Qt::TimeZone:
#if !QT_CONFIG(timezone)
        break;
#else
        Q_ASSERT(d->m_timeZone.isValid());
        return d->m_timeZone.d->isDaylightTime(toMSecsSinceEpoch());
#endif // timezone
    case Qt::LocalTime: {
        auto status = extractDaylightStatus(getStatus(d));
        if (status == QDateTimePrivate::UnknownDaylightTime)
            QDateTimePrivate::localMSecsToEpochMSecs(getMSecs(d), &status);
        return (status == QDateTimePrivate::DaylightTime);
        }
    }
    return false;
}

/*!
    Sets the date part of this datetime to \a date. If no time is set yet, it
    is set to midnight. If \a date is invalid, this QDateTime becomes invalid.

    \sa date(), setTime(), setTimeSpec()
*/

void QDateTime::setDate(QDate date)
{
    setDateTime(d, date, time());
    checkValidDateTime(d);
}

/*!
    Sets the time part of this datetime to \a time. If \a time is not valid,
    this function sets it to midnight. Therefore, it's possible to clear any
    set time in a QDateTime by setting it to a default QTime:

    \code
        QDateTime dt = QDateTime::currentDateTime();
        dt.setTime(QTime());
    \endcode

    \sa time(), setDate(), setTimeSpec()
*/

void QDateTime::setTime(QTime time)
{
    setDateTime(d, date(), time);
    checkValidDateTime(d);
}

/*!
    Sets the time specification used in this datetime to \a spec.
    The datetime will refer to a different point in time.

    If \a spec is Qt::OffsetFromUTC then the timeSpec() will be set
    to Qt::UTC, i.e. an effective offset of 0.

    If \a spec is Qt::TimeZone then the spec will be set to Qt::LocalTime,
    i.e. the current system time zone.

    Example:
    \snippet code/src_corelib_time_qdatetime.cpp 19

    \sa timeSpec(), setDate(), setTime(), setTimeZone(), Qt::TimeSpec
*/

void QDateTime::setTimeSpec(Qt::TimeSpec spec)
{
    QT_PREPEND_NAMESPACE(setTimeSpec(d, spec, 0));
    if (spec == Qt::OffsetFromUTC || spec == Qt::UTC)
        refreshSimpleDateTime(d);
    else
        refreshZonedDateTime(d, Qt::LocalTime);
}

/*!
    \since 5.2

    Sets the timeSpec() to Qt::OffsetFromUTC and the offset to \a offsetSeconds.
    The datetime will refer to a different point in time.

    The maximum and minimum offset is 14 positive or negative hours.  If
    \a offsetSeconds is larger or smaller than that, then the result is
    undefined.

    If \a offsetSeconds is 0 then the timeSpec() will be set to Qt::UTC.

    \sa isValid(), offsetFromUtc()
*/

void QDateTime::setOffsetFromUtc(int offsetSeconds)
{
    QT_PREPEND_NAMESPACE(setTimeSpec(d, Qt::OffsetFromUTC, offsetSeconds));
    refreshSimpleDateTime(d);
}

#if QT_CONFIG(timezone)
/*!
    \since 5.2

    Sets the time zone used in this datetime to \a toZone.
    The datetime will refer to a different point in time.

    If \a toZone is invalid then the datetime will be invalid.

    \sa timeZone(), Qt::TimeSpec
*/

void QDateTime::setTimeZone(const QTimeZone &toZone)
{
    d.detach();         // always detach
    d->m_status = mergeSpec(d->m_status, Qt::TimeZone);
    d->m_offsetFromUtc = 0;
    d->m_timeZone = toZone;
    refreshZonedDateTime(d, Qt::TimeZone);
}
#endif // timezone

/*!
    \since 4.7

    Returns the datetime as the number of milliseconds that have passed
    since 1970-01-01T00:00:00.000, Coordinated Universal Time (Qt::UTC).

    On systems that do not support time zones, this function will
    behave as if local time were Qt::UTC.

    The behavior for this function is undefined if the datetime stored in
    this object is not valid. However, for all valid dates, this function
    returns a unique value.

    \sa toSecsSinceEpoch(), setMSecsSinceEpoch()
*/
qint64 QDateTime::toMSecsSinceEpoch() const
{
    // Note: QDateTimeParser relies on this producing a useful result, even when
    // !isValid(), at least when the invalidity is a time in a fall-back (that
    // we'll have adjusted to lie outside it, but marked invalid because it's
    // not what was asked for). Other things may be doing similar.
    switch (getSpec(d)) {
    case Qt::UTC:
        return getMSecs(d);

    case Qt::OffsetFromUTC:
        Q_ASSERT(!d.isShort());
        return d->m_msecs - d->m_offsetFromUtc * MSECS_PER_SEC;

    case Qt::LocalTime: {
        // recalculate the local timezone
        auto status = extractDaylightStatus(getStatus(d));
        // If short, use offset saved by refreshZonedDateTime() on creation:
        if (!d.isShort())
            return d->m_msecs - d->m_offsetFromUtc * MSECS_PER_SEC;
        // Offset from UTC not recorded: need to recompute.
        return QDateTimePrivate::localMSecsToEpochMSecs(getMSecs(d), &status);
    }

    case Qt::TimeZone:
        Q_ASSERT(!d.isShort());
#if QT_CONFIG(timezone)
        // Use offset refreshZonedDateTime() saved on creation:
        if (d->m_timeZone.isValid())
            return d->m_msecs - d->m_offsetFromUtc * MSECS_PER_SEC;
#endif
        return 0;
    }
    Q_UNREACHABLE();
    return 0;
}

/*!
    \since 5.8

    Returns the datetime as the number of seconds that have passed since
    1970-01-01T00:00:00.000, Coordinated Universal Time (Qt::UTC).

    On systems that do not support time zones, this function will
    behave as if local time were Qt::UTC.

    The behavior for this function is undefined if the datetime stored in
    this object is not valid. However, for all valid dates, this function
    returns a unique value.

    \sa toMSecsSinceEpoch(), setSecsSinceEpoch()
*/
qint64 QDateTime::toSecsSinceEpoch() const
{
    return toMSecsSinceEpoch() / MSECS_PER_SEC;
}

/*!
    \since 4.7

    Sets the date and time given the number of milliseconds \a msecs that have
    passed since 1970-01-01T00:00:00.000, Coordinated Universal Time
    (Qt::UTC). On systems that do not support time zones this function
    will behave as if local time were Qt::UTC.

    Note that passing the minimum of \c qint64
    (\c{std::numeric_limits<qint64>::min()}) to \a msecs will result in
    undefined behavior.

    \sa toMSecsSinceEpoch(), setSecsSinceEpoch()
*/
void QDateTime::setMSecsSinceEpoch(qint64 msecs)
{
    auto status = getStatus(d);
    const auto spec = extractSpec(status);

    status &= ~QDateTimePrivate::ValidityMask;
    switch (spec) {
    case Qt::UTC:
        status |= QDateTimePrivate::ValidWhenMask;
        break;
    case Qt::OffsetFromUTC:
        if (!add_overflow(msecs, d->m_offsetFromUtc * MSECS_PER_SEC, &msecs))
            status |= QDateTimePrivate::ValidWhenMask;
        break;
    case Qt::TimeZone:
        Q_ASSERT(!d.isShort());
#if QT_CONFIG(timezone)
        d.detach();
        if (!d->m_timeZone.isValid())
            break;
        status = mergeDaylightStatus(status,
                                     d->m_timeZone.d->isDaylightTime(msecs)
                                     ? QDateTimePrivate::DaylightTime
                                     : QDateTimePrivate::StandardTime);
        d->m_offsetFromUtc = d->m_timeZone.d->offsetFromUtc(msecs);
        // NB: cast to qint64 here is important to make sure a matching
        // add_overflow is found, GCC 7.5.0 fails without this cast
        if (!add_overflow(msecs, qint64(d->m_offsetFromUtc * MSECS_PER_SEC), &msecs))
            status |= QDateTimePrivate::ValidWhenMask;
#endif // timezone
        break;
    case Qt::LocalTime: {
        QDate dt;
        QTime tm;
        QDateTimePrivate::DaylightStatus dstStatus;
        if (QDateTimePrivate::epochMSecsToLocalTime(msecs, &dt, &tm, &dstStatus)) {
            setDateTime(d, dt, tm);
            status = getStatus(d);
        }
        if ((status & QDateTimePrivate::ValidDate) && (status & QDateTimePrivate::ValidTime)) {
            refreshZonedDateTime(d, spec); // FIXME: we do this again, below
            msecs = getMSecs(d);
            status = mergeDaylightStatus(getStatus(d), dstStatus);
        }
        break;
        }
    }

    if (msecsCanBeSmall(msecs) && d.isShort()) {
        // we can keep short
        d.data.msecs = qintptr(msecs);
        d.data.status = status.toInt();
    } else {
        d.detach();
        d->m_status = status & ~QDateTimePrivate::ShortData;
        d->m_msecs = msecs;
    }

    if (spec == Qt::LocalTime || spec == Qt::TimeZone) {
        refreshZonedDateTime(d, spec);
        Q_ASSERT((d.isShort() ? d.data.msecs : d->m_msecs) == msecs);
    }
}

/*!
    \since 5.8

    Sets the date and time given the number of seconds \a secs that have
    passed since 1970-01-01T00:00:00.000, Coordinated Universal Time
    (Qt::UTC). On systems that do not support time zones this function
    will behave as if local time were Qt::UTC.

    \sa toSecsSinceEpoch(), setMSecsSinceEpoch()
*/
void QDateTime::setSecsSinceEpoch(qint64 secs)
{
    qint64 msecs;
    if (!mul_overflow(secs, std::integral_constant<qint64, MSECS_PER_SEC>(), &msecs)) {
        setMSecsSinceEpoch(msecs);
    } else if (d.isShort()) {
        d.data.status &= ~int(QDateTimePrivate::ValidWhenMask);
    } else {
        d.detach();
        d->m_status &= ~QDateTimePrivate::ValidWhenMask;
    }
}

#if QT_CONFIG(datestring) // depends on, so implies, textdate
/*!
    \overload

    Returns the datetime as a string in the \a format given.

    If the \a format is Qt::TextDate, the string is formatted in the default
    way. The day and month names will be in English. An example of this
    formatting is "Wed May 20 03:40:13 1998". For localized formatting, see
    \l{QLocale::toString()}.

    If the \a format is Qt::ISODate, the string format corresponds
    to the ISO 8601 extended specification for representations of
    dates and times, taking the form yyyy-MM-ddTHH:mm:ss[Z|[+|-]HH:mm],
    depending on the timeSpec() of the QDateTime. If the timeSpec()
    is Qt::UTC, Z will be appended to the string; if the timeSpec() is
    Qt::OffsetFromUTC, the offset in hours and minutes from UTC will
    be appended to the string. To include milliseconds in the ISO 8601
    date, use the \a format Qt::ISODateWithMs, which corresponds to
    yyyy-MM-ddTHH:mm:ss.zzz[Z|[+|-]HH:mm].

    If the \a format is Qt::RFC2822Date, the string is formatted
    following \l{RFC 2822}.

    If the datetime is invalid, an empty string will be returned.

    \warning The Qt::ISODate format is only valid for years in the
    range 0 to 9999.

    \sa fromString(), QDate::toString(), QTime::toString(),
    QLocale::toString()
*/
QString QDateTime::toString(Qt::DateFormat format) const
{
    QString buf;
    if (!isValid())
        return buf;

    switch (format) {
    case Qt::RFC2822Date:
        buf = QLocale::c().toString(*this, u"dd MMM yyyy hh:mm:ss ");
        buf += toOffsetString(Qt::TextDate, offsetFromUtc());
        return buf;
    default:
    case Qt::TextDate: {
        const QPair<QDate, QTime> p = getDateTime(d);
        buf = toStringTextDate(p.first);
        // Insert time between date's day and year:
        buf.insert(buf.lastIndexOf(u' '),
                   u' ' + p.second.toString(Qt::TextDate));
        // Append zone/offset indicator, as appropriate:
        switch (timeSpec()) {
        case Qt::LocalTime:
            break;
#if QT_CONFIG(timezone)
        case Qt::TimeZone:
            buf += u' ' + d->m_timeZone.displayName(
                *this, QTimeZone::OffsetName, QLocale::c());
            break;
#endif
        default:
#if 0 // ### Qt 7 GMT: use UTC instead, see qnamespace.qdoc documentation
            buf += QLatin1String(" UTC");
#else
            buf += QLatin1String(" GMT");
#endif
            if (getSpec(d) == Qt::OffsetFromUTC)
                buf += toOffsetString(Qt::TextDate, offsetFromUtc());
        }
        return buf;
    }
    case Qt::ISODate:
    case Qt::ISODateWithMs: {
        const QPair<QDate, QTime> p = getDateTime(d);
        buf = toStringIsoDate(p.first);
        if (buf.isEmpty())
            return QString();   // failed to convert
        buf += u'T' + p.second.toString(format);
        switch (getSpec(d)) {
        case Qt::UTC:
            buf += u'Z';
            break;
        case Qt::OffsetFromUTC:
#if QT_CONFIG(timezone)
        case Qt::TimeZone:
#endif
            buf += toOffsetString(Qt::ISODate, offsetFromUtc());
            break;
        default:
            break;
        }
        return buf;
    }
    }
}

/*!
    \fn QString QDateTime::toString(const QString &format, QCalendar cal) const
    \fn QString QDateTime::toString(QStringView format, QCalendar cal) const

    Returns the datetime as a string. The \a format parameter determines the
    format of the result string. If \a cal is supplied, it determines the calendar
    used to represent the date; it defaults to Gregorian. See QTime::toString()
    and QDate::toString() for the supported specifiers for time and date,
    respectively.

    Any sequence of characters enclosed in single quotes will be included
    verbatim in the output string (stripped of the quotes), even if it contains
    formatting characters. Two consecutive single quotes ("''") are replaced by
    a single quote in the output. All other characters in the format string are
    included verbatim in the output string.

    Formats without separators (e.g. "ddMM") are supported but must be used with
    care, as the resulting strings aren't always reliably readable (e.g. if "dM"
    produces "212" it could mean either the 2nd of December or the 21st of
    February).

    Example format strings (assumed that the QDateTime is 21 May 2001
    14:13:09.120):

    \table
    \header \li Format       \li Result
    \row \li dd.MM.yyyy      \li 21.05.2001
    \row \li ddd MMMM d yy   \li Tue May 21 01
    \row \li hh:mm:ss.zzz    \li 14:13:09.120
    \row \li hh:mm:ss.z      \li 14:13:09.12
    \row \li h:m:s ap        \li 2:13:9 pm
    \endtable

    If the datetime is invalid, an empty string will be returned.

    \note Day and month names as well as AM/PM indication are given in English (C locale).
    If localized month and day names and localized forms of AM/PM are used, use
    QLocale::system().toDateTime().

    \sa fromString(), QDate::toString(), QTime::toString(), QLocale::toString()
*/
QString QDateTime::toString(QStringView format, QCalendar cal) const
{
    return QLocale::c().toString(*this, format, cal);
}
#endif // datestring

static inline void massageAdjustedDateTime(QDateTimeData &d, QDate date, QTime time)
{
    /*
      If we have just adjusted to a day with a DST transition, our given time
      may lie in the transition hour (either missing or duplicated).  For any
      other time, telling mktime (deep in the bowels of localMSecsToEpochMSecs)
      we don't know its DST-ness will produce no adjustment (just a decision as
      to its DST-ness); but for a time in spring's missing hour it'll adjust the
      time while picking a DST-ness.  (Handling of autumn is trickier, as either
      DST-ness is valid, without adjusting the time.  We might want to propagate
      the daylight status in that case, but it's hard to do so without breaking
      (far more common) other cases; and it makes little difference, as the two
      answers do then differ only in DST-ness.)
    */
    auto spec = getSpec(d);
    if (spec == Qt::LocalTime) {
        QDateTimePrivate::DaylightStatus status = QDateTimePrivate::UnknownDaylightTime;
        QDateTimePrivate::localMSecsToEpochMSecs(timeToMSecs(date, time), &status, &date, &time);
#if QT_CONFIG(timezone)
    } else if (spec == Qt::TimeZone && d.d->m_timeZone.isValid()) {
        QDateTimePrivate::DaylightStatus status = QDateTimePrivate::UnknownDaylightTime;
        QDateTimePrivate::zoneMSecsToEpochMSecs(timeToMSecs(date, time),
                                                d.d->m_timeZone, &status, &date, &time);
#endif // timezone
    }
    setDateTime(d, date, time);
    checkValidDateTime(d);
}

/*!
    Returns a QDateTime object containing a datetime \a ndays days
    later than the datetime of this object (or earlier if \a ndays is
    negative).

    If the timeSpec() is Qt::LocalTime or Qt::TimeZone and the resulting
    date and time fall in the Standard Time to Daylight-Saving Time transition
    hour then the result will be adjusted accordingly, i.e. if the transition
    is at 2am and the clock goes forward to 3am and the result falls between
    2am and 3am then the result will be adjusted to fall after 3am.

    \sa daysTo(), addMonths(), addYears(), addSecs()
*/

QDateTime QDateTime::addDays(qint64 ndays) const
{
    if (isNull())
        return QDateTime();

    QDateTime dt(*this);
    QPair<QDate, QTime> p = getDateTime(d);
    massageAdjustedDateTime(dt.d, p.first.addDays(ndays), p.second);
    return dt;
}

/*!
    Returns a QDateTime object containing a datetime \a nmonths months
    later than the datetime of this object (or earlier if \a nmonths
    is negative).

    If the timeSpec() is Qt::LocalTime or Qt::TimeZone and the resulting
    date and time fall in the Standard Time to Daylight-Saving Time transition
    hour then the result will be adjusted accordingly, i.e. if the transition
    is at 2am and the clock goes forward to 3am and the result falls between
    2am and 3am then the result will be adjusted to fall after 3am.

    \sa daysTo(), addDays(), addYears(), addSecs()
*/

QDateTime QDateTime::addMonths(int nmonths) const
{
    if (isNull())
        return QDateTime();

    QDateTime dt(*this);
    QPair<QDate, QTime> p = getDateTime(d);
    massageAdjustedDateTime(dt.d, p.first.addMonths(nmonths), p.second);
    return dt;
}

/*!
    Returns a QDateTime object containing a datetime \a nyears years
    later than the datetime of this object (or earlier if \a nyears is
    negative).

    If the timeSpec() is Qt::LocalTime or Qt::TimeZone and the resulting
    date and time fall in the Standard Time to Daylight-Saving Time transition
    hour then the result will be adjusted accordingly, i.e. if the transition
    is at 2am and the clock goes forward to 3am and the result falls between
    2am and 3am then the result will be adjusted to fall after 3am.

    \sa daysTo(), addDays(), addMonths(), addSecs()
*/

QDateTime QDateTime::addYears(int nyears) const
{
    if (isNull())
        return QDateTime();

    QDateTime dt(*this);
    QPair<QDate, QTime> p = getDateTime(d);
    massageAdjustedDateTime(dt.d, p.first.addYears(nyears), p.second);
    return dt;
}

/*!
    Returns a QDateTime object containing a datetime \a s seconds
    later than the datetime of this object (or earlier if \a s is
    negative).

    If this datetime is invalid, an invalid datetime will be returned.

    \sa addMSecs(), secsTo(), addDays(), addMonths(), addYears()
*/

QDateTime QDateTime::addSecs(qint64 s) const
{
    qint64 msecs;
    if (mul_overflow(s, std::integral_constant<qint64, MSECS_PER_SEC>(), &msecs))
        return QDateTime();
    return addMSecs(msecs);
}

/*!
    Returns a QDateTime object containing a datetime \a msecs milliseconds
    later than the datetime of this object (or earlier if \a msecs is
    negative).

    If this datetime is invalid, an invalid datetime will be returned.

    \sa addSecs(), msecsTo(), addDays(), addMonths(), addYears()
*/
QDateTime QDateTime::addMSecs(qint64 msecs) const
{
    if (!isValid())
        return QDateTime();

    QDateTime dt(*this);
    switch (getSpec(d)) {
    case Qt::LocalTime:
    case Qt::TimeZone:
        // Convert to real UTC first in case this crosses a DST transition:
        if (!add_overflow(toMSecsSinceEpoch(), msecs, &msecs)) {
            dt.setMSecsSinceEpoch(msecs);
        } else if (dt.d.isShort()) {
            dt.d.data.status &= ~int(QDateTimePrivate::ValidWhenMask);
        } else {
            dt.d.detach();
            dt.d->m_status &= ~QDateTimePrivate::ValidWhenMask;
        }
        break;
    case Qt::UTC:
    case Qt::OffsetFromUTC:
        // No need to convert, just add on
        if (add_overflow(getMSecs(d), msecs, &msecs)) {
            if (dt.d.isShort()) {
                dt.d.data.status &= ~int(QDateTimePrivate::ValidWhenMask);
            } else {
                dt.d.detach();
                dt.d->m_status &= ~QDateTimePrivate::ValidWhenMask;
            }
        } else if (d.isShort()) {
            // need to check if we need to enlarge first
            if (msecsCanBeSmall(msecs)) {
                dt.d.data.msecs = qintptr(msecs);
            } else {
                dt.d.detach();
                dt.d->m_msecs = msecs;
            }
        } else {
            dt.d.detach();
            dt.d->m_msecs = msecs;
        }
        break;
    }
    return dt;
}

/*!
    Returns the number of days from this datetime to the \a other
    datetime. The number of days is counted as the number of times
    midnight is reached between this datetime to the \a other
    datetime. This means that a 10 minute difference from 23:55 to
    0:05 the next day counts as one day.

    If the \a other datetime is earlier than this datetime,
    the value returned is negative.

    Example:
    \snippet code/src_corelib_time_qdatetime.cpp 15

    \sa addDays(), secsTo(), msecsTo()
*/

qint64 QDateTime::daysTo(const QDateTime &other) const
{
    return date().daysTo(other.date());
}

/*!
    Returns the number of seconds from this datetime to the \a other
    datetime. If the \a other datetime is earlier than this datetime,
    the value returned is negative.

    Before performing the comparison, the two datetimes are converted
    to Qt::UTC to ensure that the result is correct if daylight-saving
    (DST) applies to one of the two datetimes but not the other.

    Returns 0 if either datetime is invalid.

    Example:
    \snippet code/src_corelib_time_qdatetime.cpp 11

    \sa addSecs(), daysTo(), QTime::secsTo()
*/

qint64 QDateTime::secsTo(const QDateTime &other) const
{
    return msecsTo(other) / MSECS_PER_SEC;
}

/*!
    Returns the number of milliseconds from this datetime to the \a other
    datetime. If the \a other datetime is earlier than this datetime,
    the value returned is negative.

    Before performing the comparison, the two datetimes are converted
    to Qt::UTC to ensure that the result is correct if daylight-saving
    (DST) applies to one of the two datetimes and but not the other.

    Returns 0 if either datetime is invalid.

    \sa addMSecs(), daysTo(), QTime::msecsTo()
*/

qint64 QDateTime::msecsTo(const QDateTime &other) const
{
    if (!isValid() || !other.isValid())
        return 0;

    return other.toMSecsSinceEpoch() - toMSecsSinceEpoch();
}

/*!
    Returns a copy of this datetime converted to the given time
    \a spec.

    If \a spec is Qt::OffsetFromUTC then it is set to Qt::UTC.  To set to a
    spec of Qt::OffsetFromUTC use toOffsetFromUtc().

    If \a spec is Qt::TimeZone then it is set to Qt::LocalTime,
    i.e. the local Time Zone.

    Example:
    \snippet code/src_corelib_time_qdatetime.cpp 16

    \sa timeSpec(), toTimeZone(), toOffsetFromUtc()
*/

QDateTime QDateTime::toTimeSpec(Qt::TimeSpec spec) const
{
    if (getSpec(d) == spec && (spec == Qt::UTC || spec == Qt::LocalTime))
        return *this;

    if (!isValid()) {
        QDateTime ret = *this;
        ret.setTimeSpec(spec);
        return ret;
    }

    return fromMSecsSinceEpoch(toMSecsSinceEpoch(), spec, 0);
}

/*!
    \since 5.2

    \fn QDateTime QDateTime::toOffsetFromUtc(int offsetSeconds) const

    Returns a copy of this datetime converted to a spec of Qt::OffsetFromUTC
    with the given \a offsetSeconds.

    If the \a offsetSeconds equals 0 then a UTC datetime will be returned

    \sa setOffsetFromUtc(), offsetFromUtc(), toTimeSpec()
*/

QDateTime QDateTime::toOffsetFromUtc(int offsetSeconds) const
{
    if (getSpec(d) == Qt::OffsetFromUTC
            && d->m_offsetFromUtc == offsetSeconds)
        return *this;

    if (!isValid()) {
        QDateTime ret = *this;
        ret.setOffsetFromUtc(offsetSeconds);
        return ret;
    }

    return fromMSecsSinceEpoch(toMSecsSinceEpoch(), Qt::OffsetFromUTC, offsetSeconds);
}

#if QT_CONFIG(timezone)
/*!
    \since 5.2

    Returns a copy of this datetime converted to the given \a timeZone

    \sa timeZone(), toTimeSpec()
*/

QDateTime QDateTime::toTimeZone(const QTimeZone &timeZone) const
{
    if (getSpec(d) == Qt::TimeZone && d->m_timeZone == timeZone)
        return *this;

    if (!isValid()) {
        QDateTime ret = *this;
        ret.setTimeZone(timeZone);
        return ret;
    }

    return fromMSecsSinceEpoch(toMSecsSinceEpoch(), timeZone);
}
#endif // timezone

/*!
    \internal
    Returns \c true if this datetime is equal to the \a other datetime;
    otherwise returns \c false.

    \sa precedes(), operator==()
*/

bool QDateTime::equals(const QDateTime &other) const
{
    if (!isValid())
        return !other.isValid();
    if (!other.isValid())
        return false;

    if (usesSameOffset(d, other.d))
        return getMSecs(d) == getMSecs(other.d);

    // Convert to UTC and compare
    return toMSecsSinceEpoch() == other.toMSecsSinceEpoch();
}

/*!
    \fn bool QDateTime::operator==(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs is the same as \a rhs; otherwise returns \c false.

    Two datetimes are different if either the date, the time, or the time zone
    components are different. Since 5.14, all invalid datetime are equal (and
    less than all valid datetimes).

    \sa operator!=(), operator<(), operator<=(), operator>(), operator>=()
*/

/*!
    \fn bool QDateTime::operator!=(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs is different from \a rhs; otherwise returns \c
    false.

    Two datetimes are different if either the date, the time, or the time zone
    components are different. Since 5.14, all invalid datetime are equal (and
    less than all valid datetimes).

    \sa operator==()
*/

/*!
    \internal
    Returns \c true if \a lhs is earlier than the \a rhs
    datetime; otherwise returns \c false.

    \sa equals(), operator<()
*/

bool QDateTime::precedes(const QDateTime &other) const
{
    if (!isValid())
        return other.isValid();
    if (!other.isValid())
        return false;

    if (usesSameOffset(d, other.d))
        return getMSecs(d) < getMSecs(other.d);

    // Convert to UTC and compare
    return toMSecsSinceEpoch() < other.toMSecsSinceEpoch();
}

/*!
    \fn bool QDateTime::operator<(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs is earlier than \a rhs;
    otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QDateTime::operator<=(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs is earlier than or equal to \a rhs; otherwise
    returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QDateTime::operator>(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs is later than \a rhs; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QDateTime::operator>=(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs is later than or equal to \a rhs;
    otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn QDateTime QDateTime::currentDateTime()
    Returns the current datetime, as reported by the system clock, in
    the local time zone.

    \sa currentDateTimeUtc(), QDate::currentDate(), QTime::currentTime(), toTimeSpec()
*/

/*!
    \fn QDateTime QDateTime::currentDateTimeUtc()
    \since 4.7
    Returns the current datetime, as reported by the system clock, in
    UTC.

    \sa currentDateTime(), QDate::currentDate(), QTime::currentTime(), toTimeSpec()
*/

/*!
    \fn qint64 QDateTime::currentMSecsSinceEpoch()
    \since 4.7

    Returns the number of milliseconds since 1970-01-01T00:00:00 Universal
    Coordinated Time. This number is like the POSIX time_t variable, but
    expressed in milliseconds instead.

    \sa currentDateTime(), currentDateTimeUtc(), toTimeSpec()
*/

/*!
    \fn qint64 QDateTime::currentSecsSinceEpoch()
    \since 5.8

    Returns the number of seconds since 1970-01-01T00:00:00 Universal
    Coordinated Time.

    \sa currentMSecsSinceEpoch()
*/

#if defined(Q_OS_WIN)
static inline uint msecsFromDecomposed(int hour, int minute, int sec, int msec = 0)
{
    return MSECS_PER_HOUR * hour + MSECS_PER_MIN * minute + MSECS_PER_SEC * sec + msec;
}

QDate QDate::currentDate()
{
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    return QDate(st.wYear, st.wMonth, st.wDay);
}

QTime QTime::currentTime()
{
    QTime ct;
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    ct.setHMS(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return ct;
}

QDateTime QDateTime::currentDateTime()
{
    QTime t;
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    QDate d(st.wYear, st.wMonth, st.wDay);
    t.mds = msecsFromDecomposed(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return QDateTime(d, t);
}

QDateTime QDateTime::currentDateTimeUtc()
{
    QTime t;
    SYSTEMTIME st = {};
    GetSystemTime(&st);
    QDate d(st.wYear, st.wMonth, st.wDay);
    t.mds = msecsFromDecomposed(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return QDateTime(d, t, Qt::UTC);
}

qint64 QDateTime::currentMSecsSinceEpoch() noexcept
{
    SYSTEMTIME st = {};
    GetSystemTime(&st);
    const qint64 daysAfterEpoch = QDate(1970, 1, 1).daysTo(QDate(st.wYear, st.wMonth, st.wDay));

    return msecsFromDecomposed(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds) +
           daysAfterEpoch * MSECS_PER_DAY;
}

qint64 QDateTime::currentSecsSinceEpoch() noexcept
{
    SYSTEMTIME st = {};
    GetSystemTime(&st);
    const qint64 daysAfterEpoch = QDate(1970, 1, 1).daysTo(QDate(st.wYear, st.wMonth, st.wDay));

    return st.wHour * SECS_PER_HOUR + st.wMinute * SECS_PER_MIN + st.wSecond +
           daysAfterEpoch * SECS_PER_DAY;
}

#elif defined(Q_OS_UNIX)
QDate QDate::currentDate()
{
    return QDateTime::currentDateTime().date();
}

QTime QTime::currentTime()
{
    return QDateTime::currentDateTime().time();
}

QDateTime QDateTime::currentDateTime()
{
    return fromMSecsSinceEpoch(currentMSecsSinceEpoch(), Qt::LocalTime);
}

QDateTime QDateTime::currentDateTimeUtc()
{
    return fromMSecsSinceEpoch(currentMSecsSinceEpoch(), Qt::UTC);
}

qint64 QDateTime::currentMSecsSinceEpoch() noexcept
{
    // posix compliant system
    // we have milliseconds
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * MSECS_PER_SEC + tv.tv_usec / 1000;
}

qint64 QDateTime::currentSecsSinceEpoch() noexcept
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec;
}
#else
#error "What system is this?"
#endif

/*!
  Returns a datetime whose date and time are the number of milliseconds \a msecs
  that have passed since 1970-01-01T00:00:00.000, Coordinated Universal
  Time (Qt::UTC) and converted to the given \a spec.

  Note that there are possible values for \a msecs that lie outside the valid
  range of QDateTime, both negative and positive. The behavior of this
  function is undefined for those values.

  If the \a spec is not Qt::OffsetFromUTC then the \a offsetSeconds will be
  ignored.  If the \a spec is Qt::OffsetFromUTC and the \a offsetSeconds is 0
  then the spec will be set to Qt::UTC, i.e. an offset of 0 seconds.

  If \a spec is Qt::TimeZone then the spec will be set to Qt::LocalTime,
  i.e. the current system time zone.

  \sa toMSecsSinceEpoch(), setMSecsSinceEpoch()
*/
QDateTime QDateTime::fromMSecsSinceEpoch(qint64 msecs, Qt::TimeSpec spec, int offsetSeconds)
{
    QDateTime dt;
    QT_PREPEND_NAMESPACE(setTimeSpec(dt.d, spec, offsetSeconds));
    dt.setMSecsSinceEpoch(msecs);
    return dt;
}

/*!
  \since 5.8

  Returns a datetime whose date and time are the number of seconds \a secs
  that have passed since 1970-01-01T00:00:00.000, Coordinated Universal
  Time (Qt::UTC) and converted to the given \a spec.

  Note that there are possible values for \a secs that lie outside the valid
  range of QDateTime, both negative and positive. The behavior of this
  function is undefined for those values.

  If the \a spec is not Qt::OffsetFromUTC then the \a offsetSeconds will be
  ignored.  If the \a spec is Qt::OffsetFromUTC and the \a offsetSeconds is 0
  then the spec will be set to Qt::UTC, i.e. an offset of 0 seconds.

  If \a spec is Qt::TimeZone then the spec will be set to Qt::LocalTime,
  i.e. the current system time zone.

  \sa toSecsSinceEpoch(), setSecsSinceEpoch()
*/
QDateTime QDateTime::fromSecsSinceEpoch(qint64 secs, Qt::TimeSpec spec, int offsetSeconds)
{
    constexpr qint64 maxSeconds = std::numeric_limits<qint64>::max() / MSECS_PER_SEC;
    constexpr qint64 minSeconds = std::numeric_limits<qint64>::min() / MSECS_PER_SEC;
    if (secs > maxSeconds || secs < minSeconds)
        return QDateTime(); // Would {und,ov}erflow
    return fromMSecsSinceEpoch(secs * MSECS_PER_SEC, spec, offsetSeconds);
}

#if QT_CONFIG(timezone)
/*!
    \since 5.2

    Returns a datetime whose date and time are the number of milliseconds \a msecs
    that have passed since 1970-01-01T00:00:00.000, Coordinated Universal
    Time (Qt::UTC) and with the given \a timeZone.

    \sa fromSecsSinceEpoch()
*/
QDateTime QDateTime::fromMSecsSinceEpoch(qint64 msecs, const QTimeZone &timeZone)
{
    QDateTime dt;
    dt.setTimeZone(timeZone);
    if (timeZone.isValid())
        dt.setMSecsSinceEpoch(msecs);
    return dt;
}

/*!
    \since 5.8

    Returns a datetime whose date and time are the number of seconds \a secs
    that have passed since 1970-01-01T00:00:00.000, Coordinated Universal
    Time (Qt::UTC) and with the given \a timeZone.

    \sa fromMSecsSinceEpoch()
*/
QDateTime QDateTime::fromSecsSinceEpoch(qint64 secs, const QTimeZone &timeZone)
{
    constexpr qint64 maxSeconds = std::numeric_limits<qint64>::max() / MSECS_PER_SEC;
    constexpr qint64 minSeconds = std::numeric_limits<qint64>::min() / MSECS_PER_SEC;
    if (secs > maxSeconds || secs < minSeconds)
        return QDateTime(); // Would {und,ov}erflow
    return fromMSecsSinceEpoch(secs * MSECS_PER_SEC, timeZone);
}
#endif

#if QT_CONFIG(datestring) // depends on, so implies, textdate

/*!
    \fn QDateTime QDateTime::fromString(const QString &string, Qt::DateFormat format)

    Returns the QDateTime represented by the \a string, using the
    \a format given, or an invalid datetime if this is not possible.

    Note for Qt::TextDate: only English short month names (e.g. "Jan" in short
    form or "January" in long form) are recognized.

    \sa toString(), QLocale::toDateTime()
*/

/*!
    \overload
    \since 6.0
*/
QDateTime QDateTime::fromString(QStringView string, Qt::DateFormat format)
{
    if (string.isEmpty())
        return QDateTime();

    switch (format) {
    case Qt::RFC2822Date: {
        const ParsedRfcDateTime rfc = rfcDateImpl(string);

        if (!rfc.date.isValid() || !rfc.time.isValid())
            return QDateTime();

        QDateTime dateTime(rfc.date, rfc.time, Qt::UTC);
        dateTime.setOffsetFromUtc(rfc.utcOffset);
        return dateTime;
    }
    case Qt::ISODate:
    case Qt::ISODateWithMs: {
        const int size = string.size();
        if (size < 10)
            return QDateTime();

        QDate date = QDate::fromString(string.first(10), Qt::ISODate);
        if (!date.isValid())
            return QDateTime();
        if (size == 10)
            return date.startOfDay();

        Qt::TimeSpec spec = Qt::LocalTime;
        QStringView isoString = string.sliced(10); // trim "yyyy-MM-dd"

        // Must be left with T (or space) and at least one digit for the hour:
        if (isoString.size() < 2
            || !(isoString.startsWith(u'T', Qt::CaseInsensitive)
                 // RFC 3339 (section 5.6) allows a space here.  (It actually
                 // allows any separator one considers more readable, merely
                 // giving space as an example - but let's not go wild !)
                 || isoString.startsWith(u' '))) {
            return QDateTime();
        }
        isoString = isoString.sliced(1); // trim 'T' (or space)

        int offset = 0;
        // Check end of string for Time Zone definition, either Z for UTC or [+-]HH:mm for Offset
        if (isoString.endsWith(u'Z', Qt::CaseInsensitive)) {
            spec = Qt::UTC;
            isoString.chop(1); // trim 'Z'
        } else {
            // the loop below is faster but functionally equal to:
            // const int signIndex = isoString.indexOf(QRegulargExpression(QStringLiteral("[+-]")));
            int signIndex = isoString.size() - 1;
            Q_ASSERT(signIndex >= 0);
            bool found = false;
            do {
                QChar character(isoString[signIndex]);
                found = character == u'+' || character == u'-';
            } while (!found && --signIndex >= 0);

            if (found) {
                bool ok;
                offset = fromOffsetString(isoString.sliced(signIndex), &ok);
                if (!ok)
                    return QDateTime();
                isoString = isoString.first(signIndex);
                spec = Qt::OffsetFromUTC;
            }
        }

        // Might be end of day (24:00, including variants), which QTime considers invalid.
        // ISO 8601 (section 4.2.3) says that 24:00 is equivalent to 00:00 the next day.
        bool isMidnight24 = false;
        QTime time = fromIsoTimeString(isoString, format, &isMidnight24);
        if (!time.isValid())
            return QDateTime();
        if (isMidnight24) // time is 0:0, but we want the start of next day:
            return date.addDays(1).startOfDay(spec, offset);
        return QDateTime(date, time, spec, offset);
    }
    case Qt::TextDate: {
        QList<QStringView> parts = string.split(u' ', Qt::SkipEmptyParts);

        // Documented as "ddd MMM d HH:mm:ss yyyy" with optional offset-suffix;
        // and allow time either before or after year.
        if (parts.count() < 5 || parts.count() > 6)
            return QDateTime();

        // Year and time can be in either order.
        // Guess which by looking for ':' in the time
        int yearPart = 3;
        int timePart = 3;
        if (parts.at(3).contains(u':'))
            yearPart = 4;
        else if (parts.at(4).contains(u':'))
            timePart = 4;
        else
            return QDateTime();

        bool ok = false;
        int day = parts.at(2).toInt(&ok);
        int year = ok ? parts.at(yearPart).toInt(&ok) : 0;
        int month = fromShortMonthName(parts.at(1));
        if (!ok || year == 0 || day == 0 || month < 1)
            return QDateTime();

        const QDate date(year, month, day);
        if (!date.isValid())
            return QDateTime();

        const QTime time = fromIsoTimeString(parts.at(timePart), format, nullptr);
        if (!time.isValid())
            return QDateTime();

        if (parts.count() == 5)
            return QDateTime(date, time, Qt::LocalTime);

        QStringView tz = parts.at(5);
        if (tz.startsWith(QLatin1String("UTC"))
            // GMT has long been deprecated as an alias for UTC.
            || tz.startsWith(QLatin1String("GMT"), Qt::CaseInsensitive)) {
            tz = tz.sliced(3);
            if (tz.isEmpty())
                return QDateTime(date, time, Qt::UTC);

            int offset = fromOffsetString(tz, &ok);
            return ok ? QDateTime(date, time, Qt::OffsetFromUTC, offset) : QDateTime();
        }
        return QDateTime();
    }
    }

    return QDateTime();
}

/*!
    \fn QDateTime QDateTime::fromString(const QString &string, const QString &format, QCalendar cal)

    Returns the QDateTime represented by the \a string, using the \a
    format given, or an invalid datetime if the string cannot be parsed.

    Uses the calendar \a cal if supplied, else Gregorian.

    In addition to the expressions, recognized in the format string to represent
    parts of the date and time, by QDate::fromString() and QTime::fromString(),
    this method supports:

    \table
    \header \li Expression \li Output
    \row \li t \li the timezone (for example "CEST")
    \endtable

    If no 't' format specifier is present, the system's local time-zone is used.
    For the defaults of all other fields, see QDate::fromString() and QTime::fromString().

    For example:

    \snippet code/src_corelib_time_qdatetime.cpp 14

    All other input characters will be treated as text. Any non-empty sequence
    of characters enclosed in single quotes will also be treated (stripped of
    the quotes) as text and not be interpreted as expressions.

    \snippet code/src_corelib_time_qdatetime.cpp 12

    If the format is not satisfied, an invalid QDateTime is returned.  If the
    format is satisfied but \a string represents an invalid date-time (e.g. in a
    gap skipped by a time-zone transition), an invalid QDateTime is returned,
    whose toMSecsSinceEpoch() represents a near-by date-time that is
    valid. Passing that to fromMSecsSinceEpoch() will produce a valid date-time
    that isn't faithfully represented by the string parsed.

    The expressions that don't have leading zeroes (d, M, h, m, s, z) will be
    greedy. This means that they will use two digits (or three, for z) even if this will
    put them outside the range and/or leave too few digits for other
    sections.

    \snippet code/src_corelib_time_qdatetime.cpp 13

    This could have meant 1 January 00:30.00 but the M will grab
    two digits.

    Incorrectly specified fields of the \a string will cause an invalid
    QDateTime to be returned. For example, consider the following code,
    where the two digit year 12 is read as 1912 (see the table below for all
    field defaults); the resulting datetime is invalid because 23 April 1912
    was a Tuesday, not a Monday:

    \snippet code/src_corelib_time_qdatetime.cpp 20

    The correct code is:

    \snippet code/src_corelib_time_qdatetime.cpp 21

    \note Day and month names as well as AM/PM indication must be given in English (C locale).
    If localized month and day names and localized forms of AM/PM are used, use
    QLocale::system().toDateTime().

    \sa toString(), QDate::fromString(), QTime::fromString(),
    QLocale::toDateTime()
*/

/*!
    \fn QDateTime QDateTime::fromString(QStringView string, QStringView format, QCalendar cal)
    \overload
    \since 6.0
*/

/*!
    \overload
    \since 6.0
*/
QDateTime QDateTime::fromString(const QString &string, QStringView format, QCalendar cal)
{
#if QT_CONFIG(datetimeparser)
    QDateTime datetime;

    QDateTimeParser dt(QMetaType::QDateTime, QDateTimeParser::FromString, cal);
    dt.setDefaultLocale(QLocale::c());
    if (dt.parseFormat(format) && (dt.fromString(string, &datetime) || !datetime.isValid()))
        return datetime;
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
    Q_UNUSED(cal);
#endif
    return QDateTime();
}

#endif // datestring
/*!
    \fn QDateTime QDateTime::toLocalTime() const

    Returns a datetime containing the date and time information in
    this datetime, but specified using the Qt::LocalTime definition.

    Example:

    \snippet code/src_corelib_time_qdatetime.cpp 17

    \sa toTimeSpec()
*/

/*!
    \fn QDateTime QDateTime::toUTC() const

    Returns a datetime containing the date and time information in
    this datetime, but specified using the Qt::UTC definition.

    Example:

    \snippet code/src_corelib_time_qdatetime.cpp 18

    \sa toTimeSpec()
*/

/*****************************************************************************
  Date/time stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM
/*!
    \relates QDate

    Writes the \a date to stream \a out.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &out, QDate date)
{
    if (out.version() < QDataStream::Qt_5_0)
        return out << quint32(date.jd);
    else
        return out << qint64(date.jd);
}

/*!
    \relates QDate

    Reads a date from stream \a in into the \a date.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &in, QDate &date)
{
    if (in.version() < QDataStream::Qt_5_0) {
        quint32 jd;
        in >> jd;
        // Older versions consider 0 an invalid jd.
        date.jd = (jd != 0 ? jd : QDate::nullJd());
    } else {
        qint64 jd;
        in >> jd;
        date.jd = jd;
    }

    return in;
}

/*!
    \relates QTime

    Writes \a time to stream \a out.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &out, QTime time)
{
    if (out.version() >= QDataStream::Qt_4_0) {
        return out << quint32(time.mds);
    } else {
        // Qt3 had no support for reading -1, QTime() was valid and serialized as 0
        return out << quint32(time.isNull() ? 0 : time.mds);
    }
}

/*!
    \relates QTime

    Reads a time from stream \a in into the given \a time.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &in, QTime &time)
{
    quint32 ds;
    in >> ds;
    if (in.version() >= QDataStream::Qt_4_0) {
        time.mds = int(ds);
    } else {
        // Qt3 would write 0 for a null time
        time.mds = (ds == 0) ? QTime::NullTime : int(ds);
    }
    return in;
}

/*!
    \relates QDateTime

    Writes \a dateTime to the \a out stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, const QDateTime &dateTime)
{
    QPair<QDate, QTime> dateAndTime;

    if (out.version() >= QDataStream::Qt_5_2) {

        // In 5.2 we switched to using Qt::TimeSpec and added offset support
        dateAndTime = getDateTime(dateTime.d);
        out << dateAndTime << qint8(dateTime.timeSpec());
        if (dateTime.timeSpec() == Qt::OffsetFromUTC)
            out << qint32(dateTime.offsetFromUtc());
#if QT_CONFIG(timezone)
        else if (dateTime.timeSpec() == Qt::TimeZone)
            out << dateTime.timeZone();
#endif // timezone

    } else if (out.version() == QDataStream::Qt_5_0) {

        // In Qt 5.0 we incorrectly serialised all datetimes as UTC.
        // This approach is wrong and should not be used again; it breaks
        // the guarantee that a deserialised local datetime is the same time
        // of day, regardless of which timezone it was serialised in.
        dateAndTime = getDateTime((dateTime.isValid() ? dateTime.toUTC() : dateTime).d);
        out << dateAndTime << qint8(dateTime.timeSpec());

    } else if (out.version() >= QDataStream::Qt_4_0) {

        // From 4.0 to 5.1 (except 5.0) we used QDateTimePrivate::Spec
        dateAndTime = getDateTime(dateTime.d);
        out << dateAndTime;
        switch (dateTime.timeSpec()) {
        case Qt::UTC:
            out << (qint8)QDateTimePrivate::UTC;
            break;
        case Qt::OffsetFromUTC:
            out << (qint8)QDateTimePrivate::OffsetFromUTC;
            break;
        case Qt::TimeZone:
            out << (qint8)QDateTimePrivate::TimeZone;
            break;
        case Qt::LocalTime:
            out << (qint8)QDateTimePrivate::LocalUnknown;
            break;
        }

    } else { // version < QDataStream::Qt_4_0

        // Before 4.0 there was no TimeSpec, only Qt::LocalTime was supported
        dateAndTime = getDateTime(dateTime.d);
        out << dateAndTime;

    }

    return out;
}

/*!
    \relates QDateTime

    Reads a datetime from the stream \a in into \a dateTime.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &in, QDateTime &dateTime)
{
    QDate dt;
    QTime tm;
    qint8 ts = 0;
    Qt::TimeSpec spec = Qt::LocalTime;
    qint32 offset = 0;
#if QT_CONFIG(timezone)
    QTimeZone tz;
#endif // timezone

    if (in.version() >= QDataStream::Qt_5_2) {

        // In 5.2 we switched to using Qt::TimeSpec and added offset support
        in >> dt >> tm >> ts;
        spec = static_cast<Qt::TimeSpec>(ts);
        if (spec == Qt::OffsetFromUTC) {
            in >> offset;
            dateTime = QDateTime(dt, tm, spec, offset);
#if QT_CONFIG(timezone)
        } else if (spec == Qt::TimeZone) {
            in >> tz;
            dateTime = QDateTime(dt, tm, tz);
#endif // timezone
        } else {
            dateTime = QDateTime(dt, tm, spec);
        }

    } else if (in.version() == QDataStream::Qt_5_0) {

        // In Qt 5.0 we incorrectly serialised all datetimes as UTC
        in >> dt >> tm >> ts;
        spec = static_cast<Qt::TimeSpec>(ts);
        dateTime = QDateTime(dt, tm, Qt::UTC);
        dateTime = dateTime.toTimeSpec(spec);

    } else if (in.version() >= QDataStream::Qt_4_0) {

        // From 4.0 to 5.1 (except 5.0) we used QDateTimePrivate::Spec
        in >> dt >> tm >> ts;
        switch ((QDateTimePrivate::Spec)ts) {
        case QDateTimePrivate::UTC:
            spec = Qt::UTC;
            break;
        case QDateTimePrivate::OffsetFromUTC:
            spec = Qt::OffsetFromUTC;
            break;
        case QDateTimePrivate::TimeZone:
            spec = Qt::TimeZone;
#if QT_CONFIG(timezone)
            // FIXME: need to use a different constructor !
#endif
            break;
        case QDateTimePrivate::LocalUnknown:
        case QDateTimePrivate::LocalStandard:
        case QDateTimePrivate::LocalDST:
            spec = Qt::LocalTime;
            break;
        }
        dateTime = QDateTime(dt, tm, spec, offset);

    } else { // version < QDataStream::Qt_4_0

        // Before 4.0 there was no TimeSpec, only Qt::LocalTime was supported
        in >> dt >> tm;
        dateTime = QDateTime(dt, tm, spec, offset);

    }

    return in;
}
#endif // QT_NO_DATASTREAM

/*****************************************************************************
  Date / Time Debug Streams
*****************************************************************************/

#if !defined(QT_NO_DEBUG_STREAM) && QT_CONFIG(datestring)
QDebug operator<<(QDebug dbg, QDate date)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QDate(";
    if (date.isValid())
        dbg.nospace() << date.toString(Qt::ISODate);
    else
        dbg.nospace() << "Invalid";
    dbg.nospace() << ')';
    return dbg;
}

QDebug operator<<(QDebug dbg, QTime time)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QTime(";
    if (time.isValid())
        dbg.nospace() << time.toString(u"HH:mm:ss.zzz");
    else
        dbg.nospace() << "Invalid";
    dbg.nospace() << ')';
    return dbg;
}

QDebug operator<<(QDebug dbg, const QDateTime &date)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QDateTime(";
    if (date.isValid()) {
        const Qt::TimeSpec ts = date.timeSpec();
        dbg.noquote() << date.toString(u"yyyy-MM-dd HH:mm:ss.zzz t")
                      << ' ' << ts;
        switch (ts) {
        case Qt::UTC:
            break;
        case Qt::OffsetFromUTC:
            dbg.space() << date.offsetFromUtc() << 's';
            break;
        case Qt::TimeZone:
#if QT_CONFIG(timezone)
            dbg.space() << date.timeZone().id();
#endif // timezone
            break;
        case Qt::LocalTime:
            break;
        }
    } else {
        dbg.nospace() << "Invalid";
    }
    return dbg.nospace() << ')';
}
#endif // debug_stream && datestring

/*! \fn size_t qHash(const QDateTime &key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
size_t qHash(const QDateTime &key, size_t seed)
{
    // Use to toMSecsSinceEpoch instead of individual qHash functions for
    // QDate/QTime/spec/offset because QDateTime::operator== converts both arguments
    // to the same timezone. If we don't, qHash would return different hashes for
    // two QDateTimes that are equivalent once converted to the same timezone.
    return key.isValid() ? qHash(key.toMSecsSinceEpoch(), seed) : seed;
}

/*! \fn size_t qHash(QDate key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
size_t qHash(QDate key, size_t seed) noexcept
{
    return qHash(key.toJulianDay(), seed);
}

/*! \fn size_t qHash(QTime key, size_t seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
size_t qHash(QTime key, size_t seed) noexcept
{
    return qHash(key.msecsSinceStartOfDay(), seed);
}

QT_END_NAMESPACE
