/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qcalendar.h"
#include "qcalendarbackend_p.h"
#include "qgregoriancalendar_p.h"
#ifndef QT_BOOTSTRAPPED
#include "qjuliancalendar_p.h"
#include "qmilankoviccalendar_p.h"
#endif
#if QT_CONFIG(jalalicalendar)
#include "qjalalicalendar_p.h"
#endif
#if QT_CONFIG(islamiccivilcalendar)
#include "qislamiccivilcalendar_p.h"
#endif

#include "qatomic.h"
#include "qdatetime.h"
#include "qcalendarmath_p.h"
#include <qhash.h>
#include <qmutex.h>
#include <private/qlocking_p.h>
#include <qdebug.h>

#include <vector>

QT_BEGIN_NAMESPACE

static const QCalendarBackend *backendFromEnum(QCalendar::System system);

namespace {

struct CalendarName : public QString
{
    CalendarName(const QString &name) : QString(name) {}
};

inline bool operator==(const CalendarName &u, const CalendarName &v)
{
    return u.compare(v, Qt::CaseInsensitive) == 0;
}

inline size_t qHash(const CalendarName &key, size_t seed = 0) noexcept
{
    return qHash(key.toLower(), seed);
}

static QBasicMutex registryMutex; // Protects registry from concurrent access
struct Registry
{
    std::vector<QCalendarBackend *> byId;
    QHash<CalendarName, QCalendarBackend *> byName;
    QAtomicPointer<const QCalendarBackend> gregorianCalendar = nullptr;
    QAtomicInteger<int> status = 0; // 1: populated, 2: destructing

    Registry()
    {
        byId.resize(int(QCalendar::System::Last) + 1);
    }

    ~Registry()
    {
        status.storeRelaxed(2);
        const auto lock = qt_scoped_lock(registryMutex);
        qDeleteAll(byId);
    }

    bool registerName(QCalendarBackend *calendar, const QString &name)
    {
        Q_ASSERT(!name.isEmpty());
        if (status.loadRelaxed() > 1 || name.isEmpty())
            return false;
        const auto lock = qt_scoped_lock(registryMutex);

        const auto found = byName.find(name);
        if (found != byName.end()) {
            // Re-registering a calendar with a name it has already is OK (and
            // can be used to test whether its constructor successfully
            // registered its primary name).
            return found.value() == calendar;
        }
        byName.insert(name, calendar);
        return true;
    }
    void addCalendar(QCalendarBackend *calendar, const QString &name, QCalendar::System id)
    {
        if (status.loadRelaxed() > 1 || name.isEmpty() || !registerName(calendar, name))
            return;
        const auto lock = qt_scoped_lock(registryMutex);
        if (id == QCalendar::System::User) {
            byId.push_back(calendar);
        } else {
            Q_ASSERT(byId.size() > size_t(id));
            Q_ASSERT(byId[size_t(id)] == nullptr);
            byId[size_t(id)] = calendar;
        }
        if (id == QCalendar::System::Gregorian) {
            // We succeeded in registering the name, so must be the first
            // instantiator of QGregorianCalendar to get here.
            const bool ok = gregorianCalendar.testAndSetRelease(nullptr, calendar);
#if defined(QT_FORCE_ASSERTS) || !defined(QT_NO_DEBUG)
            Q_ASSERT(ok);
#else
            Q_UNUSED(ok);
#endif
        }
    }
    /*
        \internal
        Ensures each \c{enum}-available calendar has been instantiated.

        This arranges for each to register itself by name; it only does anything on
        its first call, which ensures that name-based lookups can always find all
        the calendars available via the \c enum.
    */
    void populate()
    {
        if (status.loadRelaxed())
            return;

        for (int i = 0; i <= int(QCalendar::System::Last); ++i) {
            {
                const auto lock = qt_scoped_lock(registryMutex); // so we can check byId[i]
                if (status.loadRelaxed()) // Might as well check while we're locked
                    return;
                if (byId[i])
                    continue;
            }
            (void)backendFromEnum(QCalendar::System(i));
        }

        status.testAndSetRelease(0, 1);
    }
};

}

Q_GLOBAL_STATIC(Registry, calendarRegistry);

// Must not be called in a thread that's holding registryMutex locked,
// since it calls constructors, which need to register.
static const QCalendarBackend *backendFromEnum(QCalendar::System system)
{
    QCalendarBackend *backend = nullptr;
    switch (system) {
    case QCalendar::System::Gregorian:
        backend = new QGregorianCalendar;
        break;
#ifndef QT_BOOTSTRAPPED
    case QCalendar::System::Julian:
        backend = new QJulianCalendar;
        break;
    case QCalendar::System::Milankovic:
        backend = new QMilankovicCalendar;
        break;
#endif
#if QT_CONFIG(jalalicalendar)
    case QCalendar::System::Jalali:
        backend = new QJalaliCalendar;
        break;
#endif
#if QT_CONFIG(islamiccivilcalendar)
    case QCalendar::System::IslamicCivil:
        backend = new QIslamicCivilCalendar;
        break;
#else // When highest-numbered system isn't enabled, ensure we have a case for Last:
    case QCalendar::System::Last:
#endif
    case QCalendar::System::User:
        Q_UNREACHABLE();
    }
    if (!backend)
        return backend;
    const QString name = backend->name();
    // Check for successful registration:
    if (calendarRegistry->registerName(backend, name)) {
#if defined(QT_FORCE_ASSERTS) || !defined(QT_NO_DEBUG)
        const auto lock = qt_scoped_lock(registryMutex);
        Q_ASSERT(backend == calendarRegistry->byId[size_t(system)]);
#endif // else Q_ASSERT() is a no-op and we don't need the lock
        return backend;
    }
    // Duplicate registration: caller can be sure that byId[system] is correctly
    // set, provided system <= Last.
    delete backend;
    return nullptr;
}

