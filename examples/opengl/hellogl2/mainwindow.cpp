// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "window.h"
#include <QApplication>
#include <QKeySequence>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>

MainWindow::MainWindow()
{
#ifndef Q_OS_WASM
    QMenu *menuWindow = menuBar()->addMenu(tr("&Window"));
    menuWindow->addAction(tr("Add new"), QKeySequence(Qt::CTRL | Qt::Key_N),
                          this, &MainWindow::onAddNew);
    menuWindow->addAction(tr("Quit"), QKeySequence(Qt::CTRL | Qt::Key_Q),
                          qApp, QApplication::closeAllWindows);
#endif
    onAddNew();
}

void MainWindow::onAddNew()
{
    if (!centralWidget())
        setCentralWidget(new Window);
    else
        QMessageBox::information(this, tr("Cannot Add New Window"),
                                 tr("Already occupied. Undock first."));
}
