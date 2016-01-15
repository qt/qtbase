/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef Q_NATIVE_INPUT
#define Q_NATIVE_INPUT

#include <QtCore>

QT_BEGIN_NAMESPACE
namespace Qt {
namespace Native {
    enum Status {Success, Failure};
}}
QT_END_NAMESPACE

// ----------------------------------------------------------------------------
// Declare a set of native events that can be used to communicate with
// client applications in an platform independent way
// ----------------------------------------------------------------------------

class QNativeEvent
{
public:
    static const int eventId = 1;

    QNativeEvent(Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    virtual ~QNativeEvent(){};
    virtual int id() const { return eventId; };
    virtual QString toString() const = 0;
    Qt::KeyboardModifiers modifiers; // Yields for mouse events too.
};

class QNativeMouseEvent : public QNativeEvent {
public:
    static const int eventId = 2;

    QNativeMouseEvent(){};
    QNativeMouseEvent(QPoint globalPos, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    virtual ~QNativeMouseEvent(){};
    virtual int id() const { return eventId; };

    QPoint globalPos;
};

class QNativeMouseMoveEvent : public QNativeMouseEvent {
public:
    static const int eventId = 4;

    QNativeMouseMoveEvent(){};
    QNativeMouseMoveEvent(QPoint globalPos, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    virtual ~QNativeMouseMoveEvent(){};
    virtual int id() const { return eventId; };
    virtual QString toString() const;
};

class QNativeMouseButtonEvent : public QNativeMouseEvent {
public:
    static const int eventId = 8;

    QNativeMouseButtonEvent(){};
    QNativeMouseButtonEvent(QPoint globalPos, Qt::MouseButton button, int clickCount, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    virtual ~QNativeMouseButtonEvent(){};
    virtual int id() const { return eventId; };
    virtual QString toString() const;

    Qt::MouseButton button;
    int clickCount;
};

class QNativeMouseDragEvent : public QNativeMouseButtonEvent {
public:
    static const int eventId = 16;

    QNativeMouseDragEvent(){};
    QNativeMouseDragEvent(QPoint globalPos, Qt::MouseButton button, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    virtual ~QNativeMouseDragEvent(){};
    virtual int id() const { return eventId; };
    virtual QString toString() const;
};

class QNativeMouseWheelEvent : public QNativeMouseEvent {
public:
    static const int eventId = 32;

    QNativeMouseWheelEvent(){};
    QNativeMouseWheelEvent(QPoint globalPos, int delta, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    virtual ~QNativeMouseWheelEvent(){};
    virtual int id() const { return eventId; };
    virtual QString toString() const;

    int delta;
};

class QNativeKeyEvent : public QNativeEvent {
    public:
    static const int eventId = 64;

    QNativeKeyEvent(){};
    QNativeKeyEvent(int nativeKeyCode, bool press, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    QNativeKeyEvent(int nativeKeyCode, bool press, QChar character, Qt::KeyboardModifiers modifiers);
    virtual ~QNativeKeyEvent(){};
    virtual int id() const { return eventId; };
    virtual QString toString() const;

    int nativeKeyCode;
    bool press;
    QChar character;

    // Some Qt to Native mappings:
    static int Key_A;
    static int Key_B;
    static int Key_C;
    static int Key_1;
    static int Key_Backspace;
    static int Key_Enter;
    static int Key_Del;
};

class QNativeModifierEvent : public QNativeEvent {
public:
    static const int eventId = 128;

    QNativeModifierEvent(Qt::KeyboardModifiers modifiers = Qt::NoModifier, int nativeKeyCode = 0);
    virtual ~QNativeModifierEvent(){};
    virtual int id() const { return eventId; };
    virtual QString toString() const;

    int nativeKeyCode;
};

// ----------------------------------------------------------------------------
// Declare a set of related output / input functions for convenience:
// ----------------------------------------------------------------------------

QDebug operator<<(QDebug d, QNativeEvent *e);
QDebug operator<<(QDebug d, const QNativeEvent &e);

QTextStream &operator<<(QTextStream &s, QNativeEvent *e);
QTextStream &operator<<(QTextStream &s, QNativeMouseEvent *e);
QTextStream &operator<<(QTextStream &s, QNativeMouseMoveEvent *e);
QTextStream &operator<<(QTextStream &s, QNativeMouseButtonEvent *e);
QTextStream &operator<<(QTextStream &s, QNativeMouseDragEvent *e);
QTextStream &operator<<(QTextStream &s, QNativeMouseWheelEvent *e);
QTextStream &operator<<(QTextStream &s, QNativeKeyEvent *e);
QTextStream &operator<<(QTextStream &s, QNativeModifierEvent *e);

QTextStream &operator>>(QTextStream &s, QNativeMouseMoveEvent *e);
QTextStream &operator>>(QTextStream &s, QNativeMouseButtonEvent *e);
QTextStream &operator>>(QTextStream &s, QNativeMouseDragEvent *e);
QTextStream &operator>>(QTextStream &s, QNativeMouseWheelEvent *e);
QTextStream &operator>>(QTextStream &s, QNativeKeyEvent *e);
QTextStream &operator>>(QTextStream &s, QNativeModifierEvent *e);

// ----------------------------------------------------------------------------
// Declare the main class that is supposed to be sub-classed by components
// that are to receive native events
// ----------------------------------------------------------------------------

class QNativeInput
{
    public:
    QNativeInput(bool subscribe = true);
    virtual ~QNativeInput();

    // Callback methods. Should be implemented by interested sub-classes:
    void notify(QNativeEvent *event);
    virtual void nativeEvent(QNativeEvent *event);
    virtual void nativeMousePressEvent(QNativeMouseButtonEvent *){};
    virtual void nativeMouseReleaseEvent(QNativeMouseButtonEvent *){};
    virtual void nativeMouseMoveEvent(QNativeMouseMoveEvent *){};
    virtual void nativeMouseDragEvent(QNativeMouseDragEvent *){};
    virtual void nativeMouseWheelEvent(QNativeMouseWheelEvent *){};
    virtual void nativeKeyPressEvent(QNativeKeyEvent *){};
    virtual void nativeKeyReleaseEvent(QNativeKeyEvent *){};
    virtual void nativeModifierEvent(QNativeModifierEvent *){};

    // The following methods will differ in implementation from OS to OS:
    static Qt::Native::Status sendNativeMouseButtonEvent(const QNativeMouseButtonEvent &event);
    static Qt::Native::Status sendNativeMouseMoveEvent(const QNativeMouseMoveEvent &event);
    static Qt::Native::Status sendNativeMouseDragEvent(const QNativeMouseDragEvent &event);
    static Qt::Native::Status sendNativeMouseWheelEvent(const QNativeMouseWheelEvent &event);
    static Qt::Native::Status sendNativeKeyEvent(const QNativeKeyEvent &event, int pid = 0);
    static Qt::Native::Status sendNativeModifierEvent(const QNativeModifierEvent &event);
    // sendNativeEvent will NOT differ from OS to OS.
    static Qt::Native::Status sendNativeEvent(const QNativeEvent &event, int pid = 0);

    // The following methods will differ in implementation from OS to OS:
    Qt::Native::Status subscribeForNativeEvents();
    Qt::Native::Status unsubscribeForNativeEvents();
};

#endif // Q_NATIVE_INPUT
