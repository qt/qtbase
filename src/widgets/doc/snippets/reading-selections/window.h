// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QItemSelection>
#include <QMainWindow>
#include <QAbstractItemModel>
#include <QWidget>
#include <QTableView>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void fillSelection();
    void clearSelection();
    void selectAll();

private:
    QAbstractItemModel *model;
    QItemSelectionModel *selectionModel;
    QTableView *table;
};

#endif
