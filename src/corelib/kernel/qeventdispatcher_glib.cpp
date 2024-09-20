// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeventdispatcher_glib_p.h"
#include "qeventdispatcher_unix_p.h"

#include <private/qthread_p.h>

#include "qcoreapplication.h"
#include "qsocketnotifier.h"

#include <QtCore/qlist.h>

#include <QtCore/q26numeric.h>

#include <glib.h>

using namespace std::chrono;
using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

struct GPollFDWithQSocketNotifier
{
    GPollFD pollfd;
    QSocketNotifier *socketNotifier;
};

struct GSocketNotifierSource
{
    GSource source;
    QList<GPollFDWithQSocketNotifier *> pollfds;
    qsizetype activeNotifierPos;
};

static gboolean socketNotifierSourcePrepare(GSource *, gint *timeout)
{
    if (timeout)
        *timeout = -1;
    return false;
}

static gboolean socketNotifierSourceCheck(GSource *source)
{
    GSocketNotifierSource *src = reinterpret_cast<GSocketNotifierSource *>(source);

    bool pending = false;
    for (int i = 0; !pending && i < src->pollfds.size(); ++i) {
        GPollFDWithQSocketNotifier *p = src->pollfds.at(i);

        if (p->pollfd.revents & G_IO_NVAL) {
            // disable the invalid socket notifier
            const char * const t[] = { "Read", "Write", "Exception" };
            qWarning("QSocketNotifier: Invalid socket %d and type '%s', disabling...",
                     p->pollfd.fd, t[int(p->socketNotifier->type())]);
            // ### note, modifies src->pollfds!
            p->socketNotifier->setEnabled(false);
            i--;
        } else {
            pending = pending || ((p->pollfd.revents & p->pollfd.events) != 0);
        }
    }

    return pending;
}

static gboolean socketNotifierSourceDispatch(GSource *source, GSourceFunc, gpointer)
{
    QEvent event(QEvent::SockAct);

    GSocketNotifierSource *src = reinterpret_cast<GSocketNotifierSource *>(source);
    for (src->activeNotifierPos = 0; src->activeNotifierPos < src->pollfds.size();
         ++src->activeNotifierPos) {
        GPollFDWithQSocketNotifier *p = src->pollfds.at(src->activeNotifierPos);

        if ((p->pollfd.revents & p->pollfd.events) != 0)
            QCoreApplication::sendEvent(p->socketNotifier, &event);
    }

    return true; // ??? don't remove, right?
}

Q_CONSTINIT static GSourceFuncs socketNotifierSourceFuncs = {
    socketNotifierSourcePrepare,
    socketNotifierSourceCheck,
    socketNotifierSourceDispatch,
    nullptr,
    nullptr,
    nullptr
};

struct GTimerSource
{
    GSource source;
    QTimerInfoList timerList;
    QEventLoop::ProcessEventsFlags processEventsFlags;
    bool runWithIdlePriority;
};

static gboolean timerSourcePrepareHelper(GTimerSource *src, gint *timeout)
{
    if (src->processEventsFlags & QEventLoop::X11ExcludeTimers) {
        *timeout = -1;
        return true;
    }

    auto remaining = src->timerList.timerWait().value_or(-1ms);
    *timeout = q26::saturate_cast<gint>(ceil<milliseconds>(remaining).count());

    return (*timeout == 0);
}

static gboolean timerSourceCheckHelper(GTimerSource *src)
{
    if (src->timerList.isEmpty()
        || (src->processEventsFlags & QEventLoop::X11ExcludeTimers))
        return false;

    return !src->timerList.hasPendingTimers();
}

static gboolean timerSourcePrepare(GSource *source, gint *timeout)
{
    gint dummy;
    if (!timeout)
        timeout = &dummy;

    GTimerSource *src = reinterpret_cast<GTimerSource *>(source);
    if (src->runWithIdlePriority) {
        if (timeout)
            *timeout = -1;
        return false;
    }

    return timerSourcePrepareHelper(src, timeout);
}

static gboolean timerSourceCheck(GSource *source)
{
    GTimerSource *src = reinterpret_cast<GTimerSource *>(source);
    if (src->runWithIdlePriority)
        return false;
    return timerSourceCheckHelper(src);
}

static gboolean timerSourceDispatch(GSource *source, GSourceFunc, gpointer)
{
    GTimerSource *timerSource = reinterpret_cast<GTimerSource *>(source);
    if (timerSource->processEventsFlags & QEventLoop::X11ExcludeTimers)
        return true;
    timerSource->runWithIdlePriority = true;
    (void) timerSource->timerList.activateTimers();
    return true; // ??? don't remove, right again?
}

Q_CONSTINIT static GSourceFuncs timerSourceFuncs = {
    timerSourcePrepare,
    timerSourceCheck,
    timerSourceDispatch,
    nullptr,
    nullptr,
    nullptr
};

