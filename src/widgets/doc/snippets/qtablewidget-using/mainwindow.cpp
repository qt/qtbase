// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include <math.h>

#include "mainwindow.h"

MainWindow::MainWindow()
{
    QMenu *fileMenu = new QMenu(tr("&File"));

    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcut(tr("Ctrl+Q"));

    QMenu *itemsMenu = new QMenu(tr("&Items"));

    QAction *sumItemsAction = itemsMenu->addAction(tr("&Sum Items"));
    QAction *averageItemsAction = itemsMenu->addAction(tr("&Average Items"));

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(itemsMenu);

//! [0]
    tableWidget = new QTableWidget(12, 3, this);
//! [0]
    tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

//! [1]
    QTableWidgetItem *valuesHeaderItem = new QTableWidgetItem(tr("Values"));
    tableWidget->setHorizontalHeaderItem(0, valuesHeaderItem);
//! [1]
    valuesHeaderItem->setTextAlignment(Qt::AlignVCenter);
    QTableWidgetItem *squaresHeaderItem = new QTableWidgetItem(tr("Squares"));
    squaresHeaderItem->setIcon(QIcon(QPixmap(":/Images/squared.png")));
    squaresHeaderItem->setTextAlignment(Qt::AlignVCenter);
//! [2]
    QTableWidgetItem *cubesHeaderItem = new QTableWidgetItem(tr("Cubes"));
    cubesHeaderItem->setIcon(QIcon(QPixmap(":/Images/cubed.png")));
    cubesHeaderItem->setTextAlignment(Qt::AlignVCenter);
//! [2]
    tableWidget->setHorizontalHeaderItem(1, squaresHeaderItem);
    tableWidget->setHorizontalHeaderItem(2, cubesHeaderItem);

    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    connect(sumItemsAction, &QAction::triggered, this, &MainWindow::sumItems);
    connect(averageItemsAction, &QAction::triggered, this, &MainWindow::averageItems);

    setupTableItems();

    setCentralWidget(tableWidget);
    setWindowTitle(tr("Table Widget"));
}

void MainWindow::setupTableItems()
{
    for (int row = 0; row < tableWidget->rowCount()-1; ++row) {
        for (int column = 0; column < tableWidget->columnCount(); ++column) {
//! [3]
    QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(
        pow(row, column+1)));
    tableWidget->setItem(row, column, newItem);
//! [3]
        }
    }
    for (int column = 0; column < tableWidget->columnCount(); ++column) {
        QTableWidgetItem *newItem = new QTableWidgetItem;
        newItem->setFlags(Qt::ItemIsEnabled);
        tableWidget->setItem(tableWidget->rowCount()-1, column, newItem);
    }
}

void MainWindow::averageItems()
{
    const QList<QTableWidgetItem *> selected = tableWidget->selectedItems();
    int number = 0;
    double total = 0;

    for (QTableWidgetItem *item : selected) {
        bool ok;
        double value = item->text().toDouble(&ok);

        if (ok && !item->text().isEmpty()) {
            total += value;
            number++;
        }
    }
    if (number > 0)
        tableWidget->currentItem()->setText(QString::number(total/number));
}

void MainWindow::sumItems()
{
//! [4]
    const QList<QTableWidgetItem *> selected = tableWidget->selectedItems();
    int number = 0;
    double total = 0;

    for (QTableWidgetItem *item : selected) {
        bool ok;
        double value = item->text().toDouble(&ok);

        if (ok && !item->text().isEmpty()) {
            total += value;
            number++;
        }
    }
//! [4]
    if (number > 0)
        tableWidget->currentItem()->setText(QString::number(total));
}
