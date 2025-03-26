// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstandardgestures_p.h"
#include "qgesture.h"
#include "qgesture_p.h"
#include "qevent.h"
#include "qwidget.h"
#if QT_CONFIG(scrollarea)
#include "qabstractscrollarea.h"
#endif
#if QT_CONFIG(graphicsview)
#include <qgraphicssceneevent.h>
#endif
#include "qdebug.h"

#ifndef QT_NO_GESTURES

using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

// If the change in scale for a single touch event is out of this range,
// we consider it to be spurious.
static const qreal kSingleStepScaleMax = 2.0;
static const qreal kSingleStepScaleMin = 0.1;

QGesture *QPanGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
#if (defined(Q_OS_MACOS) || defined(Q_OS_WIN)) && !defined(QT_NO_NATIVE_GESTURES) && QT_CONFIG(scrollarea)
        // for scroll areas on Windows and OS X we want to use native gestures instead
        if (!qobject_cast<QAbstractScrollArea *>(target->parent()))
            static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
#else
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
#endif
    }
    return new QPanGesture;
}

static QPointF panOffset(const QList<QEventPoint> &touchPoints, int maxCount)
{
    QPointF result;
    const int count = qMin(touchPoints.size(), maxCount);
    for (int p = 0; p < count; ++p)
        result += touchPoints.at(p).position() - touchPoints.at(p).pressPosition();
    return result / qreal(count);
}

QGestureRecognizer::Result QPanGestureRecognizer::recognize(QGesture *state,
                                                            QObject *,
                                                            QEvent *event)
{
    QPanGesture *q = static_cast<QPanGesture *>(state);
    QPanGesturePrivate *d = q->d_func();

    QGestureRecognizer::Result result = QGestureRecognizer::Ignore;
    switch (event->type()) {
    case QEvent::TouchBegin: {
        result = QGestureRecognizer::MayBeGesture;
        d->lastOffset = d->offset = QPointF();
        d->pointCount = m_pointCount;
        break;
    }
    case QEvent::TouchEnd: {
        if (q->state() != Qt::NoGesture) {
            const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);
            if (ev->points().size() == d->pointCount) {
                d->lastOffset = d->offset;
                d->offset = panOffset(ev->points(), d->pointCount);
            }
            result = QGestureRecognizer::FinishGesture;
        } else {
            result = QGestureRecognizer::CancelGesture;
        }
        break;
    }
    case QEvent::TouchUpdate: {
        const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);
        if (ev->points().size() >= d->pointCount) {
            d->lastOffset = d->offset;
            d->offset = panOffset(ev->points(), d->pointCount);
            if (d->offset.x() > 10  || d->offset.y() > 10 ||
                d->offset.x() < -10 || d->offset.y() < -10) {
                q->setHotSpot(ev->points().first().globalPressPosition());
                result = QGestureRecognizer::TriggerGesture;
            } else {
                result = QGestureRecognizer::MayBeGesture;
            }
        }
        break;
    }
    default:
        break;
    }
    return result;
}

void QPanGestureRecognizer::reset(QGesture *state)
{
    QPanGesture *pan = static_cast<QPanGesture*>(state);
    QPanGesturePrivate *d = pan->d_func();

    d->lastOffset = d->offset = QPointF();
    d->acceleration = 0;

    QGestureRecognizer::reset(state);
}


//
// QPinchGestureRecognizer
//

QPinchGestureRecognizer::QPinchGestureRecognizer()
{
}

QGesture *QPinchGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    return new QPinchGesture;
}

