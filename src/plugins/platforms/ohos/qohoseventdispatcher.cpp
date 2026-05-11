// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohoseventdispatcher.h"
#include "qohosjsmain.h"
#include "qohosdeadlockprotector.h"

QOhosEventDispatcher::QOhosEventDispatcher(QObject *parent) :
    QUnixEventDispatcherQPA(parent)
{
    if (QtOhos::blockEventLoopsWhenSuspended())
        QOhosEventDispatcherStopper::instance()->addEventDispatcher(this);
}

QOhosEventDispatcher::~QOhosEventDispatcher()
{
    if (QtOhos::blockEventLoopsWhenSuspended())
        QOhosEventDispatcherStopper::instance()->removeEventDispatcher(this);
}

enum States {Running = 0, StopRequest = 1, Stopping = 2};

void QOhosEventDispatcher::start()
{
    int prevState = m_stopRequest.fetchAndStoreAcquire(Running);
    if (prevState == Stopping) {
        m_semaphore.release();
        wakeUp();
    } else if (prevState == Running) {
        qWarning("Error: start without corresponding stop");
    }
    //else if prevState == StopRequest, no action needed
}

void QOhosEventDispatcher::stop()
{
    if (m_stopRequest.testAndSetAcquire(Running, StopRequest))
        wakeUp();
    else
        qWarning("Error: start/stop out of sync");
}

void QOhosEventDispatcher::goingToStop(bool stop)
{
    m_goingToStop.storeRelaxed(stop);
    if (!stop)
        wakeUp();
}

bool QOhosEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    if (m_goingToStop.loadRelaxed())
        flags |= QEventLoop::ExcludeSocketNotifiers | QEventLoop::X11ExcludeTimers;

    {
        QOhosDeadlockProtector protector;
        if (protector.acquire() && m_stopRequest.testAndSetAcquire(StopRequest, Stopping)) {
            m_semaphore.acquire();
            wakeUp();
        }
    }

    return QUnixEventDispatcherQPA::processEvents(flags);
}

QOhosEventDispatcherStopper *QOhosEventDispatcherStopper::instance()
{
    static QOhosEventDispatcherStopper ohosEventDispatcherStopper;
    return &ohosEventDispatcherStopper;
}

void QOhosEventDispatcherStopper::startAll()
{
    QMutexLocker lock(&m_mutex);
    if (!m_started.testAndSetOrdered(0, 1))
        return;

    for (QOhosEventDispatcher *d : std::as_const(m_dispatchers))
        d->start();
}

void QOhosEventDispatcherStopper::stopAll()
{
    QMutexLocker lock(&m_mutex);
    if (!m_started.testAndSetOrdered(1, 0))
        return;

    for (QOhosEventDispatcher *d : std::as_const(m_dispatchers))
        d->stop();
}

void QOhosEventDispatcherStopper::addEventDispatcher(QOhosEventDispatcher *dispatcher)
{
    QMutexLocker lock(&m_mutex);
    m_dispatchers.push_back(dispatcher);
}

void QOhosEventDispatcherStopper::removeEventDispatcher(QOhosEventDispatcher *dispatcher)
{
    QMutexLocker lock(&m_mutex);
    m_dispatchers.erase(std::find(m_dispatchers.begin(), m_dispatchers.end(), dispatcher));
}

void QOhosEventDispatcherStopper::goingToStop(bool stop)
{
    QMutexLocker lock(&m_mutex);
    for (QOhosEventDispatcher *d : std::as_const(m_dispatchers))
        d->goingToStop(stop);
}
