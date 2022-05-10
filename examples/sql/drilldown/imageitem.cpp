// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "imageitem.h"

//! [0]
ImageItem::ImageItem(int id, const QPixmap &pixmap, QGraphicsItem *parent)
    : QGraphicsPixmapItem(pixmap, parent)
{
    recordId = id;
    setAcceptHoverEvents(true);

    timeLine.setDuration(150);
    timeLine.setFrameRange(0, 150);

    connect(&timeLine, &QTimeLine::frameChanged, this, &ImageItem::setFrame);
    connect(&timeLine, &QTimeLine::finished, this, &ImageItem::updateItemPosition);

    adjust();
}
//! [0]

//! [1]
void ImageItem::hoverEnterEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    timeLine.setDirection(QTimeLine::Forward);

    if (z != 1.0) {
        z = 1.0;
        updateItemPosition();
    }

    if (timeLine.state() == QTimeLine::NotRunning)
        timeLine.start();
}
//! [1]

//! [2]
void ImageItem::hoverLeaveEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    timeLine.setDirection(QTimeLine::Backward);
    if (z != 0.0)
        z = 0.0;

    if (timeLine.state() == QTimeLine::NotRunning)
        timeLine.start();
}
//! [2]

//! [3]
void ImageItem::setFrame(int frame)
{
    adjust();
    QPointF center = boundingRect().center();

    setTransform(QTransform::fromTranslate(center.x(), center.y()), true);
    setTransform(QTransform::fromScale(1 + frame / 330.0, 1 + frame / 330.0), true);
    setTransform(QTransform::fromTranslate(-center.x(), -center.y()), true);
}
//! [3]

//! [4]
void ImageItem::adjust()
{
    setTransform(QTransform::fromScale(120 / boundingRect().width(),
                                       120 / boundingRect().height()));
}
//! [4]

//! [5]
int ImageItem::id() const
{
    return recordId;
}
//! [5]

//! [6]
void ImageItem::updateItemPosition()
{
    setZValue(z);
}
//! [6]


