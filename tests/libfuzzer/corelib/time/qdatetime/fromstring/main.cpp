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

static constexpr struct {
    QLatin1StringView format;
    int baseYear = 1900;
} patterns[] = {
    { ""_L1 }, // Accepts only an empty string, returns default
    { "M/d/yyyy"_L1 },
    { "yy"_L1 },
    { "yyyy"_L1 },
    { "M"_L1 },
    { "MM"_L1 },
    { "MMM"_L1 },
    { "MMMM"_L1 },
    { "d"_L1 },
    { "dd"_L1 },
    { "ddd"_L1 },
    { "dddd"_L1 },
    { "h"_L1 },
    { "hh"_L1 },
    { "H"_L1 },
    { "HH"_L1 },
    { "m"_L1 },
    { "mm"_L1 },
    { "s"_L1 },
    { "ss"_L1 },
    { "z"_L1 },
    { "zzz"_L1 },
    { "a"_L1 },
    { "A"_L1 },
    { "Ap"_L1 },
    { "t"_L1 },
    { "tt"_L1 },
    { "ttt"_L1 },
    { "tttt"_L1 },
    { "M/d/yyyy hh:mm"_L1 },
    { "M/d/yyyy hh:mm A"_L1 },
    { "M/d/yyyy, hh:mm"_L1 },
    { "M/d/yyyy, hh:mm A"_L1 },
    { "MMM d yyyy"_L1 },
    { "MMM d yyyy hh:mm"_L1 },
    { "MMM d yyyy hh:mm:ss"_L1 },
    { "MMM d yyyy, hh:mm"_L1 },
    { "MMM d yyyy, hh:mm:ss"_L1 },
    { "MMMM d yyyy"_L1 },
    { "MMMM d yyyy hh:mm"_L1 },
    { "MMMM d yyyy hh:mm:ss"_L1 },
    { "MMMM d yyyy, hh:mm"_L1 },
    { "MMMM d yyyy, hh:mm:ss"_L1 },
    { "MMMM d yyyy, hh:mm:ss t"_L1 },
    { "MMM d, yyyy"_L1 },
    { "MMM d, yyyy hh:mm"_L1 },
    { "MMM d, yyyy hh:mm:ss"_L1 },
    { "MMMM d, yyyy"_L1 },
    { "MMMM d, yyyy hh:mm"_L1 },
    { "MMMM d, yyyy hh:mm:ss"_L1 },
    { "MMMM d, yyyy hh:mm:ss t"_L1 },
    { "d MMM yyyy"_L1 },
    { "d MMM yyyy hh:mm"_L1 },
    { "d MMM yyyy hh:mm:ss"_L1 },
    { "d MMM yyyy, hh:mm"_L1 },
    { "d MMM yyyy, hh:mm:ss"_L1 },
    { "d MMMM yyyy"_L1 },
    { "d MMMM yyyy hh:mm"_L1 },
    { "d MMMM yyyy hh:mm:ss"_L1 },
    { "d MMMM yyyy, hh:mm"_L1 },
    { "d MMMM yyyy, hh:mm:ss"_L1 },
    { "d MMM, yyyy"_L1 },
    { "d MMM, yyyy hh:mm"_L1 },
    { "d MMM, yyyy hh:mm:ss"_L1 },
    { "d MMMM, yyyy"_L1 },
    { "d MMMM, yyyy hh:mm"_L1 },
    { "d MMMM, yyyy hh:mm:ss"_L1 },
    { "yyyy-MM-ddThh:mm:ss.zt"_L1 },
    { "yyMMddHHmmss"_L1, 1950 }, // ASN.1 UTC type
    { "yyyyMMddHHmmsst"_L1 }, // ASN.1 generalized type
    { "yyyy'-'MM'-'dd'T'HHmmss'.'zzz' 't"_L1 }, // correctly matched quotes
    { "yyyy''MM''dd HHmmss''zzz''t"_L1 }, // doubled quotes as literal quotes
    { "yyyy MMMM dddd HH mm ss.zzz t'unmatched"_L1 }, // malformed - rejected
};

// libFuzzer entry-point for testing QDateTime::fromString()
extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size)
{
    const QString userString = QString::fromUtf8(Data, Size);

    QDateTime::fromString(userString, Qt::TextDate);
    QDateTime::fromString(userString, Qt::ISODate);
    QDateTime::fromString(userString, Qt::RFC2822Date);
    QDateTime::fromString(userString, Qt::ISODateWithMs);

    QDateTime::fromString(userString, patterns[0].format, patterns[0].baseYear,
                          QCalendar(QCalendar::System::Gregorian));
    for (int sys = int(QCalendar::System::Julian); sys <= int(QCalendar::System::Last); ++sys) {
        QCalendar cal(QCalendar::System(sys));
        if (cal.isValid()) // There are gaps in the enum, so it might not be.
            QDateTime::fromString(userString, patterns[0].format, patterns[0].baseYear, cal);
    }

    for (const auto &pattern : patterns) {
        #ifdef LOG_FORMAT
        qDebug() << "Trying format:" << pattern.format << "with base year:" << pattern.baseYear;
        #endif
        QDateTime::fromString(userString, pattern.format, pattern.baseYear);
    }
    return 0;
}
