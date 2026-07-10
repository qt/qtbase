// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser
#include "private/qtparsetemporal_p.h"

#include "private/qcalendarmath_p.h"
#include "private/qlocale_p.h"
#include "private/qstringiterator_p.h"
#include "private/qttemporalpattern_p.h"

#include <algorithm> // sort, stable_sort
#include <optional>
#include <utility> // exchange, move, pair
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
    static constexpr int UnknownAmHour = -12, UnknownPmHour = 36;
    enum Flaw : quint16 {
        // Flaws that justify prefering a shorter parse without the flaw over
        // longer with it, in order of decreasing severity:
        Irreconcilable = 1, // field values cannot be reconciled
        // Ideally continuations() would catch irreconcilable issues, but if one
        // is expensive to spot it can be left for resolve() to flag up.
        LegacyResolves = 2, // field values can only be resolved by ignoring timeType
        ResolutionChanges = 4, // resolved values don't match parsed values
        ZeroPad = 8, // used zero-padded part of text where field didn't require it
        Narrow = 0x10, // used fewer digits from text than field width, where allowed

        // Flaws not worth giving up a longer parse over, in decreasing order of
        // strength of preference among those of equal length:
        SelfResolved = 0x100, // ambiguous field values resolve cleanly
    };
    Q_DECLARE_FLAGS(Flaws, Flaw)
    Flaws wanton = {};

    // Constructor for initial empty parse:
    PartialParse(qsizetype from) { results.startIndex = results.endIndex = from; }
    // Constructors extending a parse with something more:
    PartialParse(const PartialParse &base, const QtParseCommon::ParsedText &more)
        : PartialParse(base)
    {
        Q_ASSERT(results.endIndex == more.startIndex);
        results.endIndex = more.endIndex;
    }
    PartialParse(const PartialParse &base, const QtParseTimeZone::ParsedZone &more)
        : PartialParse(base, (const QtParseCommon::ParsedText &)more)
    {
        results.zone = more.zone;
        results.timeType = more.timeType;
    }

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

        // In decreasing order of severity
        if (auto res = order(Flaw::LegacyResolves); res != 0)
            return res;
        if (auto res = order(Flaw::ResolutionChanges); res != 0)
            return res;
        if (auto res = order(Flaw::Narrow); res != 0)
            return res;

        // The above take precedence over size: a shorter parse without those
        // flaws is better than a longer one with them.

        // Longer is better, to be understood as "sorts before" i.e. less than.
        if (auto res = Qt::compareThreeWay(alt.results.size(), results.size()); res != 0)
            return res;

        // The following remains as a preference only among matches of the same
        // length: it's nice to avoid, but a longer parse is still better.

        if (auto res = order(Flaw::ZeroPad); res != 0)
            return res;
        if (auto res = order(Flaw::SelfResolved); res != 0)
            return res;

        return Qt::weak_ordering::equivalent;
    }
};
Q_DECLARE_OPERATORS_FOR_FLAGS(PartialParse::Flaws)

QLocaleData::DigitSequence
parseDigitSequence(QStringView text, qsizetype from, const QLocale &locale, bool allowSign)
{
    const auto *const data = QLocalePrivate::get(locale)->m_data;
    using DS = QLocaleData::DigitSequence;
    DS::Options flags;
    if (allowSign)
        flags.setFlag(DS::Option::AllowSign, true);
    return data->digitSequence(text, flags, from);
}

std::vector<PartialParse> spacePadExtend(std::vector<PartialParse> matched, QStringView text)
{
    // Pass the whole text: the results.endIndex of the last entry in matched is
    // an index into it that we shall use to add entries that extend that entry.
    Q_ASSERT(!matched.empty());
    // Assumes matched.back()'s last field is allowed to end in space-padding
    // and inserts a partial parse resulting from accepting each subsequent
    // space as extending the match. Each longer match is inserted before all
    // shorter matches. Only extensions of matched.back() are added, so call
    // after adding each entry to matched, if adding several.
    const qsizetype position = matched.size() - 1;
    PartialParse copy = matched.back();
    QStringIterator iter(text, copy.results.endIndex);
    while (iter.hasNext() && QChar::isSpace(iter.next())) {
        Q_ASSERT(iter.index() > copy.results.endIndex);
        copy.results.endIndex = iter.index();
        matched.insert(matched.begin() + position, copy);
    }
    return matched;
}

