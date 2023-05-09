// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qevent.h"
#include "qcursor.h"
#include "private/qguiapplication_p.h"
#include "private/qinputdevice_p.h"
#include "private/qpointingdevice_p.h"
#include "qpa/qplatformintegration.h"
#include "private/qevent_p.h"
#include "private/qeventpoint_p.h"
#include "qfile.h"
#include "qhashfunctions.h"
#include "qmetaobject.h"
#include "qmimedata.h"
#include "qevent_p.h"
#include "qmath.h"
#include "qloggingcategory.h"

#if QT_CONFIG(draganddrop)
#include <qpa/qplatformdrag.h>
#include <private/qdnd_p.h>
#endif

#if QT_CONFIG(shortcut)
#include <private/qshortcut_p.h>
#endif

#include <private/qdebug_p.h>

#define Q_IMPL_POINTER_EVENT(Class) \
    Class::Class(const Class &) = default; \
    Class::~Class() = default; \
    Class* Class::clone() const \
    { \
        auto c = new Class(*this); \
        for (auto &point : c->m_points) \
            QMutableEventPoint::detach(point); \
        QEvent *e = c; \
        /* check that covariant return is safe to add */ \
        Q_ASSERT(reinterpret_cast<quintptr>(c) == reinterpret_cast<quintptr>(e)); \
        return c; \
    }



QT_BEGIN_NAMESPACE

static_assert(sizeof(QMutableTouchEvent) == sizeof(QTouchEvent));
static_assert(sizeof(QMutableSinglePointEvent) == sizeof(QSinglePointEvent));
static_assert(sizeof(QMouseEvent) == sizeof(QSinglePointEvent));
static_assert(sizeof(QVector2D) == sizeof(quint64));

/*!
    \class QEnterEvent
    \ingroup events
    \inmodule QtGui

    \brief The QEnterEvent class contains parameters that describe an enter event.

    Enter events occur when the mouse cursor enters a window or a widget.

    \since 5.0
*/

/*!
    Constructs an enter event object originating from \a device.

    The points \a localPos, \a scenePos and \a globalPos specify the
    mouse cursor's position relative to the receiving widget or item,
    window, and screen or desktop, respectively.
*/
QEnterEvent::QEnterEvent(const QPointF &localPos, const QPointF &scenePos, const QPointF &globalPos, const QPointingDevice *device)
    : QSinglePointEvent(QEvent::Enter, device, localPos, scenePos, globalPos, Qt::NoButton, Qt::NoButton, Qt::NoModifier)
{
}

Q_IMPL_POINTER_EVENT(QEnterEvent)

/*!
   \fn QPoint QEnterEvent::globalPos() const
   \deprecated [6.0] Use globalPosition() instead.

   Returns the global position of the mouse cursor \e{at the time of the event}.
*/
/*!
   \fn int QEnterEvent::globalX() const
   \deprecated [6.0] Use globalPosition().x() instead.

   Returns the global position on the X-axis of the mouse cursor \e{at the time of the event}.
*/
/*!
   \fn int QEnterEvent::globalY() const
   \deprecated [6.0] Use globalPosition().y() instead.

   Returns the global position on the Y-axis of the mouse cursor \e{at the time of the event}.
*/
/*!
   \fn QPointF QEnterEvent::localPos() const
   \deprecated [6.0] Use position() instead.

   Returns the mouse cursor's position relative to the receiving widget.
*/
/*!
   \fn QPoint QEnterEvent::pos() const
   \deprecated [6.0] Use position().toPoint() instead.

   Returns the position of the mouse cursor relative to the receiving widget.
*/
/*!
   \fn QPointF QEnterEvent::screenPos() const
   \deprecated [6.0] Use globalPosition() instead.

   Returns the position of the mouse cursor relative to the receiving screen.
*/
/*!
   \fn QPointF QEnterEvent::windowPos() const
   \deprecated [6.0] Use scenePosition() instead.

   Returns the position of the mouse cursor relative to the receiving window.
*/
/*!
   \fn int QEnterEvent::x() const
   \deprecated [6.0] Use position().x() instead.

   Returns the x position of the mouse cursor relative to the receiving widget.
*/
/*!
   \fn int QEnterEvent::y() const
   \deprecated [6.0] Use position().y() instead.

   Returns the y position of the mouse cursor relative to the receiving widget.
*/

/*!
    \class QInputEvent
    \ingroup events
    \inmodule QtGui

    \brief The QInputEvent class is the base class for events that
    describe user input.
*/

/*!
  \internal
*/
QInputEvent::QInputEvent(Type type, const QInputDevice *dev, Qt::KeyboardModifiers modifiers)
    : QEvent(type, QEvent::InputEventTag{}), m_dev(dev), m_modState(modifiers), m_reserved(0)
{}

/*!
  \internal
*/
QInputEvent::QInputEvent(QEvent::Type type, QEvent::PointerEventTag, const QInputDevice *dev, Qt::KeyboardModifiers modifiers)
    : QEvent(type, QEvent::PointerEventTag{}), m_dev(dev), m_modState(modifiers), m_reserved(0)
{}

/*!
  \internal
*/
QInputEvent::QInputEvent(QEvent::Type type, QEvent::SinglePointEventTag, const QInputDevice *dev, Qt::KeyboardModifiers modifiers)
    : QEvent(type, QEvent::SinglePointEventTag{}), m_dev(dev), m_modState(modifiers), m_reserved(0)
{}

Q_IMPL_EVENT_COMMON(QInputEvent)

/*!
    \fn QInputDevice *QInputEvent::device() const
    \since 6.0

    Returns the source device that generated the original event.

    In case of a synthesized event, for example a mouse event that was
    generated from a touch event, \c device() continues to return the touchscreen
    device, so that you can tell that it did not come from an actual mouse.
    Thus \c {mouseEvent.source()->type() != QInputDevice::DeviceType::Mouse}
    is one possible replacement for the Qt 5 expression
    \c {mouseEvent.source() == Qt::MouseEventSynthesizedByQt}.

    \sa QPointerEvent::pointingDevice()
*/

/*!
    \fn QInputDevice::DeviceType QInputEvent::deviceType() const

    Returns the type of device that generated the event.
*/

/*!
    \fn Qt::KeyboardModifiers QInputEvent::modifiers() const

    Returns the keyboard modifier flags that existed immediately
    before the event occurred.

    \sa QGuiApplication::keyboardModifiers()
*/

/*! \fn void QInputEvent::setModifiers(Qt::KeyboardModifiers modifiers)

    \internal

    Sets the keyboard modifiers flags for this event.
*/

/*!
    \fn quint64 QInputEvent::timestamp() const

    Returns the window system's timestamp for this event.
    It will normally be in milliseconds since some arbitrary point
    in time, such as the time when the system was started.
*/

/*! \fn void QInputEvent::setTimestamp(quint64 atimestamp)

    \internal

    Sets the timestamp for this event.
*/

/*!
    \class QPointerEvent
    \since 6.0
    \inmodule QtGui

    \brief A base class for pointer events.
*/

/*!
    \fn qsizetype QPointerEvent::pointCount() const

    Returns the number of points in this pointer event.
*/

/*!
    Returns a QEventPoint reference for the point at index \a i.
*/
QEventPoint &QPointerEvent::point(qsizetype i)
{
    return m_points[i];
}

/*!
    \fn const QList<QEventPoint> &QPointerEvent::points() const

    Returns a list of points in this pointer event.
*/

/*!
    \fn QPointingDevice::PointerType QPointerEvent::pointerType() const

    Returns the type of point that generated the event.
*/

/*!
    \internal
*/
QPointerEvent::QPointerEvent(QEvent::Type type, const QPointingDevice *dev,
                             Qt::KeyboardModifiers modifiers, const QList<QEventPoint> &points)
    : QInputEvent(type, QEvent::PointerEventTag{}, dev, modifiers), m_points(points)
{
}

/*!
  \internal
*/
QPointerEvent::QPointerEvent(QEvent::Type type, QEvent::SinglePointEventTag, const QInputDevice *dev, Qt::KeyboardModifiers modifiers)
    : QInputEvent(type, QEvent::SinglePointEventTag{}, dev, modifiers)
{
}

Q_IMPL_POINTER_EVENT(QPointerEvent);

/*!
    Returns the point whose \l {QEventPoint::id()}{id} matches the given \a id,
    or \c nullptr if no such point is found.
*/
QEventPoint *QPointerEvent::pointById(int id)
{
    for (auto &p : m_points) {
        if (p.id() == id)
            return &p;
    }
    return nullptr;
}

/*!
    Returns \c true if every point in points() has either an exclusiveGrabber()
    or one or more passiveGrabbers().
*/
bool QPointerEvent::allPointsGrabbed() const
{
    for (const auto &p : points()) {
        if (!exclusiveGrabber(p) && passiveGrabbers(p).isEmpty())
            return false;
    }
    return true;
}

/*!
    Returns \c true if isPointAccepted() is \c true for every point in
    points(); otherwise \c false.
*/
bool QPointerEvent::allPointsAccepted() const
{
    for (const auto &p : points()) {
        if (!p.isAccepted())
            return false;
    }
    return true;
}

/*!
    \reimp
*/
void QPointerEvent::setAccepted(bool accepted)
{
    QEvent::setAccepted(accepted);
    for (auto &p : m_points)
        p.setAccepted(accepted);
}

/*!
    Returns the source device from which this event originates.

    This is the same as QInputEvent::device() but typecast for convenience.
*/
const QPointingDevice *QPointerEvent::pointingDevice() const
{
    return static_cast<const QPointingDevice *>(m_dev);
}

/*! \internal
    Sets the timestamp for this event and its points().
*/
void QPointerEvent::setTimestamp(quint64 timestamp)
{
    QInputEvent::setTimestamp(timestamp);
    for (auto &p : m_points)
        QMutableEventPoint::setTimestamp(p, timestamp);
}

/*!
    Returns the object which has been set to receive all future update events
    and the release event containing the given \a point.

    It's mainly for use in Qt Quick at this time.
*/
QObject *QPointerEvent::exclusiveGrabber(const QEventPoint &point) const
{
    Q_ASSERT(pointingDevice());
    auto persistentPoint = QPointingDevicePrivate::get(pointingDevice())->queryPointById(point.id());
    if (Q_UNLIKELY(!persistentPoint)) {
        qWarning() << "point is not in activePoints" << point;
        return nullptr;
    }
    return persistentPoint->exclusiveGrabber;
}

/*!
    Informs the delivery logic that the given \a exclusiveGrabber is to
    receive all future update events and the release event containing
    the given \a point, and that delivery to other items can be skipped.

    It's mainly for use in Qt Quick at this time.
*/
void QPointerEvent::setExclusiveGrabber(const QEventPoint &point, QObject *exclusiveGrabber)
{
    Q_ASSERT(pointingDevice());
    auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(pointingDevice()));
    devPriv->setExclusiveGrabber(this, point, exclusiveGrabber);
}

/*!
    Returns the list of objects that have been requested to receive all
    future update events and the release event containing the given \a point.

    It's only for use by \l {Qt Quick Input Handlers}.

    \sa QPointerEvent::addPassiveGrabber()
*/
QList<QPointer<QObject> > QPointerEvent::passiveGrabbers(const QEventPoint &point) const
{
    Q_ASSERT(pointingDevice());
    auto persistentPoint = QPointingDevicePrivate::get(pointingDevice())->queryPointById(point.id());
    if (Q_UNLIKELY(!persistentPoint)) {
        qWarning() << "point is not in activePoints" << point;
        return {};
    }
    return persistentPoint->passiveGrabbers;
}

/*!
    Informs the delivery logic that the given \a grabber is to receive all
    future update events and the release event containing the given \a point,
    regardless where else those events may be delivered.

    It's only for use by \l {Qt Quick Input Handlers}.

    Returns \c false if \a grabber was already added, \c true otherwise.
*/
bool QPointerEvent::addPassiveGrabber(const QEventPoint &point, QObject *grabber)
{
    Q_ASSERT(pointingDevice());
    auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(pointingDevice()));
    return devPriv->addPassiveGrabber(this, point, grabber);
}

/*!
    Removes the passive \a grabber from the given \a point if it was previously added.
    Returns \c true if it had been a passive grabber before, \c false if not.

    It's only for use by \l {Qt Quick Input Handlers}.

    \sa QPointerEvent::addPassiveGrabber()
*/
bool QPointerEvent::removePassiveGrabber(const QEventPoint &point, QObject *grabber)
{
    Q_ASSERT(pointingDevice());
    auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(pointingDevice()));
    return devPriv->removePassiveGrabber(this, point, grabber);
}

/*!
    Removes all passive grabbers from the given \a point.

    It's only for use by \l {Qt Quick Input Handlers}.

    \sa QPointerEvent::addPassiveGrabber()
*/
void QPointerEvent::clearPassiveGrabbers(const QEventPoint &point)
{
    Q_ASSERT(pointingDevice());
    auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(pointingDevice()));
    devPriv->clearPassiveGrabbers(this, point);
}

/*!
    \class QSinglePointEvent
    \since 6.0
    \inmodule QtGui

    \brief A base class for pointer events containing a single point, such as
           mouse events.
*/

/*! \fn Qt::MouseButton QSinglePointEvent::button() const

    Returns the button that caused the event.

    The returned value is always Qt::NoButton for mouse move events, as
    well as \l TabletMove, \l TabletEnterProximity, and
    \l TabletLeaveProximity events.

    \sa buttons()
*/

/*! \fn Qt::MouseButtons QSinglePointEvent::buttons() const

    Returns the button state when the event was generated.

    The button state is a combination of Qt::LeftButton, Qt::RightButton,
    and Qt::MiddleButton using the OR operator.

    For mouse move or \l TabletMove events, this is all buttons that are
    pressed down.

    For mouse press, double click, or \l TabletPress events, this includes
    the button that caused the event.

    For mouse release or \l TabletRelease events, this excludes the button
    that caused the event.

    \sa button()
*/

/*! \fn QPointF QSinglePointEvent::position() const

    Returns the position of the point in this event, relative to the widget or
    item that received the event.

    If you move your widgets around in response to mouse events, use
    globalPosition() instead.

    \sa globalPosition()
*/

/*! \fn QPointF QSinglePointEvent::scenePosition() const

    Returns the position of the point in this event, relative to the window or
    scene.

    \sa QEventPoint::scenePosition
*/

/*! \fn QPointF QSinglePointEvent::globalPosition() const

    Returns the position of the point in this event on the screen or virtual
    desktop.

    \note The global position of a mouse pointer is recorded \e{at the time
    of the event}. This is important on asynchronous window systems
    such as X11; whenever you move your widgets around in response to
    mouse events, globalPosition() can differ a lot from the current
    cursor position returned by QCursor::pos().

    \sa position()
*/

/*!
    \internal
*/
QSinglePointEvent::QSinglePointEvent(QEvent::Type type, const QPointingDevice *dev,
                                     const QPointF &localPos, const QPointF &scenePos, const QPointF &globalPos,
                                     Qt::MouseButton button, Qt::MouseButtons buttons,
                                     Qt::KeyboardModifiers modifiers, Qt::MouseEventSource source)
    : QPointerEvent(type, QEvent::SinglePointEventTag{}, dev, modifiers),
      m_button(button),
      m_mouseState(buttons),
      m_source(source),
      m_reserved(0), m_reserved2(0),
      m_doubleClick(false), m_phase(0), m_invertedScrolling(0)
{
    bool isPress = (button != Qt::NoButton && (button | buttons) == buttons);
    bool isWheel = (type == QEvent::Type::Wheel);
    auto devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice *>(pointingDevice()));
    auto epd = devPriv->pointById(0);
    QEventPoint &p = epd->eventPoint;
    Q_ASSERT(p.device() == dev);
    // p is a reference to a non-detached instance that lives in QPointingDevicePrivate::activePoints.
    // Update persistent info in that instance.
    if (isPress || isWheel)
        QMutableEventPoint::setGlobalLastPosition(p, globalPos);
    else
        QMutableEventPoint::setGlobalLastPosition(p, p.globalPosition());
    QMutableEventPoint::setGlobalPosition(p, globalPos);
    if (isWheel && p.state() != QEventPoint::State::Updated)
        QMutableEventPoint::setGlobalPressPosition(p, globalPos);
    if (type == MouseButtonDblClick)
        QMutableEventPoint::setState(p, QEventPoint::State::Stationary);
    else if (button == Qt::NoButton || isWheel)
        QMutableEventPoint::setState(p, QEventPoint::State::Updated);
    else if (isPress)
        QMutableEventPoint::setState(p, QEventPoint::State::Pressed);
    else
        QMutableEventPoint::setState(p, QEventPoint::State::Released);
    QMutableEventPoint::setScenePosition(p, scenePos);
    // Now detach, and update the detached instance with ephemeral state.
    QMutableEventPoint::detach(p);
    QMutableEventPoint::setPosition(p, localPos);
    m_points.append(p);
}

/*! \internal
    Constructs a single-point event with the given \a point, which must be an instance
    (or copy of one) that already exists in QPointingDevicePrivate::activePoints.
    Unlike the other constructor, it does not modify the given \a point in any way.
    This is useful when synthesizing a QMouseEvent from one point taken from a QTouchEvent, for example.

    \sa QMutableSinglePointEvent()
*/
QSinglePointEvent::QSinglePointEvent(QEvent::Type type, const QPointingDevice *dev, const QEventPoint &point,
                                     Qt::MouseButton button, Qt::MouseButtons buttons,
                                     Qt::KeyboardModifiers modifiers, Qt::MouseEventSource source)
    : QPointerEvent(type, QEvent::SinglePointEventTag{}, dev, modifiers),
      m_button(button),
      m_mouseState(buttons),
      m_source(source),
      m_reserved(0), m_reserved2(0),
      m_doubleClick(false), m_phase(0), m_invertedScrolling(0)
{
    m_points << point;
}

Q_IMPL_POINTER_EVENT(QSinglePointEvent)

/*!
    Returns \c true if this event represents a \l {button()}{button} being pressed.
*/
bool QSinglePointEvent::isBeginEvent() const
{
    // A double-click event does not begin a sequence: it comes after a press event,
    // and while it tells which button caused the double-click, it doesn't represent
    // a change of button state. So it's an update event.
    return m_button != Qt::NoButton && m_mouseState.testFlag(m_button)
            && type() != QEvent::MouseButtonDblClick;
}

/*!
    Returns \c true if this event does not include a change in \l {buttons()}{button state}.
*/
bool QSinglePointEvent::isUpdateEvent() const
{
    // A double-click event is an update event even though it tells which button
    // caused the double-click, because a MouseButtonPress event was sent right before it.
    return m_button == Qt::NoButton || type() == QEvent::MouseButtonDblClick;
}

