// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE
class CircleWidget;

//! [0]
class Window : public QWidget
{
    Q_OBJECT

public:
    Window();

private:
    QLabel *createLabel(const QString &text);

    QLabel *aliasedLabel;
    QLabel *antialiasedLabel;
    QLabel *intLabel;
    QLabel *floatLabel;
    CircleWidget *circleWidgets[2][2];
};
//! [0]

#endif // WINDOW_H
