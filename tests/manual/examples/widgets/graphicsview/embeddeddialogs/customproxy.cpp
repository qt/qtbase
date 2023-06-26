// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "customproxy.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

CustomProxy::CustomProxy(QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QGraphicsProxyWidget(parent, wFlags), timeLine(new QTimeLine(250, this))
{
    connect(timeLine, &QTimeLine::valueChanged,
            this, &CustomProxy::updateStep);
    connect(timeLine, &QTimeLine::stateChanged,
            this, &CustomProxy::stateChanged);
}

QRectF CustomProxy::boundingRect() const
{
    return QGraphicsProxyWidget::boundingRect().adjusted(0, 0, 10, 10);
}

void CustomProxy::paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option,
                                   QWidget *widget)
{
    const QColor color(0, 0, 0, 64);

    QRectF r = windowFrameRect();
    QRectF right(r.right(), r.top() + 10, 10, r.height() - 10);
    QRectF bottom(r.left() + 10, r.bottom(), r.width(), 10);
    bool intersectsRight = right.intersects(option->exposedRect);
    bool intersectsBottom = bottom.intersects(option->exposedRect);
    if (intersectsRight && intersectsBottom) {
        QPainterPath path;
        path.addRect(right);
        path.addRect(bottom);
        painter->setPen(Qt::NoPen);
        painter->setBrush(color);
        painter->drawPath(path);
    } else if (intersectsBottom) {
        painter->fillRect(bottom, color);
    } else if (intersectsRight) {
        painter->fillRect(right, color);
    }

    QGraphicsProxyWidget::paintWindowFrame(painter, option, widget);
}

void CustomProxy::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsProxyWidget::hoverEnterEvent(event);
    scene()->setActiveWindow(this);
    if (qFuzzyCompare(timeLine->currentValue(), 1))
        zoomIn();
}

void CustomProxy::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsProxyWidget::hoverLeaveEvent(event);
    if (!popupShown
            && (timeLine->direction() != QTimeLine::Backward || qFuzzyIsNull(timeLine->currentValue()))) {
        zoomOut();
    }
}

bool CustomProxy::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    if (watched->isWindow()
            && (event->type() == QEvent::UngrabMouse || event->type() == QEvent::GrabMouse)) {
        popupShown = watched->isVisible();
        if (!popupShown && !isUnderMouse())
            zoomOut();
    }
    return QGraphicsProxyWidget::sceneEventFilter(watched, event);
}

QVariant CustomProxy::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemChildAddedChange || change == ItemChildRemovedChange) {
        if (change == ItemChildAddedChange) {
            currentPopup = qvariant_cast<QGraphicsItem *>(value);
            currentPopup->setCacheMode(ItemCoordinateCache);
            if (scene())
                currentPopup->installSceneEventFilter(this);
        } else if (scene()) {
            currentPopup->removeSceneEventFilter(this);
            currentPopup = nullptr;
        }
    } else if (currentPopup && change == ItemSceneHasChanged) {
        currentPopup->installSceneEventFilter(this);
    }
    return QGraphicsProxyWidget::itemChange(change, value);
}

void CustomProxy::updateStep(qreal step)
{
    QRectF r = boundingRect();
    setTransform(QTransform()
                 .translate(r.width() / 2, r.height() / 2)
                 .rotate(step * 30, Qt::XAxis)
                 .rotate(step * 10, Qt::YAxis)
                 .rotate(step * 5, Qt::ZAxis)
                 .scale(1 + 1.5 * step, 1 + 1.5 * step)
                 .translate(-r.width() / 2, -r.height() / 2));
}

void CustomProxy::stateChanged(QTimeLine::State state)
{
    if (state == QTimeLine::Running) {
        if (timeLine->direction() == QTimeLine::Forward)
            setCacheMode(ItemCoordinateCache);
    } else if (state == QTimeLine::NotRunning) {
        if (timeLine->direction() == QTimeLine::Backward)
            setCacheMode(DeviceCoordinateCache);
    }
}

void CustomProxy::zoomIn()
{
    if (timeLine->direction() != QTimeLine::Forward)
        timeLine->setDirection(QTimeLine::Forward);
    if (timeLine->state() == QTimeLine::NotRunning)
        timeLine->start();
}

void CustomProxy::zoomOut()
{
    if (timeLine->direction() != QTimeLine::Backward)
        timeLine->setDirection(QTimeLine::Backward);
    if (timeLine->state() == QTimeLine::NotRunning)
        timeLine->start();
}
