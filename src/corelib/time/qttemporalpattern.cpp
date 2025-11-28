// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "private/qttemporalpattern_p.h"

#include <QtCore/qbitarray.h>

QT_BEGIN_NAMESPACE

namespace QtTemporalPattern {
/*!
    \internal
    \since 6.12
    \namespace QtTemporalPattern
    \brief Supporting types and functions for temporal patterns.

    Temporal patterns describe how a date, time or datetime may be serialized as
    text or parsed from text. This namespace provides the common tools used by
    the classes describing such patterns: \l QDateTimePattern, \l QTimePattern
    and \l QDatePattern.
*/

/* Note (QTBUG-70516): For now (Qt 6.12) it suffices to cover everything that
   the existing Qt datetime format strings are capable of. However, the intent
   is to eventually expand this to fully support CLDR's datetime formats so that
   we can switch from converting those to Qt formats when we scan CLDR to
   actually storing the CLDR format in the qlocale_data_p.h tables and
   constructing our Q(Date|Time)+Pattern objects from those formats. See comment
   in header for a link to the relevant parts of LDML.
*/

/*!
    \enum QtTemporalPattern::TemporalFieldCategory

    This enumeration characterizes the information supplied by a field.

    For the details of how that information is conveyed, see \l
    {QtTemporalPattern::}{TemporalFieldFlag}. For the classification of fields
    by whether they specify date, time or zone, see \l
    {QtTemporalPattern::}{DateTimePart}.

    \value Literal Describes a text fragment that frames the data, such as
                   separators and delimiters.
    \value TimeZone Identifies the timezone or offset from UTC of a datetime.
    \value SecondFraction The fraction of a second (an optional time field).
    \value Second The second within the minute (an optional time field).
    \value Minute The minute within the hour (a time field).
    \value PeriodInDay A subdivision of the day, such as before noon vs after
                       noon (a time field). May serve to disambiguate the hour
                       when specified only modulo 12. (Currently only the am/pm
                       distinction is supported.)
    \value HourMod12 The hour in the range 1 through 12 (a time field). May be
                     ambiguous on its own.
    \value Hour The hour in the range from 0 through 23 (a time field).

    \value DayOfWeek The day within the week (a date field). Usually specified
                     by name. Some locales may use numbers for their Narrow
                     Verbal and/or Standalone forms. No form is supported for
                     Numeric | DayOfWeek.
    \value DayOfMonth The day number within the month (a date field). Runs from
                      1 for the first day of the month up to the length of the
                      month. Always numeric, regardless of field flags.

    \value Month The month within the year (a date field). When specified
                 numerically, the first month of the year is numbered 1; there
                 is no month 0. Some locales may use numbers for their Narrow
                 Verbal form.

    \value YearWithinCentury The two-digit year number (a date field). Widely
                             used in date formats, despite its potential for
                             ambiguity. May be disambiguated by other fields or
                             by specifying base year for the hundred years
                             presumed when only two digits are given and other
                             fields don't disambiguate.
    \value Year The full year number (a date field).

    Fields identified as numbers, along with hour, minute, second and fraction
    of second, are always given in numeric form (regardless of Numeric, Verbal
    or Standalone flags).

    The SecondFraction field describes the digits immediately following the
    fractional-part separator. Leading zeros are significant and not considered
    to be padding: trailing zeros are understood as padding. QTime and QDateTime
    only handle times to millisecond precision, so if more than three digits are
    found when parsing a SecondFraction field, the excess are used only to round
    to the nearest millisecond. (If fewer than three digits are parsed, the
    value is implicitly extended on the right with zeros to obtain milliseconds
    precision. Whether such a field is accepted will depend on the setting of
    the \c width of the field and the absence of \c ZeroPad from its
    \c{options}, in the usual ways.) If more than three digits are specified
    when serializing, the digits after the first three shall all be zeros (and
    are omitted if the \c ZeroPad option is not specified).

    \note For negative years, the YearWithinCentury will be understood as the
    number of completed years since the start of the most recent year that is a
    multiple of 100. Thus year -1 (which Qt understands, when using the
    Gregorian calendar, to mean 1 BCE) has a YearWithinCentury value of 99,
    since the last year that's a multiple of 100 is -100 (for Gregorian, 100
    BCE).

    \sa {QtTemporalPattern::}{TemporalFieldFlag}, {QtTemporalPattern::}{DateTimePart}
*/

/* For numbers as narrow days of the week, at least three different numberings
   are used, all of which are used in locales whose "first day of the week" is
   Monday.  (Furthermore, some locales with digits as narrow names only do that
   for some days, with letters for others.) So there is no way to determine how
   days of the week are numbered from one locale to the next, except in so far
   as their narrow Verbal or Standalone forms use digits. A Numeric version of
   the day of the week field would thus either not know what to do for most
   locales, be inconsistent between locales or be inconsistent, for some
   locales, between Narrow Verbal / Standalone formats and Numeric format.
   Hence the lack of a meaning of Numeric for DayOfWeek.

   Some locales use month numbers for the narrow format of months; but none are
   known to do so inconsistently with the usual month numbering (although Pashto
   does this only partially, using non-numeric narrow standalone forms for the
   first two months). This does still conflict with the numeric form of months,
   at least in some cases, that use ASCII digits for narrow forms of month
   numbers but locale-appropriate digits for their numeric forms. However, the
   conflict is only in how the numbers are represented, not in the numeric
   values used for months.
*/

/*!
    \internal
    \namespace QtTemporalPattern::FieldGroup
    \brief Masks identifying mutually-exclusive families of options.

    Each constant in this namespace is of type \l {QtTemporalPattern::}
    {TemporalFieldFlags} and combines a mutually-exclusive family of \l
    {QtTemporalPattern::} {TemporalFieldFlag} values into a mask by which to
    identify that family. Within each such family, if none of the flags is
    specified, any of them may be applied but if any of them are specified then
    only one of those specified may be applied.

    \value FormMask The three general forms of a field: numeric, verbal and
                    standalone.
    \value WidthMask The four field widths: wide, short, abbreviated, and
                     narrow.
    \value UtcPrefixMask The two options for prefixes on offset forms of
                         timezones, with and without a UTC prefix.
    \value PaddingMask The two padding options for when fields don't fill the
                       width allotted to them: spaces and zeros.
    \value SeasonMask The three perspectives from which to describe a timezone:
                      the generic, regardless of the time of year, the standard,
                      and how it's described when exercising daylight-saving
                      time (if it does at all).
*/
/* Namespace QtParseCommon has helper functions, defined in qtparsetimezone_p.h,
   to apply the above rule related to these masks.
*/

/*!
    \enum QtTemporalPattern::TemporalFieldFlag

    This enumeration describes how datetime fields are expressed in text.

    A combination of these flags is used to qualify a \l {QtTemporalPattern::}
    {TemporalFieldCategory}, indicating whether it is conveyed by words or
    numbers, how tersely or in which terms, depending on the field
    category. Interpretation of a flag in this enumeration varies with the field
    category it is qualifying and with other flags combined with it.

    Some flags, such as the four width options, are mutually exclusive. For each
    group of mutually-exclusive flags, a mask constant is defined in namespace
    QtTemporalPattern::FieldGroup, of type \l {QtTemporalPattern::}
    {TemporalFieldFlags}, that combines the members of that group. If no member
    of such a group is set in a flags variable, it is treated as if all members
    of that group were set. When serializing, the behavior when two or more
    conflicting flags, in a group relevant to the field, are combined is
    unspecified. It may depend on other fields or variables passed to the
    serialization function. When parsing, such conflicting flags allow the
    parser to match any of the conflicting options specified. If this leads to
    ambiguity, parsing prefers the option that consumes more of the text to
    parse.

    Unless otherwise indicated, fields are localized. For example, ZeroPad pads
    with, and Numeric uses, locale-appropriate digits when localized, and the
    names of months, days of the week and timezones are translated into the
    appropriate language (if possible).

    The primary distinction in how fields are expressed is the form of the
    field. These first three options are mutually incompatible and are grouped
    together as the \l {QtTemporalPattern::FieldGroup::} {FormMask} constant:

    \value Numeric Express the field numerically as a series of digits.
    \value Verbal Name the field value in its usual in-format grammatical form.
    \value Standalone Name the field value in its stand-alone grammatical form.

    Next comes the width of the field. The meanings of these fields depends on
    other flags and the field category, where relevant. Some field categories
    may ignore these entirely, others may draw fewer distinctions and use the
    same meaning for some widths. These four options are mutually exclusive and
    grouped together as the \l {QtTemporalPattern::FieldGroup::} {WidthMask}
    constant:

    \value Narrow Use a the narrowest supported form for the field. In some
                  locales, the Narrow Verbal forms of some fields may use
                  numbers, potentially conflicting with one of its Numeric
                  forms.
    \value Abbreviated Use an abbreviated form of the field.
    \value Short Use a short form of the field.
    \value Wide Use a wide form of the field.

    Where a \c width is specified for a field but the field's value is naturally
    shorter, it is necessary to indicate how to pad it to the desired width.
    These two options are mutually exclusive and grouped together as the \l
    {QtTemporalPattern::FieldGroup::} {PaddingMask} constant:

    \value ZeroPad Only for numeric fields: pad to \c width with zeros. When
                   parsing, accept zeros that don't affect value, reject fields
                   that are narrower than their specified \c width.
    \value SpacePad For a field with a positive \c width, pad to that \c width
                    with spaces. When parsing, allow leading and trailing
                    spacing characters.

    If neither form of padding is indicated, the natural representation of a
    value is used even if it fails to reach the \c width specified and parsing
    will accept a narrower field (although it will prefer a match with full
    width). In a \l {QtTemporalPattern::TemporalFieldCategory} {TimeZone} field
    (see below) using an offset form, SpacePad is ignored and ZeroPad or its
    absence only has its usual meaning for hour fields, while controlling the
    presence of zero minute and second offsets following the hour field.

    Where a text to be matched (for example, a literal or the name of a month or
    day of the week) contains spaces, by default the spaces must match exactly.
    Since users commonly treat anything that looks like a space the same, it is
    usually desirable to match spaces flexibly. Where the text to be parsed is
    taken from a larger text, it's also possible that this larger text has been
    flowed, as a paragraph, which may have turned some spaces into line breaks,
    possibly with added indentation. Coping with such cases is supported by the
    option

    \value FlexSpace Where a field to be matched contains spacing characters, or
                     a run of them, any spacing character or run of them will be
                     accepted as matching. A character is deemed to be a spacing
                     character if \l QChar::isSpace() is true for it.

    The following options are only relevant to Verbal and Standalone
    fields. They are not treated as a group or descrbed by a mask, as the locale
    provides relevant fields with the appropriate (possibly mixed) case for the
    lcoale. By default, the locale's form is used when serializing and matched
    (case-sensitively) when parsing. The first two of these are mutually
    exclusive. They change that default, forcing the case when serializing and
    requiring the specified case (or one of the specified cases) when
    parsing. The third has no effect when serializing and overrides the other
    two, if either is present, when parsing.

    \value LowerCase When serializing, force lower-case.
    \value UpperCase When serializing, force upper-case.
    \value IgnoreCase When parsing, match the field case-insensitively.

    Modern revisions of ISO 8601 permit years outside the range from 0 through
    9999 but require that they have a sign. By default a + sign on a positive
    year is allowed and silently ignored (without counting towards its \c
    width), but the YearSignIso8601 applies a modified version of the ISO rule:
    the year field will only match a text with more digits than its specified \c
    width if that text starts with a sign. When the field's \c width is 4, this
    implements the ISO rule, but it can be applied to other widths, if needed.

    In contrast to ISO's use of year 0000 to indicate 1 BCE, with negative year
    values representing successively earlier years, where the calendar in use
    has no year zero, Qt describes the year before year 1 as year -1, skipping
    over 0 and treating 0 as an invalid value for the year. In particular, this
    applies to the Gregorian calendar, which is used by default: 1 CE is
    represented as year 1, with 1 BC as year -1. If the calendar in use reports
    \c true from \l {QCalendar::} {hasYearZero()}, Qt duly accepts a year 0
    between years -1 and +1. ISO also specifies that years before 1583 (the
    first full year after the Gregorian calendar came into play) or after 9999
    should not be used except by prior agreement between the producer and
    consumer of the serialized date or datetime. Qt leaves such agreement as a
    matter between the user and those they communicate with, simply accepting
    any year number within the range of QDate or QDateTime, as appropriate.

    \section2 Timezone representation

    The following options are only relevant to timezones. If used with other
    categories of field, they are ignored. By default, when the other fields of
    a timestamp determine a datetime, the zone's localized name in effect at
    that datetime is used when serializing and matched when parsing.

    There are both localized and international standard formats available. The
    following pair of options select which of those to use. They are grouped
    together as the \l {QtTemporalPattern::FieldGroup::} {LocalizationMask}
    constant:

    \value LocalizedZone For a TimeZone, use localized forms.
    \value Iso8601 For a TimeZone, use an ISO 8601 offset format.

    When both are specified, or neither is, localized forms are preferred. See
    \l {Locale-independent offset forms}, below, for details of the ISO
    8601-based formats. See \c{Standalone | Short} below for the IANA DB and
    \l{Local time} for a system-dependent form, both of which ignore the choice
    between LocalizedZone and Iso8601.

    The following three options, selecting seasonal variations, are mutually
    exclusive and only relevant to Verbal or Standalone forms. They are
    independent of the choice between LocalizedZone and Iso8601. They are
    grouped together as the \l {QtTemporalPattern::FieldGroup::} {SeasonMask}
    constant:

    \value GenericTime For a non-Numeric TimeZone, use its generic name.
    \value StandardTime For a non-Numeric TimeZone, use its standard-time name.
    \value DaylightSavingTime For a non-Numeric TimeZone, use its
                              daylight-saving name.

    For localized timezones, Numeric selects a basic offset format or an offset
    format with a base prefix (typically a localized form of UTC or GMT), Verbal
    selects a form of the zone's name, and Standalone selects the IANA ID or a
    localized name based on a city that serves as exemplar for the zone (for the
    given locale). The effects of the width options above then depend on which
    of these is used. When LocalizedZone is set, the following forms are
    available:

    \list
      \li For Numeric, the timezone offset is used:
        \list
          \li Wide uses the hour, minute and second, as for Short, but when
              parsing it will accept a fractional part of the seconds (following
              the locale-appropriate separator, after the whole number part of
              the seconds), if present. As Qt only supports whole numbers of
              seconds as offsets, this fractional part's only effect is to round
              up the second part if it is at least a half second. (See note on
              rounding in the next section.)
          \li Short uses the hour, minute and second, rounding to the nearest
              second if the offset is not a whole number of seconds.
          \li Abbreviated uses the hour and minute, rounding to the nearest
              minute if the offset is not a whole number of minutes.
          \li Narrow uses the hour, rounding to nearest if the actual offset is
              not a whole number of hours.
        \endlist
      \li For Verbal, the zone name is used:
        \list
          \li Wide uses the full name of the zone, e.g. "Pacific Time", "Pacific
              Standard Time" or "Pacific Daylight Time", depending on season.
          \li Short is treated as Wide.
          \li Abbreviated uses the zone abbreviation, e.g. "PT", "PST" or "PDT".
              Note that these cannot be reliably parsed.
          \li Narrow is treated as Abbreviated.
        \endlist
      \li For Standalone, the city name:
        \list
          \li Wide uses the locale's generic location format, such as "Los
              Angeles Time".
          \li Short uses the full IANA ID, such as America/Los_Angeles.
              This is not localized (the LocalizedZone flag is ignored).
          \li Abbreviated just gives the localized exemplar city, e.g. "Los
              Angeles".
          \li Narrow is treated as Abbreviated.
        \endlist
    \endlist

    Where a timezone is specified simply in terms of an offset from UTC, it does
    not necessarily have an associated city or a name other than its offset
    representations. For such zones, when serializing, Verbal and Standalone are
    treated as Numeric, with the exception of \c{Staldalone | Short}, as the
    IANA ID is not localized and this form, like an IANA ID, is accepted by the
    QTimeZone(QByteArray) constructor. (Note that these do not include ZeroPad:
    see the next section for its effect on offset forms.)  When parsing,
    therefore, these offset formats are accepted for the Verbal and Standalone
    forms that would map to them for offset zones. This applies to \l
    {QTimeZone::fromDurationAheadOfUtc()} lightweight time representations (for
    which \l {QTimeZone::}{isUtcOrFixedOffset()} is true) as well as to those,
    with a UTC-offset backend, constructed by QTimeZone(int) or, with an ID of
    form \c{"UTC"} or \c{"UTC±HH:mm"}, by QTimeZone(QByteArray),

    \section3 Offset modifiers

    In the various timezone offset formats (both above and below), there are
    potentially hour, minute and second parts of the offset, depending on the
    width option selected. The minute and second parts, when present, always use
    two digits, rendering the usual meaning of ZeroPad redundant. Instead, for
    these parts, ZeroPad controls whether trailing zero parts are included.
    When ZeroPad is set they are included when serializing and required to be
    present when parsing. When ZeroPad is not set, they are omitted when
    serializing and, if trailng parts are missing when parsing, they are taken
    to have value zero. Zero parts are always accepted when parsing, even when
    ZeroPad is not set.

    The hour part of an offset is never omitted when serializing and is always
    required when parsing. When ZeroPad is not set, when parsing offset formats
    with separators between parts, a single-digit hour is accepted. (For the
    localized offset formats above, whether there are separators between parts
    depends on the locale.) When ZeroPad is not set, serialization only produces
    a single-digit hour if there are no later parts (typically because they were
    all zero, hence omitted). Otherwise, the hour part is always serialized with
    two digits and parsing requires it to have two digits.

    As ever, if more than one width is specified, any given is allowed. On
    serializing, narrower formats are preferred unless they lose precision in
    the offset that an allowed longer format would include. Thus \c{Narrow |
    Short} would include only the hours for a whole-hour offset, but would
    include hours, minutes and seconds for an hour-and-a-half offset (albeit
    omitting the zero seconds unless ZeroPad is set). Where the width thus
    selected requires rounding due to omitting a part of the offset that isn't
    zero, exact halves round away from zero. On parsing, the longest allowed
    form for which a match is found will be used.

    Offset formats may also include a prefix (localized in the forms above) that
    identifies the offset as being relative to UTC or GMT. The following pair of
    mutually exclusive options control whether that is omitted or included.
    They are grouped together as the \l {QtTemporalPattern::FieldGroup::}
    {UtcPrefixMask} constant:

    \value AcceptUtcPrefix Include a UTC prefix on offsets when serializing and
                           require it when parsing.
    \value NeedNoUtcPrefix Leave off the UTC prefix when serializing and reject
                           it when parsing.

    If neither or both of these are specified, serializing omits the prefix and
    parsing permits it but does not require it.

    \section3 Locale-independent offset forms

    In addition to these localized forms, a timestamp may also represent its
    zone in a locale-independent offset form. In effect, this use the C locale,
    with ASCII digits 0 through 9, the ASCII + and - as signs, and the ASCII
    colon as separator (where relevant). These are selected by the \c Iso8601
    flag.

    \value AllowZSuffix Modifies Iso8601 to allow use of a \c{Z} suffix to
                        denote a zero UTC offset. See RFC 9557 note below.

    If AllowZSuffix is set and ZeroPad is not, serialization will use \c{Z} to
    represent a zero offset. When ZeroPad is set, serialization ignores
    AllowZSuffix. If AllowZSufix is set, regardless of ZeroPad, parsing will
    accept a \c{Z} suffix as meaning zero offset.

    The Iso8601 format is modified by other flags as follows (when not using
    \c{Z} to represent a zero offset):
    \list
      \li With Numeric it uses a basic format (with no separators), with Verbal
          or Standalone it uses separators between hours, minutes and seconds,
          in so far as these appear.
      \li The width options Wide, Short, Abbreviated and Narrow have the same
          meanings as for Numeric localized offset formats, modified by the
          ZeroPad option, as described above.
    \endlist

    \note In some contexts, notably where \l
    {https://www.rfc-editor.org/rfc/rfc9557.html} {RFC 9557} is used, a zone
    indicator on a timestamp may be understood as giving context to the
    information in which it appears, as opposed to only indicating the zone with
    respect to which the timestamp itself is expressed. In such contexts, a
    \c{Z} suffix, applied by \c AllowZSuffix, only indicates that the timestamp
    itself is given with respect to UTC and does not give any context to the
    information in which it appears. For contrast, in such contexts, a +00:00
    suffix does indicate not only that the timestamp is given in UTC but also
    that UTC is the relevant zone for the context of the information. Prior to
    RFC 9557, some RFCs (incompatibly with ISO 8601) proposed using -00:00 to
    indicate what RFC 9557 has specified as the meaning for a \c{Z}
    suffix. Nothing in Qt attends to this distinction.

    \section3 Local time

    Where Qt can determine how the local system describes its local time, Qt has
    no control over the form in which the system supplies it, nor does Qt know
    whether, or how, it is localized, with the result that this representation
    is system-dependent. As another system may be using a different local time,
    or representing it differently, there is no guarantee that what one system
    supplies for local time can be successfully understood on another system. To
    opt in to using this system-dependent representation of local time, supply
    the following option. Even then, when serializing, if the options given
    permit any other available representation of the timezone, that is preferred
    over this.

    \value LocalTimeName Allow the system-supplied local time name to describe
                         the system's local time.

    The system local time may come in separate forms for standard time and
    daylight-saving time. When it does, the options above for selecting between
    these affect LocalTimeName as usual, with GenericTime ignored.

    \sa {QtTemporalPattern::}{TemporalFieldCategory}, {QtTemporalPattern::}{TemporalFieldFlags}
*/
// RFC 9557: see QTBUG-114172

// TemporalFieldFlags and DateTimeParts: docs taken care of automagically by
// QDoc, recognizing QFLAG() usage.

/*!
    \enum QtTemporalPattern::DateTimePart
    \brief This enumeraction classifies the various field categories.

    The classification addresses which parts of a datetime a field contributes
    data to.

    \value None A literal field contributes no data.
    \value Date Various fields contribute to the date.
    \value Time Various fields contribute to the time.
    \value Zone Only the \l {QtTemporalPattern::TemporalFieldFlag::}{TimeZone}
           field contributes information about the timezone.

    These may be combined in a \l {QtTemporalPattern::}{DateTimeParts} to
    express a set of parts that a set of fields might suffice to describe,
    whether fully or only in part.

    \sa {QtTemporalPattern::}{TemporalFieldCategory}, {QtTemporalPattern::}{supports()}
*/

/*!
    \fn QtTemporalPattern::classify(QtTemporalPattern::TemporalFieldCategory category) noexcept
    \brief Classify a field category according to the part of a datetime it contributes to.

    This maps a \l {QtTemporalPattern::}{TemporalFieldCategory} to the \l
    {QtTemporalPattern::}{DateTimePart} for which it supplies data.
*/

/*!
    \class QtTemporalPattern::TemporalField
    \brief Describes one field in a temporal pattern.

    A single field is characterized by its \l
    {QtTemporalPattern::TemporalFieldCategory}{category}, some \l
    {QtTemporalPattern::TemporalFieldFlags}{options} identifying how the field
    is to be expressed and, where relevant, a datum. For a Literal field, the
    datum is the string to be matched.

    For numeric fields, a width may optionally be specified, indicating
    the expected number of digits in the field. If options specifies
    zero-padding, this is a minimum number of digits; otherwise, shorter fields
    are accepted. In any case, longer values are accepted, where the resulting
    numeric value is valid for the field.  A zero width does not mean an empty
    field will be accepted: it is effectively equivalent to a width of 1.

    \note in some locales, the digits may require surrogate pairs to encode, so
    the UTF-16 length of the field may exceed the number of digits.  In a year
    field with a sign, the sign does not count towards the width of the
    field. The width only counts digits.  The width is, in any case, a lower
    bound: more digits may be read, if present and not consumed by some later
    field. The \c endIndex of any parse result is the only reliable source of
    truth on the UTF-16 end of the text parsed.
*/
// TODO: should we support digit grouping in year numbers, at least with > 4 digits ?

/*!
    \fn QtTemporalPattern::hasFieldsFor(QSpan<const QtTemporalPattern::TemporalField> range)
    \brief Identify the parts to which the given fields contribute data.

    Takes a \a range of TemporalField instances and returns a \l
    {QtTemporalPattern::}{DateTimeParts} indicating which parts \l classify()
    says any of the fields contribute to. Note that this only tessts for
    contribution to a part, not for full coverage of the part. See \l supports()
    for that.

    \sa classify(), supports()
*/

/*!
    \enum QtTemporalPattern::SupportType
    \brief This enumeration identifies how well some fields describe requested datetime parts.

    The fields of a pattern may contain partial or complete data on each of the
    \l {QtTemporalPattern::TemporalFieldPart}{parts of} a datetime. When parsing
    or serializing only a date or only a time, fields for the other or for
    timezone are extraneous, making the pattern unable to serialize just the
    intended type, as it lacks the data for those fields. It would also, when
    parsing, be obliged discard some of the data it parses, as the type it
    returns cannot express it.

    When serializing a datetime, if the fields present do not suffice to fully
    encode the date or time, it will not be possible for a reader of the
    resulting text to unambiguously determine the datetime. If timezone is not
    specified, it is possible to convert the datetime to some specific choice of
    zone: provided both ends of the communication use the same zone, it is then
    possible to recover the exact point in time, albeit without knowing the
    timezone originally used to encode it.

    Where data is partially supplied, it is possible that the partial data
    suffices to meet the readers needs, although this typically involves the
    reader in making some default assumptions about the missing fields. For
    example, a two-digit year may need some assumptions about the century
    (possibly aided by information about the date and day of the week) to
    determine what year to presume the sender intended.

    \value None No fields were found.
    \value HasStrays Some field not relevant to the requested parts was present.
    \value Partial Either some requested part is present and some other is
                   missing or some fields for a requested part are present but
                   not enough to fully describe that part.
    \value Clear Enough fields of the requested parts were found to fully
                 determine them and no fields of unwanted parts were present.

    Where partial fields for the requested parts were found along with fields
    for unwanted parts, HasStrays is used in preference to Partial, as it is
    considered a more significant defect.

    \sa {QtTemporalPattern::}{TemporalFieldPart}, {QtTemporalPattern::}{supports()}
*/

/*!
    \fn QtTemporalPattern::supports(QtTemporalPattern::DateTimeParts wanted, QSpan<QtTemporalPattern::TemporalField> range, bool hasBaseYear) noexcept
    \brief Assess how well the given fields support the \a wanted parts.

    If any field in \a range belongs to a part not included in \a wanted,
    returns \l {SupportType::}{HasStrays}.  Otherwise,

    \list
      \li If no fields are present, aside from \l
          {TemporalFieldCategory::}{Literal} ones, returns \l
          {SupportType::}{None}.
      \li If the fields present completely specify all parts in \a wanted,
          returns \l {SupportType::}{Clear}.
      \li If some fields are specified but some \a wanted part is inadequately
          specified, or entirely unspecified, returns \l
          {SupportType::}{Partial}.
    \endlist

    For these purposes,

    \list
      \li The \l {DateTimePart::}{Zone} part is specified by the \l
          {TemporalFieldCategory::}{TimeZone}.
      \li The \l {DateTimePart::}{Time} part is specified by any combination of
          fields that identify the hour and minute. If the \l
          {TemporalFieldCategory::}{SecondFractions} field is present, the part is
          considered incompletely specified unless the \l
          {TemporalFieldCategory::}{Seconds} field is also present.
      \li The \l {DateTimePart::}{Date} part is specified by any combination of
          fields that identify a date. This usually means \l
          {TemporalFieldCategory::}{Year}, \l {TemporalFieldCategory::}{Month}
          and \l {TemporalFieldCategory::}{DayOfMonth} although Year may be
          indirectly specified by \l
          {TemporalFieldCategory::}{YearWithinCentury} if the presence of \l
          {TemporalFieldCategory::}{DayOfWeek} enables disambiguation among
          centuries close to the present, given the other date fields.
    \endlist
*/

SupportType supports(DateTimeParts wanted, QSpan<const TemporalField> range,
                     bool hasBaseYear) noexcept
{
    // TODO: may need to take into account calendar (and perhaps locale).
    SupportType support = SupportType::HasStrays;
    SupportType zone = SupportType::None;
    QBitArray date(40);
    QBitArray time(24);
    for (const TemporalField &field : range) {
        const DateTimePart part = field.part();
        switch (part) {
        case DateTimePart::None: // Literal contributes no data.
            continue;
        case DateTimePart::Date:
            date.setBit(quint8(field.category) - 64);
            break;
        case DateTimePart::Time:
            time.setBit(quint8(field.category) - 16);
            break;
        case DateTimePart::Zone:
            if (field.options.testAnyFlags(TemporalFieldFlag::Wide | TemporalFieldFlag::Short)
                || field.options.testAnyFlags(TemporalFieldFlag::Numeric
                                              | TemporalFieldFlag::Standalone
                                              | TemporalFieldFlag::Iso8601)) {
                zone = SupportType::Clear;
            } else if (zone == SupportType::None) {
                // Zone name abbreviation: does not uniquely identify zone.
                zone = SupportType::Partial;
            }
            break;
        }
        if (!wanted.testFlag(part))
            return support;
    }

    bool partsSeen = wanted.testFlag(DateTimePart::Zone);
    support = partsSeen ? zone : SupportType::None;

    constexpr auto join = [](SupportType lhs, SupportType rhs) ->  SupportType {
        Q_PRE(lhs != SupportType::HasStrays);
        Q_PRE(rhs != SupportType::HasStrays);
        // For combining SupportTypes from different Parts to determine support
        // for their composite. Simplest case is when they're the same:
        if (lhs == rhs)
            return lhs;
        // Every combination of distinct values among the other three is partial:
        return SupportType::Partial;
    };

    if (wanted.testFlag(DateTimePart::Date)) {
        const SupportType hasDate = [=]() {
            constexpr auto bitFor = [](TemporalFieldCategory cat) {
                return quint8(cat) - 64;
            };
#define CHECK(field) date.testBit(bitFor(TemporalFieldCategory::field))
            // if (CHECK(JulianDay)) return SupportType::Clear;
            int fields = 0;
            bool partial = false;
            if (CHECK(Year)) {
                ++fields;
            } else if (CHECK(YearWithinCentury)) {
                if (hasBaseYear) {
                    ++fields;
             // } else if (CHECK(Century)) { ++fields;
                } else if (CHECK(DayOfWeek) && CHECK(DayOfMonth) && CHECK(Month)) {
                    // We can infer century from those three by assuming it's
                    // close to the present.
                    ++fields;
                } else {
                    partial = true;
                }
         // } else if (CHECK(Century)) { partial = true;
            }
            if (CHECK(Month))
                ++fields;
            if (CHECK(DayOfMonth))
                ++fields;
            else if (CHECK(DayOfWeek))
                partial = true;
#undef CHECK
            if (fields == 3 && !partial)
                return SupportType::Clear;
            if (fields || partial)
                return SupportType::Partial;
            return SupportType::None;
        }();
        support = partsSeen ? join(support, hasDate) : hasDate;
        partsSeen = true;
    }

    if (wanted.testFlag(DateTimePart::Time)) {
        const SupportType hasTime = [=]() {
            constexpr auto bitFor = [](TemporalFieldCategory cat) {
                return quint8(cat) - 16;
            };
#define CHECK(field) time.testBit(bitFor(TemporalFieldCategory::field))
            // if (CHECK(MillisecondInDay)) return SupportType::Clear;
            int fields = 0;
            bool partial = false;
            if (CHECK(Hour)) {
                ++fields;
            } else if (CHECK(HourMod12)) {
                if (CHECK(PeriodInDay))
                    ++fields;
                else
                    partial = true;
            } else if (CHECK(PeriodInDay)) {
                partial = true;
            }
            if (CHECK(Minute))
                ++fields;
            // Hour and minute are required, but seconds and later are optional;
            // however, having a finer field without an coarser one leaves a gap => Partial.
            if (CHECK(Second))
                partial = fields < 2;
            else if (CHECK(SecondFraction))
                partial = true;
#undef CHECK
            if (fields == 2 && !partial)
                return SupportType::Clear;
            if (fields || partial)
                return SupportType::Partial;
            return SupportType::None;
        }();
        support = partsSeen ? join(support, hasTime) : hasTime;
        partsSeen = true;
    }
    // Shall still be None if we've seen no fields:
    Q_ASSERT(partsSeen || support == SupportType::None);
    return support;
}

} // namespace QtTemporalPattern

