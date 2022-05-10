// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DATEFORMATS_H
#define DATEFORMATS_H

#include <QWidget>
#include <QLocale>

class QLineEdit;
class QScrollArea;
class QGridLayout;
class QComboBox;

class DateFormatsWidget : public QWidget
{
    Q_OBJECT
public:
    DateFormatsWidget();

private:
    void addItem(const QString &label, QWidget *);
    QLineEdit *addItem(const QString &label);

    QScrollArea *scrollArea;
    QWidget *scrollAreaWidget;
    QGridLayout *layout;

    QLineEdit *shortDateFormat;
    QLineEdit *longDateFormat;
    QLineEdit *shortTimeFormat;
    QLineEdit *longTimeFormat;
    QLineEdit *shortDateTimeFormat;
    QLineEdit *longDateTimeFormat;
    QLineEdit *amText;
    QLineEdit *pmText;
    QLineEdit *firstDayOfWeek;
    QComboBox *monthNamesShort, *monthNamesLong;
    QComboBox *standaloneMonthNamesShort, *standaloneMonthNamesLong;
    QComboBox *dayNamesShort, *dayNamesLong;
    QComboBox *standaloneDayNamesShort, *standaloneDayNamesLong;

private slots:
    void localeChanged(QLocale locale);
};

#endif
