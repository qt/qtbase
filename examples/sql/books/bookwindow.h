// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BOOKWINDOW_H
#define BOOKWINDOW_H

#include <QMainWindow>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QGridLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QSpinBox)
QT_FORWARD_DECLARE_CLASS(QSqlError)
QT_FORWARD_DECLARE_CLASS(QSqlRelationalTableModel)
QT_FORWARD_DECLARE_CLASS(QTableView)

class BookWindow: public QMainWindow
{
    Q_OBJECT
public:
    BookWindow();

private slots:
    void about();

private:
    void showError(const QSqlError &err);
    QSqlRelationalTableModel *model = nullptr;
    int authorIdx = 0, genreIdx = 0;

    void createLayout();
    void createModel();
    void configureWidgets();
    void createMappings();
    void createMenuBar();

    QWidget *window = nullptr;

    QGridLayout *gridLayout = nullptr;
    QTableView *tableView = nullptr;
    QLabel *titleLabel = nullptr;
    QLineEdit *titleLineEdit = nullptr;
    QLabel *authorLabel = nullptr;
    QComboBox *authorComboBox = nullptr;
    QLabel *genreLabel = nullptr;
    QComboBox *genreComboBox = nullptr;
    QLabel *yearLabel = nullptr;
    QSpinBox *yearSpinBox = nullptr;
    QLabel *ratingLabel = nullptr;
    QComboBox *ratingComboBox = nullptr;
};

#endif
