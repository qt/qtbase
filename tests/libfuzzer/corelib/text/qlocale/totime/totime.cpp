// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTime>
#include <QLatin1StringView>
#include <QLocale>
#include <QString>

// Enable reporting of the currently used format, e.g. when reproducing issues
// #define LOG_FORMAT
#ifdef LOG_FORMAT
#include <QDebug>
#endif
using namespace Qt::StringLiterals;

// isNull() => iterate FormatType
static constexpr QLatin1StringView formats[] = {
    {},
    ""_L1, // Accepts only an empty string, returns default
    // Each supported format atom:
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
    "a"_L1,
    "A"_L1,
    "Ap"_L1,
    // Specific formats (or time parts thereof):
    "HHmmss.zzz"_L1, // ISO8601
    "HHmmss"_L1, // ASN.1 UTC type
    "HHmmsst"_L1, // ASN.1 generalized type
    // Quote handling
    { "HHmmss'.'zzz"_L1 }, // correctly matched quotes
    { "HHmmss''zzz"_L1 }, // doubled quotes as literal quotes
    { "HH mm ss.zzz'unmatched"_L1 }, // malformed - rejected
};

static constexpr QLatin1StringView locales[] = {
    "C"_L1, "en-US"_L1, "fr-FR"_L1,
    "ru-RU"_L1, "zh-CN"_L1, "ar-EG"_L1, "hi-IN"_L1, "ja-JP"_L1,
    "ccp-BD"_L1, "ff-Adlm"_L1, // non-BMP digits
};

// libFuzzer entry-point for testing QLocale::toTime()
extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size)
{
    const QString userString = QString::fromUtf8(Data, Size);

    constexpr QLocale::FormatType forms[] = {
        QLocale::LongFormat, QLocale::ShortFormat, QLocale::NarrowFormat
    };
    [[maybe_unused]] QTime time;
    for (const auto &format : formats) {
        for (const auto loc : locales) {
            QLocale locale(loc);
#ifdef LOG_FORMAT
            qDebug() << "Trying format:" << format << "for locale:" << loc;
#endif
            if (format.isNull()) {
                for (const auto form : forms)
                    time = locale.toTime(userString, form);
            } else {
                time = locale.toTime(userString, format);
            }
        }
    }
    return 0;
}
