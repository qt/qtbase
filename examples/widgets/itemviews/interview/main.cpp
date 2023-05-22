// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "model.h"

#include <QApplication>
#include <QHeaderView>
#include <QListView>
#include <QSplitter>
#include <QTableView>
#include <QTreeView>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QSplitter page;

    QAbstractItemModel *data = new Model(1000, 10, &page);
    QItemSelectionModel *selections = new QItemSelectionModel(data);

    QTableView *table = new QTableView;
    table->setModel(data);
    table->setSelectionModel(selections);
    table->horizontalHeader()->setSectionsMovable(true);
    table->verticalHeader()->setSectionsMovable(true);
    // Set StaticContents to enable minimal repaints on resizes.
    table->viewport()->setAttribute(Qt::WA_StaticContents);
    page.addWidget(table);

    QTreeView *tree = new QTreeView;
    tree->setModel(data);
    tree->setSelectionModel(selections);
    tree->setUniformRowHeights(true);
    tree->header()->setStretchLastSection(false);
    tree->viewport()->setAttribute(Qt::WA_StaticContents);
    // Disable the focus rect to get minimal repaints when scrolling on Mac.
    tree->setAttribute(Qt::WA_MacShowFocusRect, false);
    page.addWidget(tree);

    QListView *list = new QListView;
    list->setModel(data);
    list->setSelectionModel(selections);
    list->setViewMode(QListView::IconMode);
    list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list->setAlternatingRowColors(false);
    list->viewport()->setAttribute(Qt::WA_StaticContents);
    list->setAttribute(Qt::WA_MacShowFocusRect, false);
    page.addWidget(list);

    page.setWindowIcon(QPixmap(":/images/interview.png"));
    page.setWindowTitle("Interview");
    page.show();

    return app.exec();
}
