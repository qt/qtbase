// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2013 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtimezone.h"
#if QT_CONFIG(timezone)
#  include "qtimezoneprivate_p.h"
#endif

#include <QtCore/qdatastream.h>
#include <QtCore/qdatetime.h>

#include <qdebug.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(timezone)
// Create default time zone using appropriate backend
static QTimeZonePrivate *newBackendTimeZone()
{
#ifdef QT_NO_SYSTEMLOCALE
#if QT_CONFIG(icu)
    return new QIcuTimeZonePrivate();
#else
    return new QUtcTimeZonePrivate();
#endif
#else
#if defined(Q_OS_DARWIN)
    return new QMacTimeZonePrivate();
#elif defined(Q_OS_ANDROID)
    return new QAndroidTimeZonePrivate();
#elif defined(Q_OS_UNIX)
    return new QTzTimeZonePrivate();
#elif QT_CONFIG(icu)
    return new QIcuTimeZonePrivate();
#elif defined(Q_OS_WIN)
    return new QWinTimeZonePrivate();
#else
    return new QUtcTimeZonePrivate();
#endif // System Locales
#endif // QT_NO_SYSTEMLOCALE
}

// Create named time zone using appropriate backend
static QTimeZonePrivate *newBackendTimeZone(const QByteArray &ianaId)
{
    Q_ASSERT(!ianaId.isEmpty());
#ifdef QT_NO_SYSTEMLOCALE
#if QT_CONFIG(icu)
    return new QIcuTimeZonePrivate(ianaId);
#else
    return new QUtcTimeZonePrivate(ianaId);
#endif
#else
#if defined(Q_OS_DARWIN)
    return new QMacTimeZonePrivate(ianaId);
#elif defined(Q_OS_ANDROID)
    return new QAndroidTimeZonePrivate(ianaId);
#elif defined(Q_OS_UNIX)
    return new QTzTimeZonePrivate(ianaId);
#elif QT_CONFIG(icu)
    return new QIcuTimeZonePrivate(ianaId);
#elif defined(Q_OS_WIN)
    return new QWinTimeZonePrivate(ianaId);
#else
    return new QUtcTimeZonePrivate(ianaId);
#endif // System Locales
#endif // QT_NO_SYSTEMLOCALE
}

class QTimeZoneSingleton
{
public:
    QTimeZoneSingleton() : backend(newBackendTimeZone()) {}

    // The global_tz is the tz to use in static methods such as availableTimeZoneIds() and
    // isTimeZoneIdAvailable() and to create named IANA time zones.  This is usually the host
    // system, but may be different if the host resources are insufficient or if
    // QT_NO_SYSTEMLOCALE is set.  A simple UTC backend is used if no alternative is available.
    QExplicitlySharedDataPointer<QTimeZonePrivate> backend;
};

Q_GLOBAL_STATIC(QTimeZoneSingleton, global_tz);
#endif // feature timezone

