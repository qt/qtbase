// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QTreeView; //forward declarations
class QStandardItemModel;
class QStandardItem;
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    QTreeView *treeView;
    QStandardItemModel *standardModel;
    QList<QStandardItem *> prepareRow(const QString &first,
                                      const QString &second,
                                      const QString &third) const;
public:
    MainWindow(QWidget *parent = nullptr);
};

#endif // MAINWINDOW_H
