// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qdatetime.h"

#include "qcalendar.h"
#include "qdatastream.h"
#include "qdebug.h"
#include "qlocale.h"
#include "qset.h"

#include "private/qcalendarmath_p.h"
#include "private/qdatetime_p.h"
#if QT_CONFIG(datetimeparser)
#include "private/qdatetimeparser_p.h"
#endif
#ifdef Q_OS_DARWIN
#include "private/qcore_mac_p.h"
#endif
#include "private/qgregoriancalendar_p.h"
#include "private/qlocale_tools_p.h"
#include "private/qlocaltime_p.h"
#include "private/qnumeric_p.h"
#include "private/qstringconverter_p.h"
#include "private/qstringiterator_p.h"
#if QT_CONFIG(timezone)
#include "private/qtimezoneprivate_p.h"
#endif

#include <cmath>
#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

#include <private/qtools_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtPrivate::DateTimeConstants;
using namespace QtMiscUtils;

/*****************************************************************************
  Date/Time Constants
 *****************************************************************************/

/*****************************************************************************
  QDate static helper functions
 *****************************************************************************/
static_assert(std::is_trivially_copyable_v<QCalendar::YearMonthDay>);

static inline QDate fixedDate(QCalendar::YearMonthDay parts, QCalendar cal)
{
    if ((parts.year < 0 && !cal.isProleptic()) || (parts.year == 0 && !cal.hasYearZero()))
        return QDate();

    parts.day = qMin(parts.day, cal.daysInMonth(parts.month, parts.year));
    return cal.dateFromParts(parts);
}

static inline QDate fixedDate(QCalendar::YearMonthDay parts)
{
    if (parts.year) {
        parts.day = qMin(parts.day, QGregorianCalendar::monthLength(parts.month, parts.year));
        const auto jd = QGregorianCalendar::julianFromParts(parts.year, parts.month, parts.day);
        if (jd)
            return QDate::fromJulianDay(*jd);
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
        if (monthName == QLatin1StringView(qt_shortMonthNames[i], 3))
            return i + 1;
    }
    return -1;
}
#endif // textdate

#if QT_CONFIG(datestring) // depends on, so implies, textdate
namespace {
using ParsedInt = QSimpleParsedNumber<qulonglong>;

/*
    Reads a whole number that must be the whole text.
*/
ParsedInt readInt(QLatin1StringView text)
{
    // Various date formats' fields (e.g. all in ISO) should not accept spaces
    // or signs, so check that the string starts with a digit and that qstrntoull()
    // converted the whole string.

    if (text.isEmpty() || !isAsciiDigit(text.front().toLatin1()))
        return {};

    QSimpleParsedNumber res = qstrntoull(text.data(), text.size(), 10);
    return res.used == text.size() ? res : ParsedInt{};
}

ParsedInt readInt(QStringView text)
{
    if (text.isEmpty())
        return {};

    // Converting to Latin-1 because QStringView::toULongLong() works with
    // US-ASCII only by design anyway.
    // Also QStringView::toULongLong() can't be used here as it will happily ignore
    // spaces and accept signs; but various date formats' fields (e.g. all in ISO)
    // should not.
    QVarLengthArray<char> latin1(text.size());
    QLatin1::convertFromUnicode(latin1.data(), text);
    return readInt(QLatin1StringView{latin1.data(), latin1.size()});
}

} // namespace

struct ParsedRfcDateTime {
    QDate date;
    QTime time;
    int utcOffset = 0;
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

    QVarLengthArray<QStringView, 6> words;

    auto tokens = s.tokenize(u' ', Qt::SkipEmptyParts);
    auto it = tokens.begin();
    for (int i = 0; i < 6 && it != tokens.end(); ++i, ++it)
        words.emplace_back(*it);

    if (words.size() < 3 || it != tokens.end())
        return result;
    const QChar colon(u':');
    bool ok = true;
    QDate date;

    const auto isShortName = [](QStringView name) {
        return (name.size() == 3 && name[0].isUpper()
                && name[1].isLower() && name[2].isLower());
    };