/*!
    \class QTimeZone
    \inmodule QtCore
    \since 5.2
    \threadsafe

    \brief QTimeZone identifies how a time representation relates to UTC.

    When dates and times are combined, the meaning of the result depends on how
    time is being represented. There are various international standards for
    representing time; one of these, UTC, corresponds to the traditional
    standard of solar mean time at Greenwich (a.k.a. GMT). All other time
    systems supported by Qt are ultimately specified in relation to UTC. An
    instance of this class provides a stateless calculator for conversions
    between UTC and other time representations.

    Some time representations are simply defined at a fixed offset to UTC.
    Others are defined by governments for use within their jurisdictions. The
    latter are properly known as time zones, but QTimeZone (since Qt 6.5) is
    unifies their representation with that of general time systems. One time
    zone generally supported on most operating systems is designated local time;
    this is presumed to correspond to the time zone within which the user is
    living.

    For time zones other than local time, UTC and those at fixed offsets from
    UTC, Qt can only provide support when the operating system provides some way
    to access that information. When Qt is built, the \c timezone feature
    controls whether such information is available. When it is not, some
    constructors and methods of QTimeZone are excluded from its API; these are
    documented as depending on feature \c timezone. Note that, even when Qt is
    built with this feature enabled, it may be unavailable to users whose
    systems are misconfigured, or where some standard packages (for example, the
    \c tzdata package on Linux) are not installed. This feature is enabled by
    default when time zone information is available.

    This class is primarily designed for use in QDateTime; most applications
    will not need to access this class directly and should instead use an
    instance of it when constructing a QDateTime.

    \note For consistency with QDateTime, QTimeZone does not account for leap
    seconds.

    \section1 Remarks

    QTimeZone, like QDateTime, measures offsets from UTC in seconds. This
    contrasts with their measurement of time generally, which they do in
    milliseconds. Real-world time zones generally have UTC offsets that are
    whole-number multiples of five minutes (300 seconds), at least since well
    before 1970. A positive offset from UTC gives a time representation puts
    noon on any given day before UTC noon on that day; a negative offset puts
    noon after UTC noon on the same day.

    \section2 Lightweight Time Representations

    QTimeZone can represent UTC, local time and fixed offsets from UTC even when
    feature \c timezone is disabled. The form in which it does so is also
    available when the feature is enabled; it is a more lightweight form and
    processing using it will typically be more efficient, unless methods only
    available when feature \c timezone is enabled are being exercised. See \l
    Initialization and \l QTimeZone::fromSecondsAheadOfUtc(int) for how to
    construct these representations.

    This documentation distinguishes between "time zone", used to describe a
    time representation described by system-supplied or standard information,
    and time representations more generally, which include these lightweight
    forms. The methods available only when feature \c timezone is enabled are
    apt to be cheaper for time zones than for lightweight time representations,
    for which these methods may construct a suitable transient time zone object
    to which to forward the query.

    \section2 IANA Time Zone IDs

    QTimeZone uses the IANA time zone IDs as defined in the IANA Time Zone
    Database (http://www.iana.org/time-zones). This is to ensure a standard ID
    across all supported platforms.  Most platforms support the IANA IDs
    and the IANA Database natively, but for Windows a mapping is required to
    the native IDs.  See below for more details.

    The IANA IDs can and do change on a regular basis, and can vary depending
    on how recently the host system data was updated.  As such you cannot rely
    on any given ID existing on any host system.  You must use
    availableTimeZoneIds() to determine what IANA IDs are available.

    The IANA IDs and database are also know as the Olson IDs and database,
    named after the original compiler of the database.

    \section2 UTC Offset Time Zones

    A default UTC time zone backend is provided which is always available when
    feature \c timezone is enabled. This provides a set of generic Offset From
    UTC time zones in the range UTC-16:00 to UTC+16:00. These time zones can be
    created using either the standard ISO format names, such as "UTC+00:00", as
    listed by availableTimeZoneIds(), or using a name of similar form in
    combination with the number of offset seconds.

    \section2 Windows Time Zones

    Windows native time zone support is severely limited compared to the
    standard IANA TZ Database.  Windows time zones cover larger geographic
    areas and are thus less accurate in their conversions.  They also do not
    support as much historical data and so may only be accurate for the
    current year.  In particular, when MS's zone data claims that DST was
    observed prior to 1900 (this is historically known to be untrue), the
    claim is ignored and the standard time (allegedly) in force in 1900 is
    taken to have always been in effect.

    QTimeZone uses a conversion table derived from the Unicode CLDR data to map
    between IANA IDs and Windows IDs.  Depending on your version of Windows
    and Qt, this table may not be able to provide a valid conversion, in which
    "UTC" will be returned.

    QTimeZone provides a public API to use this conversion table.  The Windows ID
    used is the Windows Registry Key for the time zone which is also the MS
    Exchange EWS ID as well, but is different to the Time Zone Name (TZID) and
    COD code used by MS Exchange in versions before 2007.

    \note When Qt is built with the ICU library, it is used in preference to the
    Windows system APIs, bypassing all problems with those APIs using different
    names.

    \section2 System Time Zone

    The method systemTimeZoneId() returns the current system IANA time zone
    ID which on Unix-like systems will always be correct.  On Windows this ID is
    translated from the Windows system ID using an internal translation
    table and the user's selected country.  As a consequence there is a small
    chance any Windows install may have IDs not known by Qt, in which case
    "UTC" will be returned.

    Creating a new QTimeZone instance using the system time zone ID will only
    produce a fixed named copy of the time zone, it will not change if the
    system time zone changes.  QTimeZone::systemTimeZone() will return an
    instance representing the zone named by this system ID.  Note that
    constructing a QDateTime using this system zone may behave differently than
    constructing a QDateTime that uses Qt::LocalTime as its Qt::TimeSpec, as the
    latter directly uses system APIs for accessing local time information, which
    may behave differently (and, in particular, might adapt if the user adjusts
    the system zone setting).

    \section2 Time Zone Offsets

    The difference between UTC and the local time in a time zone is expressed
    as an offset in seconds from UTC, i.e. the number of seconds to add to UTC
    to obtain the local time.  The total offset is comprised of two component
    parts, the standard time offset and the daylight-saving time offset.  The
    standard time offset is the number of seconds to add to UTC to obtain
    standard time in the time zone.  The daylight-saving time offset is the
    number of seconds to add to the standard time offset to obtain
    daylight-saving time (abbreviated DST and sometimes called "daylight time"
    or "summer time") in the time zone. The usual case for DST (using
    standard time in winter, DST in summer) has a positive daylight-saving
    time offset. However, some zones have negative DST offsets, used in
    winter, with summer using standard time.

    Note that the standard and DST offsets for a time zone may change over time
    as countries have changed DST laws or even their standard time offset.

    \section2 License

    This class includes data obtained from the CLDR data files under the terms
    of the Unicode Data Files and Software License. See
    \l{unicode-cldr}{Unicode Common Locale Data Repository (CLDR)} for details.

    \sa QDateTime, QCalendar
*/

/*!
    \variable QTimeZone::MinUtcOffsetSecs
    \brief Timezone offsets from UTC are expected to be no lower than this.

    The lowest UTC offset of any early 21st century timezone is -12 hours (Baker
    Island, USA), or 12 hours west of Greenwich.

    Historically, until 1844, The Philippines (then controlled by Spain) used
    the same date as Spain's American holdings, so had offsets close to 16 hours
    west of Greenwich. As The Philippines was using local solar mean time, it is
    possible some outlying territory of it may have been operating at more than
    16 hours west of Greenwich, but no early 21st century timezone traces its
    history back to such an extreme.

    \sa MaxUtcOffsetSecs
*/
/*!
    \variable QTimeZone::MaxUtcOffsetSecs
    \brief Timezone offsets from UTC are expected to be no higher than this.

    The highest UTC offset of any early 21st century timezone is +14 hours
    (Christmas Island, Kiribati, Kiritimati), or 14 hours east of Greenwich.

    Historically, before 1867, when Russia sold Alaska to America, Alaska used
    the same date as Russia, so had offsets over 15 hours east of Greenwich. As
    Alaska was using local solar mean time, its offsets varied, but all were
    less than 16 hours east of Greenwich.

    \sa MinUtcOffsetSecs
*/

#if QT_CONFIG(timezone)
/*!
    \enum QTimeZone::TimeType

    The type of time zone time, for example when requesting the name.  In time
    zones that do not apply DST, all three values may return the same result.

    \value StandardTime
           The standard time in a time zone, i.e. when Daylight-Saving is not
           in effect.
           For example when formatting a display name this will show something
           like "Pacific Standard Time".
    \value DaylightTime
           A time when Daylight-Saving is in effect.
           For example when formatting a display name this will show something
           like "Pacific daylight-saving time".
    \value GenericTime
           A time which is not specifically Standard or Daylight-Saving time,
           either an unknown time or a neutral form.
           For example when formatting a display name this will show something
           like "Pacific Time".

    This type is only available when feature \c timezone is enabled.
*/

/*!
    \enum QTimeZone::NameType

    The type of time zone name.

    \value DefaultName
           The default form of the time zone name, e.g. LongName, ShortName or OffsetName
    \value LongName
           The long form of the time zone name, e.g. "Central European Time"
    \value ShortName
           The short form of the time zone name, usually an abbreviation, e.g. "CET"
    \value OffsetName
           The standard ISO offset form of the time zone name, e.g. "UTC+01:00"

    This type is only available when feature \c timezone is enabled.
*/

