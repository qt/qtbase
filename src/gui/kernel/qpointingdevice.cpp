// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpointingdevice.h"
#include "qpointingdevice_p.h"
#include "qwindowsysteminterface_p.h"
#include "qeventpoint_p.h"

#include <QList>
#include <QLoggingCategory>
#include <QMutex>
#include <QCoreApplication>

#include <private/qdebug_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcPointerGrab, "qt.pointer.grab");

/*!
    \class QPointingDevice
    \brief The QPointingDevice class describes a device from which mouse, touch or tablet events originate.
    \since 6.0
    \ingroup events
    \inmodule QtGui

    Each QPointerEvent contains a QPointingDevice pointer to allow accessing
    device-specific properties like type and capabilities. It is the
    responsibility of the platform or generic plug-ins to register the
    available pointing devices via QWindowSystemInterface before generating any
    pointer events. Applications do not need to instantiate this class, they
    should just access the global instances pointed to by QPointerEvent::device().
*/

/*! \enum QInputDevice::DeviceType

    This enum represents the type of device that generated a QPointerEvent.

    \value Unknown
        The device cannot be identified.

    \value Mouse
        A mouse.

    \value TouchScreen
        In this type of device, the touch surface and display are integrated.
        This means the surface and display typically have the same size, such
        that there is a direct relationship between the touch points' physical
        positions and the coordinate reported by QEventPoint. As a
        result, Qt allows the user to interact directly with multiple QWidgets,
        QGraphicsItems, or Qt Quick Items at the same time.

    \value TouchPad
        In this type of device, the touch surface is separate from the display.
        There is not a direct relationship between the physical touch location
        and the on-screen coordinates. Instead, they are calculated relative to
        the current mouse position, and the user must use the touch-pad to move
        this reference point. Unlike touch-screens, Qt allows users to only
        interact with a single QWidget or QGraphicsItem at a time.

    \value Stylus
        A pen-like device used on a graphics tablet such as a Wacom tablet,
        or on a touchscreen that provides a separate stylus sensing capability.

    \value Airbrush
        A stylus with a thumbwheel to adjust
        \l {QTabletEvent::tangentialPressure}{tangentialPressure}.

    \value Puck
        A device that is similar to a flat mouse with a transparent circle with
        cross-hairs.

    \value Keyboard
        A keyboard.

    \value AllDevices
        Any of the above (used as a default filter value).
*/

/*! \enum QPointingDevice::PointerType

    This enum represents what is interacting with the pointing device.

    There is some redundancy between this property and \l {QInputDevice::DeviceType}.
    For example, if a touchscreen is used, then the \c DeviceType is
    \c TouchScreen and \c PointerType is \c Finger (always). But on a graphics
    tablet, it's often possible for both ends of the stylus to be used, and
    programs need to distinguish them. Therefore the concept is extended so
    that every QPointerEvent has a PointerType, and it can simplify some event
    handling code to ignore the DeviceType and react differently depending on
    the PointerType alone.

    Valid values are:

    \value Unknown
        The pointer type is unknown.
    \value Generic
        A mouse or something acting like a mouse (the core pointer on X11).
    \value Finger
        The user's finger.
    \value Pen
        The drawing end of a stylus.
    \value Eraser
        The other end of the stylus (if it has a virtual eraser on the other end).
    \value Cursor
        A transparent circle with cross-hairs as found on a
        \l {QInputDevice::DeviceType}{Puck} device.
    \value AllPointerTypes
        Any of the above (used as a default filter value).
*/

/*! \enum QPointingDevice::GrabTransition

    This enum represents a transition of exclusive or passive grab
    from one object (possibly \c nullptr) to another (possibly \c nullptr).
    It is emitted as an argument of the QPointingDevice::grabChanged() signal.

    Valid values are:

    \value GrabExclusive
        Emitted after QPointerEvent::setExclusiveGrabber().
    \value UngrabExclusive
        Emitted after QPointerEvent::setExclusiveGrabber() when the grabber is
        set to \c nullptr, to notify that the grab has terminated normally.
    \value CancelGrabExclusive
        Emitted after QPointerEvent::setExclusiveGrabber() when the grabber is set
        to a different object, to notify that the old grabber's grab is "stolen".
    \value GrabPassive
        Emitted after QPointerEvent::addPassiveGrabber().
    \value UngrabPassive
        Emitted when a passive grab is terminated normally,
        for example after QPointerEvent::removePassiveGrabber().
    \value CancelGrabPassive
        Emitted when a passive grab is terminated abnormally (a gesture is canceled).
    \value OverrideGrabPassive
        This value is not currently used.
*/

