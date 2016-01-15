/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
        globalPos = static_cast<QMouseEvent *>(event)->globalPos();
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
