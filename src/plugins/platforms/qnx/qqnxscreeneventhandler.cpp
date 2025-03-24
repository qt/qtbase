// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include "qqnxglobal.h"

#include "qqnxscreeneventhandler.h"
#include "qqnxscreeneventthread.h"
#include "qqnxintegration.h"
#include "qqnxkeytranslator.h"
#include "qqnxscreen.h"
#include "qqnxscreeneventfilter.h"
#include "qqnxscreentraits.h"

#include <QDebug>
#include <QGuiApplication>

#include <errno.h>
#include <sys/keycodes.h>

using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(lcQpaScreenEvents, "qt.qpa.screen.events");

static int qtKey(int virtualKey, QChar::Category category)
{
    if (Q_UNLIKELY(category == QChar::Other_NotAssigned))
        return virtualKey;
    else if (category == QChar::Other_PrivateUse)
        return qtKeyForPrivateUseQnxKey(virtualKey);
    else
        return QChar::toUpper(virtualKey);
}

static QString keyString(int sym, QChar::Category category)
{
    if (Q_UNLIKELY(category == QChar::Other_NotAssigned)) {
        return QString();
    } else if (category == QChar::Other_PrivateUse) {
        return keyStringForPrivateUseQnxKey(sym);
    } else {
        return QStringView{QChar::fromUcs4(sym)}.toString();
    }
}

static QString capKeyString(int cap, int modifiers, int key)
{
    if (cap >= 0x20 && cap <= 0x0ff) {
        if (modifiers & KEYMOD_CTRL)
            return QChar((int)(key & 0x3f));
    }
    return QString();
}

template <typename T>
static void finishCloseEvent(screen_event_t event)
{
    T t;
    screen_get_event_property_pv(event,
            screen_traits<T>::propertyName,
            reinterpret_cast<void**>(&t));
    screen_traits<T>::destroy(t);
}

static void finishCloseEvent(screen_event_t event)
{
    // Let libscreen know that we're finished with anything that may have been acquired.
    int objectType = SCREEN_OBJECT_TYPE_CONTEXT;
    screen_get_event_property_iv(event, SCREEN_PROPERTY_OBJECT_TYPE, &objectType);
    switch (objectType) {
    case SCREEN_OBJECT_TYPE_CONTEXT:
        finishCloseEvent<screen_context_t>(event);
        break;
    case SCREEN_OBJECT_TYPE_DEVICE:
        finishCloseEvent<screen_device_t>(event);
        break;
    case SCREEN_OBJECT_TYPE_DISPLAY:
        // no screen_destroy_display
        break;
    case SCREEN_OBJECT_TYPE_GROUP:
        finishCloseEvent<screen_group_t>(event);
        break;
    case SCREEN_OBJECT_TYPE_PIXMAP:
        finishCloseEvent<screen_pixmap_t>(event);
        break;
    case SCREEN_OBJECT_TYPE_SESSION:
        finishCloseEvent<screen_session_t>(event);
        break;
#if _SCREEN_VERSION >= _SCREEN_MAKE_VERSION(2, 0, 0)
    case SCREEN_OBJECT_TYPE_STREAM:
        finishCloseEvent<screen_stream_t>(event);
        break;
#endif
    case SCREEN_OBJECT_TYPE_WINDOW:
        finishCloseEvent<screen_window_t>(event);
        break;
    }
}

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQnxScreenEventHandler::QQnxScreenEventHandler(QQnxIntegration *integration)
    : m_qnxIntegration(integration)
    , m_lastButtonState(Qt::NoButton)
    , m_lastMouseWindow(0)
    , m_touchDevice(0)
    , m_mouseDevice(0)
    , m_eventThread(0)
{
    // Create a touch device
    m_touchDevice = new QPointingDevice(
            "touchscreen"_L1, 1, QInputDevice::DeviceType::TouchScreen,
            QPointingDevice::PointerType::Finger,
            QPointingDevice::Capability::Position | QPointingDevice::Capability::Area
                    | QPointingDevice::Capability::Pressure
                    | QPointingDevice::Capability::NormalizedPosition,
            MaximumTouchPoints, 8);
    QWindowSystemInterface::registerInputDevice(m_touchDevice);

    m_mouseDevice = new QPointingDevice("mouse"_L1, 2, QInputDevice::DeviceType::Mouse,
                                        QPointingDevice::PointerType::Generic,
                                        QPointingDevice::Capability::Position, 1, 8);
    QWindowSystemInterface::registerInputDevice(m_mouseDevice);

    // initialize array of touch points
    for (int i = 0; i < MaximumTouchPoints; i++) {

        // map array index to id
        m_touchPoints[i].id = i;

        // pressure is not supported - use default
        m_touchPoints[i].pressure = 1.0;

        // nothing touching
        m_touchPoints[i].state = QEventPoint::State::Released;
    }
}

