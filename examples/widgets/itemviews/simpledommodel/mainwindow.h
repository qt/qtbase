// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class DomModel;
QT_BEGIN_NAMESPACE
class QMenu;
class QTreeView;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

public slots:
    void openFile();

private:
    DomModel *model;
    QMenu *fileMenu;
    QString xmlPath;
    QTreeView *view;
};

#endif // MAINWINDOW_H