    /* Reject entirely (return) if the string is malformed; however, if the date
     * is merely invalid, (break, so as to) go on to parsing of the time.
     */
    int yearIndex;
    do { // "loop" so that we can use break on merely invalid, but "right shape" date.
        QStringView dayName;
        bool rfcX22 = true;
        const QStringView maybeDayName = words.front();
        if (maybeDayName.endsWith(u',')) {
            dayName = maybeDayName.chopped(1);
            words.erase(words.begin());
        } else if (!maybeDayName.front().isDigit()) {
            dayName = maybeDayName;
            words.erase(words.begin());
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
        const QStringView when = words.front();
        words.erase(words.begin());
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
        const QStringView zone = words.front();
        words.erase(words.begin());
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

// Return offset in ±HH:mm format
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
// Parse offset in ±HH[[:]mm] format
static int fromOffsetString(QStringView offsetString, bool *valid) noexcept
{
    *valid = false;

    const qsizetype size = offsetString.size();
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
        mmIndex = hhLen = 2; // ±HHmm or ±HH format
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

    \compares strong
    \compareswith strong std::chrono::year_month_day std::chrono::year_month_day_last \
                  std::chrono::year_month_weekday std::chrono::year_month_weekday_last
    These comparison operators are only available when using C++20.
    \endcompareswith

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
    static_assert(maxJd() == JulianDayMax);
    static_assert(minJd() == JulianDayMin);
    jd = QGregorianCalendar::julianFromParts(y, m, d).value_or(nullJd());
}

QDate::QDate(int y, int m, int d, QCalendar cal)
{
    *this = cal.dateFromParts(y, m, d);
}

/*!
    \fn QDate::QDate(std::chrono::year_month_day date)
    \fn QDate::QDate(std::chrono::year_month_day_last date)
    \fn QDate::QDate(std::chrono::year_month_weekday date)
    \fn QDate::QDate(std::chrono::year_month_weekday_last date)

    \since 6.4

    Constructs a QDate representing the same date as \a date. This allows for
    easy interoperability between the Standard Library calendaring classes and
    Qt datetime classes.

    For example:

    \snippet code/src_corelib_time_qdatetime.cpp 22

    \note Unlike QDate, std::chrono::year and the related classes feature the
    year zero. This means that if \a date is in the year zero or before, the
    resulting QDate object will have an year one less than the one specified by
    \a date.

    \note This function requires C++20.
*/

/*!
    \fn QDate QDate::fromStdSysDays(const std::chrono::sys_days &days)
    \since 6.4

    Returns a QDate \a days days after January 1st, 1970 (the UNIX epoch). If
    \a days is negative, the returned date will be before the epoch.

    \note This function requires C++20.

    \sa toStdSysDays()
*/

/*!
    \fn std::chrono::sys_days QDate::toStdSysDays() const

    Returns the number of days between January 1st, 1970 (the UNIX epoch) and
    this date, represented as a \c{std::chrono::sys_days} object. If this date
    is before the epoch, the number of days will be negative.

    \note This function requires C++20.

    \sa fromStdSysDays(), daysTo()
*/

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
        if (const auto first = QGregorianCalendar::julianFromParts(year(), 1, 1))
            return jd - *first + 1;
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

#if QT_DEPRECATED_SINCE(6, 9)
// Only called by deprecated methods (so bootstrap builds warn unused without this #if).
static QTimeZone asTimeZone(Qt::TimeSpec spec, int offset, const char *warner)
{
    if (warner) {
        switch (spec) {
        case Qt::TimeZone:
            qWarning("%s: Pass a QTimeZone instead of Qt::TimeZone.", warner);
            break;
        case Qt::LocalTime:
            if (offset) {
                qWarning("%s: Ignoring offset (%d seconds) passed with Qt::LocalTime",
                         warner, offset);
            }
            break;
        case Qt::UTC:
            if (offset) {
                qWarning("%s: Ignoring offset (%d seconds) passed with Qt::UTC",
                         warner, offset);
                offset = 0;
            }
            break;
        case Qt::OffsetFromUTC:
            break;
        }
    }
    return QTimeZone::isUtcOrFixedOffset(spec)
        ? QTimeZone::fromSecondsAheadOfUtc(offset)
        : QTimeZone(QTimeZone::LocalTime);
}
#endif // Helper for 6.9 deprecation

enum class DaySide { Start, End };

static bool inDateTimeRange(qint64 jd, DaySide side)
{
    using Bounds = std::numeric_limits<qint64>;
    if (jd < Bounds::min() + JULIAN_DAY_FOR_EPOCH)
        return false;
    jd -= JULIAN_DAY_FOR_EPOCH;
    const qint64 maxDay = Bounds::max() / MSECS_PER_DAY;
    const qint64 minDay = Bounds::min() / MSECS_PER_DAY - 1;
    // (Divisions rounded towards zero, as MSECS_PER_DAY is even - so doesn't
    // divide max() - and has factors other than two, so doesn't divide min().)
    // Range includes start of last day and end of first:
    switch (side) {
    case DaySide::Start:
        return jd > minDay && jd <= maxDay;
    case DaySide::End:
        return jd >= minDay && jd < maxDay;
    }
    Q_UNREACHABLE_RETURN(false);
}

static QDateTime toEarliest(QDate day, const QTimeZone &zone)
{
    Q_ASSERT(!zone.isUtcOrFixedOffset());
    // And the day starts in a gap. First find a moment not in that gap.
    const auto moment = [=](QTime time) {
        return QDateTime(day, time, zone, QDateTime::TransitionResolution::Reject);
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
        const int mid = (high + low) / 2;
        const QDateTime probe = QDateTime(day, QTime(mid / 60, mid % 60), zone,
                                          QDateTime::TransitionResolution::PreferBefore);
        if (probe.isValid() && probe.date() == day) {
            high = mid;
            when = probe;
        } else {
            low = mid;
        }
    }
    // Transitions out of local solar mean time, and the few international
    // date-line crossings before that (Alaska, Philippines), may have happened
    // between minute boundaries. Don't try to fix milliseconds.
    if (QDateTime p = moment(when.time().addSecs(-1)); Q_UNLIKELY(p.isValid() && p.date() == day)) {
        high *= 60;
        low *= 60;
        while (high > low + 1) {
            const int mid = (high + low) / 2;
            const int min = mid / 60;
            const QDateTime probe = moment(QTime(min / 60, min % 60, mid % 60));
            if (probe.isValid() && probe.date() == day) {
                high = mid;
                when = probe;
            } else {
                low = mid;
            }
        }
    }
    return when.isValid() ? when : QDateTime();
}

/*!
    \since 5.14

    Returns the start-moment of the day.

    When a day starts depends on a how time is described: each day starts and
    ends earlier for those in time-zones further west and later for those in
    time-zones further east. The time representation to use can be specified by
    an optional time \a zone. The default time representation is the system's
    local time.

    Usually, the start of the day is midnight, 00:00: however, if a time-zone
    transition causes the given date to skip over that midnight (e.g. a DST
    spring-forward skipping over the first hour of the day day), the actual
    earliest time in the day is returned. This can only arise when the time
    representation is a time-zone or local time.

    When \a zone has a timeSpec() of is Qt::OffsetFromUTC or Qt::UTC, the time
    representation has no transitions so the start of the day is QTime(0, 0).

    In the rare case of a date that was entirely skipped (this happens when a
    zone east of the international date-line switches to being west of it), the
    return shall be invalid. Passing an invalid time-zone as \a zone will also
    produce an invalid result, as shall dates that start outside the range
    representable by QDateTime.

    \sa endOfDay()
*/
QDateTime QDate::startOfDay(const QTimeZone &zone) const
{
    if (!inDateTimeRange(jd, DaySide::Start) || !zone.isValid())
        return QDateTime();

    QDateTime when(*this, QTime(0, 0), zone,
                   QDateTime::TransitionResolution::RelativeToBefore);
    if (Q_UNLIKELY(!when.isValid() || when.date() != *this)) {
#if QT_CONFIG(timezone)
        // The start of the day must have fallen in a spring-forward's gap; find the spring-forward:
        if (zone.timeSpec() == Qt::TimeZone && zone.hasTransitions()) {
            QTimeZone::OffsetData tran
                // There's unlikely to be another transition before noon tomorrow.
                // However, the whole of today may have been skipped !
                = zone.previousTransition(QDateTime(addDays(1), QTime(12, 0), zone));
            const QDateTime &at = tran.atUtc.toTimeZone(zone);
            if (at.isValid() && at.date() == *this)
                return at;
        }
#endif

        when = toEarliest(*this, zone);
    }

    return when;
}

/*!
    \overload
    \since 6.5
*/
QDateTime QDate::startOfDay() const
{
    return startOfDay(QTimeZone::LocalTime);
}

#if QT_DEPRECATED_SINCE(6, 9)
/*!
    \overload
    \since 5.14
    \deprecated [6.9] Use \c{startOfDay(const QTimeZone &)} instead.

    Returns the start-moment of the day.

    When a day starts depends on a how time is described: each day starts and
    ends earlier for those with higher offsets from UTC and later for those with
    lower offsets from UTC. The time representation to use can be specified
    either by a \a spec and \a offsetSeconds (ignored unless \a spec is
    Qt::OffsetSeconds) or by a time zone.

    Usually, the start of the day is midnight, 00:00: however, if a local time
    transition causes the given date to skip over that midnight (e.g. a DST
    spring-forward skipping over the first hour of the day day), the actual
    earliest time in the day is returned.

    When \a spec is Qt::OffsetFromUTC, \a offsetSeconds gives an implied zone's
    offset from UTC. As UTC and such zones have no transitions, the start of the
    day is QTime(0, 0) in these cases.

    In the rare case of a date that was entirely skipped (this happens when a
    zone east of the international date-line switches to being west of it), the
    return shall be invalid. Passing Qt::TimeZone as \a spec (instead of passing
    a QTimeZone) will also produce an invalid result, as shall dates that start
    outside the range representable by QDateTime.
*/
QDateTime QDate::startOfDay(Qt::TimeSpec spec, int offsetSeconds) const
{
    QTimeZone zone = asTimeZone(spec, offsetSeconds, "QDate::startOfDay");
    // If spec was Qt::TimeZone, zone's is Qt::LocalTime.
    return zone.timeSpec() == spec ? startOfDay(zone) :  QDateTime();
}
#endif // 6.9 deprecation

static QDateTime toLatest(QDate day, const QTimeZone &zone)
{
    Q_ASSERT(!zone.isUtcOrFixedOffset());
    // And the day ends in a gap. First find a moment not in that gap:
    const auto moment = [=](QTime time) {
        return QDateTime(day, time, zone, QDateTime::TransitionResolution::Reject);
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
        const int mid = (high + low) / 2;
        const QDateTime probe = QDateTime(day, QTime(mid / 60, mid % 60, 59, 999), zone,
                                          QDateTime::TransitionResolution::PreferAfter);
        if (probe.isValid() && probe.date() == day) {
            low = mid;
            when = probe;
        } else {
            high = mid;
        }
    }
    // Transitions out of local solar mean time, and the few international
    // date-line crossings before that (Alaska, Philippines), may have happened
    // between minute boundaries. Don't try to fix milliseconds.
    if (QDateTime p = moment(when.time().addSecs(1)); Q_UNLIKELY(p.isValid() && p.date() == day)) {
        high *= 60;
        low *= 60;
        while (high > low + 1) {
            const int mid = (high + low) / 2;
            const int min = mid / 60;
            const QDateTime probe = moment(QTime(min / 60, min % 60, mid % 60, 999));
            if (probe.isValid() && probe.date() == day) {
                low = mid;
                when = probe;
            } else {
                high = mid;
            }
        }
    }
    return when.isValid() ? when : QDateTime();
}

/*!
    \since 5.14

    Returns the end-moment of the day.

    When a day ends depends on a how time is described: each day starts and ends
    earlier for those in time-zones further west and later for those in
    time-zones further east. The time representation to use can be specified by
    an optional time \a zone. The default time representation is the system's
    local time.

    Usually, the end of the day is one millisecond before the midnight, 24:00:
    however, if a time-zone transition causes the given date to skip over that
    moment (e.g. a DST spring-forward skipping over 23:00 and the following
    hour), the actual latest time in the day is returned. This can only arise
    when the time representation is a time-zone or local time.

    When \a zone has a timeSpec() of Qt::OffsetFromUTC or Qt::UTC, the time
    representation has no transitions so the end of the day is QTime(23, 59, 59,
    999).

    In the rare case of a date that was entirely skipped (this happens when a
    zone east of the international date-line switches to being west of it), the
    return shall be invalid. Passing an invalid time-zone as \a zone will also
    produce an invalid result, as shall dates that end outside the range
    representable by QDateTime.

    \sa startOfDay()
*/
QDateTime QDate::endOfDay(const QTimeZone &zone) const
{
    if (!inDateTimeRange(jd, DaySide::End) || !zone.isValid())
        return QDateTime();

    QDateTime when(*this, QTime(23, 59, 59, 999), zone,
                   QDateTime::TransitionResolution::RelativeToAfter);
    if (Q_UNLIKELY(!when.isValid() || when.date() != *this)) {
#if QT_CONFIG(timezone)
        // The end of the day must have fallen in a spring-forward's gap; find the spring-forward:
        if (zone.timeSpec() == Qt::TimeZone && zone.hasTransitions()) {
            QTimeZone::OffsetData tran
                // It's unlikely there's been another transition since yesterday noon.
                // However, the whole of today may have been skipped !
                = zone.nextTransition(QDateTime(addDays(-1), QTime(12, 0), zone));
            const QDateTime &at = tran.atUtc.toTimeZone(zone);
            if (at.isValid() && at.date() == *this)
                return at;
        }
#endif

        when = toLatest(*this, zone);
    }
    return when;
}

/*!
    \overload
    \since 6.5
*/
QDateTime QDate::endOfDay() const
{
    return endOfDay(QTimeZone::LocalTime);
}

#if QT_DEPRECATED_SINCE(6, 9)
/*!
    \overload
    \since 5.14
    \deprecated [6.9] Use \c{endOfDay(const QTimeZone &) instead.

    Returns the end-moment of the day.

    When a day ends depends on a how time is described: each day starts and ends
    earlier for those with higher offsets from UTC and later for those with
    lower offsets from UTC. The time representation to use can be specified
    either by a \a spec and \a offsetSeconds (ignored unless \a spec is
    Qt::OffsetSeconds) or by a time zone.

    Usually, the end of the day is one millisecond before the midnight, 24:00:
    however, if a local time transition causes the given date to skip over that
    moment (e.g. a DST spring-forward skipping over 23:00 and the following
    hour), the actual latest time in the day is returned.

    When \a spec is Qt::OffsetFromUTC, \a offsetSeconds gives the implied zone's
    offset from UTC. As UTC and such zones have no transitions, the end of the
    day is QTime(23, 59, 59, 999) in these cases.

    In the rare case of a date that was entirely skipped (this happens when a
    zone east of the international date-line switches to being west of it), the
    return shall be invalid. Passing Qt::TimeZone as \a spec (instead of passing
    a QTimeZone) will also produce an invalid result, as shall dates that end
    outside the range representable by QDateTime.
*/
QDateTime QDate::endOfDay(Qt::TimeSpec spec, int offsetSeconds) const
{
    QTimeZone zone = asTimeZone(spec, offsetSeconds, "QDate::endOfDay");
    // If spec was Qt::TimeZone, zone's is Qt::LocalTime.
    return endOfDay(zone);
}
#endif // 6.9 deprecation

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
    \since 5.14

    Returns the date as a string. The \a format parameter determines the format
    of the result string. If \a cal is supplied, it determines the calendar used
    to represent the date; it defaults to Gregorian. Prior to Qt 5.14, there was
    no \a cal parameter and the Gregorian calendar was always used.

    These expressions may be used in the \a format parameter:

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

    \note Day and month names are given in English (C locale). To get localized
    month and day names, use QLocale::system().toString().

    \note If a format character is repeated more times than the longest
    expression in the table above using it, this part of the format will be read
    as several expressions with no separator between them; the longest above,
    possibly repeated as many times as there are copies of it, ending with a
    residue that may be a shorter expression. Thus \c{'MMMMMMMMMM'} for a date
    in May will contribute \c{"MayMay05"} to the output.

    \sa fromString(), QDateTime::toString(), QTime::toString(), QLocale::toString()
*/
QString QDate::toString(QStringView format, QCalendar cal) const
{
    return QLocale::c().toString(*this, format, cal);
}

// Out-of-line no-calendar overloads, since QCalendar is a non-trivial type
/*!
    \overload
    \since 5.10
*/
QString QDate::toString(QStringView format) const
{
    return QLocale::c().toString(*this, format, QCalendar());
}

/*!
    \overload
    \since 4.6
*/
QString QDate::toString(const QString &format) const
{
    return QLocale::c().toString(*this, qToStringViewIgnoringNull(format), QCalendar());
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
    const auto maybe = QGregorianCalendar::julianFromParts(year, month, day);
    jd = maybe.value_or(nullJd());
    return bool(maybe);
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
    \fn QDate QDate::addDuration(std::chrono::days ndays) const

    \since 6.4

    Returns a QDate object containing a date \a ndays later than the
    date of this object (or earlier if \a ndays is negative).

    Returns a null date if the current date is invalid or the new date is
    out of range.

    \note Adding durations expressed in \c{std::chrono::months} or
    \c{std::chrono::years} does not yield the same result obtained by using
    addMonths() or addYears(). The former are fixed durations, calculated in
    relation to the solar year; the latter use the Gregorian calendar definitions
    of months/years.

    \note This function requires C++20.

    \sa addMonths(), addYears(), daysTo()
*/

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

    return fixedDate(parts, cal);
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

    return fixedDate(parts);
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

    // If we just crossed (or hit) a missing year zero, adjust year by ±1:
    if (!cal.hasYearZero() && ((old_y > 0) != (parts.year > 0) || !parts.year))
        parts.year += nyears > 0 ? +1 : -1;

    return fixedDate(parts, cal);
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

    // If we just crossed (or hit) a missing year zero, adjust year by ±1:
    if ((old_y > 0) != (parts.year > 0) || !parts.year)
        parts.year += nyears > 0 ? +1 : -1;

    return fixedDate(parts);
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
    \fn bool QDate::operator==(const QDate &lhs, const QDate &rhs)

    Returns \c true if \a lhs and \a rhs represent the same day, otherwise
    \c false.
*/

/*!
    \fn bool QDate::operator!=(const QDate &lhs, const QDate &rhs)

    Returns \c true if \a lhs and \a rhs represent distinct days; otherwise
    returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QDate::operator<(const QDate &lhs, const QDate &rhs)

    Returns \c true if \a lhs is earlier than \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QDate::operator<=(const QDate &lhs, const QDate &rhs)

    Returns \c true if \a lhs is earlier than or equal to \a rhs;
    otherwise returns \c false.
*/

/*!
    \fn bool QDate::operator>(const QDate &lhs, const QDate &rhs)

    Returns \c true if \a lhs is later than \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QDate::operator>=(const QDate &lhs, const QDate &rhs)

    Returns \c true if \a lhs is later than or equal to \a rhs;
    otherwise returns \c false.
*/

/*!
    \fn QDate::currentDate()
    Returns the system clock's current date.

    \sa QTime::currentTime(), QDateTime::currentDateTime()
*/

#if QT_CONFIG(datestring) // depends on, so implies, textdate

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
        QVarLengthArray<QStringView, 4> parts;
        auto tokens = string.tokenize(u' ', Qt::SkipEmptyParts);
        auto it = tokens.begin();
        for (int i = 0; i < 4 && it != tokens.end(); ++i, ++it)
            parts.emplace_back(*it);

        if (parts.size() != 4 || it != tokens.end())
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
        if (string.size() >= 10 && string[4].isPunct() && string[7].isPunct()
                && (string.size() == 10 || !string[10].isDigit())) {
            const ParsedInt year = readInt(string.first(4));
            const ParsedInt month = readInt(string.sliced(5, 2));
            const ParsedInt day = readInt(string.sliced(8, 2));
            if (year.ok() && year.result > 0 && year.result <= 9999 && month.ok() && day.ok())
                return QDate(year.result, month.result, day.result);
        }
        break;
    }
    return QDate();
}

/*!
    \fn QDate QDate::fromString(const QString &string, const QString &format, int baseYear, QCalendar cal)

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

    \note Day and month names must be given in English (C locale). If localized
    month and day names are to be recognized, use QLocale::system().toDate().

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
    \row    \li Year   \li \a baseYear (or 1900)
    \row    \li Month  \li 1 (January)
    \row    \li Day    \li 1
    \endtable

    When \a format only specifies the last two digits of a year, the 100 years
    starting at \a baseYear are the candidates first considered. Prior to 6.7
    there was no \a baseYear parameter and 1900 was always used. This is the
    default for \a baseYear, selecting a year from then to 1999. Passing 1976 as
    \a baseYear will select a year from 1976 through 2075, for example. When the
    format also includes month, day (of month) and day-of-week, these suffice to
    imply the century. In such a case, a matching date is selected in the
    nearest century to the one indicated by \a baseYear, prefering later over
    earlier. See \l QCalendar::matchCenturyToWeekday() and \l {Date ambiguities}
    for further details,

    The following examples demonstrate the default values:

    \snippet code/src_corelib_time_qdatetime.cpp 3

    \note If a format character is repeated more times than the longest
    expression in the table above using it, this part of the format will be read
    as several expressions with no separator between them; the longest above,
    possibly repeated as many times as there are copies of it, ending with a
    residue that may be a shorter expression. Thus \c{'MMMMMMMMMM'} would match
    \c{"MayMay05"} and set the month to May. Likewise, \c{'MMMMMM'} would match
    \c{"May08"} and find it inconsistent, leading to an invalid date.

    \section2 Date ambiguities

    Different cultures use different formats for dates and, as a result, users
    may mix up the order in which date fields should be given. For example,
    \c{"Wed 28-Nov-01"} might mean either 2028 November 1st or the 28th of
    November, 2001 (each of which happens to be a Wednesday). Using format
    \c{"ddd yy-MMM-dd"} it shall be interpreted the first way, using \c{"ddd
    dd-MMM-yy"} the second. However, which the user meant may depend on the way
    the user normally writes dates, rather than the format the code was
    expecting.

    The example considered above mixed up day of the month and a two-digit year.
    Similar confusion can arise over interchanging the month and day of the
    month, when both are given as numbers. In these cases, including a day of
    the week field in the date format can provide some redundancy, that may help
    to catch errors of this kind. However, as in the example above, this is not
    always effective: the interchange of two fields (or their meanings) may
    produce dates with the same day of the week.

    Including a day of the week in the format can also resolve the century of a
    date specified using only the last two digits of its year. Unfortunately,
    when combined with a date in which the user (or other source of data) has
    mixed up two of the fields, this resolution can lead to finding a date which
    does match the format's reading but isn't the one intended by its author.
    Likewise, if the user simply gets the day of the week wrong, in an otherwise
    correct date, this can lead a date in a different century. In each case,
    finding a date in a different century can turn a wrongly-input date into a
    wildly different one.

    The best way to avoid date ambiguities is to use four-digit years and months
    specified by name (whether full or abbreviated), ideally collected via user
    interface idioms that make abundantly clear to the user which part of the
    date they are selecting. Including a day of the week can also help by
    providing the means to check consistency of the data. Where data comes from
    the user, using a format supplied by a locale selected by the user, it is
    best to use a long format as short formats are more likely to use two-digit
    years. Of course, it is not always possible to control the format - data may
    come from a source you do not control, for example.

    As a result of these possible sources of confusion, particularly when you
    cannot be sure an unambiguous format is in use, it is important to check
    that the result of reading a string as a date is not just valid but
    reasonable for the purpose for which it was supplied. If the result is
    outside some range of reasonable values, it may be worth getting the user to
    confirm their date selection, showing the date read from the string in a
    long format that does include month name and four-digit year, to make it
    easier for them to recognize any errors.

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
QDate QDate::fromString(const QString &string, QStringView format, int baseYear, QCalendar cal)
{
    QDate date;
#if QT_CONFIG(datetimeparser)
    QDateTimeParser dt(QMetaType::QDate, QDateTimeParser::FromString, cal);
    dt.setDefaultLocale(QLocale::c());
    if (dt.parseFormat(format))
        dt.fromString(string, &date, nullptr, baseYear);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
    Q_UNUSED(baseYear);
    Q_UNUSED(cal);
#endif
    return date;
}

/*!
    \fn QDate QDate::fromString(const QString &string, const QString &format, QCalendar cal)
    \overload
    \since 5.14
*/

/*!
    \fn QDate QDate::fromString(const QString &string, QStringView format, QCalendar cal)
    \overload
    \since 6.0
*/

/*!
    \fn QDate QDate::fromString(QStringView string, QStringView format, int baseYear, QCalendar cal)
    \overload
    \since 6.7
*/

/*!
    \fn QDate QDate::fromString(QStringView string, QStringView format, int baseYear)
    \overload
    \since 6.7

    Uses a default-constructed QCalendar.
*/

/*!
    \overload
    \since 6.7

    Uses a default-constructed QCalendar.
*/
QDate QDate::fromString(const QString &string, QStringView format, int baseYear)
{
    return fromString(string, format, baseYear, QCalendar());
}

/*!
    \fn QDate QDate::fromString(const QString &string, const QString &format, int baseYear)
    \overload
    \since 6.7

    Uses a default-constructed QCalendar.
*/
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

    \compares strong

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

    Returns a string representing the time.

    The \a format parameter determines the format of the result string. If the
    time is invalid, an empty string will be returned.

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
    \row \li z or zz
         \li The fractional part of the second, to go after a decimal point,
             without trailing zeroes. Thus \c{"s.z"} reports the seconds to full
             available (millisecond) precision without trailing zeroes (0 to
             999). For example, \c{"s.z"} would produce \c{"0.25"} for a time a
             quarter second into a minute.
    \row \li zzz
         \li The fractional part of the second, to millisecond precision,
             including trailing zeroes where applicable (000 to 999). For
             example, \c{"ss.zzz"} would produce \c{"00.250"} for a time a
             quarter second into a minute.
    \row \li AP or A
         \li Use AM/PM display. \c A/AP will be replaced by 'AM' or 'PM'. In
             localized forms (only relevant to \l{QLocale::toString()}), the
             locale-appropriate text is converted to upper-case.
    \row \li ap or a
         \li Use am/pm display. \c a/ap will be replaced by 'am' or 'pm'. In
             localized forms (only relevant to \l{QLocale::toString()}), the
             locale-appropriate text is converted to lower-case.
    \row \li aP or Ap
         \li Use AM/PM display (since 6.3). \c aP/Ap will be replaced by 'AM' or
             'PM'. In localized forms (only relevant to
             \l{QLocale::toString()}), the locale-appropriate text (returned by
             \l{QLocale::amText()} or \l{QLocale::pmText()}) is used without
             change of case.
    \row \li t
         \li The timezone abbreviation (for example "CEST"). Note that time zone
             abbreviations are not unique. In particular, \l fromString() cannot
             parse this.
    \row \li tt
         \li The timezone's offset from UTC with no colon between the hours and
             minutes (for example "+0200").
    \row \li ttt
         \li The timezone's offset from UTC with a colon between the hours and
             minutes (for example "+02:00").
    \row \li tttt
         \li The timezone name, as provided by \l QTimeZone::displayName() with
             the \l QTimeZone::LongName type. This may depend on the operating
             system in use. If no such name is available, the IANA ID of the
             zone (such as "Europe/Berlin") may be used.  It may give no
             indication of whether the datetime was in daylight-saving time or
             standard time, which may lead to ambiguity if the datetime falls in
             an hour repeated by a transition between the two.
    \endtable

    \note To get localized forms of AM or PM (the \c{AP}, \c{ap}, \c{A}, \c{a},
    \c{aP} or \c{Ap} formats) or of time zone representations (the \c{t}
    formats), use QLocale::system().toString().

    When the timezone cannot be determined or no suitable representation of it
    is available, the \c{t} forms to represent it may be skipped. See \l
    QTimeZone::displayName() for details of when it returns an empty string.

    Any non-empty sequence of characters enclosed in single quotes will be
    included verbatim in the output string (stripped of the quotes), even if it
    contains formatting characters. Two consecutive single quotes ("''") are
    replaced by a single quote in the output. All other characters in the format
    string are included verbatim in the output string.

    Formats without separators (e.g. "ddMM") are supported but must be used with
    care, as the resulting strings aren't always reliably readable (e.g. if "dM"
    produces "212" it could mean either the 2nd of December or the 21st of
    February).

    Example format strings (assuming that the QTime is 14:13:09.042)

    \table
    \header \li Format \li Result
    \row \li hh:mm:ss.zzz \li 14:13:09.042
    \row \li h:m:s ap     \li 2:13:9 pm
    \row \li H:m:s a      \li 14:13:9 pm
    \endtable

    \note If a format character is repeated more times than the longest
    expression in the table above using it, this part of the format will be read
    as several expressions with no separator between them; the longest above,
    possibly repeated as many times as there are copies of it, ending with a
    residue that may be a shorter expression. Thus \c{'HHHHH'} for the time
    08:00 will contribute \c{"08088"} to the output.

    \sa fromString(), QDate::toString(), QDateTime::toString(), QLocale::toString()
*/
// ### Qt 7 The 't' format specifiers should be specific to QDateTime (compare fromString).
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
    mds = ((h * MINS_PER_HOUR + m) * SECS_PER_MIN + s) * MSECS_PER_SEC + ms;
    Q_ASSERT(mds >= 0 && mds < MSECS_PER_DAY);
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
    if (isValid())
        t.mds = QRoundingDown::qMod<MSECS_PER_DAY>(ds() + ms);
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
    \fn bool QTime::operator==(const QTime &lhs, const QTime &rhs)

    Returns \c true if \a lhs is equal to \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator!=(const QTime &lhs, const QTime &rhs)

    Returns \c true if \a lhs is different from \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator<(const QTime &lhs, const QTime &rhs)

    Returns \c true if \a lhs is earlier than \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator<=(const QTime &lhs, const QTime &rhs)

    Returns \c true if \a lhs is earlier than or equal to \a rhs;
    otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator>(const QTime &lhs, const QTime &rhs)

