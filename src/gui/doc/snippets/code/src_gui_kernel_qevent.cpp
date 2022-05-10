// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QWheelEvent>

namespace src_gui_kernel_qevent {
class MyWidget //: public QWidget
{
    void wheelEvent(QWheelEvent *event);
    void scrollWithPixels(QPoint point);
    void scrollWithDegrees(QPoint point);
};


//! [0]
void MyWidget::wheelEvent(QWheelEvent *event)
{
    QPoint numPixels = event->pixelDelta();
    QPoint numDegrees = event->angleDelta() / 8;

    if (!numPixels.isNull()) {
        scrollWithPixels(numPixels);
    } else if (!numDegrees.isNull()) {
        QPoint numSteps = numDegrees / 15;
        scrollWithDegrees(numSteps);
    }

    event->accept();
}
//! [0]


} // src_gui_kernel_qevent
