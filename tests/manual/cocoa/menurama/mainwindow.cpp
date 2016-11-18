/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the qtbase module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "menuramaapplication.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
