// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtimezoneprivate_p.h"

#include <chrono>

QT_BEGIN_NAMESPACE

using namespace std::chrono;
using namespace std::chrono_literals;

#define EXCEPTION_CHECKED(expression, fallback) \
    QT_TRY { \
        expression; \
    } QT_CATCH (const std::runtime_error &) { \
        fallback; \
    }

static std::chrono::sys_time<std::chrono::milliseconds>
chronoForEpochMillis(qint64 millis)
{
    return sys_time<milliseconds>(milliseconds(millis));
}

static std::optional<std::chrono::sys_info>
infoAtEpochMillis(const std::chrono::time_zone *zone, qint64 millis)
{
    EXCEPTION_CHECKED(return zone->get_info(chronoForEpochMillis(millis)), return std::nullopt);
}

static const std::chrono::time_zone *idToZone(std::string_view id)
{
    EXCEPTION_CHECKED(return get_tzdb().locate_zone(id), return nullptr);
}

static QChronoTimeZonePrivate::Data
fromSysInfo(std::chrono::sys_info info, qint64 atMSecsSinceEpoch)
{
    QString abbreviation = QString::fromLatin1(info.abbrev);
    int offsetFromUtc = info.offset.count();
    // offset is in seconds, save is in minutes
    int standardTimeOffset = offsetFromUtc - seconds(info.save).count();
    return QChronoTimeZonePrivate::Data(abbreviation, atMSecsSinceEpoch,
                                        offsetFromUtc, standardTimeOffset);
}

QChronoTimeZonePrivate::QChronoTimeZonePrivate()
    : m_timeZone(std::chrono::current_zone())
{
    if (m_timeZone)
        m_id.assign(m_timeZone->name());
}

QChronoTimeZonePrivate::QChronoTimeZonePrivate(QByteArrayView id)
    : m_timeZone(idToZone(std::string_view(id.data(), id.size())))
{
    if (m_timeZone)
        m_id.assign(m_timeZone->name());
}

QChronoTimeZonePrivate::~QChronoTimeZonePrivate()
        = default;

QByteArray QChronoTimeZonePrivate::systemTimeZoneId() const
{
    if (const time_zone *zone = std::chrono::current_zone()) {
        std::string_view name = zone->name();
        return {name.data(), qsizetype(name.size())};
    }
    return {};
}

QString QChronoTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    if (auto info = infoAtEpochMillis(m_timeZone, atMSecsSinceEpoch))
        return fromSysInfo(*info, atMSecsSinceEpoch).abbreviation;
    return {};
}

int QChronoTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    if (auto info = infoAtEpochMillis(m_timeZone, atMSecsSinceEpoch))
        return fromSysInfo(*info, atMSecsSinceEpoch).offsetFromUtc;
    return invalidSeconds();
}

int QChronoTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    // Subtracting minutes from seconds will convert the minutes to seconds.
    if (auto info = infoAtEpochMillis(m_timeZone, atMSecsSinceEpoch))
        return int((info->offset - info->save).count());
    return invalidSeconds();
}

int QChronoTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    if (auto info = infoAtEpochMillis(m_timeZone, atMSecsSinceEpoch))
        return int(std::chrono::seconds(info->save).count());
    return invalidSeconds();
}

bool QChronoTimeZonePrivate::hasDaylightTime() const
{
    Data data = QTimeZonePrivate::data(QTimeZone::DaylightTime);
    return data.daylightTimeOffset != 0
        && data.daylightTimeOffset != invalidSeconds();
}

bool QChronoTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    if (auto info = infoAtEpochMillis(m_timeZone, atMSecsSinceEpoch))
        return info->save != 0min;
    return false;
}

QChronoTimeZonePrivate::Data
QChronoTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    if (auto info = infoAtEpochMillis(m_timeZone, forMSecsSinceEpoch))
        return fromSysInfo(*info, forMSecsSinceEpoch);
    return {};
}

bool QChronoTimeZonePrivate::hasTransitions() const
{
    return true;
}

QChronoTimeZonePrivate::Data
QChronoTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    if (const auto info = infoAtEpochMillis(m_timeZone, afterMSecsSinceEpoch)) {
        const auto tran = info->end;
        qint64 when = milliseconds(tran.time_since_epoch()).count();
        if (when > afterMSecsSinceEpoch)
            return fromSysInfo(m_timeZone->get_info(tran), when);
        // else we were already at (or after) the end-of-time "transition"
    }
    return {};
}

QChronoTimeZonePrivate::Data
QChronoTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    if (const auto info = infoAtEpochMillis(m_timeZone, beforeMSecsSinceEpoch - 1)) {
        qint64 when = milliseconds(info->begin.time_since_epoch()).count();
        if (when < beforeMSecsSinceEpoch)
            return fromSysInfo(*info, when);
        // else we were already at (or before) the start-of-time "transition"
    }
    return {};
}

QT_END_NAMESPACE
