// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qlocale_p.h"

#include "qstringlist.h"
#include "qvariant.h"
#include "qdatetime.h"

#include "private/qstringiterator_p.h"
#include "private/qgregoriancalendar_p.h"
#ifdef Q_OS_DARWIN
#include "private/qcore_mac_p.h"
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <QtCore/qloggingcategory.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/******************************************************************************
** Wrappers for Mac locale system functions
*/

Q_STATIC_LOGGING_CATEGORY(lcLocale, "qt.core.locale")

static void printLocalizationInformation()
{
    if (!lcLocale().isDebugEnabled())
        return;

#if defined(Q_OS_MACOS)
    // Trigger initialization of standard user defaults, so that Foundation picks
    // up -AppleLanguages and -AppleLocale passed on the command line.
    Q_UNUSED(NSUserDefaults.standardUserDefaults);
#endif

    auto singleLineDescription = [](NSArray *array) {
        NSString *str = [array description];
        str = [str stringByReplacingOccurrencesOfString:@"\n" withString:@""];
        return [str stringByReplacingOccurrencesOfString:@"    " withString:@""];
    };

    bool allowMixedLocalizations = [NSBundle.mainBundle.infoDictionary[@"CFBundleAllowMixedLocalizations"] boolValue];

    NSBundle *foundation = [NSBundle bundleForClass:NSBundle.class];
    qCDebug(lcLocale).nospace() << "Launched with locale \"" << NSLocale.currentLocale.localeIdentifier
        << "\" based on user's preferred languages " << singleLineDescription(NSLocale.preferredLanguages)
        << ", main bundle localizations " << singleLineDescription(NSBundle.mainBundle.localizations)
        << ", and allowing mixed localizations " << allowMixedLocalizations
        << "; resulting in main bundle preferred localizations "
        << singleLineDescription(NSBundle.mainBundle.preferredLocalizations)
        << " and Foundation preferred localizations "
        << singleLineDescription(foundation.preferredLocalizations);
    qCDebug(lcLocale) << "Reflected by Qt as system locale"
        << QLocale::system() << "with UI languges " << QLocale::system().uiLanguages();
}
Q_COREAPP_STARTUP_FUNCTION(printLocalizationInformation);

static QString getMacLocaleName()
{
    QCFType<CFLocaleRef> l = CFLocaleCopyCurrent();
    CFStringRef locale = CFLocaleGetIdentifier(l);
    return QString::fromCFString(locale);
}

static QVariant macMonthName(int month, QSystemLocale::QueryType type)
{
    month -= 1;
    if (month < 0 || month > 11)
        return {};

    QCFType<CFDateFormatterRef> formatter
        = CFDateFormatterCreate(0, QCFType<CFLocaleRef>(CFLocaleCopyCurrent()),
                                kCFDateFormatterNoStyle,  kCFDateFormatterNoStyle);

    CFDateFormatterKey formatterType;
    switch (type) {
        case QSystemLocale::MonthNameLong:
            formatterType = kCFDateFormatterMonthSymbols;
            break;
        case QSystemLocale::MonthNameShort:
            formatterType = kCFDateFormatterShortMonthSymbols;
            break;
        case QSystemLocale::MonthNameNarrow:
            formatterType = kCFDateFormatterVeryShortMonthSymbols;
            break;
        case QSystemLocale::StandaloneMonthNameLong:
            formatterType = kCFDateFormatterStandaloneMonthSymbols;
            break;
        case QSystemLocale::StandaloneMonthNameShort:
            formatterType = kCFDateFormatterShortStandaloneMonthSymbols;
            break;
        case QSystemLocale::StandaloneMonthNameNarrow:
            formatterType = kCFDateFormatterVeryShortStandaloneMonthSymbols;
            break;
        default:
            qWarning("macMonthName: Unsupported query type %d", type);
            return {};
    }
    QCFType<CFArrayRef> values
        = static_cast<CFArrayRef>(CFDateFormatterCopyProperty(formatter, formatterType));

    if (values != 0) {
        CFStringRef cfstring = static_cast<CFStringRef>(CFArrayGetValueAtIndex(values, month));
        return QString::fromCFString(cfstring);
    }
    return {};
}

