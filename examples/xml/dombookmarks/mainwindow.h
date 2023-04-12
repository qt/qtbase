// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class XbelTree;

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

private:
    void createMenus();

    XbelTree *xbelTree;
};
//! [0]

#endif
