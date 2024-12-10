// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androidjnimain.h"
#include "qandroidsystemlocale.h"
#include "qdatetime.h"
#include "qstringlist.h"
#include "qvariant.h"

#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/QJniObject>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(Locale, "java/util/Locale")
Q_DECLARE_JNI_CLASS(Resources, "android/content/res/Resources")
Q_DECLARE_JNI_CLASS(Configuration, "android/content/res/Configuration")
Q_DECLARE_JNI_CLASS(LocaleList, "android/os/LocaleList")
Q_DECLARE_JNI_CLASS(DateFormat, "android/text/format/DateFormat")

using namespace QtJniTypes;

QAndroidSystemLocale::QAndroidSystemLocale() : m_locale(QLocale::C)
{
}

void QAndroidSystemLocale::getLocaleFromJava() const
{
    const Locale javaLocaleObject = []{
        const QJniObject javaContext = QtAndroidPrivate::context();
        if (javaContext.isValid()) {
            const QJniObject resources = javaContext.callMethod<Resources>("getResources");
            const QJniObject configuration = resources.callMethod<Configuration>("getConfiguration");
            return configuration.getField<Locale>("locale");
        } else {
            return Locale::callStaticMethod<Locale>("getDefault");
        }
    }();

    const QString languageCode = javaLocaleObject.callMethod<QString>("getLanguage");
    const QString extraCodes[3] = {
        javaLocaleObject.callMethod<QString>("getScript"),
        javaLocaleObject.callMethod<QString>("getCountry"),
        javaLocaleObject.callMethod<QString>("getVariant"),
    };
    QString fullName = languageCode;
    for (const QString &code : extraCodes) {
        if (code.isEmpty())
            continue;
        if (!fullName.isEmpty())
            fullName += u'_';
        fullName += code;
    }

    const bool is24HourFormat = DateFormat::callStaticMethod<bool>("is24HourFormat",
                                    QNativeInterface::QAndroidApplication::context());

    QWriteLocker locker(&m_lock);
    m_locale = QLocale(fullName);
    m_24hFormat = is24HourFormat;
}

QString QAndroidSystemLocale::convertTo24hFormat(const QString &format) const
{
    if (!m_24hFormat)
        return format;

    QString format24(format);
    bool inQuoted = false;
    for (qsizetype i = 0; i < format24.size(); ++i) {
        if (format24[i] == QLatin1Char('\'')) {
            inQuoted = !inQuoted;
            continue;
        }
        if (inQuoted)
            continue;

        // remove AM/PM markerg from format string
        const auto c = format24[i].toUpper();
        if (c == QLatin1Char('A') || c == QLatin1Char('P'))
            format24.remove(i--, 1);
    }

    return format24.trimmed();
}

QString QAndroidSystemLocale::timeToString(const QTime &time, QLocale::FormatType type) const
{
    if (m_24hFormat)
        return m_locale.toString(time, convertTo24hFormat(m_locale.timeFormat(type)));
    return m_locale.toString(time, type);
}

QString QAndroidSystemLocale::dateTimeToString(const QDateTime &dt, QLocale::FormatType type) const
{
    if (m_24hFormat)
        return m_locale.toString(dt, convertTo24hFormat(m_locale.dateTimeFormat(type)));
    return m_locale.toString(dt, type);
}

