// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QPARSE_TIMEZONE_P_H
#define QPARSE_TIMEZONE_P_H

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

#include <QtCore/private/qtparsecommon_p.h>
#include <QtCore/private/qdatetime_p.h>
#include <QtCore/qflags.h>
#include <QtCore/qlocale.h>
#include <QtCore/qtimezone.h>
#include <QtCore/private/qttemporalpattern_p.h>

QT_REQUIRE_CONFIG(datetimeparser);

QT_BEGIN_NAMESPACE

namespace QtParseTimeZone {

struct ParsedZone : public QtParseCommon::ParsedText
{
    QTimeZone zone{QTimeZone::LocalTime};
    QDateTimePrivate::DaylightStatus timeType = QDateTimePrivate::UnknownDaylightTime;

    constexpr QDateTime::TransitionResolution resolveType() const noexcept
    {
        // How to resolve QDateTime construction to favour the timeType parsed:
        using Res = QDateTime::TransitionResolution;
        switch (timeType) {
        case QDateTimePrivate::StandardTime: return Res::PreferStandard;
        case QDateTimePrivate::DaylightTime: return Res::PreferDaylightSaving;
        case QDateTimePrivate::UnknownDaylightTime: return Res::LegacyBehavior;
        }
        Q_UNREACHABLE_RETURN(Res::LegacyBehavior);
    }
};

constexpr QtTemporalPattern::TemporalFieldFlags AnyOffsetForm{
    QtTemporalPattern::TemporalFieldFlag::Numeric };
constexpr QtTemporalPattern::TemporalFieldFlags AnyGlobalZoneName{
    QtTemporalPattern::TemporalFieldFlag::Verbal
    | QtTemporalPattern::TemporalFieldFlag::Standalone };

// Corresponding to QDTP::findTimeZone's modes (count('t') in Qt format field):
constexpr QtTemporalPattern::TemporalFieldFlags AnyZoneName{
    AnyGlobalZoneName | QtTemporalPattern::TemporalFieldFlag::LocalTimeName }; // mode 4
constexpr QtTemporalPattern::TemporalFieldFlags BasicColonDigitOffset{ // mode 3
    QtTemporalPattern::TemporalFieldFlag::Numeric
    | QtTemporalPattern::TemporalFieldFlag::Abbreviated
    | QtTemporalPattern::TemporalFieldFlag::Narrow };
    // TODO: distinguish modes 2 and 3
constexpr QtTemporalPattern::TemporalFieldFlags BasicDigitOnlyOffset{ // mode 2
    QtTemporalPattern::TemporalFieldFlag::Numeric
    | QtTemporalPattern::TemporalFieldFlag::Abbreviated
    | QtTemporalPattern::TemporalFieldFlag::Narrow };
constexpr QtTemporalPattern::TemporalFieldFlags AllLegacyForm{ // mode 1
    QtTemporalPattern::TemporalFieldFlag::AllowZSuffix | BasicDigitOnlyOffset | BasicColonDigitOffset
    | QtTemporalPattern::TemporalFieldFlag::Wide
    | QtTemporalPattern::TemporalFieldFlag::Short | AnyZoneName };
constexpr QtTemporalPattern::TemporalFieldFlags AnyGlobalZoneForm{
    QtTemporalPattern::TemporalFieldFlag::AllowZSuffix | AnyOffsetForm | AnyGlobalZoneName };
constexpr QtTemporalPattern::TemporalFieldFlags AnyZoneForm{
    QtTemporalPattern::TemporalFieldFlag::AllowZSuffix | AnyOffsetForm | AnyZoneName };

// Parsing functions return lists, sorted with "better" matches earlier; for
// now, longer is better.

Q_AUTOTEST_EXPORT // so that we can test it
QList<ParsedZone> prefix(QStringView text, const QLocale &locale, qsizetype from = 0,
                         QtTemporalPattern::TemporalFieldFlags flags = AnyZoneForm);
// QList<ParsedZone> find(QStringView text, const QLocale &locale, qsizetype from = 0,
//                        QtTemporalPattern::TemporalFieldFlags flags = AnyZoneForm);
} // QtParseTimeZone

QT_END_NAMESPACE

#endif // QPARSE_TIMEZONE_P_H