QGestureRecognizer::Result QPinchGestureRecognizer::recognize(QGesture *state,
                                                              QObject *,
                                                              QEvent *event)
{
    QPinchGesture *q = static_cast<QPinchGesture *>(state);
    QPinchGesturePrivate *d = q->d_func();

    QGestureRecognizer::Result result = QGestureRecognizer::Ignore;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        result = QGestureRecognizer::MayBeGesture;
        break;
    }
    case QEvent::TouchEnd: {
        if (q->state() != Qt::NoGesture) {
            result = QGestureRecognizer::FinishGesture;
        } else {
            result = QGestureRecognizer::CancelGesture;
        }
        break;
    }
    case QEvent::TouchUpdate: {
        const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);
        d->changeFlags = { };
        if (ev->points().size() == 2) {
            const QEventPoint &p1 = ev->points().at(0);
            const QEventPoint &p2 = ev->points().at(1);

            d->hotSpot = p1.globalPosition();
            d->isHotSpotSet = true;

            QPointF centerPoint = (p1.globalPosition() + p2.globalPosition()) / 2.0;
            if (d->isNewSequence) {
                d->startPosition[0] = p1.globalPosition();
                d->startPosition[1] = p2.globalPosition();
                d->lastCenterPoint = centerPoint;
            } else {
                d->lastCenterPoint = d->centerPoint;
            }
            d->centerPoint = centerPoint;

            d->changeFlags |= QPinchGesture::CenterPointChanged;

            if (d->isNewSequence) {
                d->scaleFactor = 1.0;
                d->lastScaleFactor = 1.0;
            } else {
                d->lastScaleFactor = d->scaleFactor;
                QLineF line(p1.globalPosition(), p2.globalPosition());
                QLineF lastLine(p1.globalLastPosition(),  p2.globalLastPosition());
                qreal newScaleFactor = line.length() / lastLine.length();
                if (newScaleFactor > kSingleStepScaleMax || newScaleFactor < kSingleStepScaleMin)
                    return QGestureRecognizer::Ignore;
                d->scaleFactor = newScaleFactor;
            }
            d->totalScaleFactor = d->totalScaleFactor * d->scaleFactor;
            d->changeFlags |= QPinchGesture::ScaleFactorChanged;

            qreal angle = QLineF(p1.globalPosition(), p2.globalPosition()).angle();
            if (angle > 180)
                angle -= 360;
            qreal startAngle = QLineF(p1.globalPressPosition(), p2.globalPressPosition()).angle();
            if (startAngle > 180)
                startAngle -= 360;
            const qreal rotationAngle = startAngle - angle;
            if (d->isNewSequence)
                d->lastRotationAngle = 0.0;
            else
                d->lastRotationAngle = d->rotationAngle;
            d->rotationAngle = rotationAngle;
            d->totalRotationAngle += d->rotationAngle - d->lastRotationAngle;
            d->changeFlags |= QPinchGesture::RotationAngleChanged;

            d->totalChangeFlags |= d->changeFlags;
            d->isNewSequence = false;
            result = QGestureRecognizer::TriggerGesture;
        } else {
            d->isNewSequence = true;
            if (q->state() == Qt::NoGesture)
                result = QGestureRecognizer::Ignore;
            else
                result = QGestureRecognizer::FinishGesture;
        }
        break;
    }
    default:
        break;
    }
    return result;
}

void QPinchGestureRecognizer::reset(QGesture *state)
{
    QPinchGesture *pinch = static_cast<QPinchGesture *>(state);
    QPinchGesturePrivate *d = pinch->d_func();

    d->totalChangeFlags = d->changeFlags = { };

    d->startCenterPoint = d->lastCenterPoint = d->centerPoint = QPointF();
    d->totalScaleFactor = d->lastScaleFactor = d->scaleFactor = 1;
    d->totalRotationAngle = d->lastRotationAngle = d->rotationAngle = 0;

    d->isNewSequence = true;
    d->startPosition[0] = d->startPosition[1] = QPointF();

    QGestureRecognizer::reset(state);
}

//
// QSwipeGestureRecognizer
//

QSwipeGestureRecognizer::QSwipeGestureRecognizer()
{
}

QGesture *QSwipeGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    return new QSwipeGesture;
}

