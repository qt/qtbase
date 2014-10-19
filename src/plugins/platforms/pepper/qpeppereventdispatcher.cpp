/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpeppereventdispatcher.h"
#include "qpeppermodule.h"

#include <qdebug.h>
#include <QtCore/qcoreapplication.h>

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_EVENTDISPATHCER, "qt.platform.pepper.eventdispatcher");

QPepperEventDispatcher::QPepperEventDispatcher(QObject *parent)
:QUnixEventDispatcherQPA(parent)
,m_currentTimerSerial(0)
,m_completionCallbackFactory(this)
{
    qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "QPepperEventDispatcher()";
    messageLoop = pp::MessageLoop::GetCurrent();
}

QPepperEventDispatcher::~QPepperEventDispatcher()
{

}

void QPepperEventDispatcher::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
    qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "QPepperEventDispatcher::registerTimer" << timerId << interval << object;

    // Maintain QObject *-> Qt timer id mapping.
    m_activeObjectTimers.insertMulti(object, timerId);

    // Capture timer detail for later use
    PepperTimerInfo timerInfo(timerId, interval, timerType, object);
    m_timerDetails.insert(timerId, timerInfo);

    startTimer(timerInfo);
}

bool QPepperEventDispatcher::unregisterTimer(int timerId)
{
    qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "QPepperEventDispatcher::unregisterTimer" << timerId;

    // Remove QObject -> Qt timer id mapping.
    QObject *timerObject = m_timerDetails.value(timerId).object;
    m_activeObjectTimers.remove(timerObject, timerId);

    m_timerDetails.remove(timerId);

    // Remove the timerId and the currently active serial.
    int timerSerial = m_activeTimerSerials.value(timerId);
    m_activeTimerSerials.remove(timerId);
    m_activeTimerIds.remove(timerSerial);

    return true;
}

bool QPepperEventDispatcher::unregisterTimers(QObject *object)
{
    qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "QPepperEventDispatcher::unregisterTimers" << object;

    // Find all active Qt timer ids for the given object and copy them to a list.
    // (we will update the QObject * -> timerId hash while iterating the list later on)
    QList<int> timerIds;
    QMultiHash<QObject *, int>::iterator it = m_activeObjectTimers.find(object);
    while (it != m_activeObjectTimers.end() && it.key() == object)
        timerIds.append(it.value());

    // Unregister each timer.
    foreach(int timerId, timerIds)
        unregisterTimer(timerId);

    return true;
}

bool QPepperEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    do {
        QUnixEventDispatcherQPA::processEvents(flags);
    } while (hasPendingEvents());
    return true;
}

bool QPepperEventDispatcher::hasPendingEvents()
{
    return QUnixEventDispatcherQPA::hasPendingEvents();
}

void QPepperEventDispatcher::flush()
{
    qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "QPepperEventDispatcher::flush";

    QUnixEventDispatcherQPA::flush();
}

void QPepperEventDispatcher::wakeup()
{
    qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "QPepperEventDispatcher::wakeup";
    scheduleProcessEvents();
}

void QPepperEventDispatcher::startTimer(PepperTimerInfo info)
{
    qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "startTimer" << info.timerId;

    // Allocate a timer serial value for this startTimer call.
    //
    // Qt reuses timer id's. This means there is time for Qt to unregister a
    // timer and register a new timer with the same id before our timer
    // callback is called.
    //
    // This is compounded by single-shot timer behavior: We are not informed
    // that a given timer is of the single-shot variant. Instead Qt will
    // unregister it during the timer fire event.
    //
    // This is further compounded by the fact that the Pepper timer API is of
    // the single-shot variant. After sending the timer event to Qt a decision
    // has to be made if the timer should be rescheduled or not.
    //
    // In summary: To avoid confusing separate uses of the same timer id we
    // give each call to CallOnMainThread a serial number which we can
    // independently cancel.
    //
    ++m_currentTimerSerial;
    m_activeTimerIds[m_currentTimerSerial] = info.timerId;
    m_activeTimerSerials[info.timerId] = m_currentTimerSerial;

    QtModule::core()->CallOnMainThread(info.interval,
        m_completionCallbackFactory.NewCallback(&QPepperEventDispatcher::timerCallback), m_currentTimerSerial);
}

void QPepperEventDispatcher::timerCallback(int32_t timerSerial)
{
    qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "timerCallback" << timerSerial;

    // The timer might have been unregistered after the CallOnMainThread was
    // made in startTimer. In that case don't fire.
    if (!m_activeTimerIds.contains(timerSerial))
        return;

    // Get the timer info for the timerSerial/timerID.
    int timerId = m_activeTimerIds.value(timerSerial);
    const PepperTimerInfo &info = m_timerDetails.value(timerId);

    // Send the timer event
    QTimerEvent e(info.timerId);
    QCoreApplication::sendEvent(info.object, &e);
    processEvents();

    // After running Qt and application code the timer may have been unregistered,
    // and the timer id may have been reused. The timerSerial will hower not
    // be reused; use that to determine if the timer is active.
    if (m_activeTimerIds.contains(timerSerial))
        startTimer(info);

    // one serial number per callback, we are done with this one.
    m_activeTimerIds.remove(timerSerial);
}

void QPepperEventDispatcher::scheduleProcessEvents()
{
    qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "scheduleProcessEvents";
    pp::CompletionCallback processEvents =
        m_completionCallbackFactory.NewCallback(&QPepperEventDispatcher::processEventsCallback);
    int32_t result = messageLoop.PostWork(processEvents);
    if (result != PP_OK)
        qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "scheduleProcessEvents PostWork error" << result;
}

void QPepperEventDispatcher::processEventsCallback(int32_t status)
{
    Q_UNUSED(status);
    qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "processEvents";

    processEvents();
}


