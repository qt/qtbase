// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef NUMBERFORMATS_H
#define NUMBERFORMATS_H

#include <QWidget>
#include <QLocale>

class QLineEdit;
class QLabel;

class NumberFormatsWidget : public QWidget
{
    Q_OBJECT
public:
    NumberFormatsWidget();

private:
    QLineEdit *createLineEdit();

    QLineEdit *number1, *number2, *number3;
    QLabel *measurementLabel;
    QLineEdit *measurementSystem;

private slots:
    void localeChanged(QLocale locale);
};

#endif