static QVariant macDayName(int day, QSystemLocale::QueryType type)
{
    if (day < 1 || day > 7)
        return {};

    QCFType<CFDateFormatterRef> formatter
        = CFDateFormatterCreate(0, QCFType<CFLocaleRef>(CFLocaleCopyCurrent()),
                                kCFDateFormatterNoStyle,  kCFDateFormatterNoStyle);

    CFDateFormatterKey formatterType;
    switch (type) {
    case QSystemLocale::DayNameLong:
        formatterType = kCFDateFormatterWeekdaySymbols;
        break;
    case QSystemLocale::DayNameShort:
        formatterType = kCFDateFormatterShortWeekdaySymbols;
        break;
    case QSystemLocale::DayNameNarrow:
        formatterType = kCFDateFormatterVeryShortWeekdaySymbols;
        break;
    case QSystemLocale::StandaloneDayNameLong:
        formatterType = kCFDateFormatterStandaloneWeekdaySymbols;
        break;
    case QSystemLocale::StandaloneDayNameShort:
        formatterType = kCFDateFormatterShortStandaloneWeekdaySymbols;
        break;
    case QSystemLocale::StandaloneDayNameNarrow:
        formatterType = kCFDateFormatterVeryShortStandaloneWeekdaySymbols;
        break;
    default:
        qWarning("macDayName: Unsupported query type %d", type);
        return {};
    }
    QCFType<CFArrayRef> values =
            static_cast<CFArrayRef>(CFDateFormatterCopyProperty(formatter, formatterType));

    if (values != 0) {
        CFStringRef cfstring = static_cast<CFStringRef>(CFArrayGetValueAtIndex(values, day % 7));
        return QString::fromCFString(cfstring);
    }
    return {};
}

static QString macZeroDigit()
{
    static QString cachedZeroDigit;

    if (cachedZeroDigit.isNull()) {
        QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
        QCFType<CFNumberFormatterRef> numberFormatter =
                CFNumberFormatterCreate(nullptr, locale, kCFNumberFormatterNoStyle);
        const int zeroDigit = 0;
        QCFType<CFStringRef> value
            = CFNumberFormatterCreateStringWithValue(nullptr, numberFormatter,
                                                     kCFNumberIntType, &zeroDigit);
        cachedZeroDigit = QString::fromCFString(value);
    }

    static QMacNotificationObserver localeChangeObserver = QMacNotificationObserver(
        nil, NSCurrentLocaleDidChangeNotification, [&] {
            qCDebug(lcLocale) << "System locale changed";
            cachedZeroDigit = QString();
    });

    return cachedZeroDigit;
}

static QString zeroPad(QString &&number, qsizetype minDigits, const QString &zero)
{
    // Need to pad with zeros, possibly after a sign.
    qsizetype insert = -1, digits = 0;
    auto it = QStringIterator(number);
    while (it.hasNext()) {
        qsizetype here = it.index();
        if (QChar::isDigit(it.next())) {
            if (insert < 0)
                insert = here;
            ++digits;
        } // else: assume we're stepping over a sign (or maybe grouping separator)
    }
    Q_ASSERT(digits > 0);
    Q_ASSERT(insert >= 0);
    while (digits++ < minDigits)
        number.insert(insert, zero);

    return std::move(number);
}

static QString trimTwoDigits(QString &&number)
{
    // Retain any sign, but remove all but the last two digits.
    // We know number has at least four digits - it came from fourDigitYear().
    // Note that each digit might be a surrogate pair.
    qsizetype first = -1, prev = -1, last = -1;
    auto it = QStringIterator(number);
    while (it.hasNext()) {
        qsizetype here = it.index();
        if (QChar::isDigit(it.next())) {
            if (first == -1)
                last = first = here;
            else if (last != -1)
                prev = std::exchange(last, here);
        }
    }
    Q_ASSERT(first >= 0);
    Q_ASSERT(prev > first);
    Q_ASSERT(last > prev);
    number.remove(first, prev - first);
    return std::move(number);
}

