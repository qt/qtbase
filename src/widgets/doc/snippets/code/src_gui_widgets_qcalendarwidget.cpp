// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QCalendarWidget *calendar;
calendar->setGridVisible(true);
//! [0]


//! [1]
QCalendarWidget *calendar;
calendar->setGridVisible(true);
calendar->setMinimumDate(QDate(2006, 6, 19));
//! [1]


//! [2]
QCalendarWidget *calendar;
calendar->setGridVisible(true);
calendar->setMaximumDate(QDate(2006, 7, 3));
//! [2]


//! [3]
QCalendarWidget *calendar;

calendar->setDateRange(min, max);
//! [3]


//! [4]
QCalendarWidget *calendar;

calendar->setMinimumDate(min);
calendar->setMaximumDate(max);
//! [4]


//! [5]
QCalendarWidget *calendar;
calendar->setGridVisible(true);
//! [5]