/*!
    \internal
    \since 6.12
    \class QDateTimePattern
    \brief A description of a serialization format for a datetime
*/

/*!
    \fn QDateTimePattern::forLocale(const QLocale &locale, QLocale::FormatType format)
    Construct a QDateTimePattern appropriate to the given \a locale.

    The \a format can be used to select how compact or expansive the pattern is.

    \sa fromQtFormat
*/

/*
//! [is-valid]
    Returns \c true if this pattern can be used to reliably transmit {\1}s.

    Returns \c false if there is unresolved ambiguity in the texts it will
    produce when serializing or the texts that will match it when parsing.
    Some apparent ambiguities may be resolved by interactions between other
    fields or the \c{defaults} parameter to \l{parse()}.
//! [is-valid]
*/

// TODO: currently (see supports(), above) doesn't take locale or calendar into account.
/*!
    \fn QDateTimePattern::isValid() const noexcept

    \include qttemporalpattern.cpp {is-valid} {datetime}
    \include qttemporalpattern.cpp base-year-disambiguates

    Timezone abbreviations are ambiguous.
*/

/*!
    \fn QDateTimePattern::setLocale(const QLocale &loc) noexcept

//! [set-locale]
    Sets the locale, for use for fields whose representation depends on locale,
    to \a loc. By default the application's current default locale is used.

    \sa QLocale::setDefault()
//! [set-locale]
*/