QVariant QAndroidSystemLocale::query(QueryType type, QVariant &&in) const
{
    if (type == LocaleChanged) {
        getLocaleFromJava();
        return QVariant();
    }

    QReadLocker locker(&m_lock);

    switch (type) {
    case DecimalPoint:
        return m_locale.decimalPoint();
    case Grouping:
        return QVariant::fromValue(localeData(m_locale)->m_data->groupSizes());
    case GroupSeparator:
        return m_locale.groupSeparator();
    case ZeroDigit:
        return m_locale.zeroDigit();
    case NegativeSign:
        return m_locale.negativeSign();
    case DateFormatLong:
        return m_locale.dateFormat(QLocale::LongFormat);
    case DateFormatShort:
        return m_locale.dateFormat(QLocale::ShortFormat);
    case TimeFormatLong:
        return convertTo24hFormat(m_locale.timeFormat(QLocale::LongFormat));
    case TimeFormatShort:
        return convertTo24hFormat(m_locale.timeFormat(QLocale::ShortFormat));
    case DayNameLong:
        return m_locale.dayName(in.toInt(), QLocale::LongFormat);
    case DayNameShort:
        return m_locale.dayName(in.toInt(), QLocale::ShortFormat);
    case DayNameNarrow:
        return m_locale.dayName(in.toInt(), QLocale::NarrowFormat);
    case StandaloneDayNameLong:
        return m_locale.standaloneDayName(in.toInt(), QLocale::LongFormat);
    case StandaloneDayNameShort:
        return m_locale.standaloneDayName(in.toInt(), QLocale::ShortFormat);
    case StandaloneDayNameNarrow:
        return m_locale.standaloneDayName(in.toInt(), QLocale::NarrowFormat);
    case MonthNameLong:
        return m_locale.monthName(in.toInt(), QLocale::LongFormat);
    case MonthNameShort:
        return m_locale.monthName(in.toInt(), QLocale::ShortFormat);
    case MonthNameNarrow:
        return m_locale.monthName(in.toInt(), QLocale::NarrowFormat);
    case StandaloneMonthNameLong:
        return m_locale.standaloneMonthName(in.toInt(), QLocale::LongFormat);
    case StandaloneMonthNameShort:
        return m_locale.standaloneMonthName(in.toInt(), QLocale::ShortFormat);
    case StandaloneMonthNameNarrow:
        return m_locale.standaloneMonthName(in.toInt(), QLocale::NarrowFormat);
    case DateToStringLong:
        return m_locale.toString(in.toDate(), QLocale::LongFormat);
    case DateToStringShort:
        return m_locale.toString(in.toDate(), QLocale::ShortFormat);
    case TimeToStringLong:
        return timeToString(in.toTime(), QLocale::LongFormat);
    case TimeToStringShort:
        return timeToString(in.toTime(), QLocale::ShortFormat);
    case DateTimeFormatLong:
        return convertTo24hFormat(m_locale.dateTimeFormat(QLocale::LongFormat));
    case DateTimeFormatShort:
        return convertTo24hFormat(m_locale.dateTimeFormat(QLocale::ShortFormat));
    case DateTimeToStringLong:
        return dateTimeToString(in.toDateTime(), QLocale::LongFormat);
    case DateTimeToStringShort:
        return dateTimeToString(in.toDateTime(), QLocale::ShortFormat);
    case PositiveSign:
        return m_locale.positiveSign();
    case AMText:
        return m_locale.amText();
    case PMText:
        return m_locale.pmText();
    case FirstDayOfWeek:
        return m_locale.firstDayOfWeek();
    case CurrencySymbol:
        return m_locale .currencySymbol(QLocale::CurrencySymbolFormat(in.toUInt()));
    case CurrencyToString: {
        switch (in.metaType().id()) {
        case QMetaType::Int:
            return m_locale .toCurrencyString(in.toInt());
        case QMetaType::UInt:
            return m_locale .toCurrencyString(in.toUInt());
        case QMetaType::Double:
            return m_locale .toCurrencyString(in.toDouble());
        case QMetaType::LongLong:
            return m_locale .toCurrencyString(in.toLongLong());
        case QMetaType::ULongLong:
            return m_locale .toCurrencyString(in.toULongLong());
        default:
            break;
        }
        return QString();
    }
    case StringToStandardQuotation:
        return m_locale.quoteString(in.value<QStringView>());
    case StringToAlternateQuotation:
        return m_locale.quoteString(in.value<QStringView>(), QLocale::AlternateQuotation);
    case ListToSeparatedString:
        return m_locale.createSeparatedList(in.value<QStringList>());
    case LocaleChanged:
        Q_ASSERT_X(false, Q_FUNC_INFO, "This can't happen.");
    case UILanguages: {
        if (QtAndroidPrivate::androidSdkVersion() >= 24) {
            LocaleList localeListObject = LocaleList::callStaticMethod<LocaleList>("getDefault");
            if (localeListObject.isValid()) {
                QString lang = localeListObject.callMethod<QString>("toLanguageTags");
                // Some devices return with it enclosed in []'s so check if both exists before
                // removing to ensure it is formatted correctly
                if (lang.startsWith(QChar('[')) && lang.endsWith(QChar(']')))
                    lang = lang.mid(1, lang.length() - 2);
                return lang.split(QChar(','));
            }
        }
        return QVariant();
    }
    default:
        break;
    }
    return QVariant();
}

QLocale QAndroidSystemLocale::fallbackLocale() const
{
    QReadLocker locker(&m_lock);
    return m_locale;
}

QT_END_NAMESPACE