/*!
    Returns \c true if this event represents a \l {button()}{button} being released.
*/
bool QSinglePointEvent::isEndEvent() const
{
    return m_button != Qt::NoButton && !m_mouseState.testFlag(m_button);
}

/*!
    \property QSinglePointEvent::exclusivePointGrabber
    \brief the object that will receive future updates

    The exclusive grabber is an object that has chosen to receive all future
    update events and the release event containing the same point that this
    event carries.

    Setting the exclusivePointGrabber property is a convenience equivalent to:
    \code
    setExclusiveGrabber(points().first(), exclusiveGrabber);
    \endcode
*/

/*!
    \class QMouseEvent
    \ingroup events
    \inmodule QtGui

    \brief The QMouseEvent class contains parameters that describe a mouse event.

    Mouse events occur when a mouse button is pressed or released
    inside a widget, or when the mouse cursor is moved.

    Mouse move events will occur only when a mouse button is pressed
    down, unless mouse tracking has been enabled with
    QWidget::setMouseTracking().

    Qt automatically grabs the mouse when a mouse button is pressed
    inside a widget; the widget will continue to receive mouse events
    until the last mouse button is released.

    A mouse event contains a special accept flag that indicates
    whether the receiver wants the event. You should call ignore() if
    the mouse event is not handled by your widget. A mouse event is
    propagated up the parent widget chain until a widget accepts it
    with accept(), or an event filter consumes it.

    \note If a mouse event is propagated to a \l{QWidget}{widget} for
    which Qt::WA_NoMousePropagation has been set, that mouse event
    will not be propagated further up the parent widget chain.

    The state of the keyboard modifier keys can be found by calling the
    \l{QInputEvent::modifiers()}{modifiers()} function, inherited from
    QInputEvent.

    The position() function gives the cursor position
    relative to the widget or item that receives the mouse event.
    If you move the widget as a result of the mouse event, use the
    global position returned by globalPosition() to avoid a shaking motion.

    The QWidget::setEnabled() function can be used to enable or
    disable mouse and keyboard events for a widget.

    Reimplement the QWidget event handlers, QWidget::mousePressEvent(),
    QWidget::mouseReleaseEvent(), QWidget::mouseDoubleClickEvent(),
    and QWidget::mouseMoveEvent() to receive mouse events in your own
    widgets.

    \sa QWidget::setMouseTracking(), QWidget::grabMouse(),
    QCursor::pos()
*/

#if QT_DEPRECATED_SINCE(6, 4)
/*!
    \deprecated [6.4] Use another constructor instead (global position is required).

    Constructs a mouse event object originating from \a device.

    The \a type parameter must be one of QEvent::MouseButtonPress,
    QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick,
    or QEvent::MouseMove.

    The \a localPos is the mouse cursor's position relative to the
    receiving widget or item. The window position is set to the same value
    as \a localPos.
    The \a button that caused the event is given as a value from
    the Qt::MouseButton enum. If the event \a type is
    \l MouseMove, the appropriate button for this event is Qt::NoButton.
    The mouse and keyboard states at the time of the event are specified by
    \a buttons and \a modifiers.

    The globalPosition() is initialized to QCursor::pos(), which may not
    be appropriate. Use the other constructor to specify the global
    position explicitly.
*/
QMouseEvent::QMouseEvent(Type type, const QPointF &localPos, Qt::MouseButton button,
                         Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, const QPointingDevice *device)
    : QSinglePointEvent(type, device, localPos, localPos,
#ifdef QT_NO_CURSOR
                        localPos,
#else
                        QCursor::pos(),
#endif
                        button, buttons, modifiers)
{
}
#endif

/*!
    Constructs a mouse event object originating from \a device.

    The \a type parameter must be QEvent::MouseButtonPress,
    QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick,
    or QEvent::MouseMove.

    The \a localPos is the mouse cursor's position relative to the
    receiving widget or item. The cursor's position in screen coordinates is
    specified by \a globalPos. The window position is set to the same value
    as \a localPos. The \a button that caused the event is
    given as a value from the \l Qt::MouseButton enum. If the event \a
    type is \l MouseMove, the appropriate button for this event is
    Qt::NoButton. \a buttons is the state of all buttons at the
    time of the event, \a modifiers the state of all keyboard
    modifiers.

*/
QMouseEvent::QMouseEvent(Type type, const QPointF &localPos, const QPointF &globalPos,
                         Qt::MouseButton button, Qt::MouseButtons buttons,
                         Qt::KeyboardModifiers modifiers, const QPointingDevice *device)
    : QMouseEvent(type, localPos, localPos, globalPos, button, buttons, modifiers, device)
{
}

/*!
    Constructs a mouse event object.

    The \a type parameter must be QEvent::MouseButtonPress,
    QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick,
    or QEvent::MouseMove.

    The points \a localPos, \a scenePos and \a globalPos specify the
    mouse cursor's position relative to the receiving widget or item,
    window, and screen or desktop, respectively.

    The \a button that caused the event is given as a value from the
    \l Qt::MouseButton enum. If the event \a type is \l MouseMove,
    the appropriate button for this event is Qt::NoButton. \a buttons
    is the state of all buttons at the time of the event, \a modifiers
    is the state of all keyboard modifiers.
*/
QMouseEvent::QMouseEvent(QEvent::Type type, const QPointF &localPos,
                         const QPointF &scenePos, const QPointF &globalPos,
                         Qt::MouseButton button, Qt::MouseButtons buttons,
                         Qt::KeyboardModifiers modifiers, const QPointingDevice *device)
    : QSinglePointEvent(type, device, localPos, scenePos, globalPos, button, buttons, modifiers)
{
}

QMouseEvent::QMouseEvent(QEvent::Type type, const QPointF &localPos, const QPointF &windowPos,
                         const QPointF &globalPos, Qt::MouseButton button, Qt::MouseButtons buttons,
                         Qt::KeyboardModifiers modifiers, Qt::MouseEventSource source,
                         const QPointingDevice *device)
    : QSinglePointEvent(type, device, localPos, windowPos, globalPos, button, buttons, modifiers, source)
{
}

Q_IMPL_POINTER_EVENT(QMouseEvent)

/*!
    \fn Qt::MouseEventSource QMouseEvent::source() const
    \since 5.3
    \deprecated [6.0] Use pointingDevice() instead.

    Returns information about the mouse event source.

    The mouse event source can be used to distinguish between genuine
    and artificial mouse events. The latter are events that are
    synthesized from touch events by the operating system or Qt itself.
    This enum tells you from where it was synthesized; but often
    it's more useful to know from which device it was synthesized,
    so try to use pointingDevice() instead.

    \note Many platforms provide no such information. On such platforms
    \l Qt::MouseEventNotSynthesized is returned always.

    \sa Qt::MouseEventSource
    \sa QGraphicsSceneMouseEvent::source()

    \note In Qt 5-based code, source() was often used to attempt to distinguish
    mouse events from an actual mouse vs. those that were synthesized because
    some legacy QQuickItem or QWidget subclass did not react to a QTouchEvent.
    However, you could not tell whether it was synthesized from a QTouchEvent
    or a QTabletEvent, and other information was lost. pointingDevice()
    tells you the specific device that it came from, so you might check
    \c {pointingDevice()->type()} or \c {pointingDevice()->capabilities()} to
    decide how to react to this event. But it's even better to react to the
    original event rather than handling only mouse events.
*/
// Note: the docs mention 6.0 as a deprecation version. That is correct and
// intended, because we want our users to stop using it! Internally we will
// deprecate it when we port our code away from using it.
Qt::MouseEventSource QMouseEvent::source() const
{
    return Qt::MouseEventSource(m_source);
}

/*!
    \since 5.3

    Returns the mouse event flags.

    The mouse event flags provide additional information about a mouse event.

    \sa Qt::MouseEventFlag
    \sa QGraphicsSceneMouseEvent::flags()
*/
Qt::MouseEventFlags QMouseEvent::flags() const
{
    return (m_doubleClick ? Qt::MouseEventCreatedDoubleClick : Qt::NoMouseEventFlag);
}

/*!
    \fn QPointF QMouseEvent::localPos() const
    \deprecated [6.0] Use position() instead.

    \since 5.0

    Returns the position of the mouse cursor as a QPointF, relative to the
    widget or item that received the event.

    If you move the widget as a result of the mouse event, use the
    screen position returned by screenPos() to avoid a shaking
    motion.

    \sa x(), y(), windowPos(), screenPos()
*/

/*!
    \fn void QMouseEvent::setLocalPos(const QPointF &localPosition)

    \since 5.8

    \internal

    Sets the local position in the mouse event to \a localPosition. This allows to re-use one event
    when sending it to a series of receivers that expect the local pos in their
    respective local coordinates.
*/

/*!
    \fn QPointF QMouseEvent::windowPos() const
    \deprecated [6.0] Use scenePosition() instead.

    \since 5.0

    Returns the position of the mouse cursor as a QPointF, relative to the
    window that received the event.

    If you move the widget as a result of the mouse event, use the
    global position returned by globalPos() to avoid a shaking
    motion.

    \sa x(), y(), pos(), localPos(), screenPos()
*/

/*!
    \fn QPointF QMouseEvent::screenPos() const
    \deprecated [6.0] Use globalPosition() instead.

    \since 5.0

    Returns the position of the mouse cursor as a QPointF, relative to the
    screen that received the event.

    \sa x(), y(), pos(), localPos(), windowPos()
*/

/*!
    \fn QPoint QMouseEvent::pos() const
    \deprecated [6.0] Use position() instead.

    Returns the position of the mouse cursor, relative to the widget
    that received the event.

    If you move the widget as a result of the mouse event, use the
    global position returned by globalPos() to avoid a shaking
    motion.

    \sa x(), y(), globalPos()
*/

/*!
    \fn QPoint QMouseEvent::globalPos() const
    \deprecated [6.0] Use globalPosition().toPoint() instead.

    Returns the global position of the mouse cursor \e{at the time
    of the event}. This is important on asynchronous window systems
    like X11. Whenever you move your widgets around in response to
    mouse events, globalPos() may differ a lot from the current
    pointer position QCursor::pos(), and from
    QWidget::mapToGlobal(pos()).

    \sa globalX(), globalY()
*/

/*!
    \fn int QMouseEvent::x() const
    \deprecated [6.0] Use position().x() instead.

    Returns the x position of the mouse cursor, relative to the
    widget that received the event.

    \sa y(), pos()
*/

/*!
    \fn int QMouseEvent::y() const
    \deprecated [6.0] Use position().y() instead.

    Returns the y position of the mouse cursor, relative to the
    widget that received the event.

    \sa x(), pos()
*/

/*!
    \fn int QMouseEvent::globalX() const
    \deprecated [6.0] Use globalPosition().x() instead.

    Returns the global x position of the mouse cursor at the time of
    the event.

    \sa globalY(), globalPos()
*/

/*!
    \fn int QMouseEvent::globalY() const
    \deprecated [6.0] Use globalPosition().y() instead.

    Returns the global y position of the mouse cursor at the time of
    the event.

    \sa globalX(), globalPos()
*/

/*!
    \class QHoverEvent
    \ingroup events
    \inmodule QtGui

    \brief The QHoverEvent class contains parameters that describe a mouse event.

    Mouse events occur when a mouse cursor is moved into, out of, or within a
    widget, and if the widget has the Qt::WA_Hover attribute.

    The function pos() gives the current cursor position, while oldPos() gives
    the old mouse position.

    There are a few similarities between the events QEvent::HoverEnter
    and QEvent::HoverLeave, and the events QEvent::Enter and QEvent::Leave.
    However, they are slightly different because we do an update() in the event
    handler of HoverEnter and HoverLeave.

    QEvent::HoverMove is also slightly different from QEvent::MouseMove. Let us
    consider a top-level window A containing a child B which in turn contains a
    child C (all with mouse tracking enabled):

    \image hoverevents.png

    Now, if you move the cursor from the top to the bottom in the middle of A,
    you will get the following QEvent::MouseMove events:

    \list 1
        \li A::MouseMove
        \li B::MouseMove
        \li C::MouseMove
    \endlist

    You will get the same events for QEvent::HoverMove, except that the event
    always propagates to the top-level regardless whether the event is accepted
    or not. It will only stop propagating with the Qt::WA_NoMousePropagation
    attribute.

    In this case the events will occur in the following way:

    \list 1
        \li A::HoverMove
        \li A::HoverMove, B::HoverMove
        \li A::HoverMove, B::HoverMove, C::HoverMove
    \endlist

*/

/*!
    \fn QPoint QHoverEvent::pos() const
    \deprecated [6.0] Use position().toPoint() instead.

    Returns the position of the mouse cursor, relative to the widget
    that received the event.

    On QEvent::HoverLeave events, this position will always be
    QPoint(-1, -1).

    \sa oldPos()
*/

/*!
    \fn QPoint QHoverEvent::oldPos() const

    Returns the previous position of the mouse cursor, relative to the widget
    that received the event. If there is no previous position, oldPos() will
    return the same position as pos().

    On QEvent::HoverEnter events, this position will always be
    QPoint(-1, -1).

    \sa pos()
*/

/*!
    \fn const QPointF &QHoverEvent::posF() const
    \deprecated [6.0] Use position() instead.

    Returns the position of the mouse cursor, relative to the widget
    that received the event.

    On QEvent::HoverLeave events, this position will always be
    QPointF(-1, -1).

    \sa oldPosF()
*/

/*!
    \fn const QPointF &QHoverEvent::oldPosF() const

    Returns the previous position of the mouse cursor, relative to the widget
    that received the event. If there is no previous position, oldPosF() will
    return the same position as posF().

    On QEvent::HoverEnter events, this position will always be
    QPointF(-1, -1).

    \sa posF()
*/

/*!
    Constructs a hover event object originating from \a device.

    The \a type parameter must be QEvent::HoverEnter,
    QEvent::HoverLeave, or QEvent::HoverMove.

    The \a scenePos is the current mouse cursor's position relative to the
    receiving window or scene, \a oldPos is its previous such position, and
    \a globalPos is the mouse position in absolute coordinates.
    \a modifiers hold the state of all keyboard modifiers at the time
    of the event.
*/
QHoverEvent::QHoverEvent(Type type, const QPointF &scenePos, const QPointF &globalPos, const QPointF &oldPos,
                         Qt::KeyboardModifiers modifiers, const QPointingDevice *device)
    : QSinglePointEvent(type, device, scenePos, scenePos, globalPos, Qt::NoButton, Qt::NoButton, modifiers), m_oldPos(oldPos)
{
}

#if QT_DEPRECATED_SINCE(6, 3)
/*!
    \deprecated [6.3] Use the other constructor instead (global position is required).

    Constructs a hover event object originating from \a device.

    The \a type parameter must be QEvent::HoverEnter,
    QEvent::HoverLeave, or QEvent::HoverMove.

    The \a pos is the current mouse cursor's position relative to the
    receiving widget, while \a oldPos is its previous such position.
    \a modifiers hold the state of all keyboard modifiers at the time
    of the event.
*/
QHoverEvent::QHoverEvent(Type type, const QPointF &pos, const QPointF &oldPos,
                         Qt::KeyboardModifiers modifiers, const QPointingDevice *device)
    : QSinglePointEvent(type, device, pos, pos, pos, Qt::NoButton, Qt::NoButton, modifiers), m_oldPos(oldPos)
{
}
#endif

Q_IMPL_POINTER_EVENT(QHoverEvent)

#if QT_CONFIG(wheelevent)
/*!
    \class QWheelEvent
    \brief The QWheelEvent class contains parameters that describe a wheel event.
    \inmodule QtGui

    \ingroup events

    Wheel events are sent to the widget under the mouse cursor, but
    if that widget does not handle the event they are sent to the
    focus widget. Wheel events are generated for both mouse wheels
    and trackpad scroll gestures. There are two ways to read the
    wheel event delta: angleDelta() returns the deltas in wheel
    degrees. These values are always provided. pixelDelta() returns
    the deltas in screen pixels, and is available on platforms that
    have high-resolution trackpads, such as \macos. If that is the
    case, device()->type() will return QInputDevice::DeviceType::Touchpad.

    The functions position() and globalPosition() return the mouse cursor's
    location at the time of the event.

    A wheel event contains a special accept flag that indicates
    whether the receiver wants the event. You should call ignore() if
    you do not handle the wheel event; this ensures that it will be
    sent to the parent widget.

    The QWidget::setEnabled() function can be used to enable or
    disable mouse and keyboard events for a widget.

    The event handler QWidget::wheelEvent() receives wheel events.

    \sa QMouseEvent, QWidget::grabMouse()
*/

/*!
  \enum QWheelEvent::anonymous
  \internal

  \value DefaultDeltasPerStep Defaqult deltas per step
*/

/*!
    \fn Qt::MouseEventSource QWheelEvent::source() const
    \since 5.5
    \deprecated [6.0] Use pointingDevice() instead.

    Returns information about the wheel event source.

    The source can be used to distinguish between events that come from a mouse
    with a physical wheel and events that are generated by some other means,
    such as a flick gesture on a touchpad.
    This enum tells you from where it was synthesized; but often
    it's more useful to know from which device it was synthesized,
    so try to use pointingDevice() instead.

    \note Many platforms provide no such information. On such platforms
    \l Qt::MouseEventNotSynthesized is returned always.

    \sa Qt::MouseEventSource
*/

/*!
    \fn bool QWheelEvent::inverted() const
    \since 5.7

    Returns whether the delta values delivered with the event are inverted.

    Normally, a vertical wheel will produce a QWheelEvent with positive delta
    values if the top of the wheel is rotating away from the hand operating it.
    Similarly, a horizontal wheel movement will produce a QWheelEvent with
    positive delta values if the top of the wheel is moved to the left.

    However, on some platforms this is configurable, so that the same
    operations described above will produce negative delta values (but with the
    same magnitude). With the inverted property a wheel event consumer can
    choose to always follow the direction of the wheel, regardless of the
    system settings, but only for specific widgets. (One such use case could be
    that the user is rotating the wheel in the same direction as a visual
    Tumbler rotates. Another usecase is to make a slider handle follow the
    direction of movement of fingers on a touchpad regardless of system
    configuration.)

    \note Many platforms provide no such information. On such platforms
    \l inverted always returns false.
*/

