/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/qevent.h>
#include <QtWidgets/qtwidgetsglobal.h>
#if QT_CONFIG(graphicsview)
#include <QtWidgets/qgraphicssceneevent.h>
#endif

#include "private/qapplication_p.h"

QT_BEGIN_NAMESPACE

QEvent *QApplicationPrivate::cloneEvent(QEvent *e)
{
    switch (e->type()) {
#if QT_CONFIG(statustip)
    case QEvent::StatusTip:
        return new QStatusTipEvent(*static_cast<QStatusTipEvent*>(e));
#endif // QT_CONFIG(statustip)

#if QT_CONFIG(toolbar)
    case QEvent::ToolBarChange:
        return new QToolBarChangeEvent(*static_cast<QToolBarChangeEvent*>(e));
#endif // QT_CONFIG(toolbar)

#if QT_CONFIG(graphicsview)
    case QEvent::GraphicsSceneMouseMove:
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseRelease:
    case QEvent::GraphicsSceneMouseDoubleClick: {
        QGraphicsSceneMouseEvent *me = static_cast<QGraphicsSceneMouseEvent*>(e);
        QGraphicsSceneMouseEvent *me2 = new QGraphicsSceneMouseEvent(me->type());
        me2->setWidget(me->widget());
        me2->setPos(me->pos());
        me2->setScenePos(me->scenePos());
        me2->setScreenPos(me->screenPos());
// ### for all buttons
        me2->setButtonDownPos(Qt::LeftButton, me->buttonDownPos(Qt::LeftButton));
        me2->setButtonDownPos(Qt::RightButton, me->buttonDownPos(Qt::RightButton));
        me2->setButtonDownScreenPos(Qt::LeftButton, me->buttonDownScreenPos(Qt::LeftButton));
        me2->setButtonDownScreenPos(Qt::RightButton, me->buttonDownScreenPos(Qt::RightButton));
        me2->setLastPos(me->lastPos());
        me2->setLastScenePos(me->lastScenePos());
        me2->setLastScreenPos(me->lastScreenPos());
        me2->setButtons(me->buttons());
        me2->setButton(me->button());
        me2->setModifiers(me->modifiers());
        me2->setSource(me->source());
        me2->setFlags(me->flags());
        return me2;
    }

    case QEvent::GraphicsSceneContextMenu: {
        QGraphicsSceneContextMenuEvent *me = static_cast<QGraphicsSceneContextMenuEvent*>(e);
        QGraphicsSceneContextMenuEvent *me2 = new QGraphicsSceneContextMenuEvent(me->type());
        me2->setWidget(me->widget());
        me2->setPos(me->pos());
        me2->setScenePos(me->scenePos());
        me2->setScreenPos(me->screenPos());
        me2->setModifiers(me->modifiers());
        me2->setReason(me->reason());
        return me2;
    }

    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHoverMove:
    case QEvent::GraphicsSceneHoverLeave: {
        QGraphicsSceneHoverEvent *he = static_cast<QGraphicsSceneHoverEvent*>(e);
        QGraphicsSceneHoverEvent *he2 = new QGraphicsSceneHoverEvent(he->type());
        he2->setPos(he->pos());
        he2->setScenePos(he->scenePos());
        he2->setScreenPos(he->screenPos());
        he2->setLastPos(he->lastPos());
        he2->setLastScenePos(he->lastScenePos());
        he2->setLastScreenPos(he->lastScreenPos());
        he2->setModifiers(he->modifiers());
        return he2;
    }
    case QEvent::GraphicsSceneHelp:
        return new QHelpEvent(*static_cast<QHelpEvent*>(e));
    case QEvent::GraphicsSceneDragEnter:
    case QEvent::GraphicsSceneDragMove:
    case QEvent::GraphicsSceneDragLeave:
    case QEvent::GraphicsSceneDrop: {
        QGraphicsSceneDragDropEvent *dde = static_cast<QGraphicsSceneDragDropEvent*>(e);
        QGraphicsSceneDragDropEvent *dde2 = new QGraphicsSceneDragDropEvent(dde->type());
        dde2->setPos(dde->pos());
        dde2->setScenePos(dde->scenePos());
        dde2->setScreenPos(dde->screenPos());
        dde2->setButtons(dde->buttons());
        dde2->setModifiers(dde->modifiers());
        return dde2;
    }
    case QEvent::GraphicsSceneWheel: {
        QGraphicsSceneWheelEvent *we = static_cast<QGraphicsSceneWheelEvent*>(e);
        QGraphicsSceneWheelEvent *we2 = new QGraphicsSceneWheelEvent(we->type());
        we2->setPos(we->pos());
        we2->setScenePos(we->scenePos());
        we2->setScreenPos(we->screenPos());
        we2->setButtons(we->buttons());
        we2->setModifiers(we->modifiers());
        we2->setOrientation(we->orientation());
        we2->setDelta(we->delta());
        return we2;
    }

    case QEvent::GraphicsSceneResize: {
        QGraphicsSceneResizeEvent *re = static_cast<QGraphicsSceneResizeEvent*>(e);
        QGraphicsSceneResizeEvent *re2 = new QGraphicsSceneResizeEvent();
        re2->setOldSize(re->oldSize());
        re2->setNewSize(re->newSize());
        return re2;
    }
    case QEvent::GraphicsSceneMove: {
        QGraphicsSceneMoveEvent *me = static_cast<QGraphicsSceneMoveEvent*>(e);
        QGraphicsSceneMoveEvent *me2 = new QGraphicsSceneMoveEvent();
        me2->setWidget(me->widget());
        me2->setNewPos(me->newPos());
        me2->setOldPos(me->oldPos());
        return me2;
    }
#endif
    default:
        ;
    }
    return QApplicationPrivateBase::cloneEvent(e);
}

QT_END_NAMESPACE
