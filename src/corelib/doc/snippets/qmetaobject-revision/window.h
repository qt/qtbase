// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

//! [Window class with revision]
class Window : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int normalProperty READ normalProperty)
    Q_PROPERTY(int newProperty READ newProperty REVISION(2, 1))

public:
    Window();
    int normalProperty();
    int newProperty();
public slots:
    void normalMethod();
    Q_REVISION(2, 1) void newMethod();
};
//! [Window class with revision]

#endif