static QString fourDigitYear(int year, const QString &zero)
{
    // Return year formatted as an (at least) four digit number:
    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    QCFType<CFNumberFormatterRef> numberFormatter =
            CFNumberFormatterCreate(nullptr, locale, kCFNumberFormatterNoStyle);
    QCFType<CFStringRef> value = CFNumberFormatterCreateStringWithValue(nullptr, numberFormatter,
                                                                        kCFNumberIntType, &year);
    auto text = QString::fromCFString(value);
    if (year > -1000 && year < 1000)
        text = zeroPad(std::move(text), 4, zero);
    return text;
}

static QString macDateToStringImpl(QDate date, CFDateFormatterStyle style)
{
    // Use noon on the given date, to avoid complications that can arise for
    // dates before 1900 (see QTBUG-54955) using different UTC offset than
    // QDateTime extrapolates backwards from time_t functions that only work
    // back to 1900. (Alaska and Phillipines may still be borked, though.)
    QCFType<CFDateRef> myDate = QDateTime(date, QTime(12, 0)).toCFDate();
    QCFType<CFLocaleRef> mylocale = CFLocaleCopyCurrent();
    QCFType<CFDateFormatterRef> myFormatter
        = CFDateFormatterCreate(kCFAllocatorDefault, mylocale, style,
                                kCFDateFormatterNoStyle);
    QCFType<CFStringRef> text = CFDateFormatterCreateStringWithDate(nullptr, myFormatter, myDate);
    return QString::fromCFString(text);
}

static QVariant macDateToString(QDate date, bool short_format)
{
    const int year = date.year();
    QString fakeYear, trueYear;
    if (year < 1583) {
        // System API (in macOS 11.0, at least) discards sign :-(
        // Simply negating the year won't do as the resulting year typically has
        // a different pattern of week-days.
        // Furthermore (see QTBUG-54955), Darwin uses the Julian calendar for
        // dates before 1582-10-15, leading to discrepancies.
        int matcher = QGregorianCalendar::yearSharingWeekDays(date);
        Q_ASSERT(matcher >= 1583);
        Q_ASSERT(matcher % 100 != date.month());
        Q_ASSERT(matcher % 100 != date.day());
        // i.e. there can't be any confusion between the two-digit year and
        // month or day-of-month in the formatted date.
        QString zero = macZeroDigit();
        fakeYear = fourDigitYear(matcher, zero);
        trueYear = fourDigitYear(year, zero);
        date = QDate(matcher, date.month(), date.day());
    }
    QString text = macDateToStringImpl(date, short_format
                                       ? kCFDateFormatterShortStyle
                                       : kCFDateFormatterLongStyle);
    if (year < 1583) {
        if (text.contains(fakeYear))
            return std::move(text).replace(fakeYear, trueYear);
        // Cope with two-digit year:
        fakeYear = trimTwoDigits(std::move(fakeYear));
        trueYear = trimTwoDigits(std::move(trueYear));
        if (text.contains(fakeYear))
            return std::move(text).replace(fakeYear, trueYear);
        // That should have worked.
        qWarning("Failed to fix up year when formatting a date in year %d", year);
    }
    return text;
}

static QVariant macTimeToString(QTime time, bool short_format)
{
    QCFType<CFDateRef> myDate = QDateTime(QDate::currentDate(), time).toCFDate();
    QCFType<CFLocaleRef> mylocale = CFLocaleCopyCurrent();
    CFDateFormatterStyle style = short_format ? kCFDateFormatterShortStyle :  kCFDateFormatterLongStyle;
    QCFType<CFDateFormatterRef> myFormatter = CFDateFormatterCreate(kCFAllocatorDefault,
                                                                    mylocale,
                                                                    kCFDateFormatterNoStyle,
                                                                    style);
    QCFType<CFStringRef> text = CFDateFormatterCreateStringWithDate(0, myFormatter, myDate);
    return QString::fromCFString(text);
}

