// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BOOKWINDOW_H
#define BOOKWINDOW_H

#include <QtWidgets>
#include <QtSql>

#include "ui_bookwindow.h"


class BookWindow: public QMainWindow
{
    Q_OBJECT
public:
    BookWindow();

private slots:
    void about();

private:
    void showError(const QSqlError &err);
    Ui::BookWindow ui;
    QSqlRelationalTableModel *model;
    int authorIdx, genreIdx;

    void createMenuBar();
};

#endif
