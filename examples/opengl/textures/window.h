// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

class GLWidget;

class Window : public QWidget
{
    Q_OBJECT

public:
    Window();

private slots:
    void setCurrentGlWidget();
    void rotateOneStep();

private:
    enum { NumRows = 2, NumColumns = 3 };

    GLWidget *glWidgets[NumRows][NumColumns];
    GLWidget *currentGlWidget;
};

#endif