QtParseCommon::ParsedText matchesAt(QStringView text, qsizetype from, const QString &sought,
                                    TemporalFieldFlags flags)
{
    using F = TemporalFieldFlag;
    const bool allowLeadingSpace = flags.testFlag(F::SpacePad);
    Q_ASSERT(sought.size() > 0);
    // Note: returns the first match within text[from:]. If sought is all space
    // and SpacePad is set, there may be later matches if text[from:] starts
    // with more space than (possibly some non-matching spaces, then) that.
    // caller is expected to follow the match implied by the return from this
    // with more generated by spacePadExtend().

    const auto beginLength = [flex = flags.testFlag(F::FlexSpace)]
        (QStringView view, QStringView target, Qt::CaseSensitivity cs = Qt::CaseSensitive) {
        // Technical hitch: case-insensitive comparison may match a string of
        // different length. Roll a brute-force length-determining version:
        const auto matchFront = [cs](QStringView view, QStringView target) {
            if (view.startsWith(target, cs)) {
                qsizetype length = target.size();
                while (view.first(length - 1).startsWith(target, cs))
                    --length;
                while (!view.first(length).startsWith(target, cs))
                    ++length;
                Q_ASSERT(length > 0);
                return length;
            }
            return qsizetype(-1);
        };
        const auto spaceForward = [](QStringIterator &iter) {
            // Steps iter past next non-space, returns index at which it appeared.
            qsizetype used;
            do {
                used = iter.index();
            } while (iter.hasNext() && QChar::isSpace(iter.next()));
            return used;
        };
        constexpr qsizetype failed = 0;
        qsizetype matched = 0;
        if (flex) {
            QStringIterator iter(target);
            while (iter.hasNext()) {
                qsizetype head = iter.index();
                if (QChar::isSpace(iter.next())) {
                    qsizetype same = head > 0 ? matchFront(view, target.first(head)) : 0;
                    if (same < 0)
                        return failed;
                    QStringIterator viter(view, same);
                    // Require at least one spacing character in view to match those in target:
                    if (!viter.hasNext() || !QChar::isSpace(viter.next()))
                        return failed;
                    same = spaceForward(viter);
                    matched += same;
                    view = view.sliced(same);
                    target = target.sliced(spaceForward(iter));
                    iter = QStringIterator(target);
                }
            }
        }
        const qsizetype tail = target.isEmpty() ? 0 : matchFront(view, target);
        if (tail < 0)
            return failed;
        return matched + tail;
    };
    // TODO: consider a comparison that ignores Unicode invisibles, like BiDi
    // markers, when matching.
    qsizetype offset = 0;
    do {
        QStringView view = text.sliced(from + offset);
        if (flags.testFlag(F::IgnoreCase)) {
            if (qsizetype match = beginLength(view, sought, Qt::CaseInsensitive))
                return {from, from + offset + match};
        } else if (flags.testAnyFlags(F::LowerCase | F::UpperCase)) {
            // If either case is specified, only match specified cases.
            // If both cases are specified, accept either (but not mixed).
            if (flags.testFlag(F::LowerCase)) {
                if (qsizetype match = beginLength(view, sought.toLower()))
                    return {from, from + offset + match};
            }
            if (flags.testFlag(F::UpperCase)) {
                if (qsizetype match = beginLength(view, sought.toUpper()))
                    return {from, from + offset + match};
            }
            // Otherwise, only an exact match is accepted:
        } else if (qsizetype match = beginLength(view, sought)) {
            return {from, from + offset + match};
        }

        // No match at this position; maybe later if leading space is allowed:
        if (!allowLeadingSpace) {
            Q_ASSERT(!offset);
            break;
        }

        // Consume one space at a time until we find a match:
        QStringIterator iter(text.sliced(from), offset);
        if (!iter.hasNext() || !QChar::isSpace(iter.next()))
            break;

        Q_ASSERT(iter.index() > offset);
        offset = iter.index();
    } while (text.size() >= offset + sought.size() / 2);
    // Loop wants to test text.size() >= offset + sought.size(), but see beginLength().
    return {};
}

bool longerEarlier(const PartialParse &left, const PartialParse &right)
{
    // True if we want left before right in our sorted lists.
    // We want longer matches before shorter:
    return left.results.endIndex > right.results.endIndex;
}

template <typename Action>
void forEachLocaleFormat(TemporalFieldFlags flags, Action action)
{
    using Flag = TemporalFieldFlag;
    constexpr auto Widths = FieldGroup::WidthMask;
    if (matchesFlagWithin(flags, Flag::Wide, Widths))
        action(QLocale::LongFormat);
    if (matchesFlagsWithin(flags, Flag::Short | Flag::Abbreviated, Widths))
        action(QLocale::ShortFormat);
    if (matchesFlagWithin(flags, Flag::Narrow, Widths))
        action(QLocale::NarrowFormat);
}

class TemporalFieldMatcher
{
    const QLocale locale;
    const QCalendar calendar;
    const std::optional<int> baseYear;

