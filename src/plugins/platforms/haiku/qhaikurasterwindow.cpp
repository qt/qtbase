/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
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

#include "qhaikurasterwindow.h"

#include "qhaikukeymapper.h"

#include <View.h>
#include <Window.h>

#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_METATYPE(QEvent::Type)
Q_DECLARE_METATYPE(Qt::MouseButtons)
Q_DECLARE_METATYPE(Qt::MouseEventSource)
Q_DECLARE_METATYPE(Qt::KeyboardModifiers)
Q_DECLARE_METATYPE(Qt::Orientation)

HaikuViewProxy::HaikuViewProxy(BWindow *window, QObject *parent)
    : QObject(parent)
    , BView(BRect(0, 0, window->Bounds().right, window->Bounds().bottom), 0, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS)
{
}

void HaikuViewProxy::MessageReceived(BMessage *message)
{
    switch (message->what) {
    case B_MOUSE_WHEEL_CHANGED:
        {
             float deltaX = 0;
             if (message->FindFloat("be:wheel_delta_x", &deltaX) != B_OK)
                 deltaX = 0;

             float deltaY = 0;
             if (message->FindFloat("be:wheel_delta_y", &deltaY) != B_OK)
                 deltaY = 0;

             if (deltaX != 0 || deltaY != 0) {
                BPoint localPos;
                uint32 keyState = 0;
                GetMouse(&localPos, &keyState);
                const Qt::KeyboardModifiers keyboardModifiers = keyStateToModifiers(modifiers());

                const BPoint globalPos = ConvertToScreen(localPos);
                const QPoint globalPosition = QPoint(globalPos.x, globalPos.y);
                const QPoint localPosition = QPoint(localPos.x, localPos.y);

                if (deltaX != 0)
                    Q_EMIT wheelEvent(localPosition, globalPosition, (deltaX * -120), Qt::Horizontal, keyboardModifiers);

                if (deltaY != 0)
                    Q_EMIT wheelEvent(localPosition, globalPosition, (deltaY * -120), Qt::Vertical, keyboardModifiers);
             }
             break;
        }
    default:
        BView::MessageReceived(message);
        break;
    }

}

void HaikuViewProxy::Draw(BRect updateRect)
{
    BView::Draw(updateRect);

    Q_EMIT drawRequest(QRect(updateRect.left, updateRect.top, updateRect.Width(), updateRect.Height()));
}

void HaikuViewProxy::MouseDown(BPoint localPos)
{
    BPoint dummyPos;
    uint32 keyState = 0;
    GetMouse(&dummyPos, &keyState);

    const Qt::MouseButtons mouseButtons = keyStateToMouseButtons(keyState);
    const Qt::KeyboardModifiers keyboardModifiers = keyStateToModifiers(modifiers());
    const Qt::MouseEventSource source = Qt::MouseEventNotSynthesized;

    const BPoint globalPos = ConvertToScreen(localPos);
    const QPoint globalPosition = QPoint(globalPos.x, globalPos.y);
    const QPoint localPosition = QPoint(localPos.x, localPos.y);

    Q_EMIT mouseEvent(localPosition, globalPosition, mouseButtons, keyboardModifiers, source);
}

void HaikuViewProxy::MouseUp(BPoint localPos)
{
    BPoint dummyPos;
    uint32 keyState = 0;
    GetMouse(&dummyPos, &keyState);

    const Qt::MouseButtons mouseButtons = keyStateToMouseButtons(keyState);
    const Qt::KeyboardModifiers keyboardModifiers = keyStateToModifiers(modifiers());
    const Qt::MouseEventSource source = Qt::MouseEventNotSynthesized;

    const BPoint globalPos = ConvertToScreen(localPos);
    const QPoint globalPosition = QPoint(globalPos.x, globalPos.y);
    const QPoint localPosition = QPoint(localPos.x, localPos.y);

    Q_EMIT mouseEvent(localPosition, globalPosition, mouseButtons, keyboardModifiers, source);
}

void HaikuViewProxy::MouseMoved(BPoint pos, uint32 code, const BMessage *dragMessage)
{
    switch (code) {
    case B_INSIDE_VIEW:
        {
            BPoint dummyPos;
            uint32 keyState = 0;
            GetMouse(&dummyPos, &keyState);

            const Qt::MouseButtons mouseButtons = keyStateToMouseButtons(keyState);
            const Qt::KeyboardModifiers keyboardModifiers = keyStateToModifiers(modifiers());
            const Qt::MouseEventSource source = Qt::MouseEventNotSynthesized;

            const BPoint globalPos = ConvertToScreen(pos);
            const QPoint globalPosition = QPoint(globalPos.x, globalPos.y);
            const QPoint localPosition = QPoint(pos.x, pos.y);

            Q_EMIT mouseEvent(localPosition, globalPosition, mouseButtons, keyboardModifiers, source);
        }
        break;
    case B_ENTERED_VIEW:
        Q_EMIT enteredView();
        break;
    case B_EXITED_VIEW:
        Q_EMIT exitedView();
        break;
    }

    BView::MouseMoved(pos, code, dragMessage);
}

