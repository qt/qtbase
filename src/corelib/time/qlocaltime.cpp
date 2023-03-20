// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlocaltime_p.h"
#include "qplatformdefs.h"

#include "private/qcalendarmath_p.h"
#if QT_CONFIG(datetimeparser)
#include "private/qdatetimeparser_p.h"
#endif
#include "private/qgregoriancalendar_p.h"
#include "private/qnumeric_p.h"
#include "private/qtenvironmentvariables_p.h"
#if QT_CONFIG(timezone)
#include "private/qtimezoneprivate_p.h"
#endif

#include <time.h>
#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

using namespace QtPrivate::DateTimeConstants;
namespace {
/*
    Qt represents n BCE as -n, whereas struct tm's tm_year field represents a
    year by the number of years after (negative for before) 1900, so that 1+m
    BCE is -1900 -m; so treating 1 BCE as 0 CE. We thus shift by different
    offsets depending on whether the year is BCE or CE.
*/
constexpr int tmYearFromQYear(int year) { return year - (year < 0 ? 1899 : 1900); }
constexpr int qYearFromTmYear(int year) { return year + (year < -1899 ? 1899 : 1900); }

constexpr inline qint64 tmSecsWithinDay(const struct tm &when)
{
    return (when.tm_hour * MINS_PER_HOUR + when.tm_min) * SECS_PER_MIN + when.tm_sec;
}

/* If mktime() returns -1, is it really an error ?

   It might return -1 because we're looking at the last second of 1969 and
   mktime does support times before 1970 (POSIX says "If the year is <1970 or
   the value is negative, the relationship is undefined" and MS rejects the
   value, consistent with that; so we don't call mktime() on MS in this case and
   can't get -1 unless it's a real error). However, on UNIX, that's -1 UTC time
   and all we know, aside from mktime's return, is the local time. (We could
   check errno, but we call mktime from within a qt_scoped_lock(QBasicMutex),
   whose unlocking and destruction of the locker might frob errno.)

   We can assume the zone offset is a multiple of five minutes and less than a
   day, so this can only arise for the last second of a minute that differs from
   59 by a multiple of 5 on the last day of 1969 or the first day of 1970.  That
   makes for a cheap pre-test; if it holds, we can ask mktime about the first
   second of the same minute; if it gives us -60, then the -1 we originally saw
   is not an error (or was an error, but needn't have been).
*/
inline bool meansEnd1969(tm *local)
{
#ifdef Q_OS_WIN
    Q_UNUSED(local);
    return false;
#else
    if (local->tm_sec < 59 || local->tm_year < 69 || local->tm_year > 70
        || local->tm_min % 5 != 4 // Assume zone offset is a multiple of 5 mins
        || (local->tm_year == 69
            ? local->tm_mon < 11 || local->tm_mday < 31
            : local->tm_mon > 0 || local->tm_mday > 1)) {
        return false;
    }
    tm copy = *local;
    copy.tm_sec--; // Preceding second should get -2, not -1
    if (qMkTime(&copy) != -2)
        return false;
    // The original call to qMkTime() may have returned -1 as failure, not
    // updating local, even though it could have; so fake it here. Assumes there
    // was no transition in the last minute of the day !
    *local = copy;
    local->tm_sec++; // Advance back to the intended second
    return true;
#endif
}

/*
    Call mktime but bypass its fixing of denormal times.

    The POSIX spec says mktime() accepts a struct tm whose fields lie outside
    the usual ranges; the parameter is not const-qualified and will be updated
    to have values in those ranges. However, MS's implementation doesn't do that
    (or hasn't always done it); and the only member we actually want updated is
    the tm_isdst flag.  (Aside: MS's implementation also only works for tm_year
    >= 70; this is, in fact, in accordance with the POSIX spec; but all known
    UNIX libc implementations in fact have a signed time_t and Do The Sensible
    Thing, to the best of their ability, at least for 0 <= tm_year < 70; see
    meansEnd1969 for the handling of the last second of UTC's 1969.)

    If we thought we knew tm_isdst and mktime() disagrees, it'll let us know
    either by correcting it - in which case it adjusts the struct tm to reflect
    the same time, but represented using the right tm_isdst, so typically an
    hour earlier or later - or by returning -1. When this happens, the way we
    actually use mktime(), we don't want a revised time with corrected DST, we
    want the original time with its corrected DST; so we retry the call, this
    time not claiming to know the DST-ness.

    POSIX doesn't actually say what to do if the specified struct tm describes a
    time in a spring-forward gap: read literally, this is an unrepresentable
    time and it could return -1, setting errno to EOVERFLOW. However, actual
    implementations chose a time one side or the other of the gap. For example,
    if we claim to know DST, glibc pushes to the other side of the gap (changing
    tm_isdst), but stays on the indicated branch of a repetition (no change to
    tm_isdst); this matches how QTimeZonePrivate::dataForLocalTime() uses its
    hint; in either case, if we don't claim to know DST, glibc picks the DST
    candidate. (Experiments conducted with glibc 2.31-9.)
*/
inline bool callMkTime(tm *local, time_t *secs)
{
    constexpr time_t maybeError = -1; // mktime()'s return on error; or last second of 1969 UTC.
    const tm copy = *local;
    *secs = qMkTime(local);
    bool good = *secs != maybeError || meansEnd1969(local);
    if (copy.tm_isdst >= 0 && (!good || local->tm_isdst != copy.tm_isdst)) {
        // We thought we knew DST-ness, but were wrong:
        *local = copy;
        local->tm_isdst = -1;
        *secs = qMkTime(local);
        good = *secs != maybeError || meansEnd1969(local);
    }
#if defined(Q_OS_WIN)
    // Windows mktime for the missing hour backs up 1 hour instead of advancing
    // 1 hour. If time differs and is standard time then this has happened, so
    // add 2 hours to the time and 1 hour to the secs
    if (local->tm_isdst == 0 && local->tm_hour != copy.tm_hour) {
        local->tm_hour += 2;
        if (local->tm_hour > 23) {
            local->tm_hour -= 24;
            if (++local->tm_mday > QGregorianCalendar::monthLength(
                    local->tm_mon + 1, qYearFromTmYear(local->tm_year))) {
                local->tm_mday = 1;
                if (++local->tm_mon > 11) {
                    local->tm_mon = 0;
                    ++local->tm_year;
                }
            }
        }
        *secs += 3600;
        local->tm_isdst = 1;
    }
#endif // Q_OS_WIN
    return good;
}

struct tm timeToTm(qint64 localDay, int secs, QDateTimePrivate::DaylightStatus dst)
{
    Q_ASSERT(0 <= secs && secs < 3600 * 24);
    const auto ymd = QGregorianCalendar::partsFromJulian(JULIAN_DAY_FOR_EPOCH + localDay);
    struct tm local = {};
    local.tm_year = tmYearFromQYear(ymd.year);
    local.tm_mon = ymd.month - 1;
    local.tm_mday = ymd.day;
    local.tm_hour = secs / 3600;
    local.tm_min = (secs % 3600) / 60;
    local.tm_sec = (secs % 60);
    local.tm_isdst = int(dst);
    return local;
}

inline std::optional<qint64> tmToJd(const struct tm &date)
{
    return QGregorianCalendar::julianFromParts(qYearFromTmYear(date.tm_year),
                                               date.tm_mon + 1, date.tm_mday);
}

#define IC(N) std::integral_constant<qint64, N>()

// True if combining day and seconds overflows qint64; otherwise, sets *epochSeconds
inline bool daysAndSecondsOverflow(qint64 julianDay, qint64 daySeconds, qint64 *epochSeconds)
{
    return qMulOverflow(julianDay - JULIAN_DAY_FOR_EPOCH, IC(SECS_PER_DAY), epochSeconds)
        || qAddOverflow(*epochSeconds, daySeconds, epochSeconds);
}

// True if combining seconds and millis overflows; otherwise sets *epochMillis
inline bool secondsAndMillisOverflow(qint64 epochSeconds, qint64 millis, qint64 *epochMillis)
{
    return qMulOverflow(epochSeconds, IC(MSECS_PER_SEC), epochMillis)
        || qAddOverflow(*epochMillis, millis, epochMillis);
}

#undef IC

} // namespace

