// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "glwidget.h"

MainWindow::MainWindow()
{
    setMinimumSize(800, 400);
    GLWidget *glwidget1 = new GLWidget(this);
    glwidget1->resize(400, 400);

    GLWidget *glwidget2 = new GLWidget(this);
    glwidget2->resize(400, 400);
    glwidget2->move(400, 0);
}
