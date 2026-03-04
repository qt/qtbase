// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include "person.h"

#include <QtWidgets/QWidget>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QComboBox;
class QDataWidgetMapper;
class QLineEdit;
class QPushButton;
class QRangeModel;
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
    QLineEdit *nameEdit;
    QTextEdit *addressEdit;
    QComboBox *typeComboBox;
    QPushButton *nextButton;
    QPushButton *previousButton;
    QList<Person> data;
    QRangeModel *model;
    QDataWidgetMapper *mapper;
};
//! [Window definition]

#endif // WINDOW_H
