/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <QtCore/private/qglobal_p.h>
#include "QtCore/qstring.h"
#include "QtCore/qvarlengtharray.h"
#include "QtCore/qvariant.h"
#include "QtCore/qnumeric.h"
#include <QtCore/qcalendar.h>

#include "qlocale.h"

#include <limits>
#include <cmath>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SYSTEMLOCALE
struct QLocaleData;
class Q_CORE_EXPORT QSystemLocale
{
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
        CountryId, // uint
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
        MonthNameLong, // QString, in: int
        MonthNameShort, // QString, in: int
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
        NativeCountryName, // QString
        StandaloneMonthNameLong, // QString, in: int
        StandaloneMonthNameShort // QString, in: int
    };
    virtual QVariant query(QueryType type, QVariant in = QVariant()) const;
    virtual QLocale fallbackUiLocale() const;

    inline uint fallbackUiLocaleIndex() const;
private:
    QSystemLocale(bool);
    friend class QSystemLocaleSingleton;
};
Q_DECLARE_TYPEINFO(QSystemLocale::QueryType, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(QSystemLocale::CurrencyToStringArgument, Q_RELOCATABLE_TYPE);
#endif

#if QT_CONFIG(icu)
namespace QIcu {
    QString toUpper(const QByteArray &localeId, const QString &str, bool *ok);
    QString toLower(const QByteArray &localeId, const QString &str, bool *ok);
}
#endif


struct QLocaleId
{
    Q_CORE_EXPORT static QLocaleId fromName(const QString &name);
    inline bool operator==(QLocaleId other) const
    { return language_id == other.language_id && script_id == other.script_id && country_id == other.country_id; }
    inline bool operator!=(QLocaleId other) const
    { return !operator==(other); }
    inline bool isValid() const
    {
        return language_id <= QLocale::LastLanguage && script_id <= QLocale::LastScript
                && country_id <= QLocale::LastCountry;
    }
    inline bool matchesAll() const
    {
        return !language_id && !script_id && !country_id;
    }
    // Use as: filter.accept...(candidate)
    inline bool acceptLanguage(quint16 lang) const
    {
        // Always reject AnyLanguage (only used for last entry in locale_data array).
        // So, when searching for AnyLanguage, accept everything *but* AnyLanguage.
        return language_id ? lang == language_id : lang;
    }
    inline bool acceptScriptCountry(QLocaleId other) const
    {
        return (!country_id || other.country_id == country_id)
                && (!script_id || other.script_id == script_id);
    }

    QLocaleId withLikelySubtagsAdded() const;
    QLocaleId withLikelySubtagsRemoved() const;

    QByteArray name(char separator = '-') const;

    ushort language_id = 0, script_id = 0, country_id = 0;
};
Q_DECLARE_TYPEINFO(QLocaleId, Q_PRIMITIVE_TYPE);

struct QLocaleData
{
public:
    // Having an index for each locale enables us to have diverse sources of
    // data, e.g. calendar locales, as well as the main CLDR-derived data.
    static int findLocaleIndex(QLocaleId localeId);
    static const QLocaleData *c();

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

    QString decimalForm(QString &&digits, int decpt, int precision,
                        PrecisionMode pm, bool mustMarkDecimal,
                        bool groupDigits) const;
    QString exponentForm(QString &&digits, int decpt, int precision,
                         PrecisionMode pm, bool mustMarkDecimal,
                         int minExponentDigits) const;
    QString signPrefix(bool negative, unsigned flags) const;
    QString applyIntegerFormatting(QString &&numStr, bool negative, int precision,
                                   int base, int width, unsigned flags) const;

public:
    QString doubleToString(double d,
                           int precision = -1,
                           DoubleForm form = DFSignificantDigits,
                           int width = -1,
                           unsigned flags = NoFlags) const;
    QString longLongToString(qint64 l, int precision = -1,
                             int base = 10,
                             int width = -1,
                             unsigned flags = NoFlags) const;
    QString unsLongLongToString(quint64 l, int precision = -1,
                                int base = 10,
                                int width = -1,
                                unsigned flags = NoFlags) const;

    // this function is meant to be called with the result of stringToDouble or bytearrayToDouble
    static float convertDoubleToFloat(double d, bool *ok)
    {
        if (qIsInf(d))
            return float(d);
        if (std::fabs(d) > std::numeric_limits<float>::max()) {
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

    double stringToDouble(QStringView str, bool *ok, QLocale::NumberOptions options) const;
    qint64 stringToLongLong(QStringView str, int base, bool *ok, QLocale::NumberOptions options) const;
    quint64 stringToUnsLongLong(QStringView str, int base, bool *ok, QLocale::NumberOptions options) const;

    // this function is used in QIntValidator (QtGui)
    Q_CORE_EXPORT static qint64 bytearrayToLongLong(const char *num, int base, bool *ok);
    static quint64 bytearrayToUnsLongLong(const char *num, int base, bool *ok);

    bool numberToCLocale(QStringView s, QLocale::NumberOptions number_options,
                         CharBuff *result) const;
    inline char numericToCLocale(QStringView in) const;

    // this function is used in QIntValidator (QtGui)
    Q_CORE_EXPORT bool validateChars(QStringView str, NumberMode numMode, QByteArray *buff, int decDigits = -1,
            QLocale::NumberOptions number_options = QLocale::DefaultNumberOptions) const;

    // Access to assorted data members:
    QLocaleId id() const { return QLocaleId { m_language_id, m_script_id, m_country_id }; }

    QString decimalPoint() const;
    QString groupSeparator() const;
    QString listSeparator() const;
    QString percentSign() const;
    QString zeroDigit() const;
    char32_t zeroUcs() const;
    QString positiveSign() const;
    QString negativeSign() const;
    QString exponentSeparator() const;

    struct DataRange
    {
        quint16 offset;
        quint16 size;
        QString getData(const char16_t *table) const
        {
            return size > 0
                ? QString::fromRawData(reinterpret_cast<const QChar *>(table + offset), size)
                : QString();
        }
        QStringView viewData(const char16_t *table) const
        {
            return { reinterpret_cast<const QChar *>(table + offset), size };
        }
        QString getListEntry(const char16_t *table, int index) const
        {
            return listEntry(table, index).getData(table);
        }
        QStringView viewListEntry(const char16_t *table, int index) const
        {
            return listEntry(table, index).viewData(table);
        }
        char32_t ucsFirst(const char16_t *table) const
        {
            if (size && !QChar::isSurrogate(table[offset]))
                return table[offset];
            if (size > 1 && QChar::isHighSurrogate(table[offset]))
                return QChar::surrogateToUcs4(table[offset], table[offset + 1]);
            return 0;
        }
    private:
        DataRange listEntry(const char16_t *table, int index) const
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
    X(endonymLanguage) X(endonymCountry)

#define rangeGetter(name) \
    DataRange name() const { return { m_ ## name ## _idx, m_ ## name ## _size }; }
    ForEachQLocaleRange(rangeGetter)
#undef rangeGetter

public:
    quint16 m_language_id, m_script_id, m_country_id;

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

class Q_CORE_EXPORT QLocalePrivate
{
public:
    constexpr QLocalePrivate(const QLocaleData *data, const uint index,
                             QLocale::NumberOptions numberOptions = QLocale::DefaultNumberOptions,
                             int refs = 0)
        : m_data(data), ref Q_BASIC_ATOMIC_INITIALIZER(refs),
          m_index(index), m_numberOptions(numberOptions) {}

    quint16 languageId() const { return m_data->m_language_id; }
    quint16 countryId() const { return m_data->m_country_id; }

    QByteArray bcp47Name(char separator = '-') const;
    QByteArray rawName(char separator = '-') const;

    inline QLatin1String languageCode() const { return languageToCode(QLocale::Language(m_data->m_language_id)); }
    inline QLatin1String scriptCode() const { return scriptToCode(QLocale::Script(m_data->m_script_id)); }
    inline QLatin1String countryCode() const { return countryToCode(QLocale::Country(m_data->m_country_id)); }

    static const QLocalePrivate *get(const QLocale &l) { return l.d; }
    static QLatin1String languageToCode(QLocale::Language language);
    static QLatin1String scriptToCode(QLocale::Script script);
    static QLatin1String countryToCode(QLocale::Country country);
    static QLocale::Language codeToLanguage(QStringView code) noexcept;
    static QLocale::Script codeToScript(QStringView code) noexcept;
    static QLocale::Country codeToCountry(QStringView code) noexcept;

    QLocale::MeasurementSystem measurementSystem() const;

    // System locale has an m_data all its own; all others have m_data = locale_data + m_index
    const QLocaleData *const m_data;
    QBasicAtomicInt ref;
    const uint m_index;
    QLocale::NumberOptions m_numberOptions;
};

#ifndef QT_NO_SYSTEMLOCALE
uint QSystemLocale::fallbackUiLocaleIndex() const { return fallbackUiLocale().d->m_index; }
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

QString qt_readEscapedFormatString(QStringView format, int *idx);
bool qt_splitLocaleName(QStringView name, QStringView *lang = nullptr,
                        QStringView *script = nullptr, QStringView *cntry = nullptr);
int qt_repeatCount(QStringView s);

enum { AsciiSpaceMask = (1u << (' ' - 1)) |
                        (1u << ('\t' - 1)) |   // 9: HT - horizontal tab
                        (1u << ('\n' - 1)) |   // 10: LF - line feed
                        (1u << ('\v' - 1)) |   // 11: VT - vertical tab
                        (1u << ('\f' - 1)) |   // 12: FF - form feed
                        (1u << ('\r' - 1)) };  // 13: CR - carriage return
constexpr inline bool ascii_isspace(uchar c)
{
    return c >= 1u && c <= 32u && (AsciiSpaceMask >> uint(c - 1)) & 1u;
}

static_assert(ascii_isspace(' '));
static_assert(ascii_isspace('\t'));
static_assert(ascii_isspace('\n'));
static_assert(ascii_isspace('\v'));
static_assert(ascii_isspace('\f'));
static_assert(ascii_isspace('\r'));
static_assert(!ascii_isspace('\0'));
static_assert(!ascii_isspace('\a'));
static_assert(!ascii_isspace('a'));
static_assert(!ascii_isspace('\177'));
static_assert(!ascii_isspace(uchar('\200')));
static_assert(!ascii_isspace(uchar('\xA0')));
static_assert(!ascii_isspace(uchar('\377')));

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QStringView)
Q_DECLARE_METATYPE(QList<Qt::DayOfWeek>)
#ifndef QT_NO_SYSTEMLOCALE
Q_DECLARE_METATYPE(QSystemLocale::CurrencyToStringArgument)
#endif

#endif // QLOCALE_P_H
