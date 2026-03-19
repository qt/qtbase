// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser
#include "private/qtparsetemporal_p.h"

#include "private/qttemporalpattern_p.h"

#include <optional>
#include <utility> // exchange
#include <vector>

QT_BEGIN_NAMESPACE

namespace {
using namespace QtParseTemporal;
using namespace QtTemporalPattern;

struct PartialParse
{
    ParsedTemporal results;
    int periodInDay = -1; // 0: am, 1: pm
    int hourMod12 = 0; // 1 through 12
    int yearWithinCentury = -1; // 0 through 99
    enum Flaw : quint16 {
        // Flaws that justify prefering a shorter parse without the flaw over
        // longer with it, in order of decreasing severity:
        Irreconcilable = 1, // field values cannot be reconciled
        // Ideally continuations() would catch irreconcilable issues, but if one
        // is expensive to spot it can be left for resolve() to flag up.

        // Flaws not worth giving up a longer parse over, in decreasing order of
        // strength of preference among those of equal length:
        // None, for now
    };
    Q_DECLARE_FLAGS(Flaws, Flaw)
    Flaws wanton = {};

    // Constructor for initial empty parse:
    PartialParse(qsizetype from) { results.startIndex = results.endIndex = from; }

    Qt::weak_ordering compare(const PartialParse &alt) const noexcept
    {
        // Measures of how this->wanton differs from alt.wanton: it's worse if
        // it has a flaw that alt lacks, better if the opposite. Here, "less" is
        // used to mean "this is better than alt" as we sort better entries
        // earlier in our lists of partial parse candidates.
        const auto order = [better = alt.wanton & ~wanton,
                            worse = wanton & ~alt.wanton](Flaw test) {
            Q_ASSERT(!(worse & better)); // So at most one of these testFlag()s is true:
            if (better.testFlag(test))
                return Qt::weak_ordering::less;
            if (worse.testFlag(test))
                return Qt::weak_ordering::greater;
            return Qt::weak_ordering::equivalent;
        };

        if (auto res = order(Flaw::Irreconcilable); res != 0)
            return res;

        // The above takes precedence over size: a shorter parse without those
        // flaws is better than a longer one with them.

        // Longer is better, to be understood as "sorts before" i.e. less than.
        if (auto res = Qt::compareThreeWay(alt.results.size(), results.size()); res != 0)
            return res;

        // The following remains as a preference only among matches of the same
        // length: it's nice to avoid, but a longer parse is still better.
        // None, for now.

        return Qt::weak_ordering::equivalent;
    }
};
Q_DECLARE_OPERATORS_FOR_FLAGS(PartialParse::Flaws)

class TemporalFieldMatcher
{
    const QLocale locale;
    const QCalendar calendar;
    const std::optional<int> baseYear;

public:
    TemporalFieldMatcher(const QLocale &loc, QCalendar cal, std::optional<int> centuryStart)
        : locale(loc), calendar(cal), baseYear(centuryStart)
    {}

