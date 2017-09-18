/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
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

#include <QtWidgets>
#include <math.h>

#include "tabletcanvas.h"

//! [0]
TabletCanvas::TabletCanvas()
  : QWidget(nullptr)
  , m_alphaChannelValuator(TangentialPressureValuator)
  , m_colorSaturationValuator(NoValuator)
  , m_lineWidthValuator(PressureValuator)
  , m_color(Qt::red)
  , m_brush(m_color)
  , m_pen(m_brush, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
  , m_deviceDown(false)
{
    resize(500, 500);
    setAutoFillBackground(true);
    setAttribute(Qt::WA_TabletTracking);
}
//! [0]

//! [1]
bool TabletCanvas::saveImage(const QString &file)
{
    return m_pixmap.save(file);
}
//! [1]

//! [2]
bool TabletCanvas::loadImage(const QString &file)
{
    bool success = m_pixmap.load(file);

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
            if (!m_deviceDown) {
                m_deviceDown = true;
                lastPoint.pos = event->posF();
                lastPoint.pressure = event->pressure();
                lastPoint.rotation = event->rotation();
            }
            break;
        case QEvent::TabletMove:
#ifndef Q_OS_IOS
            if (event->device() == QTabletEvent::RotationStylus)
                updateCursor(event);
#endif
            if (m_deviceDown) {
                updateBrush(event);
                QPainter painter(&m_pixmap);
                paintPixmap(painter, event);
                lastPoint.pos = event->posF();
                lastPoint.pressure = event->pressure();
                lastPoint.rotation = event->rotation();
            }
            break;
        case QEvent::TabletRelease:
            if (m_deviceDown && event->buttons() == Qt::NoButton)
                m_deviceDown = false;
            update();
            break;
        default:
            break;
    }
    event->accept();
}
//! [3]

//! [4]
void TabletCanvas::initPixmap()
{
    qreal dpr = devicePixelRatioF();
    QPixmap newPixmap = QPixmap(width() * dpr, height() * dpr);
    newPixmap.setDevicePixelRatio(dpr);
    newPixmap.fill(Qt::white);
    QPainter painter(&newPixmap);
    if (!m_pixmap.isNull())
        painter.drawPixmap(0, 0, m_pixmap);
    painter.end();
    m_pixmap = newPixmap;
}

void TabletCanvas::paintEvent(QPaintEvent *)
{
    if (m_pixmap.isNull())
        initPixmap();
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_pixmap);
}
//! [4]

//! [5]
void TabletCanvas::paintPixmap(QPainter &painter, QTabletEvent *event)
{
    static qreal maxPenRadius = pressureToWidth(1.0);
    painter.setRenderHint(QPainter::Antialiasing);

    switch (event->device()) {
//! [6]
        case QTabletEvent::Airbrush:
            {
                painter.setPen(Qt::NoPen);
                QRadialGradient grad(lastPoint.pos, m_pen.widthF() * 10.0);
                QColor color = m_brush.color();
                color.setAlphaF(color.alphaF() * 0.25);
                grad.setColorAt(0, m_brush.color());
                grad.setColorAt(0.5, Qt::transparent);
                painter.setBrush(grad);
                qreal radius = grad.radius();
                painter.drawEllipse(event->posF(), radius, radius);
                update(QRect(event->pos() - QPoint(radius, radius), QSize(radius * 2, radius * 2)));
            }
            break;
        case QTabletEvent::RotationStylus:
            {
                m_brush.setStyle(Qt::SolidPattern);
                painter.setPen(Qt::NoPen);
                painter.setBrush(m_brush);
                QPolygonF poly;
                qreal halfWidth = pressureToWidth(lastPoint.pressure);
                QPointF brushAdjust(qSin(qDegreesToRadians(-lastPoint.rotation)) * halfWidth,
                                    qCos(qDegreesToRadians(-lastPoint.rotation)) * halfWidth);
                poly << lastPoint.pos + brushAdjust;
                poly << lastPoint.pos - brushAdjust;
                halfWidth = m_pen.widthF();
                brushAdjust = QPointF(qSin(qDegreesToRadians(-event->rotation())) * halfWidth,
                                      qCos(qDegreesToRadians(-event->rotation())) * halfWidth);
                poly << event->posF() - brushAdjust;
                poly << event->posF() + brushAdjust;
                painter.drawConvexPolygon(poly);
                update(poly.boundingRect().toRect());
            }
            break;
//! [6]
        case QTabletEvent::Puck:
        case QTabletEvent::FourDMouse:
            {
                const QString error(tr("This input device is not supported by the example."));
#if QT_CONFIG(statustip)
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
#if QT_CONFIG(statustip)
                QStatusTipEvent status(error);
                QApplication::sendEvent(this, &status);
#else
                qWarning() << error;
#endif
            }
            // FALL-THROUGH
        case QTabletEvent::Stylus:
            painter.setPen(m_pen);
            painter.drawLine(lastPoint.pos, event->posF());
            update(QRect(lastPoint.pos.toPoint(), event->pos()).normalized()
                   .adjusted(-maxPenRadius, -maxPenRadius, maxPenRadius, maxPenRadius));
            break;
    }
}
//! [5]

