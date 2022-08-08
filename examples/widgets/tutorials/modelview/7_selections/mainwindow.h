// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QTreeView; // forward declarations
class QStandardItemModel;
class QItemSelection;
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void selectionChangedSlot(const QItemSelection &newSelection, const QItemSelection &oldSelection);

private:
    QTreeView *treeView;
    QStandardItemModel *standardModel;
};

#endif // MAINWINDOW_H
