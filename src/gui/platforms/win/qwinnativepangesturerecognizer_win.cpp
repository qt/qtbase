/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qwinnativepangesturerecognizer_win_p.h"

#include "qevent.h"
#include "qgraphicsitem.h"
#include "qgesture.h"

#include "private/qgesture_p.h"
#include "private/qevent_p.h"
#include "private/qapplication_p.h"
#include "private/qwidget_p.h"

#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_NATIVE_GESTURES)

QWinNativePanGestureRecognizer::QWinNativePanGestureRecognizer()
{
}

QGesture *QWinNativePanGestureRecognizer::create(QObject *target)
{
    if (!target)
        return new QPanGesture; // a special case
    if (!target->isWidgetType())
        return 0;
    if (qobject_cast<QGraphicsObject *>(target))
        return 0;

    QWidget *q = static_cast<QWidget *>(target);
    QWidgetPrivate *d = q->d_func();
    d->nativeGesturePanEnabled = true;
    d->winSetupGestures();

    return new QPanGesture;
}

QGestureRecognizer::Result QWinNativePanGestureRecognizer::recognize(QGesture *state,
                                                                     QObject *,
                                                                     QEvent *event)
{
    QPanGesture *q = static_cast<QPanGesture*>(state);
    QPanGesturePrivate *d = q->d_func();

    QGestureRecognizer::Result result = QGestureRecognizer::Ignore;
    if (event->type() == QEvent::NativeGesture) {
        QNativeGestureEvent *ev = static_cast<QNativeGestureEvent*>(event);
        switch(ev->gestureType) {
        case QNativeGestureEvent::GestureBegin:
            break;
        case QNativeGestureEvent::Pan:
            result = QGestureRecognizer::TriggerGesture;
            event->accept();
            break;
        case QNativeGestureEvent::GestureEnd:
            if (q->state() == Qt::NoGesture)
                return QGestureRecognizer::Ignore; // some other gesture has ended
            result = QGestureRecognizer::FinishGesture;
            break;
        default:
            return QGestureRecognizer::Ignore;
        }
        if (q->state() == Qt::NoGesture) {
            d->lastOffset = d->offset = QPointF();
            d->startPosition = ev->position;
        } else {
            d->lastOffset = d->offset;
            d->offset = QPointF(ev->position.x() - d->startPosition.x(),
                                ev->position.y() - d->startPosition.y());
        }
    }
    return result;
}

void QWinNativePanGestureRecognizer::reset(QGesture *state)
{
    QPanGesture *pan = static_cast<QPanGesture*>(state);
    QPanGesturePrivate *d = pan->d_func();

    d->lastOffset = d->offset = QPointF();
    d->startPosition = QPoint();
    d->acceleration = 0;

    QGestureRecognizer::reset(state);
}

#endif // QT_NO_NATIVE_GESTURES

QT_END_NAMESPACE

#endif // QT_NO_GESTURES
