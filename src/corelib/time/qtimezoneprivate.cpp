// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2013 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qtimezone.h"
#include "qtimezoneprivate_p.h"
#if QT_CONFIG(timezone_locale)
#  include "qtimezonelocale_p.h"
#endif
#include "qtimezoneprivate_data_p.h"

#include <qdatastream.h>
#include <qdebug.h>
#include <qstring.h>

#include <private/qcalendarmath_p.h>
#include <private/qnumeric_p.h>
#include <private/qtools_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace QtMiscUtils;
using namespace QtTimeZoneCldr;
using namespace Qt::StringLiterals;

// For use with std::is_sorted() in assertions:
[[maybe_unused]]
constexpr bool earlierZoneData(const ZoneData &less, const ZoneData &more) noexcept
{
    return less.windowsIdKey < more.windowsIdKey
        || (less.windowsIdKey == more.windowsIdKey && less.territory < more.territory);
}

[[maybe_unused]]
static bool earlierWinData(const WindowsData &less, const WindowsData &more) noexcept
{
    // Actually only tested in the negative, to check more < less never happens,
    // so should be true if more < less in either part; hence || not && combines.
    return less.windowsIdKey < more.windowsIdKey
        || less.windowsId().compare(more.windowsId(), Qt::CaseInsensitive) < 0;
}

// For use with std::lower_bound():
constexpr bool atLowerUtcOffset(const UtcData &entry, qint32 offsetSeconds) noexcept
{
    return entry.offsetFromUtc < offsetSeconds;
}

constexpr bool atLowerWindowsKey(const WindowsData &entry, qint16 winIdKey) noexcept
{
    return entry.windowsIdKey < winIdKey;
}

static bool earlierAliasId(const AliasData &entry, QByteArrayView aliasId) noexcept
{
    return entry.aliasId().compare(aliasId, Qt::CaseInsensitive) < 0;
}

static bool earlierWindowsId(const WindowsData &entry, QByteArrayView winId) noexcept
{
    return entry.windowsId().compare(winId, Qt::CaseInsensitive) < 0;
}

constexpr bool zoneAtLowerWindowsKey(const ZoneData &entry, qint16 winIdKey) noexcept
{
    return entry.windowsIdKey < winIdKey;
}

// Static table-lookup helpers
static quint16 toWindowsIdKey(const QByteArray &winId)
{
    // Key and winId are monotonic, table is sorted on them.
    const auto data = std::lower_bound(std::begin(windowsDataTable), std::end(windowsDataTable),
                                       winId, earlierWindowsId);
    if (data != std::end(windowsDataTable) && data->windowsId() == winId)
        return data->windowsIdKey;
    return 0;
}

static QByteArray toWindowsIdLiteral(quint16 windowsIdKey)
{
    // Caller should be passing a valid (in range) key; and table is sorted in
    // increasing order, with no gaps in numbering, starting with key = 1 at
    // index [0]. So this should normally work:
    if (Q_LIKELY(windowsIdKey > 0 && windowsIdKey <= std::size(windowsDataTable))) {
        const auto &data = windowsDataTable[windowsIdKey - 1];
        if (Q_LIKELY(data.windowsIdKey == windowsIdKey))
            return data.windowsId().toByteArray();
    }
    // Fall back on binary chop - key and winId are monotonic, table is sorted on them:
    const auto data = std::lower_bound(std::begin(windowsDataTable), std::end(windowsDataTable),
                                       windowsIdKey, atLowerWindowsKey);
    if (data != std::end(windowsDataTable) && data->windowsIdKey == windowsIdKey)
        return data->windowsId().toByteArray();

    return QByteArray();
}

static auto zoneStartForWindowsId(quint16 windowsIdKey) noexcept
{
    // Caller must check the resulting iterator isn't std::end(zoneDataTable)
    // and does match windowsIdKey, since this is just the lower bound.
    return std::lower_bound(std::begin(zoneDataTable), std::end(zoneDataTable),
                            windowsIdKey, zoneAtLowerWindowsKey);
}

/*
    Base class implementing common utility routines, only instantiate for a null tz.
*/

