/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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

#include "qqnxbpseventfilter.h"
#include "qqnxnavigatoreventhandler.h"
#include "qqnxscreen.h"
#include "qqnxscreeneventhandler.h"
#include "qqnxvirtualkeyboardbps.h"

#include <QAbstractEventDispatcher>
#include <QDebug>

#include <bps/event.h>
#include <bps/navigator.h>
#include <bps/screen.h>

QT_BEGIN_NAMESPACE

static QQnxBpsEventFilter *s_instance = 0;

QQnxBpsEventFilter::QQnxBpsEventFilter(QQnxNavigatorEventHandler *navigatorEventHandler,
                                       QQnxScreenEventHandler *screenEventHandler,
                                       QQnxVirtualKeyboardBps *virtualKeyboard, QObject *parent)
    : QObject(parent)
    , m_navigatorEventHandler(navigatorEventHandler)
    , m_screenEventHandler(screenEventHandler)
    , m_virtualKeyboard(virtualKeyboard)
{
    Q_ASSERT(s_instance == 0);

    s_instance = this;
}

QQnxBpsEventFilter::~QQnxBpsEventFilter()
{
    Q_ASSERT(s_instance == this);

    s_instance = 0;
}

void QQnxBpsEventFilter::installOnEventDispatcher(QAbstractEventDispatcher *dispatcher)
{
#if defined(QQNXBPSEVENTFILTER_DEBUG)
    qDebug() << Q_FUNC_INFO << "dispatcher=" << dispatcher;
#endif

    if (navigator_request_events(0) != BPS_SUCCESS)
        qWarning("QQNX: failed to register for navigator events");

    QAbstractEventDispatcher::EventFilter previousEventFilter = dispatcher->setEventFilter(dispatcherEventFilter);

    // the QPA plugin creates the event dispatcher so we are the first event
    // filter assert on that just in case somebody adds another event filter
    // in the QQnxIntegration constructor instead of adding a new section in here
    Q_ASSERT(previousEventFilter == 0);
    Q_UNUSED(previousEventFilter);
}

void QQnxBpsEventFilter::registerForScreenEvents(QQnxScreen *screen)
{
    if (screen_request_events(screen->nativeContext()) != BPS_SUCCESS)
        qWarning("QQNX: failed to register for screen events on screen %p", screen->nativeContext());
}

void QQnxBpsEventFilter::unregisterForScreenEvents(QQnxScreen *screen)
{
    if (screen_stop_events(screen->nativeContext()) != BPS_SUCCESS)
        qWarning("QQNX: failed to unregister for screen events on screen %p", screen->nativeContext());
}

bool QQnxBpsEventFilter::dispatcherEventFilter(void *message)
{
#if defined(QQNXBPSEVENTFILTER_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    if (s_instance == 0)
        return false;

    bps_event_t *event = static_cast<bps_event_t *>(message);
    return s_instance->bpsEventFilter(event);
}

bool QQnxBpsEventFilter::bpsEventFilter(bps_event_t *event)
{
    const int eventDomain = bps_event_get_domain(event);

#if defined(QQNXBPSEVENTFILTER_DEBUG)
    qDebug() << Q_FUNC_INFO << "event=" << event << "domain=" << eventDomain;
#endif

    if (eventDomain == screen_get_domain()) {
        screen_event_t screenEvent = screen_event_get_event(event);
        return m_screenEventHandler->handleEvent(screenEvent);
    }

    if (eventDomain == navigator_get_domain())
        return handleNavigatorEvent(event);

    if (m_virtualKeyboard->handleEvent(event))
        return true;

    return false;
}

bool QQnxBpsEventFilter::handleNavigatorEvent(bps_event_t *event)
{
    switch (bps_event_get_code(event)) {
    case NAVIGATOR_ORIENTATION_CHECK: {
        const int angle = navigator_event_get_orientation_angle(event);

        #if defined(QQNXBPSEVENTFILTER_DEBUG)
        qDebug() << "QQNX: Navigator ORIENTATION CHECK event. angle=" << angle;
        #endif

        const bool result = m_navigatorEventHandler->handleOrientationCheck(angle);

        #if defined(QQNXBPSEVENTFILTER_DEBUG)
        qDebug() << "QQNX: Navigator ORIENTATION CHECK event. result=" << result;
        #endif

        // reply to navigator whether orientation is acceptable
        navigator_orientation_check_response(event, result);
        break;
    }

    case NAVIGATOR_ORIENTATION: {
        const int angle = navigator_event_get_orientation_angle(event);

        #if defined(QQNXBPSEVENTFILTER_DEBUG)
        qDebug() << "QQNX: Navigator ORIENTATION event. angle=" << angle;
        #endif

        m_navigatorEventHandler->handleOrientationChange(angle);

        navigator_done_orientation(event);
        break;
    }

    case NAVIGATOR_SWIPE_DOWN:
        #if defined(QQNXBPSEVENTFILTER_DEBUG)
        qDebug() << "QQNX: Navigator SWIPE DOWN event";
        #endif

        m_navigatorEventHandler->handleSwipeDown();
        break;

    case NAVIGATOR_EXIT:
        #if defined(QQNXBPSEVENTFILTER_DEBUG)
        qDebug() << "QQNX: Navigator EXIT event";
        #endif

        m_navigatorEventHandler->handleExit();
        break;

    default:
        #if defined(QQNXBPSEVENTFILTER_DEBUG)
        qDebug() << "QQNX: Unhandled navigator event. code=" << bps_event_get_code(event);
        #endif
        return false;
    }

    return true;
}

QT_END_NAMESPACE
