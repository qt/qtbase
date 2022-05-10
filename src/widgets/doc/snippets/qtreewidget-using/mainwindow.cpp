// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "mainwindow.h"

MainWindow::MainWindow()
{
    QMenu *fileMenu = new QMenu(tr("&File"));

    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcut(tr("Ctrl+Q"));

    QMenu *itemsMenu = new QMenu(tr("&Items"));

    insertAction = itemsMenu->addAction(tr("&Insert Item"));
    removeAction = itemsMenu->addAction(tr("&Remove Item"));
    removeAction->setEnabled(false);
    itemsMenu->addSeparator();
    ascendingAction = itemsMenu->addAction(tr("Sort in &Ascending Order"));
    descendingAction = itemsMenu->addAction(tr("Sort in &Descending Order"));
    autoSortAction = itemsMenu->addAction(tr("&Automatically Sort Items"));
    autoSortAction->setCheckable(true);
    itemsMenu->addSeparator();
    QAction *findItemsAction = itemsMenu->addAction(tr("&Find Items"));
    findItemsAction->setShortcut(tr("Ctrl+F"));

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(itemsMenu);

/*  For convenient quoting:
//! [0]
    QTreeWidget *treeWidget = new QTreeWidget(this);
//! [0]
*/
    treeWidget = new QTreeWidget(this);
//! [1]
    treeWidget->setColumnCount(2);
//! [1] //! [2]
    QStringList headers;
    headers << tr("Subject") << tr("Default");
    treeWidget->setHeaderLabels(headers);
//! [2]

    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    connect(ascendingAction, &QAction::triggered, this, &MainWindow::sortAscending);
    connect(autoSortAction, &QAction::triggered, this, &MainWindow::updateSortItems);
    connect(descendingAction, &QAction::triggered, this, &MainWindow::sortDescending);
    connect(findItemsAction, &QAction::triggered, this, &MainWindow::findItems);
    connect(insertAction, &QAction::triggered, this, &MainWindow::insertItem);
    connect(removeAction, &QAction::triggered, this, &MainWindow::removeItem);
    connect(treeWidget, &QTreeWidget::currentItemChanged,
            this, &MainWindow::updateMenus);

    setupTreeItems();
    updateMenus(treeWidget->currentItem());

    setCentralWidget(treeWidget);
    setWindowTitle(tr("Tree Widget"));
}

void MainWindow::setupTreeItems()
{
//! [3]
    QTreeWidgetItem *cities = new QTreeWidgetItem(treeWidget);
    cities->setText(0, tr("Cities"));
    QTreeWidgetItem *osloItem = new QTreeWidgetItem(cities);
    osloItem->setText(0, tr("Oslo"));
    osloItem->setText(1, tr("Yes"));
//! [3]

    (new QTreeWidgetItem(cities))->setText(0, tr("Stockholm"));
    (new QTreeWidgetItem(cities))->setText(0, tr("Helsinki"));
    (new QTreeWidgetItem(cities))->setText(0, tr("Copenhagen"));

//! [4] //! [5]
    QTreeWidgetItem *planets = new QTreeWidgetItem(treeWidget, cities);
//! [4]
    planets->setText(0, tr("Planets"));
//! [5]
    (new QTreeWidgetItem(planets))->setText(0, tr("Mercury"));
    (new QTreeWidgetItem(planets))->setText(0, tr("Venus"));

    QTreeWidgetItem *earthItem = new QTreeWidgetItem(planets);
    earthItem->setText(0, tr("Earth"));
    earthItem->setText(1, tr("Yes"));

    (new QTreeWidgetItem(planets))->setText(0, tr("Mars"));
    (new QTreeWidgetItem(planets))->setText(0, tr("Jupiter"));
    (new QTreeWidgetItem(planets))->setText(0, tr("Saturn"));
    (new QTreeWidgetItem(planets))->setText(0, tr("Uranus"));
    (new QTreeWidgetItem(planets))->setText(0, tr("Neptune"));
    (new QTreeWidgetItem(planets))->setText(0, tr("Pluto"));
}

void MainWindow::findItems()
{
    QString itemText = QInputDialog::getText(this, tr("Find Items"),
        tr("Text to find (including wildcards):"));

    if (itemText.isEmpty())
        return;

    const QList<QTreeWidgetItem *> items = treeWidget->selectedItems();
    for (QTreeWidgetItem *item : items)
        item->setSelected(false);

//! [7]
    const QList<QTreeWidgetItem *> found = treeWidget->findItems(
        itemText, Qt::MatchWildcard);

    for (QTreeWidgetItem *item : found) {
        item->setSelected(true);
        // Show the item->text(0) for each item.
    }
//! [7]
}

void MainWindow::insertItem()
{
    QTreeWidgetItem *currentItem = treeWidget->currentItem();

    if (!currentItem)
        return;

    QString itemText = QInputDialog::getText(this, tr("Insert Item"),
        tr("Input text for the new item:"));

    if (itemText.isEmpty())
        return;

//! [8]
    QTreeWidgetItem *parent = currentItem->parent();
    QTreeWidgetItem *newItem;
    if (parent)
        newItem = new QTreeWidgetItem(parent, treeWidget->currentItem());
    else
//! [8] //! [9]
        newItem = new QTreeWidgetItem(treeWidget, treeWidget->currentItem());
//! [9]

    newItem->setText(0, itemText);
}

void MainWindow::removeItem()
{
    QTreeWidgetItem *currentItem = treeWidget->currentItem();

    if (!currentItem)
        return;

//! [10]
    QTreeWidgetItem *parent = currentItem->parent();
    int index;

    if (parent) {
        index = parent->indexOfChild(treeWidget->currentItem());
        delete parent->takeChild(index);
    } else {
        index = treeWidget->indexOfTopLevelItem(treeWidget->currentItem());
        delete treeWidget->takeTopLevelItem(index);
//! [10] //! [11]
    }
//! [11]
}

void MainWindow::sortAscending()
{
    treeWidget->sortItems(0, Qt::AscendingOrder);
}

void MainWindow::sortDescending()
{
    treeWidget->sortItems(0, Qt::DescendingOrder);
}

void MainWindow::updateMenus(QTreeWidgetItem *current)
{
    insertAction->setEnabled(current != 0);
    removeAction->setEnabled(current != 0);
}

void MainWindow::updateSortItems()
{
    ascendingAction->setEnabled(!autoSortAction->isChecked());
    descendingAction->setEnabled(!autoSortAction->isChecked());

    treeWidget->setSortingEnabled(autoSortAction->isChecked());
}
