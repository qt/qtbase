// Copyright (C) 2013 John Layt <jlayt@kde.org>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtimezone.h"
#include "qtimezoneprivate_p.h"

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

// Convert TimeType and NameType into ICU UCalendarDisplayNameType
static UCalendarDisplayNameType ucalDisplayNameType(QTimeZone::TimeType timeType, QTimeZone::NameType nameType)
{
    // TODO ICU C UCalendarDisplayNameType does not support full set of C++ TimeZone::EDisplayType
    switch (nameType) {
    case QTimeZone::ShortName :
    case QTimeZone::OffsetName :
        if (timeType == QTimeZone::DaylightTime)
            return UCAL_SHORT_DST;
        // Includes GenericTime
        return UCAL_SHORT_STANDARD;
    case QTimeZone::DefaultName :
    case QTimeZone::LongName :
        if (timeType == QTimeZone::DaylightTime)
            return UCAL_DST;
        // Includes GenericTime
        return UCAL_STANDARD;
    }
    return UCAL_STANDARD;
}

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

// Qt wrapper around ucal_getTimeZoneDisplayName()
static QString ucalTimeZoneDisplayName(UCalendar *ucal, QTimeZone::TimeType timeType,
                                       QTimeZone::NameType nameType,
                                       const QString &localeCode)
{
    int32_t size = 50;
    QString result(size, Qt::Uninitialized);
    UErrorCode status = U_ZERO_ERROR;

    // size = ucal_getTimeZoneDisplayName(cal, type, locale, result, resultLength, status)
    size = ucal_getTimeZoneDisplayName(ucal,
                                       ucalDisplayNameType(timeType, nameType),
                                       localeCode.toUtf8(),
                                       reinterpret_cast<UChar *>(result.data()),
                                       size,
                                       &status);

    // If overflow, then resize and retry
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        result.resize(size);
        status = U_ZERO_ERROR;
        size = ucal_getTimeZoneDisplayName(ucal,
                                           ucalDisplayNameType(timeType, nameType),
                                           localeCode.toUtf8(),
                                           reinterpret_cast<UChar *>(result.data()),
                                           size,
                                           &status);
    }

    // If successful on first or second go, resize and return
    if (U_SUCCESS(status)) {
        result.resize(size);
        return result;
    }

    return QString();
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
    QTimeZonePrivate::Data tran = QTimeZonePrivate::invalidData();

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
    // TODO No ICU API, use short name instead
    if (dst == 0)
        tran.abbreviation = ucalTimeZoneDisplayName(m_ucal, QTimeZone::StandardTime,
                                                    QTimeZone::ShortName, QLocale().name());
    else
        tran.abbreviation = ucalTimeZoneDisplayName(m_ucal, QTimeZone::DaylightTime,
                                                    QTimeZone::ShortName, QLocale().name());
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
    // Need to check validity here as ICu will create a GMT tz if name is invalid
    if (availableTimeZoneIds().contains(ianaId))
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
    // Return standard offset format name as ICU C api doesn't support it yet
    if (nameType == QTimeZone::OffsetName) {
        const Data nowData = data(QDateTime::currentMSecsSinceEpoch());
        // We can't use transitions reliably to find out right dst offset
        // Instead use dst offset api to try get it if needed
        if (timeType == QTimeZone::DaylightTime)
            return isoOffsetFormat(nowData.standardTimeOffset + ucalDaylightOffset(m_id));
        else
            return isoOffsetFormat(nowData.standardTimeOffset);
    }
    return ucalTimeZoneDisplayName(m_ucal, timeType, nameType, locale.name());
}

QString QIcuTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    // TODO No ICU API, use short name instead
    if (isDaylightTime(atMSecsSinceEpoch))
        return displayName(QTimeZone::DaylightTime, QTimeZone::ShortName, QLocale());
    else
        return displayName(QTimeZone::StandardTime, QTimeZone::ShortName, QLocale());
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
    QTimeZonePrivate::Data data = invalidData();
#if U_ICU_VERSION_MAJOR_NUM >= 50
    data = ucalTimeZoneTransition(m_ucal, UCAL_TZ_TRANSITION_PREVIOUS_INCLUSIVE,
                                  forMSecsSinceEpoch);
    if (data.atMSecsSinceEpoch == invalidMSecs()) // before first transition
#endif
    {
        ucalOffsetsAtTime(m_ucal, forMSecsSinceEpoch, &data.standardTimeOffset,
                          &data.daylightTimeOffset);
        data.offsetFromUtc = data.standardTimeOffset + data.daylightTimeOffset;
        data.abbreviation = abbreviation(forMSecsSinceEpoch);
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
    return invalidData();
#endif
}

QTimeZonePrivate::Data QIcuTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    // Available in ICU C++ api, and draft C api in v50
#if U_ICU_VERSION_MAJOR_NUM >= 50
    return ucalTimeZoneTransition(m_ucal, UCAL_TZ_TRANSITION_PREVIOUS, beforeMSecsSinceEpoch);
#else
    Q_UNUSED(beforeMSecsSinceEpoch);
    return invalidData();
#endif
}

QByteArray QIcuTimeZonePrivate::systemTimeZoneId() const
{
    // No ICU C API to obtain system tz
    // TODO Assume default hasn't been changed and is the latests system
    return ucalDefaultTimeZoneId();
}

QList<QByteArray> QIcuTimeZonePrivate::availableTimeZoneIds() const
{
    UErrorCode status = U_ZERO_ERROR;
    UEnumeration *uenum = ucal_openTimeZones(&status);
    QList<QByteArray> result;
    if (U_SUCCESS(status))
        result = uenumToIdList(uenum);
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
    return result;
#else
    return QTimeZonePrivate::availableTimeZoneIds(offsetFromUtc);
#endif
}

QT_END_NAMESPACE
