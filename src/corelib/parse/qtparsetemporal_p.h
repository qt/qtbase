// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QTPARSE_TEMPORAL_P_H
#define QTPARSE_TEMPORAL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qcalendar.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qflags.h>
#include <QtCore/qlocale.h>
#include <QtCore/qstringview.h>
#include <QtCore/qtimezone.h>

#include <QtCore/private/qtparsetimezone_p.h>
#include <QtCore/private/qttemporalpattern_p.h>

#include <optional>

QT_REQUIRE_CONFIG(datetimeparser);

QT_BEGIN_NAMESPACE

namespace QtParseTemporal {
struct ParsedTemporal : public QtParseTimeZone::ParsedZone {
    // Time fields:
    int millis = -1;
    int second = -1;
    int minute = -1;
    int hour = -1; // Before parsing last field, -1 just means unknown hour.
    // After: -1: fully unknown, < -1: unknown am, > 24 unknown pm, otherwise known,
    // Date fields:
    int dayOfWeek = 0;
    int dayOfMonth = 0;
    int month = 0;
    // Some calendars do allow any integer as year, for all that we implement
    // most without a year zero. So use an optional for the year.
    std::optional<int> year;
    // Indexing (of boundaries between fields):
    std::vector<qsizetype> bounds;
    // (See ParsedText base for start and end; and ParsedZone for zone.)

    // Convenience accessors for the data fields:
    QDate date(QCalendar cal, QDate defaults = {}) const;
    QTime time(QTime defaults = {}) const;
    // The latter uses hour's quirky am/pm hints to adapt defaults.hour().
};

Q_AUTOTEST_EXPORT
ParsedTemporal prefix(QStringView text, QSpan<const QtTemporalPattern::TemporalField> fields,
                      const QLocale &locale, QCalendar cal,
                      std::optional<int> baseYear = std::nullopt,
                      qsizetype from = 0);
} // QtParseTemporal

QT_END_NAMESPACE

#endif // QTPARSE_TEMPORAL_P_H
