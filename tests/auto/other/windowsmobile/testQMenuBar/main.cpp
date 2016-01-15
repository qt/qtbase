/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtTest/QtTest>
#include <QtCore/QDate>
#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtGui>
#include <windows.h>

int main(int argc, char * argv[])
{
    QList<QWidget*> widgets;
    QApplication app(argc, argv);

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Test");
    QMenu *fileMenu = mainWindow.menuBar()->addMenu("File");
    QMenu *editMenu = mainWindow.menuBar()->addMenu("Edit");
    QMenu *viewMenu = mainWindow.menuBar()->addMenu("View");
    QMenu *toolsMenu = mainWindow.menuBar()->addMenu("Tools");
    QMenu *optionsMenu = mainWindow.menuBar()->addMenu("Options");
    QMenu *helpMenu = mainWindow.menuBar()->addMenu("Help");

    qApp->processEvents();

    fileMenu->addAction("Open");
    QAction *close = fileMenu->addAction("Close");
    fileMenu->addSeparator();
    fileMenu->addAction("Exit");

    close->setEnabled(false);

    editMenu->addAction("Cut");
    editMenu->addAction("Pase");
    editMenu->addAction("Copy");
    editMenu->addSeparator();
    editMenu->addAction("Find");

    viewMenu->addAction("Hide");
    viewMenu->addAction("Show");
    viewMenu->addAction("Explore");
    QAction *visible = viewMenu->addAction("Visible");
    visible->setCheckable(true);
    visible->setChecked(true);

    toolsMenu->addMenu("Hammer");
    toolsMenu->addMenu("Caliper");
    toolsMenu->addMenu("Helm");

    optionsMenu->addMenu("Settings");
    optionsMenu->addMenu("Standard");
    optionsMenu->addMenu("Extended");

    QMenu *subMenu = helpMenu->addMenu("Help");
    subMenu->addAction("Index");
    subMenu->addSeparator();
    subMenu->addAction("Vodoo Help");
    helpMenu->addAction("Contens");
    helpMenu->addSeparator();
    helpMenu->addAction("About");

    QToolBar toolbar;
    mainWindow.addToolBar(&toolbar);
    toolbar.addAction(QIcon(qApp->style()->standardPixmap(QStyle::SP_FileIcon)), QString("textAction"));

    QTextEdit textEdit;
    mainWindow.setCentralWidget(&textEdit);

    mainWindow.showMaximized();

    app.exec();
}