/*! \fn void QPointingDevice::grabChanged(QObject *grabber, QPointingDevice::GrabTransition transition, const QPointerEvent *event, const QEventPoint &point) const

    This signal is emitted when the \a grabber object gains or loses an
    exclusive or passive grab of \a point during delivery of \a event.
    The \a transition tells what happened, from the perspective of the
    \c grabber object.

    \note A grab transition from one object to another results in two signals,
    to notify that one object has lost its grab, and to notify that there is
    another grabber. In other cases, when transitioning to or from a non-grabbing
    state, only one signal is emitted: the \a grabber argument is never \c nullptr.

    \sa QPointerEvent::setExclusiveGrabber(), QPointerEvent::addPassiveGrabber(), QPointerEvent::removePassiveGrabber()
*/

/*!
    Creates a new invalid pointing device instance as a child of \a parent.
*/
QPointingDevice::QPointingDevice(QObject *parent)
    : QInputDevice(*(new QPointingDevicePrivate("unknown"_L1, -1,
                                              DeviceType::Unknown, PointerType::Unknown,
                                              Capability::None, 0, 0)), parent)
{
}

QPointingDevice::~QPointingDevice()
{
}

/*!
    Creates a new pointing device instance with the given
    \a name, \a deviceType, \a pointerType, \a capabilities, \a maxPoints,
    \a buttonCount, \a seatName, \a uniqueId and \a parent.
*/
QPointingDevice::QPointingDevice(const QString &name, qint64 id, QInputDevice::DeviceType deviceType,
                                 QPointingDevice::PointerType pointerType, Capabilities capabilities, int maxPoints, int buttonCount,
                                 const QString &seatName, QPointingDeviceUniqueId uniqueId, QObject *parent)
    : QInputDevice(*(new QPointingDevicePrivate(name, id, deviceType, pointerType, capabilities, maxPoints, buttonCount, seatName, uniqueId)), parent)
{
}

/*!
    \internal
*/
QPointingDevice::QPointingDevice(QPointingDevicePrivate &d, QObject *parent)
    : QInputDevice(d, parent)
{
}

#if QT_DEPRECATED_SINCE(6, 0)
/*!
    \internal
    \deprecated [6.0] Please use the constructor rather than setters.

    Sets the device type \a devType and infers the pointer type.
*/
void QPointingDevice::setType(DeviceType devType)
{
    Q_D(QPointingDevice);
    d->deviceType = devType;
    if (d->pointerType == PointerType::Unknown)
        switch (devType) {
        case DeviceType::Mouse:
            d->pointerType = PointerType::Generic;
            break;
        case DeviceType::TouchScreen:
        case DeviceType::TouchPad:
            d->pointerType = PointerType::Finger;
            break;
        case DeviceType::Puck:
            d->pointerType = PointerType::Cursor;
            break;
        case DeviceType::Stylus:
        case DeviceType::Airbrush:
            d->pointerType = PointerType::Pen;
            break;
        default:
            break;
        }
}

/*!
    \internal
    \deprecated [6.0] Please use the constructor rather than setters.
*/
void QPointingDevice::setCapabilities(QInputDevice::Capabilities caps)
{
    Q_D(QPointingDevice);
    d->capabilities = caps;
}

/*!
    \internal
    \deprecated [6.0] Please use the constructor rather than setters.
*/
void QPointingDevice::setMaximumTouchPoints(int c)
{
    Q_D(QPointingDevice);
    d->maximumTouchPoints = c;
}
#endif // QT_DEPRECATED_SINCE(6, 0)

/*!
    Returns the pointer type.
*/
QPointingDevice::PointerType QPointingDevice::pointerType() const
{
    Q_D(const QPointingDevice);
    return d->pointerType;
}

/*!
    Returns the maximum number of simultaneous touch points (fingers) that
    can be detected.
*/
int QPointingDevice::maximumPoints() const
{
    Q_D(const QPointingDevice);
    return d->maximumTouchPoints;
}

/*!
    Returns the maximum number of on-device buttons that can be detected.
*/
int QPointingDevice::buttonCount() const
{
    Q_D(const QPointingDevice);
    return d->buttonCount;
}