    // Numeric
    struct FieldConfig
    {
        // Where to write the int, once read:
        int &(*target)(PartialParse &);
        // Acceptable values:
        int maxValue = 0; // 0 means unbounded
        int unset; // Default value in ParsedTemporal, invalid for field.
        // Form of the parsed text:
        qsizetype width; // min digits
        qsizetype maxDigits = 0; // <= 0 means unbounded
        // If unbounded, beyond max(width, roundAfter, -maxDigits) prefer fewer digits to more.
        qsizetype roundAfter = -1; // >= 0: is fractional part: round to this many digits
        bool allowSign = false;
    };
    // For use as FieldConfig::target:
    static int &millisTarget(PartialParse &grow) { return grow.results.millis; }
    static int &secondTarget(PartialParse &grow) { return grow.results.second; }
    static int &minuteTarget(PartialParse &grow) { return grow.results.minute; }
    static int &hourTarget(PartialParse &grow) { return grow.results.hour; }
    static int &hourMod12Target(PartialParse &grow) { return grow.hourMod12; }
    static int &dayOfWeekTarget(PartialParse &grow) { return grow.results.dayOfWeek; }
    static int &dayOfMonthTarget(PartialParse &grow) { return grow.results.dayOfMonth; }
    static int &monthTarget(PartialParse &grow) { return grow.results.month; }
    static int &yearTarget(PartialParse &grow)
    {
        if (!grow.results.year)
            grow.results.year = 0;
        return *grow.results.year;
    }
    static int &yearWithinCenturyTarget(PartialParse &grow) { return grow.yearWithinCentury; }

    std::vector<PartialParse>
    numericExtend(const PartialParse &base, QStringView text,
                  TemporalFieldFlags flags, FieldConfig &&config) const;

    // Verbal, Standalone:
    std::vector<PartialParse> monthNameExtend(const PartialParse &base, QStringView text,
                                              TemporalFieldFlags flags) const;
    std::vector<PartialParse> dayNameExtend(const PartialParse &base, QStringView text,
                                            TemporalFieldFlags flags) const;
    std::pair<qsizetype, int> dayPeriodPrefix(const PartialParse &base, QStringView text,
                                              TemporalFieldFlags flags) const;
public:
    TemporalFieldMatcher(const QLocale &loc, QCalendar cal, std::optional<int> centuryStart)
        : locale(loc), calendar(cal), baseYear(centuryStart)
    {}

    std::vector<PartialParse> continuations(const PartialParse &base, QStringView text,
                                            const TemporalField &field) const;
    bool isSelfConsistent(const PartialParse &parsed, TemporalFieldCategory category) const;
    bool resolve(PartialParse &parsed) const;
};

bool TemporalFieldMatcher::isSelfConsistent(const PartialParse &parse,
                                            TemporalFieldCategory category) const
{
    // Take into account calendar, and potentially baseYear, but only do cheap
    // checks. This will be run on *each* candidate parse after *each* field,
    // need not check conditions the current field could not have affected.
    using Cat = TemporalFieldCategory;
    if (category == Cat::Literal) // Can't have introduced any inconsistency.
        return true;

    const bool newYear = category == Cat::Year || category == Cat::YearWithinCentury;
    if (newYear && parse.yearWithinCentury >= 0 && parse.results.year
        && (*parse.results.year - parse.yearWithinCentury) % 100) {
        return false;
    }

    const bool newDate = (newYear || category == Cat::Month || category == Cat::DayOfMonth
                          || category == Cat::DayOfWeek);
    if (newDate && parse.results.month && parse.results.dayOfMonth) {
        // Calendrical calculations: somewhat expensive, but still arithmetic.
        if (parse.results.year) {
            if (!calendar.isDateValid(*parse.results.year, parse.results.month,
                                      parse.results.dayOfMonth)) {
                return false;
            }
            if (parse.results.dayOfWeek) {
                QDate date = calendar.dateFromParts(*parse.results.year, parse.results.month,
                                                    parse.results.dayOfMonth);
                if (calendar.dayOfWeek(date) != parse.results.dayOfWeek)
                    return false;
            }
        } else if (calendar.daysInMonth(parse.results.month) < parse.results.dayOfMonth) {
            return false;
        }
    }

    if ((category == Cat::PeriodInDay && parse.results.hour >= 0)
        || (category == Cat::Hour && parse.periodInDay >= 0)) {
        // 00, 01, ... 11 are 12, 1, ... 11 am; 12, 13, ... 23 are 12, 1, ..., 11 pm.
        if (parse.periodInDay ? parse.results.hour < 12 : parse.results.hour >= 12)
            return false;
    }

    if ((category == Cat::Hour && parse.hourMod12 > 0)
        || (category == Cat::HourMod12 && parse.results.hour >= 0)) {
        if ((parse.results.hour - parse.hourMod12) % 12)
            return false;
    }
    return true;
}

