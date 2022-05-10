// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TOUCHWIDGET_H
#define TOUCHWIDGET_H

#include <QWidget>

class TouchWidget : public QWidget
{
    Q_OBJECT

public:
    bool acceptTouchBegin, acceptTouchUpdate, acceptTouchEnd;
    bool seenTouchBegin, seenTouchUpdate, seenTouchEnd;
    bool closeWindowOnTouchEnd;
    int touchPointCount;

    bool acceptMousePress, acceptMouseMove, acceptMouseRelease;
    bool seenMousePress, seenMouseMove, seenMouseRelease;
    bool closeWindowOnMouseRelease;

    inline TouchWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        reset();
    }

    void reset();

    bool event(QEvent *event);
};

#endif // TOUCHWIDGET_H
