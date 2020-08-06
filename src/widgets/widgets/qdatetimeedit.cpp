/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include <private/qapplication_p.h>
#include <private/qdatetimeedit_p.h>
#include <qabstractspinbox.h>
#include <qapplication.h>
#include <qdatetimeedit.h>
#include <qdesktopwidget.h>
#include <private/qdesktopwidget_p.h>
#include <qdebug.h>
#include <qevent.h>
#include <qlineedit.h>
#include <private/qlineedit_p.h>
#include <qlocale.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qset.h>
#include <qstyle.h>

#include <algorithm>

//#define QDATETIMEEDIT_QDTEDEBUG
#ifdef QDATETIMEEDIT_QDTEDEBUG
#  define QDTEDEBUG qDebug() << QString::fromLatin1("%1:%2").arg(__FILE__).arg(__LINE__)
#  define QDTEDEBUGN qDebug
#else
#  define QDTEDEBUG if (false) qDebug()
#  define QDTEDEBUGN if (false) qDebug
#endif

QT_BEGIN_NAMESPACE

// --- QDateTimeEdit ---

/*!
  \class QDateTimeEdit
  \brief The QDateTimeEdit class provides a widget for editing dates and times.

  \ingroup basicwidgets
  \inmodule QtWidgets

  \image windows-datetimeedit.png

  QDateTimeEdit allows the user to edit dates by using the keyboard or
  the arrow keys to increase and decrease date and time values. The
  arrow keys can be used to move from section to section within the
  QDateTimeEdit box. Dates and times appear in accordance with the
  format set; see setDisplayFormat().

  \snippet code/src_gui_widgets_qdatetimeedit.cpp 0

  Here we've created a new QDateTimeEdit object initialized with
  today's date, and restricted the valid date range to today plus or
  minus 365 days. We've set the order to month, day, year.

  The range of valid values for a QDateTimeEdit is controlled by the properties
  \l minimumDateTime, \l maximumDateTime, and their respective date and time
  components. By default, any date-time from the start of 100 CE to the end of
  9999 CE is valid.

  \section1 Using a Pop-up Calendar Widget

  QDateTimeEdit can be configured to allow a QCalendarWidget to be used
  to select dates. This is enabled by setting the calendarPopup property.
  Additionally, you can supply a custom calendar widget for use as the
  calendar pop-up by calling the setCalendarWidget() function. The existing
  calendar widget can be retrieved with calendarWidget().

  \section1 Keyboard Tracking

  When \l{QAbstractSpinBox::keyboardTracking}{keyboard tracking} is enabled
  (the default), every keystroke of editing a field triggers signals for value
  changes.

  When the allowed \l{QDateTimeEdit::setDateTimeRange}{range} is narrower than
  some time interval whose end it straddles, keyboard tracking prevents the
  user editing the date or time to access the later part of the interval. For
  example, for a range from 29.04.2020 to 02.05.2020 and an initial date of
  30.04.2020, the user can change neither the month (May 30th is outside the
  range) nor the day (April 2nd is outside the range).

  When keyboard tracking is disabled, changes are only signalled when focus
  leaves the text field after edits have modified the content. This allows the
  user to edit via an invalid date-time to reach a valid one.

  \sa QDateEdit, QTimeEdit, QDate, QTime
*/

/*!
  \enum QDateTimeEdit::Section

  \value NoSection
  \value AmPmSection
  \value MSecSection
  \value SecondSection
  \value MinuteSection
  \value HourSection
  \value DaySection
  \value MonthSection
  \value YearSection
  \omitvalue DateSections_Mask
  \omitvalue TimeSections_Mask
*/

/*!
  \fn void QDateTimeEdit::dateTimeChanged(const QDateTime &datetime)

  This signal is emitted whenever the date or time is changed. The
  new date and time is passed in \a datetime.

  \sa {Keyboard Tracking}
*/

/*!
  \fn void QDateTimeEdit::timeChanged(const QTime &time)

  This signal is emitted whenever the time is changed. The new time
  is passed in \a time.

  \sa {Keyboard Tracking}
*/

/*!
  \fn void QDateTimeEdit::dateChanged(const QDate &date)

  This signal is emitted whenever the date is changed. The new date
  is passed in \a date.

  \sa {Keyboard Tracking}
*/


/*!
  Constructs an empty date time editor with a \a parent.
*/

QDateTimeEdit::QDateTimeEdit(QWidget *parent)
    : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
    Q_D(QDateTimeEdit);
    d->init(QDATETIMEEDIT_DATE_INITIAL.startOfDay());
}

/*!
  Constructs an empty date time editor with a \a parent. The value
  is set to \a datetime.
*/

QDateTimeEdit::QDateTimeEdit(const QDateTime &datetime, QWidget *parent)
    : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
    Q_D(QDateTimeEdit);
    d->init(datetime.isValid() ? datetime : QDATETIMEEDIT_DATE_INITIAL.startOfDay());
}

/*!
  \fn QDateTimeEdit::QDateTimeEdit(const QDate &date, QWidget *parent)

  Constructs an empty date time editor with a \a parent.
  The value is set to \a date.
*/

QDateTimeEdit::QDateTimeEdit(const QDate &date, QWidget *parent)
    : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
    Q_D(QDateTimeEdit);
    d->init(date.isValid() ? date : QDATETIMEEDIT_DATE_INITIAL);
}

/*!
  \fn QDateTimeEdit::QDateTimeEdit(const QTime &time, QWidget *parent)

  Constructs an empty date time editor with a \a parent.
  The value is set to \a time.
*/

QDateTimeEdit::QDateTimeEdit(const QTime &time, QWidget *parent)
    : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
    Q_D(QDateTimeEdit);
    d->init(time.isValid() ? time : QDATETIMEEDIT_TIME_MIN);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
/*!
  \internal
*/
QDateTimeEdit::QDateTimeEdit(const QVariant &var, QVariant::Type parserType, QWidget *parent)
    : QDateTimeEdit(var, QMetaType::Type(parserType), parent)
{ }
/*!
  \internal
*/
#endif

QDateTimeEdit::QDateTimeEdit(const QVariant &var, QMetaType::Type parserType, QWidget *parent)
    : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
    Q_D(QDateTimeEdit);
    d->parserType = parserType;
    d->init(var);
}

/*!
    Destructor.
*/
QDateTimeEdit::~QDateTimeEdit()
{
}

/*!
  \property QDateTimeEdit::dateTime
  \brief The QDateTime that is set in the QDateTimeEdit.

  When setting this property the timespec of the QDateTimeEdit remains the same
  and the timespec of the new QDateTime is ignored.

  By default, this property is set to the start of 2000 CE. It can only be set
  to a valid QDateTime value. If any operation causes this property to have an
  invalid date-time as value, it is reset to the value of the \l minimumDateTime
  property.

  If the QDateTimeEdit has no date fields, setting this property sets the
  widget's date-range to start and end on the date of the new value of this
  property.

  \sa date, time, minimumDateTime, maximumDateTime
*/

QDateTime QDateTimeEdit::dateTime() const
{
    Q_D(const QDateTimeEdit);
    return d->value.toDateTime();
}

void QDateTimeEdit::setDateTime(const QDateTime &datetime)
{
    Q_D(QDateTimeEdit);
    if (datetime.isValid()) {
        d->clearCache();
        const QDate date = datetime.date();
        if (!(d->sections & DateSections_Mask))
            setDateRange(date, date);
        d->setValue(QDateTime(date, datetime.time(), d->spec), EmitIfChanged);
    }
}

/*!
  \property QDateTimeEdit::date
  \brief The QDate that is set in the widget.

  By default, this property contains a date that refers to January 1, 2000.

  \sa time, dateTime
*/

/*!
    Returns the date of the date time edit.
*/
QDate QDateTimeEdit::date() const
{
    Q_D(const QDateTimeEdit);
    return d->value.toDate();
}

void QDateTimeEdit::setDate(const QDate &date)
{
    Q_D(QDateTimeEdit);
    if (date.isValid()) {
        if (!(d->sections & DateSections_Mask))
            setDateRange(date, date);

        d->clearCache();
        QDateTime when(date, d->value.toTime(), d->spec);
        // The specified time might not exist on the specified day,
        // i.e. the time is in the gap a spring-forward jumps over.
        if (!when.isValid())
            when = QDateTime::fromMSecsSinceEpoch(when.toMSecsSinceEpoch(), d->spec);
        Q_ASSERT(when.isValid());
        d->setValue(when, EmitIfChanged);
        d->updateTimeSpec();
    }
}

/*!
  \property QDateTimeEdit::time
  \brief The QTime that is set in the widget.

  By default, this property contains a time of 00:00:00 and 0 milliseconds.

  \sa date, dateTime
*/

/*!
    Returns the time of the date time edit.
*/
QTime QDateTimeEdit::time() const
{
    Q_D(const QDateTimeEdit);
    return d->value.toTime();
}

void QDateTimeEdit::setTime(const QTime &time)
{
    Q_D(QDateTimeEdit);
    if (time.isValid()) {
        d->clearCache();
        d->setValue(QDateTime(d->value.toDate(), time, d->spec), EmitIfChanged);
    }
}


QCalendar QDateTimeEdit::calendar() const
{
    Q_D(const QDateTimeEdit);
    return d->calendar;
}

void QDateTimeEdit::setCalendar(QCalendar calendar)
{
    Q_D(QDateTimeEdit);
    // Set invalid date time to prevent runtime crashes on calendar change
    QDateTime previousValue = d->value.toDateTime();
    setDateTime(QDateTime());
    d->setCalendar(calendar);
    setDateTime(previousValue);
}

/*!
  \since 4.4
  \property QDateTimeEdit::minimumDateTime

  \brief The minimum datetime of the date time edit.

  Changing this property implicitly updates the \l minimumDate and \l
  minimumTime properties to the date and time parts of this property,
  respectively. When setting this property, the \l maximumDateTime is adjusted,
  if necessary, to ensure that the range remains valid. Otherwise, changing this
  property preserves the \l minimumDateTime property.

  This property can only be set to a valid QDateTime value. The earliest
  date-time that setMinimumDateTime() accepts is the start of 100 CE. The
  property's default is the start of September 14, 1752 CE. This default can be
  restored with clearMinimumDateTime().

  \sa maximumDateTime, minimumTime, minimumDate, setDateTimeRange(),
      QDateTime::isValid(), {Keyboard Tracking}
*/

QDateTime QDateTimeEdit::minimumDateTime() const
{
    Q_D(const QDateTimeEdit);
    return d->minimum.toDateTime();
}

void QDateTimeEdit::clearMinimumDateTime()
{
    setMinimumDateTime(QDATETIMEEDIT_COMPAT_DATE_MIN.startOfDay());
}

void QDateTimeEdit::setMinimumDateTime(const QDateTime &dt)
{
    Q_D(QDateTimeEdit);
    if (dt.isValid() && dt.date() >= QDATETIMEEDIT_DATE_MIN) {
        const QDateTime m = dt.toTimeSpec(d->spec);
        const QDateTime max = d->maximum.toDateTime();
        d->setRange(m, (max > m ? max : m));
    }
}