/*!
    \fn QDateTimePattern::locale() const noexcep

    Returns the current locale in use by this pattern.

    \sa setLocale()
*/

/*!
    \fn QDateTimePattern::setCalendar(QCalendar calendar) noexcept

    \include qttemporalpattern.cpp set-calendar
*/

/*!
    \fn QDateTimePattern::calendar(QCalendar calendar) const noexcept

    Returns the current calendar in use by this pattern.

    \sa setCalendar()
*/

/*!
    \fn QDateTimePattern::setBaseYear(int centuryStart) noexcept

    \include qttemporalpattern.cpp set-base-year
*/

/*!
    \fn QDateTimePattern::clearBaseYear() noexcept

    \include qttemporalpattern.cpp clear-base-year
*/

/*!
    \fn QDateTimePattern::baseYear() const noexcept

    \include qttemporalpattern.cpp get-base-year
*/

/*!
    Parse and return the datetime represented by \a text.

//! [parser-defaults]
    If \a defaults is provided and valid, any fields the format described by
    this pattern does not provide will be copied from \a defaults to construct
    the returned value.
//! [parser-defaults]
*/

QtTemporalPattern::ParseResult<QDateTime>
QDateTimePattern::parse(QStringView text, const QDateTime &defaults) const
{
    Q_UNUSED(text);
    Q_UNUSED(defaults);
    return {};
}

