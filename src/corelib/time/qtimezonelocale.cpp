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
using namespace QtTimeZoneLocale; // QTZL_data_p.h
using namespace QtTimeZoneCldr; // QTZP_data_p.h
// Accessors for the QTZL_data_p.h

// Accessors for the QTZP_data_p.h

} // nameless namespace

QString QTimeZonePrivate::localeName(qint64 atMSecsSinceEpoch, int offsetFromUtc,
                                     QTimeZone::TimeType timeType,
                                     QTimeZone::NameType nameType,
                                     const QLocale &locale) const
{
    Q_ASSERT(nameType != QTimeZone::OffsetName || locale.language() != QLocale::C);
    // Get data from QTZ[LP]_data_p.h

    Q_UNUSED(atMSecsSinceEpoch);
    Q_UNUSED(offsetFromUtc);
    Q_UNUSED(timeType);
    return QString();
}
#endif // ICU

QT_END_NAMESPACE