/*!
    \since 5.14

    \class QCalendarBackend
    \inmodule QtCore
    \internal
    \reentrant
    \brief The QCalendarBackend class provides basic calendaring functions.

    QCalendarBackend provides the base class on which all calendar types are
    implemented. On construction, the backend is registered with its primary
    name.

    A backend, once successfully registered with its primary name, may also be
    registered with aliases, where the calendar is known by several
    names. Registering with the name used by CLDR (the Unicode consortium's
    Common Locale Data Repository) is recommended, particularly when interacting
    with third-party software. Once a backend is registered for a name,
    QCalendar can be constructed using that name to select the backend.

    Each built-in backend has a distinct primary name and all built-in backends
    are instantiated before any custom backend is registered, to prevent custom
    backends with conflicting names from replacing built-in backends.

    Each calendar backend must inherit from QCalendarBackend and implement its
    pure virtual methods. It may also override some other virtual methods, as
    needed.

    Most backends are pure code, with no data elements. Such backends should
    normally be implemented as singletons. For a backend to be added to the
    QCalendar::System \c enum, it must be such a singleton, with a case in
    backendFromEnum()'s switch statement (above) to instantiate it.

    \section1 Instantiating backends

    Backends may be defined by third-party, plugin or user code. When such
    custom backends are instantiated, in their calls to the QCalendarBackend
    base-class constructor, each instance should pass a distinct primary name to
    the base-class constructor and omit the \c system parameter.

    A backend class that has instance variables as well as code may be
    instantiated many times, each with a distinct primary name, to implement
    distinct backends - presumably variants on some parameterized calendar.
    Each instance is then a distinct backend. A pure code backend class shall
    typically only be instantiated once, as it is only capable of representing
    one backend.

    Each backend should be instantiated exactly once, on the heap (using the C++
    \c new operator); this will register it with the QCalendar implementation
    code and ensure it is available, by its primary name, to all code that may
    subsequently need it.  It will be deleted on program termination along with
    the registry in which QCalendar records backends.

    The single exception to this is that each backend's instantiator should
    verify that it was registered successfully with its primary name. It can do
    this by calling registerAlias() with that name; this will return true if it
    is already registered with the name. If it returns false, the instantiation
    has used a name that was already in use so the new backend has not been
    registered and the instantiator retains ownership of the backend instance;
    it will not be accessible to QCalendar. (Since registerAlias() is protected,
    a custom backend's class shall typically need to provide a method to perform
    this check for its instantiator.)

    Built-in backends, identified by QCalendar::System values other than User,
    should only be instantiated by code in the implementation of QCalendar; no
    other code should ever instantiate one. As noted above, such a backend must
    be a singleton. Its constructor passes down the \c enum member that
    identifies it as \c system to the base-class constructor.

    The shareable base-classes for backends, QRomanCalendar and QHijriCalendar,
    are not themselves identified by QCalendar::System and may be used as
    base-classes for custom calendar backends, but cannot be instantiated
    themselves.

    \sa registerAlias(), QDate, QDateTime, QDateEdit, QDateTimeEdit,
        QCalendarWidget
*/

/*!
    Constructs the calendar and registers it under \a name using \a system.

    On successful registration, the calendar backend registry takes over
    ownership of the instance and shall delete it on program exit in the course
    of the registry's own destruction. The instance can determine whether it was
    successfully registered by calling registerAlias() with the same \a name it
    passed to this base-class constructor. If that returns \c false, the
    instance has not been registered, QCalendar cannot use it, it should not
    attempt to register any other aliases and the code that instantiated the
    backend is responsible for deleting it.

    The \a system is optional and should only be passed by built-in
    implementations of the standard calendars documented in \l
    QCalendar::System. Custom backends should not pass \a system.

    Only one backend instance should ever be registered for any given \a system:
    in the event of a backend being created when one with the same \a system
    already exists, the new backend is not registered. The \a name passed with a
    \a system (other than \l{QCalendar::System}{User}) must be the \c{name()} of
    the backend constructed.

    The \a name must be non-empty and unique; after one backend has been
    registered for a name or alias, no other backend can be registered with that
    name. The presence of another backend registered with the same name may mean
    the backend is redundant, as the system already has a backend to handle the
    given calendar type.

    \note \c{QCalendar(name).isValid()} will return true precisely when the
    given \c name is in use already. This can be used as a test before
    instantiating a backend with the given \c name.

    \sa calendarSystem(), registerAlias()
*/
QCalendarBackend::QCalendarBackend(const QString &name, QCalendar::System system)
{
    Q_ASSERT(!name.isEmpty());
    // Will lock the registry mutex on its own, so no need to do it here:
    calendarRegistry->addCalendar(this, name, system);
}

