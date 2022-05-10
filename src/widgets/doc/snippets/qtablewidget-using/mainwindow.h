// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>

class QAction;
class QTableWidget;
class QTableWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

public slots:
    void averageItems();
    void sumItems();

private:
    void setupTableItems();

    QAction *removeAction;
//! [0]
    QTableWidget *tableWidget;
//! [0]
};

#endif
