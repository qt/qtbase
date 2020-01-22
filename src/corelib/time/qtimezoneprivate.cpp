/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Copyright (C) 2013 John Layt <jlayt@kde.org>
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


#include "qtimezone.h"
#include "qtimezoneprivate_p.h"
#include "qtimezoneprivate_data_p.h"

#include <qdatastream.h>
#include <qdebug.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

/*
    Static utilities for looking up Windows ID tables
*/

static const int windowsDataTableSize = sizeof(windowsDataTable) / sizeof(QWindowsData) - 1;
static const int zoneDataTableSize = sizeof(zoneDataTable) / sizeof(QZoneData) - 1;
static const int utcDataTableSize = sizeof(utcDataTable) / sizeof(QUtcData) - 1;


static const QZoneData *zoneData(quint16 index)
{
    Q_ASSERT(index < zoneDataTableSize);
    return &zoneDataTable[index];
}

static const QWindowsData *windowsData(quint16 index)
{
    Q_ASSERT(index < windowsDataTableSize);
    return &windowsDataTable[index];
}

static const QUtcData *utcData(quint16 index)
{
    Q_ASSERT(index < utcDataTableSize);
    return &utcDataTable[index];
}

// Return the Windows ID literal for a given QWindowsData
static QByteArray windowsId(const QWindowsData *windowsData)
{
    return (windowsIdData + windowsData->windowsIdIndex);
}

// Return the IANA ID literal for a given QWindowsData
static QByteArray ianaId(const QWindowsData *windowsData)
{
    return (ianaIdData + windowsData->ianaIdIndex);
}

// Return the IANA ID literal for a given QZoneData
static QByteArray ianaId(const QZoneData *zoneData)
{
    return (ianaIdData + zoneData->ianaIdIndex);
}

static QByteArray utcId(const QUtcData *utcData)
{
    return (ianaIdData + utcData->ianaIdIndex);
}

static quint16 toWindowsIdKey(const QByteArray &winId)
{
    for (quint16 i = 0; i < windowsDataTableSize; ++i) {
        const QWindowsData *data = windowsData(i);
        if (windowsId(data) == winId)
            return data->windowsIdKey;
    }
    return 0;
}

static QByteArray toWindowsIdLiteral(quint16 windowsIdKey)
{
    for (quint16 i = 0; i < windowsDataTableSize; ++i) {
        const QWindowsData *data = windowsData(i);
        if (data->windowsIdKey == windowsIdKey)
            return windowsId(data);
    }
    return QByteArray();
}

/*
    Base class implementing common utility routines, only intantiate for a null tz.
*/

QTimeZonePrivate::QTimeZonePrivate()
{
}

QTimeZonePrivate::QTimeZonePrivate(const QTimeZonePrivate &other)
    : QSharedData(other), m_id(other.m_id)
{
}

QTimeZonePrivate::~QTimeZonePrivate()
{
}

QTimeZonePrivate *QTimeZonePrivate::clone() const
{
    return new QTimeZonePrivate(*this);
}

bool QTimeZonePrivate::operator==(const QTimeZonePrivate &other) const
{
    // TODO Too simple, but need to solve problem of comparing different derived classes
    // Should work for all System and ICU classes as names guaranteed unique, but not for Simple.
    // Perhaps once all classes have working transitions can compare full list?
    return (m_id == other.m_id);
}

bool QTimeZonePrivate::operator!=(const QTimeZonePrivate &other) const
{
    return !(*this == other);
}

bool QTimeZonePrivate::isValid() const
{
    return !m_id.isEmpty();
}

QByteArray QTimeZonePrivate::id() const
{
    return m_id;
}

QLocale::Country QTimeZonePrivate::country() const
{
    // Default fall-back mode, use the zoneTable to find Region of known Zones
    for (int i = 0; i < zoneDataTableSize; ++i) {
        const QZoneData *data = zoneData(i);
        if (ianaId(data).split(' ').contains(m_id))
            return (QLocale::Country)data->country;
    }
    return QLocale::AnyCountry;
}

QString QTimeZonePrivate::comment() const
{
    return QString();
}

