// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [quoting modelview_a]
#include "mainwindow.h"

#include <QTreeView>
#include <QStandardItemModel>
#include <QItemSelectionModel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , treeView(new QTreeView(this))
    , standardModel(new QStandardItemModel(this))
{
    setCentralWidget(treeView);
    auto *rootNode = standardModel->invisibleRootItem();


    // defining a couple of items
    auto *americaItem = new QStandardItem("America");
    auto *mexicoItem =  new QStandardItem("Canada");
    auto *usaItem =     new QStandardItem("USA");
    auto *bostonItem =  new QStandardItem("Boston");
    auto *europeItem =  new QStandardItem("Europe");
    auto *italyItem =   new QStandardItem("Italy");
    auto *romeItem =    new QStandardItem("Rome");
    auto *veronaItem =  new QStandardItem("Verona");

    // building up the hierarchy
    rootNode->    appendRow(americaItem);
    rootNode->    appendRow(europeItem);
    americaItem-> appendRow(mexicoItem);
    americaItem-> appendRow(usaItem);
    usaItem->     appendRow(bostonItem);
    europeItem->  appendRow(italyItem);
    italyItem->   appendRow(romeItem);
    italyItem->   appendRow(veronaItem);

    // register the model
    treeView->setModel(standardModel);
    treeView->expandAll();

    // selection changes shall trigger a slot
    QItemSelectionModel *selectionModel = treeView->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            this, &MainWindow::selectionChangedSlot);
}
//! [quoting modelview_a]

//------------------------------------------------------------------------------------

//! [quoting modelview_b]
void MainWindow::selectionChangedSlot(const QItemSelection & /*newSelection*/, const QItemSelection & /*oldSelection*/)
{
    // get the text of the selected item
    const QModelIndex index = treeView->selectionModel()->currentIndex();
    QString selectedText = index.data(Qt::DisplayRole).toString();
    // find out the hierarchy level of the selected item
    int hierarchyLevel = 1;
    QModelIndex seekRoot = index;
    while (seekRoot.parent().isValid()) {
        seekRoot = seekRoot.parent();
        hierarchyLevel++;
    }
    QString showString = QString("%1, Level %2").arg(selectedText)
                         .arg(hierarchyLevel);
    setWindowTitle(showString);
}
//! [quoting modelview_b]


