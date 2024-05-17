// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include <QGraphicsItem>
#include <QImage>
#include <QPixmap>
#include <QTransform>

class ImageItem : public QGraphicsObject
{
    Q_OBJECT
public:
    ImageItem(const QImage &image);
    void setImage(const QImage &image);
    QImage image() const;
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    QImage image_;
    QPixmap pixmap_;
    QTransform transform;
};

class GestureImageItem : public ImageItem
{
    Q_OBJECT

public:
    GestureImageItem(const QImage &image);

protected:
    bool event(QEvent *event);
};

#endif // IMAGEITEM_H
