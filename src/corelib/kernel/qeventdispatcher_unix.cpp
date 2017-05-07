/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qplatformdefs.h"

#include "qcoreapplication.h"
#include "qpair.h"
#include "qsocketnotifier.h"
#include "qthread.h"
#include "qelapsedtimer.h"

#include "qeventdispatcher_unix_p.h"
#include <private/qthread_p.h>
#include <private/qcoreapplication_p.h>
#include <private/qcore_unix_p.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef QT_NO_EVENTFD
#  include <sys/eventfd.h>
#endif

// VxWorks doesn't correctly set the _POSIX_... options
#if defined(Q_OS_VXWORKS)
#  if defined(_POSIX_MONOTONIC_CLOCK) && (_POSIX_MONOTONIC_CLOCK <= 0)
#    undef _POSIX_MONOTONIC_CLOCK
#    define _POSIX_MONOTONIC_CLOCK 1
#  endif
#  include <pipeDrv.h>
#  include <sys/time.h>
#endif

#if (_POSIX_MONOTONIC_CLOCK-0 <= 0) || defined(QT_BOOTSTRAPPED)
#  include <sys/times.h>
#endif

QT_BEGIN_NAMESPACE

static const char *socketType(QSocketNotifier::Type type)
{
    switch (type) {
    case QSocketNotifier::Read:
        return "Read";
    case QSocketNotifier::Write:
        return "Write";
    case QSocketNotifier::Exception:
        return "Exception";
    }

    Q_UNREACHABLE();
}

QThreadPipe::QThreadPipe()
{
    fds[0] = -1;
    fds[1] = -1;
#if defined(Q_OS_VXWORKS)
    name[0] = '\0';
#endif
}

QThreadPipe::~QThreadPipe()
{
    if (fds[0] >= 0)
        close(fds[0]);

    if (fds[1] >= 0)
        close(fds[1]);

#if defined(Q_OS_VXWORKS)
    pipeDevDelete(name, true);
#endif
}

#if defined(Q_OS_VXWORKS)
static void initThreadPipeFD(int fd)
{
    int ret = fcntl(fd, F_SETFD, FD_CLOEXEC);
    if (ret == -1)
        perror("QEventDispatcherUNIXPrivate: Unable to init thread pipe");

    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        perror("QEventDispatcherUNIXPrivate: Unable to get flags on thread pipe");

    ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1)
        perror("QEventDispatcherUNIXPrivate: Unable to set flags on thread pipe");
}
#endif

bool QThreadPipe::init()
{
#if defined(Q_OS_NACL)
   // do nothing.
#elif defined(Q_OS_VXWORKS)
    qsnprintf(name, sizeof(name), "/pipe/qt_%08x", int(taskIdSelf()));

    // make sure there is no pipe with this name
    pipeDevDelete(name, true);

    // create the pipe
    if (pipeDevCreate(name, 128 /*maxMsg*/, 1 /*maxLength*/) != OK) {
        perror("QThreadPipe: Unable to create thread pipe device %s", name);
        return false;
    }

    if ((fds[0] = open(name, O_RDWR, 0)) < 0) {
        perror("QThreadPipe: Unable to open pipe device %s", name);
        return false;
    }

    initThreadPipeFD(fds[0]);
    fds[1] = fds[0];
#else
#  ifndef QT_NO_EVENTFD
    if ((fds[0] = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) >= 0)
        return true;
#  endif
    if (qt_safe_pipe(fds, O_NONBLOCK) == -1) {
        perror("QThreadPipe: Unable to create pipe");
        return false;
    }
#endif

    return true;
}

pollfd QThreadPipe::prepare() const
{
    return qt_make_pollfd(fds[0], POLLIN);
}

void QThreadPipe::wakeUp()
{
    if (wakeUps.testAndSetAcquire(0, 1)) {
#ifndef QT_NO_EVENTFD
        if (fds[1] == -1) {
            // eventfd
            eventfd_t value = 1;
            int ret;
            EINTR_LOOP(ret, eventfd_write(fds[0], value));
            return;
        }
#endif
        char c = 0;
        qt_safe_write(fds[1], &c, 1);
    }
}

int QThreadPipe::check(const pollfd &pfd)
{
    Q_ASSERT(pfd.fd == fds[0]);

    char c[16];
    const int readyread = pfd.revents & POLLIN;

    if (readyread) {
        // consume the data on the thread pipe so that
        // poll doesn't immediately return next time
#if defined(Q_OS_VXWORKS)
        ::read(fds[0], c, sizeof(c));
        ::ioctl(fds[0], FIOFLUSH, 0);
#else
#  ifndef QT_NO_EVENTFD
        if (fds[1] == -1) {
            // eventfd
            eventfd_t value;
            eventfd_read(fds[0], &value);
        } else
#  endif
        {
            while (::read(fds[0], c, sizeof(c)) > 0) {}
        }
#endif

        if (!wakeUps.testAndSetRelease(1, 0)) {
            // hopefully, this is dead code
            qWarning("QThreadPipe: internal error, wakeUps.testAndSetRelease(1, 0) failed!");
        }
    }

    return readyread;
}

