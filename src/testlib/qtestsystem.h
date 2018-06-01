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

#ifndef QTESTSYSTEM_H
#define QTESTSYSTEM_H

#include <QtTest/qtestcase.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdeadlinetimer.h>
#ifdef QT_GUI_LIB
#  include <QtGui/QWindow>
#endif
#ifdef QT_WIDGETS_LIB
#  include <QtWidgets/QWidget>
#endif

QT_BEGIN_NAMESPACE

namespace QTest
{
    template <typename Functor>
    Q_REQUIRED_RESULT static bool qWaitFor(Functor predicate, int timeout = 5000)
    {
        // We should not spin the event loop in case the predicate is already true,
        // otherwise we might send new events that invalidate the predicate.
        if (predicate())
            return true;

        // qWait() is expected to spin the event loop, even when called with a small
        // timeout like 1ms, so we we can't use a simple while-loop here based on
        // the deadline timer not having timed out. Use do-while instead.

        int remaining = timeout;
        QDeadlineTimer deadline(remaining, Qt::PreciseTimer);

        do {
            QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

            remaining = deadline.remainingTime();
            if (remaining > 0) {
                QTest::qSleep(qMin(10, remaining));
                remaining = deadline.remainingTime();
            }

            if (predicate())
                return true;

            remaining = deadline.remainingTime();
        } while (remaining > 0);

        return predicate(); // Last chance
    }

    Q_DECL_UNUSED inline static void qWait(int ms)
    {
        // Ideally this method would be implemented in terms of qWaitFor, with
        // a predicate that always returns false, but due to a compiler bug in
        // GCC 6 we can't do that.

        Q_ASSERT(QCoreApplication::instance());

        QDeadlineTimer timer(ms, Qt::PreciseTimer);
        int remaining = ms;
        do {
            QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
            QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
            remaining = timer.remainingTime();
            if (remaining <= 0)
                break;
            QTest::qSleep(qMin(10, remaining));
            remaining = timer.remainingTime();
        } while (remaining > 0);
    }

#ifdef QT_GUI_LIB
    Q_REQUIRED_RESULT inline static bool qWaitForWindowActive(QWindow *window, int timeout = 5000)
    {
        return qWaitFor([&]() { return window->isActive(); }, timeout);
    }

    Q_REQUIRED_RESULT inline static bool qWaitForWindowExposed(QWindow *window, int timeout = 5000)
    {
        return qWaitFor([&]() { return window->isExposed(); }, timeout);
    }
#endif

#ifdef QT_WIDGETS_LIB
    Q_REQUIRED_RESULT inline static bool qWaitForWindowActive(QWidget *widget, int timeout = 5000)
    {
        if (QWindow *window = widget->window()->windowHandle())
            return qWaitForWindowActive(window, timeout);
        return false;
    }

    Q_REQUIRED_RESULT inline static bool qWaitForWindowExposed(QWidget *widget, int timeout = 5000)
    {
        if (QWindow *window = widget->window()->windowHandle())
            return qWaitForWindowExposed(window, timeout);
        return false;
    }
#endif

#if QT_DEPRECATED_SINCE(5, 0)
#  ifdef QT_WIDGETS_LIB

    QT_DEPRECATED Q_REQUIRED_RESULT inline static bool qWaitForWindowShown(QWidget *widget, int timeout = 5000)
    {
        return qWaitForWindowExposed(widget, timeout);
    }
#  endif // QT_WIDGETS_LIB
#endif // QT_DEPRECATED_SINCE(5, 0)
}

QT_END_NAMESPACE

#endif

