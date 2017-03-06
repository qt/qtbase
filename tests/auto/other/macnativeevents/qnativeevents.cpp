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

#include "qnativeevents.h"

QNativeInput::QNativeInput(bool subscribe)
{
    if (subscribe)
        subscribeForNativeEvents();
}

QNativeInput::~QNativeInput()
{
    unsubscribeForNativeEvents();
}

void QNativeInput::notify(QNativeEvent *event)
{
    nativeEvent(event);
}

void QNativeInput::nativeEvent(QNativeEvent *event)
{
    switch (event->id()){
        case QNativeMouseButtonEvent::eventId:{
            QNativeMouseButtonEvent *e = static_cast<QNativeMouseButtonEvent *>(event);
            (e->clickCount > 0) ? nativeMousePressEvent(e) : nativeMouseReleaseEvent(e);
            break; }
        case QNativeMouseMoveEvent::eventId:
            nativeMouseMoveEvent(static_cast<QNativeMouseMoveEvent *>(event));
            break;
        case QNativeMouseDragEvent::eventId:
            nativeMouseDragEvent(static_cast<QNativeMouseDragEvent *>(event));
            break;
        case QNativeMouseWheelEvent::eventId:
            nativeMouseWheelEvent(static_cast<QNativeMouseWheelEvent *>(event));
            break;
        case QNativeKeyEvent::eventId:{
            QNativeKeyEvent *e = static_cast<QNativeKeyEvent *>(event);
            e->press ? nativeKeyPressEvent(e) : nativeKeyReleaseEvent(e);
            break; }
        case QNativeModifierEvent::eventId:
            nativeModifierEvent(static_cast<QNativeModifierEvent *>(event));
            break;
        default:
            break;
    }
}

Qt::Native::Status QNativeInput::sendNativeEvent(const QNativeEvent &event)
{
    switch (event.id()){
        case QNativeMouseMoveEvent::eventId:
            return sendNativeMouseMoveEvent(static_cast<const QNativeMouseMoveEvent &>(event));
        case QNativeMouseButtonEvent::eventId:
            return sendNativeMouseButtonEvent(static_cast<const QNativeMouseButtonEvent &>(event));
        case QNativeMouseDragEvent::eventId:
            return sendNativeMouseDragEvent(static_cast<const QNativeMouseDragEvent &>(event));
        case QNativeMouseWheelEvent::eventId:
            return sendNativeMouseWheelEvent(static_cast<const QNativeMouseWheelEvent &>(event));
        case QNativeKeyEvent::eventId:
            return sendNativeKeyEvent(static_cast<const QNativeKeyEvent &>(event));
        case QNativeModifierEvent::eventId:
            return sendNativeModifierEvent(static_cast<const QNativeModifierEvent &>(event));
        case QNativeEvent::eventId:
            qWarning() << "Warning: Cannot send a pure native event. Use a sub class.";
        default:
            return Qt::Native::Failure;
    }
}

QNativeEvent::QNativeEvent(Qt::KeyboardModifiers modifiers)
    : modifiers(modifiers){}

QNativeMouseEvent::QNativeMouseEvent(QPoint pos, Qt::KeyboardModifiers modifiers)
    : QNativeEvent(modifiers), globalPos(pos){}

QNativeMouseMoveEvent::QNativeMouseMoveEvent(QPoint pos, Qt::KeyboardModifiers modifiers)
    : QNativeMouseEvent(pos, modifiers){}

QNativeMouseButtonEvent::QNativeMouseButtonEvent(QPoint globalPos, Qt::MouseButton button, int clickCount, Qt::KeyboardModifiers modifiers)
    : QNativeMouseEvent(globalPos, modifiers), button(button), clickCount(clickCount){}

