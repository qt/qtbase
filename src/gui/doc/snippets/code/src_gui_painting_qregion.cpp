// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QPaintEvent>
#include <QPainter>

namespace src_gui_painting_qregion {
struct MyWidget : public QPaintDevice
{
    void paintEvent(QPaintEvent *);
};

//! [0]
void MyWidget::paintEvent(QPaintEvent *)
{
    QRegion r1(QRect(100, 100, 200, 80),    // r1: elliptic region
               QRegion::Ellipse);
    QRegion r2(QRect(100, 120, 90, 30));    // r2: rectangular region
    QRegion r3 = r1.intersected(r2);        // r3: intersection

    QPainter painter(this);
    painter.setClipRegion(r3);
    // ...                                  // paint clipped graphics
}
//! [0]

} // src_gui_painting_qregion
