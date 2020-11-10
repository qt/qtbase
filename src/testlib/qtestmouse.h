/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#ifndef QTESTMOUSE_H
#define QTESTMOUSE_H

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

#include <QtTest/qttestglobal.h>
#include <QtTest/qtestassert.h>
#include <QtTest/qtestsystem.h>
#include <QtTest/qtestspontaneevent.h>
#include <QtCore/qpoint.h>
#include <QtCore/qstring.h>
#include <QtCore/qpointer.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>

#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qwidget.h>
#endif

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT void qt_handleMouseEvent(QWindow *window, const QPointF &local, const QPointF &global,
                                      Qt::MouseButtons state, Qt::MouseButton button,
                                      QEvent::Type type, Qt::KeyboardModifiers mods, int timestamp);

namespace QTestPrivate
{
    extern Q_TESTLIB_EXPORT Qt::MouseButtons qtestMouseButtons;
}

namespace QTest
{
    enum MouseAction { MousePress, MouseRelease, MouseClick, MouseDClick, MouseMove };

    extern Q_TESTLIB_EXPORT int lastMouseTimestamp;

    // This value is used to emulate timestamps to avoid creating double clicks by mistake.
    // Use this constant instead of QStyleHints::mouseDoubleClickInterval property to avoid tests
    // to depend on platform themes.
    static const int mouseDoubleClickInterval = 500;

    /*! \internal
        This function creates a QPA mouse event of type specified by \a action
        and calls QWindowSystemInterface::handleMouseEvent(), simulating the
        windowing system and bypassing the platform plugin. \a delay is the
        amount of time to be added to the simulated clock so that
        QInputEvent::timestamp() will be greater than that of the previous
        event. We expect all event-handling code to rely on the event
        timestamps, not the system clock; therefore tests can be run faster
        than real-time.
    */
    static void mouseEvent(MouseAction action, QWindow *window, Qt::MouseButton button,
                           Qt::KeyboardModifiers stateKey, QPoint pos, int delay=-1)
    {
        QTEST_ASSERT(window);
        extern int Q_TESTLIB_EXPORT defaultMouseDelay();

        // pos is in window local coordinates
        const QSize windowSize = window->geometry().size();
        if (windowSize.width() <= pos.x() || windowSize.height() <= pos.y()) {
            QTest::qWarn(qPrintable(QString::fromLatin1("Mouse event at %1, %2 occurs outside of target window (%3x%4).")
                .arg(pos.x()).arg(pos.y()).arg(windowSize.width()).arg(windowSize.height())));
        }

        if (delay == -1 || delay < defaultMouseDelay())
            delay = defaultMouseDelay();
        lastMouseTimestamp += qMax(1, delay);

        if (pos.isNull())
            pos = QPoint(window->width() / 2, window->height() / 2);

        QTEST_ASSERT(uint(stateKey) == 0 || stateKey & Qt::KeyboardModifierMask);

        stateKey &= static_cast<unsigned int>(Qt::KeyboardModifierMask);

        QPointF global = window->mapToGlobal(pos);
        QPointer<QWindow> w(window);

        using namespace QTestPrivate;
        switch (action)
        {
        case MouseDClick:
            qtestMouseButtons.setFlag(button, true);
            qt_handleMouseEvent(w, pos, global, qtestMouseButtons, button, QEvent::MouseButtonPress,
                                stateKey, lastMouseTimestamp);
            qtestMouseButtons.setFlag(button, false);
            qt_handleMouseEvent(w, pos, global, qtestMouseButtons, button, QEvent::MouseButtonRelease,
                                stateKey, lastMouseTimestamp);
            Q_FALLTHROUGH();
        case MousePress:
        case MouseClick:
            qtestMouseButtons.setFlag(button, true);
            qt_handleMouseEvent(w, pos, global, qtestMouseButtons, button, QEvent::MouseButtonPress,
                                stateKey, lastMouseTimestamp);
            if (action == MousePress)
                break;
            Q_FALLTHROUGH();
        case MouseRelease:
            qtestMouseButtons.setFlag(button, false);
            qt_handleMouseEvent(w, pos, global, qtestMouseButtons, button, QEvent::MouseButtonRelease,
                                stateKey, lastMouseTimestamp);
            lastMouseTimestamp += mouseDoubleClickInterval; // avoid double clicks being generated
            break;
        case MouseMove:
            qt_handleMouseEvent(w, pos, global, qtestMouseButtons, Qt::NoButton, QEvent::MouseMove,
                                stateKey, lastMouseTimestamp);
            break;
        default:
            QTEST_ASSERT(false);
        }
        qApp->processEvents();
    }

