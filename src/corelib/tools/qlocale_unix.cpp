/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

#if defined(Q_OS_BLACKBERRY)
#include <QtCore/private/qcore_unix_p.h>
#include <QCoreApplication>

#include <unistd.h>
#include <errno.h>
#include <sys/pps.h>
#endif

QT_BEGIN_NAMESPACE

#if defined(Q_OS_BLACKBERRY)
static const char ppsServicePath[] = "/pps/services/locale/uom";
static const size_t ppsBufferSize = 256;

QQNXLocaleData::QQNXLocaleData()
    :ppsNotifier(0)
    ,ppsFd(-1)
{
    readPPSLocale();
}

QQNXLocaleData::~QQNXLocaleData()
{
    if (ppsFd != -1)
        qt_safe_close(ppsFd);
}

void QQNXLocaleData::updateMeasurementSystem()
{
    char buffer[ppsBufferSize];

    errno = 0;
    int bytes = qt_safe_read(ppsFd, buffer, ppsBufferSize - 1);
    if (bytes == -1) {
        qWarning("Failed to read Locale pps, errno=%d", errno);
        return;
    }
    // ensure data is null terminated
    buffer[bytes] = '\0';

    pps_decoder_t ppsDecoder;
    pps_decoder_initialize(&ppsDecoder, 0);
    if (pps_decoder_parse_pps_str(&ppsDecoder, buffer) == PPS_DECODER_OK) {
        pps_decoder_push(&ppsDecoder, 0);
        const char *measurementBuff;
        if (pps_decoder_get_string(&ppsDecoder, "uom", &measurementBuff) == PPS_DECODER_OK) {
            if (qstrcmp(measurementBuff, "imperial") == 0) {
                pps_decoder_cleanup(&ppsDecoder);
                ppsMeasurement = QLocale::ImperialSystem;
                return;
            }
        }
    }

    pps_decoder_cleanup(&ppsDecoder);
    ppsMeasurement = QLocale::MetricSystem;
}

void QQNXLocaleData::readPPSLocale()
{
    errno = 0;
    ppsFd = qt_safe_open(ppsServicePath, O_RDONLY);
    if (ppsFd == -1) {
        qWarning("Failed to open Locale pps, errno=%d", errno);
        return;
    }

    updateMeasurementSystem();
    if (QCoreApplication::instance()) {
        ppsNotifier = new QSocketNotifier(ppsFd, QSocketNotifier::Read, this);
        QObject::connect(ppsNotifier, SIGNAL(activated(int)), this, SLOT(updateMeasurementSystem()));
    }
}
#endif

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
    lc_numeric = QLocale(QString::fromLatin1(numeric));
    lc_time = QLocale(QString::fromLatin1(time));
    lc_monetary = QLocale(QString::fromLatin1(monetary));
    lc_messages = QLocale(QString::fromLatin1(lc_messages_var));
}


Q_GLOBAL_STATIC(QSystemLocaleData, qSystemLocaleData)
#if defined(Q_OS_BLACKBERRY)
    Q_GLOBAL_STATIC(QQNXLocaleData, qqnxLocaleData)
#endif

#endif

#ifndef QT_NO_SYSTEMLOCALE

QLocale QSystemLocale::fallbackUiLocale() const
{
    QByteArray lang = qgetenv("LC_ALL");
    if (lang.isEmpty())
        lang = qgetenv("LC_MESSAGES");
    if (lang.isEmpty())
        lang = qgetenv("LANG");
    // if the locale is the "C" locale, then we can return the language we found here:
    if (lang.isEmpty() || lang == QByteArray("C") || lang == QByteArray("POSIX"))
        return QLocale(QString::fromLatin1(lang));

    // if the locale is not the "C" locale and LANGUAGE is not empty, return
    // the first part of LANGUAGE if LANGUAGE is set and has a first part:
    QByteArray language = qgetenv("LANGUAGE");
    if (!language.isEmpty()) {
        language = language.split(':').first();
        if (!language.isEmpty())
            return QLocale(QString::fromLatin1(language));
    }

    return QLocale(QString::fromLatin1(lang));
}

QVariant QSystemLocale::query(QueryType type, QVariant in) const
{
    QSystemLocaleData *d = qSystemLocaleData();
#if defined(Q_OS_BLACKBERRY)
    QQNXLocaleData *qnxd = qqnxLocaleData();
#endif

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
        switch (in.type()) {
        case QVariant::Int:
            return lc_monetary.toCurrencyString(in.toInt());
        case QVariant::UInt:
            return lc_monetary.toCurrencyString(in.toUInt());
        case QVariant::Double:
            return lc_monetary.toCurrencyString(in.toDouble());
        case QVariant::LongLong:
            return lc_monetary.toCurrencyString(in.toLongLong());
        case QVariant::ULongLong:
            return lc_monetary.toCurrencyString(in.toULongLong());
        default:
            break;
        }
        return QString();
    }
    case MeasurementSystem: {
        const QString meas_locale = QString::fromLatin1(d->lc_measurement_var.constData(), d->lc_measurement_var.size());
        if (meas_locale.compare(QLatin1String("Metric"), Qt::CaseInsensitive) == 0)
            return QLocale::MetricSystem;
        if (meas_locale.compare(QLatin1String("Other"), Qt::CaseInsensitive) == 0)
            return QLocale::MetricSystem;
#if defined(Q_OS_BLACKBERRY)
        return qnxd->ppsMeasurement;
#endif
        return QVariant((int)QLocale(meas_locale).measurementSystem());
    }
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
        return lc_messages.quoteString(in.value<QStringRef>());
    case StringToAlternateQuotation:
        return lc_messages.quoteString(in.value<QStringRef>(), QLocale::AlternateQuotation);
    case ListToSeparatedString:
        return lc_messages.createSeparatedList(in.value<QStringList>());
    case LocaleChanged:
        Q_ASSERT(false);
    default:
        break;
    }
    return QVariant();
}
#endif // QT_NO_SYSTEMLOCALE

QT_END_NAMESPACE
