/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qwindowsysteminterface.h"
#include <qpa/qplatformwindow.h>
#include "qwindowsysteminterface_p.h"
#include "private/qguiapplication_p.h"
#include "private/qevent_p.h"
#include "private/qtouchdevice_p.h"
#include <QAbstractEventDispatcher>
#include <qpa/qplatformintegration.h>
#include <qdebug.h>
#include "qhighdpiscaling_p.h"
#include <QtCore/qscopedvaluerollback.h>

#if QT_CONFIG(draganddrop)
#include <qpa/qplatformdrag.h>
#endif

QT_BEGIN_NAMESPACE


QElapsedTimer QWindowSystemInterfacePrivate::eventTime;
bool QWindowSystemInterfacePrivate::synchronousWindowSystemEvents = false;
bool QWindowSystemInterfacePrivate::TabletEvent::platformSynthesizesMouse = true;
QWaitCondition QWindowSystemInterfacePrivate::eventsFlushed;
QMutex QWindowSystemInterfacePrivate::flushEventMutex;
QAtomicInt QWindowSystemInterfacePrivate::eventAccepted;
QWindowSystemEventHandler *QWindowSystemInterfacePrivate::eventHandler;
QWindowSystemInterfacePrivate::WindowSystemEventList QWindowSystemInterfacePrivate::windowSystemEventQueue;

extern QPointer<QWindow> qt_last_mouse_receiver;


// ------------------- QWindowSystemInterfacePrivate -------------------

/*
    Handles a window system event asynchronously by posting the event to Qt Gui.

    This function posts the event on the window system event queue and wakes the
    Gui event dispatcher. Qt Gui will then handle the event asynchonously at a
    later point.
*/
template<>
bool QWindowSystemInterfacePrivate::handleWindowSystemEvent<QWindowSystemInterface::AsynchronousDelivery>(WindowSystemEvent *ev)
{
    windowSystemEventQueue.append(ev);
    if (QAbstractEventDispatcher *dispatcher = QGuiApplicationPrivate::qt_qpa_core_dispatcher())
        dispatcher->wakeUp();
    return true;
}

/*
    Handles a window system event synchronously.

    Qt Gui will process the event immediately. The return value indicates if Qt
    accepted the event.

    If the event is delivered from another thread than the Qt main thread the
    window system event queue is flushed, which may deliver other events as
    well.
*/
template<>
bool QWindowSystemInterfacePrivate::handleWindowSystemEvent<QWindowSystemInterface::SynchronousDelivery>(WindowSystemEvent *ev)
{
    bool accepted = true;
    if (QThread::currentThread() == QGuiApplication::instance()->thread()) {
        // Process the event immediately on the current thread and return the accepted state.
        QGuiApplicationPrivate::processWindowSystemEvent(ev);
        accepted = ev->eventAccepted;
        delete ev;
    } else {
        // Post the event on the Qt main thread queue and flush the queue.
        // This will wake up the Gui thread which will process the event.
        // Return the accepted state for the last event on the queue,
        // which is the event posted by this function.
        handleWindowSystemEvent<QWindowSystemInterface::AsynchronousDelivery>(ev);
        accepted = QWindowSystemInterface::flushWindowSystemEvents();
    }
    return accepted;
}

/*
    Handles a window system event.

    By default this function posts the event on the window system event queue and
    wakes the Gui event dispatcher. Qt Gui will then handle the event asynchonously
    at a later point. The return value is not used in asynchronous mode and will
    always be true.

    In synchronous mode Qt Gui will process the event immediately. The return value
    indicates if Qt accepted the event. If the event is delivered from another thread
    than the Qt main thread the window system event queue is flushed, which may deliver
    other events as well.

    \sa flushWindowSystemEvents(), setSynchronousWindowSystemEvents()
*/
template<>
bool QWindowSystemInterfacePrivate::handleWindowSystemEvent<QWindowSystemInterface::DefaultDelivery>(QWindowSystemInterfacePrivate::WindowSystemEvent *ev)
{
    if (synchronousWindowSystemEvents)
        return handleWindowSystemEvent<QWindowSystemInterface::SynchronousDelivery>(ev);
    else
        return handleWindowSystemEvent<QWindowSystemInterface::AsynchronousDelivery>(ev);
}

int QWindowSystemInterfacePrivate::windowSystemEventsQueued()
{
    return windowSystemEventQueue.count();
}

bool QWindowSystemInterfacePrivate::nonUserInputEventsQueued()
{
    return windowSystemEventQueue.nonUserInputEventsQueued();
}

QWindowSystemInterfacePrivate::WindowSystemEvent * QWindowSystemInterfacePrivate::getWindowSystemEvent()
{
    return windowSystemEventQueue.takeFirstOrReturnNull();
}

QWindowSystemInterfacePrivate::WindowSystemEvent *QWindowSystemInterfacePrivate::getNonUserInputWindowSystemEvent()
{
    return windowSystemEventQueue.takeFirstNonUserInputOrReturnNull();
}

QWindowSystemInterfacePrivate::WindowSystemEvent *QWindowSystemInterfacePrivate::peekWindowSystemEvent(EventType t)
{
    return windowSystemEventQueue.peekAtFirstOfType(t);
}

void QWindowSystemInterfacePrivate::removeWindowSystemEvent(WindowSystemEvent *event)
{
    windowSystemEventQueue.remove(event);
}

void QWindowSystemInterfacePrivate::installWindowSystemEventHandler(QWindowSystemEventHandler *handler)
{
    if (!eventHandler)
        eventHandler = handler;
}

void QWindowSystemInterfacePrivate::removeWindowSystemEventhandler(QWindowSystemEventHandler *handler)
{
    if (eventHandler == handler)
        eventHandler = 0;
}

QWindowSystemEventHandler::~QWindowSystemEventHandler()
{
    QWindowSystemInterfacePrivate::removeWindowSystemEventhandler(this);
}

bool QWindowSystemEventHandler::sendEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e)
{
    QGuiApplicationPrivate::processWindowSystemEvent(e);
    return true;
}

//------------------------------------------------------------
//
// Callback functions for plugins:
//