/*!
    Constructs a wheel event object.

    \since 5.12
    The \a pos provides the location of the mouse cursor
    within the window. The position in global coordinates is specified
    by \a globalPos.

    \a pixelDelta contains the scrolling distance in pixels on screen, while
    \a angleDelta contains the wheel rotation angle. \a pixelDelta is
    optional and can be null.

    The mouse and keyboard states at the time of the event are specified by
    \a buttons and \a modifiers.

    The scrolling phase of the event is specified by \a phase, and the
    \a source indicates whether this is a genuine or artificial (synthesized)
    event.

    If the system is configured to invert the delta values delivered with the
    event (such as natural scrolling of the touchpad on macOS), \a inverted
    should be \c true. Otherwise, \a inverted is \c false

    The device from which the wheel event originated is specified by \a device.

    \sa position(), globalPosition(), angleDelta(), pixelDelta(), phase(), inverted(), device()
*/
QWheelEvent::QWheelEvent(const QPointF &pos, const QPointF &globalPos, QPoint pixelDelta, QPoint angleDelta,
                         Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, Qt::ScrollPhase phase,
                         bool inverted, Qt::MouseEventSource source, const QPointingDevice *device)
    : QSinglePointEvent(Wheel, device, pos, pos, globalPos, Qt::NoButton, buttons, modifiers, source),
      m_pixelDelta(pixelDelta), m_angleDelta(angleDelta)
{
    m_phase = phase;
    m_invertedScrolling = inverted;
}

Q_IMPL_POINTER_EVENT(QWheelEvent)

/*!
    Returns \c true if this event's phase() is Qt::ScrollBegin.
*/
bool QWheelEvent::isBeginEvent() const
{
    return m_phase == Qt::ScrollBegin;
}

/*!
    Returns \c true if this event's phase() is Qt::ScrollUpdate or Qt::ScrollMomentum.
*/
bool QWheelEvent::isUpdateEvent() const
{
    return m_phase == Qt::ScrollUpdate || m_phase == Qt::ScrollMomentum;
}

/*!
    Returns \c true if this event's phase() is Qt::ScrollEnd.
*/
bool QWheelEvent::isEndEvent() const
{
    return m_phase == Qt::ScrollEnd;
}

#endif // QT_CONFIG(wheelevent)

/*!
    \fn QPoint QWheelEvent::pixelDelta() const

    Returns the scrolling distance in pixels on screen. This value is
    provided on platforms that support high-resolution pixel-based
    delta values, such as \macos. The value should be used directly
    to scroll content on screen.

    Example:

    \snippet code/src_gui_kernel_qevent.cpp 0

    \note On platforms that support scrolling \l{phase()}{phases}, the delta may be null when:
    \list
    \li scrolling is about to begin, but the distance did not yet change (Qt::ScrollBegin),
    \li or scrolling has ended and the distance did not change anymore (Qt::ScrollEnd).
    \endlist
    \note On X11 this value is driver-specific and unreliable, use angleDelta() instead.
*/

/*!
    \fn QPoint QWheelEvent::angleDelta() const

    Returns the relative amount that the wheel was rotated, in eighths of a
    degree. A positive value indicates that the wheel was rotated forwards away
    from the user; a negative value indicates that the wheel was rotated
    backwards toward the user. \c angleDelta().y() provides the angle through
    which the common vertical mouse wheel was rotated since the previous event.
    \c angleDelta().x() provides the angle through which the horizontal mouse
    wheel was rotated, if the mouse has a horizontal wheel; otherwise it stays
    at zero. Some mice allow the user to tilt the wheel to perform horizontal
    scrolling, and some touchpads support a horizontal scrolling gesture; that
    will also appear in \c angleDelta().x().

    Most mouse types work in steps of 15 degrees, in which case the
    delta value is a multiple of 120; i.e., 120 units * 1/8 = 15 degrees.

    However, some mice have finer-resolution wheels and send delta values
    that are less than 120 units (less than 15 degrees). To support this
    possibility, you can either cumulatively add the delta values from events
    until the value of 120 is reached, then scroll the widget, or you can
    partially scroll the widget in response to each wheel event.  But to
    provide a more native feel, you should prefer \l pixelDelta() on platforms
    where it's available.

    Example:

    \snippet code/src_gui_kernel_qevent.cpp 0

    \note On platforms that support scrolling \l{phase()}{phases}, the delta may be null when:
    \list
    \li scrolling is about to begin, but the distance did not yet change (Qt::ScrollBegin),
    \li or scrolling has ended and the distance did not change anymore (Qt::ScrollEnd).
    \endlist

    \sa pixelDelta()
*/

/*!
    \fn Qt::ScrollPhase QWheelEvent::phase() const
    \since 5.2

    Returns the scrolling phase of this wheel event.

    \note The Qt::ScrollBegin and Qt::ScrollEnd phases are currently
    supported only on \macos.
*/


/*!
    \class QKeyEvent
    \brief The QKeyEvent class describes a key event.

    \ingroup events
    \inmodule QtGui

    Key events are sent to the widget with keyboard input focus
    when keys are pressed or released.

    A key event contains a special accept flag that indicates whether
    the receiver will handle the key event. This flag is set by default
    for QEvent::KeyPress and QEvent::KeyRelease, so there is no need to
    call accept() when acting on a key event. For QEvent::ShortcutOverride
    the receiver needs to explicitly accept the event to trigger the override.
    Calling ignore() on a key event will propagate it to the parent widget.
    The event is propagated up the parent widget chain until a widget
    accepts it or an event filter consumes it.

    The QWidget::setEnabled() function can be used to enable or disable
    mouse and keyboard events for a widget.

    The event handlers QWidget::keyPressEvent(), QWidget::keyReleaseEvent(),
    QGraphicsItem::keyPressEvent() and QGraphicsItem::keyReleaseEvent()
    receive key events.

    \sa QFocusEvent, QWidget::grabKeyboard()
*/

/*!
    Constructs a key event object.

    The \a type parameter must be QEvent::KeyPress, QEvent::KeyRelease,
    or QEvent::ShortcutOverride.

    Int \a key is the code for the Qt::Key that the event loop should listen
    for. If \a key is 0, the event is not a result of a known key; for
    example, it may be the result of a compose sequence or keyboard macro.
    The \a modifiers holds the keyboard modifiers, and the given \a text
    is the Unicode text that the key generated. If \a autorep is true,
    isAutoRepeat() will be true. \a count is the number of keys involved
    in the event.
*/
QKeyEvent::QKeyEvent(Type type, int key, Qt::KeyboardModifiers modifiers, const QString& text,
                     bool autorep, quint16 count)
    : QInputEvent(type, QInputDevice::primaryKeyboard(), modifiers), m_text(text), m_key(key),
      m_scanCode(0), m_virtualKey(0), m_nativeModifiers(0),
      m_count(count), m_autoRepeat(autorep)
{
     if (type == QEvent::ShortcutOverride)
        ignore();
}

/*!
    Constructs a key event object.

    The \a type parameter must be QEvent::KeyPress, QEvent::KeyRelease,
    or QEvent::ShortcutOverride.

    Int \a key is the code for the Qt::Key that the event loop should listen
    for. If \a key is 0, the event is not a result of a known key; for
    example, it may be the result of a compose sequence or keyboard macro.
    The \a modifiers holds the keyboard modifiers, and the given \a text
    is the Unicode text that the key generated. If \a autorep is true,
    isAutoRepeat() will be true. \a count is the number of keys involved
    in the event.

    In addition to the normal key event data, also contains \a nativeScanCode,
    \a nativeVirtualKey and \a nativeModifiers. This extra data is used by the
    shortcut system, to determine which shortcuts to trigger.
*/
QKeyEvent::QKeyEvent(Type type, int key, Qt::KeyboardModifiers modifiers,
                     quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers,
                     const QString &text, bool autorep, quint16 count, const QInputDevice *device)
    : QInputEvent(type, device, modifiers), m_text(text), m_key(key),
      m_scanCode(nativeScanCode), m_virtualKey(nativeVirtualKey), m_nativeModifiers(nativeModifiers),
      m_count(count), m_autoRepeat(autorep)
{
    if (type == QEvent::ShortcutOverride)
        ignore();
}


Q_IMPL_EVENT_COMMON(QKeyEvent)

/*!
    \fn quint32 QKeyEvent::nativeScanCode() const
    \since 4.2

    Returns the native scan code of the key event.  If the key event
    does not contain this data 0 is returned.

    \note The native scan code may be 0, even if the key event contains
    extended information.
*/

/*!
    \fn quint32 QKeyEvent::nativeVirtualKey() const
    \since 4.2

    Returns the native virtual key, or key sym of the key event.
    If the key event does not contain this data 0 is returned.

    \note The native virtual key may be 0, even if the key event contains extended information.
*/

/*!
    \fn quint32 QKeyEvent::nativeModifiers() const
    \since 4.2

    Returns the native modifiers of a key event.
    If the key event does not contain this data 0 is returned.

    \note The native modifiers may be 0, even if the key event contains extended information.
*/

/*!
    \fn int QKeyEvent::key() const

    Returns the code of the key that was pressed or released.

    See \l Qt::Key for the list of keyboard codes. These codes are
    independent of the underlying window system. Note that this
    function does not distinguish between capital and non-capital
    letters, use the text() function (returning the Unicode text the
    key generated) for this purpose.

    A value of either 0 or Qt::Key_unknown means that the event is not
    the result of a known key; for example, it may be the result of
    a compose sequence, a keyboard macro, or due to key event
    compression.

    \sa Qt::WA_KeyCompression
*/

/*!
    \fn QString QKeyEvent::text() const

    Returns the Unicode text that this key generated.

    The text is not limited to the printable range of Unicode
    code points, and may include control characters or characters
    from other Unicode categories, including QChar::Other_PrivateUse.

    The text may also be empty, for example when modifier keys such as
    Shift, Control, Alt, and Meta are pressed (depending on the platform).
    The key() function will always return a valid value.

    \sa Qt::WA_KeyCompression
*/

/*!
    Returns the keyboard modifier flags that existed immediately
    after the event occurred.

    \warning This function cannot always be trusted. The user can
    confuse it by pressing both \uicontrol{Shift} keys simultaneously and
    releasing one of them, for example.

    \sa QGuiApplication::keyboardModifiers()
*/

Qt::KeyboardModifiers QKeyEvent::modifiers() const
{
    if (key() == Qt::Key_Shift)
        return Qt::KeyboardModifiers(QInputEvent::modifiers()^Qt::ShiftModifier);
    if (key() == Qt::Key_Control)
        return Qt::KeyboardModifiers(QInputEvent::modifiers()^Qt::ControlModifier);
    if (key() == Qt::Key_Alt)
        return Qt::KeyboardModifiers(QInputEvent::modifiers()^Qt::AltModifier);
    if (key() == Qt::Key_Meta)
        return Qt::KeyboardModifiers(QInputEvent::modifiers()^Qt::MetaModifier);
    if (key() == Qt::Key_AltGr)
        return Qt::KeyboardModifiers(QInputEvent::modifiers()^Qt::GroupSwitchModifier);
    return QInputEvent::modifiers();
}

/*!
    \fn QKeyCombination QKeyEvent::keyCombination() const

    Returns a QKeyCombination object containing both the key() and
    the modifiers() carried by this event.

    \since 6.0
*/

#if QT_CONFIG(shortcut)
/*!
    \fn bool QKeyEvent::matches(QKeySequence::StandardKey key) const
    \since 4.2

    Returns \c true if the key event matches the given standard \a key;
    otherwise returns \c false.
*/
bool QKeyEvent::matches(QKeySequence::StandardKey matchKey) const
{
    //The keypad and group switch modifier should not make a difference
    uint searchkey = (modifiers() | key()) & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier);

    const QList<QKeySequence> bindings = QKeySequence::keyBindings(matchKey);
    return bindings.contains(QKeySequence(searchkey));
}
#endif // QT_CONFIG(shortcut)


/*!
    \fn bool QKeyEvent::isAutoRepeat() const

    Returns \c true if this event comes from an auto-repeating key;
    returns \c false if it comes from an initial key press.

    Note that if the event is a multiple-key compressed event that is
    partly due to auto-repeat, this function could return either true
    or false indeterminately.
*/

/*!
    \fn int QKeyEvent::count() const

    Returns the number of keys involved in this event. If text()
    is not empty, this is simply the length of the string.

    \sa Qt::WA_KeyCompression
*/

/*!
    \class QFocusEvent
    \brief The QFocusEvent class contains event parameters for widget focus
    events.
    \inmodule QtGui

    \ingroup events

    Focus events are sent to widgets when the keyboard input focus
    changes. Focus events occur due to mouse actions, key presses
    (such as \uicontrol{Tab} or \uicontrol{Backtab}), the window system, popup
    menus, keyboard shortcuts, or other application-specific reasons.
    The reason for a particular focus event is returned by reason()
    in the appropriate event handler.

    The event handlers QWidget::focusInEvent(),
    QWidget::focusOutEvent(), QGraphicsItem::focusInEvent and
    QGraphicsItem::focusOutEvent() receive focus events.

    \sa QWidget::setFocus(), QWidget::setFocusPolicy(), {Keyboard Focus in Widgets}
*/

/*!
    Constructs a focus event object.

    The \a type parameter must be either QEvent::FocusIn or
    QEvent::FocusOut. The \a reason describes the cause of the change
    in focus.
*/
QFocusEvent::QFocusEvent(Type type, Qt::FocusReason reason)
    : QEvent(type), m_reason(reason)
{}

Q_IMPL_EVENT_COMMON(QFocusEvent)

/*!
    Returns the reason for this focus event.
 */
Qt::FocusReason QFocusEvent::reason() const
{
    return m_reason;
}

/*!
    \fn bool QFocusEvent::gotFocus() const

    Returns \c true if type() is QEvent::FocusIn; otherwise returns
    false.
*/

/*!
    \fn bool QFocusEvent::lostFocus() const

    Returns \c true if type() is QEvent::FocusOut; otherwise returns
    false.
*/


/*!
    \class QPaintEvent
    \brief The QPaintEvent class contains event parameters for paint events.
    \inmodule QtGui

    \ingroup events

    Paint events are sent to widgets that need to update themselves,
    for instance when part of a widget is exposed because a covering
    widget was moved.

    The event contains a region() that needs to be updated, and a
    rect() that is the bounding rectangle of that region. Both are
    provided because many widgets cannot make much use of region(),
    and rect() can be much faster than region().boundingRect().

    \section1 Automatic Clipping

    Painting is clipped to region() during the processing of a paint
    event. This clipping is performed by Qt's paint system and is
    independent of any clipping that may be applied to a QPainter used to
    draw on the paint device.

    As a result, the value returned by QPainter::clipRegion() on
    a newly-constructed QPainter will not reflect the clip region that is
    used by the paint system.

    \sa QPainter, QWidget::update(), QWidget::repaint(),
        QWidget::paintEvent()
*/

/*!
    Constructs a paint event object with the region that needs to
    be updated. The region is specified by \a paintRegion.
*/
QPaintEvent::QPaintEvent(const QRegion& paintRegion)
    : QEvent(Paint), m_rect(paintRegion.boundingRect()), m_region(paintRegion), m_erased(false)
{}

/*!
    Constructs a paint event object with the rectangle that needs
    to be updated. The region is specified by \a paintRect.
*/
QPaintEvent::QPaintEvent(const QRect &paintRect)
    : QEvent(Paint), m_rect(paintRect),m_region(paintRect), m_erased(false)
{}


Q_IMPL_EVENT_COMMON(QPaintEvent)

/*!
    \fn const QRect &QPaintEvent::rect() const

    Returns the rectangle that needs to be updated.

    \sa region(), QPainter::setClipRect()
*/

/*!
    \fn const QRegion &QPaintEvent::region() const

    Returns the region that needs to be updated.

    \sa rect(), QPainter::setClipRegion()
*/


/*!
    \class QMoveEvent
    \brief The QMoveEvent class contains event parameters for move events.
    \inmodule QtGui

    \ingroup events

    Move events are sent to widgets that have been moved to a new
    position relative to their parent.

    The event handler QWidget::moveEvent() receives move events.

    \sa QWidget::move(), QWidget::setGeometry()
*/

/*!
    Constructs a move event with the new and old widget positions,
    \a pos and \a oldPos respectively.
*/
QMoveEvent::QMoveEvent(const QPoint &pos, const QPoint &oldPos)
    : QEvent(Move), m_pos(pos), m_oldPos(oldPos)
{}

Q_IMPL_EVENT_COMMON(QMoveEvent)

/*!
    \fn const QPoint &QMoveEvent::pos() const

    Returns the new position of the widget. This excludes the window
    frame for top level widgets.
*/

/*!
    \fn const QPoint &QMoveEvent::oldPos() const

    Returns the old position of the widget.
*/

/*!
    \class QExposeEvent
    \since 5.0
    \brief The QExposeEvent class contains event parameters for expose events.
    \inmodule QtGui

    \ingroup events

    Expose events are sent to windows when they move between the un-exposed and
    exposed states.

    An exposed window is potentially visible to the user. If the window is moved
    off screen, is made totally obscured by another window, is minimized, or
    similar, an expose event is sent to the window, and isExposed() might
    change to false.

    Expose events should not be used to paint. Handle QPaintEvent
    instead.

    The event handler QWindow::exposeEvent() receives expose events.
*/

/*!
    Constructs an expose event for the given \a exposeRegion which must be
    in local coordinates.
*/
QExposeEvent::QExposeEvent(const QRegion &exposeRegion)
    : QEvent(Expose)
    , m_region(exposeRegion)
{
}

Q_IMPL_EVENT_COMMON(QExposeEvent)

/*!
    \class QPlatformSurfaceEvent
    \since 5.5
    \brief The QPlatformSurfaceEvent class is used to notify about native platform surface events.
    \inmodule QtGui

    \ingroup events

    Platform window events are synchronously sent to windows and offscreen surfaces when their
    underlying native surfaces are created or are about to be destroyed.

    Applications can respond to these events to know when the underlying platform
    surface exists.
*/

/*!
    \enum QPlatformSurfaceEvent::SurfaceEventType

    This enum describes the type of platform surface event. The possible types are:

    \value SurfaceCreated               The underlying native surface has been created
    \value SurfaceAboutToBeDestroyed    The underlying native surface will be destroyed immediately after this event

    The \c SurfaceAboutToBeDestroyed event type is useful as a means of stopping rendering to
    a platform window before it is destroyed.
*/

/*!
    \fn QPlatformSurfaceEvent::SurfaceEventType QPlatformSurfaceEvent::surfaceEventType() const

    Returns the specific type of platform surface event.
*/

/*!
    Constructs a platform surface event for the given \a surfaceEventType.
*/
QPlatformSurfaceEvent::QPlatformSurfaceEvent(SurfaceEventType surfaceEventType)
    : QEvent(PlatformSurface)
    , m_surfaceEventType(surfaceEventType)
{
}

Q_IMPL_EVENT_COMMON(QPlatformSurfaceEvent)

