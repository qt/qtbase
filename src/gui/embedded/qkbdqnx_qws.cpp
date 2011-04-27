/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qkbdqnx_qws.h"
#include "QtCore/qsocketnotifier.h"
#include "QtCore/qdebug.h"

#include <sys/dcmd_input.h>
#include <photon/keycodes.h>

#include "qplatformdefs.h"
#include <errno.h>


QT_BEGIN_NAMESPACE

/*!
    \class QWSQnxKeyboardHandler
    \preliminary
    \ingroup qws
    \since 4.6
    \internal

    \brief The QWSQnxKeyboardHandler class implements a keyboard driver
    for the QNX \c{devi-hid} input manager.

    To be able to compile this mouse handler, \l{Qt for Embedded Linux}
    must be configured with the \c -qt-kbd-qnx option, see the
    \l{Qt for Embedded Linux Character Input} documentation for details.

    In order to use this keyboard handler, the \c{devi-hid} input manager
    must be set up and run with the resource manager interface (option \c{-r}).
    Also, Photon must not be running.

    Example invocation from command line: \c{/usr/photon/bin/devi-hid -Pr kbd mouse}
    Note that after running \c{devi-hid}, you will not be able to use the local
    shell anymore. It is suggested to run the command in a shell scrip, that launches
    a Qt application after invocation of \c{devi-hid}.

    To make \l{Qt for Embedded Linux} explicitly choose the qnx keyboard
    handler, set the QWS_KEYBOARD environment variable to \c{qnx}. By default,
    the first keyboard device (\c{/dev/devi/keyboard0}) is used. To override, pass a device
    name as the first and only parameter, for example
    \c{QWS_KEYBOARD=qnx:/dev/devi/keyboard1; export QWS_KEYBOARD}.

    \sa {Qt for Embedded Linux Character Input}, {Qt for Embedded Linux}
*/

/*!
    Constructs a keyboard handler for the specified \a device, defaulting to
    \c{/dev/devi/keyboard0}.

    Note that you should never instanciate this class, instead let QKbdDriverFactory
    handle the keyboard handlers.

    \sa QKbdDriverFactory
 */
QWSQnxKeyboardHandler::QWSQnxKeyboardHandler(const QString &device)
{
    // open the keyboard device
    keyboardFD = QT_OPEN(device.isEmpty() ? "/dev/devi/keyboard0" : device.toLatin1().constData(),
                         QT_OPEN_RDONLY);
    if (keyboardFD == -1) {
        qErrnoWarning(errno, "QWSQnxKeyboardHandler: Unable to open device");
        return;
    }

    // create a socket notifier so we'll wake up whenever keyboard input is detected.
    QSocketNotifier *notifier = new QSocketNotifier(keyboardFD, QSocketNotifier::Read, this);
    connect(notifier, SIGNAL(activated(int)), SLOT(socketActivated()));

    qDebug() << "QWSQnxKeyboardHandler: connected.";

}

/*!
    Destroys this keyboard handler and closes the connection to the keyboard device.
 */
QWSQnxKeyboardHandler::~QWSQnxKeyboardHandler()
{
    QT_CLOSE(keyboardFD);
}

/*! \internal
    Translates the QNX keyboard events to Qt keyboard events
 */