/*!
    Returns a unique ID (of dubious utility) for the device.

    You probably should rather be concerned with QPointerEventPoint::uniqueId().
*/
QPointingDeviceUniqueId QPointingDevice::uniqueId() const
{
    Q_D(const QPointingDevice);
    return d->uniqueId;
}

/*!
    Returns the primary pointing device (the core pointer, traditionally
    assumed to be a mouse) on the given seat \a seatName.

    If multiple pointing devices are registered, this function prefers a mouse
    or touchpad that matches the given \a seatName and that does not have
    another device as its parent. Usually only one master or core device does
    not have a parent device. But if such a device is not found, this function
    creates a new virtual "core pointer" mouse. Thus Qt continues to work on
    platforms that are not yet doing input device discovery and registration.
*/
const QPointingDevice *QPointingDevice::primaryPointingDevice(const QString& seatName)
{
    const auto v = devices();
    const QPointingDevice *mouse = nullptr;
    const QPointingDevice *touchpad = nullptr;
    for (const QInputDevice *dev : v) {
        if (!seatName.isNull() && dev->seatName() != seatName)
            continue;
        if (dev->type() == QInputDevice::DeviceType::Mouse) {
            if (!mouse)
                mouse = static_cast<const QPointingDevice *>(dev);
            // the core pointer is likely a mouse, and its parent is not another input device
            if (!mouse->parent() || !qobject_cast<const QInputDevice *>(mouse->parent()))
                return mouse;
        } else if (dev->type() == QInputDevice::DeviceType::TouchPad) {
            if (!touchpad || !dev->parent() || dev->parent()->metaObject() != dev->metaObject())
                touchpad = static_cast<const QPointingDevice *>(dev);
        }
    }
    if (!mouse && !touchpad) {
        qCDebug(lcQpaInputDevices) << "no mouse-like devices registered for seat" << seatName
                                   << "The platform plugin should have provided one via "
                                      "QWindowSystemInterface::registerInputDevice(). Creating a default mouse for now.";
        mouse = new QPointingDevice("core pointer"_L1, 1, DeviceType::Mouse,
                                    PointerType::Generic, Capability::Position, 1, 3, seatName,
                                    QPointingDeviceUniqueId(), QCoreApplication::instance());
        QInputDevicePrivate::registerDevice(mouse);
        return mouse;
    }
    if (v.size() > 1)
        qCDebug(lcQpaInputDevices) << "core pointer ambiguous for seat" << seatName;
    if (mouse)
        return mouse;
    return touchpad;
}

QPointingDevicePrivate::~QPointingDevicePrivate()
    = default;

/*!
    \internal
    Finds the device instance belonging to the drawing or eraser end of a particular stylus,
    identified by its \a deviceType, \a pointerType, \a uniqueId and \a systemId.
    Returns the device found, or \c nullptr if none was found.

    If \a systemId is \c 0, it's not significant for the search.

    If an instance matching the given \a deviceType and \a pointerType but with
    only a default-constructed \c uniqueId is found, it will be assumed to be
    the one we're looking for, its \c uniqueId will be updated to match the
    given \a uniqueId, and its \c capabilities will be updated to match the
    given \a capabilities. This is for the benefit of any platform plugin that can
    discover the tablet itself at startup, along with the supported stylus types,
    but then discovers specific styli later on as they come into proximity.
*/
const QPointingDevice *QPointingDevicePrivate::queryTabletDevice(QInputDevice::DeviceType deviceType,
                                                                 QPointingDevice::PointerType pointerType,
                                                                 QPointingDeviceUniqueId uniqueId,
                                                                 QPointingDevice::Capabilities capabilities,
                                                                 qint64 systemId)
{
    const auto &devices = QInputDevice::devices();
    for (const QInputDevice *dev : devices) {
        if (dev->type() < QPointingDevice::DeviceType::Puck || dev->type() > QPointingDevice::DeviceType::Airbrush)
            continue;
        const QPointingDevice *pdev = static_cast<const QPointingDevice *>(dev);
        const auto devPriv = QPointingDevicePrivate::get(pdev);
        bool uniqueIdDiscovered = (devPriv->uniqueId.numericId() == 0 && uniqueId.numericId() != 0);
        if (devPriv->deviceType == deviceType && devPriv->pointerType == pointerType &&
                (!systemId || devPriv->systemId == systemId) &&
                (devPriv->uniqueId == uniqueId || uniqueIdDiscovered)) {
            if (uniqueIdDiscovered) {
                const_cast<QPointingDevicePrivate *>(devPriv)->uniqueId = uniqueId;
                if (capabilities)
                    const_cast<QPointingDevicePrivate *>(devPriv)->capabilities = capabilities;
                qCDebug(lcQpaInputDevices) << "discovered unique ID and capabilities of tablet tool" << pdev;
            }
            return pdev;
        }
    }
    return nullptr;
}

