// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CALENDAR_H
#define CALENDAR_H

#include <QWidget>
#include <QDate>
#include <QLocale>

class QComboBox;
class QGridLayout;
class QGroupBox;
class QCalendarWidget;
class QLabel;
class QDateEdit;
class QCheckBox;

class CalendarWidget : public QWidget
{
    Q_OBJECT

public:
    CalendarWidget();

private slots:
    void localeChanged(QLocale locale);
    void firstDayChanged(int index);
    void selectionModeChanged(int index);
    void horizontalHeaderChanged(int index);
    void verticalHeaderChanged(int index);
    void selectedDateChanged();
    void minimumDateChanged(QDate date);
    void maximumDateChanged(QDate date);
    void updateWeekendDays();
    void weekdayFormatChanged();
    void weekendFormatChanged();
    void reformatHeaders();
    void reformatCalendarPage();

private:
    bool isWeekendDay(Qt::DayOfWeek);
    void createPreviewGroupBox();
    void createGeneralOptionsGroupBox();
    void createDatesGroupBox();
    void createTextFormatsGroupBox();
    QComboBox *createColorComboBox();

    QGroupBox *previewGroupBox;
    QGridLayout *previewLayout;
    QCalendarWidget *calendar;

    QGroupBox *generalOptionsGroupBox;
    QLabel *localeLabel;
    QLabel *firstDayLabel;

    QLabel *selectionModeLabel;
    QLabel *horizontalHeaderLabel;
    QLabel *verticalHeaderLabel;
    QComboBox *localeCombo;
    QComboBox *firstDayCombo;
    QComboBox *selectionModeCombo;
    QCheckBox *gridCheckBox;
    QCheckBox *navigationCheckBox;
    QComboBox *horizontalHeaderCombo;
    QComboBox *verticalHeaderCombo;

    QGroupBox *datesGroupBox;
    QLabel *currentDateLabel;
    QLabel *minimumDateLabel;
    QLabel *maximumDateLabel;
    QDateEdit *currentDateEdit;
    QDateEdit *minimumDateEdit;
    QDateEdit *maximumDateEdit;

    QGroupBox *textFormatsGroupBox;
    QLabel *weekdayColorLabel;
    QLabel *weekendColorLabel;
    QLabel *headerTextFormatLabel;
    QComboBox *weekdayColorCombo;
    QComboBox *weekendColorCombo;
    QComboBox *headerTextFormatCombo;

    QCheckBox *firstFridayCheckBox;
    QCheckBox *mayFirstCheckBox;
};

#endif
