// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QDateTime>

// Enable to report the currently used format, e.g. when reproducing issues
// #define LOG_FORMAT
#ifdef LOG_FORMAT
#include <QDebug>
#endif

static const QString formats[] = {
    QStringLiteral("M/d/yyyy"),
    QStringLiteral("h"),
    QStringLiteral("hh"),
    QStringLiteral("H"),
    QStringLiteral("HH"),
    QStringLiteral("m"),
    QStringLiteral("mm"),
    QStringLiteral("s"),
    QStringLiteral("ss"),
    QStringLiteral("z"),
    QStringLiteral("zzz"),
    QStringLiteral("A"),
    QStringLiteral("t"),
    QStringLiteral("M/d/yyyy hh:mm"),
    QStringLiteral("M/d/yyyy hh:mm A"),
    QStringLiteral("M/d/yyyy, hh:mm"),
    QStringLiteral("M/d/yyyy, hh:mm A"),
    QStringLiteral("MMM d yyyy"),
    QStringLiteral("MMM d yyyy hh:mm"),
    QStringLiteral("MMM d yyyy hh:mm:ss"),
    QStringLiteral("MMM d yyyy, hh:mm"),
    QStringLiteral("MMM d yyyy, hh:mm:ss"),
    QStringLiteral("MMMM d yyyy"),
    QStringLiteral("MMMM d yyyy hh:mm"),
    QStringLiteral("MMMM d yyyy hh:mm:ss"),
    QStringLiteral("MMMM d yyyy, hh:mm"),
    QStringLiteral("MMMM d yyyy, hh:mm:ss"),
    QStringLiteral("MMMM d yyyy, hh:mm:ss t"),
    QStringLiteral("MMM d, yyyy"),
    QStringLiteral("MMM d, yyyy hh:mm"),
    QStringLiteral("MMM d, yyyy hh:mm:ss"),
    QStringLiteral("MMMM d, yyyy"),
    QStringLiteral("MMMM d, yyyy hh:mm"),
    QStringLiteral("MMMM d, yyyy hh:mm:ss"),
    QStringLiteral("MMMM d, yyyy hh:mm:ss t"),
    QStringLiteral("d MMM yyyy"),
    QStringLiteral("d MMM yyyy hh:mm"),
    QStringLiteral("d MMM yyyy hh:mm:ss"),
    QStringLiteral("d MMM yyyy, hh:mm"),
    QStringLiteral("d MMM yyyy, hh:mm:ss"),
    QStringLiteral("d MMMM yyyy"),
    QStringLiteral("d MMMM yyyy hh:mm"),
    QStringLiteral("d MMMM yyyy hh:mm:ss"),
    QStringLiteral("d MMMM yyyy, hh:mm"),
    QStringLiteral("d MMMM yyyy, hh:mm:ss"),
    QStringLiteral("d MMM, yyyy"),
    QStringLiteral("d MMM, yyyy hh:mm"),
    QStringLiteral("d MMM, yyyy hh:mm:ss"),
    QStringLiteral("d MMMM, yyyy"),
    QStringLiteral("d MMMM, yyyy hh:mm"),
    QStringLiteral("d MMMM, yyyy hh:mm:ss"),
    QStringLiteral("yyyy-MM-ddThh:mm:ss.zt"),
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
