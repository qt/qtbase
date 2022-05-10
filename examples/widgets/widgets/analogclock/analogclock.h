// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ANALOGCLOCK_H
#define ANALOGCLOCK_H

#include <QWidget>

//! [0]
class AnalogClock : public QWidget
{
    Q_OBJECT

public:
    AnalogClock(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};
//! [0]

#endif
