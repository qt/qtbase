// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QStandardItemModel>
#include <QFile>
#include <QTextStream>

#include "freezetablewidget.h"

int main(int argc, char* argv[])
{
    QApplication app( argc, argv );
    QStandardItemModel *model=new QStandardItemModel();

    QFile file(":/grades.txt");
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        QString line = stream.readLine();
        QStringList list = line.simplified().split(',');
        model->setHorizontalHeaderLabels(list);

        int row = 0;
        QStandardItem *newItem = nullptr;
        while (!stream.atEnd()) {
            line = stream.readLine();
            if (!line.startsWith('#') && line.contains(',')) {
                list = line.simplified().split(',');
                for (int col = 0; col < list.length(); ++col){
                    newItem = new QStandardItem(list.at(col));
                    model->setItem(row, col, newItem);
                }
                ++row;
            }
        }
    }
    file.close();

    FreezeTableWidget *tableView = new FreezeTableWidget(model);

    tableView->setWindowTitle(QObject::tr("Frozen Column Example"));
    tableView->resize(560, 680);
    tableView->show();
    return app.exec();
}
