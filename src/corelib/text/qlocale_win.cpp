// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qlocale_p.h"
#include "qlocale_tools_p.h"

#include "qstringlist.h"
#include "qvariant.h"
#include "qdatetime.h"
#include "qdebug.h"

#include "QtCore/private/qgregoriancalendar_p.h" // for yearSharingWeekDays()

#include <q20algorithm.h>

// TODO QTBUG-121193: port away from the use of LCID to always use names.
#include <qt_windows.h>
#include <time.h>

#if QT_CONFIG(cpp_winrt)
#   include <QtCore/private/qt_winrtbase_p.h>

#   include <winrt/Windows.Foundation.h>
#   include <winrt/Windows.Foundation.Collections.h>
#   include <winrt/Windows.System.UserProfile.h>
#endif // QT_CONFIG(cpp_winrt)

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// Shared interpretation of %LANG%
static auto scanLangEnv()
{
    struct R
    {
        QByteArray name; // empty means unknown; lookup from id may work
        LCID id = 0; // 0 means unknown; lookup from name may work
    };
    const QByteArray lang = qgetenv("LANG");
    if (lang.size() && (lang == "C" || qt_splitLocaleName(QString::fromLocal8Bit(lang)))) {
        // See if we have a Windows locale code instead of a locale name:
        const auto [id, used] = qstrntoll(lang.data(), lang.size(), 0);
        if (used > 0 && id && INT_MIN <= id && id <= INT_MAX)
            return R {QByteArray(), static_cast<LCID>(id)};
        return R {lang, 0};
    }
    return R{};
}

static auto getDefaultWinId()
{
    const auto [name, id] = scanLangEnv();
    if (id)
        return id;

    if (!name.isEmpty()) {
        LCID id = LocaleNameToLCID(static_cast<LPCWSTR>(
                                       QString::fromUtf8(name).toStdWString().data()), 0);
        if (id)
            return id;
    }

    return GetUserDefaultLCID();
}

static QByteArray getWinLocaleName(LCID id = LOCALE_USER_DEFAULT);

#ifndef QT_NO_SYSTEMLOCALE

#ifndef MUI_LANGUAGE_NAME
#define MUI_LANGUAGE_NAME 0x8
#endif
#ifndef LOCALE_SSHORTESTDAYNAME1
#  define LOCALE_SSHORTESTDAYNAME1 0x0060
#  define LOCALE_SSHORTESTDAYNAME2 0x0061
#  define LOCALE_SSHORTESTDAYNAME3 0x0062
#  define LOCALE_SSHORTESTDAYNAME4 0x0063
#  define LOCALE_SSHORTESTDAYNAME5 0x0064
#  define LOCALE_SSHORTESTDAYNAME6 0x0065
#  define LOCALE_SSHORTESTDAYNAME7 0x0066
#endif
#ifndef LOCALE_SNATIVELANGUAGENAME
#  define LOCALE_SNATIVELANGUAGENAME 0x00000004
#endif
#ifndef LOCALE_SNATIVECOUNTRYNAME
#  define LOCALE_SNATIVECOUNTRYNAME 0x00000008
#endif
#ifndef LOCALE_SSHORTTIME
#  define LOCALE_SSHORTTIME 0x00000079
#endif

namespace {
template <typename T>
static QVariant nullIfEmpty(T &&value)
{
    // For use where we should fall back to CLDR if we got an empty value.
    if (value.isEmpty())
        return {};
    return std::move(value);
}
}

struct QSystemLocalePrivate
{
    QSystemLocalePrivate();

    QVariant zeroDigit();
    QVariant decimalPoint();
    QVariant groupingSizes();
    QVariant groupSeparator();
    QVariant negativeSign();
    QVariant positiveSign();
    QVariant dateFormat(QLocale::FormatType);
    QVariant timeFormat(QLocale::FormatType);
    QVariant dateTimeFormat(QLocale::FormatType);
    QVariant dayName(int, QLocale::FormatType);
    QVariant standaloneMonthName(int, QLocale::FormatType);
    QVariant monthName(int, QLocale::FormatType);
    QVariant toString(QDate, QLocale::FormatType);
    QVariant toString(QTime, QLocale::FormatType);
    QVariant toString(const QDateTime &, QLocale::FormatType);
    QVariant measurementSystem();
    QVariant collation();
    QVariant amText();
    QVariant pmText();
    QVariant firstDayOfWeek();
    QVariant currencySymbol(QLocale::CurrencySymbolFormat);
    QVariant toCurrencyString(const QSystemLocale::CurrencyToStringArgument &);
    QVariant uiLanguages();
    QVariant nativeLanguageName();
    QVariant nativeTerritoryName();

    void update();

private:
    enum SubstitutionType {
        SUnknown,
        SContext,
        SAlways,
        SNever
    };

    // cached values:
    LCID lcid;
    SubstitutionType substitutionType = SUnknown;
    QString zero; // cached value for zeroDigit()
    QLocaleData::GroupSizes sizes; // cached value for groupingSizes()

    int getLocaleInfo(LCTYPE type, LPWSTR data, int size);
    QVariant getLocaleInfo(LCTYPE type);
    int getLocaleInfo_int(LCTYPE type);

    int getCurrencyFormat(DWORD flags, LPCWSTR value, const CURRENCYFMTW *format, LPWSTR data, int size);
    int getDateFormat(DWORD flags, const SYSTEMTIME * date, LPCWSTR format, LPWSTR data, int size);
    int getTimeFormat(DWORD flags, const SYSTEMTIME *date, LPCWSTR format, LPWSTR data, int size);

    SubstitutionType substitution();
    QString substituteDigits(QString &&string);
    QString correctDigits(QString &&string);
    QString yearFix(int year, int fakeYear, QString &&formatted);

    static QString winToQtFormat(QStringView sys_fmt);

};
Q_GLOBAL_STATIC(QSystemLocalePrivate, systemLocalePrivate)

