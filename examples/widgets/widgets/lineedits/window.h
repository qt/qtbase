// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

//! [0]
class Window : public QWidget
{
    Q_OBJECT

public:
    Window(QWidget *parent = nullptr);

public slots:
    void echoChanged(int);
    void validatorChanged(int);
    void alignmentChanged(int);
    void inputMaskChanged(int);
    void accessChanged(int);

private:
    QLineEdit *echoLineEdit;
    QLineEdit *validatorLineEdit;
    QLineEdit *alignmentLineEdit;
    QLineEdit *inputMaskLineEdit;
    QLineEdit *accessLineEdit;
};
//! [0]

#endif