QString QTimeZonePrivate::displayName(qint64 atMSecsSinceEpoch,
                                      QTimeZone::NameType nameType,
                                      const QLocale &locale) const
{
    if (nameType == QTimeZone::OffsetName)
        return isoOffsetFormat(offsetFromUtc(atMSecsSinceEpoch));

    if (isDaylightTime(atMSecsSinceEpoch))
        return displayName(QTimeZone::DaylightTime, nameType, locale);
    else
        return displayName(QTimeZone::StandardTime, nameType, locale);
}

QString QTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                      QTimeZone::NameType nameType,
                                      const QLocale &locale) const
{
    Q_UNUSED(timeType)
    Q_UNUSED(nameType)
    Q_UNUSED(locale)
    return QString();
}

QString QTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return QString();
}

int QTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    return standardTimeOffset(atMSecsSinceEpoch) + daylightTimeOffset(atMSecsSinceEpoch);
}

int QTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return invalidSeconds();
}

int QTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return invalidSeconds();
}

bool QTimeZonePrivate::hasDaylightTime() const
{
    return false;
}

bool QTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return false;
}

QTimeZonePrivate::Data QTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    Q_UNUSED(forMSecsSinceEpoch)
    return invalidData();
}

// Private only method for use by QDateTime to convert local msecs to epoch msecs
QTimeZonePrivate::Data QTimeZonePrivate::dataForLocalTime(qint64 forLocalMSecs, int hint) const
{
    if (!hasDaylightTime()) // No DST means same offset for all local msecs
        return data(forLocalMSecs - standardTimeOffset(forLocalMSecs) * 1000);

    /*
      We need a UTC time at which to ask for the offset, in order to be able to
      add that offset to forLocalMSecs, to get the UTC time we
      need. Fortunately, no time-zone offset is more than 14 hours; and DST
      transitions happen (much) more than thirty-two hours apart.  So sampling
      offset sixteen hours each side gives us information we can be sure
      brackets the correct time and at most one DST transition.
    */
    const qint64 sixteenHoursInMSecs(16 * 3600 * 1000);
    Q_STATIC_ASSERT(-sixteenHoursInMSecs / 1000 < QTimeZone::MinUtcOffsetSecs
                  && sixteenHoursInMSecs / 1000 > QTimeZone::MaxUtcOffsetSecs);
    const qint64 recent = forLocalMSecs - sixteenHoursInMSecs;
    const qint64 imminent = forLocalMSecs + sixteenHoursInMSecs;
    /*
      Offsets are Local - UTC, positive to the east of Greenwich, negative to
      the west; DST offset always exceeds standard offset, when DST applies.
      When we have offsets on either side of a transition, the lower one is
      standard, the higher is DST.

      Non-DST transitions (jurisdictions changing time-zone and time-zones
      changing their standard offset, typically) are described below as if they
      were DST transitions (since these are more usual and familiar); the code
      mostly concerns itself with offsets from UTC, described in terms of the
      common case for changes in that.  If there is no actual change in offset
      (e.g. a DST transition cancelled by a standard offset change), this code
      should handle it gracefully; without transitions, it'll see early == late
      and take the easy path; with transitions, tran and nextTran get the
      correct UTC time as atMSecsSinceEpoch so comparing to nextStart selects
      the right one.  In all other cases, the transition changes offset and the
      reasoning that applies to DST applies just the same.  Aside from hinting,
      the only thing that looks at DST-ness at all, other than inferred from
      offset changes, is the case without transition data handling an invalid
      time in the gap that a transition passed over.

      The handling of hint (see below) is apt to go wrong in non-DST
      transitions.  There isn't really a great deal we can hope to do about that
      without adding yet more unreliable complexity to the heuristics in use for
      already obscure corner-cases.
     */

    /*
      The hint (really a QDateTimePrivate::DaylightStatus) is > 0 if caller
      thinks we're in DST, 0 if in standard.  A value of -2 means never-DST, so
      should have been handled above; if it slips through, it's wrong but we
      should probably treat it as standard anyway (never-DST means
      always-standard, after all).  If the hint turns out to be wrong, fall back
      on trying the other possibility: which makes it harmless to treat -1
      (meaning unknown) as standard (i.e. try standard first, then try DST).  In
      practice, away from a transition, the only difference hint makes is to
      which candidate we try first: if the hint is wrong (or unknown and
      standard fails), we'll try the other candidate and it'll work.

      For the obscure (and invalid) case where forLocalMSecs falls in a
      spring-forward's missing hour, a common case is that we started with a
      date/time for which the hint was valid and adjusted it naively; for that
      case, we should correct the adjustment by shunting across the transition
      into where hint is wrong.  So half-way through the gap, arrived at from
      the DST side, should be read as an hour earlier, in standard time; but, if
      arrived at from the standard side, should be read as an hour later, in
      DST.  (This shall be wrong in some cases; for example, when a country
      changes its transition dates and changing a date/time by more than six
      months lands it on a transition.  However, these cases are even more
      obscure than those where the heuristic is good.)
     */

    if (hasTransitions()) {
        /*
          We have transitions.

          Each transition gives the offsets to use until the next; so we need the
          most recent transition before the time forLocalMSecs describes.  If it
          describes a time *in* a transition, we'll need both that transition and
          the one before it.  So find one transition that's probably after (and not
          much before, otherwise) and another that's definitely before, then work
          out which one to use.  When both or neither work on forLocalMSecs, use
          hint to disambiguate.
        */

        // Get a transition definitely before the local MSecs; usually all we need.
        // Only around the transition times might we need another.
        Data tran = previousTransition(recent);
        Q_ASSERT(forLocalMSecs < 0 || // Pre-epoch TZ info may be unavailable
                 forLocalMSecs - tran.offsetFromUtc * 1000 >= tran.atMSecsSinceEpoch);
        Data nextTran = nextTransition(tran.atMSecsSinceEpoch);
        /*
          Now walk those forward until they bracket forLocalMSecs with transitions.

          One of the transitions should then be telling us the right offset to use.
          In a transition, we need the transition before it (to describe the run-up
          to the transition) and the transition itself; so we need to stop when
          nextTran is that transition.
        */
        while (nextTran.atMSecsSinceEpoch != invalidMSecs()
               && forLocalMSecs > nextTran.atMSecsSinceEpoch + nextTran.offsetFromUtc * 1000) {
            Data newTran = nextTransition(nextTran.atMSecsSinceEpoch);
            if (newTran.atMSecsSinceEpoch == invalidMSecs()
                || newTran.atMSecsSinceEpoch + newTran.offsetFromUtc * 1000 > imminent) {
                // Definitely not a relevant tansition: too far in the future.
                break;
            }
            tran = nextTran;
            nextTran = newTran;
        }

        // Check we do *really* have transitions for this zone:
        if (tran.atMSecsSinceEpoch != invalidMSecs()) {

            /*
              So now tran is definitely before and nextTran is either after or only
              slightly before.  One is standard time; we interpret the other as DST
              (although the transition might in fact by a change in standard offset).  Our
              hint tells us which of those to use (defaulting to standard if no hint): try
              it first; if that fails, try the other; if both fail, life's tricky.
            */
            Q_ASSERT(forLocalMSecs < 0
                     || forLocalMSecs - tran.offsetFromUtc * 1000 > tran.atMSecsSinceEpoch);
            const qint64 nextStart = nextTran.atMSecsSinceEpoch;
            // Work out the UTC values it might make sense to return:
            nextTran.atMSecsSinceEpoch = forLocalMSecs - nextTran.offsetFromUtc * 1000;
            tran.atMSecsSinceEpoch = forLocalMSecs - tran.offsetFromUtc * 1000;

            // If both or neither have zero DST, treat the one with lower offset as standard:
            const bool nextIsDst = !nextTran.daylightTimeOffset == !tran.daylightTimeOffset
                ? tran.offsetFromUtc < nextTran.offsetFromUtc : nextTran.daylightTimeOffset;
            // If that agrees with hint > 0, our first guess is to use nextTran; else tran.
            const bool nextFirst = nextIsDst == (hint > 0) && nextStart != invalidMSecs();
            for (int i = 0; i < 2; i++) {
                /*
                  On the first pass, the case we consider is what hint told us to expect
                  (except when hint was -1 and didn't actually tell us what to expect),
                  so it's likely right.  We only get a second pass if the first failed,
                  by which time the second case, that we're trying, is likely right.
                */
                if (nextFirst ? i == 0 : i) {
                    Q_ASSERT(nextStart != invalidMSecs());
                    if (nextStart <= nextTran.atMSecsSinceEpoch)
                        return nextTran;
                } else {
                    // If next is invalid, nextFirst is false, to route us here first:
                    if (nextStart == invalidMSecs() || nextStart > tran.atMSecsSinceEpoch)
                        return tran;
                }
            }

            /*
              Neither is valid (e.g. in a spring-forward's gap) and
              nextTran.atMSecsSinceEpoch < nextStart <= tran.atMSecsSinceEpoch, so
              0 < tran.atMSecsSinceEpoch - nextTran.atMSecsSinceEpoch
              = (nextTran.offsetFromUtc - tran.offsetFromUtc) * 1000
            */
            int dstStep = (nextTran.offsetFromUtc - tran.offsetFromUtc) * 1000;
            Q_ASSERT(dstStep > 0); // How else could we get here ?
            if (nextFirst) { // hint thought we needed nextTran, so use tran
                tran.atMSecsSinceEpoch -= dstStep;
                return tran;
            }
            nextTran.atMSecsSinceEpoch += dstStep;
            return nextTran;
        }
        // System has transitions but not for this zone.
        // Try falling back to offsetFromUtc
    }

    /* Bracket and refine to discover offset. */
    qint64 utcEpochMSecs;

    int early = offsetFromUtc(recent);
    int late = offsetFromUtc(imminent);
    if (early == late) { // > 99% of the time
        utcEpochMSecs = forLocalMSecs - early * 1000;
    } else {
        // Close to a DST transition: early > late is near a fall-back,
        // early < late is near a spring-forward.
        const int offsetInDst = qMax(early, late);
        const int offsetInStd = qMin(early, late);
        // Candidate values for utcEpochMSecs (if forLocalMSecs is valid):
        const qint64 forDst = forLocalMSecs - offsetInDst * 1000;
        const qint64 forStd = forLocalMSecs - offsetInStd * 1000;
        // Best guess at the answer:
        const qint64 hinted = hint > 0 ? forDst : forStd;
        if (offsetFromUtc(hinted) == (hint > 0 ? offsetInDst : offsetInStd)) {
            utcEpochMSecs = hinted;
        } else if (hint <= 0 && offsetFromUtc(forDst) == offsetInDst) {
            utcEpochMSecs = forDst;
        } else if (hint > 0 && offsetFromUtc(forStd) == offsetInStd) {
            utcEpochMSecs = forStd;
        } else {
            // Invalid forLocalMSecs: in spring-forward gap.
            const int dstStep = daylightTimeOffset(early < late ? imminent : recent) * 1000;
            Q_ASSERT(dstStep); // There can't be a transition without it !
            utcEpochMSecs = (hint > 0) ? forStd - dstStep : forDst + dstStep;
        }
    }

    return data(utcEpochMSecs);
}