QSystemLocalePrivate::QSystemLocalePrivate()
    : lcid(getDefaultWinId())
{
}

inline int QSystemLocalePrivate::getCurrencyFormat(DWORD flags, LPCWSTR value, const CURRENCYFMTW *format, LPWSTR data, int size)
{
    return GetCurrencyFormat(lcid, flags, value, format, data, size);
}

inline int QSystemLocalePrivate::getDateFormat(DWORD flags, const SYSTEMTIME * date, LPCWSTR format, LPWSTR data, int size)
{
    return GetDateFormat(lcid, flags, date, format, data, size);
}

inline int QSystemLocalePrivate::getTimeFormat(DWORD flags, const SYSTEMTIME *date, LPCWSTR format, LPWSTR data, int size)
{
    return GetTimeFormat(lcid, flags, date, format, data, size);
}

inline int QSystemLocalePrivate::getLocaleInfo(LCTYPE type, LPWSTR data, int size)
{
    return GetLocaleInfo(lcid, type, data, size);
}

QVariant QSystemLocalePrivate::getLocaleInfo(LCTYPE type)
{
    // https://docs.microsoft.com/en-us/windows/win32/intl/locale-spositivesign
    // says empty for LOCALE_SPOSITIVESIGN means "+", although GetLocaleInfo()
    // is documented to return 0 only on failure, so it's not clear how it
    // returns empty to mean this; hence the two checks for it below.
    const QString plus = QStringLiteral("+");
    QVarLengthArray<wchar_t, 64> buf(64);
    // Need to distinguish empty QString packaged as (non-null) QVariant from null QVariant:
    if (!getLocaleInfo(type, buf.data(), buf.size())) {
        const auto lastError = GetLastError();
        if (type == LOCALE_SPOSITIVESIGN && lastError == ERROR_SUCCESS)
            return plus;
        if (lastError != ERROR_INSUFFICIENT_BUFFER)
            return {};
        int cnt = getLocaleInfo(type, 0, 0);
        if (cnt == 0)
            return {};
        buf.resize(cnt);
        if (!getLocaleInfo(type, buf.data(), buf.size()))
            return {};
    }
    if (type == LOCALE_SPOSITIVESIGN && !buf[0])
        return plus;
    return QString::fromWCharArray(buf.data());
}

int QSystemLocalePrivate::getLocaleInfo_int(LCTYPE type)
{
    DWORD value;
    int r = GetLocaleInfo(lcid, type | LOCALE_RETURN_NUMBER,
                          reinterpret_cast<wchar_t *>(&value),
                          sizeof(value) / sizeof(wchar_t));
    return r == sizeof(value) / sizeof(wchar_t) ? value : 0;
}

QSystemLocalePrivate::SubstitutionType QSystemLocalePrivate::substitution()
{
    if (substitutionType == SUnknown) {
        wchar_t buf[8];
        if (!getLocaleInfo(LOCALE_IDIGITSUBSTITUTION, buf, 8)) {
            substitutionType = SNever;
            return substitutionType;
        }
        if (buf[0] == '1')
            substitutionType = SNever;
        else if (buf[0] == '0')
            substitutionType = SContext;
        else if (buf[0] == '2')
            substitutionType = SAlways;
        else {
            wchar_t digits[11]; // See zeroDigit() for why 11.
            if (!getLocaleInfo(LOCALE_SNATIVEDIGITS, digits, 11)) {
                substitutionType = SNever;
                return substitutionType;
            }
            if (buf[0] == digits[0] + 2)
                substitutionType = SAlways;
            else
                substitutionType = SNever;
        }
    }
    return substitutionType;
}

QString QSystemLocalePrivate::substituteDigits(QString &&string)
{
    zeroDigit(); // Ensure zero is set.
    switch (zero.size()) {
    case 1: {
        ushort z = zero.at(0).unicode();
        if (z == '0') // Nothing to do
            break;
        Q_ASSERT(z > '9');
        ushort *const qch = reinterpret_cast<ushort *>(string.data());
        for (qsizetype i = 0, stop = string.size(); i < stop; ++i) {
            ushort &ch = qch[i];
            if (ch >= '0' && ch <= '9')
                ch = unicodeForDigit(ch - '0', z);
        }
        break;
    }
    case 2: {
        // Surrogate pair (high, low):
        char32_t z = QChar::surrogateToUcs4(zero.at(0), zero.at(1));
        for (int i = 0; i < 10; i++) {
            char32_t digit = unicodeForDigit(i, z);
            const QChar s[2] = { QChar::highSurrogate(digit), QChar::lowSurrogate(digit) };
            string.replace(QString(QLatin1Char('0' + i)), QString(s, 2));
        }
        break;
    }
    default:
        Q_ASSERT(!"Expected zero digit to be a single UCS2 code-point or a surrogate pair");
    case 0: // Apparently this locale info was not available.
        break;
    }
    return std::move(string);
}

QString QSystemLocalePrivate::correctDigits(QString &&string)
{
    return substitution() == SAlways ? substituteDigits(std::move(string)) : std::move(string);
}

QVariant QSystemLocalePrivate::zeroDigit()
{
    if (zero.isEmpty()) {
        /* Ten digits plus a terminator.

           https://docs.microsoft.com/en-us/windows/win32/intl/locale-snative-constants
           "Native equivalents of ASCII 0 through 9. The maximum number of
           characters allowed for this string is eleven, including a terminating
           null character."
         */
        wchar_t digits[11];
        if (getLocaleInfo(LOCALE_SNATIVEDIGITS, digits, 11)) {
            // assert all(digits[i] == i + digits[0] for i in range(1, 10)),
            // assumed above (unless digits[0] is 0x3007; see QTBUG-85409).
            zero = QString::fromWCharArray(digits, 1);
        }
    }
    return nullIfEmpty(zero); // Do not std::move().
}

