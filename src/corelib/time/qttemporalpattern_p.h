// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTTEMPORALPATTERN_P_H
#define QTTEMPORALPATTERN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an implementation
// detail. This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#include <QtCore/qcalendar.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qflags.h>
#include <QtCore/qlist.h>
#include <QtCore/qlocale.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qspan.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>

#include <optional>

QT_REQUIRE_CONFIG(datestring);
QT_BEGIN_NAMESPACE

namespace QtTemporalPattern {

    // Model the LDML spec's range of possible fields, see QTBUG-70516 and
    // https://www.unicode.org/reports/tr35/tr35-dates.html#table-date-field-symbol-table
    enum class TemporalFieldCategory : quint8 {
        Literal = 0,

        // Special:
        TimeZone = 4,

        // Time:
        // MillisecondInDay = 17,
        Second = 20, SecondFraction = 21,
        Minute = 24, // MinuteFraction = 25,
        PeriodInDay = 30, // am/pm; LDML also has noon, midnight, "at night" and others.
        HourMod12 = 31, Hour = 32, // HourFraction = 33,

        // Date:
        DayOfWeek = 64, DayOfMonth = 65, // DayOfYear = 68, JulianDay = 69,
        // WeekOfMonth = 72,
        // WeekOfYear = 73, // QTBUG-57212, QTBUG-83678, QTBUG-117055
        Month = 80,
        // Quarter = 84,
        // RelatedGregorianYear = 90, Century = 91,
        YearWithinCentury = 92, Year = 93,
        // Era = 100,

        EndCategories
    };
    constexpr inline auto EndTemporalFieldCategories = qToUnderlying(TemporalFieldCategory::EndCategories);

    enum class TemporalFieldFlag : quint32 {
        Numeric = 1, Verbal = 2, Standalone = 4,
        Narrow = 0x10, Abbreviated = 0x20, Short = 0x40, Wide = 0x80,
        ZeroPad = 0x0100, // For TimeZone, ZeroPad applies only to hour fields.
        SpacePad = 0x0200, FlexSpace = 0x0400,
        // Qt used to impose case on am/pm fields:
        LowerCase = 0x1000, UpperCase = 0x2000, // Otherwise follow locale-supplied case.
        IgnoreCase = 0x4000, // Parsing Literal or Verbal: match case insensitively
        // Year-specific:
        YearSignIso8601 = 0x8000,
        // Zone-specific:
        LocalizedZone = 0x01'0000, // Localized forms, various.
        Iso8601 = 0x02'0000, // Non-localized standard offset forms.
        AcceptUtcPrefix = 0x04'0000, NeedNoUtcPrefix = 0x08'0000, // On offsets
        AllowZSuffix = 0x10'0000, // Iso8601 zero-offset may be indicated by Z.
        // Time-types (if none specified, infer time type from date/time fields):
        GenericTime = 0x20'0000, StandardTime = 0x40'0000, DaylightSavingTime = 0x80'0000,
        // Local time from system info:
        LocalTimeName = 0x8000'0000 // That's the last bit available to us.
    };
    Q_DECLARE_FLAGS(TemporalFieldFlags, TemporalFieldFlag)
    Q_DECLARE_OPERATORS_FOR_FLAGS(TemporalFieldFlags)

    namespace FieldGroup {
        constexpr TemporalFieldFlags FormMask = TemporalFieldFlag::Numeric
            | TemporalFieldFlag::Verbal | TemporalFieldFlag::Standalone;
        constexpr TemporalFieldFlags WidthMask
            = TemporalFieldFlag::Narrow | TemporalFieldFlag::Abbreviated
            | TemporalFieldFlag::Short | TemporalFieldFlag::Wide;
        constexpr TemporalFieldFlags PaddingMask
            = TemporalFieldFlag::ZeroPad | TemporalFieldFlag::SpacePad;
        constexpr TemporalFieldFlags LocalizationMask
            = TemporalFieldFlag::LocalizedZone | TemporalFieldFlag::Iso8601;
        constexpr TemporalFieldFlags UtcPrefixMask
            = TemporalFieldFlag::AcceptUtcPrefix | TemporalFieldFlag::NeedNoUtcPrefix;
        constexpr TemporalFieldFlags SeasonMask = TemporalFieldFlag::GenericTime
            | TemporalFieldFlag::StandardTime | TemporalFieldFlag::DaylightSavingTime;
    }