void QQnxScreenEventHandler::addScreenEventFilter(QQnxScreenEventFilter *filter)
{
    m_eventFilters.append(filter);
}

void QQnxScreenEventHandler::removeScreenEventFilter(QQnxScreenEventFilter *filter)
{
    m_eventFilters.removeOne(filter);
}

bool QQnxScreenEventHandler::handleEvent(screen_event_t event)
{
    // get the event type
    int qnxType;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &qnxType),
                        "Failed to query event type");

    return handleEvent(event, qnxType);
}

bool QQnxScreenEventHandler::handleEvent(screen_event_t event, int qnxType)
{
    switch (qnxType) {
    case SCREEN_EVENT_MTOUCH_TOUCH:
    case SCREEN_EVENT_MTOUCH_MOVE:
    case SCREEN_EVENT_MTOUCH_RELEASE:
        handleTouchEvent(event, qnxType);
        break;

    case SCREEN_EVENT_KEYBOARD:
        handleKeyboardEvent(event);
        break;

    case SCREEN_EVENT_POINTER:
        handlePointerEvent(event);
        break;

    case SCREEN_EVENT_CREATE:
        handleCreateEvent(event);
        break;

    case SCREEN_EVENT_CLOSE:
        handleCloseEvent(event);
        break;

    case SCREEN_EVENT_DISPLAY:
        handleDisplayEvent(event);
        break;

    case SCREEN_EVENT_PROPERTY:
        handlePropertyEvent(event);
        break;

    case SCREEN_EVENT_MANAGER:
        handleManagerEvent(event);
        break;

    default:
        // event ignored
        qCDebug(lcQpaScreenEvents) << Q_FUNC_INFO << "Unknown event" << qnxType;
        return false;
    }

    return true;
}

void QQnxScreenEventHandler::injectKeyboardEvent(int flags, int sym, int modifiers, int scan, int cap)
{
    Q_UNUSED(scan);

    if (!(flags & KEY_CAP_VALID))
        return;

    // Correct erroneous information.
    if ((flags & KEY_SYM_VALID) && sym == static_cast<int>(0xFFFFFFFF))
        flags &= ~(KEY_SYM_VALID);

    Qt::KeyboardModifiers qtMod = Qt::NoModifier;
    if (modifiers & KEYMOD_SHIFT)
        qtMod |= Qt::ShiftModifier;
    if (modifiers & KEYMOD_CTRL)
        qtMod |= Qt::ControlModifier;
    if (modifiers & KEYMOD_ALT)
        qtMod |= Qt::AltModifier;
    if (isKeypadKey(cap))
        qtMod |= Qt::KeypadModifier;

    QEvent::Type type = (flags & KEY_DOWN) ? QEvent::KeyPress : QEvent::KeyRelease;

    int virtualKey = (flags & KEY_SYM_VALID) ? sym : cap;
    QChar::Category category = QChar::category(virtualKey);
    int key = qtKey(virtualKey, category);
    QString keyStr = (flags & KEY_SYM_VALID) ? keyString(sym, category) :
                                               capKeyString(cap, modifiers, key);

    QWindowSystemInterface::handleExtendedKeyEvent(QGuiApplication::focusWindow(), type, key, qtMod,
            scan, virtualKey, modifiers, keyStr, flags & KEY_REPEAT);
    qCDebug(lcQpaScreenEvents) << "Qt key t=" << type << ", k=" << key << ", s=" << keyStr;
}

