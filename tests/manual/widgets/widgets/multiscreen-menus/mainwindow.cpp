// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui/QtEvents>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setStyleSheet("QMenu { menu-scrollable: 0 }");

    auto *mb = new QMenuBar(this);
    setMenuBar(mb);

    auto *m = new QMenu(mb);
    m->setTitle("&Menu");
    m->setTearOffEnabled(true);

    for (int i = 0; i < 80; ++i)
        m->addAction("Menu Item #" + QString::number(i));

    mb->addMenu(m);

    ui->menuButton->setMenu(m);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::contextMenuEvent(QContextMenuEvent *e)
{
    const auto *mb = menuBar();
    mb->actions().first()->menu()->popup(mb->mapToGlobal(e->pos()));
}