/*!
    Serialize the given \a datetime to a string representation.
*/

QString QDateTimePattern::serialize(const QDateTime &datetime) const
{
    Q_UNUSED(datetime);
    return {};
}

/*!
    Construct a QDateTimePattern described by the given \a format string.

    \sa forLocale()
*/

QDateTimePattern QDateTimePattern::fromQtFormat(QStringView format)
{
    Q_UNUSED(format);
    return QDateTimePattern({});
}

/*!
    \internal
    \since 6.12
    \class QTimePattern
    \brief A description of a serialization format for a time
*/

/*!
    \fn QTimePattern::forLocale(const QLocale &locale, QLocale::FormatType format)
    Construct a QTimePattern appropriate to the given \a locale.

    The \a format can be used to select how compact or expansive the pattern is.

    \sa fromQtFormat
*/

/*!
    \fn QTimePattern::isValid() const noexcept

    \include qttemporalpattern.cpp {is-valid} {time}
*/

/*!
    \fn QTimePattern::setLocale(const QLocale &loc) noexcept

    \include qttemporalpattern.cpp set-locale
*/

/*!
    \fn QTimePattern::locale() const noexcep

    Returns the current locale in use by this pattern.

    \sa setLocale()
*/

/*!
    Parse and return the time represented by \a text.

    \include qttemporalpattern.cpp parser-defaults
*/

