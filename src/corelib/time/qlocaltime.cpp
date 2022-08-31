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

#ifdef __GLIBC__ // Extends struct tm with some extra fields:
#define HAVE_TM_GMTOFF // tm_gmtoff is the UTC offset.
#define HAVE_TM_ZONE // tm_zone is the zone abbreviation.
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

/* Call mktime() and make sense of the result.

   This packages the call to mktime() with the needed determination of whether
   that succeeded and whether the call has materially perturbed, including
   normalizing, the struct tm it was passed (as opposed to merely filling in
   details).
*/
class MkTimeResult
{
    // mktime()'s return on error; or last second of 1969 UTC:
    static constexpr time_t maybeError = -1;
    inline bool meansEnd1969();
    bool changed(const struct tm &prior) const;

public:
    struct tm local = {}; // Describes the local time in familiar form.
    time_t utcSecs = maybeError; // Seconds since UTC epoch.
    bool good = false; // Ignore the rest unless this is true.
    bool adjusted = true; // Is local at odds with prior ?
    MkTimeResult() { local.tm_isdst = -1; }

    // Note: the calls to qMkTime() and meansEnd1969() potentially modify local.
    explicit MkTimeResult(const struct tm &prior)
    : local(prior), utcSecs(qMkTime(&local)),
      good(utcSecs != maybeError || meansEnd1969()),
      adjusted(changed(prior))
    {}
};

/* If mktime() returns -1, is it really an error ?

   It might return -1 because we're looking at the last second of 1969 and
   mktime does support times before 1970 (POSIX says "If the year is <1970 or
   the value is negative, the relationship is undefined" and MS rejects the
   value, consistent with that; so we don't call mktime() on MS in this case and
   can't get -1 unless it's a real error). However, on UNIX, that's -1 UTC time
   and all we know, aside from mktime's return, is the local time. (We could
   check errno, but we call mktime from within a qt_scoped_lock(QBasicMutex),
   whose unlocking and destruction of the locker might frob errno.)

   We can assume time-zone offsets are less than a day, so this can only arise
   if the struct tm describes either the last day of 1969 or the first day of
   1970. When we do know the offset (a glibc extension supplies it as a member
   of struct tm), we can determine whether we're on the last second of the day,
   refining that check. That makes for a cheap pre-test; if it holds, we can ask
   mktime() about the preceding second; if it gives us -2, then the -1 we
   originally saw is not (or at least didn't need to be) an error. We can then
   synthesize a corrected value for local using the -2 result.
*/
inline bool MkTimeResult::meansEnd1969()
{
#ifdef Q_OS_WIN
    return false;
#else
    if (local.tm_year < 69 || local.tm_year > 70
#  ifdef HAVE_TM_GMTOFF
        // Africa/Monrovia had offset 00:44:30 at the epoch, so (although all
        // other zones' offsets were round multiples of five minutes) we need
        // the offset to determine whether the time might match:
        || (tmSecsWithinDay(local) - local.tm_gmtoff + 1) % SECS_PER_DAY
#  endif
        || (local.tm_year == 69 // ... and less than a day:
            ? local.tm_mon < 11 || local.tm_mday < 31
            : local.tm_mon > 0 || local.tm_mday > 1)) {
        return false;
    }
    struct tm copy = local;
    copy.tm_sec--; // Preceding second should get -2, not -1
    if (qMkTime(&copy) != -2)
        return false;
    // The original call to qMkTime() may have returned -1 as failure, not
    // updating local, even though it could have; so fake it here. Assumes there
    // was no transition in the last minute of the day !
    local = copy;
    local.tm_sec++; // Advance back to the intended second
    return true;
#endif
}

bool MkTimeResult::changed(const struct tm &prior) const
{
    // If mktime() has been passed a copy of prior and local is its value on
    // return, this checks whether mktime() has made a material change
    // (including normalization) to the value, as opposed to merely filling in
    // the fields that it's specified to fill in. It returns true if there has
    // been any material change.
    return !(prior.tm_year == local.tm_year && prior.tm_mon == local.tm_mon
             && prior.tm_mday == local.tm_mday && prior.tm_hour == local.tm_hour
             && prior.tm_min == local.tm_min && prior.tm_sec == local.tm_sec
             && (prior.tm_isdst == -1
                 ? local.tm_isdst >= 0 : prior.tm_isdst == local.tm_isdst));
}

