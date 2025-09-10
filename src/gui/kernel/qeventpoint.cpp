// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeventpoint.h"
#include "private/qeventpoint_p.h"
#include "private/qpointingdevice_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPointerVel, "qt.pointer.velocity")
Q_LOGGING_CATEGORY(lcEPDetach, "qt.pointer.eventpoint.detach")

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QEventPointPrivate)

/*! \class QEventPoint
    \brief The QEventPoint class provides information about a point in a QPointerEvent.
    \since 6.0
    \inmodule QtGui
*/

/*!
    \enum QEventPoint::State

    Specifies the state of this event point.

    \value  Unknown
            Unknown state.

    \value  Stationary
            The event point did not move.

    \value  Pressed
            The touch point or button is pressed.

    \value  Updated
            The event point was updated.

    \value  Released
            The touch point or button was released.
*/

/*!
    \internal
    Constructs an invalid event point with the given \a id and the \a device
    from which it originated.

    This acts as a default constructor in usages like QMap<int, QEventPoint>,
    as in qgraphicsscene_p.h.
*/
QEventPoint::QEventPoint(int id, const QPointingDevice *device)
    : d(new QEventPointPrivate(id, device)) {}

/*!
    Constructs an event point with the given \a pointId, \a state,
    \a scenePosition and \a globalPosition.
*/
QEventPoint::QEventPoint(int pointId, State state, const QPointF &scenePosition, const QPointF &globalPosition)
    : d(new QEventPointPrivate(pointId, state, scenePosition, globalPosition)) {}

/*!
    Constructs an event point by making a shallow copy of \a other.
*/
QEventPoint::QEventPoint(const QEventPoint &other) noexcept = default;

/*!
    Assigns \a other to this event point and returns a reference to this
    event point.
*/
QEventPoint &QEventPoint::operator=(const QEventPoint &other) noexcept = default;

/*!
    \fn QEventPoint::QEventPoint(QEventPoint &&other) noexcept

    Constructs an event point by moving \a other.
*/

/*!
    \fn QEventPoint &QEventPoint::operator=(QEventPoint &&other) noexcept

    Move-assigns \a other to this event point instance.
*/

/*!
    Returns \c true if this event point is equal to \a other, otherwise
    return \c false.
*/
bool QEventPoint::operator==(const QEventPoint &other) const noexcept
{
    if (d == other.d)
        return true;
    if (!d || !other.d)
        return false;
    return *d == *other.d;
}

/*!
    \fn bool QEventPoint::operator!=(const QEventPoint &other) const noexcept

    Returns \c true if this event point is not equal to \a other, otherwise
    return \c false.
*/

/*!
    Destroys the event point.
*/
QEventPoint::~QEventPoint() = default;

/*! \fn QPointF QEventPoint::pos() const
    \deprecated [6.0] Use position() instead.

    Returns the position of this point, relative to the widget
    or item that received the event.
*/

/*!
    \property QEventPoint::position
    \brief the position of this point.

    The position is relative to the widget or item that received the event.
*/
QPointF QEventPoint::position() const
{ return d ? d->pos : QPointF(); }

/*!
    \property QEventPoint::pressPosition
    \brief the position at which this point was pressed.

    The position is relative to the widget or item that received the event.

    \sa position
*/
QPointF QEventPoint::pressPosition() const
{ return d ? d->globalPressPos - d->globalPos + d->pos : QPointF(); }

/*!
    \property QEventPoint::grabPosition
    \brief the position at which this point was grabbed.

    The position is relative to the widget or item that received the event.

    \sa position
*/
QPointF QEventPoint::grabPosition() const
{ return d ? d->globalGrabPos - d->globalPos + d->pos : QPointF(); }

/*!
    \property QEventPoint::lastPosition
    \brief the position of this point from the previous press or move event.

    The position is relative to the widget or item that received the event.

    \sa position, pressPosition
*/
QPointF QEventPoint::lastPosition() const
{ return d ? d->globalLastPos - d->globalPos + d->pos : QPointF(); }