QTimeZonePrivate::QTimeZonePrivate()
{
    // If std::is_sorted() were constexpr, the first could be a static_assert().
    // From C++20, we should be able to rework it in terms of std::all_of().
    Q_ASSERT(std::is_sorted(std::begin(zoneDataTable), std::end(zoneDataTable),
                            earlierZoneData));
    Q_ASSERT(std::is_sorted(std::begin(windowsDataTable), std::end(windowsDataTable),
                            earlierWinData));
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

QLocale::Territory QTimeZonePrivate::territory() const
{
    // Default fall-back mode, use the zoneTable to find Region of known Zones
    const QLatin1StringView sought(m_id.data(), m_id.size());
    for (const ZoneData &data : zoneDataTable) {
        for (QLatin1StringView token : data.ids()) {
            if (token == sought)
                return QLocale::Territory(data.territory);
        }
    }
    return QLocale::AnyTerritory;
}

QString QTimeZonePrivate::comment() const
{
    return QString();
}

QString QTimeZonePrivate::displayName(qint64 atMSecsSinceEpoch,
                                      QTimeZone::NameType nameType,
                                      const QLocale &locale) const
{
    const Data tran = data(atMSecsSinceEpoch);
    if (tran.atMSecsSinceEpoch != invalidMSecs()) {
        if (nameType == QTimeZone::OffsetName && locale.language() == QLocale::C)
            return isoOffsetFormat(tran.offsetFromUtc);
        if (nameType == QTimeZone::ShortName && isDataLocale(locale))
            return tran.abbreviation;

        QTimeZone::TimeType timeType
            = tran.daylightTimeOffset != 0 ? QTimeZone::DaylightTime : QTimeZone::StandardTime;
#if QT_CONFIG(timezone_locale)
        return localeName(atMSecsSinceEpoch, tran.offsetFromUtc, timeType, nameType, locale);
#else
        return displayName(timeType, nameType, locale);
#endif
    }
    return QString();
}

QString QTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                      QTimeZone::NameType nameType,
                                      const QLocale &locale) const
{
    const Data tran = data(timeType);
    if (tran.atMSecsSinceEpoch != invalidMSecs()) {
        if (nameType == QTimeZone::OffsetName && isDataLocale(locale))
            return isoOffsetFormat(tran.offsetFromUtc);

#if QT_CONFIG(timezone_locale)
        return localeName(tran.atMSecsSinceEpoch, tran.offsetFromUtc, timeType, nameType, locale);
#endif
    }
    return QString();
}

QString QTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    return displayName(atMSecsSinceEpoch, QTimeZone::ShortName, QLocale::c());
}

int QTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    const int std = standardTimeOffset(atMSecsSinceEpoch);
    const int dst = daylightTimeOffset(atMSecsSinceEpoch);
    const int bad = invalidSeconds();
    return std == bad || dst == bad ? bad : std + dst;
}

int QTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch);
    return invalidSeconds();
}

int QTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch);
    return invalidSeconds();
}

bool QTimeZonePrivate::hasDaylightTime() const
{
    return false;
}

bool QTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch);
    return false;
}

QTimeZonePrivate::Data QTimeZonePrivate::data(QTimeZone::TimeType timeType) const
{
    // True if tran is valid and has the DST-ness to match timeType:
    const auto validMatch = [timeType](const QTimeZonePrivate::Data &tran) {
        return tran.atMSecsSinceEpoch != invalidMSecs()
            && ((timeType == QTimeZone::DaylightTime) != (tran.daylightTimeOffset == 0));
    };

    // Get current tran, use if suitable:
    const qint64 currentMSecs = QDateTime::currentMSecsSinceEpoch();
    QTimeZonePrivate::Data tran = data(currentMSecs);
    if (validMatch(tran))
        return tran;

    if (hasTransitions()) {
        // Otherwise, next tran probably flips DST-ness:
        tran = nextTransition(currentMSecs);
        if (validMatch(tran))
            return tran;

        // Failing that, prev (or present, if current MSecs is exactly a
        // transition moment) tran defines what data() got us and the one before
        // that probably flips DST-ness; failing that, keep marching backwards
        // in search of a DST interval:
        tran = previousTransition(currentMSecs + 1);
        while (tran.atMSecsSinceEpoch != invalidMSecs()) {
            tran = previousTransition(tran.atMSecsSinceEpoch);
            if (validMatch(tran))
                return tran;
        }
    }
    return {};
}

/*!
    \internal

    Returns true if the abbreviation given in data()'s returns is appropriate
    for use in the given \a locale.

    Base implementation assumes data() corresponds to the system locale; derived
    classes should override if their data() is something else (such as
    C/English).
*/
bool QTimeZonePrivate::isDataLocale(const QLocale &locale) const
{
    // Guess data is for the system locale unless backend overrides that.
    return locale == QLocale::system();
}

QTimeZonePrivate::Data QTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    Q_UNUSED(forMSecsSinceEpoch);
    return {};
}