void QQnxScreenEventHandler::setScreenEventThread(QQnxScreenEventThread *eventThread)
{
    m_eventThread = eventThread;
    connect(m_eventThread, &QQnxScreenEventThread::eventsPending,
            this, &QQnxScreenEventHandler::processEvents);
}

void QQnxScreenEventHandler::processEvents()
{
    if (!m_eventThread)
        return;

    screen_event_t event = nullptr;
    if (screen_create_event(&event) != 0)
        return;

    int count = 0;
    for (;;) {
        if (screen_get_event(m_eventThread->context(), event, 0) != 0)
            break;

        int type = SCREEN_EVENT_NONE;
        screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &type);
        if (type == SCREEN_EVENT_NONE)
            break;

        ++count;
        qintptr result = 0;
        QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance();
        bool handled = dispatcher && dispatcher->filterNativeEvent(QByteArrayLiteral("screen_event_t"), event, &result);
        if (!handled)
            handleEvent(event);

        if (type == SCREEN_EVENT_CLOSE)
            finishCloseEvent(event);
    }

    m_eventThread->armEventsPending(count);
    screen_destroy_event(event);
}

void QQnxScreenEventHandler::handleKeyboardEvent(screen_event_t event)
{
    // get flags of key event
    int flags;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_FLAGS, &flags),
                        "Failed to query event flags");

    // get key code
    int sym;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_SYM, &sym),
                        "Failed to query event sym");

    int modifiers;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_MODIFIERS, &modifiers),
                        "Failed to query event modifieres");

    int scan;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_SCAN, &scan),
                        "Failed to query event scan");

    int cap;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_CAP, &cap),
                        "Failed to query event cap");

    int sequenceId = 0;
    bool inject = true;

    Q_FOREACH (QQnxScreenEventFilter *filter, m_eventFilters) {
        if (filter->handleKeyboardEvent(flags, sym, modifiers, scan, cap, sequenceId)) {
            inject = false;
            break;
        }
    }

    if (inject)
        injectKeyboardEvent(flags, sym, modifiers, scan, cap);
}

