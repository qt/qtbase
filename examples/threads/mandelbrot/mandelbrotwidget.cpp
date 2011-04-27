/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include <math.h>

#include "mandelbrotwidget.h"

//! [0]
const double DefaultCenterX = -0.637011f;
const double DefaultCenterY = -0.0395159f;
const double DefaultScale = 0.00403897f;

const double ZoomInFactor = 0.8f;
const double ZoomOutFactor = 1 / ZoomInFactor;
const int ScrollStep = 20;
//! [0]

//! [1]
MandelbrotWidget::MandelbrotWidget(QWidget *parent)
    : QWidget(parent)
{
    centerX = DefaultCenterX;
    centerY = DefaultCenterY;
    pixmapScale = DefaultScale;
    curScale = DefaultScale;

    qRegisterMetaType<QImage>("QImage");
    connect(&thread, SIGNAL(renderedImage(QImage,double)),
            this, SLOT(updatePixmap(QImage,double)));

    setWindowTitle(tr("Mandelbrot"));
#ifndef QT_NO_CURSOR
    setCursor(Qt::CrossCursor);
#endif
    resize(550, 400);
}
//! [1]

//! [2]
void MandelbrotWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (pixmap.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter,
                         tr("Rendering initial image, please wait..."));
//! [2] //! [3]
        return;
//! [3] //! [4]
    }
//! [4]

//! [5]
    if (curScale == pixmapScale) {
//! [5] //! [6]
        painter.drawPixmap(pixmapOffset, pixmap);
//! [6] //! [7]
    } else {
//! [7] //! [8]
        double scaleFactor = pixmapScale / curScale;
        int newWidth = int(pixmap.width() * scaleFactor);
        int newHeight = int(pixmap.height() * scaleFactor);
        int newX = pixmapOffset.x() + (pixmap.width() - newWidth) / 2;
        int newY = pixmapOffset.y() + (pixmap.height() - newHeight) / 2;

        painter.save();
        painter.translate(newX, newY);
        painter.scale(scaleFactor, scaleFactor);
        QRectF exposed = painter.matrix().inverted().mapRect(rect()).adjusted(-1, -1, 1, 1);
        painter.drawPixmap(exposed, pixmap, exposed);
        painter.restore();
    }
//! [8] //! [9]

    QString text = tr("Use mouse wheel or the '+' and '-' keys to zoom. "
                      "Press and hold left mouse button to scroll.");
    QFontMetrics metrics = painter.fontMetrics();
    int textWidth = metrics.width(text);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 127));
    painter.drawRect((width() - textWidth) / 2 - 5, 0, textWidth + 10,
                     metrics.lineSpacing() + 5);
    painter.setPen(Qt::white);
    painter.drawText((width() - textWidth) / 2,
                     metrics.leading() + metrics.ascent(), text);
}
//! [9]

//! [10]
void MandelbrotWidget::resizeEvent(QResizeEvent * /* event */)
{
    thread.render(centerX, centerY, curScale, size());
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
    default:
        QWidget::keyPressEvent(event);
    }
}
//! [11]

//! [12]
void MandelbrotWidget::wheelEvent(QWheelEvent *event)
{
    int numDegrees = event->delta() / 8;
    double numSteps = numDegrees / 15.0f;
    zoom(pow(ZoomInFactor, numSteps));
}
//! [12]

//! [13]
void MandelbrotWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        lastDragPos = event->pos();
}
//! [13]

//! [14]
void MandelbrotWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        pixmapOffset += event->pos() - lastDragPos;
        lastDragPos = event->pos();
        update();
    }
}
//! [14]

//! [15]
void MandelbrotWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pixmapOffset += event->pos() - lastDragPos;
        lastDragPos = QPoint();

        int deltaX = (width() - pixmap.width()) / 2 - pixmapOffset.x();
        int deltaY = (height() - pixmap.height()) / 2 - pixmapOffset.y();
        scroll(deltaX, deltaY);
    }
}
//! [15]

//! [16]
void MandelbrotWidget::updatePixmap(const QImage &image, double scaleFactor)
{
    if (!lastDragPos.isNull())
        return;

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
    thread.render(centerX, centerY, curScale, size());
}
//! [17]

//! [18]
void MandelbrotWidget::scroll(int deltaX, int deltaY)
{
    centerX += deltaX * curScale;
    centerY += deltaY * curScale;
    update();
    thread.render(centerX, centerY, curScale, size());
}
//! [18]