    Returns \c true if \a lhs is later than \a rhs; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator>=(const QTime &lhs, const QTime &rhs)

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
    const qsizetype dot = string.indexOf(u'.'), comma = string.indexOf(u',');
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
    if (tail.isEmpty() ? dot != -1 || comma != -1 : !frac.ok())
        return QTime();
    Q_ASSERT(frac.ok() ^ tail.isEmpty());
    double fraction = frac.ok() ? frac.result * std::pow(0.1, tail.size()) : 0.0;

    const qsizetype size = string.size();
    if (size < 2 || size > 8)
        return QTime();

    ParsedInt hour = readInt(string.first(2));
    if (!hour.ok() || hour.result > (format == Qt::TextDate ? 23 : 24))
        return QTime();

    ParsedInt minute{};
    if (string.size() > 2) {
        if (string[2] == u':' && string.size() > 4)
            minute = readInt(string.sliced(3, 2));
        if (!minute.ok() || minute.result >= MINS_PER_HOUR)
            return QTime();
    } else if (format == Qt::TextDate) { // Requires minutes
        return QTime();
    } else if (frac.ok()) {
        Q_ASSERT(!(fraction < 0.0) && fraction < 1.0);
        fraction *= MINS_PER_HOUR;
        minute.result = qulonglong(fraction);
        fraction -= minute.result;
    }

    ParsedInt second{};
    if (string.size() > 5) {
        if (string[5] == u':' && string.size() == 8)
            second = readInt(string.sliced(6, 2));
        if (!second.ok() || second.result >= SECS_PER_MIN)
            return QTime();
    } else if (frac.ok()) {
        if (format == Qt::TextDate) // Doesn't allow fraction of minutes
            return QTime();
        Q_ASSERT(!(fraction < 0.0) && fraction < 1.0);
        fraction *= SECS_PER_MIN;
        second.result = qulonglong(fraction);
        fraction -= second.result;
    }