/*!
    Destroys the calendar.

    Client code should only call this if instantiation failed to register the
    backend, as revealed by the instanee failing to registerAlias() with the
    name it passed to this base-class's constructor. Only a backend that fails
    to register can safely be deleted; and the client code that instantiated it
    is indeed responsible for deleting it.

    Once a backend has been successfully registered, there may be QCalendar
    instances using it; deleting it while they still reference it would lead to
    undefined behavior. Such a backend shall be deleted when the calendar
    backend registry is deleted on program exit; the registry takes over
    ownership of the instance on successful registration.

    \sa registerAlias()
*/
QCalendarBackend::~QCalendarBackend()
{
    // Either the registry is destroying itself, in which case it takes care of
    // dropping any references to this, or this never got registered, so there
    // is no need to tell the registry to forget it.
}

/*!
    The calendar system of this calendar.

    Each calendar backend constructible from the QCalendar::System \c enum
    should return the member of that \c enum that produces it. Other calendars
    should return User.

    \sa QCalendarBackend::fromEnum()
*/
QCalendar::System QCalendarBackend::calendarSystem() const
{
    return QCalendar::System::User;
}

/*!
    \fn QString QCalendarBackend::name() const;

    This pure virtual method should be overloaded by each backend implementation
    to return the name that the backend passes to the base-class as its name.
*/

/*!
    The primary name of this calendar.
*/
QString QCalendar::name() const
{
    return d ? d->name() : QString();
}

// date queries
/*!
   \fn int QCalendarBackend::daysInMonth(int month, int year) const

   Returns number of days in the month number \a month, in year \a year.

   An implementation should return 0 if the given year had no such month. If
   year is QCalendar::Unspecified, return the usual number of days for the
   month, in those years that include it.

   Calendars with intercallary days may represent these as extra days of the
   preceding month, or as short months separate from the usual ones. In the
   former case, daysInMonth(month, year) should be the number of ordinary days
   in the month, although \c{isDateValid(year, month, day)} might return \c true
   for some larger values of \c day.

   \sa daysInYear(), monthsInYear(), minimumDaysInMonth(), maximumDaysInMonth()
*/

// properties of the calendar

/*!
    \fn bool QCalendarBackend::isLeapYear(int year) const

    Returns \c true if the specified \a year is a leap year for this calendar.

    \sa daysInYear(), isDateValid()
*/

/*!
    \fn bool QCalendarBackend::isLunar() const

    Returns \c true if this calendar is a lunar calendar. Otherwise returns \c
    false.

    A lunar calendar is a calendar based upon the monthly cycles of the Moon's
    phases (synodic months). This contrasts with solar calendars, whose annual
    cycles are based only upon the solar year.

    \sa isLuniSolar(), isSolar(), isProleptic()
*/

/*!
    \fn bool QCalendarBackend::isLuniSolar() const

    Returns \c true if this calendar is a lunisolar calendar. Otherwise returns
    \c false.

    A lunisolar calendar is a calendar whose date indicates both the moon phase
    and the time of the solar year.

    \sa isLunar(), isSolar(), isProleptic()
*/

/*!
    \fn bool QCalendarBackend::isSolar() const

    Returns \c true if this calendar is a solar calendar. Otherwise returns
    \c false.

    A solar calendar is a calendar whose dates indicate the season or almost
    equivalently the apparent position of the sun relative to the fixed stars.
    The Gregorian calendar, widely accepted as standard in the world,
    is an example of solar calendar.

    \sa isLuniSolar(), isLunar(), isProleptic()
*/

/*!
    Returns the total number of days in the year number \a year.
    Returns zero if there is no such year in this calendar.

    This base implementation returns 366 for leap years and 365 for ordinary
    years.

    \sa monthsInYear(), daysInMonth(), isLeapYear()
*/
int QCalendarBackend::daysInYear(int year) const
{
    return monthsInYear(year) ? isLeapYear(year) ? 366 : 365 : 0;
}

/*!
    Returns the total number of months in the year number \a year.
    Returns zero if there is no such year in this calendar.

    This base implementation returns 12 for any valid year.

    \sa daysInYear(), maximumMonthsInYear(), isDateValid()
*/
int QCalendarBackend::monthsInYear(int year) const
{
    return year > 0 || (year < 0 ? isProleptic() : hasYearZero()) ? 12 : 0;
}

/*!
    Returns \c true if the date specified by \a year, \a month, and \a day is
    valid for this calendar; otherwise returns \c false. For example,
    the date 2018-04-19 is valid for the Gregorian calendar, but 2018-16-19 and
    2018-04-38 are invalid.

    Calendars with intercallary days may represent these as extra days of the
    preceding month or as short months separate from the usual ones. In the
    former case, a \a day value greater than \c{daysInMonth(\a{month},
    \a{year})} may be valid.

    \sa daysInMonth(), monthsInYear()
*/
bool QCalendarBackend::isDateValid(int year, int month, int day) const
{
    return day > 0 && day <= daysInMonth(month, year);
}

