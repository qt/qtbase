
/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "imagewidget.h"

#include <QtWidgets>

//! [constructor]
ImageWidget::ImageWidget(QWidget *parent)
    : QWidget(parent),
    position(0),
    horizontalOffset(0),
    verticalOffset(0),
    rotationAngle(0),
    scaleFactor(1),
    currentStepScaleFactor(1)

{
    setMinimumSize(QSize(100,100));

//! [enable gestures]
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);
//! [enable gestures]
}
//! [constructor]

//! [event handler]
bool ImageWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent*>(event));
    return QWidget::event(event);
}
//! [event handler]

void ImageWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    float iw = currentImage.width();
    float ih = currentImage.height();
    float wh = height();
    float ww = width();

    p.translate(ww/2, wh/2);
    p.translate(horizontalOffset, verticalOffset);
    p.rotate(rotationAngle);
    p.scale(currentStepScaleFactor * scaleFactor, currentStepScaleFactor * scaleFactor);
    p.translate(-iw/2, -ih/2);
    p.drawImage(0, 0, currentImage);
}

void ImageWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    rotationAngle = 0;
    scaleFactor = 1;
    currentStepScaleFactor = 1;
    verticalOffset = 0;
    horizontalOffset = 0;
    update();
}

//! [gesture event handler]
bool ImageWidget::gestureEvent(QGestureEvent *event)
{
    if (QGesture *swipe = event->gesture(Qt::SwipeGesture))
        swipeTriggered(static_cast<QSwipeGesture *>(swipe));
    else if (QGesture *pan = event->gesture(Qt::PanGesture))
        panTriggered(static_cast<QPanGesture *>(pan));
    if (QGesture *pinch = event->gesture(Qt::PinchGesture))
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    return true;
}
//! [gesture event handler]

void ImageWidget::panTriggered(QPanGesture *gesture)
{
#ifndef QT_NO_CURSOR
    switch (gesture->state()) {
        case Qt::GestureStarted:
        case Qt::GestureUpdated:
            setCursor(Qt::SizeAllCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
    }
#endif
    QPointF delta = gesture->delta();
    horizontalOffset += delta.x();
    verticalOffset += delta.y();
    update();
}

void ImageWidget::pinchTriggered(QPinchGesture *gesture)
{
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    if (changeFlags & QPinchGesture::RotationAngleChanged) {
        qreal value = gesture->property("rotationAngle").toReal();
        qreal lastValue = gesture->property("lastRotationAngle").toReal();
        rotationAngle += value - lastValue;
    }
    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        qreal value = gesture->property("scaleFactor").toReal();
        currentStepScaleFactor = value;
    }
    if (gesture->state() == Qt::GestureFinished) {
        scaleFactor *= currentStepScaleFactor;
        currentStepScaleFactor = 1;
    }
    update();
}

//! [swipe function]
void ImageWidget::swipeTriggered(QSwipeGesture *gesture)
{
    if (gesture->state() == Qt::GestureFinished) {
        if (gesture->horizontalDirection() == QSwipeGesture::Left
            || gesture->verticalDirection() == QSwipeGesture::Up)
            goPrevImage();
        else
            goNextImage();
        update();
    }
}
//! [swipe function]

void ImageWidget::resizeEvent(QResizeEvent*)
{
    update();
}

void ImageWidget::openDirectory(const QString &path)
{
    this->path = path;
    QDir dir(path);
    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.png";
    files = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);

    position = 0;
    goToImage(0);
    update();
}

QImage ImageWidget::loadImage(const QString &fileName)
{
    QImageReader reader(fileName);
    if (!reader.canRead()) {
        qDebug() << fileName << ": can't load image";
        return QImage();
    }

    QImage image;
    if (!reader.read(&image)) {
        qDebug() << fileName << ": corrupted image";
        return QImage();
    }
    return image;
}

void ImageWidget::goNextImage()
{
    if (files.isEmpty())
        return;

    if (position < files.size()-1) {
        ++position;
        prevImage = currentImage;
        currentImage = nextImage;
        if (position+1 < files.size())
            nextImage = loadImage(path+QLatin1String("/")+files.at(position+1));
        else
            nextImage = QImage();
    }
    update();
}

void ImageWidget::goPrevImage()
{
    if (files.isEmpty())
        return;

    if (position > 0) {
        --position;
        nextImage = currentImage;
        currentImage = prevImage;
        if (position > 0)
            prevImage = loadImage(path+QLatin1String("/")+files.at(position-1));
        else
            prevImage = QImage();
    }
    update();
}

void ImageWidget::goToImage(int index)
{
    if (files.isEmpty())
        return;

    if (index < 0 || index >= files.size()) {
        qDebug() << "goToImage: invalid index: " << index;
        return;
    }

    if (index == position+1) {
        goNextImage();
        return;
    }

    if (position > 0 && index == position-1) {
        goPrevImage();
        return;
    }

    position = index;

    if (index > 0)
        prevImage = loadImage(path+QLatin1String("/")+files.at(position-1));
    else
        prevImage = QImage();
    currentImage = loadImage(path+QLatin1String("/")+files.at(position));
    if (position+1 < files.size())
        nextImage = loadImage(path+QLatin1String("/")+files.at(position+1));
    else
        nextImage = QImage();
    update();
}
