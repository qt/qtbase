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

#include "qprintengine.h"

#ifndef QT_NO_CUPS

QT_BEGIN_NAMESPACE

QStringList QCUPSSupport::cupsOptionsList(QPrinter *printer)
{
    return printer->printEngine()->property(PPK_CupsOptions).toStringList();
}

void QCUPSSupport::setCupsOptions(QPrinter *printer, const QStringList &cupsOptions)
{
    printer->printEngine()->setProperty(PPK_CupsOptions, QVariant(cupsOptions));
}

void QCUPSSupport::setCupsOption(QStringList &cupsOptions, const QString &option, const QString &value)
{
    if (cupsOptions.contains(option)) {
        cupsOptions.replace(cupsOptions.indexOf(option) + 1, value);
    } else {
        cupsOptions.append(option);
        cupsOptions.append(value);
    }
}

void QCUPSSupport::clearCupsOption(QStringList &cupsOptions, const QString &option)
{
    // ### use const_iterator once QList::erase takes them
    const QStringList::iterator it = std::find(cupsOptions.begin(), cupsOptions.end(), option);
    if (it != cupsOptions.end()) {
        Q_ASSERT(it + 1 < cupsOptions.end());
        cupsOptions.erase(it, it+1);
    }
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
            return localDateTime.toUTC().time().toString(QStringLiteral("HH:mm"));
        }
        // else fall through:
    case QCUPSSupport::NoHold:
        return QString();
    }
    Q_UNREACHABLE();
    return QString();
}

void QCUPSSupport::setJobHold(QPrinter *printer, const JobHoldUntil jobHold, const QTime &holdUntilTime)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    const QString jobHoldUntilArgument = jobHoldToString(jobHold, holdUntilTime);
    if (!jobHoldUntilArgument.isEmpty()) {
        setCupsOption(cupsOptions,
                      QStringLiteral("job-hold-until"),
                      jobHoldUntilArgument);
    } else {
        clearCupsOption(cupsOptions, QStringLiteral("job-hold-until"));
    }
    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::setJobBilling(QPrinter *printer, const QString &jobBilling)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    setCupsOption(cupsOptions, QStringLiteral("job-billing"), jobBilling);
    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::setJobPriority(QPrinter *printer, int priority)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    setCupsOption(cupsOptions, QStringLiteral("job-priority"), QString::number(priority));
    setCupsOptions(printer, cupsOptions);
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
};

void QCUPSSupport::setBannerPages(QPrinter *printer, const BannerPage startBannerPage, const BannerPage endBannerPage)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    const QString startBanner = bannerPageToString(startBannerPage);
    const QString endBanner   = bannerPageToString(endBannerPage);

    setCupsOption(cupsOptions, QStringLiteral("job-sheets"), startBanner + QLatin1Char(',') + endBanner);
    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::setPageSet(QPrinter *printer, const PageSet pageSet)
{
    QStringList cupsOptions = cupsOptionsList(printer);
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

    setCupsOption(cupsOptions, QStringLiteral("page-set"), pageSetString);
    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::setPagesPerSheetLayout(QPrinter *printer,  const PagesPerSheet pagesPerSheet,
                                          const PagesPerSheetLayout pagesPerSheetLayout)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    // WARNING: the following trick (with a [2]-extent) only works as
    // WARNING: long as there's only one two-digit number in the list
    // WARNING: and it is the last one (before the "\0")!
    static const char pagesPerSheetData[][2] = { "1", "2", "4", "6", "9", {'1', '6'}, "\0" };
    static const char pageLayoutData[][5] = {"lrtb", "lrbt", "rlbt", "rltb", "btlr", "btrl", "tblr", "tbrl"};
    setCupsOption(cupsOptions, QStringLiteral("number-up"), QLatin1String(pagesPerSheetData[pagesPerSheet]));
    setCupsOption(cupsOptions, QStringLiteral("number-up-layout"), QLatin1String(pageLayoutData[pagesPerSheetLayout]));
    setCupsOptions(printer, cupsOptions);
}

void QCUPSSupport::setPageRange(QPrinter *printer, int pageFrom, int pageTo)
{
    QStringList cupsOptions = cupsOptionsList(printer);
    setCupsOption(cupsOptions, QStringLiteral("page-ranges"), QStringLiteral("%1-%2").arg(pageFrom).arg(pageTo));
    setCupsOptions(printer, cupsOptions);
}

QT_END_NAMESPACE

#endif // QT_NO_CUPS