struct tm timeToTm(qint64 localDay, int secs)
{
    Q_ASSERT(0 <= secs && secs < SECS_PER_DAY);
    const auto ymd = QGregorianCalendar::partsFromJulian(JULIAN_DAY_FOR_EPOCH + localDay);
    struct tm local = {};
    local.tm_year = tmYearFromQYear(ymd.year);
    local.tm_mon = ymd.month - 1;
    local.tm_mday = ymd.day;
    local.tm_hour = secs / 3600;
    local.tm_min = (secs % 3600) / 60;
    local.tm_sec = (secs % 60);
    local.tm_isdst = -1;
    return local;
}

// Transitions account for a small fraction of 1% of the time.
// So mark functions only used in handling them as cold.
Q_DECL_COLD_FUNCTION
struct tm matchYearMonth(struct tm when, const struct tm &base)
{
    // Adjust *when to be a denormal representation of the same point in time
    // but with tm_year and tm_mon the same as base. In practice this will
    // represent an adjacent month, so don't worry too much about optimising for
    // any other case; we almost certainly run zero or one iteration of one of
    // the year loops then zero or one iteration of one of the month loops.
    while (when.tm_year > base.tm_year) {
        --when.tm_year;
        when.tm_mon += 12;
    }
    while (when.tm_year < base.tm_year) {
        ++when.tm_year;
        when.tm_mon -= 12;
    }
    Q_ASSERT(when.tm_year == base.tm_year);
    while (when.tm_mon > base.tm_mon) {
        const auto yearMon = QRoundingDown::qDivMod<12>(when.tm_mon);
        int year = yearMon.quotient;
        // We want the month before's Qt month number, which is the tm_mon mod 12:
        int month = yearMon.remainder;
        if (month == 0) {
            --year;
            month = 12;
        }
        year += when.tm_year;
        when.tm_mday += QGregorianCalendar::monthLength(month, qYearFromTmYear(year));
        --when.tm_mon;
    }
    while (when.tm_mon < base.tm_mon) {
        const auto yearMon = QRoundingDown::qDivMod<12>(when.tm_mon);
        // Qt month number is offset from tm_mon by one:
        when.tm_mday -= QGregorianCalendar::monthLength(
            yearMon.remainder + 1, qYearFromTmYear(yearMon.quotient + when.tm_year));
        ++when.tm_mon;
    }
    Q_ASSERT(when.tm_mon == base.tm_mon);
    return when;
}

Q_DECL_COLD_FUNCTION
struct tm adjacentDay(struct tm when, int dayStep)
{
    // Before we adjust it, when is a return from timeToTm(), so in normal form.
    Q_ASSERT(dayStep * dayStep == 1);
    when.tm_mday += dayStep;
    // That may have bumped us across a month boundary or even a year one.
    // So now we normalize it.

    if (dayStep < 0) {
        if (when.tm_mday <= 0) {
            // Month before's day-count; but tm_mon's value is one less than Qt's
            // month numbering so, before we decrement it, it has the value we need,
            // unless it's 0.
            int daysInMonth = when.tm_mon
                ? QGregorianCalendar::monthLength(when.tm_mon, qYearFromTmYear(when.tm_year))
                : QGregorianCalendar::monthLength(12, qYearFromTmYear(when.tm_year - 1));
            when.tm_mday += daysInMonth;
            if (--when.tm_mon < 0) {
                --when.tm_year;
                when.tm_mon = 11;
            }
            Q_ASSERT(when.tm_mday >= 1);
        }
    } else if (when.tm_mday > 28) {
        // We have to wind through months one at a time, since their lengths vary.
        int daysInMonth = QGregorianCalendar::monthLength(
            when.tm_mon + 1, qYearFromTmYear(when.tm_year));
        if (when.tm_mday > daysInMonth) {
            when.tm_mday -= daysInMonth;
            if (++when.tm_mon > 11) {
                ++when.tm_year;
                when.tm_mon = 0;
            }
            Q_ASSERT(when.tm_mday <= QGregorianCalendar::monthLength(
                         when.tm_mon + 1, qYearFromTmYear(when.tm_year)));
        }
    }
    return when;
}