/*!
    Returns \c true if this calendar is a proleptic calendar. Otherwise returns
    \c false.

    A proleptic calendar results from allowing negative year numbers to indicate
    years before the nominal start of the calendar system.

    \sa isLuniSolar(), isSolar(), isLunar(), hasYearZero()
*/

bool QCalendarBackend::isProleptic() const
{
    return true;
}

/*!
    Returns \c true if year number \c 0 is considered a valid year in this
    calendar. Otherwise returns \c false.

    \sa isDateValid(), isProleptic()
*/

bool QCalendarBackend::hasYearZero() const
{
    return false;
}

/*!
    Returns the maximum number of days in a month for any year.

    This base implementation returns 31, as this is a common case.

    For calendars with intercallary days, although daysInMonth() doesn't include
    the intercallary days in its count for an individual month,
    maximumDaysInMonth() should include intercallary days, so that it is the
    maximum value of \c day for which \c{isDateValid(year, month, day)} can be
    true.

    \sa maximumMonthsInYear(), daysInMonth()
*/
int QCalendarBackend::maximumDaysInMonth() const
{
    return 31;
}

/*!
    Returns the minimum number of days in any valid month of any valid year.

    This base implementation returns 29, as this is a common case.

    \sa maximumMonthsInYear(), daysInMonth()
*/
int QCalendarBackend::minimumDaysInMonth() const
{
    return 29;
}

/*!
    Returns the maximum number of months possible in any year.

    This base implementation returns 12, as this is a common case.

    \sa maximumDaysInMonth(), monthsInYear()
*/
int QCalendarBackend::maximumMonthsInYear() const
{
    return 12;
}

// Julian day number calculations

/*!
    \fn bool QCalendarBackend::dateToJulianDay(int year, int month, int day, qint64 *jd) const

    Computes the Julian day number corresponding to the specified \a year, \a
    month, and \a day. Returns true and sets \a jd if there is such a date in
    this calendar; otherwise, returns false.

    \sa QCalendar::partsFromDate(), julianDayToDate()
*/

/*!
    \fn QCalendar::YearMonthDay QCalendarBackend::julianDayToDate(qint64 jd) const

    Computes the year, month, and day in this calendar for the given Julian day
    number \a jd. If the given day falls outside this calendar's scope
    (e.g. before the start-date of a non-proleptic calendar), the returned
    structure's isValid() is false; otherwise, its year, month, and day fields
    provide this calendar's description of the date.

    \sa QCalendar::dateFromParts(), dateToJulianDay()
*/

/*!
   Returns the day of the week for the given Julian Day Number \a jd.

   This is 1 for Monday through 7 for Sunday.

   Calendars with intercallary days may return larger values for these
   intercallary days. They should avoid using 0 for any special purpose (it is
   already used in QDate::dayOfWeek() to mean an invalid date). The calendar
   should treat the numbers used as an \c enum, whose values need not be
   contiguous, nor need they follow closely from the 1 through 7 of the usual
   returns. It suffices that weekDayName() can recognize each such number as
   identifying a distinct name, that it returns to identify the particular
   intercallary day.

   This base implementation uses the day-numbering that various calendars have
   borrowed off the Hebrew calendar.

   \sa weekDayName(), standaloneWeekDayName(), QDate::dayOfWeek()
 */
int QCalendarBackend::dayOfWeek(qint64 jd) const
{
    return QRoundingDown::qMod(jd, 7) + 1;
}

// Month and week-day name look-ups (implemented in qlocale.cpp):
/*!
    \fn QString QCalendarBackend::monthName(const QLocale &locale, int month, int year,
                                            QLocale::FormatType format) const

    Returns the name of the specified \a month in the given \a year for the
    chosen \a locale, using the given \a format to determine how complete the
    name is.

    If \a year is Unspecified, return the name for the month that usually has
    this number within a typical year. Calendars with a leap month that isn't
    always the last may need to take account of the year to map the month number
    to the particular year's month with that number.

    \note Backends for which CLDR provides data can configure the default
    implementation of the two month name look-up methods by arranging for
    localeMonthIndexData() and localeMonthData() to provide access to the CLDR
    data (see cldr2qlocalexml.py, qlocalexml2cpp.py and existing backends).
    Conversely, backends that override both month name look-up methods need not
    return anything meaningful from localeMonthIndexData() or localeMonthData().

    \sa standaloneMonthName(), QLocale::monthName()
*/

/*!
    \fn QString QCalendarBackend::standaloneMonthName(const QLocale &locale, int month, int year
                                                      QLocale::FormatType format) const

    Returns the standalone name of the specified \a month in the chosen \a
    locale, using the specified \a format to determine how complete the name is.

    If \a year is Unspecified, return the standalone name for the month that
    usually has this number within a typical year. Calendars with a leap month
    that isn't always the last may need to take account of the year to map the
    month number to the particular year's month with that number.

    \sa monthName(), QLocale::standaloneMonthName()
*/

/*!
    \fn QString QCalendarBackend::weekDayName(const QLocale &locale, int day,
                                              QLocale::FormatType format) const

    Returns the name of the specified \a day of the week in the chosen \a
    locale, using the specified \a format to determine how complete the name is.

    The base implementation handles \a day values from 1 to 7 using the day
    names CLDR provides, which are suitable for calendards that use the same
    (Hebrew-derived) week as the Gregorian calendar.

    Calendars whose dayOfWeek() returns a value outside the range from 1 to 7
    need to reimplement this method to handle such extra week-day values. They
    can assume that \a day is a value returned by the same calendar's
    dayOfWeek().

    \sa dayOfWeek(), standaloneWeekDayName(), QLocale::dayName()
*/

