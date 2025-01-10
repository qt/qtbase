// Copyright (C) 2019 The Qt Company Ltd.
// Copyright (C) 2013 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtimezone.h"
#include "qtimezoneprivate_p.h"

#include "private/qcore_mac_p.h"
#include "qstringlist.h"

#include <Foundation/NSTimeZone.h>

#include <qdebug.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

/*
    Private

    OS X system implementation
*/

// Create the system default time zone
QMacTimeZonePrivate::QMacTimeZonePrivate()
{
    // Reset the cached system tz then instantiate it:
    [NSTimeZone resetSystemTimeZone];
    m_nstz = [NSTimeZone.systemTimeZone retain];
    Q_ASSERT(m_nstz);
    m_id = QString::fromNSString(m_nstz.name).toUtf8();
}

// Create a named time zone
QMacTimeZonePrivate::QMacTimeZonePrivate(const QByteArray &ianaId)
    : m_nstz(nil)
{
    init(ianaId);
}

QMacTimeZonePrivate::QMacTimeZonePrivate(const QMacTimeZonePrivate &other)
    : QTimeZonePrivate(other), m_nstz([other.m_nstz copy])
{
}

QMacTimeZonePrivate::~QMacTimeZonePrivate()
{
    [m_nstz release];
}

QMacTimeZonePrivate *QMacTimeZonePrivate::clone() const
{
    return new QMacTimeZonePrivate(*this);
}

void QMacTimeZonePrivate::init(const QByteArray &ianaId)
{
    m_nstz = [[NSTimeZone timeZoneWithName:QString::fromUtf8(ianaId).toNSString()] retain];
    if (m_nstz) {
        m_id = ianaId;
    } else {
        // macOS has been seen returning a systemTimeZone which reports its name
        // as Asia/Kolkata, which doesn't appear in knownTimeZoneNames (which
        // calls the zone Asia/Calcutta). So explicitly check for the name
        // systemTimeZoneId() returns, and use systemTimeZone if we get it:
        m_nstz = [NSTimeZone.systemTimeZone retain];
        Q_ASSERT(m_nstz);
        if (QString::fromNSString(m_nstz.name).toUtf8() == ianaId)
            m_id = ianaId;
    }
}

QString QMacTimeZonePrivate::comment() const
{
    return QString::fromNSString(m_nstz.description);
}

QString QMacTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                         QTimeZone::NameType nameType,
                                         const QLocale &locale) const
{
    // TODO Mac doesn't support OffsetName yet so use standard offset name
    if (nameType == QTimeZone::OffsetName) {
        const Data nowData = data(QDateTime::currentMSecsSinceEpoch());
        // TODO Cheat for now, assume if has dst the offset if 1 hour
        if (timeType == QTimeZone::DaylightTime && hasDaylightTime())
            return isoOffsetFormat(nowData.standardTimeOffset + 3600);
        else
            return isoOffsetFormat(nowData.standardTimeOffset);
    }

    NSTimeZoneNameStyle style = NSTimeZoneNameStyleStandard;

    switch (nameType) {
    case QTimeZone::ShortName :
        if (timeType == QTimeZone::DaylightTime)
            style = NSTimeZoneNameStyleShortDaylightSaving;
        else if (timeType == QTimeZone::GenericTime)
            style = NSTimeZoneNameStyleShortGeneric;
        else
            style = NSTimeZoneNameStyleShortStandard;
        break;
    case QTimeZone::DefaultName :
    case QTimeZone::LongName :
        if (timeType == QTimeZone::DaylightTime)
            style = NSTimeZoneNameStyleDaylightSaving;
        else if (timeType == QTimeZone::GenericTime)
            style = NSTimeZoneNameStyleGeneric;
        else
            style = NSTimeZoneNameStyleStandard;
        break;
    case QTimeZone::OffsetName :
        // Unreachable
        break;
    }

    NSString *macLocaleCode = locale.name().toNSString();
    NSLocale *macLocale = [[NSLocale alloc] initWithLocaleIdentifier:macLocaleCode];
    const QString result = QString::fromNSString([m_nstz localizedName:style locale:macLocale]);
    [macLocale release];
    return result;
}

