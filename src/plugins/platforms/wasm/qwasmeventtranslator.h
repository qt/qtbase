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
#include <QHash>

QT_BEGIN_NAMESPACE

class QWindow;

class QWasmEventTranslator : public QObject
{
    Q_OBJECT

public:

    explicit QWasmEventTranslator(QWasmScreen *screen);

    static int keyboard_cb(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData);
    static int mouse_cb(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData);
    static int focus_cb(int eventType, const EmscriptenFocusEvent *focusEvent, void *userData);
    static int wheel_cb(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData);

    static int touchCallback(int eventType, const EmscriptenTouchEvent *ev, void *userData);

    void processEvents();
    void initEventHandlers();
    int handleTouch(int eventType, const EmscriptenTouchEvent *touchEvent);

Q_SIGNALS:
    void getWindowAt(const QPoint &point, QWindow **window);
private:
    QWasmScreen *screen();
    Qt::Key translateEmscriptKey(const EmscriptenKeyboardEvent *emscriptKey);
    template <typename Event>
    QFlags<Qt::KeyboardModifier> translatKeyModifier(const Event *event);
    QFlags<Qt::KeyboardModifier> translateKeyboardEventModifier(const EmscriptenKeyboardEvent *keyEvent);
    QFlags<Qt::KeyboardModifier> translateMouseEventModifier(const EmscriptenMouseEvent *mouseEvent);
    Qt::MouseButton translateMouseButton(unsigned short button);

    void processMouse(int eventType, const EmscriptenMouseEvent *mouseEvent);
    bool processKeyboard(int eventType, const EmscriptenKeyboardEvent *keyEvent);

    Qt::Key translateDeadKey(Qt::Key deadKey, Qt::Key accentBaseKey);

    QHash<Qt::Key , Qt::Key> tildeKeyTable { // ~
        { Qt::Key_A, Qt::Key_Atilde},
        { Qt::Key_N, Qt::Key_Ntilde},
        { Qt::Key_O, Qt::Key_Otilde}
     };
    QHash<Qt::Key , Qt::Key> graveKeyTable { // `
        { Qt::Key_A, Qt::Key_Agrave},
        { Qt::Key_E, Qt::Key_Egrave},
        { Qt::Key_I, Qt::Key_Igrave},
        { Qt::Key_O, Qt::Key_Ograve},
        { Qt::Key_U, Qt::Key_Ugrave}
     };
    QHash<Qt::Key , Qt::Key> acuteKeyTable { // '
        { Qt::Key_A, Qt::Key_Aacute},
        { Qt::Key_E, Qt::Key_Eacute},
        { Qt::Key_I, Qt::Key_Iacute},
        { Qt::Key_O, Qt::Key_Oacute},
        { Qt::Key_U, Qt::Key_Uacute},
        { Qt::Key_Y, Qt::Key_Yacute}
     };
    QHash<Qt::Key , Qt::Key> diaeresisKeyTable { // umlaut ¨
        { Qt::Key_A, Qt::Key_Adiaeresis},
        { Qt::Key_E, Qt::Key_Ediaeresis},
        { Qt::Key_I, Qt::Key_Idiaeresis},
        { Qt::Key_O, Qt::Key_Odiaeresis},
        { Qt::Key_U, Qt::Key_Udiaeresis},
        { Qt::Key_Y, Qt::Key_ydiaeresis}
     };
    QHash<Qt::Key , Qt::Key> circumflexKeyTable { // ^
        { Qt::Key_A, Qt::Key_Acircumflex},
        { Qt::Key_E, Qt::Key_Ecircumflex},
        { Qt::Key_I, Qt::Key_Icircumflex},
        { Qt::Key_O, Qt::Key_Ocircumflex},
        { Qt::Key_U, Qt::Key_Ucircumflex}
     };

    QMap <int, QPointF> pressedTouchIds;

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

    Qt::Key m_emDeadKey = Qt::Key_unknown;
    bool m_emStickyDeadKey = false;
};

QT_END_NAMESPACE
#endif // QWASMEVENTTRANSLATOR_H
