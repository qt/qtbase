// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mandelbrotwidget.h"

#include <QGesture>
#include <QGestureEvent>
#include <QKeyEvent>
#include <QPainter>

#include <math.h>

//! [0]
constexpr double DefaultCenterX = -0.637011;
constexpr double DefaultCenterY = -0.0395159;
constexpr double DefaultScale = 0.00403897;

constexpr double ZoomInFactor = 0.8;
constexpr double ZoomOutFactor = 1 / ZoomInFactor;
constexpr int ScrollStep = 20;
//! [0]

//! [1]
MandelbrotWidget::MandelbrotWidget(QWidget *parent) :
    QWidget(parent),
    centerX(DefaultCenterX),
    centerY(DefaultCenterY),
    pixmapScale(DefaultScale),
    curScale(DefaultScale)
{
    help = tr("Zoom with mouse wheel, +/- keys or pinch.  Scroll with arrow keys or by dragging.");
    connect(&thread, &RenderThread::renderedImage,
            this, &MandelbrotWidget::updatePixmap);

    setWindowTitle(tr("Mandelbrot"));
#if QT_CONFIG(cursor)
    setCursor(Qt::CrossCursor);
#endif
}
//! [1]

//! [2]
void MandelbrotWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (pixmap.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter|Qt::TextWordWrap,
                         tr("Rendering initial image, please wait..."));
//! [2] //! [3]
        return;
//! [3] //! [4]
    }
//! [4]

//! [5]
    if (qFuzzyCompare(curScale, pixmapScale)) {
//! [5] //! [6]
        painter.drawPixmap(pixmapOffset, pixmap);
//! [6] //! [7]
    } else {
//! [7] //! [8]
        const auto previewPixmap = qFuzzyCompare(pixmap.devicePixelRatio(), qreal(1))
            ? pixmap
            : pixmap.scaled(pixmap.deviceIndependentSize().toSize(), Qt::KeepAspectRatio,
                            Qt::SmoothTransformation);
        const double scaleFactor = pixmapScale / curScale;
        const int newWidth = int(previewPixmap.width() * scaleFactor);
        const int newHeight = int(previewPixmap.height() * scaleFactor);
        const int newX = pixmapOffset.x() + (previewPixmap.width() - newWidth) / 2;
        const int newY = pixmapOffset.y() + (previewPixmap.height() - newHeight) / 2;

        painter.save();
        painter.translate(newX, newY);
        painter.scale(scaleFactor, scaleFactor);

        QRectF exposed = painter.transform().inverted().mapRect(rect());
        exposed = exposed.adjusted(-1, -1, 1, 1);
        painter.drawPixmap(exposed, previewPixmap, exposed);
        painter.restore();
    }
//! [8] //! [9]

    const QFontMetrics metrics = painter.fontMetrics();
    if (!info.isEmpty()){
        const int infoWidth = metrics.horizontalAdvance(info);
        const int infoHeight = (infoWidth/width() + 1) * (metrics.height() + 5);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 127));
        painter.drawRect((width() - infoWidth) / 2 - 5, 0, infoWidth + 10, infoHeight);

        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignHCenter|Qt::AlignTop|Qt::TextWordWrap, info);
    }

    const int helpWidth = metrics.horizontalAdvance(help);
    const int helpHeight = (helpWidth/width() + 1) * (metrics.height() + 5);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 127));
    painter.drawRect((width() - helpWidth) / 2 - 5, height()-helpHeight, helpWidth + 10,
                     helpHeight);

    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignHCenter|Qt::AlignBottom|Qt::TextWordWrap, help);

}
//! [9]

//! [10]
void MandelbrotWidget::resizeEvent(QResizeEvent * /* event */)
{
    thread.render(centerX, centerY, curScale, size(), devicePixelRatio());
}
//! [10]

//! [11]
void MandelbrotWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        zoom(ZoomInFactor);
        break;
    case Qt::Key_Minus:
        zoom(ZoomOutFactor);
        break;
    case Qt::Key_Left:
        scroll(-ScrollStep, 0);
        break;
    case Qt::Key_Right:
        scroll(+ScrollStep, 0);
        break;
    case Qt::Key_Down:
        scroll(0, -ScrollStep);
        break;
    case Qt::Key_Up:
        scroll(0, +ScrollStep);
        break;
    case Qt::Key_Q:
        close();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}
//! [11]

#if QT_CONFIG(wheelevent)
//! [12]
void MandelbrotWidget::wheelEvent(QWheelEvent *event)
{
    const int numDegrees = event->angleDelta().y() / 8;
    const double numSteps = numDegrees / double(15);
    zoom(pow(ZoomInFactor, numSteps));
}
//! [12]
#endif

//! [13]
void MandelbrotWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        lastDragPos = event->position().toPoint();
}
//! [13]

//! [14]
void MandelbrotWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        pixmapOffset += event->position().toPoint() - lastDragPos;
        lastDragPos = event->position().toPoint();
        update();
    }
}
//! [14]

//! [15]
void MandelbrotWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pixmapOffset += event->position().toPoint() - lastDragPos;
        lastDragPos = QPoint();

        const auto pixmapSize = pixmap.deviceIndependentSize().toSize();
        const int deltaX = (width() - pixmapSize.width()) / 2 - pixmapOffset.x();
        const int deltaY = (height() - pixmapSize.height()) / 2 - pixmapOffset.y();
        scroll(deltaX, deltaY);
    }
}
//! [15]

//! [16]
void MandelbrotWidget::updatePixmap(const QImage &image, double scaleFactor)
{
    if (!lastDragPos.isNull())
        return;

    info = image.text(RenderThread::infoKey());

    pixmap = QPixmap::fromImage(image);
    pixmapOffset = QPoint();
    lastDragPos = QPoint();
    pixmapScale = scaleFactor;
    update();
}
//! [16]

//! [17]
void MandelbrotWidget::zoom(double zoomFactor)
{
    curScale *= zoomFactor;
    update();
    thread.render(centerX, centerY, curScale, size(), devicePixelRatio());
}
//! [17]

//! [18]
void MandelbrotWidget::scroll(int deltaX, int deltaY)
{
    centerX += deltaX * curScale;
    centerY += deltaY * curScale;
    update();
    thread.render(centerX, centerY, curScale, size(), devicePixelRatio());
}
//! [18]

//! [gesture1]
#ifndef QT_NO_GESTURES
bool MandelbrotWidget::gestureEvent(QGestureEvent *event)
{
    if (auto *pinch = static_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture))) {
        if (pinch->changeFlags().testFlag(QPinchGesture::ScaleFactorChanged))
            zoom(1.0 / pinch->scaleFactor());
        return true;
    }
    return false;
}

bool MandelbrotWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent*>(event));
    return QWidget::event(event);
}
#endif
//! [gesture1]
