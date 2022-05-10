// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIGITALCLOCK_H
#define DIGITALCLOCK_H

#include <QLCDNumber>

//! [0]
class DigitalClock : public QLCDNumber
{
    Q_OBJECT

public:
    DigitalClock(QWidget *parent = nullptr);

private slots:
    void showTime();
};
//! [0]

#endif