void QQnxScreenEventHandler::handlePointerEvent(screen_event_t event)
{
    errno = 0;

    // Query the window that was clicked
    screen_window_t qnxWindow;
    void *handle;
    Q_SCREEN_CHECKERROR(screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, &handle),
                        "Failed to query event window");

    qnxWindow = static_cast<screen_window_t>(handle);

    // Query the button states
    int buttonState = 0;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_BUTTONS, &buttonState),
                        "Failed to query event button state");

    // Query the window position
    int windowPos[2];
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, windowPos),
            "Failed to query event window position");

    // Query the screen position
    int pos[2];
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, pos),
                        "Failed to query event position");

    // Query the wheel delta
    int wheelDelta = 0;
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_iv(event, SCREEN_PROPERTY_MOUSE_WHEEL, &wheelDelta),
            "Failed to query event wheel delta");

    long long timestamp;
    Q_SCREEN_CHECKERROR(screen_get_event_property_llv(event, SCREEN_PROPERTY_TIMESTAMP, &timestamp),
                        "Failed to get timestamp");

    // Map window handle to top-level QWindow
    QWindow *w = QQnxIntegration::instance()->window(qnxWindow);

    // Generate enter and leave events as needed.
    if (qnxWindow != m_lastMouseWindow) {
        QWindow *wOld = QQnxIntegration::instance()->window(m_lastMouseWindow);

        if (wOld) {
            QWindowSystemInterface::handleLeaveEvent(wOld);
            qCDebug(lcQpaScreenEvents) << "Qt leave, w=" << wOld;
        }

        if (w) {
            QWindowSystemInterface::handleEnterEvent(w);
            qCDebug(lcQpaScreenEvents) << "Qt enter, w=" << w;
        }
    }

    m_lastMouseWindow = qnxWindow;

    // Apply scaling to wheel delta and invert value for Qt. We'll probably want to scale
    // this via a system preference at some point. But for now this is a sane value and makes
    // the wheel usable.
    wheelDelta *= -10;

    // convert point to local coordinates
    QPoint globalPoint(pos[0], pos[1]);
    QPoint localPoint(windowPos[0], windowPos[1]);

    // Convert buttons.
    // Some QNX header files invert 'Right Button versus "Left Button' ('Right' == 0x01). But they also offer a 'Button Swap' bit,
    // so we may receive events as shown. (If this is wrong, the fix is easy.)
    // QNX Button mask is 8 buttons wide, with a maximum value of x080.
    Qt::MouseButtons buttons = Qt::NoButton;
    if (buttonState & 0x01)
        buttons |= Qt::LeftButton;
    if (buttonState & 0x02)
        buttons |= Qt::MiddleButton;
    if (buttonState & 0x04)
        buttons |= Qt::RightButton;
    if (buttonState & 0x08)
        buttons |= Qt::ExtraButton1;    // AKA 'Qt::BackButton'
    if (buttonState & 0x10)
        buttons |= Qt::ExtraButton2;    // AKA 'Qt::ForwardButton'
    if (buttonState & 0x20)
        buttons |= Qt::ExtraButton3;
    if (buttonState & 0x40)
        buttons |= Qt::ExtraButton4;
    if (buttonState & 0x80)
        buttons |= Qt::ExtraButton5;

    if (w) {
        // Inject mouse event into Qt only if something has changed.
        if (m_lastGlobalMousePoint != globalPoint || m_lastLocalMousePoint != localPoint) {
            QWindowSystemInterface::handleMouseEvent(w, timestamp, m_mouseDevice, localPoint,
                                                     globalPoint, buttons, Qt::NoButton,
                                                     QEvent::MouseMove);
            qCDebug(lcQpaScreenEvents) << "Qt mouse move, w=" << w << ", (" << localPoint.x() << ","
                                       << localPoint.y() << "), b=" << static_cast<int>(buttons);
        }

        if (m_lastButtonState != buttons) {
            static const auto supportedButtons = { Qt::LeftButton,   Qt::MiddleButton,
                                                   Qt::RightButton,  Qt::ExtraButton1,
                                                   Qt::ExtraButton2, Qt::ExtraButton3,
                                                   Qt::ExtraButton4, Qt::ExtraButton5 };

            int releasedButtons = (m_lastButtonState ^ buttons) & ~buttons;
            for (auto button : supportedButtons) {
                if (releasedButtons & button) {
                    QWindowSystemInterface::handleMouseEvent(w, timestamp, m_mouseDevice,
                                                             localPoint, globalPoint, buttons,
                                                             button, QEvent::MouseButtonRelease);
                    qCDebug(lcQpaScreenEvents) << "Qt mouse release, w=" << w << ", (" << localPoint.x()
                                               << "," << localPoint.y() << "), b=" << button;
                }
            }

            if (m_lastButtonState != 0 && buttons == 0) {
                (static_cast<QQnxWindow *>(w->handle()))->handleActivationEvent();
            }

            int pressedButtons = (m_lastButtonState ^ buttons) & buttons;
            for (auto button : supportedButtons) {
                if (pressedButtons & button) {
                    QWindowSystemInterface::handleMouseEvent(w, timestamp, m_mouseDevice,
                                                             localPoint, globalPoint, buttons,
                                                             button, QEvent::MouseButtonPress);
                    qCDebug(lcQpaScreenEvents) << "Qt mouse press, w=" << w << ", (" << localPoint.x()
                                               << "," << localPoint.y() << "), b=" << button;
                }
            }
        }

        if (wheelDelta) {
            // Screen only supports a single wheel, so we will assume Vertical orientation for
            // now since that is pretty much standard.
            QPoint angleDelta(0, wheelDelta);
            QWindowSystemInterface::handleWheelEvent(w, timestamp, m_mouseDevice, localPoint,
                                                     globalPoint, QPoint(), angleDelta);
            qCDebug(lcQpaScreenEvents) << "Qt wheel, w=" << w << ", (" << localPoint.x() << ","
                                << localPoint.y() << "), d=" << static_cast<int>(wheelDelta);
        }
    }

    m_lastGlobalMousePoint = globalPoint;
    m_lastLocalMousePoint = localPoint;
    m_lastButtonState = buttons;
}

