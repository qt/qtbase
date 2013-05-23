/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqnxscreeneventhandler.h"
#include "qqnxintegration.h"
#include "qqnxkeytranslator.h"
#include "qqnxscreen.h"

#include <QDebug>
#include <QGuiApplication>

#include <errno.h>
#include <sys/keycodes.h>

#if defined(QQNXSCREENEVENT_DEBUG)
#define qScreenEventDebug qDebug
#else
#define qScreenEventDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxScreenEventHandler::QQnxScreenEventHandler(QQnxIntegration *integration)
    : m_qnxIntegration(integration)
    , m_lastButtonState(Qt::NoButton)
    , m_lastMouseWindow(0)
    , m_touchDevice(0)
{
    // Create a touch device
    m_touchDevice = new QTouchDevice;
    m_touchDevice->setType(QTouchDevice::TouchScreen);
    m_touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::Pressure | QTouchDevice::NormalizedPosition);
    QWindowSystemInterface::registerTouchDevice(m_touchDevice);

    // initialize array of touch points
    for (int i = 0; i < MaximumTouchPoints; i++) {

        // map array index to id
        m_touchPoints[i].id = i;

        // pressure is not supported - use default
        m_touchPoints[i].pressure = 1.0;

        // nothing touching
        m_touchPoints[i].state = Qt::TouchPointReleased;
    }
}

bool QQnxScreenEventHandler::handleEvent(screen_event_t event)
{
    // get the event type
    errno = 0;
    int qnxType;
    int result = screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &qnxType);
    if (result)
        qFatal("QQNX: failed to query event type, errno=%d", errno);

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

    default:
        // event ignored
        qScreenEventDebug() << Q_FUNC_INFO << "unknown event" << qnxType;
        return false;
    }

    return true;
}

void QQnxScreenEventHandler::injectKeyboardEvent(int flags, int sym, int modifiers, int scan, int cap)
{
    Q_UNUSED(scan);

    Qt::KeyboardModifiers qtMod = Qt::NoModifier;
    if (modifiers & KEYMOD_SHIFT)
        qtMod |= Qt::ShiftModifier;
    if (modifiers & KEYMOD_CTRL)
        qtMod |= Qt::ControlModifier;
    if (modifiers & KEYMOD_ALT)
        qtMod |= Qt::AltModifier;

    // determine event type
    QEvent::Type type = (flags & KEY_DOWN) ? QEvent::KeyPress : QEvent::KeyRelease;

    // Check if the key cap is valid
    if (flags & KEY_CAP_VALID) {
        Qt::Key key;
        QString keyStr;

        if (cap >= 0x20 && cap <= 0x0ff) {
            key = Qt::Key(std::toupper(cap));   // Qt expects the CAP to be upper case.

            if ( qtMod & Qt::ControlModifier ) {
                keyStr = QChar((int)(key & 0x3f));
            } else {
                if (flags & KEY_SYM_VALID)
                    keyStr = QChar(sym);
            }
        } else if ((cap > 0x0ff && cap < UNICODE_PRIVATE_USE_AREA_FIRST) || cap > UNICODE_PRIVATE_USE_AREA_LAST) {
            key = (Qt::Key)cap;
            keyStr = QChar(sym);
        } else {
            if (isKeypadKey(cap))
                qtMod |= Qt::KeypadModifier; // Is this right?
            key = keyTranslator(cap);
        }

        QWindowSystemInterface::handleExtendedKeyEvent(QGuiApplication::focusWindow(), type, key, qtMod,
                scan, sym, modifiers, keyStr);
        qScreenEventDebug() << Q_FUNC_INFO << "Qt key t=" << type << ", k=" << key << ", s=" << keyStr;
    }
}

void QQnxScreenEventHandler::handleKeyboardEvent(screen_event_t event)
{
    // get flags of key event
    errno = 0;
    int flags;
    int result = screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_FLAGS, &flags);
    if (result)
        qFatal("QQNX: failed to query event flags, errno=%d", errno);

    // get key code
    errno = 0;
    int sym;
    result = screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_SYM, &sym);
    if (result)
        qFatal("QQNX: failed to query event sym, errno=%d", errno);

    int modifiers;
    result = screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_MODIFIERS, &modifiers);
    if (result)
        qFatal("QQNX: failed to query event modifiers, errno=%d", errno);

    int scan;
    result = screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_SCAN, &scan);
    if (result)
        qFatal("QQNX: failed to query event modifiers, errno=%d", errno);

    int cap;
    result = screen_get_event_property_iv(event, SCREEN_PROPERTY_KEY_CAP, &cap);
    if (result)
        qFatal("QQNX: failed to query event cap, errno=%d", errno);

    injectKeyboardEvent(flags, sym, modifiers, scan, cap);
}

