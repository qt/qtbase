// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../connection.h"
#include "customsqlmodel.h"
#include "editablesqlmodel.h"

#include <QApplication>
#include <QTableView>

void initializeModel(QSqlQueryModel *model)
{
    model->setQuery("select * from person");
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("First name"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Last name"));
}

QTableView* createView(QSqlQueryModel *model, const QString &title = "")
{
    QTableView *view = new QTableView;
    view->setModel(model);
    static int offset = 0;

    view->setWindowTitle(title);
    view->move(100 + offset, 100 + offset);
    offset += 20;
    view->show();

    return view;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (!createConnection())
        return EXIT_FAILURE;

    QSqlQueryModel plainModel;
    EditableSqlModel editableModel;
    CustomSqlModel customModel;

    initializeModel(&plainModel);
    initializeModel(&editableModel);
    initializeModel(&customModel);

    createView(&plainModel, QObject::tr("Plain Query Model"));
    createView(&editableModel, QObject::tr("Editable Query Model"));
    createView(&customModel, QObject::tr("Custom Query Model"));

    return app.exec();
}
