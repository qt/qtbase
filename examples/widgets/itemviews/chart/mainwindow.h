// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QAbstractItemView;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void openFile();
    void saveFile();

private:
    void setupModel();
    void setupViews();
    void loadFile(const QString &path);

    QAbstractItemModel *model = nullptr;
    QAbstractItemView *pieChart = nullptr;
};

#endif // MAINWINDOW_H