QGestureRecognizer::Result QSwipeGestureRecognizer::recognize(QGesture *state,
                                                              QObject *,
                                                              QEvent *event)
{
    QSwipeGesture *q = static_cast<QSwipeGesture *>(state);
    QSwipeGesturePrivate *d = q->d_func();

    QGestureRecognizer::Result result = QGestureRecognizer::Ignore;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        d->velocityValue = 1;
        d->time.start();
        d->state = QSwipeGesturePrivate::Started;
        result = QGestureRecognizer::MayBeGesture;
        break;
    }
    case QEvent::TouchEnd: {
        if (q->state() != Qt::NoGesture) {
            result = QGestureRecognizer::FinishGesture;
        } else {
            result = QGestureRecognizer::CancelGesture;
        }
        break;
    }
    case QEvent::TouchUpdate: {
        const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);
        if (d->state == QSwipeGesturePrivate::NoGesture)
            result = QGestureRecognizer::CancelGesture;
        else if (ev->points().size() == 3) {
            d->state = QSwipeGesturePrivate::ThreePointsReached;
            const QEventPoint &p1 = ev->points().at(0);
            const QEventPoint &p2 = ev->points().at(1);
            const QEventPoint &p3 = ev->points().at(2);

            if (d->lastPositions[0].isNull()) {
                d->lastPositions[0] = p1.globalPressPosition().toPoint();
                d->lastPositions[1] = p2.globalPressPosition().toPoint();
                d->lastPositions[2] = p3.globalPressPosition().toPoint();
            }
            d->hotSpot = p1.globalPosition();
            d->isHotSpotSet = true;

            int xDistance = (p1.globalPosition().x() - d->lastPositions[0].x() +
                             p2.globalPosition().x() - d->lastPositions[1].x() +
                             p3.globalPosition().x() - d->lastPositions[2].x()) / 3;
            int yDistance = (p1.globalPosition().y() - d->lastPositions[0].y() +
                             p2.globalPosition().y() - d->lastPositions[1].y() +
                             p3.globalPosition().y() - d->lastPositions[2].y()) / 3;

            const int distance = xDistance >= yDistance ? xDistance : yDistance;
            int elapsedTime = d->time.restart();
            if (!elapsedTime)
                elapsedTime = 1;
            d->velocityValue = 0.9 * d->velocityValue + (qreal) distance / elapsedTime;
            d->swipeAngle = QLineF(p1.globalPressPosition(), p1.globalPosition()).angle();

            static const int MoveThreshold = 50;
            static const int directionChangeThreshold = MoveThreshold / 8;
            if (qAbs(xDistance) > MoveThreshold || qAbs(yDistance) > MoveThreshold) {
                // measure the distance to check if the direction changed
                d->lastPositions[0] = p1.globalPosition().toPoint();
                d->lastPositions[1] = p2.globalPosition().toPoint();
                d->lastPositions[2] = p3.globalPosition().toPoint();
                result = QGestureRecognizer::TriggerGesture;
                // QTBUG-46195, small changes in direction should not cause the gesture to be canceled.
                if (d->verticalDirection == QSwipeGesture::NoDirection || qAbs(yDistance) > directionChangeThreshold) {
                    const QSwipeGesture::SwipeDirection vertical = yDistance > 0
                        ? QSwipeGesture::Down : QSwipeGesture::Up;
                    if (d->verticalDirection != QSwipeGesture::NoDirection && d->verticalDirection != vertical)
                        result = QGestureRecognizer::CancelGesture;
                    d->verticalDirection = vertical;
                }
                if (d->horizontalDirection == QSwipeGesture::NoDirection || qAbs(xDistance) > directionChangeThreshold) {
                    const QSwipeGesture::SwipeDirection horizontal = xDistance > 0
                        ? QSwipeGesture::Right : QSwipeGesture::Left;
                    if (d->horizontalDirection != QSwipeGesture::NoDirection && d->horizontalDirection != horizontal)
                        result = QGestureRecognizer::CancelGesture;
                    d->horizontalDirection = horizontal;
                }
            } else {
                if (q->state() != Qt::NoGesture)
                    result = QGestureRecognizer::TriggerGesture;
                else
                    result = QGestureRecognizer::MayBeGesture;
            }
        } else if (ev->points().size() > 3) {
            result = QGestureRecognizer::CancelGesture;
        } else { // less than 3 touch points
            switch (d->state) {
            case QSwipeGesturePrivate::NoGesture:
                result = QGestureRecognizer::MayBeGesture;
                break;
            case QSwipeGesturePrivate::Started:
                result = QGestureRecognizer::Ignore;
                break;
            case QSwipeGesturePrivate::ThreePointsReached:
                result = (ev->touchPointStates() & QEventPoint::State::Pressed)
                    ? QGestureRecognizer::CancelGesture : QGestureRecognizer::Ignore;
                break;
            }
        }
        break;
    }
    default:
        break;
    }
    return result;
}

