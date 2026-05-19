// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "gestures.h"

#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QWidget>

namespace {

qreal swipeDirectionAngle(QSwipeGesture::SwipeDirection d)
{
    switch (d) {
    case QSwipeGesture::NoDirection:
    case QSwipeGesture::Right:
        break;
    case QSwipeGesture::Left:
        return 180;
    case QSwipeGesture::Up:
        return 90;
    case QSwipeGesture::Down:
        return 270;
    }
    return 0;
}

// Draws an arrow assuming a mathematical coordinate system, Y axis pointing
// upwards, angle counterclockwise (that is, 45' is pointing up/right).
void drawArrow(const QPointF &center, qreal length, qreal angleDegrees,
               const QColor &color, int arrowSize, QPainter &painter)
{
    painter.save();
    painter.translate(center); // Transform center to (0,0) rotate and draw arrow pointing right.
    painter.rotate(-angleDegrees);
    QPen pen = painter.pen();
    pen.setColor(color);
    pen.setWidth(2);
    painter.setPen(pen);
    const QPointF endPoint(length, 0);
    painter.drawLine(QPointF(0, 0), endPoint);
    painter.drawLine(endPoint, endPoint + QPoint(-arrowSize, -arrowSize));
    painter.drawLine(endPoint, endPoint + QPoint(-arrowSize, arrowSize));
    painter.restore();
}

} // namespace

Gesture::Gesture(const QWidget *w, const QGesture *source)
    : m_type(source->gestureType())
    , m_hotSpot(w->mapFromGlobal(source->hotSpot().toPoint()))
    , m_hasHotSpot(source->hasHotSpot())
{
}

QPointF Gesture::drawHotSpot(const QRectF &rect, QPainter &painter) const
{
    const QPointF h = m_hasHotSpot ? m_hotSpot : rect.center();
    painter.drawEllipse(h, 15, 15);
    return h;
}

Gesture *Gesture::fromQGesture(const QWidget *w, const QGesture *source)
{
    Gesture *result = nullptr;
    switch (source->gestureType()) {
    case Qt::TapGesture:
    case Qt::TapAndHoldGesture:
    case Qt::PanGesture:
        result = new PanGesture(w, static_cast<const QPanGesture *>(source));
        break;
    case Qt::PinchGesture:
    case Qt::CustomGesture:
    case Qt::LastGestureType:
        break;
    case Qt::SwipeGesture:
        result = new SwipeGesture(w, static_cast<const QSwipeGesture *>(source));
        break;
    }
    return result;
}

PanGesture::PanGesture(const QWidget *w, const QPanGesture *source)
    : Gesture(w, source), m_offset(source->offset())
{
}

void PanGesture::draw(const QRectF &rect, QPainter &painter) const
{
    const QPointF hotSpot = drawHotSpot(rect, painter);
    painter.drawLine(hotSpot, hotSpot + m_offset);
}

SwipeGesture::SwipeGesture(const QWidget *w, const QSwipeGesture *source)
    : Gesture(w, source)
    , m_horizontal(source->horizontalDirection())
    , m_vertical(source->verticalDirection())
    , m_angle(source->swipeAngle())
{
}

void SwipeGesture::draw(const QRectF &rect, QPainter &painter) const
{
    enum { arrowLength = 50, arrowHeadSize = 10 };
    const QPointF hotSpot = drawHotSpot(rect, painter);
    drawArrow(hotSpot, arrowLength, swipeDirectionAngle(m_horizontal), Qt::red, arrowHeadSize, painter);
    drawArrow(hotSpot, arrowLength, swipeDirectionAngle(m_vertical), Qt::green, arrowHeadSize, painter);
    drawArrow(hotSpot, arrowLength, m_angle, Qt::blue, arrowHeadSize, painter);
}
