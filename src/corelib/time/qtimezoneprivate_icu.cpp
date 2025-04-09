// Copyright (C) 2013 John Layt <jlayt@kde.org>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtimezone.h"
#include "qtimezoneprivate_p.h"
#include "qtimezonelocale_p.h"

#include <unicode/ucal.h>

#include <qdebug.h>
#include <qlist.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

/*
    Private

    ICU implementation
*/

// ICU utilities

// Qt wrapper around ucal_getDefaultTimeZone()
static QByteArray ucalDefaultTimeZoneId()
{
    int32_t size = 30;
    QString result(size, Qt::Uninitialized);
    UErrorCode status = U_ZERO_ERROR;

    // size = ucal_getDefaultTimeZone(result, resultLength, status)
    size = ucal_getDefaultTimeZone(reinterpret_cast<UChar *>(result.data()), size, &status);

    // If overflow, then resize and retry
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        result.resize(size);
        status = U_ZERO_ERROR;
        size = ucal_getDefaultTimeZone(reinterpret_cast<UChar *>(result.data()), size, &status);
    }

    // If successful on first or second go, resize and return
    if (U_SUCCESS(status)) {
        result.resize(size);
        return std::move(result).toUtf8();
    }

    return QByteArray();
}

// Qt wrapper around ucal_get() for offsets
static bool ucalOffsetsAtTime(UCalendar *m_ucal, qint64 atMSecsSinceEpoch,
                              int *utcOffset, int *dstOffset)
{
    *utcOffset = 0;
    *dstOffset = 0;

    // Clone the ucal so we don't change the shared object
    UErrorCode status = U_ZERO_ERROR;
    UCalendar *ucal = ucal_clone(m_ucal, &status);
    if (!U_SUCCESS(status))
        return false;

    // Set the date to find the offset for
    status = U_ZERO_ERROR;
    ucal_setMillis(ucal, atMSecsSinceEpoch, &status);

    int32_t utc = 0;
    if (U_SUCCESS(status)) {
        status = U_ZERO_ERROR;
        // Returns msecs
        utc = ucal_get(ucal, UCAL_ZONE_OFFSET, &status) / 1000;
    }

    int32_t dst = 0;
    if (U_SUCCESS(status)) {
        status = U_ZERO_ERROR;
        // Returns msecs
        dst = ucal_get(ucal, UCAL_DST_OFFSET, &status) / 1000;
    }

    ucal_close(ucal);
    if (U_SUCCESS(status)) {
        *utcOffset = utc;
        *dstOffset = dst;
        return true;
    }
    return false;
}

#if U_ICU_VERSION_MAJOR_NUM >= 50
// Qt wrapper around qt_ucal_getTimeZoneTransitionDate & ucal_get
static QTimeZonePrivate::Data ucalTimeZoneTransition(UCalendar *m_ucal,
                                                     UTimeZoneTransitionType type,
                                                     qint64 atMSecsSinceEpoch)
{
    QTimeZonePrivate::Data tran;

    // Clone the ucal so we don't change the shared object
    UErrorCode status = U_ZERO_ERROR;
    UCalendar *ucal = ucal_clone(m_ucal, &status);
    if (!U_SUCCESS(status))
        return tran;

    // Set the date to find the transition for
    status = U_ZERO_ERROR;
    ucal_setMillis(ucal, atMSecsSinceEpoch, &status);

    // Find the transition time
    UDate tranMSecs = 0;
    status = U_ZERO_ERROR;
    bool ok = ucal_getTimeZoneTransitionDate(ucal, type, &tranMSecs, &status);

    // Catch a known violation (in ICU 67) of the specified behavior:
    if (U_SUCCESS(status) && ok && type == UCAL_TZ_TRANSITION_NEXT) {
        // At the end of time, that can "succeed" with tranMSecs ==
        // atMSecsSinceEpoch, which should be treated as a failure.
        // (At the start of time, previous correctly fails.)
        ok = qint64(tranMSecs) > atMSecsSinceEpoch;
    }

    // Set the transition time to find the offsets for
    if (U_SUCCESS(status) && ok) {
        status = U_ZERO_ERROR;
        ucal_setMillis(ucal, tranMSecs, &status);
    }

    int32_t utc = 0;
    if (U_SUCCESS(status) && ok) {
        status = U_ZERO_ERROR;
        utc = ucal_get(ucal, UCAL_ZONE_OFFSET, &status) / 1000;
    }

    int32_t dst = 0;
    if (U_SUCCESS(status) && ok) {
        status = U_ZERO_ERROR;
        dst = ucal_get(ucal, UCAL_DST_OFFSET, &status) / 1000;
    }

    ucal_close(ucal);
    if (!U_SUCCESS(status) || !ok)
        return tran;
    tran.atMSecsSinceEpoch = tranMSecs;
    tran.offsetFromUtc = utc + dst;
    tran.standardTimeOffset = utc;
    tran.daylightTimeOffset = dst;
    // TODO No ICU API, use short name as abbreviation.
    QTimeZone::TimeType timeType = dst == 0 ? QTimeZone::StandardTime : QTimeZone::DaylightTime;
    using namespace QtTimeZoneLocale;
    tran.abbreviation = ucalTimeZoneDisplayName(m_ucal, timeType,
                                                QTimeZone::ShortName, QLocale().name().toUtf8());
    return tran;
}
#endif // U_ICU_VERSION_SHORT