QVariant QSystemLocalePrivate::decimalPoint()
{
    return nullIfEmpty(getLocaleInfo(LOCALE_SDECIMAL).toString());
}

QVariant QSystemLocalePrivate::groupingSizes()
{
    if (sizes.higher == 0) {
        wchar_t grouping[10];
        /*
         * Nine digits/semicolons plus a terminator.

         * https://learn.microsoft.com/en-us/windows/win32/intl/locale-sgrouping
         * "Sizes for each group of digits to the left of the decimal. The maximum
         * number of characters allowed for this string is ten, including a
         * terminating null character."
         */
        int dataSize = getLocaleInfo(LOCALE_SGROUPING, grouping, int(std::size(grouping)));
        if (dataSize) {
            // MS does not seem to include {first} so it will always be NAN.
            QString sysGroupingStr = QString::fromWCharArray(grouping, dataSize);
            auto tokenized = sysGroupingStr.tokenize(u";");
            int width[2] = {0, 0};
            int index = 0;
            for (const auto tok : tokenized) {
                bool ok = false;
                int value = tok.toInt(&ok);
                if (!ok || !value || index >= 2)
                    break;
                width[index++] = value;
            }
            // The MS docs allow patterns Qt doesn't support, so we treat "X;Y" as "X;Y;0"
            // and "X" as "X;0" and ignore all but the first two widths. The MS API does
            // not support an equivalent of sizes.first.
            if (index > 1) {
                sizes.least = width[0];
                sizes.higher = width[1];
            } else if (index) {
                sizes.least = sizes.higher = width[0];
            }
        }
    }
    return QVariant::fromValue(sizes);
}

QVariant QSystemLocalePrivate::groupSeparator()
{
    return getLocaleInfo(LOCALE_STHOUSAND); // Empty means don't group digits.
}

QVariant QSystemLocalePrivate::negativeSign()
{
    return nullIfEmpty(getLocaleInfo(LOCALE_SNEGATIVESIGN).toString());
}

QVariant QSystemLocalePrivate::positiveSign()
{
    return nullIfEmpty(getLocaleInfo(LOCALE_SPOSITIVESIGN).toString());
}

QVariant QSystemLocalePrivate::dateFormat(QLocale::FormatType type)
{
    switch (type) {
    case QLocale::ShortFormat:
        return nullIfEmpty(winToQtFormat(getLocaleInfo(LOCALE_SSHORTDATE).toString()));
    case QLocale::LongFormat:
        return nullIfEmpty(winToQtFormat(getLocaleInfo(LOCALE_SLONGDATE).toString()));
    case QLocale::NarrowFormat:
        break;
    }
    return QVariant();
}

QVariant QSystemLocalePrivate::timeFormat(QLocale::FormatType type)
{
    switch (type) {
    case QLocale::ShortFormat:
        return nullIfEmpty(winToQtFormat(getLocaleInfo(LOCALE_SSHORTTIME).toString()));
    case QLocale::LongFormat:
        return nullIfEmpty(winToQtFormat(getLocaleInfo(LOCALE_STIMEFORMAT).toString()));
    case QLocale::NarrowFormat:
        break;
    }
    return {};
}

QVariant QSystemLocalePrivate::dateTimeFormat(QLocale::FormatType type)
{
    QVariant d = dateFormat(type), t = timeFormat(type);
    if (d.typeId() == QMetaType::QString && t.typeId() == QMetaType::QString)
        return QString(d.toString() + u' ' + t.toString());
    return {};
}

QVariant QSystemLocalePrivate::dayName(int day, QLocale::FormatType type)
{
    if (day < 1 || day > 7)
        return {};

    static constexpr LCTYPE short_day_map[]
        = { LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2,
            LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5,
            LOCALE_SABBREVDAYNAME6, LOCALE_SABBREVDAYNAME7 };

    static constexpr LCTYPE long_day_map[]
        = { LOCALE_SDAYNAME1, LOCALE_SDAYNAME2,
            LOCALE_SDAYNAME3, LOCALE_SDAYNAME4, LOCALE_SDAYNAME5,
            LOCALE_SDAYNAME6, LOCALE_SDAYNAME7 };

    static constexpr LCTYPE narrow_day_map[]
        = { LOCALE_SSHORTESTDAYNAME1, LOCALE_SSHORTESTDAYNAME2,
            LOCALE_SSHORTESTDAYNAME3, LOCALE_SSHORTESTDAYNAME4,
            LOCALE_SSHORTESTDAYNAME5, LOCALE_SSHORTESTDAYNAME6,
            LOCALE_SSHORTESTDAYNAME7 };

    return nullIfEmpty(getLocaleInfo(
                           (type == QLocale::LongFormat ? long_day_map
                            : type == QLocale::NarrowFormat ? narrow_day_map
                            : short_day_map)[day - 1]).toString());
}

QVariant QSystemLocalePrivate::standaloneMonthName(int month, QLocale::FormatType type)
{
    static constexpr LCTYPE short_month_map[]
        = { LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2, LOCALE_SABBREVMONTHNAME3,
            LOCALE_SABBREVMONTHNAME4, LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
            LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8, LOCALE_SABBREVMONTHNAME9,
            LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12 };

    static constexpr LCTYPE long_month_map[]
        = { LOCALE_SMONTHNAME1, LOCALE_SMONTHNAME2, LOCALE_SMONTHNAME3,
            LOCALE_SMONTHNAME4, LOCALE_SMONTHNAME5, LOCALE_SMONTHNAME6,
            LOCALE_SMONTHNAME7, LOCALE_SMONTHNAME8, LOCALE_SMONTHNAME9,
            LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12 };

    if (month < 1 || month > 12)
        return {};

    // Month is Jan = 1, ... Dec = 12; adjust by 1 to match array indexing from 0:
    return nullIfEmpty(getLocaleInfo(
        (type == QLocale::LongFormat ? long_month_map : short_month_map)[month - 1]).toString());
}

