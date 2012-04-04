/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qwindowsysteminterface_qpa.h"
#include "qplatformwindow_qpa.h"
#include "qwindowsysteminterface_qpa_p.h"
#include "private/qguiapplication_p.h"
#include "private/qevent_p.h"
#include "private/qtouchdevice_p.h"
#include <QAbstractEventDispatcher>
#include <QPlatformDrag>
#include <qdebug.h>

QT_BEGIN_NAMESPACE


QElapsedTimer QWindowSystemInterfacePrivate::eventTime;

//------------------------------------------------------------
//
// Callback functions for plugins:
//

QList<QWindowSystemInterfacePrivate::WindowSystemEvent *> QWindowSystemInterfacePrivate::windowSystemEventQueue;
QMutex QWindowSystemInterfacePrivate::queueMutex;

extern QPointer<QWindow> qt_last_mouse_receiver;

/*!
    \class QWindowSystemInterface
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa
    \brief The QWindowSystemInterface provides an event queue for the QPA platform.

    The platform plugins call the various functions to notify about events. The events are queued
    until sendWindowSystemEvents() is called by the event dispatcher.
*/

void QWindowSystemInterface::handleEnterEvent(QWindow *tlw)
{
    if (tlw) {
        QWindowSystemInterfacePrivate::EnterEvent *e = new QWindowSystemInterfacePrivate::EnterEvent(tlw);
        QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
    }
}

