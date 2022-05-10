// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QPainter>

namespace src_gui_painting_qbrush {

struct Wrapper : public QPaintDevice {
    void wrapper();
};
void Wrapper::wrapper() {


//! [0]
QPainter painter(this);

painter.setBrush(Qt::cyan);
painter.setPen(Qt::darkCyan);
painter.drawRect(0, 0, 100,100);

painter.setBrush(Qt::NoBrush);
painter.setPen(Qt::darkGreen);
painter.drawRect(40, 40, 100, 100);
//! [0]


} // Wrapper::wrapper
} // src_gui_painting_qbrush