QVariant QSystemLocalePrivate::monthName(int month, QLocale::FormatType type)
{
    SYSTEMTIME st = {};
    st.wYear = 2001;
    st.wMonth = month;
    st.wDay = 10;

    const DWORD flags{}; // Must be clear when passing a format string.
    // MS's docs for the LOCALE_SMONTHNAME* say to include the day in a format.
    // Educated guess: this works for the LOCALE_SABBREVMONTHNAME*, too, in so
    // far as the abbreviated plain name might differ from abbreviated
    // standalone one.
    const wchar_t *const format = type == QLocale::LongFormat ? L"ddMMMM" : L"ddMMM";
    wchar_t buf[255];
    if (getDateFormat(flags, &st, format, buf, 255) > 2) {
        // Elide the two digits of day number
        return nullIfEmpty(correctDigits(QString::fromWCharArray(buf + 2)));
    }
    return {};
}

static QString fourDigitYear(int year)
{
    // Return year formatted as an (at least) four digit number:
    return QStringLiteral("%1").arg(year, 4, 10, QChar(u'0'));
}

QString QSystemLocalePrivate::yearFix(int year, int fakeYear, QString &&formatted)
{
    // Replace our ersatz fakeYear (that MS formats faithfully) with the correct
    // form of year.  We know the two-digit short form of fakeYear can not be
    // mistaken for the month or day-of-month in the formatted date.
    Q_ASSERT(fakeYear >= 1970 && fakeYear <= 2400);
    const bool matchTwo = year >= 0 && year % 100 == fakeYear % 100;
    auto yearUsed = fourDigitYear(fakeYear);
    QString sign(year < 0 ? 1 : 0, u'-');
    auto trueYear = fourDigitYear(year < 0 ? -year : year);
    if (formatted.contains(yearUsed))
        return std::move(formatted).replace(yearUsed, sign + trueYear);

    auto tail = QStringView{yearUsed}.last(2);
    Q_ASSERT(!matchTwo || tail == QString(sign + trueYear.last(2)));
    if (formatted.contains(tail)) {
        if (matchTwo)
            return std::move(formatted);
        return std::move(formatted).replace(tail.toString(), sign + trueYear.last(2));
    }

    // Localized digits (regardless of SAlways), perhaps ?
    // First call to substituteDigits() ensures zero is initialized:
    trueYear = substituteDigits(std::move(trueYear));
    if (zero != u'0') {
        yearUsed = substituteDigits(std::move(yearUsed));
        if (year < 0)
            sign = negativeSign().toString();

        if (formatted.contains(yearUsed))
            return std::move(formatted).replace(yearUsed, sign + trueYear);

        const qsizetype twoDigits = 2 * zero.size();
        tail = QStringView{yearUsed}.last(twoDigits);
        if (formatted.contains(tail)) {
            if (matchTwo)
                return std::move(formatted);
            return std::move(formatted).replace(tail.toString(), sign + trueYear.last(twoDigits));
        }
    }
    qWarning("Failed to fix up year in formatted date-string using %d for %d", fakeYear, year);
    return std::move(formatted);
}

QVariant QSystemLocalePrivate::toString(QDate date, QLocale::FormatType type)
{
    SYSTEMTIME st = {};
    const int year = date.year();
    // st.wYear is unsigned; and GetDateFormat() is documented to not handle
    // dates before 1601.
    const bool fixup = year < 1601;
    st.wYear = fixup ? QGregorianCalendar::yearSharingWeekDays(date) : year;
    st.wMonth = date.month();
    st.wDay = date.day();

    Q_ASSERT(!fixup || st.wYear % 100 != st.wMonth);
    Q_ASSERT(!fixup || st.wYear % 100 != st.wDay);
    // i.e. yearFix() can trust a match of its fakeYear's last two digits to not
    // be the month or day part of the formatted date.

    DWORD flags = (type == QLocale::LongFormat ? DATE_LONGDATE : DATE_SHORTDATE);
    wchar_t buf[255];
    if (getDateFormat(flags, &st, NULL, buf, 255)) {
        QString text = QString::fromWCharArray(buf);
        if (fixup)
            text = yearFix(year, st.wYear, std::move(text));
        return nullIfEmpty(correctDigits(std::move(text)));
    }
    return {};
}

QVariant QSystemLocalePrivate::toString(QTime time, QLocale::FormatType type)
{
    SYSTEMTIME st = {};
    st.wHour = time.hour();
    st.wMinute = time.minute();
    st.wSecond = time.second();
    st.wMilliseconds = 0;

    DWORD flags = 0;
    // keep the same conditional as timeFormat() above
    const QString format = type == QLocale::ShortFormat
        ? getLocaleInfo(LOCALE_SSHORTTIME).toString()
        : QString();
    auto formatStr = reinterpret_cast<const wchar_t *>(format.isEmpty() ? nullptr : format.utf16());

    wchar_t buf[255];
    if (getTimeFormat(flags, &st, formatStr, buf, int(std::size(buf))))
        return nullIfEmpty(correctDigits(QString::fromWCharArray(buf)));
    return {};
}

QVariant QSystemLocalePrivate::toString(const QDateTime &dt, QLocale::FormatType type)
{
    QVariant d = toString(dt.date(), type), t = toString(dt.time(), type);
    if (d.typeId() == QMetaType::QString && t.typeId() == QMetaType::QString)
        return QString(d.toString() + u' ' + t.toString());
    return {};
}

QVariant QSystemLocalePrivate::measurementSystem()
{
    wchar_t output[2];

    if (getLocaleInfo(LOCALE_IMEASURE, output, 2)) {
        if (output[0] == L'1' && !output[1])
            return QLocale::ImperialSystem;
    }

    return QLocale::MetricSystem;
}

