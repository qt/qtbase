/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qlocale_blackberry.h"
#include "qlocale_p.h"

#include "qdatetime.h"

#include "qcoreapplication.h"
#include "private/qcore_unix_p.h"

#include <errno.h>
#include <sys/pps.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SYSTEMLOCALE

static const char ppsUomPath[] = "/pps/services/locale/uom";
static const char ppsRegionLocalePath[] = "/pps/services/locale/settings";
static const char ppsLanguageLocalePath[] = "/pps/services/confstr/_CS_LOCALE";
static const char ppsHourFormatPath[] = "/pps/system/settings";

static const size_t ppsBufferSize = 256;

QBBSystemLocaleData::QBBSystemLocaleData()
    : languageNotifier(0)
    , regionNotifier(0)
    , measurementNotifier(0)
    , hourNotifier(0)
{
    if ((measurementFd = qt_safe_open(ppsUomPath, O_RDONLY)) == -1)
        qWarning("Failed to open uom pps, errno=%d", errno);

    if ((regionFd = qt_safe_open(ppsRegionLocalePath, O_RDONLY)) == -1)
        qWarning("Failed to open region pps, errno=%d", errno);

    if ((languageFd = qt_safe_open(ppsLanguageLocalePath, O_RDONLY)) == -1)
        qWarning("Failed to open language pps, errno=%d", errno);

    if ((hourFd = qt_safe_open(ppsHourFormatPath, O_RDONLY)) == -1)
        qWarning("Failed to open hour format pps, errno=%d", errno);

    // we cannot call this directly, because by the time this constructor is
    // called, the event dispatcher has not yet been created, causing the
    // subsequent call to QSocketNotifier constructor to fail.
    QMetaObject::invokeMethod(this, "installSocketNotifiers", Qt::QueuedConnection);

    readLanguageLocale();
    readRegionLocale();
    readMeasurementSystem();
    readHourFormat();
}

QBBSystemLocaleData::~QBBSystemLocaleData()
{
    if (measurementFd != -1)
        qt_safe_close(measurementFd);

    if (languageFd != -1)
        qt_safe_close(languageFd);

    if (regionFd != -1)
        qt_safe_close(regionFd);

    if (hourFd != -1)
        qt_safe_close(hourFd);
}

uint QBBSystemLocaleData::measurementSystem()
{
    return m_measurementSystem;
}

QVariant QBBSystemLocaleData::timeFormat(QLocale::FormatType formatType)
{
    return getCorrectFormat(regionLocale().timeFormat(formatType), formatType);
}

QVariant QBBSystemLocaleData::dateTimeFormat(QLocale::FormatType formatType)
{
    return getCorrectFormat(regionLocale().dateTimeFormat(formatType), formatType);
}

QLocale QBBSystemLocaleData::languageLocale()
{
    if (!lc_language.isEmpty())
        return QLocale(QLatin1String(lc_language));

    return QLocale::c();
}

QLocale QBBSystemLocaleData::regionLocale()
{
    if (!lc_region.isEmpty())
        return QLocale(QLatin1String(lc_region));

    return QLocale::c();
}

void QBBSystemLocaleData::installSocketNotifiers()
{
    Q_ASSERT(!languageNotifier || !regionNotifier || !measurementNotifier || !hourNotifier);
    Q_ASSERT(QCoreApplication::instance());

    languageNotifier = new QSocketNotifier(languageFd, QSocketNotifier::Read, this);
    QObject::connect(languageNotifier, SIGNAL(activated(int)), this, SLOT(readLanguageLocale()));

    regionNotifier = new QSocketNotifier(regionFd, QSocketNotifier::Read, this);
    QObject::connect(regionNotifier, SIGNAL(activated(int)), this, SLOT(readRegionLocale()));

    measurementNotifier = new QSocketNotifier(measurementFd, QSocketNotifier::Read, this);
    QObject::connect(measurementNotifier, SIGNAL(activated(int)), this, SLOT(readMeasurementSystem()));

    hourNotifier = new QSocketNotifier(hourFd, QSocketNotifier::Read, this);
    QObject::connect(hourNotifier, SIGNAL(activated(int)), this, SLOT(readHourFormat()));
}

void QBBSystemLocaleData::readLanguageLocale()
{
    lc_language = readPpsValue("_CS_LOCALE", languageFd);
}

void QBBSystemLocaleData::readRegionLocale()
{
    lc_region = readPpsValue("region", regionFd);
}

void QBBSystemLocaleData::readMeasurementSystem()
{
    QByteArray measurement = readPpsValue("uom", measurementFd);
    m_measurementSystem = (qstrcmp(measurement, "imperial") == 0) ? QLocale::ImperialSystem : QLocale::MetricSystem;
}

void QBBSystemLocaleData::readHourFormat()
{
    QByteArray hourFormat = readPpsValue("hourFormat", hourFd);
    is24HourFormat = (qstrcmp(hourFormat, "24") == 0);
}

