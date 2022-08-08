// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QTreeView; // forward declarations
class QStandardItemModel;
class QStandardItem;
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QList<QStandardItem *> prepareRow(const QString &first,
                                      const QString &second,
                                      const QString &third) const;

    QTreeView *treeView;
    QStandardItemModel *standardModel;
};

#endif // MAINWINDOW_H