void QSwipeGestureRecognizer::reset(QGesture *state)
{
    QSwipeGesture *q = static_cast<QSwipeGesture *>(state);
    QSwipeGesturePrivate *d = q->d_func();

    d->verticalDirection = d->horizontalDirection = QSwipeGesture::NoDirection;
    d->swipeAngle = 0;

    d->lastPositions[0] = d->lastPositions[1] = d->lastPositions[2] = QPoint();
    d->state = QSwipeGesturePrivate::NoGesture;
    d->velocityValue = 0;
    d->time.invalidate();

    QGestureRecognizer::reset(state);
}

//
// QTapGestureRecognizer
//

QTapGestureRecognizer::QTapGestureRecognizer()
{
}

QGesture *QTapGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    return new QTapGesture;
}

QGestureRecognizer::Result QTapGestureRecognizer::recognize(QGesture *state,
                                                            QObject *,
                                                            QEvent *event)
{
    QTapGesture *q = static_cast<QTapGesture *>(state);
    QTapGesturePrivate *d = q->d_func();

    QGestureRecognizer::Result result = QGestureRecognizer::CancelGesture;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        const auto ev = static_cast<const QTouchEvent *>(event);
        d->position = ev->points().at(0).position();
        q->setHotSpot(ev->points().at(0).globalPosition());
        result = QGestureRecognizer::TriggerGesture;
        break;
    }
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        const auto ev = static_cast<const QTouchEvent *>(event);
        if (q->state() != Qt::NoGesture && ev->points().size() == 1) {
            const QEventPoint &p = ev->points().at(0);
            QPoint delta = p.position().toPoint() - p.pressPosition().toPoint();
            enum { TapRadius = 40 };
            if (delta.manhattanLength() <= TapRadius) {
                if (event->type() == QEvent::TouchEnd)
                    result = QGestureRecognizer::FinishGesture;
                else
                    result = QGestureRecognizer::TriggerGesture;
            }
        }
        break;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        result = QGestureRecognizer::Ignore;
        break;
    default:
        result = QGestureRecognizer::Ignore;
        break;
    }
    return result;
}

void QTapGestureRecognizer::reset(QGesture *state)
{
    QTapGesture *q = static_cast<QTapGesture *>(state);
    QTapGesturePrivate *d = q->d_func();

    d->position = QPointF();

    QGestureRecognizer::reset(state);
}

//
// QTapAndHoldGestureRecognizer
//

QTapAndHoldGestureRecognizer::QTapAndHoldGestureRecognizer()
{
}

QGesture *QTapAndHoldGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    return new QTapAndHoldGesture;
}

