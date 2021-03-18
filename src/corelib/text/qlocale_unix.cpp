/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qlocale_p.h"

#include "qstringbuilder.h"
#include "qdatetime.h"
#include "qstringlist.h"
#include "qvariant.h"
#include "qreadwritelock.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SYSTEMLOCALE
struct QSystemLocaleData
{
    QSystemLocaleData()
        : lc_numeric(QLocale::C)
         ,lc_time(QLocale::C)
         ,lc_monetary(QLocale::C)
         ,lc_messages(QLocale::C)
    {
        readEnvironment();
    }

    void readEnvironment();

    QReadWriteLock lock;

    QLocale lc_numeric;
    QLocale lc_time;
    QLocale lc_monetary;
    QLocale lc_messages;
    QByteArray lc_messages_var;
    QByteArray lc_measurement_var;
    QByteArray lc_collate_var;
    QStringList uiLanguages;
};

void QSystemLocaleData::readEnvironment()
{
    QWriteLocker locker(&lock);

    QByteArray all = qgetenv("LC_ALL");
    QByteArray numeric  = all.isEmpty() ? qgetenv("LC_NUMERIC") : all;
    QByteArray time     = all.isEmpty() ? qgetenv("LC_TIME") : all;
    QByteArray monetary = all.isEmpty() ? qgetenv("LC_MONETARY") : all;
    lc_messages_var     = all.isEmpty() ? qgetenv("LC_MESSAGES") : all;
    lc_measurement_var  = all.isEmpty() ? qgetenv("LC_MEASUREMENT") : all;
    lc_collate_var      = all.isEmpty() ? qgetenv("LC_COLLATE") : all;
    QByteArray lang = qgetenv("LANG");
    if (lang.isEmpty())
        lang = QByteArray("C");
    if (numeric.isEmpty())
        numeric = lang;
    if (time.isEmpty())
        time = lang;
    if (monetary.isEmpty())
        monetary = lang;
    if (lc_messages_var.isEmpty())
        lc_messages_var = lang;
    if (lc_measurement_var.isEmpty())
        lc_measurement_var = lang;
    if (lc_collate_var.isEmpty())
        lc_collate_var = lang;
    lc_numeric = QLocale(QString::fromLatin1(numeric));
    lc_time = QLocale(QString::fromLatin1(time));
    lc_monetary = QLocale(QString::fromLatin1(monetary));
    lc_messages = QLocale(QString::fromLatin1(lc_messages_var));
}

Q_GLOBAL_STATIC(QSystemLocaleData, qSystemLocaleData)

#endif

#ifndef QT_NO_SYSTEMLOCALE

static bool contradicts(const QString &maybe, const QString &known)
{
    if (maybe.isEmpty())
        return false;

    /*
      If \a known (our current best shot at deciding which language to use)
      provides more information (e.g. script, country) than \a maybe (a
      candidate to replace \a known) and \a maybe agrees with \a known in what
      it does provide, we keep \a known; this happens when \a maybe comes from
      LANGUAGE (usually a simple language code) and LANG includes script and/or
      country.  A textual comparison won't do because, for example, bn (Bengali)
      isn't a prefix of ben_IN, but the latter is a refinement of the former.
      (Meanwhile, bn is a prefix of bnt, Bantu; and a prefix of ben is be,
      Belarusian.  There are many more such prefixings between two- and
      three-letter codes.)
     */
    QLocale::Language langm, langk;
    QLocale::Script scriptm, scriptk;
    QLocale::Country landm, landk;
    QLocalePrivate::getLangAndCountry(maybe, langm, scriptm, landm);
    QLocalePrivate::getLangAndCountry(known, langk, scriptk, landk);
    return (langm != QLocale::AnyLanguage && langm != langk)
        || (scriptm != QLocale::AnyScript && scriptm != scriptk)
        || (landm != QLocale::AnyCountry && landm != landk);
}

QLocale QSystemLocale::fallbackUiLocale() const
{
    // See man 7 locale for precedence - LC_ALL beats LC_MESSAGES beats LANG:
    QString lang = qEnvironmentVariable("LC_ALL");
    if (lang.isEmpty())
        lang = qEnvironmentVariable("LC_MESSAGES");
    if (lang.isEmpty())
        lang = qEnvironmentVariable("LANG");
    // if the locale is the "C" locale, then we can return the language we found here:
    if (lang.isEmpty() || lang == QLatin1String("C") || lang == QLatin1String("POSIX"))
        return QLocale(lang);

    // ... otherwise, if the first part of LANGUAGE says more than or
    // contradicts what we have, use that:
    QString language = qEnvironmentVariable("LANGUAGE");
    if (!language.isEmpty()) {
        language = language.split(QLatin1Char(':')).constFirst();
        if (contradicts(language, lang))
            return QLocale(language);
    }

    return QLocale(lang);
}

