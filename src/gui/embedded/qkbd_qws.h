/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QKBD_QWS_H
#define QKBD_QWS_H

#include <QtGui/qapplication.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_QWS_KEYBOARD

class QWSKbPrivate;

class  Q_GUI_EXPORT QWSKeyboardHandler
{
public:
    QWSKeyboardHandler();
    QWSKeyboardHandler(const QString &device);
    virtual ~QWSKeyboardHandler();

    virtual void processKeyEvent(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                            bool isPress, bool autoRepeat);

    enum KeycodeAction {
        None               = 0,

        CapsLockOff        = 0x01000000,
        CapsLockOn         = 0x01000001,
        NumLockOff         = 0x02000000,
        NumLockOn          = 0x02000001,
        ScrollLockOff      = 0x03000000,
        ScrollLockOn       = 0x03000001,

        Reboot             = 0x04000000,

        PreviousConsole    = 0x05000000,
        NextConsole        = 0x05000001,
        SwitchConsoleFirst = 0x06000000,
        SwitchConsoleLast  = 0x0600007f,
        SwitchConsoleMask  = 0x0000007f,
    };

    KeycodeAction processKeycode(quint16 keycode, bool pressed, bool autorepeat);

protected:
    int transformDirKey(int key);
    void beginAutoRepeat(int uni, int code, Qt::KeyboardModifiers mod);
    void endAutoRepeat();

private:
    QWSKbPrivate *d;
};

#endif // QT_NO_QWS_KEYBOARD

QT_END_NAMESPACE

QT_END_HEADER

#endif // QKBD_QWS_H
