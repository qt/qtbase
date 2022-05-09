// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlocaltime_p.h"
#include "qplatformdefs.h"

#include "private/qgregoriancalendar_p.h"
#include "private/qnumeric_p.h"
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
// constexpr int tmYearFromQYear(int year) { return year - (year < 0 ? 1899 : 1900); }
constexpr int qYearFromTmYear(int year) { return year + (year < -1899 ? 1899 : 1900); }

bool qtLocalTime(time_t utc, struct tm *local)
{
    // This should really be done under the environmentMutex wrapper qglobal.cpp
    // uses in qTzSet() and friends. However, the only sane way to do that would
    // be to move this whole function there (and replace its qTzSet() with a
    // naked tzset(), since it'd already be mutex-protected).
#if defined(Q_OS_WIN)
    // The doc of localtime_s() doesn't explicitly say that it calls _tzset(),
    // but does say that localtime_s() corrects for the same things _tzset()
    // sets the globals for, so presumably localtime_s() behaves as if it did.
    return !localtime_s(local, &utc);
#elif QT_CONFIG(thread) && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    // Use the reentrant version of localtime() where available, as it is
    // thread-safe and doesn't use a shared static data area.
    // As localtime() is specified to work as if it called tzset(), but
    // localtime_r() does not have this constraint, make an explicit call.
    // The explicit call should also request a re-parse of timezone info.
    qTzSet();
    if (tm *res = localtime_r(&utc, local)) {
        Q_ASSERT(res == local);
        return true;
    }
    return false;
#else
    // Returns shared static data which may be overwritten at any time
    // So copy the result asap
    if (tm *res = localtime(&utc)) {
        *local = *res;
        return true;
    }
    return false;
#endif
}
} // namespace

namespace QLocalTime {

// Calls the platform variant of localtime() for the given utcMillis, and
// returns the local milliseconds, offset from UTC and DST status.
QDateTimePrivate::ZoneState utcToLocal(qint64 utcMillis)
{
    const int signFix = utcMillis % MSECS_PER_SEC && utcMillis < 0 ? 1 : 0;
    const time_t epochSeconds = utcMillis / MSECS_PER_SEC - signFix;
    const int msec = utcMillis % MSECS_PER_SEC + signFix * MSECS_PER_SEC;
    Q_ASSERT(msec >= 0 && msec < MSECS_PER_SEC);
    if (qint64(epochSeconds) * MSECS_PER_SEC + msec != utcMillis)
        return {utcMillis};

    tm local;
    if (!qtLocalTime(epochSeconds, &local))
        return {utcMillis};

    qint64 jd;
    if (Q_UNLIKELY(!QGregorianCalendar::julianFromParts(qYearFromTmYear(local.tm_year),
                                                        local.tm_mon + 1, local.tm_mday, &jd))) {
        return {utcMillis};
    }
    const qint64 daySeconds
        = (local.tm_hour * 60 + local.tm_min) * 60 + local.tm_sec;
    Q_ASSERT(0 <= daySeconds && daySeconds < SECS_PER_DAY);
    qint64 localSeconds, localMillis;
    if (Q_UNLIKELY(
            mul_overflow(jd - JULIAN_DAY_FOR_EPOCH, std::integral_constant<qint64, SECS_PER_DAY>(),
                         &localSeconds)
            || add_overflow(localSeconds, daySeconds, &localSeconds)
            || mul_overflow(localSeconds, std::integral_constant<qint64, MSECS_PER_SEC>(),
                            &localMillis)
            || add_overflow(localMillis, qint64(msec), &localMillis))) {
        return {utcMillis};
    }
    const auto dst
        = local.tm_isdst ? QDateTimePrivate::DaylightTime : QDateTimePrivate::StandardTime;
    return { localMillis, int(localSeconds - epochSeconds), dst };
}

} // QLocalTime

QT_END_NAMESPACE