/*!
  \since 4.4
  \property QDateTimeEdit::maximumDateTime

  \brief The maximum datetime of the date time edit.

  Changing this property implicitly updates the \l maximumDate and \l
  maximumTime properties to the date and time parts of this property,
  respectively. When setting this property, the \l minimumDateTime is adjusted,
  if necessary, to ensure that the range remains valid. Otherwise, changing this
  property preserves the \l minimumDateTime property.

  This property can only be set to a valid QDateTime value. The latest
  date-time that setMaximumDateTime() accepts is the end of 9999 CE. This is the
  default for this property. This default can be restored with
  clearMaximumDateTime().

  \sa minimumDateTime, maximumTime, maximumDate(), setDateTimeRange(),
      QDateTime::isValid(), {Keyboard Tracking}
*/

QDateTime QDateTimeEdit::maximumDateTime() const
{
    Q_D(const QDateTimeEdit);
    return d->maximum.toDateTime();
}

void QDateTimeEdit::clearMaximumDateTime()
{
    setMaximumDateTime(QDATETIMEEDIT_DATE_MAX.endOfDay());
}

void QDateTimeEdit::setMaximumDateTime(const QDateTime &dt)
{
    Q_D(QDateTimeEdit);
    if (dt.isValid() && dt.date() <= QDATETIMEEDIT_DATE_MAX) {
        const QDateTime m = dt.toTimeSpec(d->spec);
        const QDateTime min = d->minimum.toDateTime();
        d->setRange((min < m ? min : m), m);
    }
}

/*!
  \since 4.4
  \brief Set the range of allowed date-times for the date time edit.

  This convenience function sets the \l minimumDateTime and \l maximumDateTime
  properties.

  \snippet code/src_gui_widgets_qdatetimeedit.cpp 1

  is analogous to:

  \snippet code/src_gui_widgets_qdatetimeedit.cpp 2

  If either \a min or \a max is invalid, this function does nothing. If \a max
  is less than \a min, \a min is used also as \a max.

  If the range is narrower then a time interval whose end it spans, for example
  a week that spans the end of a month, users can only edit the date-time to one
  in the later part of the range if keyboard-tracking is disabled.

  \sa minimumDateTime, maximumDateTime, setDateRange(), setTimeRange(),
      QDateTime::isValid(), {Keyboard Tracking}
*/

void QDateTimeEdit::setDateTimeRange(const QDateTime &min, const QDateTime &max)
{
    Q_D(QDateTimeEdit);
    // FIXME: does none of the range checks applied to setMin/setMax methods !
    const QDateTime minimum = min.toTimeSpec(d->spec);
    const QDateTime maximum = (min > max ? minimum : max.toTimeSpec(d->spec));
    d->setRange(minimum, maximum);
}

/*!
  \property QDateTimeEdit::minimumDate

  \brief The minimum date of the date time edit.

  Changing this property updates the date of the \l minimumDateTime property
  while preserving the \l minimumTime property. When setting this property,
  the \l maximumDate is adjusted, if necessary, to ensure that the range remains
  valid. When this happens, the \l maximumTime property is also adjusted if it
  is less than the \l minimumTime property. Otherwise, changes to this property
  preserve the \l maximumDateTime property.

  This property can only be set to a valid QDate object describing a date on
  which the current \l minimumTime property makes a valid QDateTime object. The
  earliest date that setMinimumDate() accepts is the start of 100 CE. The
  default for this property is September 14, 1752 CE. This default can be
  restored with clearMinimumDateTime().

  \sa maximumDate, minimumTime, minimumDateTime, setDateRange(),
      QDate::isValid(), {Keyboard Tracking}
*/

QDate QDateTimeEdit::minimumDate() const
{
    Q_D(const QDateTimeEdit);
    return d->minimum.toDate();
}

void QDateTimeEdit::setMinimumDate(const QDate &min)
{
    Q_D(QDateTimeEdit);
    if (min.isValid() && min >= QDATETIMEEDIT_DATE_MIN) {
        setMinimumDateTime(QDateTime(min, d->minimum.toTime(), d->spec));
    }
}

void QDateTimeEdit::clearMinimumDate()
{
    setMinimumDate(QDATETIMEEDIT_COMPAT_DATE_MIN);
}

/*!
  \property QDateTimeEdit::maximumDate

  \brief The maximum date of the date time edit.

  Changing this property updates the date of the \l maximumDateTime property
  while preserving the \l maximumTime property. When setting this property, the
  \l minimumDate is adjusted, if necessary, to ensure that the range remains
  valid. When this happens, the \l minimumTime property is also adjusted if it
  is greater than the \l maximumTime property. Otherwise, changes to this
  property preserve the \l minimumDateTime property.

  This property can only be set to a valid QDate object describing a date on
  which the current \l maximumTime property makes a valid QDateTime object. The
  latest date that setMaximumDate() accepts is the end of 9999 CE. This is the
  default for this property. This default can be restored with
  clearMaximumDateTime().

  \sa minimumDate, maximumTime, maximumDateTime, setDateRange(),
      QDate::isValid(), {Keyboard Tracking}
*/

QDate QDateTimeEdit::maximumDate() const
{
    Q_D(const QDateTimeEdit);
    return d->maximum.toDate();
}

void QDateTimeEdit::setMaximumDate(const QDate &max)
{
    Q_D(QDateTimeEdit);
    if (max.isValid())
        setMaximumDateTime(QDateTime(max, d->maximum.toTime(), d->spec));
}

void QDateTimeEdit::clearMaximumDate()
{
    setMaximumDate(QDATETIMEEDIT_DATE_MAX);
}

/*!
  \property QDateTimeEdit::minimumTime

  \brief The minimum time of the date time edit.

  Changing this property updates the time of the \l minimumDateTime property
  while preserving the \l minimumDate and \l maximumDate properties. If those
  date properties coincide, when setting this property, the \l maximumTime
  property is adjusted, if necessary, to ensure that the range remains
  valid. Otherwise, changing this property preserves the \l maximumDateTime
  property.

  This property can be set to any valid QTime value. By default, this property
  contains a time of 00:00:00 and 0 milliseconds. This default can be restored
  with clearMinimumTime().

  \sa maximumTime, minimumDate, minimumDateTime, setTimeRange(),
      QTime::isValid(), {Keyboard Tracking}
*/

QTime QDateTimeEdit::minimumTime() const
{
    Q_D(const QDateTimeEdit);
    return d->minimum.toTime();
}

void QDateTimeEdit::setMinimumTime(const QTime &min)
{
    Q_D(QDateTimeEdit);
    if (min.isValid()) {
        const QDateTime m(d->minimum.toDate(), min, d->spec);
        setMinimumDateTime(m);
    }
}

void QDateTimeEdit::clearMinimumTime()
{
    setMinimumTime(QDATETIMEEDIT_TIME_MIN);
}

/*!
  \property QDateTimeEdit::maximumTime

  \brief The maximum time of the date time edit.

  Changing this property updates the time of the \l maximumDateTime property
  while preserving the \l minimumDate and \l maximumDate properties. If those
  date properties coincide, when setting this property, the \l minimumTime
  property is adjusted, if necessary, to ensure that the range remains
  valid. Otherwise, changing this property preserves the \l minimumDateTime
  property.

  This property can be set to any valid QTime value. By default, this property
  contains a time of 23:59:59 and 999 milliseconds. This default can be restored
  with clearMaximumTime().

  \sa minimumTime, maximumDate, maximumDateTime, setTimeRange(),
      QTime::isValid(), {Keyboard Tracking}
*/
QTime QDateTimeEdit::maximumTime() const
{
    Q_D(const QDateTimeEdit);
    return d->maximum.toTime();
}

void QDateTimeEdit::setMaximumTime(const QTime &max)
{
    Q_D(QDateTimeEdit);
    if (max.isValid()) {
        const QDateTime m(d->maximum.toDate(), max, d->spec);
        setMaximumDateTime(m);
    }
}

void QDateTimeEdit::clearMaximumTime()
{
    setMaximumTime(QDATETIMEEDIT_TIME_MAX);
}

/*!
  \brief Set the range of allowed dates for the date time edit.

  This convenience function sets the \l minimumDate and \l maximumDate
  properties.

  \snippet code/src_gui_widgets_qdatetimeedit.cpp 3

  is analogous to:

  \snippet code/src_gui_widgets_qdatetimeedit.cpp 4

  If either \a min or \a max is invalid, this function does nothing. This
  function preserves the \l minimumTime property. If \a max is less than \a min,
  the new maximumDateTime property shall be the new minimumDateTime property. If
  \a max is equal to \a min and the \l maximumTime property was less then the \l
  minimumTime property, the \l maximumTime property is set to the \l minimumTime
  property. Otherwise, this preserves the \l maximumTime property.

  If the range is narrower then a time interval whose end it spans, for example
  a week that spans the end of a month, users can only edit the date to one in
  the later part of the range if keyboard-tracking is disabled.

  \sa minimumDate, maximumDate, setDateTimeRange(), QDate::isValid(), {Keyboard Tracking}
*/

void QDateTimeEdit::setDateRange(const QDate &min, const QDate &max)
{
    Q_D(QDateTimeEdit);
    if (min.isValid() && max.isValid()) {
        setDateTimeRange(QDateTime(min, d->minimum.toTime(), d->spec),
                         QDateTime(max, d->maximum.toTime(), d->spec));
    }
}

/*!
  \brief Set the range of allowed times for the date time edit.

  This convenience function sets the \l minimumTime and \l maximumTime
  properties.

  Note that these only constrain the date time edit's value on,
  respectively, the \l minimumDate and \l maximumDate. When these date
  properties do not coincide, times after \a max are allowed on dates
  before \l maximumDate and times before \a min are allowed on dates
  after \l minimumDate.

  \snippet code/src_gui_widgets_qdatetimeedit.cpp 5

  is analogous to:

  \snippet code/src_gui_widgets_qdatetimeedit.cpp 6

  If either \a min or \a max is invalid, this function does nothing. This
  function preserves the \l minimumDate and \l maximumDate properties. If those
  properties coincide and \a max is less than \a min, \a min is used as \a max.

  If the range is narrower then a time interval whose end it spans, for example
  the interval from ten to an hour to ten past the same hour, users can only
  edit the time to one in the later part of the range if keyboard-tracking is
  disabled.

  \sa minimumTime, maximumTime, setDateTimeRange(), QTime::isValid(), {Keyboard Tracking}
*/

void QDateTimeEdit::setTimeRange(const QTime &min, const QTime &max)
{
    Q_D(QDateTimeEdit);
    if (min.isValid() && max.isValid()) {
        setDateTimeRange(QDateTime(d->minimum.toDate(), min, d->spec),
                         QDateTime(d->maximum.toDate(), max, d->spec));
    }
}

/*!
  \property QDateTimeEdit::displayedSections

  \brief The currently displayed fields of the date time edit.

  Returns a bit set of the displayed sections for this format.

  \sa setDisplayFormat(), displayFormat()
*/