QNativeMouseDragEvent::QNativeMouseDragEvent(QPoint globalPos, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
    : QNativeMouseButtonEvent(globalPos, button, true, modifiers){}

QNativeMouseWheelEvent::QNativeMouseWheelEvent(QPoint globalPos, int delta, Qt::KeyboardModifiers modifiers)
    : QNativeMouseEvent(globalPos, modifiers), delta(delta){}

QNativeKeyEvent::QNativeKeyEvent(int nativeKeyCode, bool press, Qt::KeyboardModifiers modifiers)
    : QNativeEvent(modifiers), nativeKeyCode(nativeKeyCode), press(press), character(QChar()){}

QNativeModifierEvent::QNativeModifierEvent(Qt::KeyboardModifiers modifiers, int nativeKeyCode)
    : QNativeEvent(modifiers), nativeKeyCode(nativeKeyCode){}

QNativeKeyEvent::QNativeKeyEvent(int nativeKeyCode, bool press, QChar character, Qt::KeyboardModifiers modifiers)
    : QNativeEvent(modifiers), nativeKeyCode(nativeKeyCode), press(press), character(character){}

static QString getButtonAsString(const QNativeMouseButtonEvent *e)
{
    switch (e->button){
        case Qt::LeftButton:
            return "button = LeftButton";
            break;
        case Qt::RightButton:
            return "button = RightButton";
            break;
        case Qt::MidButton:
            return "button = MidButton";
            break;
        default:
            return "button = Other";
            break;
    }
}

static QString getModifiersAsString(const QNativeEvent *e)
{
    if (e->modifiers == 0)
        return "modifiers = none";

    QString tmp = "modifiers = ";
    if (e->modifiers.testFlag(Qt::ShiftModifier))
        tmp += "Shift";
    if (e->modifiers.testFlag(Qt::ControlModifier))
        tmp += "Control";
    if (e->modifiers.testFlag(Qt::AltModifier))
        tmp += "Alt";
    if (e->modifiers.testFlag(Qt::MetaModifier))
        tmp += "Meta";
    return tmp;
}

static QString getPosAsString(QPoint pos)
{
    return QString("QPoint(%1, %2)").arg(pos.x()).arg(pos.y());
}

static QString getBoolAsString(bool b)
{
    return b ? QString("true") : QString("false");
}

QString QNativeMouseMoveEvent::toString() const
{
    return QString("QNativeMouseMoveEvent(globalPos = %1 %2)").arg(getPosAsString(globalPos))
        .arg(getModifiersAsString(this));
}

QString QNativeMouseButtonEvent::toString() const
{
    return QString("QNativeMouseButtonEvent(globalPos = %1, %2, clickCount = %3, %4)").arg(getPosAsString(globalPos))
        .arg(getButtonAsString(this)).arg(clickCount).arg(getModifiersAsString(this));
}

QString QNativeMouseDragEvent::toString() const
{
    return QString("QNativeMouseDragEvent(globalPos = %1, %2, clickCount = %3, %4)").arg(getPosAsString(globalPos))
    .arg(getButtonAsString(this)).arg(clickCount).arg(getModifiersAsString(this));
}

QString QNativeMouseWheelEvent::toString() const
{
    return QString("QNativeMouseWheelEvent(globalPos = %1, delta = %2, %3)").arg(getPosAsString(globalPos))
        .arg(delta).arg(getModifiersAsString(this));
}

QString QNativeKeyEvent::toString() const
{
    return QString("QNativeKeyEvent(press = %1, native key code = %2, character = %3, %4)").arg(getBoolAsString(press))
        .arg(nativeKeyCode).arg(character.isPrint() ? character : QString("<no char>"))
        .arg(getModifiersAsString(this));
}

QString QNativeModifierEvent::toString() const
{
    return QString("QNativeModifierEvent(%1, native key code = %2)").arg(getModifiersAsString(this))
        .arg(nativeKeyCode);
}

QDebug operator<<(QDebug d, QNativeEvent *e)
{
    Q_UNUSED(e);
    return d << e->toString();
}

QDebug operator<<(QDebug d, const QNativeEvent &e)
{
    Q_UNUSED(e);
    return d << e.toString();
}

QTextStream &operator<<(QTextStream &s, QNativeEvent *e)
{
    return s << e->eventId << ' ' << e->modifiers << " QNativeEvent";
}

QTextStream &operator<<(QTextStream &s, QNativeMouseEvent *e)
{
    return s << e->eventId << ' ' << e->globalPos.x() << ' ' << e->globalPos.y() << ' ' << e->modifiers << ' ' << e->toString();
}

QTextStream &operator<<(QTextStream &s, QNativeMouseMoveEvent *e)
{
    return s << e->eventId << ' ' << e->globalPos.x() << ' ' << e->globalPos.y() << ' ' << e->modifiers << ' ' << e->toString();
}

QTextStream &operator<<(QTextStream &s, QNativeMouseButtonEvent *e)
{
    return s << e->eventId << ' ' << e->globalPos.x() << ' ' << e->globalPos.y() << ' ' << e->button
        << ' ' << e->clickCount << ' ' << e->modifiers << ' ' << e->toString();
}

QTextStream &operator<<(QTextStream &s, QNativeMouseDragEvent *e)
{
    return s << e->eventId << ' ' << e->globalPos.x() << ' ' << e->globalPos.y() << ' ' << e->button << ' ' << e->clickCount
        << ' ' << e->modifiers << ' ' << e->toString();
}

QTextStream &operator<<(QTextStream &s, QNativeMouseWheelEvent *e)
{
    return s << e->eventId << ' ' << e->globalPos.x() << ' ' << e->globalPos.y() << ' ' << e->delta
        << ' ' << e->modifiers << ' ' << e->toString();
}

QTextStream &operator<<(QTextStream &s, QNativeKeyEvent *e)
{
    return s << e->eventId << ' ' << e->press << ' ' << e->nativeKeyCode << ' ' << e->character
        << ' ' << e->modifiers << ' ' << e->toString();
}

QTextStream &operator<<(QTextStream &s, QNativeModifierEvent *e)
{
    return s << e->eventId << ' ' << e->modifiers << ' ' << e->nativeKeyCode << ' ' << e->toString();
}




QTextStream &operator>>(QTextStream &s, QNativeMouseMoveEvent *e)
{
    // Skip reading eventId.
    QString humanReadable;
    int x, y, modifiers;
    s >> x >> y >> modifiers >> humanReadable;
    e->globalPos.setX(x);
    e->globalPos.setY(y);
    e->modifiers = Qt::KeyboardModifiers(modifiers);
    return s;
}

QTextStream &operator>>(QTextStream &s, QNativeMouseButtonEvent *e)
{
    // Skip reading eventId.
    QString humanReadable;
    int x, y, button, clickCount, modifiers;
    s >> x >> y >> button >> clickCount >> modifiers >> humanReadable;
    e->globalPos.setX(x);
    e->globalPos.setY(y);
    e->clickCount = clickCount;
    e->modifiers = Qt::KeyboardModifiers(modifiers);
    switch (button){
        case 1:
            e->button = Qt::LeftButton;
            break;
        case 2:
            e->button = Qt::RightButton;
            break;
        case 3:
            e->button = Qt::MidButton;
            break;
        default:
            e->button = Qt::NoButton;
            break;
    }
    return s;
}

QTextStream &operator>>(QTextStream &s, QNativeMouseDragEvent *e)
{
    // Skip reading eventId.
    QString humanReadable;
    int x, y, button, clickCount, modifiers;
    s >> x >> y >> button >> clickCount >> modifiers >> humanReadable;
    e->globalPos.setX(x);
    e->globalPos.setY(y);
    e->clickCount = clickCount;
    e->modifiers = Qt::KeyboardModifiers(modifiers);
    switch (button){
        case 1:
            e->button = Qt::LeftButton;
            break;
        case 2:
            e->button = Qt::RightButton;
            break;
        case 3:
            e->button = Qt::MidButton;
            break;
        default:
            e->button = Qt::NoButton;
            break;
    }
    return s;
}

QTextStream &operator>>(QTextStream &s, QNativeMouseWheelEvent *e)
{
    // Skip reading eventId.
    QString humanReadable;
    int x, y, modifiers;
    s >> x >> y >> e->delta >> modifiers >> humanReadable;
    e->globalPos.setX(x);
    e->globalPos.setY(y);
    e->modifiers = Qt::KeyboardModifiers(modifiers);
    return s;
}

QTextStream &operator>>(QTextStream &s, QNativeKeyEvent *e)
{
    // Skip reading eventId.
    QString humanReadable;
    int press, modifiers;
    QString character;
    s >> press >> e->nativeKeyCode >> character >> modifiers >> humanReadable;
    e->press = bool(press);
    e->character = character[0];
    e->modifiers = Qt::KeyboardModifiers(modifiers);
    return s;
}

QTextStream &operator>>(QTextStream &s, QNativeModifierEvent *e)
{
    // Skip reading eventId.
    QString humanReadable;
    int modifiers;
    s >> modifiers >> e->nativeKeyCode >> humanReadable;
    e->modifiers = Qt::KeyboardModifiers(modifiers);
    return s;
}

