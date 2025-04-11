// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qtimezonelocale_p.h>
#include <private/qtimezoneprivate_p.h>

#if !QT_CONFIG(icu)
#  include <QtCore/qspan.h>
#  include <private/qdatetime_p.h>
// Use data generated from CLDR:
#  include "qtimezonelocale_data_p.h"
#  include "qtimezoneprivate_data_p.h"
#  ifdef QT_CLDR_ZONE_DEBUG
#    include "../text/qlocale_data_p.h"
QT_BEGIN_NAMESPACE
static_assert(std::size(locale_data) == std::size(QtTimeZoneLocale::localeZoneData));
// Size includes terminal rows: for now, they do match in tag IDs, but they needn't.
static_assert([]() {
    for (std::size_t i = 0; i < std::size(locale_data); ++i) {
        const auto &loc = locale_data[i];
        const auto &zone = QtTimeZoneLocale::localeZoneData[i];
        if (loc.m_language_id != zone.m_language_id
                || loc.m_script_id != zone.m_script_id
                || loc.m_territory_id != zone.m_territory_id) {
            return false;
        }
    }
    return true;
}());
QT_END_NAMESPACE
#  endif
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(icu) // Get data from ICU:
namespace {

// Convert TimeType and NameType into ICU UCalendarDisplayNameType
UCalendarDisplayNameType ucalDisplayNameType(QTimeZone::TimeType timeType,
                                             QTimeZone::NameType nameType)
{
    // TODO ICU C UCalendarDisplayNameType does not support full set of C++ TimeZone::EDisplayType
    // For now, treat Generic as Standard
    switch (nameType) {
    case QTimeZone::ShortName:
        return timeType == QTimeZone::DaylightTime ? UCAL_SHORT_DST : UCAL_SHORT_STANDARD;
    case QTimeZone::DefaultName:
    case QTimeZone::LongName:
        return timeType == QTimeZone::DaylightTime ? UCAL_DST : UCAL_STANDARD;
    case QTimeZone::OffsetName:
        Q_UNREACHABLE(); // Callers of ucalTimeZoneDisplayName() should take care of OffsetName.
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

bool ucalKnownTimeZoneId(const QString &ianaStr)
{
    const UChar *const name = reinterpret_cast<const UChar *>(ianaStr.constData());
    // We are not interested in the value, but we have to pass something.
    // No known IANA zone name is (up to 2023) longer than 30 characters.
    constexpr size_t size = 64;
    UChar buffer[size];

    // TODO: convert to ucal_getIanaTimeZoneID(), new draft in ICU 74, once we
    // can rely on its availability, assuming it works the same once not draft.
    UErrorCode status = U_ZERO_ERROR;
    UBool isSys = false;
    // Returns the length of the IANA zone name (but we don't care):
    ucal_getCanonicalTimeZoneID(name, ianaStr.size(), buffer, size, &isSys, &status);
    // We're only interested if the result is a "system" (i.e. IANA) ID:
    return isSys;
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
    // Need to check id is known to ICU, since ucal_open() will return a
    // misleading "valid" GMT ucal when it doesn't recognise id.
    if (!QtTimeZoneLocale::ucalKnownTimeZoneId(id))
        return QString();

    const QByteArray loc = locale.name().toUtf8();
    UErrorCode status = U_ZERO_ERROR;
    // TODO: QTBUG-124271 can we cache any of this ?
    UCalendar *ucal = ucal_open(reinterpret_cast<const UChar *>(id.data()), id.size(),
                                loc.constData(), UCAL_DEFAULT, &status);
    if (ucal && U_SUCCESS(status)) {
        auto tidier = qScopeGuard([ucal]() { ucal_close(ucal); });
        return QtTimeZoneLocale::ucalTimeZoneDisplayName(ucal, timeType, nameType, loc);
    }
    return QString();
}
#else // No ICU, use QTZ[LP]_data_p.h data for feature timezone_locale.
namespace QtTimeZoneLocale {
// Inline methods promised in QTZL_p.h
using namespace QtTimeZoneCldr; // QTZP_data_p.h
constexpr QByteArrayView LocaleZoneExemplar::ianaId() const { return ianaIdData + ianaIdIndex; }
constexpr QByteArrayView LocaleZoneNames::ianaId() const { return ianaIdData + ianaIdIndex; }
} // QtTimeZoneLocale

namespace {
using namespace QtTimeZoneLocale; // QTZL_p.h QTZL_data_p.h
using namespace QtTimeZoneCldr; // QTZP_data_p.h
// Accessors for the QTZL_data_p.h

template <typename Row, typename Sought, typename Condition>
const Row *findTableEntryFor(const QSpan<Row> data, Sought value, Condition test)
{
    // We have the present locale's data (if any). Its rows are sorted on
    // (localeIndex and) a field for which we want the Sought value. The test()
    // compares that field.
    auto begin = data.begin(), end = data.end();
    Q_ASSERT(begin == end || end->localeIndex > begin->localeIndex);
    Q_ASSERT(begin == end || end[-1].localeIndex == begin->localeIndex);
    auto row = std::lower_bound(begin, end, value, test);
    return row == end ? nullptr : row;
}

QString exemplarCityFor(const LocaleZoneData &locale, const LocaleZoneData &next,
                        QByteArrayView iana)
{
    auto xct = findTableEntryFor(
        QSpan(localeZoneExemplarTable).first(next.m_exemplarTableStart
            ).sliced(locale.m_exemplarTableStart),
        iana, [](auto &row, QByteArrayView key) { return row.ianaId() < key; });
    if (xct && xct->ianaId() == iana)
        return xct->exemplarCity().getData(exemplarCityTable);
    return {};
}

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

    qWarning("Metazone %s lacks World data for %ls",
             from->metaZoneId().constData(),
             qUtf16Printable(QLocale::territoryToString(territory)));
    return nullptr;
}

QString addPadded(qsizetype width, const QString &zero, const QString &number, QString &&onto)
{
    // TODO (QTBUG-122834): QLocale::toString() should support zero-padding directly.
    width -= number.size() / zero.size();
    while (width > 0) {
        onto += zero;
        --width;
    }
    return std::move(onto) + number;
}

QString formatOffset(QStringView format, int offsetMinutes, const QLocale &locale,
                     QLocale::FormatType form)
{
    Q_ASSERT(offsetMinutes >= 0);
    const QString hour = locale.toString(offsetMinutes / 60);
    const QString mins = locale.toString(offsetMinutes % 60);
    // If zero.size() > 1, digits are surrogate pairs; each only counts one
    // towards width of the field, even if it contributes more to result.size().
    const QString zero = locale.zeroDigit();
    QStringView tail = format;
    QString result;
    while (!tail.isEmpty()) {
        if (tail.startsWith(u'\'')) {
            qsizetype end = tail.indexOf(u'\'', 1);
            if (end < 0) {
                qWarning("Unbalanced quote in offset format string: %s",
                         format.toUtf8().constData());
                return result + tail; // Include the quote; format is bogus.
            } else if (end == 1) {
                // Special case: adjacent quotes signify a simple quote.
                result += u'\'';
                tail = tail.sliced(2);
            } else {
                Q_ASSERT(end > 1); // We searched from index 1.
                while (end + 1 < tail.size() && tail[end + 1] == u'\'') {
                    // Special case: adjacent quotes inside a quoted string also
                    // signify a simple quote.
                    result += tail.sliced(1, end); // Include a quote at the end
                    tail = tail.sliced(end + 1); // Still starts with a quote
                    end = tail.indexOf(u'\'', 1); // Where's the next ?
                    if (end < 0) {
                        qWarning("Unbalanced quoted quote in offset format string: %s",
                                 format.toUtf8().constData());
                        return result + tail;
                    }
                    Q_ASSERT(end > 0);
                }
                // Skip leading and trailng quotes:
                result += tail.sliced(1, end - 1);
                tail = tail.sliced(end + 1);
            }
        } else if (tail.startsWith(u'H')) {
            qsizetype width = 1;
            while (width < tail.size() && tail[width] == u'H')
                ++width;
            tail = tail.sliced(width);
            if (form != QLocale::NarrowFormat)
                result = addPadded(width, zero, hour, std::move(result));
            else
                result += hour;
        } else if (tail.startsWith(u'm')) {
            qsizetype width = 1;
            while (width < tail.size() && tail[width] == u'm')
                ++width;
            // (At CLDR v45, all locales use two-digit minutes.)
            // (No known zone has single-digit non-zero minutes.)
            tail = tail.sliced(width);
            if (form != QLocale::NarrowFormat)
                result = addPadded(width, zero, mins, std::move(result));
            else if (offsetMinutes % 60)
                result += mins;
            else if (result.endsWith(u':') || result.endsWith(u'.'))
                result.chop(1);
            // (At CLDR v45, mm follows H either immediately or after a colon or dot.)
        } else if (tail[0].isHighSurrogate() && tail.size() > 1
                   && tail[1].isLowSurrogate()) {
            result += tail.first(2);
            tail = tail.sliced(2);
        } else {
            result += tail.front();
            tail = tail.sliced(1);
        }
    }
    return result;
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

// The QDateTime is only needed by the fall-back implementation in qlocale.cpp;
// the calls below don't need to pass a valid QDateTime (based on its
// atMSecsSinceEpoch); an invalid QDateTime() will suffice and be ignored.
QString zoneOffsetFormat(const QLocale &locale, qsizetype locInd, QLocale::FormatType width,
                         const QDateTime &, int offsetSeconds)
{
    // QLocale::LongFormat gets the full GMT-prefix plus hour offset.
    // QLocale::ShortFormat gets just the hour offset (with full with).
    // QLocale::NarrowFormat gets the GMT-prefix plus the pruned hour format.
    // The last drops :00 for zero minutes and removes leading 0 from the hour.
    const LocaleZoneData &locData = localeZoneData[locInd];

    auto hourFormatR = offsetSeconds < 0 ? locData.negHourFormat() : locData.posHourFormat();
    QStringView hourFormat = hourFormatR.viewData(hourFormatTable);
    Q_ASSERT(!hourFormat.isEmpty());
    // Sign is already handled by choice of the hourFormat:
    offsetSeconds = qAbs(offsetSeconds);
    // Offsets are only displayed in minutes - round seconds (if any) to nearest
    // minute (prefering an even minute when rounding an exact half):
    const int offsetMinutes = (offsetSeconds + 29 + (1 & (offsetSeconds / 60))) / 60;

    const QString hourOffset = formatOffset(hourFormat, offsetMinutes, locale, width);
    if (width == QLocale::ShortFormat)
        return hourOffset;

    QStringView offsetFormat = locData.offsetGmtFormat().viewData(gmtFormatTable);
    Q_ASSERT(!offsetFormat.isEmpty());
    return offsetFormat.arg(hourOffset);
}

} // QtTimeZoneLocale

QString QTimeZonePrivate::localeName(qint64 atMSecsSinceEpoch, int offsetFromUtc,
                                     QTimeZone::TimeType timeType,
                                     QTimeZone::NameType nameType,
                                     const QLocale &locale) const
{
    if (nameType == QTimeZone::OffsetName) {
        // Doesn't need fallbacks, since every locale has hour and offset formats.
        return QtTimeZoneLocale::zoneOffsetFormat(locale, locale.d->m_index, QLocale::LongFormat,
                                                  QDateTime(), offsetFromUtc);
    }

    // An IANA ID may give clues to fall back on for abbreviation or exemplar city:
    QByteArray ianaAbbrev, ianaTail;
    const auto scanIana = [&](QByteArrayView iana) {
        // Scan the name of each zone whose data we consider using and, if the
        // name gives us a clue to a fallback for which we have nothing better
        // yet, remember it (and ignore later clues for that fallback).
        if (!ianaAbbrev.isEmpty() && !ianaTail.isEmpty())
            return;
        qsizetype cut = iana.lastIndexOf('/');
        QByteArrayView tail = cut < 0 ? iana : iana.sliced(cut + 1);
        // Deal with a couple of special cases
        if (tail == "McMurdo") { // Exceptional lowercase-uppercase sequence without space
            if (ianaTail.isEmpty())
                ianaTail = "McMurdo"_ba;
            return;
        } else if (tail == "DumontDUrville") { // Chopped to fit into IANA's 14-char limit
            if (ianaTail.isEmpty())
                ianaTail = "Dumont d'Urville"_ba;
            return;
        } else if (tail.isEmpty()) {
            // Custom zone with perverse m_id ?
            return;
        }

        // Even if it is abbr or city name, we don't care if we've found one before.
        bool maybeAbbr = ianaAbbrev.isEmpty(), maybeCityName = ianaTail.isEmpty(), inword = false;
        char sign = '\0';
        for (char ch : tail) {
            if (ch == '+' || ch == '-') {
                if (ch == '+' || !inword)
                    maybeCityName = false;
                inword = false;
                if (maybeAbbr) {
                    if (sign)
                        maybeAbbr = false; // two signs: no
                    else
                        sign = ch;
                }
            } else if (ch == '_') {
                maybeAbbr = false;
                if (!inword) // No double-underscore, or leading underscore
                    maybeCityName = false;
                inword = false;
            } else if (QChar::isLower(ch)) {
                maybeAbbr = false;
                // Dar_es_Salaam shows both cases as word starts
                inword = true;
            } else if (QChar::isUpper(ch)) {
                if (sign)
                    maybeAbbr = false;
                if (inword)
                    maybeCityName = false;
                inword = true;
            } else if (QChar::isDigit(ch)) {
                if (!sign)
                    maybeAbbr = false;
                maybeCityName = false;
                inword = false;
            }

            if (!maybeAbbr && !maybeCityName)
                break;
        }
        if (maybeAbbr && maybeCityName) // No real IANA ID matches both
            return;

        if (maybeAbbr) {
            if (tail.endsWith("-0") || tail.endsWith("+0"))
                tail = tail.chopped(2);
            ianaAbbrev = tail.toByteArray();
            if (sign && iana.startsWith("Etc/")) { // Reverse convention for offsets
                if (sign == '-')
                    ianaAbbrev = ianaAbbrev.replace('-', '+');
                else if (sign == '+')
                    ianaAbbrev = ianaAbbrev.replace('+', '-');
            }
        }
        if (maybeCityName)
            ianaTail = tail.toByteArray().replace('_', ' ');
    }; // end scanIana

    scanIana(m_id);
    if (QByteArray iana = aliasToIana(m_id); !iana.isEmpty() && iana != m_id)
        scanIana(iana);

    // Requires locData, nextData set suitably - save repetition of member:
#define tableLookup(table, member, sought, test) \
    findTableEntryFor(QSpan(table).first(nextData.member).sliced(locData.member), sought, test)
    // Note: any commas in test need to be within parentheses; but the only
    // comma a comparison should need is in its (parenthesised) parameter list.

    const QList<qsizetype> indices = fallbackLocalesFor(locale.d->m_index);
    QString exemplarCity; // In case we need it.
    const auto metaIdBefore = [](auto &row, quint16 key) { return row.metaIdIndex < key; };

    // First try for an actual name:
    for (const qsizetype locInd : indices) {
        const LocaleZoneData &locData = localeZoneData[locInd];
        // After the row for the last actual locale, there's a terminal row:
        Q_ASSERT(std::size_t(locInd) < std::size(localeZoneData) - 1);
        const LocaleZoneData &nextData = localeZoneData[locInd + 1];

        QByteArrayView iana{m_id};
        if (quint16 metaKey = metaZoneAt(iana, atMSecsSinceEpoch)) {
            if (const MetaZoneData *metaFrom = metaZoneStart(metaKey)) {
                quint16 metaIdIndex = metaFrom->metaIdIndex;
                QLocaleData::DataRange range{0, 0};
                const char16_t *strings = nullptr;
                if (nameType == QTimeZone::ShortName) {
                    auto row = tableLookup(localeMetaZoneShortNameTable, m_metaShortTableStart,
                                           metaIdIndex, metaIdBefore);
                    if (row && row->metaIdIndex == metaIdIndex) {
                        range = row->shortName(timeType);
                        strings = shortMetaZoneNameTable;
                    }
                } else { // LongName or DefaultName
                    auto row = tableLookup(localeMetaZoneLongNameTable, m_metaLongTableStart,
                                           metaIdIndex, metaIdBefore);
                    if (row && row->metaIdIndex == metaIdIndex) {
                        range = row->longName(timeType);
                        strings = longMetaZoneNameTable;
                    }
                }
                Q_ASSERT(strings || !range.size);

                if (range.size)
                    return range.getData(strings);

                if (const auto *metaRow = metaZoneDataFor(metaFrom, locale.territory()))
                    iana = metaRow->ianaId(); // Use IANA ID of zone in use at that time
            }
        }

        // Use exemplar city from closest match to locale, m_id:
        if (exemplarCity.isEmpty()) {
            exemplarCity = exemplarCityFor(locData, nextData, m_id);
            if (exemplarCity.isEmpty())
                exemplarCity = exemplarCityFor(locData, nextData, iana);
        }
        if (iana != m_id) // Check for hints to abbreviation and exemplar city:
            scanIana(iana);

        // That may give us a revised IANA ID; if the first search fails, fall back
        // to m_id, if different.
        do {
            auto row = tableLookup(
                localeZoneNameTable, m_zoneTableStart,
                iana, [](auto &row, QByteArrayView key) { return row.ianaId() < key; });
            if (row && row->ianaId() == iana) {
                QLocaleData::DataRange range = row->name(nameType, timeType);
                if (range.size) {
                    auto table = nameType == QTimeZone::ShortName
                        ? shortZoneNameTable
                        : longZoneNameTable;
                    return range.getData(table);
                }
            }
        } while (std::exchange(iana, QByteArrayView{m_id}) != m_id);
    }
    // Most zones should now have ianaAbbrev or ianaTail set, maybe even both.
    // We've now tried all the candidates we'll see for those.
    // If an IANA ID's last component looked like a city name, use it.
    if (exemplarCity.isEmpty() && !ianaTail.isEmpty())
        exemplarCity = QString::fromLatin1(ianaTail); // It's ASCII

    switch (nameType) {
    case QTimeZone::DefaultName:
    case QTimeZone::LongName:
        for (const qsizetype locInd : indices) {
            const LocaleZoneData &locData = localeZoneData[locInd];
            QStringView regionFormat
                = locData.regionFormatRange(timeType).viewData(regionFormatTable);
            if (!regionFormat.isEmpty()) {
                QString where = exemplarCity;
                // TODO: if empty, use territory name
                if (!where.isEmpty())
                    return regionFormat.arg(where);
            }
        }
#if 0 // See comment within.
        for (const qsizetype locInd : indices) {
            const LocaleZoneData &locData = localeZoneData[locInd];
            QStringView fallbackFormat = locData.fallbackFormat().viewData(fallbackFormatTable);
            // Use fallbackFormat - probably never needed, as regionFormat is
            // never empty, and this also needs city or territory name (along
            // with metazone name).
        }
#endif
        break;

    case QTimeZone::ShortName:
        // If an IANA ID's last component looked like an abbreviation (UTC, EST, ...) use it.
        if (!ianaAbbrev.isEmpty())
            return QString::fromLatin1(ianaAbbrev); // It's ASCII
        break;

    case QTimeZone::OffsetName:
        Q_UNREACHABLE_RETURN(QString());
    }

#undef tableLookup

    // Final fall-back: ICU seems to use a compact form of offset time for
    // short-forms it doesn't know. This seems to correspond to the short form
    // of LDML's Localized GMT format.
    return QtTimeZoneLocale::zoneOffsetFormat(locale, locale.d->m_index, QLocale::NarrowFormat,
                                              QDateTime(), offsetFromUtc);
}
#endif // ICU or not

QT_END_NAMESPACE
