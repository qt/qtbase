/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#if defined(WINVER) && WINVER < 0x0603
#  undef WINVER
#endif
#if !defined(WINVER)
#  define WINVER 0x0603 // Enable pointer functions for MinGW
#endif

#include "qwindowspointerhandler.h"
#include "qwindowskeymapper.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsintegration.h"
#include "qwindowsscreen.h"

#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>
#include <QtGui/qtouchdevice.h>
#include <QtGui/qwindow.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qqueue.h>

#include <algorithm>

#include <windowsx.h>

QT_BEGIN_NAMESPACE

enum {
    QT_PT_POINTER  = 1,
    QT_PT_TOUCH    = 2,
    QT_PT_PEN      = 3,
    QT_PT_MOUSE    = 4,
    QT_PT_TOUCHPAD = 5, // MinGW is missing PT_TOUCHPAD
};

bool QWindowsPointerHandler::translatePointerEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et, MSG msg, LRESULT *result)
{
    *result = 0;
    const quint32 pointerId = GET_POINTERID_WPARAM(msg.wParam);

    if (!QWindowsContext::user32dll.getPointerType(pointerId, &m_pointerType)) {
        qWarning() << "GetPointerType() failed:" << qt_error_string();
        return false;
    }

    switch (m_pointerType) {
    case QT_PT_POINTER:
    case QT_PT_MOUSE:
    case QT_PT_TOUCHPAD: {
        // Let Mouse/TouchPad be handled using legacy messages.
        return false;
    }
    case QT_PT_TOUCH: {
        quint32 pointerCount = 0;
        if (!QWindowsContext::user32dll.getPointerFrameTouchInfo(pointerId, &pointerCount, nullptr)) {
            qWarning() << "GetPointerFrameTouchInfo() failed:" << qt_error_string();
            return false;
        }
        QVarLengthArray<POINTER_TOUCH_INFO, 10> touchInfo(pointerCount);
        if (!QWindowsContext::user32dll.getPointerFrameTouchInfo(pointerId, &pointerCount, touchInfo.data())) {
            qWarning() << "GetPointerFrameTouchInfo() failed:" << qt_error_string();
            return false;
        }

        if (!pointerCount)
            return false;

        // The history count is the same for all the touchpoints in touchInfo
        quint32 historyCount = touchInfo[0].pointerInfo.historyCount;
        // dispatch any skipped frames if event compression is disabled by the app
        if (historyCount > 1 && !QCoreApplication::testAttribute(Qt::AA_CompressHighFrequencyEvents)) {
            touchInfo.resize(pointerCount * historyCount);
            if (!QWindowsContext::user32dll.getPointerFrameTouchInfoHistory(pointerId,
                                                                            &historyCount,
                                                                            &pointerCount,
                                                                            touchInfo.data())) {
                qWarning() << "GetPointerFrameTouchInfoHistory() failed:" << qt_error_string();
                return false;
            }

            // history frames are returned with the most recent frame first so we iterate backwards
            bool result = true;
            for (auto it = touchInfo.rbegin(), end = touchInfo.rend(); it != end; it += pointerCount) {
                result &= translateTouchEvent(window, hwnd, et, msg,
                                              &(*(it + (pointerCount - 1))), pointerCount);
            }
            return result;
        }

        return translateTouchEvent(window, hwnd, et, msg, touchInfo.data(), pointerCount);
    }
    case QT_PT_PEN: {
        POINTER_PEN_INFO penInfo;
        if (!QWindowsContext::user32dll.getPointerPenInfo(pointerId, &penInfo)) {
            qWarning() << "GetPointerPenInfo() failed:" << qt_error_string();
            return false;
        }

        quint32 historyCount = penInfo.pointerInfo.historyCount;
        // dispatch any skipped frames if generic or tablet event compression is disabled by the app
        if (historyCount > 1
            && (!QCoreApplication::testAttribute(Qt::AA_CompressHighFrequencyEvents)
                || !QCoreApplication::testAttribute(Qt::AA_CompressTabletEvents))) {
            QVarLengthArray<POINTER_PEN_INFO, 10> penInfoHistory(historyCount);

            if (!QWindowsContext::user32dll.getPointerPenInfoHistory(pointerId,
                                                                     &historyCount,
                                                                     penInfoHistory.data())) {
                qWarning() << "GetPointerPenInfoHistory() failed:" << qt_error_string();
                return false;
            }

            // history frames are returned with the most recent frame first so we iterate backwards
            bool result = true;
            for (auto it = penInfoHistory.rbegin(), end = penInfoHistory.rend(); it != end; ++it) {
                result &= translatePenEvent(window, hwnd, et, msg, &(*(it)));
            }
            return result;
        }

        return translatePenEvent(window, hwnd, et, msg, &penInfo);
    }
    }
    return false;
}