// Private only method for use by QDateTime to convert local msecs to epoch msecs
QDateTimePrivate::ZoneState QTimeZonePrivate::stateAtZoneTime(
    qint64 forLocalMSecs, QDateTimePrivate::TransitionOptions resolve) const
{
    auto dataToState = [](const QTimeZonePrivate::Data &d) {
        return QDateTimePrivate::ZoneState(d.atMSecsSinceEpoch + d.offsetFromUtc * 1000,
                                           d.offsetFromUtc,
                                           d.daylightTimeOffset ? QDateTimePrivate::DaylightTime
                                                                : QDateTimePrivate::StandardTime);
    };

    /*
      We need a UTC time at which to ask for the offset, in order to be able to
      add that offset to forLocalMSecs, to get the UTC time we need.
      Fortunately, all time-zone offsets have been less than 17 hours; and DST
      transitions happen (much) more than thirty-four hours apart. So sampling
      offset seventeen hours each side gives us information we can be sure
      brackets the correct time and at most one DST transition.
    */
    std::integral_constant<qint64, 17 * 3600 * 1000> seventeenHoursInMSecs;
    static_assert(-seventeenHoursInMSecs / 1000 < QTimeZone::MinUtcOffsetSecs
                  && seventeenHoursInMSecs / 1000 > QTimeZone::MaxUtcOffsetSecs);
    qint64 millis;
    // Clip the bracketing times to the bounds of the supported range.
    const qint64 recent =
        qSubOverflow(forLocalMSecs, seventeenHoursInMSecs, &millis) || millis < minMSecs()
        ? minMSecs() : millis; // Necessarily <= forLocalMSecs + 1.
    // (Given that minMSecs() is std::numeric_limits<qint64>::min() + 1.)
    const qint64 imminent =
        qAddOverflow(forLocalMSecs, seventeenHoursInMSecs, &millis)
        ? maxMSecs() : millis; // Necessarily >= forLocalMSecs
    // At most one of those was clipped to its boundary value:
    Q_ASSERT(recent < imminent && seventeenHoursInMSecs < imminent - recent + 1);

    const Data past = data(recent), future = data(imminent);
    // > 99% of the time, past and future will agree:
    if (Q_LIKELY(past.offsetFromUtc == future.offsetFromUtc
                 && past.standardTimeOffset == future.standardTimeOffset
                 // Those two imply same daylightTimeOffset.
                 && past.abbreviation == future.abbreviation)) {
        Data data = future;
        data.atMSecsSinceEpoch = forLocalMSecs - future.offsetFromUtc * 1000;
        return dataToState(data);
    }

    /*
      Offsets are Local - UTC, positive to the east of Greenwich, negative to
      the west; DST offset normally exceeds standard offset, when DST applies.
      When we have offsets on either side of a transition, the lower one is
      standard, the higher is DST, unless we have data telling us it's the other
      way round.

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
      reasoning that applies to DST applies just the same.

      The resolution of transitions, specified by \a resolve, may be lead astray
      if (as happens on Windows) the backend has been obliged to guess whether a
      transition is in fact a DST one or a change to standard offset; or to
      guess that the higher-offset side is the DST one (the reverse of this is
      true for Ireland, using negative DST). There's not much we can do about
      that, though.
    */
    if (hasTransitions()) {
        /*
          We have transitions.

          Each transition gives the offsets to use until the next; so we need
          the most recent transition before the time forLocalMSecs describes. If
          it describes a time *in* a transition, we'll need both that transition
          and the one before it. So find one transition that's probably after
          (and not much before, otherwise) and another that's definitely before,
          then work out which one to use. When both or neither work on
          forLocalMSecs, use resolve to disambiguate.
        */

        // Get a transition definitely before the local MSecs; usually all we need.
        // Only around the transition times might we need another.
        Data tran = past; // Data after last transition before our window.
        Q_ASSERT(forLocalMSecs < 0 || // Pre-epoch TZ info may be unavailable
                 forLocalMSecs - tran.offsetFromUtc * 1000 >= tran.atMSecsSinceEpoch);
        // If offset actually exceeds 17 hours, that assert may trigger.
        Data nextTran = nextTransition(tran.atMSecsSinceEpoch);
        /*
          Now walk those forward until they bracket forLocalMSecs with transitions.

          One of the transitions should then be telling us the right offset to use.
          In a transition, we need the transition before it (to describe the run-up
          to the transition) and the transition itself; so we need to stop when
          nextTran is (invalid or) that transition.
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
        const qint64 nextStart = nextTran.atMSecsSinceEpoch;

        // Check we do *really* have transitions for this zone:
        if (tran.atMSecsSinceEpoch != invalidMSecs()) {
            /* So now tran is definitely before ... */
            Q_ASSERT(forLocalMSecs < 0
                     || forLocalMSecs - tran.offsetFromUtc * 1000 > tran.atMSecsSinceEpoch);
            // Work out the UTC value it would make sense to return if using tran:
            tran.atMSecsSinceEpoch = forLocalMSecs - tran.offsetFromUtc * 1000;

            // If there are no transition after it, the answer is easy - or
            // should be - but Darwin's handling of the distant future (in macOS
            // 15, QTBUG-126391) runs out of transitions in 506'712 CE, despite
            // knowing about offset changes long after that. So only trust the
            // easy answer if offsets match; otherwise, fall through to the
            // transitions-unknown code.
            if (nextStart == invalidMSecs() && tran.offsetFromUtc == future.offsetFromUtc)
                return dataToState(tran); // Last valid transition.
        }

        if (tran.atMSecsSinceEpoch != invalidMSecs() && nextStart != invalidMSecs()) {
            /*
              ... and nextTran is either after or only slightly before. We're
              going to interpret one as standard time, the other as DST
              (although the transition might in fact be a change in standard
              offset, or a change in DST offset, e.g. to/from double-DST).

              Usually exactly one of those shall be relevant and we'll use it;
              but if we're close to nextTran we may be in a transition, to be
              settled according to resolve's rules.
            */
            // Work out the UTC value it would make sense to return if using nextTran:
            nextTran.atMSecsSinceEpoch = forLocalMSecs - nextTran.offsetFromUtc * 1000;

            bool fallBack = false;
            if (nextStart > nextTran.atMSecsSinceEpoch) {
                // If both UTC values are before nextTran's offset applies, use tran:
                if (nextStart > tran.atMSecsSinceEpoch)
                    return dataToState(tran);

                Q_ASSERT(tran.offsetFromUtc < nextTran.offsetFromUtc);
                // We're in a spring-forward.
            } else if (nextStart <= tran.atMSecsSinceEpoch) {
                // Both UTC values say we should be using nextTran:
                return dataToState(nextTran);
            } else {
                Q_ASSERT(nextTran.offsetFromUtc < tran.offsetFromUtc);
                fallBack = true; // We're in a fall-back.
            }
            // (forLocalMSecs - nextStart) / 1000 lies between the two offsets.

            // Apply resolve:
            // Determine whether FlipForReverseDst affects the outcome:
            const bool flipped
                = resolve.testFlag(QDateTimePrivate::FlipForReverseDst)
                && (fallBack ? !tran.daylightTimeOffset && nextTran.daylightTimeOffset
                             : tran.daylightTimeOffset && !nextTran.daylightTimeOffset);

            if (fallBack) {
                if (resolve.testFlag(flipped
                                     ? QDateTimePrivate::FoldUseBefore
                                     : QDateTimePrivate::FoldUseAfter)) {
                    return dataToState(nextTran);
                }
                if (resolve.testFlag(flipped
                                     ? QDateTimePrivate::FoldUseAfter
                                     : QDateTimePrivate::FoldUseBefore)) {
                    return dataToState(tran);
                }
            } else {
                /* Neither is valid (e.g. in a spring-forward's gap) and
                   nextTran.atMSecsSinceEpoch < nextStart <= tran.atMSecsSinceEpoch.
                   So swap their atMSecsSinceEpoch to give each a moment on the
                   side of the transition that it describes, then select the one
                   after or before according to the option set:
                */
                std::swap(tran.atMSecsSinceEpoch, nextTran.atMSecsSinceEpoch);
                if (resolve.testFlag(flipped
                                     ? QDateTimePrivate::GapUseBefore
                                     : QDateTimePrivate::GapUseAfter))
                    return dataToState(nextTran);
                if (resolve.testFlag(flipped
                                     ? QDateTimePrivate::GapUseAfter
                                     : QDateTimePrivate::GapUseBefore))
                    return dataToState(tran);
            }
            // Reject
            return {forLocalMSecs};
        }
        // Before first transition, or system has transitions but not for this zone.
        // Try falling back to offsetFromUtc (works for before first transition, at least).
    }

    /* Bracket and refine to discover offset. */
    qint64 utcEpochMSecs;

    // We don't have true data on DST-ness, so can't apply FlipForReverseDst.
    int early = past.offsetFromUtc;
    int late = future.offsetFromUtc;
    if (early == late || late == invalidSeconds()) {
        if (early == invalidSeconds()
            || qSubOverflow(forLocalMSecs, early * qint64(1000), &utcEpochMSecs)) {
            return {forLocalMSecs}; // Outside representable range
        }
    } else {
        // Candidate values for utcEpochMSecs (if forLocalMSecs is valid):
        const qint64 forEarly = forLocalMSecs - early * 1000;
        const qint64 forLate = forLocalMSecs - late * 1000;
        // If either of those doesn't have the offset we got it from, it's on
        // the wrong side of the transition (and both may be, for a gap):
        const bool earlyOk = offsetFromUtc(forEarly) == early;
        const bool lateOk = offsetFromUtc(forLate) == late;

        if (earlyOk) {
            if (lateOk) {
                Q_ASSERT(early > late);
                // fall-back's repeated interval
                if (resolve.testFlag(QDateTimePrivate::FoldUseBefore))
                    utcEpochMSecs = forEarly;
                else if (resolve.testFlag(QDateTimePrivate::FoldUseAfter))
                    utcEpochMSecs = forLate;
                else
                    return {forLocalMSecs};
            } else {
                // Before and clear of the transition:
                utcEpochMSecs = forEarly;
            }
        } else if (lateOk) {
            // After and clear of the transition:
            utcEpochMSecs = forLate;
        } else {
            // forLate <= gap < forEarly
            Q_ASSERT(late > early);
            const int dstStep = (late - early) * 1000;
            if (resolve.testFlag(QDateTimePrivate::GapUseBefore))
                utcEpochMSecs = forEarly - dstStep;
            else if (resolve.testFlag(QDateTimePrivate::GapUseAfter))
                utcEpochMSecs = forLate + dstStep;
            else
                return {forLocalMSecs};
        }
    }

    return dataToState(data(utcEpochMSecs));
}

