// Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QStandardItemModel>
#include <QTableView>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QStandardItemModel model(4, 4);
    QTableView tableView;
    tableView.setSizeAdjustPolicy(QTableView::AdjustToContents);
    tableView.setModel(&model);

    for (int row = 0; row < model.rowCount(); ++row) {
        for (int column = 0; column < model.columnCount(); ++column) {
            QModelIndex index = model.index(row, column, QModelIndex());
            model.setData(index, QVariant(QString("%1,%2").arg(row).arg(column)));
        }
    }

    tableView.setSpan(1, 1, 2, 2);

    tableView.show();

    return app.exec();
}
