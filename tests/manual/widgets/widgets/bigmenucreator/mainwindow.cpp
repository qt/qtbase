/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

bool MainWindow::newMenubar = false;
bool MainWindow::parentlessMenubar = false;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    const int level = 3;
    QMenuBar *mb;
    if (newMenubar)
        mb = new QMenuBar(parentlessMenubar ? nullptr : this);
    else
        mb = ui->menuBar;
    populateMenu(mb, level);
    if (newMenubar)
        setMenuBar(mb);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// We do all the permutations on the following 3 operations:
//
//      A: Add action to parent menu
//      P: Populate the submenu
//      S: Set action submenu
//
// Recursing on menu population gives more combinations of
// creation and insertions.

void MainWindow::populateMenu(QWidget *menu, int level)
{
    if (level > 0) {
        --level;
        doAPS(menu, level);
        doASP(menu, level);
        doPAS(menu, level);
        doSPA(menu, level);
        doSAP(menu, level);
        doPSA(menu, level);
    } else {
        static int itemCounter = 0;
        static const char *sym[] = { "Foo", "Bar", "Baz", "Quux" };
        for (uint i = 0; i < sizeof(sym) / sizeof(sym[0]); i++) {
            QString title = QString::fromLatin1("%1 Item %2").arg(QLatin1String(sym[i])).arg(itemCounter);
            menu->addAction(new QAction(title));
        }
        ++itemCounter;
    }
}

void MainWindow::doAPS(QWidget *menu, int level)
{
    auto *action = new QAction("A P S");
    menu->addAction(action);
    auto *submenu = new QMenu;
    populateMenu(submenu, level);
    action->setMenu(submenu);
}

void MainWindow::doASP(QWidget *menu, int level)
{
    auto *action = new QAction("A S P");
    menu->addAction(action);
    auto *submenu = new QMenu;
    action->setMenu(submenu);
    populateMenu(submenu, level);
}

void MainWindow::doPAS(QWidget *menu, int level)
{
    auto *submenu = new QMenu;
    populateMenu(submenu, level);
    auto *action = new QAction("P A S");
    menu->addAction(action);
    action->setMenu(submenu);
}

void MainWindow::doSPA(QWidget *menu, int level)
{
    auto *action = new QAction("S P A");
    auto *submenu = new QMenu;
    action->setMenu(submenu);
    populateMenu(submenu, level);
    menu->addAction(action);
}

void MainWindow::doSAP(QWidget *menu, int level)
{
    auto *action = new QAction("S A P");
    auto *submenu = new QMenu;
    action->setMenu(submenu);
    menu->addAction(action);
    populateMenu(submenu, level);
}

void MainWindow::doPSA(QWidget *menu, int level)
{
    auto *action = new QAction("P S A");
    auto *submenu = new QMenu;
    populateMenu(submenu, level);
    action->setMenu(submenu);
    menu->addAction(action);
}
