// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QTreeWidget;
QT_END_NAMESPACE

//! [0]
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

public slots:
    void open();
    void saveAs();
    void about();
#if !defined(QT_NO_CONTEXTMENU) && !defined(QT_NO_CLIPBOARD)
    void onCustomContextMenuRequested(const QPoint &pos);
#endif
private:
    void createMenus();

    QTreeWidget *treeWidget;
};
//! [0]

#endif