    // Implement group-related tests:
    constexpr inline
    bool matchesFlagWithin(QtTemporalPattern::TemporalFieldFlags flags,
                           QtTemporalPattern::TemporalFieldFlag sought,
                           QtTemporalPattern::TemporalFieldFlags group)
    {
        Q_ASSERT(group.testFlag(sought)); // In case caller passed the wrong group.
        return flags.testFlag(sought) || !flags.testAnyFlags(group);
    }

    constexpr inline
    bool matchesFlagsWithin(QtTemporalPattern::TemporalFieldFlags flags,
                            QtTemporalPattern::TemporalFieldFlags sought,
                            QtTemporalPattern::TemporalFieldFlags group)
    {
        Q_ASSERT(!(sought & ~group)); // i.e. all of sought are in group.
        return flags.testAnyFlags(sought) || !flags.testAnyFlags(group);
    }

    // Classify the categories:
    enum class DateTimePart { None, Date = 1, Time = 2, Zone = 4 };
    Q_DECLARE_FLAGS(DateTimeParts, DateTimePart)
    Q_DECLARE_OPERATORS_FOR_FLAGS(DateTimeParts)
#ifdef QT_BUILD_INTERNAL
    Q_NAMESPACE_EXPORT(Q_CORE_EXPORT)
    Q_FLAG_NS(TemporalFieldFlags)
    Q_FLAG_NS(DateTimeParts)
#endif // For testing.

    inline DateTimePart classify(TemporalFieldCategory category) noexcept
    {
        if (category == TemporalFieldCategory::Literal)
            return DateTimePart::None;
        if (quint8(category) < 16)
            return DateTimePart::Zone;
        if (quint8(category) < 64) // a.k.a. 0x40
            return DateTimePart::Time;
        return DateTimePart::Date;
    }

    // Combine with related information:
    struct TemporalField {
        QString literal; // Only relevant to Literal
        qsizetype width; // Lower bound, usually only relevant to Numeric.
        TemporalFieldFlags options;
        TemporalFieldCategory category;
        DateTimePart part() const noexcept { return classify(category); }
        friend bool comparesEqual(const TemporalField &lhs, const TemporalField &rhs) noexcept
        {
            return lhs.literal == rhs.literal
                && lhs.width == rhs.width
                && lhs.options == rhs.options
                && lhs.category == rhs.category;
        }
        Q_DECLARE_EQUALITY_COMPARABLE(TemporalField)
    };

    inline DateTimeParts hasFieldsFor(QSpan<const TemporalField> range)
    {
        DateTimeParts result = DateTimePart::None;
        for (const TemporalField &it : range)
            result |= it.part();
        return result;
    }

    enum class SupportType { Partial = -1, None = 0, Clear = 1, HasStrays };
    Q_CORE_EXPORT
    SupportType supports(DateTimeParts wanted, QSpan<const TemporalField> range,
                         bool hasBaseYear = false) noexcept;
} // namespace QtTemporalPattern

/* We may eventually want to make the following public API (along with a
   forward-declaration of QtTemporalPattern::TemporalField), while leaving the
   above in a private header. See QTBUG-70516, QTBUG-81056.

   This may, of course, involve splitting the types below into a public facade
   with the actual data members hidden from view behind a shared d-pointer to a
   private internal class.

   Please bear that in mind when considering the design of the APIs below, and
   any future changes thereto.
*/

namespace QtTemporalPattern {
template <typename Payload>
struct ParseResult
{
    Payload payload;
    qsizetype size = 0;
};
} // namespace QtTemporalPattern

class QDateTimePattern
{
    using Field = QtTemporalPattern::TemporalField;
    QList<Field> m_fields;
    QLocale m_locale;
    QCalendar m_calendar;
    std::optional<int> m_baseYear;
    explicit QDateTimePattern(const QList<Field> &fs) : m_fields(fs) {}
    explicit QDateTimePattern(const QList<Field> &fs, int centuryStart)
        : m_fields(fs), m_baseYear(centuryStart) {}

    QtTemporalPattern::SupportType dateTimeSupport() const noexcept
    {
        using namespace QtTemporalPattern;
        constexpr DateTimeParts NeededParts = DateTimePart::Date | DateTimePart::Time;
        const DateTimeParts got = hasFieldsFor(m_fields);
        // Must have date and time (zone optional) and support what it has:
        if (got.testFlags(NeededParts))
            return supports(got, m_fields, m_baseYear.has_value());
        return got ? SupportType::Partial : SupportType::None;
    }
public:
    bool isValid() const noexcept
    {
        return dateTimeSupport() == QtTemporalPattern::SupportType::Clear;
    }
    void setLocale(const QLocale &loc) { m_locale = loc; }
    const QLocale &locale() const noexcept { return m_locale; }