bool QTimeZonePrivate::hasTransitions() const
{
    return false;
}

QTimeZonePrivate::Data QTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    Q_UNUSED(afterMSecsSinceEpoch);
    return {};
}

QTimeZonePrivate::Data QTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    Q_UNUSED(beforeMSecsSinceEpoch);
    return {};
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
    // Fall-back implementation, can be made faster in subclasses.
    // Backends that don't cache the available list SHOULD override this.
    const QList<QByteArray> tzIds = availableTimeZoneIds();
    return std::binary_search(tzIds.begin(), tzIds.end(), ianaId);
}

QList<QByteArray> QTimeZonePrivate::availableTimeZoneIds() const
{
    return QList<QByteArray>();
}

static QList<QByteArray> selectAvailable(QList<QByteArrayView> &&desired,
                                         const QList<QByteArray> &all)
{
    std::sort(desired.begin(), desired.end());
    const auto newEnd = std::unique(desired.begin(), desired.end());
    const auto newSize = std::distance(desired.begin(), newEnd);
    QList<QByteArray> result;
    result.reserve(qMin(all.size(), newSize));
    std::set_intersection(all.begin(), all.end(), desired.cbegin(),
                          std::next(desired.cbegin(), newSize), std::back_inserter(result));
    return result;
}

QList<QByteArrayView> QTimeZonePrivate::matchingTimeZoneIds(QLocale::Territory territory) const
{
    // Default fall-back mode, use the zoneTable to find Region of know Zones
    QList<QByteArrayView> regions;
    // Get all Zones in the table associated with this territory:
    if (territory == QLocale::World) {
        // World names are filtered out of zoneDataTable to provide the defaults
        // in windowsDataTable.
        for (const WindowsData &data : windowsDataTable)
            regions << data.ianaId();
    } else {
        for (const ZoneData &data : zoneDataTable) {
            if (data.territory == territory) {
                for (auto l1 : data.ids())
                    regions << QByteArrayView(l1.data(), l1.size());
            }
        }
    }
    return regions;
}

