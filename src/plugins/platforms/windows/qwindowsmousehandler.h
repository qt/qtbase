// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSMOUSEHANDLER_H
#define QWINDOWSMOUSEHANDLER_H

#include "qtwindowsglobal.h"
#include <QtCore/qt_windows.h>

#include <QtCore/qpointer.h>
#include <QtCore/qhash.h>
#include <QtCore/qsharedpointer.h>
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

class QWindow;
class QPointingDevice;

class QWindowsMouseHandler
{
    Q_DISABLE_COPY_MOVE(QWindowsMouseHandler)
public:
    using QPointingDevicePtr = QSharedPointer<QPointingDevice>;

    QWindowsMouseHandler();

    const QPointingDevicePtr &touchDevice() const { return m_touchDevice; }
    void setTouchDevice(const QPointingDevicePtr &d) { m_touchDevice = d; }

    bool translateMouseEvent(QWindow *widget, HWND hwnd,
                             QtWindows::WindowsEventType t, MSG msg,
                             LRESULT *result);
    bool translateTouchEvent(QWindow *widget, HWND hwnd,
                             QtWindows::WindowsEventType t, MSG msg,
                             LRESULT *result);
    bool translateGestureEvent(QWindow *window, HWND hwnd,
                               QtWindows::WindowsEventType,
                               MSG msg, LRESULT *);
    bool translateScrollEvent(QWindow *window, HWND hwnd,
                              MSG msg, LRESULT *result);

    static inline Qt::MouseButtons keyStateToMouseButtons(WPARAM);
    static inline Qt::KeyboardModifiers keyStateToModifiers(int);
    static inline int mouseButtonsToKeyState(Qt::MouseButtons);

    static Qt::MouseButtons queryMouseButtons();
    QWindow *windowUnderMouse() const { return m_windowUnderMouse.data(); }
    void clearWindowUnderMouse() { m_windowUnderMouse = nullptr; }
    void clearEvents();

    static const QPointingDevice *primaryMouse();

private:
    inline bool translateMouseWheelEvent(QWindow *window, HWND hwnd,
                                         MSG msg, LRESULT *result);

    QPointer<QWindow> m_windowUnderMouse;
    QPointer<QWindow> m_trackedWindow;
    QHash<DWORD, int> m_touchInputIDToTouchPointID;
    QHash<int, QPointF> m_lastTouchPositions;
    QPointingDevicePtr m_touchDevice;
    bool m_leftButtonDown = false;
    QWindow *m_previousCaptureWindow = nullptr;
    QEvent::Type m_lastEventType = QEvent::None;
    Qt::MouseButton m_lastEventButton = Qt::NoButton;
};

Qt::MouseButtons QWindowsMouseHandler::keyStateToMouseButtons(WPARAM wParam)
{
    Qt::MouseButtons mb(Qt::NoButton);
    if (wParam & MK_LBUTTON)
        mb |= Qt::LeftButton;
    if (wParam & MK_MBUTTON)
        mb |= Qt::MiddleButton;
    if (wParam & MK_RBUTTON)
        mb |= Qt::RightButton;
    if (wParam & MK_XBUTTON1)
        mb |= Qt::XButton1;
    if (wParam & MK_XBUTTON2)
        mb |= Qt::XButton2;
    return mb;
}

Qt::KeyboardModifiers QWindowsMouseHandler::keyStateToModifiers(int wParam)
{
    Qt::KeyboardModifiers mods(Qt::NoModifier);
    if (wParam & MK_CONTROL)
      mods |= Qt::ControlModifier;
    if (wParam & MK_SHIFT)
      mods |= Qt::ShiftModifier;
    if (GetKeyState(VK_MENU) < 0)
      mods |= Qt::AltModifier;
    return mods;
}

int QWindowsMouseHandler::mouseButtonsToKeyState(Qt::MouseButtons mb)
{
    int result = 0;
    if (mb & Qt::LeftButton)
        result |= MK_LBUTTON;
    if (mb & Qt::MiddleButton)
        result |= MK_MBUTTON;
    if (mb & Qt::RightButton)
        result |= MK_RBUTTON;
    if (mb & Qt::XButton1)
        result |= MK_XBUTTON1;
    if (mb & Qt::XButton2)
        result |= MK_XBUTTON2;
    return result;
}

QT_END_NAMESPACE

#endif // QWINDOWSMOUSEHANDLER_H
