// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "controller.h"

using org::example::Examples::CarInterface;

Controller::Controller(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    car = new CarInterface("org.example.CarExample", "/Car", QDBusConnection::sessionBus(), this);

    connect(ui.accelerate, &QPushButton::clicked, car, &CarInterface::accelerate);
    connect(ui.decelerate, &QPushButton::clicked, car, &CarInterface::decelerate);
    connect(ui.left, &QPushButton::clicked, car, &CarInterface::turnLeft);
    connect(ui.right, &QPushButton::clicked, car, &CarInterface::turnRight);

    startTimer(1000);
}

void Controller::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
    ui.label->setText(car->isValid() ? tr("connected") : tr("disconnected"));
}
