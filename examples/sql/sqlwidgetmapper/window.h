// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QDataWidgetMapper;
class QItemSelectionModel;
class QLabel;
class QLineEdit;
class QPushButton;
class QSqlRelationalTableModel;
class QStandardItemModel;
class QStringListModel;
class QTextEdit;
QT_END_NAMESPACE

//! [Window definition]
class Window : public QWidget
{
    Q_OBJECT

public:
    Window(QWidget *parent = nullptr);

private slots:
    void updateButtons(int row);

private:
    void setupModel();

    QLabel *nameLabel;
    QLabel *addressLabel;
    QLabel *typeLabel;
    QLineEdit *nameEdit;
    QTextEdit *addressEdit;
    QComboBox *typeComboBox;
    QPushButton *nextButton;
    QPushButton *previousButton;

    QSqlRelationalTableModel *model;
    QItemSelectionModel *selectionModel;
    QDataWidgetMapper *mapper;
    int typeIndex;
};
//! [Window definition]

#endif
