/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWASMEVENTTRANSLATOR_H
#define QWASMEVENTTRANSLATOR_H

#include <QtCore/qobject.h>
#include <QtCore/qrect.h>
#include <QtCore/qpoint.h>
#include <emscripten/html5.h>
#include "qwasmwindow.h"
#include <QtGui/qtouchdevice.h>

QT_BEGIN_NAMESPACE

class QWindow;

class QWasmEventTranslator : public QObject
{
    Q_OBJECT

    enum KeyCode {
        // numpad
        KeyNumPad0 = 0x60,
        KeyNumPad1 = 0x61,
        KeyNumPad2 = 0x62,
        KeyNumPad3 = 0x63,
        KeyNumPad4 = 0x64,
        KeyNumPad5 = 0x65,
        KeyNumPad6 = 0x66,
        KeyNumPad7 = 0x67,
        KeyNumPad8 = 0x68,
        KeyNumPad9 = 0x69,
        KeyMultiply = 0x6A,
        KeyAdd = 0x6B,
        KeySeparator = 0x6C,
        KeySubtract = 0x6D,
        KeyDecimal = 0x6E,
        KeyDivide = 0x6F,
        KeyMeta = 0x5B,
        KeyMetaRight = 0x5C,
        ////////
        KeyClear = 0x90,
        KeyEnter = 0xD,
        KeyBackSpace = 0x08,
        KeyCancel = 0x03,
        KeyTab = 0x09,
        KeyShift = 0x10,
        KeyControl = 0x11,
        KeyAlt = 0x12,
        KeyPause = 0x13,
        KeyCapsLock = 0x14,
        KeyEscape = 0x1B,
        KeySpace = 0x20,
        KeyPageUp = 0x21,
        KeyPageDown = 0x22,
        KeyEnd = 0x23,
        KeyHome = 0x24,
        KeyLeft = 0x25,
        KeyUp = 0x26,
        KeyRight = 0x27,
        KeyDown = 0x28,
        KeyComma = 0xBC,
        KeyPeriod = 0xBE,
        KeySlash = 0xBF,
        KeyZero = 0x30,
        KeyOne = 0x31,
        KeyTwo = 0x32,
        KeyThree = 0x33,
        KeyFour = 0x34,
        KeyFive = 0x35,
        KeySix = 0x36,
        KeySeven = 0x37,
        KeyEight = 0x38,
        KeyNine = 0x39,
        KeyBrightnessDown = 0xD8,
        KeyBrightnessUp = 0xD9,
        KeyMediaTrackPrevious = 0xB1,
        KeyMediaPlayPause = 0xB3,
        KeyMediaTrackNext = 0xB0,
        KeyAudioVolumeMute = 0xAD,
        KeyAudioVolumeDown = 0xAE,
        KeyAudioVolumeUp = 0xAF,
        KeySemiColon = 0xBA,
        KeyEquals = 0xBB,
        KeyMinus = 0xBD,
        KeyA = 0x41,
        KeyB = 0x42,
        KeyC = 0x43,
        KeyD = 0x44,
        KeyE = 0x45,
        KeyF = 0x46,
        KeyG = 0x47,
        KeyH = 0x48,
        KeyI = 0x49,
        KeyJ = 0x4A,
        KeyK = 0x4B,
        KeyL = 0x4C,
        KeyM = 0x4D,
        KeyN = 0x4E,
        KeyO = 0x4F,
        KeyP = 0x50,
        KeyQ = 0x51,
        KeyR = 0x52,
        KeyS = 0x53,
        KeyT = 0x54,
        KeyU = 0x55,
        KeyV = 0x56,
        KeyW = 0x57,
        KeyX = 0x58,
        KeyY = 0x59,
        KeyZ = 0x5A,
        KeyOpenBracket = 0xDB,
        KeyBackSlash = 0xDC,
        KeyCloseBracket = 0xDD,
        KeyF1 = 0x70,
        KeyF2 = 0x71,
        KeyF3 = 0x72,
        KeyF4 = 0x73,
        KeyF5 = 0x74,
        KeyF6 = 0x75,
        KeyF7 = 0x76,
        KeyF8 = 0x77,
        KeyF9 = 0x78,
        KeyF10 = 0x79,
        KeyF11 = 0x7A,
        KeyF12 = 0x7B,
        KeyDelete = 0x2E,
        KeyNumLock = 0x90,
        KeyScrollLock = 0x91,
        KeyPrintScreen = 0x9A,
        KeyInsert = 0x9B,
        KeyHelp = 0x9C,
        KeyBackQuote = 0xC0,
        KeyQuote = 0xDE,
        KeyFinal = 0x18,
        KeyConvert = 0x1C,
        KeyNonConvert = 0x1D,
        KeyAccept = 0x1E,
        KeyModeChange = 0x1F,
        KeyKana = 0x15,
        KeyKanji = 0x19,
        KeyUndefined = 0x0
      };

public:

    explicit QWasmEventTranslator(QObject *parent = 0);

    static int keyboard_cb(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData);
    static int mouse_cb(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData);
    static int focus_cb(int eventType, const EmscriptenFocusEvent *focusEvent, void *userData);
    static int wheel_cb(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData);

    static int touchCallback(int eventType, const EmscriptenTouchEvent *ev, void *userData);

    void processEvents();

Q_SIGNALS:
    void getWindowAt(const QPoint &point, QWindow **window);
private:
    static Qt::Key translateEmscriptKey(const EmscriptenKeyboardEvent *emscriptKey, bool *outAlphanumretic);
    template <typename Event>
    static QFlags<Qt::KeyboardModifier> translatKeyModifier(const Event *event);
    static QFlags<Qt::KeyboardModifier> translateKeyboardEventModifier(const EmscriptenKeyboardEvent *keyEvent);
    static QFlags<Qt::KeyboardModifier> translateMouseEventModifier(const EmscriptenMouseEvent *mouseEvent);
    static Qt::MouseButton translateMouseButton(unsigned short button);

    void processMouse(int eventType, const EmscriptenMouseEvent *mouseEvent);

private:
    QWindow *draggedWindow;
    QWindow *pressedWindow;
    QWindow *lastWindow;
    Qt::MouseButtons pressedButtons;

    QWasmWindow::ResizeMode resizeMode;
    QPoint resizePoint;
    QRect resizeStartRect;
    QTouchDevice *touchDevice;
    quint64 getTimestamp();
};

QT_END_NAMESPACE
#endif // QWASMEVENTTRANSLATOR_H
