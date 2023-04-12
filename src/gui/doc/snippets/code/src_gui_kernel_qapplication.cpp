// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QStyleFactory>
#include <QWidget>


namespace src_gui_kernel_qapplication {
struct MyWidget
{
    QSize sizeHint() const;

    int foo = 0;
    MyWidget operator- (MyWidget& other)
    {
        MyWidget tmp = other;
        return tmp;
    };
    int manhattanLength() { return 0; }
};

void startTheDrag() {};
void wrapper1() {
MyWidget startPos;
MyWidget currentPos;
int x = 0;
int y = 0;

//! [6]
if ((startPos - currentPos).manhattanLength() >=
        QApplication::startDragDistance())
    startTheDrag();
//! [6]

} // wrapper1

} // src_gui_kernel_qapplication
