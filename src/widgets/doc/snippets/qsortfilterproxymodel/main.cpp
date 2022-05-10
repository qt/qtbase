// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QObject *parent = &app;

    QStringList numbers;
    numbers << "One" << "Two" << "Three" << "Four" << "Five";

    QAbstractItemModel *stringListModel = new QStringListModel(numbers, parent);

//! [0]
    QSortFilterProxyModel *filterModel = new QSortFilterProxyModel(parent);
    filterModel->setSourceModel(stringListModel);
//! [0]

    QWidget *window = new QWidget;

//! [1]
    QListView *filteredView = new QListView;
    filteredView->setModel(filterModel);
//! [1]
    filteredView->setWindowTitle("Filtered view onto a string list model");

    QLineEdit *patternEditor = new QLineEdit;
    QObject::connect(patternEditor, &QLineEdit::textChanged,
                     filterModel, &QSortFilterProxyModel::setFilterWildcard);

    QVBoxLayout *layout = new QVBoxLayout(window);
    layout->addWidget(filteredView);
    layout->addWidget(patternEditor);

    window->show();
    return app.exec();
}
