/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qandroideventdispatcher.h"
#include "androidjnimain.h"
#include "androiddeadlockprotector.h"

QAndroidEventDispatcher::QAndroidEventDispatcher(QObject *parent) :
    QUnixEventDispatcherQPA(parent)
{
    if (QtAndroid::blockEventLoopsWhenSuspended())
        QAndroidEventDispatcherStopper::instance()->addEventDispatcher(this);
}

QAndroidEventDispatcher::~QAndroidEventDispatcher()
{
    if (QtAndroid::blockEventLoopsWhenSuspended())
        QAndroidEventDispatcherStopper::instance()->removeEventDispatcher(this);
}

enum States {Running = 0, StopRequest = 1, Stopping = 2};

void QAndroidEventDispatcher::start()
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

void QAndroidEventDispatcher::stop()
{
    if (m_stopRequest.testAndSetAcquire(Running, StopRequest))
        wakeUp();
    else
        qWarning("Error: start/stop out of sync");
}

void QAndroidEventDispatcher::goingToStop(bool stop)
{
    m_goingToStop.storeRelaxed(stop ? 1 : 0);
    if (!stop)
        wakeUp();
}

bool QAndroidEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    if (m_goingToStop.loadRelaxed())
        flags |= QEventLoop::ExcludeSocketNotifiers | QEventLoop::X11ExcludeTimers;

    {
        AndroidDeadlockProtector protector;
        if (protector.acquire() && m_stopRequest.testAndSetAcquire(StopRequest, Stopping)) {
            m_semaphore.acquire();
            wakeUp();
        }
    }

    return QUnixEventDispatcherQPA::processEvents(flags);
}

QAndroidEventDispatcherStopper *QAndroidEventDispatcherStopper::instance()
{
    static QAndroidEventDispatcherStopper androidEventDispatcherStopper;
    return &androidEventDispatcherStopper;
}

void QAndroidEventDispatcherStopper::startAll()
{
    QMutexLocker lock(&m_mutex);
    if (!m_started.testAndSetOrdered(0, 1))
        return;

    for (QAndroidEventDispatcher *d : qAsConst(m_dispatchers))
        d->start();
}

void QAndroidEventDispatcherStopper::stopAll()
{
    QMutexLocker lock(&m_mutex);
    if (!m_started.testAndSetOrdered(1, 0))
        return;

    for (QAndroidEventDispatcher *d : qAsConst(m_dispatchers))
        d->stop();
}

void QAndroidEventDispatcherStopper::addEventDispatcher(QAndroidEventDispatcher *dispatcher)
{
    QMutexLocker lock(&m_mutex);
    m_dispatchers.push_back(dispatcher);
}

void QAndroidEventDispatcherStopper::removeEventDispatcher(QAndroidEventDispatcher *dispatcher)
{
    QMutexLocker lock(&m_mutex);
    m_dispatchers.erase(std::find(m_dispatchers.begin(), m_dispatchers.end(), dispatcher));
}

void QAndroidEventDispatcherStopper::goingToStop(bool stop)
{
    QMutexLocker lock(&m_mutex);
    for (QAndroidEventDispatcher *d : qAsConst(m_dispatchers))
        d->goingToStop(stop);
}