// Mac uses the Unicode CLDR format codes
// http://www.unicode.org/reports/tr35/tr35-dates.html#Date_Field_Symbol_Table
// See also qtbase/util/locale_database/dateconverter.py
// Makes the assumption that input formats are always well formed and consecutive letters
// never exceed the maximum for the format code.
static QVariant macToQtFormat(QStringView sys_fmt)
{
    QString result;
    qsizetype i = 0;

    while (i < sys_fmt.size()) {
        if (sys_fmt.at(i).unicode() == '\'') {
            QString text = qt_readEscapedFormatString(sys_fmt, &i);
            if (text == "'"_L1)
                result += "''"_L1;
            else
                result += u'\'' + text + u'\'';
            continue;
        }

        QChar c = sys_fmt.at(i);
        qsizetype repeat = qt_repeatCount(sys_fmt.sliced(i));

        switch (c.unicode()) {
            // Qt does not support the following options
        case 'A': // Milliseconds in Day (1..n): 1..n = padded number
        case 'C': // Input skeleton symbol.
        case 'D': // Day of Year (1..3): 1..3 = padded number
        case 'F': // Day of Week in Month (1): 1 = number
        case 'g': // Modified Julian Day (1..n): 1..n = padded number
        case 'G': // Era (1..5): 4 = long, 1..3 = short, 5 = narrow
        case 'j': // Input skeleton symbol.
        case 'J': // Input skeleton symbol.
        case 'l': // Deprecated Chinese leap month indicator.
        case 'q': // Standalone Quarter (1..4): 4 = long, 3 = short, 1,2 = padded number
        case 'Q': // Quarter (1..4): 4 = long, 3 = short, 1,2 = padded number
        case 'U': // Cyclic Year Name (1..5): 4 = long, 1..3 = short, 5 = narrow
        case 'w': // Week of Year (1,2): 1,2 = padded number
        case 'W': // Week of Month (1): 1 = number
        case 'Y': // Year for Week-of-year calendars (1..n): 1..n = padded number
            break;

        case 'u': // Extended Year (1..n), padded number.
            // Explicitly has no special case for 'uu' as only the last two digits.
            result += "yyyy"_L1;
            break;
        case 'y': // Year (1..n): 2 = short year, 1 & 3..n = padded number
            // Qt only supports long (4) or short (2) year, use long for all others
            if (repeat == 2)
                result += "yy"_L1;
            else
                result += "yyyy"_L1;
            break;
        case 'L': // Standalone Month (1..5): 4 = long, 3 = short, 1,2 = number, 5 = narrow
        case 'M': // Month (1..5): 4 = long, 3 = short, 1,2 = number, 5 = narrow
            // Qt only supports long, short and number, use short for narrow
            if (repeat == 5)
                result += "MMM"_L1;
            else
                result += QString(repeat, u'M');
            break;
        case 'd': // Day of Month (1,2): 1,2 padded number
            result += QString(repeat, c);
            break;
        case 'c': // Standalone version of 'e'
        case 'e': // Local Day of Week (1..6): 4 = long, 3 = short, 5,6 = narrow, 1,2 padded number
            // "Local" only affects numeric form: depends on locale's start-day of the week.
        case 'E': // Day of Week (1..6): 4 = long, 1..3 = short, 5,6 = narrow
            // Qt only supports long, short: use short for narrow and padded number.
            if (repeat == 4)
                result += "dddd"_L1;
            else
                result += "ddd"_L1;
            break;
        case 'a': // AM/PM (1..n): Qt supports no distinctions
        case 'b': // Like a, but also distinguishing noon, midnight (ignore difference).
        case 'B': // Flexible day period (at night, &c.)
            // Translate to Qt AM/PM, using locale-appropriate case:
            result += "Ap"_L1;
            break;
        case 'h': // Hour [1..12] (1,2): 1,2 = padded number
        case 'K': // Hour [0..11] (1,2): 1,2 = padded number
            result += QString(repeat, 'h'_L1);
            break;
        case 'H': // Hour [0..23] (1,2): 1,2 = padded number
        case 'k': // Hour [1..24] (1,2): 1,2 = padded number
            // Qt H is 0..23 hour
            result += QString(repeat, 'H'_L1);
            break;
        case 'm': // Minutes (1,2): 1,2 = padded number
        case 's': // Seconds (1,2): 1,2 = padded number
            result += QString(repeat, c);
            break;
        case 'S': // Fractional second (1..n): 1..n = truncates to decimal places
            // Qt uses msecs either unpadded or padded to 3 places
            if (repeat < 3)
                result += u'z';
            else
                result += "zzz"_L1;
            break;
        case 'O': // Time Zone (1, 4)
            result += u't';
            break;
        case 'v': // Time Zone (1, 4)
        case 'V': // Time Zone (1..4)
            result += "tttt"_L1;
            break;
        case 'x': // Time Zone (1..5)
        case 'X': // Time Zone (1..5)
            result += (repeat > 1 && (repeat & 1)) ? "ttt"_L1 : "tt"_L1;
            break;
        case 'z': // Time Zone (1..4)
        case 'Z': // Time Zone (1..5)
            result += repeat < 4 ? "tt"_L1 : repeat > 4 ? "ttt"_L1 : "t"_L1;
            break;
        default:
            // a..z and A..Z are reserved for format codes, so any occurrence of these not
            // already processed are not known and so unsupported formats to be ignored.
            // All other chars are allowed as literals.
            if (c < u'A' || c > u'z' || (c > u'Z' && c < u'a'))
                result += QString(repeat, c);
            break;
        }

        i += repeat;
    }

    return !result.isEmpty() ? QVariant::fromValue(result) : QVariant();
}