/*!
    \fn const QRegion &QExposeEvent::region() const
    \deprecated [6.0] Use QPaintEvent instead.

    Returns the window area that has been exposed. The region is given in local coordinates.
*/

/*!
    \class QResizeEvent
    \brief The QResizeEvent class contains event parameters for resize events.
    \inmodule QtGui

    \ingroup events

    Resize events are sent to widgets that have been resized.

    The event handler QWidget::resizeEvent() receives resize events.

    \sa QWidget::resize(), QWidget::setGeometry()
*/

/*!
    Constructs a resize event with the new and old widget sizes, \a
    size and \a oldSize respectively.
*/
QResizeEvent::QResizeEvent(const QSize &size, const QSize &oldSize)
    : QEvent(Resize), m_size(size), m_oldSize(oldSize)
{}

Q_IMPL_EVENT_COMMON(QResizeEvent)

/*!
    \fn const QSize &QResizeEvent::size() const

    Returns the new size of the widget. This is the same as
    QWidget::size().
*/

/*!
    \fn const QSize &QResizeEvent::oldSize() const

    Returns the old size of the widget.
*/


/*!
    \class QCloseEvent
    \brief The QCloseEvent class contains parameters that describe a close event.

    \ingroup events
    \inmodule QtGui

    Close events are sent to widgets that the user wants to close,
    usually by choosing "Close" from the window menu, or by clicking
    the \uicontrol{X} title bar button. They are also sent when you call
    QWidget::close() to close a widget programmatically.

    Close events contain a flag that indicates whether the receiver
    wants the widget to be closed or not. When a widget accepts the
    close event, it is hidden (and destroyed if it was created with
    the Qt::WA_DeleteOnClose flag). If it refuses to accept the close
    event nothing happens. (Under X11 it is possible that the window
    manager will forcibly close the window; but at the time of writing
    we are not aware of any window manager that does this.)

    The event handler QWidget::closeEvent() receives close events. The
    default implementation of this event handler accepts the close
    event. If you do not want your widget to be hidden, or want some
    special handling, you should reimplement the event handler and
    ignore() the event.

    If you want the widget to be deleted when it is closed, create it
    with the Qt::WA_DeleteOnClose flag. This is very useful for
    independent top-level windows in a multi-window application.

    \l{QObject}s emits the \l{QObject::destroyed()}{destroyed()}
    signal when they are deleted.

    If the last top-level window is closed, the
    QGuiApplication::lastWindowClosed() signal is emitted.

    The isAccepted() function returns \c true if the event's receiver has
    agreed to close the widget; call accept() to agree to close the
    widget and call ignore() if the receiver of this event does not
    want the widget to be closed.

    \sa QWidget::close(), QWidget::hide(), QObject::destroyed(),
        QCoreApplication::exec(), QCoreApplication::quit(),
        QGuiApplication::lastWindowClosed()
*/

/*!
    Constructs a close event object.

    \sa accept()
*/
QCloseEvent::QCloseEvent()
    : QEvent(Close)
{}

Q_IMPL_EVENT_COMMON(QCloseEvent)

/*!
   \class QIconDragEvent
   \brief The QIconDragEvent class indicates that a main icon drag has begun.
    \inmodule QtGui

   \ingroup events

   Icon drag events are sent to widgets when the main icon of a window
   has been dragged away. On \macos, this happens when the proxy
   icon of a window is dragged off the title bar.

   It is normal to begin using drag and drop in response to this
   event.

   \sa {Drag and Drop}, QMimeData, QDrag
*/

/*!
    Constructs an icon drag event object with the accept flag set to
    false.

    \sa accept()
*/
QIconDragEvent::QIconDragEvent()
    : QEvent(IconDrag)
{ ignore(); }

Q_IMPL_EVENT_COMMON(QIconDragEvent)

/*!
    \class QContextMenuEvent
    \brief The QContextMenuEvent class contains parameters that describe a context menu event.
    \inmodule QtGui

    \ingroup events

    Context menu events are sent to widgets when a user performs
    an action associated with opening a context menu.
    The actions required to open context menus vary between platforms;
    for example, on Windows, pressing the menu button or clicking the
    right mouse button will cause this event to be sent.

    When this event occurs it is customary to show a QMenu with a
    context menu, if this is relevant to the context.
*/

#ifndef QT_NO_CONTEXTMENU
/*!
    Constructs a context menu event object with the accept parameter
    flag set to false.

    The \a reason parameter must be QContextMenuEvent::Mouse or
    QContextMenuEvent::Keyboard.

    The \a pos parameter specifies the mouse position relative to the
    receiving widget. \a globalPos is the mouse position in absolute
    coordinates. The \a modifiers holds the keyboard modifiers.
*/
QContextMenuEvent::QContextMenuEvent(Reason reason, const QPoint &pos, const QPoint &globalPos,
                                     Qt::KeyboardModifiers modifiers)
    : QInputEvent(ContextMenu, QPointingDevice::primaryPointingDevice(), modifiers), m_pos(pos), m_globalPos(globalPos), m_reason(reason)
{}

Q_IMPL_EVENT_COMMON(QContextMenuEvent)

#if QT_DEPRECATED_SINCE(6, 4)
/*!
    \deprecated [6.4] Use the other constructor instead (global position is required).

    Constructs a context menu event object with the accept parameter
    flag set to false.

    The \a reason parameter must be QContextMenuEvent::Mouse or
    QContextMenuEvent::Keyboard.

    The \a pos parameter specifies the mouse position relative to the
    receiving widget.

    The globalPos() is initialized to QCursor::pos(), which may not be
    appropriate. Use the other constructor to specify the global
    position explicitly.
*/
QContextMenuEvent::QContextMenuEvent(Reason reason, const QPoint &pos)
    : QInputEvent(ContextMenu, QInputDevice::primaryKeyboard()), m_pos(pos), m_reason(reason)
{
#ifndef QT_NO_CURSOR
    m_globalPos = QCursor::pos();
#endif
}
#endif

/*!
    \fn const QPoint &QContextMenuEvent::pos() const

    Returns the position of the mouse pointer relative to the widget
    that received the event.

    \sa x(), y(), globalPos()
*/

/*!
    \fn int QContextMenuEvent::x() const

    Returns the x position of the mouse pointer, relative to the
    widget that received the event.

    \sa y(), pos()
*/

/*!
    \fn int QContextMenuEvent::y() const

    Returns the y position of the mouse pointer, relative to the
    widget that received the event.

    \sa x(), pos()
*/

/*!
    \fn const QPoint &QContextMenuEvent::globalPos() const

    Returns the global position of the mouse pointer at the time of
    the event.

    \sa x(), y(), pos()
*/

/*!
    \fn int QContextMenuEvent::globalX() const

    Returns the global x position of the mouse pointer at the time of
    the event.

    \sa globalY(), globalPos()
*/

/*!
    \fn int QContextMenuEvent::globalY() const

    Returns the global y position of the mouse pointer at the time of
    the event.

    \sa globalX(), globalPos()
*/
#endif // QT_NO_CONTEXTMENU

/*!
    \enum QContextMenuEvent::Reason

    This enum describes the reason why the event was sent.

    \value Mouse The mouse caused the event to be sent. Normally this
    means the right mouse button was clicked, but this is platform
    dependent.

    \value Keyboard The keyboard caused this event to be sent. On
    Windows, this means the menu button was pressed.

    \value Other The event was sent by some other means (i.e. not by
    the mouse or keyboard).
*/


/*!
    \fn QContextMenuEvent::Reason QContextMenuEvent::reason() const

    Returns the reason for this context event.
*/


/*!
    \class QInputMethodEvent
    \brief The QInputMethodEvent class provides parameters for input method events.
    \inmodule QtGui

    \ingroup events

    Input method events are sent to widgets when an input method is
    used to enter text into a widget. Input methods are widely used
    to enter text for languages with non-Latin alphabets.

    Note that when creating custom text editing widgets, the
    Qt::WA_InputMethodEnabled window attribute must be set explicitly
    (using the QWidget::setAttribute() function) in order to receive
    input method events.

    The events are of interest to authors of keyboard entry widgets
    who want to be able to correctly handle languages with complex
    character input. Text input in such languages is usually a three
    step process:

    \list 1
    \li \b{Starting to Compose}

       When the user presses the first key on a keyboard, an input
       context is created. This input context will contain a string
       of the typed characters.

    \li \b{Composing}

       With every new key pressed, the input method will try to create a
       matching string for the text typed so far called preedit
       string. While the input context is active, the user can only move
       the cursor inside the string belonging to this input context.

    \li \b{Completing}

       At some point, the user will activate a user interface component
       (perhaps using a particular key) where they can choose from a
       number of strings matching the text they have typed so far. The
       user can either confirm their choice cancel the input; in either
       case the input context will be closed.
    \endlist

    QInputMethodEvent models these three stages, and transfers the
    information needed to correctly render the intermediate result. A
    QInputMethodEvent has two main parameters: preeditString() and
    commitString(). The preeditString() parameter gives the currently
    active preedit string. The commitString() parameter gives a text
    that should get added to (or replace parts of) the text of the
    editor widget. It usually is a result of the input operations and
    has to be inserted to the widgets text directly before the preedit
    string.

    If the commitString() should replace parts of the text in
    the editor, replacementLength() will contain the number of
    characters to be replaced. replacementStart() contains the position
    at which characters are to be replaced relative from the start of
    the preedit string.

    A number of attributes control the visual appearance of the
    preedit string (the visual appearance of text outside the preedit
    string is controlled by the widget only). The AttributeType enum
    describes the different attributes that can be set.

    A class implementing QWidget::inputMethodEvent() or
    QGraphicsItem::inputMethodEvent() should at least understand and
    honor the \l TextFormat and \l Cursor attributes.

    Since input methods need to be able to query certain properties
    from the widget or graphics item, subclasses must also implement
    QWidget::inputMethodQuery() and QGraphicsItem::inputMethodQuery(),
    respectively.

    When receiving an input method event, the text widget has to performs the
    following steps:

    \list 1
    \li If the widget has selected text, the selected text should get
       removed.

    \li Remove the text starting at replacementStart() with length
       replacementLength() and replace it by the commitString(). If
       replacementLength() is 0, replacementStart() gives the insertion
       position for the commitString().

       When doing replacement the area of the preedit
       string is ignored, thus a replacement starting at -1 with a length
       of 2 will remove the last character before the preedit string and
       the first character afterwards, and insert the commit string
       directly before the preedit string.

       If the widget implements undo/redo, this operation gets added to
       the undo stack.

    \li If there is no current preedit string, insert the
       preeditString() at the current cursor position; otherwise replace
       the previous preeditString with the one received from this event.

       If the widget implements undo/redo, the preeditString() should not
       influence the undo/redo stack in any way.

       The widget should examine the list of attributes to apply to the
       preedit string. It has to understand at least the TextFormat and
       Cursor attributes and render them as specified.
    \endlist

    \sa QInputMethod
*/

/*!
    \enum QInputMethodEvent::AttributeType

    \value TextFormat
    A QTextCharFormat for the part of the preedit string specified by
    start and length. value contains a QVariant of type QTextFormat
    specifying rendering of this part of the preedit string. There
    should be at most one format for every part of the preedit
    string. If several are specified for any character in the string the
    behaviour is undefined. A conforming implementation has to at least
    honor the backgroundColor, textColor and fontUnderline properties
    of the format.

    \value Cursor If set, a cursor should be shown inside the preedit
    string at position start. The length variable determines whether
    the cursor is visible or not. If the length is 0 the cursor is
    invisible. If value is a QVariant of type QColor this color will
    be used for rendering the cursor, otherwise the color of the
    surrounding text will be used. There should be at most one Cursor
    attribute per event. If several are specified the behaviour is
    undefined.

    \value Language
    The variant contains a QLocale object specifying the language of a
    certain part of the preedit string. There should be at most one
    language set for every part of the preedit string. If several are
    specified for any character in the string the behavior is undefined.

    \value Ruby
    The ruby text for a part of the preedit string. There should be at
    most one ruby text set for every part of the preedit string. If
    several are specified for any character in the string the behaviour
    is undefined.

    \value Selection
    If set, the edit cursor should be moved to the specified position
    in the editor text contents. In contrast with \c Cursor, this
    attribute does not work on the preedit text, but on the surrounding
    text. The cursor will be moved after the commit string has been
    committed, and the preedit string will be located at the new edit
    position.
    The start position specifies the new position and the length
    variable can be used to set a selection starting from that point.
    The value is unused.

    \sa Attribute
*/

/*!
    \class QInputMethodEvent::Attribute
    \inmodule QtGui
    \brief The QInputMethodEvent::Attribute class stores an input method attribute.
*/

/*!
    \fn QInputMethodEvent::Attribute::Attribute(AttributeType type, int start, int length, QVariant value)

    Constructs an input method attribute. \a type specifies the type
    of attribute, \a start and \a length the position of the
    attribute, and \a value the value of the attribute.
*/

/*!
    \fn QInputMethodEvent::Attribute::Attribute(AttributeType type, int start, int length)
    \overload
    \since 5.7

    Constructs an input method attribute with no value. \a type
    specifies the type of attribute, and \a start and \a length
    the position of the attribute.
*/

/*!
    Constructs an event of type QEvent::InputMethod. The
    attributes(), preeditString(), commitString(), replacementStart(),
    and replacementLength() are initialized to default values.

    \sa setCommitString()
*/
QInputMethodEvent::QInputMethodEvent()
    : QEvent(QEvent::InputMethod), m_replacementStart(0), m_replacementLength(0)
{
}

/*!
    Constructs an event of type QEvent::InputMethod. The
    preedit text is set to \a preeditText, the attributes to
    \a attributes.

    The commitString(), replacementStart(), and replacementLength()
    values can be set using setCommitString().

    \sa preeditString(), attributes()
*/
QInputMethodEvent::QInputMethodEvent(const QString &preeditText, const QList<Attribute> &attributes)
    : QEvent(QEvent::InputMethod), m_preedit(preeditText), m_attributes(attributes),
      m_replacementStart(0), m_replacementLength(0)
{
}

Q_IMPL_EVENT_COMMON(QInputMethodEvent)

/*!
    Sets the commit string to \a commitString.

    The commit string is the text that should get added to (or
    replace parts of) the text of the editor widget. It usually is a
    result of the input operations and has to be inserted to the
    widgets text directly before the preedit string.

    If the commit string should replace parts of the text in
    the editor, \a replaceLength specifies the number of
    characters to be replaced. \a replaceFrom specifies the position
    at which characters are to be replaced relative from the start of
    the preedit string.

    \sa commitString(), replacementStart(), replacementLength()
*/
void QInputMethodEvent::setCommitString(const QString &commitString, int replaceFrom, int replaceLength)
{
    m_commit = commitString;
    m_replacementStart = replaceFrom;
    m_replacementLength = replaceLength;
}

/*!
    \fn const QList<Attribute> &QInputMethodEvent::attributes() const

    Returns the list of attributes passed to the QInputMethodEvent
    constructor. The attributes control the visual appearance of the
    preedit string (the visual appearance of text outside the preedit
    string is controlled by the widget only).

    \sa preeditString(), Attribute
*/

/*!
    \fn const QString &QInputMethodEvent::preeditString() const

    Returns the preedit text, i.e. the text before the user started
    editing it.

    \sa commitString(), attributes()
*/

/*!
    \fn const QString &QInputMethodEvent::commitString() const

    Returns the text that should get added to (or replace parts of)
    the text of the editor widget. It usually is a result of the
    input operations and has to be inserted to the widgets text
    directly before the preedit string.

    \sa setCommitString(), preeditString(), replacementStart(), replacementLength()
*/

/*!
    \fn int QInputMethodEvent::replacementStart() const

    Returns the position at which characters are to be replaced relative
    from the start of the preedit string.

    \sa replacementLength(), setCommitString()
*/

/*!
    \fn int QInputMethodEvent::replacementLength() const

    Returns the number of characters to be replaced in the preedit
    string.

    \sa replacementStart(), setCommitString()
*/

/*!
    \class QInputMethodQueryEvent
    \since 5.0
    \inmodule QtGui

    \brief The QInputMethodQueryEvent class provides an event sent by the input context to input objects.

    It is used by the
    input method to query a set of properties of the object to be
    able to support complex input method operations as support for
    surrounding text and reconversions.

    queries() specifies which properties are queried.

    The object should call setValue() on the event to fill in the requested
    data before calling accept().
*/

/*!
    \fn Qt::InputMethodQueries QInputMethodQueryEvent::queries() const

    Returns the properties queried by the event.
 */

/*!
    Constructs a query event for properties given by \a queries.
 */
QInputMethodQueryEvent::QInputMethodQueryEvent(Qt::InputMethodQueries queries)
    : QEvent(InputMethodQuery),
      m_queries(queries)
{
}

Q_IMPL_EVENT_COMMON(QInputMethodQueryEvent)

/*!
    Sets property \a query to \a value.
 */
void QInputMethodQueryEvent::setValue(Qt::InputMethodQuery query, const QVariant &value)
{
    for (int i = 0; i < m_values.size(); ++i) {
        if (m_values.at(i).query == query) {
            m_values[i].value = value;
            return;
        }
    }
    QueryPair pair = { query, value };
    m_values.append(pair);
}

/*!
    Returns value of the property \a query.
 */
QVariant QInputMethodQueryEvent::value(Qt::InputMethodQuery query) const
{
    for (int i = 0; i < m_values.size(); ++i)
        if (m_values.at(i).query == query)
            return m_values.at(i).value;
    return QVariant();
}

#if QT_CONFIG(tabletevent)