bool QTimeZonePrivate::hasTransitions() const
{
    return false;
}

QTimeZonePrivate::Data QTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    Q_UNUSED(afterMSecsSinceEpoch)
    return invalidData();
}

QTimeZonePrivate::Data QTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    Q_UNUSED(beforeMSecsSinceEpoch)
    return invalidData();
}

QTimeZonePrivate::DataList QTimeZonePrivate::transitions(qint64 fromMSecsSinceEpoch,
                                                         qint64 toMSecsSinceEpoch) const
{
    DataList list;
    if (toMSecsSinceEpoch >= fromMSecsSinceEpoch) {
        // fromMSecsSinceEpoch is inclusive but nextTransitionTime() is exclusive so go back 1 msec
        Data next = nextTransition(fromMSecsSinceEpoch - 1);
        while (next.atMSecsSinceEpoch != invalidMSecs()
               && next.atMSecsSinceEpoch <= toMSecsSinceEpoch) {
            list.append(next);
            next = nextTransition(next.atMSecsSinceEpoch);
        }
    }
    return list;
}

QByteArray QTimeZonePrivate::systemTimeZoneId() const
{
    return QByteArray();
}

bool QTimeZonePrivate::isTimeZoneIdAvailable(const QByteArray& ianaId) const
{
    // Fall-back implementation, can be made faster in subclasses
    const QList<QByteArray> tzIds = availableTimeZoneIds();
    return std::binary_search(tzIds.begin(), tzIds.end(), ianaId);
}