/*!
    \internal
    Finds the device instance identified by its \a systemId.
    Returns the device found, or \c nullptr if none was found.
*/
const QPointingDevice *QPointingDevicePrivate::pointingDeviceById(qint64 systemId)
{
    const auto &devices = QInputDevice::devices();
    for (const QInputDevice *dev : devices) {
        if (dev->type() >= QPointingDevice::DeviceType::Keyboard)
            continue;
        const QPointingDevice *pdev = static_cast<const QPointingDevice *>(dev);
        const auto devPriv = QPointingDevicePrivate::get(pdev);
        if (devPriv->systemId == systemId)
            return pdev;
    }
    return nullptr;
}

/*!
    \internal
    First, ensure that the \a cancelEvent's QTouchEvent::points() list contains
    all points that have exclusive grabs. Then send the event to each object
    that has an exclusive grab of any of the points.
*/
void QPointingDevicePrivate::sendTouchCancelEvent(QTouchEvent *cancelEvent)
{
    // An incoming TouchCancel event will typically not contain any points, but
    // QQuickPointerHandler::onGrabChanged needs to be called for each point
    // that has an exclusive grabber. Adding those points to the event makes it
    // an easy iteration there.
    if (cancelEvent->points().isEmpty()) {
        for (auto &epd : activePoints.values()) {
            if (epd.exclusiveGrabber)
                QMutableTouchEvent::from(cancelEvent)->addPoint(epd.eventPoint);
        }
    }
    for (auto &epd : activePoints.values()) {
        if (epd.exclusiveGrabber)
            QCoreApplication::sendEvent(epd.exclusiveGrabber, cancelEvent);
        // The next touch event can only be a TouchBegin, so clean up.
        cancelEvent->setExclusiveGrabber(epd.eventPoint, nullptr);
        cancelEvent->clearPassiveGrabbers(epd.eventPoint);
    }
}

/*! \internal
    Returns the active EventPointData instance with the given \a id, if available,
    or \c nullptr if not.
*/
QPointingDevicePrivate::EventPointData *QPointingDevicePrivate::queryPointById(int id) const
{
    auto it = activePoints.find(id);
    if (it == activePoints.end())
        return nullptr;
    return &it.value();
}

/*! \internal
    Returns the active EventPointData instance with the given \a id, if available;
    if not, appends a new instance and returns it.
*/
QPointingDevicePrivate::EventPointData *QPointingDevicePrivate::pointById(int id) const
{
    const auto [it, inserted] = activePoints.try_emplace(id);
    if (inserted) {
        Q_Q(const QPointingDevice);
        auto &epd = it.value();
        QMutableEventPoint::setId(epd.eventPoint, id);
        QMutableEventPoint::setDevice(epd.eventPoint, q);
    }
    return &it.value();
}

/*! \internal
    Remove the active EventPointData instance with the given \a id.
*/
void QPointingDevicePrivate::removePointById(int id)
{
    activePoints.remove(id);
}

/*!
    \internal
    Find the first non-null target (widget) via QMutableEventPoint::target()
    in the active points. This is the widget that will receive any event that
    comes from a touchpad, even if some of the touchpoints fall spatially on
    other windows.
*/
QObject *QPointingDevicePrivate::firstActiveTarget() const
{
    for (auto &pt : activePoints.values()) {
        if (auto target = QMutableEventPoint::target(pt.eventPoint))
            return target;
    }
    return nullptr;
}

/*! \internal
    Find the first non-null QWindow instance via QMutableEventPoint::window()
    in the active points. This is the window that will receive any event that
    comes from a touchpad, even if some of the touchpoints fall spatially on
    other windows.
*/
QWindow *QPointingDevicePrivate::firstActiveWindow() const
{
    for (auto &pt : activePoints.values()) {
        if (auto window = QMutableEventPoint::window(pt.eventPoint))
            return window;
    }
    return nullptr;
}