QList<QByteArray> QTimeZonePrivate::availableTimeZoneIds(QLocale::Territory territory) const
{
    return selectAvailable(matchingTimeZoneIds(territory), availableTimeZoneIds());
}

QList<QByteArrayView> QTimeZonePrivate::matchingTimeZoneIds(int offsetFromUtc) const
{
    // Default fall-back mode: use the zoneTable to find offsets of know zones.
    QList<QByteArrayView> offsets;
    // First get all Zones in the table using the given offset:
    for (const WindowsData &winData : windowsDataTable) {
        if (winData.offsetFromUtc == offsetFromUtc) {
            for (auto data = zoneStartForWindowsId(winData.windowsIdKey);
                 data != std::end(zoneDataTable) && data->windowsIdKey == winData.windowsIdKey;
                 ++data) {
                for (auto l1 : data->ids())
                    offsets << QByteArrayView(l1.data(), l1.size());
            }
        }
    }
    return offsets;
}

QList<QByteArray> QTimeZonePrivate::availableTimeZoneIds(int offsetFromUtc) const
{
    return selectAvailable(matchingTimeZoneIds(offsetFromUtc), availableTimeZoneIds());
}

#ifndef QT_NO_DATASTREAM
void QTimeZonePrivate::serialize(QDataStream &ds) const
{
    ds << QString::fromUtf8(m_id);
}
#endif // QT_NO_DATASTREAM

// Static Utility Methods

QTimeZone::OffsetData QTimeZonePrivate::invalidOffsetData()
{
    return { QString(), QDateTime(),
             invalidSeconds(), invalidSeconds(), invalidSeconds() };
}

QTimeZone::OffsetData QTimeZonePrivate::toOffsetData(const QTimeZonePrivate::Data &data)
{
    if (data.atMSecsSinceEpoch == invalidMSecs())
        return invalidOffsetData();

    return {
        data.abbreviation,
        QDateTime::fromMSecsSinceEpoch(data.atMSecsSinceEpoch, QTimeZone::UTC),
        data.offsetFromUtc, data.standardTimeOffset, data.daylightTimeOffset };
}