    inline void mousePress(QWindow *window, Qt::MouseButton button,
                           Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers(),
                           QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MousePress, window, button, stateKey, pos, delay); }
    inline void mouseRelease(QWindow *window, Qt::MouseButton button,
                             Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers(),
                             QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseRelease, window, button, stateKey, pos, delay); }
    inline void mouseClick(QWindow *window, Qt::MouseButton button,
                           Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers(),
                           QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseClick, window, button, stateKey, pos, delay); }
    inline void mouseDClick(QWindow *window, Qt::MouseButton button,
                            Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers(),
                            QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseDClick, window, button, stateKey, pos, delay); }
    inline void mouseMove(QWindow *window, QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseMove, window, Qt::NoButton, Qt::KeyboardModifiers(), pos, delay); }

#ifdef QT_WIDGETS_LIB
    static void mouseEvent(MouseAction action, QWidget *widget, Qt::MouseButton button,
                           Qt::KeyboardModifiers stateKey, QPoint pos, int delay=-1)
    {
        QTEST_ASSERT(widget);

        if (pos.isNull())
            pos = widget->rect().center();

#ifdef QTEST_QPA_MOUSE_HANDLING
        QWindow *w = widget->window()->windowHandle();
        QTEST_ASSERT(w);
        mouseEvent(action, w, button, stateKey, w->mapFromGlobal(widget->mapToGlobal(pos)), delay);
#else
        extern int Q_TESTLIB_EXPORT defaultMouseDelay();

        if (delay == -1 || delay < defaultMouseDelay())
            delay = defaultMouseDelay();
        lastMouseTimestamp += qMax(1, delay);

        if (action == MouseClick) {
            mouseEvent(MousePress, widget, button, stateKey, pos);
            mouseEvent(MouseRelease, widget, button, stateKey, pos);
            return;
        }

        QTEST_ASSERT(stateKey == 0 || stateKey & Qt::KeyboardModifierMask);

        stateKey &= static_cast<unsigned int>(Qt::KeyboardModifierMask);

        QEvent::Type meType;
        Qt::MouseButton meButton;
        switch (action)
        {
            case MousePress:
                meType = QEvent::MouseButtonPress;
                meButton = button;
                break;
            case MouseRelease:
                meType = QEvent::MouseButtonRelease;
                meButton = Qt::MouseButton();
                break;
            case MouseDClick:
                meType = QEvent::MouseButtonDblClick;
                meButton = button;
                break;
            case MouseMove:
                QCursor::setPos(widget->mapToGlobal(pos));
                qApp->processEvents();
                return;
            default:
                QTEST_ASSERT(false);
        }
        QMouseEvent me(meType, pos, widget->mapToGlobal(pos), button, meButton, stateKey, QPointingDevice::primaryPointingDevice());
        me.setTimestamp(lastMouseTimestamp);
        if (action == MouseRelease) // avoid double clicks being generated
            lastMouseTimestamp += mouseDoubleClickInterval;

        QSpontaneKeyEvent::setSpontaneous(&me);
        if (!qApp->notify(widget, &me)) {
            static const char *const mouseActionNames[] =
                { "MousePress", "MouseRelease", "MouseClick", "MouseDClick", "MouseMove" };
            QString warning = QString::fromLatin1("Mouse event \"%1\" not accepted by receiving widget");
            QTest::qWarn(warning.arg(QString::fromLatin1(mouseActionNames[static_cast<int>(action)])).toLatin1().data());
        }
#endif
    }

    inline void mousePress(QWidget *widget, Qt::MouseButton button,
                           Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers(),
                           QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MousePress, widget, button, stateKey, pos, delay); }
    inline void mouseRelease(QWidget *widget, Qt::MouseButton button,
                             Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers(),
                             QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseRelease, widget, button, stateKey, pos, delay); }
    inline void mouseClick(QWidget *widget, Qt::MouseButton button,
                           Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers(),
                           QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseClick, widget, button, stateKey, pos, delay); }
    inline void mouseDClick(QWidget *widget, Qt::MouseButton button,
                            Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers(),
                            QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseDClick, widget, button, stateKey, pos, delay); }
    inline void mouseMove(QWidget *widget, QPoint pos = QPoint(), int delay=-1)
    { mouseEvent(MouseMove, widget, Qt::NoButton, Qt::KeyboardModifiers(), pos, delay); }
#endif // QT_WIDGETS_LIB
}

QT_END_NAMESPACE

#endif // QTESTMOUSE_H
