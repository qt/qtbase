/****************************************************************************
**
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QConcatenateTablesProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QTreeView>

static void prepareModel(const QString &prefix, QStandardItemModel *model)
{
    for (int row = 0; row < model->rowCount(); ++row) {
        for (int column = 0; column < model->columnCount(); ++column) {
            QStandardItem *item = new QStandardItem(prefix + QString(" %1,%2").arg(row).arg(column));
            item->setDragEnabled(true);
            item->setDropEnabled(true);
            model->setItem(row, column, item);
        }
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QStandardItemModel firstModel(4, 4);
    prepareModel("First", &firstModel);
    QStandardItemModel secondModel(2, 2);

    QConcatenateTablesProxyModel proxy;
    proxy.addSourceModel(&firstModel);
    proxy.addSourceModel(&secondModel);

    prepareModel("Second", &secondModel);

    QTableView tableView;
    tableView.setWindowTitle("concat proxy, in QTableView");
    tableView.setDragDropMode(QAbstractItemView::DragDrop);
    tableView.setModel(&proxy);
    tableView.show();

    QTreeView treeView;
    treeView.setWindowTitle("concat proxy, in QTreeView");
    treeView.setDragDropMode(QAbstractItemView::DragDrop);
    treeView.setModel(&proxy);
    treeView.show();

    // For comparison, views on top on QStandardItemModel

    QTableView tableViewTest;
    tableViewTest.setWindowTitle("first model, in QTableView");
    tableViewTest.setDragDropMode(QAbstractItemView::DragDrop);
    tableViewTest.setModel(&firstModel);
    tableViewTest.show();

    QTreeView treeViewTest;
    treeViewTest.setWindowTitle("first model, in QTreeView");
    treeViewTest.setDragDropMode(QAbstractItemView::DragDrop);
    treeViewTest.setModel(&firstModel);
    treeViewTest.show();

    return app.exec();
}
