// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
  main.cpp

  A simple example of how to access items from an existing model.
*/

#include <QtGui>

/*!
    Create a default directory model and, using the index-based interface to
    the model and some QLabel widgets, populate the window's layout with the
    names of objects in the directory.

    Note that we only want to read the filenames in the highest level of the
    directory, so we supply a default (invalid) QModelIndex to the model in
    order to indicate that we want top-level items.
*/

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget *window = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(window);
    QLabel *title = new QLabel("Some items from the directory model", window);
    title->setBackgroundRole(QPalette::Base);
    title->setMargin(8);
    layout->addWidget(title);

//! [0]
    QFileSystemModel *model = new QFileSystemModel;
    connect(model, &QFileSystemModel::directoryLoaded, [model](const QString &directory) {
        QModelIndex parentIndex = model->index(directory);
        int numRows = model->rowCount(parentIndex);
    });
    model->setRootPath(QDir::currentPath);
//! [0]

//! [1]
    for (int row = 0; row < numRows; ++row) {
        QModelIndex index = model->index(row, 0, parentIndex);
//! [1]

//! [2]
        QString text = model->data(index, Qt::DisplayRole).toString();
        // Display the text in a widget.
//! [2]

        QLabel *label = new QLabel(text, window);
        layout->addWidget(label);
//! [3]
    }
//! [3]

    window->setWindowTitle("A simple model example");
    window->show();
    return app.exec();
}