bool TemporalFieldMatcher::resolve(PartialParse &parse) const
{
    // Final pass, modifying parsed as needed, true if parse.result has been
    // given a value consistent with all fields of parse. Applies fully rigorous
    // checks, given what isSelfConsistent() already checked. May record flaws
    // in parse.wanton where relevant tests reveal them.
    if (parse.yearWithinCentury >= 0) {
        if (parse.results.year) {
            // Previously checked by isSelfConsistent():
            Q_ASSERT((*parse.results.year - parse.yearWithinCentury) % 100 == 0);
        } else if (baseYear) {
            const auto baseSplit =QRoundingDown::qDivMod<100>(*baseYear);
            int year = baseSplit.quotient * 100 + parse.yearWithinCentury;
            if (parse.yearWithinCentury < baseSplit.remainder)
                year += 100;

            if (parse.results.month) {
                // Check the year has this month and, if given, enough days in
                // it for dayOfMonth:
                const auto enough = [dom = parse.results.dayOfMonth](int dim) {
                    return dim > 0  && (!dom || dom <= dim);
                };
                if (!enough(calendar.daysInMonth(parse.results.month, year))) {
                    // Search outwards for a better century:
                    bool fixed = false;
                    for (int off = 1; off < 10; ++off) {
                        int offset = off * 100;
                        if (enough(calendar.daysInMonth(parse.results.month, year + offset))) {
                            year += offset;
                            fixed = true;
                            break;
                        }
                        if (enough(calendar.daysInMonth(parse.results.month, year - offset))) {
                            year -= offset;
                            fixed = true;
                            break;
                        }
                    }
                    // No century within a millennium each way will do:
                    if (!fixed)
                        return false;
                }

                if (parse.results.dayOfMonth) {
                    if (parse.results.dayOfWeek) {
                        QCalendar::YearMonthDay ymd
                            = { year, parse.results.month, parse.results.dayOfMonth };
                        const QDate resolved
                            = calendar.matchCenturyToWeekday(ymd, parse.results.dayOfWeek);
                        if (!resolved.isValid())
                            return false;
                        year = resolved.year(calendar);
                    } else {
                        const QDate resolved(year, parse.results.month, parse.results.dayOfMonth);
                        if (!resolved.isValid())
                            return false;
                    }
                }
            }

            parse.results.year = year;
        }
    }

    if (parse.results.hour < 0 && parse.hourMod12 > 0) {
        Q_ASSERT(parse.hourMod12 <= 12);
        parse.results.hour = parse.hourMod12 < 12 || parse.periodInDay < 0 ? parse.hourMod12 : 0;
        if (parse.periodInDay > 0)
            parse.results.hour += 12;
    }

    if (parse.results.year && parse.results.month && parse.results.dayOfMonth
        && parse.results.zone.isValid() && parse.results.hour >= 0) {
        // Should be able to construct a datetime with this:
        const QDate date(*parse.results.year, parse.results.month, parse.results.dayOfMonth,
                         calendar);
        Q_ASSERT(date.isValid()); // Should be ensured by earlier checks.
        const QTime time = parse.results.time(QTime());
        Q_ASSERT(time.isValid()); // Should be ensured by earlier checks.

        // Is the given time in a transition of the given zone, on the given date ?
        if (!Q_LIKELY(QDateTime(date, time, parse.results.zone,
                                QDateTime::TransitionResolution::Reject).isValid())) {
            // Ambiguity, gap or outright borkage.
            using Flaw = PartialParse::Flaw;
            QDateTime dt(date, time, parse.results.zone, parse.results.resolveType());
            if (!dt.isValid()) {
                // Fall back to default resolution (same as LegacyBehavior):
                dt = QDateTime(date, time, parse.results.zone);
                // If that succeeded, Abbreviated (bad); otherwise Narrow (worse).
                parse.wanton |= dt.isValid() ? Flaw::LegacyResolves : Flaw::Irreconcilable;
            }
            if (dt.date() != date || dt.time() != time
                || dt.timeRepresentation() != parse.results.zone) {
                // OK, resolution *worked* but didn't get exactly what we asked
                // for (presumably a spring-forward's gap):
                parse.wanton |= Flaw::ResolutionChanges;
                // ... but we don't change parse.results because they should
                // reflect what parsing learned; the caller can rediscover this.
            } else {
                // We got what we asked for (presumably the expected branch of a
                // fall-back):
                parse.wanton |= Flaw::SelfResolved;
            }
        }
    }

    if (parse.results.hour < 0) {
        // Leave ParsedTemporal::time() a clue to am/pm, if known:
        if (parse.periodInDay > 0)
            parse.results.hour = PartialParse::UnknownPmHour;
        else if (parse.periodInDay == 0)
            parse.results.hour = PartialParse::UnknownAmHour;
    }
    return true;
}