namespace {
struct MouseEvent {
    QEvent::Type type;
    Qt::MouseButton button;
};
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

static Qt::MouseButtons mouseButtonsFromKeyState(WPARAM keyState)
{
    Qt::MouseButtons result = Qt::NoButton;
    if (keyState & MK_LBUTTON)
        result |= Qt::LeftButton;
    if (keyState & MK_RBUTTON)
        result |= Qt::RightButton;
    if (keyState & MK_MBUTTON)
        result |= Qt::MiddleButton;
    if (keyState & MK_XBUTTON1)
        result |= Qt::XButton1;
    if (keyState & MK_XBUTTON2)
        result |= Qt::XButton2;
    return result;
}

static Qt::MouseButtons queryMouseButtons()
{
    Qt::MouseButtons result = Qt::NoButton;
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

static QWindow *getWindowUnderPointer(QWindow *window, QPoint globalPos)
{
    QWindowsWindow *platformWindow = static_cast<QWindowsWindow *>(window->handle());

    QWindow *currentWindowUnderPointer = platformWindow->hasMouseCapture() ?
                QWindowsScreen::windowAt(globalPos, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT) : window;

    while (currentWindowUnderPointer && currentWindowUnderPointer->flags() & Qt::WindowTransparentForInput)
        currentWindowUnderPointer = currentWindowUnderPointer->parent();

    // QTBUG-44332: When Qt is running at low integrity level and
    // a Qt Window is parented on a Window of a higher integrity process
    // using QWindow::fromWinId() (for example, Qt running in a browser plugin)
    // ChildWindowFromPointEx() may not find the Qt window (failing with ERROR_ACCESS_DENIED)
    if (!currentWindowUnderPointer) {
        const QRect clientRect(QPoint(0, 0), window->size());
        if (clientRect.contains(globalPos))
            currentWindowUnderPointer = window;
    }
    return currentWindowUnderPointer;
}

static bool trackLeave(HWND hwnd)
{
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = hwnd;
    tme.dwHoverTime = HOVER_DEFAULT;
    return TrackMouseEvent(&tme);
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

static QTouchDevice *createTouchDevice()
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

QTouchDevice *QWindowsPointerHandler::ensureTouchDevice()
{
    if (!m_touchDevice)
        m_touchDevice = createTouchDevice();
    return m_touchDevice;
}

void QWindowsPointerHandler::clearEvents()
{
    m_lastEventType = QEvent::None;
    m_lastEventButton = Qt::NoButton;
}

void QWindowsPointerHandler::handleCaptureRelease(QWindow *window,
                                                  QWindow *currentWindowUnderPointer,
                                                  HWND hwnd,
                                                  QEvent::Type eventType,
                                                  Qt::MouseButtons mouseButtons)
{
    auto *platformWindow = static_cast<QWindowsWindow *>(window->handle());

    // Qt expects the platform plugin to capture the mouse on any button press until release.
    if (!platformWindow->hasMouseCapture() && eventType == QEvent::MouseButtonPress) {

        platformWindow->setMouseGrabEnabled(true);
        platformWindow->setFlag(QWindowsWindow::AutoMouseCapture);
        qCDebug(lcQpaEvents) << "Automatic mouse capture " << window;

        // Implement "Click to focus" for native child windows (unless it is a native widget window).
        if (!window->isTopLevel() && !window->inherits("QWidgetWindow") && QGuiApplication::focusWindow() != window)
            window->requestActivate();

    } else if (platformWindow->hasMouseCapture()
               && platformWindow->testFlag(QWindowsWindow::AutoMouseCapture)
               && eventType == QEvent::MouseButtonRelease
               && !mouseButtons) {

        platformWindow->setMouseGrabEnabled(false);
        qCDebug(lcQpaEvents) << "Releasing automatic mouse capture " << window;
    }

    // Enter new window: track to generate leave event.
    // If there is an active capture, only track if the current window is capturing,
    // so we don't get extra leave when cursor leaves the application.
    if (window != m_currentWindow &&
            (!platformWindow->hasMouseCapture() || currentWindowUnderPointer == window)) {
        trackLeave(hwnd);
        m_currentWindow =  window;
    }
}

void QWindowsPointerHandler::handleEnterLeave(QWindow *window,
                                              QWindow *currentWindowUnderPointer,
                                              QPoint globalPos)
{
    auto *platformWindow = static_cast<QWindowsWindow *>(window->handle());
    const bool hasCapture = platformWindow->hasMouseCapture();

    // No enter or leave events are sent as long as there is an autocapturing window.
    if (!hasCapture || !platformWindow->testFlag(QWindowsWindow::AutoMouseCapture)) {

        // Leave is needed if:
        // 1) There is no capture and we move from a window to another window.
        //    Note: Leaving the application entirely is handled in translateMouseEvent(WM_MOUSELEAVE).
        // 2) There is capture and we move out of the capturing window.
        // 3) There is a new capture and we were over another window.
        if ((m_windowUnderPointer && m_windowUnderPointer != currentWindowUnderPointer
                && (!hasCapture || window == m_windowUnderPointer))
            || (hasCapture && m_previousCaptureWindow != window && m_windowUnderPointer
                && m_windowUnderPointer != window)) {

            qCDebug(lcQpaEvents) << "Leaving window " << m_windowUnderPointer;
            QWindowSystemInterface::handleLeaveEvent(m_windowUnderPointer);

            if (hasCapture && currentWindowUnderPointer != window) {
                // Clear tracking if capturing and current window is not the capturing window
                // to avoid leave when mouse actually leaves the application.
                m_currentWindow = nullptr;
                // We are not officially in any window, but we need to set some cursor to clear
                // whatever cursor the left window had, so apply the cursor of the capture window.
                platformWindow->applyCursor();
            }
        }

        // Enter is needed if:
        // 1) There is no capture and we move to a new window.
        // 2) There is capture and we move into the capturing window.
        // 3) The capture just ended and we are over non-capturing window.
        if ((currentWindowUnderPointer && m_windowUnderPointer != currentWindowUnderPointer
                && (!hasCapture || currentWindowUnderPointer == window))
            || (m_previousCaptureWindow && !hasCapture && currentWindowUnderPointer
                && currentWindowUnderPointer != m_previousCaptureWindow)) {

            QPoint wumLocalPos;
            if (QWindowsWindow *wumPlatformWindow = QWindowsWindow::windowsWindowOf(currentWindowUnderPointer)) {
                wumLocalPos = wumPlatformWindow->mapFromGlobal(globalPos);
                wumPlatformWindow->applyCursor();
            }
            qCDebug(lcQpaEvents) << "Entering window " << currentWindowUnderPointer;
            QWindowSystemInterface::handleEnterEvent(currentWindowUnderPointer, wumLocalPos, globalPos);
        }

        // We need to track m_windowUnderPointer separately from m_currentWindow, as Windows
        // mouse tracking will not trigger WM_MOUSELEAVE for leaving window when mouse capture is set.
        m_windowUnderPointer = currentWindowUnderPointer;
    }

    m_previousCaptureWindow = hasCapture ? window : nullptr;
}

bool QWindowsPointerHandler::translateTouchEvent(QWindow *window, HWND hwnd,
                                                 QtWindows::WindowsEventType et,
                                                 MSG msg, PVOID vTouchInfo, quint32 count)
{
    Q_UNUSED(hwnd);

    if (et & QtWindows::NonClientEventFlag)
        return false; // Let DefWindowProc() handle Non Client messages.

    if (count < 1)
        return false;

    if (msg.message == WM_POINTERCAPTURECHANGED) {
        QWindowSystemInterface::handleTouchCancelEvent(window, m_touchDevice,
                                                       QWindowsKeyMapper::queryKeyboardModifiers());
        m_lastTouchPositions.clear();
        return true;
    }

    // Only handle down/up/update, ignore others like WM_POINTERENTER, WM_POINTERLEAVE, etc.
    if (msg.message > WM_POINTERUP)
        return false;

    const QScreen *screen = window->screen();
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen)
        return false;

    auto *touchInfo = static_cast<POINTER_TOUCH_INFO *>(vTouchInfo);

    const QRect screenGeometry = screen->geometry();

    QList<QWindowSystemInterface::TouchPoint> touchPoints;

    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaEvents).noquote().nospace() << Qt::showbase
                << __FUNCTION__
                << " message=" << Qt::hex << msg.message
                << " count=" << Qt::dec << count;

    Qt::TouchPointStates allStates;

    for (quint32 i = 0; i < count; ++i) {
        if (QWindowsContext::verbose > 1)
            qCDebug(lcQpaEvents).noquote().nospace() << Qt::showbase
                    << "    TouchPoint id=" << touchInfo[i].pointerInfo.pointerId
                    << " frame=" << touchInfo[i].pointerInfo.frameId
                    << " flags=" << Qt::hex << touchInfo[i].pointerInfo.pointerFlags;

        QWindowSystemInterface::TouchPoint touchPoint;
        const quint32 pointerId = touchInfo[i].pointerInfo.pointerId;
        int id = m_touchInputIDToTouchPointID.value(pointerId, -1);
        if (id == -1) {
            id = m_touchInputIDToTouchPointID.size();
            m_touchInputIDToTouchPointID.insert(pointerId, id);
        }
        touchPoint.id = id;
        touchPoint.pressure = (touchInfo[i].touchMask & TOUCH_MASK_PRESSURE) ?
                    touchInfo[i].pressure / 1024.0 : 1.0;
        if (m_lastTouchPositions.contains(touchPoint.id))
            touchPoint.normalPosition = m_lastTouchPositions.value(touchPoint.id);

        const QPointF screenPos = QPointF(touchInfo[i].pointerInfo.ptPixelLocation.x,
                                          touchInfo[i].pointerInfo.ptPixelLocation.y);

        if (touchInfo[i].touchMask & TOUCH_MASK_CONTACTAREA)
            touchPoint.area.setSize(QSizeF(touchInfo[i].rcContact.right - touchInfo[i].rcContact.left,
                                           touchInfo[i].rcContact.bottom - touchInfo[i].rcContact.top));
        touchPoint.area.moveCenter(screenPos);
        QPointF normalPosition = QPointF(screenPos.x() / screenGeometry.width(),
                                         screenPos.y() / screenGeometry.height());
        const bool stationaryTouchPoint = (normalPosition == touchPoint.normalPosition);
        touchPoint.normalPosition = normalPosition;

        if (touchInfo[i].pointerInfo.pointerFlags & POINTER_FLAG_DOWN) {
            touchPoint.state = Qt::TouchPointPressed;
            m_lastTouchPositions.insert(touchPoint.id, touchPoint.normalPosition);
        } else if (touchInfo[i].pointerInfo.pointerFlags & POINTER_FLAG_UP) {
            touchPoint.state = Qt::TouchPointReleased;
            m_lastTouchPositions.remove(touchPoint.id);
        } else {
            touchPoint.state = stationaryTouchPoint ? Qt::TouchPointStationary : Qt::TouchPointMoved;
            m_lastTouchPositions.insert(touchPoint.id, touchPoint.normalPosition);
        }
        allStates |= touchPoint.state;

        touchPoints.append(touchPoint);

        // Avoid getting repeated messages for this frame if there are multiple pointerIds
        QWindowsContext::user32dll.skipPointerFrameMessages(touchInfo[i].pointerInfo.pointerId);
    }

    // all touch points released, forget the ids we've seen.
    if (allStates == Qt::TouchPointReleased)
        m_touchInputIDToTouchPointID.clear();

    QWindowSystemInterface::handleTouchEvent(window, m_touchDevice, touchPoints,
                                             QWindowsKeyMapper::queryKeyboardModifiers());
    return false; // Allow mouse messages to be generated.
}

bool QWindowsPointerHandler::translatePenEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et,
                                               MSG msg, PVOID vPenInfo)
{
#if QT_CONFIG(tabletevent)
    if (et & QtWindows::NonClientEventFlag)
        return false; // Let DefWindowProc() handle Non Client messages.

    auto *penInfo = static_cast<POINTER_PEN_INFO *>(vPenInfo);

    RECT pRect, dRect;
    if (!QWindowsContext::user32dll.getPointerDeviceRects(penInfo->pointerInfo.sourceDevice, &pRect, &dRect))
        return false;

    const auto sourceDevice = (qint64)penInfo->pointerInfo.sourceDevice;
    const QPoint globalPos = QPoint(penInfo->pointerInfo.ptPixelLocation.x, penInfo->pointerInfo.ptPixelLocation.y);
    const QPoint localPos = QWindowsGeometryHint::mapFromGlobal(hwnd, globalPos);
    const QPointF hiResGlobalPos = QPointF(dRect.left + qreal(penInfo->pointerInfo.ptHimetricLocation.x - pRect.left)
                                           / (pRect.right - pRect.left) * (dRect.right - dRect.left),
                                           dRect.top + qreal(penInfo->pointerInfo.ptHimetricLocation.y - pRect.top)
                                           / (pRect.bottom - pRect.top) * (dRect.bottom - dRect.top));
    const qreal pressure = (penInfo->penMask & PEN_MASK_PRESSURE) ? qreal(penInfo->pressure) / 1024.0 : 0.5;
    const qreal rotation = (penInfo->penMask & PEN_MASK_ROTATION) ? qreal(penInfo->rotation) : 0.0;
    const qreal tangentialPressure = 0.0;
    const int xTilt = (penInfo->penMask & PEN_MASK_TILT_X) ? penInfo->tiltX : 0;
    const int yTilt = (penInfo->penMask & PEN_MASK_TILT_Y) ? penInfo->tiltY : 0;
    const int z = 0;

    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaEvents).noquote().nospace() << Qt::showbase
            << __FUNCTION__ << " sourceDevice=" << sourceDevice
            << " globalPos=" << globalPos << " localPos=" << localPos << " hiResGlobalPos=" << hiResGlobalPos
            << " message=" << Qt::hex << msg.message
            << " flags=" << Qt::hex << penInfo->pointerInfo.pointerFlags;

    const QTabletEvent::TabletDevice device = QTabletEvent::Stylus;
    QTabletEvent::PointerType type;
    // Since it may be the middle button, so if the checks fail then it should
    // be set to Middle if it was used.
    Qt::MouseButtons mouseButtons = queryMouseButtons();

    const bool pointerInContact = IS_POINTER_INCONTACT_WPARAM(msg.wParam);
    if (pointerInContact)
        mouseButtons = Qt::LeftButton;

    if (penInfo->penFlags & (PEN_FLAG_ERASER | PEN_FLAG_INVERTED)) {
        type = QTabletEvent::Eraser;
    } else {
        type = QTabletEvent::Pen;
        if (pointerInContact && penInfo->penFlags & PEN_FLAG_BARREL)
            mouseButtons = Qt::RightButton; // Either left or right, not both
    }

    switch (msg.message) {
    case WM_POINTERENTER: {
        QWindowSystemInterface::handleTabletEnterProximityEvent(device, type, sourceDevice);
        m_windowUnderPointer = window;
        // The local coordinates may fall outside the window.
        // Wait until the next update to send the enter event.
        m_needsEnterOnPointerUpdate = true;
        break;
    }
    case WM_POINTERLEAVE:
        if (m_windowUnderPointer && m_windowUnderPointer == m_currentWindow) {
            QWindowSystemInterface::handleLeaveEvent(m_windowUnderPointer);
            m_windowUnderPointer = nullptr;
            m_currentWindow = nullptr;
        }
        QWindowSystemInterface::handleTabletLeaveProximityEvent(device, type, sourceDevice);
        break;
    case WM_POINTERDOWN:
    case WM_POINTERUP:
    case WM_POINTERUPDATE: {
        QWindow *target = QGuiApplicationPrivate::tabletDevicePoint(sourceDevice).target; // Pass to window that grabbed it.
        if (!target && m_windowUnderPointer)
            target = m_windowUnderPointer;
        if (!target)
            target = window;

        if (m_needsEnterOnPointerUpdate) {
            m_needsEnterOnPointerUpdate = false;
            if (window != m_currentWindow) {
                // make sure we subscribe to leave events for this window
                trackLeave(hwnd);

                QWindowSystemInterface::handleEnterEvent(window, localPos, globalPos);
                m_currentWindow = window;
                if (QWindowsWindow *wumPlatformWindow = QWindowsWindow::windowsWindowOf(target))
                    wumPlatformWindow->applyCursor();
            }
        }
        const Qt::KeyboardModifiers keyModifiers = QWindowsKeyMapper::queryKeyboardModifiers();

        QWindowSystemInterface::handleTabletEvent(target, localPos, hiResGlobalPos, device, type, mouseButtons,
                                                  pressure, xTilt, yTilt, tangentialPressure, rotation, z,
                                                  sourceDevice, keyModifiers);
        return false;  // Allow mouse messages to be generated.
    }
    }
    return true;
