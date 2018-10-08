/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <qpa/qwindowsysteminterface.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>
#include <QtGui/qtouchdevice.h>
#include <QtGui/qwindow.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qoperatingsystemversion.h>

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

    POINTER_INPUT_TYPE pointerType;
    if (!QWindowsContext::user32dll.getPointerType(pointerId, &pointerType)) {
        qWarning() << "GetPointerType() failed:" << qt_error_string();
        return false;
    }

    switch (pointerType) {
    case QT_PT_POINTER:
    case QT_PT_MOUSE:
    case QT_PT_TOUCHPAD: {
        POINTER_INFO pointerInfo;
        if (!QWindowsContext::user32dll.getPointerInfo(pointerId, &pointerInfo)) {
            qWarning() << "GetPointerInfo() failed:" << qt_error_string();
            return false;
        }
        return translateMouseTouchPadEvent(window, hwnd, et, msg, &pointerInfo);
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

static void getMouseEventInfo(UINT message, POINTER_BUTTON_CHANGE_TYPE changeType, QPoint globalPos, QEvent::Type *eventType, Qt::MouseButton *mouseButton)
{
    static const QHash<POINTER_BUTTON_CHANGE_TYPE, Qt::MouseButton> buttonMapping {
        {POINTER_CHANGE_FIRSTBUTTON_DOWN, Qt::LeftButton},
        {POINTER_CHANGE_FIRSTBUTTON_UP, Qt::LeftButton},
        {POINTER_CHANGE_SECONDBUTTON_DOWN, Qt::RightButton},
        {POINTER_CHANGE_SECONDBUTTON_UP, Qt::RightButton},
        {POINTER_CHANGE_THIRDBUTTON_DOWN, Qt::MiddleButton},
        {POINTER_CHANGE_THIRDBUTTON_UP, Qt::MiddleButton},
        {POINTER_CHANGE_FOURTHBUTTON_DOWN, Qt::XButton1},
        {POINTER_CHANGE_FOURTHBUTTON_UP, Qt::XButton1},
        {POINTER_CHANGE_FIFTHBUTTON_DOWN, Qt::XButton2},
        {POINTER_CHANGE_FIFTHBUTTON_UP, Qt::XButton2},
    };

    static const POINTER_BUTTON_CHANGE_TYPE downChanges[] = {
        POINTER_CHANGE_FIRSTBUTTON_DOWN,
        POINTER_CHANGE_SECONDBUTTON_DOWN,
        POINTER_CHANGE_THIRDBUTTON_DOWN,
        POINTER_CHANGE_FOURTHBUTTON_DOWN,
        POINTER_CHANGE_FIFTHBUTTON_DOWN,
    };

    static const POINTER_BUTTON_CHANGE_TYPE upChanges[] = {
        POINTER_CHANGE_FIRSTBUTTON_UP,
        POINTER_CHANGE_SECONDBUTTON_UP,
        POINTER_CHANGE_THIRDBUTTON_UP,
        POINTER_CHANGE_FOURTHBUTTON_UP,
        POINTER_CHANGE_FIFTHBUTTON_UP,
    };

    if (!eventType || !mouseButton)
        return;

    const bool nonClient = message == WM_NCPOINTERUPDATE ||
                           message == WM_NCPOINTERDOWN ||
                           message == WM_NCPOINTERUP;

    if (std::find(std::begin(downChanges),
                  std::end(downChanges), changeType) < std::end(downChanges)) {
        *eventType = nonClient ? QEvent::NonClientAreaMouseButtonPress :
                                 QEvent::MouseButtonPress;
    } else if (std::find(std::begin(upChanges),
                         std::end(upChanges), changeType) < std::end(upChanges)) {
        *eventType = nonClient ? QEvent::NonClientAreaMouseButtonRelease :
                                 QEvent::MouseButtonRelease;
    } else if (message == WM_POINTERWHEEL || message == WM_POINTERHWHEEL) {
        *eventType = QEvent::Wheel;
    } else {
        *eventType = nonClient ? QEvent::NonClientAreaMouseMove :
                                 QEvent::MouseMove;
    }

    *mouseButton = buttonMapping.value(changeType, Qt::NoButton);

    // Pointer messages lack a double click indicator. Check if this is the case here.
    if (*eventType == QEvent::MouseButtonPress ||
        *eventType == QEvent::NonClientAreaMouseButtonPress) {
        static LONG lastTime = 0;
        static Qt::MouseButton lastButton = Qt::NoButton;
        static QEvent::Type lastEvent = QEvent::None;
        static QPoint lastPos;
        LONG messageTime = GetMessageTime();
        if (*mouseButton == lastButton
            && *eventType == lastEvent
            && messageTime - lastTime < (LONG)GetDoubleClickTime()
            && qAbs(globalPos.x() - lastPos.x()) < GetSystemMetrics(SM_CXDOUBLECLK)
            && qAbs(globalPos.y() - lastPos.y()) < GetSystemMetrics(SM_CYDOUBLECLK)) {
            *eventType = nonClient ? QEvent::NonClientAreaMouseButtonDblClick :
                                     QEvent::MouseButtonDblClick;
        }
        lastTime = messageTime;
        lastButton = *mouseButton;
        lastEvent = *eventType;
        lastPos = globalPos;
    }
}

static QWindow *getWindowUnderPointer(QWindow *window, QPoint globalPos)
{
    QWindow *currentWindowUnderPointer = QWindowsScreen::windowAt(globalPos, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);

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
    qCDebug(lcQpaEvents) << "Digitizers:" << hex << showbase << (digitizers & ~NID_READY)
        << "Ready:" << (digitizers & NID_READY) << dec << noshowbase
        << "Tablet PC:" << tabletPc << "Max touch points:" << maxTouchPoints;
    QTouchDevice *result = new QTouchDevice;
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

Qt::MouseButtons QWindowsPointerHandler::queryMouseButtons()
{
    Qt::MouseButtons result = 0;
    const bool mouseSwapped = GetSystemMetrics(SM_SWAPBUTTON);
    if (GetAsyncKeyState(VK_LBUTTON) < 0)
        result |= mouseSwapped ? Qt::RightButton: Qt::LeftButton;
    if (GetAsyncKeyState(VK_RBUTTON) < 0)
        result |= mouseSwapped ? Qt::LeftButton : Qt::RightButton;
    if (GetAsyncKeyState(VK_MBUTTON) < 0)
        result |= Qt::MidButton;
    if (GetAsyncKeyState(VK_XBUTTON1) < 0)
        result |= Qt::XButton1;
    if (GetAsyncKeyState(VK_XBUTTON2) < 0)
        result |= Qt::XButton2;
    return result;
}

bool QWindowsPointerHandler::translateMouseTouchPadEvent(QWindow *window, HWND hwnd,
                                                         QtWindows::WindowsEventType et,
                                                         MSG msg, PVOID vPointerInfo)
{
    POINTER_INFO *pointerInfo = static_cast<POINTER_INFO *>(vPointerInfo);
    const QPoint globalPos = QPoint(pointerInfo->ptPixelLocation.x, pointerInfo->ptPixelLocation.y);
    const QPoint localPos = QWindowsGeometryHint::mapFromGlobal(hwnd, globalPos);
    const Qt::KeyboardModifiers keyModifiers = QWindowsKeyMapper::queryKeyboardModifiers();
    const Qt::MouseButtons mouseButtons = queryMouseButtons();

    QWindow *currentWindowUnderPointer = getWindowUnderPointer(window, globalPos);
    QWindowsWindow *platformWindow = static_cast<QWindowsWindow *>(window->handle());

    switch (msg.message) {
    case WM_NCPOINTERDOWN:
    case WM_NCPOINTERUP:
    case WM_NCPOINTERUPDATE:
    case WM_POINTERDOWN:
    case WM_POINTERUP:
    case WM_POINTERUPDATE: {

        QEvent::Type eventType;
        Qt::MouseButton button;
        getMouseEventInfo(msg.message, pointerInfo->ButtonChangeType, globalPos, &eventType, &button);

        if (et & QtWindows::NonClientEventFlag) {
            QWindowSystemInterface::handleFrameStrutMouseEvent(window, localPos, globalPos, mouseButtons, button, eventType,
                                                               keyModifiers, Qt::MouseEventNotSynthesized);
            return false;  // To allow window dragging, etc.
        } else {
            if (currentWindowUnderPointer != m_windowUnderPointer) {
                if (m_windowUnderPointer && m_windowUnderPointer == m_currentWindow) {
                    QWindowSystemInterface::handleLeaveEvent(m_windowUnderPointer);
                    m_currentWindow = nullptr;
                }

                if (currentWindowUnderPointer) {
                    if (currentWindowUnderPointer != m_currentWindow) {
                        QWindowSystemInterface::handleEnterEvent(currentWindowUnderPointer, localPos, globalPos);
                        m_currentWindow = currentWindowUnderPointer;
                        if (QWindowsWindow *wumPlatformWindow = QWindowsWindow::windowsWindowOf(currentWindowUnderPointer))
                            wumPlatformWindow->applyCursor();
                        trackLeave(hwnd);
                    }
                } else {
                    platformWindow->applyCursor();
                }
                m_windowUnderPointer = currentWindowUnderPointer;
            }

            QWindowSystemInterface::handleMouseEvent(window, localPos, globalPos, mouseButtons, button, eventType,
                                                     keyModifiers, Qt::MouseEventNotSynthesized);

            // The initial down click over the QSizeGrip area, which posts a resize WM_SYSCOMMAND
            // has go to through DefWindowProc() for resizing to work, so we return false here,
            // unless the mouse is captured, as it would mess with menu processing.
            return msg.message != WM_POINTERDOWN || GetCapture();
        }
    }
    case WM_POINTERHWHEEL:
    case WM_POINTERWHEEL: {

        int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);

        // Qt horizontal wheel rotation orientation is opposite to the one in WM_POINTERHWHEEL
        if (msg.message == WM_POINTERHWHEEL)
            delta = -delta;

        const QPoint angleDelta = (msg.message == WM_POINTERHWHEEL || (keyModifiers & Qt::AltModifier)) ?
                    QPoint(delta, 0) : QPoint(0, delta);

        if (isValidWheelReceiver(window))
            QWindowSystemInterface::handleWheelEvent(window, localPos, globalPos, QPoint(), angleDelta, keyModifiers);
        return true;
    }
    case WM_POINTERLEAVE:
        return true;
    }
    return false;
}

bool QWindowsPointerHandler::translateTouchEvent(QWindow *window, HWND hwnd,
                                                 QtWindows::WindowsEventType et,
                                                 MSG msg, PVOID vTouchInfo, quint32 count)
{
    Q_UNUSED(hwnd);
    Q_UNUSED(et);

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

    POINTER_TOUCH_INFO *touchInfo = static_cast<POINTER_TOUCH_INFO *>(vTouchInfo);

    const QRect screenGeometry = screen->geometry();

    QList<QWindowSystemInterface::TouchPoint> touchPoints;

    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaEvents).noquote().nospace() << showbase
                << __FUNCTION__
                << " message=" << hex << msg.message
                << " count=" << dec << count;

    for (quint32 i = 0; i < count; ++i) {
        if (QWindowsContext::verbose > 1)
            qCDebug(lcQpaEvents).noquote().nospace() << showbase
                    << "    TouchPoint id=" << touchInfo[i].pointerInfo.pointerId
                    << " frame=" << touchInfo[i].pointerInfo.frameId
                    << " flags=" << hex << touchInfo[i].pointerInfo.pointerFlags;

        QWindowSystemInterface::TouchPoint touchPoint;
        touchPoint.id = touchInfo[i].pointerInfo.pointerId;
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
        touchPoints.append(touchPoint);

        // Avoid getting repeated messages for this frame if there are multiple pointerIds
        QWindowsContext::user32dll.skipPointerFrameMessages(touchInfo[i].pointerInfo.pointerId);
    }

    QWindowSystemInterface::handleTouchEvent(window, m_touchDevice, touchPoints,
                                             QWindowsKeyMapper::queryKeyboardModifiers());

    return true;
}

bool QWindowsPointerHandler::translatePenEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et,
                                               MSG msg, PVOID vPenInfo)
{
    if (et & QtWindows::NonClientEventFlag)
        return false; // Let DefWindowProc() handle Non Client messages.

    POINTER_PEN_INFO *penInfo = static_cast<POINTER_PEN_INFO *>(vPenInfo);

    RECT pRect, dRect;
    if (!QWindowsContext::user32dll.getPointerDeviceRects(penInfo->pointerInfo.sourceDevice, &pRect, &dRect))
        return false;

    const quint32 pointerId = penInfo->pointerInfo.pointerId;
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
        qCDebug(lcQpaEvents).noquote().nospace() << showbase
            << __FUNCTION__ << " pointerId=" << pointerId
            << " globalPos=" << globalPos << " localPos=" << localPos << " hiResGlobalPos=" << hiResGlobalPos
            << " message=" << hex << msg.message
            << " flags=" << hex << penInfo->pointerInfo.pointerFlags;

    const QTabletEvent::TabletDevice device = QTabletEvent::Stylus;
    QTabletEvent::PointerType type;
    Qt::MouseButtons mouseButtons;

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
        QWindowSystemInterface::handleTabletEnterProximityEvent(device, type, pointerId);
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
        QWindowSystemInterface::handleTabletLeaveProximityEvent(device, type, pointerId);
        break;
    case WM_POINTERDOWN:
    case WM_POINTERUP:
    case WM_POINTERUPDATE: {
        QWindow *target = QGuiApplicationPrivate::tabletDevicePoint(pointerId).target; // Pass to window that grabbed it.
        if (!target && m_windowUnderPointer)
            target = m_windowUnderPointer;
        if (!target)
            target = window;

        if (m_needsEnterOnPointerUpdate) {
            m_needsEnterOnPointerUpdate = false;
            if (window != m_currentWindow) {
                QWindowSystemInterface::handleEnterEvent(window, localPos, globalPos);
                m_currentWindow = window;
                if (QWindowsWindow *wumPlatformWindow = QWindowsWindow::windowsWindowOf(target))
                    wumPlatformWindow->applyCursor();
            }
        }
        const Qt::KeyboardModifiers keyModifiers = QWindowsKeyMapper::queryKeyboardModifiers();

        QWindowSystemInterface::handleTabletEvent(target, localPos, hiResGlobalPos, device, type, mouseButtons,
                                                  pressure, xTilt, yTilt, tangentialPressure, rotation, z,
                                                  pointerId, keyModifiers);
        break;
    }
    }
    return true;
}

