// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "mymodel.h"

#include <QTableView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , tableView(new QTableView(this))
{
    setCentralWidget(tableView);
    auto *myModel = new MyModel(this);
    tableView->setModel(myModel);

    // transfer changes to the model to the window title
    connect(myModel, &MyModel::editCompleted,
            this, &QWidget::setWindowTitle);
}