/*!
    \fn QString QCalendarBackend::standaloneWeekDayName(const QLocale &locale, int day,
                                                        QLocale::FormatType format) const

    Returns the standalone name of the specified \a day of the week in the
    chosen \a locale, using the specified \a format to determine how complete
    the name is.

    The base implementation handles \a day values from 1 to 7 using the
    standalone day names CLDR provides, which are suitable for calendards that
    use the same (Hebrew-derived) week as the Gregorian calendar.

    Calendars whose dayOfWeek() returns a value outside the range from 1 to 7
    need to reimplement this method to handle such extra week-day values. They
    can assume that \a day is a value returned by the same calendar's
    dayOfWeek().

    \sa dayOfWeek(), weekDayName(), QLocale::standaloneDayName()
*/

/*!
    \fn QString QCalendarBackend::dateTimeToString(QStringView format, const QDateTime &datetime,
                                                   QDate dateOnly, QTime timeOnly,
                                                   const QLocale &locale) const

    Returns a string representing a given date, time or date-time.

    If \a datetime is specified and valid, it is used and both date and time
    format tokens are converted to appropriate representations of the parts of
    the datetime. Otherwise, if \a dateOnly is valid, only date format tokens
    are converted; else, if \a timeOnly is valid, only time format tokens are
    converted. If none are valid, an empty string is returned.

    The specified \a locale influences how some format tokens are converted; for
    example, when substituting day and month names and their short-forms. For
    the supported formatting tokens, see QDate::toString() and
    QTime::toString(). As described above, the provided date, time and date-time
    determine which of these tokens are recognized: where these appear in \a
    format they are replaced by data. Any text in \a format not recognized as a
    format token is copied verbatim into the result string.

    \sa QDate::toString(), QTime::toString(), QDateTime::toString()
*/
// End of methods implemented in qlocale.cpp

/*!
    Returns a list of names of the available calendar systems. Any
    QCalendarBackend sub-class must be registered before being exposed to Date
    and Time APIs.

    \sa registerAlias(), fromName()
*/
QStringList QCalendarBackend::availableCalendars()
{
    if (calendarRegistry.isDestroyed())
        return {};
    calendarRegistry->populate();
    const auto registryLock = qt_scoped_lock(registryMutex);
    return QStringList(calendarRegistry->byName.keyBegin(), calendarRegistry->byName.keyEnd());
}

/*!
    Registers an alias for this calendar backend. Once a backend is registered,
    its name will be included in the list of available calendars and the
    calendar can be instantiated by name.

    Returns \c false if the given \a name is already in use by a different
    backend or \c true if this calendar is already registered with this
    name. (This can be used, with its primary name, to test whether a backend's
    construction successfully registered it.) Otherwise it registers this
    calendar backend for this name and returns \c true.

    \sa availableCalendars(), fromName()
*/
bool QCalendarBackend::registerAlias(const QString &name)
{
    if (calendarRegistry.isDestroyed() || name.isEmpty())
        return false;
    // Constructing this accessed the registry, so ensured it exists:
    Q_ASSERT(calendarRegistry.exists());

    // Not taking the lock on the registry here because it's just one call
    // (which internally locks anyway).
    return calendarRegistry->registerName(this, name);
}

/*!
    \internal
    Returns a pointer to a named calendar backend.

    If the given \a name is present in availableCalendars(), the backend
    matching it is returned; otherwise, \c nullptr is returned. Matching of
    names ignores case. Note that this won't provoke construction of a calendar
    backend, it will only return ones that have been instantiated (and not yet
    destroyed) by some other means. However, calendars available via the
    QCalendar::System \c enum are always registered when this is called.

    \sa availableCalendars(), registerAlias(), fromEnum()
*/
const QCalendarBackend *QCalendarBackend::fromName(QStringView name)
{
    if (calendarRegistry.isDestroyed())
        return nullptr;
    calendarRegistry->populate();
    const auto registryLock = qt_scoped_lock(registryMutex);
    auto it = calendarRegistry->byName.find(name.toString());
    return it == calendarRegistry->byName.end() ? nullptr : *it;
}

/*!
    \internal
    \overload
 */
const QCalendarBackend *QCalendarBackend::fromName(QLatin1String name)
{
    if (calendarRegistry.isDestroyed())
        return nullptr;
    calendarRegistry->populate();
    const auto registryLock = qt_scoped_lock(registryMutex);
    auto it = calendarRegistry->byName.find(QString(name));
    return it == calendarRegistry->byName.end() ? nullptr : *it;
}