/*!
    \class QTabletEvent
    \brief The QTabletEvent class contains parameters that describe a Tablet event.
    \inmodule QtGui

    \ingroup events

    \e{Tablet events} are generated from tablet peripherals such as Wacom
    tablets and various other brands, and electromagnetic stylus devices
    included with some types of tablet computers. (It is not the same as
    \l QTouchEvent which a touchscreen generates, even when a passive stylus is
    used on a touchscreen.)

    Tablet events are similar to mouse events; for example, the \l x(), \l y(),
    \l pos(), \l globalX(), \l globalY(), and \l globalPos() accessors provide
    the cursor position, and you can see which \l buttons() are pressed
    (pressing the stylus tip against the tablet surface is equivalent to a left
    mouse button). But tablet events also pass through some extra information
    that the tablet device driver provides; for example, you might want to do
    subpixel rendering with higher resolution coordinates (\l globalPosF()),
    adjust color brightness based on the \l pressure() of the tool against the
    tablet surface, use different brushes depending on the type of tool in use
    (\l deviceType()), modulate the brush shape in some way according to the
    X-axis and Y-axis tilt of the tool with respect to the tablet surface
    (\l xTilt() and \l yTilt()), and use a virtual eraser instead of a brush if
    the user switches to the other end of a double-ended stylus
    (\l pointerType()).

    Every event contains an accept flag that indicates whether the receiver
    wants the event. You should call QTabletEvent::accept() if you handle the
    tablet event; otherwise it will be sent to the parent widget. The exception
    are TabletEnterProximity and TabletLeaveProximity events: these are only
    sent to QApplication and do not check whether or not they are accepted.

    The QWidget::setEnabled() function can be used to enable or disable
    mouse, tablet and keyboard events for a widget.

    The event handler QWidget::tabletEvent() receives TabletPress,
    TabletRelease and TabletMove events. Qt will first send a
    tablet event, then if it is not accepted by any widget, it will send a
    mouse event. This allows users of applications that are not designed for
    tablets to use a tablet like a mouse. However high-resolution drawing
    applications should handle the tablet events, because they can occur at a
    higher frequency, which is a benefit for smooth and accurate drawing.
    If the tablet events are rejected, the synthetic mouse events may be
    compressed for efficiency.

    Note that pressing the stylus button while the stylus hovers over the
    tablet will generate a button press on some types of tablets, while on
    other types it will be necessary to press the stylus against the tablet
    surface in order to register the simultaneous stylus button press.

    \section1 Notes for X11 Users

    If the tablet is configured in xorg.conf to use the Wacom driver, there
    will be separate XInput "devices" for the stylus, eraser, and (optionally)
    cursor and touchpad. Qt recognizes these by their names. Otherwise, if the
    tablet is configured to use the evdev driver, there will be only one device
    and applications may not be able to distinguish the stylus from the eraser.

    \section1 Notes for Windows Users

    Tablet support currently requires the WACOM windows driver providing the DLL
    \c{wintab32.dll} to be installed. It is contained in older packages,
    for example \c{pentablet_5.3.5-3.exe}.

*/

/*!
    Construct a tablet event of the given \a type.

    The \a pos parameter indicates where the event occurred in the widget;
    \a globalPos is the corresponding position in absolute coordinates.

    \a pressure gives the pressure exerted on the device \a dev.

    \a xTilt and \a yTilt give the device's degree of tilt from the
    x and y axes respectively.

    \a keyState specifies which keyboard modifiers are pressed (e.g.,
    \uicontrol{Ctrl}).

    The \a z parameter gives the Z coordinate of the device on the tablet;
    this is usually given by a wheel on a 4D mouse. If the device does not
    support a Z-axis (i.e. \l QPointingDevice::capabilities() does not include
    \c ZPosition), pass \c 0 here.

    The \a tangentialPressure parameter gives the tangential pressure
    thumbwheel value from an airbrush. If the device does not support
    tangential pressure (i.e. \l QPointingDevice::capabilities() does not
    include \c TangentialPressure), pass \c 0 here.

    \a rotation gives the device's rotation in degrees.
    4D mice, the Wacom Art Pen, and the Apple Pencil support rotation.
    If the device does not support rotation (i.e. \l QPointingDevice::capabilities()
    does not include \c Rotation), pass \c 0 here.

    The \a button that caused the event is given as a value from the
    \l Qt::MouseButton enum. If the event \a type is not \l TabletPress or
    \l TabletRelease, the appropriate button for this event is \l Qt::NoButton.

    \a buttons is the state of all buttons at the time of the event.

    \sa pos(), globalPos(), device(), pressure(), xTilt(), yTilt(), uniqueId(), rotation(),
      tangentialPressure(), z()
*/
QTabletEvent::QTabletEvent(Type type, const QPointingDevice *dev, const QPointF &pos, const QPointF &globalPos,
                 qreal pressure, float xTilt, float yTilt,
                 float tangentialPressure, qreal rotation, float z,
                 Qt::KeyboardModifiers keyState,
                 Qt::MouseButton button, Qt::MouseButtons buttons)
    : QSinglePointEvent(type, dev, pos, pos, globalPos, button, buttons, keyState),
      m_tangential(tangentialPressure),
      m_xTilt(xTilt),
      m_yTilt(yTilt),
      m_z(z)
{
    QEventPoint &p = point(0);
    QMutableEventPoint::setPressure(p, pressure);
    QMutableEventPoint::setRotation(p, rotation);
}

Q_IMPL_POINTER_EVENT(QTabletEvent)

/*!
    \fn qreal QTabletEvent::tangentialPressure() const

    Returns the tangential pressure for the device.  This is typically given by a finger
    wheel on an airbrush tool.  The range is from -1.0 to 1.0. 0.0 indicates a
    neutral position.  Current airbrushes can only move in the positive
    direction from the neutrual position. If the device does not support
    tangential pressure, this value is always 0.0.

    \note The value is stored as a single-precision float.

    \sa pressure()
*/

/*!
    \fn qreal QTabletEvent::rotation() const

    Returns the rotation of the current tool in degrees, where zero means the
    tip of the stylus is pointing towards the top of the tablet, a positive
    value means it's turned to the right, and a negative value means it's
    turned to the left. This can be given by a 4D Mouse or a rotation-capable
    stylus (such as the Wacom Art Pen or the Apple Pencil). If the device does
    not support rotation, this value is always 0.0.
*/

/*!
    \fn qreal QTabletEvent::pressure() const

    Returns the pressure for the device. 0.0 indicates that the stylus is not
    on the tablet, 1.0 indicates the maximum amount of pressure for the stylus.

    \sa tangentialPressure()
*/

/*!
    \fn qreal QTabletEvent::xTilt() const

    Returns the angle between the device (a pen, for example) and the
    perpendicular in the direction of the x axis.
    Positive values are towards the tablet's physical right. The angle
    is in the range -60 to +60 degrees.

    \image qtabletevent-tilt.png

    \note The value is stored as a single-precision float.

    \sa yTilt()
*/

/*!
    \fn qreal QTabletEvent::yTilt() const

    Returns the angle between the device (a pen, for example) and the
    perpendicular in the direction of the y axis.
    Positive values are towards the bottom of the tablet. The angle is
    within the range -60 to +60 degrees.

    \note The value is stored as a single-precision float.

    \sa xTilt()
*/

/*!
    \fn QPoint QTabletEvent::pos() const
    \deprecated [6.0] Use position().toPoint() instead.

    Returns the position of the device, relative to the widget that
    received the event.

    If you move widgets around in response to mouse events, use
    globalPos() instead of this function.

    \sa x(), y(), globalPos()
*/

/*!
    \fn int QTabletEvent::x() const
    \deprecated [6.0] Use position().x() instead.

    Returns the x position of the device, relative to the widget that
    received the event.

    \sa y(), pos()
*/

/*!
    \fn int QTabletEvent::y() const
    \deprecated [6.0] Use position().y() instead.

    Returns the y position of the device, relative to the widget that
    received the event.

    \sa x(), pos()
*/

/*!
    \fn qreal QTabletEvent::z() const

    Returns the z position of the device. Typically this is represented by a
    wheel on a 4D Mouse. If the device does not support a Z-axis, this value is
    always zero. This is \b not the same as pressure.

    \note The value is stored as a single-precision float.

    \sa pressure()
*/

/*!
    \fn QPoint QTabletEvent::globalPos() const
    \deprecated [6.0] Use globalPosition().toPoint() instead.

    Returns the global position of the device \e{at the time of the
    event}. This is important on asynchronous windows systems like X11;
    whenever you move your widgets around in response to mouse events,
    globalPos() can differ significantly from the current position
    QCursor::pos().

    \sa globalX(), globalY()
*/

/*!
    \fn int QTabletEvent::globalX() const
    \deprecated [6.0] Use globalPosition().x() instead.

    Returns the global x position of the mouse pointer at the time of
    the event.

    \sa globalY(), globalPos()
*/

/*!
    \fn int QTabletEvent::globalY() const
    \deprecated [6.0] Use globalPosition().y() instead.

    Returns the global y position of the tablet device at the time of
    the event.

    \sa globalX(), globalPos()
*/

/*!
    \fn qint64 QTabletEvent::uniqueId() const
    \deprecated [6.0] Use pointingDevice().uniqueId() instead.

    Returns a unique ID for the current device, making it possible
    to differentiate between multiple devices being used at the same
    time on the tablet.

    Support of this feature is dependent on the tablet.

    Values for the same device may vary from OS to OS.

    Later versions of the Wacom driver for Linux will now report
    the ID information. If you have a tablet that supports unique ID
    and are not getting the information on Linux, consider upgrading
    your driver.

    As of Qt 4.2, the unique ID is the same regardless of the orientation
    of the pen. Earlier versions would report a different value when using
    the eraser-end versus the pen-end of the stylus on some OS's.

    \sa pointerType()
*/

/*!
    \fn const QPointF &QTabletEvent::posF() const
    \deprecated [6.0] Use position() instead.

    Returns the position of the device, relative to the widget that
    received the event.

    If you move widgets around in response to mouse events, use
    globalPosF() instead of this function.

    \sa globalPosF()
*/

/*!
    \fn const QPointF &QTabletEvent::globalPosF() const
    \deprecated [6.0] Use globalPosition() instead.
    Returns the global position of the device \e{at the time of the
    event}. This is important on asynchronous windows systems like X11;
    whenever you move your widgets around in response to mouse events,
    globalPosF() can differ significantly from the current position
    QCursor::pos().

    \sa posF()
*/

#endif // QT_CONFIG(tabletevent)

#ifndef QT_NO_GESTURES
/*!
    \class QNativeGestureEvent
    \since 5.2
    \brief The QNativeGestureEvent class contains parameters that describe a gesture event.
    \inmodule QtGui
    \ingroup events

    Native gesture events are generated by the operating system, typically by
    interpreting trackpad touch events. Gesture events are high-level events
    such as zoom, rotate or pan. Several types hold incremental values: that is,
    value() and delta() provide the difference from the previous event to the
    current event.

    \table
    \header
        \li Event Type
        \li Description
        \li Touch sequence
    \row
        \li Qt::ZoomNativeGesture
        \li Magnification delta in percent.
        \li \macos and Wayland: Two-finger pinch.
    \row
        \li Qt::SmartZoomNativeGesture
        \li Boolean magnification state.
        \li \macos: Two-finger douple tap (trackpad) / One-finger douple tap (magic mouse).
    \row
        \li Qt::RotateNativeGesture
        \li Rotation delta in degrees.
        \li \macos and Wayland: Two-finger rotate.
    \row
        \li Qt::SwipeNativeGesture
        \li Swipe angle in degrees.
        \li \macos: Configurable in trackpad settings.
    \row
        \li Qt::PanNativeGesture
        \li Displacement delta in pixels.
        \li Wayland: Three or more fingers moving as a group, in any direction.
    \endtable

    In addition, BeginNativeGesture and EndNativeGesture are sent before and after
    gesture event streams:

        BeginNativeGesture
        ZoomNativeGesture
        ZoomNativeGesture
        ZoomNativeGesture
        EndNativeGesture

    The event stream may include interleaved gestures of different types:
    for example the two-finger pinch gesture generates a stream of Zoom and
    Rotate events, and PanNativeGesture may sometimes be interleaved with
    those, depending on the platform.

    Other types are standalone events: SmartZoomNativeGesture and
    SwipeNativeGesture occur only once each time the gesture is detected.

    \note On a touchpad, moving two fingers as a group (the two-finger flick gesture)
    is usually reserved for scrolling; in that case, Qt generates QWheelEvents.
    This is the reason that three or more fingers are needed to generate a
    PanNativeGesture.

    \sa Qt::NativeGestureType, QGestureEvent, QWheelEvent
*/

#if QT_DEPRECATED_SINCE(6, 2)
/*!
    \deprecated [6.2] Use the other constructor, because \a intValue is no longer stored separately.

    Constructs a native gesture event of type \a type originating from \a device.

    The points \a localPos, \a scenePos and \a globalPos specify the
    gesture position relative to the receiving widget or item,
    window, and screen or desktop, respectively.

    \a realValue is the \macos event parameter, \a sequenceId and \a intValue are the Windows event parameters.
    \since 5.10

    \note It's not possible to store realValue and \a intValue simultaneously:
    one or the other must be zero. If \a realValue == 0 and \a intValue != 0,
    it is stored in the same variable, such that value() returns the value
    given as \a intValue.
*/
QNativeGestureEvent::QNativeGestureEvent(Qt::NativeGestureType type, const QPointingDevice *device,
                                        const QPointF &localPos, const QPointF &scenePos,
                                        const QPointF &globalPos, qreal realValue, quint64 sequenceId,
                                        quint64 intValue)
    : QSinglePointEvent(QEvent::NativeGesture, device, localPos, scenePos, globalPos, Qt::NoButton,
                        Qt::NoButton, Qt::NoModifier),
      m_sequenceId(sequenceId), m_realValue(realValue), m_gestureType(type)
{
    if (qIsNull(realValue) && intValue != 0)
        m_realValue = intValue;
}
#endif // deprecated

/*!
    Constructs a native gesture event of type \a type originating from \a device
    describing a gesture at \a scenePos in which \a fingerCount fingers are involved.

    The points \a localPos, \a scenePos and \a globalPos specify the gesture
    position relative to the receiving widget or item, window, and screen or
    desktop, respectively.

    \a value has a gesture-dependent interpretation: for RotateNativeGesture or
    SwipeNativeGesture, it's an angle in degrees. For ZoomNativeGesture,
    \a value is an incremental scaling factor, usually much less than 1,
    indicating that the target item should have its scale adjusted like this:
    item.scale = item.scale * (1 + event.value)

    For PanNativeGesture, \a delta gives the distance in pixels that the
    viewport, widget or item should be moved or panned.

    \note The \a delta is stored in single precision (QVector2D), so \l delta()
    may return slightly different values in some cases. This is subject to change
    in future versions of Qt.

    \since 6.2
*/
QNativeGestureEvent::QNativeGestureEvent(Qt::NativeGestureType type, const QPointingDevice *device, int fingerCount,
                                         const QPointF &localPos, const QPointF &scenePos,
                                         const QPointF &globalPos, qreal value, const QPointF &delta,
                                         quint64 sequenceId)
    : QSinglePointEvent(QEvent::NativeGesture, device, localPos, scenePos, globalPos, Qt::NoButton,
                        Qt::NoButton, Qt::NoModifier),
      m_sequenceId(sequenceId), m_delta(delta), m_realValue(value), m_gestureType(type), m_fingerCount(fingerCount)
{
    Q_ASSERT(fingerCount < 16); // we store it in 4 bits unsigned
}

Q_IMPL_POINTER_EVENT(QNativeGestureEvent)

/*!
    \fn QNativeGestureEvent::gestureType() const
    \since 5.2

    Returns the gesture type.
*/

/*!
    \fn QNativeGestureEvent::fingerCount() const
    \since 6.2

    Returns the number of fingers participating in the gesture, if known.
    When gestureType() is Qt::BeginNativeGesture or Qt::EndNativeGesture, often
    this information is unknown, and fingerCount() returns \c 0.
*/

/*!
    \fn QNativeGestureEvent::value() const
    \since 5.2

    Returns the gesture value. The value should be interpreted based on the
    gesture type. For example, a Zoom gesture provides a scale factor delta while a Rotate
    gesture provides a rotation delta.

    \sa QNativeGestureEvent, gestureType()
*/

/*!
    \fn QNativeGestureEvent::delta() const
    \since 6.2

    Returns the distance moved since the previous event, in pixels.
    A Pan gesture provides the distance in pixels by which the target widget,
    item or viewport contents should be moved.

    \sa QPanGesture::delta()
*/

/*!
    \fn QPoint QNativeGestureEvent::globalPos() const
    \since 5.2
    \deprecated [6.0] Use globalPosition().toPoint() instead.

    Returns the position of the gesture as a QPointF in screen coordinates
*/

/*!
    \fn QPoint QNativeGestureEvent::pos() const
    \since 5.2
    \deprecated [6.0] Use position().toPoint() instead.

    Returns the position of the mouse cursor, relative to the widget
    or item that received the event.
*/

/*!
    \fn QPointF QNativeGestureEvent::localPos() const
    \since 5.2
    \deprecated [6.0] Use position() instead.

    Returns the position of the gesture as a QPointF, relative to the
    widget or item that received the event.
*/

/*!
    \fn QPointF QNativeGestureEvent::screenPos() const
    \since 5.2
    \deprecated [6.0] Use globalPosition() instead.

    Returns the position of the gesture as a QPointF in screen coordinates.
*/

/*!
    \fn QPointF QNativeGestureEvent::windowPos() const
    \since 5.2
    \deprecated [6.0] Use scenePosition() instead.

    Returns the position of the gesture as a QPointF, relative to the
    window that received the event.
*/
#endif // QT_NO_GESTURES

#if QT_CONFIG(draganddrop)
/*!
    Creates a QDragMoveEvent of the required \a type indicating
    that the mouse is at position \a pos given within a widget.

    The mouse and keyboard states are specified by \a buttons and
    \a modifiers, and the \a actions describe the types of drag
    and drop operation that are possible.
    The drag data is passed as MIME-encoded information in \a data.

    \warning Do not attempt to create a QDragMoveEvent yourself.
    These objects rely on Qt's internal state.
*/
QDragMoveEvent::QDragMoveEvent(const QPoint& pos, Qt::DropActions actions, const QMimeData *data,
                               Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, Type type)
    : QDropEvent(pos, actions, data, buttons, modifiers, type)
    , m_rect(pos, QSize(1, 1))
{}

Q_IMPL_EVENT_COMMON(QDragMoveEvent)

/*!
    \fn void QDragMoveEvent::accept(const QRect &rectangle)

    The same as accept(), but also notifies that future moves will
    also be acceptable if they remain within the \a rectangle
    given on the widget. This can improve performance, but may
    also be ignored by the underlying system.

    If the rectangle is empty, drag move events will be sent
    continuously. This is useful if the source is scrolling in a
    timer event.
*/

/*!
    \fn void QDragMoveEvent::accept()

    \overload

    Calls QDropEvent::accept().
*/

/*!
    \fn void QDragMoveEvent::ignore()

    \overload

    Calls QDropEvent::ignore().
*/

/*!
    \fn void QDragMoveEvent::ignore(const QRect &rectangle)

    The opposite of the accept(const QRect&) function.
    Moves within the \a rectangle are not acceptable, and will be
    ignored.
*/

/*!
    \fn QRect QDragMoveEvent::answerRect() const

    Returns the rectangle in the widget where the drop will occur if accepted.
    You can use this information to restrict drops to certain places on the
    widget.
*/


