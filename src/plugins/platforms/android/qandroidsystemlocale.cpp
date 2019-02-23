/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qandroidsystemlocale.h"
#include "androidjnimain.h"
#include <QtCore/private/qjni_p.h>
#include <QtCore/private/qjnihelpers_p.h>
#include "qdatetime.h"
#include "qstringlist.h"
#include "qvariant.h"

QT_BEGIN_NAMESPACE

QAndroidSystemLocale::QAndroidSystemLocale() : m_locale(QLocale::C)
{
}

void QAndroidSystemLocale::getLocaleFromJava() const
{
    QWriteLocker locker(&m_lock);

    QJNIObjectPrivate javaLocaleObject;
    QJNIObjectPrivate javaActivity(QtAndroid::activity());
    if (!javaActivity.isValid())
        javaActivity = QtAndroid::service();
    if (javaActivity.isValid()) {
        QJNIObjectPrivate resources = javaActivity.callObjectMethod("getResources", "()Landroid/content/res/Resources;");
        QJNIObjectPrivate configuration = resources.callObjectMethod("getConfiguration", "()Landroid/content/res/Configuration;");

        javaLocaleObject = configuration.getObjectField("locale", "Ljava/util/Locale;");
    } else {
        javaLocaleObject = QJNIObjectPrivate::callStaticObjectMethod("java/util/Locale", "getDefault", "()Ljava/util/Locale;");
    }

    QString languageCode = javaLocaleObject.callObjectMethod("getLanguage", "()Ljava/lang/String;").toString();
    QString countryCode = javaLocaleObject.callObjectMethod("getCountry", "()Ljava/lang/String;").toString();

    m_locale = QLocale(languageCode + QLatin1Char('_') + countryCode);
}

QVariant QAndroidSystemLocale::query(QueryType type, QVariant in) const
{
    if (type == LocaleChanged) {
        getLocaleFromJava();
        return QVariant();
    }

    QReadLocker locker(&m_lock);

    switch (type) {
    case DecimalPoint:
        return m_locale.decimalPoint();
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
        return m_locale.timeFormat(QLocale::LongFormat);
    case TimeFormatShort:
        return m_locale.timeFormat(QLocale::ShortFormat);
    case DayNameLong:
        return m_locale.dayName(in.toInt(), QLocale::LongFormat);
    case DayNameShort:
        return m_locale.dayName(in.toInt(), QLocale::ShortFormat);
    case MonthNameLong:
        return m_locale.monthName(in.toInt(), QLocale::LongFormat);
    case MonthNameShort:
        return m_locale.monthName(in.toInt(), QLocale::ShortFormat);
    case StandaloneMonthNameLong:
        return m_locale.standaloneMonthName(in.toInt(), QLocale::LongFormat);
    case StandaloneMonthNameShort:
        return m_locale.standaloneMonthName(in.toInt(), QLocale::ShortFormat);
    case DateToStringLong:
        return m_locale.toString(in.toDate(), QLocale::LongFormat);
    case DateToStringShort:
        return m_locale.toString(in.toDate(), QLocale::ShortFormat);
    case TimeToStringLong:
        return m_locale.toString(in.toTime(), QLocale::LongFormat);
    case TimeToStringShort:
        return m_locale.toString(in.toTime(), QLocale::ShortFormat);
    case DateTimeFormatLong:
        return m_locale.dateTimeFormat(QLocale::LongFormat);
    case DateTimeFormatShort:
        return m_locale.dateTimeFormat(QLocale::ShortFormat);
    case DateTimeToStringLong:
        return m_locale.toString(in.toDateTime(), QLocale::LongFormat);
    case DateTimeToStringShort:
        return m_locale.toString(in.toDateTime(), QLocale::ShortFormat);
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
        switch (in.type()) {
        case QVariant::Int:
            return m_locale .toCurrencyString(in.toInt());
        case QVariant::UInt:
            return m_locale .toCurrencyString(in.toUInt());
        case QVariant::Double:
            return m_locale .toCurrencyString(in.toDouble());
        case QVariant::LongLong:
            return m_locale .toCurrencyString(in.toLongLong());
        case QVariant::ULongLong:
            return m_locale .toCurrencyString(in.toULongLong());
        default:
            break;
        }
        return QString();
    }
    case StringToStandardQuotation:
        return m_locale.quoteString(in.value<QStringRef>());
    case StringToAlternateQuotation:
        return m_locale.quoteString(in.value<QStringRef>(), QLocale::AlternateQuotation);
    case ListToSeparatedString:
        return m_locale.createSeparatedList(in.value<QStringList>());
    case LocaleChanged:
        Q_ASSERT_X(false, Q_FUNC_INFO, "This can't happen.");
    case UILanguages: {
        if (QtAndroidPrivate::androidSdkVersion() >= 24) {
            QJNIObjectPrivate localeListObject =
                QJNIObjectPrivate::callStaticObjectMethod("android/os/LocaleList", "getDefault",
                                                          "()Landroid/os/LocaleList;");
            if (localeListObject.isValid()) {
                QString lang = localeListObject.callObjectMethod("toLanguageTags",
                                                                 "()Ljava/lang/String;").toString();
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

QLocale QAndroidSystemLocale::fallbackUiLocale() const
{
    QReadLocker locker(&m_lock);
    return m_locale;
}

QT_END_NAMESPACE