    std::vector<PartialParse> continuations(const PartialParse &base, QStringView text,
                                            const TemporalField &field) const;
    bool isSelfConsistent(const PartialParse &parsed, TemporalFieldCategory category) const;
    bool resolve(PartialParse &parsed) const;
};

bool TemporalFieldMatcher::isSelfConsistent(const PartialParse &, TemporalFieldCategory) const
{
    // Take into account calendar, and potentially baseYear, but only do cheap
    // checks. This will be run on *each* candidate parse after *each* field,
    // need not check conditions the current field could not have affected.
    return true;
}

bool TemporalFieldMatcher::resolve(PartialParse &) const
{
    // Final pass, modifying parsed as needed, true if parse.result has been
    // given a value consistent with all fields of parse. Applies fully rigorous
    // checks, given what isSelfConsistent() already checked. May record flaws
    // in parse.wanton where relevant tests reveal them.
    return true;
}

/*!
    \internal
    Find all matches to \a field, within \a text, that extend \a base.

    Each match must begin at offset \c{base.results.endIndex} within \a text.
    For each match, update a copy of \a base with the match's result, to include
    in the returned list.

    May use \c calendar to determine the range of values allowed for field.
    Does not attempt to determine consistency between fields; see resolve() and
    isSelfConsistent() for that. Updates the copy's member holding the value
    described by \a field to reflect the match.

    Ignores base.result.startIndex and base.result.bounds and updates each
    copy's .endIndex to reflect the end of the match. (This leaves the caller to
    decide whether to transfer that to .bounds.)

    For fields that allow space padding, this consumes leading space as
    necessary to make a match and includes a match for each end position at
    which it could end; before any dangling space and after each space that
    follows. Later calls to \c continuations() shall filter out any earlier
    matches that precludes later fields matching just after its end. Successive
    space-padded fields surrounded by large amounts of space are apt to lead to
    many matches, as are final space-padded fields followed by large amounts of
    space. (TODO: we can almost certainly mitigate this with a trivial
    heuristic, once everything is working.)

    For those matches with various flaws, relative to the field specification
    (such as using zero padding when not obliged to), the copy's .wanton records
    that flaw.

    Sort order of the returned list should put entries likely to represent more
    suitable matches (ignoring .wanton complications) earlier. For most fields,
    that means longer matches come first. (For full year field matches with > 4
    digits, though, that reverses.)
*/
std::vector<PartialParse>
TemporalFieldMatcher::continuations([[maybe_unused]] const PartialParse &base,
                                    [[maybe_unused]] QStringView text,
                                    const TemporalField &field) const
{
    std::vector<PartialParse> matches;
    switch (field.category) {
        using Cat = TemporalFieldCategory;
    case Cat::Literal:
        break;
    case Cat::TimeZone:
        break;

        // case Cat::MillisecondInDay: break;
    case Cat::SecondFraction:
        break;
    case Cat::Second:
        break;
        // case Cat::MinuteFraction: break;
    case Cat::Minute:
        break;
        // case Cat::HourFraction: break;
    case Cat::HourMod12:
        break;
    case Cat::Hour:
        break;
    case Cat::PeriodInDay: // am/pm; LDML also has noon, midnight, "at night" and others.
        break;

    case Cat::DayOfWeek:
        break;
    case Cat::DayOfMonth:
        break;
        // case Cat::DayOfYear: break;
        // case Cat::JulianDay: break;
        // case Cat::WeekOfMonth: break;
        // case Cat::WeekOfYear: break;
    case Cat::Month:
        break;
        // case Cat::Quarter: break;
    case Cat::YearWithinCentury:
        break;
    case Cat::Year:
        break;
        // case Cat::RelatedGregorianYear: break;
        // case Cat::Century: break;
        // case Cat::Era: break;
    }
    return matches;
}

} // nameless namespace

namespace QtParseTemporal {
QDate ParsedTemporal::date(QCalendar cal, QDate defaults) const
{
    if (defaults.isValid()) {
        return QDate(year ? *year : defaults.year(cal), month ? month : defaults.month(cal),
                     dayOfMonth ? dayOfMonth : defaults.day(cal), cal);
    }
    if (year && month && dayOfMonth)
        return QDate(*year, month, dayOfMonth, cal);
    return {};
}

QTime ParsedTemporal::time(QTime defaults) const
{
    if (defaults.isValid()) {
        return QTime(hour < 0 ? defaults.hour() : hour,
                     minute < 0 ? defaults.minute() : minute,
                     second < 0 ? defaults.second() : second,
                     millis < 0 ? defaults.msec() : millis);
    }

    if (hour < 0 || hour > 24)
        return {};
    if (minute < 0)
        return QTime(hour, 0);
    if (second < 0)
        return QTime(hour, minute);
    if (millis < 0)
        return QTime(hour, minute, second);
    return QTime(hour, minute, second, millis);
}

ParsedTemporal prefix(QStringView text, QSpan<const QtTemporalPattern::TemporalField> fields,
                      const QLocale &locale, QCalendar cal,
                      std::optional<int> baseYear, qsizetype from)
{
    if (from < 0 || from >= text.size())
        return {};

    const TemporalFieldMatcher matcher(locale, cal, baseYear);
    // Technically this is the correct (empty) result when fields.isEmpty():
    std::vector<PartialParse> maybe{PartialParse(from)};

    qsizetype toCome = fields.size();
    for (const QtTemporalPattern::TemporalField &field : fields) {
        --toCome;
        const std::vector<PartialParse> prior = std::exchange(maybe, {});
        for (const PartialParse &base : prior) {
            std::vector<PartialParse> more
                = matcher.continuations(base, text, field);
            for (PartialParse &candidate : more) {
                // Consistency won't have been changed by a literal field:
                if ((field.category == TemporalFieldCategory::Literal
                     || matcher.isSelfConsistent(candidate, field.category))) {
                    if (toCome) // Earlier fields' ends go in bounds:
                        candidate.results.bounds.push_back(candidate.results.endIndex);
                    else if (!matcher.resolve(candidate)) // Last field: makes sense of it all.
                        continue;
                    maybe.push_back(std::move(candidate));
                }
            }
        }
        if (maybe.empty()) // No point continuing
            return {};
    }
    // Now select our most favourable entry from maybe.

    // Preference can be fine-tuned via .wanton, see PartialParse::Flaw.
    PartialParse best = maybe.front();
    for (const PartialParse &match : QSpan{maybe}.sliced(1)) {
        if (match.compare(best) < 0)
            best = match;
    }
    return best.results;
}

} // QtParseTemporal

QT_END_NAMESPACE