#define QT_DEFINE_QPA_EVENT_HANDLER(ReturnType, HandlerName, ...) \
    template Q_GUI_EXPORT ReturnType QWindowSystemInterface::HandlerName<QWindowSystemInterface::DefaultDelivery>(__VA_ARGS__); \
    template Q_GUI_EXPORT ReturnType QWindowSystemInterface::HandlerName<QWindowSystemInterface::SynchronousDelivery>(__VA_ARGS__); \
    template Q_GUI_EXPORT ReturnType QWindowSystemInterface::HandlerName<QWindowSystemInterface::AsynchronousDelivery>(__VA_ARGS__); \
    template<typename Delivery> ReturnType QWindowSystemInterface::HandlerName(__VA_ARGS__)

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

QT_DEFINE_QPA_EVENT_HANDLER(void, handleEnterEvent, QWindow *window, const QPointF &local, const QPointF &global)
{
    if (window) {
        QWindowSystemInterfacePrivate::EnterEvent *e
                = new QWindowSystemInterfacePrivate::EnterEvent(window, QHighDpi::fromNativeLocalPosition(local, window), QHighDpi::fromNativePixels(global, window));
        QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
    }
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleLeaveEvent, QWindow *window)
{
    QWindowSystemInterfacePrivate::LeaveEvent *e = new QWindowSystemInterfacePrivate::LeaveEvent(window);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

/*!
    This method can be used to ensure leave and enter events are both in queue when moving from
    one QWindow to another. This allows QWindow subclasses to check for a queued enter event
    when handling the leave event (\c QWindowSystemInterfacePrivate::peekWindowSystemEvent) to
    determine where mouse went and act accordingly. E.g. QWidgetWindow needs to know if mouse
    cursor moves between windows in same window hierarchy.
*/
void QWindowSystemInterface::handleEnterLeaveEvent(QWindow *enter, QWindow *leave, const QPointF &local, const QPointF& global)
{
    handleLeaveEvent<AsynchronousDelivery>(leave);
    handleEnterEvent(enter, local, global);
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleWindowActivated, QWindow *window, Qt::FocusReason r)
{
    QWindowSystemInterfacePrivate::ActivatedWindowEvent *e =
        new QWindowSystemInterfacePrivate::ActivatedWindowEvent(window, r);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleWindowStateChanged, QWindow *window, Qt::WindowStates newState, int oldState)
{
    Q_ASSERT(window);
    if (oldState < Qt::WindowNoState)
        oldState = window->windowStates();

    QWindowSystemInterfacePrivate::WindowStateChangedEvent *e =
        new QWindowSystemInterfacePrivate::WindowStateChangedEvent(window, newState, Qt::WindowStates(oldState));
    QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleWindowScreenChanged, QWindow *window, QScreen *screen)
{

    QWindowSystemInterfacePrivate::WindowScreenChangedEvent *e =
        new QWindowSystemInterfacePrivate::WindowScreenChangedEvent(window, screen);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleSafeAreaMarginsChanged, QWindow *window)
{
    QWindowSystemInterfacePrivate::SafeAreaMarginsChangedEvent *e =
        new QWindowSystemInterfacePrivate::SafeAreaMarginsChangedEvent(window);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleApplicationStateChanged, Qt::ApplicationState newState, bool forcePropagate)
{
    Q_ASSERT(QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ApplicationState));
    QWindowSystemInterfacePrivate::ApplicationStateChangedEvent *e =
        new QWindowSystemInterfacePrivate::ApplicationStateChangedEvent(newState, forcePropagate);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

QWindowSystemInterfacePrivate::GeometryChangeEvent::GeometryChangeEvent(QWindow *window, const QRect &newGeometry)
    : WindowSystemEvent(GeometryChange)
    , window(window)
    , newGeometry(newGeometry)
{
    if (const QPlatformWindow *pw = window->handle())
        requestedGeometry = QHighDpi::fromNativePixels(pw->QPlatformWindow::geometry(), window);
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleGeometryChange, QWindow *window, const QRect &newRect)
{
    Q_ASSERT(window);
    QWindowSystemInterfacePrivate::GeometryChangeEvent *e = new QWindowSystemInterfacePrivate::GeometryChangeEvent(window, QHighDpi::fromNativePixels(newRect, window));
    if (window->handle()) {
        // Persist the new geometry so that QWindow::geometry() can be queried in the resize event
        window->handle()->QPlatformWindow::setGeometry(newRect);
        // FIXME: This does not work during platform window creation, where the QWindow does not
        // have its handle set up yet. Platforms that deliver events during window creation need
        // to handle the persistence manually, e.g. by overriding geometry().
    }
    QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

QWindowSystemInterfacePrivate::ExposeEvent::ExposeEvent(QWindow *window, const QRegion &region)
    : WindowSystemEvent(Expose)
    , window(window)
    , isExposed(window && window->handle() ? window->handle()->isExposed() : false)
    , region(region)
{
}

/*! \internal
    Handles an expose event.

    The platform plugin sends expose events when an area of the window
    is invalidated or window exposure changes. \a region is in window
    local coordinates. An empty region indicates that the window is
    obscured, but note that the exposed property of the QWindow will be set
    based on what QPlatformWindow::isExposed() returns at the time of this call,
    not based on what the region is. // FIXME: this should probably be fixed.

    The platform plugin may omit sending expose events (or send obscure
    events) for windows that are on screen but where the client area is
    completely covered by other windows or otherwise not visible. Expose
    event consumers can then use this to disable updates for such windows.
    This is required behavior on platforms where OpenGL swapbuffers stops
    blocking for obscured windows (like macOS).
*/
QT_DEFINE_QPA_EVENT_HANDLER(void, handleExposeEvent, QWindow *window, const QRegion &region)
{
    QWindowSystemInterfacePrivate::ExposeEvent *e =
        new QWindowSystemInterfacePrivate::ExposeEvent(window, QHighDpi::fromNativeLocalExposedRegion(region, window));
    QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

void QWindowSystemInterface::handleCloseEvent(QWindow *window, bool *accepted)
{
    if (window) {
        QWindowSystemInterfacePrivate::CloseEvent *e =
                new QWindowSystemInterfacePrivate::CloseEvent(window, accepted);
        QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
    }
}

/*!

\a w == 0 means that the event is in global coords only, \a local will be ignored in this case

*/
#if QT_DEPRECATED_SINCE(5, 11)
QT_DEFINE_QPA_EVENT_HANDLER(void, handleMouseEvent, QWindow *window, const QPointF &local, const QPointF &global, Qt::MouseButtons b,
                                              Qt::KeyboardModifiers mods, Qt::MouseEventSource source)
{
    handleMouseEvent<Delivery>(window, local, global, b, Qt::NoButton, QEvent::None, mods, source);
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleMouseEvent, QWindow *window, ulong timestamp, const QPointF &local, const QPointF &global, Qt::MouseButtons b,
                                              Qt::KeyboardModifiers mods, Qt::MouseEventSource source)
{
    handleMouseEvent<Delivery>(window, timestamp, local, global, b, Qt::NoButton, QEvent::None, mods, source);
}

void QWindowSystemInterface::handleFrameStrutMouseEvent(QWindow *window, const QPointF &local, const QPointF &global, Qt::MouseButtons b,
                                                        Qt::KeyboardModifiers mods, Qt::MouseEventSource source)
{
    handleFrameStrutMouseEvent(window, local, global, b, Qt::NoButton, QEvent::None, mods, source);
}

void QWindowSystemInterface::handleFrameStrutMouseEvent(QWindow *window, ulong timestamp, const QPointF &local, const QPointF &global, Qt::MouseButtons b,
                                                        Qt::KeyboardModifiers mods, Qt::MouseEventSource source)
{
    handleFrameStrutMouseEvent(window, timestamp, local, global, b, Qt::NoButton, QEvent::None, mods, source);
}
#endif // QT_DEPRECATED_SINCE(5, 11)

QT_DEFINE_QPA_EVENT_HANDLER(void, handleMouseEvent, QWindow *window,
                            const QPointF &local, const QPointF &global, Qt::MouseButtons state,
                            Qt::MouseButton button, QEvent::Type type, Qt::KeyboardModifiers mods,
                            Qt::MouseEventSource source)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleMouseEvent<Delivery>(window, time, local, global, state, button, type, mods, source);
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleMouseEvent, QWindow *window, ulong timestamp,
                            const QPointF &local, const QPointF &global, Qt::MouseButtons state,
                            Qt::MouseButton button, QEvent::Type type, Qt::KeyboardModifiers mods,
                            Qt::MouseEventSource source)
{
    auto localPos = QHighDpi::fromNativeLocalPosition(local, window);
    auto globalPos = QHighDpi::fromNativePixels(global, window);

    QWindowSystemInterfacePrivate::MouseEvent *e =
        new QWindowSystemInterfacePrivate::MouseEvent(window, timestamp, localPos, globalPos,
                                                      state, mods, button, type, source);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

void QWindowSystemInterface::handleFrameStrutMouseEvent(QWindow *window,
                                                        const QPointF &local, const QPointF &global,
                                                        Qt::MouseButtons state,
                                                        Qt::MouseButton button, QEvent::Type type,
                                                        Qt::KeyboardModifiers mods,
                                                        Qt::MouseEventSource source)
{
    const unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleFrameStrutMouseEvent(window, time, local, global, state, button, type, mods, source);
}

void QWindowSystemInterface::handleFrameStrutMouseEvent(QWindow *window, ulong timestamp,
                                                        const QPointF &local, const QPointF &global,
                                                        Qt::MouseButtons state,
                                                        Qt::MouseButton button, QEvent::Type type,
                                                        Qt::KeyboardModifiers mods,
                                                        Qt::MouseEventSource source)
{
    auto localPos = QHighDpi::fromNativeLocalPosition(local, window);
    auto globalPos = QHighDpi::fromNativePixels(global, window);

    QWindowSystemInterfacePrivate::MouseEvent *e =
            new QWindowSystemInterfacePrivate::MouseEvent(window, timestamp, localPos, globalPos,
                                                          state, mods, button, type, source, true);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

bool QWindowSystemInterface::handleShortcutEvent(QWindow *window, ulong timestamp, int keyCode, Qt::KeyboardModifiers modifiers, quint32 nativeScanCode,
                                      quint32 nativeVirtualKey, quint32 nativeModifiers, const QString &text, bool autorepeat, ushort count)
{
#ifndef QT_NO_SHORTCUT
    if (!window)
        window = QGuiApplication::focusWindow();

    QShortcutMap &shortcutMap = QGuiApplicationPrivate::instance()->shortcutMap;
    if (shortcutMap.state() == QKeySequence::NoMatch) {
        // Check if the shortcut is overridden by some object in the event delivery path (typically the focus object).
        // If so, we should not look up the shortcut in the shortcut map, but instead deliver the event as a regular
        // key event, so that the target that accepted the shortcut override event can handle it. Note that we only
        // do this if the shortcut map hasn't found a partial shortcut match yet. If it has, the shortcut can not be
        // overridden.
        QWindowSystemInterfacePrivate::KeyEvent *shortcutOverrideEvent = new QWindowSystemInterfacePrivate::KeyEvent(window, timestamp,
            QEvent::ShortcutOverride, keyCode, modifiers, nativeScanCode, nativeVirtualKey, nativeModifiers, text, autorepeat, count);

        {
            if (QWindowSystemInterfacePrivate::handleWindowSystemEvent<SynchronousDelivery>(shortcutOverrideEvent))
                return false;
        }
    }

    // The shortcut event is dispatched as a QShortcutEvent, not a QKeyEvent, but we use
    // the QKeyEvent as a container for the various properties that the shortcut map needs
    // to inspect to determine if a shortcut matched the keys that were pressed.
    QKeyEvent keyEvent(QEvent::ShortcutOverride, keyCode, modifiers, nativeScanCode,
        nativeVirtualKey, nativeModifiers, text, autorepeat, count);

    return shortcutMap.tryShortcut(&keyEvent);
#else
    Q_UNUSED(window)
    Q_UNUSED(timestamp)
    Q_UNUSED(keyCode)
    Q_UNUSED(modifiers)
    Q_UNUSED(nativeScanCode)
    Q_UNUSED(nativeVirtualKey)
    Q_UNUSED(nativeModifiers)
    Q_UNUSED(text)
    Q_UNUSED(autorepeat)
    Q_UNUSED(count)
    return false;
#endif
}

QT_DEFINE_QPA_EVENT_HANDLER(bool, handleKeyEvent, QWindow *window, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text, bool autorep, ushort count) {
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    return handleKeyEvent<Delivery>(window, time, t, k, mods, text, autorep, count);
}

QT_DEFINE_QPA_EVENT_HANDLER(bool, handleKeyEvent, QWindow *window, ulong timestamp, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text, bool autorep, ushort count)
{
#if defined(Q_OS_OSX)
    if (t == QEvent::KeyPress && QWindowSystemInterface::handleShortcutEvent(window, timestamp, k, mods, 0, 0, 0, text, autorep, count))
        return true;
#endif

    QWindowSystemInterfacePrivate::KeyEvent * e =
            new QWindowSystemInterfacePrivate::KeyEvent(window, timestamp, t, k, mods, text, autorep, count);
    return QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

bool QWindowSystemInterface::handleExtendedKeyEvent(QWindow *window, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                                                    quint32 nativeScanCode, quint32 nativeVirtualKey,
                                                    quint32 nativeModifiers,
                                                    const QString& text, bool autorep,
                                                    ushort count, bool tryShortcutOverride)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    return handleExtendedKeyEvent(window, time, type, key, modifiers, nativeScanCode, nativeVirtualKey, nativeModifiers,
                           text, autorep, count, tryShortcutOverride);
}

bool QWindowSystemInterface::handleExtendedKeyEvent(QWindow *window, ulong timestamp, QEvent::Type type, int key,
                                                    Qt::KeyboardModifiers modifiers,
                                                    quint32 nativeScanCode, quint32 nativeVirtualKey,
                                                    quint32 nativeModifiers,
                                                    const QString& text, bool autorep,
                                                    ushort count, bool tryShortcutOverride)
{
#if defined(Q_OS_OSX)
    if (tryShortcutOverride && type == QEvent::KeyPress && QWindowSystemInterface::handleShortcutEvent(window,
            timestamp, key, modifiers, nativeScanCode, nativeVirtualKey, nativeModifiers, text, autorep, count)) {
        return true;
    }
#else
    Q_UNUSED(tryShortcutOverride)
#endif

    QWindowSystemInterfacePrivate::KeyEvent * e =
            new QWindowSystemInterfacePrivate::KeyEvent(window, timestamp, type, key, modifiers,
                nativeScanCode, nativeVirtualKey, nativeModifiers, text, autorep, count);
    return QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

QWindowSystemInterfacePrivate::WheelEvent::WheelEvent(QWindow *window, ulong time, const QPointF &local, const QPointF &global, QPoint pixelD,
        QPoint angleD, int qt4D, Qt::Orientation qt4O, Qt::KeyboardModifiers mods, Qt::ScrollPhase phase, Qt::MouseEventSource src, bool inverted)
    : InputEvent(window, time, Wheel, mods), pixelDelta(pixelD), angleDelta(angleD), qt4Delta(qt4D),
      qt4Orientation(qt4O), localPos(local), globalPos(global), phase(phase), source(src), inverted(inverted)
{
}

#if QT_DEPRECATED_SINCE(5, 10)
void QWindowSystemInterface::handleWheelEvent(QWindow *window, const QPointF &local, const QPointF &global, int d, Qt::Orientation o, Qt::KeyboardModifiers mods) {
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    handleWheelEvent(window, time, local, global, d, o, mods);
QT_WARNING_POP
}

void QWindowSystemInterface::handleWheelEvent(QWindow *window, ulong timestamp, const QPointF &local, const QPointF &global, int d, Qt::Orientation o, Qt::KeyboardModifiers mods)
{
    QPoint point = (o == Qt::Vertical) ? QPoint(0, d) : QPoint(d, 0);
    handleWheelEvent(window, timestamp, local, global, QPoint(), point, mods);
}
#endif // QT_DEPRECATED_SINCE(5, 10)

void QWindowSystemInterface::handleWheelEvent(QWindow *window, const QPointF &local, const QPointF &global, QPoint pixelDelta, QPoint angleDelta, Qt::KeyboardModifiers mods, Qt::ScrollPhase phase, Qt::MouseEventSource source)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleWheelEvent(window, time, local, global, pixelDelta, angleDelta, mods, phase, source);
}

void QWindowSystemInterface::handleWheelEvent(QWindow *window, ulong timestamp, const QPointF &local, const QPointF &global, QPoint pixelDelta, QPoint angleDelta, Qt::KeyboardModifiers mods, Qt::ScrollPhase phase,
                                              Qt::MouseEventSource source, bool invertedScrolling)
{
    // Qt 4 sends two separate wheel events for horizontal and vertical
    // deltas. For Qt 5 we want to send the deltas in one event, but at the
    // same time preserve source and behavior compatibility with Qt 4.
    //
    // In addition high-resolution pixel-based deltas are also supported.
    // Platforms that does not support these may pass a null point here.
    // Angle deltas must always be sent in addition to pixel deltas.
    QWindowSystemInterfacePrivate::WheelEvent *e;

    // Pass Qt::ScrollBegin and Qt::ScrollEnd through
    // even if the wheel delta is null.
    if (angleDelta.isNull() && phase == Qt::ScrollUpdate)
        return;

    // Simple case: vertical deltas only:
    if (angleDelta.y() != 0 && angleDelta.x() == 0) {
        e = new QWindowSystemInterfacePrivate::WheelEvent(window, timestamp, QHighDpi::fromNativeLocalPosition(local, window), QHighDpi::fromNativePixels(global, window), pixelDelta, angleDelta, angleDelta.y(), Qt::Vertical,
                                                          mods, phase, source, invertedScrolling);
        QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
        return;
    }

    // Simple case: horizontal deltas only:
    if (angleDelta.y() == 0 && angleDelta.x() != 0) {
        e = new QWindowSystemInterfacePrivate::WheelEvent(window, timestamp, QHighDpi::fromNativeLocalPosition(local, window), QHighDpi::fromNativePixels(global, window), pixelDelta, angleDelta, angleDelta.x(), Qt::Horizontal, mods, phase, source, invertedScrolling);
        QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
        return;
    }

    // Both horizontal and vertical deltas: Send two wheel events.
    // The first event contains the Qt 5 pixel and angle delta as points,
    // and in addition the Qt 4 compatibility vertical angle delta.
    e = new QWindowSystemInterfacePrivate::WheelEvent(window, timestamp, QHighDpi::fromNativeLocalPosition(local, window), QHighDpi::fromNativePixels(global, window), pixelDelta, angleDelta, angleDelta.y(), Qt::Vertical, mods, phase, source, invertedScrolling);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);

    // The second event contains null pixel and angle points and the
    // Qt 4 compatibility horizontal angle delta.
    e = new QWindowSystemInterfacePrivate::WheelEvent(window, timestamp, QHighDpi::fromNativeLocalPosition(local, window), QHighDpi::fromNativePixels(global, window), QPoint(), QPoint(), angleDelta.x(), Qt::Horizontal, mods, phase, source, invertedScrolling);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::registerTouchDevice(const QTouchDevice *device)
{
    QTouchDevicePrivate::registerDevice(device);
}

void QWindowSystemInterface::unregisterTouchDevice(const QTouchDevice *device)
{
    QTouchDevicePrivate::unregisterDevice(device);
}

bool QWindowSystemInterface::isTouchDeviceRegistered(const QTouchDevice *device)
{
    return QTouchDevicePrivate::isRegistered(device);
}

static int g_nextPointId = 1;

// map from device-independent point id (arbitrary) to "Qt point" ids
typedef QMap<quint64, int> PointIdMap;
Q_GLOBAL_STATIC(PointIdMap, g_pointIdMap)

/*!
    \internal
    This function maps potentially arbitrary point ids \a pointId in the 32 bit
    value space to start from 1 and increase incrementally for each touch point
    held down. If all touch points are released it will reset the id back to 1
    for the following touch point.

    We can then assume that the touch points ids will never become too large,
    and it will then put the device identifier \a deviceId in the upper 8 bits.
    This leaves us with max 255 devices, and 16.7M taps without full release
    before we run out of value space.
*/
static int acquireCombinedPointId(quint8 deviceId, int pointId)
{
    quint64 combinedId64 = (quint64(deviceId) << 32) + pointId;
    auto it = g_pointIdMap->constFind(combinedId64);
    int uid;
    if (it == g_pointIdMap->constEnd()) {
        uid = g_nextPointId++;
        g_pointIdMap->insert(combinedId64, uid);
    } else {
        uid = *it;
    }
    return (deviceId << 24) + uid;
}

QList<QTouchEvent::TouchPoint>
    QWindowSystemInterfacePrivate::fromNativeTouchPoints(const QList<QWindowSystemInterface::TouchPoint> &points,
                                                         const QWindow *window, quint8 deviceId,
                                                         QEvent::Type *type)
{
    QList<QTouchEvent::TouchPoint> touchPoints;
    Qt::TouchPointStates states;
    QTouchEvent::TouchPoint p;

    touchPoints.reserve(points.count());
    QList<QWindowSystemInterface::TouchPoint>::const_iterator point = points.constBegin();
    QList<QWindowSystemInterface::TouchPoint>::const_iterator end = points.constEnd();
    while (point != end) {
        p.setId(acquireCombinedPointId(deviceId, point->id));
        if (point->uniqueId >= 0)
            p.setUniqueId(point->uniqueId);
        p.setPressure(point->pressure);
        p.setRotation(point->rotation);
        states |= point->state;
        p.setState(point->state);

        const QPointF screenPos = point->area.center();
        p.setScreenPos(QHighDpi::fromNativePixels(screenPos, window));
        p.setScreenRect(QHighDpi::fromNativePixels(point->area, window));

        // The local pos and rect are not set, they will be calculated
        // when the event gets processed by QGuiApplication.

        p.setNormalizedPos(QHighDpi::fromNativePixels(point->normalPosition, window));
        p.setVelocity(QHighDpi::fromNativePixels(point->velocity, window));
        p.setFlags(point->flags);
        p.setRawScreenPositions(QHighDpi::fromNativePixels(point->rawPositions, window));

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

    if (states == Qt::TouchPointReleased) {
        g_nextPointId = 1;
        g_pointIdMap->clear();
    }

    return touchPoints;
}

QList<QWindowSystemInterface::TouchPoint>
    QWindowSystemInterfacePrivate::toNativeTouchPoints(const QList<QTouchEvent::TouchPoint>& pointList,
                                                       const QWindow *window)
{
    QList<QWindowSystemInterface::TouchPoint> newList;
    newList.reserve(pointList.size());
    for (const QTouchEvent::TouchPoint &pt : pointList) {
        QWindowSystemInterface::TouchPoint p;
        p.id = pt.id();
        p.flags = pt.flags();
        p.normalPosition = QHighDpi::toNativeLocalPosition(pt.normalizedPos(), window);
        p.area = QHighDpi::toNativePixels(pt.screenRect(), window);
        p.pressure = pt.pressure();
        p.state = pt.state();
        p.velocity = QHighDpi::toNativePixels(pt.velocity(), window);
        p.rawPositions = QHighDpi::toNativePixels(pt.rawScreenPositions(), window);
        newList.append(p);
    }
    return newList;
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleTouchEvent, QWindow *window, QTouchDevice *device,
                                              const QList<TouchPoint> &points, Qt::KeyboardModifiers mods)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTouchEvent<Delivery>(window, time, device, points, mods);
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleTouchEvent, QWindow *window, ulong timestamp, QTouchDevice *device,
                                              const QList<TouchPoint> &points, Qt::KeyboardModifiers mods)
{
    if (!points.size()) // Touch events must have at least one point
        return;

    if (!QTouchDevicePrivate::isRegistered(device)) // Disallow passing bogus, non-registered devices.
        return;

    QEvent::Type type;
    QList<QTouchEvent::TouchPoint> touchPoints =
            QWindowSystemInterfacePrivate::fromNativeTouchPoints(points, window, QTouchDevicePrivate::get(device)->id, &type);

    QWindowSystemInterfacePrivate::TouchEvent *e =
            new QWindowSystemInterfacePrivate::TouchEvent(window, timestamp, type, device, touchPoints, mods);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleTouchCancelEvent, QWindow *window, QTouchDevice *device,
                                                    Qt::KeyboardModifiers mods)
{
    unsigned long time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTouchCancelEvent<Delivery>(window, time, device, mods);
}

QT_DEFINE_QPA_EVENT_HANDLER(void, handleTouchCancelEvent, QWindow *window, ulong timestamp, QTouchDevice *device,
                                                    Qt::KeyboardModifiers mods)
{
    QWindowSystemInterfacePrivate::TouchEvent *e =
            new QWindowSystemInterfacePrivate::TouchEvent(window, timestamp, QEvent::TouchCancel, device,
                                                         QList<QTouchEvent::TouchPoint>(), mods);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent<Delivery>(e);
}

void QWindowSystemInterface::handleScreenOrientationChange(QScreen *screen, Qt::ScreenOrientation orientation)
{
    QWindowSystemInterfacePrivate::ScreenOrientationEvent *e =
            new QWindowSystemInterfacePrivate::ScreenOrientationEvent(screen, orientation);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenGeometryChange(QScreen *screen, const QRect &geometry, const QRect &availableGeometry)
{
    QWindowSystemInterfacePrivate::ScreenGeometryEvent *e =
        new QWindowSystemInterfacePrivate::ScreenGeometryEvent(screen, QHighDpi::fromNativeScreenGeometry(geometry, screen), QHighDpi::fromNative(availableGeometry, screen, geometry.topLeft()));
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(QScreen *screen, qreal dpiX, qreal dpiY)
{
    QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent *e =
            new QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent(screen, dpiX, dpiY); // ### tja
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleScreenRefreshRateChange(QScreen *screen, qreal newRefreshRate)
{
    QWindowSystemInterfacePrivate::ScreenRefreshRateEvent *e =
            new QWindowSystemInterfacePrivate::ScreenRefreshRateEvent(screen, newRefreshRate);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleThemeChange(QWindow *window)
{
    QWindowSystemInterfacePrivate::ThemeChangeEvent *e = new QWindowSystemInterfacePrivate::ThemeChangeEvent(window);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

#if QT_CONFIG(draganddrop)
QPlatformDragQtResponse QWindowSystemInterface::handleDrag(QWindow *window, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions)
{
    return QGuiApplicationPrivate::processDrag(window, dropData, QHighDpi::fromNativeLocalPosition(p, window) ,supportedActions);
}

QPlatformDropQtResponse QWindowSystemInterface::handleDrop(QWindow *window, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions)
{
    return QGuiApplicationPrivate::processDrop(window, dropData, QHighDpi::fromNativeLocalPosition(p, window),supportedActions);
}
#endif // QT_CONFIG(draganddrop)

/*!
    \fn static QWindowSystemInterface::handleNativeEvent(QWindow *window, const QByteArray &eventType, void *message, long *result)
    \brief Passes a native event identified by \a eventType to the \a window.

    \note This function can only be called from the GUI thread.
*/

bool QWindowSystemInterface::handleNativeEvent(QWindow *window, const QByteArray &eventType, void *message, long *result)
{
    return QGuiApplicationPrivate::processNativeEvent(window, eventType, message, result);
}

void QWindowSystemInterface::handleFileOpenEvent(const QString& fileName)
{
    QWindowSystemInterfacePrivate::FileOpenEvent e(fileName);
    QGuiApplicationPrivate::processWindowSystemEvent(&e);
}

void QWindowSystemInterface::handleFileOpenEvent(const QUrl &url)
{
    QWindowSystemInterfacePrivate::FileOpenEvent e(url);
    QGuiApplicationPrivate::processWindowSystemEvent(&e);
}

void QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(bool v)
{
    platformSynthesizesMouse = v;
}

void QWindowSystemInterface::handleTabletEvent(QWindow *window, ulong timestamp, const QPointF &local, const QPointF &global,
                                               int device, int pointerType, Qt::MouseButtons buttons, qreal pressure, int xTilt, int yTilt,
                                               qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                               Qt::KeyboardModifiers modifiers)
{
    QWindowSystemInterfacePrivate::TabletEvent *e =
        new QWindowSystemInterfacePrivate::TabletEvent(window, timestamp,
                                                       QHighDpi::fromNativeLocalPosition(local, window),
                                                       QHighDpi::fromNativePixels(global, window),
                                                       device, pointerType, buttons, pressure,
                                                       xTilt, yTilt, tangentialPressure, rotation, z, uid, modifiers);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleTabletEvent(QWindow *window, const QPointF &local, const QPointF &global,
                                               int device, int pointerType, Qt::MouseButtons buttons, qreal pressure, int xTilt, int yTilt,
                                               qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                               Qt::KeyboardModifiers modifiers)
{
    ulong time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTabletEvent(window, time, local, global, device, pointerType, buttons, pressure,
                      xTilt, yTilt, tangentialPressure, rotation, z, uid, modifiers);
}

#if QT_DEPRECATED_SINCE(5, 10)
void QWindowSystemInterface::handleTabletEvent(QWindow *window, ulong timestamp, bool down, const QPointF &local, const QPointF &global,
                                               int device, int pointerType, qreal pressure, int xTilt, int yTilt,
                                               qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                               Qt::KeyboardModifiers modifiers)
{
    handleTabletEvent(window, timestamp, local, global, device, pointerType, (down ? Qt::LeftButton : Qt::NoButton), pressure,
                      xTilt, yTilt, tangentialPressure, rotation, z, uid, modifiers);
}

void QWindowSystemInterface::handleTabletEvent(QWindow *window, bool down, const QPointF &local, const QPointF &global,
                                               int device, int pointerType, qreal pressure, int xTilt, int yTilt,
                                               qreal tangentialPressure, qreal rotation, int z, qint64 uid,
                                               Qt::KeyboardModifiers modifiers)
{
    handleTabletEvent(window, local, global, device, pointerType, (down ? Qt::LeftButton : Qt::NoButton), pressure,
                      xTilt, yTilt, tangentialPressure, rotation, z, uid, modifiers);
}
#endif // QT_DEPRECATED_SINCE(5, 10)

void QWindowSystemInterface::handleTabletEnterProximityEvent(ulong timestamp, int device, int pointerType, qint64 uid)
{
    QWindowSystemInterfacePrivate::TabletEnterProximityEvent *e =
            new QWindowSystemInterfacePrivate::TabletEnterProximityEvent(timestamp, device, pointerType, uid);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleTabletEnterProximityEvent(int device, int pointerType, qint64 uid)
{
    ulong time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTabletEnterProximityEvent(time, device, pointerType, uid);
}

void QWindowSystemInterface::handleTabletLeaveProximityEvent(ulong timestamp, int device, int pointerType, qint64 uid)
{
    QWindowSystemInterfacePrivate::TabletLeaveProximityEvent *e =
            new QWindowSystemInterfacePrivate::TabletLeaveProximityEvent(timestamp, device, pointerType, uid);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleTabletLeaveProximityEvent(int device, int pointerType, qint64 uid)
{
    ulong time = QWindowSystemInterfacePrivate::eventTime.elapsed();
    handleTabletLeaveProximityEvent(time, device, pointerType, uid);
}

#ifndef QT_NO_GESTURES
void QWindowSystemInterface::handleGestureEvent(QWindow *window, QTouchDevice *device, ulong timestamp, Qt::NativeGestureType type,
                                                QPointF &local, QPointF &global)
{
    QWindowSystemInterfacePrivate::GestureEvent *e =
        new QWindowSystemInterfacePrivate::GestureEvent(window, timestamp, type, device, local, global);
       QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleGestureEventWithRealValue(QWindow *window, QTouchDevice *device, ulong timestamp, Qt::NativeGestureType type,
                                                                qreal value, QPointF &local, QPointF &global)
{
    QWindowSystemInterfacePrivate::GestureEvent *e =
        new QWindowSystemInterfacePrivate::GestureEvent(window, timestamp, type, device, local, global);
    e->realValue = value;
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

void QWindowSystemInterface::handleGestureEventWithSequenceIdAndValue(QWindow *window, QTouchDevice *device, ulong timestamp, Qt::NativeGestureType type,
                                                                         ulong sequenceId, quint64 value, QPointF &local, QPointF &global)
{
    QWindowSystemInterfacePrivate::GestureEvent *e =
        new QWindowSystemInterfacePrivate::GestureEvent(window, timestamp, type, device, local, global);
    e->sequenceId = sequenceId;
    e->intValue = value;
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}
#endif // QT_NO_GESTURES

void QWindowSystemInterface::handlePlatformPanelEvent(QWindow *w)
{
    QWindowSystemInterfacePrivate::PlatformPanelEvent *e =
            new QWindowSystemInterfacePrivate::PlatformPanelEvent(w);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}

#ifndef QT_NO_CONTEXTMENU
void QWindowSystemInterface::handleContextMenuEvent(QWindow *window, bool mouseTriggered,
                                                    const QPoint &pos, const QPoint &globalPos,
                                                    Qt::KeyboardModifiers modifiers)
{
    QWindowSystemInterfacePrivate::ContextMenuEvent *e =
            new QWindowSystemInterfacePrivate::ContextMenuEvent(window, mouseTriggered, pos,
                                                                globalPos, modifiers);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}
#endif

#if QT_CONFIG(whatsthis)
void QWindowSystemInterface::handleEnterWhatsThisEvent()
{
    QWindowSystemInterfacePrivate::WindowSystemEvent *e =
            new QWindowSystemInterfacePrivate::WindowSystemEvent(QWindowSystemInterfacePrivate::EnterWhatsThisMode);
    QWindowSystemInterfacePrivate::handleWindowSystemEvent(e);
}
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QWindowSystemInterface::TouchPoint &p)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "TouchPoint(" << p.id << " @" << p.area << " normalized " << p.normalPosition
                  << " press " << p.pressure << " vel " << p.velocity << " state " << (int)p.state;
    return dbg;
}
#endif

// ------------------ Event dispatcher functionality ------------------

/*!
    Make Qt Gui process all events on the event queue immediately. Return the
    accepted state for the last event on the queue.
*/
bool QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ProcessEventsFlags flags)
{
    const int count = QWindowSystemInterfacePrivate::windowSystemEventQueue.count();
    if (!count)
        return false;
    if (!QGuiApplication::instance()) {
        qWarning().nospace()
            << "QWindowSystemInterface::flushWindowSystemEvents() invoked after "
               "QGuiApplication destruction, discarding " << count << " events.";
        QWindowSystemInterfacePrivate::windowSystemEventQueue.clear();
        return false;
    }
    if (QThread::currentThread() != QGuiApplication::instance()->thread()) {
        // Post a FlushEvents event which will trigger a call back to
        // deferredFlushWindowSystemEvents from the Gui thread.
        QMutexLocker locker(&QWindowSystemInterfacePrivate::flushEventMutex);
        QWindowSystemInterfacePrivate::FlushEventsEvent *e = new QWindowSystemInterfacePrivate::FlushEventsEvent(flags);
        QWindowSystemInterfacePrivate::handleWindowSystemEvent<AsynchronousDelivery>(e);
        QWindowSystemInterfacePrivate::eventsFlushed.wait(&QWindowSystemInterfacePrivate::flushEventMutex);
    } else {
        sendWindowSystemEvents(flags);
    }
    return QWindowSystemInterfacePrivate::eventAccepted.load() > 0;
}

void QWindowSystemInterface::deferredFlushWindowSystemEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_ASSERT(QThread::currentThread() == QGuiApplication::instance()->thread());

    QMutexLocker locker(&QWindowSystemInterfacePrivate::flushEventMutex);
    sendWindowSystemEvents(flags);
    QWindowSystemInterfacePrivate::eventsFlushed.wakeOne();
}

bool QWindowSystemInterface::sendWindowSystemEvents(QEventLoop::ProcessEventsFlags flags)
{
    int nevents = 0;

    while (QWindowSystemInterfacePrivate::windowSystemEventsQueued()) {
        QWindowSystemInterfacePrivate::WindowSystemEvent *event =
            (flags & QEventLoop::ExcludeUserInputEvents) ?
                QWindowSystemInterfacePrivate::getNonUserInputWindowSystemEvent() :
                QWindowSystemInterfacePrivate::getWindowSystemEvent();
        if (!event)
            break;

        if (QWindowSystemInterfacePrivate::eventHandler) {
            if (QWindowSystemInterfacePrivate::eventHandler->sendEvent(event))
                nevents++;
        } else {
            nevents++;
            QGuiApplicationPrivate::processWindowSystemEvent(event);
        }

        // Record the accepted state for the processed event
        // (excluding flush events). This state can then be
        // returned by flushWindowSystemEvents().
        if (event->type != QWindowSystemInterfacePrivate::FlushEvents)
            QWindowSystemInterfacePrivate::eventAccepted.store(event->eventAccepted);

        delete event;
    }

    return (nevents > 0);
}

void QWindowSystemInterface::setSynchronousWindowSystemEvents(bool enable)
{
    QWindowSystemInterfacePrivate::synchronousWindowSystemEvents = enable;
}

int QWindowSystemInterface::windowSystemEventsQueued()
{
    return QWindowSystemInterfacePrivate::windowSystemEventsQueued();
}

bool QWindowSystemInterface::nonUserInputEventsQueued()
{
    return QWindowSystemInterfacePrivate::nonUserInputEventsQueued();
}

// --------------------- QtTestLib support ---------------------

// The following functions are used by testlib, and need to be synchronous to avoid
// race conditions with plugins delivering native events from secondary threads.
// FIXME: It seems unnecessary to export these wrapper functions, when qtestlib could access
// QWindowSystemInterface directly (by adding dependency to gui-private), see QTBUG-63146.

Q_GUI_EXPORT void qt_handleMouseEvent(QWindow *window, const QPointF &local, const QPointF &global,
                                      Qt::MouseButtons state, Qt::MouseButton button,
                                      QEvent::Type type, Qt::KeyboardModifiers mods, int timestamp)
{
    const qreal factor = QHighDpiScaling::factor(window);
    QWindowSystemInterface::handleMouseEvent<QWindowSystemInterface::SynchronousDelivery>(window,
                timestamp, local * factor, global * factor, state, button, type, mods);
}

// Wrapper for compatibility with Qt < 5.11
// ### Qt6: Remove
Q_GUI_EXPORT void qt_handleMouseEvent(QWindow *window, const QPointF &local, const QPointF &global,
                                      Qt::MouseButtons b, Qt::KeyboardModifiers mods, int timestamp)
{
    const qreal factor = QHighDpiScaling::factor(window);
    QWindowSystemInterface::handleMouseEvent<QWindowSystemInterface::SynchronousDelivery>(window,
                timestamp, local * factor, global * factor, b, Qt::NoButton, QEvent::None, mods);
}

// Wrapper for compatibility with Qt < 5.6
// ### Qt6: Remove
Q_GUI_EXPORT void qt_handleMouseEvent(QWindow *w, const QPointF &local, const QPointF &global, Qt::MouseButtons b, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    qt_handleMouseEvent(w, local, global, b, mods, QWindowSystemInterfacePrivate::eventTime.elapsed());
}

Q_GUI_EXPORT void qt_handleKeyEvent(QWindow *window, QEvent::Type t, int k, Qt::KeyboardModifiers mods, const QString & text = QString(), bool autorep = false, ushort count = 1)
{
    QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(window, t, k, mods, text, autorep, count);
}

Q_GUI_EXPORT bool qt_sendShortcutOverrideEvent(QObject *o, ulong timestamp, int k, Qt::KeyboardModifiers mods, const QString &text = QString(), bool autorep = false, ushort count = 1)
{
#ifndef QT_NO_SHORTCUT

    // FIXME: This method should not allow targeting a specific object, but should
    // instead forward the event to a window, which then takes care of normal event
    // propagation. We need to fix a lot of tests before we can refactor this (the
    // window needs to be exposed and active and have a focus object), so we leave
    // it as is for now. See QTBUG-48577.

    QGuiApplicationPrivate::modifier_buttons = mods;

    QKeyEvent qevent(QEvent::ShortcutOverride, k, mods, text, autorep, count);
    qevent.setTimestamp(timestamp);

    QShortcutMap &shortcutMap = QGuiApplicationPrivate::instance()->shortcutMap;
    if (shortcutMap.state() == QKeySequence::NoMatch) {
        // Try sending as QKeyEvent::ShortcutOverride first
        QCoreApplication::sendEvent(o, &qevent);
        if (qevent.isAccepted())
            return false;
    }

    // Then as QShortcutEvent
    return shortcutMap.tryShortcut(&qevent);
#else
    Q_UNUSED(o)
    Q_UNUSED(timestamp)
    Q_UNUSED(k)
    Q_UNUSED(mods)
    Q_UNUSED(text)
    Q_UNUSED(autorep)
    Q_UNUSED(count)
    return false;
#endif
}

namespace QTest
{
    Q_GUI_EXPORT QTouchDevice * createTouchDevice(QTouchDevice::DeviceType devType = QTouchDevice::TouchScreen)
    {
        QTouchDevice *ret = new QTouchDevice();
        ret->setType(devType);
        QWindowSystemInterface::registerTouchDevice(ret);
        return ret;
    }
}

Q_GUI_EXPORT void qt_handleTouchEvent(QWindow *window, QTouchDevice *device,
                                const QList<QTouchEvent::TouchPoint> &points,
                                Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::SynchronousDelivery>(window, device,
        QWindowSystemInterfacePrivate::toNativeTouchPoints(points, window), mods);
}


QT_END_NAMESPACE
