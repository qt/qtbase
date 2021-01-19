/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QCALENDAR_BACKEND_P_H
#define QCALENDAR_BACKEND_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of calendar implementations.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobjectdefs.h>
#include <QtCore/qcalendar.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

// Locale-related parts, mostly handled in ../text/qlocale.cpp
struct QLocaleDataEntry {
    quint16 index, size;
};

struct QCalendarLocale {
    quint16 m_language_id, m_script_id, m_country_id;
    // Month name indexes:
    QLocaleDataEntry m_standalone_short;
    QLocaleDataEntry m_standalone_long;
    QLocaleDataEntry m_standalone_narrow;
    QLocaleDataEntry m_short;
    QLocaleDataEntry m_long;
    QLocaleDataEntry m_narrow;
};

// Partial implementation, of methods with common forms, in qcalendar.cpp
class Q_CORE_EXPORT QCalendarBackend
{
    friend class QCalendar;
public:
    virtual ~QCalendarBackend();
    virtual QString name() const = 0;
    virtual QCalendar::System calendarSystem() const;
    // Date queries:
    virtual int daysInMonth(int month, int year = QCalendar::Unspecified) const = 0;
    virtual int daysInYear(int year) const;
    virtual int monthsInYear(int year) const;
    virtual bool isDateValid(int year, int month, int day) const;
    // Properties of the calendar:
    virtual bool isLeapYear(int year) const = 0;
    virtual bool isLunar() const = 0;
    virtual bool isLuniSolar() const = 0;
    virtual bool isSolar() const = 0;
    virtual bool isProleptic() const;
    virtual bool hasYearZero() const;
    virtual int maximumDaysInMonth() const;
    virtual int minimumDaysInMonth() const;
    virtual int maximumMonthsInYear() const;
    // Julian Day conversions:
    virtual bool dateToJulianDay(int year, int month, int day, qint64 *jd) const = 0;
    virtual QCalendar::YearMonthDay julianDayToDate(qint64 jd) const = 0;
    // Day of week and week numbering:
    virtual int dayOfWeek(qint64 jd) const;

    // Names of months and week-days (implemented in qlocale.cpp):
    virtual QString monthName(const QLocale &locale, int month, int year,
                              QLocale::FormatType format) const;
    virtual QString standaloneMonthName(const QLocale &locale, int month, int year,
                                        QLocale::FormatType format) const;
    virtual QString weekDayName(const QLocale &locale, int day,
                                QLocale::FormatType format) const;
    virtual QString standaloneWeekDayName(const QLocale &locale, int day,
                                          QLocale::FormatType format) const;

    // Formatting of date-times (implemented in qlocale.cpp):
    virtual QString dateTimeToString(QStringView format, const QDateTime &datetime,
                                     const QDate &dateOnly, const QTime &timeOnly,
                                     const QLocale &locale) const;

    // Calendar enumeration by name:
    static QStringList availableCalendars();

protected:
    QCalendarBackend(const QString &name, QCalendar::System system = QCalendar::System::User);

    // Locale support:
    virtual const QCalendarLocale *localeMonthIndexData() const = 0;
    virtual const ushort *localeMonthData() const = 0;

    bool registerAlias(const QString &name);

private:
    // QCalendar's access to its registry:
    static const QCalendarBackend *fromName(QStringView name);
    static const QCalendarBackend *fromName(QLatin1String name);
    // QCalendar's access to singletons:
    static const QCalendarBackend *fromEnum(QCalendar::System system);
};

QT_END_NAMESPACE

#endif // QCALENDAR_BACKEND_P_H