static QVariant getGroupingSizes()
{
    // It does not seem like you can directly query the group sizes from CFLocale as there
    // is no key that corresponds to it, see:
    // https://developer.apple.com/documentation/corefoundation/cflocalekey
    // We have to create a number formatter for the locale and query the data from there.
    // see: https://developer.apple.com/documentation/corefoundation/1390801-cfnumberformattercopyproperty
    QLocaleData::GroupSizes sizes;
    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    QCFType<CFNumberFormatterRef> numberFormatter =
            CFNumberFormatterCreate(NULL, locale, kCFNumberFormatterDecimalStyle);
    CFTypeRef numTref =
            CFNumberFormatterCopyProperty(numberFormatter, kCFNumberFormatterGroupingSize);
    CFNumberRef num = static_cast<CFNumberRef>(numTref);
    int value;
    if (CFNumberGetValue(num, kCFNumberIntType, &value) && value > 0) {
        sizes.least = value;
        sizes.higher = value;
    }
    return QVariant::fromValue(sizes);
}

static QVariant getMacDateFormat(CFDateFormatterStyle style)
{
    QCFType<CFLocaleRef> l = CFLocaleCopyCurrent();
    QCFType<CFDateFormatterRef> formatter = CFDateFormatterCreate(kCFAllocatorDefault,
                                                                  l, style, kCFDateFormatterNoStyle);
    return macToQtFormat(QString::fromCFString(CFDateFormatterGetFormat(formatter)));
}

static QVariant getMacTimeFormat(CFDateFormatterStyle style)
{
    QCFType<CFLocaleRef> l = CFLocaleCopyCurrent();
    QCFType<CFDateFormatterRef> formatter = CFDateFormatterCreate(kCFAllocatorDefault,
                                                                  l, kCFDateFormatterNoStyle, style);
    return macToQtFormat(QString::fromCFString(CFDateFormatterGetFormat(formatter)));
}

static QVariant getCFLocaleValue(CFStringRef key)
{
    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    CFTypeRef value = CFLocaleGetValue(locale, key);
    if (!value)
        return QVariant();
    return QString::fromCFString(CFStringRef(static_cast<CFTypeRef>(value)));
}

static QVariant macMeasurementSystem()
{
    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    CFStringRef system = static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleMeasurementSystem));
    if (QString::fromCFString(system) == "Metric"_L1) {
        return QLocale::MetricSystem;
    } else {
        return QLocale::ImperialSystem;
    }
}


static quint8 macFirstDayOfWeek()
{
    QCFType<CFCalendarRef> calendar = CFCalendarCopyCurrent();
    quint8 day = static_cast<quint8>(CFCalendarGetFirstWeekday(calendar))-1;
    if (day == 0)
        day = 7;
    return day;
}