struct GIdleTimerSource
{
    GSource source;
    GTimerSource *timerSource;
};

static gboolean idleTimerSourcePrepare(GSource *source, gint *timeout)
{
    GIdleTimerSource *idleTimerSource = reinterpret_cast<GIdleTimerSource *>(source);
    GTimerSource *timerSource = idleTimerSource->timerSource;
    if (!timerSource->runWithIdlePriority) {
        // Yield to the normal priority timer source
        if (timeout)
            *timeout = -1;
        return false;
    }

    return timerSourcePrepareHelper(timerSource, timeout);
}

static gboolean idleTimerSourceCheck(GSource *source)
{
    GIdleTimerSource *idleTimerSource = reinterpret_cast<GIdleTimerSource *>(source);
    GTimerSource *timerSource = idleTimerSource->timerSource;
    if (!timerSource->runWithIdlePriority) {
        // Yield to the normal priority timer source
        return false;
    }
    return timerSourceCheckHelper(timerSource);
}

static gboolean idleTimerSourceDispatch(GSource *source, GSourceFunc, gpointer)
{
    GTimerSource *timerSource = reinterpret_cast<GIdleTimerSource *>(source)->timerSource;
    (void) timerSourceDispatch(&timerSource->source, nullptr, nullptr);
    return true;
}

Q_CONSTINIT static GSourceFuncs idleTimerSourceFuncs = {
    idleTimerSourcePrepare,
    idleTimerSourceCheck,
    idleTimerSourceDispatch,
    nullptr,
    nullptr,
    nullptr
};

struct GPostEventSource
{
    GSource source;
    QAtomicInt serialNumber;
    int lastSerialNumber;
    QEventDispatcherGlibPrivate *d;
};

static gboolean postEventSourcePrepare(GSource *s, gint *timeout)
{
    QThreadData *data = QThreadData::current();
    if (!data)
        return false;

    gint dummy;
    if (!timeout)
        timeout = &dummy;
    const bool canWait = data->canWaitLocked();
    *timeout = canWait ? -1 : 0;

    GPostEventSource *source = reinterpret_cast<GPostEventSource *>(s);
    source->d->wakeUpCalled = source->serialNumber.loadRelaxed() != source->lastSerialNumber;
    return !canWait || source->d->wakeUpCalled;
}

static gboolean postEventSourceCheck(GSource *source)
{
    return postEventSourcePrepare(source, nullptr);
}

static gboolean postEventSourceDispatch(GSource *s, GSourceFunc, gpointer)
{
    GPostEventSource *source = reinterpret_cast<GPostEventSource *>(s);
    source->lastSerialNumber = source->serialNumber.loadRelaxed();
    QCoreApplication::sendPostedEvents();
    source->d->runTimersOnceWithNormalPriority();
    return true; // i dunno, george...
}

Q_CONSTINIT static GSourceFuncs postEventSourceFuncs = {
    postEventSourcePrepare,
    postEventSourceCheck,
    postEventSourceDispatch,
    nullptr,
    nullptr,
    nullptr
};


QEventDispatcherGlibPrivate::QEventDispatcherGlibPrivate(GMainContext *context)
    : mainContext(context)
{
#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 32
    if (qEnvironmentVariableIsEmpty("QT_NO_THREADED_GLIB")) {
        Q_CONSTINIT static QBasicMutex mutex;
        QMutexLocker locker(&mutex);
        if (!g_thread_supported())
            g_thread_init(NULL);
    }
#endif

    if (mainContext) {
        g_main_context_ref(mainContext);
    } else {
        QCoreApplication *app = QCoreApplication::instance();
        if (app && QThread::currentThread() == app->thread()) {
            mainContext = g_main_context_default();
            g_main_context_ref(mainContext);
        } else {
            mainContext = g_main_context_new();
        }
    }

#if GLIB_CHECK_VERSION (2, 22, 0)
    g_main_context_push_thread_default (mainContext);
#endif

    // setup post event source
    GSource *source = g_source_new(&postEventSourceFuncs, sizeof(GPostEventSource));
    g_source_set_name(source, "[Qt] GPostEventSource");
    postEventSource = reinterpret_cast<GPostEventSource *>(source);

    postEventSource->serialNumber.storeRelaxed(1);
    postEventSource->d = this;
    g_source_set_can_recurse(&postEventSource->source, true);
    g_source_attach(&postEventSource->source, mainContext);

    // setup socketNotifierSource
    source = g_source_new(&socketNotifierSourceFuncs, sizeof(GSocketNotifierSource));
    g_source_set_name(source, "[Qt] GSocketNotifierSource");
    socketNotifierSource = reinterpret_cast<GSocketNotifierSource *>(source);
    (void) new (&socketNotifierSource->pollfds) QList<GPollFDWithQSocketNotifier *>();
    g_source_set_can_recurse(&socketNotifierSource->source, true);
    g_source_attach(&socketNotifierSource->source, mainContext);

    // setup normal and idle timer sources
    source = g_source_new(&timerSourceFuncs, sizeof(GTimerSource));
    g_source_set_name(source, "[Qt] GTimerSource");
    timerSource = reinterpret_cast<GTimerSource *>(source);
    (void) new (&timerSource->timerList) QTimerInfoList();
    timerSource->processEventsFlags = QEventLoop::AllEvents;
    timerSource->runWithIdlePriority = false;
    g_source_set_can_recurse(&timerSource->source, true);
    g_source_attach(&timerSource->source, mainContext);

    source = g_source_new(&idleTimerSourceFuncs, sizeof(GIdleTimerSource));
    g_source_set_name(source, "[Qt] GIdleTimerSource");
    idleTimerSource = reinterpret_cast<GIdleTimerSource *>(source);
    idleTimerSource->timerSource = timerSource;
    g_source_set_can_recurse(&idleTimerSource->source, true);
    g_source_attach(&idleTimerSource->source, mainContext);
}