QtTemporalPattern::ParseResult<QTime>
QTimePattern::parse(QStringView text, QTime defaults) const
{
    Q_UNUSED(text);
    Q_UNUSED(defaults);
    return {};
}

/*!
    Serialize the given \a time to a string representation.
*/

QString QTimePattern::serialize(const QTime &time) const
{
    Q_UNUSED(time);
    return {};
}

/*!
    Construct a QTimePattern described by the given \a format string.

    \sa forLocale()
*/

QTimePattern QTimePattern::fromQtFormat(QStringView format)
{
    Q_UNUSED(format);
    return QTimePattern({});
}

/*!
    \internal
    \since 6.12
    \class QDatePattern
    \brief A description of a serialization format for a date
*/

/*!
    \fn QDatePattern::forLocale(const QLocale &locale, QLocale::FormatType format)
    Construct a QDatePattern appropriate to the given \a locale.

    The \a format can be used to select how compact or expansive the pattern is.

    \sa fromQtFormat
*/

/*!
    \fn QDatePattern::isValid() const noexcept

    \include qttemporalpattern.cpp {is-valid} {date}

//! [base-year-disambiguates]
    The ambiguity of two-digit years may be resolved by configuring the hundred
    years among which to select a matching years.
//! [base-year-disambiguates]

    \sa setBaseYear()
*/