void HaikuViewProxy::KeyDown(const char*, int32)
{
    handleKeyEvent(QEvent::KeyPress, Window()->CurrentMessage());
}

void HaikuViewProxy::KeyUp(const char*, int32)
{
    handleKeyEvent(QEvent::KeyRelease, Window()->CurrentMessage());
}

Qt::MouseButtons HaikuViewProxy::keyStateToMouseButtons(uint32 keyState) const
{
    Qt::MouseButtons mouseButtons(Qt::NoButton);
    if (keyState & B_PRIMARY_MOUSE_BUTTON)
        mouseButtons |= Qt::LeftButton;
    if (keyState & B_SECONDARY_MOUSE_BUTTON)
        mouseButtons |= Qt::RightButton;
    if (keyState & B_TERTIARY_MOUSE_BUTTON)
        mouseButtons |= Qt::MiddleButton;

    return mouseButtons;
}

Qt::KeyboardModifiers HaikuViewProxy::keyStateToModifiers(uint32 keyState) const
{
    Qt::KeyboardModifiers modifiers(Qt::NoModifier);

    if (keyState & B_SHIFT_KEY)
        modifiers |= Qt::ShiftModifier;
    if (keyState & B_CONTROL_KEY)
        modifiers |= Qt::AltModifier;
    if (keyState & B_COMMAND_KEY)
        modifiers |= Qt::ControlModifier;

    return modifiers;
}

void HaikuViewProxy::handleKeyEvent(QEvent::Type type, BMessage *message)
{
    int32 key = 0;
    uint32 code = 0;
    const char *bytes = nullptr;
    QString text;

    if (message) {
        if (message->FindString("bytes", &bytes) == B_OK) {
            text = QString::fromLocal8Bit(bytes, strlen(bytes));
        }

        if (message->FindInt32("key", &key) == B_OK) {
            code = QHaikuKeyMapper::translateKeyCode(key, (modifiers() & B_NUM_LOCK));
        }
    }

    const Qt::KeyboardModifiers keyboardModifiers = keyStateToModifiers(modifiers());

    Q_EMIT keyEvent(type, code, keyboardModifiers, text);
}


QHaikuRasterWindow::QHaikuRasterWindow(QWindow *window)
    : QHaikuWindow(window)
{
    qRegisterMetaType<QEvent::Type>();
    qRegisterMetaType<Qt::MouseButtons>();
    qRegisterMetaType<Qt::MouseEventSource>();
    qRegisterMetaType<Qt::KeyboardModifiers>();
    qRegisterMetaType<Qt::Orientation>();

    HaikuViewProxy *haikuView = new HaikuViewProxy(m_window);
    connect(haikuView, SIGNAL(mouseEvent(QPoint,QPoint,Qt::MouseButtons,Qt::KeyboardModifiers,Qt::MouseEventSource)),
            this, SLOT(haikuMouseEvent(QPoint,QPoint,Qt::MouseButtons,Qt::KeyboardModifiers,Qt::MouseEventSource)));
    connect(haikuView, SIGNAL(wheelEvent(QPoint,QPoint,int,Qt::Orientation,Qt::KeyboardModifiers)),
            this, SLOT(haikuWheelEvent(QPoint,QPoint,int,Qt::Orientation,Qt::KeyboardModifiers)));
    connect(haikuView, SIGNAL(keyEvent(QEvent::Type,int,Qt::KeyboardModifiers,QString)),
            this, SLOT(haikuKeyEvent(QEvent::Type,int,Qt::KeyboardModifiers,QString)));
    connect(haikuView, SIGNAL(enteredView()), this, SLOT(haikuEnteredView()));
    connect(haikuView, SIGNAL(exitedView()), this, SLOT(haikuExitedView()));
    connect(haikuView, SIGNAL(drawRequest(QRect)), this, SLOT(haikuDrawRequest(QRect)));

    m_view = haikuView;

    m_window->LockLooper();
    m_window->AddChild(m_view);
    m_view->MakeFocus();
    m_window->UnlockLooper();
}

QHaikuRasterWindow::~QHaikuRasterWindow()
{
    m_window->LockLooper();
    m_view->RemoveSelf();
    m_window->UnlockLooper();

    delete m_view;
    m_view = nullptr;
}

BView* QHaikuRasterWindow::nativeViewHandle() const
{
    return m_view;
}

QT_END_NAMESPACE
