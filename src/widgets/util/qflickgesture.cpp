// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgesture.h"
#include "qapplication.h"
#include "qevent.h"
#include "qwidget.h"
#if QT_CONFIG(graphicsview)
#include "qgraphicsitem.h"
#include "qgraphicsscene.h"
#include "qgraphicssceneevent.h"
#include "qgraphicsview.h"
#endif
#include "qscroller.h"
#include <QtGui/qpointingdevice.h>
#include "private/qapplication_p.h"
#include "private/qevent_p.h"
#include "private/qflickgesture_p.h"
#include "qdebug.h"

#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

//#define QFLICKGESTURE_DEBUG

#ifdef QFLICKGESTURE_DEBUG
#  define qFGDebug  qDebug
#else
#  define qFGDebug  while (false) qDebug
#endif

extern bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event);

static QMouseEvent *copyMouseEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove: {
        return static_cast<QMouseEvent *>(e->clone());
    }
#if QT_CONFIG(graphicsview)
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseRelease:
    case QEvent::GraphicsSceneMouseMove: {
        QGraphicsSceneMouseEvent *me = static_cast<QGraphicsSceneMouseEvent *>(e);
        QEvent::Type met = me->type() == QEvent::GraphicsSceneMousePress ? QEvent::MouseButtonPress :
                           (me->type() == QEvent::GraphicsSceneMouseRelease ? QEvent::MouseButtonRelease : QEvent::MouseMove);
        QMouseEvent *cme = new QMouseEvent(met, QPoint(0, 0), QPoint(0, 0), me->screenPos(),
                                           me->button(), me->buttons(), me->modifiers(), me->source());
        cme->setTimestamp(me->timestamp());
        return cme;
    }
#endif // QT_CONFIG(graphicsview)
    default:
        return nullptr;
    }
}

class PressDelayHandler : public QObject
{
private:
    PressDelayHandler(QObject *parent = nullptr)
        : QObject(parent)
        , pressDelayTimer(0)
        , sendingEvent(false)
        , mouseButton(Qt::NoButton)
        , mouseTarget(nullptr)
        , mouseEventSource(Qt::MouseEventNotSynthesized)
    { }

public:
    enum {
        UngrabMouseBefore = 1,
        RegrabMouseAfterwards = 2
    };

    static PressDelayHandler *instance()
    {
        static PressDelayHandler *inst = nullptr;
        if (!inst)
            inst = new PressDelayHandler(QCoreApplication::instance());
        return inst;
    }

    bool shouldEventBeIgnored(QEvent *) const
    {
        return sendingEvent;
    }

    bool isDelaying() const
    {
        return !pressDelayEvent.isNull();
    }

    void pressed(QEvent *e, int delay)
    {
        if (!pressDelayEvent) {
            pressDelayEvent.reset(copyMouseEvent(e));
            pressDelayTimer = startTimer(delay);
            mouseTarget = QApplication::widgetAt(pressDelayEvent->globalPosition().toPoint());
            mouseButton = pressDelayEvent->button();
            mouseEventSource = pressDelayEvent->source();
            qFGDebug("QFG: consuming/delaying mouse press");
        } else {
            qFGDebug("QFG: NOT consuming/delaying mouse press");
        }
        e->setAccepted(true);
    }

    bool released(QEvent *e, bool scrollerWasActive, bool scrollerIsActive)
    {
        // consume this event if the scroller was or is active
        bool result = scrollerWasActive || scrollerIsActive;

        // stop the timer
        if (pressDelayTimer) {
            killTimer(pressDelayTimer);
            pressDelayTimer = 0;
        }
        // we still haven't even sent the press, so do it now
        if (pressDelayEvent && mouseTarget && !scrollerIsActive) {
            QScopedPointer<QMouseEvent> releaseEvent(copyMouseEvent(e));

            qFGDebug() << "QFG: re-sending mouse press (due to release) for " << mouseTarget;
            sendMouseEvent(pressDelayEvent.data(), UngrabMouseBefore);

            qFGDebug() << "QFG: faking mouse release (due to release) for " << mouseTarget;
            sendMouseEvent(releaseEvent.data());

            result = true; // consume this event
        } else if (mouseTarget && scrollerIsActive) {
            // we grabbed the mouse explicitly when the scroller became active, so undo that now
            sendMouseEvent(nullptr, UngrabMouseBefore);
        }
        pressDelayEvent.reset(nullptr);
        mouseTarget = nullptr;
        return result;
    }

