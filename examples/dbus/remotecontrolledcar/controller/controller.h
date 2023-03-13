// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "ui_controller.h"
#include "car_interface.h"

class Controller : public QWidget
{
    Q_OBJECT

public:
    explicit Controller(QWidget *parent = nullptr);

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    Ui::Controller ui;
    org::example::Examples::CarInterface *car;
};

#endif

