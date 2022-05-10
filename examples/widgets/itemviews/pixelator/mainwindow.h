// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ImageModel;
QT_BEGIN_NAMESPACE
class QAction;
class QTableView;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

    void openImage(const QString &fileName);

public slots:
    void chooseImage();
    void printImage();
    void showAboutBox();
    void updateView();

private:
    ImageModel *model;
    QAction *printAction;
    QString currentPath;
    QTableView *view;
};

#endif // MAINWINDOW_H
