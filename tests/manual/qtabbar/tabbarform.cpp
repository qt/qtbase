// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include "tabbarform.h"

TabBarForm::TabBarForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabBarForm)
{
    ui->setupUi(this);
}

TabBarForm::~TabBarForm()
{
    delete ui;
}