void QQnxScreenEventHandler::handlePointerEvent(screen_event_t event)
{
    errno = 0;

    // Query the window that was clicked
    screen_window_t qnxWindow;
    void *handle;
    int result = screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, &handle);
    if (result)
        qFatal("QQNX: failed to query event window, errno=%d", errno);

    qnxWindow = static_cast<screen_window_t>(handle);

    // Query the button states
    int buttonState = 0;
    result = screen_get_event_property_iv(event, SCREEN_PROPERTY_BUTTONS, &buttonState);
    if (result)
        qFatal("QQNX: failed to query event button state, errno=%d", errno);

    // Query the window position
    int windowPos[2];
    result = screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, windowPos);
    if (result)
        qFatal("QQNX: failed to query event window position, errno=%d", errno);

    // Query the screen position
    int pos[2];
    result = screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, pos);
    if (result)
        qFatal("QQNX: failed to query event position, errno=%d", errno);

    // Query the wheel delta
    int wheelDelta = 0;
    result = screen_get_event_property_iv(event, SCREEN_PROPERTY_MOUSE_WHEEL, &wheelDelta);
    if (result)
        qFatal("QQNX: failed to query event wheel delta, errno=%d", errno);

    // Map window handle to top-level QWindow
    QWindow *w = QQnxIntegration::window(qnxWindow);

    // Generate enter and leave events as needed.
    if (qnxWindow != m_lastMouseWindow) {
        QWindow *wOld = QQnxIntegration::window(m_lastMouseWindow);

        if (wOld) {
            QWindowSystemInterface::handleLeaveEvent(wOld);
            qScreenEventDebug() << Q_FUNC_INFO << "Qt leave, w=" << wOld;
        }

        if (w) {
            QWindowSystemInterface::handleEnterEvent(w);
            qScreenEventDebug() << Q_FUNC_INFO << "Qt enter, w=" << w;
        }
    }

    // If we don't have a navigator, we don't get activation events.
    if (buttonState && w && w != QGuiApplication::focusWindow() && !m_qnxIntegration->supportsNavigatorEvents())
        QWindowSystemInterface::handleWindowActivated(w);

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
        buttons |= Qt::MidButton;
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
        if (m_lastGlobalMousePoint != globalPoint ||
            m_lastLocalMousePoint != localPoint ||
            m_lastButtonState != buttons) {
            QWindowSystemInterface::handleMouseEvent(w, localPoint, globalPoint, buttons);
            qScreenEventDebug() << Q_FUNC_INFO << "Qt mouse, w=" << w << ", (" << localPoint.x() << "," << localPoint.y() << "), b=" << static_cast<int>(buttons);
        }

        if (wheelDelta) {
            // Screen only supports a single wheel, so we will assume Vertical orientation for
            // now since that is pretty much standard.
            QWindowSystemInterface::handleWheelEvent(w, localPoint, globalPoint, wheelDelta, Qt::Vertical);
            qScreenEventDebug() << Q_FUNC_INFO << "Qt wheel, w=" << w << ", (" << localPoint.x() << "," << localPoint.y() << "), d=" << static_cast<int>(wheelDelta);
        }
    }

    m_lastGlobalMousePoint = globalPoint;
    m_lastLocalMousePoint = localPoint;
    m_lastButtonState = buttons;
}