// Is the format of the ID valid ?
bool QTimeZonePrivate::isValidId(const QByteArray &ianaId)
{
    /*
      Main rules for defining TZ/IANA names, as per
      https://www.iana.org/time-zones/repository/theory.html, are:
       1. Use only valid POSIX file name components
       2. Within a file name component, use only ASCII letters, `.', `-' and `_'.
       3. Do not use digits (except in a [+-]\d+ suffix, when used).
       4. A file name component must not exceed 14 characters or start with `-'

      However, the rules are really guidelines - a later one says
       - Do not change established names if they only marginally violate the
         above rules.
      We may, therefore, need to be a bit slack in our check here, if we hit
      legitimate exceptions in real time-zone databases. In particular, ICU
      includes some non-standard names with some components > 14 characters
      long; so does Android, possibly deriving them from ICU.

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
#if defined(Q_OS_ANDROID) || QT_CONFIG(icu)
    // Android has its own naming of zones. It may well come from ICU.
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
        } else if (!isAsciiLower(ch)
                && !isAsciiUpper(ch)
                && !(ch == '_')
                && !(ch == '.')
                   // Should ideally check these only happen as an offset:
                && !isAsciiDigit(ch)
                && !(ch == '+')
                && !(ch == ':')) {
            return false; // violates (2)
        }
    }
    if (sectionLength < MinSectionLength || sectionLength > MaxSectionLength)
        return false; // violates (4)
    return true;
}

QString QTimeZonePrivate::isoOffsetFormat(int offsetFromUtc, QTimeZone::NameType mode)
{
    if (mode == QTimeZone::ShortName && !offsetFromUtc)
        return utcQString();

    char sign = '+';
    if (offsetFromUtc < 0) {
        sign = '-';
        offsetFromUtc = -offsetFromUtc;
    }
    const int secs = offsetFromUtc % 60;
    const int mins = (offsetFromUtc / 60) % 60;
    const int hour = offsetFromUtc / 3600;
    QString result = QString::asprintf("UTC%c%02d", sign, hour);
    if (mode != QTimeZone::ShortName || secs || mins)
        result += QString::asprintf(":%02d", mins);
    if (mode == QTimeZone::LongName || secs)
        result += QString::asprintf(":%02d", secs);
    return result;
}

QByteArray QTimeZonePrivate::aliasToIana(QByteArrayView alias)
{
    const auto data = std::lower_bound(std::begin(aliasMappingTable), std::end(aliasMappingTable),
                                       alias, earlierAliasId);
    if (data != std::end(aliasMappingTable) && data->aliasId() == alias)
        return data->ianaId().toByteArray();
    // Note: empty return means not an alias, which is true of an ID that others
    // are aliases to, as the table omits self-alias entries. Let caller sort
    // that out, rather than allocating to return alias.toByteArray().
    return {};
}

QByteArray QTimeZonePrivate::ianaIdToWindowsId(const QByteArray &id)
{
    const auto idUtf8 = QUtf8StringView(id);

    for (const ZoneData &data : zoneDataTable) {
        for (auto l1 : data.ids()) {
            if (l1 == idUtf8)
                return toWindowsIdLiteral(data.windowsIdKey);
        }
    }
    // If the IANA ID is the default for any Windows ID, it has already shown up
    // as an ID for it in some territory; no need to search windowsDataTable[].
    return QByteArray();
}

QByteArray QTimeZonePrivate::windowsIdToDefaultIanaId(const QByteArray &windowsId)
{
    const auto data = std::lower_bound(std::begin(windowsDataTable), std::end(windowsDataTable),
                                       windowsId, earlierWindowsId);
    if (data != std::end(windowsDataTable) && data->windowsId() == windowsId) {
        QByteArrayView id = data->ianaId();
        Q_ASSERT(id.indexOf(' ') == -1);
        return id.toByteArray();
    }
    return QByteArray();
}

QByteArray QTimeZonePrivate::windowsIdToDefaultIanaId(const QByteArray &windowsId,
                                                      QLocale::Territory territory)
{
    const QList<QByteArray> list = windowsIdToIanaIds(windowsId, territory);
    return list.size() > 0 ? list.first() : QByteArray();
}

QList<QByteArray> QTimeZonePrivate::windowsIdToIanaIds(const QByteArray &windowsId)
{
    const quint16 windowsIdKey = toWindowsIdKey(windowsId);
    QList<QByteArray> list;

    for (auto data = zoneStartForWindowsId(windowsIdKey);
         data != std::end(zoneDataTable) && data->windowsIdKey == windowsIdKey;
         ++data) {
        for (auto l1 : data->ids())
            list << QByteArray(l1.data(), l1.size());
    }
    // The default, windowsIdToDefaultIanaId(windowsId), is always an entry for
    // at least one territory: cldr.py asserts this, in readWindowsTimeZones().
    // So we don't need to add it here.

    // Return the full list in alpha order
    std::sort(list.begin(), list.end());
    return list;
}

QList<QByteArray> QTimeZonePrivate::windowsIdToIanaIds(const QByteArray &windowsId,
                                                       QLocale::Territory territory)
{
    QList<QByteArray> list;
    if (territory == QLocale::World) {
        // World data are in windowsDataTable, not zoneDataTable.
        list << windowsIdToDefaultIanaId(windowsId);
    } else {
        const quint16 windowsIdKey = toWindowsIdKey(windowsId);
        const qint16 land = static_cast<quint16>(territory);
        for (auto data = zoneStartForWindowsId(windowsIdKey);
             data != std::end(zoneDataTable) && data->windowsIdKey == windowsIdKey;
             ++data) {
            // Return the region matches in preference order
            if (data->territory == land) {
                for (auto l1 : data->ids())
                    list << QByteArray(l1.data(), l1.size());
                break;
            }
        }
    }

    return list;
}

// Define template for derived classes to reimplement so QSharedDataPointer clone() works correctly
template<> QTimeZonePrivate *QSharedDataPointer<QTimeZonePrivate>::clone()
{
    return d->clone();
}

static bool isEntryInIanaList(QByteArrayView id, QByteArrayView ianaIds)
{
    qsizetype cut;
    while ((cut = ianaIds.indexOf(' ')) >= 0) {
        if (id == ianaIds.first(cut))
            return true;
        ianaIds = ianaIds.sliced(cut + 1);
    }
    return id == ianaIds;
}

/*
    UTC Offset backend.

    Always present, based on UTC-offset zones.
    Complements platform-specific backends.
    Equivalent to Qt::OffsetFromUtc lightweight time representations.
*/

// Create default UTC time zone
QUtcTimeZonePrivate::QUtcTimeZonePrivate()
{
    const QString name = utcQString();
    init(utcQByteArray(), 0, name, name, QLocale::AnyTerritory, name);
}

// Create a named UTC time zone
QUtcTimeZonePrivate::QUtcTimeZonePrivate(const QByteArray &id)
{
    // Look for the name in the UTC list, if found set the values
    for (const UtcData &data : utcDataTable) {
        if (isEntryInIanaList(id, data.id())) {
            QString name = QString::fromUtf8(id);
            init(id, data.offsetFromUtc, name, name, QLocale::AnyTerritory, name);
            break;
        }
    }
}

qint64 QUtcTimeZonePrivate::offsetFromUtcString(QByteArrayView id)
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

    qint32 seconds = 0;
    int prior = 0; // Number of fields parsed thus far
    for (auto offset : QLatin1StringView(id.mid(4)).tokenize(':'_L1)) {
        bool ok = false;
        unsigned short field = offset.toUShort(&ok);
        // Bound hour above at 24, minutes and seconds at 60:
        if (!ok || field >= (prior ? 60 : 24))
            return invalidSeconds();
        seconds = seconds * 60 + field;
        if (++prior > 3)
            return invalidSeconds(); // Too many numbers
    }

    if (!prior)
        return invalidSeconds(); // No numbers

    while (prior++ < 3)
        seconds *= 60;

    return seconds * sign;
}