/*!
    \class QTimeZone::OffsetData
    \inmodule QtCore

    The time zone offset data for a given moment in time.

    This provides the time zone offsets and abbreviation to use at that moment
    in time. When a function returns this type, it may use an invalid datetime
    to indicate that the query it is answering has no valid answer, so check
    \c{atUtc.isValid()} before using the results.

    \list
    \li OffsetData::atUtc  The datetime of the offset data in UTC time.
    \li OffsetData::offsetFromUtc  The total offset from UTC in effect at the datetime.
    \li OffsetData::standardTimeOffset  The standard time offset component of the total offset.
    \li OffsetData::daylightTimeOffset  The DST offset component of the total offset.
    \li OffsetData::abbreviation  The abbreviation in effect at the datetime.
    \endlist

    For example, for time zone "Europe/Berlin" the OffsetDate in standard and DST might be:

    \list
    \li atUtc = QDateTime(QDate(2013, 1, 1), QTime(0, 0), QTimeZone::UTC)
    \li offsetFromUtc = 3600
    \li standardTimeOffset = 3600
    \li daylightTimeOffset = 0
    \li abbreviation = "CET"
    \endlist

    \list
    \li atUtc = QDateTime(QDate(2013, 6, 1), QTime(0, 0), QTimeZone::UTC)
    \li offsetFromUtc = 7200
    \li standardTimeOffset = 3600
    \li daylightTimeOffset = 3600
    \li abbreviation = "CEST"
    \endlist

    This type is only available when feature \c timezone is enabled.
*/

/*!
    \typedef QTimeZone::OffsetDataList

    Synonym for QList<OffsetData>.

    This type is only available when feature \c timezone is enabled.
*/
#endif // timezone backends

QTimeZone::Data::Data() noexcept : d(nullptr)
{
    // Assumed by the conversion between spec and mode:
    static_assert(int(Qt::TimeZone) == 3);
}

QTimeZone::Data::Data(const Data &other) noexcept
{
#if QT_CONFIG(timezone)
    if (!other.isShort() && other.d)
        other.d->ref.ref();
#endif
    d = other.d;
}

QTimeZone::Data::Data(QTimeZonePrivate *dptr) noexcept
    : d(dptr)
{
#if QT_CONFIG(timezone)
    if (d)
        d->ref.ref();
#endif
}

QTimeZone::Data::~Data()
{
#if QT_CONFIG(timezone)
    if (!isShort() && d && !d->ref.deref())
        delete d;
    d = nullptr;
#endif
}

QTimeZone::Data &QTimeZone::Data::operator=(const QTimeZone::Data &other) noexcept
{
#if QT_CONFIG(timezone)
    if (!other.isShort())
        return *this = other.d;
    if (!isShort() && d && !d->ref.deref())
        delete d;
#endif
    d = other.d;
    return *this;
}

/*!
    Create a null/invalid time zone instance.
*/

QTimeZone::QTimeZone() noexcept
{
    // Assumed by (at least) Data::swap() and {copy,move} {assign,construct}:
    static_assert(sizeof(ShortData) <= sizeof(Data::d));
    // Needed for ShortData::offset to represent all valid offsets:
    static_assert(qintptr(1) << (sizeof(void *) * 8 - 2) >= MaxUtcOffsetSecs);
}

#if QT_CONFIG(timezone)
QTimeZone::Data &QTimeZone::Data::operator=(QTimeZonePrivate *dptr) noexcept
{
    if (!isShort()) {
        if (d == dptr)
            return *this;
        if (d && !d->ref.deref())
            delete d;
    }
    if (dptr)
        dptr->ref.ref();
    d = dptr;
    Q_ASSERT(!isShort());
    return *this;
}

/*!
    Creates a time zone instance with the requested IANA ID \a ianaId.

    The ID must be one of the available system IDs or a valid UTC-with-offset
    ID, otherwise an invalid time zone will be returned.

    This constructor is only available when feature \c timezone is enabled.

    \sa availableTimeZoneIds()
*/

QTimeZone::QTimeZone(const QByteArray &ianaId)
{
    // Try and see if it's a CLDR UTC offset ID - just as quick by creating as
    // by looking up.
    d = new QUtcTimeZonePrivate(ianaId);
    // If not a CLDR UTC offset ID then try creating it with the system backend.
    // Relies on backend not creating valid TZ with invalid name.
    if (!d->isValid()) {
        if (ianaId.isEmpty())
            d = newBackendTimeZone();
        else if (global_tz->backend->isTimeZoneIdAvailable(ianaId))
            d = newBackendTimeZone(ianaId);
        // else: No such ID, avoid creating a TZ cache entry for it.
    }
    // Can also handle UTC with arbitrary (valid) offset, but only do so as
    // fall-back, since either of the above may handle it more informatively.
    if (!d->isValid()) {
        qint64 offset = QUtcTimeZonePrivate::offsetFromUtcString(ianaId);
        if (offset != QTimeZonePrivate::invalidSeconds()) {
            // Should have abs(offset) < 24 * 60 * 60 = 86400.
            qint32 seconds = qint32(offset);
            Q_ASSERT(qint64(seconds) == offset);
            // NB: this canonicalises the name, so it might not match ianaId
            d = new QUtcTimeZonePrivate(seconds);
        }
    }
}

/*!
    Creates a time zone instance with the given offset, \a offsetSeconds, from UTC.

    The \a offsetSeconds from UTC must be in the range -16 hours to +16 hours
    otherwise an invalid time zone will be returned.

    This constructor is only available when feature \c timezone is enabled. The
    returned instance is equivalent to the lightweight time representation
    \c{QTimeZone::fromSecondsAfterUtc(offsetSeconds)}, albeit implemented as a
    time zone.

    \sa MinUtcOffsetSecs, MaxUtcOffsetSecs
*/

QTimeZone::QTimeZone(int offsetSeconds)
    : d((offsetSeconds >= MinUtcOffsetSecs && offsetSeconds <= MaxUtcOffsetSecs)
        ? new QUtcTimeZonePrivate(offsetSeconds) : nullptr)
{
}

/*!
    Creates a custom time zone instance at fixed offset from UTC.

    The returned time zone has an ID of \a zoneId and an offset from UTC of \a
    offsetSeconds.  The \a name will be the name used by displayName() for the
    LongName, the \a abbreviation will be used by displayName() for the
    ShortName and by abbreviation(), and the optional \a territory will be used
    by territory().  The \a comment is an optional note that may be displayed in
    a GUI to assist users in selecting a time zone.

    The \a zoneId \e{must not} be one of the available system IDs returned by
    availableTimeZoneIds(). The \a offsetSeconds from UTC must be in the range
    -16 hours to +16 hours.

    If the custom time zone does not have a specific territory then set it to the
    default value of QLocale::AnyTerritory.

    This constructor is only available when feature \c timezone is enabled.

    \sa id(), offsetFromUtc(), displayName(), abbreviation(), territory(), comment(),
        MinUtcOffsetSecs, MaxUtcOffsetSecs
*/

