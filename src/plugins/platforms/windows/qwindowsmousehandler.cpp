/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwindowsmousehandler.h"
#include "qwindowskeymapper.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsintegration.h"
#include "qwindowsscreen.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>
#include <QtGui/qtouchdevice.h>
#include <QtGui/qwindow.h>
#include <QtGui/qcursor.h>

#include <QtCore/qdebug.h>
#include <QtCore/qscopedpointer.h>

#include <windowsx.h>

QT_BEGIN_NAMESPACE

static inline void compressMouseMove(MSG *msg)
{
    // Compress mouse move events
    if (msg->message == WM_MOUSEMOVE) {
        MSG mouseMsg;
        while (PeekMessage(&mouseMsg, msg->hwnd, WM_MOUSEFIRST,
                           WM_MOUSELAST, PM_NOREMOVE)) {
            if (mouseMsg.message == WM_MOUSEMOVE) {
#define PEEKMESSAGE_IS_BROKEN 1
#ifdef PEEKMESSAGE_IS_BROKEN
                // Since the Windows PeekMessage() function doesn't
                // correctly return the wParam for WM_MOUSEMOVE events
                // if there is a key release event in the queue
                // _before_ the mouse event, we have to also consider
                // key release events (kls 2003-05-13):
                MSG keyMsg;
                bool done = false;
                while (PeekMessage(&keyMsg, nullptr, WM_KEYFIRST, WM_KEYLAST,
                                   PM_NOREMOVE)) {
                    if (keyMsg.time < mouseMsg.time) {
                        if ((keyMsg.lParam & 0xC0000000) == 0x40000000) {
                            PeekMessage(&keyMsg, nullptr, keyMsg.message,
                                        keyMsg.message, PM_REMOVE);
                        } else {
                            done = true;
                            break;
                        }
                    } else {
                        break; // no key event before the WM_MOUSEMOVE event
                    }
                }
                if (done)
                    break;
#else
                // Actually the following 'if' should work instead of
                // the above key event checking, but apparently
                // PeekMessage() is broken :-(
                if (mouseMsg.wParam != msg.wParam)
                    break; // leave the message in the queue because
                // the key state has changed
#endif
                // Update the passed in MSG structure with the
                // most recent one.
                msg->lParam = mouseMsg.lParam;
                msg->wParam = mouseMsg.wParam;
                // Extract the x,y coordinates from the lParam as we do in the WndProc
                msg->pt.x = GET_X_LPARAM(mouseMsg.lParam);
                msg->pt.y = GET_Y_LPARAM(mouseMsg.lParam);
                clientToScreen(msg->hwnd, &(msg->pt));
                // Remove the mouse move message
                PeekMessage(&mouseMsg, msg->hwnd, WM_MOUSEMOVE,
                            WM_MOUSEMOVE, PM_REMOVE);
            } else {
                break; // there was no more WM_MOUSEMOVE event
            }
        }
    }
}

static inline QTouchDevice *createTouchDevice()
{
    const int digitizers = GetSystemMetrics(SM_DIGITIZER);
    if (!(digitizers & (NID_INTEGRATED_TOUCH | NID_EXTERNAL_TOUCH)))
        return nullptr;
    const int tabletPc = GetSystemMetrics(SM_TABLETPC);
    const int maxTouchPoints = GetSystemMetrics(SM_MAXIMUMTOUCHES);
    qCDebug(lcQpaEvents) << "Digitizers:" << Qt::hex << Qt::showbase << (digitizers & ~NID_READY)
        << "Ready:" << (digitizers & NID_READY) << Qt::dec << Qt::noshowbase
        << "Tablet PC:" << tabletPc << "Max touch points:" << maxTouchPoints;
    auto *result = new QTouchDevice;
    result->setType(digitizers & NID_INTEGRATED_TOUCH
                    ? QTouchDevice::TouchScreen : QTouchDevice::TouchPad);
    QTouchDevice::Capabilities capabilities = QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::NormalizedPosition;
    if (result->type() == QTouchDevice::TouchPad)
        capabilities |= QTouchDevice::MouseEmulation;
    result->setCapabilities(capabilities);
    result->setMaximumTouchPoints(maxTouchPoints);
    return result;
}

/*!
    \class QWindowsMouseHandler
    \brief Windows mouse handler

    Dispatches mouse and touch events. Separate for code cleanliness.

    \internal
*/