QGestureRecognizer::Result
QTapAndHoldGestureRecognizer::recognize(QGesture *state, QObject *object,
                                        QEvent *event)
{
    QTapAndHoldGesture *q = static_cast<QTapAndHoldGesture *>(state);
    QTapAndHoldGesturePrivate *d = q->d_func();

    if (object == state && event->type() == QEvent::Timer) {
        d->tapAndHoldTimer.stop();
        return QGestureRecognizer::FinishGesture | QGestureRecognizer::ConsumeEventHint;
    }

    enum { TapRadius = 40 };

    switch (event->type()) {
#if QT_CONFIG(graphicsview)
    case QEvent::GraphicsSceneMousePress: {
        const QGraphicsSceneMouseEvent *gsme = static_cast<const QGraphicsSceneMouseEvent *>(event);
        d->position = gsme->screenPos();
        q->setHotSpot(d->position);
        d->tapAndHoldTimer.start(QTapAndHoldGesturePrivate::Timeout * 1ms, q);
        return QGestureRecognizer::MayBeGesture; // we don't show a sign of life until the timeout
    }
#endif
    case QEvent::MouseButtonPress: {
        const QMouseEvent *me = static_cast<const QMouseEvent *>(event);
        d->position = me->globalPosition().toPoint();
        q->setHotSpot(d->position);
        d->tapAndHoldTimer.start(QTapAndHoldGesturePrivate::Timeout * 1ms, q);
        return QGestureRecognizer::MayBeGesture; // we don't show a sign of life until the timeout
    }
    case QEvent::TouchBegin: {
        const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);
        d->position = ev->points().at(0).globalPressPosition();
        q->setHotSpot(d->position);
        d->tapAndHoldTimer.start(QTapAndHoldGesturePrivate::Timeout * 1ms, q);
        return QGestureRecognizer::MayBeGesture; // we don't show a sign of life until the timeout
    }
#if QT_CONFIG(graphicsview)
    case QEvent::GraphicsSceneMouseRelease:
#endif
    case QEvent::MouseButtonRelease:
    case QEvent::TouchEnd:
        return QGestureRecognizer::CancelGesture; // get out of the MayBeGesture state
    case QEvent::TouchUpdate: {
        const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);
        if (d->tapAndHoldTimer.isActive() && ev->points().size() == 1) {
            const QEventPoint &p = ev->points().at(0);
            QPoint delta = p.position().toPoint() - p.pressPosition().toPoint();
            if (delta.manhattanLength() <= TapRadius)
                return QGestureRecognizer::MayBeGesture;
        }
        return QGestureRecognizer::CancelGesture;
    }
    case QEvent::MouseMove: {
        const QMouseEvent *me = static_cast<const QMouseEvent *>(event);
        QPoint delta = me->globalPosition().toPoint() - d->position.toPoint();
        if (d->tapAndHoldTimer.isActive() && delta.manhattanLength() <= TapRadius)
            return QGestureRecognizer::MayBeGesture;
        return QGestureRecognizer::CancelGesture;
    }
#if QT_CONFIG(graphicsview)
    case QEvent::GraphicsSceneMouseMove: {
        const QGraphicsSceneMouseEvent *gsme = static_cast<const QGraphicsSceneMouseEvent *>(event);
        QPoint delta = gsme->screenPos() - d->position.toPoint();
        if (d->tapAndHoldTimer.isActive() && delta.manhattanLength() <= TapRadius)
            return QGestureRecognizer::MayBeGesture;
        return QGestureRecognizer::CancelGesture;
    }
#endif
    default:
        return QGestureRecognizer::Ignore;
    }
}

void QTapAndHoldGestureRecognizer::reset(QGesture *state)
{
    QTapAndHoldGesture *q = static_cast<QTapAndHoldGesture *>(state);
    QTapAndHoldGesturePrivate *d = q->d_func();

    d->position = QPointF();
    d->tapAndHoldTimer.stop();

    QGestureRecognizer::reset(state);
}

QT_END_NAMESPACE

#endif // QT_NO_GESTURES
