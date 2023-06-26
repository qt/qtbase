// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ledwidget.h"

LEDWidget::LEDWidget(QWidget *parent)
    : QLabel(parent), onPixmap(":/ledon.png"), offPixmap(":/ledoff.png")
{
    setPixmap(offPixmap);
    flashTimer.setInterval(200);
    flashTimer.setSingleShot(true);
    connect(&flashTimer, &QTimer::timeout, this, &LEDWidget::extinguish);
};

void LEDWidget::extinguish()
{
    setPixmap(offPixmap);
}

void LEDWidget::flash()
{
    setPixmap(onPixmap);
    flashTimer.start();
}

