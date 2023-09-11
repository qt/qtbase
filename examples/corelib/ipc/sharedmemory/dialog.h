// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QSharedMemory>

#include "ui_dialog.h"

//! [0]
class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);

public slots:
    void loadFromFile();
    void loadFromMemory();

private:
    void detach();

private:
    Ui::Dialog ui;
    QSharedMemory sharedMemory;
};
//! [0]

#endif // DIALOG_H

