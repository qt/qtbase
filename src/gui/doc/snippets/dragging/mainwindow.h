// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QPoint>

class QLabel;
class QLineEdit;
class QMouseEvent;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QLabel *iconLabel;
    QLineEdit *nameEdit;
    QPixmap iconPixmap;
    QPoint dragStartPosition;
    QTextEdit *commentEdit;
};

#endif
