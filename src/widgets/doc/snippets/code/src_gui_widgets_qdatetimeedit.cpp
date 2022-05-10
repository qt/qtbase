// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QDateTimeEdit *dateEdit = new QDateTimeEdit(QDate::currentDate());
dateEdit->setMinimumDate(QDate::currentDate().addDays(-365));
dateEdit->setMaximumDate(QDate::currentDate().addDays(365));
dateEdit->setDisplayFormat("yyyy.MM.dd");
//! [0]


//! [1]
setDateTimeRange(min, max);
//! [1]


//! [2]
setMinimumDateTime(min);
setMaximumDateTime(max);
//! [2]


//! [3]
setDateRange(min, max);
//! [3]


//! [4]
setMinimumDate(min);
setMaximumDate(max);
//! [4]


//! [5]
setTimeRange(min, max);
//! [5]


//! [6]
setMinimumTime(min);
setMaximumTime(max);
//! [6]
