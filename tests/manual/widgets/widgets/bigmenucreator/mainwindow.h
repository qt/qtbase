// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void doAPS(QWidget *menu, int level);
    void doASP(QWidget *menu, int level);
    void doPAS(QWidget *menu, int level);

    void doSPA(QWidget *menu, int level);
    void doSAP(QWidget *menu, int level);
    void doPSA(QWidget *menu, int level);

    void populateMenu(QWidget *menu, int level);

    static bool newMenubar;
    static bool parentlessMenubar;

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
