// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStandardItemModel;
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
    QLabel *ageLabel;
    QLineEdit *nameEdit;
    QTextEdit *addressEdit;
    QSpinBox *ageSpinBox;
    QPushButton *nextButton;
    QPushButton *previousButton;

    QStandardItemModel *model;
    QDataWidgetMapper *mapper;
};
//! [Window definition]

#endif // WINDOW_H