void QWSQnxKeyboardHandler::socketActivated()
{
    _keyboard_packet packet;

    // read one keyboard event
    int bytesRead = QT_READ(keyboardFD, &packet, sizeof(_keyboard_packet));
    if (bytesRead == -1) {
        qErrnoWarning(errno, "QWSQnxKeyboardHandler::socketActivated(): Unable to read data.");
        return;
    }

    // the bytes read must be the size of a keyboard packet
    Q_ASSERT(bytesRead == sizeof(_keyboard_packet));

#if 0
    qDebug() << "keyboard got scancode"
             << hex << packet.data.modifiers
             << packet.data.flags
             << packet.data.key_cap
             << packet.data.key_sym
             << packet.data.key_scan;
#endif

    // QNX is nice enough to translate the raw keyboard data into a QNX data structure
    // Now we just have to translate it into a format Qt understands.

    // figure out whether it's a press
    bool isPress = packet.data.key_cap & KEY_DOWN;
    // figure out whether the key is still pressed and the key event is repeated
    bool isRepeat = packet.data.key_cap & KEY_REPEAT;

    Qt::Key key = Qt::Key_unknown;
    int unicode = 0xffff;

    // TODO - this switch is not complete!
    switch (packet.data.key_scan) {
    case KEYCODE_SPACE: key = Qt::Key_Space; unicode = 0x20; break;
    case KEYCODE_F1: key = Qt::Key_F1; break;
    case KEYCODE_F2: key = Qt::Key_F2; break;
    case KEYCODE_F3: key = Qt::Key_F3; break;
    case KEYCODE_F4: key = Qt::Key_F4; break;
    case KEYCODE_F5: key = Qt::Key_F5; break;
    case KEYCODE_F6: key = Qt::Key_F6; break;
    case KEYCODE_F7: key = Qt::Key_F7; break;
    case KEYCODE_F8: key = Qt::Key_F8; break;
    case KEYCODE_F9: key = Qt::Key_F9; break;
    case KEYCODE_F10: key = Qt::Key_F10; break;
    case KEYCODE_F11: key = Qt::Key_F11; break;
    case KEYCODE_F12: key = Qt::Key_F12; break;
    case KEYCODE_BACKSPACE: key = Qt::Key_Backspace; break;
    case KEYCODE_TAB: key = Qt::Key_Tab; break;
    case KEYCODE_RETURN: key = Qt::Key_Return; break;
    case KEYCODE_KP_ENTER: key = Qt::Key_Enter; break;
    case KEYCODE_UP:
    case KEYCODE_KP_UP:
        key = Qt::Key_Up; break;
    case KEYCODE_DOWN:
    case KEYCODE_KP_DOWN:
        key = Qt::Key_Down; break;
    case KEYCODE_LEFT:
    case KEYCODE_KP_LEFT:
        key = Qt::Key_Left; break;
    case KEYCODE_RIGHT:
    case KEYCODE_KP_RIGHT:
        key = Qt::Key_Right; break;
    case KEYCODE_HOME:
    case KEYCODE_KP_HOME:
        key = Qt::Key_Home; break;
    case KEYCODE_END:
    case KEYCODE_KP_END:
        key = Qt::Key_End; break;
    case KEYCODE_PG_UP:
    case KEYCODE_KP_PG_UP:
        key = Qt::Key_PageUp; break;
    case KEYCODE_PG_DOWN:
    case KEYCODE_KP_PG_DOWN:
        key = Qt::Key_PageDown; break;
    case KEYCODE_INSERT:
    case KEYCODE_KP_INSERT:
        key = Qt::Key_Insert; break;
    case KEYCODE_DELETE:
    case KEYCODE_KP_DELETE:
        key = Qt::Key_Delete; break;
    case KEYCODE_ESCAPE:
        key = Qt::Key_Escape; break;
    default: // none of the above, try the key_scan directly
        unicode = packet.data.key_scan;
        break;
    }

    // figure out the modifiers that are currently pressed
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    if (packet.data.flags & KEYMOD_SHIFT)
        modifiers |= Qt::ShiftModifier;
    if (packet.data.flags & KEYMOD_CTRL)
        modifiers |= Qt::ControlModifier;
    if (packet.data.flags & KEYMOD_ALT)
        modifiers |= Qt::AltModifier;

    // if the unicode value is not ascii, we ignore it.
    // TODO - do a complete mapping between all QNX scan codes and Qt codes
    if (unicode != 0xffff && !isascii(unicode))
        return; // unprintable character

    // call processKeyEvent. This is where all the magic happens to insert a
    // key event into Qt's event loop.
    // Note that for repeated key events, isPress must be true
    // (on QNX, isPress is not set when the key event is repeated).
    processKeyEvent(unicode, key, modifiers, isPress || isRepeat, isRepeat);
}

QT_END_NAMESPACE
