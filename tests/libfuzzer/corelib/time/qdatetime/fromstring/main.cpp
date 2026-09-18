// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QDateTime>
#include <QString>

// Enable to report the currently used format, e.g. when reproducing issues
// #define LOG_FORMAT
#ifdef LOG_FORMAT
#include <QDebug>
#endif
using namespace Qt::StringLiterals;

static constexpr QLatin1StringView formats[] = {
    "M/d/yyyy"_L1,
    "h"_L1,
    "hh"_L1,
    "H"_L1,
    "HH"_L1,
    "m"_L1,
    "mm"_L1,
    "s"_L1,
    "ss"_L1,
    "z"_L1,
    "zzz"_L1,
    "A"_L1,
    "t"_L1,
    "M/d/yyyy hh:mm"_L1,
    "M/d/yyyy hh:mm A"_L1,
    "M/d/yyyy, hh:mm"_L1,
    "M/d/yyyy, hh:mm A"_L1,
    "MMM d yyyy"_L1,
    "MMM d yyyy hh:mm"_L1,
    "MMM d yyyy hh:mm:ss"_L1,
    "MMM d yyyy, hh:mm"_L1,
    "MMM d yyyy, hh:mm:ss"_L1,
    "MMMM d yyyy"_L1,
    "MMMM d yyyy hh:mm"_L1,
    "MMMM d yyyy hh:mm:ss"_L1,
    "MMMM d yyyy, hh:mm"_L1,
    "MMMM d yyyy, hh:mm:ss"_L1,
    "MMMM d yyyy, hh:mm:ss t"_L1,
    "MMM d, yyyy"_L1,
    "MMM d, yyyy hh:mm"_L1,
    "MMM d, yyyy hh:mm:ss"_L1,
    "MMMM d, yyyy"_L1,
    "MMMM d, yyyy hh:mm"_L1,
    "MMMM d, yyyy hh:mm:ss"_L1,
    "MMMM d, yyyy hh:mm:ss t"_L1,
    "d MMM yyyy"_L1,
    "d MMM yyyy hh:mm"_L1,
    "d MMM yyyy hh:mm:ss"_L1,
    "d MMM yyyy, hh:mm"_L1,
    "d MMM yyyy, hh:mm:ss"_L1,
    "d MMMM yyyy"_L1,
    "d MMMM yyyy hh:mm"_L1,
    "d MMMM yyyy hh:mm:ss"_L1,
    "d MMMM yyyy, hh:mm"_L1,
    "d MMMM yyyy, hh:mm:ss"_L1,
    "d MMM, yyyy"_L1,
    "d MMM, yyyy hh:mm"_L1,
    "d MMM, yyyy hh:mm:ss"_L1,
    "d MMMM, yyyy"_L1,
    "d MMMM, yyyy hh:mm"_L1,
    "d MMMM, yyyy hh:mm:ss"_L1,
    "yyyy-MM-ddThh:mm:ss.zt"_L1,
};

// libFuzzer entry-point for testing QDateTimeParser
extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size)
{
    const QString userString = QString::fromUtf8(Data, Size);

    QDateTime::fromString(userString, Qt::TextDate);
    QDateTime::fromString(userString, Qt::ISODate);
    QDateTime::fromString(userString, Qt::RFC2822Date);
    QDateTime::fromString(userString, Qt::ISODateWithMs);

    QDateTime::fromString(userString, formats[0], QCalendar(QCalendar::System::Gregorian));
    for (int sys = int(QCalendar::System::Julian); sys <= int(QCalendar::System::Last); ++sys)
        QDateTime::fromString(userString, formats[0], QCalendar(QCalendar::System(sys)));

    for (const auto &format : formats) {
        #ifdef LOG_FORMAT
        qDebug() << "Trying format:" << format;
        #endif
        QDateTime::fromString(userString, format);
    }
    return 0;
}
