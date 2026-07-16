// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser
#include "private/qtparsetimezone_p.h"

#include "qdatetime.h"
#include "qlocale.h"
#include "private/qlocale_p.h"
#include <QtCore/qloggingcategory.h>
#include "qstring.h"
#include "private/qtenvironmentvariables_p.h" // for tzName()
#include "qtimezone.h"
#if QT_CONFIG(timezone)
#  include "private/qtimezoneprivate_p.h"
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {

QList<QtParseTimeZone::ParsedZone>
addMatch(QList<QtParseTimeZone::ParsedZone> &&matches,
         QtParseTimeZone::ParsedZone &&match, [[maybe_unused]] bool gmtStart)
{
    // Input matches is sorted with x before y when isBetter(x, y); add our new
    // entry just after the last that isBetter(than, it).
    using namespace QtParseTimeZone;

    // How discerning isBetter() can be depends on whether zones can have backends.
    const auto isBetter = [
#if QT_CONFIG(timezone)
        // GMT may be recognized as various other things, but if named as such
        // and supported by our backend, prefer it over others (of the same
        // length) that aren't, with the exception of LocalTime:
        newIsBackendGmt = gmtStart && match.size() == 3
            && match.zone.timeSpec() == Qt::TimeZone && match.zone.id() == "GMT",
#endif
        newAddr = &match] (const ParsedZone &left, const ParsedZone &right) {
        Q_ASSERT(left.startIndex == right.startIndex);
        if (left.endIndex > right.endIndex)
            return true;
        if (left.endIndex < right.endIndex)
            return false;
        // For historical reasons (e.g. QTBUG-114575) we prefer local time over
        // other ways of referring to the same zone:
        if (left.zone.timeSpec() == Qt::LocalTime && right.zone.timeSpec() != Qt::LocalTime)
            return true;
        if (left.zone.timeSpec() != Qt::LocalTime && right.zone.timeSpec() == Qt::LocalTime)
            return false;
#if QT_CONFIG(timezone)
        if (newIsBackendGmt) // The following is true exactly when left is the same as match:
            return right.zone.timeSpec() != Qt::TimeZone || right.zone.id() != "GMT";
#endif
        return &right == newAddr;
    };
    const auto pos = std::upper_bound(matches.begin(), matches.end(), match, isBetter);
    // Could condition the following on match not being a duplicate of pos[-1],
    // for pos != begin(), but hopefully we simply aren't sending duplicates
    // this way, anyway.
    matches.insert(pos, match);
    return std::move(matches);
}

#if QT_CONFIG(timezone)
constexpr char zoneNamePunctuation[] = "+-./:_";

QDateTimePrivate::DaylightStatus timeTypeToStatus(QTimeZone::TimeType type) {
    using QDTP = QDateTimePrivate;
    switch (type) {
    case QTimeZone::GenericTime: return QDTP::UnknownDaylightTime;
    case QTimeZone::StandardTime: return QDTP::StandardTime;
    case QTimeZone::DaylightTime: return QDTP::DaylightTime;
    }
    Q_UNREACHABLE_RETURN(QDTP::UnknownDaylightTime);
}

auto matchIanaId(QStringView text)
{
    struct R {
        QTimeZone zone;
        qsizetype length = 0;
        operator bool() const noexcept { return length > 0; }
    };
    // Collect up plausibly-valid characters; let QTimeZone work out what's
    // truly valid.
    const auto invalidZoneNameCharacter = [] (const QChar &c) {
        static constexpr auto matcher = QtPrivate::makeCharacterSetMatch<zoneNamePunctuation>();
        const auto cu = c.unicode();
        return cu >= 127u || !(matcher.matches(uchar(cu)) || c.isLetterOrNumber());
    };
    int index = std::distance(text.cbegin(),
                              std::find_if(text.cbegin(), text.cend(), invalidZoneNameCharacter));
    if (!index)
        return R{};
    Q_ASSERT(index <= text.size());
    text.truncate(index);

    // Limit name fragments (between slashes) to 20 characters.
    // (Valid time-zone IDs are allowed up to 14 and Android has quirks up to 17.)
    // Limit number of fragments to six; no known zone name has more than four.
    int lastSlash = -1;
    int count = 0;
    while (lastSlash < index) {
        const int newToken = lastSlash + 1;
        int slash = text.indexOf(u'/', newToken);
        if (slash < 0)
            slash = index; // i.e. the end of the candidate text
        else if (++count > 5)
            index = slash; // Truncate
        if (slash - newToken > 20)
            index = newToken + 20; // Truncate
        // If any of those conditions was met, index <= slash, so this exits the loop:
        lastSlash = slash;
    }
    // Only ASCII characters aren't invalid, so we can now convert to Latin1.
    QByteArray name = text.first(index).toLatin1();
    // Subsequent truncation won't trigger reallocation, so is efficient despite
    // the owning container.

    // IANA includes a limited few three-letter abbreviations as IDs.
    // Find longest IANA ID match:
    for (; index >= 3; name.truncate(--index)) {
        QTimeZone zone(name);
        if (zone.isValid())
            return R{zone, index};
    }

    // Not a known IANA ID.
    return R{};
}
#endif // feature timezone

auto matchSystemName(QStringView text, const QLocale &locale)
{
    using QDTP = QDateTimePrivate;
    struct R {
        qsizetype length = 0;
        QDTP::DaylightStatus season = QDTP::UnknownDaylightTime;
        operator bool() const noexcept { return length > 0; }
    } best;
    qTzSet();
    // On MS-Win, at least when system zone is UTC, qTzName() can return empty.
    for (int i = 0; i < 2; ++i) {
        const QString zone(qTzName(i));
        if (zone.size() > best.length && text.startsWith(zone))
            best = { zone.size(), i ? QDTP::DaylightTime : QDTP::StandardTime };
    }
#if QT_CONFIG(timezone)
    // Mimic each candidate QLocale::toString() could have used, to ensure round-trips work:
    const auto consider = [text, &best](QStringView zone, QDTP::DaylightStatus season) {
        if (text.startsWith(zone)) {
            // UTC-based zone's displayName() only includes minutes if non-zero:
            constexpr qsizetype utcSignHourWidth = 6, withMinutesWidth = 9;
            if (withMinutesWidth > best.length && zone.size() == utcSignHourWidth
                    && zone.startsWith("UTC"_L1)
                    && text.sliced(utcSignHourWidth).startsWith(":00"_L1)) {
                best = { withMinutesWidth, QDTP::UnknownDaylightTime };
            } else if (zone.size() > best.length) {
                best = { zone.size(), season };
            }
        }
    };
    /* QLocale::toString would skip this if locale == QLocale::system(), but we
       might not be using the same system locale as whoever generated the text
       we're parsing. So consider it anyway. */
    if (const QTimeZone sys = QTimeZone::systemTimeZone(); sys.hasDaylightTime()) {
        constexpr QTimeZone::TimeType types[] = {
            QTimeZone::GenericTime, QTimeZone::StandardTime, QTimeZone::DaylightTime };
        for (const auto timeType : types) {
            consider(sys.displayName(timeType, QTimeZone::ShortName, locale),
                     timeTypeToStatus(timeType));
        }
    } else {
        consider(sys.displayName(QTimeZone::GenericTime, QTimeZone::ShortName, locale),
                 QDTP::UnknownDaylightTime);
    }
#else
    Q_UNUSED(locale);
#endif
    return best;
}

struct SizeOffset {
    qsizetype length = 0;
    int secondsEast = 0;
    constexpr SizeOffset(qsizetype size, int offset) : length(size), secondsEast(offset) {}
};

// Locale-independent ISO 8601 offset forms
QList<SizeOffset> matchIso8601(QStringView text, QtTemporalPattern::TemporalFieldFlags flags)
{
    constexpr int MaxOffsetHours
        = (std::max)(-QTimeZone::MinUtcOffsetSecs, QTimeZone::MaxUtcOffsetSecs) / 3600;
    QList<SizeOffset> matches;
    using namespace QtTemporalPattern;
    using namespace FieldGroup;
    using Flag = TemporalFieldFlag;

    if (flags.testFlag(Flag::AllowZSuffix) && text.startsWith(QLatin1Char('Z'))) {
        matches.emplace_back(1, 0);
        // No other ISO 8601 offset form starts with Z.
        return matches;
    }

    qsizetype used = 0;
    QStringView tail = text; // Invariant: is a prefix of text.sliced(used)
    if (tail.startsWith(u"UTC")) {
        if (!matchesFlagWithin(flags, Flag::AcceptUtcPrefix, UtcPrefixMask))
            return matches;
        used += 3;
        tail = tail.sliced(3);
    } else if (!matchesFlagWithin(flags, Flag::NeedNoUtcPrefix, UtcPrefixMask)) {
        return matches;
    }
    const bool negate = tail.startsWith(u'-');
    if (!negate && !tail.startsWith(u'+'))
        return matches;
    ++used;
    tail = tail.sliced(1);

    const auto extend = [&matches, negate](qsizetype length, int secondsEast) {
        if (negate)
            secondsEast = -secondsEast;
        if (secondsEast >= QTimeZone::MinUtcOffsetSecs
            && secondsEast <= QTimeZone::MaxUtcOffsetSecs) {
            matches.emplace_back(length, secondsEast);
        }
    };

    int hours = 0, minutes = 0, seconds = 0;
    const bool zeroPad = flags.testFlag(Flag::ZeroPad);
    constexpr TemporalFieldFlags WithColon = Flag::Verbal | Flag::Standalone;
    qsizetype colon = tail.indexOf(u':');
    if (colon == 0) // No digits in (first field of) offset.
        return matches;

    if (!matchesFlagsWithin(flags, WithColon, FormMask)) { // Colon forbidden.
        if (colon > 0) {
            // Treat as juxtaposed fields with cruft starting at the colon:
            tail = tail.first(colon);
            colon = -1;
        }
    } else if (!matchesFlagWithin(flags, Flag::Numeric, FormMask)) { // Colon required
        if (colon > 2) {
            // Too long for a single field. Treat as hour field followed by trailing
            // cruft, since our colon is too late to separate it from a later field.
            tail = tail.first(2);
            colon = -1; // There is no longer a colon in tail.
        } else if (colon < 0) {
            // Lack of expected colon - we have, at most, an hour field:
            if (tail.size() > 2)
                tail = tail.first(2);
        }
    } // else: if a colon is there, read fields up to it.
    // If we have a colon at the end of the hour field, each field must end in a
    // colon. No field is wider than two digits, so a colon further out than
    // that isn't the end of the hour field, just part of some dangling cruft.
    const bool hasColon = colon > 0 && colon <= 2;
    bool ok;
    qsizetype fieldUsed = qMin(2, hasColon ? colon : tail.size());
    hours = tail.first(fieldUsed).toInt(&ok);
    if (!ok || hours > MaxOffsetHours || (zeroPad && fieldUsed < 2)) {
        if (zeroPad) // Hour field must have full width.
            return matches;
        hours = tail.first(1).toInt(&ok);
        fieldUsed = 1;
        // Single-digit hour is only allowed in colon-separated form; if we
        // don't have an actual colon, the parse must end after this field.
        if (!ok)
            return matches;
    }
    tail = tail.sliced(fieldUsed);
    used += fieldUsed;

    qsizetype fieldEnd[3] = { used, 0, 0 };
    int fieldsSeen = 1; // Seen hour field
    // If we're allowed more than just hour, see what we've got:
    if ((flags & WidthMask) != QtTemporalPattern::TemporalFieldFlags{Flag::Narrow}) {
        for (int i = 0; i < 2 && fieldUsed && !tail.isEmpty(); ++i) {
            QStringView digits = tail;
            qsizetype sepLen = 0;
            if (hasColon || fieldUsed == 1) {
                if (fieldUsed != colon)
                    break;
                Q_ASSERT(tail.startsWith(u':'));
                digits = digits.sliced(1);
                sepLen = 1;
            }
            int &field = i ? seconds : minutes;
            colon = hasColon && !i ? digits.indexOf(u':') : -1;
            if (colon == 0) // Empty field
                break;
            if ((colon == -1 ? digits.size() : colon) < 2) // Not enough digits for field.
                break;
            field = digits.first(2).toInt(&ok);
            if (!ok)
                break;
            fieldUsed = 2; // So next iteration sees that to compare to colon.
            tail = tail.sliced(sepLen + fieldUsed);
            used += sepLen + fieldUsed;
            fieldEnd[fieldsSeen++] = used;
            // Quit loop after 1st iteration unless accepting seconds field:
            if (!i && !matchesFlagsWithin(flags, Flag::Wide | Flag::Short, WidthMask))
                break;
        }
    }
    // Check we got enough fields, add entries, with longer matches earlier:
    switch (fieldsSeen) {
    case 3: // Would have exited loop early unless:
        Q_ASSERT(matchesFlagsWithin(flags, Flag::Wide | Flag::Short, WidthMask));
        // TODO: if Wide, check for fractional part.
        extend(fieldEnd[--fieldsSeen], (hours * 60 + minutes) * 60 + seconds);
        Q_FALLTHROUGH();
    case 2: // Hour and minute supplied.
        if (!zeroPad || matchesFlagWithin(flags, Flag::Abbreviated, WidthMask))
            extend(fieldEnd[fieldsSeen - 1], (hours * 60 + minutes) * 60);
        --fieldsSeen;
        Q_FALLTHROUGH();
    case 1: // Only hour supplied: need Narrow if ZeroPad:
        if (zeroPad && !matchesFlagWithin(flags, Flag::Narrow, WidthMask))
            break;
        extend(fieldEnd[--fieldsSeen], hours * 60 * 60);
    }
    return matches;
}

}

