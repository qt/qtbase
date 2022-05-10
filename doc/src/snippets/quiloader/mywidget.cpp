// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>
#include <QtUiTools>

#include "mywidget.h"

//! [0]
MyWidget::MyWidget(QWidget *parent)
    : QWidget(parent)
{
    QUiLoader loader;
    QFile file(":/forms/myform.ui");
    file.open(QFile::ReadOnly);
    QWidget *myWidget = loader.load(&file, this);
    file.close();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(myWidget);
    setLayout(layout);
}
//! [0]