QTimeZone::QTimeZone(const QByteArray &zoneId, int offsetSeconds, const QString &name,
                     const QString &abbreviation, QLocale::Territory territory, const QString &comment)
    : d(QUtcTimeZonePrivate().isTimeZoneIdAvailable(zoneId)
        || global_tz->backend->isTimeZoneIdAvailable(zoneId)
        ? nullptr // Don't let client code hijack a real zone name.
        : new QUtcTimeZonePrivate(zoneId, offsetSeconds, name, abbreviation, territory, comment))
{
}

/*!
    \internal

    Private. Create time zone with given private backend

    This constructor is only available when feature \c timezone is enabled.
*/

QTimeZone::QTimeZone(QTimeZonePrivate &dd)
    : d(&dd)
{
}

/*!
    \since 6.5
    Converts this QTimeZone to one whose timeSpec() is Qt::TimeZone.

    In all cases, the result's \l timeSpec() is Qt::TimeZone. When this
    QTimeZone's timeSpec() is Qt::TimeZone, this QTimeZone itself is returned.

    If timeSpec() is Qt::UTC, QTimeZone::utc() is returned. If it is
    Qt::OffsetFromUTC then QTimeZone(int) is passed its offset and the result is
    returned.

    If timeSpec() is Qt::LocalTime then an instance of the current system time
    zone will be returned. This will not change to reflect any subsequent change
    to the system time zone. It represents the local time that was in effect
    when asBackendZone() was called.

    When using a lightweight time representation - local time, UTC time or time
    at a fixed offset from UTC - using methods only supported when feature \c
    timezone is enabled may be more expensive than using a corresponding time
    zone. This method maps a lightweight time representation to a corresponding
    time zone - that is, an instance based on system-supplied or standard data.

    This method is only available when feature \c timezone is enabled.

    \sa QTimeZone(QTimeZone::Initialization), fromSecondsAheadOfUtc()
*/

QTimeZone QTimeZone::asBackendZone() const
{
    switch (timeSpec()) {
    case Qt::TimeZone:
        return *this;
    case Qt::LocalTime:
        return systemTimeZone();
    case Qt::UTC:
        return utc();
    case Qt::OffsetFromUTC:
        return QTimeZone(*new QUtcTimeZonePrivate(int(d.s.offset)));
    }
    return QTimeZone();
}
#endif // timezone backends

/*!
    \since 6.5
    \enum QTimeZone::Initialization

    The type of the simplest lightweight time representations.

    This enumeration identifies a type of lightweight time representation to
    pass to a QTimeZone constructor, where no further data are required. They
    correspond to the like-named members of Qt::TimeSpec.

    \value LocalTime This time representation corresponds to the one implicitly
                     used by system functions using \c time_t and \c {struct tm}
                     value to map between local time and UTC time.

    \value UTC This time representation, Coordinated Universal Time, is the base
               representation to which civil time is referred in all supported
               time representations. It is defined by the International
               Telecommunication Union.
*/

/*!
    \since 6.5
    \fn QTimeZone::QTimeZone(Initialization spec) noexcept

    Creates a lightweight instance describing UTC or local time.

    \sa fromSecondsAheadOfUtc(), asBackendZone(), utc(), systemTimeZone()
*/

/*!
    \since 6.5
    \fn QTimeZone::fromSecondsAheadOfUtc(int offset)
    \fn QTimeZone::fromDurationAheadOfUtc(std::chrono::seconds offset)

    Returns a time representation at a fixed \a offset, in seconds, ahead of
    UTC.

    The \a offset from UTC must be in the range -16 hours to +16 hours otherwise
    an invalid time zone will be returned. The returned QTimeZone is a
    lightweight time representation, not a time zone (backed by system-supplied
    or standard data).

    If the offset is 0, the \l timeSpec() of the returned instance will be
    Qt::UTC. Otherwise, if \a offset is valid, timeSpec() is
    Qt::OffsetFromUTC. An invalid time zone, when returned, has Qt::TimeZone as
    its timeSpec().

    \sa QTimeZone(int), asBackendZone(), fixedSecondsAheadOfUtc(),
        MinUtcOffsetSecs, MaxUtcOffsetSecs
*/

/*!
    \since 6.5
    \fn Qt::TimeSpec QTimeZone::timeSpec() const noexcept

    Returns a Qt::TimeSpec identifying the type of time representation.

    If the result is Qt::TimeZone, this time description is a time zone (backed
    by system-supplied or standard data); otherwise, it is a lightweight time
    representation. If the result is Qt::LocalTime it describes local time: see
    Qt::TimeSpec for details.

    \sa fixedSecondsAheadOfUtc(), asBackendZone()
*/

/*!
    \since 6.5
    \fn int QTimeZone::fixedSecondsAheadOfUtc() const noexcept

    For a lightweight time representation whose \l timeSpec() is Qt::OffsetFromUTC,
    this returns the fixed offset from UTC that it describes. For any other time
    representation it returns 0, even if that time representation does have a
    constant offset from UTC.
*/

/*!
    \since 6.5
    \fn QTimeZone::isUtcOrFixedOffset(Qt::TimeSpec spec) noexcept

    Returns \c true if \a spec is Qt::UTC or Qt::OffsetFromUTC.
*/

/*!
    \since 6.5
    \fn QTimeZone::isUtcOrFixedOffset() const noexcept

    Returns \c true if \l timeSpec() is Qt::UTC or Qt::OffsetFromUTC.

    When it is true, the time description does not change over time, such as
    having seasonal daylight-saving changes, as may happen for local time or a
    time zone. Knowing this may save the calling code to need for various other
    checks.
*/

/*!
    Copy constructor: copy \a other to this.
*/

QTimeZone::QTimeZone(const QTimeZone &other) noexcept
    : d(other.d)
{
}

/*!
    \fn QTimeZone::QTimeZone(QTimeZone &&other) noexcept

    Move constructor of this from \a other.
*/

/*!
    Destroys the time zone.
*/

QTimeZone::~QTimeZone()
{
}