// Convert a uenum to a QList<QByteArray>
static QList<QByteArray> uenumToIdList(UEnumeration *uenum)
{
    QList<QByteArray> list;
    int32_t size = 0;
    UErrorCode status = U_ZERO_ERROR;
    // TODO Perhaps use uenum_unext instead?
    QByteArray result = uenum_next(uenum, &size, &status);
    while (U_SUCCESS(status) && !result.isEmpty()) {
        list << result;
        status = U_ZERO_ERROR;
        result = uenum_next(uenum, &size, &status);
    }
    std::sort(list.begin(), list.end());
    list.erase(std::unique(list.begin(), list.end()), list.end());
    return list;
}

// Qt wrapper around ucal_getDSTSavings()
static int ucalDaylightOffset(const QByteArray &id)
{
    UErrorCode status = U_ZERO_ERROR;
    const QString utf16 = QString::fromLatin1(id);
    const int32_t dstMSecs = ucal_getDSTSavings(
        reinterpret_cast<const UChar *>(utf16.data()), &status);
    return U_SUCCESS(status) ? dstMSecs / 1000 : 0;
}

// Create the system default time zone
QIcuTimeZonePrivate::QIcuTimeZonePrivate()
    : m_ucal(nullptr)
{
    // TODO No ICU C API to obtain system tz, assume default hasn't been changed
    init(ucalDefaultTimeZoneId());
}

// Create a named time zone
QIcuTimeZonePrivate::QIcuTimeZonePrivate(const QByteArray &ianaId)
    : m_ucal(nullptr)
{
    // ICU misleadingly maps invalid IDs to GMT.
    if (isTimeZoneIdAvailable(ianaId))
        init(ianaId);
}

QIcuTimeZonePrivate::QIcuTimeZonePrivate(const QIcuTimeZonePrivate &other)
    : QTimeZonePrivate(other), m_ucal(nullptr)
{
    // Clone the ucal so we don't close the shared object
    UErrorCode status = U_ZERO_ERROR;
    m_ucal = ucal_clone(other.m_ucal, &status);
    if (!U_SUCCESS(status)) {
        m_id.clear();
        m_ucal = nullptr;
    }
}

QIcuTimeZonePrivate::~QIcuTimeZonePrivate()
{
    ucal_close(m_ucal);
}

QIcuTimeZonePrivate *QIcuTimeZonePrivate::clone() const
{
    return new QIcuTimeZonePrivate(*this);
}

void QIcuTimeZonePrivate::init(const QByteArray &ianaId)
{
    m_id = ianaId;

    const QString id = QString::fromUtf8(m_id);
    UErrorCode status = U_ZERO_ERROR;
    //TODO Use UCAL_GREGORIAN for now to match QLocale, change to UCAL_DEFAULT once full ICU support
    m_ucal = ucal_open(reinterpret_cast<const UChar *>(id.data()), id.size(),
                       QLocale().name().toUtf8(), UCAL_GREGORIAN, &status);

    if (!U_SUCCESS(status)) {
        m_id.clear();
        m_ucal = nullptr;
    }
}