namespace QtParseTimeZone {

/*!
    \internal
    \since 6.12
    \namespace QtParseTimeZone
    \brief A toolset for parsing time zone identification strings

    A time zone may be identified by an offset from UTC or, in various ways, by
    a name. This namespace provides a \l {QtParseTimeZone::}{prefix()} function
    to parse an initial portion of a string as such an identifier, controlled by
    configuration options provided by \l
    {QtTemporalPattern::TemporalFieldFlags}, along with several combinations of
    those options that select particular commonly-used choices.

    The constants are of type \l {QtTemporalPattern::TemporalFieldFlags}:
    \list

      \li AnyOffsetForm Enables all offset options.
      \li BasicDigitOnlyOffset The Qt 'tt' offset format: HH or HHmm, no
          separator between the hour and minute fields, no UTC or GMT prefix,
          just the sequence of digits.
      \li BasicColonDigitOffset The Qt 'ttt' offset format: HH or HH:mm, fields
          within the offset are separated by colons, there is no UTC or GMT
          prefix.
      \li AnyZoneName The Qt 'tttt' format: the IANA ID or localized long name
          of the zone.
      \li AllLegacyForm The Qt 't' format: any zone representation supported up
          to Qt 6.10.
      \li AnyZoneForm Enables all options.

    \endlist
*/
// TODO: this is not, currently, quite true. The colon distinction is a myth.

/*!
    \internal
    \since 6.12
    \class QtParseTimeZone::ParsedZone
    \brief Describes a text fragment representing a timezone.

    Returned by functions that parse a timezone representation from a text. Its
    member variables are:
    \list

      \li zone A timezone representing the result of parsing
      \li timeType A \l QDateTimePrivate::DaylightStatus indicating the form in
                   which the zone is described by its representation
      \li startIndex Parsed text offset of the start of the text matched
      \li endIndex Parsed text offset of the end of the text matched

    \endlist

    The portion of the text that matched stretches from \c startIndex to \c
    endIndex and can be obtained by passing the same text to \c used(). This
    shall be empty if \c isEmpty() is \c true.

    The \c zone describes the timezone matched. If \c isEmpty() is \c true, \c
    zone shall be a lightweight time representation for local time, since a
    timestamp with no specified zone is conventionally understood to be in local
    time (although whose local time may be unclear). If this leaves a tail of
    the text parsed that is otherwise not recognized, it may mean that the text
    was malformed, or represented a timezone not recognized by the parser. If
    the portion of the text matched takes a locale-appropriate form for a fixed
    offset from UTC, \c zone shall be a lightweight time representation for UTC,
    if the offset is zero, or for the specified offset from UTC. Otherwise, the
    text matched identified a specific timezone (this only happens if feature \c
    timezone is enabled) and \c zone is a timezone backed by system data.
*/

/*!
    \internal
    \since 6.12
    Parses an initial portion of \a text as a timezone, as described by \a locale

    The acceptable forms of a timezone text are controlled by \a flags.
*/
QList<ParsedZone> prefix(QStringView text, const QLocale &locale, qsizetype from,
                         QtTemporalPattern::TemporalFieldFlags flags)
{
    using QDTP = QDateTimePrivate;
    QList<ParsedZone> matches;
    if (from < 0 || from >= text.size())
        return matches;

    QStringView tail = text.sliced(from);
    const auto includeMatch = [&matches, from, gmtStart = tail.startsWith(u"GMT")]
        (qsizetype used, QTimeZone &&zone, QDTP::DaylightStatus type) {
        Q_ASSERT(zone.isValid());
        matches = addMatch(std::move(matches), {{from, from + used}, zone, type}, gmtStart);
    };

    using namespace QtTemporalPattern;
    using namespace FieldGroup;
    using Flag = TemporalFieldFlag;

    if (matchesFlagWithin(flags, Flag::Iso8601, FieldGroup::LocalizationMask)) {
        // Locale-independent offset forms:
        const auto matches = matchIso8601(tail, flags);
        for (const auto &match : matches) {
            includeMatch(match.length,
                         QTimeZone::fromSecondsAheadOfUtc(match.secondsEast),
                         QDTP::UnknownDaylightTime);
        }
    }

    // Locale-dependent forms:
#if QT_CONFIG(timezone)
    if (matchesFlagWithin(flags, Flag::LocalizedZone, FieldGroup::LocalizationMask)) {
        const auto addPrefixIfMatch = [includeMatch] (QTimeZonePrivate::NamePrefixMatch &&prefix) {
            if (prefix) {
                includeMatch(prefix.nameLength, QTimeZone(prefix.ianaId),
                             timeTypeToStatus(prefix.timeType));
            }
        };
        bool checkOffsetFallbacks = false;

        if (matchesFlagWithin(flags, Flag::Numeric, FormMask)
            && matchesFlagsWithin(flags, Flag::Wide | Flag::Short, WidthMask)) {
            // TODO: have findOffsetPrefix() return a list:
            addPrefixIfMatch(QTimeZonePrivate::findOffsetPrefix(tail, locale, flags));
            checkOffsetFallbacks = true; // Might cover some corner cases differently:
        }

        // IANA after offset-as-such because we prefer offset from UTC
        // representations over more complex backend representations:
        if (matchesFlagWithin(flags, Flag::Standalone, FormMask)
            && matchesFlagWithin(flags, Flag::Short, WidthMask)) {
            if (auto match = matchIanaId(tail))
                includeMatch(match.length, std::move(match.zone), QDTP::UnknownDaylightTime);
        }
        // ... but before long name, even though that may match some offset forms,
        // but it only does that as a fall-back, so the IANA choice is better in
        // that case.

        if (matchesFlagWithin(flags, Flag::Verbal, FormMask)
            && matchesFlagsWithin(flags, Flag::Wide | Flag::Short, WidthMask)) {
            // TODO: findLongNamePrefix() would prefer to be first tried with a date-time.
            addPrefixIfMatch(QTimeZonePrivate::findLongNamePrefix(tail, locale));
            // (We don't want offset format to match 'tttt', so do need to limit this.)
            // The final fall-back for QTZL's localeName() is a
            // zoneOffsetFormat(,, Numeric | Abbreviated | NeedNoUtcPrefix | ZeroPad ,,):
            checkOffsetFallbacks = true;
        }

        if (checkOffsetFallbacks) {
            addPrefixIfMatch(QTimeZonePrivate::findNarrowOffsetPrefix(tail, locale));
            addPrefixIfMatch(QTimeZonePrivate::findLongUtcPrefix(tail));
        }
    }
#endif

    if (flags.testFlag(Flag::LocalTimeName)) {
        if (const auto sys = matchSystemName(tail, locale))
            includeMatch(sys.length, QTimeZone(QTimeZone::LocalTime), sys.season);
    }

    return matches;
}

// ParsedZone find(QStringView text, const QLocale &locale,
//                 QtTemporalPattern::TemporalFieldFlags flags, qsizetype from) { }
} // QtParseTimeZone

QT_END_NAMESPACE
