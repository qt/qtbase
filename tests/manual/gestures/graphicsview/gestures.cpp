// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
        break;
    case QEvent::TouchEnd:
        if (d->gestureFired)
            result = QGestureRecognizer::FinishGesture;
        else
            result = QGestureRecognizer::CancelGesture;
        break;
    case QEvent::TouchUpdate:
        if (d->state() != Qt::NoGesture) {
            QTouchEvent *ev = static_cast<QTouchEvent*>(event);
            if (ev->points().size() == 3) {
                d->gestureFired = true;
                result = QGestureRecognizer::TriggerGesture;
            } else {
                result = QGestureRecognizer::MayBeGesture;
                for (const QEventPoint &pt : ev->points()) {
                    const int distance = (pt.globalPosition().toPoint() - pt.globalPressPosition().toPoint()).manhattanLength();
                    if (distance > 20)
                        result = QGestureRecognizer::CancelGesture;
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
