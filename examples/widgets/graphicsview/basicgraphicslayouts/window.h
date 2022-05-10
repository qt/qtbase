// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QGraphicsWidget>

//! [0]
class Window : public QGraphicsWidget
{
    Q_OBJECT
public:
    Window(QGraphicsWidget *parent = nullptr);

};
//! [0]

#endif  //WINDOW_H