    void scrollerWasIntercepted()
    {
        qFGDebug("QFG: deleting delayed mouse press, since scroller was only intercepted");
        if (pressDelayEvent) {
            // we still haven't even sent the press, so just throw it away now
            if (pressDelayTimer) {
                killTimer(pressDelayTimer);
                pressDelayTimer = 0;
            }
            pressDelayEvent.reset(nullptr);
        }
        mouseTarget = nullptr;
    }

    void scrollerBecameActive(Qt::KeyboardModifiers eventModifiers, Qt::MouseButtons eventButtons)
    {
        if (pressDelayEvent) {
            // we still haven't even sent the press, so just throw it away now
            qFGDebug("QFG: deleting delayed mouse press, since scroller is active now");
            if (pressDelayTimer) {
                killTimer(pressDelayTimer);
                pressDelayTimer = 0;
            }
            pressDelayEvent.reset(nullptr);
            mouseTarget = nullptr;
        } else if (mouseTarget) {
            // we did send a press, so we need to fake a release now
            QPoint farFarAway(-QWIDGETSIZE_MAX, -QWIDGETSIZE_MAX);

            qFGDebug() << "QFG: sending a fake mouse release at far-far-away to " << mouseTarget;
            QMouseEvent re(QEvent::MouseButtonRelease, QPoint(), farFarAway, farFarAway,
                           mouseButton, eventButtons & ~mouseButton,
                           eventModifiers, mouseEventSource);
            sendMouseEvent(&re, RegrabMouseAfterwards);
            // don't clear the mouseTarget just yet, since we need to explicitly ungrab the mouse on release!
        }
    }

protected:
    void timerEvent(QTimerEvent *e) override
    {
        if (e->timerId() == pressDelayTimer) {
            if (pressDelayEvent && mouseTarget) {
                qFGDebug() << "QFG: timer event: re-sending mouse press to " << mouseTarget;
                sendMouseEvent(pressDelayEvent.data(), UngrabMouseBefore);
            }
            pressDelayEvent.reset(nullptr);

            if (pressDelayTimer) {
                killTimer(pressDelayTimer);
                pressDelayTimer = 0;
            }
        }
    }

    void sendMouseEvent(QMouseEvent *me, int flags = 0)
    {
        if (mouseTarget) {
            sendingEvent = true;

#if QT_CONFIG(graphicsview)
            QGraphicsItem *grabber = nullptr;
            if (mouseTarget->parentWidget()) {
                if (QGraphicsView *gv = qobject_cast<QGraphicsView *>(mouseTarget->parentWidget())) {
                    if (gv->scene())
                        grabber = gv->scene()->mouseGrabberItem();
                }
            }

            if (grabber && (flags & UngrabMouseBefore)) {
                // GraphicsView Mouse Handling Workaround #1:
                // we need to ungrab the mouse before re-sending the press,
                // since the scene had already set the mouse grabber to the
                // original (and consumed) event's receiver
                qFGDebug() << "QFG: ungrabbing" << grabber;
                grabber->ungrabMouse();
            }
#else
            Q_UNUSED(flags);
#endif // QT_CONFIG(graphicsview)

            if (me) {
                QMouseEvent copy(me->type(), mouseTarget->mapFromGlobal(me->globalPosition()),
                                 mouseTarget->topLevelWidget()->mapFromGlobal(me->globalPosition()), me->globalPosition(),
                                 me->button(), me->buttons(), me->modifiers(),
                                 me->source(), me->pointingDevice());
                copy.setTimestamp(me->timestamp());
                qt_sendSpontaneousEvent(mouseTarget, &copy);
            }

#if QT_CONFIG(graphicsview)
            if (grabber && (flags & RegrabMouseAfterwards)) {
                // GraphicsView Mouse Handling Workaround #2:
                // we need to re-grab the mouse after sending a faked mouse
                // release, since we still need the mouse moves for the gesture
                // (the scene will clear the item's mouse grabber status on
                // release).
                qFGDebug() << "QFG: re-grabbing" << grabber;
                grabber->grabMouse();
            }
#endif
            sendingEvent = false;
        }
    }


private:
    int pressDelayTimer;
    QScopedPointer<QMouseEvent> pressDelayEvent;
    bool sendingEvent;
    Qt::MouseButton mouseButton;
    QPointer<QWidget> mouseTarget;
    Qt::MouseEventSource mouseEventSource;
};


