// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "menuramaapplication.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto *a = ui->menuStuff->addAction("Enabled Submenu (QTBUG-63172)");
    auto *qtbug63172_Menu = new QMenu;
    qtbug63172_Menu->addAction("We're Good!");
    a->setMenu(qtbug63172_Menu);

    startTimer(1000);

    connect(ui->menuAfter_aboutToShow, &QMenu::aboutToShow, [=] {
        menuApp->populateMenu(ui->menuAfter_aboutToShow, true /*clear*/);
    });

    connect(ui->menuDynamic_Stuff, &QMenu::aboutToShow, [=] {
        menuApp->addDynMenu(QLatin1String("Menu Added After aboutToShow()"), ui->menuDynamic_Stuff);

        const QLatin1String itemTitle = QLatin1String("Disabled Item Added After aboutToShow()");
        if (QAction *a = menuApp->findAction(itemTitle, ui->menuDynamic_Stuff))
            ui->menuDynamic_Stuff->removeAction(a);
        QAction *a = ui->menuDynamic_Stuff->addAction(itemTitle);
        a->setEnabled(false);
    });

    connect(ui->pushButton, &QPushButton::clicked, [=] {
        menuApp->populateMenu(ui->menuOn_Click, true /*clear*/);
    });

    connect(ui->addManyButton, &QPushButton::clicked, [=] {
        QMenu *menu = new QMenu(QLatin1String("Many More ") +
                                QString::number(ui->menuBar->actions().count()));
        ui->menuBar->insertMenu(ui->menuDynamic_Stuff->menuAction(), menu);
        for (int i = 0; i < 2000; i++) {
            auto *action = menu->addAction(QLatin1String("Item ") + QString::number(i));
            if (i & 0x1)
                action->setEnabled(false);
            if (i & 0x2)
                action->setVisible(false);
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::timerEvent(QTimerEvent *)
{
    menuApp->populateMenu(ui->menuPopulated_by_Timer, true /*clear*/);
    menuApp->addDynMenu(QLatin1String("Added by Timer"), ui->menuDynamic_Stuff);
}

void MainWindow::enableStuffMenu(bool enable)
{
    ui->menuStuff->setEnabled(enable);
}

void MainWindow::on_actionQuit_triggered()
{
    menuApp->exit();
}
