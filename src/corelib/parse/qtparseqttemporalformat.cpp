// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser
#include "private/qtparseqttemporalformat_p.h"

#include "private/qlocale_p.h"
#include "private/qstringiterator_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QtParseQtTemporalFormat {

inline constexpr char timeFormats[] = "Hhmsz"; // Omits [aA][pP]? deliberately.
inline constexpr char dateFormats[] = "Mdy";

ParsedDateTimeFormat prefix(QStringView pattern, QtTemporalPattern::DateTimeParts form)
{
    using namespace QtTemporalPattern;

    ParsedDateTimeFormat result;
    constexpr char32_t Invalid = ~char32_t(0);
    static_assert(Invalid > QChar::LastValidCodePoint);
    const bool includeDate = form.testFlag(DateTimePart::Date);
    const bool includeTime = form.testFlag(DateTimePart::Time);
    const bool includeZone = form.testFlag(DateTimePart::Zone);

    QStringIterator iter(pattern);
    char32_t pending = 0;
    const auto countRepeats = [&pending, &iter, &result](char32_t first, qsizetype bound) {
        // Consumes all repeats of \a first, returns min(bound, number of repeats).
        Q_ASSERT(!QChar::requiresSurrogates(first)); // It's always an ASCII format char
        Q_ASSERT(pending == 0);
        qsizetype count = 1; // We've already seen first
        result.endIndex = iter.index(); // ... and tacitly consumed it.
        while (iter.hasNext() && count < bound) {
            const auto read = iter.next(Invalid);
            if (read > QChar::LastValidCodePoint) {
                pending = Invalid;
                break;
            }
            if (read != first) {
                pending = read;
                break;
            }
            ++count;
            result.endIndex = iter.index();
        }
        return count;
    };

    constexpr char32_t SingleQuote = U'\'';
    static constexpr auto matchTimeFormats = QtPrivate::makeCharacterSetMatch<timeFormats>();
    static constexpr auto matchDateFormats = QtPrivate::makeCharacterSetMatch<dateFormats>();
    const auto isFormatChar = [includeDate, includeTime, includeZone](char32_t ch) {
        if (ch >= 0x80)
            return false;
        if (includeTime) {
            if (matchTimeFormats.matches(uchar(ch)) || ch == U'A' || ch == U'a')
                return true;
        }
        if (includeZone && ch == U't')
            return true;
        return includeDate && matchDateFormats.matches(uchar(ch));
    };

    constexpr auto formatCategory = [](uchar ch, int count) {
        using Cat = TemporalFieldCategory;
        switch (ch) {
        case 'A': // case 'P':
        case 'a': // case 'p':
            return Cat::PeriodInDay;
        case 'd': return count < 3 ? Cat::DayOfMonth : Cat::DayOfWeek;
        case 'H': return Cat::Hour;
        case 'h': return Cat::HourMod12;
        case 'M': return Cat::Month;
        case 'm': return Cat::Minute;
        case 's': return Cat::Second;
        case 't': return Cat::TimeZone;
        case 'y': return count < 3 ? Cat::YearWithinCentury : Cat::Year;
        case 'z': return Cat::SecondFraction;
        }
        // Should only be called with a ch that would pass an isFormatChar() check.
        Q_UNREACHABLE_RETURN(Cat::Literal);
    };
    constexpr auto formatFlags = [](uchar ch, int count) -> TemporalFieldFlags {
        using F = TemporalFieldFlag;
        constexpr TemporalFieldFlags TextCommon = F::IgnoreCase;
        switch (ch) {
        case 'A': // case 'P':
            return count ? F::UpperCase | TextCommon : TextCommon;
        case 'a': // case 'p':
            return count ? F::LowerCase | TextCommon : TextCommon;
        case 'd': case 'M': // Day and Month share a pattern:
            switch (count) {
            case 2: return F::Numeric | F::ZeroPad;
            case 3: return F::Verbal | F::Abbreviated | TextCommon;
            default:
                return count < 2 ? F::Numeric : F::Verbal | F::Wide | TextCommon;
            };
            Q_UNREACHABLE();
            break;
        case 'H': case 'h': case 'm': case 's': // Shared pattern:
            return count > 1 ? F::Numeric | F::ZeroPad : F::Numeric;
        case 't':
            switch (count) {
            case 1: // 't': matches everything, serializes as abbreviation
                return F::AllowZSuffix | F::LocalTimeName;
                // The next two forms aren't localized - should they be ?
                // The issue is that they're documented to differ in whether
                // separators are used, but we don't control that (nor should
                // we, or the format author) for localized forms.
            case 2: // 'tt': offset (no-prefix, no separator)
                return F::Iso8601 | F::Numeric | F::ZeroPad;
            case 3: // 'ttt': offset (no-prefix, separator)
                return F::Iso8601 | F::Verbal | F::ZeroPad;
            default: // 'tttt': long name, IANA ID or LocalTime name
                return F::LocalizedZone | F::Verbal | F::Standalone | F::Wide | F::Short
                     | F::LocalTimeName;
                // This includes both metazone and exemplar city versions of long name.
            }
            Q_UNREACHABLE();
            break;
        case 'y':
            if (count > 2)
                return F::Numeric | F::ZeroPad | F::YearSignIso8601;
            return F::Numeric | F::ZeroPad;
        case 'z':
            if (count > 2)
                return F::Numeric | F::ZeroPad;
            return F::Numeric;
        }
        // Should only be called by branches that passed an isFormatChar() check.
        Q_UNREACHABLE_RETURN({});
    };

    const auto store = [&result](QString &&literal, qsizetype count,
                                 TemporalFieldFlags flags,
                                 TemporalFieldCategory category) {
        result.fields.append(TemporalField{std::move(literal), count, flags, category});
    };

    bool seenDayPeriod = false, seenHourMod12 = false; // See post-processing.
    while (pending <= QChar::LastValidCodePoint && (pending || iter.hasNext())) {
        char32_t ch;
        if (pending) {
            ch = std::exchange(pending, 0);
        } else {
            result.endIndex = iter.index();
            ch = iter.next(Invalid);
        }
        if (ch > QChar::LastValidCodePoint)
            break;

        if (ch < 0x80 && includeTime) {
            if (matchTimeFormats.matches(uchar(ch))) {
                qsizetype count = countRepeats(ch, ch == U'z' ? 3 : 2);
                if (ch == U'z' && count == 2) // Backwards compatibility
                    count = 1; // (but we still consume both 'z' characters from the format)
                store(QString(), count, formatFlags(uchar(ch), count),
                      formatCategory(uchar(ch), count));
                if (ch == U'h')
                    seenHourMod12 = true;
                continue;
            }
            if (ch == U'A' || ch == U'a') {
                // Follow old QDTP (for now, at least) in using count to represent case choice.
                qsizetype count = ch == U'a' ? 1 : 2;
                // AP or ap are just the same as A or a; but Ap or aP selects
                // locale-appropriate case:
                result.endIndex = iter.index();
                const auto read = iter.hasNext() ? iter.next(Invalid) : Invalid;
                if (read > QChar::LastValidCodePoint) {
                    pending = Invalid;
                } else if (read == U'P') {
                    if (ch == U'a')
                        count = 0;
                    result.endIndex = iter.index();
                } else if (read == U'p') {
                    if (ch == U'A')
                        count = 0;
                    result.endIndex = iter.index();
                } else {
                    pending = read;
                }
                store(QString(), count, formatFlags(uchar(ch), count),
                      formatCategory(uchar(ch), count));
                seenDayPeriod = true;
                continue;
            }
        }
        // Date and Zone fields are more straightforward, except for 'y':
        if (ch == U'y' && includeDate) {
            // For 'y', a pair is a year-within-century, double that for a full
            // year; beyond that, evenly many more are more of those but an odd
            // 'y' is a literal. We thus need to only consume 2 or 4 'y' tokens,
            // so can't use countRepeat() with its simple maximum. We need to
            // leave the odd 'y', if present, for a later iteration to consume
            // or, if it's all there is, for use as a literal - in which case we
            // mustn't have set pending. Fortunately 'y' is ASCII so we don't
            // have to worry about surrogates:
            qsizetype count = 1;
            QStringView tail = pattern.sliced(iter.index() - 1);
            if (tail.size() > 4)
                tail = tail.first(4);
            while (count < tail.size() && char32_t(tail[count].unicode()) == ch)
                ++count;
            if (count == 3)
                --count;
            if (count > 1) {
                Q_ASSERT(count == 2 || count == 4);
                // Advance iter over what we've accepted:
                iter.setPosition(iter.position() - 1 + count);
                store(QString(), count, formatFlags(uchar(ch), count),
                      formatCategory(uchar(ch), count));
                result.endIndex = iter.index();
                continue;
            }
            // else: fall through to treat the lone 'y' as a literal.
        } else if (ch < 0x80 && ((includeDate && matchDateFormats.matches(uchar(ch)))
                                 || (includeZone && ch == U't'))) {
            qsizetype count = countRepeats(ch, 4);
            store(QString(), count, formatFlags(uchar(ch), count),
                  formatCategory(uchar(ch), count));
            continue;
        }
        Q_ASSERT(pending == 0); // Everything that might set it has continue;d

        // Not a field indicator, so parse as a literal:
        QString literal;
        QString quote; // If non-null: unfinished quote, to be appended to literal when closed.
        if (ch == SingleQuote) { // Defer it to first iteration of loop below.
            pending = ch;
        } else {
            literal = QString(QStringView(QChar::fromUcs4(ch)));
            result.endIndex = iter.index();
        }
        while (pending <= QChar::LastValidCodePoint && (pending || iter.hasNext())) {
            if (pending) {
                ch = std::exchange(pending, 0);
            } else {
                if (quote.isNull()) // i.e. we're not in an incomplete quote
                    result.endIndex = iter.index();
                ch = iter.next(Invalid);
            }
            if (ch > QChar::LastValidCodePoint)
                break;

            if (ch == SingleQuote) {
                if (quote.isNull()) { // Provisionally start a quote
                    quote = u""_s; // empty is not null
                } else {
                    // Even if this is the first quote of a pair, denoting a
                    // single quote within the quote, there's a valid parse that
                    // ends at it, adding the quote-so-far to literal.
                    literal += quote;
                    result.endIndex = iter.index();
                    quote = QString(); // Set back to null
                }
                ch = iter.hasNext() ? iter.next(Invalid) : Invalid;
                if (ch == SingleQuote) {
                    // Paired single quote denotes a single quote:
                    if (quote.isNull()) {
                        // The quote we thought was ending actually continues:
                        // the continuation starts with a literal single quote.
                        quote = u"'"_s;
                    } else {
                        // Our provisionally-started quote was actually the
                        // first half of an pair of quotes not inside others.
                        Q_ASSERT(quote.isEmpty()); // We just set it.
                        quote = QString();
                        literal.append(u'\'');
                        result.endIndex = iter.index();
                    }
                    continue;
                }

                if (ch > QChar::LastValidCodePoint) {
                    pending = ch;
                    break;
                }
            }

            Q_ASSERT(ch != SingleQuote);
            if (quote.isNull() && isFormatChar(ch)) {
                pending = ch;
                break;
            }
            if (ch <= QChar::LastValidCodePoint) {
                if (quote.isNull()) {
                    result.endIndex = iter.index();
                    literal.append(QStringView(QChar::fromUcs4(ch)));
                } else {
                    quote.append(QStringView(QChar::fromUcs4(ch)));
                }
            }
        }
        // Even if we truncated due to an unclosed quote, we have a literal to
        // include in the prefix we can parse as a pattern:
        if (!literal.isEmpty())
            store(std::move(literal), 0, {}, TemporalFieldCategory::Literal);

        // If we're in an unclosed quote, we cleared pending or marked it invalid:
        Q_ASSERT(quote.isNull() || !pending || pending > QChar::LastValidCodePoint);
    }

    // Post-process to deal with a quirk of the legacy format: if there's no
    // AM/PM field, then 'h' format is read as 'H' format.
    if (seenHourMod12 && !seenDayPeriod) {
        for (TemporalField &field : result.fields) {
            if (field.category == TemporalFieldCategory::HourMod12)
                field.category = TemporalFieldCategory::Hour;
        }
    }

    return result;
}

} // QtParseQtTemporalFormat

QT_END_NAMESPACE
