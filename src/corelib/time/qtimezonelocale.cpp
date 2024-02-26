// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qtimezonelocale_p.h>
#include <private/qtimezoneprivate_p.h>

#if !QT_CONFIG(icu)
#  include <private/qdatetime_p.h>
// Use data generated from CLDR:
#  include <private/qtimezonelocale_data_p.h>
#  include <private/qtimezoneprivate_data_p.h>
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(icu) // Get data from ICU:
namespace {

// Convert TimeType and NameType into ICU UCalendarDisplayNameType
constexpr UCalendarDisplayNameType ucalDisplayNameType(QTimeZone::TimeType timeType,
                                                       QTimeZone::NameType nameType)
{
    // TODO ICU C UCalendarDisplayNameType does not support full set of C++ TimeZone::EDisplayType
    // For now, treat Generic as Standard
    switch (nameType) {
    case QTimeZone::OffsetName:
        Q_UNREACHABLE(); // Callers of ucalTimeZoneDisplayName() should take care of OffsetName.
    case QTimeZone::ShortName:
        return timeType == QTimeZone::DaylightTime ? UCAL_SHORT_DST : UCAL_SHORT_STANDARD;
    case QTimeZone::DefaultName:
    case QTimeZone::LongName:
        return timeType == QTimeZone::DaylightTime ? UCAL_DST : UCAL_STANDARD;
    }
    Q_UNREACHABLE_RETURN(UCAL_STANDARD);
}

} // nameless namespace

namespace QtTimeZoneLocale {

// Qt wrapper around ucal_getTimeZoneDisplayName()
// Used directly by ICU backend; indirectly by TZ (see below).
QString ucalTimeZoneDisplayName(UCalendar *ucal,
                                QTimeZone::TimeType timeType,
                                QTimeZone::NameType nameType,
                                const QByteArray &localeCode)
{
    constexpr int32_t BigNameLength = 50;
    int32_t size = BigNameLength;
    QString result(size, Qt::Uninitialized);
    auto dst = [&result]() { return reinterpret_cast<UChar *>(result.data()); };
    UErrorCode status = U_ZERO_ERROR;
    const UCalendarDisplayNameType utype = ucalDisplayNameType(timeType, nameType);

    // size = ucal_getTimeZoneDisplayName(cal, type, locale, result, resultLength, status)
    size = ucal_getTimeZoneDisplayName(ucal, utype, localeCode.constData(),
                                       dst(), size, &status);

    // If overflow, then resize and retry
    if (size > BigNameLength || status == U_BUFFER_OVERFLOW_ERROR) {
        result.resize(size);
        status = U_ZERO_ERROR;
        size = ucal_getTimeZoneDisplayName(ucal, utype, localeCode.constData(),
                                           dst(), size, &status);
    }

    if (!U_SUCCESS(status))
        return QString();

    // Resize and return:
    result.resize(size);
    return result;
}

} // QtTimeZoneLocale

