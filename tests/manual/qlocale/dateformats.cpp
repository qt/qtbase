/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "dateformats.h"

#include <QLineEdit>
#include <QScrollArea>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QDateTime>

DateFormatsWidget::DateFormatsWidget()
{
    scrollArea = new QScrollArea;
    scrollAreaWidget = new QWidget;
    scrollArea->setWidget(scrollAreaWidget);
    scrollArea->setWidgetResizable(true);
    layout = new QGridLayout(scrollAreaWidget);
    QVBoxLayout *l = new QVBoxLayout(this);
    l->addWidget(scrollArea);

    shortDateFormat     = addItem("Date format (short):");
    longDateFormat      = addItem("Date format (long):");
    shortTimeFormat     = addItem("Time format (short):");
    longTimeFormat      = addItem("Time format (long):");
    shortDateTimeFormat = addItem("DateTime format (short):");
    longDateTimeFormat  = addItem("DateTime format (long):");
    amText = addItem("Before noon:");
    pmText = addItem("After noon:");
    firstDayOfWeek = addItem("First day of week:");

    monthNamesShort = new QComboBox;
    monthNamesLong = new QComboBox;
    standaloneMonthNamesShort = new QComboBox;
    standaloneMonthNamesLong = new QComboBox;
    dayNamesShort = new QComboBox;
    dayNamesLong = new QComboBox;
    standaloneDayNamesShort = new QComboBox;
    standaloneDayNamesLong = new QComboBox;

    int row = layout->rowCount();
    layout->addWidget(new QLabel("Month names [short/long]:"), row, 0);
    layout->addWidget(monthNamesShort, row, 1);
    layout->addWidget(monthNamesLong, row, 2);
    row = layout->rowCount();
    layout->addWidget(new QLabel("Standalone month names [short/long]:"), row, 0);
    layout->addWidget(standaloneMonthNamesShort, row, 1);
    layout->addWidget(standaloneMonthNamesLong, row, 2);
    row = layout->rowCount();
    layout->addWidget(new QLabel("Day names [short/long]:"), row, 0);
    layout->addWidget(dayNamesShort, row, 1);
    layout->addWidget(dayNamesLong, row, 2);
    row = layout->rowCount();
    layout->addWidget(new QLabel("Standalone day names [short/long]:"), row, 0);
    layout->addWidget(standaloneDayNamesShort, row, 1);
    layout->addWidget(standaloneDayNamesLong, row, 2);
}

QString toString(Qt::DayOfWeek dow)
{
    static const char *names[] = {
        "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
    };
    return QString::fromLatin1(names[dow-1]);
}

void DateFormatsWidget::localeChanged(QLocale locale)
{
    setLocale(locale);
    QDateTime now = QDateTime::currentDateTime();
    shortDateFormat->setText(locale.toString(now.date(), QLocale::ShortFormat));
    shortDateFormat->setToolTip(locale.dateFormat(QLocale::ShortFormat));
    longDateFormat->setText(locale.toString(now.date(), QLocale::LongFormat));
    longDateFormat->setToolTip(locale.dateFormat(QLocale::LongFormat));
    shortTimeFormat->setText(locale.toString(now.time(), QLocale::ShortFormat));
    shortTimeFormat->setToolTip(locale.timeFormat(QLocale::ShortFormat));
    longTimeFormat->setText(locale.toString(now.time(), QLocale::LongFormat));
    longTimeFormat->setToolTip(locale.timeFormat(QLocale::LongFormat));
    shortDateTimeFormat->setText(locale.toString(now, QLocale::ShortFormat));
    shortDateTimeFormat->setToolTip(locale.dateTimeFormat(QLocale::ShortFormat));
    longDateTimeFormat->setText(locale.toString(now, QLocale::LongFormat));
    longDateTimeFormat->setToolTip(locale.dateTimeFormat(QLocale::LongFormat));
    amText->setText(locale.amText());
    pmText->setText(locale.pmText());
    firstDayOfWeek->setText(toString(locale.firstDayOfWeek()));

    int mns = monthNamesShort->currentIndex();
    int mnl = monthNamesLong->currentIndex();
    int smns = standaloneMonthNamesShort->currentIndex();
    int smnl = standaloneMonthNamesLong->currentIndex();
    int dnl = dayNamesLong->currentIndex();
    int dns = dayNamesShort->currentIndex();
    int sdnl = standaloneDayNamesLong->currentIndex();
    int sdns = standaloneDayNamesShort->currentIndex();

    monthNamesShort->clear();
    monthNamesLong->clear();
    standaloneMonthNamesShort->clear();
    standaloneMonthNamesLong->clear();
    dayNamesLong->clear();
    dayNamesShort->clear();
    standaloneDayNamesLong->clear();
    standaloneDayNamesShort->clear();

    for (int i = 1; i <= 12; ++i)
        monthNamesShort->addItem(locale.monthName(i, QLocale::ShortFormat));
    monthNamesShort->setCurrentIndex(mns >= 0 ? mns : 0);
    for (int i = 1; i <= 12; ++i)
        monthNamesLong->addItem(locale.monthName(i, QLocale::LongFormat));
    monthNamesLong->setCurrentIndex(mnl >= 0 ? mnl : 0);

    for (int i = 1; i <= 12; ++i)
        standaloneMonthNamesShort->addItem(locale.standaloneMonthName(i, QLocale::ShortFormat));
    standaloneMonthNamesShort->setCurrentIndex(smns >= 0 ? smns : 0);
    for (int i = 1; i <= 12; ++i)
        standaloneMonthNamesLong->addItem(locale.standaloneMonthName(i, QLocale::LongFormat));
    standaloneMonthNamesLong->setCurrentIndex(smnl >= 0 ? smnl : 0);

    for (int i = 1; i <= 7; ++i)
        dayNamesLong->addItem(locale.dayName(i, QLocale::LongFormat));
    dayNamesLong->setCurrentIndex(dnl >= 0 ? dnl : 0);
    for (int i = 1; i <= 7; ++i)
        dayNamesShort->addItem(locale.dayName(i, QLocale::ShortFormat));
    dayNamesShort->setCurrentIndex(dns >= 0 ? dns : 0);

    for (int i = 1; i <= 7; ++i)
        standaloneDayNamesLong->addItem(locale.standaloneDayName(i, QLocale::LongFormat));
    standaloneDayNamesLong->setCurrentIndex(sdnl >= 0 ? sdnl : 0);
    for (int i = 1; i <= 7; ++i)
        standaloneDayNamesShort->addItem(locale.standaloneDayName(i, QLocale::ShortFormat));
    standaloneDayNamesShort->setCurrentIndex(sdns >= 0 ? sdns : 0);
}

void DateFormatsWidget::addItem(const QString &label, QWidget *w)
{
    QLabel *lbl = new QLabel(label);
    int row = layout->rowCount();
    layout->addWidget(lbl, row, 0);
    layout->addWidget(w, row, 1, 1, 2);
}

QLineEdit *DateFormatsWidget::addItem(const QString &label)
{
    QLineEdit *le = new QLineEdit;
    le->setReadOnly(true);
    addItem(label, le);
    return le;
}
