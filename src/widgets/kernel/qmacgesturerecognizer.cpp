// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmacgesturerecognizer_p.h"
#include "qgesture.h"
#include "qgesture_p.h"
#include "qevent.h"
#include "qwidget.h"
#include "qdebug.h"
#include <QtCore/qcoreapplication.h>

#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

QMacSwipeGestureRecognizer::QMacSwipeGestureRecognizer()
{
}

QGesture *QMacSwipeGestureRecognizer::create(QObject * /*target*/)
{
    return new QSwipeGesture;
}

QGestureRecognizer::Result
QMacSwipeGestureRecognizer::recognize(QGesture *gesture, QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::NativeGesture && obj->isWidgetType()) {
        QNativeGestureEvent *ev = static_cast<QNativeGestureEvent*>(event);
        switch (ev->gestureType()) {
            case Qt::SwipeNativeGesture: {
                QSwipeGesture *g = static_cast<QSwipeGesture *>(gesture);
                g->setSwipeAngle(ev->value());
                g->setHotSpot(ev->globalPosition());
                return QGestureRecognizer::FinishGesture | QGestureRecognizer::ConsumeEventHint;
                break; }
            default:
                break;
        }
    }

    return QGestureRecognizer::Ignore;
}

void QMacSwipeGestureRecognizer::reset(QGesture *gesture)
{
    QSwipeGesture *g = static_cast<QSwipeGesture *>(gesture);
    g->setSwipeAngle(0);
    QGestureRecognizer::reset(gesture);
}

////////////////////////////////////////////////////////////////////////

QMacPinchGestureRecognizer::QMacPinchGestureRecognizer()
{
}

QGesture *QMacPinchGestureRecognizer::create(QObject * /*target*/)
{
    return new QPinchGesture;
}

QGestureRecognizer::Result
QMacPinchGestureRecognizer::recognize(QGesture *gesture, QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::NativeGesture && obj->isWidgetType()) {
        QPinchGesture *g = static_cast<QPinchGesture *>(gesture);
        QNativeGestureEvent *ev = static_cast<QNativeGestureEvent*>(event);
        switch (ev->gestureType()) {
        case Qt::BeginNativeGesture:
            reset(gesture);
            g->setStartCenterPoint(static_cast<QWidget*>(obj)->mapFromGlobal(ev->globalPosition().toPoint()));
            g->setCenterPoint(g->startCenterPoint());
            g->setChangeFlags(QPinchGesture::CenterPointChanged);
            g->setTotalChangeFlags(g->totalChangeFlags() | g->changeFlags());
            g->setHotSpot(ev->globalPosition());
            return QGestureRecognizer::MayBeGesture | QGestureRecognizer::ConsumeEventHint;
        case Qt::RotateNativeGesture:
            g->setLastScaleFactor(g->scaleFactor());
            g->setLastRotationAngle(g->rotationAngle());
            g->setRotationAngle(g->rotationAngle() + ev->value());
            g->setChangeFlags(QPinchGesture::RotationAngleChanged);
            g->setTotalChangeFlags(g->totalChangeFlags() | g->changeFlags());
            g->setHotSpot(ev->globalPosition());
            return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
        case Qt::ZoomNativeGesture:
            g->setLastScaleFactor(g->scaleFactor());
            g->setLastRotationAngle(g->rotationAngle());
            g->setScaleFactor(1 + ev->value());
            g->setTotalScaleFactor(g->totalScaleFactor() * g->scaleFactor());
            g->setChangeFlags(QPinchGesture::ScaleFactorChanged);
            g->setTotalChangeFlags(g->totalChangeFlags() | g->changeFlags());
            g->setHotSpot(ev->globalPosition());
            return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
        case Qt::SmartZoomNativeGesture:
            g->setLastScaleFactor(g->scaleFactor());
            g->setLastRotationAngle(g->rotationAngle());
            g->setScaleFactor(ev->value() ? 1.7f : 1.0f);
            g->setChangeFlags(QPinchGesture::ScaleFactorChanged);
            g->setTotalChangeFlags(g->totalChangeFlags() | g->changeFlags());
            g->setHotSpot(ev->globalPosition());
            return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
        case Qt::EndNativeGesture:
            return QGestureRecognizer::FinishGesture | QGestureRecognizer::ConsumeEventHint;
        default:
            break;
        }
    }

    return QGestureRecognizer::Ignore;
}

