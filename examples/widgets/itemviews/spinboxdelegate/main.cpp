// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
  main.cpp

  A simple example that shows how a view can use a custom delegate to edit
  data obtained from a model.
*/

#include "delegate.h"

#include <QApplication>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTableView>

//! [0]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QStandardItemModel model(4, 2);
    QTableView tableView;
    tableView.setModel(&model);

    SpinBoxDelegate delegate;
    tableView.setItemDelegate(&delegate);
//! [0]

    tableView.horizontalHeader()->setStretchLastSection(true);

//! [1]
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 2; ++column) {
            QModelIndex index = model.index(row, column, QModelIndex());
            model.setData(index, QVariant((row + 1) * (column + 1)));
        }
//! [1] //! [2]
    }
//! [2]

//! [3]
    tableView.setWindowTitle(QObject::tr("Spin Box Delegate"));
    tableView.show();
    return app.exec();
}
//! [3]