static QVariant macCurrencySymbol(QLocale::CurrencySymbolFormat format)
{
    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    switch (format) {
    case QLocale::CurrencyIsoCode:
        return QString::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleCurrencyCode)));
    case QLocale::CurrencySymbol:
        return QString::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleCurrencySymbol)));
    case QLocale::CurrencyDisplayName: {
        CFStringRef code = static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleCurrencyCode));
        QCFType<CFStringRef> value = CFLocaleCopyDisplayNameForPropertyValue(locale, kCFLocaleCurrencyCode, code);
        return QString::fromCFString(value);
    }
    default:
        break;
    }
    return {};
}

#ifndef QT_NO_SYSTEMLOCALE
static QVariant macFormatCurrency(const QSystemLocale::CurrencyToStringArgument &arg)
{
    QCFType<CFNumberRef> value;
    switch (arg.value.metaType().id()) {
    case QMetaType::Int:
    case QMetaType::UInt: {
        int v = arg.value.toInt();
        value = CFNumberCreate(NULL, kCFNumberIntType, &v);
        break;
    }
    case QMetaType::Double: {
        double v = arg.value.toDouble();
        value = CFNumberCreate(NULL, kCFNumberDoubleType, &v);
        break;
    }
    case QMetaType::LongLong:
    case QMetaType::ULongLong: {
        qint64 v = arg.value.toLongLong();
        value = CFNumberCreate(NULL, kCFNumberLongLongType, &v);
        break;
    }
    default:
        return {};
    }

    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    QCFType<CFNumberFormatterRef> currencyFormatter =
            CFNumberFormatterCreate(NULL, locale, kCFNumberFormatterCurrencyStyle);
    if (!arg.symbol.isEmpty()) {
        CFNumberFormatterSetProperty(currencyFormatter, kCFNumberFormatterCurrencySymbol,
                                     arg.symbol.toCFString());
    }
    QCFType<CFStringRef> result = CFNumberFormatterCreateStringWithNumber(NULL, currencyFormatter, value);
    return QString::fromCFString(result);
}

static QVariant macQuoteString(QSystemLocale::QueryType type, QStringView str)
{
    QString begin, end;
    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    switch (type) {
    case QSystemLocale::StringToStandardQuotation:
        begin = QString::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleQuotationBeginDelimiterKey)));
        end = QString::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleQuotationEndDelimiterKey)));
        return QString(begin % str % end);
    case QSystemLocale::StringToAlternateQuotation:
        begin = QString::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleAlternateQuotationBeginDelimiterKey)));
        end = QString::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleAlternateQuotationEndDelimiterKey)));
        return QString(begin % str % end);
     default:
        break;
    }
    return QVariant();
}
#endif //QT_NO_SYSTEMLOCALE

#ifndef QT_NO_SYSTEMLOCALE

QLocale QSystemLocale::fallbackLocale() const
{
    return QLocale(getMacLocaleName());
}

template <auto CodeToValueFunction>
static QVariant getLocaleValue(CFStringRef key)
{
    if (auto code = getCFLocaleValue(key); !code.isNull()) {
        // If an invalid locale is requested with -AppleLocale, the system APIs
        // will report invalid or empty locale values back to us, which codeToLanguage()
        // and friends will fail to parse, resulting in returning QLocale::Any{L/C/S}.
        // If this is the case, we fall down and return a null-variant, which
        // QLocale's updateSystemPrivate() will interpret to use fallback logic.
        if (auto value = CodeToValueFunction(code.toString()))
            return value;
    }
    return QVariant();
}

static QLocale::Language codeToLanguage(QStringView s)
{
    return QLocalePrivate::codeToLanguage(s);
}