Q_DECL_COLD_FUNCTION
qint64 secondsBetween(const struct tm &start, const struct tm &stop)
{
    // Nominal difference between start and stop, in seconds (negative if start
    // is after stop); may differ from actual UTC difference if there's a
    // transition between them.
    struct tm from = matchYearMonth(start, stop);
    qint64 diff = stop.tm_mday - from.tm_mday; // in days
    diff = diff * 24 + stop.tm_hour - from.tm_hour; // in hours
    diff = diff * 60 + stop.tm_min - from.tm_min; // in minutes
    return diff * 60 + stop.tm_sec - from.tm_sec; // in seconds
}

Q_DECL_COLD_FUNCTION
MkTimeResult hopAcrossGap(const MkTimeResult &outside, const struct tm &base)
{
    // base fell in a gap; outside is one resolution
    // This returns the other resolution, if possible.
    const qint64 shift = secondsBetween(outside.local, base);
    struct tm across;
    // Shift is the nominal time adjustment between outside and base; now obtain
    // the actual time that far from outside:
    if (qLocalTime(outside.utcSecs + shift, &across)) {
        const qint64 wider = secondsBetween(outside.local, across);
        // That should be bigger than shift (typically by a factor of two), in
        // the same direction:
        if (shift > 0 ? wider > shift : wider < shift) {
            MkTimeResult result(across);
            if (result.good && !result.adjusted)
                return result;
        }
    }
    // This can surely only arise if the other resolution lies outside the
    // time_t-range supported by the system functions.
    return {};
}

Q_DECL_COLD_FUNCTION
MkTimeResult resolveRejected(struct tm base, MkTimeResult result,
                             QDateTimePrivate::TransitionOptions resolve)
{
    // May result from a time outside the supported range of system time_t
    // functions, or from a gap (on a platform where mktime() rejects them).
    // QDateTime filters on times well outside the supported range, but may
    // pass values only slightly outside the range.

    // The easy case - no need to find a resolution anyway:
    if (!resolve.testAnyFlags(QDateTimePrivate::GapMask))
        return {};

    constexpr time_t twoDaysInSeconds = 2 * 24 * 60 * 60;
    // Bracket base, one day each side (in case the zone skipped a whole day):
    MkTimeResult early(adjacentDay(base, -1));
    MkTimeResult later(adjacentDay(base, +1));
    if (!early.good || !later.good) // Assume out of range, rather than gap.
        return {};

    // OK, looks like a gap.
    Q_ASSERT(twoDaysInSeconds + early.utcSecs > later.utcSecs);
    result.adjusted = true;

    // Extrapolate backwards from later if this option is set:
    QDateTimePrivate::TransitionOption beforeLater = QDateTimePrivate::GapUseBefore;
    if (resolve.testFlag(QDateTimePrivate::FlipForReverseDst)) {
        // Reverse DST has DST before a gap and not after:
        if (early.local.tm_isdst == 1 && !later.local.tm_isdst)
            beforeLater = QDateTimePrivate::GapUseAfter;
    }
    if (resolve.testFlag(beforeLater)) // Result will be before the gap:
        result.utcSecs = later.utcSecs - secondsBetween(base, later.local);
    else // Result will be after the gap:
        result.utcSecs = early.utcSecs + secondsBetween(early.local, base);

    if (!qLocalTime(result.utcSecs, &result.local)) // Abandon hope.
        return {};

    return result;
}