QList<QByteArray> QTimeZonePrivate::availableTimeZoneIds() const
{
    return QList<QByteArray>();
}

QList<QByteArray> QTimeZonePrivate::availableTimeZoneIds(QLocale::Country country) const
{
    // Default fall-back mode, use the zoneTable to find Region of know Zones
    QList<QByteArray> regions;

    // First get all Zones in the Zones table belonging to the Region
    for (int i = 0; i < zoneDataTableSize; ++i) {
        if (zoneData(i)->country == country)
            regions += ianaId(zoneData(i)).split(' ');
    }

    std::sort(regions.begin(), regions.end());
    regions.erase(std::unique(regions.begin(), regions.end()), regions.end());

    // Then select just those that are available
    const QList<QByteArray> all = availableTimeZoneIds();
    QList<QByteArray> result;
    result.reserve(qMin(all.size(), regions.size()));
    std::set_intersection(all.begin(), all.end(), regions.cbegin(), regions.cend(),
                          std::back_inserter(result));
    return result;
}

QList<QByteArray> QTimeZonePrivate::availableTimeZoneIds(int offsetFromUtc) const
{
    // Default fall-back mode, use the zoneTable to find Offset of know Zones
    QList<QByteArray> offsets;
    // First get all Zones in the table using the Offset
    for (int i = 0; i < windowsDataTableSize; ++i) {
        const QWindowsData *winData = windowsData(i);
        if (winData->offsetFromUtc == offsetFromUtc) {
            for (int j = 0; j < zoneDataTableSize; ++j) {
                const QZoneData *data = zoneData(j);
                if (data->windowsIdKey == winData->windowsIdKey)
                    offsets += ianaId(data).split(' ');
            }
        }
    }

    std::sort(offsets.begin(), offsets.end());
    offsets.erase(std::unique(offsets.begin(), offsets.end()), offsets.end());

    // Then select just those that are available
    const QList<QByteArray> all = availableTimeZoneIds();
    QList<QByteArray> result;
    result.reserve(qMin(all.size(), offsets.size()));
    std::set_intersection(all.begin(), all.end(), offsets.cbegin(), offsets.cend(),
                          std::back_inserter(result));
    return result;
}

