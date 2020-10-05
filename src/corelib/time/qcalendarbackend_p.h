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
#include <QtCore/private/qlocale_p.h>

QT_BEGIN_NAMESPACE

// Locale-related parts, mostly handled in ../text/qlocale.cpp

struct QCalendarLocale {
    quint16 m_language_id, m_script_id, m_country_id;

#define rangeGetter(name) \
    QLocaleData::DataRange name() const { return { m_ ## name ## _idx, m_ ## name ## _size }; }

    rangeGetter(longMonthStandalone) rangeGetter(longMonth)
    rangeGetter(shortMonthStandalone) rangeGetter(shortMonth)
    rangeGetter(narrowMonthStandalone) rangeGetter(narrowMonth)
#undef rangeGetter

    // Month name indexes:
    quint16 m_longMonthStandalone_idx, m_longMonth_idx;
    quint16 m_shortMonthStandalone_idx, m_shortMonth_idx;
    quint16 m_narrowMonthStandalone_idx, m_narrowMonth_idx;

    // Twelve long month names (separated by commas) can add up to more than 256
    // QChars - e.g. kde_TZ gets to 264.
    quint16 m_longMonthStandalone_size, m_longMonth_size;
    quint8 m_shortMonthStandalone_size, m_shortMonth_size;
    quint8 m_narrowMonthStandalone_size, m_narrowMonth_size;
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
                                     QDate dateOnly, QTime timeOnly,
                                     const QLocale &locale) const;

    // Calendar enumeration by name:
    static QStringList availableCalendars();

protected:
    QCalendarBackend(const QString &name, QCalendar::System system = QCalendar::System::User);

    // Locale support:
    virtual const QCalendarLocale *localeMonthIndexData() const = 0;
    virtual const char16_t *localeMonthData() const = 0;

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
