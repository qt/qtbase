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

#include "tabletcanvas.h"

//! [0]
TabletCanvas::TabletCanvas()
{
    resize(500, 500);
    myBrush = QBrush();
    myPen = QPen();
    initPixmap();
    setAutoFillBackground(true);
    deviceDown = false;
    myColor = Qt::red;
    myTabletDevice = QTabletEvent::Stylus;
    alphaChannelType = NoAlpha;
    colorSaturationType = NoSaturation;
    lineWidthType = LineWidthPressure;
}

void TabletCanvas::initPixmap()
{
    QPixmap newPixmap = QPixmap(width(), height());
    newPixmap.fill(Qt::white);
    QPainter painter(&newPixmap);
    if (!pixmap.isNull())
        painter.drawPixmap(0, 0, pixmap);
    painter.end();
    pixmap = newPixmap;
}
//! [0]

//! [1]
bool TabletCanvas::saveImage(const QString &file)
{
    return pixmap.save(file);
}
//! [1]

//! [2]
bool TabletCanvas::loadImage(const QString &file)
{
    bool success = pixmap.load(file);

    if (success) {
        update();
        return true;
    }
    return false;
}
//! [2]

//! [3]
void TabletCanvas::tabletEvent(QTabletEvent *event)
{

    switch (event->type()) {
        case QEvent::TabletPress:
            if (!deviceDown) {
                deviceDown = true;
                polyLine[0] = polyLine[1] = polyLine[2] = event->pos();
            }
            break;
        case QEvent::TabletRelease:
            if (deviceDown)
                deviceDown = false;
            break;
        case QEvent::TabletMove:
            polyLine[2] = polyLine[1];
            polyLine[1] = polyLine[0];
            polyLine[0] = event->pos();

            if (deviceDown) {
                updateBrush(event);
                QPainter painter(&pixmap);
                paintPixmap(painter, event);
            }
            break;
        default:
            break;
    }
    update();
}
//! [3]

//! [4]
void TabletCanvas::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, pixmap);
}
//! [4]

//! [5]
void TabletCanvas::paintPixmap(QPainter &painter, QTabletEvent *event)
{
    QPoint brushAdjust(10, 10);

    switch (myTabletDevice) {
        case QTabletEvent::Airbrush:
            myBrush.setColor(myColor);
            myBrush.setStyle(brushPattern(event->pressure()));
            painter.setPen(Qt::NoPen);
            painter.setBrush(myBrush);

            for (int i = 0; i < 3; ++i) {
                painter.drawEllipse(QRect(polyLine[i] - brushAdjust,
                                    polyLine[i] + brushAdjust));
            }
            break;
        case QTabletEvent::Puck:
        case QTabletEvent::FourDMouse:
        case QTabletEvent::RotationStylus:
            {
                const QString error(tr("This input device is not supported by the example."));
#ifndef QT_NO_STATUSTIP
                QStatusTipEvent status(error);
                QApplication::sendEvent(this, &status);
#else
                qWarning() << error;
#endif
            }
            break;
        default:
            {
                const QString error(tr("Unknown tablet device - treating as stylus"));
#ifndef QT_NO_STATUSTIP
                QStatusTipEvent status(error);
                QApplication::sendEvent(this, &status);
#else
                qWarning() << error;
#endif
            }
            // FALL-THROUGH
        case QTabletEvent::Stylus:
            painter.setBrush(myBrush);
            painter.setPen(myPen);
            painter.drawLine(polyLine[1], event->pos());
            break;
    }
}
//! [5]

//! [6]
Qt::BrushStyle TabletCanvas::brushPattern(qreal value)
{
    int pattern = int((value) * 100.0) % 7;

    switch (pattern) {
        case 0:
            return Qt::SolidPattern;
        case 1:
            return Qt::Dense1Pattern;
        case 2:
            return Qt::Dense2Pattern;
        case 3:
            return Qt::Dense3Pattern;
        case 4:
            return Qt::Dense4Pattern;
        case 5:
            return Qt::Dense5Pattern;
        case 6:
            return Qt::Dense6Pattern;
        default:
            return Qt::Dense7Pattern;
    }
}
//! [6]

//! [7]
void TabletCanvas::updateBrush(QTabletEvent *event)
{
    int hue, saturation, value, alpha;
    myColor.getHsv(&hue, &saturation, &value, &alpha);

    int vValue = int(((event->yTilt() + 60.0) / 120.0) * 255);
    int hValue = int(((event->xTilt() + 60.0) / 120.0) * 255);
//! [7] //! [8]

    switch (alphaChannelType) {
        case AlphaPressure:
            myColor.setAlpha(int(event->pressure() * 255.0));
            break;
        case AlphaTilt:
            myColor.setAlpha(maximum(abs(vValue - 127), abs(hValue - 127)));
            break;
        default:
            myColor.setAlpha(255);
    }

//! [8] //! [9]
    switch (colorSaturationType) {
        case SaturationVTilt:
            myColor.setHsv(hue, vValue, value, alpha);
            break;
        case SaturationHTilt:
            myColor.setHsv(hue, hValue, value, alpha);
            break;
        case SaturationPressure:
            myColor.setHsv(hue, int(event->pressure() * 255.0), value, alpha);
            break;
        default:
            ;
    }

//! [9] //! [10]
    switch (lineWidthType) {
        case LineWidthPressure:
            myPen.setWidthF(event->pressure() * 10 + 1);
            break;
        case LineWidthTilt:
            myPen.setWidthF(maximum(abs(vValue - 127), abs(hValue - 127)) / 12);
            break;
        default:
            myPen.setWidthF(1);
    }

//! [10] //! [11]
    if (event->pointerType() == QTabletEvent::Eraser) {
        myBrush.setColor(Qt::white);
        myPen.setColor(Qt::white);
        myPen.setWidthF(event->pressure() * 10 + 1);
    } else {
        myBrush.setColor(myColor);
        myPen.setColor(myColor);
    }
}
//! [11]

void TabletCanvas::resizeEvent(QResizeEvent *)
{
    initPixmap();
    polyLine[0] = polyLine[1] = polyLine[2] = QPoint();
}