/*!
    \class QDropEvent
    \ingroup events
    \ingroup draganddrop
    \inmodule QtGui

    \brief The QDropEvent class provides an event which is sent when a
    drag and drop action is completed.

    When a widget \l{QWidget::setAcceptDrops()}{accepts drop events}, it will
    receive this event if it has accepted the most recent QDragEnterEvent or
    QDragMoveEvent sent to it.

    The drop event contains a proposed action, available from proposedAction(), for
    the widget to either accept or ignore. If the action can be handled by the
    widget, you should call the acceptProposedAction() function. Since the
    proposed action can be a combination of \l Qt::DropAction values, it may be
    useful to either select one of these values as a default action or ask
    the user to select their preferred action.

    If the proposed drop action is not suitable, perhaps because your custom
    widget does not support that action, you can replace it with any of the
    \l{possibleActions()}{possible drop actions} by calling setDropAction()
    with your preferred action. If you set a value that is not present in the
    bitwise OR combination of values returned by possibleActions(), the default
    copy action will be used. Once a replacement drop action has been set, call
    accept() instead of acceptProposedAction() to complete the drop operation.

    The mimeData() function provides the data dropped on the widget in a QMimeData
    object. This contains information about the MIME type of the data in addition to
    the data itself.

    \sa QMimeData, QDrag, {Drag and Drop}
*/

/*!
    \fn const QMimeData *QDropEvent::mimeData() const

    Returns the data that was dropped on the widget and its associated MIME
    type information.
*/

// ### pos is in which coordinate system?
/*!
    Constructs a drop event of a certain \a type corresponding to a
    drop at the point specified by \a pos in the destination widget's
    coordinate system.

    The \a actions indicate which types of drag and drop operation can
    be performed, and the drag data is stored as MIME-encoded data in \a data.

    The states of the mouse buttons and keyboard modifiers at the time of
    the drop are specified by \a buttons and \a modifiers.
*/
QDropEvent::QDropEvent(const QPointF& pos, Qt::DropActions actions, const QMimeData *data,
                       Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, Type type)
    : QEvent(type), m_pos(pos), m_mouseState(buttons),
      m_modState(modifiers), m_actions(actions),
      m_data(data)
{
    m_defaultAction = m_dropAction =
        QGuiApplicationPrivate::platformIntegration()->drag()->defaultAction(m_actions, modifiers);
    ignore();
}

Q_IMPL_EVENT_COMMON(QDropEvent)


/*!
    If the source of the drag operation is a widget in this
    application, this function returns that source; otherwise it
    returns \nullptr. The source of the operation is the first parameter to
    the QDrag object used instantiate the drag.

    This is useful if your widget needs special behavior when dragging
    to itself.

    \sa QDrag::QDrag()
*/
QObject* QDropEvent::source() const
{
    if (const QDragManager *manager = QDragManager::self())
        return manager->source();
    return nullptr;
}


void QDropEvent::setDropAction(Qt::DropAction action)
{
    if (!(action & m_actions) && action != Qt::IgnoreAction)
        action = m_defaultAction;
    m_dropAction = action;
}

/*!
    \fn QPoint QDropEvent::pos() const
    \deprecated [6.0] Use position().toPoint() instead.

    Returns the position where the drop was made.
*/

/*!
    \fn const QPointF& QDropEvent::posF() const
    \deprecated [6.0] Use position() instead.

    Returns the position where the drop was made.
*/

/*!
    \fn QPointF QDropEvent::position() const
    \since 6.0

    Returns the position where the drop was made.
*/

/*!
    \fn Qt::MouseButtons QDropEvent::mouseButtons() const
    \deprecated [6.0] Use buttons() instead.

    Returns the mouse buttons that are pressed.
*/

/*!
    \fn Qt::MouseButtons QDropEvent::buttons() const
    \since 6.0

    Returns the mouse buttons that are pressed.
*/

/*!
    \fn Qt::KeyboardModifiers QDropEvent::keyboardModifiers() const
    \deprecated [6.0] Use modifiers() instead.

    Returns the modifier keys that are pressed.
*/

/*!
    \fn Qt::KeyboardModifiers QDropEvent::modifiers() const
    \since 6.0

    Returns the modifier keys that are pressed.
*/

/*!
    \fn void QDropEvent::setDropAction(Qt::DropAction action)

    Sets the \a action to be performed on the data by the target.
    Use this to override the \l{proposedAction()}{proposed action}
    with one of the \l{possibleActions()}{possible actions}.

    If you set a drop action that is not one of the possible actions, the
    drag and drop operation will default to a copy operation.

    Once you have supplied a replacement drop action, call accept()
    instead of acceptProposedAction().

    \sa dropAction()
*/

/*!
    \fn Qt::DropAction QDropEvent::dropAction() const

    Returns the action to be performed on the data by the target. This may be
    different from the action supplied in proposedAction() if you have called
    setDropAction() to explicitly choose a drop action.

    \sa setDropAction()
*/

/*!
    \fn Qt::DropActions QDropEvent::possibleActions() const

    Returns an OR-combination of possible drop actions.

    \sa dropAction()
*/

/*!
    \fn Qt::DropAction QDropEvent::proposedAction() const

    Returns the proposed drop action.

    \sa dropAction()
*/

/*!
    \fn void QDropEvent::acceptProposedAction()

    Sets the drop action to be the proposed action.

    \sa setDropAction(), proposedAction(), {QEvent::accept()}{accept()}
*/

/*!
    \class QDragEnterEvent
    \brief The QDragEnterEvent class provides an event which is sent
    to a widget when a drag and drop action enters it.

    \ingroup events
    \ingroup draganddrop
    \inmodule QtGui

    A widget must accept this event in order to receive the \l
    {QDragMoveEvent}{drag move events} that are sent while the drag
    and drop action is in progress. The drag enter event is always
    immediately followed by a drag move event.

    QDragEnterEvent inherits most of its functionality from
    QDragMoveEvent, which in turn inherits most of its functionality
    from QDropEvent.

    \sa QDragLeaveEvent, QDragMoveEvent, QDropEvent
*/