QEventDispatcherUNIXPrivate::QEventDispatcherUNIXPrivate()
{
    if (Q_UNLIKELY(threadPipe.init() == false))
        qFatal("QEventDispatcherUNIXPrivate(): Can not continue without a thread pipe");
}

QEventDispatcherUNIXPrivate::~QEventDispatcherUNIXPrivate()
{
    // cleanup timers
    qDeleteAll(timerList);
}

void QEventDispatcherUNIXPrivate::setSocketNotifierPending(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);

    if (pendingNotifiers.contains(notifier))
        return;

    pendingNotifiers << notifier;
}

int QEventDispatcherUNIXPrivate::activateTimers()
{
    return timerList.activateTimers();
}

void QEventDispatcherUNIXPrivate::markPendingSocketNotifiers()
{
    for (const pollfd &pfd : qAsConst(pollfds)) {
        if (pfd.fd < 0 || pfd.revents == 0)
            continue;

        auto it = socketNotifiers.find(pfd.fd);
        Q_ASSERT(it != socketNotifiers.end());

        const QSocketNotifierSetUNIX &sn_set = it.value();

        static const struct {
            QSocketNotifier::Type type;
            short flags;
        } notifiers[] = {
            { QSocketNotifier::Read,      POLLIN  | POLLHUP | POLLERR },
            { QSocketNotifier::Write,     POLLOUT | POLLHUP | POLLERR },
            { QSocketNotifier::Exception, POLLPRI | POLLHUP | POLLERR }
        };

        for (const auto &n : notifiers) {
            QSocketNotifier *notifier = sn_set.notifiers[n.type];

            if (!notifier)
                continue;

            if (pfd.revents & POLLNVAL) {
                qWarning("QSocketNotifier: Invalid socket %d with type %s, disabling...",
                         it.key(), socketType(n.type));
                notifier->setEnabled(false);
            }

            if (pfd.revents & n.flags)
                setSocketNotifierPending(notifier);
        }
    }

    pollfds.clear();
}

int QEventDispatcherUNIXPrivate::activateSocketNotifiers()
{
    markPendingSocketNotifiers();

    if (pendingNotifiers.isEmpty())
        return 0;

    int n_activated = 0;
    QEvent event(QEvent::SockAct);

    while (!pendingNotifiers.isEmpty()) {
        QSocketNotifier *notifier = pendingNotifiers.takeFirst();
        QCoreApplication::sendEvent(notifier, &event);
        ++n_activated;
    }

    return n_activated;
}

QEventDispatcherUNIX::QEventDispatcherUNIX(QObject *parent)
    : QAbstractEventDispatcher(*new QEventDispatcherUNIXPrivate, parent)
{ }

QEventDispatcherUNIX::QEventDispatcherUNIX(QEventDispatcherUNIXPrivate &dd, QObject *parent)
    : QAbstractEventDispatcher(dd, parent)
{ }

QEventDispatcherUNIX::~QEventDispatcherUNIX()
{ }

/*!
    \internal
*/
void QEventDispatcherUNIX::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *obj)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1 || interval < 0 || !obj) {
        qWarning("QEventDispatcherUNIX::registerTimer: invalid arguments");
        return;
    } else if (obj->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherUNIX::registerTimer: timers cannot be started from another thread");
        return;
    }
#endif

    Q_D(QEventDispatcherUNIX);
    d->timerList.registerTimer(timerId, interval, timerType, obj);
}

/*!
    \internal
*/
bool QEventDispatcherUNIX::unregisterTimer(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherUNIX::unregisterTimer: invalid argument");
        return false;
    } else if (thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherUNIX::unregisterTimer: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QEventDispatcherUNIX);
    return d->timerList.unregisterTimer(timerId);
}

/*!
    \internal
*/
bool QEventDispatcherUNIX::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherUNIX::unregisterTimers: invalid argument");
        return false;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherUNIX::unregisterTimers: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QEventDispatcherUNIX);
    return d->timerList.unregisterTimers(object);
}

QList<QEventDispatcherUNIX::TimerInfo>
QEventDispatcherUNIX::registeredTimers(QObject *object) const
{
    if (!object) {
        qWarning("QEventDispatcherUNIX:registeredTimers: invalid argument");
        return QList<TimerInfo>();
    }

    Q_D(const QEventDispatcherUNIX);
    return d->timerList.registeredTimers(object);
}

/*****************************************************************************
 QEventDispatcher implementations for UNIX
 *****************************************************************************/

void QEventDispatcherUNIX::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
    int sockfd = notifier->socket();
    QSocketNotifier::Type type = notifier->type();
