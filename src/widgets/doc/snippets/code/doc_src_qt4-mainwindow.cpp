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


//! [4]
QPopupMenu *fileMenu = new QPopupMenu(this);
openAction->addTo(fileMenu);
saveAction->addTo(fileMenu);
...
menuBar()->insertItem(tr("&File"), fileMenu);
//! [4]


//! [5]
QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
fileMenu->addAction(openAction);
fileMenu->addAction(saveAction);
...
//! [5]


//! [6]
QToolBar *fileTools = new QToolBar(this, "file toolbar");
openAction->addTo(fileTools);
saveAction->addTo(fileTools);
...
//! [6]


//! [7]
QToolBar *fileTools = addToolBar(tr("File Tool Bar"));
fileTools->addAction(openAction);
fileTools->addAction(saveAction);
...
//! [7]


//! [8]
QDockWidget *dockWidget = new QDockWidget(this);
mainWin->moveDockWidget(dockWidget, Qt::DockLeft);
//! [8]


//! [9]
QDockWidget *dockWidget = new QDockWidget(mainWindow);
mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
//! [9]
