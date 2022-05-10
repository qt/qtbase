// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QDockWidget>
#include <QStatusBar>
#include <QSpinBox>
#include <QAction>

#include "../shared/shared.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QMainWindow mainWindow;

    mainWindow.setCentralWidget(new StaticWidget());
    mainWindow.setStatusBar(new QStatusBar());

    QDockWidget *dockWidget = new QDockWidget();
    dockWidget->setWidget(new StaticWidget());
    mainWindow.addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    QToolBar *toolBar = new QToolBar();

    toolBar->addWidget(new StaticWidget())->setVisible(true);;

    toolBar->addWidget(new QSpinBox())->setVisible(true);;
    mainWindow.addToolBar(toolBar);

    mainWindow.resize(600, 400);
    mainWindow.show();

    return app.exec();
}
