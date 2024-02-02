// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mousepangesturerecognizer.h"

#include <QEvent>
#include <QMouseEvent>
#include <QGesture>

MousePanGestureRecognizer::MousePanGestureRecognizer()
{
}

QGesture *MousePanGestureRecognizer::create(QObject *)
{
    return new QPanGesture;
}

QGestureRecognizer::Result MousePanGestureRecognizer::recognize(QGesture *state, QObject *, QEvent *event)
{
    QPanGesture *g = static_cast<QPanGesture *>(state);
    if (event->type() == QEvent::TouchBegin) {
        // ignore the following mousepress event
        g->setProperty("ignoreMousePress", QVariant::fromValue<bool>(true));
    } else if (event->type() == QEvent::TouchEnd) {
        g->setProperty("ignoreMousePress", QVariant::fromValue<bool>(false));
    }
    QMouseEvent *me = static_cast<QMouseEvent *>(event);
    if (event->type() == QEvent::MouseButtonPress) {
        if (g->property("ignoreMousePress").toBool())
            return QGestureRecognizer::Ignore;
        g->setHotSpot(me->globalPos());
        g->setProperty("startPos", me->globalPos());
        g->setProperty("pressed", QVariant::fromValue<bool>(true));
        return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
    } else if (event->type() == QEvent::MouseMove) {
        if (g->property("pressed").toBool()) {
            QPoint offset = me->globalPos() - g->property("startPos").toPoint();
            g->setLastOffset(g->offset());
            g->setOffset(QPointF(offset.x(), offset.y()));
            return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
        }
        return QGestureRecognizer::CancelGesture;
    } else if (event->type() == QEvent::MouseButtonRelease) {
        if (g->property("pressed").toBool())
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
    g->setProperty("ignoreMousePress", QVariant::fromValue<bool>(false));
    QGestureRecognizer::reset(state);
}