std::vector<PartialParse>
TemporalFieldMatcher::numericExtend(const PartialParse &base, QStringView text,
                                    TemporalFieldFlags flags, FieldConfig &&config) const
{
    std::vector<PartialParse> matches;

    using Flag = TemporalFieldFlag;
    qsizetype leadingSpace = 0;
    const bool spacePad = flags.testFlag(Flag::SpacePad);
    if (spacePad) {
        QStringIterator iter(text, base.results.endIndex);
        while (iter.hasNext() && QChar::isSpace(iter.next()))
            ++leadingSpace;
        // If that's used up the string, the code below shall reject the field.
    }

    const auto parsed = parseDigitSequence(text, base.results.endIndex + leadingSpace,
                                           locale, config.allowSign);
    const bool zeroPad = flags.testFlag(Flag::ZeroPad);
    // If !zeroPad, we allow < config.width but flag with Narrow in wanton fields.
    const int width = zeroPad || spacePad ? qMax(1, config.width - leadingSpace) : 1;
    // This is necessarily positive: the use of chop(1) below depends on that.

    QByteArrayView digits{parsed.digits};
    // Parsed field must be representable in an int, so don't try to read more
    // digits than an int can hold (digits10 is how many 9s in a row an int can
    // hold; but int can hold some sequences one digit longer than that):
    constexpr int intMaxDigits = std::numeric_limits<int>::digits10 + 1;
    if (digits.size() > intMaxDigits)
        digits = digits.first(intMaxDigits);
    // Take config and flags into account, too:
    if (config.maxDigits > 0) {
        // Allow config.width to override config.maxDigits:
        const int maxWidth = qMax(config.maxDigits, config.width);
        if (digits.size() > maxWidth)
            digits = digits.first(maxWidth);
    } else if (flags.testFlag(Flag::YearSignIso8601) && !parsed.sign) {
        // Limit width because a field longer than width would need a sign.
        const int maxWidth = config.width > 0 ? config.width : qMax(-config.maxDigits, 1);
        if (digits.size() > maxWidth)
            digits = digits.first(maxWidth);
    }
    // For unbounded, work out in advance when to switch from prepending to
    // appending; otherwise, set a cut-off that'll be true already.
    const qsizetype appendThreshold = config.maxDigits <= 0
        ? qMax(-config.maxDigits, qMax(config.width, config.roundAfter)) - 1
        : digits.size();

    for (; digits.size() >= width; digits.chop(1)) {
        bool ok = false;
        unsigned whole = digits.toUInt(&ok);
        if (!ok)
            continue;
        if (config.maxValue > 0 && config.roundAfter < 0 && whole > unsigned(config.maxValue))
            continue;

        // If calendar has a year zero, we need to allow 0 in (full) year fields (width >= 4).
        bool forbidZero = config.unset == 0 && (config.width < 4 || !calendar.hasYearZero());
        auto optvalue = [whole, forbidZero,
                         negate = parsed.sign == '-']() -> std::optional<int> {
            constexpr unsigned maxInt = std::numeric_limits<int>::max();
            if (negate && whole == 1 + maxInt)
                return std::numeric_limits<int>::min();
            if (whole > maxInt || (forbidZero && !whole))
                return {};
            return negate ? -int(whole) : int(whole);
        }();
        if (!optvalue) // Overflow or too low
            continue;
        int value = *optvalue;

        if (config.roundAfter >= 0) {
            // Fractional part
            if (digits.size() < config.roundAfter) {
                // Interpolate omitted zero-padding up to rounding size:
                for (int i = int(digits.size()); i < config.roundAfter; ++i)
                    value *= 10;
            } else if (digits.size() > config.roundAfter) {
                double v = value;
                for (int i = int(digits.size()); i > config.roundAfter; --i)
                    v /= 10.;
                // A timestamp that's before the end of a specified second
                // should be rounded to the last we can before that second,
                // especially if it's the last second of its minute, in turn
                // especially if that's the last second of its hour (and so on).
                value = v > config.maxValue ? config.maxValue : qRound(v);
                // There may of course be use-cases where rounding up to the
                // next second is desired. If it turns out those are
                // significant, we can perhaps add a field option for it.
            }
            // else: exact match to number of digits, nothing to frob.
        }

        PartialParse grow = base;
        int &target = config.target(grow);
        if (target <= config.unset) // If unset, store:
            target = value;
        else if (target != value) // Conflicts with earlier field: skip this reading.
            continue;
        grow.results.endIndex = parsed.digitStart + digits.size() * parsed.digitWidth;

        if (!zeroPad && digits.size() > qMax(1, config.width)
            && (config.roundAfter < 0 ? digits.startsWith('0') : digits.endsWith('0'))) {
            grow.wanton |= PartialParse::Flaw::ZeroPad;
        }
        if (digits.size() + leadingSpace < config.width) // (can only happen if !zeroPad)
            grow.wanton |= PartialParse::Flaw::Narrow;

        // Entries in matches are all longer than this one, as we're reducing
        // digits. Mostly we want shorter after longer, but (for example) we
        // prefer 4-digit years over longer matches.
        if (digits.size() > appendThreshold)
            matches.insert(matches.begin(), std::move(grow));
        else
            matches.push_back(std::move(grow));
    }
    return matches;
}