void QQnxScreenEventHandler::handleTouchEvent(screen_event_t event, int qnxType)
{
    // get display coordinates of touch
    int pos[2];
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, pos),
                        "Failed to query event position");

    QCursor::setPos(pos[0], pos[1]);

    // get window coordinates of touch
    int windowPos[2];
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, windowPos),
                        "Failed to query event window position");

    // determine which finger touched
    int touchId;
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_ID, &touchId),
                        "Failed to query event touch id");

    // determine which window was touched
    void *handle;
    Q_SCREEN_CHECKERROR(screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, &handle),
                        "Failed to query event window");

    errno = 0;
    int touchArea[2];
    Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_SIZE, touchArea),
                        "Failed to query event touch area");

    int touchPressure;
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_PRESSURE, &touchPressure),
            "Failed to query event touch pressure");

    screen_window_t qnxWindow = static_cast<screen_window_t>(handle);

    // check if finger is valid
    if (touchId < MaximumTouchPoints) {

        // Map window handle to top-level QWindow
        QWindow *w = QQnxIntegration::instance()->window(qnxWindow);

        // Generate enter and leave events as needed.
        if (qnxWindow != m_lastMouseWindow) {
            QWindow *wOld = QQnxIntegration::instance()->window(m_lastMouseWindow);

            if (wOld) {
                QWindowSystemInterface::handleLeaveEvent(wOld);
                qCDebug(lcQpaScreenEvents) << "Qt leave, w=" << wOld;
            }

            if (w) {
                QWindowSystemInterface::handleEnterEvent(w);
                qCDebug(lcQpaScreenEvents) << "Qt enter, w=" << w;
            }
        }
        m_lastMouseWindow = qnxWindow;

        if (w) {
            if (qnxType == SCREEN_EVENT_MTOUCH_RELEASE)
                (static_cast<QQnxWindow *>(w->handle()))->handleActivationEvent();

            // get size of screen which contains window
            QPlatformScreen *platformScreen = QPlatformScreen::platformScreenForWindow(w);
            QSizeF screenSize = platformScreen->geometry().size();

            // update cached position of current touch point
            m_touchPoints[touchId].normalPosition =
                            QPointF(static_cast<qreal>(pos[0]) / screenSize.width(),
                                    static_cast<qreal>(pos[1]) / screenSize.height());

            m_touchPoints[touchId].area = QRectF(w->geometry().left() + windowPos[0] - (touchArea[0]>>1),
                                                 w->geometry().top()  + windowPos[1] - (touchArea[1]>>1),
                                                 (touchArea[0]>>1), (touchArea[1]>>1));
            QWindow *parent = w->parent();
            while (parent) {
                m_touchPoints[touchId].area.translate(parent->geometry().topLeft());
                parent = parent->parent();
            }

            //Qt expects the pressure between 0 and 1. There is however no definite upper limit for
            //the integer value of touch event pressure. The 200 was determined by experiment, it
            //usually does not get higher than that.
            m_touchPoints[touchId].pressure = static_cast<qreal>(touchPressure)/200.0;
            // Can happen, because there is no upper limit for pressure
            if (m_touchPoints[touchId].pressure > 1)
                m_touchPoints[touchId].pressure = 1;

            // determine event type and update state of current touch point
            QEvent::Type type = QEvent::None;
            switch (qnxType) {
            case SCREEN_EVENT_MTOUCH_TOUCH:
                m_touchPoints[touchId].state = QEventPoint::State::Pressed;
                type = QEvent::TouchBegin;
                break;
            case SCREEN_EVENT_MTOUCH_MOVE:
                m_touchPoints[touchId].state = QEventPoint::State::Updated;
                type = QEvent::TouchUpdate;
                break;
            case SCREEN_EVENT_MTOUCH_RELEASE:
                m_touchPoints[touchId].state = QEventPoint::State::Released;
                type = QEvent::TouchEnd;
                break;
            }

            // build list of active touch points
            QList<QWindowSystemInterface::TouchPoint> pointList;
            for (int i = 0; i < MaximumTouchPoints; i++) {
                if (i == touchId) {
                    // current touch point is always active
                    pointList.append(m_touchPoints[i]);
                } else if (m_touchPoints[i].state != QEventPoint::State::Released) {
                    // finger is down but did not move
                    m_touchPoints[i].state = QEventPoint::State::Stationary;
                    pointList.append(m_touchPoints[i]);
                }
            }

            // inject event into Qt
            QWindowSystemInterface::handleTouchEvent(w, m_touchDevice, pointList);
            qCDebug(lcQpaScreenEvents) << "Qt touch, w =" << w
                                       << ", p=" << m_touchPoints[touchId].area.topLeft()
                                       << ", t=" << type;
        }
    }
}

