// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class Window : public QWidget
{
    Q_OBJECT

public:
    Window();

private:
    QLabel *createLabel(const QString &text);
};

#endif // WINDOW_H
