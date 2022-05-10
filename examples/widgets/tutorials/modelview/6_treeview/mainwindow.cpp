// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [Quoting ModelView Tutorial]
// modelview.cpp
#include "mainwindow.h"

#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , treeView(new QTreeView(this))
    , standardModel(new QStandardItemModel(this))
{
    setCentralWidget(treeView);

    QList<QStandardItem *> preparedRow = prepareRow("first", "second", "third");
    QStandardItem *item = standardModel->invisibleRootItem();
    // adding a row to the invisible root item produces a root element
    item->appendRow(preparedRow);

    QList<QStandardItem *> secondRow = prepareRow("111", "222", "333");
    // adding a row to an item starts a subtree
    preparedRow.first()->appendRow(secondRow);

    treeView->setModel(standardModel);
    treeView->expandAll();
}

QList<QStandardItem *> MainWindow::prepareRow(const QString &first,
                                              const QString &second,
                                              const QString &third) const
{
    return {new QStandardItem(first),
            new QStandardItem(second),
            new QStandardItem(third)};
}
//! [Quoting ModelView Tutorial]
