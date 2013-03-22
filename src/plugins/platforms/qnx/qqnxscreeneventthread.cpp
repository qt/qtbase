/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqnxscreeneventthread.h"
#include "qqnxscreeneventhandler.h"

#include <QtCore/QDebug>

#include <errno.h>
#include <unistd.h>

#include <cctype>

#if defined(QQNXSCREENEVENTTHREAD_DEBUG)
#define qScreenEventThreadDebug qDebug
#else
#define qScreenEventThreadDebug QT_NO_QDEBUG_MACRO
#endif

QQnxScreenEventThread::QQnxScreenEventThread(screen_context_t context, QQnxScreenEventHandler *screenEventHandler)
    : QThread(),
      m_screenContext(context),
      m_screenEventHandler(screenEventHandler),
      m_quit(false)
{
}

QQnxScreenEventThread::~QQnxScreenEventThread()
{
    // block until thread terminates
    shutdown();
}

void QQnxScreenEventThread::injectKeyboardEvent(int flags, int sym, int mod, int scan, int cap)
{
    QQnxScreenEventHandler::injectKeyboardEvent(flags, sym, mod, scan, cap);
}

void QQnxScreenEventThread::run()
{
    screen_event_t event;

    // create screen event
    errno = 0;
    int result = screen_create_event(&event);
    if (result)
        qFatal("QQNX: failed to create screen event, errno=%d", errno);

    qScreenEventThreadDebug() << Q_FUNC_INFO << "screen event thread started";

    // loop indefinitely
    while (!m_quit) {

        // block until screen event is available
        errno = 0;
        result = screen_get_event(m_screenContext, event, -1);
        if (result)
            qFatal("QQNX: failed to get screen event, errno=%d", errno);

        // process received event
        // get the event type
        errno = 0;
        int qnxType;
        result = screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &qnxType);
        if (result)
            qFatal("QQNX: failed to query screen event type, errno=%d", errno);

        if (qnxType == SCREEN_EVENT_USER) {
            // treat all user events as shutdown requests
            qScreenEventThreadDebug() << Q_FUNC_INFO << "QNX user screen event";
            m_quit = true;
        } else {
            m_screenEventHandler->handleEvent(event, qnxType);
        }
    }

    qScreenEventThreadDebug() << Q_FUNC_INFO << "screen event thread stopped";

    // cleanup
    screen_destroy_event(event);
}

void QQnxScreenEventThread::shutdown()
{
    screen_event_t event;

    // create screen event
    errno = 0;
    int result = screen_create_event(&event);
    if (result)
        qFatal("QQNX: failed to create screen event, errno=%d", errno);

    // set the event type as user
    errno = 0;
    int type = SCREEN_EVENT_USER;
    result = screen_set_event_property_iv(event, SCREEN_PROPERTY_TYPE, &type);
    if (result)
        qFatal("QQNX: failed to set screen event type, errno=%d", errno);

    // NOTE: ignore SCREEN_PROPERTY_USER_DATA; treat all user events as shutdown events

    // post event to event loop so it will wake up and die
    errno = 0;
    result = screen_send_event(m_screenContext, event, getpid());
    if (result)
        qFatal("QQNX: failed to set screen event type, errno=%d", errno);

    // cleanup
    screen_destroy_event(event);

    qScreenEventThreadDebug() << Q_FUNC_INFO << "screen event thread shutdown begin";

    // block until thread terminates
    wait();

    qScreenEventThreadDebug() << Q_FUNC_INFO << "screen event thread shutdown end";
}
