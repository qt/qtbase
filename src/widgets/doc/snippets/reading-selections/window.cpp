// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
  window.cpp

  A minimal subclass of QTableView with slots to allow the selection model
  to be monitored.
*/

#include <QAbstractItemModel>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>

#include "model.h"
#include "window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Selected Items in a Table Model");

    model = new TableModel(8, 4, this);

    table = new QTableView(this);
    table->setModel(model);

    QMenu *actionMenu = new QMenu(tr("&Actions"), this);
    QAction *fillAction = actionMenu->addAction(tr("&Fill Selection"));
    QAction *clearAction = actionMenu->addAction(tr("&Clear Selection"));
    QAction *selectAllAction = actionMenu->addAction(tr("&Select All"));
    menuBar()->addMenu(actionMenu);

    connect(fillAction, &QAction::triggered, this, &MainWindow::fillSelection);
    connect(clearAction, &QAction::triggered, this, &MainWindow::clearSelection);
    connect(selectAllAction, &QAction::triggered, this, &MainWindow::selectAll);

    selectionModel = table->selectionModel();

    statusBar();
    setCentralWidget(table);
}

void MainWindow::fillSelection()
{
//! [0]
    const QModelIndexList indexes = selectionModel->selectedIndexes();

    for (const QModelIndex &index : indexes) {
        QString text = QString("(%1,%2)").arg(index.row()).arg(index.column());
        model->setData(index, text);
    }
//! [0]
}

void MainWindow::clearSelection()
{
    const QModelIndexList indexes = selectionModel->selectedIndexes();

    for (const QModelIndex &index : indexes)
        model->setData(index, QString());
}

void MainWindow::selectAll()
{
//! [1]
    QModelIndex parent = QModelIndex();
//! [1] //! [2]
    QModelIndex topLeft = model->index(0, 0, parent);
    QModelIndex bottomRight = model->index(model->rowCount(parent)-1,
        model->columnCount(parent)-1, parent);
//! [2]

//! [3]
    QItemSelection selection(topLeft, bottomRight);
    selectionModel->select(selection, QItemSelectionModel::Select);
//! [3]
}
