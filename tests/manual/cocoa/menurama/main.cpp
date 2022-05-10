// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mainwindow.h"
#include "menuramaapplication.h"

#include <QtGui/QAction>
#include <QtWidgets/QMenu>

int main(int argc, char *argv[])
{
    MenuramaApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    auto *dockMenu = new QMenu();
    dockMenu->setAsDockMenu();
    dockMenu->addAction(QLatin1String("New Window"), [=] {
        auto *w = new MainWindow;
        w->setAttribute(Qt::WA_DeleteOnClose, true);
        w->show();
    });
    auto *disabledAction = dockMenu->addAction(QLatin1String("Disabled Item"), [=] {
        qDebug() << "Should not happen!";
        Q_UNREACHABLE();
    });
    disabledAction->setEnabled(false);
    dockMenu->addAction(QLatin1String("Last Item Before Separator"), [=] {
        qDebug() << "Last Item triggered";
    });
    auto *hiddenAction = dockMenu->addAction(QLatin1String("Invisible Item (FIXME rdar:39615815)"), [=] {
        qDebug() << "Should not happen!";
        Q_UNREACHABLE();
    });
    hiddenAction->setVisible(false);
    dockMenu->addSeparator();
    auto *toolsMenu = dockMenu->addMenu(QLatin1String("Menurama Tools"));
    toolsMenu->addAction(QLatin1String("Hammer"), [=] {
        qDebug() << "Bang! Bang!";
    });
    toolsMenu->addAction(QLatin1String("Wrench"), [=] {
        qDebug() << "Clang! Clang!";
    });
    toolsMenu->addAction(QLatin1String("Screwdriver"), [=] {
        qDebug() << "Squeak! Squeak!";
    });

    MainWindow w;
    w.show();

    return a.exec();
}