QDateTimeEdit::Sections QDateTimeEdit::displayedSections() const
{
    Q_D(const QDateTimeEdit);
    return d->sections;
}

/*!
  \property QDateTimeEdit::currentSection

  \brief The current section of the spinbox.
*/

QDateTimeEdit::Section QDateTimeEdit::currentSection() const
{
    Q_D(const QDateTimeEdit);
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplicationPrivate::keypadNavigationEnabled() && d->focusOnButton)
        return NoSection;
#endif
    return QDateTimeEditPrivate::convertToPublic(d->sectionType(d->currentSectionIndex));
}

void QDateTimeEdit::setCurrentSection(Section section)
{
    Q_D(QDateTimeEdit);
    if (section == NoSection || !(section & d->sections))
        return;

    d->updateCache(d->value, d->displayText());
    const int size = d->sectionNodes.size();
    int index = d->currentSectionIndex + 1;
    for (int i=0; i<2; ++i) {
        while (index < size) {
            if (QDateTimeEditPrivate::convertToPublic(d->sectionType(index)) == section) {
                d->edit->setCursorPosition(d->sectionPos(index));
                QDTEDEBUG << d->sectionPos(index);
                return;
            }
            ++index;
        }
        index = 0;
    }
}

/*!
  \since 4.3

  Returns the Section at \a index.

  If the format is 'yyyy/MM/dd', sectionAt(0) returns YearSection,
  sectionAt(1) returns MonthSection, and sectionAt(2) returns
  YearSection,
*/

QDateTimeEdit::Section QDateTimeEdit::sectionAt(int index) const
{
    Q_D(const QDateTimeEdit);
    if (index < 0 || index >= d->sectionNodes.size())
        return NoSection;
    return QDateTimeEditPrivate::convertToPublic(d->sectionType(index));
}

/*!
  \since 4.3

  \property QDateTimeEdit::sectionCount

  \brief The number of sections displayed.
  If the format is 'yyyy/yy/yyyy', sectionCount returns 3
*/

int QDateTimeEdit::sectionCount() const
{
    Q_D(const QDateTimeEdit);
    return d->sectionNodes.size();
}


/*!
  \since 4.3

  \property QDateTimeEdit::currentSectionIndex

  \brief The current section index of the spinbox.

  If the format is 'yyyy/MM/dd', the displayText is '2001/05/21', and
  the cursorPosition is 5, currentSectionIndex returns 1. If the
  cursorPosition is 3, currentSectionIndex is 0, and so on.

  \sa setCurrentSection(), currentSection()
*/

int QDateTimeEdit::currentSectionIndex() const
{
    Q_D(const QDateTimeEdit);
    return d->currentSectionIndex;
}

void QDateTimeEdit::setCurrentSectionIndex(int index)
{
    Q_D(QDateTimeEdit);
    if (index < 0 || index >= d->sectionNodes.size())
        return;
    d->edit->setCursorPosition(d->sectionPos(index));
}

/*!
  \since 4.4

  \brief Returns the calendar widget for the editor if calendarPopup is
  set to true and (sections() & DateSections_Mask) != 0.

  This function creates and returns a calendar widget if none has been set.
*/


QCalendarWidget *QDateTimeEdit::calendarWidget() const
{
    Q_D(const QDateTimeEdit);
    if (!d->calendarPopup || !(d->sections & QDateTimeParser::DateSectionMask))
        return nullptr;
    if (!d->monthCalendar) {
        const_cast<QDateTimeEditPrivate*>(d)->initCalendarPopup();
    }
    return d->monthCalendar->calendarWidget();
}

/*!
  \since 4.4

  Sets the given \a calendarWidget as the widget to be used for the calendar
  pop-up. The editor does not automatically take ownership of the calendar widget.

  \note calendarPopup must be set to true before setting the calendar widget.
  \sa calendarPopup
*/
void QDateTimeEdit::setCalendarWidget(QCalendarWidget *calendarWidget)
{
    Q_D(QDateTimeEdit);
    if (Q_UNLIKELY(!calendarWidget)) {
        qWarning("QDateTimeEdit::setCalendarWidget: Cannot set a null calendar widget");
        return;
    }

    if (Q_UNLIKELY(!d->calendarPopup)) {
        qWarning("QDateTimeEdit::setCalendarWidget: calendarPopup is set to false");
        return;
    }

    if (Q_UNLIKELY(!(d->display & QDateTimeParser::DateSectionMask))) {
        qWarning("QDateTimeEdit::setCalendarWidget: no date sections specified");
        return;
    }
    d->initCalendarPopup(calendarWidget);
}


/*!
  \since 4.2

  Selects \a section. If \a section doesn't exist in the currently
  displayed sections, this function does nothing. If \a section is
  NoSection, this function will unselect all text in the editor.
  Otherwise, this function will move the cursor and the current section
  to the selected section.

  \sa currentSection()
*/

void QDateTimeEdit::setSelectedSection(Section section)
{
    Q_D(QDateTimeEdit);
    if (section == NoSection) {
        d->edit->setSelection(d->edit->cursorPosition(), 0);
    } else if (section & d->sections) {
        if (currentSection() != section)
            setCurrentSection(section);
        d->setSelected(d->currentSectionIndex);
    }
}



/*!
  \fn QString QDateTimeEdit::sectionText(Section section) const

  Returns the text from the given \a section.

  \sa currentSection()
*/

QString QDateTimeEdit::sectionText(Section section) const
{
    Q_D(const QDateTimeEdit);
    if (section == QDateTimeEdit::NoSection || !(section & d->sections)) {
        return QString();
    }

    d->updateCache(d->value, d->displayText());
    const int sectionIndex = d->absoluteIndex(section, 0);
    return d->sectionText(sectionIndex);
}

/*!
  \property QDateTimeEdit::displayFormat

  \brief The format used to display the time/date of the date time edit.

  This format is described in QDateTime::toString() and QDateTime::fromString()

  Example format strings (assuming that the date is 2nd of July 1969):

  \table
  \header \li Format \li Result
  \row \li dd.MM.yyyy \li 02.07.1969
  \row \li MMM d yy \li Jul 2 69
  \row \li MMMM d yy \li July 2 69
  \endtable

  Note that if you specify a two digit year, it will be interpreted
  to be in the century in which the date time edit was initialized.
  The default century is the 21st (2000-2099).

  If you specify an invalid format the format will not be set.

  \sa QDateTime::toString(), displayedSections()
*/

QString QDateTimeEdit::displayFormat() const
{
    Q_D(const QDateTimeEdit);
    return isRightToLeft() ? d->unreversedFormat : d->displayFormat;
}

void QDateTimeEdit::setDisplayFormat(const QString &format)
{
    Q_D(QDateTimeEdit);
    if (d->parseFormat(format)) {
        d->unreversedFormat.clear();
        if (isRightToLeft()) {
            d->unreversedFormat = format;
            d->displayFormat.clear();
            for (int i=d->sectionNodes.size() - 1; i>=0; --i) {
                d->displayFormat += d->separators.at(i + 1);
                d->displayFormat += d->sectionNode(i).format();
            }
            d->displayFormat += d->separators.at(0);
            std::reverse(d->separators.begin(), d->separators.end());
            std::reverse(d->sectionNodes.begin(), d->sectionNodes.end());
        }

        d->formatExplicitlySet = true;
        d->sections = QDateTimeEditPrivate::convertSections(d->display);
        d->clearCache();

        d->currentSectionIndex = qMin(d->currentSectionIndex, d->sectionNodes.size() - 1);
        const bool timeShown = (d->sections & TimeSections_Mask);
        const bool dateShown = (d->sections & DateSections_Mask);
        Q_ASSERT(dateShown || timeShown);
        if (timeShown && !dateShown) {
            QTime time = d->value.toTime();
            setDateRange(d->value.toDate(), d->value.toDate());
            if (d->minimum.toTime() >= d->maximum.toTime()) {
                setTimeRange(QDATETIMEEDIT_TIME_MIN, QDATETIMEEDIT_TIME_MAX);
                // if the time range became invalid during the adjustment, the time would have been reset
                setTime(time);
            }
        } else if (dateShown && !timeShown) {
            setTimeRange(QDATETIMEEDIT_TIME_MIN, QDATETIMEEDIT_TIME_MAX);
            d->value = QDateTime(d->value.toDate(), QTime(), d->spec);
        }
        d->updateEdit();
        d->_q_editorCursorPositionChanged(-1, 0);
    }
}

/*!
    \property QDateTimeEdit::calendarPopup
    \brief The current calendar pop-up show mode.
    \since 4.2

    The calendar pop-up will be shown upon clicking the arrow button.
    This property is valid only if there is a valid date display format.

    \sa setDisplayFormat()
*/

bool QDateTimeEdit::calendarPopup() const
{
    Q_D(const QDateTimeEdit);
    return d->calendarPopup;
}

void QDateTimeEdit::setCalendarPopup(bool enable)
{
    Q_D(QDateTimeEdit);
    if (enable == d->calendarPopup)
        return;
    setAttribute(Qt::WA_MacShowFocusRect, !enable);
    d->calendarPopup = enable;
#ifdef QT_KEYPAD_NAVIGATION
    if (!enable)
        d->focusOnButton = false;
#endif
    d->updateEditFieldGeometry();
    update();
}

/*!
    \property QDateTimeEdit::timeSpec
    \brief The current timespec used by the date time edit.
    \since 4.4
*/

Qt::TimeSpec QDateTimeEdit::timeSpec() const
{
    Q_D(const QDateTimeEdit);
    return d->spec;
}

void QDateTimeEdit::setTimeSpec(Qt::TimeSpec spec)
{
    Q_D(QDateTimeEdit);
    if (spec != d->spec) {
        d->spec = spec;
        d->updateTimeSpec();
    }
}

/*!
  \reimp
*/

QSize QDateTimeEdit::sizeHint() const
{
    Q_D(const QDateTimeEdit);
    if (d->cachedSizeHint.isEmpty()) {
        ensurePolished();

        const QFontMetrics fm(fontMetrics());
        int h = d->edit->sizeHint().height();
        int w = 0;
        QString s;
        s = d->textFromValue(d->minimum) + QLatin1Char(' ');
        w = qMax<int>(w, fm.horizontalAdvance(s));
        s = d->textFromValue(d->maximum) + QLatin1Char(' ');
        w = qMax<int>(w, fm.horizontalAdvance(s));
        if (d->specialValueText.size()) {
            s = d->specialValueText;
            w = qMax<int>(w, fm.horizontalAdvance(s));
        }
        w += 2; // cursor blinking space

        QSize hint(w, h);

#ifdef Q_OS_MAC
        if (d->calendarPopupEnabled()) {
            QStyleOptionComboBox opt;
            d->cachedSizeHint = style()->sizeFromContents(QStyle::CT_ComboBox, &opt, hint, this);
        } else
#endif
        {
            QStyleOptionSpinBox opt;
            initStyleOption(&opt);
            d->cachedSizeHint = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this)
                                .expandedTo(QApplication::globalStrut());
        }

        d->cachedMinimumSizeHint = d->cachedSizeHint;
        // essentially make minimumSizeHint return the same as sizeHint for datetimeedits
    }
    return d->cachedSizeHint;
}