QEventDispatcherGlibPrivate::~QEventDispatcherGlibPrivate()
    = default;

void QEventDispatcherGlibPrivate::runTimersOnceWithNormalPriority()
{
    timerSource->runWithIdlePriority = false;
}

QEventDispatcherGlib::QEventDispatcherGlib(QObject *parent)
    : QAbstractEventDispatcherV2(*(new QEventDispatcherGlibPrivate), parent)
{
}

QEventDispatcherGlib::QEventDispatcherGlib(GMainContext *mainContext, QObject *parent)
    : QAbstractEventDispatcherV2(*(new QEventDispatcherGlibPrivate(mainContext)), parent)
{ }

QEventDispatcherGlib::~QEventDispatcherGlib()
{
    Q_D(QEventDispatcherGlib);

    // destroy all timer sources
    d->timerSource->timerList.clearTimers();
    d->timerSource->timerList.~QTimerInfoList();
    g_source_destroy(&d->timerSource->source);
    g_source_unref(&d->timerSource->source);
    d->timerSource = nullptr;
    g_source_destroy(&d->idleTimerSource->source);
    g_source_unref(&d->idleTimerSource->source);
    d->idleTimerSource = nullptr;

    // destroy socket notifier source
    for (int i = 0; i < d->socketNotifierSource->pollfds.size(); ++i) {
        GPollFDWithQSocketNotifier *p = d->socketNotifierSource->pollfds[i];
        g_source_remove_poll(&d->socketNotifierSource->source, &p->pollfd);
        delete p;
    }
    d->socketNotifierSource->pollfds.~QList<GPollFDWithQSocketNotifier *>();
    g_source_destroy(&d->socketNotifierSource->source);
    g_source_unref(&d->socketNotifierSource->source);
    d->socketNotifierSource = nullptr;

    // destroy post event source
    g_source_destroy(&d->postEventSource->source);
    g_source_unref(&d->postEventSource->source);
    d->postEventSource = nullptr;

    Q_ASSERT(d->mainContext != nullptr);
#if GLIB_CHECK_VERSION (2, 22, 0)
    g_main_context_pop_thread_default (d->mainContext);
#endif
    g_main_context_unref(d->mainContext);
    d->mainContext = nullptr;
}

bool QEventDispatcherGlib::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QEventDispatcherGlib);

    const bool canWait = flags.testAnyFlag(QEventLoop::WaitForMoreEvents);
    if (canWait)
        emit aboutToBlock();
    else
        emit awake();

    // tell postEventSourcePrepare() and timerSource about any new flags
    QEventLoop::ProcessEventsFlags savedFlags = d->timerSource->processEventsFlags;
    d->timerSource->processEventsFlags = flags;

    if (!(flags & QEventLoop::EventLoopExec)) {
        // force timers to be sent at normal priority
        d->timerSource->runWithIdlePriority = false;
    }

    bool result = g_main_context_iteration(d->mainContext, canWait);
    while (!result && canWait)
        result = g_main_context_iteration(d->mainContext, canWait);

    d->timerSource->processEventsFlags = savedFlags;

    if (canWait)
        emit awake();

    return result;
}

void QEventDispatcherGlib::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
    int sockfd = int(notifier->socket());
    int type = notifier->type();
#ifndef QT_NO_DEBUG
    if (sockfd < 0) {
        qWarning("QSocketNotifier: Internal error");
        return;
    } else if (notifier->thread() != thread()
               || thread() != QThread::currentThread()) {
        qWarning("QSocketNotifier: socket notifiers cannot be enabled from another thread");
        return;
    }
#endif

    Q_D(QEventDispatcherGlib);


    GPollFDWithQSocketNotifier *p = new GPollFDWithQSocketNotifier;
    p->pollfd.fd = sockfd;
    switch (type) {
    case QSocketNotifier::Read:
        p->pollfd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
        break;
    case QSocketNotifier::Write:
        p->pollfd.events = G_IO_OUT | G_IO_ERR;
        break;
    case QSocketNotifier::Exception:
        p->pollfd.events = G_IO_PRI | G_IO_ERR;
        break;
    }
    p->socketNotifier = notifier;

    d->socketNotifierSource->pollfds.append(p);

    g_source_add_poll(&d->socketNotifierSource->source, &p->pollfd);
}

void QEventDispatcherGlib::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
#ifndef QT_NO_DEBUG
    if (notifier->socket() < 0) {
        qWarning("QSocketNotifier: Internal error");
        return;
    } else if (notifier->thread() != thread()
               || thread() != QThread::currentThread()) {
        qWarning("QSocketNotifier: socket notifiers cannot be disabled from another thread");
        return;
    }
#endif

    Q_D(QEventDispatcherGlib);

    for (qsizetype i = 0; i < d->socketNotifierSource->pollfds.size(); ++i) {
        GPollFDWithQSocketNotifier *p = d->socketNotifierSource->pollfds.at(i);
        if (p->socketNotifier == notifier) {
            // found it
            g_source_remove_poll(&d->socketNotifierSource->source, &p->pollfd);

            d->socketNotifierSource->pollfds.removeAt(i);
            delete p;

            // Keep a position in the list for the next item.
            if (i <= d->socketNotifierSource->activeNotifierPos)
                --d->socketNotifierSource->activeNotifierPos;

            return;
        }
    }
}

void QEventDispatcherGlib::registerTimer(Qt::TimerId timerId, Duration interval,
                                         Qt::TimerType timerType, QObject *object)
{
#ifndef QT_NO_DEBUG
    if (qToUnderlying(timerId) < 1 || interval < 0ns || !object) {
        qWarning("QEventDispatcherGlib::registerTimer: invalid arguments");
        return;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherGlib::registerTimer: timers cannot be started from another thread");
        return;
    }
#endif

    Q_D(QEventDispatcherGlib);
    d->timerSource->timerList.registerTimer(timerId, interval, timerType, object);
}

bool QEventDispatcherGlib::unregisterTimer(Qt::TimerId timerId)
{
#ifndef QT_NO_DEBUG
    if (qToUnderlying(timerId) < 1) {
        qWarning("QEventDispatcherGlib::unregisterTimer: invalid argument");
        return false;
    } else if (thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherGlib::unregisterTimer: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QEventDispatcherGlib);
    return d->timerSource->timerList.unregisterTimer(timerId);
}

bool QEventDispatcherGlib::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherGlib::unregisterTimers: invalid argument");
        return false;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherGlib::unregisterTimers: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QEventDispatcherGlib);
    return d->timerSource->timerList.unregisterTimers(object);
}

QList<QEventDispatcherGlib::TimerInfoV2> QEventDispatcherGlib::timersForObject(QObject *object) const
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherGlib:timersForObject: invalid argument");
        return {};
    }
#endif

    Q_D(const QEventDispatcherGlib);
    return d->timerSource->timerList.registeredTimers(object);
}

QEventDispatcherGlib::Duration QEventDispatcherGlib::remainingTime(Qt::TimerId timerId) const
{
#ifndef QT_NO_DEBUG
    if (qToUnderlying(timerId) < 1) {
        qWarning("QEventDispatcherGlib::remainingTimeTime: invalid argument");
        return Duration::min();
    }
#endif

    Q_D(const QEventDispatcherGlib);
    return d->timerSource->timerList.remainingDuration(timerId);
}

void QEventDispatcherGlib::interrupt()
{
    wakeUp();
}

void QEventDispatcherGlib::wakeUp()
{
    Q_D(QEventDispatcherGlib);
    d->postEventSource->serialNumber.ref();
    g_main_context_wakeup(d->mainContext);
}

bool QEventDispatcherGlib::versionSupported()
{
#if !defined(GLIB_MAJOR_VERSION) || !defined(GLIB_MINOR_VERSION) || !defined(GLIB_MICRO_VERSION)
    return false;
#else
    return ((GLIB_MAJOR_VERSION << 16) + (GLIB_MINOR_VERSION << 8) + GLIB_MICRO_VERSION) >= 0x020301;
#endif
}

QEventDispatcherGlib::QEventDispatcherGlib(QEventDispatcherGlibPrivate &dd, QObject *parent)
    : QAbstractEventDispatcherV2(dd, parent)
{
}

QT_END_NAMESPACE

#include "moc_qeventdispatcher_glib_p.cpp"