QByteArray QBBSystemLocaleData::readPpsValue(const char *ppsObject, int ppsFd)
{
    QByteArray result;
    if (!ppsObject || ppsFd == -1)
        return result;

    char buffer[ppsBufferSize];

    int bytes = qt_safe_read(ppsFd, buffer, ppsBufferSize - 1);
    if (bytes == -1) {
        qWarning("Failed to read Locale pps, errno=%d", errno);
        return result;
    }
    // ensure data is null terminated
    buffer[bytes] = '\0';

    pps_decoder_t ppsDecoder;
    pps_decoder_initialize(&ppsDecoder, 0);
    if (pps_decoder_parse_pps_str(&ppsDecoder, buffer) == PPS_DECODER_OK) {
        pps_decoder_push(&ppsDecoder, 0);
        const char *ppsBuff;
        if (pps_decoder_get_string(&ppsDecoder, ppsObject, &ppsBuff) == PPS_DECODER_OK) {
            result = ppsBuff;
        } else {
            int val;
            if (pps_decoder_get_int(&ppsDecoder, ppsObject, &val) == PPS_DECODER_OK)
                result = QByteArray::number(val);
        }
    }

    pps_decoder_cleanup(&ppsDecoder);

    return result;
}

QString QBBSystemLocaleData::getCorrectFormat(const QString &baseFormat, QLocale::FormatType formatType)
{
    QString format = baseFormat;
    if (is24HourFormat) {
        if (format.contains(QStringLiteral("AP"), Qt::CaseInsensitive)) {
            format.replace(QStringLiteral("AP"), QStringLiteral(""), Qt::CaseInsensitive);
            format.replace(QStringLiteral("h"), QStringLiteral("H"), Qt::CaseSensitive);
        }

    } else {

        if (!format.contains(QStringLiteral("AP"), Qt::CaseInsensitive)) {
            format.contains(QStringLiteral("HH"), Qt::CaseSensitive) ?
                format.replace(QStringLiteral("HH"), QStringLiteral("hh"), Qt::CaseSensitive) :
                format.replace(QStringLiteral("H"), QStringLiteral("h"), Qt::CaseSensitive);

            formatType == QLocale::LongFormat ? format.append(QStringLiteral(" AP t")) : format.append(QStringLiteral(" AP"));
        }
    }

    return format;
}

Q_GLOBAL_STATIC(QBBSystemLocaleData, bbSysLocaleData)

QLocale QSystemLocale::fallbackUiLocale() const
{
    return bbSysLocaleData()->languageLocale();
}

QVariant QSystemLocale::query(QueryType type, QVariant in) const
{
    QBBSystemLocaleData *d = bbSysLocaleData();

    QReadLocker locker(&d->lock);

    const QLocale &lc_language = d->languageLocale();
    const QLocale &lc_region = d->regionLocale();

    switch (type) {
    case DecimalPoint:
        return lc_region.decimalPoint();
    case GroupSeparator:
        return lc_region.groupSeparator();
    case NegativeSign:
        return lc_region.negativeSign();
    case PositiveSign:
        return lc_region.positiveSign();
    case DateFormatLong:
        return lc_region.dateFormat(QLocale::LongFormat);
    case DateFormatShort:
        return lc_region.dateFormat(QLocale::ShortFormat);
    case TimeFormatLong:
        return d->timeFormat(QLocale::LongFormat);
    case TimeFormatShort:
        return d->timeFormat(QLocale::ShortFormat);
    case DateTimeFormatLong:
        return d->dateTimeFormat(QLocale::LongFormat);
    case DateTimeFormatShort:
        return d->dateTimeFormat(QLocale::ShortFormat);
    case DayNameLong:
        return lc_language.dayName(in.toInt(), QLocale::LongFormat);
    case DayNameShort:
        return lc_language.dayName(in.toInt(), QLocale::ShortFormat);
    case MonthNameLong:
        return lc_language.monthName(in.toInt(), QLocale::LongFormat);
    case MonthNameShort:
        return lc_language.monthName(in.toInt(), QLocale::ShortFormat);
    case StandaloneMonthNameLong:
        return lc_language.standaloneMonthName(in.toInt(), QLocale::LongFormat);
    case StandaloneMonthNameShort:
        return lc_language.standaloneMonthName(in.toInt(), QLocale::ShortFormat);
    case DateToStringLong:
        return lc_region.toString(in.toDate(), QLocale::LongFormat);
    case DateToStringShort:
        return lc_region.toString(in.toDate(), QLocale::ShortFormat);
    case TimeToStringLong:
        return lc_region.toString(in.toTime(), d->timeFormat(QLocale::LongFormat).toString());
    case TimeToStringShort:
        return lc_region.toString(in.toTime(), d->timeFormat(QLocale::ShortFormat).toString());
    case DateTimeToStringShort:
        return lc_region.toString(in.toDateTime(), d->dateTimeFormat(QLocale::ShortFormat).toString());
    case DateTimeToStringLong:
        return lc_region.toString(in.toDateTime(), d->dateTimeFormat(QLocale::LongFormat).toString());
    case MeasurementSystem:
        return d->measurementSystem();
    case ZeroDigit:
        return lc_region.zeroDigit();
    case CountryId:
        return lc_region.country();
    case LanguageId:
        return lc_language.language();
    case AMText:
        return lc_language.amText();
    case PMText:
        return lc_language.pmText();
    default:
        break;
    }
    return QVariant();
}

#endif

QT_END_NAMESPACE