QVariant QSystemLocalePrivate::collation()
{
    return getLocaleInfo(LOCALE_SSORTLOCALE);
}

QVariant QSystemLocalePrivate::amText()
{
    wchar_t output[15]; // maximum length including  terminating zero character for Win2003+

    if (getLocaleInfo(LOCALE_S1159, output, 15))
        return nullIfEmpty(QString::fromWCharArray(output));

    return QVariant();
}

QVariant QSystemLocalePrivate::pmText()
{
    wchar_t output[15]; // maximum length including  terminating zero character for Win2003+

    if (getLocaleInfo(LOCALE_S2359, output, 15))
        return nullIfEmpty(QString::fromWCharArray(output));

    return QVariant();
}

QVariant QSystemLocalePrivate::firstDayOfWeek()
{
    wchar_t output[4]; // maximum length including  terminating zero character for Win2003+

    if (getLocaleInfo(LOCALE_IFIRSTDAYOFWEEK, output, 4))
        return QString::fromWCharArray(output).toUInt()+1;

    return 1;
}

QVariant QSystemLocalePrivate::currencySymbol(QLocale::CurrencySymbolFormat format)
{
    wchar_t buf[13];
    switch (format) {
    case QLocale::CurrencySymbol:
        // Some locales do have empty currency symbol. All the same, fall back
        // to CLDR for confirmation if MS claims that applies.
        if (getLocaleInfo(LOCALE_SCURRENCY, buf, 13))
            return nullIfEmpty(QString::fromWCharArray(buf));
        break;
    case QLocale::CurrencyIsoCode:
        if (getLocaleInfo(LOCALE_SINTLSYMBOL, buf, 9))
            return nullIfEmpty(QString::fromWCharArray(buf));
        break;
    case QLocale::CurrencyDisplayName: {
        QVarLengthArray<wchar_t, 64> buf(64);
        if (!getLocaleInfo(LOCALE_SNATIVECURRNAME, buf.data(), buf.size())) {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                break;
            buf.resize(255); // should be large enough, right?
            if (!getLocaleInfo(LOCALE_SNATIVECURRNAME, buf.data(), buf.size()))
                break;
        }
        return nullIfEmpty(QString::fromWCharArray(buf.data()));
    }
    default:
        break;
    }
    return QVariant();
}

QVariant QSystemLocalePrivate::toCurrencyString(const QSystemLocale::CurrencyToStringArgument &arg)
{
    QString value;
    switch (arg.value.typeId()) {
    case QMetaType::Int:
        value = QLocaleData::c()->longLongToString(
            arg.value.toInt(), -1, 10, -1, QLocale::OmitGroupSeparator);
        break;
    case QMetaType::UInt:
        value = QLocaleData::c()->unsLongLongToString(
            arg.value.toUInt(), -1, 10, -1, QLocale::OmitGroupSeparator);
        break;
    case QMetaType::Double:
        value = QLocaleData::c()->doubleToString(
            arg.value.toDouble(), -1, QLocaleData::DFDecimal, -1, QLocale::OmitGroupSeparator);
        break;
    case QMetaType::LongLong:
        value = QLocaleData::c()->longLongToString(
            arg.value.toLongLong(), -1, 10, -1, QLocale::OmitGroupSeparator);
        break;
    case QMetaType::ULongLong:
        value = QLocaleData::c()->unsLongLongToString(
            arg.value.toULongLong(), -1, 10, -1, QLocale::OmitGroupSeparator);
        break;
    default:
        return QVariant();
    }

    QVarLengthArray<wchar_t, 64> out(64);

    QString decimalSep;
    QString thousandSep;
    CURRENCYFMT format;
    CURRENCYFMT *pformat = NULL;
    if (!arg.symbol.isEmpty()) {
        format.NumDigits = getLocaleInfo_int(LOCALE_ICURRDIGITS);
        format.LeadingZero = getLocaleInfo_int(LOCALE_ILZERO);
        decimalSep = getLocaleInfo(LOCALE_SMONDECIMALSEP).toString();
        format.lpDecimalSep = reinterpret_cast<wchar_t *>(decimalSep.data());
        thousandSep = getLocaleInfo(LOCALE_SMONTHOUSANDSEP).toString();
        format.lpThousandSep = reinterpret_cast<wchar_t *>(thousandSep.data());
        format.NegativeOrder = getLocaleInfo_int(LOCALE_INEGCURR);
        format.PositiveOrder = getLocaleInfo_int(LOCALE_ICURRENCY);
        format.lpCurrencySymbol = (wchar_t *)arg.symbol.utf16();

        // grouping is complicated and ugly:
        // int(0)  == "123456789.00"    == string("0")
        // int(3)  == "123,456,789.00"  == string("3;0")
        // int(30) == "123456,789.00"   == string("3;0;0")
        // int(32) == "12,34,56,789.00" == string("3;2;0")
        // int(320)== "1234,56,789.00"  == string("3;2")
        QString groupingStr = getLocaleInfo(LOCALE_SMONGROUPING).toString();
        format.Grouping = groupingStr.remove(u';').toInt();
        if (format.Grouping % 10 == 0) // magic
            format.Grouping /= 10;
        else
            format.Grouping *= 10;
        pformat = &format;
    }

    int ret = getCurrencyFormat(0, reinterpret_cast<const wchar_t *>(value.utf16()),
                                  pformat, out.data(), out.size());
    if (ret == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        ret = getCurrencyFormat(0, reinterpret_cast<const wchar_t *>(value.utf16()),
                                  pformat, out.data(), 0);
        out.resize(ret);
        getCurrencyFormat(0, reinterpret_cast<const wchar_t *>(value.utf16()),
                            pformat, out.data(), out.size());
    }

    return nullIfEmpty(correctDigits(QString::fromWCharArray(out.data())));
}

