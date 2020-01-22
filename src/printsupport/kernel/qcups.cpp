/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qcups_p.h"

#include "qprintdevice_p.h"
#include "qprintengine.h"

QT_BEGIN_NAMESPACE

static QStringList cupsOptionsList(QPrinter *printer) noexcept
{
    return printer->printEngine()->property(PPK_CupsOptions).toStringList();
}

void setCupsOptions(QPrinter *printer, const QStringList &cupsOptions) noexcept
{
    printer->printEngine()->setProperty(PPK_CupsOptions, QVariant(cupsOptions));
}

void QCUPSSupport::setCupsOption(QPrinter *printer, const QString &option, const QString &value)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    if (cupsOptions.contains(option)) {
        cupsOptions.replace(cupsOptions.indexOf(option) + 1, value);
    } else {
        cupsOptions.append(option);
        cupsOptions.append(value);
    }
    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::clearCupsOption(QPrinter *printer, const QString &option)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    // ### use const_iterator once QList::erase takes them
    const QStringList::iterator it = std::find(cupsOptions.begin(), cupsOptions.end(), option);
    if (it != cupsOptions.end()) {
        Q_ASSERT(it + 1 < cupsOptions.end());
        cupsOptions.erase(it, it+1);
        setCupsOptions(printer, cupsOptions);
    }
}

void QCUPSSupport::clearCupsOptions(QPrinter *printer)
{
    setCupsOptions(printer, QStringList());
}

static inline QString jobHoldToString(const QCUPSSupport::JobHoldUntil jobHold, const QTime holdUntilTime)
{
    switch (jobHold) {
    case QCUPSSupport::Indefinite:
        return QStringLiteral("indefinite");
    case QCUPSSupport::DayTime:
        return QStringLiteral("day-time");
    case QCUPSSupport::Night:
        return QStringLiteral("night");
    case QCUPSSupport::SecondShift:
        return QStringLiteral("second-shift");
    case QCUPSSupport::ThirdShift:
        return QStringLiteral("third-shift");
    case QCUPSSupport::Weekend:
        return QStringLiteral("weekend");
    case QCUPSSupport::SpecificTime:
        if (!holdUntilTime.isNull()) {
            // CUPS expects the time in UTC, user has entered in local time, so get the UTS equivalent
            QDateTime localDateTime = QDateTime::currentDateTime();
            // Check if time is for tomorrow in case of DST change overnight
            if (holdUntilTime < localDateTime.time())
                localDateTime = localDateTime.addDays(1);
            localDateTime.setTime(holdUntilTime);
            return localDateTime.toUTC().time().toString(u"HH:mm");
        }
        // else fall through:
        Q_FALLTHROUGH();
    case QCUPSSupport::NoHold:
        return QString();
    }
    Q_UNREACHABLE();
    return QString();
}

QCUPSSupport::JobHoldUntilWithTime QCUPSSupport::parseJobHoldUntil(const QString &jobHoldUntil)
{
    if (jobHoldUntil == QLatin1String("indefinite")) {
        return { QCUPSSupport::Indefinite, QTime() };
    } else if (jobHoldUntil == QLatin1String("day-time")) {
        return { QCUPSSupport::DayTime, QTime() };
    } else if (jobHoldUntil == QLatin1String("night")) {
        return { QCUPSSupport::Night, QTime() };
    } else if (jobHoldUntil == QLatin1String("second-shift")) {
        return { QCUPSSupport::SecondShift, QTime() };
    } else if (jobHoldUntil == QLatin1String("third-shift")) {
        return { QCUPSSupport::ThirdShift, QTime() };
    } else if (jobHoldUntil == QLatin1String("weekend")) {
        return { QCUPSSupport::Weekend, QTime() };
    }


    QTime parsedTime = QTime::fromString(jobHoldUntil, QStringLiteral("h:m:s"));
    if (!parsedTime.isValid())
        parsedTime = QTime::fromString(jobHoldUntil, QStringLiteral("h:m"));
    if (parsedTime.isValid()) {
        // CUPS time is in UTC, user expects local time, so get the equivalent
        QDateTime dateTimeUtc = QDateTime::currentDateTimeUtc();
        dateTimeUtc.setTime(parsedTime);
        return { QCUPSSupport::SpecificTime, dateTimeUtc.toLocalTime().time() };
    }

    return { QCUPSSupport::NoHold, QTime() };
}

ppd_option_t *QCUPSSupport::findPpdOption(const char *optionName, QPrintDevice *printDevice)
{
    ppd_file_t *ppd = qvariant_cast<ppd_file_t*>(printDevice->property(PDPK_PpdFile));

    if (ppd) {
        for (int i = 0; i < ppd->num_groups; ++i) {
            ppd_group_t *group = &ppd->groups[i];

            for (int i = 0; i < group->num_options; ++i) {
                ppd_option_t *option = &group->options[i];

                if (qstrcmp(option->keyword, optionName) == 0)
                    return option;
            }
        }
    }

    return nullptr;
}