QString QMacTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
    return QString::fromNSString([m_nstz abbreviationForDate:[NSDate dateWithTimeIntervalSince1970:seconds]]);
}

int QMacTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
    return [m_nstz secondsFromGMTForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

int QMacTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    return offsetFromUtc(atMSecsSinceEpoch) - daylightTimeOffset(atMSecsSinceEpoch);
}

int QMacTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
    return [m_nstz daylightSavingTimeOffsetForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

bool QMacTimeZonePrivate::hasDaylightTime() const
{
    // TODO No Mac API, assume if has transitions
    return hasTransitions();
}

bool QMacTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
    return [m_nstz isDaylightSavingTimeForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

QTimeZonePrivate::Data QMacTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    const NSTimeInterval seconds = forMSecsSinceEpoch / 1000.0;
    NSDate *date = [NSDate dateWithTimeIntervalSince1970:seconds];
    Data data;
    data.atMSecsSinceEpoch = forMSecsSinceEpoch;
    data.offsetFromUtc = [m_nstz secondsFromGMTForDate:date];
    data.daylightTimeOffset = [m_nstz daylightSavingTimeOffsetForDate:date];
    data.standardTimeOffset = data.offsetFromUtc - data.daylightTimeOffset;
    data.abbreviation = QString::fromNSString([m_nstz abbreviationForDate:date]);
    return data;
}

bool QMacTimeZonePrivate::hasTransitions() const
{
    // TODO No direct Mac API, so return if has next after 1970, i.e. since start of tz
    // TODO Not sure what is returned in event of no transitions, assume will be before requested date
    NSDate *epoch = [NSDate dateWithTimeIntervalSince1970:0];
    const NSDate *date = [m_nstz nextDaylightSavingTimeTransitionAfterDate:epoch];
    const bool result = (date.timeIntervalSince1970 > epoch.timeIntervalSince1970);
    return result;
}

QTimeZonePrivate::Data QMacTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    QTimeZonePrivate::Data tran;
    const NSTimeInterval seconds = afterMSecsSinceEpoch / 1000.0;
    NSDate *nextDate = [NSDate dateWithTimeIntervalSince1970:seconds];
    nextDate = [m_nstz nextDaylightSavingTimeTransitionAfterDate:nextDate];
    const NSTimeInterval nextSecs = nextDate.timeIntervalSince1970;
    if (nextDate == nil || nextSecs <= seconds) {
        [nextDate release];
        return invalidData();
    }
    tran.atMSecsSinceEpoch = nextSecs * 1000;
    tran.offsetFromUtc = [m_nstz secondsFromGMTForDate:nextDate];
    tran.daylightTimeOffset = [m_nstz daylightSavingTimeOffsetForDate:nextDate];
    tran.standardTimeOffset = tran.offsetFromUtc - tran.daylightTimeOffset;
    tran.abbreviation = QString::fromNSString([m_nstz abbreviationForDate:nextDate]);
    return tran;
}