void QQnxScreenEventHandler::handleCloseEvent(screen_event_t event)
{
    screen_window_t window = 0;
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, (void**)&window),
            "Failed to query window property");

    Q_EMIT windowClosed(window);

    // Map window handle to top-level QWindow
    QWindow *w = QQnxIntegration::instance()->window(window);
    if (w != 0)
        QWindowSystemInterface::handleCloseEvent(w);
}

void QQnxScreenEventHandler::handleCreateEvent(screen_event_t event)
{
    screen_window_t window = 0;
    int object_type = -1;

    Q_SCREEN_CHECKERROR(
        screen_get_event_property_iv(event, SCREEN_PROPERTY_OBJECT_TYPE, &object_type),
        "Failed to query object type for create event");

    switch (object_type) {
    // Other object types than window produces an unnessary warning, thus ignore 
    case SCREEN_OBJECT_TYPE_CONTEXT:
    case SCREEN_OBJECT_TYPE_GROUP:
    case SCREEN_OBJECT_TYPE_DISPLAY:
    case SCREEN_OBJECT_TYPE_DEVICE:
    case SCREEN_OBJECT_TYPE_PIXMAP:
    case SCREEN_OBJECT_TYPE_SESSION:
    case SCREEN_OBJECT_TYPE_STREAM:
        break;
    case SCREEN_OBJECT_TYPE_WINDOW:
    {
        Q_SCREEN_CHECKERROR(
            screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, (void**)&window),
            "Failed to query window property");

        Q_EMIT newWindowCreated(window);
        break;
    }
    default:
        qCDebug(lcQpaScreenEvents) << "Ignore create event for object type: " << object_type;
        break;
    }
}

void QQnxScreenEventHandler::handleDisplayEvent(screen_event_t event)
{
    screen_display_t nativeDisplay = 0;
    if (screen_get_event_property_pv(event, SCREEN_PROPERTY_DISPLAY, (void **)&nativeDisplay) != 0) {
        qWarning("QQnx: failed to query display property, errno=%d", errno);
        return;
    }

    int isAttached = 0;
    if (screen_get_event_property_iv(event, SCREEN_PROPERTY_ATTACHED, &isAttached) != 0) {
        qWarning("QQnx: failed to query display attached property, errno=%d", errno);
        return;
    }

    qCDebug(lcQpaScreenEvents) << "display attachment is now:" << isAttached;

    QQnxScreen *screen = m_qnxIntegration->screenForNative(nativeDisplay);

    if (!screen) {
        if (isAttached) {
            int val[2];
            screen_get_display_property_iv(nativeDisplay, SCREEN_PROPERTY_SIZE, val);
            if (val[0] == 0 && val[1] == 0) //If screen size is invalid, wait for the next event
                return;

            qCDebug(lcQpaScreenEvents) << "Creating new QQnxScreen for newly attached display";
            m_qnxIntegration->createDisplay(nativeDisplay, false /* not primary, we assume */);
        }
    } else if (!isAttached) {
        // We never remove the primary display, the qpa plugin doesn't support that and it crashes.
        // To support it, this would be needed:
        // - Adjust all qnx qpa code which uses screens
        // - Make QWidgetRepaintManager not dereference a null paint device
        // - Create platform resources ( QQnxWindow ) for all QWindow because they would be deleted
        //   when you delete the screen

        if (!screen->isPrimaryScreen()) {
            // libscreen display is deactivated, let's remove the QQnxScreen / QScreen
            qCDebug(lcQpaScreenEvents) << "Removing display";
            m_qnxIntegration->removeDisplay(screen);
        }
    }
}