/*!
    \fn QDatePattern::setLocale(const QLocale &loc) noexcept

    \include qttemporalpattern.cpp set-locale
*/

/*!
    \fn QDatePattern::locale() const noexcep

    Returns the current locale in use by this pattern.

    \sa setLocale()
*/

/*!
    \fn QDatePattern::setCalendar(QCalendar calendar) noexcept

//! [set-calendar]
    Sets the calendar used by this pattern. This influences the names and
    lengths of months and how these vary from year to year. In some cases it may
    also influence the number of months in the year or even the pattern and
    names of days of the week.

    By default the Gregorian calendar is used.

    \sa getCalendar(), QCalendar
//! [set-calendar]
*/

/*!
    \fn QDatePattern::calendar(QCalendar calendar) const noexcept

    Returns the current calendar in use by this pattern.

    \sa setCalendar()
*/

/*!
    \fn QDatePattern::setBaseYear(int centuryStart) noexcept

//! [set-base-year]
    Configures the handling of two-digit years, if any are present in the pattern.

    If a two-digit year is present, the year ending in those two digits in the
    range from \a centuryStart to \c {centuryStart + 99} shall be its default
    interpretation. This may be amended if the month, day of the month and day
    of the week indicate some other nearby century.

    \note the years here, including \a centuryStart, are expressed with respect
    to \c{calendar()}, whose year numbering need not match that of the Gregorian
    calendar.

    Calling this method makes no difference unless the pattern does in fact use
    a two-digit year, nor does it affect serialization.

    \sa clearBaseYear()
//! [set-base-year]
*/