/*!
    Constructs a QDragEnterEvent that represents a drag entering a
    widget at the given \a point with mouse and keyboard states specified by
    \a buttons and \a modifiers.

    The drag data is passed as MIME-encoded information in \a data, and the
    specified \a actions describe the possible types of drag and drop
    operation that can be performed.

    \warning Do not create a QDragEnterEvent yourself since these
    objects rely on Qt's internal state.
*/
QDragEnterEvent::QDragEnterEvent(const QPoint& point, Qt::DropActions actions, const QMimeData *data,
                                 Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
    : QDragMoveEvent(point, actions, data, buttons, modifiers, DragEnter)
{}

Q_IMPL_EVENT_COMMON(QDragEnterEvent)

/*!
    \class QDragMoveEvent
    \brief The QDragMoveEvent class provides an event which is sent while a drag and drop action is in progress.

    \ingroup events
    \ingroup draganddrop
    \inmodule QtGui

    A widget will receive drag move events repeatedly while the drag
    is within its boundaries, if it accepts
    \l{QWidget::setAcceptDrops()}{drop events} and \l
    {QWidget::dragEnterEvent()}{enter events}. The widget should
    examine the event to see what kind of \l{mimeData()}{data} it
    provides, and call the accept() function to accept the drop if appropriate.

    The rectangle supplied by the answerRect() function can be used to restrict
    drops to certain parts of the widget. For example, we can check whether the
    rectangle intersects with the geometry of a certain child widget and only
    call \l{QDropEvent::acceptProposedAction()}{acceptProposedAction()} if that
    is the case.

    Note that this class inherits most of its functionality from
    QDropEvent.

    \sa QDragEnterEvent, QDragLeaveEvent, QDropEvent
*/

/*!
    \class QDragLeaveEvent
    \brief The QDragLeaveEvent class provides an event that is sent to a widget when a drag and drop action leaves it.

    \ingroup events
    \ingroup draganddrop
    \inmodule QtGui

    This event is always preceded by a QDragEnterEvent and a series
    of \l{QDragMoveEvent}s. It is not sent if a QDropEvent is sent
    instead.

    \sa QDragEnterEvent, QDragMoveEvent, QDropEvent
*/

/*!
    Constructs a QDragLeaveEvent.

    \warning Do not create a QDragLeaveEvent yourself since these
    objects rely on Qt's internal state.
*/
QDragLeaveEvent::QDragLeaveEvent()
    : QEvent(DragLeave)
{}

Q_IMPL_EVENT_COMMON(QDragLeaveEvent)

#endif // QT_CONFIG(draganddrop)

/*!
    \class QHelpEvent
    \brief The QHelpEvent class provides an event that is used to request helpful information
    about a particular point in a widget.

    \ingroup events
    \ingroup helpsystem
    \inmodule QtGui

    This event can be intercepted in applications to provide tooltips
    or "What's This?" help for custom widgets. The type() can be
    either QEvent::ToolTip or QEvent::WhatsThis.

    \sa QToolTip, QWhatsThis, QStatusTipEvent, QWhatsThisClickedEvent
*/

/*!
    Constructs a help event with the given \a type corresponding to the
    widget-relative position specified by \a pos and the global position
    specified by \a globalPos.

    \a type must be either QEvent::ToolTip or QEvent::WhatsThis.

    \sa pos(), globalPos()
*/
QHelpEvent::QHelpEvent(Type type, const QPoint &pos, const QPoint &globalPos)
    : QEvent(type), m_pos(pos), m_globalPos(globalPos)
{}

/*!
    \fn int QHelpEvent::x() const

    Same as pos().x().

    \sa y(), pos(), globalPos()
*/

/*!
    \fn int QHelpEvent::y() const

    Same as pos().y().

    \sa x(), pos(), globalPos()
*/

/*!
    \fn int QHelpEvent::globalX() const

    Same as globalPos().x().

    \sa x(), globalY(), globalPos()
*/

/*!
    \fn int QHelpEvent::globalY() const

    Same as globalPos().y().

    \sa y(), globalX(), globalPos()
*/

/*!
    \fn const QPoint &QHelpEvent::pos()  const

    Returns the mouse cursor position when the event was generated,
    relative to the widget to which the event is dispatched.

    \sa globalPos(), x(), y()
*/

/*!
    \fn const QPoint &QHelpEvent::globalPos() const

    Returns the mouse cursor position when the event was generated
    in global coordinates.

    \sa pos(), globalX(), globalY()
*/

Q_IMPL_EVENT_COMMON(QHelpEvent)

#ifndef QT_NO_STATUSTIP

/*!
    \class QStatusTipEvent
    \brief The QStatusTipEvent class provides an event that is used to show messages in a status bar.

    \ingroup events
    \ingroup helpsystem
    \inmodule QtGui

    Status tips can be set on a widget using the
    QWidget::setStatusTip() function.  They are shown in the status
    bar when the mouse cursor enters the widget. For example:

    \table 100%
    \row
    \li
    \snippet qstatustipevent/main.cpp 1
    \dots
    \snippet qstatustipevent/main.cpp 3
    \li
    \image qstatustipevent-widget.png Widget with status tip.
    \endtable

    Status tips can also be set on actions using the
    QAction::setStatusTip() function:

    \table 100%
    \row
    \li
    \snippet qstatustipevent/main.cpp 0
    \snippet qstatustipevent/main.cpp 2
    \dots
    \snippet qstatustipevent/main.cpp 3
    \li
    \image qstatustipevent-action.png Action with status tip.
    \endtable

    Finally, status tips are supported for the item view classes
    through the Qt::StatusTipRole enum value.

    \sa QStatusBar, QHelpEvent, QWhatsThisClickedEvent
*/

/*!
    Constructs a status tip event with the text specified by \a tip.

    \sa tip()
*/
QStatusTipEvent::QStatusTipEvent(const QString &tip)
    : QEvent(StatusTip), m_tip(tip)
{}

Q_IMPL_EVENT_COMMON(QStatusTipEvent)

/*!
    \fn QString QStatusTipEvent::tip() const

    Returns the message to show in the status bar.

    \sa QStatusBar::showMessage()
*/

#endif // QT_NO_STATUSTIP

#if QT_CONFIG(whatsthis)

/*!
    \class QWhatsThisClickedEvent
    \brief The QWhatsThisClickedEvent class provides an event that
    can be used to handle hyperlinks in a "What's This?" text.

    \ingroup events
    \ingroup helpsystem
    \inmodule QtGui

    \sa QWhatsThis, QHelpEvent, QStatusTipEvent
*/

/*!
    Constructs an event containing a URL specified by \a href when a link
    is clicked in a "What's This?" message.

    \sa href()
*/
QWhatsThisClickedEvent::QWhatsThisClickedEvent(const QString &href)
    : QEvent(WhatsThisClicked), m_href(href)
{}

Q_IMPL_EVENT_COMMON(QWhatsThisClickedEvent)

/*!
    \fn QString QWhatsThisClickedEvent::href() const

    Returns the URL that was clicked by the user in the "What's
    This?" text.
*/

#endif // QT_CONFIG(whatsthis)

#ifndef QT_NO_ACTION

/*!
    \class QActionEvent
    \brief The QActionEvent class provides an event that is generated
    when a QAction is added, removed, or changed.

    \ingroup events
    \inmodule QtGui

    Actions can be added to controls, for example by using QWidget::addAction().
    This generates an \l ActionAdded event, which you can handle to provide
    custom behavior. For example, QToolBar reimplements
    QWidget::actionEvent() to create \l{QToolButton}s for the
    actions.

    \sa QAction, QWidget::addAction(), QWidget::removeAction(), QWidget::actions()
*/

/*!
    Constructs an action event. The \a type can be \l ActionChanged,
    \l ActionAdded, or \l ActionRemoved.

    \a action is the action that is changed, added, or removed. If \a
    type is ActionAdded, the action is to be inserted before the
    action \a before. If \a before is \nullptr, the action is appended.
*/
QActionEvent::QActionEvent(int type, QAction *action, QAction *before)
    : QEvent(static_cast<QEvent::Type>(type)), m_action(action), m_before(before)
{}

Q_IMPL_EVENT_COMMON(QActionEvent)

/*!
    \fn QAction *QActionEvent::action() const

    Returns the action that is changed, added, or removed.

    \sa before()
*/

/*!
    \fn QAction *QActionEvent::before() const

    If type() is \l ActionAdded, returns the action that should
    appear before action(). If this function returns \nullptr, the action
    should be appended to already existing actions on the same
    widget.

    \sa action(), QWidget::actions()
*/

#endif // QT_NO_ACTION

/*!
    \class QHideEvent
    \brief The QHideEvent class provides an event which is sent after a widget is hidden.

    \ingroup events
    \inmodule QtGui

    This event is sent just before QWidget::hide() returns, and also
    when a top-level window has been hidden (iconified) by the user.

    If spontaneous() is true, the event originated outside the
    application. In this case, the user hid the window using the
    window manager controls, either by iconifying the window or by
    switching to another virtual desktop where the window is not
    visible. The window will become hidden but not withdrawn. If the
    window was iconified, QWidget::isMinimized() returns \c true.

    \sa QShowEvent
*/

/*!
    Constructs a QHideEvent.
*/
QHideEvent::QHideEvent()
    : QEvent(Hide)
{}

Q_IMPL_EVENT_COMMON(QHideEvent)

/*!
    \class QShowEvent
    \brief The QShowEvent class provides an event that is sent when a widget is shown.

    \ingroup events
    \inmodule QtGui

    There are two kinds of show events: show events caused by the
    window system (spontaneous), and internal show events. Spontaneous (QEvent::spontaneous())
    show events are sent just after the window system shows the
    window; they are also sent when a top-level window is redisplayed
    after being iconified. Internal show events are delivered just
    before the widget becomes visible.

    \sa QHideEvent
*/

/*!
    Constructs a QShowEvent.
*/
QShowEvent::QShowEvent()
    : QEvent(Show)
{}

Q_IMPL_EVENT_COMMON(QShowEvent)

/*!
    \class QFileOpenEvent
    \brief The QFileOpenEvent class provides an event that will be
    sent when there is a request to open a file or a URL.

    \ingroup events
    \inmodule QtGui

    File open events will be sent to the QApplication::instance()
    when the operating system requests that a file or URL should be opened.
    This is a high-level event that can be caused by different user actions
    depending on the user's desktop environment; for example, double
    clicking on an file icon in the Finder on \macos.

    This event is only used to notify the application of a request.
    It may be safely ignored.

    \note This class is currently supported for \macos only.

    \section1 \macos Example

    In order to trigger the event on \macos, the application must be configured
    to let the OS know what kind of file(s) it should react on.

    For example, the following \c Info.plist file declares that the application
    can act as a viewer for files with a PNG extension:

    \snippet qfileopenevent/Info.plist Custom Info.plist

    The following implementation of a QApplication subclass shows how to handle
    QFileOpenEvent to open the file that was, for example, dropped on the Dock
    icon of the application.

    \snippet qfileopenevent/main.cpp QApplication subclass

    Note how \c{QFileOpenEvent::file()} is not guaranteed to be the name of a
    local file that can be opened using QFile. The contents of the string depend
    on the source application.
*/

/*!
    \internal

    Constructs a file open event for the given \a file.
*/
QFileOpenEvent::QFileOpenEvent(const QString &file)
    : QEvent(FileOpen), m_file(file), m_url(QUrl::fromLocalFile(file))
{
}

/*!
    \internal

    Constructs a file open event for the given \a url.
*/
QFileOpenEvent::QFileOpenEvent(const QUrl &url)
    : QEvent(FileOpen), m_file(url.toLocalFile()), m_url(url)
{
}

Q_IMPL_EVENT_COMMON(QFileOpenEvent)

/*!
    \fn QString QFileOpenEvent::file() const

    Returns the name of the file that the application should open.

    This is not guaranteed to be the path to a local file.
*/

/*!
    \fn QUrl QFileOpenEvent::url() const

    Returns the url that the application should open.

    \since 4.6
*/

#if QT_DEPRECATED_SINCE(6, 6)
/*!
    \fn bool QFileOpenEvent::openFile(QFile &file, QIODevice::OpenMode flags) const
    \deprecated [6.6] interpret the string returned by file()

    Opens a QFile on the \a file referenced by this event in the mode specified
    by \a flags. Returns \c true if successful; otherwise returns \c false.

    This is necessary as some files cannot be opened by name, but require specific
    information stored in this event.

    \since 4.8
*/
bool QFileOpenEvent::openFile(QFile &file, QIODevice::OpenMode flags) const
{
    file.setFileName(m_file);
    return file.open(flags);
}
#endif

#ifndef QT_NO_TOOLBAR
/*!
    \internal
    \class QToolBarChangeEvent
    \brief The QToolBarChangeEvent class provides an event that is
    sent whenever a the toolbar button is clicked on \macos.

    \ingroup events
    \inmodule QtGui

    The QToolBarChangeEvent is sent when the toolbar button is clicked. On
    \macos, this is the long oblong button on the right side of the window
    title bar. The default implementation is to toggle the appearance (hidden or
    shown) of the associated toolbars for the window.
*/

/*!
    \internal

    Construct a QToolBarChangeEvent given the current button state in \a state.
*/
QToolBarChangeEvent::QToolBarChangeEvent(bool t)
    : QEvent(ToolBarChange), m_toggle(t)
{}

Q_IMPL_EVENT_COMMON(QToolBarChangeEvent)

/*!
    \fn bool QToolBarChangeEvent::toggle() const
    \internal
*/

/*
    \fn Qt::ButtonState QToolBarChangeEvent::state() const

    Returns the keyboard modifier flags at the time of the event.

    The returned value is a selection of the following values,
    combined using the OR operator:
    Qt::ShiftButton, Qt::ControlButton, Qt::MetaButton, and Qt::AltButton.
*/

#endif // QT_NO_TOOLBAR

#if QT_CONFIG(shortcut)

/*!
    Constructs a shortcut event for the given \a key press,
    associated with the QShortcut ID \a id.

    \deprecated use the other constructor

    \a ambiguous specifies whether there is more than one QShortcut
    for the same key sequence.
*/
QShortcutEvent::QShortcutEvent(const QKeySequence &key, int id, bool ambiguous)
    : QEvent(Shortcut), m_sequence(key), m_shortcutId(id), m_ambiguous(ambiguous)
{
}

/*!
    Constructs a shortcut event for the given \a key press,
    associated with the QShortcut \a shortcut.
    \since 6.5

    \a ambiguous specifies whether there is more than one QShortcut
    for the same key sequence.
*/
QShortcutEvent::QShortcutEvent(const QKeySequence &key, const QShortcut *shortcut, bool ambiguous)
    : QEvent(Shortcut), m_sequence(key), m_shortcutId(0), m_ambiguous(ambiguous)
{
    if (shortcut) {
        auto priv = static_cast<const QShortcutPrivate *>(QShortcutPrivate::get(shortcut));
        auto index = priv->sc_sequences.indexOf(key);
        if (index < 0) {
            qWarning() << "Given QShortcut does not contain key-sequence " << key;
            return;
        }
        m_shortcutId = priv->sc_ids[index];
    }
}

Q_IMPL_EVENT_COMMON(QShortcutEvent)

#endif // QT_CONFIG(shortcut)

#ifndef QT_NO_DEBUG_STREAM

static inline void formatTouchEvent(QDebug d, const QTouchEvent &t)
{
    d << "QTouchEvent(";
    QtDebugUtils::formatQEnum(d, t.type());
    d << " device: " << t.device()->name();
    d << " states: ";
    QtDebugUtils::formatQFlags(d, t.touchPointStates());
    d << ", " << t.points().size() << " points: " << t.points() << ')';
}

static void formatUnicodeString(QDebug d, const QString &s)
{
    d << '"' << Qt::hex;
    for (int i = 0; i < s.size(); ++i) {
        if (i)
            d << ',';
        d << "U+" << s.at(i).unicode();
    }
    d << Qt::dec << '"';
}

static QDebug operator<<(QDebug dbg, const QInputMethodEvent::Attribute &attr)
{
    dbg << "[type= " << attr.type << ", start=" << attr.start << ", length=" << attr.length
        << ", value=" << attr.value << ']';
    return dbg;
}

static inline void formatInputMethodEvent(QDebug d, const QInputMethodEvent *e)
{
    d << "QInputMethodEvent(";
    if (!e->preeditString().isEmpty()) {
        d << "preedit=";
        formatUnicodeString(d, e->preeditString());
    }
    if (!e->commitString().isEmpty()) {
        d << ", commit=";
        formatUnicodeString(d, e->commitString());
    }
    if (e->replacementLength()) {
        d << ", replacementStart=" << e->replacementStart() << ", replacementLength="
          << e->replacementLength();
    }
    const auto attributes = e->attributes();
    auto it = attributes.cbegin();
    const auto end = attributes.cend();
    if (it != end) {
        d << ", attributes= {";
        d << *it;
        ++it;
        for (; it != end; ++it)
            d << ',' << *it;
        d << '}';
    }
    d << ')';
}

static inline void formatInputMethodQueryEvent(QDebug d, const QInputMethodQueryEvent *e)
{
    QDebugStateSaver saver(d);
    d.noquote();
    const Qt::InputMethodQueries queries = e->queries();
    d << "QInputMethodQueryEvent(queries=" << Qt::showbase << Qt::hex << int(queries)
      << Qt::noshowbase << Qt::dec << ", {";
    for (unsigned mask = 1; mask <= Qt::ImInputItemClipRectangle; mask<<=1) {
        if (queries & mask) {
            const Qt::InputMethodQuery query = static_cast<Qt::InputMethodQuery>(mask);
            const QVariant value = e->value(query);
            if (value.isValid()) {
                d << '[';
                QtDebugUtils::formatQEnum(d, query);
                d << '=';
                if (query == Qt::ImHints)
                    QtDebugUtils::formatQFlags(d, Qt::InputMethodHints(value.toInt()));
                else
                    d << value.toString();
                d << "],";
            }
        }
    }
    d << "})";
}

static const char *eventClassName(QEvent::Type t)
{
    switch (t) {
    case QEvent::ActionAdded:
    case QEvent::ActionRemoved:
    case QEvent::ActionChanged:
        return "QActionEvent";
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::NonClientAreaMouseMove:
    case QEvent::NonClientAreaMouseButtonPress:
    case QEvent::NonClientAreaMouseButtonRelease:
    case QEvent::NonClientAreaMouseButtonDblClick:
        return "QMouseEvent";
    case QEvent::DragEnter:
        return "QDragEnterEvent";
    case QEvent::DragMove:
        return "QDragMoveEvent";
    case QEvent::Drop:
        return "QDropEvent";
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
        return "QKeyEvent";
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::FocusAboutToChange:
        return "QFocusEvent";
    case QEvent::ChildAdded:
    case QEvent::ChildPolished:
    case QEvent::ChildRemoved:
        return "QChildEvent";
    case QEvent::Paint:
        return "QPaintEvent";
    case QEvent::Move:
        return "QMoveEvent";
    case QEvent::Resize:
        return "QResizeEvent";
    case QEvent::Show:
        return "QShowEvent";
    case QEvent::Hide:
        return "QHideEvent";
    case QEvent::Enter:
        return "QEnterEvent";
    case QEvent::Close:
        return "QCloseEvent";
    case QEvent::FileOpen:
        return "QFileOpenEvent";
#ifndef QT_NO_GESTURES
    case QEvent::NativeGesture:
        return "QNativeGestureEvent";
    case QEvent::Gesture:
    case QEvent::GestureOverride:
        return "QGestureEvent";
#endif
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        return "QHoverEvent";
    case QEvent::TabletEnterProximity:
    case QEvent::TabletLeaveProximity:
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
        return "QTabletEvent";
    case QEvent::StatusTip:
        return "QStatusTipEvent";
    case QEvent::ToolTip:
        return "QHelpEvent";
    case QEvent::WindowStateChange:
        return "QWindowStateChangeEvent";
    case QEvent::Wheel:
        return "QWheelEvent";
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        return "QTouchEvent";
    case QEvent::Shortcut:
        return "QShortcutEvent";
    case QEvent::InputMethod:
        return "QInputMethodEvent";
    case QEvent::InputMethodQuery:
        return "QInputMethodQueryEvent";
    case QEvent::OrientationChange:
        return "QScreenOrientationChangeEvent";
    case QEvent::ScrollPrepare:
        return "QScrollPrepareEvent";
    case QEvent::Scroll:
        return "QScrollEvent";
    case QEvent::GraphicsSceneMouseMove:
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseRelease:
    case QEvent::GraphicsSceneMouseDoubleClick:
        return "QGraphicsSceneMouseEvent";
    case QEvent::GraphicsSceneContextMenu:
    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHoverMove:
    case QEvent::GraphicsSceneHoverLeave:
    case QEvent::GraphicsSceneHelp:
    case QEvent::GraphicsSceneDragEnter:
    case QEvent::GraphicsSceneDragMove:
    case QEvent::GraphicsSceneDragLeave:
    case QEvent::GraphicsSceneDrop:
    case QEvent::GraphicsSceneWheel:
        return "QGraphicsSceneEvent";
    case QEvent::Timer:
        return "QTimerEvent";
    case QEvent::PlatformSurface:
        return "QPlatformSurfaceEvent";
    default:
        break;
    }
    return "QEvent";
}

#  if QT_CONFIG(draganddrop)

static void formatDropEvent(QDebug d, const QDropEvent *e)
{
    const QEvent::Type type = e->type();
    d << eventClassName(type) << "(dropAction=";
    QtDebugUtils::formatQEnum(d, e->dropAction());
    d << ", proposedAction=";
    QtDebugUtils::formatQEnum(d, e->proposedAction());
    d << ", possibleActions=";
    QtDebugUtils::formatQFlags(d, e->possibleActions());
    d << ", posF=";
    QtDebugUtils::formatQPoint(d,  e->position());
    if (type == QEvent::DragMove || type == QEvent::DragEnter)
        d << ", answerRect=" << static_cast<const QDragMoveEvent *>(e)->answerRect();
    d << ", formats=" << e->mimeData()->formats();
    QtDebugUtils::formatNonNullQFlags(d, ", keyboardModifiers=", e->modifiers());
    d << ", ";
    QtDebugUtils::formatQFlags(d, e->buttons());
}

#  endif // QT_CONFIG(draganddrop)

#  if QT_CONFIG(tabletevent)

static void formatTabletEvent(QDebug d, const QTabletEvent *e)
{
    const QEvent::Type type = e->type();

    d << eventClassName(type)  << '(';
    QtDebugUtils::formatQEnum(d, type);
    d << ' ';
    QtDebugUtils::formatQFlags(d, e->buttons());
    d << " pos=";
    QtDebugUtils::formatQPoint(d,  e->position());
    d << " z=" << e->z()
      << " xTilt=" << e->xTilt()
      << " yTilt=" << e->yTilt();
    if (type == QEvent::TabletPress || type == QEvent::TabletMove)
        d << " pressure=" << e->pressure();
    if (e->device()->hasCapability(QInputDevice::Capability::Rotation))
        d << " rotation=" << e->rotation();
    if (e->deviceType() == QInputDevice::DeviceType::Airbrush)
        d << " tangentialPressure=" << e->tangentialPressure();
    d << " dev=" << e->device() << ')';
}

#  endif // QT_CONFIG(tabletevent)

QDebug operator<<(QDebug dbg, const QEventPoint *tp)
{
    if (!tp) {
        dbg << "QEventPoint(0x0)";
        return dbg;
    }
    return operator<<(dbg, *tp);
}

QDebug operator<<(QDebug dbg, const QEventPoint &tp)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QEventPoint(id=" << tp.id() << " ts=" << tp.timestamp();
    dbg << " pos=";
    QtDebugUtils::formatQPoint(dbg, tp.position());
    dbg << " scn=";
    QtDebugUtils::formatQPoint(dbg, tp.scenePosition());
    dbg << " gbl=";
    QtDebugUtils::formatQPoint(dbg, tp.globalPosition());
    dbg << ' ';
    QtDebugUtils::formatQEnum(dbg, tp.state());
    if (!qFuzzyIsNull(tp.pressure()) && !qFuzzyCompare(tp.pressure(), 1))
        dbg << " pressure=" << tp.pressure();
    if (!tp.ellipseDiameters().isEmpty() || !qFuzzyIsNull(tp.rotation())) {
        dbg << " ellipse=("
            << tp.ellipseDiameters().width() << "x" << tp.ellipseDiameters().height()
            << " \u2221 " << tp.rotation() << ')';
    }
    dbg << " vel=";
    QtDebugUtils::formatQPoint(dbg, tp.velocity().toPointF());
    dbg << " press=";
    QtDebugUtils::formatQPoint(dbg, tp.pressPosition());
    dbg << " last=";
    QtDebugUtils::formatQPoint(dbg, tp.lastPosition());
    dbg << " \u0394 ";
    QtDebugUtils::formatQPoint(dbg, tp.position() - tp.lastPosition());
    dbg << ')';
    return dbg;
}

QDebug operator<<(QDebug dbg, const QEvent *e)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    if (!e) {
        dbg << "QEvent(this = 0x0)";
        return dbg;
    }
    // More useful event output could be added here
    const QEvent::Type type = e->type();
    bool isMouse = false;
    switch (type) {
    case QEvent::Expose:
        dbg << "QExposeEvent()";
        break;
    case QEvent::Paint:
        dbg << "QPaintEvent(" << static_cast<const QPaintEvent *>(e)->region() << ')';
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::NonClientAreaMouseButtonPress:
    case QEvent::NonClientAreaMouseMove:
    case QEvent::NonClientAreaMouseButtonRelease:
    case QEvent::NonClientAreaMouseButtonDblClick:
        isMouse = true;
        Q_FALLTHROUGH();
    case QEvent::HoverEnter:
    case QEvent::HoverMove:
    case QEvent::HoverLeave:
    {
        const QSinglePointEvent *spe = static_cast<const QSinglePointEvent*>(e);
        const Qt::MouseButton button = spe->button();
        const Qt::MouseButtons buttons = spe->buttons();
        dbg << eventClassName(type) << '(';
        QtDebugUtils::formatQEnum(dbg, type);
        if (isMouse) {
            if (type != QEvent::MouseMove && type != QEvent::NonClientAreaMouseMove) {
                dbg << ' ';
                QtDebugUtils::formatQEnum(dbg, button);
            }
            if (buttons && button != buttons) {
                dbg << " btns=";
                QtDebugUtils::formatQFlags(dbg, buttons);
            }
        }
        QtDebugUtils::formatNonNullQFlags(dbg, ", ", spe->modifiers());
        dbg << " pos=";
        QtDebugUtils::formatQPoint(dbg, spe->position());
        dbg << " scn=";
        QtDebugUtils::formatQPoint(dbg, spe->scenePosition());
        dbg << " gbl=";
        QtDebugUtils::formatQPoint(dbg, spe->globalPosition());
        dbg << " dev=" << spe->device() << ')';
        if (isMouse) {
            auto src = static_cast<const QMouseEvent*>(e)->source();
            if (src != Qt::MouseEventNotSynthesized) {
                dbg << " source=";
                QtDebugUtils::formatQEnum(dbg, src);
            }
        }
    }
        break;
#  if QT_CONFIG(wheelevent)
    case QEvent::Wheel: {
        const QWheelEvent *we = static_cast<const QWheelEvent *>(e);
        dbg << "QWheelEvent(" << we->phase();
        if (!we->pixelDelta().isNull() || !we->angleDelta().isNull())
            dbg << ", pixelDelta=" << we->pixelDelta() << ", angleDelta=" << we->angleDelta();
        dbg << ')';
    }
        break;
#  endif // QT_CONFIG(wheelevent)
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
    {
        const QKeyEvent *ke = static_cast<const QKeyEvent *>(e);
        dbg << "QKeyEvent(";
        QtDebugUtils::formatQEnum(dbg, type);
        dbg << ", ";
        QtDebugUtils::formatQEnum(dbg, static_cast<Qt::Key>(ke->key()));
        QtDebugUtils::formatNonNullQFlags(dbg, ", ", ke->modifiers());
        if (!ke->text().isEmpty())
            dbg << ", text=" << ke->text();
        if (ke->isAutoRepeat())
            dbg << ", autorepeat, count=" << ke->count();
        dbg << ')';
    }
        break;
#if QT_CONFIG(shortcut)
    case QEvent::Shortcut: {
        const QShortcutEvent *se = static_cast<const QShortcutEvent *>(e);
        dbg << "QShortcutEvent(" << se->key().toString() << ", id=" << se->shortcutId();
        if (se->isAmbiguous())
            dbg << ", ambiguous";
        dbg << ')';
    }
        break;
#endif
    case QEvent::FocusAboutToChange:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        dbg << "QFocusEvent(";
        QtDebugUtils::formatQEnum(dbg, type);
        dbg << ", ";
        QtDebugUtils::formatQEnum(dbg, static_cast<const QFocusEvent *>(e)->reason());
        dbg << ')';
        break;
    case QEvent::Move: {
        const QMoveEvent *me = static_cast<const QMoveEvent *>(e);
        dbg << "QMoveEvent(";
        QtDebugUtils::formatQPoint(dbg, me->pos());
        if (!me->spontaneous())
            dbg << ", non-spontaneous";
        dbg << ')';
    }
         break;
    case QEvent::Resize: {
        const QResizeEvent *re = static_cast<const QResizeEvent *>(e);
        dbg << "QResizeEvent(";
        QtDebugUtils::formatQSize(dbg, re->size());
        if (!re->spontaneous())
            dbg << ", non-spontaneous";
        dbg << ')';
    }
        break;
#  if QT_CONFIG(draganddrop)
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::Drop:
        formatDropEvent(dbg, static_cast<const QDropEvent *>(e));
        break;
#  endif // QT_CONFIG(draganddrop)
    case QEvent::InputMethod:
        formatInputMethodEvent(dbg, static_cast<const QInputMethodEvent *>(e));
        break;
    case QEvent::InputMethodQuery:
        formatInputMethodQueryEvent(dbg, static_cast<const QInputMethodQueryEvent *>(e));
        break;
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        formatTouchEvent(dbg, *static_cast<const QTouchEvent*>(e));
        break;
    case QEvent::ChildAdded:
    case QEvent::ChildPolished:
    case QEvent::ChildRemoved:
        dbg << "QChildEvent(";
        QtDebugUtils::formatQEnum(dbg, type);
        dbg << ", " << (static_cast<const QChildEvent*>(e))->child() << ')';
        break;
#  ifndef QT_NO_GESTURES
    case QEvent::NativeGesture: {
        const QNativeGestureEvent *ne = static_cast<const QNativeGestureEvent *>(e);
        dbg << "QNativeGestureEvent(";
        QtDebugUtils::formatQEnum(dbg, ne->gestureType());
        dbg << ", fingerCount=" << ne->fingerCount() << ", localPos=";
        QtDebugUtils::formatQPoint(dbg, ne->position());
        if (!qIsNull(ne->value()))
            dbg << ", value=" << ne->value();
        if (!ne->delta().isNull()) {
            dbg << ", delta=";
            QtDebugUtils::formatQPoint(dbg, ne->delta());
        }
        dbg << ')';
    }
         break;
#  endif // !QT_NO_GESTURES
    case QEvent::ApplicationStateChange:
        dbg << "QApplicationStateChangeEvent(";
        QtDebugUtils::formatQEnum(dbg, static_cast<const QApplicationStateChangeEvent *>(e)->applicationState());
        dbg << ')';
        break;
#  ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        dbg << "QContextMenuEvent(" << static_cast<const QContextMenuEvent *>(e)->pos() << ')';
        break;
#  endif // !QT_NO_CONTEXTMENU
#  if QT_CONFIG(tabletevent)
    case QEvent::TabletEnterProximity:
    case QEvent::TabletLeaveProximity:
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
        formatTabletEvent(dbg, static_cast<const QTabletEvent *>(e));
        break;
#  endif // QT_CONFIG(tabletevent)
    case QEvent::Enter:
        dbg << "QEnterEvent(" << static_cast<const QEnterEvent *>(e)->position() << ')';
        break;
    case QEvent::Timer:
        dbg << "QTimerEvent(id=" << static_cast<const QTimerEvent *>(e)->timerId() << ')';
        break;
    case QEvent::PlatformSurface:
        dbg << "QPlatformSurfaceEvent(surfaceEventType=";
        switch (static_cast<const QPlatformSurfaceEvent *>(e)->surfaceEventType()) {
        case QPlatformSurfaceEvent::SurfaceCreated:
            dbg << "SurfaceCreated";
            break;
        case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
            dbg << "SurfaceAboutToBeDestroyed";
            break;
        }
        dbg << ')';
        break;
    case QEvent::ScrollPrepare: {
        const QScrollPrepareEvent *se = static_cast<const QScrollPrepareEvent *>(e);
        dbg << "QScrollPrepareEvent(viewportSize=" << se->viewportSize()
            << ", contentPosRange=" << se->contentPosRange()
            << ", contentPos=" << se->contentPos() << ')';
    }
        break;
    case QEvent::Scroll: {
        const QScrollEvent *se = static_cast<const QScrollEvent *>(e);
        dbg << "QScrollEvent(contentPos=" << se->contentPos()
            << ", overshootDistance=" << se->overshootDistance()
            << ", scrollState=" << se->scrollState() << ')';
    }
        break;
    default:
        dbg << eventClassName(type) << '(';
        QtDebugUtils::formatQEnum(dbg, type);
        dbg << ", " << (const void *)e << ')';
        break;
    }
    return dbg;
}
#endif // !QT_NO_DEBUG_STREAM