/*!
  \reimp
*/

bool QDateTimeEdit::event(QEvent *event)
{
    Q_D(QDateTimeEdit);
    switch (event->type()) {
    case QEvent::ApplicationLayoutDirectionChange: {
        const bool was = d->formatExplicitlySet;
        const QString oldFormat = d->displayFormat;
        d->displayFormat.clear();
        setDisplayFormat(oldFormat);
        d->formatExplicitlySet = was;
        break; }
    case QEvent::LocaleChange:
        d->updateEdit();
        break;
    case QEvent::StyleChange:
#ifdef Q_OS_MAC
    case QEvent::MacSizeChange:
#endif
        d->setLayoutItemMargins(QStyle::SE_DateTimeEditLayoutItem);
        break;
    default:
        break;
    }
    return QAbstractSpinBox::event(event);
}

/*!
  \reimp
*/

void QDateTimeEdit::clear()
{
    Q_D(QDateTimeEdit);
    d->clearSection(d->currentSectionIndex);
}
/*!
  \reimp
*/

void QDateTimeEdit::keyPressEvent(QKeyEvent *event)
{
    Q_D(QDateTimeEdit);
    int oldCurrent = d->currentSectionIndex;
    bool select = true;
    bool inserted = false;

    switch (event->key()) {
#ifdef QT_KEYPAD_NAVIGATION
    case Qt::Key_NumberSign:    //shortcut to popup calendar
        if (QApplicationPrivate::keypadNavigationEnabled() && d->calendarPopupEnabled()) {
            d->initCalendarPopup();
            d->positionCalendarPopup();
            d->monthCalendar->show();
            return;
        }
        break;
    case Qt::Key_Select:
        if (QApplicationPrivate::keypadNavigationEnabled()) {
            if (hasEditFocus()) {
                if (d->focusOnButton) {
                    d->initCalendarPopup();
                    d->positionCalendarPopup();
                    d->monthCalendar->show();
                    d->focusOnButton = false;
                    return;
                }
                setEditFocus(false);
                selectAll();
            } else {
                setEditFocus(true);

                //hide cursor
                d->edit->d_func()->setCursorVisible(false);
                d->edit->d_func()->control->setBlinkingCursorEnabled(false);
                d->setSelected(0);
            }
        }
        return;
#endif
    case Qt::Key_Enter:
    case Qt::Key_Return:
        d->interpret(AlwaysEmit);
        d->setSelected(d->currentSectionIndex, true);
        event->ignore();
        emit editingFinished();
        emit d->edit->returnPressed();
        return;
    default:
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplicationPrivate::keypadNavigationEnabled() && !hasEditFocus()
            && !event->text().isEmpty() && event->text().at(0).isLetterOrNumber()) {
            setEditFocus(true);

            //hide cursor
            d->edit->d_func()->setCursorVisible(false);
            d->edit->d_func()->control->setBlinkingCursorEnabled(false);
            d->setSelected(0);
            oldCurrent = 0;
        }
#endif
        if (!d->isSeparatorKey(event)) {
            inserted = select = !event->text().isEmpty() && event->text().at(0).isPrint()
                       && !(event->modifiers() & ~(Qt::ShiftModifier|Qt::KeypadModifier));
            break;
        }
        Q_FALLTHROUGH();
    case Qt::Key_Left:
    case Qt::Key_Right:
        if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
            if (
#ifdef QT_KEYPAD_NAVIGATION
                QApplicationPrivate::keypadNavigationEnabled() && !hasEditFocus()
                || !QApplicationPrivate::keypadNavigationEnabled() &&
#endif
                !(event->modifiers() & Qt::ControlModifier)) {
                select = false;
                break;
            }
        }
        Q_FALLTHROUGH();
    case Qt::Key_Backtab:
    case Qt::Key_Tab: {
        event->accept();
        if (d->specialValue()) {
            d->edit->setSelection(d->edit->cursorPosition(), 0);
            return;
        }
        const bool forward = event->key() != Qt::Key_Left && event->key() != Qt::Key_Backtab
                             && (event->key() != Qt::Key_Tab || !(event->modifiers() & Qt::ShiftModifier));
#ifdef QT_KEYPAD_NAVIGATION
        int newSection = d->nextPrevSection(d->currentSectionIndex, forward);
        if (QApplicationPrivate::keypadNavigationEnabled()) {
            if (d->focusOnButton) {
                newSection = forward ? 0 : d->sectionNodes.size() - 1;
                d->focusOnButton = false;
                update();
            } else if (newSection < 0 && select && d->calendarPopupEnabled()) {
                setSelectedSection(NoSection);
                d->focusOnButton = true;
                update();
                return;
            }
        }
        // only allow date/time sections to be selected.
        if (newSection & ~(QDateTimeParser::TimeSectionMask | QDateTimeParser::DateSectionMask))
            return;
#endif
        //key tab and backtab will be managed thrgout QWidget::event
        if (event->key() != Qt::Key_Backtab && event->key() != Qt::Key_Tab)
            focusNextPrevChild(forward);

        return; }
    }
    QAbstractSpinBox::keyPressEvent(event);
    if (select && !d->edit->hasSelectedText()) {
        if (inserted && d->sectionAt(d->edit->cursorPosition()) == QDateTimeParser::NoSectionIndex) {
            QString str = d->displayText();
            int pos = d->edit->cursorPosition();
            if (validate(str, pos) == QValidator::Acceptable
                && (d->sectionNodes.at(oldCurrent).count != 1
                    || d->sectionMaxSize(oldCurrent) == d->sectionSize(oldCurrent)
                    || d->skipToNextSection(oldCurrent, d->value.toDateTime(), d->sectionText(oldCurrent)))) {
                QDTEDEBUG << "Setting currentsection to"
                          << d->closestSection(d->edit->cursorPosition(), true) << event->key()
                          << oldCurrent << str;
                const int tmp = d->closestSection(d->edit->cursorPosition(), true);
                if (tmp >= 0)
                    d->currentSectionIndex = tmp;
            }
        }
        if (d->currentSectionIndex != oldCurrent) {
            d->setSelected(d->currentSectionIndex);
        }
    }
    if (d->specialValue()) {
        d->edit->setSelection(d->edit->cursorPosition(), 0);
    }
}

/*!
  \reimp
*/

#if QT_CONFIG(wheelevent)
void QDateTimeEdit::wheelEvent(QWheelEvent *event)
{
    QAbstractSpinBox::wheelEvent(event);
}
#endif

/*!
  \reimp
*/

void QDateTimeEdit::focusInEvent(QFocusEvent *event)
{
    Q_D(QDateTimeEdit);
    QAbstractSpinBox::focusInEvent(event);
    QString *frm = nullptr;
    const int oldPos = d->edit->cursorPosition();
    if (!d->formatExplicitlySet) {
        if (d->displayFormat == d->defaultTimeFormat) {
            frm = &d->defaultTimeFormat;
        } else if (d->displayFormat == d->defaultDateFormat) {
            frm = &d->defaultDateFormat;
        } else if (d->displayFormat == d->defaultDateTimeFormat) {
            frm = &d->defaultDateTimeFormat;
        }

        if (frm) {
            d->readLocaleSettings();
            if (d->displayFormat != *frm) {
                setDisplayFormat(*frm);
                d->formatExplicitlySet = false;
                d->edit->setCursorPosition(oldPos);
            }
        }
    }
    const bool oldHasHadFocus = d->hasHadFocus;
    d->hasHadFocus = true;
    bool first = true;
    switch (event->reason()) {
    case Qt::BacktabFocusReason:
        first = false;
        break;
    case Qt::MouseFocusReason:
    case Qt::PopupFocusReason:
        return;
    case Qt::ActiveWindowFocusReason:
        if (oldHasHadFocus)
            return;
    case Qt::ShortcutFocusReason:
    case Qt::TabFocusReason:
    default:
        break;
    }
    if (isRightToLeft())
        first = !first;
    d->updateEdit(); // needed to make it update specialValueText

    d->setSelected(first ? 0 : d->sectionNodes.size() - 1);
}

/*!
  \reimp
*/

bool QDateTimeEdit::focusNextPrevChild(bool next)
{
    Q_D(QDateTimeEdit);
    const int newSection = d->nextPrevSection(d->currentSectionIndex, next);
    switch (d->sectionType(newSection)) {
    case QDateTimeParser::NoSection:
    case QDateTimeParser::FirstSection:
    case QDateTimeParser::LastSection:
        return QAbstractSpinBox::focusNextPrevChild(next);
    default:
        d->edit->deselect();
        d->edit->setCursorPosition(d->sectionPos(newSection));
        QDTEDEBUG << d->sectionPos(newSection);
        d->setSelected(newSection, true);
        return false;
    }
}

/*!
  \reimp
*/

void QDateTimeEdit::stepBy(int steps)
{
    Q_D(QDateTimeEdit);
#ifdef QT_KEYPAD_NAVIGATION
    // with keypad navigation and not editFocus, left right change the date/time by a fixed amount.
    if (QApplicationPrivate::keypadNavigationEnabled() && !hasEditFocus()) {
        // if date based, shift by day.  else shift by 15min
        if (d->sections & DateSections_Mask) {
            setDateTime(dateTime().addDays(steps));
        } else {
            int minutes = time().hour()*60 + time().minute();
            int blocks = minutes/15;
            blocks += steps;
            /* rounding involved */
            if (minutes % 15) {
                if (steps < 0) {
                    blocks += 1; // do one less step;
                }
            }

            minutes = blocks * 15;

            /* need to take wrapping into account */
            if (!d->wrapping) {
                int max_minutes = d->maximum.toTime().hour()*60 + d->maximum.toTime().minute();
                int min_minutes = d->minimum.toTime().hour()*60 + d->minimum.toTime().minute();

                if (minutes >= max_minutes) {
                    setTime(maximumTime());
                    return;
                } else if (minutes <= min_minutes) {
                    setTime(minimumTime());
                    return;
                }
            }
            setTime(QTime(minutes/60, minutes%60));
        }
        return;
    }
#endif
    // don't optimize away steps == 0. This is the only way to select
    // the currentSection in Qt 4.1.x
    if (d->specialValue() && displayedSections() != AmPmSection) {
        for (int i=0; i<d->sectionNodes.size(); ++i) {
            if (d->sectionType(i) != QDateTimeParser::AmPmSection) {
                d->currentSectionIndex = i;
                break;
            }
        }
    }
    d->setValue(d->stepBy(d->currentSectionIndex, steps, false), EmitIfChanged);
    d->updateCache(d->value, d->displayText());

    d->setSelected(d->currentSectionIndex);
    d->updateTimeSpec();
}

