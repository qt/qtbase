/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#if defined(QQNXSCREENEVENTTHREAD_DEBUG)
    qDebug() << "QQNX: screen event thread started";
#endif

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
    #if defined(QQNXSCREENEVENTTHREAD_DEBUG)
            qDebug() << "QQNX: QNX user screen event";
    #endif
            m_quit = true;
        } else {
            m_screenEventHandler->handleEvent(event, qnxType);
        }
    }

#if defined(QQNXSCREENEVENTTHREAD_DEBUG)
    qDebug() << "QQNX: screen event thread stopped";
#endif

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

#if defined(QQNXSCREENEVENTTHREAD_DEBUG)
    qDebug() << "QQNX: screen event thread shutdown begin";
#endif

    // block until thread terminates
    wait();

#if defined(QQNXSCREENEVENTTHREAD_DEBUG)
    qDebug() << "QQNX: screen event thread shutdown end";
#endif
}