/*!
    \property QEventPoint::scenePosition
    \brief the scene position of this point.

    The scene position is the position relative to QQuickWindow if handled in QQuickItem::event(),
    in QGraphicsScene coordinates if handled by an override of QGraphicsItem::touchEvent(),
    or the window position in widget applications.

    \sa scenePressPosition, position, globalPosition
*/
QPointF QEventPoint::scenePosition() const
{ return d ? d->scenePos : QPointF(); }

/*!
    \property QEventPoint::scenePressPosition
    \brief the scene position at which this point was pressed.

    The scene position is the position relative to QQuickWindow if handled in QQuickItem::event(),
    in QGraphicsScene coordinates if handled by an override of QGraphicsItem::touchEvent(),
    or the window position in widget applications.

    \sa scenePosition, pressPosition, globalPressPosition
*/
QPointF QEventPoint::scenePressPosition() const
{ return d ? d->globalPressPos - d->globalPos + d->scenePos : QPointF(); }

/*!
    \property QEventPoint::sceneGrabPosition
    \brief the scene position at which this point was grabbed.

    The scene position is the position relative to QQuickWindow if handled in QQuickItem::event(),
    in QGraphicsScene coordinates if handled by an override of QGraphicsItem::touchEvent(),
    or the window position in widget applications.

    \sa scenePosition, grabPosition, globalGrabPosition
*/
QPointF QEventPoint::sceneGrabPosition() const
{ return d ? d->globalGrabPos - d->globalPos + d->scenePos : QPointF(); }

/*!
    \property QEventPoint::sceneLastPosition
    \brief the scene position of this point from the previous press or move event.

    The scene position is the position relative to QQuickWindow if handled in QQuickItem::event(),
    in QGraphicsScene coordinates if handled by an override of QGraphicsItem::touchEvent(),
    or the window position in widget applications.

    \sa scenePosition, scenePressPosition
*/
QPointF QEventPoint::sceneLastPosition() const
{ return d ? d->globalLastPos - d->globalPos + d->scenePos : QPointF(); }

/*!
    \property QEventPoint::globalPosition
    \brief the global position of this point.

    The global position is relative to the screen or virtual desktop.

    \sa globalPressPosition, position, scenePosition
*/
QPointF QEventPoint::globalPosition() const
{ return d ? d->globalPos : QPointF(); }

/*!
    \property QEventPoint::globalPressPosition
    \brief the global position at which this point was pressed.

    The global position is relative to the screen or virtual desktop.

    \sa globalPosition, pressPosition, scenePressPosition
*/
QPointF QEventPoint::globalPressPosition() const
{ return d ? d->globalPressPos : QPointF(); }

/*!
    \property QEventPoint::globalGrabPosition
    \brief the global position at which this point was grabbed.

    The global position is relative to the screen or virtual desktop.

    \sa globalPosition, grabPosition, sceneGrabPosition
*/
QPointF QEventPoint::globalGrabPosition() const
{ return d ? d->globalGrabPos : QPointF(); }

/*!
    \property QEventPoint::globalLastPosition
    \brief the global position of this point from the previous press or move event.

    The global position is relative to the screen or virtual desktop.

    \sa globalPosition, lastPosition, sceneLastPosition
*/
QPointF QEventPoint::globalLastPosition() const
{ return d ? d->globalLastPos : QPointF(); }

/*!
    \property QEventPoint::velocity
    \brief a velocity vector, in units of pixels per second, in the coordinate.
    system of the screen or desktop.

    \note If the device's capabilities include QInputDevice::Velocity, it means
    velocity comes from the operating system (perhaps the touch hardware or
    driver provides it). But usually the \c Velocity capability is not set,
    indicating that the velocity is calculated by Qt, using a simple Kalman
    filter to provide a smoothed average velocity rather than an instantaneous
    value. Effectively it tells how fast and in what direction the user has
    been dragging this point over the last few events, with the most recent
    event having the strongest influence.

    \sa QInputDevice::capabilities(), QInputEvent::device()
*/
QVector2D QEventPoint::velocity() const
{ return d ? d->velocity : QVector2D(); }