/* Some month names may be prefixes of others.
   For example, the English long forms of Islamic calendar month names include:
   * Rabiʻ I, Rabiʻ II
   * Jumada I, Jumada II
   Their short-forms are likewise:
   * Rab. I, Rab. II
   * Jum. I, Jum. II
   In each case, one month name is a prefix of the next month's name.

   In any sane format, greedy parsing shall suffice but ill-considered formats
   happen. So the initial parse recognizes every possible match and we sort out
   any mistakes greed might make as we parse later fields.
*/
std::vector<PartialParse>
TemporalFieldMatcher::monthNameExtend(const PartialParse &base, QStringView text,
                                      TemporalFieldFlags flags) const
{
    std::vector<PartialParse> matches;
    using Flag = TemporalFieldFlag;

    auto addIfMatch = [&matches, base, text, flags](int month, QString &&name) {
        // tryEachMonth() has ensured this:
        Q_ASSERT(!base.results.month || base.results.month == month);
        if (name.isEmpty()) // Locale doesn't know this month's name.
            return;
        // If matchesAt(), add to matches:
        auto match = matchesAt(text, base.results.endIndex, name, flags);
        if (match) {
            matches.emplace_back(base, match).results.month = month;
            if (flags.testFlag(Flag::SpacePad))
                matches = spacePadExtend(std::move(matches), text);
        }
    };

    constexpr auto Forms = FieldGroup::FormMask;
    constexpr int noYear = QCalendar::Unspecified;
    const bool verb = matchesFlagWithin(flags, Flag::Verbal, Forms);
    const bool lone = matchesFlagWithin(flags, Flag::Standalone, Forms);
    const int year = base.results.year ? *base.results.year : noYear;
    // We could try to take account of baseYear, when yearWithinCentury is
    // known, but that's susceptible to tweaks and perturbation from other
    // fields, so stick with noYear and the usual naming of months if we don't
    // know year. We can consider adding a QCalendar::parseMonthName() that can
    // consult the internal lists of localized month names, both for efficiency
    // and to ensure we try all names, including those that appear only in some
    // years. If we do that, its return should package month number, whether the
    // month appears in all years and whether it was standalone or plain, along
    // with the start and end indices of the match within the text.
    auto tryEachNameType = [this, verb, lone, year,
                            addIfMatch](QLocale::FormatType form, int month) {
        if (lone)
            addIfMatch(month, calendar.standaloneMonthName(locale, month, year, form));
        if (verb)
            addIfMatch(month, calendar.monthName(locale, month, year, form));
    };
    // This could in principle, for non-system locales, be done more efficiently
    // by walking the internal ';'-joined list of month names QCalendarBackend
    // can give us. The entanglement between QCalendarBackend and QLocale
    // internals is, however, already quite untidy enough, so leave that for
    // if/when we discover it's a significant bottle-neck and/or we've unpicked
    // the existing entanglement a bit first.

    auto tryEachMonth = [month = base.results.month, bound = calendar.maximumMonthsInYear(),
                         tryEachNameType](QLocale::FormatType form) {
        if (month > 0) {
            tryEachNameType(form, month);
        } else {
            for (int i = bound; i > 0; --i)
                tryEachNameType(form, i);
        }
    };
    forEachLocaleFormat(flags, tryEachMonth);

    return matches;
}

std::vector<PartialParse>
TemporalFieldMatcher::dayNameExtend(const PartialParse &base, QStringView text,
                                    TemporalFieldFlags flags) const
{
    std::vector<PartialParse> matches;
    using Flag = TemporalFieldFlag;

    auto addIfMatch = [&matches, base, text, flags](int dow, QString &&name) {
        // tryEachDayOfWeek() has ensured this:
        Q_ASSERT(!base.results.dayOfWeek || base.results.dayOfWeek == dow);
        if (name.isEmpty()) // Locale doesn't know this day of the week's name.
            return;
        // If matchesAt(), add to matches:
        auto match = matchesAt(text, base.results.endIndex, name, flags);
        if (match) {
            matches.emplace_back(base, match).results.dayOfWeek = dow;
            if (flags.testFlag(Flag::SpacePad))
                matches = spacePadExtend(std::move(matches), text);
        }
    };

    constexpr auto Forms = FieldGroup::FormMask;
    const bool verb = matchesFlagWithin(flags, Flag::Verbal, Forms);
    const bool lone = matchesFlagWithin(flags, Flag::Standalone, Forms);
    auto tryEachNameType = [this, addIfMatch, verb, lone](QLocale::FormatType form, int dow) {
        if (lone)
            addIfMatch(dow, calendar.standaloneWeekDayName(locale, dow, form));
        if (verb)
            addIfMatch(dow, calendar.weekDayName(locale, dow, form));
    };
    // As for month names (see above), some collaboration with QCalendarBackend
    // might make this more efficient for non-system locales, at the expense of
    // adding to the existing tangle of complexity.

    auto tryEachDayOfWeek = [dow = base.results.dayOfWeek,
                             tryEachNameType](QLocale::FormatType form) {
        if (dow > 0) {
            tryEachNameType(form, dow);
        } else {
            // Iterate possible day numbers. Issue: some calendars might have
            // intercalary days with numbers > 7. When that happens, we may
            // need to let this run past 7 until it's seen some empty answers.
            for (int i = 1; i <= 7; ++i)
                tryEachNameType(form, i);
        }
    };
    forEachLocaleFormat(flags, tryEachDayOfWeek);

    std::sort(matches.begin(), matches.end(), longerEarlier);
    return matches;
}