/*! \internal
    Return the exclusive grabber of the first point in activePoints.
    This is mainly for autotests that try to verify the "current" grabber
    outside the context of event delivery, which is something that the rest
    of the codebase should not be doing.
*/
QObject *QPointingDevicePrivate::firstPointExclusiveGrabber() const
{
    if (activePoints.isEmpty())
        return nullptr;
    return activePoints.values().first().exclusiveGrabber;
}

void QPointingDevicePrivate::setExclusiveGrabber(const QPointerEvent *event, const QEventPoint &point, QObject *exclusiveGrabber)
{
    Q_Q(QPointingDevice);
    auto persistentPoint = queryPointById(point.id());
    if (!persistentPoint) {
        qWarning() << "point is not in activePoints" << point;
        return;
    }
    Q_ASSERT(persistentPoint->eventPoint.id() == point.id());
    if (persistentPoint->exclusiveGrabber == exclusiveGrabber)
        return;
    auto oldGrabber = persistentPoint->exclusiveGrabber;
    persistentPoint->exclusiveGrabber = exclusiveGrabber;
    if (oldGrabber)
        emit q->grabChanged(oldGrabber, exclusiveGrabber ? QPointingDevice::CancelGrabExclusive : QPointingDevice::UngrabExclusive,
                            event, persistentPoint->eventPoint);
    if (Q_UNLIKELY(lcPointerGrab().isDebugEnabled())) {
        qCDebug(lcPointerGrab) << name << "point" << point.id() << point.state()
                               << "@" << point.scenePosition()
                               << ": grab" << oldGrabber << "->" << exclusiveGrabber;
    }
    QMutableEventPoint::setGlobalGrabPosition(persistentPoint->eventPoint, point.globalPosition());
    if (exclusiveGrabber)
        emit q->grabChanged(exclusiveGrabber, QPointingDevice::GrabExclusive, event, point);
    else
        persistentPoint->exclusiveGrabberContext.clear();
}

/*!
    \internal
    Call QEventPoint::setExclusiveGrabber(nullptr) on each active point that has a grabber.
*/
bool QPointingDevicePrivate::removeExclusiveGrabber(const QPointerEvent *event, const QObject *grabber)
{
    bool ret = false;
    for (auto &pt : activePoints.values()) {
        if (pt.exclusiveGrabber == grabber) {
            setExclusiveGrabber(event, pt.eventPoint, nullptr);
            ret = true;
        }
    }
    return ret;
}

bool QPointingDevicePrivate::addPassiveGrabber(const QPointerEvent *event, const QEventPoint &point, QObject *grabber)
{
    Q_Q(QPointingDevice);
    auto persistentPoint = queryPointById(point.id());
    if (!persistentPoint) {
        qWarning() << "point is not in activePoints" << point;
        return false;
    }
    if (persistentPoint->passiveGrabbers.contains(grabber))
        return false;
    if (Q_UNLIKELY(lcPointerGrab().isDebugEnabled())) {
        qCDebug(lcPointerGrab) << name << "point" << point.id() << point.state()
                               << ": grab (passive)" << grabber;
    }
    persistentPoint->passiveGrabbers << grabber;
    emit q->grabChanged(grabber, QPointingDevice::GrabPassive, event, point);
    return true;
}

bool QPointingDevicePrivate::setPassiveGrabberContext(QPointingDevicePrivate::EventPointData *epd, QObject *grabber, QObject *context)
{
    qsizetype i = epd->passiveGrabbers.indexOf(grabber);
    if (i < 0)
        return false;
    if (epd->passiveGrabbersContext.size() <= i)
        epd->passiveGrabbersContext.resize(i + 1);
    epd->passiveGrabbersContext[i] = context;
    return true;
}

bool QPointingDevicePrivate::removePassiveGrabber(const QPointerEvent *event, const QEventPoint &point, QObject *grabber)
{
    Q_Q(QPointingDevice);
    auto persistentPoint = queryPointById(point.id());
    if (!persistentPoint) {
        qWarning() << "point is not in activePoints" << point;
        return false;
    }
    qsizetype i = persistentPoint->passiveGrabbers.indexOf(grabber);
    if (i >= 0) {
        if (Q_UNLIKELY(lcPointerGrab().isDebugEnabled())) {
            qCDebug(lcPointerGrab) << name << "point" << point.id() << point.state()
                                   << ": removing passive grabber" << grabber;
        }
        emit q->grabChanged(grabber, QPointingDevice::UngrabPassive, event, point);
        persistentPoint->passiveGrabbers.removeAt(i);
        if (persistentPoint->passiveGrabbersContext.size()) {
            Q_ASSERT(persistentPoint->passiveGrabbersContext.size() > i);
            persistentPoint->passiveGrabbersContext.removeAt(i);
        }
        return true;
    }
    return false;
}