// SetCursorPos()/TrackMouseEvent() will generate old-style WM_MOUSE messages. Handle them here.
bool QWindowsPointerHandler::translateMouseEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et, MSG msg, LRESULT *result)
{
    Q_UNUSED(et);

    *result = 0;
    if (msg.message != WM_MOUSELEAVE && msg.message != WM_MOUSEMOVE)
        return false;

    const QPoint localPos(GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
    const QPoint globalPos = QWindowsGeometryHint::mapToGlobal(hwnd, localPos);

    QWindowsWindow *platformWindow = static_cast<QWindowsWindow *>(window->handle());

    if (msg.message == WM_MOUSELEAVE) {
        if (window == m_currentWindow) {
            QWindowSystemInterface::handleLeaveEvent(window);
            m_windowUnderPointer = nullptr;
            m_currentWindow = nullptr;
            platformWindow->applyCursor();
        }
        return false;
    }

    // Windows sends a mouse move with no buttons pressed to signal "Enter"
    // when a window is shown over the cursor. Discard the event and only use
    // it for generating QEvent::Enter to be consistent with other platforms -
    // X11 and macOS.
    static QPoint lastMouseMovePos;
    const bool discardEvent = msg.wParam == 0 && (m_windowUnderPointer.isNull() || globalPos == lastMouseMovePos);
    lastMouseMovePos = globalPos;

    QWindow *currentWindowUnderPointer = getWindowUnderPointer(window, globalPos);

    if (currentWindowUnderPointer != m_windowUnderPointer) {
        if (m_windowUnderPointer && m_windowUnderPointer == m_currentWindow) {
            QWindowSystemInterface::handleLeaveEvent(m_windowUnderPointer);
            m_currentWindow = nullptr;
        }

        if (currentWindowUnderPointer) {
            if (currentWindowUnderPointer != m_currentWindow) {
                QWindowSystemInterface::handleEnterEvent(currentWindowUnderPointer, localPos, globalPos);
                m_currentWindow = currentWindowUnderPointer;
                if (QWindowsWindow *wumPlatformWindow = QWindowsWindow::windowsWindowOf(currentWindowUnderPointer))
                    wumPlatformWindow->applyCursor();
                trackLeave(hwnd);
            }
        } else {
            platformWindow->applyCursor();
        }
        m_windowUnderPointer = currentWindowUnderPointer;
    }

    const Qt::KeyboardModifiers keyModifiers = QWindowsKeyMapper::queryKeyboardModifiers();
    const Qt::MouseButtons mouseButtons = queryMouseButtons();

    if (!discardEvent)
        QWindowSystemInterface::handleMouseEvent(window, localPos, globalPos, mouseButtons, Qt::NoButton, QEvent::MouseMove,
                                                 keyModifiers, Qt::MouseEventNotSynthesized);
    return false;
}

QT_END_NAMESPACE