// TODO: should the default state be clear or some specific year ?
// That year could depend on the present, e.g. present minus 49 years.

/*!
    \fn QDatePattern::clearBaseYear() noexcept

//! [clear-base-year]
    Leaves the handling of two-digit years to other fields to disambiguate.

    This is the default state, so has no effect unless \l setBaseYear() has
    previously been called.

    If a two-digit year is present, this leaves unspecified which hundred years
    to presume it lies within. If a two-digit year is present and other fields
    of the pattern do not suffice to make clear which hundred years to select
    based on the given last two digits, the text produced when serializing with
    this format shall generally be ambiguous. While readers of the text may be
    able to disambiguate the year anyway, there is scope for misunderstanding if
    their heuristics for doing so do not match your expectation. On parsing, the
    pattern may deliver a result that is off by some whole number of centuries
    from what the parsed text's author intended.

    \sa setBaseYear(), isValid()
//! [clear-base-year]
*/

/*!
    \fn QDatePattern::baseYear() const noexcept

//! [get-base-year]

    Returns the current base year, with respect to \c{calendar()}, used by this
    pattern if it needs to resolve a two-digit year. The result is \c
    {std::nullopt} when no base year is set, which is the default state.

    \sa setBaseYear()
//! [get-base-year]
*/

/*!
    Parse and return the date represented by \a text.

    \include qttemporalpattern.cpp parser-defaults
*/

QtTemporalPattern::ParseResult<QDate>
QDatePattern::parse(QStringView text, QDate defaults) const
{
    Q_UNUSED(text);
    Q_UNUSED(defaults);
    return {};
}

/*!
    Serialize the given \a date to a string representation.

    \sa parse()
*/

QString QDatePattern::serialize(const QDate &date) const
{
    Q_UNUSED(date);
    return {};
}

/*!
    Construct a QDatePattern described by the given \a format string.

    \sa forLocale()
*/

QDatePattern QDatePattern::fromQtFormat(QStringView format)
{
    Q_UNUSED(format);
    return QDatePattern({});
}

QT_END_NAMESPACE