Q_DECL_COLD_FUNCTION
bool preferAlternative(QDateTimePrivate::TransitionOptions resolve,
                       // is_dst flags of incumbent and an alternative:
                       int gotDst, int altDst,
                       // True precisely if alternative selects a later UTC time:
                       bool altIsLater,
                       // True for a gap, false for a fold:
                       bool inGap)
{
    // If resolve has this option set, prefer the later candidate, else the earlier:
    QDateTimePrivate::TransitionOption preferLater = inGap ? QDateTimePrivate::GapUseAfter
                                                           : QDateTimePrivate::FoldUseAfter;
    if (resolve.testFlag(QDateTimePrivate::FlipForReverseDst)) {
        // gotDst and altDst are {-1: unknown, 0: standard, 1: daylight-saving}
        // So gotDst ^ altDst is 1 precisely if exactly one candidate thinks it's DST.
        if ((altDst ^ gotDst) == 1) {
            // In this case, we can tell whether we have reversed DST: that's a
            // gap with DST before it or a fold with DST after it.
#if 1
            const bool isReversed = (altDst == 1) != (altIsLater == inGap);
#else // Pedagogic version of the same thing:
            bool isReversed;
            if (altIsLater == inGap) // alt is after a gap or before a fold, so summer-time
                isReversed = altDst != 1; // flip if summer-time isn't DST
            else // alt is before a gap or after a fold, so winter-time
                isReversed = altDst == 1; // flip if winter-time is DST
#endif
            if (isReversed) {
                preferLater = inGap ? QDateTimePrivate::GapUseBefore
                                    : QDateTimePrivate::FoldUseBefore;
            }
        } // Otherwise, we can't tell, so assume not.
    }
    return resolve.testFlag(preferLater) == altIsLater;
}