/*!
    \internal
    Returns a pointer to a calendar backend, specified by \c enum.

    This will instantiate the indicated calendar (which will enable fromName()
    to return it subsequently), but only for the Qt-supported calendars for
    which (where relevant) the appropriate feature has been enabled.
*/
const QCalendarBackend *QCalendarBackend::fromEnum(QCalendar::System system)
{
    if (calendarRegistry.isDestroyed() || system == QCalendar::System::User)
        return nullptr;
    {
        const auto registryLock = qt_scoped_lock(registryMutex);
        Q_ASSERT(calendarRegistry->byId.size() >= size_t(system));
        if (auto *c = calendarRegistry->byId[size_t(system)])
            return c;
    }
    if (auto *result = backendFromEnum(system))
        return result;
    const auto registryLock = qt_scoped_lock(registryMutex);
    return calendarRegistry->byId[size_t(system)];
}

/*!
    \since 5.14

    \class QCalendar
    \inmodule QtCore
    \reentrant
    \brief The QCalendar class describes calendar systems.

    A QCalendar object maps a year, month, and day-number to a specific day
    (ultimately identified by its Julian day number), using the rules of a
    particular system.

    The default QCalendar() is a proleptic Gregorian calendar, which has no year
    zero. Other calendars may be supported by enabling suitable features or
    loading plugins. Calendars supported as features can be constructed by
    passing the QCalendar::System enumeration to the constructor. All supported
    calendars may be constructed by name, once they have been constructed. (Thus
    plugins instantiate their calendar backend to register it.) Built-in
    backends, accessible via QCalendar::System, are also always available by
    name.

    A QCalendar value is immutable.

    \sa QDate, QDateTime
*/

/*!
    \enum QCalendar::System

    This enumerated type is used to specify a choice of calendar system.

    \value Gregorian The default calendar, used internationally.
    \value Julian An ancient Roman calendar with too few leap years.
    \value Milankovic A revised Julian calendar used by some Orthodox churches.
    \value Jalali The Solar Hijri calendar (also called Persian).
    \value IslamicCivil The (tabular) Islamic Civil calendar.
    \omitvalue Last
    \omitvalue User

    \sa QCalendar
*/

/*!
    \fn QCalendar::QCalendar()
    \fn QCalendar::QCalendar(QCalendar::System system)
    \fn QCalendar::QCalendar(QLatin1String name)
    \fn QCalendar::QCalendar(QStringView name)

    Constructs a calendar object.

    The choice of calendar to use may be indicated as \a system, using the
    enumeration QCalendar::System, or by \a name, using a string (either Unicode
    or Latin 1). Construction by name may depend on an instance of the given
    calendar being constructed by other means first. With no argument, the
    default constructor returns the Gregorian calendar.

    \sa QCalendar, System, isValid()
*/

QCalendar::QCalendar()
    : d(nullptr)
{
    if (calendarRegistry.isDestroyed())
        return;
    d = calendarRegistry->gregorianCalendar.loadAcquire();
    if (!d) {
        auto fresh = new QGregorianCalendar;
        if (!calendarRegistry->gregorianCalendar.testAndSetOrdered(fresh, fresh, d))
            delete fresh;
        Q_ASSERT(d);
    }
}

QCalendar::QCalendar(QCalendar::System system)
    : d(QCalendarBackend::fromEnum(system))
{
    // If system is valid, we should get a valid d for that system.
    Q_ASSERT(uint(system) > uint(QCalendar::System::Last) || (d && d->calendarSystem() == system));
}

QCalendar::QCalendar(QLatin1String name)
    : d(QCalendarBackend::fromName(name)) {}

QCalendar::QCalendar(QStringView name)
    : d(QCalendarBackend::fromName(name)) {}

/*!
    \fn bool QCalendar::isValid() const

    Returns true if this is a valid calendar object.

    Constructing a calendar with an unrecognised calendar name may result in an
    invalid object. Use this method to check after creating a calendar by name.
*/

// Date queries:

/*!
    Returns the number of days in the given \a month of the given \a year.

    Months are numbered consecutively, starting with 1 for the first month of
    each year. If \a year is \c Unspecified (its default, if not passed), the
    month's length in a normal year is returned.

    \sa maximumDaysInMonth(), minimumDaysInMonth()
*/
int QCalendar::daysInMonth(int month, int year) const
{
    return d ? d->daysInMonth(month, year) : 0;
}

/*!
    Returns the number of days in the given \a year.

    Handling of \c Unspecified as \a year is undefined.
*/
int QCalendar::daysInYear(int year) const
{
    return d ? d->daysInYear(year) : 0;
}

/*!
    Returns the number of months in the given \a year.

    If \a year is \c Unspecified, returns the maximum number of months in a
    year.

    \sa maximumMonthsInYear()
*/
int QCalendar::monthsInYear(int year) const
{
    return d ? year == Unspecified ? d->maximumMonthsInYear() : d->monthsInYear(year) : 0;
}

/*!
    Returns \c true precisely if the given \a year, \a month, and \a day specify
    a valid date in this calendar.

    Usually this means 1 <= month <= monthsInYear(year) and 1 <= day <=
    daysInMonth(month, year). However, calendars with intercallary days or
    months may complicate that.
*/
bool QCalendar::isDateValid(int year, int month, int day) const
{
    return d && d->isDateValid(year, month, day);
}

// properties of the calendar

/*!
    Returns \c true if this calendar object is the Gregorian calendar object
    used as default calendar by other Qt APIs, e.g. in QDate.
*/
bool QCalendar::isGregorian() const
{
    Q_ASSERT(calendarRegistry.exists());
    return d == calendarRegistry->gregorianCalendar.loadRelaxed();
}

