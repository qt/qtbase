/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
#include "diagramscene.h"
#include "diagramitem.h"
#include "commands.h"

#include <QAction>
#include <QGraphicsView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QUndoView>

//! [0]
MainWindow::MainWindow()
{
    undoStack = new QUndoStack(this);

    createActions();
    createMenus();

    createUndoView();

    diagramScene = new DiagramScene();
    QBrush pixmapBrush(QPixmap(":/images/cross.png").scaled(30, 30));
    diagramScene->setBackgroundBrush(pixmapBrush);
    diagramScene->setSceneRect(QRect(0, 0, 500, 500));

    connect(diagramScene, &DiagramScene::itemMoved,
            this, &MainWindow::itemMoved);

    setWindowTitle("Undo Framework");
    QGraphicsView *view = new QGraphicsView(diagramScene);
    setCentralWidget(view);
    resize(700, 500);
}
//! [0]

//! [1]
void MainWindow::createUndoView()
{
    undoView = new QUndoView(undoStack);
    undoView->setWindowTitle(tr("Command List"));
    undoView->show();
    undoView->setAttribute(Qt::WA_QuitOnClose, false);
}
//! [1]

//! [2]
void MainWindow::createActions()
{
    deleteAction = new QAction(tr("&Delete Item"), this);
    deleteAction->setShortcut(tr("Del"));
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteItem);
//! [2] //! [3]

//! [3] //! [4]
    addBoxAction = new QAction(tr("Add &Box"), this);
//! [4]
    addBoxAction->setShortcut(tr("Ctrl+O"));
    connect(addBoxAction, &QAction::triggered, this, &MainWindow::addBox);

    addTriangleAction = new QAction(tr("Add &Triangle"), this);
    addTriangleAction->setShortcut(tr("Ctrl+T"));
    connect(addTriangleAction, &QAction::triggered, this, &MainWindow::addTriangle);

//! [5]
    undoAction = undoStack->createUndoAction(this, tr("&Undo"));
    undoAction->setShortcuts(QKeySequence::Undo);

    redoAction = undoStack->createRedoAction(this, tr("&Redo"));
    redoAction->setShortcuts(QKeySequence::Redo);
//! [5]

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcuts(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    aboutAction = new QAction(tr("&About"), this);
    QList<QKeySequence> aboutShortcuts;
    aboutShortcuts << tr("Ctrl+A") << tr("Ctrl+B");
    aboutAction->setShortcuts(aboutShortcuts);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);
}

//! [6]
void MainWindow::createMenus()
{
//! [6]
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(exitAction);

//! [7]
    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);
    editMenu->addSeparator();
    editMenu->addAction(deleteAction);
    connect(editMenu, &QMenu::aboutToShow,
            this, &MainWindow::itemMenuAboutToShow);
    connect(editMenu, &QMenu::aboutToHide,
            this, &MainWindow::itemMenuAboutToHide);

//! [7]
    itemMenu = menuBar()->addMenu(tr("&Item"));
    itemMenu->addAction(addBoxAction);
    itemMenu->addAction(addTriangleAction);

    helpMenu = menuBar()->addMenu(tr("&About"));
    helpMenu->addAction(aboutAction);
//! [8]
}
//! [8]

//! [9]
void MainWindow::itemMoved(DiagramItem *movedItem,
                           const QPointF &oldPosition)
{
    undoStack->push(new MoveCommand(movedItem, oldPosition));
}
//! [9]

//! [10]
void MainWindow::deleteItem()
{
    if (diagramScene->selectedItems().isEmpty())
        return;

    QUndoCommand *deleteCommand = new DeleteCommand(diagramScene);
    undoStack->push(deleteCommand);
}
//! [10]

//! [11]
void MainWindow::itemMenuAboutToHide()
{
    deleteAction->setEnabled(true);
}
//! [11]

//! [12]
void MainWindow::itemMenuAboutToShow()
{
    deleteAction->setEnabled(!diagramScene->selectedItems().isEmpty());
}
//! [12]

//! [13]
void MainWindow::addBox()
{
    QUndoCommand *addCommand = new AddCommand(DiagramItem::Box, diagramScene);
    undoStack->push(addCommand);
}
//! [13]

//! [14]
void MainWindow::addTriangle()
{
    QUndoCommand *addCommand = new AddCommand(DiagramItem::Triangle,
                                              diagramScene);
    undoStack->push(addCommand);
}
//! [14]

//! [15]
void MainWindow::about()
{
    QMessageBox::about(this, tr("About Undo"),
                       tr("The <b>Undo</b> example demonstrates how to "
                          "use Qt's undo framework."));
}
//! [15]