    void setBaseYear(int centuryStart) { m_baseYear = centuryStart; }
    void clearBaseYear() noexcept { m_baseYear = std::nullopt; }
    std::optional<int> baseYear() const noexcept { return m_baseYear; }

    void setCalendar(QCalendar cal) { m_calendar = cal; }
    QCalendar calendar() const noexcept { return m_calendar; }

    Q_CORE_EXPORT QtTemporalPattern::ParseResult<QDateTime>
    parse(QStringView text, const QDateTime &defaults = {}) const;
    Q_CORE_EXPORT QString serialize(const QDateTime &datetime) const;

    static Q_CORE_EXPORT QDateTimePattern fromQtFormat(QStringView format);
    static
    QDateTimePattern forLocale(const QLocale &locale,
                               QLocale::FormatType format = QLocale::LongFormat)
    {
        auto pat = fromQtFormat(locale.dateTimeFormat(format));
        pat.setLocale(locale);
        return pat;
    }
};

class QTimePattern
{
    using Field = QtTemporalPattern::TemporalField;
    QList<Field> m_fields;
    QLocale m_locale;
    explicit QTimePattern(const QList<Field> &fs) : m_fields(fs) {}

    QtTemporalPattern::SupportType timeSupport() const noexcept
    {
        using namespace QtTemporalPattern;
        return supports({DateTimePart::Time}, m_fields);
    }
public:
    bool isValid() const noexcept
    {
        return timeSupport() == QtTemporalPattern::SupportType::Clear;
    }
    void setLocale(const QLocale &loc) { m_locale = loc; }
    const QLocale &locale() const noexcept { return m_locale; }

    Q_CORE_EXPORT QtTemporalPattern::ParseResult<QTime>
    parse(QStringView text, QTime defaults = {}) const;
    Q_CORE_EXPORT QString serialize(const QTime &time) const;

    static Q_CORE_EXPORT QTimePattern fromQtFormat(QStringView format);
    static
    QTimePattern forLocale(const QLocale &locale, QLocale::FormatType format = QLocale::LongFormat)
    {
        auto pat = fromQtFormat(locale.timeFormat(format));
        pat.setLocale(locale);
        return pat;
    }
};

class QDatePattern
{
    using Field = QtTemporalPattern::TemporalField;
    QList<Field> m_fields;
    QLocale m_locale;
    QCalendar m_calendar;
    std::optional<int> m_baseYear;
    explicit QDatePattern(const QList<Field> &fs) : m_fields(fs) {}
    explicit QDatePattern(const QList<Field> &fs, int centuryStart)
        : m_fields(fs), m_baseYear(centuryStart) {}

    QtTemporalPattern::SupportType dateSupport() const noexcept
    {
        using namespace QtTemporalPattern;
        return supports({DateTimePart::Date}, m_fields, m_baseYear.has_value());
    }
public:
    bool isValid() const noexcept
    {
        return dateSupport() == QtTemporalPattern::SupportType::Clear;
    }
    void setLocale(const QLocale &loc) { m_locale = loc; }
    const QLocale &locale() const noexcept { return m_locale; }

    void setBaseYear(int centuryStart) { m_baseYear = centuryStart; }
    void clearBaseYear() noexcept { m_baseYear = std::nullopt; }
    std::optional<int> baseYear() const noexcept { return m_baseYear; }

    void setCalendar(QCalendar cal) { m_calendar = cal; }
    QCalendar calendar() const noexcept { return m_calendar; }

    Q_CORE_EXPORT QtTemporalPattern::ParseResult<QDate>
    parse(QStringView text, QDate defaults = {}) const;
    Q_CORE_EXPORT QString serialize(const QDate &date) const;

    static Q_CORE_EXPORT QDatePattern fromQtFormat(QStringView format);
    static
    QDatePattern forLocale(const QLocale &locale, QLocale::FormatType format = QLocale::LongFormat)
    {
        auto pat = fromQtFormat(locale.dateFormat(format));
        pat.setLocale(locale);
        return pat;
    }
};

QT_END_NAMESPACE

#endif // QTTEMPORALPATTERN_P_H