#else
    Q_UNUSED(window);
    Q_UNUSED(hwnd);
    Q_UNUSED(et);
    Q_UNUSED(msg);
    Q_UNUSED(vPenInfo);
    return false;
#endif
}

static inline bool isMouseEventSynthesizedFromPenOrTouch()
{
    // For details, see
    // https://docs.microsoft.com/en-us/windows/desktop/tablet/system-events-and-mouse-messages
    const LONG_PTR SIGNATURE_MASK = 0xFFFFFF00;
    const LONG_PTR MI_WP_SIGNATURE = 0xFF515700;

    return ((::GetMessageExtraInfo() & SIGNATURE_MASK) == MI_WP_SIGNATURE);
}

bool QWindowsPointerHandler::translateMouseWheelEvent(QWindow *window,
                                                      QWindow *currentWindowUnderPointer,
                                                      MSG msg,
                                                      QPoint globalPos,
                                                      Qt::KeyboardModifiers keyModifiers)
{
    QWindow *receiver = currentWindowUnderPointer;
    if (!isValidWheelReceiver(receiver))
        receiver = window;
    if (!isValidWheelReceiver(receiver))
        return true;

    int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);

    // Qt horizontal wheel rotation orientation is opposite to the one in WM_MOUSEHWHEEL
    if (msg.message == WM_MOUSEHWHEEL)
        delta = -delta;

    const QPoint angleDelta = (msg.message == WM_MOUSEHWHEEL || (keyModifiers & Qt::AltModifier)) ?
                QPoint(delta, 0) : QPoint(0, delta);

    QPoint localPos = QWindowsGeometryHint::mapFromGlobal(receiver, globalPos);

    QWindowSystemInterface::handleWheelEvent(receiver, localPos, globalPos, QPoint(), angleDelta, keyModifiers);
    return true;
}

