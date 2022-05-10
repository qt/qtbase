// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTimer;

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void toggleOverrideCursor();

private:
    void keyPressEvent(QKeyEvent* event);

    Ui::MainWindow *ui;
    QTimer *timer;
    int override;

    QCursor ccurs;
    QCursor bcurs;
};

#endif // MAINWINDOW_H
