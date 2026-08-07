// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2013 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qtimezone.h"
#include "qtimezoneprivate_p.h"
#if QT_CONFIG(timezone_locale)
#  include "qtimezonelocale_p.h"
#endif
#include "qtimezoneprivate_data_p.h"

#include <QtCore/qbitarray.h>
#include <qdatastream.h>
#include <qdebug.h>
#include <qstring.h>

#include <private/qcalendarmath_p.h>
#include <private/qduplicatetracker_p.h>
#include <private/qnumeric_p.h>
#if QT_CONFIG(icu) || !QT_CONFIG(timezone_locale)
#  include <private/qstringiterator_p.h>
#endif
#include <private/qtools_p.h>

#include <algorithm>

#ifdef Q_OS_WASM
#include <emscripten/val.h>
#endif

QT_BEGIN_NAMESPACE

using namespace QtMiscUtils;
using namespace QtTimeZoneCldr;
using namespace Qt::StringLiterals;

// For use with std::is_sorted() in assertions:
[[maybe_unused]]
constexpr bool earlierZoneData(ZoneData less, ZoneData more) noexcept
{
    return less.windowsIdKey < more.windowsIdKey
        || (less.windowsIdKey == more.windowsIdKey && less.territory < more.territory);
}

[[maybe_unused]]
static bool earlierWinData(WindowsData less, WindowsData more) noexcept
{
    // Actually only tested in the negative, to check more < less never happens,
    // so should be true if more < less in either part; hence || not && combines.
    return less.windowsIdKey < more.windowsIdKey
        || less.windowsId().compare(more.windowsId(), Qt::CaseInsensitive) < 0;
}

// For use with std::lower_bound():
constexpr bool atLowerUtcOffset(UtcData entry, qint32 offsetSeconds) noexcept
{
    return entry.offsetFromUtc < offsetSeconds;
}

constexpr bool atLowerWindowsKey(WindowsData entry, qint16 winIdKey) noexcept
{
    return entry.windowsIdKey < winIdKey;
}

static bool earlierAliasId(AliasData entry, QByteArrayView aliasId) noexcept
{
    return entry.aliasId().compare(aliasId, Qt::CaseInsensitive) < 0;
}

static bool earlierWindowsId(WindowsData entry, QByteArrayView winId) noexcept
{
    return entry.windowsId().compare(winId, Qt::CaseInsensitive) < 0;
}

constexpr bool zoneAtLowerWindowsKey(ZoneData entry, qint16 winIdKey) noexcept
{
    return entry.windowsIdKey < winIdKey;
}

// Static table-lookup helpers
static quint16 toWindowsIdKey(QByteArrayView winId)
{
    // Key and winId are monotonic, table is sorted on them.
    const auto data = std::lower_bound(std::begin(windowsDataTable), std::end(windowsDataTable),
                                       winId, earlierWindowsId);
    if (data != std::end(windowsDataTable) && data->windowsId() == winId)
        return data->windowsIdKey;
    return 0;
}

static QByteArrayView toWindowsIdLiteral(quint16 windowsIdKey)
{
    // Caller should be passing a valid (in range) key; and table is sorted in
    // increasing order, with no gaps in numbering, starting with key = 1 at
    // index [0]. So this should normally work:
    if (Q_LIKELY(windowsIdKey > 0 && windowsIdKey <= std::size(windowsDataTable))) {
        const auto &data = windowsDataTable[windowsIdKey - 1];
        if (Q_LIKELY(data.windowsIdKey == windowsIdKey))
            return data.windowsId();
    }
    // Fall back on binary chop - key and winId are monotonic, table is sorted on them:
    const auto data = std::lower_bound(std::begin(windowsDataTable), std::end(windowsDataTable),
                                       windowsIdKey, atLowerWindowsKey);
    if (data != std::end(windowsDataTable) && data->windowsIdKey == windowsIdKey)
        return data->windowsId();

    return {};
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

QTimeZonePrivate::~QTimeZonePrivate()
{
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
        if (nameType == QTimeZone::OffsetName && isAnglicLocale(locale))
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
#if QT_CONFIG(timezone_locale) // Takes care of offsetformat:
        return localeName(tran.atMSecsSinceEpoch, tran.offsetFromUtc, timeType, nameType, locale);
#else // All this base can help with is offset names:
        if (nameType == QTimeZone::OffsetName && isAnglicLocale(locale))
            return isoOffsetFormat(tran.offsetFromUtc);
#endif // Hopefully derived classes can do better.
    }
    return QString();
}