/*!
  This virtual function is used by the date time edit whenever it
  needs to display \a dateTime.

  If you reimplement this, you may also need to reimplement validate().

  \sa dateTimeFromText(), validate()
*/
QString QDateTimeEdit::textFromDateTime(const QDateTime &dateTime) const
{
    Q_D(const QDateTimeEdit);
    return locale().toString(dateTime, d->displayFormat, d->calendar);
}


/*!
  Returns an appropriate datetime for the given \a text.

  This virtual function is used by the datetime edit whenever it
  needs to interpret text entered by the user as a value.

  \sa textFromDateTime(), validate()
*/
QDateTime QDateTimeEdit::dateTimeFromText(const QString &text) const
{
    Q_D(const QDateTimeEdit);
    QString copy = text;
    int pos = d->edit->cursorPosition();
    QValidator::State state = QValidator::Acceptable;
    return d->validateAndInterpret(copy, pos, state);
}

/*!
  \reimp
*/

QValidator::State QDateTimeEdit::validate(QString &text, int &pos) const
{
    Q_D(const QDateTimeEdit);
    QValidator::State state;
    d->validateAndInterpret(text, pos, state);
    return state;
}

/*!
  \reimp
*/


void QDateTimeEdit::fixup(QString &input) const
{
    Q_D(const QDateTimeEdit);
    QValidator::State state;
    int copy = d->edit->cursorPosition();

    QDateTime value = d->validateAndInterpret(input, copy, state, true);
    /*
        String was valid, but the datetime still is not; use the time that
        has the same distance from epoch.
        CorrectToPreviousValue correction is handled by QAbstractSpinBox.
    */
    if (!value.isValid() && d->correctionMode == QAbstractSpinBox::CorrectToNearestValue) {
        value = QDateTime::fromMSecsSinceEpoch(value.toMSecsSinceEpoch(), value.timeSpec());
        input = textFromDateTime(value);
    }
}


/*!
  \reimp
*/

QDateTimeEdit::StepEnabled QDateTimeEdit::stepEnabled() const
{
    Q_D(const QDateTimeEdit);
    if (d->readOnly)
        return {};
    if (d->specialValue()) {
        return (d->minimum == d->maximum ? StepEnabled{} : StepEnabled(StepUpEnabled));
    }

    QAbstractSpinBox::StepEnabled ret = { };

#ifdef QT_KEYPAD_NAVIGATION
    if (QApplicationPrivate::keypadNavigationEnabled() && !hasEditFocus()) {
        if (d->wrapping)
            return StepEnabled(StepUpEnabled | StepDownEnabled);
        // 3 cases.  date, time, datetime.  each case look
        // at just the relavant component.
        QVariant max, min, val;
        if (!(d->sections & DateSections_Mask)) {
            // time only, no date
            max = d->maximum.toTime();
            min = d->minimum.toTime();
            val = d->value.toTime();
        } else if (!(d->sections & TimeSections_Mask)) {
            // date only, no time
            max = d->maximum.toDate();
            min = d->minimum.toDate();
            val = d->value.toDate();
        } else {
            // both
            max = d->maximum;
            min = d->minimum;
            val = d->value;
        }
        if (val != min)
            ret |= QAbstractSpinBox::StepDownEnabled;
        if (val != max)
            ret |= QAbstractSpinBox::StepUpEnabled;
        return ret;
    }
#endif
    switch (d->sectionType(d->currentSectionIndex)) {
    case QDateTimeParser::NoSection:
    case QDateTimeParser::FirstSection:
    case QDateTimeParser::LastSection: return { };
    default: break;
    }
    if (d->wrapping)
        return StepEnabled(StepDownEnabled|StepUpEnabled);

    QVariant v = d->stepBy(d->currentSectionIndex, 1, true);
    if (v != d->value) {
        ret |= QAbstractSpinBox::StepUpEnabled;
    }
    v = d->stepBy(d->currentSectionIndex, -1, true);
    if (v != d->value) {
        ret |= QAbstractSpinBox::StepDownEnabled;
    }

    return ret;
}


/*!
  \reimp
*/

void QDateTimeEdit::mousePressEvent(QMouseEvent *event)
{
    Q_D(QDateTimeEdit);
    if (!d->calendarPopupEnabled()) {
        QAbstractSpinBox::mousePressEvent(event);
        return;
    }
    d->updateHoverControl(event->pos());
    if (d->hoverControl == QStyle::SC_ComboBoxArrow) {
        event->accept();
        if (d->readOnly) {
            return;
        }
        d->updateArrow(QStyle::State_Sunken);
        d->initCalendarPopup();
        d->positionCalendarPopup();
        //Show the calendar
        d->monthCalendar->show();
    } else {
        QAbstractSpinBox::mousePressEvent(event);
    }
}

/*!
  \class QTimeEdit
  \brief The QTimeEdit class provides a widget for editing times based on
  the QDateTimeEdit widget.

  \ingroup basicwidgets
  \inmodule QtWidgets

  \image windows-timeedit.png

  Many of the properties and functions provided by QTimeEdit are implemented in
  QDateTimeEdit. These are the relevant properties of this class:

  \list
  \li \l{QDateTimeEdit::time}{time} holds the time displayed by the widget.
  \li \l{QDateTimeEdit::minimumTime}{minimumTime} defines the minimum (earliest) time
     that can be set by the user.
  \li \l{QDateTimeEdit::maximumTime}{maximumTime} defines the maximum (latest) time
     that can be set by the user.
  \li \l{QDateTimeEdit::displayFormat}{displayFormat} contains a string that is used
     to format the time displayed in the widget.
  \endlist

  \sa QDateEdit, QDateTimeEdit
*/

/*!
  Constructs an empty time editor with a \a parent.
*/


QTimeEdit::QTimeEdit(QWidget *parent)
    : QDateTimeEdit(QDATETIMEEDIT_TIME_MIN, QMetaType::QTime, parent)
{
    connect(this, &QTimeEdit::timeChanged, this, &QTimeEdit::userTimeChanged);
}

/*!
  Constructs an empty time editor with a \a parent. The time is set
  to \a time.
*/

QTimeEdit::QTimeEdit(const QTime &time, QWidget *parent)
    : QDateTimeEdit(time, QMetaType::QTime, parent)
{
    connect(this, &QTimeEdit::timeChanged, this, &QTimeEdit::userTimeChanged);
}

/*!
  Destructor.
*/
QTimeEdit::~QTimeEdit()
{
}

/*!
  \property QTimeEdit::time
  \internal
  \sa QDateTimeEdit::time
*/

/*!
  \fn void QTimeEdit::userTimeChanged(const QTime &time)

  This signal only exists to fully implement the time Q_PROPERTY on the class.
  Normally timeChanged should be used instead.

  \internal
*/


/*!
  \class QDateEdit
  \brief The QDateEdit class provides a widget for editing dates based on
  the QDateTimeEdit widget.

  \ingroup basicwidgets
  \inmodule QtWidgets

  \image windows-dateedit.png

  Many of the properties and functions provided by QDateEdit are implemented in
  QDateTimeEdit. These are the relevant properties of this class:

  \list
  \li \l{QDateTimeEdit::date}{date} holds the date displayed by the widget.
  \li \l{QDateTimeEdit::minimumDate}{minimumDate} defines the minimum (earliest)
     date that can be set by the user.
  \li \l{QDateTimeEdit::maximumDate}{maximumDate} defines the maximum (latest) date
     that can be set by the user.
  \li \l{QDateTimeEdit::displayFormat}{displayFormat} contains a string that is used
     to format the date displayed in the widget.
  \endlist

  \sa QTimeEdit, QDateTimeEdit
*/

/*!
  Constructs an empty date editor with a \a parent.
*/

QDateEdit::QDateEdit(QWidget *parent)
    : QDateTimeEdit(QDATETIMEEDIT_DATE_INITIAL, QMetaType::QDate, parent)
{
    connect(this, &QDateEdit::dateChanged, this, &QDateEdit::userDateChanged);
}

/*!
  Constructs an empty date editor with a \a parent. The date is set
  to \a date.
*/

QDateEdit::QDateEdit(const QDate &date, QWidget *parent)
    : QDateTimeEdit(date, QMetaType::QDate, parent)
{
    connect(this, &QDateEdit::dateChanged, this, &QDateEdit::userDateChanged);
}

/*!
  Destructor.
*/
QDateEdit::~QDateEdit()
{
}

/*!
  \property QDateEdit::date
  \internal
  \sa QDateTimeEdit::date
*/

/*!
  \fn void QDateEdit::userDateChanged(const QDate &date)

  This signal only exists to fully implement the date Q_PROPERTY on the class.
  Normally dateChanged should be used instead.

  \internal
*/


// --- QDateTimeEditPrivate ---

/*!
  \internal
  Constructs a QDateTimeEditPrivate object
*/


QDateTimeEditPrivate::QDateTimeEditPrivate()
    : QDateTimeParser(QMetaType::QDateTime, QDateTimeParser::DateTimeEdit, QCalendar())
{
    hasHadFocus = false;
    formatExplicitlySet = false;
    cacheGuard = false;
    fixday = true;
    type = QMetaType::QDateTime;
    sections = { };
    cachedDay = -1;
    currentSectionIndex = FirstSectionIndex;

    first.pos = 0;
    calendarPopup = false;
    minimum = QDATETIMEEDIT_COMPAT_DATE_MIN.startOfDay();
    maximum = QDATETIMEEDIT_DATE_MAX.endOfDay();
    arrowState = QStyle::State_None;
    monthCalendar = nullptr;
    readLocaleSettings();

#ifdef QT_KEYPAD_NAVIGATION
    focusOnButton = false;
#endif
}

void QDateTimeEditPrivate::updateTimeSpec()
{
    minimum = minimum.toDateTime().toTimeSpec(spec);
    maximum = maximum.toDateTime().toTimeSpec(spec);
    value = value.toDateTime().toTimeSpec(spec);

    // time zone changes can lead to 00:00:00 becomes 01:00:00 and 23:59:59 becomes 00:59:59 (invalid range)
    const bool dateShown = (sections & QDateTimeEdit::DateSections_Mask);
    if (!dateShown) {
        if (minimum.toTime() >= maximum.toTime()){
            minimum = value.toDate().startOfDay(spec);
            maximum = value.toDate().endOfDay(spec);
        }
    }
}

void QDateTimeEditPrivate::updateEdit()
{
    const QString newText = (specialValue() ? specialValueText : textFromValue(value));
    if (newText == displayText())
        return;
    int selsize = edit->selectedText().size();
    const QSignalBlocker blocker(edit);

    edit->setText(newText);

    if (!specialValue()
#ifdef QT_KEYPAD_NAVIGATION
        && !(QApplicationPrivate::keypadNavigationEnabled() && !edit->hasEditFocus())
#endif
            ) {
        int cursor = sectionPos(currentSectionIndex);
        QDTEDEBUG << "cursor is " << cursor << currentSectionIndex;
        cursor = qBound(0, cursor, displayText().size());
        QDTEDEBUG << cursor;
        if (selsize > 0) {
            edit->setSelection(cursor, selsize);
            QDTEDEBUG << cursor << selsize;
        } else {
            edit->setCursorPosition(cursor);
            QDTEDEBUG << cursor;

        }
    }
}


