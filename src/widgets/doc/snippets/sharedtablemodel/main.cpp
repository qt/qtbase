// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
  main.cpp

  A simple example that shows how a single model can be shared between
  multiple views.
*/

#include <QApplication>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QTableView>

#include "model.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    TableModel *model = new TableModel(4, 2, &app);

//! [0]
    QTableView *firstTableView = new QTableView;
    QTableView *secondTableView = new QTableView;
//! [0]

//! [1]
    firstTableView->setModel(model);
    secondTableView->setModel(model);
//! [1]

    firstTableView->horizontalHeader()->setModel(model);

    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 2; ++column) {
            QModelIndex index = model->index(row, column, QModelIndex());
            model->setData(index, QVariant(QString("(%1, %2)").arg(row).arg(column)));
        }
    }

//! [2]
    secondTableView->setSelectionModel(firstTableView->selectionModel());
//! [2]

    firstTableView->setWindowTitle("First table view");
    secondTableView->setWindowTitle("Second table view");
    firstTableView->show();
    secondTableView->show();
    return app.exec();
}