QString QTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    if (QLocale() != QLocale::c()) {
        const QString name = displayName(atMSecsSinceEpoch, QTimeZone::ShortName, QLocale());
        if (!name.isEmpty())
            return name;
    }
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
    const auto validMatch = [timeType](const Data &tran) {
        return tran.atMSecsSinceEpoch != invalidMSecs()
            && ((timeType == QTimeZone::DaylightTime) != (tran.daylightTimeOffset == 0));
    };

    // Get current tran, use if suitable:
    const qint64 currentMSecs = QDateTime::currentMSecsSinceEpoch();
    Data tran = data(currentMSecs);
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
    auto dataToState = [](const Data &d) {
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
    // At most one of those was clipped to its boundary value, but recent may be millis + 1:
    Q_ASSERT(recent < imminent && seventeenHoursInMSecs - 1 <= imminent - recent);
    // (We only actually need imminent - recent > abs(zone offset), so the stray 2 ms won't hurt.)

    const Data past = data(recent), future = data(imminent);
    if (future.atMSecsSinceEpoch == invalidMSecs()
        && past.atMSecsSinceEpoch == invalidMSecs()) {
        // Failed to get any useful data near this time: apparently out of range
        // for the backend.
        return { forLocalMSecs };
    }
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
        // An offset of 17 hours or more could trigger that assert.
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

template <typename Pred>
static QByteArrayView aliasMatching(QByteArrayView name, Pred test)
{
    if (test(name))
        return name;
    {
        // First, if it's an alias, map name to its CLDR form:
        const auto data = std::lower_bound(std::begin(aliasMappingTable),
                                           std::end(aliasMappingTable),
                                           name, earlierAliasId);
        if (data != std::end(aliasMappingTable) && data->aliasId() == name) {
            name = data->ianaId();
            if (test(name))
                return name;
        }
        // Now name is the canonical CLDR name, even if it was previously an alias.
    }
    // Failing that, traverse the whole alias mapping table in search of an
    // alias for name that satisfies test():
    for (const auto &data : aliasMappingTable) {
        QByteArrayView alias = data.aliasId();
        if (data.ianaId() == name && test(alias))
            return alias;
    }
    return {};
}

QByteArrayView QTimeZonePrivate::availableAlias(QByteArrayView ianaId) const
{
    return aliasMatching(ianaId, [this](QByteArrayView id) { return isTimeZoneIdAvailable(id); });
}

bool QTimeZonePrivate::isTimeZoneIdAvailable(QByteArrayView ianaId) const
{
    // Fall-back implementation, can be made faster in subclasses.
    // Backends that don't cache the available list SHOULD override this.
    const QList<QByteArray> tzIds = availableTimeZoneIds();
    return std::binary_search(tzIds.begin(), tzIds.end(), ianaId);
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
    // Default fall-back mode: use the CLDR data to find zones for this territory.
    QList<QByteArrayView> regions;
#if QT_CONFIG(timezone_locale) && !QT_CONFIG(icu)
    regions = QtTimeZoneLocale::ianaIdsForTerritory(territory);
#endif
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

QList<QByteArray> QTimeZonePrivate::uniqueSortedAliasPadded(QList<QByteArray> &&zoneIds)
{
    // Inputs are not expected to be sorted. (Use padSortedWithAliases() when they are.)
    const QList<QByteArray> source = zoneIds;
    // If we include a zone, include also its CLDR-standard name:
    for (const auto &name : source) {
        const auto zone = aliasToIana(name);
        if (!zone.isEmpty()) {
            zoneIds << zone.toByteArray();
            Q_ASSERT(aliasToIana(zone).isEmpty());
        }
    }
    std::sort(zoneIds.begin(), zoneIds.end());
    zoneIds.erase(std::unique(zoneIds.begin(), zoneIds.end()), zoneIds.end());
    return zoneIds;
}

QList<QByteArray> QTimeZonePrivate::padSortedWithAliases(QList<QByteArray> &&zoneIds)
{
    // Input is assumed sorted; this is preserved, as is uniqueness if it was unique.
    const QList<QByteArray> source = zoneIds;
    for (const auto &name : source) {
        const auto zone = aliasToIana(name);
        const auto pos = std::lower_bound(zoneIds.begin(), zoneIds.end(), zone);
        if (pos != zoneIds.end() && *pos != zone)
            zoneIds.insert(pos, zone.toByteArray());
    }
    return zoneIds;
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
bool QTimeZonePrivate::isValidId(QByteArrayView ianaId)
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

#if QT_CONFIG(icu) || !QT_CONFIG(timezone_locale)
static QTimeZonePrivate::NamePrefixMatch
findUtcOffsetPrefix(QStringView text, const QLocale &locale)
{
    // First, see if we have a {UTC,GMT}+offset. This would ideally use
    // locale-appropriate versions of the offset format, but we don't know those.
    qsizetype signLen = 0;
    char sign = '\0';
    auto signStart = [&signLen, &sign, locale](QStringView str) {
        QString signStr = locale.negativeSign();
        if (str.startsWith(signStr)) {
            sign = '-';
            signLen = signStr.size();
            return true;
        }
        // Special case: U+2212 MINUS SIGN (cf. qlocale.cpp's NumericTokenizer)
        if (str.startsWith(u'\u2212')) {
            sign = '-';
            signLen = 1;
            return true;
        }
        signStr = locale.positiveSign();
        if (str.startsWith(signStr)) {
            sign = '+';
            signLen = signStr.size();
            return true;
        }
        return false;
    };
    // Should really use locale-appropriate
    if (!((text.startsWith(u"UTC") || text.startsWith(u"GMT")) && signStart(text.sliced(3))))
        return {};

    QStringView offset = text.sliced(3 + signLen);
    QStringIterator iter(offset);
    qsizetype hourEnd = 0, hmMid = 0, minEnd = 0;
    int digits = 0;
    char32_t ch = 0;
    while (digits < 4 && iter.hasNext()) {
        ch = iter.next();
        if (!QChar::isDigit(ch))
            break;

        ++digits;
        // Have hourEnd keep track of the end of the last-but-two digit, if
        // we have that many; use hmMid to hold the last-but-one.
        hourEnd = std::exchange(hmMid, std::exchange(minEnd, iter.index()));
    }
    if (!digits) // No offset.
        return {};

    QStringView hourStr, minStr;
    if (digits == 4) {
        minStr = offset.first(minEnd).sliced(hourEnd);
    } else if (digits < 3 && iter.hasNext() && QChar::isPunct(ch)) {
        hourEnd = minEnd; // Use all digits seen thus far for hour.
        hmMid = iter.index(); // Reuse as minStart, in effect.
        int mindig = 0;
        while (mindig < 2 && iter.hasNext() && QChar::isDigit(iter.next())) {
            ++mindig;
            minEnd = iter.index();
        }
        if (mindig == 2)
            minStr = offset.first(minEnd).sliced(hmMid);
        else
            minEnd = hourEnd; // Ignore punctuator and beyond
    } else { // Not enough digits for a minute field.
        minEnd = hourEnd;
    }
    hourStr = offset.first(hourEnd);

    bool ok = false;
    uint hour = 0, minute = 0;
    if (!hourStr.isEmpty())
        hour = locale.toUInt(hourStr, &ok);
    if (ok && !minStr.isEmpty()) {
        minute = locale.toUInt(minStr, &ok);
        // If the part after a punctuator is bad, pretend we never saw it:
        if ((!ok || minute >= 60) && minEnd > hourEnd + minStr.size()) {
            minEnd = hourEnd;
            minute = 0;
            ok = true;
        }
        // but if we had too many digits for just an hour, and its tail
        // isn't minutes, then this isn't an offset form.
    }

    constexpr int MaxOffsetSeconds
        = qMax(QTimeZone::MaxUtcOffsetSecs, -QTimeZone::MinUtcOffsetSecs);
    if (!ok || (hour * 60 + minute) * 60 > MaxOffsetSeconds)
        return {}; // Let the zone-name scan find UTC or GMT prefix as a zone name.

    // Transform offset into the form the QTimeZone constructor prefers:
    char buffer[26];
    // We need: 3 for "UTC", 1 for sign, 2+2 for digits, 1 for colon between, 1
    // for '\0'; but gcc [-Werror=format-truncation=] doesn't know the %02u
    // fields can't be longer than 2 digits, so complains if we don't have space
    // for 10 digits in each.
    if (minute)
        std::snprintf(buffer, sizeof(buffer), "UTC%c%02u:%02u", sign, hour, minute);
    else
        std::snprintf(buffer, sizeof(buffer), "UTC%c%02u", sign, hour);

    return { QByteArray(buffer, qstrnlen(buffer, sizeof(buffer))),
             3 + signLen + minEnd,
             QTimeZone::GenericTime };
}

QTimeZonePrivate::NamePrefixMatch
QTimeZonePrivate::findLongNamePrefix(QStringView text, const QLocale &locale,
                                     std::optional<qint64> atEpochMillis)
{
    // Search all known zones for one that matches a prefix of text in our locale.
    // We allow an offset form, as those are used as long names for QUtcTZP:
    QTimeZonePrivate::NamePrefixMatch best = findUtcOffsetPrefix(text, locale);

    const auto matchLength = [text](QStringView name) -> qsizetype {
        qsizetype length = 0; // "Does not match" by default.
        if (name.size() > 0 && text.startsWith(name, Qt::CaseInsensitive)) {
            length = name.size();
            // But a case-insensitive match might have different length:
            while (!text.first(length).startsWith(name, Qt::CaseInsensitive)) {
                ++length;
                Q_ASSERT(length <= text.size());
            }
            // If we didn't need to grow, check whether we can shrink:
            if (length == name.size()) {
                while (length > 0 && text.first(length - 1).startsWith(name, Qt::CaseInsensitive))
                    --length;
            }
        }
        return length;
    };
    const auto when = atEpochMillis
        ? QDateTime::fromMSecsSinceEpoch(*atEpochMillis, QTimeZone::UTC)
        : QDateTime();
    const auto typeFor = [when](QTimeZone zone) {
        if (when.isValid() && zone.isDaylightTime(when))
            return QTimeZone::DaylightTime;
        // Assume standard time name applies equally as generic:
        return QTimeZone::GenericTime;
    };
    const auto tryZone = [&](const QByteArray &iana) {
        bool matched = false;
        constexpr QTimeZone::TimeType types[]
            = { QTimeZone::GenericTime, QTimeZone::StandardTime, QTimeZone::DaylightTime };
        QTimeZone zone(iana);
        if (!zone.isValid())
            return matched;
        if (when.isValid()) {
            const QString name = zone.displayName(when, QTimeZone::LongName, locale);
            if (qsizetype match = matchLength(name); match > best.nameLength) {
                best = { iana, match, typeFor(zone) };
                matched = true;
            }
        } else {
            const bool neverDst = !zone.hasDaylightTime();
            for (const QTimeZone::TimeType type : types) {
                if (neverDst && type == QTimeZone::DaylightTime)
                    continue;
                const QString name = zone.displayName(type, QTimeZone::LongName, locale);
                if (qsizetype match = matchLength(name); match > best.nameLength) {
                    best = { iana, match, type };
                    matched = true;
                }
            }
        }
        return matched;
    };

    const QList<QByteArray> allZones = []() {
        QList<QByteArray> avail = QTimeZone::availableTimeZoneIds();
        const auto isCanonical = [](const QByteArray &name) {
            // Canonical <=> not an alias
            return QTimeZonePrivate::aliasToIana(name).isEmpty();
        };
        [[maybe_unused]] const QList<QByteArray>::const_iterator
            firstAlias = std::partition(avail.begin(), avail.end(), isCanonical);
        // Everything before firstAlias is canonical; everything after is an alias.
        // Some available IDs may be aliases for IANA IDs not in the list.
        Q_ASSERT(std::all_of(firstAlias, avail.constEnd(), // Every alias ...
                             [from = avail.constBegin(), to = firstAlias,
                              avail](const QByteArray &alias) {
                                 // ... maps to a canonical name:
                                 QByteArrayView iana = QTimeZonePrivate::aliasToIana(alias);
                                 return std::find_if(from, to, [iana](const QByteArray &zone) {
                                     return zone == iana;
                                 }) != to || !avail.contains(iana);
                                 // ... which might not be available.
                             }));
        return avail;
    }();

    for (const QByteArray &iana : allZones) {
        // If we have a match for all of text, we can't get any better:
        if (tryZone(iana) && best.nameLength >= text.size())
            break;
    }
    // This has the problem of selecting the first IANA ID of a zone with a
    // match; where several IANA IDs share a long name, this may not be the
    // natural one to pick. Hopefully a backend that does its own name L10n will
    // at least produce one with the same offsets as the most natural choice.
    // The initialization of allZones should at least mean we prefer canonical.

    // However, some zones may have aliases that are supported and get different
    // display names, e.g. because the ID appears as part of the name.
    if (!best) {
        // Search the alias table for names we've not tried that might be
        // supported but not "available". Non-canonical aliases of available
        // names won't have been added to allZones.
        QDuplicateTracker<QByteArray, std::size(aliasMappingTable)> triedAlready;
        for (const QByteArray &iana : allZones)
            (void) triedAlready.hasSeen(iana);
        for (const auto &data : aliasMappingTable) {
            const QByteArray alias = data.aliasId().toByteArray();
            if (!triedAlready.hasSeen(alias) && tryZone(alias) && best.nameLength >= text.size())
                break;
        }
    }

    return best;
}

QTimeZonePrivate::NamePrefixMatch
QTimeZonePrivate::findNarrowOffsetPrefix(QStringView, const QLocale &)
{
    // Seemingly only needed in the timezonelocale case.
    return {};
}
#else
// Implemented in qtimezonelocale.cpp
#endif // icu || !timezone_locale

#if QT_CONFIG(timezone_locale) && !QT_CONFIG(icu)
// The timezone_locale-without-ICU backend's data suffices to do better than
// this brute force solution:
#  define BACKEND_PROVIDES_OFFSET_PREFIX
#endif
// Hopefully we can do similar for some other backends.

#ifdef BACKEND_PROVIDES_OFFSET_PREFIX
#  undef BACKEND_PROVIDES_OFFSET_PREFIX
#else // Need the brute force implementation of findOffsetPrefix():
namespace {

struct NumericPattern
{
    NumericPattern(QStringView text, const QLocale &locale);

    // +ve entries are counts of consecutive signs-and-digits,
    // -ve entries are counts of everything else, separating those blocks.
    QList<qsizetype> pattern;
    bool hasDigits;
    bool digitsAreLocale;
    unsigned char sign; // '\0': no sign; '+' or '-': one seen; '+'|'-' = '/': both seen.

private:
    // Used during construction:
    class Scanner
    {
    public:
        using Sign = unsigned char; // as for NumericPattern::sign
    private:
        bool scanForToken(QStringView sought)
        {
            // Side-effect: sets bits in mask for positions in pattern occupied by sought.
            // Returns true if any matches found.
            if (sought.isEmpty()) // Despite empty techically matching everywhere, reject.
                return false;
            qsizetype tokensMatched = 0;
            const qsizetype n = sought.size();
            qsizetype idx = -n; // To cancel the first iteration's +n:
            while ((idx = given.indexOf(sought, idx + n)) >= 0) {
                for (qsizetype i = 0; i < n; ++i)
                    mask.setBit(idx + i);
                ++tokensMatched;
            }
            return tokensMatched > 0;
        }

        Sign scanForSignsImpl(const QLocale &locale, Sign signs)
        {
            // Side-effect: sets bits in mask for positions in pattern occupied by signs.
            // Returns the bit-wise-| of '+' and '-' for signs seen.
            if (scanForToken(locale.positiveSign()))
                signs |= '+';
            if (scanForToken(locale.negativeSign()))
                signs |= '-';
            return signs;
        }

        QStringView given; // Text to be scanned
    public:
        QBitArray mask; // Bits are set for digits and signs, unset otherwise.

        Scanner(QStringView text) : given(text), mask(text.size()) {}

        bool scanForDigits(const QLocale &locale)
        {
            // Side-effect: sets bits in mask for positions in pattern occopied by digits.
            // Returns true if it finds any digits.
            bool matched = false;
            for (int i = 0; i < 10; ++i) {
                if (scanForToken(locale.toString(i)))
                    matched = true;
            }
            return matched;
        }

        Sign scanForSigns(const QLocale &locale)
        {
            // Side-effect: sets bits in maks for positions occupied by signs.
            // Returns the bit-wise-| of '+' and '-' for signs seen.
            Sign signs = scanForSignsImpl(locale, '\0');
            signs = scanForSignsImpl(QLocale::c(), signs);
            if (scanForToken(u"\u2212")) // Canonical minus sign
                signs |= '-';
            return signs;
        }

        QList<qsizetype> asPattern() const
        {
            // Re-encode mask as a sequence of counts of consecutive equal bits,
            // negated for runs of false bits, positive for runs of true bits.
            QList<qsizetype> res;
            qsizetype cur = 0;
            for (qsizetype i = 0, n = mask.size(); i < n; ++i) {
                if (mask.testBit(i)) {
                    if (cur < 0) {
                        res.push_back(cur);
                        cur = 0;
                    }
                    ++cur;
                } else {
                    if (cur > 0) {
                        res.push_back(cur);
                        cur = 0;
                    }
                    --cur;
                }
            }
            if (cur)
                res.push_back(cur);
            return res;
        }
    };
};

NumericPattern::NumericPattern(QStringView text, const QLocale &locale)
{
    // Decompose text into sequences of sign-and-digits and of literals; the
    // former are presumed to convey the numeric part of an offset, the latter
    // are literals that must match verbatim.
    Scanner scanner(text);
    digitsAreLocale = hasDigits = scanner.scanForDigits(locale);
    if (!hasDigits)
        hasDigits = scanner.scanForDigits(QLocale::c());

    sign = scanner.scanForSigns(locale);
    // Finally, convert scanner's QBitArray to our list of signed block-sizes:
    pattern = scanner.asPattern();
}

class PatternAligner
{
    QStringView txt;
    const QList<qsizetype> &txtPat;
    const QtTemporalPattern::TemporalFieldFlags options;
    qsizetype txtPos = 0, txtInd = 0;
    static constexpr uint Hour = 1, Minute = 2, Second = 4; // pseudo-flag-enum
    uint seenFields = 0;
    using Digits = QLocaleData::DigitSequence;

    bool textMatch(QStringView str, qsizetype strPos, qsizetype slen, qsizetype tlen) const
    {
        if (slen != tlen) // Cheap pre-check:
            return false;
        if (txt.sliced(txtPos, tlen).compare(str.sliced(strPos, slen), Qt::CaseInsensitive) == 0)
            return true;
        // Special case: allow a leading "UTC" to match "GMT":
        if (txtInd == 0 && slen == 3 && txt.first(3) == u"GMT" && str.first(3) == u"UTC") {
            Q_ASSERT(txtPos == 0);
            Q_ASSERT(strPos == 0);
            return true;
        }
        return false;
    }

    bool allowField(uint fieldBit) const;
    bool allowSkipField(Digits &&fmt) const;
    auto readField(QByteArrayView field, uint fieldBit, int *value);
    bool scanExtraFields(QStringView sep, const QLocaleData *locData,
                         qsizetype &txtLen, int &second);
    qsizetype scanMatchedFields(const Digits &fmt, const Digits &src, bool allowExtraFields,
                                int &hour, int &minute, int &second, int &sign);
    void reset()
    {
        txtPos = 0;
        txtInd = 0;
        seenFields = 0;
    }

public:
    PatternAligner(QStringView text, const QList<qsizetype> &textPattern,
                   QtTemporalPattern::TemporalFieldFlags flags)
        : txt(text), txtPat(textPattern), options(flags) {}

    // The arbitrary offset used, 10:37:25, is chosen to have no repeat digits
    // and no leading zeros (when presented in two-digit fields). This makes
    // recognising its representation in an offset text straightforward.
    static constexpr qint32 OffsetMagnitude = 38245; // 10h 37m 25s in seconds.
    static constexpr QByteArrayView hourAscii{"10"}, minuteAscii{"37"}, secondAscii{"25"};

    auto match(QStringView str, const QList<qsizetype> &strPat,
               const QLocaleData *locData, char signChar);
};

bool PatternAligner::allowField(uint fieldBit) const
{
    if (!fieldBit || (seenFields & fieldBit))
        return false;

    // TODO: Standalone | Short is ASCII-only; must be settled further up the call-stack
    using namespace QtTemporalPattern::FieldGroup;
    if (!options.testAnyFlags(WidthMask))
        return true;

    switch (fieldBit) {
        using namespace QtTemporalPattern;
        using F = TemporalFieldFlag;
    case Hour: // Allowed by every format
        return true;
    case Minute: // Only excluded by Narrow (and we can infer some other width is set if it isn't):
        return matchesFlagsWithin(options, WidthMask & ~F::Narrow, WidthMask);
    case Second:
        return matchesFlagsWithin(options, F::Wide | F::Short, WidthMask);
    }
    Q_UNREACHABLE_RETURN(false);
}

bool PatternAligner::allowSkipField(Digits &&fmt) const
{
    // Only ever called when fields are separated.
    uint fieldBit = 0;
    if (fmt.digits.startsWith(minuteAscii))
        fieldBit = Minute;
    else if (fmt.digits.startsWith(secondAscii))
        fieldBit = Second;
    else // Unrecognized field or hour can't be skipped.
        return false;
    // Should never arise, but if we've already seen the field we can skip it:
    if (Q_UNLIKELY(seenFields & fieldBit))
        return true;
    // Should never arise, but we can't skip minute if we've read second:
    if (Q_UNLIKELY(seenFields & Second) && fieldBit == Minute)
        return false;

    // If no widths are set, all are allowed.
    if (options.testAnyFlags(QtTemporalPattern::FieldGroup::WidthMask)) {
        using F = QtTemporalPattern::TemporalFieldFlag;
        // If the width prohibits this field, we can skip it.
        // ZeroPad also allows skipping.
        switch (fieldBit) {
        case Minute:
            return options.testAnyFlags(F::ZeroPad | F::Narrow);
        case Second:
            return options.testAnyFlags(F::ZeroPad | F::Narrow | F::Abbreviated);
        }
    }
    return true;
}

auto PatternAligner::readField(QByteArrayView field, uint fieldBit, int *value)
{
    // Greedy, subject to limits on field value:
    constexpr int MaxHourOffset
        = qMax(QTimeZone::MaxUtcOffsetSecs, -QTimeZone::MinUtcOffsetSecs) / 3600;
    static_assert(MaxHourOffset > 9); // So single-digit value is always in range.
    struct R {
        QByteArrayView used;
        bool ok;
    } res = { field, false };
    // Only hour field is allowed to be single-digit:
    if ((fieldBit != Hour && res.used.size() < 2) || !allowField(fieldBit) || !value)
        return res;
    Q_ASSERT(*value == 0); // Shouldn't be filling in a field that's already filled in.
    seenFields |= fieldBit;
    if (res.used.size() > 2)
        res.used = res.used.first(2);
    *value = res.used.toInt(&res.ok);
    if (fieldBit == Hour && (!res.ok || *value > MaxHourOffset)) {
        res.used.chop(1);
        *value = res.used.toInt(&res.ok);
    }
    return res;
}

// For when txt has fields absent from fmt:
bool PatternAligner::scanExtraFields(QStringView sep, const QLocaleData *locData,
                                     qsizetype &txtLen, int &second)
{
    Q_ASSERT(locData); // Non-empty sep => have digits.
    // Matching sep doesn't count unless there's a number after it:
    while (txtInd + 1 < txtPat.size() && txt.sliced(txtPos, -txtLen) == sep) {
        txtPos -= txtLen;
        txtLen = txtPat.at(++txtInd);
        // To have a separator, we must have seen two fields:
        Q_ASSERT((seenFields & Hour) && (seenFields & Minute));
        if (seenFields & Second) // Too many extra fields
            return false;
        Q_ASSERT(second == 0);
        const Digits asciiParse
            = locData->digitSequence(txt.sliced(txtPos, txtLen));
        QByteArrayView found = asciiParse.digits;
        bool ok = false;
        second = found.toInt(&ok);
        if (!ok || second >= 60)
            return false;
        seenFields |= Second;
        txtPos += txtLen;
        txtLen = ++txtInd < txtPat.size() ? txtPat.at(txtInd) : 0;
    }

    return true;
}

// For when we can use the content of fmt fields to indicate which field they are:
qsizetype PatternAligner::scanMatchedFields(const Digits &fmt, const Digits &src,
                                            bool allowExtraFields,
                                            int &hour, int &minute, int &second, int &sign)
{
    if (fmt.sign) {
        if (!fmt.digits.startsWith(hourAscii))
            return -1; // Sign must be applied to hour, no other field
        if (sign || !src.sign)
            return -1; // Only one sign, txt must have sign where str does
        sign = fmt.sign == src.sign ? +1 : -1;
    } else if (src.sign) {
        return -1; // If str lacks sign, so must txt.
    }

    QByteArrayView chosen{fmt.digits}, found{src.digits};
    while (chosen.size() && found.size()) {
        const uint priorFields = seenFields;
        auto read = chosen.startsWith(hourAscii) ? readField(found, Hour, &hour)
            : chosen.startsWith(minuteAscii) ? readField(found, Minute, &minute)
            : chosen.startsWith(secondAscii) ? readField(found, Second, &second)
            : readField(found, 0u, nullptr);
        if (!read.ok) // Always catches the last case of the precedeing.
            return -1;

        if (read.used.size() < 2) {
            const uint newField = (seenFields ^ priorFields);
            // Only hour can be shorter than two digits, and even then only when
            // it's all there is.
            if (newField == Hour) {
                // We can't ignore it as dangling cruft, as hour is always required.
                if (priorFields) // It wasn't the only field.
                    return -1;
                // Anything after this is dangling cruft. We must assume elided
                // zeros for minutes and seconds.
                chosen = {};
                found = found.sliced(read.used.size());
                allowExtraFields = false;
                break;
            }
            // If this isn't the first field and we've seen an hour field ...
            if (chosen.size() < fmt.digits.size() && (priorFields & Hour)) {
                // ... treat current field as start of dangling cruft.
                // Forget we've seen it:
                seenFields = priorFields;
                Q_ASSERT(newField == Minute || newField == Second);
                if (newField == Minute)
                    minute = 0;
                else
                    second = 0;
                // Assume elided zeros for remaining fields.
                chosen = {};
                allowExtraFields = false;
                break;
            }
            return -1;
        }

        Q_ASSERT(chosen.size() >= 2); // It starts with a known two-digit string.
        chosen = chosen.sliced(2);
        found = found.sliced(read.used.size());
    }
    if (chosen.size()) {
        Q_ASSERT(found.isEmpty());
        // May have elided trailing zero (mins and) seconds.
        return (seenFields & Hour) ? 0 : -1;
    }

    if (found.size() && (seenFields & Hour)) {
        // If there's no later numeric field we might just have surplus precision.
        // Or we might just have dangling cruft.
        if (allowExtraFields) {
            if (!Q_LIKELY(seenFields & Minute)) {
                const uint priorFields = seenFields;
                auto read = readField(found, Minute, &minute);
                if (!read.ok || read.used.size() < 2) {
                    minute = 0;
                    seenFields = priorFields;
                    return found.size();
                }
                found = found.sliced(read.used.size());
            }
            if (!(seenFields & Second)) {
                const uint priorFields = seenFields;
                auto read = readField(found, Second, &second);
                if (!read.ok || read.used.size() < 2) {
                    second = 0;
                    seenFields = priorFields;
                    return found.size();
                }
                found = found.sliced(read.used.size());
            }
        }
        // Otherwise, interpret remaining digits as dangling cruft.
        return found.size();
    }
    // Can't write off any residue as dangling cruft because Hour isn't set:
    return found.size() ? -1 : 0;
}

auto PatternAligner::match(QStringView str, const QList<qsizetype> &strPat,
                           const QLocaleData *locData, char signChar)
{
    Q_ASSERT(!str.isEmpty() && !strPat.isEmpty());
    // Caller shall reverse sign for the negative-format call, so the sign of
    // the offset returned here is + if txt agrees with str, - if they're
    // opposite. This fails if a sign is unexpectedly present or expected and
    // missing.
    constexpr auto AllowSign = Digits::Option::AllowSign;
    struct R {
        int offset = 0;
        qsizetype length = 0;
        operator bool() const { return length > 0; }
    };
    if ((strPat.at(0) < 0) != (txtPat.at(0) < 0))
        return R{};
    reset();

    int hour = 0, minute = 0, second = 0;
    int sign = !signChar; // Sign of txt *relative to* str.
    // Defaults to +1 if there's no overt sign in the pattern; otherwise, require overt sign.
    QStringView sep; // Separator, if any, between numeric fields.

    qsizetype skip = 0, strPos = 0, txtSkipped = 0;
    // Entries alternate between +ve for numeric fields, -ve for verbatim texts:
    for (qsizetype len : strPat) {
        if (skip > 0) {
            Q_ASSERT(len > 0);
            Q_ASSERT(locData); // We only allow skipping if we have digits => locale data
            if (!allowSkipField(locData->digitSequence(str.sliced(strPos, len))))
                return R{};
            strPos += len;
            ++txtSkipped;
            --skip;
            continue;
        }

        if (len < 0) {
            qsizetype txtLen = txtInd < txtPat.size() ? txtPat.at(txtInd) : 0;
            const bool maybeSep = txtInd > 0 && txtInd + 1 < strPat.size();
            // If we've seen a separator and str has something else, skip over
            // any extra separator-numeric pairs in txt:
            if (!sep.isEmpty() && str.sliced(strPos, -len) != sep) {
                if (!scanExtraFields(sep, locData, txtLen, second))
                    return  R{};
            }
            if (maybeSep && sep.isEmpty())
                sep = str.sliced(strPos, -len);

            if (!textMatch(str, strPos, -len, -txtLen)) {
                // Conversely, if str is sep (and txt isn't), skip unmatched
                // fields, unless ZeroPad was set:
                if (locData && !sep.isEmpty() && str.sliced(strPos, -len) == sep) {
                    strPos -= len;
                    skip = 1;
                    ++txtSkipped;
                    continue;
                }
                // If this is the last field, txt's version merely needs to start with str's:
                if (txtInd + 1 < strPat.size() - txtSkipped || len <= txtLen
                    || !textMatch(str, strPos, -len, -len)) {
                    // txt doesn't match str, so fail
                    return R{};
                }
                txtLen = len;
            }
            // Found match.
            txtPos -= txtLen;
            ++txtInd;
            strPos -= len;
            continue;
        }
        Q_ASSERT(len > 0); // len is never zero.

        if (txtPos >= txt.size()) {
            Q_ASSERT(txtInd >= txtPat.size());
            // Numeric field in str not matched in txt.
            if (!sep.isEmpty() && txtPat.back() == sep.size() && txt.endsWith(sep)
                && strPat.back() == -sep.size() && str.endsWith(sep)) {
                // If the offset format ends in a terminator that's the same as
                // its separator, we can ignore a surplus field of str. Back up
                // txt by one step, so we do verify we're skipping nothing but
                // field-and-separator pairs:
                txtInd = txtPat.size() - 1;
                txtPos -= sep.size();
                continue;
            }
            // Otherwise the missing field is a failure to match.
            return R{};
        }
        if (!locData) // len > 0 is a numeric field, so only succeed if we have digits.
            return R{};

        // Numeric field:
        QStringView field = str.sliced(strPos, len);
        QStringView toParse = txt.sliced(txtPos, txtPat.at(txtInd));
        // It may comprise several fields of our offset, with empty separator.

        const Digits asciiField = locData->digitSequence(field, AllowSign);
        if (asciiField.endIndex() != field.size())
            return R{};
        const Digits asciiParse = locData->digitSequence(toParse, AllowSign);
        if (asciiParse.endIndex() != toParse.size())
            return R{};
        const uint priorFields = seenFields;
        // Allow extra fields if there's no sep or later numeric field:
        const qsizetype spare
            = scanMatchedFields(asciiField, asciiParse,
                                !txtSkipped && txtInd + 2 >= strPat.size() && sep.isEmpty(),
                                hour, minute, second, sign);
        if (spare < 0) // Did not match
            return R{};
        if (spare) {
            // If the pattern has a required end marker, we can't write off the
            // spare as dangling cruft unless that end marker is a field
            // separator, and we've seen one of those already, after an hour
            // field, in which case we can treat the whole present field as
            // dangling cruft.
            if (spare == 1 && (seenFields & Hour)) {
                // Too short for non-Hour field, can treat as dangling cruft.
            } else if (strPat.back() < 0) {
                if (sep.isEmpty() || !(priorFields & Hour)
                    || strPat.back() != -sep.size() || !str.endsWith(sep)) {
                    return R{};
                }
                int offset = hour * 60;
                if (priorFields & Minute)
                    offset += minute;
                offset *= 60;
                if (priorFields & Second)
                    offset += second;
                return R{ sign * offset, txtPos };
            }

            // Consume what we matched; ignore the rest as dangling cruft.
            txtPos += asciiParse.digitStart
                + (asciiParse.digits.size() - spare) * asciiParse.digitWidth;
            break;
        }

        strPos += asciiField.endIndex();
        txtPos += asciiParse.endIndex();
        ++txtInd;
    }
    if (skip) // The unmatched separator was actually a terminator.
        return R{};
    return R{ sign * (second + 60 * (minute + 60 * hour)), txtPos };
}

QTimeZonePrivate::NamePrefixMatch
findOffsetPrefixImpl(QStringView text, const QLocale &locale,
                     QtTemporalPattern::TemporalFieldFlags flags)
{
    QTimeZonePrivate::NamePrefixMatch best;
    if (text.isEmpty())
        return best;

    // Note: this is brute force applied to our ignorance of the formats used
    // for offsets by locale, effectively inferring them from some sample
    // display names of particular offsets. Any backend with access to the raw
    // formats in use should prefer to implement this a lot more efficiently and
    // #if-out this version when it's available.

    const QUtcTimeZonePrivate greenwich(0); // UTC
    // Deliberately messy so we see whether minutes (and even seconds) get displayed:
    const QUtcTimeZonePrivate positive(+PatternAligner::OffsetMagnitude);
    const QUtcTimeZonePrivate negative(-PatternAligner::OffsetMagnitude);

    constexpr QTimeZone::NameType formats[] = {
        QTimeZone::OffsetName, QTimeZone::LongName, QTimeZone::ShortName
    };
    constexpr QTimeZone::TimeType seasons[] = {
        QTimeZone::GenericTime, QTimeZone::StandardTime, QTimeZone::DaylightTime
    };

    const auto acceptFormat = [flags](QTimeZone::NameType format) {
        using namespace QtTemporalPattern;
        // If no relevant flags are set, all widths and forms are allowed.
        if (!flags.testAnyFlags(FieldGroup::WidthMask | FieldGroup::FormMask))
            return true;
        using Flag = TemporalFieldFlag;
        constexpr TemporalFieldFlags Textual = Flag::Verbal | Flag::Standalone;
        constexpr TemporalFieldFlags Long = Flag::Abbreviated | Flag::Short | Flag::Wide;
        switch (format) {
        case QTimeZone::OffsetName:
            return matchesFlagWithin(flags, Flag::Numeric, FieldGroup::FormMask);
        case QTimeZone::DefaultName:
        case QTimeZone::LongName:
            return matchesFlagsWithin(flags, Textual, FieldGroup::FormMask)
                && matchesFlagsWithin(flags, Long, FieldGroup::WidthMask);
        case QTimeZone::ShortName:
            return matchesFlagsWithin(flags, Textual, FieldGroup::FormMask)
                && matchesFlagWithin(flags, Flag::Narrow, FieldGroup::WidthMask);
        }
        Q_UNREACHABLE_RETURN(false);
    };
    const auto acceptSeason = [flags](QTimeZone::TimeType season) {
        using namespace QtTemporalPattern;
        // If no season flags are given, all time types are accepted:
        if (!flags.testAnyFlags(FieldGroup::SeasonMask))
            return true;
        using Flag = TemporalFieldFlag;
        switch (season) {
        case QTimeZone::GenericTime:
            return flags.testFlag(Flag::GenericTime);
        case QTimeZone::StandardTime:
            return flags.testFlag(Flag::StandardTime);
        case QTimeZone::DaylightTime:
            return flags.testFlag(Flag::DaylightSavingTime);
        }
        Q_UNREACHABLE_RETURN(false);
    };

    /* Scan with locale-appropriate digits first, then with ASCII (C-locale)
       digits, if different. Note that both scan's use the locale-appropriate
       *format* for offsets, so the rescan with C-locale's digits and the
       locale's format may produce different results to the second call of this
       function for the C locale, which uses its own format as well as digits.
     */
    QLocale digitLocale = locale;
    for (int i = 0; i < 2; ++i) {
        // Decompose text into sequences of sign-and-digits and of literals; the
        // former are presumed to convey the numeric part of an offset, the latter
        // are literals that must match verbatim. If !textPattern.hasDigits then
        // only a plain UTC/GMT zone-indicator can be hoped for.
        const NumericPattern textPattern(text, digitLocale);
        Q_ASSERT(!textPattern.pattern.isEmpty());
        PatternAligner aligner(text, textPattern.pattern, flags);

        // Updates best if it finds a better match.
        // Returns true if candidate uses digitLocale's digits.
        const auto consider = [&best, &aligner, txtSign = textPattern.sign, digitLocale]
            (QStringView candidate, char sign, QTimeZone::TimeType season) {
            const auto idForOffset = [sign](int offsetSeconds) -> QByteArray {
                if (!offsetSeconds)
                    return "UTC";
                if (sign == '-')
                    offsetSeconds = -offsetSeconds;
                return QTimeZonePrivate::isoOffsetFormat(offsetSeconds,
                                                         QTimeZone::OffsetName).toLatin1();
            };
            const auto localeDataFor = [loc = digitLocale] (const NumericPattern &pat) {
                if (pat.hasDigits) {
                    if (pat.digitsAreLocale)
                        return QLocalePrivate::get(loc)->m_data;
                    return QLocaleData::c();
                }
                return static_cast<const QLocaleData *>(nullptr);
            };
            if (const NumericPattern pat(candidate, digitLocale);
                // Try to match if text has the expected sign, or if candidate doesn't:
                (pat.sign & sign) != sign || (txtSign & sign) == sign) {
                const auto parsed = aligner.match(
                    candidate, pat.pattern, localeDataFor(pat), pat.sign);
                if (parsed && parsed.length > best.nameLength)
                    best = { idForOffset(parsed.offset), parsed.length, season };
                return pat.digitsAreLocale;
            }
            return false;
        };

        bool nativeSeen = false;
        for (auto season : seasons) {
            if (!acceptSeason(season))
                continue;
            for (auto format : formats) {
                if (!acceptFormat(format))
                    continue;
                if (const QString pos = positive.displayName(season, format, locale);
                    pos.size() > best.nameLength) {
                    if (consider(pos, '+', season))
                        nativeSeen = true;
                }
                if (const QString neg = negative.displayName(season, format, locale);
                    neg.size() > best.nameLength) {
                    if (consider(neg, '-', season))
                        nativeSeen = true;
                }
                if (const QString nul = greenwich.displayName(season, format, locale);
                    nul.size() > best.nameLength) {
                    if (text.startsWith(nul))
                        best = { "UTC"_ba, nul.size() };
                }
                if (best.nameLength == text.size()) // Shortcut when fully matched.
                    return best;
            }
        }

        // A locale might use (or our backend might, for it, use) localized text
        // combined with ASCII digits for its offset format.
        if (i == 0 && (digitLocale.zeroDigit() == u'0' || !nativeSeen))
            break;
        digitLocale = QLocale::c();
    }
    return best;
}

} // unnamed namespace

QTimeZonePrivate::NamePrefixMatch
QTimeZonePrivate::findOffsetPrefix(QStringView text, const QLocale &locale,
                                   QtTemporalPattern::TemporalFieldFlags flags)
{
    NamePrefixMatch best;
    if (auto match = findOffsetPrefixImpl(text, locale, flags))
        best = std::move(match);
    if (auto match = findOffsetPrefixImpl(text, QLocale::c(), flags);
        match.nameLength > best.nameLength) {
        best = std::move(match);
    }
    return best;
}

#endif // BACKEND_PROVIDES_OFFSET_PREFIX

QTimeZonePrivate::NamePrefixMatch
QTimeZonePrivate::findLongUtcPrefix(QStringView text)
{
    if (text.startsWith(u"UTC")) {
        if (text.size() > 4 && (text[3] == u'+' || text[3] == u'-')) {
            // Compare QUtcTimeZonePrivate::offsetFromUtcString()
            const auto digitAt = [text](qsizetype index) {
                using QtMiscUtils::isAsciiDigit;
                return index < text.size() && isAsciiDigit(text[index].unicode());
            };
            qsizetype length = 3;
            int groups = 0; // Number of groups of digits seen (allow up to three).
            do {
                // text[length] is sign or the colon after last digit-group.
                Q_ASSERT(length < text.size());
                if (!digitAt(length + 1) || (groups && !digitAt(length + 2)))
                    break;
                length += digitAt(length + 2) ? 3 : 2;
            } while (++groups < 3 && length < text.size() && text[length] == u':');
            if (length > 4)
                return { text.first(length).toLatin1(), length, QTimeZone::GenericTime };
        }
        return { utcQByteArray(), 3, QTimeZone::GenericTime };
    }

    return {};
}

QByteArrayView QTimeZonePrivate::aliasToIana(QByteArrayView alias)
{
    const auto data = std::lower_bound(std::begin(aliasMappingTable), std::end(aliasMappingTable),
                                       alias, earlierAliasId);
    if (data != std::end(aliasMappingTable) && data->aliasId() == alias)
        return data->ianaId();
    // Note: empty return means not an alias, which is true of an ID that others
    // are aliases to, as the table omits self-alias entries. We could return
    // alias, but we only want to return non-empty if it *was* an alias.
    return {};
}

QByteArrayView QTimeZonePrivate::ianaIdToWindowsId(QByteArrayView id)
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
    return {};
}

QByteArrayView QTimeZonePrivate::windowsIdToDefaultIanaId(QByteArrayView windowsId)
{
    const auto data = std::lower_bound(std::begin(windowsDataTable), std::end(windowsDataTable),
                                       windowsId, earlierWindowsId);
    if (data != std::end(windowsDataTable) && data->windowsId() == windowsId) {
        QByteArrayView id = data->ianaId();
        Q_ASSERT(id.indexOf(' ') == -1);
        return id;
    }
    return {};
}

QByteArrayView QTimeZonePrivate::windowsIdToDefaultIanaId(QByteArrayView windowsId,
                                                          QLocale::Territory territory)
{
    // Must match windowsIdToIanaIds(), but returning its first entry (or empty)
    if (territory == QLocale::World) {
        // World data are in windowsDataTable, not zoneDataTable.
        return windowsIdToDefaultIanaId(windowsId);
    }

    const quint16 windowsIdKey = toWindowsIdKey(windowsId);
    const qint16 land = static_cast<quint16>(territory);
    for (auto data = zoneStartForWindowsId(windowsIdKey);
         data != std::end(zoneDataTable) && data->windowsIdKey == windowsIdKey;
         ++data) {
        // Return the first (preferred) region match:
        if (data->territory == land)
            return *data->ids().begin();
    }

    return {};
}

QList<QByteArray> QTimeZonePrivate::windowsIdToIanaIds(QByteArrayView windowsId)
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

QList<QByteArray> QTimeZonePrivate::windowsIdToIanaIds(QByteArrayView windowsId,
                                                       QLocale::Territory territory)
{
    // Must match windowsIdToDefaultIanaId(), but collecting all candidates.
    QList<QByteArray> list;
    if (territory == QLocale::World) {
        // World data are in windowsDataTable, not zoneDataTable.
        list << windowsIdToDefaultIanaId(windowsId).toByteArray();
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
    // Don't accept other matches; QTZ's constructor falls back to its own check
    // using offsetFromUtcString() if all else fails.
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
        if (offset.size() > 2 || (prior && offset.size() < 2))
            return invalidSeconds(); // Field too long or too short
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
        name = isoOffsetFormat(offsetSeconds, QTimeZone::OffsetName);
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
    // Officially only supports C locale names; these are surely also viable for en-Latn-*.
    return isAnglicLocale(locale);
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
#if QT_CONFIG(timezone_locale)
    QString name =
#  if QT_CONFIG(icu)
        // ICU doesn't recognize m_name in "UTC±HH:mm" form as an ID - so that
        // localeName() only does the offset format, making it useless here (and
        // it's always expensive). It does, however, cope with plain UTC, so
        // skip except in that case:
        m_offsetFromUtc != 0 ? QString() :
#  endif
        QTimeZonePrivate::displayName(timeType, nameType, locale);

    // That may fall back to standard offset format, in which case we'd sooner
    // use m_name if it's non-empty (for the benefit of custom zones).
    // However, a localized fallback is better than ignoring the locale, so only
    // consider the fallback a match if it matches modulo reading GMT as UTC,
    // U+2212 as MINUS SIGN and the narrow form of offset the fallback uses.
    const auto matchesFallback = [](int offset, QStringView name) {
        // Fallback rounds offset to nearest minute:
        int seconds = offset % 60;
        int rounded = offset
            + (seconds > 30 || (seconds == 30 && (offset / 60) % 2)
               ? 60 - seconds // Round up to next minute
               : (seconds < -30 || (seconds == -30 && (offset / 60) % 2)
                  ? -(60 + seconds) // Round down to previous minute
                  : -seconds));
        const QString avoid = isoOffsetFormat(rounded);
        if (name == avoid)
            return true;
        Q_ASSERT(avoid.startsWith("UTC"_L1));
        Q_ASSERT(avoid.size() == 9);
        // Fallback may use GMT in place of UTC, but always has sign plus at
        // least one hour digit, even for +0:
        if (!(name.startsWith("GMT"_L1) || name.startsWith("UTC"_L1)) || name.size() < 5)
            return false;
        // Fallback drops trailing ":00" minute:
        QStringView tail{avoid}; // TODO: deal with sign earlier !  Also: invisible Unicode !
        tail = tail.sliced(3);
        if (name.sliced(3) == tail)
            return true;
        while (tail.endsWith(":00"_L1))
            tail = tail.chopped(3);
        while (name.endsWith(":00"_L1))
            name = name.chopped(3);
        if (name == tail)
            return true;
        // Accept U+2212 as minus sign:
        const QChar sign = name[3] == u'\u2212' ? u'-' : name[3];
        // Fallback doesn't zero-pad hour:
        return sign == tail[0] && tail.sliced(tail[1] == u'0' ? 2 : 1) == name.sliced(4);
    };
    if (!name.isEmpty() && (m_name.isEmpty() || !matchesFallback(m_offsetFromUtc, name)))
        return name;
#else // No L10N :-(
    Q_UNUSED(timeType);
    Q_UNUSED(locale);
#endif
    if (nameType == QTimeZone::ShortName)
        return m_abbreviation;
    if (nameType == QTimeZone::OffsetName)
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
#ifdef Q_OS_WASM
    const emscripten::val date = emscripten::val::global("Date").new_();
    if (date.isUndefined())
        return utcQByteArray();
    // JavaScript's getTimezoneOffset() returns minutes west of UTC.
    // Qt expects seconds east of UTC, so we negate and convert to seconds.
    const int offsetSeconds = -date.call<int>("getTimezoneOffset") * 60;
    if (offsetSeconds == 0)
        return utcQByteArray();
    return isoOffsetFormat(offsetSeconds).toUtf8();
#else
    return utcQByteArray();
#endif
}

bool QUtcTimeZonePrivate::isTimeZoneIdAvailable(QByteArrayView ianaId) const
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