QString QIcuTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                         QTimeZone::NameType nameType,
                                         const QLocale &locale) const
{
    // Base class has handled OffsetName if we came via the other overload.
    if (nameType == QTimeZone::OffsetName) {
        int offset = standardTimeOffset(QDateTime::currentMSecsSinceEpoch());
        // We can't use transitions reliably to find out right DST offset.
        // Instead use DST offset API to try to get it, when needed:
        if (timeType == QTimeZone::DaylightTime)
            offset += ucalDaylightOffset(m_id);
        // This is only valid for times since the most recent standard offset
        // change; for earlier times, caller must use the other overload.

        // Use our own formating for offset names (ICU C API doesn't support it
        // and we may as well be self-consistent anyway).
        return isoOffsetFormat(offset);
    }
    // Technically this may be suspect, if locale isn't QLocale(), since that's
    // what we used when constructing m_ucal; does ICU cope with inconsistency ?
    using namespace QtTimeZoneLocale;
    return ucalTimeZoneDisplayName(m_ucal, timeType, nameType, locale.name().toUtf8());
}

int QIcuTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    int stdOffset = 0;
    int dstOffset = 0;
    ucalOffsetsAtTime(m_ucal, atMSecsSinceEpoch, &stdOffset, & dstOffset);
    return stdOffset + dstOffset;
}

int QIcuTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    int stdOffset = 0;
    int dstOffset = 0;
    ucalOffsetsAtTime(m_ucal, atMSecsSinceEpoch, &stdOffset, & dstOffset);
    return stdOffset;
}

int QIcuTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    int stdOffset = 0;
    int dstOffset = 0;
    ucalOffsetsAtTime(m_ucal, atMSecsSinceEpoch, &stdOffset, & dstOffset);
    return dstOffset;
}

bool QIcuTimeZonePrivate::hasDaylightTime() const
{
    if (ucalDaylightOffset(m_id) != 0)
        return true;
#if U_ICU_VERSION_MAJOR_NUM >= 50
    for (qint64 when = minMSecs(); when != invalidMSecs(); ) {
        auto data = nextTransition(when);
        if (data.daylightTimeOffset && data.daylightTimeOffset != invalidSeconds())
            return true;
        when = data.atMSecsSinceEpoch;
    }
#endif
    return false;
}

bool QIcuTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    // Clone the ucal so we don't change the shared object
    UErrorCode status = U_ZERO_ERROR;
    UCalendar *ucal = ucal_clone(m_ucal, &status);
    if (!U_SUCCESS(status))
        return false;

    // Set the date to find the offset for
    status = U_ZERO_ERROR;
    ucal_setMillis(ucal, atMSecsSinceEpoch, &status);

    bool result = false;
    if (U_SUCCESS(status)) {
        status = U_ZERO_ERROR;
        result = ucal_inDaylightTime(ucal, &status);
    }

    ucal_close(ucal);
    return result;
}

QTimeZonePrivate::Data QIcuTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    // Available in ICU C++ api, and draft C api in v50
    Data data;
#if U_ICU_VERSION_MAJOR_NUM >= 50
    data = ucalTimeZoneTransition(m_ucal, UCAL_TZ_TRANSITION_PREVIOUS_INCLUSIVE,
                                  forMSecsSinceEpoch);
    if (data.atMSecsSinceEpoch == invalidMSecs()) // before first transition
#endif
    {
        ucalOffsetsAtTime(m_ucal, forMSecsSinceEpoch, &data.standardTimeOffset,
                          &data.daylightTimeOffset);
        data.offsetFromUtc = data.standardTimeOffset + data.daylightTimeOffset;
        // TODO No ICU API for abbreviation, use short name for it:
        using namespace QtTimeZoneLocale;
        QTimeZone::TimeType timeType
            = data.daylightTimeOffset ? QTimeZone::DaylightTime : QTimeZone::StandardTime;
        data.abbreviation = ucalTimeZoneDisplayName(m_ucal, timeType, QTimeZone::ShortName,
                                                    QLocale().name().toUtf8());
    }
    data.atMSecsSinceEpoch = forMSecsSinceEpoch;
    return data;
}

