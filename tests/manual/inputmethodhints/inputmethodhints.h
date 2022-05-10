// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef INPUTMETHODHINTS_H
#define INPUTMETHODHINTS_H

#include <QMainWindow>
#include "ui_inputmethodhints.h"

class inputmethodhints : public QMainWindow
{
    Q_OBJECT

public:
    inputmethodhints(QWidget *parent = nullptr);
    ~inputmethodhints();

public slots:
    void checkboxChanged(int);

private:
    Ui::MainWindow ui;
};

#endif // INPUTMETHODHINTS_H