/*!
    \property QEventPoint::state
    \brief the current state of the event point.
*/
QEventPoint::State QEventPoint::state() const
{ return d ? d->state : QEventPoint::State::Unknown; }

/*!
    \property QEventPoint::device
    \brief the pointing device from which this event point originates.
*/
const QPointingDevice *QEventPoint::device() const
{ return d ? d->device : nullptr; }

/*!
    \property QEventPoint::id
    \brief the ID number of this event point.

    \note Do not assume that ID numbers start at zero or that they are
          sequential. Such an assumption is often false due to the way
          the underlying drivers work.
*/
int QEventPoint::id() const
{ return d ? d->pointId : -1; }

/*!
    \property QEventPoint::uniqueId
    \brief the unique ID of this point or token, if any.

    It is often invalid (see \l {QPointingDeviceUniqueId::isValid()} {isValid()}),
    because touchscreens cannot uniquely identify fingers.

    When it comes from a QTabletEvent, it identifies the serial number of the
    stylus in use.

    It may identify a specific token (fiducial object) when the TUIO driver is
    in use with a touchscreen that supports them.
*/
QPointingDeviceUniqueId QEventPoint::uniqueId() const
{ return d ? d->uniqueId : QPointingDeviceUniqueId(); }

/*!
    \property QEventPoint::timestamp
    \brief the most recent time at which this point was included in a QPointerEvent.

    \sa QPointerEvent::timestamp()
*/
ulong QEventPoint::timestamp() const
{ return d ? d->timestamp : 0; }

/*!
    \property QEventPoint::lastTimestamp
    \brief the time from the previous QPointerEvent that contained this point.

    \sa globalLastPosition
*/
ulong QEventPoint::lastTimestamp() const
{ return d ? d->lastTimestamp : 0; }

/*!
    \property QEventPoint::pressTimestamp
    \brief the most recent time at which this point was pressed.

    \sa timestamp
*/
ulong QEventPoint::pressTimestamp() const
{ return d ? d->pressTimestamp : 0; }

/*!
    \property QEventPoint::timeHeld
    \brief the duration, in seconds, since this point was pressed and not released.

    \sa pressTimestamp, timestamp
*/
qreal QEventPoint::timeHeld() const
{ return d ? (d->timestamp - d->pressTimestamp) / qreal(1000) : 0.0; }

/*!
    \property QEventPoint::pressure
    \brief the pressure of this point.

    The return value is in the range \c 0.0 to \c 1.0.
*/
qreal QEventPoint::pressure() const
{ return d ? d->pressure : 0.0; }

/*!
    \property QEventPoint::rotation
    \brief the angular orientation of this point.

    The return value is in degrees, where zero (the default) indicates the finger,
    token or stylus is pointing upwards, a negative angle means it's rotated to the
    left, and a positive angle means it's rotated to the right.
    Most touchscreens do not detect rotation, so zero is the most common value.
*/
qreal QEventPoint::rotation() const
{ return d ? d->rotation : 0.0; }

/*!
    \property QEventPoint::ellipseDiameters
    \brief the width and height of the bounding ellipse of the touch point.

    The return value is in logical pixels. Most touchscreens do not detect the
    shape of the contact point, and no mice or tablet devices can detect it,
    so a null size is the most common value. On some touchscreens the diameters
    may be nonzero and always equal (the ellipse is approximated as a circle).
*/
QSizeF QEventPoint::ellipseDiameters() const
{ return d ? d->ellipseDiameters : QSizeF(); }

/*!
    \property QEventPoint::accepted
    \brief the accepted state of the event point.

    In widget-based applications, this property is not used, as it's only meaningful
    for a widget to accept or reject a complete QInputEvent.

    In Qt Quick however, it's normal for an Item or Event Handler to accept
    only the individual points in a QTouchEvent that are actually participating
    in a gesture, while other points can be delivered to other items or
    handlers. For the sake of consistency, that applies to any QPointerEvent;
    and delivery is done only when all points in a QPointerEvent have been
    accepted.

    \sa QEvent::accepted
*/
void QEventPoint::setAccepted(bool accepted)
{
    if (d)
        d->accept = accepted;
}