QVariant QSystemLocale::query(QueryType type, QVariant &&in) const
{
    QMacAutoReleasePool pool;

    switch(type) {
    case LanguageId:
        return getLocaleValue<codeToLanguage>(kCFLocaleLanguageCode);
    case TerritoryId:
        return getLocaleValue<QLocalePrivate::codeToTerritory>(kCFLocaleCountryCode);
    case ScriptId:
        return getLocaleValue<QLocalePrivate::codeToScript>(kCFLocaleScriptCode);
    case DecimalPoint:
        return getCFLocaleValue(kCFLocaleDecimalSeparator);
    case Grouping:
        return getGroupingSizes();
    case GroupSeparator:
        return getCFLocaleValue(kCFLocaleGroupingSeparator);
    case DateFormatLong:
    case DateFormatShort:
        return getMacDateFormat(type == DateFormatShort
                                ? kCFDateFormatterShortStyle
                                : kCFDateFormatterLongStyle);
    case TimeFormatLong:
    case TimeFormatShort:
        return getMacTimeFormat(type == TimeFormatShort
                                ? kCFDateFormatterShortStyle
                                : kCFDateFormatterLongStyle);
    case DayNameLong:
    case DayNameShort:
    case DayNameNarrow:
    case StandaloneDayNameLong:
    case StandaloneDayNameShort:
    case StandaloneDayNameNarrow:
        return macDayName(in.toInt(), type);
    case MonthNameLong:
    case MonthNameShort:
    case MonthNameNarrow:
    case StandaloneMonthNameLong:
    case StandaloneMonthNameShort:
    case StandaloneMonthNameNarrow:
        return macMonthName(in.toInt(), type);
    case DateToStringShort:
    case DateToStringLong:
        return macDateToString(in.toDate(), (type == DateToStringShort));
    case TimeToStringShort:
    case TimeToStringLong:
        return macTimeToString(in.toTime(), (type == TimeToStringShort));

    case NegativeSign:
    case PositiveSign:
        break;
    case ZeroDigit:
        return macZeroDigit();

    case MeasurementSystem:
        return macMeasurementSystem();

    case AMText:
    case PMText: {
        QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
        QCFType<CFDateFormatterRef> formatter = CFDateFormatterCreate(NULL, locale, kCFDateFormatterLongStyle, kCFDateFormatterLongStyle);
        QCFType<CFStringRef> value = static_cast<CFStringRef>(CFDateFormatterCopyProperty(formatter,
            (type == AMText ? kCFDateFormatterAMSymbol : kCFDateFormatterPMSymbol)));
        return QString::fromCFString(value);
    }
    case FirstDayOfWeek:
        return QVariant(macFirstDayOfWeek());
    case CurrencySymbol:
        return macCurrencySymbol(QLocale::CurrencySymbolFormat(in.toUInt()));
    case CurrencyToString:
        return macFormatCurrency(in.value<CurrencyToStringArgument>());
    case UILanguages: {
        QStringList result;
        QCFType<CFArrayRef> languages = CFLocaleCopyPreferredLanguages();
        const CFIndex cnt = CFArrayGetCount(languages);
        result.reserve(cnt);
        for (CFIndex i = 0; i < cnt; ++i) {
            const QString lang = QString::fromCFString(
                static_cast<CFStringRef>(CFArrayGetValueAtIndex(languages, i)));
            result.append(lang);
        }
        return QVariant(result);
    }
    case StringToStandardQuotation:
    case StringToAlternateQuotation:
        return macQuoteString(type, in.value<QStringView>());
    default:
        break;
    }
    return QVariant();
}

#endif // QT_NO_SYSTEMLOCALE

#if !QT_CONFIG(icu)

static QString localeConvertString(const QByteArray &localeID, const QString &str, bool *ok,
                                   bool toLowerCase)
{
    QMacAutoReleasePool pool;
    Q_ASSERT(ok);
    NSString *localestring = [[NSString alloc] initWithData:localeID.toNSData()
                                                   encoding:NSUTF8StringEncoding];
    NSLocale *locale = [NSLocale localeWithLocaleIdentifier:localestring];
    if (!locale) {
        *ok = false;
        return QString();
    }
    *ok = true;
    NSString *nsstring = str.toNSString();
    if (toLowerCase)
        nsstring = [nsstring lowercaseStringWithLocale:locale];
    else
        nsstring = [nsstring uppercaseStringWithLocale:locale];

    return QString::fromNSString(nsstring);
}

QString QLocalePrivate::toLower(const QString &str, bool *ok) const
{
    return localeConvertString(bcp47Name('-'), str, ok, true);
}

QString QLocalePrivate::toUpper(const QString &str, bool *ok) const
{
    return localeConvertString(bcp47Name('-'), str, ok, false);
}

#endif

QT_END_NAMESPACE
