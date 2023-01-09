// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "diagramscene.h"
#include "diagramitem.h"
#include "commands.h"

#include <QAction>
#include <QDockWidget>
#include <QGraphicsView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QUndoView>

//! [0]
MainWindow::MainWindow()
{
    undoStack = new QUndoStack(this);
    diagramScene = new DiagramScene();

    const QBrush pixmapBrush(QPixmap(":/icons/cross.png").scaled(30, 30));
    diagramScene->setBackgroundBrush(pixmapBrush);
    diagramScene->setSceneRect(QRect(0, 0, 500, 500));

    createActions();
    createMenus();
    createToolBars();

    createUndoView();

    connect(diagramScene, &DiagramScene::itemMoved,
            this, &MainWindow::itemMoved);
    connect(diagramScene, &DiagramScene::selectionChanged,
            this, &MainWindow::updateActions);

    setWindowTitle("Undo Framework");
    QGraphicsView *view = new QGraphicsView(diagramScene);
    setCentralWidget(view);
    adjustSize();
}
//! [0]

//! [1]
void MainWindow::createUndoView()
{
    QDockWidget *undoDockWidget = new QDockWidget;
    undoDockWidget->setWindowTitle(tr("Command List"));
    undoDockWidget->setWidget(new QUndoView(undoStack));
    addDockWidget(Qt::RightDockWidgetArea, undoDockWidget);
}
//! [1]

//! [2]
void MainWindow::createActions()
{
    deleteAction = new QAction(QIcon(":/icons/remove.png"), tr("&Delete Item"), this);
    deleteAction->setShortcut(tr("Del"));
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteItem);
//! [2]

    addBoxAction = new QAction(QIcon(":/icons/rectangle.png"), tr("Add &Box"), this);
    addBoxAction->setShortcut(tr("Ctrl+O"));
    connect(addBoxAction, &QAction::triggered, this, &MainWindow::addBox);

    addTriangleAction = new QAction(QIcon(":/icons/triangle.png"), tr("Add &Triangle"), this);
    addTriangleAction->setShortcut(tr("Ctrl+T"));
    connect(addTriangleAction, &QAction::triggered, this, &MainWindow::addTriangle);

//! [5]
    undoAction = undoStack->createUndoAction(this, tr("&Undo"));
    undoAction->setIcon(QIcon(":/icons/undo.png"));
    undoAction->setShortcuts(QKeySequence::Undo);

    redoAction = undoStack->createRedoAction(this, tr("&Redo"));
    redoAction->setIcon(QIcon(":/icons/redo.png"));
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

//! [6]
    updateActions();
}

void MainWindow::updateActions()
{
    deleteAction->setEnabled(!diagramScene->selectedItems().isEmpty());
}
//! [6]

//! [7]
void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(exitAction);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);
    editMenu->addSeparator();
    editMenu->addAction(deleteAction);
//! [7]

    itemMenu = menuBar()->addMenu(tr("&Item"));
    itemMenu->addAction(addBoxAction);
    itemMenu->addAction(addTriangleAction);

//! [8]
    helpMenu = menuBar()->addMenu(tr("&About"));
    helpMenu->addAction(aboutAction);
}

void MainWindow::createToolBars()
{
    QToolBar *editToolBar = new QToolBar;
    editToolBar->addAction(undoAction);
    editToolBar->addAction(redoAction);
    editToolBar->addSeparator();
    editToolBar->addAction(deleteAction);
//! [8]

    QToolBar *itemToolBar = new QToolBar;
    itemToolBar->addAction(addBoxAction);
    itemToolBar->addAction(addTriangleAction);
//! [9]
    addToolBar(editToolBar);
    addToolBar(itemToolBar);
}
//! [9]

//! [11]
void MainWindow::itemMoved(DiagramItem *movedItem,
                           const QPointF &oldPosition)
{
    undoStack->push(new MoveCommand(movedItem, oldPosition));
}
//! [11]

//! [12]
void MainWindow::deleteItem()
{
    if (diagramScene->selectedItems().isEmpty())
        return;

    QUndoCommand *deleteCommand = new DeleteCommand(diagramScene);
    undoStack->push(deleteCommand);
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