QVariant QSystemLocalePrivate::uiLanguages()
{
    QStringList result;
#if QT_CONFIG(cpp_winrt)
    using namespace winrt::Windows::System::UserProfile;
    QT_TRY {
        auto languages = GlobalizationPreferences::Languages();
        for (const auto &lang : languages)
            result << QString::fromStdString(winrt::to_string(lang));
    } QT_CATCH(...) {
        // pass, just fall back to WIN32 API implementation
    }
    if (!result.isEmpty())
        return result; // else just fall back to WIN32 API implementation
#endif // QT_CONFIG(cpp_winrt)
    // mingw and clang still have to use Win32 API
    unsigned long cnt = 0;
    QVarLengthArray<wchar_t, 64> buf(64);
#    if !defined(QT_BOOTSTRAPPED) // Not present in MinGW 4.9/bootstrap builds.
    unsigned long size = buf.size();
    if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &cnt, buf.data(), &size)) {
        size = 0;
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &cnt, NULL, &size)) {
            buf.resize(size);
            if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &cnt, buf.data(), &size))
                return {};
        }
    }
#    endif // !QT_BOOTSTRAPPED
    result.reserve(cnt);
    const wchar_t *str = buf.constData();
    for (; cnt > 0; --cnt) {
        QString s = QString::fromWCharArray(str);
        if (s.isEmpty())
            break; // something is wrong
        result.append(s);
        str += s.size() + 1;
    }
    return nullIfEmpty(std::move(result));
}

QVariant QSystemLocalePrivate::nativeLanguageName()
{
    return getLocaleInfo(LOCALE_SNATIVELANGUAGENAME);
}

QVariant QSystemLocalePrivate::nativeTerritoryName()
{
    return getLocaleInfo(LOCALE_SNATIVECOUNTRYNAME);
}


void QSystemLocalePrivate::update()
{
    lcid = getDefaultWinId();
    substitutionType = SUnknown;
    zero.resize(0);
}

QString QSystemLocalePrivate::winToQtFormat(QStringView sys_fmt)
{
    QString result;
    qsizetype i = 0;

    while (i < sys_fmt.size()) {
        if (sys_fmt.at(i).unicode() == u'\'') {
            QString text = qt_readEscapedFormatString(sys_fmt, &i);
            if (text == "'"_L1)
                result += "''"_L1;
            else
                result += u'\'' + text + u'\'';
            continue;
        }

        QChar c = sys_fmt.at(i);
        qsizetype repeat = qt_repeatCount(sys_fmt.mid(i));

        switch (c.unicode()) {
            // Date
            case 'y':
                if (repeat > 5)
                    repeat = 5;
                else if (repeat == 3)
                    repeat = 2;
                switch (repeat) {
                    case 1:
                        result += "yy"_L1; // "y" unsupported by Qt, use "yy"
                        break;
                    case 5:
                        result += "yyyy"_L1; // "yyyyy" same as "yyyy" on Windows
                        break;
                    default:
                        result += QString(repeat, u'y');
                        break;
                }
                break;
            case 'g':
                if (repeat > 2)
                    repeat = 2;
                switch (repeat) {
                    case 2:
                        break; // no equivalent of "gg" in Qt
                    default:
                        result += u'g';
                        break;
                }
                break;
            case 't':
                if (repeat > 2)
                    repeat = 2;
                result += "AP"_L1; // "t" unsupported, use "AP"
                break;
            default:
                result += QString(repeat, c);
                break;
        }

        i += repeat;
    }

    return result;
}

QLocale QSystemLocale::fallbackLocale() const
{
    return QLocale(QString::fromLatin1(getWinLocaleName()));
}

QVariant QSystemLocale::query(QueryType type, QVariant &&in) const
{
    QSystemLocalePrivate *d = systemLocalePrivate();
    if (!d)
        return QVariant();
    switch(type) {
    case DecimalPoint:
        return d->decimalPoint();
    case Grouping:
        return d->groupingSizes();
    case GroupSeparator:
        return d->groupSeparator();
    case NegativeSign:
        return d->negativeSign();
    case PositiveSign:
        return d->positiveSign();
    case DateFormatLong:
        return d->dateFormat(QLocale::LongFormat);
    case DateFormatShort:
        return d->dateFormat(QLocale::ShortFormat);
    case TimeFormatLong:
        return d->timeFormat(QLocale::LongFormat);
    case TimeFormatShort:
        return d->timeFormat(QLocale::ShortFormat);
    case DateTimeFormatLong:
        return d->dateTimeFormat(QLocale::LongFormat);
    case DateTimeFormatShort:
        return d->dateTimeFormat(QLocale::ShortFormat);
    case DayNameLong:
        return d->dayName(in.toInt(), QLocale::LongFormat);
    case DayNameShort:
        return d->dayName(in.toInt(), QLocale::ShortFormat);
    case DayNameNarrow:
        return d->dayName(in.toInt(), QLocale::NarrowFormat);
    case StandaloneDayNameLong:
    case StandaloneDayNameShort:
    case StandaloneDayNameNarrow:
        // Windows does not provide standalone day names, so fall back to CLDR
        return QVariant();
    case MonthNameLong:
        return d->monthName(in.toInt(), QLocale::LongFormat);
    case StandaloneMonthNameLong:
        return d->standaloneMonthName(in.toInt(), QLocale::LongFormat);
    case MonthNameShort:
        return d->monthName(in.toInt(), QLocale::ShortFormat);
    case StandaloneMonthNameShort:
        return d->standaloneMonthName(in.toInt(), QLocale::ShortFormat);
    case MonthNameNarrow:
    case StandaloneMonthNameNarrow:
        // Windows provides no narrow month names, so we fall back to CLDR
        return QVariant();
    case DateToStringShort:
        return d->toString(in.toDate(), QLocale::ShortFormat);
    case DateToStringLong:
        return d->toString(in.toDate(), QLocale::LongFormat);
    case TimeToStringShort:
        return d->toString(in.toTime(), QLocale::ShortFormat);
    case TimeToStringLong:
        return d->toString(in.toTime(), QLocale::LongFormat);
    case DateTimeToStringShort:
        return d->toString(in.toDateTime(), QLocale::ShortFormat);
    case DateTimeToStringLong:
        return d->toString(in.toDateTime(), QLocale::LongFormat);
    case ZeroDigit:
        return d->zeroDigit();
    case LanguageId:
    case ScriptId:
    case TerritoryId: {
        QLocaleId lid = QLocaleId::fromName(QString::fromLatin1(getWinLocaleName()));
        if (type == LanguageId)
            return lid.language_id;
        if (type == ScriptId)
            return lid.script_id ? lid.script_id : ushort(fallbackLocale().script());
        return lid.territory_id ? lid.territory_id : ushort(fallbackLocale().territory());
    }
    case MeasurementSystem:
        return d->measurementSystem();
    case Collation:
        return d->collation();
    case AMText:
        return d->amText();
    case PMText:
        return d->pmText();
    case FirstDayOfWeek:
        return d->firstDayOfWeek();
    case CurrencySymbol:
        return d->currencySymbol(QLocale::CurrencySymbolFormat(in.toUInt()));
    case CurrencyToString:
        return d->toCurrencyString(in.value<CurrencyToStringArgument>());
    case UILanguages:
        return d->uiLanguages();
    case LocaleChanged:
        d->update();
        break;
    case NativeLanguageName:
        return d->nativeLanguageName();
    case NativeTerritoryName:
        return d->nativeTerritoryName();
    default:
        break;
    }
    return QVariant();
}
#endif // QT_NO_SYSTEMLOCALE

