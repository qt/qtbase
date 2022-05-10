// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*!
    The main function for the string list model example. This creates and
    populates a model with values from a string list then displays the
    contents of the model using a QListView widget.
*/

#include <QAbstractItemModel>
#include <QApplication>
#include <QListView>

#include "model.h"

//! [0]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

// Unindented for quoting purposes:
//! [1]
QStringList numbers;
numbers << "One" << "Two" << "Three" << "Four" << "Five";

QAbstractItemModel *model = new StringListModel(numbers);
//! [0] //! [1] //! [2] //! [3]
QListView *view = new QListView;
//! [2]
view->setWindowTitle("View onto a string list model");
//! [4]
view->setModel(model);
//! [3] //! [4]

    model->insertRows(5, 7, QModelIndex());

    for (int row = 5; row < 12; ++row) {
        QModelIndex index = model->index(row, 0, QModelIndex());
        model->setData(index, QString::number(row+1));
    }

//! [5]
    view->show();
    return app.exec();
}
//! [5]
