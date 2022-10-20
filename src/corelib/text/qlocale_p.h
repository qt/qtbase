// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLOCALE_P_H
#define QLOCALE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "qlocale.h"

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qcalendar.h>
#include <QtCore/qlist.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <QtCore/qvarlengtharray.h>

#include <limits>
#include <cmath>

QT_BEGIN_NAMESPACE

struct QLocaleData;
// Subclassed by Android platform plugin:
class Q_CORE_EXPORT QSystemLocale
{
    QSystemLocale *next = nullptr; // Maintains a stack.
public:
    QSystemLocale();
    virtual ~QSystemLocale();

    struct CurrencyToStringArgument
    {
        CurrencyToStringArgument() { }
        CurrencyToStringArgument(const QVariant &v, const QString &s)
            : value(v), symbol(s) { }
        QVariant value;
        QString symbol;
    };

    enum QueryType {
        LanguageId, // uint
        TerritoryId, // uint
        DecimalPoint, // QString
        GroupSeparator, // QString (empty QString means: don't group digits)
        ZeroDigit, // QString
        NegativeSign, // QString
        DateFormatLong, // QString
        DateFormatShort, // QString
        TimeFormatLong, // QString
        TimeFormatShort, // QString
        DayNameLong, // QString, in: int
        DayNameShort, // QString, in: int
        DayNameNarrow, // QString, in: int
        MonthNameLong, // QString, in: int
        MonthNameShort, // QString, in: int
        MonthNameNarrow, // QString, in: int
        DateToStringLong, // QString, in: QDate
        DateToStringShort, // QString in: QDate
        TimeToStringLong, // QString in: QTime
        TimeToStringShort, // QString in: QTime
        DateTimeFormatLong, // QString
        DateTimeFormatShort, // QString
        DateTimeToStringLong, // QString in: QDateTime
        DateTimeToStringShort, // QString in: QDateTime
        MeasurementSystem, // uint
        PositiveSign, // QString
        AMText, // QString
        PMText, // QString
        FirstDayOfWeek, // Qt::DayOfWeek
        Weekdays, // QList<Qt::DayOfWeek>
        CurrencySymbol, // QString in: CurrencyToStringArgument
        CurrencyToString, // QString in: qlonglong, qulonglong or double
        Collation, // QString
        UILanguages, // QStringList
        StringToStandardQuotation, // QString in: QStringView to quote
        StringToAlternateQuotation, // QString in: QStringView to quote
        ScriptId, // uint
        ListToSeparatedString, // QString
        LocaleChanged, // system locale changed
        NativeLanguageName, // QString
        NativeTerritoryName, // QString
        StandaloneMonthNameLong, // QString, in: int
        StandaloneMonthNameShort, // QString, in: int
        StandaloneMonthNameNarrow, // QString, in: int
        StandaloneDayNameLong, // QString, in: int
        StandaloneDayNameShort, // QString, in: int
        StandaloneDayNameNarrow // QString, in: int
    };
    virtual QVariant query(QueryType type, QVariant in = QVariant()) const;