/*!
  \internal

  Selects the section \a s. If \a forward is false selects backwards.
*/

void QDateTimeEditPrivate::setSelected(int sectionIndex, bool forward)
{
    if (specialValue()
#ifdef QT_KEYPAD_NAVIGATION
        || (QApplicationPrivate::keypadNavigationEnabled() && !edit->hasEditFocus())
#endif
        ) {
        edit->selectAll();
    } else {
        const SectionNode &node = sectionNode(sectionIndex);
        if (node.type == NoSection || node.type == LastSection || node.type == FirstSection)
            return;

        updateCache(value, displayText());
        const int size = sectionSize(sectionIndex);
        if (forward) {
            edit->setSelection(sectionPos(node), size);
        } else {
            edit->setSelection(sectionPos(node) + size, -size);
        }
    }
}

/*!
  \internal

  Returns the section at index \a index or NoSection if there are no sections there.
*/

int QDateTimeEditPrivate::sectionAt(int pos) const
{
    if (pos < separators.first().size())
        return (pos == 0 ? FirstSectionIndex : NoSectionIndex);

    const QString text = displayText();
    const int textSize = text.size();
    if (textSize - pos < separators.last().size() + 1) {
        if (separators.last().size() == 0) {
            return sectionNodes.count() - 1;
        }
        return (pos == textSize ? LastSectionIndex : NoSectionIndex);
    }
    updateCache(value, text);

    for (int i=0; i<sectionNodes.size(); ++i) {
        const int tmp = sectionPos(i);
        if (pos < tmp + sectionSize(i)) {
            return (pos < tmp ? -1 : i);
        }
    }
    return -1;
}

/*!
  \internal

  Returns the closest section of index \a index. Searches forward
  for a section if \a forward is true. Otherwise searches backwards.
*/

int QDateTimeEditPrivate::closestSection(int pos, bool forward) const
{
    Q_ASSERT(pos >= 0);
    if (pos < separators.first().size())
        return forward ? 0 : FirstSectionIndex;

    const QString text = displayText();
    if (text.size() - pos < separators.last().size() + 1)
        return forward ? LastSectionIndex : sectionNodes.size() - 1;

    updateCache(value, text);
    for (int i=0; i<sectionNodes.size(); ++i) {
        const int tmp = sectionPos(sectionNodes.at(i));
        if (pos < tmp + sectionSize(i)) {
            if (pos < tmp && !forward) {
                return i-1;
            }
            return i;
        } else if (i == sectionNodes.size() - 1 && pos > tmp) {
            return i;
        }
    }
    qWarning("QDateTimeEdit: Internal Error: closestSection returned NoSection");
    return NoSectionIndex;
}

/*!
  \internal

  Returns a copy of the section that is before or after \a current, depending on \a forward.
*/

int QDateTimeEditPrivate::nextPrevSection(int current, bool forward) const
{
    Q_Q(const QDateTimeEdit);
    if (q->isRightToLeft())
        forward = !forward;

    switch (current) {
    case FirstSectionIndex: return forward ? 0 : FirstSectionIndex;
    case LastSectionIndex: return (forward ? LastSectionIndex : sectionNodes.size() - 1);
    case NoSectionIndex: return FirstSectionIndex;
    default: break;
    }
    Q_ASSERT(current >= 0 && current < sectionNodes.size());

    current += (forward ? 1 : -1);
    if (current >= sectionNodes.size()) {
        return LastSectionIndex;
    } else if (current < 0) {
        return FirstSectionIndex;
    }

    return current;
}

/*!
  \internal

  Clears the text of section \a s.
*/

void QDateTimeEditPrivate::clearSection(int index)
{
    const QLatin1Char space(' ');
    int cursorPos = edit->cursorPosition();
    const QSignalBlocker blocker(edit);
    QString t = edit->text();
    const int pos = sectionPos(index);
    if (Q_UNLIKELY(pos == -1)) {
        qWarning("QDateTimeEdit: Internal error (%s:%d)", __FILE__, __LINE__);
        return;
    }
    const int size = sectionSize(index);
    t.replace(pos, size, QString().fill(space, size));
    edit->setText(t);
    edit->setCursorPosition(cursorPos);
    QDTEDEBUG << cursorPos;
}


/*!
  \internal

  updates the cached values
*/

void QDateTimeEditPrivate::updateCache(const QVariant &val, const QString &str) const
{
    if (val != cachedValue || str != cachedText || cacheGuard) {
        cacheGuard = true;
        QString copy = str;
        int unused = edit->cursorPosition();
        QValidator::State unusedState;
        validateAndInterpret(copy, unused, unusedState);
        cacheGuard = false;
    }
}

/*!
  \internal

  parses and validates \a input
*/

QDateTime QDateTimeEditPrivate::validateAndInterpret(QString &input, int &position,
                                                     QValidator::State &state, bool fixup) const
{
    if (input.isEmpty()) {
        if (sectionNodes.size() == 1 || !specialValueText.isEmpty()) {
            state = QValidator::Intermediate;
        } else {
            state = QValidator::Invalid;
        }
        return getZeroVariant().toDateTime();
    } else if (cachedText == input && !fixup) {
        state = cachedState;
        return cachedValue.toDateTime();
    } else if (!specialValueText.isEmpty()) {
        bool changeCase = false;
        const int max = qMin(specialValueText.size(), input.size());
        int i;
        for (i=0; i<max; ++i) {
            const QChar ic = input.at(i);
            const QChar sc = specialValueText.at(i);
            if (ic != sc) {
                if (sc.toLower() == ic.toLower()) {
                    changeCase = true;
                } else {
                    break;
                }
            }
        }
        if (i == max) {
            state = specialValueText.size() == input.size() ? QValidator::Acceptable : QValidator::Intermediate;
            if (changeCase) {
                input = specialValueText.left(max);
            }
            return minimum.toDateTime();
        }
    }

    StateNode tmp = parse(input, position, value.toDateTime(), fixup);
    // Impose this widget's spec:
    tmp.value = tmp.value.toTimeSpec(spec);
    // ... but that might turn a valid datetime into an invalid one:
    if (!tmp.value.isValid() && tmp.state == Acceptable)
        tmp.state = Intermediate;

    input = tmp.input;
    position += tmp.padded;
    state = QValidator::State(int(tmp.state));
    if (state == QValidator::Acceptable) {
        if (tmp.conflicts && conflictGuard != tmp.value) {
            conflictGuard = tmp.value;
            clearCache();
            input = textFromValue(tmp.value);
            updateCache(tmp.value, input);
            conflictGuard.clear();
        } else {
            cachedText = input;
            cachedState = state;
            cachedValue = tmp.value;
        }
    } else {
        clearCache();
    }
    return (tmp.value.isNull() ? getZeroVariant().toDateTime() : tmp.value);
}


/*!
  \internal
*/

QString QDateTimeEditPrivate::textFromValue(const QVariant &f) const
{
    Q_Q(const QDateTimeEdit);
    return q->textFromDateTime(f.toDateTime());
}

/*!
  \internal

  This function's name is slightly confusing; it is not to be confused
  with QAbstractSpinBox::valueFromText().
*/

QVariant QDateTimeEditPrivate::valueFromText(const QString &f) const
{
    Q_Q(const QDateTimeEdit);
    return q->dateTimeFromText(f).toTimeSpec(spec);
}


/*!
  \internal

  Internal function called by QDateTimeEdit::stepBy(). Also takes a
  Section for which section to step on and a bool \a test for
  whether or not to modify the internal cachedDay variable. This is
  necessary because the function is called from the const function
  QDateTimeEdit::stepEnabled() as well as QDateTimeEdit::stepBy().
*/

QDateTime QDateTimeEditPrivate::stepBy(int sectionIndex, int steps, bool test) const
{
    Q_Q(const QDateTimeEdit);
    QDateTime v = value.toDateTime();
    QString str = displayText();
    int pos = edit->cursorPosition();
    const SectionNode sn = sectionNode(sectionIndex);

    int val;
    // to make sure it behaves reasonably when typing something and then stepping in non-tracking mode
    if (!test && pendingEmit) {
        if (q->validate(str, pos) != QValidator::Acceptable) {
            v = value.toDateTime();
        } else {
            v = q->dateTimeFromText(str);
        }
        val = getDigit(v, sectionIndex);
    } else {
        val = getDigit(v, sectionIndex);
    }

    val += steps;

    const int min = absoluteMin(sectionIndex);
    const int max = absoluteMax(sectionIndex, value.toDateTime());

    if (val < min) {
        val = (wrapping ? max - (min - val) + 1 : min);
    } else if (val > max) {
        val = (wrapping ? min + val - max - 1 : max);
    }

    const int oldDay = v.date().day(calendar);

    setDigit(v, sectionIndex, val);
    /*
        Stepping into a daylight saving time that doesn't exist,
        so use the time that has the same distance from epoch.
    */
    if (!v.isValid()) {
        auto msecsSinceEpoch = v.toMSecsSinceEpoch();
        // decreasing from e.g 3am to 2am would get us back to 3am, but we want 1am
        if (steps < 0 && sn.type & HourSectionMask)
            msecsSinceEpoch -= 3600 * 1000;
        v = QDateTime::fromMSecsSinceEpoch(msecsSinceEpoch, v.timeSpec());
    }
    // if this sets year or month it will make
    // sure that days are lowered if needed.

    const QDateTime minimumDateTime = minimum.toDateTime();
    const QDateTime maximumDateTime = maximum.toDateTime();
    // changing one section should only modify that section, if possible
    if (sn.type != AmPmSection && (v < minimumDateTime || v > maximumDateTime)) {
        const int localmin = getDigit(minimumDateTime, sectionIndex);
        const int localmax = getDigit(maximumDateTime, sectionIndex);

        if (wrapping) {
            // just because we hit the roof in one direction, it
            // doesn't mean that we hit the floor in the other
            if (steps > 0) {
                setDigit(v, sectionIndex, min);
                if (!(sn.type & DaySectionMask) && sections & DateSectionMask) {
                    const int daysInMonth = v.date().daysInMonth(calendar);
                    if (v.date().day(calendar) < oldDay && v.date().day(calendar) < daysInMonth) {
                        const int adds = qMin(oldDay, daysInMonth);
                        v = v.addDays(adds - v.date().day(calendar));
                    }
                }

                if (v < minimumDateTime) {
                    setDigit(v, sectionIndex, localmin);
                    if (v < minimumDateTime)
                        setDigit(v, sectionIndex, localmin + 1);
                }
            } else {
                setDigit(v, sectionIndex, max);
                if (!(sn.type & DaySectionMask) && sections & DateSectionMask) {
                    const int daysInMonth = v.date().daysInMonth(calendar);
                    if (v.date().day(calendar) < oldDay && v.date().day(calendar) < daysInMonth) {
                        const int adds = qMin(oldDay, daysInMonth);
                        v = v.addDays(adds - v.date().day(calendar));
                    }
                }

                if (v > maximumDateTime) {
                    setDigit(v, sectionIndex, localmax);
                    if (v > maximumDateTime)
                        setDigit(v, sectionIndex, localmax - 1);
                }
            }
        } else {
            setDigit(v, sectionIndex, (steps > 0 ? localmax : localmin));
        }
    }
    if (!test && oldDay != v.date().day(calendar) && !(sn.type & DaySectionMask)) {
        // this should not happen when called from stepEnabled
        cachedDay = qMax<int>(oldDay, cachedDay);
    }

    if (v < minimumDateTime) {
        if (wrapping) {
            QDateTime t = v;
            setDigit(t, sectionIndex, steps < 0 ? max : min);
            bool mincmp = (t >= minimumDateTime);
            bool maxcmp = (t <= maximumDateTime);
            if (!mincmp || !maxcmp) {
                setDigit(t, sectionIndex, getDigit(steps < 0
                                                   ? maximumDateTime
                                                   : minimumDateTime, sectionIndex));
                mincmp = (t >= minimumDateTime);
                maxcmp = (t <= maximumDateTime);
            }
            if (mincmp && maxcmp) {
                v = t;
            }
        } else {
            v = value.toDateTime();
        }
    } else if (v > maximumDateTime) {
        if (wrapping) {
            QDateTime t = v;
            setDigit(t, sectionIndex, steps > 0 ? min : max);
            bool mincmp = (t >= minimumDateTime);
            bool maxcmp = (t <= maximumDateTime);
            if (!mincmp || !maxcmp) {
                setDigit(t, sectionIndex, getDigit(steps > 0 ?
                                                   minimumDateTime :
                                                   maximumDateTime, sectionIndex));
                mincmp = (t >= minimumDateTime);
                maxcmp = (t <= maximumDateTime);
            }
            if (mincmp && maxcmp) {
                v = t;
            }
        } else {
            v = value.toDateTime();
        }
    }

    const QDateTime ret = bound(v, value, steps).toDateTime().toTimeSpec(spec);
    return ret;
}