/*
    Determine UTC time and offset, if possible, at a given local time.

    The local time is specified as a number of seconds since the epoch (so, in
    effect, a time_t, albeit delivered as qint64). If the specified local time
    falls in a transition, resolve determines what to do.

    If the specified local time is outside what the system time_t APIs will
    handle, this fails.
*/
MkTimeResult resolveLocalTime(qint64 local, QDateTimePrivate::TransitionOptions resolve)
{
    const auto localDaySecs = QRoundingDown::qDivMod<SECS_PER_DAY>(local);
    struct tm base = timeToTm(localDaySecs.quotient, localDaySecs.remainder);

    // Get provisional result (correct > 99.9 % of the time):
    MkTimeResult result(base);

    // Our callers (mostly) deal with questions of being within the range that
    // system time_t functions can handle, and timeToTm() gave us data in
    // normalized form, so the only excuse for !good or a change to the HH:mm:ss
    // fields (aside from being at the boundary of time_t's supported range) is
    // that we hit a gap, although we have to handle these cases differently:
    if (!result.good) {
        // Rejected. The tricky case: maybe mktime() doesn't resolve gaps.
        return resolveRejected(base, result, resolve);
    } else if (result.local.tm_isdst < 0) {
        // Apparently success without knowledge of whether this is DST or not.
        // Should not happen, but that means our usual understanding of what the
        // system is up to has gone out the window. So just let it be.
    } else if (result.adjusted) {
        // Shunted out of a gap.
        if (!resolve.testAnyFlags(QDateTimePrivate::GapMask)) {
            result = {};
            return result;
        }

        // Try to obtain a matching point on the other side of the gap:
        const MkTimeResult flipped = hopAcrossGap(result, base);
        // Even if that failed, result may be the correct resolution

        if (preferAlternative(resolve, result.local.tm_isdst, flipped.local.tm_isdst,
                              flipped.utcSecs > result.utcSecs, true)) {
            // If hopAcrossGap() failed and we do need its answer, give up.
            if (!flipped.good || flipped.adjusted)
                return {};

            // As resolution of local, flipped involves adjustment (across gap):
            result = flipped;
            result.adjusted = true;
        }
    } else if (resolve.testFlag(QDateTimePrivate::FlipForReverseDst)
               // In fold, DST counts as before and standard as after -
               // we may not need to check whether we're in a transition:
               && resolve.testFlag(result.local.tm_isdst ? QDateTimePrivate::FoldUseBefore
                                                         : QDateTimePrivate::FoldUseAfter)) {
        // We prefer DST or standard and got what we wanted, so we're good.
        // As below, but we don't need to check, because we're on the side of
        // the transition that it would select as valid, if we were near one.
        // NB: this branch is routinely exercised, when QDT::Data::isShort()
        // obliges us to rediscover an offsetFromUtc that ShortData has no space
        // to store, as it does remember the DST status we got before.
    } else {
        // What we gave was valid. However, it might have been in a fall-back.
        // If so, the same input but with tm_isdst flipped should also be valid.
        struct tm copy = base;
        copy.tm_isdst = !result.local.tm_isdst;
        const MkTimeResult flipped(copy);
        if (flipped.good && !flipped.adjusted) {
            // We're in a fall-back
            if (!resolve.testAnyFlags(QDateTimePrivate::FoldMask)) {
                result = {};
                return result;
            }

            // Work out which repeat to use:
            if (preferAlternative(resolve, result.local.tm_isdst, flipped.local.tm_isdst,
                                  flipped.utcSecs > result.utcSecs, false)) {
                result = flipped;
            }
        } // else: not in a transition, nothing to worry about.
    }
    return result;
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

QString localTimeAbbbreviationAt(qint64 local, QDateTimePrivate::TransitionOptions resolve)
{
    auto use = resolveLocalTime(QRoundingDown::qDiv<MSECS_PER_SEC>(local), resolve);
    if (!use.good)
        return {};
#ifdef HAVE_TM_ZONE
    if (use.local.tm_zone)
        return QString::fromLocal8Bit(use.local.tm_zone);
#endif
    return qTzName(use.local.tm_isdst > 0 ? 1 : 0);
}

QDateTimePrivate::ZoneState mapLocalTime(qint64 local, QDateTimePrivate::TransitionOptions resolve)
{
    // Revised later to match what use.local tells us:
    qint64 localSecs = local / MSECS_PER_SEC;
    auto use = resolveLocalTime(localSecs, resolve);
    if (!use.good)
        return {local};

    qint64 millis = local - localSecs * MSECS_PER_SEC;
    // Division is defined to round towards zero:
    Q_ASSERT(local < 0 ? (millis <= 0 && millis > -MSECS_PER_SEC)
                       : (millis >= 0 && millis < MSECS_PER_SEC));

    QDateTimePrivate::DaylightStatus dst =
        use.local.tm_isdst > 0 ? QDateTimePrivate::DaylightTime : QDateTimePrivate::StandardTime;

#ifdef HAVE_TM_GMTOFF
    const int offset = use.local.tm_gmtoff;
    localSecs = offset + use.utcSecs;
#else
    // Provisional offset, until we have a revised localSecs:
    int offset = localSecs - use.utcSecs;
    auto jd = tmToJd(use.local);
    if (Q_UNLIKELY(!jd))
        return {local, offset, dst, false};

    qint64 daySecs = tmSecsWithinDay(use.local);
    Q_ASSERT(0 <= daySecs && daySecs < SECS_PER_DAY);
    if (daySecs > 0 && *jd < JULIAN_DAY_FOR_EPOCH) {
        jd = *jd + 1;
        daySecs -= SECS_PER_DAY;
    }
    if (Q_UNLIKELY(daysAndSecondsOverflow(*jd, daySecs, &localSecs)))
        return {local, offset, dst, false};

    // Use revised localSecs to refine offset:
    offset = localSecs - use.utcSecs;
#endif // HAVE_TM_GMTOFF

    // The only way localSecs and millis can now have opposite sign is for
    // resolution of the local time to have kicked us across the epoch, in which
    // case there's no danger of overflow. So if overflow is in danger of
    // happening, we're already doing the best we can to avoid it.
    qint64 revised;
    if (secondsAndMillisOverflow(localSecs, millis, &revised))
        return {local, offset, QDateTimePrivate::UnknownDaylightTime, false};
    return {revised, offset, dst, true};
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
