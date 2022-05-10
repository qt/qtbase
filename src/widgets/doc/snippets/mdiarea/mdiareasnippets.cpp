// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

void mainWindowExample()
{
    QMdiArea *mdiArea = new QMdiArea;
//! [0]
    QMainWindow *mainWindow = new QMainWindow;
    mainWindow->setCentralWidget(mdiArea);
//! [0]

    mdiArea->addSubWindow(new QPushButton("Push Me Now!"));

    mainWindow->show();
}

void addingSubWindowsExample()
{
    QWidget *internalWidget1 = new QWidget;
    QWidget *internalWidget2 = new QWidget;

//! [1]
    QMdiArea mdiArea;
    QMdiSubWindow *subWindow1 = new QMdiSubWindow;
    subWindow1->setWidget(internalWidget1);
    subWindow1->setAttribute(Qt::WA_DeleteOnClose);
    mdiArea.addSubWindow(subWindow1);

    QMdiSubWindow *subWindow2 =
        mdiArea.addSubWindow(internalWidget2);

//! [1]
    subWindow1->show();
    subWindow2->show();

    mdiArea.show();
}
