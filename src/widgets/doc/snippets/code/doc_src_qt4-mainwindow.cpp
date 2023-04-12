// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
//! [0]


//! [1]
fileToolbar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
addToolBar(Qt::TopToolBarArea, fileToolbar);
//! [1]


//! [2]
setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
//! [2]


//! [3]
QWidget *centralWidget = new QWidget(this);
setCentralWidget(centralWidget);
//! [3]
