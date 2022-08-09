// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
  main.cpp

  A simple example of how to access items from an existing model.
*/

#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <QFileSystemModel>
#include <QPalette>

#include <QDir>
#include <QModelIndex>

/*!
    Create a default directory model and, using the index-based interface to
    the model and some QLabel widgets, populate the window's layout with the
    names of objects in the directory.
*/

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    auto *layout = new QVBoxLayout(&window);
    auto *title = new QLabel("Some items from the directory model", &window);
    title->setBackgroundRole(QPalette::Base);
    title->setMargin(8);
    layout->addWidget(title);

//! [0]
    auto *model = new QFileSystemModel;

    auto onDirectoryLoaded = [model, layout, &window](const QString &directory) {
        QModelIndex parentIndex = model->index(directory);
        const int numRows = model->rowCount(parentIndex);
//! [1]
        for (int row = 0; row < numRows; ++row) {
            QModelIndex index = model->index(row, 0, parentIndex);
//! [1]

//! [2]
            QString text = model->data(index, Qt::DisplayRole).toString();
//! [2]
            // Display the text in a widget.
            auto *label = new QLabel(text, &window);
            layout->addWidget(label);
//! [3]
        }
//! [3]
    };

    QObject::connect(model, &QFileSystemModel::directoryLoaded, onDirectoryLoaded);
    model->setRootPath(QDir::currentPath());
//! [0]

    window.setWindowTitle("A simple model example");
    window.show();
    return app.exec();
}
