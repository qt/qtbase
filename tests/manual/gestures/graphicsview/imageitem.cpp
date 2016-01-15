/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "imageitem.h"
#include "gestures.h"

#include <QPainter>
#include <QEvent>

ImageItem::ImageItem(const QImage &image)
{
    setImage(image);
}

void ImageItem::setImage(const QImage &image)
{
    image_ = image;
    pixmap_ = QPixmap::fromImage(image.scaled(400, 400, Qt::KeepAspectRatio));
    update();
}

QImage ImageItem::image() const
{
    return image_;
}

QRectF ImageItem::boundingRect() const
{
    const QSize size = pixmap_.size();
    return QRectF(0, 0, size.width(), size.height());
}

void ImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->drawPixmap(0, 0, pixmap_);
}


GestureImageItem::GestureImageItem(const QImage &image)
    : ImageItem(image)
{
    grabGesture(Qt::PanGesture);
    grabGesture(ThreeFingerSlideGesture::Type);
}

bool GestureImageItem::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture) {
        qDebug("gestureimageitem: gesture triggered");
        return true;
    }
    return ImageItem::event(event);
}

#include "moc_imageitem.cpp"