/*!
    Returns \c true if the given \a year is a leap year.

    Since the year is not a whole number of days long, some years are longer
    than others. The difference may be a whole month or just a single day; the
    details vary between calendars.

    \sa isDateValid()
*/
bool QCalendar::isLeapYear(int year) const
{
    return d && d->isLeapYear(year);
}

/*!
    Returns \c true if this calendar is a lunar calendar.

    A lunar calendar is one based primarily on the phases of the moon.
*/
bool QCalendar::isLunar() const
{
    return d && d->isLunar();
}

/*!
    Returns \c true if this calendar is luni-solar.

    A luni-solar calendar expresses the phases of the moon but adapts itself to
    also keep track of the Sun's varying position in the sky, relative to the
    fixed stars.
*/
bool QCalendar::isLuniSolar() const
{
    return d && d->isLuniSolar();
}

/*!
    Returns \c true if this calendar is solar.

    A solar calendar is based primarily on the Sun's varying position in the
    sky, relative to the fixed stars.
*/
bool QCalendar::isSolar() const
{
    return d && d->isSolar();
}

/*!
    Returns \c true if this calendar is proleptic.

    A proleptic calendar is able to describe years arbitrarily long before its
    first. These are represented by negative year numbers and possibly by a year
    zero.

    \sa hasYearZero()
*/
bool QCalendar::isProleptic() const
{
    return d && d->isProleptic();
}

/*!
    Returns \c true if this calendar has a year zero.

    A calendar may represent years from its first year onwards but provide no
    way to describe years before its first; such a calendar has no year zero and
    is not proleptic.

    A calendar which represents years before its first may number these years
    simply by following the usual integer counting, so that the year before the
    first is year zero, with negative-numbered years preceding this; such a
    calendar is proleptic and has a year zero. A calendar might also have a year
    zero (for example, the year of some great event, with subsequent years being
    the first year after that event, the second year after, and so on) without
    describing years before its year zero. Such a calendar would have a year
    zero without being proleptic.

    Some calendars, however, represent years before their first by an alternate
    numbering; for example, the proleptic Gregorian calendar's first year is 1
    CE and the year before it is 1 BCE, preceded by 2 BCE and so on. In this
    case, we use negative year numbers for this alternate numbering, with year
    -1 as the year before year 1, year -2 as the year before year -1 and so
    on. Such a calendar is proleptic but has no year zero.

    \sa isProleptic()
*/
bool QCalendar::hasYearZero() const
{
    return d && d->hasYearZero();
}

/*!
    Returns the number of days in the longest month in the calendar, in any year.

    \sa daysInMonth(), minimumDaysInMonth()
*/
int QCalendar::maximumDaysInMonth() const
{
    return d ? d->maximumDaysInMonth() : 0;
}

/*!
    Returns the number of days in the shortest month in the calendar, in any year.

    \sa daysInMonth(), maximumDaysInMonth()
*/
int QCalendar::minimumDaysInMonth() const
{
    return d ? d->minimumDaysInMonth() : 0;
}

/*!
    Returns the largest number of months that any year may contain.

    \sa monthName(), standaloneMonthName(), monthsInYear()
*/
int QCalendar::maximumMonthsInYear() const
{
    return d ? d->maximumMonthsInYear() : 0;
}

// Julian Day conversions:

/*!
    \fn QDate QCalendar::dateFromParts(int year, int month, int day) const
    \fn QDate QCalendar::dateFromParts(const QCalendar::YearMonthDay &parts) const

    Converts a year, month, and day to a QDate.

    The \a year, \a month, and \a day may be passed as separate numbers or
    packaged together as the members of \a parts. Returns a QDate with the given
    year, month, and day of the month in this calendar, if there is one.
    Otherwise, including the case where any of the values is
    QCalendar::Unspecified, returns a QDate whose isNull() is true.

    \sa isDateValid(), partsFromDate()
*/
QDate QCalendar::dateFromParts(int year, int month, int day) const
{
    qint64 jd;
    return d && d->dateToJulianDay(year, month, day, &jd)
        ? QDate::fromJulianDay(jd) : QDate();
}

QDate QCalendar::dateFromParts(const QCalendar::YearMonthDay &parts) const
{
    return parts.isValid() ? dateFromParts(parts.year, parts.month, parts.day) : QDate();
}

/*!
    Converts a QDate to a year, month, and day of the month.

    The returned structure's isValid() shall be false if the calendar is unable
    to represent the given \a date. Otherwise its year, month, and day
    members record the so-named parts of its representation.

    \sa dateFromParts(), isProleptic(), hasYearZero()
*/
QCalendar::YearMonthDay QCalendar::partsFromDate(QDate date) const
{
    return d && date.isValid() ? d->julianDayToDate(date.toJulianDay()) : YearMonthDay();
}

/*!
    Returns the day of the week number for the given \a date.

    Returns zero if the calendar is unable to represent the indicated date.
    Returns 1 for Monday through 7 for Sunday. Calendars with intercallary days
    may use other numbers to represent these.

    \sa partsFromDate(), Qt::DayOfWeek
*/
int QCalendar::dayOfWeek(QDate date) const
{
    return d && date.isValid() ? d->dayOfWeek(date.toJulianDay()) : 0;
}