    virtual QLocale fallbackLocale() const;
    inline qsizetype fallbackLocaleIndex() const;
};
Q_DECLARE_TYPEINFO(QSystemLocale::QueryType, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(QSystemLocale::CurrencyToStringArgument, Q_RELOCATABLE_TYPE);

#if QT_CONFIG(icu)
namespace QIcu {
    QString toUpper(const QByteArray &localeId, const QString &str, bool *ok);
    QString toLower(const QByteArray &localeId, const QString &str, bool *ok);
}
#endif


struct QLocaleId
{
    [[nodiscard]] Q_AUTOTEST_EXPORT static QLocaleId fromName(QStringView name);
    [[nodiscard]] inline bool operator==(QLocaleId other) const
    { return language_id == other.language_id && script_id == other.script_id && territory_id == other.territory_id; }
    [[nodiscard]] inline bool operator!=(QLocaleId other) const
    { return !operator==(other); }
    [[nodiscard]] inline bool isValid() const
    {
        return language_id <= QLocale::LastLanguage && script_id <= QLocale::LastScript
                && territory_id <= QLocale::LastTerritory;
    }
    [[nodiscard]] inline bool matchesAll() const
    {
        return !language_id && !script_id && !territory_id;
    }
    // Use as: filter.accept...(candidate)
    [[nodiscard]] inline bool acceptLanguage(quint16 lang) const
    {
        // Always reject AnyLanguage (only used for last entry in locale_data array).
        // So, when searching for AnyLanguage, accept everything *but* AnyLanguage.
        return language_id ? lang == language_id : lang;
    }
    [[nodiscard]] inline bool acceptScriptTerritory(QLocaleId other) const
    {
        return (!territory_id || other.territory_id == territory_id)
                && (!script_id || other.script_id == script_id);
    }

    [[nodiscard]] QLocaleId withLikelySubtagsAdded() const;
    [[nodiscard]] QLocaleId withLikelySubtagsRemoved() const;

    [[nodiscard]] QByteArray name(char separator = '-') const;

    ushort language_id = 0, script_id = 0, territory_id = 0;
};
Q_DECLARE_TYPEINFO(QLocaleId, Q_PRIMITIVE_TYPE);

struct QLocaleData
{
public:
    // Having an index for each locale enables us to have diverse sources of
    // data, e.g. calendar locales, as well as the main CLDR-derived data.
    [[nodiscard]] static qsizetype findLocaleIndex(QLocaleId localeId);
    [[nodiscard]] static const QLocaleData *c();

    enum DoubleForm {
        DFExponent = 0,
        DFDecimal,
        DFSignificantDigits,
        _DFMax = DFSignificantDigits
    };

    enum Flags {
        NoFlags             = 0,
        AddTrailingZeroes   = 0x01,
        ZeroPadded          = 0x02,
        LeftAdjusted        = 0x04,
        BlankBeforePositive = 0x08,
        AlwaysShowSign      = 0x10,
        GroupDigits         = 0x20,
        CapitalEorX         = 0x40,

        ShowBase            = 0x80,
        UppercaseBase       = 0x100,
        ZeroPadExponent     = 0x200,
        ForcePoint          = 0x400
    };

    enum NumberMode { IntegerMode, DoubleStandardMode, DoubleScientificMode };

    typedef QVarLengthArray<char, 256> CharBuff;

private:
    enum PrecisionMode {
        PMDecimalDigits =       0x01,
        PMSignificantDigits =   0x02,
        PMChopTrailingZeros =   0x03
    };

    [[nodiscard]] QString decimalForm(QString &&digits, int decpt, int precision,
                                      PrecisionMode pm, bool mustMarkDecimal,
                                      bool groupDigits) const;
    [[nodiscard]] QString exponentForm(QString &&digits, int decpt, int precision,
                                       PrecisionMode pm, bool mustMarkDecimal,
                                       int minExponentDigits) const;
    [[nodiscard]] QString signPrefix(bool negative, unsigned flags) const;
    [[nodiscard]] QString applyIntegerFormatting(QString &&numStr, bool negative, int precision,
                                                 int base, int width, unsigned flags) const;

public:
    [[nodiscard]] QString doubleToString(double d,
                                         int precision = -1,
                                         DoubleForm form = DFSignificantDigits,
                                         int width = -1,
                                         unsigned flags = NoFlags) const;
    [[nodiscard]] QString longLongToString(qint64 l, int precision = -1,
                                           int base = 10,
                                           int width = -1,
                                           unsigned flags = NoFlags) const;
    [[nodiscard]] QString unsLongLongToString(quint64 l, int precision = -1,
                                              int base = 10,
                                              int width = -1,
                                              unsigned flags = NoFlags) const;

    // this function is meant to be called with the result of stringToDouble or bytearrayToDouble
    [[nodiscard]] static float convertDoubleToFloat(double d, bool *ok)
    {
        if (qIsInf(d))
            return float(d);
        if (std::fabs(d) > (std::numeric_limits<float>::max)()) {
            if (ok)
                *ok = false;
            const float huge = std::numeric_limits<float>::infinity();
            return d < 0 ? -huge : huge;
        }
        if (d != 0 && float(d) == 0) {
            // Values that underflow double already failed. Match them:
            if (ok)
                *ok = false;
            return 0;
        }
        return float(d);
    }

    [[nodiscard]] double stringToDouble(QStringView str, bool *ok,
                                        QLocale::NumberOptions options) const;
    [[nodiscard]] qint64 stringToLongLong(QStringView str, int base, bool *ok,
                                          QLocale::NumberOptions options) const;
    [[nodiscard]] quint64 stringToUnsLongLong(QStringView str, int base, bool *ok,
                                              QLocale::NumberOptions options) const;

    // this function is used in QIntValidator (QtGui)
    [[nodiscard]] Q_CORE_EXPORT static qint64 bytearrayToLongLong(QByteArrayView num, int base,
                                                                  bool *ok);
    [[nodiscard]] static quint64 bytearrayToUnsLongLong(QByteArrayView num, int base, bool *ok);

    [[nodiscard]] bool numberToCLocale(QStringView s, QLocale::NumberOptions number_options,
                                       CharBuff *result) const;
    [[nodiscard]] inline char numericToCLocale(QStringView in) const;

    // this function is used in QIntValidator (QtGui)
    [[nodiscard]] Q_CORE_EXPORT bool validateChars(
            QStringView str, NumberMode numMode, QByteArray *buff, int decDigits = -1,
            QLocale::NumberOptions number_options = QLocale::DefaultNumberOptions) const;

    // Access to assorted data members:
    [[nodiscard]] QLocaleId id() const
    { return QLocaleId { m_language_id, m_script_id, m_territory_id }; }

    [[nodiscard]] QString decimalPoint() const;
    [[nodiscard]] QString groupSeparator() const;
    [[nodiscard]] QString listSeparator() const;
    [[nodiscard]] QString percentSign() const;
    [[nodiscard]] QString zeroDigit() const;
    [[nodiscard]] char32_t zeroUcs() const;
    [[nodiscard]] QString positiveSign() const;
    [[nodiscard]] QString negativeSign() const;
    [[nodiscard]] QString exponentSeparator() const;

    struct DataRange
    {
        quint16 offset;
        quint16 size;
        [[nodiscard]] QString getData(const char16_t *table) const
        {
            return size > 0
                ? QString::fromRawData(reinterpret_cast<const QChar *>(table + offset), size)
                : QString();
        }
        [[nodiscard]] QStringView viewData(const char16_t *table) const
        {
            return { reinterpret_cast<const QChar *>(table + offset), size };
        }
        [[nodiscard]] QString getListEntry(const char16_t *table, qsizetype index) const
        {
            return listEntry(table, index).getData(table);
        }
        [[nodiscard]] QStringView viewListEntry(const char16_t *table, qsizetype index) const
        {
            return listEntry(table, index).viewData(table);
        }
        [[nodiscard]] char32_t ucsFirst(const char16_t *table) const
        {
            if (size && !QChar::isSurrogate(table[offset]))
                return table[offset];
            if (size > 1 && QChar::isHighSurrogate(table[offset]))
                return QChar::surrogateToUcs4(table[offset], table[offset + 1]);
            return 0;
        }
    private:
        [[nodiscard]] DataRange listEntry(const char16_t *table, qsizetype index) const
        {
            const char16_t separator = ';';
            quint16 i = 0;
            while (index > 0 && i < size) {
                if (table[offset + i] == separator)
                    index--;
                i++;
            }
            quint16 end = i;
            while (end < size && table[offset + end] != separator)
                end++;
            return { quint16(offset + i), quint16(end - i) };
        }
    };

#define ForEachQLocaleRange(X) \
    X(startListPattern) X(midListPattern) X(endListPattern) X(pairListPattern) X(listDelimit) \
    X(decimalSeparator) X(groupDelim) X(percent) X(zero) X(minus) X(plus) X(exponential) \
    X(quoteStart) X(quoteEnd) X(quoteStartAlternate) X(quoteEndAlternate) \
    X(longDateFormat) X(shortDateFormat) X(longTimeFormat) X(shortTimeFormat) \
    X(longDayNamesStandalone) X(longDayNames) \
    X(shortDayNamesStandalone) X(shortDayNames) \
    X(narrowDayNamesStandalone) X(narrowDayNames) \
    X(anteMeridiem) X(postMeridiem) \
    X(byteCount) X(byteAmountSI) X(byteAmountIEC) \
    X(currencySymbol) X(currencyDisplayName) \
    X(currencyFormat) X(currencyFormatNegative) \
    X(endonymLanguage) X(endonymTerritory)

#define rangeGetter(name) \
    [[nodiscard]] DataRange name() const { return { m_ ## name ## _idx, m_ ## name ## _size }; }
    ForEachQLocaleRange(rangeGetter)
#undef rangeGetter

public:
    quint16 m_language_id, m_script_id, m_territory_id;

    // Offsets, then sizes, for each range:
#define rangeIndex(name) quint16 m_ ## name ## _idx;
    ForEachQLocaleRange(rangeIndex)
#undef rangeIndex
#define Size(name) quint8 m_ ## name ## _size;
    ForEachQLocaleRange(Size)
#undef Size

#undef ForEachQLocaleRange

    // Strays:
    char m_currency_iso_code[3];
    quint8 m_currency_digits : 2;
    quint8 m_currency_rounding : 3; // (not yet used !)
    quint8 m_first_day_of_week : 3;
    quint8 m_weekend_start : 3;
    quint8 m_weekend_end : 3;
    quint8 m_grouping_top : 2; // Must have this many before the first grouping separator
    quint8 m_grouping_higher : 3; // Number of digits between grouping separators
    quint8 m_grouping_least : 3; // Number of digits after last grouping separator (before decimal).
};

class QLocalePrivate
{
public:
    constexpr QLocalePrivate(const QLocaleData *data, qsizetype index,
                             QLocale::NumberOptions numberOptions = QLocale::DefaultNumberOptions,
                             int refs = 0)
        : m_data(data), ref Q_BASIC_ATOMIC_INITIALIZER(refs),
          m_index(index), m_numberOptions(numberOptions) {}

    [[nodiscard]] quint16 languageId() const { return m_data->m_language_id; }
    [[nodiscard]] quint16 territoryId() const { return m_data->m_territory_id; }

    [[nodiscard]] QByteArray bcp47Name(char separator = '-') const;

    [[nodiscard]] inline QLatin1StringView
    languageCode(QLocale::LanguageCodeTypes codeTypes = QLocale::AnyLanguageCode) const
    {
        return languageToCode(QLocale::Language(m_data->m_language_id), codeTypes);
    }
    [[nodiscard]] inline QLatin1StringView scriptCode() const
    { return scriptToCode(QLocale::Script(m_data->m_script_id)); }
    [[nodiscard]] inline QLatin1StringView territoryCode() const
    { return territoryToCode(QLocale::Territory(m_data->m_territory_id)); }

    [[nodiscard]] static const QLocalePrivate *get(const QLocale &l) { return l.d; }
    [[nodiscard]] static QLatin1StringView
    languageToCode(QLocale::Language language,
                   QLocale::LanguageCodeTypes codeTypes = QLocale::AnyLanguageCode);
    [[nodiscard]] static QLatin1StringView scriptToCode(QLocale::Script script);
    [[nodiscard]] static QLatin1StringView territoryToCode(QLocale::Territory territory);
    [[nodiscard]] static QLocale::Language
    codeToLanguage(QStringView code,
                   QLocale::LanguageCodeTypes codeTypes = QLocale::AnyLanguageCode) noexcept;
    [[nodiscard]] static QLocale::Script codeToScript(QStringView code) noexcept;
    [[nodiscard]] static QLocale::Territory codeToTerritory(QStringView code) noexcept;

    [[nodiscard]] QLocale::MeasurementSystem measurementSystem() const;

    // System locale has an m_data all its own; all others have m_data = locale_data + m_index
    const QLocaleData *const m_data;
    QBasicAtomicInt ref;
    const qsizetype m_index;
    QLocale::NumberOptions m_numberOptions;

    static QBasicAtomicInt s_generation;
};

#ifndef QT_NO_SYSTEMLOCALE
qsizetype QSystemLocale::fallbackLocaleIndex() const { return fallbackLocale().d->m_index; }
#endif

template <>
inline QLocalePrivate *QSharedDataPointer<QLocalePrivate>::clone()
{
    // cannot use QLocalePrivate's copy constructor
    // since it is deleted in C++11
    return new QLocalePrivate(d->m_data, d->m_index, d->m_numberOptions);
}

inline char QLocaleData::numericToCLocale(QStringView in) const
{
    Q_ASSERT(in.size() == 1 || (in.size() == 2 && in.at(0).isHighSurrogate()));

    if (in == positiveSign() || in == u"+")
        return '+';

    if (in == negativeSign() || in == u"-" || in == u"\x2212")
        return '-';

    if (in == decimalPoint())
        return '.';

    if (in.compare(exponentSeparator(), Qt::CaseInsensitive) == 0)
        return 'e';

    const QString group = groupSeparator();
    if (in == group)
        return ',';

    // In several languages group() is a non-breaking space (U+00A0) or its thin
    // version (U+202f), which look like spaces.  People (and thus some of our
    // tests) use a regular space instead and complain if it doesn't work.
    // Should this be extended generally to any case where group is a space ?
    if ((group == u"\xa0" || group == u"\x202f") && in == u" ")
        return ',';

    const char32_t inUcs4 = in.size() == 2
        ? QChar::surrogateToUcs4(in.at(0), in.at(1)) : in.at(0).unicode();
    const char32_t zeroUcs4 = zeroUcs();
    // Must match qlocale_tools.h's unicodeForDigit()
    if (zeroUcs4 == u'\u3007') {
        // QTBUG-85409: Suzhou's digits aren't contiguous !
        if (inUcs4 == zeroUcs4)
            return '0';
        if (inUcs4 > u'\u3020' && inUcs4 <= u'\u3029')
            return inUcs4 - u'\u3020';
    } else if (zeroUcs4 <= inUcs4 && inUcs4 < zeroUcs4 + 10) {
        return '0' + inUcs4 - zeroUcs4;
    }
    if ('0' <= inUcs4 && inUcs4 <= '9')
        return inUcs4;

    return 0;
}

// Also used to merely skip over an escape in a format string, advancint idx to
// point after it (so not [[nodiscard]]):
QString qt_readEscapedFormatString(QStringView format, qsizetype *idx);
[[nodiscard]] bool qt_splitLocaleName(QStringView name, QStringView *lang = nullptr,
                                      QStringView *script = nullptr, QStringView *cntry = nullptr);
[[nodiscard]] qsizetype qt_repeatCount(QStringView s);

enum { AsciiSpaceMask = (1u << (' ' - 1)) |
                        (1u << ('\t' - 1)) |   // 9: HT - horizontal tab
                        (1u << ('\n' - 1)) |   // 10: LF - line feed
                        (1u << ('\v' - 1)) |   // 11: VT - vertical tab
                        (1u << ('\f' - 1)) |   // 12: FF - form feed
                        (1u << ('\r' - 1)) };  // 13: CR - carriage return
[[nodiscard]] constexpr inline bool ascii_isspace(uchar c)
{
    return c >= 1u && c <= 32u && (AsciiSpaceMask >> uint(c - 1)) & 1u;
}

QT_END_NAMESPACE

// ### move to qnamespace.h
QT_DECL_METATYPE_EXTERN_TAGGED(QList<Qt::DayOfWeek>, QList_Qt__DayOfWeek, Q_CORE_EXPORT)
#ifndef QT_NO_SYSTEMLOCALE
QT_DECL_METATYPE_EXTERN_TAGGED(QSystemLocale::CurrencyToStringArgument,
                               QSystemLocale__CurrencyToStringArgument, Q_CORE_EXPORT)
#endif

#endif // QLOCALE_P_H