QWindowsMouseHandler::QWindowsMouseHandler() = default;

QTouchDevice *QWindowsMouseHandler::ensureTouchDevice()
{
    if (!m_touchDevice)
        m_touchDevice = createTouchDevice();
    return m_touchDevice;
}

void QWindowsMouseHandler::clearEvents()
{
    m_lastEventType = QEvent::None;
    m_lastEventButton = Qt::NoButton;
}

Qt::MouseButtons QWindowsMouseHandler::queryMouseButtons()
{
    Qt::MouseButtons result;
    const bool mouseSwapped = GetSystemMetrics(SM_SWAPBUTTON);
    if (GetAsyncKeyState(VK_LBUTTON) < 0)
        result |= mouseSwapped ? Qt::RightButton: Qt::LeftButton;
    if (GetAsyncKeyState(VK_RBUTTON) < 0)
        result |= mouseSwapped ? Qt::LeftButton : Qt::RightButton;
    if (GetAsyncKeyState(VK_MBUTTON) < 0)
        result |= Qt::MiddleButton;
    if (GetAsyncKeyState(VK_XBUTTON1) < 0)
        result |= Qt::XButton1;
    if (GetAsyncKeyState(VK_XBUTTON2) < 0)
        result |= Qt::XButton2;
    return result;
}

static QPoint lastMouseMovePos;

namespace {
struct MouseEvent {
    QEvent::Type type;
    Qt::MouseButton button;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const MouseEvent &e)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "MouseEvent(" << e.type << ", " << e.button << ')';
    return d;
}
#endif // QT_NO_DEBUG_STREAM
} // namespace

static inline Qt::MouseButton extraButton(WPARAM wParam) // for WM_XBUTTON...
{
    return GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? Qt::BackButton : Qt::ForwardButton;
}

static inline MouseEvent eventFromMsg(const MSG &msg)
{
    switch (msg.message) {
    case WM_MOUSEMOVE:
        return {QEvent::MouseMove, Qt::NoButton};
    case WM_LBUTTONDOWN:
        return {QEvent::MouseButtonPress, Qt::LeftButton};
    case WM_LBUTTONUP:
        return {QEvent::MouseButtonRelease, Qt::LeftButton};
    case WM_LBUTTONDBLCLK: // Qt QPA does not handle double clicks, send as press
        return {QEvent::MouseButtonPress, Qt::LeftButton};
    case WM_MBUTTONDOWN:
        return {QEvent::MouseButtonPress, Qt::MiddleButton};
    case WM_MBUTTONUP:
        return {QEvent::MouseButtonRelease, Qt::MiddleButton};
    case WM_MBUTTONDBLCLK:
        return {QEvent::MouseButtonPress, Qt::MiddleButton};
    case WM_RBUTTONDOWN:
        return {QEvent::MouseButtonPress, Qt::RightButton};
    case WM_RBUTTONUP:
        return {QEvent::MouseButtonRelease, Qt::RightButton};
    case WM_RBUTTONDBLCLK:
        return {QEvent::MouseButtonPress, Qt::RightButton};
    case WM_XBUTTONDOWN:
        return {QEvent::MouseButtonPress, extraButton(msg.wParam)};
    case WM_XBUTTONUP:
        return {QEvent::MouseButtonRelease, extraButton(msg.wParam)};
    case WM_XBUTTONDBLCLK:
        return {QEvent::MouseButtonPress, extraButton(msg.wParam)};
    case WM_NCMOUSEMOVE:
        return {QEvent::NonClientAreaMouseMove, Qt::NoButton};
    case WM_NCLBUTTONDOWN:
        return {QEvent::NonClientAreaMouseButtonPress, Qt::LeftButton};
    case WM_NCLBUTTONUP:
        return {QEvent::NonClientAreaMouseButtonRelease, Qt::LeftButton};
    case WM_NCLBUTTONDBLCLK:
        return {QEvent::NonClientAreaMouseButtonPress, Qt::LeftButton};
    case WM_NCMBUTTONDOWN:
        return {QEvent::NonClientAreaMouseButtonPress, Qt::MiddleButton};
    case WM_NCMBUTTONUP:
        return {QEvent::NonClientAreaMouseButtonRelease, Qt::MiddleButton};
    case WM_NCMBUTTONDBLCLK:
        return {QEvent::NonClientAreaMouseButtonPress, Qt::MiddleButton};
    case WM_NCRBUTTONDOWN:
        return {QEvent::NonClientAreaMouseButtonPress, Qt::RightButton};
    case WM_NCRBUTTONUP:
        return {QEvent::NonClientAreaMouseButtonRelease, Qt::RightButton};
    case WM_NCRBUTTONDBLCLK:
        return {QEvent::NonClientAreaMouseButtonPress, Qt::RightButton};
    default: // WM_MOUSELEAVE
        break;
    }
    return {QEvent::None, Qt::NoButton};
}

