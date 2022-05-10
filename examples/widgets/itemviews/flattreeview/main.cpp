// Copyright (C) 2017 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
  main.cpp

  A simple example that shows a multi-column list using QTreeView.
  The data is not a tree, so the first column was made movable.
*/

#include <QApplication>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTreeView>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QStandardItemModel model(4, 2);
    QTreeView treeView;
    treeView.setModel(&model);
    treeView.setRootIsDecorated(false);
    treeView.header()->setFirstSectionMovable(true);
    treeView.header()->setStretchLastSection(true);

    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 2; ++column) {
            QModelIndex index = model.index(row, column, QModelIndex());
            model.setData(index, QVariant((row + 1) * (column + 1)));
        }
    }

    treeView.setWindowTitle(QObject::tr("Flat Tree View"));
    treeView.show();
    return app.exec();
}
