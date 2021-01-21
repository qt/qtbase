/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QWINDOWSPOINTERHANDLER_H
#define QWINDOWSPOINTERHANDLER_H

#include "qtwindowsglobal.h"
#include <QtCore/qt_windows.h>

#include <QtCore/qpointer.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qhash.h>
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

class QWindow;
class QTouchDevice;

class QWindowsPointerHandler
{
    Q_DISABLE_COPY_MOVE(QWindowsPointerHandler)
public:
    QWindowsPointerHandler() = default;
    bool translatePointerEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et, MSG msg, LRESULT *result);
    bool translateMouseEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et, MSG msg, LRESULT *result);
    QTouchDevice *touchDevice() const { return m_touchDevice; }
    QTouchDevice *ensureTouchDevice();
    QWindow *windowUnderMouse() const { return m_windowUnderPointer.data(); }
    void clearWindowUnderMouse() { m_windowUnderPointer = nullptr; }
    void clearEvents();

private:
    bool translateTouchEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et, MSG msg, PVOID vTouchInfo, unsigned int count);
    bool translatePenEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et, MSG msg, PVOID vPenInfo);
    bool translateMouseWheelEvent(QWindow *window, QWindow *currentWindowUnderPointer, MSG msg, QPoint globalPos, Qt::KeyboardModifiers keyModifiers);
    void handleCaptureRelease(QWindow *window, QWindow *currentWindowUnderPointer, HWND hwnd, QEvent::Type eventType, Qt::MouseButtons mouseButtons);
    void handleEnterLeave(QWindow *window, QWindow *currentWindowUnderPointer, QPoint globalPos);

    QTouchDevice *m_touchDevice = nullptr;
    QHash<int, QPointF> m_lastTouchPositions;
    QHash<DWORD, int> m_touchInputIDToTouchPointID;
    QPointer<QWindow> m_windowUnderPointer;
    QPointer<QWindow> m_currentWindow;
    QWindow *m_previousCaptureWindow = nullptr;
    bool m_needsEnterOnPointerUpdate = false;
    QEvent::Type m_lastEventType = QEvent::None;
    Qt::MouseButton m_lastEventButton = Qt::NoButton;
    DWORD m_pointerType = 0;
};

QT_END_NAMESPACE

#endif // QWINDOWSPOINTERHANDLER_H
