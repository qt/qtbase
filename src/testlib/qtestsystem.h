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
    Q_DECL_UNUSED inline static void qWait(int ms)
    {
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
    inline static bool qWaitForWindowActive(QWindow *window, int timeout = 5000)
    {
        QDeadlineTimer timer(timeout, Qt::PreciseTimer);
        int remaining = timeout;
        while (!window->isActive() && remaining > 0) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
            QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
            QTest::qSleep(10);
            remaining = timer.remainingTime();
        }
        // Try ensuring the platform window receives the real position.
        // (i.e. that window->pos() reflects reality)
        // isActive() ( == FocusIn in case of X) does not guarantee this. It seems some WMs randomly
        // send the final ConfigureNotify (the one with the non-bogus 0,0 position) after the FocusIn.
        // If we just let things go, every mapTo/FromGlobal call the tests perform directly after
        // qWaitForWindowShown() will generate bogus results.
        if (window->isActive()) {
            int waitNo = 0; // 0, 0 might be a valid position after all, so do not wait for ever
            while (window->position().isNull()) {
                if (waitNo++ > timeout / 10)
                    break;
                qWait(10);
            }
        }
        return window->isActive();
    }

    inline static bool qWaitForWindowExposed(QWindow *window, int timeout = 5000)
    {
        QDeadlineTimer timer(timeout, Qt::PreciseTimer);
        int remaining = timeout;
        while (!window->isExposed() && remaining > 0) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
            QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
            QTest::qSleep(10);
            remaining = timer.remainingTime();
        }
        return window->isExposed();
    }
#endif

#ifdef QT_WIDGETS_LIB
    inline static bool qWaitForWindowActive(QWidget *widget, int timeout = 5000)
    {
        if (QWindow *window = widget->window()->windowHandle())
            return qWaitForWindowActive(window, timeout);
        return false;
    }

    inline static bool qWaitForWindowExposed(QWidget *widget, int timeout = 5000)
    {
        if (QWindow *window = widget->window()->windowHandle())
            return qWaitForWindowExposed(window, timeout);
        return false;
    }
#endif

#if QT_DEPRECATED_SINCE(5, 0)
#  ifdef QT_WIDGETS_LIB
    QT_DEPRECATED inline static bool qWaitForWindowShown(QWidget *widget, int timeout = 5000)
    {
        return qWaitForWindowExposed(widget, timeout);
    }
#  endif // QT_WIDGETS_LIB
#endif // QT_DEPRECATED_SINCE(5, 0)
}

QT_END_NAMESPACE

#endif