/*!
    \internal
    \class QFlickGesture
    \since 4.8
    \brief The QFlickGesture class describes a flicking gesture made by the user.
    \ingroup gestures
    The QFlickGesture is more complex than the QPanGesture that uses QScroller and QScrollerProperties
    to decide if it is triggered.
    This gesture is reacting on touch event as compared to the QMouseFlickGesture.

    \sa {Gestures in Widgets and Graphics View}, QScroller, QScrollerProperties, QMouseFlickGesture
*/

/*!
    \internal
*/
QFlickGesture::QFlickGesture(QObject *receiver, Qt::MouseButton button, QObject *parent)
    : QGesture(*new QFlickGesturePrivate, parent)
{
    d_func()->q_ptr = this;
    d_func()->receiver = receiver;
    d_func()->receiverScroller = (receiver && QScroller::hasScroller(receiver)) ? QScroller::scroller(receiver) : nullptr;
    d_func()->button = button;
}

QFlickGesture::~QFlickGesture()
{ }

QFlickGesturePrivate::QFlickGesturePrivate()
    : receiverScroller(nullptr), button(Qt::NoButton), macIgnoreWheel(false)
{ }


//
// QFlickGestureRecognizer
//


QFlickGestureRecognizer::QFlickGestureRecognizer(Qt::MouseButton button)
{
    this->button = button;
}

/*! \reimp
 */
QGesture *QFlickGestureRecognizer::create(QObject *target)
{
#if QT_CONFIG(graphicsview)
    QGraphicsObject *go = qobject_cast<QGraphicsObject*>(target);
    if (go && button == Qt::NoButton) {
        go->setAcceptTouchEvents(true);
    }
#endif
    return new QFlickGesture(target, button);
}

/*! \internal
    The recognize function detects a touch event suitable to start the attached QScroller.
    The QFlickGesture will be triggered as soon as the scroller is no longer in the state
    QScroller::Inactive or QScroller::Pressed. It will be finished or canceled
    at the next QEvent::TouchEnd.
    Note that the QScroller might continue scrolling (kinetically) at this point.
 */
QGestureRecognizer::Result QFlickGestureRecognizer::recognize(QGesture *state,
                                                              QObject *watched,
                                                              QEvent *event)
{
    Q_UNUSED(watched);

    Q_CONSTINIT static QElapsedTimer monotonicTimer;
    if (!monotonicTimer.isValid())
        monotonicTimer.start();

    QFlickGesture *q = static_cast<QFlickGesture *>(state);
    QFlickGesturePrivate *d = q->d_func();

    QScroller *scroller = d->receiverScroller;
    if (!scroller)
        return Ignore; // nothing to do without a scroller?

    QWidget *receiverWidget = qobject_cast<QWidget *>(d->receiver);
#if QT_CONFIG(graphicsview)
    QGraphicsObject *receiverGraphicsObject = qobject_cast<QGraphicsObject *>(d->receiver);
#endif

    // this is only set for events that we inject into the event loop via sendEvent()
    if (PressDelayHandler::instance()->shouldEventBeIgnored(event)) {
        //qFGDebug() << state << "QFG: ignored event: " << event->type();
        return Ignore;
    }

    const QMouseEvent *me = nullptr;
#if QT_CONFIG(graphicsview)
    const QGraphicsSceneMouseEvent *gsme = nullptr;
#endif
    const QTouchEvent *te = nullptr;
    QPoint globalPos;

    // qFGDebug() << "FlickGesture "<<state<<"watched:"<<watched<<"receiver"<<d->receiver<<"event"<<event->type()<<"button"<<button;

    Qt::KeyboardModifiers keyboardModifiers = Qt::NoModifier;
    Qt::MouseButtons mouseButtons = Qt::NoButton;
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
        if (!receiverWidget)
            return Ignore;
        if (button != Qt::NoButton) {
            me = static_cast<const QMouseEvent *>(event);
            keyboardModifiers = me->modifiers();
            mouseButtons = me->buttons();
            globalPos = me->globalPosition().toPoint();
        }
        break;
#if QT_CONFIG(graphicsview)
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseRelease:
    case QEvent::GraphicsSceneMouseMove:
        if (!receiverGraphicsObject)
            return Ignore;
        if (button != Qt::NoButton) {
            gsme = static_cast<const QGraphicsSceneMouseEvent *>(event);
            keyboardModifiers = gsme->modifiers();
            mouseButtons = gsme->buttons();
            globalPos = gsme->screenPos();
        }
        break;
#endif
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
        if (button == Qt::NoButton) {
            te = static_cast<const QTouchEvent *>(event);
            keyboardModifiers = te->modifiers();
            if (!te->points().isEmpty())
                globalPos = te->points().at(0).globalPosition().toPoint();
        }
        break;

    // consume all wheel events if the scroller is active
    case QEvent::Wheel:
        if (d->macIgnoreWheel || (scroller->state() != QScroller::Inactive))
            return Ignore | ConsumeEventHint;
        break;

    // consume all dbl click events if the scroller is active
    case QEvent::MouseButtonDblClick:
        if (scroller->state() != QScroller::Inactive)
            return Ignore | ConsumeEventHint;
        break;

    default:
        break;
    }

    if (!me
#if QT_CONFIG(graphicsview)
        && !gsme
#endif
        && !te) // Neither mouse nor touch
        return Ignore;

    // get the current pointer position in local coordinates.
    QPointF point;
    QScroller::Input inputType = (QScroller::Input) 0;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (me && me->button() == button && me->buttons() == button) {
            point = me->globalPosition().toPoint();
            inputType = QScroller::InputPress;
        } else if (me) {
            scroller->stop();
            return CancelGesture;
        }
        break;
    case QEvent::MouseButtonRelease:
        if (me && me->button() == button) {
            point = me->globalPosition().toPoint();
            inputType = QScroller::InputRelease;
        }
        break;
    case QEvent::MouseMove:
        if (me && me->buttons() == button) {
            point = me->globalPosition().toPoint();
            inputType = QScroller::InputMove;
        }
        break;

