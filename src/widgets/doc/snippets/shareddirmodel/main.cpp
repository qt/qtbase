// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
  main.cpp

  A simple example of how to view a model in several views, and share a
  selection model.
*/

#include <QtGui>

//! [0] //! [1]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QSplitter *splitter = new QSplitter;

//! [2] //! [3]
    QFileSystemModel *model = new QFileSystemModel;
    model->setRootPath(QDir::currentPath());
//! [0] //! [2] //! [4] //! [5]
    QTreeView *tree = new QTreeView(splitter);
//! [3] //! [6]
    tree->setModel(model);
//! [4] //! [6] //! [7]
    tree->setRootIndex(model->index(QDir::currentPath()));
//! [7]

    QListView *list = new QListView(splitter);
    list->setModel(model);
    list->setRootIndex(model->index(QDir::currentPath()));

//! [5]
    QItemSelectionModel *selection = new QItemSelectionModel(model);
    tree->setSelectionModel(selection);
    list->setSelectionModel(selection);

//! [8]
    splitter->setWindowTitle("Two views onto the same file system model");
    splitter->show();
    return app.exec();
}
//! [1] //! [8]
