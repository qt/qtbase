// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "imagewidget.h"

#include <QDir>
#include <QImageReader>
#include <QGestureEvent>
#include <QPainter>

Q_LOGGING_CATEGORY(lcExample, "qt.examples.imagegestures")

//! [constructor]
ImageWidget::ImageWidget(QWidget *parent)
    : QWidget(parent), position(0), horizontalOffset(0), verticalOffset(0)
    , rotationAngle(0), scaleFactor(1), currentStepScaleFactor(1)
{
    setMinimumSize(QSize(100, 100));
}
//! [constructor]

void ImageWidget::grabGestures(const QList<Qt::GestureType> &gestures)
{
    //! [enable gestures]
    for (Qt::GestureType gesture : gestures)
        grabGesture(gesture);
    //! [enable gestures]
}

//! [event handler]
bool ImageWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent*>(event));
    return QWidget::event(event);
}
//! [event handler]

//! [paint method]
void ImageWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    if (files.isEmpty() && !path.isEmpty()) {
        p.drawText(rect(), Qt::AlignCenter|Qt::TextWordWrap,
                         tr("No supported image formats found"));
        return;
    }

    const qreal iw = currentImage.width();
    const qreal ih = currentImage.height();
    const qreal wh = height();
    const qreal ww = width();

    p.translate(ww / 2, wh / 2);
    p.translate(horizontalOffset, verticalOffset);
    p.rotate(rotationAngle);
    p.scale(currentStepScaleFactor * scaleFactor, currentStepScaleFactor * scaleFactor);
    p.translate(-iw / 2, -ih / 2);
    p.drawImage(0, 0, currentImage);
}
//! [paint method]

void ImageWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    rotationAngle = 0;
    scaleFactor = 1;
    currentStepScaleFactor = 1;
    verticalOffset = 0;
    horizontalOffset = 0;
    update();
    qCDebug(lcExample) << "reset on mouse double click";
}

//! [gesture event handler]
bool ImageWidget::gestureEvent(QGestureEvent *event)
{
    qCDebug(lcExample) << "gestureEvent():" << event;
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
    qCDebug(lcExample) << "panTriggered():" << gesture;
    horizontalOffset += delta.x();
    verticalOffset += delta.y();
    update();
}

//! [pinch function]
void ImageWidget::pinchTriggered(QPinchGesture *gesture)
{
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    if (changeFlags & QPinchGesture::RotationAngleChanged) {
        qreal rotationDelta = gesture->rotationAngle() - gesture->lastRotationAngle();
        rotationAngle += rotationDelta;
        qCDebug(lcExample) << "pinchTriggered(): rotate by" <<
            rotationDelta << "->" << rotationAngle;
    }
    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        currentStepScaleFactor = gesture->totalScaleFactor();
        qCDebug(lcExample) << "pinchTriggered(): zoom by" <<
            gesture->scaleFactor() << "->" << currentStepScaleFactor;
    }
    if (gesture->state() == Qt::GestureFinished) {
        scaleFactor *= currentStepScaleFactor;
        currentStepScaleFactor = 1;
    }
    update();
}
//! [pinch function]

//! [swipe function]
void ImageWidget::swipeTriggered(QSwipeGesture *gesture)
{
    if (gesture->state() == Qt::GestureFinished) {
        if (gesture->swipeAngle() < 45 || gesture->swipeAngle() > 225) {
            // swipe direction right or down
            qCDebug(lcExample) << "swipeTriggered(): angle"
                               << gesture->swipeAngle() << "; swipe to next";
            goNextImage();
        } else {
            // swipe direction left or up
            qCDebug(lcExample) << "swipeTriggered(): angle"
                               << gesture->swipeAngle() << "; swipe to previous";
            goPrevImage();
        }
        update();
    }
}
//! [swipe function]

void ImageWidget::resizeEvent(QResizeEvent*)
{
    update();
}

void ImageWidget::openDirectory(const QString &url)
{
    path = url;
    QDir dir(path);

    QStringList nameFilters;
    const QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    for (const QByteArray &format : supportedFormats)
        nameFilters.append(QLatin1String("*.") + QString::fromLatin1(format));
    files = dir.entryInfoList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);

    position = 0;
    goToImage(0);
    update();
}

/*
    With Android's content scheme paths, it might not be possible to simply
    append a file name to the chosen directory path to be able to open the image,
    because usually paths are returned by an Android file provider and handling those
    paths manually is not guaranteed to work. For that reason, it's better to keep
    around QFileInfo objects and use absoluteFilePath().
*/
QImage ImageWidget::loadImage(const QFileInfo &fileInfo) const
{
    const QString fileName = fileInfo.absoluteFilePath();
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    qCDebug(lcExample) << "loading" << QDir::toNativeSeparators(fileName) << position << '/' << files.size();
    if (!reader.canRead()) {
        qCWarning(lcExample) << QDir::toNativeSeparators(fileName) << ": can't load image";
        return QImage();
    }

    QImage image;
    if (!reader.read(&image)) {
        qCWarning(lcExample) << QDir::toNativeSeparators(fileName) << ": corrupted image: " << reader.errorString();
        return QImage();
    }
    const QSize maximumSize(2000, 2000); // Reduce in case someone has large photo images.
    if (image.size().width() > maximumSize.width() || image.height() > maximumSize.height())
        image = image.scaled(maximumSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
            nextImage = loadImage(files.at(position + 1));
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
            prevImage = loadImage(files.at(position - 1));
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
        qCWarning(lcExample) << "goToImage: invalid index: " << index;
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
        prevImage = loadImage(files.at(position - 1));
    else
        prevImage = QImage();
    currentImage = loadImage(files.at(position));
    if (position+1 < files.size())
        nextImage = loadImage(files.at(position + 1));
    else
        nextImage = QImage();
    update();
}