    Q_ASSERT(!(fraction < 0.0) && fraction < 1.0);
    // Round millis to nearest (unlike minutes and seconds, rounded down):
    int msec = frac.ok() ? qRound(MSECS_PER_SEC * fraction) : 0;
    // But handle overflow gracefully:
    if (msec == MSECS_PER_SEC) {
        // If we can (when data were otherwise valid) validly propagate overflow
        // into other fields, do so:
        if (isMidnight24 || hour.result < 23 || minute.result < 59 || second.result < 59) {
            msec = 0;
            if (++second.result == SECS_PER_MIN) {
                second.result = 0;
                if (++minute.result == MINS_PER_HOUR) {
                    minute.result = 0;
                    ++hour.result;
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
    if (hour.result == 24 && minute.result == 0 && second.result == 0 && msec == 0) {
        Q_ASSERT(format != Qt::TextDate); // It clipped hour at 23, above.
        if (isMidnight24)
            *isMidnight24 = true;
        hour.result = 0;
    }

    return QTime(hour.result, minute.result, second.result, msec);
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
    \row \li z or zz
         \li The fractional part of the second, as would usually follow a
             decimal point, without requiring trailing zeroes (0 to 999). Thus
             \c{"s.z"} matches the seconds with up to three digits of fractional
             part supplying millisecond precision, without needing trailing
             zeroes. For example, \c{"s.z"} would recognize either \c{"00.250"}
             or \c{"0.25"} as representing a time a quarter second into its
             minute.
    \row \li zzz
         \li Three digit fractional part of the second, to millisecond
             precision, including trailing zeroes where applicable (000 to 999).
             For example, \c{"ss.zzz"} would reject \c{"0.25"} but recognize
             \c{"00.250"} as representing a time a quarter second into its
             minute.
    \row \li AP, A, ap, a, aP or Ap
         \li Either 'AM' indicating a time before 12:00 or 'PM' for later times,
             matched case-insensitively.
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

    \note If localized forms of am or pm (the AP, ap, Ap, aP, A or a formats)
    are to be recognized, use QLocale::system().toTime().

    \note If a format character is repeated more times than the longest
    expression in the table above using it, this part of the format will be read
    as several expressions with no separator between them; the longest above,
    possibly repeated as many times as there are copies of it, ending with a
    residue that may be a shorter expression. Thus \c{'HHHHH'} would match
    \c{"08088"} or \c{"080808"} and set the hour to 8; if the time string
    contained "070809" it would "match" but produce an inconsistent result,
    leading to an invalid time.

    \sa toString(), QDateTime::fromString(), QDate::fromString(),
        QLocale::toTime(), QLocale::toDateTime()
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
    return (uint(h) < 24 && uint(m) < MINS_PER_HOUR && uint(s) < SECS_PER_MIN
            && uint(ms) < MSECS_PER_SEC);
}

/*****************************************************************************
  QDateTime static helper functions
 *****************************************************************************/

// get the types from QDateTime (through QDateTimePrivate)
typedef QDateTimePrivate::QDateTimeShortData ShortData;
typedef QDateTimePrivate::QDateTimeData QDateTimeData;

// Converts milliseconds since the start of 1970 into a date and/or time:
static qint64 msecsToJulianDay(qint64 msecs)
{
    return JULIAN_DAY_FOR_EPOCH + QRoundingDown::qDiv<MSECS_PER_DAY>(msecs);
}

static QDate msecsToDate(qint64 msecs)
{
    return QDate::fromJulianDay(msecsToJulianDay(msecs));
}

static QTime msecsToTime(qint64 msecs)
{
    return QTime::fromMSecsSinceStartOfDay(QRoundingDown::qMod<MSECS_PER_DAY>(msecs));
}

// True if combining days with millis overflows; otherwise, stores result in *sumMillis
// The inputs should not have opposite signs.
static inline bool daysAndMillisOverflow(qint64 days, qint64 millisInDay, qint64 *sumMillis)
{
    return qMulOverflow(days, std::integral_constant<qint64, MSECS_PER_DAY>(), sumMillis)
        || qAddOverflow(*sumMillis, millisInDay, sumMillis);
}

// Converts a date/time value into msecs
static qint64 timeToMSecs(QDate date, QTime time)
{
    qint64 days = date.toJulianDay() - JULIAN_DAY_FOR_EPOCH;
    qint64 msecs, dayms = time.msecsSinceStartOfDay();
    if (days < 0 && dayms > 0) {
        ++days;
        dayms -= MSECS_PER_DAY;
    }
    if (daysAndMillisOverflow(days, dayms, &msecs)) {
        using Bound = std::numeric_limits<qint64>;
        return days < 0 ? Bound::min() : Bound::max();
    }
    return msecs;
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
    static const auto bounds = QLocalTime::computeSystemMillisRange();
    return (bounds.minClip || millis >= bounds.min - slack)
        && (bounds.maxClip || millis <= bounds.max + slack);
}

/*!
    \internal
    Returns a year, in the system range, with the same day-of-week pattern

    Returns the number of a year, in the range supported by system time_t
    functions, that starts and ends on the same days of the week as \a year.
    This implies it is a leap year precisely if \a year is.  If year is before
    the epoch, a year early in the supported range is used; otherwise, one late
    in that range. For a leap year, this may be as much as 26 years years from
    the range's relevant end; for normal years at most a decade from the end.

    This ensures that any DST rules based on, e.g., the last Sunday in a
    particular month will select the same date in the returned year as they
    would if applied to \a year. Of course, the zone's rules may be different in
    \a year than in the selected year, but it's hard to do better.
*/
static int systemTimeYearMatching(int year)
{
#if defined(Q_OS_WIN) || defined(Q_OS_WASM)// They don't support times before the epoch
    static constexpr int forLeapEarly[] = { 1984, 1996, 1980, 1992, 1976, 1988, 1972 };
    static constexpr int regularEarly[] = { 1978, 1973, 1974, 1975, 1970, 1971, 1977 };
#else // First year fully in 32-bit time_t range is 1902
    static constexpr int forLeapEarly[] = { 1928, 1912, 1924, 1908, 1920, 1904, 1916 };
    static constexpr int regularEarly[] = { 1905, 1906, 1907, 1902, 1903, 1909, 1910 };
#endif
    static constexpr int forLeapLate[] = { 2012, 2024, 2036, 2020, 2032, 2016, 2028 };
    static constexpr int regularLate[] = { 2034, 2035, 2030, 2031, 2037, 2027, 2033 };
    const int dow = QGregorianCalendar::yearStartWeekDay(year);
    Q_ASSERT(dow == QDate(year, 1, 1).dayOfWeek());
    const int res = (QGregorianCalendar::leapTest(year)
                     ? (year < 1970 ? forLeapEarly : forLeapLate)
                     : (year < 1970 ? regularEarly : regularLate))[dow == 7 ? 0 : dow];
    Q_ASSERT(QDate(res, 1, 1).dayOfWeek() == dow);
    Q_ASSERT(QDate(res, 12, 31).dayOfWeek() == QDate(year, 12, 31).dayOfWeek());
    return res;
}

// Sets up d and status to represent local time at the given UTC msecs since epoch:
QDateTimePrivate::ZoneState QDateTimePrivate::expressUtcAsLocal(qint64 utcMSecs)
{
    ZoneState result{utcMSecs};
    // Within the time_t supported range, localtime() can handle it:
    if (millisInSystemRange(utcMSecs)) {
        result = QLocalTime::utcToLocal(utcMSecs);
        if (result.valid)
            return result;
    }

    // Docs state any LocalTime after 2038-01-18 *will* have any DST applied.
    // When this falls outside the supported range, we need to fake it.
#if QT_CONFIG(timezone) // Use the system time-zone.
    if (const auto sys = QTimeZone::systemTimeZone(); sys.isValid()) {
        result.offset = sys.d->offsetFromUtc(utcMSecs);
        if (result.offset != QTimeZonePrivate::invalidSeconds()) {
            if (qAddOverflow(utcMSecs, result.offset * MSECS_PER_SEC, &result.when))
                return result;
            result.dst = sys.d->isDaylightTime(utcMSecs) ? DaylightTime : StandardTime;
            result.valid = true;
            return result;
        }
    }
#endif // timezone

    // Kludge
    // Do the conversion in a year with the same days of the week, so DST
    // dates might be right, and adjust by the number of days that was off:
    const qint64 jd = msecsToJulianDay(utcMSecs);
    const auto ymd = QGregorianCalendar::partsFromJulian(jd);
    qint64 diffMillis, fakeUtc;
    const auto fakeJd = QGregorianCalendar::julianFromParts(systemTimeYearMatching(ymd.year),
                                                            ymd.month, ymd.day);
    if (Q_UNLIKELY(!fakeJd
                   || qMulOverflow(jd - *fakeJd, std::integral_constant<qint64, MSECS_PER_DAY>(),
                                   &diffMillis)
                   || qSubOverflow(utcMSecs, diffMillis, &fakeUtc))) {
        return result;
    }

    result = QLocalTime::utcToLocal(fakeUtc);
    // Now correct result.when for the use of the fake date:
    if (!result.valid || qAddOverflow(result.when, diffMillis, &result.when)) {
        // If utcToLocal() failed, its return has the fake when; restore utcMSecs.
        // Fail on overflow, but preserve offset and DST-ness.
        result.when = utcMSecs;
        result.valid = false;
    }
    return result;
}

static auto millisToWithinRange(qint64 millis)
{
    struct R {
        qint64 shifted = 0;
        bool good = false;
    } result;
    qint64 jd = msecsToJulianDay(millis);
    auto ymd = QGregorianCalendar::partsFromJulian(jd);
    const auto fakeJd = QGregorianCalendar::julianFromParts(systemTimeYearMatching(ymd.year),
                                                            ymd.month, ymd.day);
    result.good = fakeJd && !daysAndMillisOverflow(*fakeJd - jd, millis, &result.shifted);
    return result;
}

/*!
    \internal
    \enum QDateTimePrivate::TransitionOption

    This enumeration is used to resolve datetime combinations which fall in \l
    {Timezone transitions}. The transition is described as a "gap" if there are
    time representations skipped over by the zone, as is common in the "spring
    forward" transitions in many zones on entering daylight-saving time. The
    transition is described as a "fold" if there are time representations
    repeated in the zone, as in a "fall back" transition out of daylight-saving
    time.

    When the options specified do not determine a resolution for a datetime, it
    is marked invalid.

    The prepared option sets above are in fact composed from low-level atomic
    options. For each of gap and fold you can chose between two candidate times,
    one before or after the transition, based on the time requested; or you can
    pick the moment of transition, or the start or end of the transition
    interval. For a gap, the start and end of the interval are the moment of the
    transition, but for a repeated interval the start of the first pass is the
    start of the transition interval, the end of the second pass is the end of
    the transition interval and the moment of the transition itself is both the
    end of the first pass and the start of the second.

    \value GapUseBefore For a time in a gap, use a time before the transition,
                        as if stepping back from a later time.
    \value GapUseAfter For a time in a gap, use a time after the transition, as
                       if stepping forward from an earlier time.
    \value FoldUseBefore For a repeated time, use the first candidate, which is
                         before the transition.
    \value FoldUseAfter For a repeated time, use the second candidate, which is
                        after the transition.
    \value FlipForReverseDst For "reversed" DST, this reverses the preceding
                             four options (see below).

    The last has no effect unless the "daylight-saving" time side of the
    transition is known to have a lower offset from UTC than the standard time
    side. (This is the "reversed" DST case of \l {Timezone transitions}.)  In
    that case, if other options would select a time after the transition, a time
    before is used instead, and vice versa. This effectively turns a preference
    for the side with lower offset into a preference for the side that is
    officially standard time, even if it has higher offset; and conversely a
    preference for higher offset into a preference for daylight-saving time,
    even if it has a lower offset. This option has no effect on a resolution
    that selects the moment of transition or the start or end of the transition
    interval.

    The result of combining more than one of the \c GapUse* options is
    undefined; likewise for the \c FoldUse*. Each of QDateTime's
    TransitionResolution values, aside from Reject, maps to a combination that
    incorporates one from each of these sets.
*/

constexpr static QDateTimePrivate::TransitionOptions
toTransitionOptions(QDateTime::TransitionResolution res)
{
    switch (res) {
    case QDateTime::TransitionResolution::RelativeToBefore:
        return QDateTimePrivate::GapUseAfter | QDateTimePrivate::FoldUseBefore;
    case QDateTime::TransitionResolution::RelativeToAfter:
        return QDateTimePrivate::GapUseBefore | QDateTimePrivate::FoldUseAfter;
    case QDateTime::TransitionResolution::PreferBefore:
        return QDateTimePrivate::GapUseBefore | QDateTimePrivate::FoldUseBefore;
    case QDateTime::TransitionResolution::PreferAfter:
        return QDateTimePrivate::GapUseAfter | QDateTimePrivate::FoldUseAfter;
    case QDateTime::TransitionResolution::PreferStandard:
        return QDateTimePrivate::GapUseBefore
            | QDateTimePrivate::FoldUseAfter
            | QDateTimePrivate::FlipForReverseDst;
    case QDateTime::TransitionResolution::PreferDaylightSaving:
        return QDateTimePrivate::GapUseAfter
            | QDateTimePrivate::FoldUseBefore
            | QDateTimePrivate::FlipForReverseDst;
    case QDateTime::TransitionResolution::Reject: break;
    }
    return {};
}

constexpr static QDateTimePrivate::TransitionOptions
toTransitionOptions(QDateTimePrivate::DaylightStatus dst)
{
    return toTransitionOptions(dst == QDateTimePrivate::DaylightTime
                               ? QDateTime::TransitionResolution::PreferDaylightSaving
                               : QDateTime::TransitionResolution::PreferStandard);
}

QString QDateTimePrivate::localNameAtMillis(qint64 millis, DaylightStatus dst)
{
    const QDateTimePrivate::TransitionOptions resolve = toTransitionOptions(dst);
    QString abbreviation;
    if (millisInSystemRange(millis, MSECS_PER_DAY)) {
        abbreviation = QLocalTime::localTimeAbbbreviationAt(millis, resolve);
        if (!abbreviation.isEmpty())
            return abbreviation;
    }

    // Otherwise, outside the system range.
#if QT_CONFIG(timezone)
    // Use the system zone:
    const auto sys = QTimeZone::systemTimeZone();
    if (sys.isValid()) {
        ZoneState state = zoneStateAtMillis(sys, millis, resolve);
        if (state.valid)
            return sys.d->abbreviation(state.when - state.offset * MSECS_PER_SEC);
    }
#endif // timezone

    // Kludge
    // Use a time in the system range with the same day-of-week pattern to its year:
    auto fake = millisToWithinRange(millis);
    if (Q_LIKELY(fake.good))
        return QLocalTime::localTimeAbbbreviationAt(fake.shifted, resolve);

    // Overflow, apparently.
    return {};
}

// Determine the offset from UTC at the given local time as millis.
QDateTimePrivate::ZoneState QDateTimePrivate::localStateAtMillis(
    qint64 millis, QDateTimePrivate::TransitionOptions resolve)
{
    // First, if millis is within a day of the viable range, try mktime() in
    // case it does fall in the range and gets useful information:
    if (millisInSystemRange(millis, MSECS_PER_DAY)) {
        auto result = QLocalTime::mapLocalTime(millis, resolve);
        if (result.valid)
            return result;
    }

    // Otherwise, outside the system range.
#if QT_CONFIG(timezone)
    // Use the system zone:
    const auto sys = QTimeZone::systemTimeZone();
    if (sys.isValid())
        return zoneStateAtMillis(sys, millis, resolve);
#endif // timezone

    // Kludge
    // Use a time in the system range with the same day-of-week pattern to its year:
    auto fake = millisToWithinRange(millis);
    if (Q_LIKELY(fake.good)) {
        auto result = QLocalTime::mapLocalTime(fake.shifted, resolve);
        if (result.valid) {
            qint64 adjusted;
            if (Q_UNLIKELY(qAddOverflow(result.when, millis - fake.shifted, &adjusted))) {
                using Bound = std::numeric_limits<qint64>;
                adjusted = millis < fake.shifted ? Bound::min() : Bound::max();
            }
            result.when = adjusted;
        } else {
            result.when = millis;
        }
        return result;
    }
    // Overflow, apparently.
    return {millis};
}

#if QT_CONFIG(timezone)
// For a TimeZone and a time expressed in zone msecs encoding, compute the
// actual DST-ness and offset, adjusting the time if needed to escape a
// spring-forward.
QDateTimePrivate::ZoneState QDateTimePrivate::zoneStateAtMillis(
    const QTimeZone &zone, qint64 millis, QDateTimePrivate::TransitionOptions resolve)
{
    Q_ASSERT(zone.isValid());
    Q_ASSERT(zone.timeSpec() == Qt::TimeZone);
    return zone.d->stateAtZoneTime(millis, resolve);
}
#endif // timezone

static inline QDateTimePrivate::ZoneState stateAtMillis(const QTimeZone &zone, qint64 millis,
                                                        QDateTimePrivate::TransitionOptions resolve)
{
    if (zone.timeSpec() == Qt::LocalTime)
        return QDateTimePrivate::localStateAtMillis(millis, resolve);
#if QT_CONFIG(timezone)
    if (zone.timeSpec() == Qt::TimeZone && zone.isValid())
        return QDateTimePrivate::zoneStateAtMillis(zone, millis, resolve);
#endif
    return {millis};
}

static inline bool specCanBeSmall(Qt::TimeSpec spec)
{
    return spec == Qt::LocalTime || spec == Qt::UTC;
}

static inline bool msecsCanBeSmall(qint64 msecs)
{
    if constexpr (!QDateTimeData::CanBeSmall)
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
    if (status.testFlag(QDateTimePrivate::SetToDaylightTime))
        return QDateTimePrivate::DaylightTime;
    if (status.testFlag(QDateTimePrivate::SetToStandardTime))
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
   See QDateTime's comparison operators and areFarEnoughApart().
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
    Q_UNREACHABLE_RETURN(false);
}

/* Even datetimes with different offset can be ordered by their getMSecs()
   provided the difference is bigger than the largest difference in offset we're
   prepared to believe in. Technically, it may be possible to construct a zone
   with an offset outside the range and get wrong results - but the answer to
   someone doing that is that their contrived timezone and its consequences are
   their own responsibility.

   If two datetimes' millis lie within the offset range of one another, we can't
   take any short-cuts, even if they're in the same zone, because there may be a
   zone transition between them. (The full 32-hour difference would only arise
   before 1845, for one date-time in The Philippines, the other in Alaska.)
*/
bool areFarEnoughApart(qint64 leftMillis, qint64 rightMillis)
{
    constexpr quint64 UtcOffsetMillisRange
        = quint64(QTimeZone::MaxUtcOffsetSecs - QTimeZone::MinUtcOffsetSecs) * MSECS_PER_SEC;
    qint64 gap = 0;
    return qSubOverflow(leftMillis, rightMillis, &gap) || QtPrivate::qUnsignedAbs(gap) > UtcOffsetMillisRange;
}

// Refresh the LocalTime or TimeZone validity and offset
static void refreshZonedDateTime(QDateTimeData &d, const QTimeZone &zone,
                                 QDateTimePrivate::TransitionOptions resolve)
{
    Q_ASSERT(zone.timeSpec() == Qt::TimeZone || zone.timeSpec() == Qt::LocalTime);
    auto status = getStatus(d);
    Q_ASSERT(extractSpec(status) == zone.timeSpec());
    int offsetFromUtc = 0;
    /* Callers are:
       * QDTP::create(), where d is too new to be shared yet
       * reviseTimeZone(), which detach()es if not short before calling this
       * checkValidDateTime(), always follows a setDateTime() that detach()ed if not short

       So we can assume d is not shared. We only need to detach() if we convert
       from short to pimpled to accommodate an oversize msecs, which can only be
       needed in the unlikely event we revise it.
    */

    // If not valid date and time then is invalid
    if (!status.testFlags(QDateTimePrivate::ValidDate | QDateTimePrivate::ValidTime)) {
        status.setFlag(QDateTimePrivate::ValidDateTime, false);
    } else {
        // We have a valid date and time and a Qt::LocalTime or Qt::TimeZone
        // that might fall into a "missing" DST transition hour.
        qint64 msecs = getMSecs(d);
        QDateTimePrivate::ZoneState state = stateAtMillis(zone, msecs, resolve);
        Q_ASSERT(!state.valid || (state.offset >= -SECS_PER_DAY && state.offset <= SECS_PER_DAY));
        if (state.dst == QDateTimePrivate::UnknownDaylightTime) { // Overflow
            status.setFlag(QDateTimePrivate::ValidDateTime, false);
        } else if (state.valid) {
            status = mergeDaylightStatus(status, state.dst);
            offsetFromUtc = state.offset;
            status.setFlag(QDateTimePrivate::ValidDateTime, true);
            if (Q_UNLIKELY(msecs != state.when)) {
                // Update msecs to the resolution:
                if (status.testFlag(QDateTimePrivate::ShortData)) {
                    if (msecsCanBeSmall(state.when)) {
                        d.data.msecs = qintptr(state.when);
                    } else {
                        // Convert to long-form so we can hold the revised msecs:
                        status.setFlag(QDateTimePrivate::ShortData, false);
                        d.detach();
                    }
                }
                if (!status.testFlag(QDateTimePrivate::ShortData))
                    d->m_msecs = state.when;
            }
        } else {
            status.setFlag(QDateTimePrivate::ValidDateTime, false);
        }
    }

    if (status.testFlag(QDateTimePrivate::ShortData)) {
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
    Q_ASSERT(QTimeZone::isUtcOrFixedOffset(extractSpec(status)));
    status.setFlag(QDateTimePrivate::ValidDateTime,
                   status.testFlags(QDateTimePrivate::ValidDate | QDateTimePrivate::ValidTime));

    if (status.testFlag(QDateTimePrivate::ShortData))
        d.data.status = status.toInt();
    else
        d->m_status = status;
}

// Clean up and set status after assorted set-up or reworking:
static void checkValidDateTime(QDateTimeData &d, QDateTime::TransitionResolution resolve)
{
    auto spec = extractSpec(getStatus(d));
    switch (spec) {
    case Qt::OffsetFromUTC:
    case Qt::UTC:
        // for these, a valid date and a valid time imply a valid QDateTime
        refreshSimpleDateTime(d);
        break;
    case Qt::TimeZone:
    case Qt::LocalTime:
        // For these, we need to check whether (the zone is valid and) the time
        // is valid for the zone. Expensive, but we have no other option.
        refreshZonedDateTime(d, d.timeZone(), toTransitionOptions(resolve));
        break;
    }
}

static void reviseTimeZone(QDateTimeData &d, const QTimeZone &zone,
                           QDateTime::TransitionResolution resolve)
{
    Qt::TimeSpec spec = zone.timeSpec();
    auto status = mergeSpec(getStatus(d), spec);
    bool reuse = d.isShort();
    int offset = 0;

    switch (spec) {
    case Qt::UTC:
        Q_ASSERT(zone.fixedSecondsAheadOfUtc() == 0);
        break;
    case Qt::OffsetFromUTC:
        reuse = false;
        offset = zone.fixedSecondsAheadOfUtc();
        Q_ASSERT(offset);
        break;
    case Qt::TimeZone:
        reuse = false;
        break;
    case Qt::LocalTime:
        break;
    }

    status &= ~(QDateTimePrivate::ValidDateTime | QDateTimePrivate::DaylightMask);
    if (reuse) {
        d.data.status = status.toInt();
    } else {
        d.detach();
        d->m_status = status & ~QDateTimePrivate::ShortData;
        d->m_offsetFromUtc = offset;
#if QT_CONFIG(timezone)
        if (spec == Qt::TimeZone)
            d->m_timeZone = zone;
#endif // timezone
    }

    if (QTimeZone::isUtcOrFixedOffset(spec))
        refreshSimpleDateTime(d);
    else
        refreshZonedDateTime(d, zone, toTransitionOptions(resolve));
}

static void setDateTime(QDateTimeData &d, QDate date, QTime time)
{
    // If the date is valid and the time is not we set time to 00:00:00
    if (!time.isValid() && date.isValid())
        time = QTime::fromMSecsSinceStartOfDay(0);

    QDateTimePrivate::StatusFlags newStatus = { };

    // Set date value and status
    qint64 days = 0;
    if (date.isValid()) {
        days = date.toJulianDay() - JULIAN_DAY_FOR_EPOCH;
        newStatus = QDateTimePrivate::ValidDate;
    }

    // Set time value and status
    int ds = 0;
    if (time.isValid()) {
        ds = time.msecsSinceStartOfDay();
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
    if (daysAndMillisOverflow(days, qint64(ds), &msecs)) {
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

static std::pair<QDate, QTime> getDateTime(const QDateTimeData &d)
{
    auto status = getStatus(d);
    const qint64 msecs = getMSecs(d);
    const auto dayMilli = QRoundingDown::qDivMod<MSECS_PER_DAY>(msecs);
    return { status.testFlag(QDateTimePrivate::ValidDate)
            ? QDate::fromJulianDay(JULIAN_DAY_FOR_EPOCH + dayMilli.quotient)
            : QDate(),
            status.testFlag(QDateTimePrivate::ValidTime)
            ? QTime::fromMSecsSinceStartOfDay(dayMilli.remainder)
            : QTime() };
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

inline QDateTime::Data::Data(const QTimeZone &zone)
{
    Qt::TimeSpec spec = zone.timeSpec();
    if (CanBeSmall && Q_LIKELY(specCanBeSmall(spec))) {
        quintptr value = mergeSpec(QDateTimePrivate::ShortData, spec).toInt();
        d = reinterpret_cast<QDateTimePrivate *>(value);
        Q_ASSERT(isShort());
    } else {
        // the structure is too small, we need to detach
        d = new QDateTimePrivate;
        d->ref.ref();
        d->m_status = mergeSpec({}, spec);
        if (spec == Qt::OffsetFromUTC)
            d->m_offsetFromUtc = zone.fixedSecondsAheadOfUtc();
        else if (spec == Qt::TimeZone)
            d->m_timeZone = zone;
        Q_ASSERT(!isShort());
    }
}

inline QDateTime::Data::Data(const Data &other) noexcept
    : data(other.data)
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

inline QDateTime::Data::Data(Data &&other) noexcept
    : data(other.data)
{
    // reset the other to a short state
    Data dummy;
    Q_ASSERT(dummy.isShort());
    other.data = dummy.data;
}

inline QDateTime::Data &QDateTime::Data::operator=(const Data &other) noexcept
{
    if (isShort() ? data == other.data : d == other.d)
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
    Q_ASSERT(b || !d->m_status.testFlag(QDateTimePrivate::ShortData));

    // even if CanBeSmall = false, we have short data for a default-constructed
    // QDateTime object. But it's unlikely.
    if constexpr (CanBeSmall)
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

void QDateTime::Data::invalidate()
{
    if (isShort()) {
        data.status &= ~int(QDateTimePrivate::ValidityMask);
    } else {
        detach();
        d->m_status &= ~QDateTimePrivate::ValidityMask;
    }
}

QTimeZone QDateTime::Data::timeZone() const
{
    switch (getSpec(*this)) {
    case Qt::UTC:
        return QTimeZone::UTC;
    case Qt::OffsetFromUTC:
        return QTimeZone::fromSecondsAheadOfUtc(d->m_offsetFromUtc);
    case Qt::TimeZone:
#if QT_CONFIG(timezone)
        if (d->m_timeZone.isValid())
            return d->m_timeZone;
#endif
        break;
    case Qt::LocalTime:
        return QTimeZone::LocalTime;
    }
    return QTimeZone();
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
QDateTime::Data QDateTimePrivate::create(QDate toDate, QTime toTime, const QTimeZone &zone,
                                         QDateTime::TransitionResolution resolve)
{
    QDateTime::Data result(zone);
    setDateTime(result, toDate, toTime);
    if (zone.isUtcOrFixedOffset())
        refreshSimpleDateTime(result);
    else
        refreshZonedDateTime(result, zone, toTransitionOptions(resolve));
    return result;
}

/*****************************************************************************
  QDateTime member functions
 *****************************************************************************/

/*!
    \class QDateTime
    \inmodule QtCore
    \ingroup shared
    \reentrant
    \brief The QDateTime class provides date and time functions.

    \compares weak

    A QDateTime object encodes a calendar date and a clock time (a "datetime")
    in accordance with a time representation. It combines features of the QDate
    and QTime classes. It can read the current datetime from the system
    clock. It provides functions for comparing datetimes and for manipulating a
    datetime by adding a number of seconds, days, months, or years.

    QDateTime can describe datetimes with respect to \l{Qt::LocalTime}{local
    time}, to \l{Qt::UTC}{UTC}, to a specified \l{Qt::OffsetFromUTC}{offset from
    UTC} or to a specified \l{Qt::TimeZone}{time zone}. Each of these time
    representations can be encapsulated in a suitable instance of the QTimeZone
    class. For example, a time zone of "Europe/Berlin" will apply the
    daylight-saving rules as used in Germany. In contrast, a fixed offset from
    UTC of +3600 seconds is one hour ahead of UTC (usually written in ISO
    standard notation as "UTC+01:00"), with no daylight-saving
    complications. When using either local time or a specified time zone,
    time-zone transitions (see \l {Timezone transitions}{below}) are taken into
    account. A QDateTime's timeSpec() will tell you which of the four types of
    time representation is in use; its timeRepresentation() provides a full
    description of that time representation, as a QTimeZone.

    A QDateTime object is typically created either by giving a date and time
    explicitly in the constructor, or by using a static function such as
    currentDateTime() or fromMSecsSinceEpoch(). The date and time can be changed
    with setDate() and setTime(). A datetime can also be set using the
    setMSecsSinceEpoch() function that takes the time, in milliseconds, since
    the start, in UTC, of the year 1970. The fromString() function returns a
    QDateTime, given a string and a date format used to interpret the date
    within the string.

    QDateTime::currentDateTime() returns a QDateTime that expresses the current
    date and time with respect to a specific time representation, such as local
    time (its default). QDateTime::currentDateTimeUtc() returns a QDateTime that
    expresses the current date and time with respect to UTC; it is equivalent to
    \c {QDateTime::currentDateTime(QTimeZone::UTC)}.

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

    Use toTimeZone() to re-express a datetime in terms of a different time
    representation. By passing a lightweight QTimeZone that represents local
    time, UTC or a fixed offset from UTC, you can convert the datetime to use
    the corresponding time representation; or you can pass a full time zone
    (whose \l {QTimeZone::timeSpec()}{timeSpec()} is \c {Qt::TimeZone}) to use
    that instead.

    \section1 Remarks

    QDateTime does not account for leap seconds.

    All conversions  to and from string formats are done using the C locale.
    For localized conversions, see QLocale.

    There is no year 0 in the Gregorian calendar. Dates in that year are
    considered invalid. The year -1 is the year "1 before Christ" or "1 before
    common era." The day before 1 January 1 CE is 31 December 1 BCE.

    Using local time (the default) or a specified time zone implies a need
    to resolve any issues around \l {Timezone transitions}{transitions}. As a
    result, operations on such QDateTime instances (notably including
    constructing them) may be more expensive than the equivalent when using UTC
    or a fixed offset from it.

    \section2 Range of Valid Dates

    The range of values that QDateTime can represent is dependent on the
    internal storage implementation. QDateTime is currently stored in a qint64
    as a serial msecs value encoding the date and time. This restricts the date
    range to about ±292 million years, compared to the QDate range of ±2 billion
    years. Care must be taken when creating a QDateTime with extreme values that
    you do not overflow the storage. The exact range of supported values varies
    depending on the time representation used.

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
    transitions, notably including DST. However, building Qt with the ICU
    library will equip QTimeZone with the same timezone database as is used on
    Unix.

    \section2 Timezone transitions

    QDateTime takes into account timezone transitions, both the transitions
    between Standard Time and Daylight-Saving Time (DST) and the transitions
    that arise when a zone changes its standard offset. For example, if the
    transition is at 2am and the clock goes forward to 3am, then there is a
    "missing" hour from 02:00:00 to 02:59:59.999. Such a transition is known as
    a "spring forward" and the times skipped over have no meaning. When a
    transition goes the other way, known as a "fall back", a time interval is
    repeated, first in the old zone (usually DST), then in the new zone (usually
    Standard Time), so times in this interval are ambiguous.

    Some zones use "reversed" DST, using standard time in summer and
    daylight-saving time (with a lowered offset) in winter. For such zones, the
    spring forward still happens in spring and skips an hour, but is a
    transition \e{out of} daylight-saving time, while the fall back still
    repeats an autumn hour but is a transition \e to daylight-saving time.

    When converting from a UTC time (or a time at fixed offset from UTC), there
    is always an unambiguous valid result in any timezone. However, when
    combining a date and time to make a datetime, expressed with respect to
    local time or a specific time-zone, the nominal result may fall in a
    transition, making it either invalid or ambiguous. Methods where this
    situation may arise take a \c resolve parameter: this is always ignored if
    the requested datetime is valid and unambiguous. See \l TransitionResolution
    for the options it lets you control. Prior to Qt 6.7, the equivalent of its
    \l LegacyBehavior was selected.

    For a spring forward's skipped interval, interpreting the requested time
    with either offset yields an actual time at which the other offset was in
    use; so passing \c TransitionResolution::RelativeToBefore for \c resolve
    will actually result in a time after the transition, that would have had the
    requested representation had the transition not happened. Likewise, \c
    TransitionResolution::RelativeToAfter for \c resolve results in a time
    before the transition, that would have had the requested representation, had
    the transition happened earlier.

    When QDateTime performs arithmetic, as with addDay() or addSecs(), it takes
    care to produce a valid result. For example, on a day when there is a spring
    forward from 02:00 to 03:00, adding one second to 01:59:59 will get
    03:00:00. Adding one day to 02:30 on the preceding day will get 03:30 on the
    day of the transition, while subtracting one day, by calling \c{addDay(-1)},
    to 02:30 on the following day will get 01:30 on the day of the transition.
    While addSecs() will deliver a time offset by the given number of seconds,
    addDays() adjusts the date and only adjusts time if it would otherwise get
    an invalid result. Applying \c{addDays(1)} to 03:00 on the day before the
    spring-forward will simply get 03:00 on the day of the transition, even
    though the latter is only 23 hours after the former; but \c{addSecs(24 * 60
    * 60)} will get 04:00 on the day of the transition, since that's 24 hours
    later. Typical transitions make some days 23 or 25 hours long.

    For datetimes that the system \c time_t can represent (from 1901-12-14 to
    2038-01-18 on systems with 32-bit \c time_t; for the full range QDateTime
    can represent if the type is 64-bit), the standard system APIs are used to
    determine local time's offset from UTC. For datetimes not handled by these
    system APIs (potentially including some within the \c time_t range),
    QTimeZone::systemTimeZone() is used, if available, or a best effort is made
    to estimate. In any case, the offset information used depends on the system
    and may be incomplete or, for past times, historically
    inaccurate. Furthermore, for future dates, the local time zone's offsets and
    DST rules may change before that date comes around.

    \section3 Whole day transitions

    A small number of zones have skipped or repeated entire days as part of
    moving The International Date Line across themselves. For these, daysTo()
    will be unaware of the duplication or gap, simply using the difference in
    calendar date; in contrast, msecsTo() and secsTo() know the true time
    interval. Likewise, addMSecs() and addSecs() correspond directly to elapsed
    time, where addDays(), addMonths() and addYears() follow the nominal
    calendar, aside from where landing in a gap or duplication requires
    resolving an ambiguity or invalidity due to a duplication or omission.

    \note Days "lost" during a change of calendar, such as from Julian to
    Gregorian, do not affect QDateTime. Although the two calendars describe
    dates differently, the successive days across the change are described by
    consecutive QDate instances, each one day later than the previous, as
    described by either calendar or by their toJulianDay() values. In contrast,
    a zone skipping or duplicating a day is changing its description of \e time,
    not date, for all that it does so by a whole 24 hours.

    \section2 Offsets From UTC

    Offsets from UTC are measured in seconds east of Greenwich. The moment
    described by a particular date and time, such as noon on a particular day,
    depends on the time representation used. Those with a higher offset from UTC
    describe an earlier moment, and those with a lower offset a later moment, by
    any given combination of date and time.

    There is no explicit size restriction on an offset from UTC, but there is an
    implicit limit imposed when using the toString() and fromString() methods
    which use a ±hh:mm format, effectively limiting the range to ± 99 hours and
    59 minutes and whole minutes only. Note that currently no time zone has an
    offset outside the range of ±14 hours and all known offsets are multiples of
    five minutes. Historical time zones have a wider range and may have offsets
    including seconds; these last cannot be faithfully represented in strings.

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
    \since 6.7
    \enum QDateTime::TransitionResolution

    This enumeration is used to resolve datetime combinations which fall in \l
    {Timezone transitions}.

    When constructing a datetime, specified in terms of local time or a
    time-zone that has daylight-saving time, or revising one with setDate(),
    setTime() or setTimeZone(), the given parameters may imply a time
    representation that either has no meaning or has two meanings in the
    zone. Such time representations are described as being in the transition. In
    either case, we can simply return an invalid datetime, to indicate that the
    operation is ill-defined. In the ambiguous case, we can alternatively select
    one of the two times that could be meant. When there is no meaning, we can
    select a time either side of it that might plausibly have been meant.  For
    example, when advancing from an earlier time, we can select the time after
    the transition that is actually the specified amount of time after the
    earlier time in question. The options specified here configure how such
    selection is performed.

    \value Reject
           Treat any time in a transition as invalid. Either it really is, or it
           is ambiguous.
    \value RelativeToBefore
           Selects a time as if stepping forward from a time before the
           transition. This interprets the requested time using the offset in
           effect before the transition and, if necessary, converts the result
           to the offset in effect at the resulting time.
    \value RelativeToAfter
           Select a time as if stepping backward from a time after the
           transition. This interprets the requested time using the offset in
           effect after the transition and, if necessary, converts the result to
           the offset in effect at the resulting time.
    \value PreferBefore
           Selects a time before the transition,
    \value PreferAfter
           Selects a time after the transition.
    \value PreferStandard
           Selects a time on the standard time side of the transition.
    \value PreferDaylightSaving
           Selects a time on the daylight-saving-time side of the transition.
    \value LegacyBehavior
           An alias for RelativeToBefore, which is used as default for
           TransitionResolution parameters, as this most closely matches the
           behavior prior to Qt 6.7.

    For \l addDays(), \l addMonths() or \l addYears(), the behavior is and
    (mostly) was to use \c RelativeToBefore if adding a positive adjustment and \c
    RelativeToAfter if adding a negative adjustment.

    \note In time zones where daylight-saving increases the offset from UTC in
    summer (known as "positive DST"), PreferStandard is an alias for
    RelativeToAfter and PreferDaylightSaving for RelativeToBefore. In time zones
    where the daylight-saving mechanism is a decrease in offset from UTC in
    winter (known as "negative DST"), the reverse applies, provided the
    operating system reports - as it does on most platforms - whether a datetime
    is in DST or standard time. For some platforms, where transition times are
    unavailable even for Qt::TimeZone datetimes, QTimeZone is obliged to presume
    that the side with lower offset from UTC is standard time, effectively
    assuming positive DST.

    The following tables illustrate how a QDateTime constructor resolves a
    request for 02:30 on a day when local time has a transition between 02:00
    and 03:00, with a nominal standard time LST and daylight-saving time LDT on
    the two sides, in the various possible cases. The transition type may be to
    skip an hour or repeat it. The type of transition and value of a parameter
    \c resolve determine which actual time on the given date is selected. First,
    the common case of positive daylight-saving, where:

    \table
    \header \li Before \li 02:00--03:00 \li After \li \c resolve \li selected
    \row \li LST \li skip \li LDT \li RelativeToBefore \li 03:30 LDT
    \row \li LST \li skip \li LDT \li RelativeToAfter \li 01:30 LST
    \row \li LST \li skip \li LDT \li PreferBefore \li 01:30 LST
    \row \li LST \li skip \li LDT \li PreferAfter \li 03:30 LDT
    \row \li LST \li skip \li LDT \li PreferStandard \li 01:30 LST
    \row \li LST \li skip \li LDT \li PreferDaylightSaving \li 03:30 LDT
    \row \li LDT \li repeat \li LST \li RelativeToBefore \li 02:30 LDT
    \row \li LDT \li repeat \li LST \li RelativeToAfter \li 02:30 LST
    \row \li LDT \li repeat \li LST \li PreferBefore \li 02:30 LDT
    \row \li LDT \li repeat \li LST \li PreferAfter \li 02:30 LST
    \row \li LDT \li repeat \li LST \li PreferStandard \li 02:30 LST
    \row \li LDT \li repeat \li LST \li PreferDaylightSaving \li 02:30 LDT
    \endtable

    Second, the case for negative daylight-saving, using LDT in winter and
    skipping an hour to transition to LST in summer, then repeating an hour at
    the transition back to winter:

    \table
    \row \li LDT \li skip \li LST \li RelativeToBefore \li 03:30 LST
    \row \li LDT \li skip \li LST \li RelativeToAfter \li 01:30 LDT
    \row \li LDT \li skip \li LST \li PreferBefore \li 01:30 LDT
    \row \li LDT \li skip \li LST \li PreferAfter \li 03:30 LST
    \row \li LDT \li skip \li LST \li PreferStandard \li 03:30 LST
    \row \li LDT \li skip \li LST \li PreferDaylightSaving \li 01:30 LDT
    \row \li LST \li repeat \li LDT \li RelativeToBefore \li 02:30 LST
    \row \li LST \li repeat \li LDT \li RelativeToAfter \li 02:30 LDT
    \row \li LST \li repeat \li LDT \li PreferBefore \li 02:30 LST
    \row \li LST \li repeat \li LDT \li PreferAfter \li 02:30 LDT
    \row \li LST \li repeat \li LDT \li PreferStandard \li 02:30 LST
    \row \li LST \li repeat \li LDT \li PreferDaylightSaving \li 02:30 LDT
    \endtable

    Reject can be used to prompt relevant QDateTime APIs to return an invalid
    datetime object so that your code can deal with transitions for itself, for
    example by alerting a user to the fact that the datetime they have selected
    is in a transition interval, to offer them the opportunity to resolve a
    conflict or ambiguity. Code using this may well find the other options above
    useful to determine relevant information to use in its own (or the user's)
    resolution. If the start or end of the transition, or the moment of the
    transition itself, is the right resolution, QTimeZone's transition APIs can
    be used to obtain that information. You can determine whether the transition
    is a repeated or skipped interval by using \l secsTo() to measure the actual
    time between noon on the previous and following days. The result will be
    less than 48 hours for a skipped interval (such as a spring-forward) and
    more than 48 hours for a repeated interval (such as a fall-back).

    \note When a resolution other than Reject is specified, a valid QDateTime
    object is returned, if possible. If the requested date-time falls in a gap,
    the returned date-time will not have the time() requested - or, in some
    cases, the date(), if a whole day was skipped. You can thus detect when a
    gap is hit by comparing date() and time() to what was requested.

    \section2 Relation to other datetime software

    The Python programming language's datetime APIs have a \c fold parameter
    that corresponds to \c RelativeToBefore (\c{fold = True}) and \c
    RelativeToAfter (\c{fold = False}).

    The \c Temporal proposal to replace JavaScript's \c Date offers four options
    for how to resolve a transition, as value for a \c disambiguation
    parameter. Its \c{'reject'} raises an exception, which roughly corresponds
    to \c Reject producing an invalid result. Its \c{'earlier'} and \c{'later'}
    options correspond to \c PreferBefore and \c PreferAfter. Its
    \c{'compatible'} option corresponds to \c RelativeToBefore (and Python's
    \c{fold = True}).

    \sa {Timezone transitions}, QDateTime::TransitionResolution
*/

/*!
    Constructs a null datetime, nominally using local time.

    A null datetime is invalid, since its date and time are invalid.

    \sa isValid(), setMSecsSinceEpoch(), setDate(), setTime(), setTimeZone()
*/
QDateTime::QDateTime() noexcept
{
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0) || defined(QT_BOOTSTRAPPED) || QT_POINTER_SIZE == 8
    static_assert(sizeof(ShortData) == sizeof(qint64));
    static_assert(sizeof(Data) == sizeof(qint64));
#endif
    static_assert(sizeof(ShortData) >= sizeof(void*), "oops, Data::swap() is broken!");
}

#if QT_DEPRECATED_SINCE(6, 9)
/*!
    \deprecated [6.9] Use \c{QDateTime(date, time)} or \c{QDateTime(date, time, QTimeZone::fromSecondsAheadOfUtc(offsetSeconds))}.

    Constructs a datetime with the given \a date and \a time, using the time
    representation implied by \a spec and \a offsetSeconds seconds.

    If \a date is valid and \a time is not, the time will be set to midnight.

    If \a spec is not Qt::OffsetFromUTC then \a offsetSeconds will be
    ignored. If \a spec is Qt::OffsetFromUTC and \a offsetSeconds is 0 then the
    timeSpec() will be set to Qt::UTC, i.e. an offset of 0 seconds.

    If \a spec is Qt::TimeZone then the spec will be set to Qt::LocalTime,
    i.e. the current system time zone.  To create a Qt::TimeZone datetime
    use the correct constructor.

    If \a date lies outside the range of dates representable by QDateTime, the
    result is invalid. If \a spec is Qt::LocalTime and the system's time-zone
    skipped over the given date and time, the result is invalid.
*/
QDateTime::QDateTime(QDate date, QTime time, Qt::TimeSpec spec, int offsetSeconds)
    : d(QDateTimePrivate::create(date, time, asTimeZone(spec, offsetSeconds, "QDateTime"),
                                 TransitionResolution::LegacyBehavior))
{
}
#endif // 6.9 deprecation

/*!
    \since 5.2

    Constructs a datetime with the given \a date and \a time, using the time
    representation described by \a timeZone.

    If \a date is valid and \a time is not, the time will be set to midnight.
    If \a timeZone is invalid then the datetime will be invalid. If \a date and
    \a time describe a moment close to a transition for \a timeZone, \a resolve
    controls how that situation is resolved.

//! [pre-resolve-note]
    \note Prior to Qt 6.7, the version of this function lacked the \a resolve
    parameter so had no way to resolve the ambiguities related to transitions.
//! [pre-resolve-note]
*/

QDateTime::QDateTime(QDate date, QTime time, const QTimeZone &timeZone, TransitionResolution resolve)
    : d(QDateTimePrivate::create(date, time, timeZone, resolve))
{
}

/*!
    \since 6.5
    \overload

    Constructs a datetime with the given \a date and \a time, using local time.

    If \a date is valid and \a time is not, midnight will be used as the
    time. If \a date and \a time describe a moment close to a transition for
    local time, \a resolve controls how that situation is resolved.

    \include qdatetime.cpp pre-resolve-note
*/

QDateTime::QDateTime(QDate date, QTime time, TransitionResolution resolve)
    : d(QDateTimePrivate::create(date, time, QTimeZone::LocalTime, resolve))
{
}

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
    Copies the \a other datetime into this and returns this copy.
*/

QDateTime &QDateTime::operator=(const QDateTime &other) noexcept
{
    d = other.d;
    return *this;
}
/*!
    \fn void QDateTime::swap(QDateTime &other)
    \since 5.0
    \memberswap{datetime}
*/

/*!
    Returns \c true if both the date and the time are null; otherwise
    returns \c false. A null datetime is invalid.

    \sa QDate::isNull(), QTime::isNull(), isValid()
*/

bool QDateTime::isNull() const
{
    // If date or time is invalid, we don't set datetime valid.
    return !getStatus(d).testAnyFlag(QDateTimePrivate::ValidityMask);
}

/*!
    Returns \c true if this datetime represents a definite moment, otherwise \c false.

    A datetime is valid if both its date and its time are valid and the time
    representation used gives a valid meaning to their combination. When the
    time representation is a specific time-zone or local time, there may be
    times on some dates that the zone skips in its representation, as when a
    daylight-saving transition skips an hour (typically during a night in
    spring). For example, if DST ends at 2am with the clock advancing to 3am,
    then datetimes from 02:00:00 to 02:59:59.999 on that day are invalid.

    \sa QDateTime::YearRange, QDate::isValid(), QTime::isValid()
*/

bool QDateTime::isValid() const
{
    return getStatus(d).testFlag(QDateTimePrivate::ValidDateTime);
}

/*!
    Returns the date part of the datetime.

    \sa setDate(), time(), timeRepresentation()
*/

QDate QDateTime::date() const
{
    return getStatus(d).testFlag(QDateTimePrivate::ValidDate) ? msecsToDate(getMSecs(d)) : QDate();
}

/*!
    Returns the time part of the datetime.

    \sa setTime(), date(), timeRepresentation()
*/

QTime QDateTime::time() const
{
    return getStatus(d).testFlag(QDateTimePrivate::ValidTime) ? msecsToTime(getMSecs(d)) : QTime();
}

/*!
    Returns the time specification of the datetime.

    This classifies its time representation as local time, UTC, a fixed offset
    from UTC (without indicating the offset) or a time zone (without giving the
    details of that time zone). Equivalent to
    \c{timeRepresentation().timeSpec()}.

    \sa setTimeZone(), timeRepresentation(), date(), time()
*/

Qt::TimeSpec QDateTime::timeSpec() const
{
    return getSpec(d);
}

/*!
    \since 6.5
    Returns a QTimeZone identifying how this datetime represents time.

    The timeSpec() of the returned QTimeZone will coincide with that of this
    datetime; if it is not Qt::TimeZone then the returned QTimeZone is a time
    representation. When their timeSpec() is Qt::OffsetFromUTC, the returned
    QTimeZone's fixedSecondsAheadOfUtc() supplies the offset.  When timeSpec()
    is Qt::TimeZone, the QTimeZone object itself is the full representation of
    that time zone.

    \sa timeZone(), setTimeZone(), QTimeZone::asBackendZone()
*/

QTimeZone QDateTime::timeRepresentation() const
{
    return d.timeZone();
}

#if QT_CONFIG(timezone)
/*!
    \since 5.2

    Returns the time zone of the datetime.

    The result is the same as \c{timeRepresentation().asBackendZone()}. In all
    cases, the result's \l {QTimeZone::timeSpec()}{timeSpec()} is Qt::TimeZone.

    When timeSpec() is Qt::LocalTime, the result will describe local time at the
    time this method was called. It will not reflect subsequent changes to the
    system time zone, even when the QDateTime from which it was obtained does.

    \sa timeRepresentation(), setTimeZone(), Qt::TimeSpec, QTimeZone::asBackendZone()
*/

QTimeZone QDateTime::timeZone() const
{
    return d.timeZone().asBackendZone();
}
#endif // timezone

/*!
    \since 5.2

    Returns this datetime's Offset From UTC in seconds.

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

    \sa setTimeZone()
*/

int QDateTime::offsetFromUtc() const
{
    const auto status = getStatus(d);
    if (!status.testFlags(QDateTimePrivate::ValidDate | QDateTimePrivate::ValidTime))
        return 0;
    // But allow invalid date-time (e.g. gap's resolution) to report its offset.
    if (!d.isShort())
        return d->m_offsetFromUtc;

    auto spec = extractSpec(status);
    if (spec == Qt::LocalTime) {
        // We didn't cache the value, so we need to calculate it:
        const auto resolve = toTransitionOptions(extractDaylightStatus(status));
        return QDateTimePrivate::localStateAtMillis(getMSecs(d), resolve).offset;
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
    \li For Qt::OffsetFromUTC it will be in the format "UTC±00:00".
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
        return "UTC"_L1;
    case Qt::OffsetFromUTC:
        return "UTC"_L1 + toOffsetString(Qt::ISODate, d->m_offsetFromUtc);
    case Qt::TimeZone:
#if !QT_CONFIG(timezone)
        break;
#else
        Q_ASSERT(d->m_timeZone.isValid());
        return d->m_timeZone.abbreviation(*this);
#endif // timezone
    case Qt::LocalTime:
#if defined(Q_OS_WIN) && QT_CONFIG(timezone)
        // MS's tzname is a full MS-name, not an abbreviation:
        if (QString sys = QTimeZone::systemTimeZone().abbreviation(*this); !sys.isEmpty())
            return sys;
        // ... but, even so, a full name isn't as bad as empty.
#endif
        return QDateTimePrivate::localNameAtMillis(getMSecs(d),
                                                   extractDaylightStatus(getStatus(d)));
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
        if (auto dst = extractDaylightStatus(getStatus(d));
            dst != QDateTimePrivate::UnknownDaylightTime) {
            return dst == QDateTimePrivate::DaylightTime;
        }
        return d->m_timeZone.d->isDaylightTime(toMSecsSinceEpoch());
#endif // timezone
    case Qt::LocalTime: {
        auto dst = extractDaylightStatus(getStatus(d));
        if (dst == QDateTimePrivate::UnknownDaylightTime) {
            dst = QDateTimePrivate::localStateAtMillis(
                getMSecs(d), toTransitionOptions(TransitionResolution::LegacyBehavior)).dst;
        }
        return dst == QDateTimePrivate::DaylightTime;
        }
    }
    return false;
}

/*!
    Sets the date part of this datetime to \a date.

    If no time is set yet, it is set to midnight. If \a date is invalid, this
    QDateTime becomes invalid.

    If \a date and time() describe a moment close to a transition for this
    datetime's time representation, \a resolve controls how that situation is
    resolved.

    \include qdatetime.cpp pre-resolve-note

    \sa date(), setTime(), setTimeZone()
*/

void QDateTime::setDate(QDate date, TransitionResolution resolve)
{
    setDateTime(d, date, time());
    checkValidDateTime(d, resolve);
}

/*!
    Sets the time part of this datetime to \a time. If \a time is not valid,
    this function sets it to midnight. Therefore, it's possible to clear any
    set time in a QDateTime by setting it to a default QTime:

    \code
        QDateTime dt = QDateTime::currentDateTime();
        dt.setTime(QTime());
    \endcode

    If date() and \a time describe a moment close to a transition for this
    datetime's time representation, \a resolve controls how that situation is
    resolved.

    \include qdatetime.cpp pre-resolve-note

    \sa time(), setDate(), setTimeZone()
*/

void QDateTime::setTime(QTime time, TransitionResolution resolve)
{
    setDateTime(d, date(), time);
    checkValidDateTime(d, resolve);
}

#if QT_DEPRECATED_SINCE(6, 9)
/*!
    \deprecated [6.9] Use setTimeZone() instead

    Sets the time specification used in this datetime to \a spec.
    The datetime may refer to a different point in time.

    If \a spec is Qt::OffsetFromUTC then the timeSpec() will be set
    to Qt::UTC, i.e. an effective offset of 0.

    If \a spec is Qt::TimeZone then the spec will be set to Qt::LocalTime,
    i.e. the current system time zone.

    Example:
    \snippet code/src_corelib_time_qdatetime.cpp 19

    \sa setTimeZone(), timeSpec(), toTimeSpec(), setDate(), setTime()
*/

void QDateTime::setTimeSpec(Qt::TimeSpec spec)
{
    reviseTimeZone(d, asTimeZone(spec, 0, "QDateTime::setTimeSpec"),
                   TransitionResolution::LegacyBehavior);
}

/*!
    \since 5.2
    \deprecated [6.9] Use setTimeZone(QTimeZone::fromSecondsAheadOfUtc(offsetSeconds)) instead

    Sets the timeSpec() to Qt::OffsetFromUTC and the offset to \a offsetSeconds.
    The datetime may refer to a different point in time.

    The maximum and minimum offset is 14 positive or negative hours.  If
    \a offsetSeconds is larger or smaller than that, then the result is
    undefined.

    If \a offsetSeconds is 0 then the timeSpec() will be set to Qt::UTC.

    \sa setTimeZone(), isValid(), offsetFromUtc(), toOffsetFromUtc()
*/

void QDateTime::setOffsetFromUtc(int offsetSeconds)
{
    reviseTimeZone(d, QTimeZone::fromSecondsAheadOfUtc(offsetSeconds),
                   TransitionResolution::Reject);
}
#endif // 6.9 deprecations

/*!
    \since 5.2

    Sets the time zone used in this datetime to \a toZone.

    The datetime may refer to a different point in time. It uses the time
    representation of \a toZone, which may change the meaning of its unchanged
    date() and time().

    If \a toZone is invalid then the datetime will be invalid. Otherwise, this
    datetime's timeSpec() after the call will match \c{toZone.timeSpec()}.

    If date() and time() describe a moment close to a transition for \a toZone,
    \a resolve controls how that situation is resolved.

    \include qdatetime.cpp pre-resolve-note

    \sa timeRepresentation(), timeZone(), Qt::TimeSpec
*/

void QDateTime::setTimeZone(const QTimeZone &toZone, TransitionResolution resolve)
{
    reviseTimeZone(d, toZone, resolve);
}

/*!
    \since 4.7

    Returns the datetime as a number of milliseconds after the start, in UTC, of
    the year 1970.

    On systems that do not support time zones, this function will
    behave as if local time were Qt::UTC.

    The behavior for this function is undefined if the datetime stored in
    this object is not valid. However, for all valid dates, this function
    returns a unique value.

    \sa toSecsSinceEpoch(), setMSecsSinceEpoch(), fromMSecsSinceEpoch()
*/
qint64 QDateTime::toMSecsSinceEpoch() const
{
    // Note: QDateTimeParser relies on this producing a useful result, even when
    // !isValid(), at least when the invalidity is a time in a fall-back (that
    // we'll have adjusted to lie outside it, but marked invalid because it's
    // not what was asked for). Other things may be doing similar. But that's
    // only relevant when we got enough data for resolution to find it invalid.
    const auto status = getStatus(d);
    if (!status.testFlags(QDateTimePrivate::ValidDate | QDateTimePrivate::ValidTime))
        return 0;

    switch (extractSpec(status)) {
    case Qt::UTC:
        return getMSecs(d);

    case Qt::OffsetFromUTC:
        Q_ASSERT(!d.isShort());
        return d->m_msecs - d->m_offsetFromUtc * MSECS_PER_SEC;

    case Qt::LocalTime:
        if (status.testFlag(QDateTimePrivate::ShortData)) {
            // Short form has nowhere to cache the offset, so recompute.
            const auto resolve = toTransitionOptions(extractDaylightStatus(getStatus(d)));
            const auto state = QDateTimePrivate::localStateAtMillis(getMSecs(d), resolve);
            return state.when - state.offset * MSECS_PER_SEC;
        }
        // Use the offset saved by refreshZonedDateTime() on creation.
        return d->m_msecs - d->m_offsetFromUtc * MSECS_PER_SEC;

    case Qt::TimeZone:
        Q_ASSERT(!d.isShort());
#if QT_CONFIG(timezone)
        // Use offset refreshZonedDateTime() saved on creation:
        if (d->m_timeZone.isValid())
            return d->m_msecs - d->m_offsetFromUtc * MSECS_PER_SEC;
#endif
        return 0;
    }
    Q_UNREACHABLE_RETURN(0);
}

/*!
    \since 5.8

    Returns the datetime as a number of seconds after the start, in UTC, of the
    year 1970.

    On systems that do not support time zones, this function will
    behave as if local time were Qt::UTC.

    The behavior for this function is undefined if the datetime stored in
    this object is not valid. However, for all valid dates, this function
    returns a unique value.

    \sa toMSecsSinceEpoch(), fromSecsSinceEpoch(), setSecsSinceEpoch()
*/
qint64 QDateTime::toSecsSinceEpoch() const
{
    return toMSecsSinceEpoch() / MSECS_PER_SEC;
}

/*!
    \since 4.7

    Sets the datetime to represent a moment a given number, \a msecs, of
    milliseconds after the start, in UTC, of the year 1970.

    On systems that do not support time zones, this function will
    behave as if local time were Qt::UTC.

    Note that passing the minimum of \c qint64
    (\c{std::numeric_limits<qint64>::min()}) to \a msecs will result in
    undefined behavior.

    \sa setSecsSinceEpoch(), toMSecsSinceEpoch(), fromMSecsSinceEpoch()
*/
void QDateTime::setMSecsSinceEpoch(qint64 msecs)
{
    auto status = getStatus(d);
    const auto spec = extractSpec(status);
    Q_ASSERT(specCanBeSmall(spec) || !d.isShort());
    QDateTimePrivate::ZoneState state(msecs);

    status &= ~QDateTimePrivate::ValidityMask;
    if (QTimeZone::isUtcOrFixedOffset(spec)) {
        if (spec == Qt::OffsetFromUTC)
            state.offset = d->m_offsetFromUtc;
        if (!state.offset || !qAddOverflow(msecs, state.offset * MSECS_PER_SEC, &state.when))
            status |= QDateTimePrivate::ValidityMask;
    } else if (spec == Qt::LocalTime) {
        state = QDateTimePrivate::expressUtcAsLocal(msecs);
        if (state.valid)
            status = mergeDaylightStatus(status | QDateTimePrivate::ValidityMask, state.dst);
#if QT_CONFIG(timezone)
    } else if (spec == Qt::TimeZone && (d.detach(), d->m_timeZone.isValid())) {
        const auto data = d->m_timeZone.d->data(msecs);
        if (Q_LIKELY(data.offsetFromUtc != QTimeZonePrivate::invalidSeconds())) {
            state.offset = data.offsetFromUtc;
            Q_ASSERT(state.offset >= -SECS_PER_DAY && state.offset <= SECS_PER_DAY);
            if (!state.offset
                || !Q_UNLIKELY(qAddOverflow(msecs, state.offset * MSECS_PER_SEC, &state.when))) {
                d->m_status = mergeDaylightStatus(status | QDateTimePrivate::ValidityMask,
                                                  data.daylightTimeOffset
                                                  ? QDateTimePrivate::DaylightTime
                                                  : QDateTimePrivate::StandardTime);
                d->m_msecs = state.when;
                d->m_offsetFromUtc = state.offset;
                return;
            } // else: zone can't represent this UTC time
        } // else: zone unable to represent given UTC time (should only happen on overflow).
#endif // timezone
    }
    Q_ASSERT(!status.testFlag(QDateTimePrivate::ValidDateTime)
             || (state.offset >= -SECS_PER_DAY && state.offset <= SECS_PER_DAY));

    if (msecsCanBeSmall(state.when) && d.isShort()) {
        // we can keep short
        d.data.msecs = qintptr(state.when);
        d.data.status = status.toInt();
    } else {
        d.detach();
        d->m_status = status & ~QDateTimePrivate::ShortData;
        d->m_msecs = state.when;
        d->m_offsetFromUtc = state.offset;
    }
}

/*!
    \since 5.8

    Sets the datetime to represent a moment a given number, \a secs, of seconds
    after the start, in UTC, of the year 1970.

    On systems that do not support time zones, this function will
    behave as if local time were Qt::UTC.

    \sa setMSecsSinceEpoch(), toSecsSinceEpoch(), fromSecsSinceEpoch()
*/
void QDateTime::setSecsSinceEpoch(qint64 secs)
{
    qint64 msecs;
    if (!qMulOverflow(secs, std::integral_constant<qint64, MSECS_PER_SEC>(), &msecs))
        setMSecsSinceEpoch(msecs);
    else
        d.invalidate();
}

#if QT_CONFIG(datestring) // depends on, so implies, textdate
/*!
    \overload

    Returns the datetime as a string in the \a format given.

    If the \a format is Qt::TextDate, the string is formatted in the default
    way. The day and month names will be in English. An example of this
    formatting is "Wed May 20 03:40:13 1998". For localized formatting, see
    \l{QLocale::toString()}.

    If the \a format is Qt::ISODate, the string format corresponds to the ISO
    8601 extended specification for representations of dates and times, taking
    the form yyyy-MM-ddTHH:mm:ss[Z|±HH:mm], depending on the timeSpec() of the
    QDateTime. If the timeSpec() is Qt::UTC, Z will be appended to the string;
    if the timeSpec() is Qt::OffsetFromUTC, the offset in hours and minutes from
    UTC will be appended to the string. To include milliseconds in the ISO 8601
    date, use the \a format Qt::ISODateWithMs, which corresponds to
    yyyy-MM-ddTHH:mm:ss.zzz[Z|±HH:mm].

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
        const std::pair<QDate, QTime> p = getDateTime(d);
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
            buf += " UTC"_L1;
#else
            buf += " GMT"_L1;
#endif
            if (getSpec(d) == Qt::OffsetFromUTC)
                buf += toOffsetString(Qt::TextDate, offsetFromUtc());
        }
        return buf;
    }
    case Qt::ISODate:
    case Qt::ISODateWithMs: {
        const std::pair<QDate, QTime> p = getDateTime(d);
        buf = toStringIsoDate(p.first);
        if (buf.isEmpty())
            return QString();   // failed to convert
        buf += u'T' + p.second.toString(format);
        switch (getSpec(d)) {
        case Qt::UTC:
            buf += u'Z';
            break;
        case Qt::OffsetFromUTC:
        case Qt::TimeZone:
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
    \since 5.14

    Returns the datetime as a string. The \a format parameter determines the
    format of the result string. If \a cal is supplied, it determines the
    calendar used to represent the date; it defaults to Gregorian. Prior to Qt
    5.14, there was no \a cal parameter and the Gregorian calendar was always
    used. See QTime::toString() and QDate::toString() for the supported
    specifiers for time and date, respectively, in the \a format parameter.

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

    \note Day and month names as well as AM/PM indicators are given in English
    (C locale).  To get localized month and day names and localized forms of
    AM/PM, use QLocale::system().toDateTime().

    \sa fromString(), QDate::toString(), QTime::toString(), QLocale::toString()
*/
QString QDateTime::toString(QStringView format, QCalendar cal) const
{
    return QLocale::c().toString(*this, format, cal);
}

// Out-of-line no-calendar overloads, since QCalendar is a non-trivial type
/*!
    \overload
    \since 5.10
*/
QString QDateTime::toString(QStringView format) const
{
    return QLocale::c().toString(*this, format, QCalendar());
}

/*!
    \overload
    \since 4.6
*/
QString QDateTime::toString(const QString &format) const
{
    return QLocale::c().toString(*this, qToStringViewIgnoringNull(format), QCalendar());
}
#endif // datestring

static inline void massageAdjustedDateTime(QDateTimeData &d, QDate date, QTime time, bool forward)
{
    const QDateTimePrivate::TransitionOptions resolve = toTransitionOptions(
        forward ? QDateTime::TransitionResolution::RelativeToBefore
                : QDateTime::TransitionResolution::RelativeToAfter);
    auto status = getStatus(d);
    Q_ASSERT(status.testFlags(QDateTimePrivate::ValidDate | QDateTimePrivate::ValidTime
                              | QDateTimePrivate::ValidDateTime));
    auto spec = extractSpec(status);
    if (QTimeZone::isUtcOrFixedOffset(spec)) {
        setDateTime(d, date, time);
        refreshSimpleDateTime(d);
        return;
    }
    qint64 local = timeToMSecs(date, time);
    const QDateTimePrivate::ZoneState state = stateAtMillis(d.timeZone(), local, resolve);
    Q_ASSERT(state.valid || state.dst == QDateTimePrivate::UnknownDaylightTime);
    if (state.dst == QDateTimePrivate::UnknownDaylightTime)
        status.setFlag(QDateTimePrivate::ValidDateTime, false);
    else
        status = mergeDaylightStatus(status | QDateTimePrivate::ValidDateTime, state.dst);

    if (status & QDateTimePrivate::ShortData) {
        d.data.msecs = state.when;
        d.data.status = status.toInt();
    } else {
        d.detach();
        d->m_status = status;
        if (state.valid) {
            d->m_msecs = state.when;
            d->m_offsetFromUtc = state.offset;
        }
    }
}

/*!
    Returns a QDateTime object containing a datetime \a ndays days
    later than the datetime of this object (or earlier if \a ndays is
    negative).

    If the timeSpec() is Qt::LocalTime or Qt::TimeZone and the resulting date
    and time fall in the Standard Time to Daylight-Saving Time transition hour
    then the result will be just beyond this gap, in the direction of change.
    If the transition is at 2am and the clock goes forward to 3am, the result of
    aiming between 2am and 3am will be adjusted to fall before 2am (if \c{ndays
    < 0}) or after 3am (otherwise).

    \sa daysTo(), addMonths(), addYears(), addSecs(), {Timezone transitions}
*/

QDateTime QDateTime::addDays(qint64 ndays) const
{
    if (isNull())
        return QDateTime();

    QDateTime dt(*this);
    std::pair<QDate, QTime> p = getDateTime(d);
    massageAdjustedDateTime(dt.d, p.first.addDays(ndays), p.second, ndays >= 0);
    return dt;
}

/*!
    Returns a QDateTime object containing a datetime \a nmonths months
    later than the datetime of this object (or earlier if \a nmonths
    is negative).

    If the timeSpec() is Qt::LocalTime or Qt::TimeZone and the resulting date
    and time fall in the Standard Time to Daylight-Saving Time transition hour
    then the result will be just beyond this gap, in the direction of change.
    If the transition is at 2am and the clock goes forward to 3am, the result of
    aiming between 2am and 3am will be adjusted to fall before 2am (if
    \c{nmonths < 0}) or after 3am (otherwise).

    \sa daysTo(), addDays(), addYears(), addSecs(), {Timezone transitions}
*/

QDateTime QDateTime::addMonths(int nmonths) const
{
    if (isNull())
        return QDateTime();

    QDateTime dt(*this);
    std::pair<QDate, QTime> p = getDateTime(d);
    massageAdjustedDateTime(dt.d, p.first.addMonths(nmonths), p.second, nmonths >= 0);
    return dt;
}

/*!
    Returns a QDateTime object containing a datetime \a nyears years
    later than the datetime of this object (or earlier if \a nyears is
    negative).

    If the timeSpec() is Qt::LocalTime or Qt::TimeZone and the resulting date
    and time fall in the Standard Time to Daylight-Saving Time transition hour
    then the result will be just beyond this gap, in the direction of change.
    If the transition is at 2am and the clock goes forward to 3am, the result of
    aiming between 2am and 3am will be adjusted to fall before 2am (if \c{nyears
    < 0}) or after 3am (otherwise).

    \sa daysTo(), addDays(), addMonths(), addSecs(), {Timezone transitions}
*/

QDateTime QDateTime::addYears(int nyears) const
{
    if (isNull())
        return QDateTime();

    QDateTime dt(*this);
    std::pair<QDate, QTime> p = getDateTime(d);
    massageAdjustedDateTime(dt.d, p.first.addYears(nyears), p.second, nyears >= 0);
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
    if (qMulOverflow(s, std::integral_constant<qint64, MSECS_PER_SEC>(), &msecs))
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
        if (!qAddOverflow(toMSecsSinceEpoch(), msecs, &msecs))
            dt.setMSecsSinceEpoch(msecs);
        else
            dt.d.invalidate();
        break;
    case Qt::UTC:
    case Qt::OffsetFromUTC:
        // No need to convert, just add on
        if (qAddOverflow(getMSecs(d), msecs, &msecs)) {
            dt.d.invalidate();
        } else if (d.isShort() && msecsCanBeSmall(msecs)) {
            dt.d.data.msecs = qintptr(msecs);
        } else {
            dt.d.detach();
            dt.d->m_msecs = msecs;
        }
        break;
    }
    return dt;
}

/*!
    \fn QDateTime QDateTime::addDuration(std::chrono::milliseconds msecs) const

    \since 6.4

    Returns a QDateTime object containing a datetime \a msecs milliseconds
    later than the datetime of this object (or earlier if \a msecs is
    negative).

    If this datetime is invalid, an invalid datetime will be returned.

    \note Adding durations expressed in \c{std::chrono::months} or
    \c{std::chrono::years} does not yield the same result obtained by using
    addMonths() or addYears(). The former are fixed durations, calculated in
    relation to the solar year; the latter use the Gregorian calendar definitions
    of months/years.

    \sa addMSecs(), msecsTo(), addDays(), addMonths(), addYears()
*/

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
    \fn std::chrono::milliseconds QDateTime::operator-(const QDateTime &lhs, const QDateTime &rhs)
    \since 6.4

    Returns the number of milliseconds between \a lhs and \a rhs.
    If \a lhs is earlier than \a rhs, the result will be negative.

    Returns 0 if either datetime is invalid.

    \sa msecsTo()
*/

/*!
    \fn QDateTime QDateTime::operator+(const QDateTime &dateTime, std::chrono::milliseconds duration)
    \fn QDateTime QDateTime::operator+(std::chrono::milliseconds duration, const QDateTime &dateTime)

    \since 6.4

    Returns a QDateTime object containing a datetime \a duration milliseconds
    later than \a dateTime (or earlier if \a duration is negative).

    If \a dateTime is invalid, an invalid datetime will be returned.

    \sa addMSecs()
*/

/*!
    \fn QDateTime &QDateTime::operator+=(std::chrono::milliseconds duration)
    \since 6.4

    Modifies this datetime object by adding the given \a duration.
    The updated object will be later if \a duration is positive,
    or earlier if it is negative.

    If this datetime is invalid, this function has no effect.

    Returns a reference to this datetime object.

    \sa addMSecs()
*/

/*!
    \fn QDateTime QDateTime::operator-(const QDateTime &dateTime, std::chrono::milliseconds duration)

    \since 6.4

    Returns a QDateTime object containing a datetime \a duration milliseconds
    earlier than \a dateTime (or later if \a duration is negative).

    If \a dateTime is invalid, an invalid datetime will be returned.

    \sa addMSecs()
*/

/*!
    \fn QDateTime &QDateTime::operator-=(std::chrono::milliseconds duration)
    \since 6.4

    Modifies this datetime object by subtracting the given \a duration.
    The updated object will be earlier if \a duration is positive,
    or later if it is negative.

    If this datetime is invalid, this function has no effect.

    Returns a reference to this datetime object.

    \sa addMSecs
*/

#if QT_DEPRECATED_SINCE(6, 9)
/*!
    \deprecated [6.9] Use \l toTimeZone() instead.

    Returns a copy of this datetime converted to the given time \a spec.

    The result represents the same moment in time as, and is equal to, this datetime.

    If \a spec is Qt::OffsetFromUTC then it is set to Qt::UTC. To set to a fixed
    offset from UTC, use toTimeZone() or toOffsetFromUtc().

    If \a spec is Qt::TimeZone then it is set to Qt::LocalTime, i.e. the local
    Time Zone. To set a specified time-zone, use toTimeZone().

    Example:
    \snippet code/src_corelib_time_qdatetime.cpp 16

    \sa setTimeSpec(), timeSpec(), toTimeZone()
*/

QDateTime QDateTime::toTimeSpec(Qt::TimeSpec spec) const
{
    return toTimeZone(asTimeZone(spec, 0, "toTimeSpec"));
}
#endif // 6.9 deprecation

/*!
    \since 5.2

    Returns a copy of this datetime converted to a spec of Qt::OffsetFromUTC
    with the given \a offsetSeconds. Equivalent to
    \c{toTimeZone(QTimeZone::fromSecondsAheadOfUtc(offsetSeconds))}.

    If the \a offsetSeconds equals 0 then a UTC datetime will be returned.

    The result represents the same moment in time as, and is equal to, this datetime.

    \sa offsetFromUtc(), toTimeZone()
*/

QDateTime QDateTime::toOffsetFromUtc(int offsetSeconds) const
{
    return toTimeZone(QTimeZone::fromSecondsAheadOfUtc(offsetSeconds));
}

/*!
    Returns a copy of this datetime converted to local time.

    The result represents the same moment in time as, and is equal to, this datetime.

    Example:

    \snippet code/src_corelib_time_qdatetime.cpp 17

    \sa toTimeZone(), toUTC(), toOffsetFromUtc()
*/
QDateTime QDateTime::toLocalTime() const
{
    return toTimeZone(QTimeZone::LocalTime);
}

/*!
    Returns a copy of this datetime converted to UTC.

    The result represents the same moment in time as, and is equal to, this datetime.

    Example:

    \snippet code/src_corelib_time_qdatetime.cpp 18

    \sa toTimeZone(), toLocalTime(), toOffsetFromUtc()
*/
QDateTime QDateTime::toUTC() const
{
    return toTimeZone(QTimeZone::UTC);
}

/*!
    \since 5.2

    Returns a copy of this datetime converted to the given \a timeZone.

    The result represents the same moment in time as, and is equal to, this datetime.

    The result describes the moment in time in terms of \a timeZone's time
    representation. For example:

    \snippet code/src_corelib_time_qdatetime.cpp 23

    If \a timeZone is invalid then the datetime will be invalid. Otherwise the
    returned datetime's timeSpec() will match \c{timeZone.timeSpec()}.

    \sa timeRepresentation(), toLocalTime(), toUTC(), toOffsetFromUtc()
*/

QDateTime QDateTime::toTimeZone(const QTimeZone &timeZone) const
{
    if (timeRepresentation() == timeZone)
        return *this;

    if (!isValid()) {
        QDateTime ret = *this;
        ret.setTimeZone(timeZone);
        return ret;
    }

    return fromMSecsSinceEpoch(toMSecsSinceEpoch(), timeZone);
}

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

    const qint64 thisMs = getMSecs(d);
    const qint64 yourMs = getMSecs(other.d);
    if (usesSameOffset(d, other.d) || areFarEnoughApart(thisMs, yourMs))
        return thisMs == yourMs;

    // Convert to UTC and compare
    return toMSecsSinceEpoch() == other.toMSecsSinceEpoch();
}

/*!
    \fn bool QDateTime::operator==(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs represents the same moment in time as \a rhs;
    otherwise returns \c false.

//! [datetime-order-details]
    Two datetimes using different time representations can have different
    offsets from UTC. In this case, they may compare equivalent even if their \l
    date() and \l time() differ, if that difference matches the difference in
    UTC offset. If their \c date() and \c time() coincide, the one with higher
    offset from UTC is less (earlier) than the one with lower offset. As a
    result, datetimes are only weakly ordered.

    Since 5.14, all invalid datetimes are equivalent and less than all valid
    datetimes.
//! [datetime-order-details]

    \sa operator!=(), operator<(), operator<=(), operator>(), operator>=()
*/

/*!
    \fn bool QDateTime::operator!=(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs is different from \a rhs; otherwise returns \c
    false.

    \include qdatetime.cpp datetime-order-details

    \sa operator==()
*/

Qt::weak_ordering compareThreeWay(const QDateTime &lhs, const QDateTime &rhs)
{
    if (!lhs.isValid())
        return rhs.isValid() ? Qt::weak_ordering::less : Qt::weak_ordering::equivalent;

    if (!rhs.isValid())
        return Qt::weak_ordering::greater; // we know that lhs is valid here

    const qint64 lhms = getMSecs(lhs.d), rhms = getMSecs(rhs.d);
    if (usesSameOffset(lhs.d, rhs.d) || areFarEnoughApart(lhms, rhms))
        return Qt::compareThreeWay(lhms, rhms);

    // Convert to UTC and compare
    return Qt::compareThreeWay(lhs.toMSecsSinceEpoch(), rhs.toMSecsSinceEpoch());
}

/*!
    \fn bool QDateTime::operator<(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs is earlier than \a rhs;
    otherwise returns \c false.

    \include qdatetime.cpp datetime-order-details

    \sa operator==()
*/

/*!
    \fn bool QDateTime::operator<=(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs is earlier than or equal to \a rhs; otherwise
    returns \c false.

    \include qdatetime.cpp datetime-order-details

    \sa operator==()
*/

/*!
    \fn bool QDateTime::operator>(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs is later than \a rhs; otherwise returns \c false.

    \include qdatetime.cpp datetime-order-details

    \sa operator==()
*/

/*!
    \fn bool QDateTime::operator>=(const QDateTime &lhs, const QDateTime &rhs)

    Returns \c true if \a lhs is later than or equal to \a rhs;
    otherwise returns \c false.

    \include qdatetime.cpp datetime-order-details

    \sa operator==()
*/

/*!
    \since 6.5
    \fn QDateTime QDateTime::currentDateTime(const QTimeZone &zone)

    Returns the system clock's current datetime, using the time representation
    described by \a zone. If \a zone is omitted, local time is used.

    \sa currentDateTimeUtc(), QDate::currentDate(), QTime::currentTime(), toTimeZone()
*/

/*!
    \overload
    \since 0.90
*/
QDateTime QDateTime::currentDateTime()
{
    return currentDateTime(QTimeZone::LocalTime);
}

/*!
    \fn QDateTime QDateTime::currentDateTimeUtc()
    \since 4.7
    Returns the system clock's current datetime, expressed in terms of UTC.

    Equivalent to \c{currentDateTime(QTimeZone::UTC)}.

    \sa currentDateTime(), QDate::currentDate(), QTime::currentTime(), toTimeZone()
*/

QDateTime QDateTime::currentDateTimeUtc()
{
    return currentDateTime(QTimeZone::UTC);
}

/*!
    \fn qint64 QDateTime::currentMSecsSinceEpoch()
    \since 4.7

    Returns the current number of milliseconds since the start, in UTC, of the year 1970.

    This number is like the POSIX time_t variable, but expressed in milliseconds
    instead of seconds.

    \sa currentDateTime(), currentDateTimeUtc(), toTimeZone()
*/

/*!
    \fn qint64 QDateTime::currentSecsSinceEpoch()
    \since 5.8

    Returns the number of seconds since the start, in UTC, of the year 1970.

    This number is like the POSIX time_t variable.

    \sa currentMSecsSinceEpoch()
*/

/*!
    \fn template <typename Clock, typename Duration> QDateTime QDateTime::fromStdTimePoint(const std::chrono::time_point<Clock, Duration> &time)
    \since 6.4

    Constructs a datetime representing the same point in time as \a time,
    using Qt::UTC as its time representation.

    The clock of \a time must be compatible with
    \c{std::chrono::system_clock}; in particular, a conversion
    supported by \c{std::chrono::clock_cast} must exist. After the
    conversion, the duration type of the result must be convertible to
    \c{std::chrono::milliseconds}.

    If this is not the case, the caller must perform the necessary
    clock conversion towards \c{std::chrono::system_clock} and the
    necessary conversion of the duration type
    (cast/round/floor/ceil/...) so that the input to this function
    satisfies the constraints above.

    \note This function requires C++20.

    \sa toStdSysMilliseconds(), fromMSecsSinceEpoch()
*/

/*!
    \since 6.4
    \overload

    Constructs a datetime representing the same point in time as \a time,
    using Qt::UTC as its time representation.
*/
QDateTime QDateTime::fromStdTimePoint(
    std::chrono::time_point<
        std::chrono::system_clock,
        std::chrono::milliseconds
    > time)
{
    return fromMSecsSinceEpoch(time.time_since_epoch().count(), QTimeZone::UTC);
}

/*!
    \fn QDateTime QDateTime::fromStdTimePoint(const std::chrono::local_time<std::chrono::milliseconds> &time)
    \since 6.4

    Constructs a datetime whose date and time are the number of milliseconds
    represented by \a time, counted since 1970-01-01T00:00:00.000 in local
    time (Qt::LocalTime).

    \note This function requires C++20.

    \sa toStdSysMilliseconds(), fromMSecsSinceEpoch()
*/

/*!
    \fn QDateTime QDateTime::fromStdLocalTime(const std::chrono::local_time<std::chrono::milliseconds> &time)
    \since 6.4

    Constructs a datetime whose date and time are the number of milliseconds
    represented by \a time, counted since 1970-01-01T00:00:00.000 in local
    time (Qt::LocalTime).

    \note This function requires C++20.

    \sa toStdSysMilliseconds(), fromMSecsSinceEpoch()
*/

/*!
    \fn QDateTime QDateTime::fromStdZonedTime(const std::chrono::zoned_time<std::chrono::milliseconds, const std::chrono::time_zone *> &time);
    \since 6.4

    Constructs a datetime representing the same point in time as \a time.
    The result will be expressed in \a{time}'s time zone.

    \note This function requires C++20.

    \sa QTimeZone

    \sa toStdSysMilliseconds(), fromMSecsSinceEpoch()
*/

/*!
    \fn std::chrono::sys_time<std::chrono::milliseconds> QDateTime::toStdSysMilliseconds() const
    \since 6.4

    Converts this datetime object to the equivalent time point expressed in
    milliseconds, using \c{std::chrono::system_clock} as a clock.

    \note This function requires C++20.

    \sa fromStdTimePoint(), toMSecsSinceEpoch()
*/

/*!
    \fn std::chrono::sys_seconds QDateTime::toStdSysSeconds() const
    \since 6.4

    Converts this datetime object to the equivalent time point expressed in
    seconds, using \c{std::chrono::system_clock} as a clock.

    \note This function requires C++20.

    \sa fromStdTimePoint(), toSecsSinceEpoch()
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

QDateTime QDateTime::currentDateTime(const QTimeZone &zone)
{
    // We can get local time or "system" time (which is UTC); otherwise, we must
    // convert, which is most efficiently done from UTC.
    const Qt::TimeSpec spec = zone.timeSpec();
    SYSTEMTIME st = {};
    // https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemtime
    // We previously used GetLocalTime for spec == LocalTime but it didn't provide enough
    // information to differentiate between repeated hours of a tradition and would report the same
    // timezone (eg always CEST, never CET) for both. But toTimeZone handles it correctly, given
    // the UTC time.
    GetSystemTime(&st);
    QDate d(st.wYear, st.wMonth, st.wDay);
    QTime t(msecsFromDecomposed(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds));
    QDateTime utc(d, t, QTimeZone::UTC);
    return spec == Qt::UTC ? utc : utc.toTimeZone(zone);
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

#elif defined(Q_OS_UNIX) // Assume POSIX-compliant
QDate QDate::currentDate()
{
    return QDateTime::currentDateTime().date();
}

QTime QTime::currentTime()
{
    return QDateTime::currentDateTime().time();
}

QDateTime QDateTime::currentDateTime(const QTimeZone &zone)
{
    return fromMSecsSinceEpoch(currentMSecsSinceEpoch(), zone);
}

qint64 QDateTime::currentMSecsSinceEpoch() noexcept
{
    struct timespec when;
    if (clock_gettime(CLOCK_REALTIME, &when) == 0) // should always succeed
        return when.tv_sec * MSECS_PER_SEC + (when.tv_nsec + 500'000) / 1'000'000;
    Q_UNREACHABLE_RETURN(0);
}

qint64 QDateTime::currentSecsSinceEpoch() noexcept
{
    struct timespec when;
    if (clock_gettime(CLOCK_REALTIME, &when) == 0) // should always succeed
        return when.tv_sec;
    Q_UNREACHABLE_RETURN(0);
}
#else
#error "What system is this?"
#endif

#if QT_DEPRECATED_SINCE(6, 9)
/*!
    \since 5.2
    \overload
    \deprecated [6.9] Pass a \l QTimeZone instead, or omit \a spec and \a offsetSeconds.

    Returns a datetime representing a moment the given number \a msecs of
    milliseconds after the start, in UTC, of the year 1970, described as
    specified by \a spec and \a offsetSeconds.

    Note that there are possible values for \a msecs that lie outside the valid
    range of QDateTime, both negative and positive. The behavior of this
    function is undefined for those values.

    If the \a spec is not Qt::OffsetFromUTC then the \a offsetSeconds will be
    ignored.  If the \a spec is Qt::OffsetFromUTC and the \a offsetSeconds is 0
    then Qt::UTC will be used as the \a spec, since UTC has zero offset.

    If \a spec is Qt::TimeZone then Qt::LocalTime will be used in its place,
    equivalent to using the current system time zone (but differently
    represented).

    \sa fromSecsSinceEpoch(), toMSecsSinceEpoch(), setMSecsSinceEpoch()
*/
QDateTime QDateTime::fromMSecsSinceEpoch(qint64 msecs, Qt::TimeSpec spec, int offsetSeconds)
{
    return fromMSecsSinceEpoch(msecs,
                               asTimeZone(spec, offsetSeconds, "QDateTime::fromMSecsSinceEpoch"));
}

/*!
    \since 5.8
    \overload
    \deprecated [6.9] Pass a \l QTimeZone instead, or omit \a spec and \a offsetSeconds.

    Returns a datetime representing a moment the given number \a secs of seconds
    after the start, in UTC, of the year 1970, described as specified by \a spec
    and \a offsetSeconds.

    Note that there are possible values for \a secs that lie outside the valid
    range of QDateTime, both negative and positive. The behavior of this
    function is undefined for those values.

    If the \a spec is not Qt::OffsetFromUTC then the \a offsetSeconds will be
    ignored.  If the \a spec is Qt::OffsetFromUTC and the \a offsetSeconds is 0
    then Qt::UTC will be used as the \a spec, since UTC has zero offset.

    If \a spec is Qt::TimeZone then Qt::LocalTime will be used in its place,
    equivalent to using the current system time zone (but differently
    represented).

    \sa fromMSecsSinceEpoch(), toSecsSinceEpoch(), setSecsSinceEpoch()
*/
QDateTime QDateTime::fromSecsSinceEpoch(qint64 secs, Qt::TimeSpec spec, int offsetSeconds)
{
    return fromSecsSinceEpoch(secs,
                              asTimeZone(spec, offsetSeconds, "QDateTime::fromSecsSinceEpoch"));
}
#endif // 6.9 deprecations

/*!
    \since 5.2

    Returns a datetime representing a moment the given number \a msecs of
    milliseconds after the start, in UTC, of the year 1970, described as
    specified by \a timeZone. The default time representation is local time.

    Note that there are possible values for \a msecs that lie outside the valid
    range of QDateTime, both negative and positive. The behavior of this
    function is undefined for those values.

    \sa fromSecsSinceEpoch(), toMSecsSinceEpoch(), setMSecsSinceEpoch()
*/
QDateTime QDateTime::fromMSecsSinceEpoch(qint64 msecs, const QTimeZone &timeZone)
{
    QDateTime dt;
    reviseTimeZone(dt.d, timeZone, TransitionResolution::Reject);
    if (timeZone.isValid())
        dt.setMSecsSinceEpoch(msecs);
    return dt;
}

/*!
    \overload
*/
QDateTime QDateTime::fromMSecsSinceEpoch(qint64 msecs)
{
    return fromMSecsSinceEpoch(msecs, QTimeZone::LocalTime);
}

/*!
    \since 5.8

    Returns a datetime representing a moment the given number \a secs of seconds
    after the start, in UTC, of the year 1970, described as specified by \a
    timeZone. The default time representation is local time.

    Note that there are possible values for \a secs that lie outside the valid
    range of QDateTime, both negative and positive. The behavior of this
    function is undefined for those values.

    \sa fromMSecsSinceEpoch(), toSecsSinceEpoch(), setSecsSinceEpoch()
*/
QDateTime QDateTime::fromSecsSinceEpoch(qint64 secs, const QTimeZone &timeZone)
{
    QDateTime dt;
    reviseTimeZone(dt.d, timeZone, TransitionResolution::Reject);
    if (timeZone.isValid())
        dt.setSecsSinceEpoch(secs);
    return dt;
}

/*!
    \overload
*/
QDateTime QDateTime::fromSecsSinceEpoch(qint64 secs)
{
    return fromSecsSinceEpoch(secs, QTimeZone::LocalTime);
}

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

        QDateTime dateTime(rfc.date, rfc.time, QTimeZone::UTC);
        dateTime.setTimeZone(QTimeZone::fromSecondsAheadOfUtc(rfc.utcOffset));
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

        QTimeZone zone = QTimeZone::LocalTime;
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

        // Check end of string for Time Zone definition, either Z for UTC or ±HH:mm for Offset
        if (isoString.endsWith(u'Z', Qt::CaseInsensitive)) {
            zone = QTimeZone::UTC;
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
                int offset = fromOffsetString(isoString.sliced(signIndex), &ok);
                if (!ok)
                    return QDateTime();
                isoString = isoString.first(signIndex);
                zone = QTimeZone::fromSecondsAheadOfUtc(offset);
            }
        }

        // Might be end of day (24:00, including variants), which QTime considers invalid.
        // ISO 8601 (section 4.2.3) says that 24:00 is equivalent to 00:00 the next day.
        bool isMidnight24 = false;
        QTime time = fromIsoTimeString(isoString, format, &isMidnight24);
        if (!time.isValid())
            return QDateTime();
        if (isMidnight24) // time is 0:0, but we want the start of next day:
            return date.addDays(1).startOfDay(zone);
        return QDateTime(date, time, zone);
    }
    case Qt::TextDate: {
        QVarLengthArray<QStringView, 6> parts;

        auto tokens = string.tokenize(u' ', Qt::SkipEmptyParts);
        auto it = tokens.begin();
        for (int i = 0; i < 6 && it != tokens.end(); ++i, ++it)
            parts.emplace_back(*it);

        // Documented as "ddd MMM d HH:mm:ss yyyy" with optional offset-suffix;
        // and allow time either before or after year.
        if (parts.size() < 5 || it != tokens.end())
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

        if (parts.size() == 5)
            return QDateTime(date, time);

        QStringView tz = parts.at(5);
        if (tz.startsWith("UTC"_L1)
            // GMT has long been deprecated as an alias for UTC.
            || tz.startsWith("GMT"_L1, Qt::CaseInsensitive)) {
            tz = tz.sliced(3);
            if (tz.isEmpty())
                return QDateTime(date, time, QTimeZone::UTC);

            int offset = fromOffsetString(tz, &ok);
            return ok ? QDateTime(date, time, QTimeZone::fromSecondsAheadOfUtc(offset))
                      : QDateTime();
        }
        return QDateTime();
    }
    }

    return QDateTime();
}

/*!
    \fn QDateTime QDateTime::fromString(const QString &string, const QString &format, int baseYear, QCalendar cal)

    Returns the QDateTime represented by the \a string, using the \a
    format given, or an invalid datetime if the string cannot be parsed.

    Uses the calendar \a cal if supplied, else Gregorian.

    \include qlocale.cpp base-year-for-two-digit

    In addition to the expressions, recognized in the format string to represent
    parts of the date and time, by QDate::fromString() and QTime::fromString(),
    this method supports:

    \table
    \header \li Expression \li Output
    \row \li t
         \li the timezone (offset, name, "Z" or offset with "UTC" prefix)
    \row \li tt
         \li the timezone in offset format with no colon between hours and
             minutes (for example "+0200")
    \row \li ttt
         \li the timezone in offset format with a colon between hours and
             minutes (for example "+02:00")
    \row \li tttt
         \li the timezone name, either what \l QTimeZone::displayName() reports
             for \l QTimeZone::LongName or the IANA ID of the zone (for example
             "Europe/Berlin"). The names recognized are those known to \l
             QTimeZone, which may depend on the operating system in use.
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
    format is satisfied but \a string represents an invalid datetime (e.g. in a
    gap skipped by a time-zone transition), an valid QDateTime is returned, that
    represents a near-by datetime that is valid.

    The expressions that don't have leading zeroes (d, M, h, m, s, z) will be
    greedy. This means that they will use two digits (or three, for z) even if this will
    put them outside the range and/or leave too few digits for other
    sections.

    \snippet code/src_corelib_time_qdatetime.cpp 13

    This could have meant 1 January 00:30.00 but the M will grab
    two digits.

    Incorrectly specified fields of the \a string will cause an invalid
    QDateTime to be returned. Only datetimes between the local time start of
    year 100 and end of year 9999 are supported. Note that datetimes near the
    ends of this range in other time-zones, notably including UTC, may fall
    outside the range (and thus be treated as invalid) depending on local time
    zone.

    \note Day and month names as well as AM/PM indicators must be given in
    English (C locale).  If localized month and day names or localized forms of
    AM/PM are to be recognized, use QLocale::system().toDateTime().

    \note If a format character is repeated more times than the longest
    expression in the table above using it, this part of the format will be read
    as several expressions with no separator between them; the longest above,
    possibly repeated as many times as there are copies of it, ending with a
    residue that may be a shorter expression. Thus \c{'tttttt'} would match
    \c{"Europe/BerlinEurope/Berlin"} and set the zone to Berlin time; if the
    datetime string contained "Europe/BerlinZ" it would "match" but produce an
    inconsistent result, leading to an invalid datetime.

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
QDateTime QDateTime::fromString(const QString &string, QStringView format, int baseYear,
                                QCalendar cal)
{
#if QT_CONFIG(datetimeparser)
    QDateTime datetime;

    QDateTimeParser dt(QMetaType::QDateTime, QDateTimeParser::FromString, cal);
    dt.setDefaultLocale(QLocale::c());
    if (dt.parseFormat(format) && (dt.fromString(string, &datetime, baseYear)
                                   || !datetime.isValid())) {
        return datetime;
    }
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
    Q_UNUSED(baseYear);
    Q_UNUSED(cal);
#endif
    return QDateTime();
}

/*!
    \fn QDateTime QDateTime::fromString(const QString &string, const QString &format, QCalendar cal)
    \overload
    \since 5.14
*/

/*!
    \fn QDateTime QDateTime::fromString(const QString &string, QStringView format, QCalendar cal)
    \overload
    \since 6.0
*/

/*!
    \fn QDateTime QDateTime::fromString(QStringView string, QStringView format, int baseYear, QCalendar cal)
    \overload
    \since 6.7
*/

/*!
    \fn QDateTime QDateTime::fromString(QStringView string, QStringView format, int baseYear)
    \overload
    \since 6.7

    Uses a default-constructed QCalendar.
*/

/*!
    \overload
    \since 6.7

    Uses a default-constructed QCalendar.
*/
QDateTime QDateTime::fromString(const QString &string, QStringView format, int baseYear)
{
    return fromString(string, format, baseYear, QCalendar());
}

/*!
    \fn QDateTime QDateTime::fromString(const QString &string, const QString &format, int baseYear)
    \overload
    \since 6.7

    Uses a default-constructed QCalendar.
*/
#endif // datestring

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
        return out << date.jd;
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
        in >> date.jd;
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
    std::pair<QDate, QTime> dateAndTime;

    // TODO: new version, route spec and details via QTimeZone
    if (out.version() >= QDataStream::Qt_5_2) {

        // In 5.2 we switched to using Qt::TimeSpec and added offset and zone support
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
    QTimeZone zone(QTimeZone::LocalTime);

    if (in.version() >= QDataStream::Qt_5_2) {

        // In 5.2 we switched to using Qt::TimeSpec and added offset and zone support
        in >> dt >> tm >> ts;
        switch (static_cast<Qt::TimeSpec>(ts)) {
        case Qt::UTC:
            zone = QTimeZone::UTC;
            break;
        case Qt::OffsetFromUTC: {
            qint32 offset = 0;
            in >> offset;
            zone = QTimeZone::fromSecondsAheadOfUtc(offset);
            break;
        }
        case Qt::LocalTime:
            break;
        case Qt::TimeZone:
            in >> zone;
            break;
        }
        // Note: no way to resolve transition ambiguity, when relevant; use default.
        dateTime = QDateTime(dt, tm, zone);

    } else if (in.version() == QDataStream::Qt_5_0) {

        // In Qt 5.0 we incorrectly serialised all datetimes as UTC
        in >> dt >> tm >> ts;
        dateTime = QDateTime(dt, tm, QTimeZone::UTC);
        if (static_cast<Qt::TimeSpec>(ts) == Qt::LocalTime)
            dateTime = dateTime.toTimeZone(zone);

    } else if (in.version() >= QDataStream::Qt_4_0) {

        // From 4.0 to 5.1 (except 5.0) we used QDateTimePrivate::Spec
        in >> dt >> tm >> ts;
        switch (static_cast<QDateTimePrivate::Spec>(ts)) {
        case QDateTimePrivate::OffsetFromUTC: // No offset was stored, so treat as UTC.
        case QDateTimePrivate::UTC:
            zone = QTimeZone::UTC;
            break;
        case QDateTimePrivate::TimeZone: // No zone was stored, so treat as LocalTime:
        case QDateTimePrivate::LocalUnknown:
        case QDateTimePrivate::LocalStandard:
        case QDateTimePrivate::LocalDST:
            break;
        }
        dateTime = QDateTime(dt, tm, zone);

    } else { // version < QDataStream::Qt_4_0

        // Before 4.0 there was no TimeSpec, only Qt::LocalTime was supported
        in >> dt >> tm;
        dateTime = QDateTime(dt, tm);

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
        // QTBUG-91070, ISODate only supports years in the range 0-9999
        if (int y = date.year(); y > 0 && y <= 9999)
            dbg.nospace() << date.toString(Qt::ISODate);
        else
            dbg.nospace() << date.toString(Qt::TextDate);
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
    \qhashold{QHash}
    \since 5.0
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
    \qhashold{QHash}
    \since 5.0
*/
size_t qHash(QDate key, size_t seed) noexcept
{
    return qHash(key.toJulianDay(), seed);
}

/*! \fn size_t qHash(QTime key, size_t seed = 0)
    \qhashold{QHash}
    \since 5.0
*/
size_t qHash(QTime key, size_t seed) noexcept
{
    return qHash(key.msecsSinceStartOfDay(), seed);
}

QT_END_NAMESPACE