std::pair<qsizetype, int>
TemporalFieldMatcher::dayPeriodPrefix(const PartialParse &base, QStringView text,
                                      TemporalFieldFlags flags) const
{
    std::pair<qsizetype, int> result = {0, -1};
    for (int i = 0; i < 2; ++i) {
        if (base.periodInDay >= 0 && base.periodInDay != i)
            continue;
        if (const QString token = i ? locale.pmText() : locale.amText(); !token.isEmpty()) {
            if (auto match = matchesAt(text, base.results.endIndex, token, flags);
                match.endIndex > result.first) {
                result = { match.endIndex, i };
            }
        }
    }
    return result;
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
TemporalFieldMatcher::continuations(const PartialParse &base, QStringView text,
                                    const TemporalField &field) const
{
    std::vector<PartialParse> matches;
    const qsizetype textPos = base.results.endIndex;
    switch (field.category) {
        using Cat = TemporalFieldCategory;
        using Flag = TemporalFieldFlag;
    case Cat::Literal:
        if (auto match = matchesAt(text, textPos, field.literal, field.options)) {
            matches.emplace_back(base, match);
            if (field.options.testFlag(Flag::SpacePad))
                matches = spacePadExtend(std::move(matches), text);
        }
        break;
    case Cat::TimeZone:
        if (const auto zones = QtParseTimeZone::prefix(text, locale, textPos, field.options);
            !zones.isEmpty()) {
            for (const auto &match : zones) {
                matches.emplace_back(base, match);
                if (field.options.testFlag(Flag::SpacePad))
                    matches = spacePadExtend(std::move(matches), text);
            }
        }
        break;

        // case Cat::MillisecondInDay: break;
    case Cat::SecondFraction:
        matches = numericExtend(base, text, field.options,
                                {millisTarget, 999, -1, field.width, 0, 3});
        break;
    case Cat::Second:
        matches = numericExtend(base, text, field.options, {secondTarget, 59, -1, field.width, 2});
        break;
        // case Cat::MinuteFraction: break;
    case Cat::Minute:
        matches = numericExtend(base, text, field.options, {minuteTarget, 59, -1, field.width, 2});
        break;
        // case Cat::HourFraction: break;
    case Cat::HourMod12:
        matches = numericExtend(base, text, field.options,
                                {hourMod12Target, 12, 0, field.width, 2});
        break;
    case Cat::Hour:
        matches = numericExtend(base, text, field.options, {hourTarget, 23, -1, field.width, 2});
        break;
    case Cat::PeriodInDay: // am/pm; LDML also has noon, midnight, "at night" and others.
        if (const auto match = dayPeriodPrefix(base, text, field.options); match.second >= 0) {
            // Ensured by dayPeriodPrefix:
            Q_ASSERT(base.periodInDay < 0 || base.periodInDay == match.second);
            PartialParse &grow = matches.emplace_back(base);
            grow.results.endIndex = match.first;
            grow.periodInDay = match.second;
            if (field.options.testFlag(Flag::SpacePad))
                matches = spacePadExtend(std::move(matches), text);
        }
        break;

    case Cat::DayOfWeek:
        matches = dayNameExtend(base, text, field.options);
        break;
    case Cat::DayOfMonth: {
        const int maxDays = calendar.maximumDaysInMonth();
        matches = numericExtend(base, text, field.options,
                                {dayOfMonthTarget, maxDays, 0, field.width,
                                 maxDays < 10 ? 1 : maxDays < 100 ? 2 : 3});
    }
        break;
        // case Cat::DayOfYear: break;
        // case Cat::JulianDay: break;
        // case Cat::WeekOfMonth: break;
        // case Cat::WeekOfYear: break;
    case Cat::Month:
        // Verbal and Standalone, in so far as supported:
        matches = monthNameExtend(base, text, field.options);
        if (matchesFlagWithin(field.options, Flag::Numeric, FieldGroup::FormMask)) {
            auto extend = numericExtend(base, text, field.options,
                                        {monthTarget, calendar.maximumMonthsInYear(),
                                         0, field.width, 2});
            if (matches.empty())
                matches = std::move(extend);
            else
                matches.insert(matches.end(), extend.begin(), extend.end());
        }
        std::sort(matches.begin(), matches.end(), longerEarlier);
        break;
        // case Cat::Quarter: break;
    case Cat::YearWithinCentury:
        matches = numericExtend(base, text, field.options,
                                {yearWithinCenturyTarget, 99, -1, field.width, 2});
        break;
    case Cat::Year:
        matches = numericExtend(base, text, field.options,
                                {yearTarget, 0, 0, field.width, -4, -1, calendar.isProleptic()});
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
        QDate draft(year ? *year : defaults.year(cal), month ? month : defaults.month(cal),
                    dayOfMonth ? dayOfMonth : defaults.day(cal), cal);

        if (dayOfWeek && draft.isValid() && draft.dayOfWeek(cal) != dayOfWeek) {
            // Defaults conflict with parsed day of the week.
            if (!dayOfMonth) {
                // (Assumes no intercalary days.)
                // Number of days to the nearest with the right day of the week:
                const int offset = (dayOfWeek + 10 - draft.dayOfWeek(cal)) % 7 - 3;
                Q_ASSERT(offset != 0); // Otherwise, day of week matched, already.
                Q_ASSERT(-4 < offset && offset < 4);
                // Prefer closer unless nearby has more in common with what we asked for:
                QDate closer = draft.addDays(offset);
                QDate nearby = draft.addDays(offset < 0 ? offset + 7 : offset - 7);
                if (nearby.isValid()
                    && (!closer.isValid()
                        || (closer.dayOfWeek(cal) != dayOfWeek
                            && nearby.dayOfWeek(cal) == dayOfWeek)
                        || (closer.month(cal) != draft.month(cal)
                            && nearby.month(cal) == draft.month(cal)))) {
                    // (We could also give year(cal) the same treatment, but
                    // different year, for dates within ten days of one another,
                    // plies different month, so check would be redundant.)
                    std::swap(nearby, draft);
                } else if (closer.isValid() && closer.dayOfWeek(cal) == dayOfWeek) {
                    std::swap(closer, draft);
                }

            } else if (!month) {
                Q_ASSERT(draft.day() == dayOfMonth);
                auto use = [&draft, cal, dow=dayOfWeek](int yr, int mon, int day) {
                    QDate maybe(yr, mon, day, cal);
                    if (!maybe.isValid() || maybe.dayOfWeek(cal) != dow)
                        return false;
                    std::swap(maybe, draft);
                    return true;
                };
                // Find nearest month with the right dayOfMonth and dayOfWeek.
                // If year was specified we're limited to it; otherwise,
                // draft.year() is derived from defaults so the search can
                // spread to nearby years.
                int loYear = draft.year(cal), hiYear = loYear;
                int loMon = draft.month(cal), hiMon = loMon;
                bool maybeLo = true, maybeHi = true;
                while (maybeLo || maybeHi) {
                    if (maybeHi) {
                        if (hiMon < cal.monthsInYear(hiYear)) {
                            ++hiMon;
                        } else if (year) {
                            Q_ASSERT(hiYear == *year);
                            maybeHi = false;
                        } else if (hiYear + 1 || cal.hasYearZero()) {
                            ++hiYear;
                            hiMon = 1;
                        } else if (cal.isProleptic()) {
                            hiYear = +1;
                            hiMon = 1;
                        } else {
                            maybeHi = false;
                        }
                    }
                    if (maybeHi && use(hiYear, hiMon, dayOfMonth))
                        break;

                    if (maybeLo) {
                        if (loMon > 1) {
                            --loMon;
                        } else if (year) {
                            Q_ASSERT(loYear == *year);
                            maybeLo = false;
                        } else if (loYear - 1 || cal.hasYearZero()) {
                            --loYear;
                            loMon = cal.monthsInYear(loYear);
                        } else if (cal.isProleptic()) {
                            loYear = -1;
                            loMon = cal.monthsInYear(loYear);
                        } else {
                            maybeLo = false;
                        }
                    }
                    if (maybeLo && use(loYear, loMon, dayOfMonth))
                        break;

                    // Avoid looping for ever: if we can't find a match within a
                    // 30 year window we probably never shall. If we haven't
                    // found a match by then, the likelihood that the input has
                    // a typo in it is fairly high, in any case.
                    if (hiYear - loYear > 30)
                        break;
                }
            } else if (!year) {
                // As for resolve()'s handling of two-digit centuries:
                QCalendar::YearMonthDay ymd = { draft.year(cal), month, dayOfMonth };
                QDate maybe = cal.matchCenturyToWeekday(ymd, dayOfWeek);
                if (maybe.isValid() && maybe.dayOfWeek(cal) == dayOfWeek)
                    std::swap(maybe, draft);
            }
            if (draft.dayOfWeek(cal) != dayOfWeek)
                return {};
        }
        return draft;
    }
    if (year && month && dayOfMonth)
        return QDate(*year, month, dayOfMonth, cal);
    return {};
}

QTime ParsedTemporal::time(QTime defaults) const
{
    if (defaults.isValid()) {
        int hr = defaults.hour();
        // hour: -1 means we have no information, less means unknown am, > 24 means unknown pm.
        if (hour < -1) // UnknownAmHour
            hr = hr % 12;
        else if (hour > 24) // UnknownPmHour
            hr = hr % 12 + 12;
        else if (hour >= 0)
            hr = hour;
        // (Note: hour == 24 is currently unused but may be relevant for 24:00:00 in future.)
        return QTime(hr,
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
    std::vector<PartialParse> maybe;
    maybe.emplace_back(from);

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

    // Although we've, thus far, prefered sensible-length matches over longer
    // ones in individual numeric fields, so that later numeric fields can take
    // up the slack and win, we still want to be greedy over-all, so prefer
    // overall longer matches to shorter ones. None the less, between matches of
    // equal length, preserve our preference, up to now, for sane lengths of
    // each field within that, as long as later fields are taking up the slack.
    // That preference can be fine-tuned via .wanton, see PartialParse::Flaw.
    PartialParse best = maybe.front();
    for (const PartialParse &match : QSpan{maybe}.sliced(1)) {
        if (match.compare(best) < 0)
            best = match;
    }
    return best.results;
}

} // QtParseTemporal

QT_END_NAMESPACE