#ifndef QT_NO_DATASTREAM
void QTimeZonePrivate::serialize(QDataStream &ds) const
{
    ds << QString::fromUtf8(m_id);
}
#endif // QT_NO_DATASTREAM

// Static Utility Methods

QTimeZonePrivate::Data QTimeZonePrivate::invalidData()
{
    Data data;
    data.atMSecsSinceEpoch = invalidMSecs();
    data.offsetFromUtc = invalidSeconds();
    data.standardTimeOffset = invalidSeconds();
    data.daylightTimeOffset = invalidSeconds();
    return data;
}

QTimeZone::OffsetData QTimeZonePrivate::invalidOffsetData()
{
    QTimeZone::OffsetData offsetData;
    offsetData.atUtc = QDateTime();
    offsetData.offsetFromUtc = invalidSeconds();
    offsetData.standardTimeOffset = invalidSeconds();
    offsetData.daylightTimeOffset = invalidSeconds();
    return offsetData;
}

QTimeZone::OffsetData QTimeZonePrivate::toOffsetData(const QTimeZonePrivate::Data &data)
{
    QTimeZone::OffsetData offsetData = invalidOffsetData();
    if (data.atMSecsSinceEpoch != invalidMSecs()) {
        offsetData.atUtc = QDateTime::fromMSecsSinceEpoch(data.atMSecsSinceEpoch, Qt::UTC);
        offsetData.offsetFromUtc = data.offsetFromUtc;
        offsetData.standardTimeOffset = data.standardTimeOffset;
        offsetData.daylightTimeOffset = data.daylightTimeOffset;
        offsetData.abbreviation = data.abbreviation;
    }
    return offsetData;
}