void QPointingDevicePrivate::clearPassiveGrabbers(const QPointerEvent *event, const QEventPoint &point)
{
    Q_Q(QPointingDevice);
    auto persistentPoint = queryPointById(point.id());
    if (!persistentPoint) {
        qWarning() << "point is not in activePoints" << point;
        return;
    }
    if (persistentPoint->passiveGrabbers.isEmpty())
        return;
    if (Q_UNLIKELY(lcPointerGrab().isDebugEnabled())) {
        qCDebug(lcPointerGrab) << name << "point" << point.id() << point.state()
                               << ": clearing" << persistentPoint->passiveGrabbers;
    }
    for (auto g : persistentPoint->passiveGrabbers)
        emit q->grabChanged(g, QPointingDevice::UngrabPassive, event, point);
    persistentPoint->passiveGrabbers.clear();
    persistentPoint->passiveGrabbersContext.clear();
}

/*!
    \internal
    Removes the given \a grabber as both passive and exclusive grabber from all
    points in activePoints where it's currently found. If \a cancel is \c true,
    the transition emitted from the grabChanged() signal will be
    \c CancelGrabExclusive or \c CancelGrabPassive. Otherwise it will be
    \c UngrabExclusive or \c UngrabPassive.

    \note This function provides a way to work around the limitation that we
    normally change grabbers only during event delivery; but it's also more expensive.
*/
void QPointingDevicePrivate::removeGrabber(QObject *grabber, bool cancel)
{
    Q_Q(QPointingDevice);
    for (auto ap : activePoints) {
        auto &epd = ap.second;
        if (epd.exclusiveGrabber.data() == grabber) {
            qCDebug(lcPointerGrab) << name << "point" << epd.eventPoint.id() << epd.eventPoint.state()
                                   << "@" << epd.eventPoint.scenePosition()
                                   << ": grab" << grabber << "-> nullptr";
            epd.exclusiveGrabber.clear();
            epd.exclusiveGrabberContext.clear();
            emit q->grabChanged(grabber,
                                cancel ? QPointingDevice::CancelGrabExclusive : QPointingDevice::UngrabExclusive,
                                nullptr, epd.eventPoint);
        }
        qsizetype pi = epd.passiveGrabbers.indexOf(grabber);
        if (pi >= 0) {
            qCDebug(lcPointerGrab) << name << "point" << epd.eventPoint.id() << epd.eventPoint.state()
                                   << ": removing passive grabber" << grabber;
            epd.passiveGrabbers.removeAt(pi);
            if (epd.passiveGrabbersContext.size()) {
                Q_ASSERT(epd.passiveGrabbersContext.size() > pi);
                epd.passiveGrabbersContext.removeAt(pi);
            }
            emit q->grabChanged(grabber,
                                cancel ? QPointingDevice::CancelGrabPassive : QPointingDevice::UngrabPassive,
                                nullptr, epd.eventPoint);
        }
    }
}

/*!
    \internal
    Finds the device instance belonging to the drawing or eraser end of a particular stylus,
    identified by its \a deviceType, \a pointerType and \a uniqueId. If an existing device
    is not found, a new one is created and registered, with a warning.

    This function is called from QWindowSystemInterface. Platform plugins should use
    \l queryTabletDeviceInstance() to check whether a tablet stylus coming into proximity
    is previously known; if not known, the plugin should create and register the stylus.
*/
const QPointingDevice *QPointingDevicePrivate::tabletDevice(QInputDevice::DeviceType deviceType,
                                                            QPointingDevice::PointerType pointerType,
                                                            QPointingDeviceUniqueId uniqueId)
{
    const QPointingDevice *dev = queryTabletDevice(deviceType, pointerType, uniqueId);
    if (!dev) {
        qCDebug(lcQpaInputDevices) << "failed to find registered tablet device"
                                   << deviceType << pointerType << Qt::hex << uniqueId.numericId()
                                   << "The platform plugin should have provided one via "
                                      "QWindowSystemInterface::registerInputDevice(). Creating a default one for now.";
        dev = new QPointingDevice("fake tablet"_L1, 2, deviceType, pointerType,
                                  QInputDevice::Capability::Position | QInputDevice::Capability::Pressure,
                                  1, 1, QString(), uniqueId, QCoreApplication::instance());
        QInputDevicePrivate::registerDevice(dev);
    }
    return dev;
}

