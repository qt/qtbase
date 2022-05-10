// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MOUSEPANGESTURERECOGNIZER_H
#define MOUSEPANGESTURERECOGNIZER_H

#include <QGestureRecognizer>

class MousePanGestureRecognizer : public QGestureRecognizer
{
public:
    MousePanGestureRecognizer();

    QGesture* create(QObject *target);
    QGestureRecognizer::Result recognize(QGesture *state, QObject *watched, QEvent *event);
    void reset(QGesture *state);
};

#endif // MOUSEPANGESTURERECOGNIZER_H