// Create from UTC offset:
QUtcTimeZonePrivate::QUtcTimeZonePrivate(qint32 offsetSeconds)
{
    QString name;
    QByteArray id;
    // If there's an IANA ID for this offset, use it:
    const auto data = std::lower_bound(std::begin(utcDataTable), std::end(utcDataTable),
                                       offsetSeconds, atLowerUtcOffset);
    if (data != std::end(utcDataTable) && data->offsetFromUtc == offsetSeconds) {
        QByteArrayView ianaId = data->id();
        qsizetype cut = ianaId.indexOf(' ');
        QByteArrayView cutId = (cut < 0 ? ianaId : ianaId.first(cut));
        if (cutId == utcQByteArray()) {
            // optimize: reuse interned strings for the common case
            id = utcQByteArray();
            name = utcQString();
        } else {
            // fallback to allocate new strings otherwise
            id = cutId.toByteArray();
            name = QString::fromUtf8(id);
        }
        Q_ASSERT(!name.isEmpty());
    } else { // Fall back to a UTC-offset name:
        name = isoOffsetFormat(offsetSeconds, QTimeZone::ShortName);
        id = name.toUtf8();
    }
    init(id, offsetSeconds, name, name, QLocale::AnyTerritory, name);
}

QUtcTimeZonePrivate::QUtcTimeZonePrivate(const QByteArray &zoneId, int offsetSeconds,
                                         const QString &name, const QString &abbreviation,
                                         QLocale::Territory territory, const QString &comment)
{
    init(zoneId, offsetSeconds, name, abbreviation, territory, comment);
}

QUtcTimeZonePrivate::QUtcTimeZonePrivate(const QUtcTimeZonePrivate &other)
    : QTimeZonePrivate(other), m_name(other.m_name),
      m_abbreviation(other.m_abbreviation),
      m_comment(other.m_comment),
      m_territory(other.m_territory),
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

// Override to shortcut past base's complications:
QTimeZonePrivate::Data QUtcTimeZonePrivate::data(QTimeZone::TimeType timeType) const
{
    Q_UNUSED(timeType);
    return data(QDateTime::currentMSecsSinceEpoch());
}

bool QUtcTimeZonePrivate::isDataLocale(const QLocale &locale) const
{
    // Officially only supports C locale names; these are surely also viable for English.
    return locale.language() == QLocale::C || locale.language() == QLocale::English;
}