struct WindowsToISOListElt {
    ushort windows_code;
    char iso_name[6];
};

namespace {
struct ByWindowsCode {
    constexpr bool operator()(int lhs, WindowsToISOListElt rhs) const noexcept
    { return lhs < int(rhs.windows_code); }
    constexpr bool operator()(WindowsToISOListElt lhs, int rhs) const noexcept
    { return int(lhs.windows_code) < rhs; }
    constexpr bool operator()(WindowsToISOListElt lhs, WindowsToISOListElt rhs) const noexcept
    { return lhs.windows_code < rhs.windows_code; }
};
} // unnamed namespace

static constexpr WindowsToISOListElt windows_to_iso_list[] = {
    { 0x0401, "ar_SA" },
    { 0x0402, "bg"    },
    { 0x0403, "ca"    },
    { 0x0404, "zh_TW" },
    { 0x0405, "cs"    },
    { 0x0406, "da"    },
    { 0x0407, "de"    },
    { 0x0408, "el"    },
    { 0x0409, "en_US" },
    { 0x040a, "es"    },
    { 0x040b, "fi"    },
    { 0x040c, "fr"    },
    { 0x040d, "he"    },
    { 0x040e, "hu"    },
    { 0x040f, "is"    },
    { 0x0410, "it"    },
    { 0x0411, "ja"    },
    { 0x0412, "ko"    },
    { 0x0413, "nl"    },
    { 0x0414, "no"    },
    { 0x0414, "nb"    }, // alternative spelling; lower_bound will find the first one
    { 0x0415, "pl"    },
    { 0x0416, "pt_BR" },
    { 0x0418, "ro"    },
    { 0x0419, "ru"    },
    { 0x041a, "hr"    },
    { 0x041c, "sq"    },
    { 0x041d, "sv"    },
    { 0x041e, "th"    },
    { 0x041f, "tr"    },
    { 0x0420, "ur"    },
    { 0x0421, "in"    },
    { 0x0422, "uk"    },
    { 0x0423, "be"    },
    { 0x0425, "et"    },
    { 0x0426, "lv"    },
    { 0x0427, "lt"    },
    { 0x0429, "fa"    },
    { 0x042a, "vi"    },
    { 0x042d, "eu"    },
    { 0x042f, "mk"    },
    { 0x0436, "af"    },
    { 0x0438, "fo"    },
    { 0x0439, "hi"    },
    { 0x043e, "ms"    },
    { 0x0458, "mt"    },
    { 0x0801, "ar_IQ" },
    { 0x0804, "zh_CN" },
    { 0x0807, "de_CH" },
    { 0x0809, "en_GB" },
    { 0x080a, "es_MX" },
    { 0x080c, "fr_BE" },
    { 0x0810, "it_CH" },
    { 0x0812, "ko"    },
    { 0x0813, "nl_BE" },
    { 0x0814, "no"    },
    { 0x0814, "nn"    }, // alternative spelling; lower_bound will find the first one
    { 0x0816, "pt"    },
    { 0x081a, "sr"    },
    { 0x081d, "sv_FI" },
    { 0x0c01, "ar_EG" },
    { 0x0c04, "zh_HK" },
    { 0x0c07, "de_AT" },
    { 0x0c09, "en_AU" },
    { 0x0c0a, "es"    },
    { 0x0c0c, "fr_CA" },
    { 0x0c1a, "sr"    },
    { 0x1001, "ar_LY" },
    { 0x1004, "zh_SG" },
    { 0x1007, "de_LU" },
    { 0x1009, "en_CA" },
    { 0x100a, "es_GT" },
    { 0x100c, "fr_CH" },
    { 0x1401, "ar_DZ" },
    { 0x1407, "de_LI" },
    { 0x1409, "en_NZ" },
    { 0x140a, "es_CR" },
    { 0x140c, "fr_LU" },
    { 0x1801, "ar_MA" },
    { 0x1809, "en_IE" },
    { 0x180a, "es_PA" },
    { 0x1c01, "ar_TN" },
    { 0x1c09, "en_ZA" },
    { 0x1c0a, "es_DO" },
    { 0x2001, "ar_OM" },
    { 0x2009, "en_JM" },
    { 0x200a, "es_VE" },
    { 0x2401, "ar_YE" },
    { 0x2409, "en"    },
    { 0x240a, "es_CO" },
    { 0x2801, "ar_SY" },
    { 0x2809, "en_BZ" },
    { 0x280a, "es_PE" },
    { 0x2c01, "ar_JO" },
    { 0x2c09, "en_TT" },
    { 0x2c0a, "es_AR" },
    { 0x3001, "ar_LB" },
    { 0x300a, "es_EC" },
    { 0x3401, "ar_KW" },
    { 0x340a, "es_CL" },
    { 0x3801, "ar_AE" },
    { 0x380a, "es_UY" },
    { 0x3c01, "ar_BH" },
    { 0x3c0a, "es_PY" },
    { 0x4001, "ar_QA" },
    { 0x400a, "es_BO" },
    { 0x440a, "es_SV" },
    { 0x480a, "es_HN" },
    { 0x4c0a, "es_NI" },
    { 0x500a, "es_PR" }
};

