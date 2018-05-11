/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qhtml5eventdispatcher.h"

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

#include <emscripten.h>

QHtml5EventDispatcher::QHtml5EventDispatcher(QObject *parent)
    : QUnixEventDispatcherQPA(parent)
{
}

QHtml5EventDispatcher::~QHtml5EventDispatcher() {}

bool QHtml5EventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    // WaitForMoreEvents is not supported (except for in combination with EventLoopExec below),
    // and we don't want the unix event dispatcher base class to attempt to wait either.
    flags &= ~QEventLoop::WaitForMoreEvents;

    // Handle normal processEvents.
    if (!(flags & QEventLoop::EventLoopExec)) {
        bool processed = false;

        // We need to give the control back to the browser due to lack of PTHREADS
        // Limit the number of events that may be processed at the time
        int maxProcessedEvents = 10;
        int processedCount = 0;
        do {
            processed = QUnixEventDispatcherQPA::processEvents(flags);
            processedCount += 1;
        } while (processed && hasPendingEvents() && processedCount < maxProcessedEvents);
        return true;
    }

    // Handle processEvents from QEventLoop::exec():
    //
    // At this point the application has created its root objects on
    // the stack and has called app.exec() which has called into this
    // function via QEventLoop.
    //
    // The application now expects that exec() will not return until
    // app exit time. However, the browser expects that we return
    // control to it periodically, also after initial setup in main().

    // EventLoopExec for nested event loops is not supported.
    Q_ASSERT(!m_hasMainLoop);
    m_hasMainLoop = true;

    // Call emscripten_set_main_loop_arg() with a callback which processes
    // events. Also set simulateInfiniteLoop to true which makes emscripten
    // return control to the browser without unwinding the C++ stack.
    auto callback = [](void *eventDispatcher) {
        static_cast<QHtml5EventDispatcher *>(eventDispatcher)->processEvents(QEventLoop::AllEvents);
    };
    int fps = 0; // update using requestAnimationFrame
    int simulateInfiniteLoop = 1;
    emscripten_set_main_loop_arg(callback, this, fps, simulateInfiniteLoop);

    // Note: the above call never returns, not even at app exit
    return false;
}
