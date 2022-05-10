// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ECHODIALOG_H
#define ECHODIALOG_H

#include <QWidget>

#include "echointerface.h"

QT_BEGIN_NAMESPACE
class QString;
class QLineEdit;
class QLabel;
class QPushButton;
class QGridLayout;
QT_END_NAMESPACE

//! [0]
class EchoWindow : public QWidget
{
    Q_OBJECT

public:
    EchoWindow();

private slots:
    void sendEcho();

private:
    void createGUI();
    bool loadPlugin();

    EchoInterface *echoInterface;
    QLineEdit *lineEdit;
    QLabel *label;
    QPushButton *button;
    QGridLayout *layout;
};
//! [0]

#endif