static_assert(q20::is_sorted(std::begin(windows_to_iso_list), std::end(windows_to_iso_list),
                             ByWindowsCode{}));

static const char *winLangCodeToIsoName(int code)
{
    int cmp = code - windows_to_iso_list[0].windows_code;
    if (cmp < 0)
        return nullptr;

    if (cmp == 0)
        return windows_to_iso_list[0].iso_name;

    const auto it = std::lower_bound(std::begin(windows_to_iso_list),
                                     std::end(windows_to_iso_list),
                                     code,
                                     ByWindowsCode{});
    if (it != std::end(windows_to_iso_list) && !ByWindowsCode{}(code, *it))
        return it->iso_name;

    return nullptr;

}

LCID qt_inIsoNametoLCID(const char *name)
{
    if (!name)
        return LOCALE_USER_DEFAULT;
    if (std::strlen(name) >= sizeof(WindowsToISOListElt::iso_name))
        return LOCALE_USER_DEFAULT; // cannot possibly match (too long)

    // normalize separators:
    char n[sizeof(WindowsToISOListElt::iso_name)];
    // we know it will fit (we checked at the top of the function)
    strncpy(n, name, sizeof(n));
    char *c = n;
    while (*c) {
        if (*c == '-')
            *c = '_';
        ++c;
    }

    for (const WindowsToISOListElt &i : windows_to_iso_list) {
        if (!memcmp(n, i.iso_name, sizeof(WindowsToISOListElt::iso_name)))
            return i.windows_code;
    }
    return LOCALE_USER_DEFAULT;
}


static QString winIso639LangName(LCID id)
{
    QString result;

    // Windows returns the wrong ISO639 for some languages, we need to detect them here using
    // the language code
    QString lang_code;
    wchar_t out[256];
    if (GetLocaleInfo(id, LOCALE_ILANGUAGE, out, 255))
        lang_code = QString::fromWCharArray(out);

    if (!lang_code.isEmpty()) {
        const QByteArray latin1 = std::move(lang_code).toLatin1();
        const auto [i, used] = qstrntoull(latin1.data(), latin1.size(), 16);
        if (used >= latin1.size() || (used > 0 && latin1[used] == '\0')) {
            switch (i) {
                case 0x814:
                    result = u"nn"_s; // Nynorsk
                    break;
                default:
                    break;
            }
        }
    }

    if (!result.isEmpty())
        return result;

    // not one of the problematic languages - do the usual lookup
    if (GetLocaleInfo(id, LOCALE_SISO639LANGNAME, out, 255))
        result = QString::fromWCharArray(out);

    return result;
}

static QString winIso3116CtryName(LCID id)
{
    QString result;

    wchar_t out[256];
    if (GetLocaleInfo(id, LOCALE_SISO3166CTRYNAME, out, 255))
        result = QString::fromWCharArray(out);

    return result;
}

static QByteArray getWinLocaleName(LCID id)
{
    if (id == LOCALE_USER_DEFAULT) {
        const auto [name, lcid] = scanLangEnv();
        if (!name.isEmpty())
            return name;
        if (lcid)
            return winLangCodeToIsoName(lcid);

        id = GetUserDefaultLCID();
    }

    QString resultusage = winIso639LangName(id);
    QString country = winIso3116CtryName(id);
    if (!country.isEmpty())
        resultusage += u'_' + country;

    return std::move(resultusage).toLatin1();
}

// Helper for plugins/platforms/windows/
Q_CORE_EXPORT QLocale qt_localeFromLCID(LCID id)
{
    return QLocale(QString::fromLatin1(getWinLocaleName(id)));
}

#if !QT_CONFIG(icu)

static QString localeConvertString(const QString &localeID, const QString &str, bool *ok,
                                   DWORD flags)
{
    Q_ASSERT(ok);
    LCID lcid = LocaleNameToLCID(reinterpret_cast<const wchar_t *>(localeID.constData()), 0);
    // First compute the size of the output string
    const int size = LCMapStringW(lcid, flags, reinterpret_cast<const wchar_t *>(str.constData()),
                                  str.size(), 0, 0);
    QString buf(size, Qt::Uninitialized);
    if (lcid == 0 || size == 0
        || LCMapStringW(lcid, flags, reinterpret_cast<const wchar_t *>(str.constData()), str.size(),
                        reinterpret_cast<wchar_t *>(buf.data()), buf.size()) == 0) {
        *ok = false;
        return QString();
    }

    *ok = true;

    return buf;
}

QString QLocalePrivate::toLower(const QString &str, bool *ok) const
{
    return localeConvertString(QString::fromUtf8(bcp47Name()), str, ok,
                               LCMAP_LOWERCASE | LCMAP_LINGUISTIC_CASING);
}

QString QLocalePrivate::toUpper(const QString &str, bool *ok) const
{
    return localeConvertString(QString::fromUtf8(bcp47Name()), str, ok,
                               LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING);
}

#endif

QT_END_NAMESPACE
