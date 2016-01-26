/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
    m_goingToStop.store(stop ? 1 : 0);
    if (!stop)
        wakeUp();
}

bool QAndroidEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    if (m_goingToStop.load())
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
    if (started)
        return;

    started = true;
    for (QAndroidEventDispatcher *d : qAsConst(m_dispatchers))
        d->start();
}

void QAndroidEventDispatcherStopper::stopAll()
{
    QMutexLocker lock(&m_mutex);
    if (!started)
        return;

    started = false;
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