void QQnxScreenEventHandler::handleTouchEvent(screen_event_t event, int qnxType)
{
    // get display coordinates of touch
    errno = 0;
    int pos[2];
    int result = screen_get_event_property_iv(event, SCREEN_PROPERTY_POSITION, pos);
    if (result)
        qFatal("QQNX: failed to query event position, errno=%d", errno);

    QCursor::setPos(pos[0], pos[1]);

    // get window coordinates of touch
    errno = 0;
    int windowPos[2];
    result = screen_get_event_property_iv(event, SCREEN_PROPERTY_SOURCE_POSITION, windowPos);
    if (result)
        qFatal("QQNX: failed to query event window position, errno=%d", errno);

    // determine which finger touched
    errno = 0;
    int touchId;
    result = screen_get_event_property_iv(event, SCREEN_PROPERTY_TOUCH_ID, &touchId);
    if (result)
        qFatal("QQNX: failed to query event touch id, errno=%d", errno);

    // determine which window was touched
    errno = 0;
    void *handle;
    result = screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, &handle);
    if (result)
        qFatal("QQNX: failed to query event window, errno=%d", errno);

    screen_window_t qnxWindow = static_cast<screen_window_t>(handle);

    // check if finger is valid
    if (touchId < MaximumTouchPoints) {

        // Map window handle to top-level QWindow
        QWindow *w = QQnxIntegration::window(qnxWindow);

        // Generate enter and leave events as needed.
        if (qnxWindow != m_lastMouseWindow) {
            QWindow *wOld = QQnxIntegration::window(m_lastMouseWindow);

            if (wOld) {
                QWindowSystemInterface::handleLeaveEvent(wOld);
                qScreenEventDebug() << Q_FUNC_INFO << "Qt leave, w=" << wOld;
            }

            if (w) {
                QWindowSystemInterface::handleEnterEvent(w);
                qScreenEventDebug() << Q_FUNC_INFO << "Qt enter, w=" << w;
            }
        }
        m_lastMouseWindow = qnxWindow;

        if (w) {
            // get size of screen which contains window
            QPlatformScreen *platformScreen = QPlatformScreen::platformScreenForWindow(w);
            QSizeF screenSize = platformScreen->physicalSize();

            // update cached position of current touch point
            m_touchPoints[touchId].normalPosition = QPointF( static_cast<qreal>(pos[0]) / screenSize.width(), static_cast<qreal>(pos[1]) / screenSize.height() );
            m_touchPoints[touchId].area = QRectF( pos[0], pos[1], 0.0, 0.0 );

            // determine event type and update state of current touch point
            QEvent::Type type = QEvent::None;
            switch (qnxType) {
            case SCREEN_EVENT_MTOUCH_TOUCH:
                m_touchPoints[touchId].state = Qt::TouchPointPressed;
                type = QEvent::TouchBegin;
                break;
            case SCREEN_EVENT_MTOUCH_MOVE:
                m_touchPoints[touchId].state = Qt::TouchPointMoved;
                type = QEvent::TouchUpdate;
                break;
            case SCREEN_EVENT_MTOUCH_RELEASE:
                m_touchPoints[touchId].state = Qt::TouchPointReleased;
                type = QEvent::TouchEnd;
                break;
            }

            // build list of active touch points
            QList<QWindowSystemInterface::TouchPoint> pointList;
            for (int i = 0; i < MaximumTouchPoints; i++) {
                if (i == touchId) {
                    // current touch point is always active
                    pointList.append(m_touchPoints[i]);
                } else if (m_touchPoints[i].state != Qt::TouchPointReleased) {
                    // finger is down but did not move
                    m_touchPoints[i].state = Qt::TouchPointStationary;
                    pointList.append(m_touchPoints[i]);
                }
            }

            // inject event into Qt
            QWindowSystemInterface::handleTouchEvent(w, m_touchDevice, pointList);
            qScreenEventDebug() << Q_FUNC_INFO << "Qt touch, w =" << w
                                << ", p=(" << pos[0] << "," << pos[1]
                                << "), t=" << type;
        }
    }
}

void QQnxScreenEventHandler::handleCloseEvent(screen_event_t event)
{
    screen_window_t window = 0;
    if (screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, (void**)&window) != 0)
        qFatal("QQnx: failed to query window property, errno=%d", errno);

    Q_EMIT windowClosed(window);

    // Map window handle to top-level QWindow
    QWindow *w = QQnxIntegration::window(window);
    if (w != 0)
        QWindowSystemInterface::handleCloseEvent(w);
}

void QQnxScreenEventHandler::handleCreateEvent(screen_event_t event)
{
    screen_window_t window = 0;
    if (screen_get_event_property_pv(event, SCREEN_PROPERTY_WINDOW, (void**)&window) != 0)
        qFatal("QQnx: failed to query window property, errno=%d", errno);

    Q_EMIT newWindowCreated(window);
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

    qScreenEventDebug() << Q_FUNC_INFO << "display attachment is now:" << isAttached;
    QQnxScreen *screen = m_qnxIntegration->screenForNative(nativeDisplay);
    if (!screen) {
        if (isAttached) {
            qScreenEventDebug() << "creating new QQnxScreen for newly attached display";
            m_qnxIntegration->createDisplay(nativeDisplay, false /* not primary, we assume */);
        }
    } else if (!isAttached) {
        // We never remove the primary display, the qpa plugin doesn't support that and it crashes.
        // To support it, this would be needed:
        // - Adjust all qnx qpa code which uses screens
        // - Make QWidgetBackingStore not dereference a null paint device
        // - Create platform resources ( QQnxWindow ) for all QWindow because they would be deleted
        //   when you delete the screen

        if (!screen->isPrimaryScreen()) {
            // libscreen display is deactivated, let's remove the QQnxScreen / QScreen
            qScreenEventDebug() << "removing display";
            m_qnxIntegration->removeDisplay(screen);
        }
    }
}

#include "moc_qqnxscreeneventhandler.cpp"

QT_END_NAMESPACE
