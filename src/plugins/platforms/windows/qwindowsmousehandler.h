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

#ifndef QWINDOWSMOUSEHANDLER_H
#define QWINDOWSMOUSEHANDLER_H

#include "qtwindowsglobal.h"
#include "qtwindows_additional.h"

#include <QtCore/QPointer>
#include <QtCore/QHash>

QT_BEGIN_NAMESPACE

class QWindow;
class QTouchDevice;

class QWindowsMouseHandler
{
    Q_DISABLE_COPY(QWindowsMouseHandler)
public:
    QWindowsMouseHandler();

    bool translateMouseEvent(QWindow *widget, HWND hwnd,
                             QtWindows::WindowsEventType t, MSG msg,
                             LRESULT *result);
    bool translateTouchEvent(QWindow *widget, HWND hwnd,
                             QtWindows::WindowsEventType t, MSG msg,
                             LRESULT *result);

    static inline Qt::MouseButtons keyStateToMouseButtons(int);
    static inline Qt::KeyboardModifiers keyStateToModifiers(int);
    static inline int mouseButtonsToKeyState(Qt::MouseButtons);

    static Qt::MouseButtons queryMouseButtons();
    QWindow *windowUnderMouse() const { return m_windowUnderMouse.data(); }

private:
    inline bool translateMouseWheelEvent(QWindow *window, HWND hwnd,
                                         MSG msg, LRESULT *result);

    QPointer<QWindow> m_windowUnderMouse;
    QHash<DWORD, int> m_touchInputIDToTouchPointID;
    QTouchDevice *m_touchDevice;
};

Qt::MouseButtons QWindowsMouseHandler::keyStateToMouseButtons(int wParam)
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