// Is the format of the ID valid ?
bool QTimeZonePrivate::isValidId(const QByteArray &ianaId)
{
    /*
      Main rules for defining TZ/IANA names as per ftp://ftp.iana.org/tz/code/Theory
       1. Use only valid POSIX file name components
       2. Within a file name component, use only ASCII letters, `.', `-' and `_'.
       3. Do not use digits (except in a [+-]\d+ suffix, when used).
       4. A file name component must not exceed 14 characters or start with `-'
      However, the rules are really guidelines - a later one says
       - Do not change established names if they only marginally violate the
         above rules.
      We may, therefore, need to be a bit slack in our check here, if we hit
      legitimate exceptions in real time-zone databases.

      In particular, aliases such as "Etc/GMT+7" and "SystemV/EST5EDT" are valid
      so we need to accept digits, ':', and '+'; aliases typically have the form
      of POSIX TZ strings, which allow a suffix to a proper IANA name.  A POSIX
      suffix starts with an offset (as in GMT+7) and may continue with another
      name (as in EST5EDT, giving the DST name of the zone); a further offset is
      allowed (for DST).  The ("hard to describe and [...] error-prone in
      practice") POSIX form even allows a suffix giving the dates (and
      optionally times) of the annual DST transitions.  Hopefully, no TZ aliases
      go that far, but we at least need to accept an offset and (single
      fragment) DST-name.

      But for the legacy complications, the following would be preferable if
      QRegExp would work on QByteArrays directly:
          const QRegExp rx(QStringLiteral("[a-z+._][a-z+._-]{,13}"
                                      "(?:/[a-z+._][a-z+._-]{,13})*"
                                          // Optional suffix:
                                          "(?:[+-]?\d{1,2}(?::\d{1,2}){,2}" // offset
                                             // one name fragment (DST):
                                             "(?:[a-z+._][a-z+._-]{,13})?)"),
                           Qt::CaseInsensitive);
          return rx.exactMatch(ianaId);
    */

    // Somewhat slack hand-rolled version:
    const int MinSectionLength = 1;
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
    // Android has its own naming of zones.
    // "Canada/East-Saskatchewan" has a 17-character second component.
    const int MaxSectionLength = 17;
#else
    const int MaxSectionLength = 14;
#endif
    int sectionLength = 0;
    for (const char *it = ianaId.begin(), * const end = ianaId.end(); it != end; ++it, ++sectionLength) {
        const char ch = *it;
        if (ch == '/') {
            if (sectionLength < MinSectionLength || sectionLength > MaxSectionLength)
                return false; // violates (4)
            sectionLength = -1;
        } else if (ch == '-') {
            if (sectionLength == 0)
                return false; // violates (4)
        } else if (!(ch >= 'a' && ch <= 'z')
                && !(ch >= 'A' && ch <= 'Z')
                && !(ch == '_')
                && !(ch == '.')
                   // Should ideally check these only happen as an offset:
                && !(ch >= '0' && ch <= '9')
                && !(ch == '+')
                && !(ch == ':')) {
            return false; // violates (2)
        }
    }
    if (sectionLength < MinSectionLength || sectionLength > MaxSectionLength)
        return false; // violates (4)
    return true;
}

QString QTimeZonePrivate::isoOffsetFormat(int offsetFromUtc)
{
    const int mins = offsetFromUtc / 60;
    return QString::fromUtf8("UTC%1%2:%3").arg(mins >= 0 ? QLatin1Char('+') : QLatin1Char('-'))
                                          .arg(qAbs(mins) / 60, 2, 10, QLatin1Char('0'))
                                          .arg(qAbs(mins) % 60, 2, 10, QLatin1Char('0'));
}

QByteArray QTimeZonePrivate::ianaIdToWindowsId(const QByteArray &id)
{
    for (int i = 0; i < zoneDataTableSize; ++i) {
        const QZoneData *data = zoneData(i);
        if (ianaId(data).split(' ').contains(id))
            return toWindowsIdLiteral(data->windowsIdKey);
    }
    return QByteArray();
}

QByteArray QTimeZonePrivate::windowsIdToDefaultIanaId(const QByteArray &windowsId)
{
    const quint16 windowsIdKey = toWindowsIdKey(windowsId);
    for (int i = 0; i < windowsDataTableSize; ++i) {
        const QWindowsData *data = windowsData(i);
        if (data->windowsIdKey == windowsIdKey)
            return ianaId(data);
    }
    return QByteArray();
}

QByteArray QTimeZonePrivate::windowsIdToDefaultIanaId(const QByteArray &windowsId,
                                                       QLocale::Country country)
{
    const QList<QByteArray> list = windowsIdToIanaIds(windowsId, country);
    if (list.count() > 0)
        return list.first();
    else
        return QByteArray();
}