/*!
    \fn QTimeZone::swap(QTimeZone &other) noexcept

    Swaps this time zone instance with \a other. This function is very
    fast and never fails.
*/

/*!
    Assignment operator, assign \a other to this.
*/

QTimeZone &QTimeZone::operator=(const QTimeZone &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn QTimeZone &QTimeZone::operator=(QTimeZone &&other)

    Move-assigns \a other to this QTimeZone instance, transferring the ownership
    of its data to this instance.
*/

/*!
    Returns \c true if this time representation is equal to the \a other.

    Two representations are different if they are internally described
    differently, even if they agree in their representation of all moments of
    time. In particular, a lightweight time representation may coincide with a
    time zone but the two will not be equal.
*/

bool QTimeZone::operator==(const QTimeZone &other) const
{
    if (d.isShort())
        return other.d.isShort() && d.s == other.d.s;

    if (!other.d.isShort()) {
        if (d.d == other.d.d)
            return true;
#if QT_CONFIG(timezone)
        return d.d && other.d.d && *d.d == *other.d.d;
#endif
    }

    return false;
}

/*!
    Returns \c true if this time zone is not equal to the \a other time zone.

    Two representations are different if they are internally described
    differently, even if they agree in their representation of all moments of
    time. In particular, a lightweight time representation may coincide with a
    time zone but the two will not be equal.
*/
bool QTimeZone::operator!=(const QTimeZone &other) const // ### Qt 7: inline
{
    return !(*this == other);
}

/*!
    Returns \c true if this time zone is valid.
*/

bool QTimeZone::isValid() const
{
#if QT_CONFIG(timezone)
    if (!d.isShort())
        return d.d && d->isValid();
#endif
    return d.isShort();
}

#if QT_CONFIG(timezone)
/*!
    Returns the IANA ID for the time zone.

    IANA IDs are used on all platforms.  On Windows these are translated from
    the Windows ID into the best match IANA ID for the time zone and territory.

    This method is only available when feature \c timezone is enabled.
*/

QByteArray QTimeZone::id() const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::UTC:
            return QTimeZonePrivate::utcQByteArray();
        case Qt::LocalTime:
            return systemTimeZoneId();
        case Qt::OffsetFromUTC:
            return QUtcTimeZonePrivate(d.s.offset).id();
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (d.d) {
        return d->id();
    }
    return QByteArray();
}

/*!
    \since 6.2

    Returns the territory for the time zone.

    A return of \l {QLocale::}{AnyTerritory} means the zone has no known
    territorial association. In some cases this may be because the zone has no
    associated territory - for example, UTC - or because the zone is used in
    several territories - for example, CET. In other cases, the QTimeZone
    backend may not know which territory the zone is associated with - for
    example, because it is not the primary zone of the territory in which it is
    used.

    This method is only available when feature \c timezone is enabled.
*/
QLocale::Territory QTimeZone::territory() const
{
    if (d.isShort()) {
        if (d.s.spec() == Qt::LocalTime)
            return systemTimeZone().territory();
    } else if (isValid()) {
        return d->territory();
    }
    return QLocale::AnyTerritory;
}

#if QT_DEPRECATED_SINCE(6, 6)
/*!
    \deprecated [6.6] Use territory() instead.

    Returns the territory for the time zone.

    This method is only available when feature \c timezone is enabled.
*/

QLocale::Country QTimeZone::country() const
{
    return territory();
}
#endif

/*!
    Returns any comment for the time zone.

    A comment may be provided by the host platform to assist users in
    choosing the correct time zone.  Depending on the platform this may not
    be localized.

    This method is only available when feature \c timezone is enabled.
*/

QString QTimeZone::comment() const
{
    if (d.isShort()) {
        // TODO: anything ?  Or just stick with empty string ?
    } else if (isValid()) {
        return d->comment();
    }
    return QString();
}

/*!
    Returns the localized time zone display name at the given \a atDateTime
    for the given \a nameType in the given \a locale.  The \a nameType and
    \a locale requested may not be supported on all platforms, in which case
    the best available option will be returned.

    If the \a locale is not provided then the application default locale will
    be used.

    The display name may change depending on DST or historical events.

    This method is only available when feature \c timezone is enabled.

    \sa abbreviation()
*/

QString QTimeZone::displayName(const QDateTime &atDateTime, NameType nameType,
                               const QLocale &locale) const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().displayName(atDateTime, nameType, locale);
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            return QUtcTimeZonePrivate(d.s.offset).QTimeZonePrivate::displayName(
                atDateTime.toMSecsSinceEpoch(), nameType, locale);
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (isValid()) {
        return d->displayName(atDateTime.toMSecsSinceEpoch(), nameType, locale);
    }

    return QString();
}

/*!
    Returns the localized time zone display name for the given \a timeType
    and \a nameType in the given \a locale. The \a nameType and \a locale
    requested may not be supported on all platforms, in which case the best
    available option will be returned.

    If the \a locale is not provided then the application default locale will
    be used.

    Where the time zone display names have changed over time then the most
    recent names will be used.

    This method is only available when feature \c timezone is enabled.

    \sa abbreviation()
*/

QString QTimeZone::displayName(TimeType timeType, NameType nameType,
                               const QLocale &locale) const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().displayName(timeType, nameType, locale);
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            return QUtcTimeZonePrivate(d.s.offset).displayName(timeType, nameType, locale);
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (isValid()) {
        return d->displayName(timeType, nameType, locale);
    }

    return QString();
}

/*!
    Returns the time zone abbreviation at the given \a atDateTime.  The
    abbreviation may change depending on DST or even historical events.

    Note that the abbreviation is not guaranteed to be unique to this time zone
    and should not be used in place of the ID or display name.

    This method is only available when feature \c timezone is enabled.

    \sa displayName()
*/

QString QTimeZone::abbreviation(const QDateTime &atDateTime) const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().abbreviation(atDateTime);
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            return QUtcTimeZonePrivate(d.s.offset).abbreviation(atDateTime.toMSecsSinceEpoch());
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (isValid()) {
        return d->abbreviation(atDateTime.toMSecsSinceEpoch());
    }

    return QString();
}

