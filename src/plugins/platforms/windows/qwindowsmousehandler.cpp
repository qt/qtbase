/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwindowsmousehandler.h"
#include "qwindowskeymapper.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "qwindowsintegration.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <QtCore/QDebug>
#include <QtCore/QScopedArrayPointer>

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
                while (PeekMessage(&keyMsg, 0, WM_KEYFIRST, WM_KEYLAST,
                                   PM_NOREMOVE)) {
                    if (keyMsg.time < mouseMsg.time) {
                        if ((keyMsg.lParam & 0xC0000000) == 0x40000000) {
                            PeekMessage(&keyMsg, 0, keyMsg.message,
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
                ClientToScreen(msg->hwnd, &(msg->pt));
                // Remove the mouse move message
                PeekMessage(&mouseMsg, msg->hwnd, WM_MOUSEMOVE,
                            WM_MOUSEMOVE, PM_REMOVE);
            } else {
                break; // there was no more WM_MOUSEMOVE event
            }
        }
    }
}

/*!
    \class QWindowsMouseHandler
    \brief Windows mouse handler

    Dispatches mouse and touch events. Separate for code cleanliness.

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsMouseHandler::QWindowsMouseHandler() :
    m_windowUnderMouse(0),
    m_touchDevice(0)
{
}

Qt::MouseButtons QWindowsMouseHandler::queryMouseButtons()
{
    Qt::MouseButtons result = 0;
    const bool mouseSwapped = GetSystemMetrics(SM_SWAPBUTTON);
    if (GetAsyncKeyState(VK_LBUTTON) < 0)
        result |= mouseSwapped ? Qt::RightButton: Qt::LeftButton;
    if (GetAsyncKeyState(VK_RBUTTON) < 0)
        result |= mouseSwapped ? Qt::LeftButton : Qt::RightButton;
    if (GetAsyncKeyState(VK_MBUTTON) < 0)
        result |= Qt::MidButton;
    return result;
}

bool QWindowsMouseHandler::translateMouseEvent(QWindow *window, HWND hwnd,
                                               QtWindows::WindowsEventType et,
                                               MSG msg, LRESULT *result)
{
    if (et == QtWindows::MouseWheelEvent)
        return translateMouseWheelEvent(window, hwnd, msg, result);

    const QPoint winEventPosition(GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
    if (et & QtWindows::NonClientEventFlag) {
        const QPoint globalPosition = winEventPosition;
        const QPoint clientPosition = QWindowsGeometryHint::mapFromGlobal(hwnd, globalPosition);
        const Qt::MouseButtons buttons = QWindowsMouseHandler::queryMouseButtons();
        QWindowSystemInterface::handleFrameStrutMouseEvent(window, clientPosition,
                                                           globalPosition, buttons,
                                                           QWindowsKeyMapper::queryKeyboardModifiers());
        return false; // Allow further event processing (dragging of windows).
    }

    *result = 0;
    if (msg.message == WM_MOUSELEAVE) {
        // When moving out of a child, MouseMove within parent is received first
        // (see below)
        if (QWindowsContext::verboseEvents)
            qDebug() << "WM_MOUSELEAVE for " << window << " current= " << m_windowUnderMouse;
        if (window == m_windowUnderMouse) {
            QWindowSystemInterface::handleLeaveEvent(window);
            m_windowUnderMouse = 0;
        }
        return true;
    }
    compressMouseMove(&msg);
    // Eat mouse move after size grip drag.
    if (msg.message == WM_MOUSEMOVE) {
        QWindowsWindow *platformWindow = static_cast<QWindowsWindow *>(window->handle());
        if (platformWindow->testFlag(QWindowsWindow::SizeGripOperation)) {
            MSG mouseMsg;
            while (PeekMessage(&mouseMsg, platformWindow->handle(), WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE)) ;
            platformWindow->clearFlag(QWindowsWindow::SizeGripOperation);
            return true;
        }
    }
    // Enter new window: track to generate leave event.
    if (m_windowUnderMouse != window) {
        // The tracking on m_windowUnderMouse might still be active and
        // trigger later on.
        if (m_windowUnderMouse) {
            if (QWindowsContext::verboseEvents)
                qDebug() << "Synthetic leave for " << m_windowUnderMouse;
            QWindowSystemInterface::handleLeaveEvent(m_windowUnderMouse);
        }
        m_windowUnderMouse = window;
        if (QWindowsContext::verboseEvents)
            qDebug() << "Entering " << window;
        QWindowsWindow::baseWindowOf(window)->applyCursor();
//#ifndef Q_OS_WINCE
        QWindowSystemInterface::handleEnterEvent(window);
#ifndef Q_OS_WINCE
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        tme.dwHoverTime = HOVER_DEFAULT; //
        if (!TrackMouseEvent(&tme))
            qWarning("TrackMouseEvent failed.");
#endif // !Q_OS_WINCE
    }
    const QPoint clientPosition = winEventPosition;
    QWindowSystemInterface::handleMouseEvent(window, clientPosition,
                                             QWindowsGeometryHint::mapToGlobal(hwnd, clientPosition),
                                             keyStateToMouseButtons((int)msg.wParam),
                                             QWindowsKeyMapper::queryKeyboardModifiers());
    return true;
}

bool QWindowsMouseHandler::translateMouseWheelEvent(QWindow *window, HWND,
                                                    MSG msg, LRESULT *)
{
    const Qt::MouseButtons buttons = keyStateToMouseButtons((int)msg.wParam);
    const Qt::KeyboardModifiers mods = keyStateToModifiers((int)msg.wParam);

    int delta;
    if (msg.message == WM_MOUSEWHEEL || msg.message == WM_MOUSEHWHEEL)
        delta = (short) HIWORD (msg.wParam);
    else
        delta = (int) msg.wParam;

    Qt::Orientation orientation = (msg.message == WM_MOUSEHWHEEL
                                  || (buttons & Qt::AltModifier)) ?
                                  Qt::Horizontal : Qt::Vertical;

    // according to the MSDN documentation on WM_MOUSEHWHEEL:
    // a positive value indicates that the wheel was rotated to the right;
    // a negative value indicates that the wheel was rotated to the left.
    // Qt defines this value as the exact opposite, so we have to flip the value!
    if (msg.message == WM_MOUSEHWHEEL)
        delta = -delta;

    const QPoint globalPos(GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
    // TODO: if there is a widget under the mouse and it is not shadowed
    // QWindow *receiver = windowAt(pos);
    // by modality, we send the event to it first.
    //synaptics touchpad shows its own widget at this position
    //so widgetAt() will fail with that HWND, try child of this widget
    // if (!receiver) receiver = window->childAt(pos);
    QWindow *receiver = window;
    QWindowSystemInterface::handleWheelEvent(receiver,
                                             QWindowsGeometryHint::mapFromGlobal(receiver, globalPos),
                                             globalPos,
                                             delta, orientation, mods);
    return true;
}

// from bool QApplicationPrivate::translateTouchEvent()
bool QWindowsMouseHandler::translateTouchEvent(QWindow *window, HWND,
                                               QtWindows::WindowsEventType,
                                               MSG msg, LRESULT *)
{
#ifndef Q_OS_WINCE
    typedef QWindowSystemInterface::TouchPoint QTouchPoint;
    typedef QList<QWindowSystemInterface::TouchPoint> QTouchPointList;

    const QRect screenGeometry = window->screen()->geometry();

    const int winTouchPointCount = msg.wParam;
    QScopedArrayPointer<TOUCHINPUT> winTouchInputs(new TOUCHINPUT[winTouchPointCount]);
    memset(winTouchInputs.data(), 0, sizeof(TOUCHINPUT) * winTouchPointCount);

    QTouchPointList touchPoints;
    touchPoints.reserve(winTouchPointCount);
    Qt::TouchPointStates allStates = 0;

    Q_ASSERT(QWindowsContext::user32dll.getTouchInputInfo);

    QWindowsContext::user32dll.getTouchInputInfo((HANDLE) msg.lParam, msg.wParam, winTouchInputs.data(), sizeof(TOUCHINPUT));
    for (int i = 0; i < winTouchPointCount; ++i) {
        const TOUCHINPUT &winTouchInput = winTouchInputs[i];
        QTouchPoint touchPoint;
        touchPoint.pressure = 1.0;
        touchPoint.id = m_touchInputIDToTouchPointID.value(winTouchInput.dwID, -1);
        if (touchPoint.id == -1) {
            touchPoint.id = m_touchInputIDToTouchPointID.size();
            m_touchInputIDToTouchPointID.insert(winTouchInput.dwID, touchPoint.id);
        }

        QPointF screenPos = QPointF(qreal(winTouchInput.x) / qreal(100.), qreal(winTouchInput.y) / qreal(100.));
        if (winTouchInput.dwMask & TOUCHINPUTMASKF_CONTACTAREA)
            touchPoint.area.setSize(QSizeF(qreal(winTouchInput.cxContact) / qreal(100.),
                                           qreal(winTouchInput.cyContact) / qreal(100.)));
        touchPoint.area.moveCenter(screenPos);

        if (winTouchInput.dwFlags & TOUCHEVENTF_DOWN) {
            touchPoint.state = Qt::TouchPointPressed;
        } else if (winTouchInput.dwFlags & TOUCHEVENTF_UP) {
            touchPoint.state = Qt::TouchPointReleased;
        } else {
            // TODO: Previous code checked"
            // screenPos == touchPoint.normalPosition -> Qt::TouchPointStationary, but
            // but touchPoint.normalPosition was never initialized?
            touchPoint.state = touchPoint.state;
        }

        touchPoint.normalPosition = QPointF(screenPos.x() / screenGeometry.width(),
                                 screenPos.y() / screenGeometry.height());

        allStates |= touchPoint.state;

        touchPoints.append(touchPoint);
    }

    QWindowsContext::user32dll.closeTouchInputHandle((HANDLE) msg.lParam);

    // all touch points released, forget the ids we've seen, they may not be reused
    if (allStates == Qt::TouchPointReleased)
        m_touchInputIDToTouchPointID.clear();

    if (!m_touchDevice) {
        m_touchDevice = new QTouchDevice;
        // TODO: Device used to be hardcoded to screen in previous code.
        m_touchDevice->setType(QTouchDevice::TouchScreen);
        m_touchDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::NormalizedPosition);
        QWindowSystemInterface::registerTouchDevice(m_touchDevice);
    }

    QWindowSystemInterface::handleTouchEvent(window,
                                             m_touchDevice,
                                             touchPoints);
    return true;
#else
    return false;
#endif
}

QT_END_NAMESPACE
