// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mousepangesturerecognizer.h"

#include <QEvent>
#include <QVariant>
#include <QGraphicsSceneMouseEvent>
#include <QMouseEvent>
#include <QGesture>

MousePanGestureRecognizer::MousePanGestureRecognizer()
{
}

QGesture* MousePanGestureRecognizer::create(QObject *)
{
    return new QPanGesture;
}

QGestureRecognizer::Result MousePanGestureRecognizer::recognize(QGesture *state, QObject *, QEvent *event)
{
    QPanGesture *g = static_cast<QPanGesture *>(state);
    QPoint globalPos;
    switch (event->type()) {
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseDoubleClick:
    case QEvent::GraphicsSceneMouseMove:
    case QEvent::GraphicsSceneMouseRelease:
        globalPos = static_cast<QGraphicsSceneMouseEvent *>(event)->screenPos();
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        globalPos = static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
        break;
    default:
        break;
    }
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick
        || event->type() == QEvent::GraphicsSceneMousePress || event->type() == QEvent::GraphicsSceneMouseDoubleClick) {
        g->setHotSpot(globalPos);
        g->setProperty("startPos", globalPos);
        g->setProperty("pressed", QVariant::fromValue<bool>(true));
        return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
    } else if (event->type() == QEvent::MouseMove || event->type() == QEvent::GraphicsSceneMouseMove) {
        if (g->property("pressed").toBool()) {
            QPoint offset = globalPos - g->property("startPos").toPoint();
            g->setLastOffset(g->offset());
            g->setOffset(QPointF(offset.x(), offset.y()));
            return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
        }
        return QGestureRecognizer::CancelGesture;
    } else if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::GraphicsSceneMouseRelease) {
        return QGestureRecognizer::FinishGesture | QGestureRecognizer::ConsumeEventHint;
    }
    return QGestureRecognizer::Ignore;
}

void MousePanGestureRecognizer::reset(QGesture *state)
{
    QPanGesture *g = static_cast<QPanGesture *>(state);
    g->setLastOffset(QPointF());
    g->setOffset(QPointF());
    g->setAcceleration(0);
    g->setProperty("startPos", QVariant());
    g->setProperty("pressed", QVariant::fromValue<bool>(false));
    QGestureRecognizer::reset(state);
}