bool QWindowsMouseHandler::translateMouseEvent(QWindow *window, HWND hwnd,
                                               QtWindows::WindowsEventType et,
                                               MSG msg, LRESULT *result)
{
    enum : quint64 { signatureMask = 0xffffff00, miWpSignature = 0xff515700 };

    if (et == QtWindows::MouseWheelEvent)
        return translateMouseWheelEvent(window, hwnd, msg, result);

    QPoint winEventPosition(GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
    if ((et & QtWindows::NonClientEventFlag) == 0 && QWindowsBaseWindow::isRtlLayout(hwnd))  {
        RECT clientArea;
        GetClientRect(hwnd, &clientArea);
        winEventPosition.setX(clientArea.right - winEventPosition.x());
    }

    QPoint clientPosition;
    QPoint globalPosition;
    if (et & QtWindows::NonClientEventFlag) {
        globalPosition = winEventPosition;
        clientPosition = QWindowsGeometryHint::mapFromGlobal(hwnd, globalPosition);
    } else {
        clientPosition = winEventPosition;
        globalPosition = QWindowsGeometryHint::mapToGlobal(hwnd, winEventPosition);
    }

    // Windows sends a mouse move with no buttons pressed to signal "Enter"
    // when a window is shown over the cursor. Discard the event and only use
    // it for generating QEvent::Enter to be consistent with other platforms -
    // X11 and macOS.
    bool discardEvent = false;
    if (msg.message == WM_MOUSEMOVE) {
        const bool samePosition = globalPosition == lastMouseMovePos;
        lastMouseMovePos = globalPosition;
        if (msg.wParam == 0 && (m_windowUnderMouse.isNull() || samePosition))
            discardEvent = true;
    }

    Qt::MouseEventSource source = Qt::MouseEventNotSynthesized;

    // Check for events synthesized from touch. Lower byte is touch index, 0 means pen.
    static const bool passSynthesizedMouseEvents =
            !(QWindowsIntegration::instance()->options() & QWindowsIntegration::DontPassOsMouseEventsSynthesizedFromTouch);
    // Check for events synthesized from touch. Lower 7 bits are touch/pen index, bit 8 indicates touch.
    // However, when tablet support is active, extraInfo is a packet serial number. This is not a problem
    // since we do not want to ignore mouse events coming from a tablet.
    // See https://msdn.microsoft.com/en-us/library/windows/desktop/ms703320.aspx
    const auto extraInfo = quint64(GetMessageExtraInfo());
    if ((extraInfo & signatureMask) == miWpSignature) {
        if (extraInfo & 0x80) { // Bit 7 indicates touch event, else tablet pen.
            source = Qt::MouseEventSynthesizedBySystem;
            if (!passSynthesizedMouseEvents)
                return false;
        }
    }

    const Qt::KeyboardModifiers keyModifiers = QWindowsKeyMapper::queryKeyboardModifiers();
    const MouseEvent mouseEvent = eventFromMsg(msg);
    Qt::MouseButtons buttons;

    if (mouseEvent.type >= QEvent::NonClientAreaMouseMove && mouseEvent.type <= QEvent::NonClientAreaMouseButtonDblClick)
        buttons = queryMouseButtons();
    else
        buttons = keyStateToMouseButtons(msg.wParam);

    // When the left/right mouse buttons are pressed over the window title bar
    // WM_NCLBUTTONDOWN/WM_NCRBUTTONDOWN messages are received. But no UP
    // messages are received on release, only WM_NCMOUSEMOVE/WM_MOUSEMOVE.
    // We detect it and generate the missing release events here. (QTBUG-75678)
    // The last event vars are cleared on QWindowsContext::handleExitSizeMove()
    // to avoid generating duplicated release events.
    if (m_lastEventType == QEvent::NonClientAreaMouseButtonPress
            && (mouseEvent.type == QEvent::NonClientAreaMouseMove || mouseEvent.type == QEvent::MouseMove)
            && (m_lastEventButton & buttons) == 0) {
            if (mouseEvent.type == QEvent::NonClientAreaMouseMove) {
                QWindowSystemInterface::handleFrameStrutMouseEvent(window, clientPosition, globalPosition, buttons, m_lastEventButton,
                                                                   QEvent::NonClientAreaMouseButtonRelease, keyModifiers, source);
            } else {
                QWindowSystemInterface::handleMouseEvent(window, clientPosition, globalPosition, buttons, m_lastEventButton,
                                                         QEvent::MouseButtonRelease, keyModifiers, source);
            }
    }
    m_lastEventType = mouseEvent.type;
    m_lastEventButton = mouseEvent.button;

    if (mouseEvent.type >= QEvent::NonClientAreaMouseMove && mouseEvent.type <= QEvent::NonClientAreaMouseButtonDblClick) {
        QWindowSystemInterface::handleFrameStrutMouseEvent(window, clientPosition,
                                                           globalPosition, buttons,
                                                           mouseEvent.button, mouseEvent.type,
                                                           keyModifiers, source);
        return false; // Allow further event processing (dragging of windows).
    }

    *result = 0;
    if (msg.message == WM_MOUSELEAVE) {
        qCDebug(lcQpaEvents) << mouseEvent << "for" << window << "previous window under mouse="
            << m_windowUnderMouse << "tracked window=" << m_trackedWindow;

        // When moving out of a window, WM_MOUSEMOVE within the moved-to window is received first,
        // so if m_trackedWindow is not the window here, it means the cursor has left the
        // application.
        if (window == m_trackedWindow) {
            QWindow *leaveTarget = m_windowUnderMouse ? m_windowUnderMouse : m_trackedWindow;
            qCDebug(lcQpaEvents) << "Generating leave event for " << leaveTarget;
            QWindowSystemInterface::handleLeaveEvent(leaveTarget);
            m_trackedWindow = nullptr;
            m_windowUnderMouse = nullptr;
        }
        return true;
    }

    auto *platformWindow = static_cast<QWindowsWindow *>(window->handle());

    // If the window was recently resized via mouse doubleclick on the frame or title bar,
    // we don't get WM_LBUTTONDOWN or WM_LBUTTONDBLCLK for the second click,
    // but we will get at least one WM_MOUSEMOVE with left button down and the WM_LBUTTONUP,
    // which will result undesired mouse press and release events.
    // To avoid those, we ignore any events with left button down if we didn't
    // get the original WM_LBUTTONDOWN/WM_LBUTTONDBLCLK.
    if (msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONDBLCLK) {
        m_leftButtonDown = true;
    } else {
        const bool actualLeftDown = buttons & Qt::LeftButton;
        if (!m_leftButtonDown && actualLeftDown) {
            // Autocapture the mouse for current window to and ignore further events until release.
            // Capture is necessary so we don't get WM_MOUSELEAVEs to confuse matters.
            // This autocapture is released normally when button is released.
            if (!platformWindow->hasMouseCapture()) {
                platformWindow->applyCursor();
                platformWindow->setMouseGrabEnabled(true);
                platformWindow->setFlag(QWindowsWindow::AutoMouseCapture);
                qCDebug(lcQpaEvents) << "Automatic mouse capture for missing buttondown event" << window;
            }
            m_previousCaptureWindow = window;
            return true;
        }
        if (m_leftButtonDown && !actualLeftDown)
            m_leftButtonDown = false;
    }

    // In this context, neither an invisible nor a transparent window (transparent regarding mouse
    // events, "click-through") can be considered as the window under mouse.
    QWindow *currentWindowUnderMouse = platformWindow->hasMouseCapture() ?
        QWindowsScreen::windowAt(globalPosition, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT) : window;
    while (currentWindowUnderMouse && currentWindowUnderMouse->flags() & Qt::WindowTransparentForInput)
        currentWindowUnderMouse = currentWindowUnderMouse->parent();
    // QTBUG-44332: When Qt is running at low integrity level and
    // a Qt Window is parented on a Window of a higher integrity process
    // using QWindow::fromWinId() (for example, Qt running in a browser plugin)
    // ChildWindowFromPointEx() may not find the Qt window (failing with ERROR_ACCESS_DENIED)
    if (!currentWindowUnderMouse) {
        const QRect clientRect(QPoint(0, 0), window->size());
        if (clientRect.contains(winEventPosition))
            currentWindowUnderMouse = window;
    }

    compressMouseMove(&msg);
    // Qt expects the platform plugin to capture the mouse on
    // any button press until release.
    if (!platformWindow->hasMouseCapture()
        && (mouseEvent.type == QEvent::MouseButtonPress || mouseEvent.type == QEvent::MouseButtonDblClick)) {
        platformWindow->setMouseGrabEnabled(true);
        platformWindow->setFlag(QWindowsWindow::AutoMouseCapture);
        qCDebug(lcQpaEvents) << "Automatic mouse capture " << window;
        // Implement "Click to focus" for native child windows (unless it is a native widget window).
        if (!window->isTopLevel() && !window->inherits("QWidgetWindow") && QGuiApplication::focusWindow() != window)
            window->requestActivate();
    } else if (platformWindow->hasMouseCapture()
               && platformWindow->testFlag(QWindowsWindow::AutoMouseCapture)
               && mouseEvent.type == QEvent::MouseButtonRelease
               && !buttons) {
        platformWindow->setMouseGrabEnabled(false);
        qCDebug(lcQpaEvents) << "Releasing automatic mouse capture " << window;
    }

    const bool hasCapture = platformWindow->hasMouseCapture();
    const bool currentNotCapturing = hasCapture && currentWindowUnderMouse != window;
    // Enter new window: track to generate leave event.
    // If there is an active capture, only track if the current window is capturing,
    // so we don't get extra leave when cursor leaves the application.
    if (window != m_trackedWindow && !currentNotCapturing) {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        tme.dwHoverTime = HOVER_DEFAULT; //
        if (!TrackMouseEvent(&tme))
            qWarning("TrackMouseEvent failed.");
        m_trackedWindow =  window;
    }

    // No enter or leave events are sent as long as there is an autocapturing window.
    if (!hasCapture || !platformWindow->testFlag(QWindowsWindow::AutoMouseCapture)) {
        // Leave is needed if:
        // 1) There is no capture and we move from a window to another window.
        //    Note: Leaving the application entirely is handled in WM_MOUSELEAVE case.
        // 2) There is capture and we move out of the capturing window.
        // 3) There is a new capture and we were over another window.
        if ((m_windowUnderMouse && m_windowUnderMouse != currentWindowUnderMouse
                && (!hasCapture || window == m_windowUnderMouse))
            || (hasCapture && m_previousCaptureWindow != window && m_windowUnderMouse
                && m_windowUnderMouse != window)) {
            qCDebug(lcQpaEvents) << "Synthetic leave for " << m_windowUnderMouse;
            QWindowSystemInterface::handleLeaveEvent(m_windowUnderMouse);
            if (currentNotCapturing) {
                // Clear tracking if capturing and current window is not the capturing window
                // to avoid leave when mouse actually leaves the application.
                m_trackedWindow = nullptr;
                // We are not officially in any window, but we need to set some cursor to clear
                // whatever cursor the left window had, so apply the cursor of the capture window.
                platformWindow->applyCursor();
            }
        }
        // Enter is needed if:
        // 1) There is no capture and we move to a new window.
        // 2) There is capture and we move into the capturing window.
        // 3) The capture just ended and we are over non-capturing window.
        if ((currentWindowUnderMouse && m_windowUnderMouse != currentWindowUnderMouse
                && (!hasCapture || currentWindowUnderMouse == window))
            || (m_previousCaptureWindow && window != m_previousCaptureWindow && currentWindowUnderMouse
                && currentWindowUnderMouse != m_previousCaptureWindow)) {
            QPoint localPosition;
            qCDebug(lcQpaEvents) << "Entering " << currentWindowUnderMouse;
            if (QWindowsWindow *wumPlatformWindow = QWindowsWindow::windowsWindowOf(currentWindowUnderMouse)) {
                localPosition = wumPlatformWindow->mapFromGlobal(globalPosition);
                wumPlatformWindow->applyCursor();
            }
            QWindowSystemInterface::handleEnterEvent(currentWindowUnderMouse, localPosition, globalPosition);
        }
        // We need to track m_windowUnderMouse separately from m_trackedWindow, as
        // Windows mouse tracking will not trigger WM_MOUSELEAVE for leaving window when
        // mouse capture is set.
        m_windowUnderMouse = currentWindowUnderMouse;
    }

    if (!discardEvent && mouseEvent.type != QEvent::None) {
        QWindowSystemInterface::handleMouseEvent(window, winEventPosition, globalPosition, buttons,
                                                 mouseEvent.button, mouseEvent.type,
                                                 keyModifiers, source);
    }
    m_previousCaptureWindow = hasCapture ? window : nullptr;
    // QTBUG-48117, force synchronous handling for the extra buttons so that WM_APPCOMMAND
    // is sent for unhandled WM_XBUTTONDOWN.
    return (msg.message != WM_XBUTTONUP && msg.message != WM_XBUTTONDOWN && msg.message != WM_XBUTTONDBLCLK)
        || QWindowSystemInterface::flushWindowSystemEvents();
}

static bool isValidWheelReceiver(QWindow *candidate)
{
    if (candidate) {
        const QWindow *toplevel = QWindowsWindow::topLevelOf(candidate);
        if (toplevel->handle() && toplevel->handle()->isForeignWindow())
            return true;
        if (const QWindowsWindow *ww = QWindowsWindow::windowsWindowOf(toplevel))
            return !ww->testFlag(QWindowsWindow::BlockedByModal);
    }

    return false;
}

static void redirectWheelEvent(QWindow *window, const QPoint &globalPos, int delta,
                               Qt::Orientation orientation, Qt::KeyboardModifiers mods)
{
    // Redirect wheel event to one of the following, in order of preference:
    // 1) The window under mouse
    // 2) The window receiving the event
    // If a window is blocked by modality, it can't get the event.

    QWindow *receiver = QWindowsScreen::windowAt(globalPos, CWP_SKIPINVISIBLE);
    while (receiver && receiver->flags().testFlag(Qt::WindowTransparentForInput))
        receiver = receiver->parent();
    bool handleEvent = true;
    if (!isValidWheelReceiver(receiver)) {
        receiver = window;
        if (!isValidWheelReceiver(receiver))
            handleEvent = false;
    }

    if (handleEvent) {
        const QPoint point = (orientation == Qt::Vertical) ? QPoint(0, delta) : QPoint(delta, 0);
        QWindowSystemInterface::handleWheelEvent(receiver,
                                                 QWindowsGeometryHint::mapFromGlobal(receiver, globalPos),
                                                 globalPos, QPoint(), point, mods);
    }
}

bool QWindowsMouseHandler::translateMouseWheelEvent(QWindow *window, HWND,
                                                    MSG msg, LRESULT *)
{
    const Qt::KeyboardModifiers mods = keyStateToModifiers(int(msg.wParam));

    int delta;
    if (msg.message == WM_MOUSEWHEEL || msg.message == WM_MOUSEHWHEEL)
        delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
    else
        delta = int(msg.wParam);

    Qt::Orientation orientation = (msg.message == WM_MOUSEHWHEEL
                                  || (mods & Qt::AltModifier)) ?
                                  Qt::Horizontal : Qt::Vertical;

    // according to the MSDN documentation on WM_MOUSEHWHEEL:
    // a positive value indicates that the wheel was rotated to the right;
    // a negative value indicates that the wheel was rotated to the left.
    // Qt defines this value as the exact opposite, so we have to flip the value!
    if (msg.message == WM_MOUSEHWHEEL)
        delta = -delta;

    const QPoint globalPos(GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
    redirectWheelEvent(window, globalPos, delta, orientation, mods);

    return true;
}

bool QWindowsMouseHandler::translateScrollEvent(QWindow *window, HWND,
                                                MSG msg, LRESULT *)
{
    // This is a workaround against some touchpads that send WM_HSCROLL instead of WM_MOUSEHWHEEL.
    // We could also handle vertical scroll here but there's no reason to, there's no bug for vertical
    // (broken vertical scroll would have been noticed long time ago), so lets keep the change small
    // and minimize the chance for regressions.

    int delta = 0;
    switch (LOWORD(msg.wParam)) {
    case SB_LINELEFT:
        delta = 120;
        break;
    case SB_LINERIGHT:
        delta = -120;
        break;
    case SB_PAGELEFT:
        delta = 240;
        break;
    case SB_PAGERIGHT:
        delta = -240;
        break;
    default:
        return false;
    }

    redirectWheelEvent(window, QCursor::pos(), delta, Qt::Horizontal, Qt::NoModifier);

    return true;
}

// from bool QApplicationPrivate::translateTouchEvent()
bool QWindowsMouseHandler::translateTouchEvent(QWindow *window, HWND,
                                               QtWindows::WindowsEventType,
                                               MSG msg, LRESULT *)
{
    using QTouchPoint = QWindowSystemInterface::TouchPoint;
    using QTouchPointList = QList<QWindowSystemInterface::TouchPoint>;

    if (!QWindowsContext::instance()->initTouch()) {
        qWarning("Unable to initialize touch handling.");
        return true;
    }

    const QScreen *screen = window->screen();
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen)
        return true;
    const QRect screenGeometry = screen->geometry();

    const int winTouchPointCount = int(msg.wParam);
    QScopedArrayPointer<TOUCHINPUT> winTouchInputs(new TOUCHINPUT[winTouchPointCount]);
    memset(winTouchInputs.data(), 0, sizeof(TOUCHINPUT) * size_t(winTouchPointCount));

    QTouchPointList touchPoints;
    touchPoints.reserve(winTouchPointCount);
    Qt::TouchPointStates allStates;

    GetTouchInputInfo(reinterpret_cast<HTOUCHINPUT>(msg.lParam),
                      UINT(msg.wParam), winTouchInputs.data(), sizeof(TOUCHINPUT));
    for (int i = 0; i < winTouchPointCount; ++i) {
        const TOUCHINPUT &winTouchInput = winTouchInputs[i];
        int id = m_touchInputIDToTouchPointID.value(winTouchInput.dwID, -1);
        if (id == -1) {
            id = m_touchInputIDToTouchPointID.size();
            m_touchInputIDToTouchPointID.insert(winTouchInput.dwID, id);
        }
        QTouchPoint touchPoint;
        touchPoint.pressure = 1.0;
        touchPoint.id = id;
        if (m_lastTouchPositions.contains(id))
            touchPoint.normalPosition = m_lastTouchPositions.value(id);

        const QPointF screenPos = QPointF(winTouchInput.x, winTouchInput.y) / qreal(100.);
        if (winTouchInput.dwMask & TOUCHINPUTMASKF_CONTACTAREA)
            touchPoint.area.setSize(QSizeF(winTouchInput.cxContact, winTouchInput.cyContact) / qreal(100.));
        touchPoint.area.moveCenter(screenPos);
        QPointF normalPosition = QPointF(screenPos.x() / screenGeometry.width(),
                                         screenPos.y() / screenGeometry.height());
        const bool stationaryTouchPoint = (normalPosition == touchPoint.normalPosition);
        touchPoint.normalPosition = normalPosition;

        if (winTouchInput.dwFlags & TOUCHEVENTF_DOWN) {
            touchPoint.state = Qt::TouchPointPressed;
            m_lastTouchPositions.insert(id, touchPoint.normalPosition);
        } else if (winTouchInput.dwFlags & TOUCHEVENTF_UP) {
            touchPoint.state = Qt::TouchPointReleased;
            m_lastTouchPositions.remove(id);
        } else {
            touchPoint.state = (stationaryTouchPoint
                     ? Qt::TouchPointStationary
                     : Qt::TouchPointMoved);
            m_lastTouchPositions.insert(id, touchPoint.normalPosition);
        }

        allStates |= touchPoint.state;

        touchPoints.append(touchPoint);
    }

    CloseTouchInputHandle(reinterpret_cast<HTOUCHINPUT>(msg.lParam));

    // all touch points released, forget the ids we've seen, they may not be reused
    if (allStates == Qt::TouchPointReleased)
        m_touchInputIDToTouchPointID.clear();

    QWindowSystemInterface::handleTouchEvent(window,
                                             m_touchDevice,
                                             touchPoints,
                                             QWindowsKeyMapper::queryKeyboardModifiers());
    return true;
}

bool QWindowsMouseHandler::translateGestureEvent(QWindow *window, HWND hwnd,
                                                 QtWindows::WindowsEventType,
                                                 MSG msg, LRESULT *)
{
    Q_UNUSED(window)
    Q_UNUSED(hwnd)
    Q_UNUSED(msg)
    return false;
}

QT_END_NAMESPACE