/*!
    \class QShortcutEvent
    \brief The QShortcutEvent class provides an event which is generated when
    the user presses a key combination.

    \ingroup events
    \inmodule QtGui

    Normally you do not need to use this class directly; QShortcut
    provides a higher-level interface to handle shortcut keys.

    \sa QShortcut
*/

/*!
    \fn const QKeySequence &QShortcutEvent::key() const

    Returns the key sequence that triggered the event.
*/

/*!
    \fn int QShortcutEvent::shortcutId() const

    \deprecated

    Returns the ID of the QShortcut object for which this event was
    generated.

    \sa QShortcut::id()
*/

/*!
    \fn bool QShortcutEvent::isAmbiguous() const

    Returns \c true if the key sequence that triggered the event is
    ambiguous.

    \sa QShortcut::activatedAmbiguously()
*/

/*!
    \class QWindowStateChangeEvent
    \ingroup events
    \inmodule QtGui

    \brief The QWindowStateChangeEvent class provides the window state before a
    window state change.
*/

/*! \fn Qt::WindowStates QWindowStateChangeEvent::oldState() const

    Returns the state of the window before the change.
*/

/*! \internal
 */
QWindowStateChangeEvent::QWindowStateChangeEvent(Qt::WindowStates oldState, bool isOverride)
    : QEvent(WindowStateChange), m_oldStates(oldState), m_override(isOverride)
{
}

/*! \internal
 */
bool QWindowStateChangeEvent::isOverride() const
{
    return m_override;
}

Q_IMPL_EVENT_COMMON(QWindowStateChangeEvent)


/*!
    \class QTouchEvent
    \brief The QTouchEvent class contains parameters that describe a touch event.
    \since 4.6
    \ingroup events
    \ingroup touch
    \inmodule QtGui

    \section1 Enabling Touch Events

    Touch events occur when pressing, releasing, or moving one or more touch points on a touch
    device (such as a touch-screen or track-pad). To receive touch events, widgets have to have the
    Qt::WA_AcceptTouchEvents attribute set and graphics items need to have the
    \l{QGraphicsItem::setAcceptTouchEvents()}{acceptTouchEvents} attribute set to true.

    When using QAbstractScrollArea based widgets, you should enable the Qt::WA_AcceptTouchEvents
    attribute on the scroll area's \l{QAbstractScrollArea::viewport()}{viewport}.

    Similarly to QMouseEvent, Qt automatically grabs each touch point on the first press inside a
    widget, and the widget will receive all updates for the touch point until it is released.
    Note that it is possible for a widget to receive events for numerous touch points, and that
    multiple widgets may be receiving touch events at the same time.

    \section1 Event Handling

    All touch events are of type QEvent::TouchBegin, QEvent::TouchUpdate, QEvent::TouchEnd or
    QEvent::TouchCancel. Reimplement QWidget::event() or QAbstractScrollArea::viewportEvent() for
    widgets and QGraphicsItem::sceneEvent() for items in a graphics view to receive touch events.

    Unlike widgets, QWindows receive touch events always, there is no need to opt in. When working
    directly with a QWindow, it is enough to reimplement QWindow::touchEvent().

    The QEvent::TouchUpdate and QEvent::TouchEnd events are sent to the widget or item that
    accepted the QEvent::TouchBegin event. If the QEvent::TouchBegin event is not accepted and not
    filtered by an event filter, then no further touch events are sent until the next
    QEvent::TouchBegin.

    Some systems may send an event of type QEvent::TouchCancel. Upon receiving this event
    applications are requested to ignore the entire active touch sequence. For example in a
    composited system the compositor may decide to treat certain gestures as system-wide
    gestures. Whenever such a decision is made (the gesture is recognized), the clients will be
    notified with a QEvent::TouchCancel event so they can update their state accordingly.

    The pointCount() and point() functions can be used to access and iterate individual
    touch points.

    The points() function returns a list of all touch points contained in the event.
    Note that this list may be empty, for example in case of a QEvent::TouchCancel event.
    Each point is an instance of the QEventPoint class. The QEventPoint::State enum
    describes the different states that a touch point may have.

    \note The list of points() will never be partial: A touch event will always contain a touch
    point for each existing physical touch contacts targeting the window or widget to which the
    event is sent. For instance, assuming that all touches target the same window or widget, an
    event with a condition of points().count()==2 is guaranteed to imply that the number of
    fingers touching the touchscreen or touchpad is exactly two.

    \section1 Event Delivery and Propagation

    By default, QGuiApplication translates the first touch point in a QTouchEvent into
    a QMouseEvent. This makes it possible to enable touch events on existing widgets that do not
    normally handle QTouchEvent. See below for information on some special considerations needed
    when doing this.

    QEvent::TouchBegin is the first touch event sent to a widget. The QEvent::TouchBegin event
    contains a special accept flag that indicates whether the receiver wants the event. By default,
    the event is accepted. You should call ignore() if the touch event is not handled by your
    widget. The QEvent::TouchBegin event is propagated up the parent widget chain until a widget
    accepts it with accept(), or an event filter consumes it. For QGraphicsItems, the
    QEvent::TouchBegin event is propagated to items under the mouse (similar to mouse event
    propagation for QGraphicsItems).

    \section1 Touch Point Grouping

    As mentioned above, it is possible that several widgets can be receiving QTouchEvents at the
    same time. However, Qt makes sure to never send duplicate QEvent::TouchBegin events to the same
    widget, which could theoretically happen during propagation if, for example, the user touched 2
    separate widgets in a QGroupBox and both widgets ignored the QEvent::TouchBegin event.

    To avoid this, Qt will group new touch points together using the following rules:

    \list

    \li When the first touch point is detected, the destination widget is determined firstly by the
    location on screen and secondly by the propagation rules.

    \li When additional touch points are detected, Qt first looks to see if there are any active
    touch points on any ancestor or descendent of the widget under the new touch point. If there
    are, the new touch point is grouped with the first, and the new touch point will be sent in a
    single QTouchEvent to the widget that handled the first touch point. (The widget under the new
    touch point will not receive an event).

    \endlist

    This makes it possible for sibling widgets to handle touch events independently while making
    sure that the sequence of QTouchEvents is always correct.

    \section1 Mouse Events and Touch Event Synthesizing

    QTouchEvent delivery is independent from that of QMouseEvent. The application flags
    Qt::AA_SynthesizeTouchForUnhandledMouseEvents and Qt::AA_SynthesizeMouseForUnhandledTouchEvents
    can be used to enable or disable automatic synthesizing of touch events to mouse events and
    mouse events to touch events.

    \section1 Caveats

    \list

    \li As mentioned above, enabling touch events means multiple widgets can be receiving touch
    events simultaneously. Combined with the default QWidget::event() handling for QTouchEvents,
    this gives you great flexibility in designing touch user interfaces. Be aware of the
    implications. For example, it is possible that the user is moving a QSlider with one finger and
    pressing a QPushButton with another. The signals emitted by these widgets will be
    interleaved.

    \li Recursion into the event loop using one of the exec() methods (e.g., QDialog::exec() or
    QMenu::exec()) in a QTouchEvent event handler is not supported. Since there are multiple event
    recipients, recursion may cause problems, including but not limited to lost events
    and unexpected infinite recursion.

    \li QTouchEvents are not affected by a \l{QWidget::grabMouse()}{mouse grab} or an
    \l{QApplication::activePopupWidget()}{active pop-up widget}. The behavior of QTouchEvents is
    undefined when opening a pop-up or grabbing the mouse while there are more than one active touch
    points.

    \endlist

    \sa QEventPoint, QEventPoint::State, Qt::WA_AcceptTouchEvents,
    QGraphicsItem::acceptTouchEvents()
*/

/*!
    \deprecated [6.2] Use another constructor.

    Constructs a QTouchEvent with the given \a eventType, \a device,
    \a touchPoints, and current keyboard \a modifiers at the time of the event.
*/

QTouchEvent::QTouchEvent(QEvent::Type eventType,
                         const QPointingDevice *device,
                         Qt::KeyboardModifiers modifiers,
                         const QList<QEventPoint> &touchPoints)
    : QPointerEvent(eventType, device, modifiers, touchPoints),
      m_target(nullptr)
{
    for (QEventPoint &point : m_points) {
        m_touchPointStates |= point.state();
        QMutableEventPoint::setDevice(point, device);
    }
}

#if QT_DEPRECATED_SINCE(6, 0)
/*!
    \deprecated [6.0] Use another constructor.

    Constructs a QTouchEvent with the given \a eventType, \a device, and
    \a touchPoints. The \a touchPointStates and \a modifiers are the current
    touch point states and keyboard modifiers at the time of the event.
*/
QTouchEvent::QTouchEvent(QEvent::Type eventType,
                         const QPointingDevice *device,
                         Qt::KeyboardModifiers modifiers,
                         QEventPoint::States touchPointStates,
                         const QList<QEventPoint> &touchPoints)
    : QPointerEvent(eventType, device, modifiers, touchPoints),
      m_target(nullptr),
      m_touchPointStates(touchPointStates)
{
    for (QEventPoint &point : m_points)
        QMutableEventPoint::setDevice(point, device);
}
#endif // QT_DEPRECATED_SINCE(6, 0)

Q_IMPL_POINTER_EVENT(QTouchEvent)

/*!
    Returns true if this event includes at least one newly-pressed touchpoint.
*/
bool QTouchEvent::isBeginEvent() const
{
    return m_touchPointStates.testFlag(QEventPoint::State::Pressed);
}

/*!
    Returns true if this event does not include newly-pressed or newly-released
    touchpoints.
*/
bool QTouchEvent::isUpdateEvent() const
{
    return !m_touchPointStates.testFlag(QEventPoint::State::Pressed) &&
           !m_touchPointStates.testFlag(QEventPoint::State::Released);
}

/*!
    Returns true if this event includes at least one newly-released touchpoint.
*/
bool QTouchEvent::isEndEvent() const
{
    return m_touchPointStates.testFlag(QEventPoint::State::Released);
}

/*! \fn QObject *QTouchEvent::target() const

    Returns the target object within the window on which the event occurred.
    This is typically a QWidget or a QQuickItem. May be 0 when no specific target is available.
*/

/*! \fn QEventPoint::States QTouchEvent::touchPointStates() const

    Returns a bitwise OR of all the touch point states for this event.
*/

/*! \fn const QList<QEventPoint> &QTouchEvent::touchPoints() const
    \deprecated [6.0] Use points() instead.

    Returns a reference to the list of touch points contained in the touch event.

    \sa QPointerEvent::point(), QPointerEvent::pointCount()
*/

/*!
    \class QScrollPrepareEvent
    \since 4.8
    \ingroup events
    \inmodule QtGui

    \brief The QScrollPrepareEvent class is sent in preparation of scrolling.

    The scroll prepare event is sent before scrolling (usually by QScroller) is started.
    The object receiving this event should set viewportSize, maxContentPos and contentPos.
    It also should accept this event to indicate that scrolling should be started.

    It is not guaranteed that a QScrollEvent will be sent after an accepted
    QScrollPrepareEvent, e.g. in a case where the maximum content position is (0, 0).

    \sa QScrollEvent, QScroller
*/

/*!
    Creates new QScrollPrepareEvent
    The \a startPos is the position of a touch or mouse event that started the scrolling.
*/
QScrollPrepareEvent::QScrollPrepareEvent(const QPointF &startPos)
    : QEvent(QEvent::ScrollPrepare), m_startPos(startPos)
{
}

Q_IMPL_EVENT_COMMON(QScrollPrepareEvent)

/*!
    \fn QPointF QScrollPrepareEvent::startPos() const

    Returns the position of the touch or mouse event that started the scrolling.
*/

/*!
    \fn QSizeF QScrollPrepareEvent::viewportSize() const
    Returns size of the area that is to be scrolled as set by setViewportSize

    \sa setViewportSize()
*/

/*!
    \fn QRectF QScrollPrepareEvent::contentPosRange() const
    Returns the range of coordinates for the content as set by setContentPosRange().
*/

/*!
    \fn QPointF QScrollPrepareEvent::contentPos() const
    Returns the current position of the content as set by setContentPos.
*/

/*!
    Sets the size of the area that is to be scrolled to \a size.

    \sa viewportSize()
*/
void QScrollPrepareEvent::setViewportSize(const QSizeF &size)
{
    m_viewportSize = size;
}

/*!
    Sets the range of content coordinates to \a rect.

    \sa contentPosRange()
*/
void QScrollPrepareEvent::setContentPosRange(const QRectF &rect)
{
    m_contentPosRange = rect;
}

/*!
    Sets the current content position to \a pos.

    \sa contentPos()
*/
void QScrollPrepareEvent::setContentPos(const QPointF &pos)
{
    m_contentPos = pos;
}


/*!
    \class QScrollEvent
    \since 4.8
    \ingroup events
    \inmodule QtGui

    \brief The QScrollEvent class is sent when scrolling.

    The scroll event is sent to indicate that the receiver should be scrolled.
    Usually the receiver should be something visual like QWidget or QGraphicsObject.

    Some care should be taken that no conflicting QScrollEvents are sent from two
    sources. Using QScroller::scrollTo is save however.

    \sa QScrollPrepareEvent, QScroller
*/

/*!
    \enum QScrollEvent::ScrollState

    This enum describes the states a scroll event can have.

    \value ScrollStarted Set for the first scroll event of a scroll activity.

    \value ScrollUpdated Set for all but the first and the last scroll event of a scroll activity.

    \value ScrollFinished Set for the last scroll event of a scroll activity.

    \sa QScrollEvent::scrollState()
*/

/*!
    Creates a new QScrollEvent
    \a contentPos is the new content position, \a overshootDistance is the
    new overshoot distance while \a scrollState indicates if this scroll
    event is the first one, the last one or some event in between.
*/
QScrollEvent::QScrollEvent(const QPointF &contentPos, const QPointF &overshootDistance, ScrollState scrollState)
    : QEvent(QEvent::Scroll), m_contentPos(contentPos), m_overshoot(overshootDistance), m_state(scrollState)
{
}

Q_IMPL_EVENT_COMMON(QScrollEvent)

/*!
    \fn QPointF QScrollEvent::contentPos() const

    Returns the new scroll position.
*/

/*!
    \fn QPointF QScrollEvent::overshootDistance() const

    Returns the new overshoot distance.
    See QScroller for an explanation of the term overshoot.

    \sa QScroller
*/

/*!
    \fn QScrollEvent::ScrollState QScrollEvent::scrollState() const

    Returns the current scroll state as a combination of ScrollStateFlag values.
    ScrollStarted (or ScrollFinished) will be set, if this scroll event is the first (or last) event in a scrolling activity.
    Please note that both values can be set at the same time, if the activity consists of a single QScrollEvent.
    All other scroll events in between will have their state set to ScrollUpdated.

    A widget could for example revert selections when scrolling is started and stopped.
*/

/*!
    Creates a new QScreenOrientationChangeEvent
    \a screenOrientation is the new orientation of the \a screen.
*/
QScreenOrientationChangeEvent::QScreenOrientationChangeEvent(QScreen *screen, Qt::ScreenOrientation screenOrientation)
    : QEvent(QEvent::OrientationChange), m_screen(screen), m_orientation(screenOrientation)
{
}

Q_IMPL_EVENT_COMMON(QScreenOrientationChangeEvent)

/*!
    \fn QScreen *QScreenOrientationChangeEvent::screen() const

    Returns the screen whose orientation changed.
*/

/*!
    \fn Qt::ScreenOrientation QScreenOrientationChangeEvent::orientation() const

    Returns the orientation of the screen.
*/

/*!
    Creates a new QApplicationStateChangeEvent.
    \a applicationState is the new state.
*/
QApplicationStateChangeEvent::QApplicationStateChangeEvent(Qt::ApplicationState applicationState)
    : QEvent(QEvent::ApplicationStateChange), m_applicationState(applicationState)
{
}

Q_IMPL_EVENT_COMMON(QApplicationStateChangeEvent)

/*!
    \fn Qt::ApplicationState QApplicationStateChangeEvent::applicationState() const

    Returns the state of the application.
*/

QMutableTouchEvent::~QMutableTouchEvent()
    = default;

/*! \internal
    Add the given \a point.
*/
void QMutableTouchEvent::addPoint(const QEventPoint &point)
{
    m_points.append(point);
    auto &added = m_points.last();
    if (!added.device())
        QMutableEventPoint::setDevice(added, pointingDevice());
    m_touchPointStates |= point.state();
}


QMutableSinglePointEvent::~QMutableSinglePointEvent()
    = default;

QT_END_NAMESPACE

#include "moc_qevent.cpp"
