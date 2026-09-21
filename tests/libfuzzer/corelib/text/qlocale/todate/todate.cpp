// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCalendar>
#include <QDate>
#include <QLatin1StringView>
#include <QLocale>
#include <QString>

// Enable reporting of the currently used format, e.g. when reproducing issues
// #define LOG_FORMAT
#ifdef LOG_FORMAT
#include <QDebug>
#endif
using namespace Qt::StringLiterals;

static constexpr struct {
    QLatin1StringView format = {}; // isNull() => iterate FormatType
    QCalendar::System calSys = QCalendar::System::Gregorian;
    int baseYear = 1900;
} patterns[] = {
    {},
    { {}, QCalendar::System::Julian },
    { {}, QCalendar::System::Milankovic },
#if QT_CONFIG(jalalicalendar)
    { {}, QCalendar::System::Jalali },
#endif
#if QT_CONFIG(islamiccivilcalendar)
    { {}, QCalendar::System::IslamicCivil },
#endif
    { ""_L1 }, // Accepts only an empty string, returns default
    // Each supported format atom:
    { "yy"_L1, QCalendar::System::Gregorian, 2000 },
    { "yyyy"_L1 },
    { "M"_L1 },
    { "MM"_L1 },
    { "MMM"_L1 },
    { "MMMM"_L1 },
    { "d"_L1 },
    { "dd"_L1 },
    { "ddd"_L1 },
    { "dddd"_L1 },
    // Specific formats (or date parts thereof):
    { "yyyyMMdd"_L1 }, // ISO8601
    { "yyMMdd"_L1, QCalendar::System::Gregorian, 1950 }, // ASN.1 UTC type
    { "yyyyMMdd"_L1 }, // ASN.1 generalized type
    // Quote handling
    { "yyyy'-'MM'-'dd"_L1 }, // correctly matched quotes
    { "yyyy''MM''dd"_L1 }, // doubled quotes as literal quotes
    { "yyyy MMMM dddd'unmatched"_L1 }, // malformed - rejected
};

static constexpr QLatin1StringView locales[] = {
    "C"_L1, "en-US"_L1, "fr-FR"_L1,
    "ru-RU"_L1, "zh-CN"_L1, "ar-EG"_L1, "hi-IN"_L1, "ja-JP"_L1,
    "ccp-BD"_L1, "ff-Adlm"_L1, // non-BMP digits
};

// libFuzzer entry-point for testing QLocale::toDate()
extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size)
{
    const QString userString = QString::fromUtf8(Data, Size);

    constexpr QLocale::FormatType formats[] = {
        QLocale::LongFormat, QLocale::ShortFormat, QLocale::NarrowFormat
    };
    [[maybe_unused]] QDate date;
    for (const auto &pattern : patterns) {
        QCalendar cal(pattern.calSys);
        if (!cal.isValid())
            continue;
        for (const auto loc : locales) {
            QLocale locale(loc);
#ifdef LOG_FORMAT
            qDebug() << "Trying format:" << pattern.format
                     << "for locale:" << loc
                     << "with base year:" << pattern.baseYear
                     << "of calendar:" << cal.name();
#endif
            if (pattern.format.isNull()) {
                for (const auto form : formats)
                    date = locale.toDate(userString, form, cal, pattern.baseYear);
            } else {
                date = locale.toDate(userString, pattern.format, cal, pattern.baseYear);
            }
        }
    }
    return 0;
}