#if QT_CONFIG(graphicsview)
    case QEvent::GraphicsSceneMousePress:
        if (gsme && gsme->button() == button && gsme->buttons() == button) {
            point = gsme->scenePos();
            inputType = QScroller::InputPress;
        } else if (gsme) {
            scroller->stop();
            return CancelGesture;
        }
        break;
    case QEvent::GraphicsSceneMouseRelease:
        if (gsme && gsme->button() == button) {
            point = gsme->scenePos();
            inputType = QScroller::InputRelease;
        }
        break;
    case QEvent::GraphicsSceneMouseMove:
        if (gsme && gsme->buttons() == button) {
            point = gsme->scenePos();
            inputType = QScroller::InputMove;
        }
        break;
#endif

    case QEvent::TouchBegin:
        inputType = QScroller::InputPress;
        Q_FALLTHROUGH();
    case QEvent::TouchEnd:
        if (!inputType)
            inputType = QScroller::InputRelease;
        Q_FALLTHROUGH();
    case QEvent::TouchUpdate:
        if (!inputType)
            inputType = QScroller::InputMove;

        if (te->pointingDevice()->type() == QInputDevice::DeviceType::TouchPad) {
            if (te->points().size() != 2)  // 2 fingers on pad
                return Ignore;

            point = te->points().at(0).scenePressPosition() +
                    ((te->points().at(0).scenePosition() - te->points().at(0).scenePressPosition()) +
                     (te->points().at(1).scenePosition() - te->points().at(1).scenePressPosition())) / 2;
        } else { // TouchScreen
            if (te->points().size() != 1) // 1 finger on screen
                return Ignore;

            point = te->points().at(0).scenePosition();
        }
        break;

    default:
        break;
    }

    // Check for an active scroller at globalPos
    if (inputType == QScroller::InputPress) {
        const auto activeScrollers = QScroller::activeScrollers();
        for (QScroller *as : activeScrollers) {
            if (as != scroller) {
                QRegion scrollerRegion;

                if (QWidget *w = qobject_cast<QWidget *>(as->target())) {
                    scrollerRegion = QRect(w->mapToGlobal(QPoint(0, 0)), w->size());
#if QT_CONFIG(graphicsview)
                } else if (QGraphicsObject *go = qobject_cast<QGraphicsObject *>(as->target())) {
                    if (const auto *scene = go->scene()) {
                        const auto goBoundingRectMappedToScene = go->mapToScene(go->boundingRect());
                        const auto views = scene->views();
                        for (QGraphicsView *gv : views) {
                            scrollerRegion |= gv->mapFromScene(goBoundingRectMappedToScene)
                                              .translated(gv->mapToGlobal(QPoint(0, 0)));
                        }
                    }
#endif
                }
                // active scrollers always have priority
                if (scrollerRegion.contains(globalPos))
                    return Ignore;
            }
        }
    }

    bool scrollerWasDragging = (scroller->state() == QScroller::Dragging);
    bool scrollerWasScrolling = (scroller->state() == QScroller::Scrolling);

    if (inputType) {
        if (QWidget *w = qobject_cast<QWidget *>(d->receiver))
            point = w->mapFromGlobal(point.toPoint());
#if QT_CONFIG(graphicsview)
        else if (QGraphicsObject *go = qobject_cast<QGraphicsObject *>(d->receiver))
            point = go->mapFromScene(point);
#endif

        // inform the scroller about the new event
        scroller->handleInput(inputType, point, monotonicTimer.elapsed());
    }

    // depending on the scroller state return the gesture state
    Result result;
    bool scrollerIsActive = (scroller->state() == QScroller::Dragging ||
                             scroller->state() == QScroller::Scrolling);

    // Consume all mouse events while dragging or scrolling to avoid nasty
    // side effects with Qt's standard widgets.
    if ((me
#if QT_CONFIG(graphicsview)
         || gsme
#endif
         ) && scrollerIsActive)
        result |= ConsumeEventHint;

    // The only problem with this approach is that we consume the
    // MouseRelease when we start the scrolling with a flick gesture, so we
    // have to fake a MouseRelease "somewhere" to not mess with the internal
    // states of Qt's widgets (a QPushButton would stay in 'pressed' state
    // forever, if it doesn't receive a MouseRelease).
    if (me
#if QT_CONFIG(graphicsview)
        || gsme
#endif
        ) {
        if (!scrollerWasDragging && !scrollerWasScrolling && scrollerIsActive)
            PressDelayHandler::instance()->scrollerBecameActive(keyboardModifiers, mouseButtons);
        else if (scrollerWasScrolling && (scroller->state() == QScroller::Dragging || scroller->state() == QScroller::Inactive))
            PressDelayHandler::instance()->scrollerWasIntercepted();
    }

    if (!inputType) {
        result |= Ignore;
    } else {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
#if QT_CONFIG(graphicsview)
        case QEvent::GraphicsSceneMousePress:
#endif
            if (scroller->state() == QScroller::Pressed) {
                int pressDelay = int(1000 * scroller->scrollerProperties().scrollMetric(QScrollerProperties::MousePressEventDelay).toReal());
                if (pressDelay > 0) {
                    result |= ConsumeEventHint;

                    PressDelayHandler::instance()->pressed(event, pressDelay);
                    event->accept();
                }
            }
            Q_FALLTHROUGH();
        case QEvent::TouchBegin:
            q->setHotSpot(globalPos);
            result |= scrollerIsActive ? TriggerGesture : MayBeGesture;
            break;

        case QEvent::MouseMove:
#if QT_CONFIG(graphicsview)
        case QEvent::GraphicsSceneMouseMove:
#endif
            if (PressDelayHandler::instance()->isDelaying())
                result |= ConsumeEventHint;
            Q_FALLTHROUGH();
        case QEvent::TouchUpdate:
            result |= scrollerIsActive ? TriggerGesture : Ignore;
            break;

#if QT_CONFIG(graphicsview)
        case QEvent::GraphicsSceneMouseRelease:
#endif
        case QEvent::MouseButtonRelease:
            if (PressDelayHandler::instance()->released(event, scrollerWasDragging || scrollerWasScrolling, scrollerIsActive))
                result |= ConsumeEventHint;
            Q_FALLTHROUGH();
        case QEvent::TouchEnd:
            result |= scrollerIsActive ? FinishGesture : CancelGesture;
            break;

        default:
            result |= Ignore;
            break;
        }
    }
    return result;
}


/*! \reimp
 */
void QFlickGestureRecognizer::reset(QGesture *state)
{
    QGestureRecognizer::reset(state);
}

QT_END_NAMESPACE

#include "moc_qflickgesture_p.cpp"

#endif // QT_NO_GESTURES
