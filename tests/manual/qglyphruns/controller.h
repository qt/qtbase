// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QWidget>

namespace Ui {
class Controller;
}

class Controller : public QWidget
{
    Q_OBJECT

public:
    explicit Controller(QWidget *parent = nullptr);
    ~Controller();

private slots:
    void updateViews();
    void updateRange();

private:
    Ui::Controller *ui;
};

#endif // CONTROLLER_H