/*!
  \internal
*/

void QDateTimeEditPrivate::emitSignals(EmitPolicy ep, const QVariant &old)
{
    Q_Q(QDateTimeEdit);
    if (ep == NeverEmit) {
        return;
    }
    pendingEmit = false;

    const bool dodate = value.toDate().isValid() && (sections & DateSectionMask);
    const bool datechanged = (ep == AlwaysEmit || old.toDate() != value.toDate());
    const bool dotime = value.toTime().isValid() && (sections & TimeSectionMask);
    const bool timechanged = (ep == AlwaysEmit || old.toTime() != value.toTime());

    updateCache(value, displayText());

    syncCalendarWidget();
    if (datechanged || timechanged)
        emit q->dateTimeChanged(value.toDateTime());
    if (dodate && datechanged)
        emit q->dateChanged(value.toDate());
    if (dotime && timechanged)
        emit q->timeChanged(value.toTime());

}

/*!
  \internal
*/

void QDateTimeEditPrivate::_q_editorCursorPositionChanged(int oldpos, int newpos)
{
    if (ignoreCursorPositionChanged || specialValue())
        return;
    const QString oldText = displayText();
    updateCache(value, oldText);

    const bool allowChange = !edit->hasSelectedText();
    const bool forward = oldpos <= newpos;
    ignoreCursorPositionChanged = true;
    int s = sectionAt(newpos);
    if (s == NoSectionIndex && forward && newpos > 0) {
        s = sectionAt(newpos - 1);
    }

    int c = newpos;

    const int selstart = edit->selectionStart();
    const int selSection = sectionAt(selstart);
    const int l = selSection != -1 ? sectionSize(selSection) : 0;

    if (s == NoSectionIndex) {
        if (l > 0 && selstart == sectionPos(selSection) && edit->selectedText().size() == l) {
            s = selSection;
            if (allowChange)
                setSelected(selSection, true);
            c = -1;
        } else {
            int closest = closestSection(newpos, forward);
            c = sectionPos(closest) + (forward ? 0 : qMax<int>(0, sectionSize(closest)));

            if (allowChange) {
                edit->setCursorPosition(c);
                QDTEDEBUG << c;
            }
            s = closest;
        }
    }

    if (allowChange && currentSectionIndex != s) {
        interpret(EmitIfChanged);
    }
    if (c == -1) {
        setSelected(s, true);
    } else if (!edit->hasSelectedText()) {
        if (oldpos < newpos) {
            edit->setCursorPosition(displayText().size() - (oldText.size() - c));
        } else {
            edit->setCursorPosition(c);
        }
    }

    QDTEDEBUG << "currentSectionIndex is set to" << sectionNode(s).name()
              << oldpos << newpos
              << "was" << sectionNode(currentSectionIndex).name();

    currentSectionIndex = s;
    Q_ASSERT_X(currentSectionIndex < sectionNodes.size(),
               "QDateTimeEditPrivate::_q_editorCursorPositionChanged()",
               qPrintable(QString::fromLatin1("Internal error (%1 %2)").
                          arg(currentSectionIndex).
                          arg(sectionNodes.size())));

    ignoreCursorPositionChanged = false;
}

/*!
  \internal

  Try to get the format from the local settings
*/
void QDateTimeEditPrivate::readLocaleSettings()
{
    const QLocale loc;
    defaultTimeFormat = loc.timeFormat(QLocale::ShortFormat);
    defaultDateFormat = loc.dateFormat(QLocale::ShortFormat);
    defaultDateTimeFormat = loc.dateTimeFormat(QLocale::ShortFormat);
}

QDateTimeEdit::Section QDateTimeEditPrivate::convertToPublic(QDateTimeParser::Section s)
{
    switch (s & ~Internal) {
    case AmPmSection: return QDateTimeEdit::AmPmSection;
    case MSecSection: return QDateTimeEdit::MSecSection;
    case SecondSection: return QDateTimeEdit::SecondSection;
    case MinuteSection: return QDateTimeEdit::MinuteSection;
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
    case DaySection: return QDateTimeEdit::DaySection;
    case MonthSection: return QDateTimeEdit::MonthSection;
    case YearSection2Digits:
    case YearSection: return QDateTimeEdit::YearSection;
    case Hour12Section:
    case Hour24Section: return QDateTimeEdit::HourSection;
    case FirstSection:
    case NoSection:
    case LastSection: break;
    }
    return QDateTimeEdit::NoSection;
}

QDateTimeEdit::Sections QDateTimeEditPrivate::convertSections(QDateTimeParser::Sections s)
{
    QDateTimeEdit::Sections ret;
    if (s & QDateTimeParser::MSecSection)
        ret |= QDateTimeEdit::MSecSection;
    if (s & QDateTimeParser::SecondSection)
        ret |= QDateTimeEdit::SecondSection;
    if (s & QDateTimeParser::MinuteSection)
        ret |= QDateTimeEdit::MinuteSection;
    if (s & (QDateTimeParser::HourSectionMask))
        ret |= QDateTimeEdit::HourSection;
    if (s & QDateTimeParser::AmPmSection)
        ret |= QDateTimeEdit::AmPmSection;
    if (s & (QDateTimeParser::DaySectionMask))
        ret |= QDateTimeEdit::DaySection;
    if (s & QDateTimeParser::MonthSection)
        ret |= QDateTimeEdit::MonthSection;
    if (s & (QDateTimeParser::YearSectionMask))
        ret |= QDateTimeEdit::YearSection;

    return ret;
}

/*!
    \reimp
*/

void QDateTimeEdit::paintEvent(QPaintEvent *event)
{
    Q_D(QDateTimeEdit);
    if (!d->calendarPopupEnabled()) {
        QAbstractSpinBox::paintEvent(event);
        return;
    }

    QStyleOptionSpinBox opt;
    initStyleOption(&opt);

    QStyleOptionComboBox optCombo;

    optCombo.init(this);
    optCombo.editable = true;
    optCombo.frame = opt.frame;
    optCombo.subControls = opt.subControls;
    optCombo.activeSubControls = opt.activeSubControls;
    optCombo.state = opt.state;
    if (d->readOnly) {
        optCombo.state &= ~QStyle::State_Enabled;
    }

    QPainter p(this);
    style()->drawComplexControl(QStyle::CC_ComboBox, &optCombo, &p, this);
}

/*
    Returns the string for AM and PM markers.

    If a translation for "AM" and "PM" is installed, then use that.
    Otherwise, use the default implementation, which uses the locale.
*/
QString QDateTimeEditPrivate::getAmPmText(AmPm ap, Case cs) const
{
    QString original;
    QString translated;
    if (ap == AmText) {
        original = QLatin1String(cs == UpperCase ? "AM" : "am");
        translated = (cs == UpperCase ? QDateTimeParser::tr("AM") : QDateTimeParser::tr("am"));
    } else {
        original = QLatin1String(cs == UpperCase ? "PM" : "pm");
        translated = (cs == UpperCase ? QDateTimeParser::tr("PM") : QDateTimeParser::tr("pm"));
    }

    // This logic fails if a translation exists but doesn't change the string,
    // which we can accept as a corner-case for which a locale-derived answer
    // will be acceptable.
    if (original != translated)
        return translated;

    return QDateTimeParser::getAmPmText(ap, cs);
}

int QDateTimeEditPrivate::absoluteIndex(QDateTimeEdit::Section s, int index) const
{
    for (int i=0; i<sectionNodes.size(); ++i) {
        if (convertToPublic(sectionNodes.at(i).type) == s && index-- == 0) {
            return i;
        }
    }
    return NoSectionIndex;
}

int QDateTimeEditPrivate::absoluteIndex(const SectionNode &s) const
{
    return sectionNodes.indexOf(s);
}

void QDateTimeEditPrivate::interpret(EmitPolicy ep)
{
    Q_Q(QDateTimeEdit);
    QString tmp = displayText();
    int pos = edit->cursorPosition();
    const QValidator::State state = q->validate(tmp, pos);
    if (state != QValidator::Acceptable
        && correctionMode == QAbstractSpinBox::CorrectToPreviousValue
        && (state == QValidator::Invalid
            || currentSectionIndex < 0
            || !(fieldInfo(currentSectionIndex) & AllowPartial))) {
        setValue(value, ep);
        updateTimeSpec();
    } else {
        QAbstractSpinBoxPrivate::interpret(ep);
    }
}

void QDateTimeEditPrivate::clearCache() const
{
    QAbstractSpinBoxPrivate::clearCache();
    cachedDay = -1;
}