namespace QLocalTime {

#ifndef QT_BOOTSTRAPPED
// Even if local time is currently in DST, this returns the standard time offset
// (in seconds) nominally in effect at present:
int getCurrentStandardUtcOffset()
{
#ifdef Q_OS_WIN
    TIME_ZONE_INFORMATION tzInfo;
    if (GetTimeZoneInformation(&tzInfo) != TIME_ZONE_ID_INVALID) {
        int bias = tzInfo.Bias; // In minutes.
        // StandardBias is usually zero, but include it if given:
        if (tzInfo.StandardDate.wMonth) // Zero month means ignore StandardBias.
            bias += tzInfo.StandardBias;
        // MS's bias is +ve in the USA, so minutes *behind* UTC - we want seconds *ahead*:
        return -bias * SECS_PER_MIN;
    }
#else
    qTzSet();
    const time_t curr = time(nullptr);
    if (curr != -1) {
        /* Set t to the UTC representation of curr; the time whose local
           standard time representation coincides with that differs from curr by
           local time's standard offset.  Note that gmtime() leaves the tm_isdst
           flag set to 0, so mktime() will, even if local time is currently
           using DST, return the time since epoch at which local standard time
           would have the same representation as UTC's representation of
           curr. The fact that mktime() also flips tm_isdst and updates the time
           fields to the DST-equivalent time needn't concern us here; all that
           matters is that it returns the time after epoch at which standard
           time's representation would have matched UTC's, had it been in
           effect.
        */
#  if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
        struct tm t;
        if (gmtime_r(&curr, &t)) {
            time_t mkt = qMkTime(&t);
            int offset = int(curr - mkt);
            Q_ASSERT(std::abs(offset) <= SECS_PER_DAY);
            return offset;
        }
#  else
        if (struct tm *tp = gmtime(&curr)) {
            struct tm t = *tp; // Copy it quick, hopefully before it can get stomped
            time_t mkt = qMkTime(&t);
            int offset = int(curr - mkt);
            Q_ASSERT(std::abs(offset) <= SECS_PER_DAY);
            return offset;
        }
#  endif
    } // else, presumably: errno == EOVERFLOW
#endif // Platform choice
    qDebug("Unable to determine current standard time offset from UTC");
    // We can't tell, presume UTC.
    return 0;
}

// This is local time's offset (in seconds), at the specified time, including
// any DST part.
int getUtcOffset(qint64 atMSecsSinceEpoch)
{
    return QDateTimePrivate::expressUtcAsLocal(atMSecsSinceEpoch).offset;
}
#endif // QT_BOOTSTRAPPED

// Calls the platform variant of localtime() for the given utcMillis, and
// returns the local milliseconds, offset from UTC and DST status.
QDateTimePrivate::ZoneState utcToLocal(qint64 utcMillis)
{
    const auto epoch = QRoundingDown::qDivMod<MSECS_PER_SEC>(utcMillis);
    const time_t epochSeconds = epoch.quotient;
    const int msec = epoch.remainder;
    Q_ASSERT(msec >= 0 && msec < MSECS_PER_SEC);
    if (qint64(epochSeconds) * MSECS_PER_SEC + msec != utcMillis) // time_t range too narrow
        return {utcMillis};

    tm local;
    if (!qLocalTime(epochSeconds, &local))
        return {utcMillis};

    auto jd = tmToJd(local);
    if (Q_UNLIKELY(!jd))
        return {utcMillis};

    const qint64 daySeconds = tmSecsWithinDay(local);
    Q_ASSERT(0 <= daySeconds && daySeconds < SECS_PER_DAY);
    qint64 localSeconds, localMillis;
    if (Q_UNLIKELY(daysAndSecondsOverflow(*jd, daySeconds, &localSeconds)
                   || secondsAndMillisOverflow(localSeconds, qint64(msec), &localMillis))) {
        return {utcMillis};
    }
    const auto dst
        = local.tm_isdst ? QDateTimePrivate::DaylightTime : QDateTimePrivate::StandardTime;
    return { localMillis, int(localSeconds - epochSeconds), dst };
}

QString localTimeAbbbreviationAt(qint64 local, QDateTimePrivate::DaylightStatus dst)
{
    const auto localDayMilli = QRoundingDown::qDivMod<MSECS_PER_DAY>(local);
    qint64 millis = localDayMilli.remainder;
    Q_ASSERT(0 <= millis && millis < MSECS_PER_DAY); // Definition of QRD::qDiv.
    struct tm tmLocal = timeToTm(localDayMilli.quotient, int(millis / MSECS_PER_SEC), dst);
    time_t utcSecs;
    if (!callMkTime(&tmLocal, &utcSecs))
        return {};
    return qTzName(tmLocal.tm_isdst > 0 ? 1 : 0);
}

QDateTimePrivate::ZoneState mapLocalTime(qint64 local, QDateTimePrivate::DaylightStatus dst)
{
    qint64 localSecs = local / MSECS_PER_SEC;
    qint64 millis = local - localSecs * MSECS_PER_SEC; // 0 or with same sign as local
    const auto localDaySec = QRoundingDown::qDivMod<SECS_PER_DAY>(localSecs);
    qint64 daySecs = localDaySec.remainder;
    Q_ASSERT(0 <= daySecs && daySecs < SECS_PER_DAY); // Definition of QRD::qDiv.

    struct tm tmLocal = timeToTm(localDaySec.quotient, daySecs, dst);
    time_t utcSecs;
    if (!callMkTime(&tmLocal, &utcSecs))
        return {local};

    // TODO: for glibc, we could use tmLocal.tm_gmtoff
    // That would give us offset directly, hence localSecs = offset + utcSecs
    // Provisional offset, until we have a revised localSeconds:
    int offset = localSecs - utcSecs;
    dst = tmLocal.tm_isdst > 0 ? QDateTimePrivate::DaylightTime : QDateTimePrivate::StandardTime;
    auto jd = tmToJd(tmLocal);
    if (Q_UNLIKELY(!jd))
        return {local, offset, dst, false};

    daySecs = tmSecsWithinDay(tmLocal);
    Q_ASSERT(0 <= daySecs && daySecs < SECS_PER_DAY);
    if (daySecs > 0 && *jd < JULIAN_DAY_FOR_EPOCH) {
        jd = *jd + 1;
        daySecs -= SECS_PER_DAY;
    }
    if (Q_UNLIKELY(daysAndSecondsOverflow(*jd, daySecs, &localSecs)))
        return {local, offset, dst, false};

    offset = localSecs - utcSecs;

    // The only way localSecs and millis can now have opposite sign is for
    // resolution of the local time to have kicked us across the epoch, in which
    // case there's no danger of overflow. So if overflow is in danger of
    // happening, we're already doing the best we can to avoid it.
    qint64 revised;
    const bool overflow = secondsAndMillisOverflow(localSecs, millis, &revised);
    return {overflow ? local : revised, offset, dst, !overflow};
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
    supported range by experiment on the first call to qdatetime.cpp's
    millisInSystemRange() by exploration among the known candidates, converting
    the result to milliseconds and flagging whether each end is the qint64
    range's bound (so millisInSystemRange will know not to try to pad beyond
    those bounds). The probed date-times are somewhat inside the range, but
    close enough to the relevant bound that we can be fairly sure the bound is
    reached, if the probe succeeds.
*/
SystemMillisRange computeSystemMillisRange()
{
    // Assert this here, as this is called just once, in a static initialization.
    Q_ASSERT(QGregorianCalendar::julianFromParts(1970, 1, 1) == JULIAN_DAY_FOR_EPOCH);

    constexpr qint64 TIME_T_MAX = std::numeric_limits<time_t>::max();
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
        local.tm_isdst = -1;
        return {qMkTime(&local) == -1 ? 0 : msecsMin, msecsMax, false, false};
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
        // Assume we do at least reach the end of a signed 32-bit time_t (since
        // our actual time_t is bigger than that):
        qint64 stop =
            quint64(std::numeric_limits<qint32>::max()) * MSECS_PER_SEC - 1 + MSECS_PER_SEC;
        // Cleared if first pass round loop fails:
        bool stopMax = true;
        for (const auto c : ends) {
            struct tm local = {};
            local.tm_year = tmYearFromQYear(c.year);
            local.tm_mon = 11;
            local.tm_mday = 31;
            local.tm_hour = 23;
            local.tm_min = local.tm_sec = 59;
            local.tm_isdst = -1;
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
            local.tm_isdst = -1;
            if (qMkTime(&local) != -1)
                return {c.millis, stop, startMin, stopMax};
            startMin = false;
        }
        return {0, stop, false, stopMax};
    }
}

} // QLocalTime

QT_END_NAMESPACE