void QQnxScreenEventHandler::handlePropertyEvent(screen_event_t event)
{
    errno = 0;
    int objectType;
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_iv(event, SCREEN_PROPERTY_OBJECT_TYPE, &objectType),
            "Failed to query object type property");

    if (objectType != SCREEN_OBJECT_TYPE_WINDOW)
        return;

    errno = 0;
    screen_window_t window = 0;
    if (Q_UNLIKELY(screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, (void**)&window) != 0))
        qFatal("QQnx: failed to query window property, errno=%d", errno);

    if (window == 0) {
        qCDebug(lcQpaScreenEvents) << "handlePositionEvent on NULL window";
        return;
    }

    errno = 0;
    int property;
    if (Q_UNLIKELY(screen_get_event_property_iv(event, SCREEN_PROPERTY_NAME, &property) != 0))
        qWarning("QQnx: failed to query window property, errno=%d", errno);

    switch (property) {
    case SCREEN_PROPERTY_FOCUS:
        handleKeyboardFocusPropertyEvent(window);
        break;
    case SCREEN_PROPERTY_SIZE:
    case SCREEN_PROPERTY_POSITION:
        handleGeometryPropertyEvent(window);
        break;
    default:
        // event ignored
        qCDebug(lcQpaScreenEvents) << "Ignore property event for property: " << property;
    }
}

void QQnxScreenEventHandler::handleKeyboardFocusPropertyEvent(screen_window_t window)
{
    errno = 0;
    int focus = 0;
    if (Q_UNLIKELY(window && screen_get_window_property_iv(window, SCREEN_PROPERTY_FOCUS, &focus) != 0))
        qWarning("QQnx: failed to query keyboard focus property, errno=%d", errno);

    QWindow *focusWindow = QQnxIntegration::instance()->window(window);

    m_focusLostTimer.stop();

    if (focus && focusWindow != QGuiApplication::focusWindow())
        QWindowSystemInterface::handleFocusWindowChanged(focusWindow, Qt::ActiveWindowFocusReason);
    else if (!focus && focusWindow == QGuiApplication::focusWindow())
        m_focusLostTimer.start(50ms, this);
}

void QQnxScreenEventHandler::handleGeometryPropertyEvent(screen_window_t window)
{
    int pos[2];
    if (screen_get_window_property_iv(window, SCREEN_PROPERTY_POSITION, pos) != 0) {
        qWarning("QQnx: failed to query window property, errno=%d", errno);
        return;
    }

    int size[2];
    if (screen_get_window_property_iv(window, SCREEN_PROPERTY_SIZE, size) != 0) {
        qWarning("QQnx: failed to query window property, errno=%d", errno);
        return;
    }

    QRect rect(pos[0], pos[1], size[0], size[1]);
    QWindow *qtWindow = QQnxIntegration::instance()->window(window);
    if (qtWindow) {
        qtWindow->setGeometry(rect);
        QWindowSystemInterface::handleGeometryChange(qtWindow, rect);
    }

    qCDebug(lcQpaScreenEvents) << qtWindow << "moved to" << rect;
}

void QQnxScreenEventHandler::timerEvent(QTimerEvent *event)
{
    if (event->id() == m_focusLostTimer.id()) {
        m_focusLostTimer.stop();
        event->accept();
    } else {
        QObject::timerEvent(event);
    }
}

QT_END_NAMESPACE

void QQnxScreenEventHandler::handleManagerEvent(screen_event_t event)
{
    errno = 0;
    int subtype;
    Q_SCREEN_CHECKERROR(
            screen_get_event_property_iv(event, SCREEN_PROPERTY_SUBTYPE, &subtype),
            "Failed to query object type property");

    errno = 0;
    screen_window_t window = 0;
    if (screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, (void**)&window) != 0)
        qFatal("QQnx: failed to query window property, errno=%d", errno);

    switch (subtype) {
    case SCREEN_EVENT_CLOSE: {
        QWindow *closeWindow = QQnxIntegration::instance()->window(window);
        closeWindow->close();
        break;
    }

    default:
        // event ignored
        qCDebug(lcQpaScreenEvents) << "Ignore manager event for subtype: " << subtype;
    }
}

#include "moc_qqnxscreeneventhandler.cpp"