bool QEventPoint::isAccepted() const
{ return d ? d->accept : false; }


/*!
    \fn QPointF QEventPoint::normalizedPos() const
    \deprecated [6.0] Use normalizedPosition() instead.
*/

/*!
    Returns the normalized position of this point.

    The coordinates are calculated by transforming globalPosition() into the
    space of QInputDevice::availableVirtualGeometry(), i.e. \c (0, 0) is the
    top-left corner and \c (1, 1) is the bottom-right corner.

    \sa globalPosition
*/
QPointF QEventPoint::normalizedPosition() const
{
    if (!d)
        return {};

    auto geom = d->device->availableVirtualGeometry();
    if (geom.isNull())
        return QPointF();
    return (globalPosition() - geom.topLeft()) / geom.width();
}

#if QT_DEPRECATED_SINCE(6, 0)
/*!
    \deprecated [6.0] Use globalPressPosition() instead.

    Returns the normalized press position of this point.
*/
QPointF QEventPoint::startNormalizedPos() const
{
    if (!d)
        return {};

    auto geom = d->device->availableVirtualGeometry();
    if (geom.isNull())
        return QPointF();
    return (globalPressPosition() - geom.topLeft()) / geom.width();
}

/*!
    \deprecated [6.0] Use globalLastPosition() instead.

    Returns the normalized position of this point from the previous press or
    move event.

    The coordinates are normalized to QInputDevice::availableVirtualGeometry(),
    i.e. \c (0, 0) is the top-left corner and \c (1, 1) is the bottom-right corner.

    \sa normalizedPosition(), globalPressPosition()
*/
QPointF QEventPoint::lastNormalizedPos() const
{
    if (!d)
        return {};

    auto geom = d->device->availableVirtualGeometry();
    if (geom.isNull())
        return QPointF();
    return (globalLastPosition() - geom.topLeft()) / geom.width();
}
#endif // QT_DEPRECATED_SINCE(6, 0)

/*! \internal
    This class is explicitly shared, which means if you construct an event and
    then the point(s) that it holds are modified before the event is delivered,
    the event will be seen to hold the modified points. The workaround is that
    any code which modifies an eventpoint that could already be included in an
    event, or code that wants to save an eventpoint for later, has
    responsibility to detach before calling any setters, so as to hold and
    modify an independent copy. (The independent copy can then be used in a
    subsequent event.)
*/
void QMutableEventPoint::detach(QEventPoint &p)
{
    if (p.d)
        p.d.detach();
    else
        p.d.reset(new QEventPointPrivate(-1, nullptr));
}

/*! \internal
    Update \a target state from the \a other point, assuming that \a target
    contains state from the previous event and \a other contains new
    values that came in from a device.

    That is: global position and other valuators will be updated, but
    the following properties will not be updated:

    \list
    \li properties that are not likely to be set after a fresh touchpoint
    has been received from a device
    \li properties that should be persistent between events (such as grabbers)
    \endlist
*/
void QMutableEventPoint::update(const QEventPoint &other, QEventPoint &target)
{
    detach(target);
    setPressure(target, other.pressure());

    switch (other.state()) {
    case QEventPoint::State::Pressed:
        setGlobalPressPosition(target, other.globalPosition());
        setGlobalLastPosition(target, other.globalPosition());
        if (target.pressure() < 0)
            setPressure(target, 1);
        break;

    case QEventPoint::State::Released:
        if (target.globalPosition() != other.globalPosition())
            setGlobalLastPosition(target, target.globalPosition());
        setPressure(target, 0);
        break;

    default: // update or stationary
        if (target.globalPosition() != other.globalPosition())
            setGlobalLastPosition(target, target.globalPosition());
        if (target.pressure() < 0)
            setPressure(target, 1);
        break;
    }

    setState(target, other.state());
    setPosition(target, other.position());
    setScenePosition(target, other.scenePosition());
    setGlobalPosition(target, other.globalPosition());
    setEllipseDiameters(target, other.ellipseDiameters());
    setRotation(target, other.rotation());
    setVelocity(target, other.velocity());
    setUniqueId(target, other.uniqueId()); // for TUIO
}

