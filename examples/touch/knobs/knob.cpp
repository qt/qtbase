/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "knob.h"

#include <QBrush>
#include <QTouchEvent>

Knob::Knob()
    : QGraphicsEllipseItem(-50, -50, 100, 100)
{
    setAcceptTouchEvents(true);
    setBrush(Qt::lightGray);

    QGraphicsEllipseItem *leftItem = new QGraphicsEllipseItem(0, 0, 20, 20, this);
    leftItem->setPos(-40, -10);
    leftItem->setBrush(Qt::darkGreen);

    QGraphicsEllipseItem *rightItem = new QGraphicsEllipseItem(0, 0, 20, 20, this);
    rightItem->setPos(20, -10);
    rightItem->setBrush(Qt::darkRed);
}

bool Knob::sceneEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);

        if (touchEvent->touchPoints().count() == 2) {
            const QTouchEvent::TouchPoint &touchPoint1 = touchEvent->touchPoints().first();
            const QTouchEvent::TouchPoint &touchPoint2 = touchEvent->touchPoints().last();

            QLineF line1(touchPoint1.lastScenePos(), touchPoint2.lastScenePos());
            QLineF line2(touchPoint1.scenePos(), touchPoint2.scenePos());

            rotate(line2.angleTo(line1));
        }

        break;
    }

    default:
        return QGraphicsItem::sceneEvent(event);
    }

    return true;
}