bool QIcuTimeZonePrivate::hasTransitions() const
{
    // Available in ICU C++ api, and draft C api in v50
#if U_ICU_VERSION_MAJOR_NUM >= 50
    return true;
#else
    return false;
#endif
}

QTimeZonePrivate::Data QIcuTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    // Available in ICU C++ api, and draft C api in v50
#if U_ICU_VERSION_MAJOR_NUM >= 50
    return ucalTimeZoneTransition(m_ucal, UCAL_TZ_TRANSITION_NEXT, afterMSecsSinceEpoch);
#else
    Q_UNUSED(afterMSecsSinceEpoch);
    return {};
#endif
}

QTimeZonePrivate::Data QIcuTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    // Available in ICU C++ api, and draft C api in v50
#if U_ICU_VERSION_MAJOR_NUM >= 50
    return ucalTimeZoneTransition(m_ucal, UCAL_TZ_TRANSITION_PREVIOUS, beforeMSecsSinceEpoch);
#else
    Q_UNUSED(beforeMSecsSinceEpoch);
    return {};
#endif
}

QByteArray QIcuTimeZonePrivate::systemTimeZoneId() const
{
    // No ICU C API to obtain system tz
    // TODO Assume default hasn't been changed and is the latests system
    return ucalDefaultTimeZoneId();
}

bool QIcuTimeZonePrivate::isTimeZoneIdAvailable(const QByteArray &ianaId) const
{
    return QtTimeZoneLocale::ucalKnownTimeZoneId(QString::fromUtf8(ianaId));
}

QList<QByteArray> QIcuTimeZonePrivate::availableTimeZoneIds() const
{
    UErrorCode status = U_ZERO_ERROR;
    UEnumeration *uenum = ucal_openTimeZones(&status);
    // Does not document order of entries.
    QList<QByteArray> result;
    if (U_SUCCESS(status))
        result = uenumToIdList(uenum); // Ensures sorted, unique.
    uenum_close(uenum);
    return result;
}

QList<QByteArray> QIcuTimeZonePrivate::availableTimeZoneIds(QLocale::Territory territory) const
{
    const QLatin1StringView regionCode = QLocalePrivate::territoryToCode(territory);
    const QByteArray regionCodeUtf8 = QString(regionCode).toUtf8();
    UErrorCode status = U_ZERO_ERROR;
    UEnumeration *uenum = ucal_openCountryTimeZones(regionCodeUtf8.data(), &status);
    QList<QByteArray> result;
    if (U_SUCCESS(status))
        result = uenumToIdList(uenum);
    uenum_close(uenum);
    // We could merge in what matchingTimeZoneIds(territory) gives us, but
    // hopefully that's redundant, as ICU packages CLDR.
    return result;
}

QList<QByteArray> QIcuTimeZonePrivate::availableTimeZoneIds(int offsetFromUtc) const
{
// TODO Available directly in C++ api but not C api, from 4.8 onwards new filter method works
#if U_ICU_VERSION_MAJOR_NUM >= 49 || (U_ICU_VERSION_MAJOR_NUM == 4 && U_ICU_VERSION_MINOR_NUM == 8)
    UErrorCode status = U_ZERO_ERROR;
    UEnumeration *uenum = ucal_openTimeZoneIDEnumeration(UCAL_ZONE_TYPE_ANY, nullptr,
                                                         &offsetFromUtc, &status);
    QList<QByteArray> result;
    if (U_SUCCESS(status))
        result = uenumToIdList(uenum);
    uenum_close(uenum);
    // We could merge in what matchingTimeZoneIds(offsetFromUtc) gives us, but
    // hopefully that's redundant, as ICU packages CLDR.
    return result;
#else
    return QTimeZonePrivate::availableTimeZoneIds(offsetFromUtc);
#endif
}

QT_END_NAMESPACE