void QCUPSSupport::setJobHold(QPrinter *printer, const JobHoldUntil jobHold, const QTime &holdUntilTime)
{
    const QString jobHoldUntilArgument = jobHoldToString(jobHold, holdUntilTime);
    if (!jobHoldUntilArgument.isEmpty()) {
        setCupsOption(printer,
                      QStringLiteral("job-hold-until"),
                      jobHoldUntilArgument);
    } else {
        clearCupsOption(printer, QStringLiteral("job-hold-until"));
    }
}

void QCUPSSupport::setJobBilling(QPrinter *printer, const QString &jobBilling)
{
    setCupsOption(printer, QStringLiteral("job-billing"), jobBilling);
}

void QCUPSSupport::setJobPriority(QPrinter *printer, int priority)
{
    setCupsOption(printer, QStringLiteral("job-priority"), QString::number(priority));
}

static inline QString bannerPageToString(const QCUPSSupport::BannerPage bannerPage)
{
    switch (bannerPage) {
    case QCUPSSupport::NoBanner:     return QStringLiteral("none");
    case QCUPSSupport::Standard:     return QStringLiteral("standard");
    case QCUPSSupport::Unclassified: return QStringLiteral("unclassified");
    case QCUPSSupport::Confidential: return QStringLiteral("confidential");
    case QCUPSSupport::Classified:   return QStringLiteral("classified");
    case QCUPSSupport::Secret:       return QStringLiteral("secret");
    case QCUPSSupport::TopSecret:    return QStringLiteral("topsecret");
    }
    Q_UNREACHABLE();
    return QString();
}

static inline QCUPSSupport::BannerPage stringToBannerPage(const QString &bannerPage)
{
    if (bannerPage == QLatin1String("none")) return QCUPSSupport::NoBanner;
    else if (bannerPage == QLatin1String("standard")) return QCUPSSupport::Standard;
    else if (bannerPage == QLatin1String("unclassified")) return QCUPSSupport::Unclassified;
    else if (bannerPage == QLatin1String("confidential")) return QCUPSSupport::Confidential;
    else if (bannerPage == QLatin1String("classified")) return QCUPSSupport::Classified;
    else if (bannerPage == QLatin1String("secret")) return QCUPSSupport::Secret;
    else if (bannerPage == QLatin1String("topsecret")) return QCUPSSupport::TopSecret;

    return QCUPSSupport::NoBanner;
}

QCUPSSupport::JobSheets QCUPSSupport::parseJobSheets(const QString &jobSheets)
{
    JobSheets result;

    const QStringList parts = jobSheets.split(QLatin1Char(','));
    if (parts.count() == 2) {
        result.startBannerPage = stringToBannerPage(parts[0]);
        result.endBannerPage = stringToBannerPage(parts[1]);
    }

    return result;
}

void QCUPSSupport::setBannerPages(QPrinter *printer, const BannerPage startBannerPage, const BannerPage endBannerPage)
{
    const QString startBanner = bannerPageToString(startBannerPage);
    const QString endBanner   = bannerPageToString(endBannerPage);

    setCupsOption(printer, QStringLiteral("job-sheets"), startBanner + QLatin1Char(',') + endBanner);
}

void QCUPSSupport::setPageSet(QPrinter *printer, const PageSet pageSet)
{
    QString pageSetString;

    switch (pageSet) {
    case OddPages:
        pageSetString = QStringLiteral("odd");
        break;
    case EvenPages:
        pageSetString = QStringLiteral("even");
        break;
    case AllPages:
        pageSetString = QStringLiteral("all");
        break;
    }

    setCupsOption(printer, QStringLiteral("page-set"), pageSetString);
}

void QCUPSSupport::setPagesPerSheetLayout(QPrinter *printer,  const PagesPerSheet pagesPerSheet,
                                          const PagesPerSheetLayout pagesPerSheetLayout)
{
    // WARNING: the following trick (with a [2]-extent) only works as
    // WARNING: long as there's only one two-digit number in the list
    // WARNING: and it is the last one (before the "\0")!
    static const char pagesPerSheetData[][2] = { "1", "2", "4", "6", "9", {'1', '6'}, "\0" };
    static const char pageLayoutData[][5] = {"lrtb", "lrbt", "rlbt", "rltb", "btlr", "btrl", "tblr", "tbrl"};
    setCupsOption(printer, QStringLiteral("number-up"), QLatin1String(pagesPerSheetData[pagesPerSheet]));
    setCupsOption(printer, QStringLiteral("number-up-layout"), QLatin1String(pageLayoutData[pagesPerSheetLayout]));
}

void QCUPSSupport::setPageRange(QPrinter *printer, int pageFrom, int pageTo)
{
    setPageRange(printer, QStringLiteral("%1-%2").arg(pageFrom).arg(pageTo));
}

void QCUPSSupport::setPageRange(QPrinter *printer, const QString &pageRange)
{
    setCupsOption(printer, QStringLiteral("page-ranges"), pageRange);
}

QT_END_NAMESPACE