#ifndef QT_NO_DEBUG
    if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QSocketNotifier: socket notifiers cannot be enabled from another thread");
        return;
    }
#endif

    Q_D(QEventDispatcherUNIX);
    QSocketNotifierSetUNIX &sn_set = d->socketNotifiers[sockfd];

    if (sn_set.notifiers[type] && sn_set.notifiers[type] != notifier)
        qWarning("%s: Multiple socket notifiers for same socket %d and type %s",
                 Q_FUNC_INFO, sockfd, socketType(type));

    sn_set.notifiers[type] = notifier;
}

void QEventDispatcherUNIX::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
    int sockfd = notifier->socket();
    QSocketNotifier::Type type = notifier->type();
#ifndef QT_NO_DEBUG
    if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QSocketNotifier: socket notifier (fd %d) cannot be disabled from another thread.\n"
                "(Notifier's thread is %s(%p), event dispatcher's thread is %s(%p), current thread is %s(%p))",
                sockfd,
                notifier->thread() ? notifier->thread()->metaObject()->className() : "QThread", notifier->thread(),
                thread() ? thread()->metaObject()->className() : "QThread", thread(),
                QThread::currentThread() ? QThread::currentThread()->metaObject()->className() : "QThread", QThread::currentThread());
        return;
    }
#endif

    Q_D(QEventDispatcherUNIX);

    d->pendingNotifiers.removeOne(notifier);

    auto i = d->socketNotifiers.find(sockfd);
    if (i == d->socketNotifiers.end())
        return;

    QSocketNotifierSetUNIX &sn_set = i.value();

    if (sn_set.notifiers[type] == nullptr)
        return;

    if (sn_set.notifiers[type] != notifier) {
        qWarning("%s: Multiple socket notifiers for same socket %d and type %s",
                 Q_FUNC_INFO, sockfd, socketType(type));
        return;
    }

    sn_set.notifiers[type] = nullptr;

    if (sn_set.isEmpty())
        d->socketNotifiers.erase(i);
}

bool QEventDispatcherUNIX::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QEventDispatcherUNIX);
    d->interrupt.store(0);

    // we are awake, broadcast it
    emit awake();
    QCoreApplicationPrivate::sendPostedEvents(0, 0, d->threadData);

    const bool include_timers = (flags & QEventLoop::X11ExcludeTimers) == 0;
    const bool include_notifiers = (flags & QEventLoop::ExcludeSocketNotifiers) == 0;
    const bool wait_for_events = flags & QEventLoop::WaitForMoreEvents;

    const bool canWait = (d->threadData->canWaitLocked()
                          && !d->interrupt.load()
                          && wait_for_events);

    if (canWait)
        emit aboutToBlock();

    if (d->interrupt.load())
        return false;

    timespec *tm = nullptr;
    timespec wait_tm = { 0, 0 };

    if (!canWait || (include_timers && d->timerList.timerWait(wait_tm)))
        tm = &wait_tm;

    d->pollfds.clear();
    d->pollfds.reserve(1 + (include_notifiers ? d->socketNotifiers.size() : 0));

    if (include_notifiers)
        for (auto it = d->socketNotifiers.cbegin(); it != d->socketNotifiers.cend(); ++it)
            d->pollfds.append(qt_make_pollfd(it.key(), it.value().events()));

    // This must be last, as it's popped off the end below
    d->pollfds.append(d->threadPipe.prepare());

    int nevents = 0;

    switch (qt_safe_poll(d->pollfds.data(), d->pollfds.size(), tm)) {
    case -1:
        perror("qt_safe_poll");
        break;
    case 0:
        break;
    default:
        nevents += d->threadPipe.check(d->pollfds.takeLast());
        if (include_notifiers)
            nevents += d->activateSocketNotifiers();
        break;
    }

    if (include_timers)
        nevents += d->activateTimers();

    // return true if we handled events, false otherwise
    return (nevents > 0);
}

bool QEventDispatcherUNIX::hasPendingEvents()
{
    extern uint qGlobalPostedEventsCount(); // from qapplication.cpp
    return qGlobalPostedEventsCount();
}

int QEventDispatcherUNIX::remainingTime(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherUNIX::remainingTime: invalid argument");
        return -1;
    }
#endif

    Q_D(QEventDispatcherUNIX);
    return d->timerList.timerRemainingTime(timerId);
}

void QEventDispatcherUNIX::wakeUp()
{
    Q_D(QEventDispatcherUNIX);
    d->threadPipe.wakeUp();
}

void QEventDispatcherUNIX::interrupt()
{
    Q_D(QEventDispatcherUNIX);
    d->interrupt.store(1);
    wakeUp();
}

void QEventDispatcherUNIX::flush()
{ }

QT_END_NAMESPACE

#include "moc_qeventdispatcher_unix_p.cpp"