/*!
    Initialize \a option with the values from this QDataTimeEdit. This method
    is useful for subclasses when they need a QStyleOptionSpinBox, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QDateTimeEdit::initStyleOption(QStyleOptionSpinBox *option) const
{
    if (!option)
        return;

    Q_D(const QDateTimeEdit);
    QAbstractSpinBox::initStyleOption(option);
    if (d->calendarPopupEnabled()) {
        option->subControls = QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxEditField
                              | QStyle::SC_ComboBoxArrow;
        if (d->arrowState == QStyle::State_Sunken)
            option->state |= QStyle::State_Sunken;
        else
            option->state &= ~QStyle::State_Sunken;
    }
}

void QDateTimeEditPrivate::init(const QVariant &var)
{
    Q_Q(QDateTimeEdit);
    switch (var.userType()) {
    case QMetaType::QDate:
        value = var.toDate().startOfDay();
        updateTimeSpec();
        q->setDisplayFormat(defaultDateFormat);
        if (sectionNodes.isEmpty()) // ### safeguard for broken locale
            q->setDisplayFormat(QLatin1String("dd/MM/yyyy"));
        break;
    case QMetaType::QDateTime:
        value = var;
        updateTimeSpec();
        q->setDisplayFormat(defaultDateTimeFormat);
        if (sectionNodes.isEmpty()) // ### safeguard for broken locale
            q->setDisplayFormat(QLatin1String("dd/MM/yyyy hh:mm:ss"));
        break;
    case QMetaType::QTime:
        value = QDateTime(QDATETIMEEDIT_DATE_INITIAL, var.toTime());
        updateTimeSpec();
        q->setDisplayFormat(defaultTimeFormat);
        if (sectionNodes.isEmpty()) // ### safeguard for broken locale
            q->setDisplayFormat(QLatin1String("hh:mm:ss"));
        break;
    default:
        Q_ASSERT_X(0, "QDateTimeEditPrivate::init", "Internal error");
        break;
    }
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplicationPrivate::keypadNavigationEnabled())
        q->setCalendarPopup(true);
#endif
    q->setInputMethodHints(Qt::ImhPreferNumbers);
    setLayoutItemMargins(QStyle::SE_DateTimeEditLayoutItem);
}

void QDateTimeEditPrivate::_q_resetButton()
{
    updateArrow(QStyle::State_None);
}

void QDateTimeEditPrivate::updateArrow(QStyle::StateFlag state)
{
    Q_Q(QDateTimeEdit);

    if (arrowState == state)
        return;
    arrowState = state;
    if (arrowState != QStyle::State_None)
        buttonState |= Mouse;
    else {
        buttonState = 0;
        hoverControl = QStyle::SC_ComboBoxFrame;
    }
    q->update();
}

/*!
    \internal
    Returns the hover control at \a pos.
    This will update the hoverRect and hoverControl.
*/
QStyle::SubControl QDateTimeEditPrivate::newHoverControl(const QPoint &pos)
{
    if (!calendarPopupEnabled())
        return QAbstractSpinBoxPrivate::newHoverControl(pos);

    Q_Q(QDateTimeEdit);

    QStyleOptionComboBox optCombo;
    optCombo.init(q);
    optCombo.editable = true;
    optCombo.subControls = QStyle::SC_All;
    hoverControl = q->style()->hitTestComplexControl(QStyle::CC_ComboBox, &optCombo, pos, q);
    return hoverControl;
}

void QDateTimeEditPrivate::updateEditFieldGeometry()
{
    if (!calendarPopupEnabled()) {
        QAbstractSpinBoxPrivate::updateEditFieldGeometry();
        return;
    }

    Q_Q(QDateTimeEdit);

    QStyleOptionComboBox optCombo;
    optCombo.init(q);
    optCombo.editable = true;
    optCombo.subControls = QStyle::SC_ComboBoxEditField;
    edit->setGeometry(q->style()->subControlRect(QStyle::CC_ComboBox, &optCombo,
                                                 QStyle::SC_ComboBoxEditField, q));
}

QVariant QDateTimeEditPrivate::getZeroVariant() const
{
    Q_ASSERT(type == QMetaType::QDateTime);
    return QDateTime(QDATETIMEEDIT_DATE_INITIAL, QTime(), spec);
}

void QDateTimeEditPrivate::setRange(const QVariant &min, const QVariant &max)
{
    QAbstractSpinBoxPrivate::setRange(min, max);
    syncCalendarWidget();
}


bool QDateTimeEditPrivate::isSeparatorKey(const QKeyEvent *ke) const
{
    if (!ke->text().isEmpty() && currentSectionIndex + 1 < sectionNodes.size() && currentSectionIndex >= 0) {
        if (fieldInfo(currentSectionIndex) & Numeric) {
            if (ke->text().at(0).isNumber())
                return false;
        } else if (ke->text().at(0).isLetterOrNumber()) {
            return false;
        }
        return separators.at(currentSectionIndex + 1).contains(ke->text());
    }
    return false;
}

void QDateTimeEditPrivate::initCalendarPopup(QCalendarWidget *cw)
{
    Q_Q(QDateTimeEdit);
    if (!monthCalendar) {
        monthCalendar = new QCalendarPopup(q, cw, calendar);
        monthCalendar->setObjectName(QLatin1String("qt_datetimedit_calendar"));
        QObject::connect(monthCalendar, SIGNAL(newDateSelected(QDate)), q, SLOT(setDate(QDate)));
        QObject::connect(monthCalendar, SIGNAL(hidingCalendar(QDate)), q, SLOT(setDate(QDate)));
        QObject::connect(monthCalendar, SIGNAL(activated(QDate)), q, SLOT(setDate(QDate)));
        QObject::connect(monthCalendar, SIGNAL(activated(QDate)), monthCalendar, SLOT(close()));
        QObject::connect(monthCalendar, SIGNAL(resetButton()), q, SLOT(_q_resetButton()));
    } else if (cw) {
        monthCalendar->setCalendarWidget(cw);
    }
    syncCalendarWidget();
}

void QDateTimeEditPrivate::positionCalendarPopup()
{
    Q_Q(QDateTimeEdit);
    QPoint pos = (q->layoutDirection() == Qt::RightToLeft) ? q->rect().bottomRight() : q->rect().bottomLeft();
    QPoint pos2 = (q->layoutDirection() == Qt::RightToLeft) ? q->rect().topRight() : q->rect().topLeft();
    pos = q->mapToGlobal(pos);
    pos2 = q->mapToGlobal(pos2);
    QSize size = monthCalendar->sizeHint();
    QRect screen = QDesktopWidgetPrivate::availableGeometry(pos);
    //handle popup falling "off screen"
    if (q->layoutDirection() == Qt::RightToLeft) {
        pos.setX(pos.x()-size.width());
        pos2.setX(pos2.x()-size.width());
        if (pos.x() < screen.left())
            pos.setX(qMax(pos.x(), screen.left()));
        else if (pos.x()+size.width() > screen.right())
            pos.setX(qMax(pos.x()-size.width(), screen.right()-size.width()));
    } else {
        if (pos.x()+size.width() > screen.right())
            pos.setX(screen.right()-size.width());
        pos.setX(qMax(pos.x(), screen.left()));
    }
    if (pos.y() + size.height() > screen.bottom())
        pos.setY(pos2.y() - size.height());
    else if (pos.y() < screen.top())
        pos.setY(screen.top());
    if (pos.y() < screen.top())
        pos.setY(screen.top());
    if (pos.y()+size.height() > screen.bottom())
        pos.setY(screen.bottom()-size.height());
    monthCalendar->move(pos);
}

bool QDateTimeEditPrivate::calendarPopupEnabled() const
{
    return (calendarPopup && (sections & (DateSectionMask)));
}

void QDateTimeEditPrivate::syncCalendarWidget()
{
    Q_Q(QDateTimeEdit);
    if (monthCalendar) {
        const QSignalBlocker blocker(monthCalendar);
        monthCalendar->setDateRange(q->minimumDate(), q->maximumDate());
        monthCalendar->setDate(q->date());
    }
}

QCalendarPopup::QCalendarPopup(QWidget *parent, QCalendarWidget *cw, QCalendar ca)
    : QWidget(parent, Qt::Popup), calendarSystem(ca)
{
    setAttribute(Qt::WA_WindowPropagation);

    dateChanged = false;
    if (!cw) {
        verifyCalendarInstance();
    } else {
        setCalendarWidget(cw);
    }
}

QCalendarWidget *QCalendarPopup::verifyCalendarInstance()
{
    if (calendar.isNull()) {
        QCalendarWidget *cw = new QCalendarWidget(this);
        cw->setCalendar(calendarSystem);
        cw->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplicationPrivate::keypadNavigationEnabled())
            cw->setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames);
#endif
        setCalendarWidget(cw);
        return cw;
    } else {
        return calendar.data();
    }
}

void QCalendarPopup::setCalendarWidget(QCalendarWidget *cw)
{
    Q_ASSERT(cw);
    QVBoxLayout *widgetLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!widgetLayout) {
        widgetLayout = new QVBoxLayout(this);
        widgetLayout->setContentsMargins(QMargins());
        widgetLayout->setSpacing(0);
    }
    delete calendar.data();
    calendar = QPointer<QCalendarWidget>(cw);
    widgetLayout->addWidget(cw);

    connect(cw, SIGNAL(activated(QDate)), this, SLOT(dateSelected(QDate)));
    connect(cw, SIGNAL(clicked(QDate)), this, SLOT(dateSelected(QDate)));
    connect(cw, SIGNAL(selectionChanged()), this, SLOT(dateSelectionChanged()));

    cw->setFocus();
}


void QCalendarPopup::setDate(QDate date)
{
    oldDate = date;
    verifyCalendarInstance()->setSelectedDate(date);
}

void QCalendarPopup::setDateRange(QDate min, QDate max)
{
    QCalendarWidget *cw = verifyCalendarInstance();
    cw->setMinimumDate(min);
    cw->setMaximumDate(max);
}

void QCalendarPopup::mousePressEvent(QMouseEvent *event)
{
    QDateTimeEdit *dateTime = qobject_cast<QDateTimeEdit *>(parentWidget());
    if (dateTime) {
        QStyleOptionComboBox opt;
        opt.init(dateTime);
        QRect arrowRect = dateTime->style()->subControlRect(QStyle::CC_ComboBox, &opt,
                                                            QStyle::SC_ComboBoxArrow, dateTime);
        arrowRect.moveTo(dateTime->mapToGlobal(arrowRect .topLeft()));
        if (arrowRect.contains(event->globalPos()) || rect().contains(event->pos()))
            setAttribute(Qt::WA_NoMouseReplay);
    }
    QWidget::mousePressEvent(event);
}

void QCalendarPopup::mouseReleaseEvent(QMouseEvent*)
{
    emit resetButton();
}

bool QCalendarPopup::event(QEvent *event)
{
#if QT_CONFIG(shortcut)
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->matches(QKeySequence::Cancel))
            dateChanged = false;
    }
#endif
    return QWidget::event(event);
}

void QCalendarPopup::dateSelectionChanged()
{
    dateChanged = true;
    emit newDateSelected(verifyCalendarInstance()->selectedDate());
}
void QCalendarPopup::dateSelected(QDate date)
{
    dateChanged = true;
    emit activated(date);
    close();
}

void QCalendarPopup::hideEvent(QHideEvent *)
{
    emit resetButton();
    if (!dateChanged)
        emit hidingCalendar(oldDate);
}

QT_END_NAMESPACE
#include "moc_qdatetimeedit.cpp"
#include "moc_qdatetimeedit_p.cpp"
