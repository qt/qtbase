/****************************************************************************
**
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
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