// Process legacy mouse messages here.
bool QWindowsPointerHandler::translateMouseEvent(QWindow *window,
                                                 HWND hwnd,
                                                 QtWindows::WindowsEventType et,
                                                 MSG msg,
                                                 LRESULT *result)
{
    *result = 0;

    QPoint eventPos(GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
    if ((et & QtWindows::NonClientEventFlag) == 0 && QWindowsBaseWindow::isRtlLayout(hwnd))  {
        RECT clientArea;
        GetClientRect(hwnd, &clientArea);
        eventPos.setX(clientArea.right - eventPos.x());
    }

    QPoint localPos;
    QPoint globalPos;

    if ((et == QtWindows::MouseWheelEvent) || (et & QtWindows::NonClientEventFlag)) {
        globalPos = eventPos;
        localPos = QWindowsGeometryHint::mapFromGlobal(hwnd, eventPos);
    } else {
        localPos = eventPos;
        globalPos = QWindowsGeometryHint::mapToGlobal(hwnd, eventPos);
    }

    const Qt::KeyboardModifiers keyModifiers = QWindowsKeyMapper::queryKeyboardModifiers();
    QWindow *currentWindowUnderPointer = getWindowUnderPointer(window, globalPos);

    if (et == QtWindows::MouseWheelEvent)
        return translateMouseWheelEvent(window, currentWindowUnderPointer, msg, globalPos, keyModifiers);

    // Windows sends a mouse move with no buttons pressed to signal "Enter"
    // when a window is shown over the cursor. Discard the event and only use
    // it for generating QEvent::Enter to be consistent with other platforms -
    // X11 and macOS.
    bool discardEvent = false;
    if (msg.message == WM_MOUSEMOVE) {
        static QPoint lastMouseMovePos;
        if (msg.wParam == 0 && (m_windowUnderPointer.isNull() || globalPos == lastMouseMovePos))
            discardEvent = true;
        lastMouseMovePos = globalPos;
    }

    Qt::MouseEventSource source = Qt::MouseEventNotSynthesized;
    // Following the logic of the old mouse handler, only events synthesized
    // for touch screen are marked as such. On some systems, using the bit 7 of
    // the extra msg info for checking if synthesized for touch does not work,
    // so we use the pointer type of the last pointer message.
    if (isMouseEventSynthesizedFromPenOrTouch() && m_pointerType == QT_PT_TOUCH) {
        if (QWindowsIntegration::instance()->options() & QWindowsIntegration::DontPassOsMouseEventsSynthesizedFromTouch)
            return false;
        source = Qt::MouseEventSynthesizedBySystem;
    }

    const MouseEvent mouseEvent = eventFromMsg(msg);
    Qt::MouseButtons mouseButtons;

    if (mouseEvent.type >= QEvent::NonClientAreaMouseMove && mouseEvent.type <= QEvent::NonClientAreaMouseButtonDblClick)
        mouseButtons = queryMouseButtons();
    else
        mouseButtons = mouseButtonsFromKeyState(msg.wParam);

    // When the left/right mouse buttons are pressed over the window title bar
    // WM_NCLBUTTONDOWN/WM_NCRBUTTONDOWN messages are received. But no UP
    // messages are received on release, only WM_NCMOUSEMOVE/WM_MOUSEMOVE.
    // We detect it and generate the missing release events here. (QTBUG-75678)
    // The last event vars are cleared on QWindowsContext::handleExitSizeMove()
    // to avoid generating duplicated release events.
    if (m_lastEventType == QEvent::NonClientAreaMouseButtonPress
            && (mouseEvent.type == QEvent::NonClientAreaMouseMove || mouseEvent.type == QEvent::MouseMove)
            && (m_lastEventButton & mouseButtons) == 0) {
            if (mouseEvent.type == QEvent::NonClientAreaMouseMove) {
                QWindowSystemInterface::handleFrameStrutMouseEvent(window, localPos, globalPos, mouseButtons, m_lastEventButton,
                                                                   QEvent::NonClientAreaMouseButtonRelease, keyModifiers, source);
            } else {
                QWindowSystemInterface::handleMouseEvent(window, localPos, globalPos, mouseButtons, m_lastEventButton,
                                                         QEvent::MouseButtonRelease, keyModifiers, source);
            }
    }
    m_lastEventType = mouseEvent.type;
    m_lastEventButton = mouseEvent.button;

    if (mouseEvent.type >= QEvent::NonClientAreaMouseMove && mouseEvent.type <= QEvent::NonClientAreaMouseButtonDblClick) {
        QWindowSystemInterface::handleFrameStrutMouseEvent(window, localPos, globalPos, mouseButtons,
                                                           mouseEvent.button, mouseEvent.type, keyModifiers, source);
        return false; // Allow further event processing
    }

    if (msg.message == WM_MOUSELEAVE) {
        if (window == m_currentWindow) {
            QWindow *leaveTarget = m_windowUnderPointer ? m_windowUnderPointer : m_currentWindow;
            qCDebug(lcQpaEvents) << "Leaving window " << leaveTarget;
            QWindowSystemInterface::handleLeaveEvent(leaveTarget);
            m_windowUnderPointer = nullptr;
            m_currentWindow = nullptr;
        }
        return true;
    }

    handleCaptureRelease(window, currentWindowUnderPointer, hwnd, mouseEvent.type, mouseButtons);
    handleEnterLeave(window, currentWindowUnderPointer, globalPos);

    if (!discardEvent && mouseEvent.type != QEvent::None) {
        QWindowSystemInterface::handleMouseEvent(window, localPos, globalPos, mouseButtons,
                                                 mouseEvent.button, mouseEvent.type, keyModifiers, source);
    }

    // QTBUG-48117, force synchronous handling for the extra buttons so that WM_APPCOMMAND
    // is sent for unhandled WM_XBUTTONDOWN.
    return (msg.message != WM_XBUTTONUP && msg.message != WM_XBUTTONDOWN && msg.message != WM_XBUTTONDBLCLK)
        || QWindowSystemInterface::flushWindowSystemEvents();
}

QT_END_NAMESPACE
