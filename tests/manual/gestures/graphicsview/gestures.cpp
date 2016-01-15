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

#include "gestures.h"

#include <QTouchEvent>

Qt::GestureType ThreeFingerSlideGesture::Type = Qt::CustomGesture;

QGesture *ThreeFingerSlideGestureRecognizer::create(QObject *)
{
    return new ThreeFingerSlideGesture;
}

QGestureRecognizer::Result ThreeFingerSlideGestureRecognizer::recognize(QGesture *state, QObject *, QEvent *event)
{
    ThreeFingerSlideGesture *d = static_cast<ThreeFingerSlideGesture *>(state);
    QGestureRecognizer::Result result;
    switch (event->type()) {
    case QEvent::TouchBegin:
        result = QGestureRecognizer::MayBeGesture;
    case QEvent::TouchEnd:
        if (d->gestureFired)
            result = QGestureRecognizer::FinishGesture;
        else
            result = QGestureRecognizer::CancelGesture;
    case QEvent::TouchUpdate:
        if (d->state() != Qt::NoGesture) {
            QTouchEvent *ev = static_cast<QTouchEvent*>(event);
            if (ev->touchPoints().size() == 3) {
                d->gestureFired = true;
                result = QGestureRecognizer::TriggerGesture;
            } else {
                result = QGestureRecognizer::MayBeGesture;
                for (int i = 0; i < ev->touchPoints().size(); ++i) {
                    const QTouchEvent::TouchPoint &pt = ev->touchPoints().at(i);
                    const int distance = (pt.pos().toPoint() - pt.startPos().toPoint()).manhattanLength();
                    if (distance > 20) {
                        result = QGestureRecognizer::CancelGesture;
                    }
                }
            }
        } else {
            result = QGestureRecognizer::CancelGesture;
        }

        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
        if (d->state() != Qt::NoGesture)
            result = QGestureRecognizer::Ignore;
        else
            result = QGestureRecognizer::CancelGesture;
        break;
    default:
        result = QGestureRecognizer::Ignore;
        break;
    }
    return result;
}

void ThreeFingerSlideGestureRecognizer::reset(QGesture *state)
{
    static_cast<ThreeFingerSlideGesture *>(state)->gestureFired = false;
    QGestureRecognizer::reset(state);
}


QGesture *RotateGestureRecognizer::create(QObject *)
{
    return new QGesture;
}

QGestureRecognizer::Result RotateGestureRecognizer::recognize(QGesture *, QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
        break;
    default:
        break;
    }
    return QGestureRecognizer::Ignore;
}

void RotateGestureRecognizer::reset(QGesture *state)
{
    QGestureRecognizer::reset(state);
}

#include "moc_gestures.cpp"