void QWindowSystemInterface::handleLeaveEvent(QWindow *tlw)
{
    QWindowSystemInterfacePrivate::LeaveEvent *e = new QWindowSystemInterfacePrivate::LeaveEvent(tlw);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleWindowActivated(QWindow *tlw)
{
    QWindowSystemInterfacePrivate::ActivatedWindowEvent *e = new QWindowSystemInterfacePrivate::ActivatedWindowEvent(tlw);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleWindowStateChanged(QWindow *tlw, Qt::WindowState newState)
{
    QWindowSystemInterfacePrivate::WindowStateChangedEvent *e =
        new QWindowSystemInterfacePrivate::WindowStateChangedEvent(tlw, newState);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleGeometryChange(QWindow *tlw, const QRect &newRect)
{
    QWindowSystemInterfacePrivate::GeometryChangeEvent *e = new QWindowSystemInterfacePrivate::GeometryChangeEvent(tlw,newRect);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleSynchronousGeometryChange(QWindow *tlw, const QRect &newRect)
{
    QWindowSystemInterfacePrivate::GeometryChangeEvent e(tlw,newRect);
    QGuiApplicationPrivate::processWindowSystemEvent(&e); // send event immediately.
}

void QWindowSystemInterface::handleCloseEvent(QWindow *tlw)
{
    if (tlw) {
        QWindowSystemInterfacePrivate::CloseEvent *e =
                new QWindowSystemInterfacePrivate::CloseEvent(tlw);
        QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
    }
}

void QWindowSystemInterface::handleSynchronousCloseEvent(QWindow *tlw)
{
    if (tlw) {
        QWindowSystemInterfacePrivate::CloseEvent e(tlw);
        QGuiApplicationPrivate::processWindowSystemEvent(&e);
    }
}

/*!

\a tlw == 0 means that \a ev is in global coords only


*/
void QWindowSystemInterface::handleMouseEvent(QWindow *w, const QPointF & local, const QPointF & global, Qt::MouseButtons b, Qt::KeyboardModifiers mods) {
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleMouseEvent(w, time, local, global, b, mods);
}

void QWindowSystemInterface::handleMouseEvent(QWindow *tlw, ulong timestamp, const QPointF & local, const QPointF & global, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    QWindowSystemInterfacePrivate::MouseEvent * e =
            new QWindowSystemInterfacePrivate::MouseEvent(tlw, timestamp, local, global, b, mods);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

bool QWindowSystemInterface::tryHandleSynchronousShortcutEvent(QWindow *w, int k, Qt::KeyboardModifiers mods,
                                                               const QString & text, bool autorep, ushort count)
{
    unsigned long timestamp = QWindowSystemInterfacePrivate::eventTime.elapsed();
    return tryHandleSynchronousShortcutEvent(w, timestamp, k, mods, text, autorep, count);
}

bool QWindowSystemInterface::tryHandleSynchronousShortcutEvent(QWindow *w, ulong timestamp, int k, Qt::KeyboardModifiers mods,
                                                               const QString & text, bool autorep, ushort count)
{
    QGuiApplicationPrivate::modifier_buttons = mods;

    QKeyEvent qevent(QEvent::ShortcutOverride, k, mods, text, autorep, count);
    qevent.setTimestamp(timestamp);
    return QGuiApplicationPrivate::instance()->shortcutMap.tryShortcutEvent(w, &qevent);
}

bool QWindowSystemInterface::tryHandleSynchronousExtendedShortcutEvent(QWindow *w, int k, Qt::KeyboardModifiers mods,
                                                                       quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers,
                                                                       const QString &text, bool autorep, ushort count)
{
    unsigned long timestamp = QWindowSystemInterfacePrivate::eventTime.elapsed();
    return tryHandleSynchronousExtendedShortcutEvent(w, timestamp, k, mods, nativeScanCode, nativeVirtualKey, nativeModifiers, text, autorep, count);
}

bool QWindowSystemInterface::tryHandleSynchronousExtendedShortcutEvent(QWindow *w, ulong timestamp, int k, Qt::KeyboardModifiers mods,
                                                                       quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers,
                                                                       const QString &text, bool autorep, ushort count)
{
    QGuiApplicationPrivate::modifier_buttons = mods;

    QKeyEvent qevent(QEvent::ShortcutOverride, k, mods, nativeScanCode, nativeVirtualKey, nativeModifiers, text, autorep, count);
    qevent.setTimestamp(timestamp);
    return QGuiApplicationPrivate::instance()->shortcutMap.tryShortcutEvent(w, &qevent);
}


void QWindowSystemInterface::handleKeyEvent(QWindow *w, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text, bool autorep, ushort count) {
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleKeyEvent(w, time, t, k, mods, text, autorep, count);
}

void QWindowSystemInterface::handleKeyEvent(QWindow *tlw, ulong timestamp, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text, bool autorep, ushort count)
{
    QWindowSystemInterfacePrivate::KeyEvent * e =
            new QWindowSystemInterfacePrivate::KeyEvent(tlw, timestamp, t, k, mods, text, autorep, count);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleExtendedKeyEvent(QWindow *w, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                                    quint32 nativeScanCode, quint32 nativeVirtualKey,
                                                    quint32 nativeModifiers,
                                                    const QString& text, bool autorep,
                                                    ushort count)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleExtendedKeyEvent(w, time, type, key, modifiers, nativeScanCode, nativeVirtualKey, nativeModifiers,
                           text, autorep, count);
}

void QWindowSystemInterface::handleExtendedKeyEvent(QWindow *tlw, ulong timestamp, QEvent::Type type, int key,
                                                    Qt::KeyboardModifiers modifiers,
                                                    quint32 nativeScanCode, quint32 nativeVirtualKey,
                                                    quint32 nativeModifiers,
                                                    const QString& text, bool autorep,
                                                    ushort count)
{
    QWindowSystemInterfacePrivate::KeyEvent * e =
            new QWindowSystemInterfacePrivate::KeyEvent(tlw, timestamp, type, key, modifiers,
                nativeScanCode, nativeVirtualKey, nativeModifiers, text, autorep, count);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleWheelEvent(QWindow *w, const QPointF & local, const QPointF & global, int d, Qt::Orientation o, Qt::KeyboardModifiers mods) {
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleWheelEvent(w, time, local, global, d, o, mods);
}

void QWindowSystemInterface::handleWheelEvent(QWindow *tlw, ulong timestamp, const QPointF & local, const QPointF & global, int d, Qt::Orientation o, Qt::KeyboardModifiers mods)
{
    QPoint point = (o == Qt::Vertical) ? QPoint(0, d) : QPoint(d, 0);
    handleWheelEvent(tlw, timestamp, local, global, QPoint(), point, mods);
}

void QWindowSystemInterface::handleWheelEvent(QWindow *w, const QPointF & local, const QPointF & global, QPoint pixelDelta, QPoint angleDelta, Qt::KeyboardModifiers mods)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleWheelEvent(w, time, local, global, pixelDelta, angleDelta, mods);
}

void QWindowSystemInterface::handleWheelEvent(QWindow *tlw, ulong timestamp, const QPointF & local, const QPointF & global, QPoint pixelDelta, QPoint angleDelta, Qt::KeyboardModifiers mods)
{
    // Qt 4 sends two separate wheel events for horizontal and vertical
    // deltas. For Qt 5 we want to send the deltas in one event, but at the
    // same time preserve source and behavior compatibility with Qt 4.
    //
    // In addition high-resolution pixel-based deltas are also supported.
    // Platforms that does not support these may pass a null point here.
    // Angle deltas must always be sent in addition to pixel deltas.
    QWindowSystemInterfacePrivate::WheelEvent *e;

    if (angleDelta.isNull())
        return;

    // Simple case: vertical deltas only:
    if (angleDelta.y() != 0 && angleDelta.x() == 0) {
        e = new QWindowSystemInterfacePrivate::WheelEvent(tlw, timestamp, local, global, pixelDelta, angleDelta, angleDelta.y(), Qt::Vertical, mods);
        QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
        return;
    }

    // Simple case: horizontal deltas only:
    if (angleDelta.y() == 0 && angleDelta.x() != 0) {
        e = new QWindowSystemInterfacePrivate::WheelEvent(tlw, timestamp, local, global, pixelDelta, angleDelta, angleDelta.x(), Qt::Horizontal, mods);
        QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
        return;
    }

    // Both horizontal and vertical deltas: Send two wheel events.
    // The first event contains the Qt 5 pixel and angle delta as points,
    // and in addition the Qt 4 compatibility vertical angle delta.
    e = new QWindowSystemInterfacePrivate::WheelEvent(tlw, timestamp, local, global, pixelDelta, angleDelta, angleDelta.y(), Qt::Vertical, mods);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);

    // The second event contains null pixel and angle points and the
    // Qt 4 compatibility horizontal angle delta.
    e = new QWindowSystemInterfacePrivate::WheelEvent(tlw, timestamp, local, global, QPoint(), QPoint(), angleDelta.x(), Qt::Horizontal, mods);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}


QWindowSystemInterfacePrivate::ExposeEvent::ExposeEvent(QWindow *exposed, const QRegion &region)
    : WindowSystemEvent(Expose)
    , exposed(exposed)
    , isExposed(exposed && exposed->handle() ? exposed->handle()->isExposed() : false)
    , region(region)
{
}

int QWindowSystemInterfacePrivate::windowSystemEventsQueued()
{
    queueMutex.lock();
    int ret = windowSystemEventQueue.count();
    queueMutex.unlock();
    return ret;
}

QWindowSystemInterfacePrivate::WindowSystemEvent * QWindowSystemInterfacePrivate::getWindowSystemEvent()
{
    queueMutex.lock();
    QWindowSystemInterfacePrivate::WindowSystemEvent *ret;
    if (windowSystemEventQueue.isEmpty())
        ret = 0;
    else
        ret = windowSystemEventQueue.takeFirst();
    queueMutex.unlock();
    return ret;
}

void QWindowSystemInterfacePrivate::queueWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *ev)
{
    queueMutex.lock();
    windowSystemEventQueue.append(ev);
    queueMutex.unlock();

    QAbstractEventDispatcher *dispatcher = QGuiApplicationPrivate::qt_qpa_core_dispatcher();
    if (dispatcher)
        dispatcher->wakeUp();
}

void QWindowSystemInterface::registerTouchDevice(QTouchDevice *device)
{
    QTouchDevicePrivate::registerDevice(device);
}

void QWindowSystemInterface::handleTouchEvent(QWindow *w, QTouchDevice *device,
                                              const QList<TouchPoint> &points, Qt::KeyboardModifiers mods)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTouchEvent(w, time, device, points, mods);
}

QList<QTouchEvent::TouchPoint> QWindowSystemInterfacePrivate::convertTouchPoints(const QList<QWindowSystemInterface::TouchPoint> &points, QEvent::Type *type)
{
    QList<QTouchEvent::TouchPoint> touchPoints;
    Qt::TouchPointStates states;
    QTouchEvent::TouchPoint p;

    QList<QWindowSystemInterface::TouchPoint>::const_iterator point = points.constBegin();
    QList<QWindowSystemInterface::TouchPoint>::const_iterator end = points.constEnd();
    while (point != end) {
        p.setId(point->id);
        p.setPressure(point->pressure);
        states |= point->state;
        p.setState(point->state);

        const QPointF screenPos = point->area.center();
        p.setScreenPos(screenPos);
        p.setScreenRect(point->area);

        // The local pos and rect are not set, they will be calculated
        // when the event gets processed by QGuiApplication.

        p.setNormalizedPos(point->normalPosition);
        p.setVelocity(point->velocity);
        p.setFlags(point->flags);
        p.setRawScreenPositions(point->rawPositions);

        touchPoints.append(p);
        ++point;
    }

    // Determine the event type based on the combined point states.
    if (type) {
        *type = QEvent::TouchUpdate;
        if (states == Qt::TouchPointPressed)
            *type = QEvent::TouchBegin;
        else if (states == Qt::TouchPointReleased)
            *type = QEvent::TouchEnd;
    }

    return touchPoints;
}

void QWindowSystemInterface::handleTouchEvent(QWindow *tlw, ulong timestamp, QTouchDevice *device,
                                              const QList<TouchPoint> &points, Qt::KeyboardModifiers mods)
{
    if (!points.size()) // Touch events must have at least one point
        return;

    if (!QTouchDevicePrivate::isRegistered(device)) // Disallow passing bogus, non-registered devices.
        return;

    QEvent::Type type;
    QList<QTouchEvent::TouchPoint> touchPoints = QWindowSystemInterfacePrivate::convertTouchPoints(points, &type);

    QWindowSystemInterfacePrivate::TouchEvent *e =
            new QWindowSystemInterfacePrivate::TouchEvent(tlw, timestamp, type, device, touchPoints, mods);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleTouchCancelEvent(QWindow *w, QTouchDevice *device,
                                                    Qt::KeyboardModifiers mods)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTouchCancelEvent(w, time, device, mods);
}

void QWindowSystemInterface::handleTouchCancelEvent(QWindow *w, ulong timestamp, QTouchDevice *device,
                                                    Qt::KeyboardModifiers mods)
{
    QWindowSystemInterfacePrivate::TouchEvent *e =
            new QWindowSystemInterfacePrivate::TouchEvent(w, timestamp, QEvent::TouchCancel, device,
                                                         QList<QTouchEvent::TouchPoint>(), mods);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenOrientationChange(QScreen *screen, Qt::ScreenOrientation orientation)
{
    QWindowSystemInterfacePrivate::ScreenOrientationEvent *e =
            new QWindowSystemInterfacePrivate::ScreenOrientationEvent(screen, orientation);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenGeometryChange(QScreen *screen, const QRect &geometry)
{
    QWindowSystemInterfacePrivate::ScreenGeometryEvent *e =
            new QWindowSystemInterfacePrivate::ScreenGeometryEvent(screen, geometry);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenAvailableGeometryChange(QScreen *screen, const QRect &availableGeometry)
{
    QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *e =
            new QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent(screen, availableGeometry);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(QScreen *screen, qreal dpiX, qreal dpiY)
{
    QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent *e =
            new QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent(screen, dpiX, dpiY);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleThemeChange(QWindow *tlw)
{
    QWindowSystemInterfacePrivate::ThemeChangeEvent *e = new QWindowSystemInterfacePrivate::ThemeChangeEvent(tlw);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleExposeEvent(QWindow *tlw, const QRegion &region)
{
    QWindowSystemInterfacePrivate::ExposeEvent *e = new QWindowSystemInterfacePrivate::ExposeEvent(tlw, region);
    QWindowSystemInterfacePrivate::queueWindowSystemEvent(e);
}

void QWindowSystemInterface::handleSynchronousExposeEvent(QWindow *tlw, const QRegion &region)
{
    QWindowSystemInterfacePrivate::ExposeEvent e(tlw, region);
    QGuiApplicationPrivate::processWindowSystemEvent(&e); // send event immediately.
}

bool QWindowSystemInterface::sendWindowSystemEvents(QAbstractEventDispatcher *eventDispatcher, QEventLoop::ProcessEventsFlags flags)
{
    int nevents = 0;

    // handle gui and posted events
    QCoreApplication::sendPostedEvents();

    while (true) {
        QWindowSystemInterfacePrivate::WindowSystemEvent *event;
        if (!(flags & QEventLoop::ExcludeUserInputEvents)
            && QWindowSystemInterfacePrivate::windowSystemEventsQueued() > 0) {
            // process a pending user input event
            event = QWindowSystemInterfacePrivate::getWindowSystemEvent();
            if (!event)
                break;
        } else {
            break;
        }

        if (eventDispatcher->filterEvent(event)) {
            delete event;
            continue;
        }

        nevents++;

        QGuiApplicationPrivate::processWindowSystemEvent(event);
        delete event;
    }

    return (nevents > 0);
}

int QWindowSystemInterface::windowSystemEventsQueued()
{
    return QWindowSystemInterfacePrivate::windowSystemEventsQueued();
}

QPlatformDragQtResponse QWindowSystemInterface::handleDrag(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions)
{
    return QGuiApplicationPrivate::processDrag(w, dropData, p,supportedActions);
}

QPlatformDropQtResponse QWindowSystemInterface::handleDrop(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions)
{
    return QGuiApplicationPrivate::processDrop(w, dropData, p,supportedActions);
}

/*!
    \fn static QWindowSystemInterface::handleNativeEvent(QWindow *window, const QByteArray &eventType, void *message, long *result)
    \brief Passes a native event identified by \a eventType to the \a window.

    \note This function can only be called from the GUI thread.
    \sa QPlatformNativeInterface::setEventFilter()
*/

bool QWindowSystemInterface::handleNativeEvent(QWindow *window, const QByteArray &eventType, void *message, long *result)
{
    return QGuiApplicationPrivate::processNativeEvent(window, eventType, message, result);
}

QT_END_NAMESPACE