/*!
    Returns the total effective offset at the given \a atDateTime, i.e. the
    number of seconds to add to UTC to obtain the local time.  This includes
    any DST offset that may be in effect, i.e. it is the sum of
    standardTimeOffset() and daylightTimeOffset() for the given datetime.

    For example, for the time zone "Europe/Berlin" the standard time offset is
    +3600 seconds and the DST offset is +3600 seconds.  During standard time
    offsetFromUtc() will return +3600 (UTC+01:00), and during DST it will
    return +7200 (UTC+02:00).

    This method is only available when feature \c timezone is enabled.

    \sa standardTimeOffset(), daylightTimeOffset()
*/

int QTimeZone::offsetFromUtc(const QDateTime &atDateTime) const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().offsetFromUtc(atDateTime);
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            return d.s.offset;
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (isValid()) {
        const int offset = d->offsetFromUtc(atDateTime.toMSecsSinceEpoch());
        if (offset !=  QTimeZonePrivate::invalidSeconds())
            return offset;
    }
    return 0;
}

/*!
    Returns the standard time offset at the given \a atDateTime, i.e. the
    number of seconds to add to UTC to obtain the local Standard Time.  This
    excludes any DST offset that may be in effect.

    For example, for the time zone "Europe/Berlin" the standard time offset is
    +3600 seconds.  During both standard and DST offsetFromUtc() will return
    +3600 (UTC+01:00).

    This method is only available when feature \c timezone is enabled.

    \sa offsetFromUtc(), daylightTimeOffset()
*/

int QTimeZone::standardTimeOffset(const QDateTime &atDateTime) const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().standardTimeOffset(atDateTime);
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            return d.s.offset;
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (isValid()) {
        const int offset = d->standardTimeOffset(atDateTime.toMSecsSinceEpoch());
        if (offset !=  QTimeZonePrivate::invalidSeconds())
            return offset;
    }
    return 0;
}

/*!
    Returns the daylight-saving time offset at the given \a atDateTime,
    i.e. the number of seconds to add to the standard time offset to obtain the
    local daylight-saving time.

    For example, for the time zone "Europe/Berlin" the DST offset is +3600
    seconds.  During standard time daylightTimeOffset() will return 0, and when
    daylight-saving is in effect it will return +3600.

    This method is only available when feature \c timezone is enabled.

    \sa offsetFromUtc(), standardTimeOffset()
*/

int QTimeZone::daylightTimeOffset(const QDateTime &atDateTime) const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().daylightTimeOffset(atDateTime);
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            return 0;
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (hasDaylightTime()) {
        const int offset = d->daylightTimeOffset(atDateTime.toMSecsSinceEpoch());
        if (offset !=  QTimeZonePrivate::invalidSeconds())
            return offset;
    }
    return 0;
}

/*!
    Returns \c true if the time zone has practiced daylight-saving at any time.

    This method is only available when feature \c timezone is enabled.

    \sa isDaylightTime(), daylightTimeOffset()
*/

bool QTimeZone::hasDaylightTime() const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().hasDaylightTime();
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            return false;
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (isValid()) {
        return d->hasDaylightTime();
    }
    return false;
}

/*!
    Returns \c true if daylight-saving was in effect at the given \a atDateTime.

    This method is only available when feature \c timezone is enabled.

    \sa hasDaylightTime(), daylightTimeOffset()
*/

bool QTimeZone::isDaylightTime(const QDateTime &atDateTime) const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().isDaylightTime(atDateTime);
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            return false;
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (hasDaylightTime()) {
        return d->isDaylightTime(atDateTime.toMSecsSinceEpoch());
    }
    return false;
}

/*!
    Returns the effective offset details at the given \a forDateTime.

    This is the equivalent of calling abbreviation() and all three offset
    functions individually but is more efficient. If this data is not available
    for the given datetime, an invalid OffsetData will be returned with an
    invalid QDateTime as its \c atUtc.

    This method is only available when feature \c timezone is enabled.

    \sa offsetFromUtc(), standardTimeOffset(), daylightTimeOffset(), abbreviation()
*/

QTimeZone::OffsetData QTimeZone::offsetData(const QDateTime &forDateTime) const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().offsetData(forDateTime);
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            return { abbreviation(forDateTime), forDateTime, int(d.s.offset), int(d.s.offset), 0 };
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    }
    if (isValid())
        return QTimeZonePrivate::toOffsetData(d->data(forDateTime.toMSecsSinceEpoch()));

    return QTimeZonePrivate::invalidOffsetData();
}

/*!
    Returns \c true if the system backend supports obtaining transitions.

    Transitions are changes in the time-zone: these happen when DST turns on or
    off and when authorities alter the offsets for the time-zone.

    This method is only available when feature \c timezone is enabled.

    \sa nextTransition(), previousTransition(), transitions()
*/

bool QTimeZone::hasTransitions() const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().hasTransitions();
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            return false;
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (isValid()) {
        return d->hasTransitions();
    }
    return false;
}

/*!
    Returns the first time zone Transition after the given \a afterDateTime.
    This is most useful when you have a Transition time and wish to find the
    Transition after it.

    If there is no transition after the given \a afterDateTime then an invalid
    OffsetData will be returned with an invalid QDateTime as its \c atUtc.

    The given \a afterDateTime is exclusive.

    This method is only available when feature \c timezone is enabled.

    \sa hasTransitions(), previousTransition(), transitions()
*/

QTimeZone::OffsetData QTimeZone::nextTransition(const QDateTime &afterDateTime) const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().nextTransition(afterDateTime);
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            break;
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (hasTransitions()) {
        return QTimeZonePrivate::toOffsetData(d->nextTransition(afterDateTime.toMSecsSinceEpoch()));
    }

    return QTimeZonePrivate::invalidOffsetData();
}

/*!
    Returns the first time zone Transition before the given \a beforeDateTime.
    This is most useful when you have a Transition time and wish to find the
    Transition before it.

    If there is no transition before the given \a beforeDateTime then an invalid
    OffsetData will be returned with an invalid QDateTime as its \c atUtc.

    The given \a beforeDateTime is exclusive.

    This method is only available when feature \c timezone is enabled.

    \sa hasTransitions(), nextTransition(), transitions()
*/

QTimeZone::OffsetData QTimeZone::previousTransition(const QDateTime &beforeDateTime) const
{
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().previousTransition(beforeDateTime);
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            break;
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (hasTransitions()) {
        return QTimeZonePrivate::toOffsetData(
            d->previousTransition(beforeDateTime.toMSecsSinceEpoch()));
    }

    return QTimeZonePrivate::invalidOffsetData();
}