qreal TabletCanvas::pressureToWidth(qreal pressure)
{
    return pressure * 10 + 1;
}

//! [7]
void TabletCanvas::updateBrush(const QTabletEvent *event)
{
    int hue, saturation, value, alpha;
    m_color.getHsv(&hue, &saturation, &value, &alpha);

    int vValue = int(((event->yTilt() + 60.0) / 120.0) * 255);
    int hValue = int(((event->xTilt() + 60.0) / 120.0) * 255);
//! [7] //! [8]

    switch (m_alphaChannelValuator) {
        case PressureValuator:
            m_color.setAlphaF(event->pressure());
            break;
        case TangentialPressureValuator:
            if (event->device() == QTabletEvent::Airbrush)
                m_color.setAlphaF(qMax(0.01, (event->tangentialPressure() + 1.0) / 2.0));
            else
                m_color.setAlpha(255);
            break;
        case TiltValuator:
            m_color.setAlpha(maximum(abs(vValue - 127), abs(hValue - 127)));
            break;
        default:
            m_color.setAlpha(255);
    }

//! [8] //! [9]
    switch (m_colorSaturationValuator) {
        case VTiltValuator:
            m_color.setHsv(hue, vValue, value, alpha);
            break;
        case HTiltValuator:
            m_color.setHsv(hue, hValue, value, alpha);
            break;
        case PressureValuator:
            m_color.setHsv(hue, int(event->pressure() * 255.0), value, alpha);
            break;
        default:
            ;
    }

//! [9] //! [10]
    switch (m_lineWidthValuator) {
        case PressureValuator:
            m_pen.setWidthF(pressureToWidth(event->pressure()));
            break;
        case TiltValuator:
            m_pen.setWidthF(maximum(abs(vValue - 127), abs(hValue - 127)) / 12);
            break;
        default:
            m_pen.setWidthF(1);
    }

//! [10] //! [11]
    if (event->pointerType() == QTabletEvent::Eraser) {
        m_brush.setColor(Qt::white);
        m_pen.setColor(Qt::white);
        m_pen.setWidthF(event->pressure() * 10 + 1);
    } else {
        m_brush.setColor(m_color);
        m_pen.setColor(m_color);
    }
}
//! [11]

//! [12]
void TabletCanvas::updateCursor(const QTabletEvent *event)
{
    QCursor cursor;
    if (event->type() != QEvent::TabletLeaveProximity) {
        if (event->pointerType() == QTabletEvent::Eraser) {
            cursor = QCursor(QPixmap(":/images/cursor-eraser.png"), 3, 28);
        } else {
            switch (event->device()) {
            case QTabletEvent::Stylus:
                cursor = QCursor(QPixmap(":/images/cursor-pencil.png"), 0, 0);
                break;
            case QTabletEvent::Airbrush:
                cursor = QCursor(QPixmap(":/images/cursor-airbrush.png"), 3, 4);
                break;
            case QTabletEvent::RotationStylus: {
                QImage origImg(QLatin1String(":/images/cursor-felt-marker.png"));
                QImage img(32, 32, QImage::Format_ARGB32);
                QColor solid = m_color;
                solid.setAlpha(255);
                img.fill(solid);
                QPainter painter(&img);
                QTransform transform = painter.transform();
                transform.translate(16, 16);
                transform.rotate(event->rotation());
                painter.setTransform(transform);
                painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
                painter.drawImage(-24, -24, origImg);
                painter.setCompositionMode(QPainter::CompositionMode_HardLight);
                painter.drawImage(-24, -24, origImg);
                painter.end();
                cursor = QCursor(QPixmap::fromImage(img), 16, 16);
            } break;
            default:
                break;
            }
        }
    }
    setCursor(cursor);
}
//! [12]

void TabletCanvas::resizeEvent(QResizeEvent *)
{
    initPixmap();
}