void QUtcTimeZonePrivate::init(const QByteArray &zoneId, int offsetSeconds, const QString &name,
                               const QString &abbreviation, QLocale::Territory territory,
                               const QString &comment)
{
    m_id = zoneId;
    m_offsetFromUtc = offsetSeconds;
    m_name = name;
    m_abbreviation = abbreviation;
    m_territory = territory;
    m_comment = comment;
}

QLocale::Territory QUtcTimeZonePrivate::territory() const
{
    return m_territory;
}

QString QUtcTimeZonePrivate::comment() const
{
    return m_comment;
}

// Override to bypass complications in base-class:
QString QUtcTimeZonePrivate::displayName(qint64 atMSecsSinceEpoch,
                                         QTimeZone::NameType nameType,
                                         const QLocale &locale) const
{
    Q_UNUSED(atMSecsSinceEpoch);
    return displayName(QTimeZone::StandardTime, nameType, locale);
}

QString QUtcTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                         QTimeZone::NameType nameType,
                                         const QLocale &locale) const
{
    Q_UNUSED(timeType);
    Q_UNUSED(locale);
    if (nameType == QTimeZone::ShortName)
        return m_abbreviation;
    else if (nameType == QTimeZone::OffsetName)
        return isoOffsetFormat(m_offsetFromUtc);
    return m_name;
}

QString QUtcTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch);
    return m_abbreviation;
}

qint32 QUtcTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch);
    return m_offsetFromUtc;
}

qint32 QUtcTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch);
    return 0;
}

QByteArray QUtcTimeZonePrivate::systemTimeZoneId() const
{
    return utcQByteArray();
}

bool QUtcTimeZonePrivate::isTimeZoneIdAvailable(const QByteArray &ianaId) const
{
    // Only the zone IDs supplied by CLDR and recognized by constructor.
    for (const UtcData &data : utcDataTable) {
        if (isEntryInIanaList(ianaId, data.id()))
            return true;
    }
    // Callers may want to || offsetFromUtcString(ianaId) != invalidSeconds(),
    // but those are technically not IANA IDs and the custom QTimeZone
    // constructor needs the return here to reflect that.
    return false;
}

QList<QByteArray> QUtcTimeZonePrivate::availableTimeZoneIds() const
{
    // Only the zone IDs supplied by CLDR and recognized by constructor.
    QList<QByteArray> result;
    result.reserve(std::size(utcDataTable));
    for (const UtcData &data : utcDataTable) {
        QByteArrayView id = data.id();
        qsizetype cut;
        while ((cut = id.indexOf(' ')) >= 0) {
            result << id.first(cut).toByteArray();
            id = id.sliced(cut + 1);
        }
        result << id.toByteArray();
    }
    // Not guaranteed to be sorted, so sort:
    std::sort(result.begin(), result.end());
    // ### assuming no duplicates
    return result;
}

QList<QByteArray> QUtcTimeZonePrivate::availableTimeZoneIds(QLocale::Territory country) const
{
    // If AnyTerritory then is request for all non-region offset codes
    if (country == QLocale::AnyTerritory)
        return availableTimeZoneIds();
    return QList<QByteArray>();
}

QList<QByteArray> QUtcTimeZonePrivate::availableTimeZoneIds(qint32 offsetSeconds) const
{
    // Only if it's present in CLDR. (May get more than one ID: UTC, UTC+00:00
    // and UTC-00:00 all have the same offset.)
    QList<QByteArray> result;
    const auto data = std::lower_bound(std::begin(utcDataTable), std::end(utcDataTable),
                                       offsetSeconds, atLowerUtcOffset);
    if (data != std::end(utcDataTable) && data->offsetFromUtc == offsetSeconds) {
        QByteArrayView id = data->id();
        qsizetype cut;
        while ((cut = id.indexOf(' ')) >= 0) {
            result << id.first(cut).toByteArray();
            id = id.sliced(cut + 1);
        }
        result << id.toByteArray();
    }
    // CLDR only has round multiples of a quarter hour, and only some of
    // those. For anything else, throw in the ID we would use for this offset
    // (if we'd accept that ID).
    QByteArray isoName = isoOffsetFormat(offsetSeconds, QTimeZone::ShortName).toUtf8();
    if (offsetFromUtcString(isoName) == qint64(offsetSeconds) && !result.contains(isoName))
        result << isoName;
    // Not guaranteed to be sorted, so sort:
    std::sort(result.begin(), result.end());
    // ### assuming no duplicates
    return result;
}

#ifndef QT_NO_DATASTREAM
void QUtcTimeZonePrivate::serialize(QDataStream &ds) const
{
    ds << QStringLiteral("OffsetFromUtc") << QString::fromUtf8(m_id) << m_offsetFromUtc << m_name
       << m_abbreviation << static_cast<qint32>(m_territory) << m_comment;
}
#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE
