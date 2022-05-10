// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

protected:
    void timerEvent(QTimerEvent *) override;

public slots:
    void enableStuffMenu(bool enable);

private slots:
    void on_actionQuit_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
