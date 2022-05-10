// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "controller.h"
#include "car_interface.h"

Controller::Controller(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    car = new org::example::Examples::CarInterface("org.example.CarExample", "/Car",
                           QDBusConnection::sessionBus(), this);
    startTimer(1000);
}

void Controller::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
    if (car->isValid())
        ui.label->setText("connected");
    else
        ui.label->setText("disconnected");
}

void Controller::on_accelerate_clicked()
{
    car->accelerate();
}

void Controller::on_decelerate_clicked()
{
    car->decelerate();
}

void Controller::on_left_clicked()
{
    car->turnLeft();
}

void Controller::on_right_clicked()
{
    car->turnRight();
}
