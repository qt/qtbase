// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "../connection.h"

#include <QApplication>
#include <QSqlTableModel>
#include <QTableView>

#include <stdlib.h>

void initializeModel(QSqlTableModel *model)
{
    model->setTable("person");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select();

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("First name"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Last name"));
}

QTableView *createView(QSqlTableModel *model, const QString &title = "")
{
    QTableView *view = new QTableView;
    view->setModel(model);
    view->setWindowTitle(title);
    return view;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (!createConnection())
        return EXIT_FAILURE;

    QSqlTableModel model;

    initializeModel(&model);

    QTableView *view1 = createView(&model, QObject::tr("Table Model (View 1)"));
    QTableView *view2 = createView(&model, QObject::tr("Table Model (View 2)"));

    view1->show();
    view2->move(view1->x() + view1->width() + 20, view1->y());
    view2->show();

    return app.exec();
}