void QMacPinchGestureRecognizer::reset(QGesture *gesture)
{
    QPinchGesture *g = static_cast<QPinchGesture *>(gesture);
    g->setChangeFlags({});
    g->setTotalChangeFlags({});
    g->setScaleFactor(1.0f);
    g->setTotalScaleFactor(1.0f);
    g->setLastScaleFactor(1.0f);
    g->setRotationAngle(0.0f);
    g->setTotalRotationAngle(0.0f);
    g->setLastRotationAngle(0.0f);
    g->setCenterPoint(QPointF());
    g->setStartCenterPoint(QPointF());
    g->setLastCenterPoint(QPointF());
    QGestureRecognizer::reset(gesture);
}

////////////////////////////////////////////////////////////////////////

QMacPanGestureRecognizer::QMacPanGestureRecognizer() : _panCanceled(true)
{
}

QGesture *QMacPanGestureRecognizer::create(QObject *target)
{
    if (!target)
        return new QPanGesture;

    if (QWidget *w = qobject_cast<QWidget *>(target)) {
        w->setAttribute(Qt::WA_AcceptTouchEvents);
        w->setAttribute(Qt::WA_TouchPadAcceptSingleTouchEvents);
        return new QPanGesture;
    }
    return nullptr;
}

void QMacPanGestureRecognizer::timerEvent(QTimerEvent *ev)
{
    if (ev->timerId() == _panTimer.timerId()) {
        if (_panTimer.isActive())
           _panTimer.stop();
        if (_target)
            QCoreApplication::sendEvent(_target, ev);
    }
}

QGestureRecognizer::Result
QMacPanGestureRecognizer::recognize(QGesture *gesture, QObject *target, QEvent *event)
{
    const int panBeginDelay = 300;
    const int panBeginRadius = 3;

    QPanGesture *g = static_cast<QPanGesture *>(gesture);

    switch (event->type()) {
    case QEvent::TouchBegin: {
        const QTouchEvent *ev = static_cast<const QTouchEvent*>(event);
        if (ev->points().size() == 1) {
            reset(gesture);
            _startPos = QCursor::pos();
            _target = target;
            _panTimer.start(panBeginDelay, this);
            _panCanceled = false;
            return QGestureRecognizer::MayBeGesture;
        }
        break;}
    case QEvent::TouchEnd: {
        if (_panCanceled)
            break;

        const QTouchEvent *ev = static_cast<const QTouchEvent*>(event);
        if (ev->points().size() == 1)
            return QGestureRecognizer::FinishGesture;
        break;}
    case QEvent::TouchUpdate: {
        if (_panCanceled)
            break;

        const QTouchEvent *ev = static_cast<const QTouchEvent*>(event);
        if (ev->points().size() == 1) {
            if (_panTimer.isActive()) {
                // INVARIANT: Still in maybeGesture. Check if the user
                // moved his finger so much that it makes sense to cancel the pan:
                const QPointF p = QCursor::pos();
                if ((p - _startPos).manhattanLength() > panBeginRadius) {
                    _panCanceled = true;
                    _panTimer.stop();
                    return QGestureRecognizer::CancelGesture;
                }
            } else {
                const QPointF p = QCursor::pos();
                const QPointF posOffset = p - _startPos;
                g->setLastOffset(g->offset());
                g->setOffset(QPointF(posOffset.x(), posOffset.y()));
                g->setHotSpot(_startPos);
                return QGestureRecognizer::TriggerGesture;
            }
        } else if (_panTimer.isActive()) {
            // I only want to cancel the pan if the user is pressing
            // more than one finger, and the pan hasn't started yet:
            _panCanceled = true;
            _panTimer.stop();
            return QGestureRecognizer::CancelGesture;
        }
        break;}
    case QEvent::Timer: {
        QTimerEvent *ev = static_cast<QTimerEvent *>(event);
        if (ev->timerId() == _panTimer.timerId()) {
            if (_panCanceled)
                break;
            // Begin new pan session!
            _startPos = QCursor::pos();
            g->setHotSpot(_startPos);
            return QGestureRecognizer::TriggerGesture | QGestureRecognizer::ConsumeEventHint;
        }
        break; }
    default:
        break;
    }

    return QGestureRecognizer::Ignore;
}

void QMacPanGestureRecognizer::reset(QGesture *gesture)
{
    QPanGesture *g = static_cast<QPanGesture *>(gesture);
    _startPos = QPointF();
    _panCanceled = true;
    _panTimer.stop();
    g->setOffset(QPointF(0, 0));
    g->setLastOffset(QPointF(0, 0));
    g->setAcceleration(qreal(1));
    QGestureRecognizer::reset(gesture);
}

QT_END_NAMESPACE

#endif // QT_NO_GESTURES