QVariant QSystemLocale::query(QueryType type, QVariant in) const
{
    QSystemLocaleData *d = qSystemLocaleData();

    if (type == LocaleChanged) {
        d->readEnvironment();
        return QVariant();
    }

    QReadLocker locker(&d->lock);

    const QLocale &lc_numeric = d->lc_numeric;
    const QLocale &lc_time = d->lc_time;
    const QLocale &lc_monetary = d->lc_monetary;
    const QLocale &lc_messages = d->lc_messages;

    switch (type) {
    case DecimalPoint:
        return lc_numeric.decimalPoint();
    case GroupSeparator:
        return lc_numeric.groupSeparator();
    case ZeroDigit:
        return lc_numeric.zeroDigit();
    case NegativeSign:
        return lc_numeric.negativeSign();
    case DateFormatLong:
        return lc_time.dateFormat(QLocale::LongFormat);
    case DateFormatShort:
        return lc_time.dateFormat(QLocale::ShortFormat);
    case TimeFormatLong:
        return lc_time.timeFormat(QLocale::LongFormat);
    case TimeFormatShort:
        return lc_time.timeFormat(QLocale::ShortFormat);
    case DayNameLong:
        return lc_time.dayName(in.toInt(), QLocale::LongFormat);
    case DayNameShort:
        return lc_time.dayName(in.toInt(), QLocale::ShortFormat);
    case MonthNameLong:
        return lc_time.monthName(in.toInt(), QLocale::LongFormat);
    case MonthNameShort:
        return lc_time.monthName(in.toInt(), QLocale::ShortFormat);
    case StandaloneMonthNameLong:
        return lc_time.standaloneMonthName(in.toInt(), QLocale::LongFormat);
    case StandaloneMonthNameShort:
        return lc_time.standaloneMonthName(in.toInt(), QLocale::ShortFormat);
    case DateToStringLong:
        return lc_time.toString(in.toDate(), QLocale::LongFormat);
    case DateToStringShort:
        return lc_time.toString(in.toDate(), QLocale::ShortFormat);
    case TimeToStringLong:
        return lc_time.toString(in.toTime(), QLocale::LongFormat);
    case TimeToStringShort:
        return lc_time.toString(in.toTime(), QLocale::ShortFormat);
    case DateTimeFormatLong:
        return lc_time.dateTimeFormat(QLocale::LongFormat);
    case DateTimeFormatShort:
        return lc_time.dateTimeFormat(QLocale::ShortFormat);
    case DateTimeToStringLong:
        return lc_time.toString(in.toDateTime(), QLocale::LongFormat);
    case DateTimeToStringShort:
        return lc_time.toString(in.toDateTime(), QLocale::ShortFormat);
    case PositiveSign:
        return lc_numeric.positiveSign();
    case AMText:
        return lc_time.amText();
    case PMText:
        return lc_time.pmText();
    case FirstDayOfWeek:
        return lc_time.firstDayOfWeek();
    case CurrencySymbol:
        return lc_monetary.currencySymbol(QLocale::CurrencySymbolFormat(in.toUInt()));
    case CurrencyToString: {
        switch (in.userType()) {
        case QMetaType::Int:
            return lc_monetary.toCurrencyString(in.toInt());
        case QMetaType::UInt:
            return lc_monetary.toCurrencyString(in.toUInt());
        case QMetaType::Double:
            return lc_monetary.toCurrencyString(in.toDouble());
        case QMetaType::LongLong:
            return lc_monetary.toCurrencyString(in.toLongLong());
        case QMetaType::ULongLong:
            return lc_monetary.toCurrencyString(in.toULongLong());
        default:
            break;
        }
        return QString();
    }
    case MeasurementSystem: {
        const QString meas_locale = QString::fromLatin1(d->lc_measurement_var);
        if (meas_locale.compare(QLatin1String("Metric"), Qt::CaseInsensitive) == 0)
            return QLocale::MetricSystem;
        if (meas_locale.compare(QLatin1String("Other"), Qt::CaseInsensitive) == 0)
            return QLocale::MetricSystem;
        return QVariant((int)QLocale(meas_locale).measurementSystem());
    }
    case Collation:
        return QString::fromLatin1(d->lc_collate_var);
    case UILanguages: {
        if (!d->uiLanguages.isEmpty())
            return d->uiLanguages;
        QString languages = QString::fromLatin1(qgetenv("LANGUAGE"));
        QStringList lst;
        if (languages.isEmpty())
            lst.append(QString::fromLatin1(d->lc_messages_var));
        else
            lst = languages.split(QLatin1Char(':'));

        for (int i = 0; i < lst.size(); ++i) {
            const QString &name = lst.at(i);
            QString lang, script, cntry;
            if (qt_splitLocaleName(name, lang, script, cntry)) {
                if (!cntry.length())
                    d->uiLanguages.append(lang);
                else
                    d->uiLanguages.append(lang % QLatin1Char('-') % cntry);
            }
        }
        return d->uiLanguages.isEmpty() ? QVariant() : QVariant(d->uiLanguages);
    }
    case StringToStandardQuotation:
        return lc_messages.quoteString(qvariant_cast<QStringRef>(in));
    case StringToAlternateQuotation:
        return lc_messages.quoteString(qvariant_cast<QStringRef>(in), QLocale::AlternateQuotation);
    case ListToSeparatedString:
        return lc_messages.createSeparatedList(in.toStringList());
    case LocaleChanged:
        Q_ASSERT(false);
    default:
        break;
    }
    return QVariant();
}
#endif // QT_NO_SYSTEMLOCALE

QT_END_NAMESPACE
