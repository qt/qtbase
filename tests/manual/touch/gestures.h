// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef GESTURES_H
#define GESTURES_H

#include <QGesture>
#include <QList>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QSharedPointer>

class QWidget;

// Draws an outline of an ellipse at center with the given diameters and color.
static inline void drawEllipse(const QPointF &center, qreal hDiameter, qreal vDiameter,
                               const QColor &color, QPainter &painter)
{
    const QPen oldPen = painter.pen();
    QPen pen = oldPen;
    pen.setColor(color);
    painter.setPen(pen);
    painter.drawEllipse(center, hDiameter / 2, vDiameter / 2);
    painter.setPen(oldPen);
}

// Fills an ellipse at center with the given diameters and color.
static inline void fillEllipse(const QPointF &center, qreal hDiameter, qreal vDiameter,
                               const QColor &color, QPainter &painter)
{
    QPainterPath painterPath;
    painterPath.addEllipse(center, hDiameter / 2, vDiameter / 2);
    painter.fillPath(painterPath, color);
}

// Hierarchy of classes containing gesture parameters and drawing functionality.
class Gesture {
    Q_DISABLE_COPY(Gesture)
public:
    static Gesture *fromQGesture(const QWidget *w, const QGesture *source);
    virtual ~Gesture() {}

    virtual void draw(const QRectF &rect, QPainter &painter) const = 0;

protected:
    explicit Gesture(const QWidget *w, const QGesture *source);

    QPointF drawHotSpot(const QRectF &rect, QPainter &painter) const;

private:
    Qt::GestureType m_type;
    QPointF m_hotSpot;
    bool m_hasHotSpot;
};

class PanGesture : public Gesture {
public:
    explicit PanGesture(const QWidget *w, const QPanGesture *source);

    void draw(const QRectF &rect, QPainter &painter) const override;

private:
    QPointF m_offset;
};

class SwipeGesture : public Gesture {
public:
    explicit SwipeGesture(const QWidget *w, const QSwipeGesture *source);

    void draw(const QRectF &rect, QPainter &painter) const override;

private:
    QSwipeGesture::SwipeDirection m_horizontal;
    QSwipeGesture::SwipeDirection m_vertical;
    qreal m_angle;
};

typedef QSharedPointer<Gesture> GesturePtr;
typedef QList<GesturePtr> GesturePtrs;

#endif // GESTURES_H
