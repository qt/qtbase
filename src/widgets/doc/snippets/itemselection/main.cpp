// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
  main.cpp

  A simple example that shows how selections can be used directly on a model.
  It shows the result of some selections made using a table view.
*/

#include <QApplication>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QTableView>

#include "model.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

//! [0]
    TableModel *model = new TableModel(8, 4, &app);

    QTableView *table = new QTableView(0);
    table->setModel(model);

    QItemSelectionModel *selectionModel = table->selectionModel();
//! [0] //! [1]
    QModelIndex topLeft;
    QModelIndex bottomRight;

    topLeft = model->index(0, 0, QModelIndex());
    bottomRight = model->index(5, 2, QModelIndex());
//! [1]

//! [2]
    QItemSelection selection(topLeft, bottomRight);
    selectionModel->select(selection, QItemSelectionModel::Select);
//! [2]

//! [3]
    QItemSelection toggleSelection;

    topLeft = model->index(2, 1, QModelIndex());
    bottomRight = model->index(7, 3, QModelIndex());
    toggleSelection.select(topLeft, bottomRight);

    selectionModel->select(toggleSelection, QItemSelectionModel::Toggle);
//! [3]

//! [4]
    QItemSelection columnSelection;

    topLeft = model->index(0, 1, QModelIndex());
    bottomRight = model->index(0, 2, QModelIndex());

    columnSelection.select(topLeft, bottomRight);

    selectionModel->select(columnSelection,
        QItemSelectionModel::Select | QItemSelectionModel::Columns);

    QItemSelection rowSelection;

    topLeft = model->index(0, 0, QModelIndex());
    bottomRight = model->index(1, 0, QModelIndex());

    rowSelection.select(topLeft, bottomRight);

    selectionModel->select(rowSelection,
        QItemSelectionModel::Select | QItemSelectionModel::Rows);
//! [4]

    table->setWindowTitle("Selected items in a table model");
    table->show();
    table->resize(460, 280);
    return app.exec();
}
