// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QTextEdit;
QT_END_NAMESPACE

class Window : public QWidget
{
    Q_OBJECT

public:
    Window();

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
    QTextEdit *textEdit;
};

#endif