QList<QByteArray> QTimeZonePrivate::windowsIdToIanaIds(const QByteArray &windowsId)
{
    const quint16 windowsIdKey = toWindowsIdKey(windowsId);
    QList<QByteArray> list;

    for (int i = 0; i < zoneDataTableSize; ++i) {
        const QZoneData *data = zoneData(i);
        if (data->windowsIdKey == windowsIdKey)
            list << ianaId(data).split(' ');
    }

    // Return the full list in alpha order
    std::sort(list.begin(), list.end());
    return list;
}

QList<QByteArray> QTimeZonePrivate::windowsIdToIanaIds(const QByteArray &windowsId,
                                                        QLocale::Country country)
{
    const quint16 windowsIdKey = toWindowsIdKey(windowsId);
    for (int i = 0; i < zoneDataTableSize; ++i) {
        const QZoneData *data = zoneData(i);
        // Return the region matches in preference order
        if (data->windowsIdKey == windowsIdKey && data->country == (quint16) country)
            return ianaId(data).split(' ');
    }

    return QList<QByteArray>();
}

// Define template for derived classes to reimplement so QSharedDataPointer clone() works correctly
template<> QTimeZonePrivate *QSharedDataPointer<QTimeZonePrivate>::clone()
{
    return d->clone();
}

/*
    UTC Offset implementation, used when QT_NO_SYSTEMLOCALE set and ICU is not being used,
    or for QDateTimes with a Qt:Spec of Qt::OffsetFromUtc.
*/

// Create default UTC time zone
QUtcTimeZonePrivate::QUtcTimeZonePrivate()
{
    const QString name = utcQString();
    init(utcQByteArray(), 0, name, name, QLocale::AnyCountry, name);
}

// Create a named UTC time zone
QUtcTimeZonePrivate::QUtcTimeZonePrivate(const QByteArray &id)
{
    // Look for the name in the UTC list, if found set the values
    for (int i = 0; i < utcDataTableSize; ++i) {
        const QUtcData *data = utcData(i);
        const QByteArray uid = utcId(data);
        if (uid == id) {
            QString name = QString::fromUtf8(id);
            init(id, data->offsetFromUtc, name, name, QLocale::AnyCountry, name);
            break;
        }
    }
}

qint64 QUtcTimeZonePrivate::offsetFromUtcString(const QByteArray &id)
{
    // Convert reasonable UTC[+-]\d+(:\d+){,2} to offset in seconds.
    // Assumption: id has already been tried as a CLDR UTC offset ID (notably
    // including plain "UTC" itself) and a system offset ID; it's neither.
    if (!id.startsWith("UTC") || id.size() < 5)
        return invalidSeconds(); // Doesn't match
    const char signChar = id.at(3);
    if (signChar != '-' && signChar != '+')
        return invalidSeconds(); // No sign
    const int sign = signChar == '-' ? -1 : 1;

    const auto offsets = id.mid(4).split(':');
    if (offsets.isEmpty() || offsets.size() > 3)
        return invalidSeconds(); // No numbers, or too many.

    qint32 seconds = 0;
    int prior = 0; // Number of fields parsed thus far
    for (const auto &offset : offsets) {
        bool ok = false;
        unsigned short field = offset.toUShort(&ok);
        // Bound hour above at 24, minutes and seconds at 60:
        if (!ok || field >= (prior ? 60 : 24))
            return invalidSeconds();
        seconds = seconds * 60 + field;
        ++prior;
    }
    while (prior++ < 3)
        seconds *= 60;

    return seconds * sign;
}

// Create offset from UTC
QUtcTimeZonePrivate::QUtcTimeZonePrivate(qint32 offsetSeconds)
{
    QString utcId;

    if (offsetSeconds == 0)
        utcId = utcQString();
    else
        utcId = isoOffsetFormat(offsetSeconds);

    init(utcId.toUtf8(), offsetSeconds, utcId, utcId, QLocale::AnyCountry, utcId);
}

QUtcTimeZonePrivate::QUtcTimeZonePrivate(const QByteArray &zoneId, int offsetSeconds,
                                         const QString &name, const QString &abbreviation,
                                         QLocale::Country country, const QString &comment)
{
    init(zoneId, offsetSeconds, name, abbreviation, country, comment);
}