QTimeZonePrivate::Data QMacTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    // The native API only lets us search forward, so we need to find an early-enough start:
    const NSTimeInterval lowerBound = std::numeric_limits<NSTimeInterval>::lowest();
    const qint64 endSecs = beforeMSecsSinceEpoch / 1000;
    const int year = 366 * 24 * 3600; // a (long) year, in seconds
    NSTimeInterval prevSecs = endSecs; // sentinel for later check
    NSTimeInterval nextSecs = prevSecs - year;
    NSTimeInterval tranSecs = lowerBound; // time at a transition; may be > endSecs

    NSDate *nextDate = [NSDate dateWithTimeIntervalSince1970:nextSecs];
    nextDate = [m_nstz nextDaylightSavingTimeTransitionAfterDate:nextDate];
    if (nextDate != nil
        && (tranSecs = nextDate.timeIntervalSince1970) < endSecs) {
        // There's a transition within the last year before endSecs:
        nextSecs = tranSecs;
    } else {
        // Need to start our search earlier:
        nextDate = [NSDate dateWithTimeIntervalSince1970:lowerBound];
        nextDate = [m_nstz nextDaylightSavingTimeTransitionAfterDate:nextDate];
        if (nextDate != nil) {
            NSTimeInterval lateSecs = nextSecs;
            nextSecs = nextDate.timeIntervalSince1970;
            Q_ASSERT(nextSecs <= endSecs - year || nextSecs == tranSecs);
            /*
              We're looking at the first ever transition for our zone, at
              nextSecs (and our zone *does* have at least one transition).  If
              it's later than endSecs - year, then we must have found it on the
              initial check and therefore set tranSecs to the same transition
              time (which, we can infer here, is >= endSecs).  In this case, we
              won't enter the binary-chop loop, below.

              In the loop, nextSecs < lateSecs < endSecs: we have a transition
              at nextSecs and there is no transition between lateSecs and
              endSecs.  The loop narrows the interval between nextSecs and
              lateSecs by looking for a transition after their mid-point; if it
              finds one < endSecs, nextSecs moves to this transition; otherwise,
              lateSecs moves to the mid-point.  This soon enough narrows the gap
              to within a year, after which walking forward one transition at a
              time (the "Wind through" loop, below) is good enough.
            */

            // Binary chop to within a year of last transition before endSecs:
            while (nextSecs + year < lateSecs) {
                // Careful about overflow, not fussy about rounding errors:
                NSTimeInterval middle = nextSecs / 2 + lateSecs / 2;
                NSDate *split = [NSDate dateWithTimeIntervalSince1970:middle];
                split = [m_nstz nextDaylightSavingTimeTransitionAfterDate:split];
                if (split != nil && (tranSecs = split.timeIntervalSince1970) < endSecs) {
                    nextDate = split;
                    nextSecs = tranSecs;
                } else {
                    lateSecs = middle;
                }
            }
            Q_ASSERT(nextDate != nil);
            // ... and nextSecs < endSecs unless first transition ever was >= endSecs.
        } // else: we have no data - prevSecs is still endSecs, nextDate is still nil
    }
    // Either nextDate is nil or nextSecs is at its transition.

    // Wind through remaining transitions (spanning at most a year), one at a time:
    while (nextDate != nil && nextSecs < endSecs) {
        prevSecs = nextSecs;
        nextDate = [m_nstz nextDaylightSavingTimeTransitionAfterDate:nextDate];
        nextSecs = nextDate.timeIntervalSince1970;
        if (nextSecs <= prevSecs) // presumably no later data available
            break;
    }
    if (prevSecs < endSecs) // i.e. we did make it into that while loop
        return data(qint64(prevSecs * 1e3));

    // No transition data; or first transition later than requested time.
    return invalidData();
}

QByteArray QMacTimeZonePrivate::systemTimeZoneId() const
{
    // Reset the cached system tz then return the name
    [NSTimeZone resetSystemTimeZone];
    Q_ASSERT(NSTimeZone.systemTimeZone);
    return QString::fromNSString(NSTimeZone.systemTimeZone.name).toUtf8();
}

bool QMacTimeZonePrivate::isTimeZoneIdAvailable(const QByteArray& ianaId) const
{
    QMacAutoReleasePool pool;
    return [NSTimeZone timeZoneWithName:QString::fromUtf8(ianaId).toNSString()] != nil;
}

QList<QByteArray> QMacTimeZonePrivate::availableTimeZoneIds() const
{
    NSEnumerator *enumerator = NSTimeZone.knownTimeZoneNames.objectEnumerator;
    QByteArray tzid = QString::fromNSString(enumerator.nextObject).toUtf8();

    QList<QByteArray> list;
    while (!tzid.isEmpty()) {
        list << tzid;
        tzid = QString::fromNSString(enumerator.nextObject).toUtf8();
    }

    std::sort(list.begin(), list.end());
    list.erase(std::unique(list.begin(), list.end()), list.end());

    return list;
}

NSTimeZone *QMacTimeZonePrivate::nsTimeZone() const
{
    return m_nstz;
}

QT_END_NAMESPACE