// Locale data access

/*!
    Returns a suitably localised name for a month.

    The month is indicated by a number, with \a month = 1 meaning the first
    month of the year and subsequent months numbered accordingly. Returns an
    empty string if the \a month number is unrecognized.

    The \a year may be Unspecified, in which case the mapping from numbers to
    names for a typical year's months should be used. Some calendars have leap
    months that aren't always at the end of the year; their mapping of month
    numbers to names may then depend on the placement of a leap month. Thus the
    year should normally be specified, if known.

    The name is returned in the form that would normally be used in a full date,
    in the specified \a locale; the \a format determines how fully it shall be
    expressed (i.e. to what extent it is abbreviated).

    \sa standaloneMonthName(), maximumMonthsInYear(), dateTimeToString()
*/
QString QCalendar::monthName(const QLocale &locale, int month, int year,
                             QLocale::FormatType format) const
{
    const int maxMonth = year == Unspecified ? maximumMonthsInYear() : monthsInYear(year);
    if (!d || month < 1 || month > maxMonth)
        return QString();

    return d->monthName(locale, month, year, format);
}

/*!
    Returns a suitably localised standalone name for a month.

    The month is indicated by a number, with \a month = 1 meaning the first
    month of the year and subsequent months numbered accordingly. Returns an
    empty string if the \a month number is unrecognized.

    The \a year may be Unspecified, in which case the mapping from numbers to
    names for a typical year's months should be used. Some calendars have leap
    months that aren't always at the end of the year; their mapping of month
    numbers to names may then depend on the placement of a leap month. Thus the
    year should normally be specified, if known.

    The name is returned in the form that would be used in isolation in the
    specified \a locale; the \a format determines how fully it shall be
    expressed (i.e. to what extent it is abbreviated).

    \sa monthName(), maximumMonthsInYear(), dateTimeToString()
*/
QString QCalendar::standaloneMonthName(const QLocale &locale, int month, int year,
                                       QLocale::FormatType format) const
{
    const int maxMonth = year == Unspecified ? maximumMonthsInYear() : monthsInYear(year);
    if (!d || month < 1 || month > maxMonth)
        return QString();

    return d->standaloneMonthName(locale, month, year, format);
}

/*!
    Returns a suitably localised name for a day of the week.

    The days of the week are numbered from 1 for Monday through 7 for
    Sunday. Some calendars may support higher numbers for other days
    (e.g. intercallary days, that are not part of any week). Returns an empty
    string if the \a day number is unrecognized.

    The name is returned in the form that would normally be used in a full date,
    in the specified \a locale; the \a format determines how fully it shall be
    expressed (i.e. to what extent it is abbreviated).

    \sa standaloneWeekDayName(), dayOfWeek()
*/
QString QCalendar::weekDayName(const QLocale &locale, int day,
                               QLocale::FormatType format) const
{
    return d ? d->weekDayName(locale, day, format) : QString();
}

/*!
    Returns a suitably localised standalone name for a day of the week.

    The days of the week are numbered from 1 for Monday through 7 for
    Sunday. Some calendars may support higher numbers for other days
    (e.g. intercallary days, that are not part of any week). Returns an empty
    string if the \a day number is unrecognized.

    The name is returned in the form that would be used in isolation (for
    example as a column heading in a calendar's tabular display of a month with
    successive weeks as rows) in the specified \a locale; the \a format
    determines how fully it shall be expressed (i.e. to what extent it is
    abbreviated).

    \sa weekDayName(), dayOfWeek()
*/
QString QCalendar::standaloneWeekDayName(const QLocale &locale, int day,
                                         QLocale::FormatType format) const
{
    return d ? d->standaloneWeekDayName(locale, day, format) : QString();
}

/*!
    Returns a string representing a given date, time or date-time.

    If \a datetime is valid, it is represented and format specifiers for both
    date and time fields are recognized; otherwise, if \a dateOnly is valid, it
    is represented and only format specifiers for date fields are recognized;
    finally, if \a timeOnly is valid, it is represented and only format
    specifiers for time fields are recognized. If none of these is valid, an
    empty string is returned.

    See QDate::toString and QTime::toString() for the supported field
    specifiers.  Characters in \a format that are recognized as field specifiers
    are replaced by text representing appropriate data from the date and/or time
    being represented. The texts to represent them may depend on the \a locale
    specified. Other charagers in \a format are copied verbatim into the
    returned string.

    \sa monthName(), weekDayName(), QDate::toString(), QTime::toString()
*/
QString QCalendar::dateTimeToString(QStringView format, const QDateTime &datetime,
                                    QDate dateOnly, QTime timeOnly,
                                    const QLocale &locale) const
{
    return d ? d->dateTimeToString(format, datetime, dateOnly, timeOnly, locale) : QString();
}

/*!
    Returns a list of names of the available calendar systems.

    These may be supplied by plugins or other code linked into an application,
    in addition to the ones provided by Qt, some of which are controlled by
    features.
*/
QStringList QCalendar::availableCalendars()
{
    return QCalendarBackend::availableCalendars();
}

QT_END_NAMESPACE