QUtcTimeZonePrivate::QUtcTimeZonePrivate(const QUtcTimeZonePrivate &other)
    : QTimeZonePrivate(other), m_name(other.m_name),
      m_abbreviation(other.m_abbreviation),
      m_comment(other.m_comment),
      m_country(other.m_country),
      m_offsetFromUtc(other.m_offsetFromUtc)
{
}

QUtcTimeZonePrivate::~QUtcTimeZonePrivate()
{
}

QUtcTimeZonePrivate *QUtcTimeZonePrivate::clone() const
{
    return new QUtcTimeZonePrivate(*this);
}

QTimeZonePrivate::Data QUtcTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    Data d;
    d.abbreviation = m_abbreviation;
    d.atMSecsSinceEpoch = forMSecsSinceEpoch;
    d.standardTimeOffset = d.offsetFromUtc = m_offsetFromUtc;
    d.daylightTimeOffset = 0;
    return d;
}

void QUtcTimeZonePrivate::init(const QByteArray &zoneId)
{
    m_id = zoneId;
}

void QUtcTimeZonePrivate::init(const QByteArray &zoneId, int offsetSeconds, const QString &name,
                               const QString &abbreviation, QLocale::Country country,
                               const QString &comment)
{
    m_id = zoneId;
    m_offsetFromUtc = offsetSeconds;
    m_name = name;
    m_abbreviation = abbreviation;
    m_country = country;
    m_comment = comment;
}

QLocale::Country QUtcTimeZonePrivate::country() const
{
    return m_country;
}

QString QUtcTimeZonePrivate::comment() const
{
    return m_comment;
}

QString QUtcTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                         QTimeZone::NameType nameType,
                                         const QLocale &locale) const
{
    Q_UNUSED(timeType)
    Q_UNUSED(locale)
    if (nameType == QTimeZone::ShortName)
        return m_abbreviation;
    else if (nameType == QTimeZone::OffsetName)
        return isoOffsetFormat(m_offsetFromUtc);
    return m_name;
}

QString QUtcTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return m_abbreviation;
}

qint32 QUtcTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return m_offsetFromUtc;
}

qint32 QUtcTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return 0;
}

QByteArray QUtcTimeZonePrivate::systemTimeZoneId() const
{
    return utcQByteArray();
}

bool QUtcTimeZonePrivate::isTimeZoneIdAvailable(const QByteArray &ianaId) const
{
    // Only the zone IDs supplied by CLDR and recognized by constructor.
    for (int i = 0; i < utcDataTableSize; ++i) {
        const QUtcData *data = utcData(i);
        if (utcId(data) == ianaId)
            return true;
    }
    // But see offsetFromUtcString(), which lets us accept some "unavailable" IDs.
    return false;
}

QList<QByteArray> QUtcTimeZonePrivate::availableTimeZoneIds() const
{
    // Only the zone IDs supplied by CLDR and recognized by constructor.
    QList<QByteArray> result;
    result.reserve(utcDataTableSize);
    for (int i = 0; i < utcDataTableSize; ++i)
        result << utcId(utcData(i));
    // Not guaranteed to be sorted, so sort:
    std::sort(result.begin(), result.end());
    // ### assuming no duplicates
    return result;
}

QList<QByteArray> QUtcTimeZonePrivate::availableTimeZoneIds(QLocale::Country country) const
{
    // If AnyCountry then is request for all non-region offset codes
    if (country == QLocale::AnyCountry)
        return availableTimeZoneIds();
    return QList<QByteArray>();
}

QList<QByteArray> QUtcTimeZonePrivate::availableTimeZoneIds(qint32 offsetSeconds) const
{
    // Only if it's present in CLDR. (May get more than one ID: UTC, UTC+00:00
    // and UTC-00:00 all have the same offset.)
    QList<QByteArray> result;
    for (int i = 0; i < utcDataTableSize; ++i) {
        const QUtcData *data = utcData(i);
        if (data->offsetFromUtc == offsetSeconds)
            result << utcId(data);
    }
    // Not guaranteed to be sorted, so sort:
    std::sort(result.begin(), result.end());
    // ### assuming no duplicates
    return result;
}

#ifndef QT_NO_DATASTREAM
void QUtcTimeZonePrivate::serialize(QDataStream &ds) const
{
    ds << QStringLiteral("OffsetFromUtc") << QString::fromUtf8(m_id) << m_offsetFromUtc << m_name
       << m_abbreviation << (qint32) m_country << m_comment;
}
#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE
