// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"

#include "qcoreapplication.h"
#include "qhash.h"
#include "qsocketnotifier.h"
#include "qthread.h"

#include "qeventdispatcher_unix_p.h"
#include <private/qthread_p.h>
#include <private/qcoreapplication_p.h>
#include <private/qcore_unix_p.h>

#include <cstdio>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#if __has_include(<sys/eventfd.h>)
#  include <sys/eventfd.h>
static constexpr bool UsingEventfd = true;
#else
static constexpr bool UsingEventfd = false;
#endif

#if defined(Q_OS_VXWORKS)
#  include <taskLib.h>
#if QT_CONFIG(vxpipedrv)
#  include "qbytearray.h"
#  include "qdatetime.h"
#  include "qdir.h" // to get application name
#  include "qrandom.h"
#  include <pipeDrv.h>
#  include <selectLib.h>
#  include <rtpLib.h>
#  include <sysLib.h>
#endif
#endif

using namespace std::chrono;
using namespace std::chrono_literals;

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
}

QThreadPipe::~QThreadPipe()
{
    if (fds[0] >= 0)
        close(fds[0]);

    if (!UsingEventfd && fds[1] >= 0)
        close(fds[1]);

#if defined(Q_OS_VXWORKS) && QT_CONFIG(vxpipedrv)
    pipeDevDelete(name, true);
#endif
}

#if defined(Q_OS_VXWORKS) && QT_CONFIG(vxpipedrv)
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
#if defined(Q_OS_WASM)
    // do nothing.
#elif defined(Q_OS_VXWORKS) && QT_CONFIG(vxpipedrv)
    RTP_DESC rtpStruct;
    rtpInfoGet((RTP_ID)NULL, &rtpStruct);

    QByteArray pipeName("/pipe/qevloop_");
    QByteArray path(rtpStruct.pathName);
    pipeName.append(path.mid(path.lastIndexOf(QDir::separator().toLatin1())+1, path.size()));
    pipeName.append("_");
    pipeName.append(QByteArray::number((uint)rtpStruct.entrAddr, 16));
    pipeName.append("_");
    pipeName.append(QByteArray::number((uint)QThread::currentThreadId(), 16));
    pipeName.append("_");
    QRandomGenerator rg(QTime::currentTime().msecsSinceStartOfDay());
    pipeName.append(QByteArray::number(rg.generate()));

    // make sure there is no pipe with this name
    pipeDevDelete(pipeName, true);

    // create the pipe
    if (pipeDevCreate(pipeName, 128 /*maxMsg*/, 1 /*maxLength*/) != OK) {
        qCritical("QThreadPipe: Unable to create thread pipe device %s : %s", name, std::strerror(errno));
        return false;
    }

    if ((fds[0] = open(pipeName, O_RDWR, 0)) < 0) {
        qCritical("QThreadPipe: Unable to open pipe device %s : %s", name, std::strerror(errno));
        return false;
    }

    initThreadPipeFD(fds[0]);
    fds[1] = fds[0];
#else
    int ret;
#  ifdef EFD_CLOEXEC
    ret = fds[0] = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
#  endif
    if (!UsingEventfd)
        ret = qt_safe_pipe(fds, O_NONBLOCK);
    if (ret == -1) {
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
    if ((wakeUps.fetchAndOrAcquire(1) & 1) == 0) {
#  ifdef EFD_CLOEXEC
        eventfd_write(fds[0], 1);
        return;
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
#if defined(Q_OS_VXWORKS) && QT_CONFIG(vxpipedrv)
        ::read(fds[0], c, sizeof(c));
        ::ioctl(fds[0], FIOFLUSH, 0);
#else
#  ifdef EFD_CLOEXEC
        eventfd_t value;
        eventfd_read(fds[0], &value);
#  endif
        if (!UsingEventfd) {
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
        qFatal("QEventDispatcherUNIXPrivate(): Cannot continue without a thread pipe");
}

QEventDispatcherUNIXPrivate::~QEventDispatcherUNIXPrivate()
{
    // cleanup timers
    timerList.clearTimers();
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
    for (const pollfd &pfd : std::as_const(pollfds)) {
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
    : QAbstractEventDispatcherV2(*new QEventDispatcherUNIXPrivate, parent)
{ }

QEventDispatcherUNIX::QEventDispatcherUNIX(QEventDispatcherUNIXPrivate &dd, QObject *parent)
    : QAbstractEventDispatcherV2(dd, parent)
{ }

QEventDispatcherUNIX::~QEventDispatcherUNIX()
{ }

/*!
    \internal
*/
void QEventDispatcherUNIX::registerTimer(Qt::TimerId timerId, Duration interval, Qt::TimerType timerType, QObject *obj)
{
#ifndef QT_NO_DEBUG
    if (qToUnderlying(timerId) < 1 || interval.count() < 0 || !obj) {
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
bool QEventDispatcherUNIX::unregisterTimer(Qt::TimerId timerId)
{
#ifndef QT_NO_DEBUG
    if (qToUnderlying(timerId) < 1) {
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

QList<QEventDispatcherUNIX::TimerInfoV2>
QEventDispatcherUNIX::timersForObject(QObject *object) const
{
    if (!object) {
        qWarning("QEventDispatcherUNIX:registeredTimers: invalid argument");
        return QList<TimerInfoV2>();
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
    d->interrupt.storeRelaxed(0);

    // we are awake, broadcast it
    emit awake();

    auto threadData = d->threadData.loadRelaxed();
    QCoreApplicationPrivate::sendPostedEvents(nullptr, 0, threadData);

    const bool include_timers = (flags & QEventLoop::X11ExcludeTimers) == 0;
    const bool include_notifiers = (flags & QEventLoop::ExcludeSocketNotifiers) == 0;
    const bool wait_for_events = (flags & QEventLoop::WaitForMoreEvents) != 0;

    const bool canWait = (threadData->canWaitLocked()
                          && !d->interrupt.loadRelaxed()
                          && wait_for_events);

    if (canWait)
        emit aboutToBlock();

    if (d->interrupt.loadRelaxed())
        return false;

    QDeadlineTimer deadline;
    if (canWait) {
        if (include_timers) {
            std::optional<nanoseconds> remaining = d->timerList.timerWait();
            deadline = remaining ? QDeadlineTimer{*remaining}
                             : QDeadlineTimer(QDeadlineTimer::Forever);
        } else {
            deadline = QDeadlineTimer(QDeadlineTimer::Forever);
        }
    } else {
        // Using the default-constructed `deadline`, which is already expired,
        // ensures the code in the do-while loop in qt_safe_poll runs at least once.
    }

    d->pollfds.clear();
    d->pollfds.reserve(1 + (include_notifiers ? d->socketNotifiers.size() : 0));

    if (include_notifiers)
        for (auto it = d->socketNotifiers.cbegin(); it != d->socketNotifiers.cend(); ++it)
            d->pollfds.append(qt_make_pollfd(it.key(), it.value().events()));

    // This must be last, as it's popped off the end below
    d->pollfds.append(d->threadPipe.prepare());

    int nevents = 0;
    switch (qt_safe_poll(d->pollfds.data(), d->pollfds.size(), deadline)) {
    case -1:
        qErrnoWarning("qt_safe_poll");
#if defined(Q_OS_VXWORKS) && defined(EDOOM)
        if (errno == EDOOM) {
            // being deleted, stop here and wait for the thread to go away
            taskSuspend(0);
        }
#endif
        if (QT_CONFIG(poll_exit_on_error))
            abort();
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

auto QEventDispatcherUNIX::remainingTime(Qt::TimerId timerId) const -> Duration
{
#ifndef QT_NO_DEBUG
    if (int(timerId) < 1) {
        qWarning("QEventDispatcherUNIX::remainingTime: invalid argument");
        return Duration::min();
    }
#endif

    Q_D(const QEventDispatcherUNIX);
    return d->timerList.remainingDuration(timerId);
}

void QEventDispatcherUNIX::wakeUp()
{
    Q_D(QEventDispatcherUNIX);
    d->threadPipe.wakeUp();
}

void QEventDispatcherUNIX::interrupt()
{
    Q_D(QEventDispatcherUNIX);
    d->interrupt.storeRelaxed(1);
    wakeUp();
}

QT_END_NAMESPACE

#include "moc_qeventdispatcher_unix_p.cpp"
