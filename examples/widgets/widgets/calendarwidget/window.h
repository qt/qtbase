// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QCalendarWidget;
class QCheckBox;
class QComboBox;
class QDate;
class QDateEdit;
class QGridLayout;
class QGroupBox;
class QLabel;
QT_END_NAMESPACE

//! [0]
class Window : public QWidget
{
    Q_OBJECT

public:
    Window(QWidget *parent = nullptr);

private slots:
    void localeChanged(int index);
    void firstDayChanged(int index);
    void selectionModeChanged(int index);
    void horizontalHeaderChanged(int index);
    void verticalHeaderChanged(int index);
    void selectedDateChanged();
    void minimumDateChanged(QDate date);
    void maximumDateChanged(QDate date);
    void weekdayFormatChanged();
    void weekendFormatChanged();
    void reformatHeaders();
    void reformatCalendarPage();

private:
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
//! [0]
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
//! [1]
    QCheckBox *mayFirstCheckBox;
};
//! [1]

#endif