/*! \internal
    Set the timestamp from the event that updated this point's positions,
    and calculate a new value for velocity().

    The velocity calculation is done here because none of the QPointerEvent
    subclass constructors take the timestamp directly, and because
    QGuiApplication traditionally constructs an event first and then sets its
    timestamp (see for example QGuiApplicationPrivate::processMouseEvent()).

    This function looks up the corresponding instance in QPointingDevicePrivate::activePoints,
    and assumes that its timestamp() still holds the previous time when this point
    was updated, its velocity() holds this point's last-known velocity, and
    its globalPosition() and globalLastPosition() hold this point's current
    and previous positions, respectively.  We assume timestamps are in milliseconds.

    The velocity calculation is skipped if the platform has promised to
    provide velocities already by setting the QInputDevice::Velocity capability.
*/
void QMutableEventPoint::setTimestamp(QEventPoint &p, ulong t)
{
    // On mouse press, if the mouse has moved from its last-known location,
    // QGuiApplicationPrivate::processMouseEvent() sends first a mouse move and
    // then a press. Both events will get the same timestamp. So we need to set
    // the press timestamp and position even when the timestamp isn't advancing,
    // but skip setting lastTimestamp and velocity because those need a time delta.
    if (p.d) {
        if (p.state() == QEventPoint::State::Pressed) {
            p.d->pressTimestamp = t;
            p.d->globalPressPos = p.d->globalPos;
        }
        if (p.d->timestamp == t)
            return;
    }
    detach(p);
    if (p.device()) {
        // get the persistent instance out of QPointingDevicePrivate::activePoints
        // (which sometimes might be the same as this instance)
        QEventPointPrivate *pd = QPointingDevicePrivate::get(
                    const_cast<QPointingDevice *>(p.d->device))->pointById(p.id())->eventPoint.d.get();
        if (t > pd->timestamp) {
            pd->lastTimestamp = pd->timestamp;
            pd->timestamp = t;
            if (p.state() == QEventPoint::State::Pressed)
                pd->pressTimestamp = t;
            if (pd->lastTimestamp > 0 && !p.device()->capabilities().testFlag(QInputDevice::Capability::Velocity)) {
                // calculate instantaneous velocity according to time and distance moved since the previous point
                QVector2D newVelocity = QVector2D(pd->globalPos - pd->globalLastPos) / (t - pd->lastTimestamp) * 1000;
                // VERY simple kalman filter: does a weighted average
                // where the older velocities get less and less significant
                static const float KalmanGain = 0.7f;
                pd->velocity = newVelocity * KalmanGain + pd->velocity * (1.0f - KalmanGain);
                qCDebug(lcPointerVel) << "velocity" << newVelocity << "filtered" << pd->velocity <<
                                         "based on movement" << pd->globalLastPos << "->" << pd->globalPos <<
                                         "over time" << pd->lastTimestamp << "->" << pd->timestamp;
            }
            if (p.d != pd) {
                p.d->lastTimestamp = pd->lastTimestamp;
                p.d->velocity = pd->velocity;
            }
        }
    }
    p.d->timestamp = t;
}

/*!
    \fn void QMutableEventPoint::setPosition(QPointF pos)
    \internal

    Sets the localized position.
    Often events need to be localized before delivery to specific widgets or
    items. This can be done directly, or in a copy (for which we have a copy
    constructor), depending on whether the original point needs to be retained.
    Usually it's calculated by mapping scenePosition() to the target anyway.
*/

QT_END_NAMESPACE

#include "moc_qeventpoint.cpp"
