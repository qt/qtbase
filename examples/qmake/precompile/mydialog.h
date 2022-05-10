// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MYDIALOG_H
#define MYDIALOG_H

#include "ui_mydialog.h"

class MyDialog : public QDialog, public Ui::MyDialog
{
    Q_OBJECT

public:
    MyDialog(QWidget *parent = nullptr);
};

#endif
