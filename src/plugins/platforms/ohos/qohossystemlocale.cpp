// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohossystemlocale.h"
#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

namespace {

QString currencyToString(const QLocale &locale, const QVariant &in)
{
    switch (in.typeId()) {
    case QMetaType::Int:
        return locale.toCurrencyString(in.toInt());
    case QMetaType::UInt:
        return locale.toCurrencyString(in.toUInt());
    case QMetaType::Double:
        return locale.toCurrencyString(in.toDouble());
    case QMetaType::LongLong:
        return locale.toCurrencyString(in.toLongLong());
    case QMetaType::ULongLong:
        return locale.toCurrencyString(in.toULongLong());
    default:
        break;
    }

    return QString();
}

}

QOhosSystemLocale::QOhosSystemLocale(const QString &systemLocaleId, const QStringList &preferredLanguages)
    : m_systemLocaleId(systemLocaleId)
    , m_preferredLanguages(preferredLanguages)
{ }

QVariant QOhosSystemLocale::query(QueryType type, QVariant &&in) const
{
    auto locale = fallbackLocale();

    switch (type) {
    case LanguageId:
        return locale.language();
    case TerritoryId:
        return locale.territory();
    case DecimalPoint:
        return locale.decimalPoint();
    case GroupSeparator:
        return locale.groupSeparator();
    case ZeroDigit:
        return locale.zeroDigit();
    case NegativeSign:
        return locale.negativeSign();
    case DateFormatLong:
        return locale.dateFormat(QLocale::LongFormat);
    case DateFormatShort:
        return locale.dateFormat(QLocale::ShortFormat);
    case TimeFormatLong:
        return locale.timeFormat(QLocale::LongFormat);
    case TimeFormatShort:
        return locale.timeFormat(QLocale::ShortFormat);
    case DayNameLong:
        return locale.dayName(in.toInt(), QLocale::LongFormat);
    case DayNameShort:
        return locale.dayName(in.toInt(), QLocale::ShortFormat);
    case MonthNameLong:
        return locale.monthName(in.toInt(), QLocale::LongFormat);
    case MonthNameShort:
        return locale.monthName(in.toInt(), QLocale::ShortFormat);
    case DateToStringLong:
        return locale.toString(in.toDate(), QLocale::LongFormat);
    case DateToStringShort:
        return locale.toString(in.toDate(), QLocale::ShortFormat);
    case TimeToStringLong:
        return locale.toString(in.toTime(), QLocale::LongFormat);
    case TimeToStringShort:
        return locale.toString(in.toTime(), QLocale::ShortFormat);
    case DateTimeFormatLong:
        return locale.dateTimeFormat(QLocale::LongFormat);
    case DateTimeFormatShort:
        return locale.dateTimeFormat(QLocale::ShortFormat);
    case DateTimeToStringLong:
        return locale.toString(in.toDateTime(), QLocale::LongFormat);
    case DateTimeToStringShort:
        return locale.toString(in.toDateTime(), QLocale::ShortFormat);
    case MeasurementSystem:
        return locale.measurementSystem();
    case PositiveSign:
        return locale.positiveSign();
    case AMText:
        return locale.amText();
    case PMText:
        return locale.pmText();
    case FirstDayOfWeek:
        return locale.firstDayOfWeek();
    case Weekdays:
        return QVariant::fromValue(locale.weekdays());
    case CurrencySymbol:
        return locale.currencySymbol(QLocale::CurrencySymbolFormat(in.toUInt()));
    case CurrencyToString:
        return currencyToString(locale, in);
    case Collation:
        return locale.collation();
    case UILanguages:
        return m_preferredLanguages;
    case StringToStandardQuotation:
        return locale.quoteString(in.value<QStringView>());
    case StringToAlternateQuotation:
        return locale.quoteString(in.value<QStringView>(), QLocale::AlternateQuotation);
    case ScriptId:
        return locale.script();
    case ListToSeparatedString:
        return locale.createSeparatedList(in.value<QStringList>());
    case LocaleChanged:
        return QVariant();
    case NativeLanguageName:
        return locale.nativeLanguageName();
    case NativeTerritoryName:
        return locale.nativeTerritoryName();
    case StandaloneMonthNameLong:
        return locale.standaloneMonthName(in.toInt(), QLocale::LongFormat);
    case StandaloneMonthNameShort:
        return locale.standaloneMonthName(in.toInt(), QLocale::ShortFormat);
    default:
        break;
    }

    return QVariant();
}

QLocale QOhosSystemLocale::fallbackLocale() const
{
    return QLocale(m_systemLocaleId);
}

QT_END_NAMESPACE