/*!
    Returns a list of all time zone transitions between the given datetimes.

    The given \a fromDateTime and \a toDateTime are inclusive.

    This method is only available when feature \c timezone is enabled.

    \sa hasTransitions(), nextTransition(), previousTransition()
*/

QTimeZone::OffsetDataList QTimeZone::transitions(const QDateTime &fromDateTime,
                                                 const QDateTime &toDateTime) const
{
    OffsetDataList list;
    if (d.isShort()) {
        switch (d.s.spec()) {
        case Qt::LocalTime:
            return systemTimeZone().transitions(fromDateTime, toDateTime);
        case Qt::UTC:
        case Qt::OffsetFromUTC:
            break;
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
    } else if (hasTransitions()) {
        const QTimeZonePrivate::DataList plist = d->transitions(fromDateTime.toMSecsSinceEpoch(),
                                                                toDateTime.toMSecsSinceEpoch());
        list.reserve(plist.size());
        for (const QTimeZonePrivate::Data &pdata : plist)
            list.append(QTimeZonePrivate::toOffsetData(pdata));
    }
    return list;
}

// Static methods

/*!
    Returns the current system time zone IANA ID.

    On Windows this ID is translated from the Windows ID using an internal
    translation table and the user's selected country.  As a consequence there
    is a small chance any Windows install may have IDs not known by Qt, in
    which case "UTC" will be returned.

    This method is only available when feature \c timezone is enabled.

    \sa systemTimeZone()
*/

QByteArray QTimeZone::systemTimeZoneId()
{
    QByteArray sys = global_tz->backend->systemTimeZoneId();
    if (!sys.isEmpty())
        return sys;
    // The system zone, despite the empty ID, may know its real ID anyway:
    auto zone = systemTimeZone();
    if (zone.isValid() && !zone.id().isEmpty())
        return zone.id();
    // TODO: "-00:00", meaning "unspecified local zone" in some RFC, may be more apt.
    // If all else fails, guess UTC.
    return QTimeZonePrivate::utcQByteArray();
}

/*!
    \since 5.5

    Returns a QTimeZone object that describes local system time.

    This method is only available when feature \c timezone is enabled. The
    returned instance is usually equivalent to the lightweight time
    representation \c {QTimeZone(QTimeZone::LocalTime)}, albeit implemented as a
    time zone.

    \sa utc(), Initialization, asBackendZone()
*/
QTimeZone QTimeZone::systemTimeZone()
{
    return QTimeZone(global_tz->backend->systemTimeZoneId());
}

/*!
    \since 5.5
    Returns a QTimeZone object that describes UTC as a time zone.

    This method is only available when feature \c timezone is enabled. It is
    equivalent to passing 0 to QTimeZone(int offsetSeconds) and to the
    lightweight time representation QTimeZone(QTimeZone::UTC), albeit
    implemented as a time zone, unlike the latter.

    \sa systemTimeZone(), Initialization, asBackendZone()
*/
QTimeZone QTimeZone::utc()
{
    return QTimeZone(QTimeZonePrivate::utcQByteArray());
}

/*!
    Returns \c true if a given time zone \a ianaId is available on this system.

    This method is only available when feature \c timezone is enabled.

    \sa availableTimeZoneIds()
*/

bool QTimeZone::isTimeZoneIdAvailable(const QByteArray &ianaId)
{
#if defined(Q_OS_UNIX) && !(defined(Q_OS_ANDROID) || defined(Q_OS_DARWIN))
    // Keep #if-ery consistent with selection of QTzTimeZonePrivate in
    // newBackendTimeZone(). Skip the pre-check, as the TZ backend accepts POSIX
    // zone IDs, which need not be valid IANA IDs.
#else
    // isValidId is not strictly required, but faster to weed out invalid
    // IDs as availableTimeZoneIds() may be slow
    if (!QTimeZonePrivate::isValidId(ianaId))
        return false;
#endif
    return QUtcTimeZonePrivate().isTimeZoneIdAvailable(ianaId)
        || QUtcTimeZonePrivate::offsetFromUtcString(ianaId) != QTimeZonePrivate::invalidSeconds()
        || global_tz->backend->isTimeZoneIdAvailable(ianaId);
}

static QList<QByteArray> set_union(const QList<QByteArray> &l1, const QList<QByteArray> &l2)
{
    QList<QByteArray> result;
    result.reserve(l1.size() + l2.size());
    std::set_union(l1.begin(), l1.end(),
                   l2.begin(), l2.end(),
                   std::back_inserter(result));
    return result;
}

/*!
    Returns a list of all available IANA time zone IDs on this system.

    This method is only available when feature \c timezone is enabled.

    \sa isTimeZoneIdAvailable()
*/

QList<QByteArray> QTimeZone::availableTimeZoneIds()
{
    return set_union(QUtcTimeZonePrivate().availableTimeZoneIds(),
                     global_tz->backend->availableTimeZoneIds());
}

/*!
    Returns a list of all available IANA time zone IDs for a given \a territory.

    As a special case, a \a territory of \l {QLocale::}{AnyTerritory} selects
    those time zones that have no kown territorial association, such as UTC. If
    you require a list of all time zone IDs for all territories then use the
    standard availableTimeZoneIds() method.

    This method is only available when feature \c timezone is enabled.

    \sa isTimeZoneIdAvailable(), territory()
*/

QList<QByteArray> QTimeZone::availableTimeZoneIds(QLocale::Territory territory)
{
    return set_union(QUtcTimeZonePrivate().availableTimeZoneIds(territory),
                     global_tz->backend->availableTimeZoneIds(territory));
}

/*!
    Returns a list of all available IANA time zone IDs with a given standard
    time offset of \a offsetSeconds.

    This method is only available when feature \c timezone is enabled.

    \sa isTimeZoneIdAvailable()
*/

QList<QByteArray> QTimeZone::availableTimeZoneIds(int offsetSeconds)
{
    return set_union(QUtcTimeZonePrivate().availableTimeZoneIds(offsetSeconds),
                     global_tz->backend->availableTimeZoneIds(offsetSeconds));
}

/*!
    Returns the Windows ID equivalent to the given \a ianaId.

    This method is only available when feature \c timezone is enabled.

    \sa windowsIdToDefaultIanaId(), windowsIdToIanaIds()
*/

QByteArray QTimeZone::ianaIdToWindowsId(const QByteArray &ianaId)
{
    return QTimeZonePrivate::ianaIdToWindowsId(ianaId);
}

/*!
    Returns the default IANA ID for a given \a windowsId.

    Because a Windows ID can cover several IANA IDs in several different
    territories, this function returns the most frequently used IANA ID with no
    regard for the territory and should thus be used with care.  It is usually
    best to request the default for a specific territory.

    This method is only available when feature \c timezone is enabled.

    \sa ianaIdToWindowsId(), windowsIdToIanaIds()
*/

QByteArray QTimeZone::windowsIdToDefaultIanaId(const QByteArray &windowsId)
{
    return QTimeZonePrivate::windowsIdToDefaultIanaId(windowsId);
}

/*!
    Returns the default IANA ID for a given \a windowsId and \a territory.

    Because a Windows ID can cover several IANA IDs within a given territory,
    the most frequently used IANA ID in that territory is returned.

    As a special case, \l{QLocale::}{AnyTerritory} returns the default of those
    IANA IDs that have no known territorial association.

    This method is only available when feature \c timezone is enabled.

    \sa ianaIdToWindowsId(), windowsIdToIanaIds(), territory()
*/

QByteArray QTimeZone::windowsIdToDefaultIanaId(const QByteArray &windowsId,
                                               QLocale::Territory territory)
{
    return QTimeZonePrivate::windowsIdToDefaultIanaId(windowsId, territory);
}

/*!
    Returns all the IANA IDs for a given \a windowsId.

    The returned list is sorted alphabetically.

    This method is only available when feature \c timezone is enabled.

    \sa ianaIdToWindowsId(), windowsIdToDefaultIanaId()
*/

QList<QByteArray> QTimeZone::windowsIdToIanaIds(const QByteArray &windowsId)
{
    return QTimeZonePrivate::windowsIdToIanaIds(windowsId);
}

/*!
    Returns all the IANA IDs for a given \a windowsId and \a territory.

    As a special case, \l{QLocale::}{AnyTerritory} selects those IANA IDs that
    have no known territorial association.

    The returned list is in order of frequency of usage, i.e. larger zones
    within a territory are listed first.

    This method is only available when feature \c timezone is enabled.

    \sa ianaIdToWindowsId(), windowsIdToDefaultIanaId(), territory()
*/

QList<QByteArray> QTimeZone::windowsIdToIanaIds(const QByteArray &windowsId,
                                                QLocale::Territory territory)
{
    return QTimeZonePrivate::windowsIdToIanaIds(windowsId, territory);
}

/*!
    \fn QTimeZone QTimeZone::fromStdTimeZonePtr(const std::chrono::time_zone *timeZone)
    \since 6.4

    Returns a QTimeZone object representing the same time zone as \a timeZone.
    The IANA ID of \a timeZone must be one of the available system IDs,
    otherwise an invalid time zone will be returned.

    This method is only available when feature \c timezone is enabled.
*/
#endif // feature timezone

template <typename Stream, typename Wrap>
void QTimeZone::Data::serialize(Stream &out, const Wrap &wrap) const
{
    if (isShort()) {
        switch (s.spec()) {
        case Qt::UTC:
            out << wrap("QTimeZone::UTC");
            break;
        case Qt::LocalTime:
            out << wrap("QTimeZone::LocalTime");
            break;
        case Qt::OffsetFromUTC:
            out << wrap("AheadOfUtcBy") << int(s.offset);
            break;
        case Qt::TimeZone:
            Q_UNREACHABLE();
            break;
        }
        return;
    }
#if QT_CONFIG(timezone)
    if constexpr (std::is_same<Stream, QDataStream>::value) {
        if (d)
            d->serialize(out);
    } else {
        // QDebug, traditionally gets a QString, hence quotes round the (possibly empty) ID:
        out << QString::fromUtf8(d ? QByteArrayView(d->id()) : QByteArrayView());
    }
#endif
}

#ifndef QT_NO_DATASTREAM
// Invalid, as an IANA ID: too long, starts with - and has other invalid characters in it
static inline QString invalidId() { return QStringLiteral("-No Time Zone Specified!"); }

QDataStream &operator<<(QDataStream &ds, const QTimeZone &tz)
{
    const auto toQString = [](const char *text) {
        return QString(QLatin1StringView(text));
    };
    if (tz.isValid())
        tz.d.serialize(ds, toQString);
    else
        ds << invalidId();
    return ds;
}

QDataStream &operator>>(QDataStream &ds, QTimeZone &tz)
{
    QString ianaId;
    ds >> ianaId;
    // That may be various things other than actual IANA IDs:
    if (ianaId == invalidId()) {
        tz = QTimeZone();
    } else if (ianaId == "OffsetFromUtc"_L1) {
        int utcOffset;
        QString name;
        QString abbreviation;
        int territory;
        QString comment;
        ds >> ianaId >> utcOffset >> name >> abbreviation >> territory >> comment;
#if QT_CONFIG(timezone)
        // Try creating as a system timezone, which succeeds (producing a valid
        // zone) iff ianaId is valid; use this if it is a plain offset from UTC
        // zone, with the right offset, ignoring the other data:
        tz = QTimeZone(ianaId.toUtf8());
        if (!tz.isValid() || tz.hasDaylightTime()
            || tz.offsetFromUtc(QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC)) != utcOffset) {
            // Construct a custom timezone using the saved values:
            tz = QTimeZone(ianaId.toUtf8(), utcOffset, name, abbreviation,
                           QLocale::Territory(territory), comment);
        }
#else
        tz = QTimeZone::fromSecondsAheadOfUtc(utcOffset);
#endif
    } else if (ianaId == "AheadOfUtcBy"_L1) {
        int utcOffset;
        ds >> utcOffset;
        tz = QTimeZone::fromSecondsAheadOfUtc(utcOffset);
    } else if (ianaId == "QTimeZone::UTC"_L1) {
        tz = QTimeZone(QTimeZone::UTC);
    } else if (ianaId == "QTimeZone::LocalTime"_L1) {
        tz = QTimeZone(QTimeZone::LocalTime);
#if QT_CONFIG(timezone)
    } else {
        tz = QTimeZone(ianaId.toUtf8());
#endif
    }
    return ds;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QTimeZone &tz)
{
    QDebugStateSaver saver(dbg);
    const auto asIs = [](const char *text) { return text; };
    // TODO Include backend and data version details?
    dbg.nospace() << "QTimeZone(";
    tz.d.serialize(dbg, asIs);
    dbg.nospace() << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE
