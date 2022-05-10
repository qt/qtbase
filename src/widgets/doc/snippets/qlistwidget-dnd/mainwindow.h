// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>

class QListWidget;
class QListWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private:
    void setupListItems();

    QListWidget *listWidget;
};

#endif