bool QPointingDevice::operator==(const QPointingDevice &other) const
{
    // Wacom tablets generate separate instances for each end of each stylus;
    // QInputDevice::operator==() says they are all the same, but we use
    // the stylus unique serial number and pointerType to distinguish them
    return QInputDevice::operator==(other) &&
            pointerType() == other.pointerType() &&
            uniqueId() == other.uniqueId();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QPointingDevice *device)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();
    debug << "QPointingDevice(";
    if (device) {
        debug << '"' << device->name() << "\" ";
        QtDebugUtils::formatQEnum(debug, device->type());
        debug << " id=" << device->systemId();
        if (!device->seatName().isEmpty())
            debug << " seat=" << device->seatName();
        if (device->pointerType() != QPointingDevice::PointerType::Generic) {
            debug << " ptrType=";
            QtDebugUtils::formatQEnum(debug, device->pointerType());
        }
        if (int(device->capabilities()) != int(QInputDevice::Capability::Position)) {
            debug << " caps=";
            QtDebugUtils::formatQFlags(debug, device->capabilities());
        }
        if (device->maximumPoints() > 1)
            debug << " maxPts=" << device->maximumPoints();
        if (device->uniqueId().isValid())
            debug << " uniqueId=" << Qt::hex << device->uniqueId().numericId() << Qt::dec;
    } else {
        debug << '0';
    }
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

/*!
    \class QPointingDeviceUniqueId
    \since 5.8
    \ingroup events
    \inmodule QtGui

    \brief QPointingDeviceUniqueId identifies a unique object, such as a tagged token
    or stylus, which is used with a pointing device.

    QPointingDeviceUniqueIds can be compared for equality, and can be used as keys in a QHash.
    You get access to the numerical ID via numericId(), if the device supports such IDs.
    For future extensions, though, you should not use that function, but compare objects
    of this type using the equality operator.

    This class is a thin wrapper around an integer ID. You pass it into and out of
    functions by value.

    \sa QEventPoint
*/

/*!
    \fn QPointingDeviceUniqueId::QPointingDeviceUniqueId()
    Constructs an invalid unique pointer ID.
*/

/*!
    Constructs a unique pointer ID from numeric ID \a id.
*/
QPointingDeviceUniqueId QPointingDeviceUniqueId::fromNumericId(qint64 id)
{
    QPointingDeviceUniqueId result;
    result.m_numericId = id;
    return result;
}

/*!
    \fn bool QPointingDeviceUniqueId::isValid() const

    Returns whether this unique pointer ID is valid, that is, it represents an actual
    pointer.
*/

/*!
    \property QPointingDeviceUniqueId::numericId
    \brief the numeric unique ID of the token represented by a touchpoint

    If the device provides a numeric ID, isValid() returns true, and this
    property provides the numeric ID;
    otherwise it is -1.

    You should not use the value of this property in portable code, but
    instead rely on equality to identify pointers.

    \sa isValid()
*/
qint64 QPointingDeviceUniqueId::numericId() const noexcept
{
    return m_numericId;
}

/*!
    \fn bool QPointingDeviceUniqueId::operator==(QPointingDeviceUniqueId lhs, QPointingDeviceUniqueId rhs)
    \since 5.8

    Returns whether the two unique pointer IDs \a lhs and \a rhs identify the same pointer
    (\c true) or not (\c false).
*/

/*!
    \fn bool QPointingDeviceUniqueId::operator!=(QPointingDeviceUniqueId lhs, QPointingDeviceUniqueId rhs)
    \since 5.8

    Returns whether the two unique pointer IDs \a lhs and \a rhs identify different pointers
    (\c true) or not (\c false).
*/

/*!
    \relates QPointingDeviceUniqueId
    \since 5.8

    Returns the hash value for \a key, using \a seed to seed the calculation.
*/
size_t qHash(QPointingDeviceUniqueId key, size_t seed) noexcept
{
    return qHash(key.numericId(), seed);
}

QT_END_NAMESPACE

#include "moc_qpointingdevice.cpp"
