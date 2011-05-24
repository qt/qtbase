/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#if !defined(QTESTKEYBOARD_H)
#define QTESTKEYBOARD_H

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

#include <QtTest/qtestassert.h>
#include <QtTest/qtest_global.h>
#include <QtTest/qtestsystem.h>
#include <QtTest/qtestspontaneevent.h>

#include <QtCore/qpointer.h>
#include <QtGui/qapplication.h>
#include <QtGui/qevent.h>
#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Test)

namespace QTest
{
    enum KeyAction { Press, Release, Click };

    static void simulateEvent(QWidget *widget, bool press, int code,
                              Qt::KeyboardModifiers modifier, QString text, bool repeat, int delay=-1)
    {
        QTEST_ASSERT(widget);
        extern int Q_TESTLIB_EXPORT defaultKeyDelay();

        if (delay == -1 || delay < defaultKeyDelay())
            delay = defaultKeyDelay();
        if(delay > 0)
            QTest::qWait(delay);

        QKeyEvent a(press ? QEvent::KeyPress : QEvent::KeyRelease, code, modifier, text, repeat);
        QSpontaneKeyEvent::setSpontaneous(&a);
        if (!qApp->notify(widget, &a))
            QTest::qWarn("Keyboard event not accepted by receiving widget");
    }

    static void sendKeyEvent(KeyAction action, QWidget *widget, Qt::Key code,
                             QString text, Qt::KeyboardModifiers modifier, int delay=-1)
    {
        QTEST_ASSERT(qApp);

        if (!widget)
            widget = QWidget::keyboardGrabber();
        if (!widget) {
            if (QWidget *apw = QApplication::activePopupWidget())
                widget = apw->focusWidget() ? apw->focusWidget() : apw;
            else
                widget = QApplication::focusWidget();
        }
        if (!widget)
            widget = QApplication::activeWindow();

        QTEST_ASSERT(widget);

        if (action == Click) {
            QPointer<QWidget> ptr(widget);
            sendKeyEvent(Press, widget, code, text, modifier, delay);
            if (!ptr) {
                // if we send key-events to embedded widgets, they might be destroyed
                // when the user presses Return
                return;
            }
            sendKeyEvent(Release, widget, code, text, modifier, delay);
            return;
        }

        bool repeat = false;

        if (action == Press) {
            if (modifier & Qt::ShiftModifier)
                simulateEvent(widget, true, Qt::Key_Shift, 0, QString(), false, delay);

            if (modifier & Qt::ControlModifier)
                simulateEvent(widget, true, Qt::Key_Control, modifier & Qt::ShiftModifier, QString(), false, delay);

            if (modifier & Qt::AltModifier)
                simulateEvent(widget, true, Qt::Key_Alt,
                              modifier & (Qt::ShiftModifier | Qt::ControlModifier), QString(), false, delay);
            if (modifier & Qt::MetaModifier)
                simulateEvent(widget, true, Qt::Key_Meta, modifier & (Qt::ShiftModifier
                                                                      | Qt::ControlModifier | Qt::AltModifier), QString(), false, delay);
            simulateEvent(widget, true, code, modifier, text, repeat, delay);
        } else if (action == Release) {
            simulateEvent(widget, false, code, modifier, text, repeat, delay);

            if (modifier & Qt::MetaModifier)
                simulateEvent(widget, false, Qt::Key_Meta, modifier, QString(), false, delay);
            if (modifier & Qt::AltModifier)
                simulateEvent(widget, false, Qt::Key_Alt, modifier &
                              (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier), QString(), false, delay);

            if (modifier & Qt::ControlModifier)
                simulateEvent(widget, false, Qt::Key_Control,
                              modifier & (Qt::ShiftModifier | Qt::ControlModifier), QString(), false, delay);

            if (modifier & Qt::ShiftModifier)
                simulateEvent(widget, false, Qt::Key_Shift, modifier & Qt::ShiftModifier, QString(), false, delay);
        }
    }

    // Convenience function
    static void sendKeyEvent(KeyAction action, QWidget *widget, Qt::Key code,
                             char ascii, Qt::KeyboardModifiers modifier, int delay=-1)
    {
        QString text;
        if (ascii)
            text = QString(QChar::fromLatin1(ascii));
        sendKeyEvent(action, widget, code, text, modifier, delay);
    }

    inline static void keyEvent(KeyAction action, QWidget *widget, char ascii,
                                Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { sendKeyEvent(action, widget, asciiToKey(ascii), ascii, modifier, delay); }
    inline static void keyEvent(KeyAction action, QWidget *widget, Qt::Key key,
                                Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { sendKeyEvent(action, widget, key, keyToAscii(key), modifier, delay); }

    inline static void keyClicks(QWidget *widget, const QString &sequence,
                                 Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    {
        for (int i=0; i < sequence.length(); i++)
            keyEvent(Click, widget, sequence.at(i).toLatin1(), modifier, delay);
    }

    inline static void keyPress(QWidget *widget, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Press, widget, key, modifier, delay); }
    inline static void keyRelease(QWidget *widget, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Release, widget, key, modifier, delay); }
    inline static void keyClick(QWidget *widget, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Click, widget, key, modifier, delay); }
    inline static void keyPress(QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Press, widget, key, modifier, delay); }
    inline static void keyRelease(QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Release, widget, key, modifier, delay); }
    inline static void keyClick(QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    { keyEvent(Click, widget, key, modifier, delay); }

}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTESTKEYBOARD_H