// Used by TZ backends when ICU is available:
QString QTimeZonePrivate::localeName(qint64 atMSecsSinceEpoch, int offsetFromUtc,
                                     QTimeZone::TimeType timeType,
                                     QTimeZone::NameType nameType,
                                     const QLocale &locale) const
{
    Q_UNUSED(atMSecsSinceEpoch);
    // TODO: use CLDR data for the offset name.
    // No ICU API for offset formats, so fall back to our ISO one, even if
    // locale isn't C:
    if (nameType == QTimeZone::OffsetName)
        return isoOffsetFormat(offsetFromUtc);

    const QString id = QString::fromUtf8(m_id);
    const QByteArray loc = locale.name().toUtf8();
    UErrorCode status = U_ZERO_ERROR;
    UCalendar *ucal = ucal_open(reinterpret_cast<const UChar *>(id.data()), id.size(),
                                loc.constData(), UCAL_DEFAULT, &status);
    if (ucal && U_SUCCESS(status)) {
        auto tidier = qScopeGuard([ucal]() { ucal_close(ucal); });
        return QtTimeZoneLocale::ucalTimeZoneDisplayName(ucal, timeType, nameType, loc);
    }
    return QString();
}
#else // No ICU, use QTZ[LP}_data_p.h data for feature timezone_locale.
namespace {
using namespace QtTimeZoneLocale; // QTZL_p.h QTZL_data_p.h
using namespace QtTimeZoneCldr; // QTZP_data_p.h
// Accessors for the QTZL_data_p.h

// Accessors for the QTZP_data_p.h
quint32 clipEpochMinute(qint64 epochMinute)
{
    // ZoneMetaHistory's quint32 UTC epoch minutes.
    // Dates from 1970-01-01 to 10136-02-16 (at 04:14) are representable.
    constexpr quint32 epoch = 0;
    // Since the .end value of an interval that does end is the first epoch
    // minutes *after* the interval, intervalEndsBefore() uses a <= test. The
    // value ~epoch (0xffffffff) is used as a sentinel value to mean "there is
    // no end", so we need a value strictly less than it for "epoch minutes too
    // big to represent" so that this value is less than "no end". So the value
    // 1 ^ ~epoch (0xfffffffe) is reserved as this "unrepresentably late time"
    // and the scripts to generate data assert that no actual interval ends then
    // or later.
    constexpr quint32 ragnarok = 1 ^ ~epoch;
    return epochMinute + 1 >= ragnarok ? ragnarok : quint32(epochMinute);
}

constexpr bool intervalEndsBefore(const ZoneMetaHistory &record, quint32 dt) noexcept
{
    // See clipEpochMinute()'s explanation of ragnarok for why this is <=
    return record.end <= dt;
}

/* The metaZoneKey of the ZoneMetaHistory entry whose ianaId() is equal to the
   given zoneId, for which atMSecsSinceEpoch refers to an instant between its
   begin and end. Returns zero if there is no such ZoneMetaHistory entry.
*/
quint16 metaZoneAt(QByteArrayView zoneId, qint64 atMSecsSinceEpoch)
{
    using namespace QtPrivate::DateTimeConstants;
    auto it = std::lower_bound(std::begin(zoneHistoryTable), std::end(zoneHistoryTable), zoneId,
                               [](const ZoneMetaHistory &record, QByteArrayView id) {
                                   return record.ianaId().compare(id, Qt::CaseInsensitive) < 0;
                               });
    if (it == std::end(zoneHistoryTable) || it->ianaId().compare(zoneId, Qt::CaseInsensitive) > 0)
        return 0;
    const auto stop =
        std::upper_bound(it, std::end(zoneHistoryTable), zoneId,
                         [](QByteArrayView id, const ZoneMetaHistory &record) {
                             return id.compare(record.ianaId(), Qt::CaseInsensitive) < 0;
                         });
    const quint32 dt = clipEpochMinute(atMSecsSinceEpoch / MSECS_PER_MIN);
    it = std::lower_bound(it, stop, dt, intervalEndsBefore);
    return it != stop && it->begin <= dt ? it->metaZoneKey : 0;
}

constexpr bool dataBeforeMeta(const MetaZoneData &row, quint16 metaKey) noexcept
{
    return row.metaZoneKey < metaKey;
}

constexpr bool metaDataBeforeTerritory(const MetaZoneData &row, qint16 territory) noexcept
{
    return row.territory < territory;
}

const MetaZoneData *metaZoneStart(quint16 metaKey)
{
    const MetaZoneData *const from =
        std::lower_bound(std::begin(metaZoneTable), std::end(metaZoneTable),
                         metaKey, dataBeforeMeta);
    if (from == std::end(metaZoneTable) || from->metaZoneKey != metaKey) {
        qWarning("No metazone data found for metazone key %d", metaKey);
        return nullptr;
    }
    return from;
}

const MetaZoneData *metaZoneDataFor(const MetaZoneData *from, QLocale::Territory territory)
{
    const quint16 metaKey = from->metaZoneKey;
    const MetaZoneData *const end =
        std::lower_bound(from, std::end(metaZoneTable), metaKey + 1, dataBeforeMeta);
    Q_ASSERT(end != from && end[-1].metaZoneKey == metaKey);
    QLocale::Territory land = territory;
    do {
        const MetaZoneData *row =
            std::lower_bound(from, end, qint16(land), metaDataBeforeTerritory);
        if (row != end && QLocale::Territory(row->territory) == land) {
            Q_ASSERT(row->metaZoneKey == metaKey);
            return row;
        }
        // Fall back to World (if territory itself isn't World).
    } while (std::exchange(land, QLocale::World) != QLocale::World);

    qWarning() << "Metazone" << from->metaZoneId() << "lacks World data for"
               << QLocale::territoryToString(territory);
    return nullptr;
}

} // nameless namespace

namespace QtTimeZoneLocale {

QList<QByteArrayView> ianaIdsForTerritory(QLocale::Territory territory)
{
    QList<QByteArrayView> result;
    {
        const TerritoryZone *row =
            std::lower_bound(std::begin(territoryZoneMap), std::end(territoryZoneMap),
                             qint16(territory),
                             [](const TerritoryZone &row, qint16 territory) {
                                 return row.territory < territory;
                             });
        if (row != std::end(territoryZoneMap) && QLocale::Territory(row->territory) == territory)
            result << row->ianaId();
    }
    for (const MetaZoneData &row : metaZoneTable) {
        if (QLocale::Territory(row.territory) == territory)
            result << row.ianaId();
    }
    return result;
}

}

QString QTimeZonePrivate::localeName(qint64 atMSecsSinceEpoch, int offsetFromUtc,
                                     QTimeZone::TimeType timeType,
                                     QTimeZone::NameType nameType,
                                     const QLocale &locale) const
{
    Q_ASSERT(nameType != QTimeZone::OffsetName || locale.language() != QLocale::C);
    // Get data from QTZ[LP]_data_p.h
    QByteArrayView iana{m_id};
    if (quint16 metaKey = metaZoneAt(iana, atMSecsSinceEpoch)) {
        if (auto metaFrom = metaZoneStart(metaKey)) {
            quint16 metaIdIndex = metaFrom->metaIdIndex;
            Q_UNUSED(metaIdIndex);

            if (const auto *metaRow = metaZoneDataFor(metaFrom, locale.territory()))
                iana = metaRow->ianaId(); // Use IANA ID of zone in use at that time
        }
    }
    Q_UNUSED(iana);

    Q_UNUSED(offsetFromUtc);
    Q_UNUSED(timeType);
    return QString();
}
#endif // ICU or not

QT_END_NAMESPACE
