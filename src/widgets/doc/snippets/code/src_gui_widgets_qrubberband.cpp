// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
void Widget::mousePressEvent(QMouseEvent *event)
{
    origin = event->pos();
    if (!rubberBand)
        rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    rubberBand->setGeometry(QRect(origin, QSize()));
    rubberBand->show();
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    rubberBand->hide();
    // determine selection, for example using QRect::intersects()
    // and QRect::contains().
}
//! [0]
