// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeventdispatcher_win_p.h"

#include "qcoreapplication.h"
#include <private/qsystemlibrary_p.h>
#include "qoperatingsystemversion.h"
#include "qpair.h"
#include "qset.h"
#include "qsocketnotifier.h"
#include "qvarlengtharray.h"

#include "qelapsedtimer.h"
#include "qcoreapplication_p.h"
#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE

#ifndef TIME_KILL_SYNCHRONOUS
#  define TIME_KILL_SYNCHRONOUS 0x0100
#endif

#ifndef QS_RAWINPUT
#  define QS_RAWINPUT 0x0400
#endif

#ifndef WM_TOUCH
#  define WM_TOUCH 0x0240
#endif
#ifndef QT_NO_GESTURES
#ifndef WM_GESTURE
#  define WM_GESTURE 0x0119
#endif
#ifndef WM_GESTURENOTIFY
#  define WM_GESTURENOTIFY 0x011A
#endif
#endif // QT_NO_GESTURES

enum {
    WM_QT_SOCKETNOTIFIER = WM_USER,
    WM_QT_SENDPOSTEDEVENTS = WM_USER + 1,
    WM_QT_ACTIVATENOTIFIERS = WM_USER + 2
};

enum {
    SendPostedEventsTimerId = ~1u
};

class QEventDispatcherWin32Private;

#if !defined(DWORD_PTR) && !defined(Q_OS_WIN64)
#define DWORD_PTR DWORD
#endif

LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);

static quint64 qt_msectime()
{
    using namespace std::chrono;
    auto t = duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
    return t.count();
}

QEventDispatcherWin32Private::QEventDispatcherWin32Private()
    : interrupt(false), internalHwnd(0),
      sendPostedEventsTimerId(0), wakeUps(0),
      activateNotifiersPosted(false)
{
}

QEventDispatcherWin32Private::~QEventDispatcherWin32Private()
{
    if (internalHwnd)
        DestroyWindow(internalHwnd);
}

// This function is called by a workerthread
void WINAPI QT_WIN_CALLBACK qt_fast_timer_proc(uint timerId, uint /*reserved*/, DWORD_PTR user, DWORD_PTR /*reserved*/, DWORD_PTR /*reserved*/)
{
    if (!timerId) // sanity check
        return;
    auto t = reinterpret_cast<WinTimerInfo*>(user);
    Q_ASSERT(t);
    QCoreApplication::postEvent(t->dispatcher, new QTimerEvent(t->timerId));
}

LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    if (message == WM_NCCREATE)
        return true;

    MSG msg;
    msg.hwnd = hwnd;
    msg.message = message;
    msg.wParam = wp;
    msg.lParam = lp;
    QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance();
    qintptr result;
    if (!dispatcher) {
        if (message == WM_TIMER)
            KillTimer(hwnd, wp);
        return 0;
    }
    if (dispatcher->filterNativeEvent(QByteArrayLiteral("windows_dispatcher_MSG"), &msg, &result))
        return result;

    auto q = reinterpret_cast<QEventDispatcherWin32 *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    QEventDispatcherWin32Private *d = nullptr;
    if (q != nullptr)
        d = q->d_func();

    switch (message) {
    case WM_QT_SOCKETNOTIFIER: {
        // socket notifier message
        int type = -1;
        switch (WSAGETSELECTEVENT(lp)) {
        case FD_READ:
        case FD_ACCEPT:
            type = 0;
            break;
        case FD_WRITE:
        case FD_CONNECT:
            type = 1;
            break;
        case FD_OOB:
            type = 2;
            break;
        case FD_CLOSE:
            type = 3;
            break;
        }
        if (type >= 0) {
            Q_ASSERT(d != nullptr);
            QSNDict *sn_vec[4] = { &d->sn_read, &d->sn_write, &d->sn_except, &d->sn_read };
            QSNDict *dict = sn_vec[type];

            QSockNot *sn = dict ? dict->value(wp) : 0;
            if (sn == nullptr) {
                d->postActivateSocketNotifiers();
            } else {
                Q_ASSERT(d->active_fd.contains(sn->fd));
                QSockFd &sd = d->active_fd[sn->fd];
                if (sd.selected) {
                    Q_ASSERT(sd.mask == 0);
                    d->doWsaAsyncSelect(sn->fd, 0);
                    sd.selected = false;
                }
                d->postActivateSocketNotifiers();

                // Ignore the message if a notification with the same type was
                // received previously. Suppressed message is definitely spurious.
                const long eventCode = WSAGETSELECTEVENT(lp);
                if ((sd.mask & eventCode) != eventCode) {
                    sd.mask |= eventCode;
                    QEvent event(type < 3 ? QEvent::SockAct : QEvent::SockClose);
                    QCoreApplication::sendEvent(sn->obj, &event);
                }
            }
        }
        return 0;
    }
    case WM_QT_ACTIVATENOTIFIERS: {
        Q_ASSERT(d != nullptr);

        // Postpone activation if we have unhandled socket notifier messages
        // in the queue. WM_QT_ACTIVATENOTIFIERS will be posted again as a result of
        // event processing.
        MSG msg;
        if (!PeekMessage(&msg, d->internalHwnd,
                         WM_QT_SOCKETNOTIFIER, WM_QT_SOCKETNOTIFIER, PM_NOREMOVE)
            && d->queuedSocketEvents.isEmpty()) {
            // register all socket notifiers
            for (QSFDict::iterator it = d->active_fd.begin(), end = d->active_fd.end();
                 it != end; ++it) {
                QSockFd &sd = it.value();
                if (!sd.selected) {
                    d->doWsaAsyncSelect(it.key(), sd.event);
                    // allow any event to be accepted
                    sd.mask = 0;
                    sd.selected = true;
                }
            }
        }
        d->activateNotifiersPosted = false;
        return 0;
    }
    case WM_TIMER:
        Q_ASSERT(d != nullptr);

        if (wp == d->sendPostedEventsTimerId)
            q->sendPostedEvents();
        else
            d->sendTimerEvent(wp);
        return 0;
    case WM_QT_SENDPOSTEDEVENTS:
        Q_ASSERT(d != nullptr);

        // We send posted events manually, if the window procedure was invoked
        // by the foreign event loop (e.g. from the native modal dialog).
        // Skip sending, if the message queue is not empty.
        // sendPostedEventsTimer will deliver posted events later.
        static const UINT mask = QS_ALLEVENTS;
        if (HIWORD(GetQueueStatus(mask)) == 0)
            q->sendPostedEvents();
        else
            d->startPostedEventsTimer();
        return 0;
    } // switch (message)

    return DefWindowProc(hwnd, message, wp, lp);
}

void QEventDispatcherWin32Private::startPostedEventsTimer()
{
    // we received WM_QT_SENDPOSTEDEVENTS, so allow posting it again
    wakeUps.storeRelaxed(0);
    if (sendPostedEventsTimerId == 0) {
        // Start a timer to deliver posted events when the message queue is emptied.
        sendPostedEventsTimerId = SetTimer(internalHwnd, SendPostedEventsTimerId,
                                           USER_TIMER_MINIMUM, NULL);
    }
}

// Provide class name and atom for the message window used by
// QEventDispatcherWin32Private via Q_GLOBAL_STATIC shared between threads.
struct QWindowsMessageWindowClassContext
{
    QWindowsMessageWindowClassContext();
    ~QWindowsMessageWindowClassContext();

    ATOM atom;
    wchar_t *className;
};

QWindowsMessageWindowClassContext::QWindowsMessageWindowClassContext()
    : atom(0), className(0)
{
    // make sure that multiple Qt's can coexist in the same process
    const QString qClassName = QStringLiteral("QEventDispatcherWin32_Internal_Widget")
        + QString::number(quintptr(qt_internal_proc));
    className = new wchar_t[qClassName.size() + 1];
    qClassName.toWCharArray(className);
    className[qClassName.size()] = 0;

    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = qt_internal_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(0);
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = 0;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = className;
    atom = RegisterClass(&wc);
    if (!atom) {
        qErrnoWarning("%ls RegisterClass() failed", qUtf16Printable(qClassName));
        delete [] className;
        className = 0;
    }
}

QWindowsMessageWindowClassContext::~QWindowsMessageWindowClassContext()
{
    if (className) {
        UnregisterClass(className, GetModuleHandle(0));
        delete [] className;
    }
}

Q_GLOBAL_STATIC(QWindowsMessageWindowClassContext, qWindowsMessageWindowClassContext)

static HWND qt_create_internal_window(const QEventDispatcherWin32 *eventDispatcher)
{
    QWindowsMessageWindowClassContext *ctx = qWindowsMessageWindowClassContext();
    if (!ctx->atom)
        return 0;
    HWND wnd = CreateWindow(ctx->className,    // classname
                            ctx->className,    // window name
                            0,                 // style
                            0, 0, 0, 0,        // geometry
                            HWND_MESSAGE,            // parent
                            0,                 // menu handle
                            GetModuleHandle(0),     // application
                            0);                // windows creation data.

    if (!wnd) {
        qErrnoWarning("CreateWindow() for QEventDispatcherWin32 internal window failed");
        return 0;
    }

    SetWindowLongPtr(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(eventDispatcher));

    return wnd;
}

static ULONG calculateNextTimeout(WinTimerInfo *t, quint64 currentTime)
{
    uint interval = t->interval;
    ULONG tolerance = TIMERV_DEFAULT_COALESCING;
    switch (t->timerType) {
    case Qt::PreciseTimer:
        // high precision timer is based on millisecond precision
        // so no adjustment is necessary
        break;

    case Qt::CoarseTimer:
        // this timer has up to 5% coarseness
        // so our boundaries are 20 ms and 20 s
        // below 20 ms, 5% inaccuracy is below 1 ms, so we convert to high precision
        // above 20 s, 5% inaccuracy is above 1 s, so we convert to VeryCoarseTimer
        if (interval >= 20000) {
            t->timerType = Qt::VeryCoarseTimer;
        } else if (interval <= 20) {
            // no adjustment necessary
            t->timerType = Qt::PreciseTimer;
            break;
        } else {
            tolerance = interval / 20;
            break;
        }
        Q_FALLTHROUGH();
    case Qt::VeryCoarseTimer:
        // the very coarse timer is based on full second precision,
        // so we round to closest second (but never to zero)
        tolerance = 1000;
        if (interval < 1000)
            interval = 1000;
        else
            interval = (interval + 500) / 1000 * 1000;
        currentTime = currentTime / 1000 * 1000;
        break;
    }

    t->interval = interval;
    t->timeout = currentTime + interval;
    return tolerance;
}

void QEventDispatcherWin32Private::registerTimer(WinTimerInfo *t)
{
    Q_ASSERT(internalHwnd);

    Q_Q(QEventDispatcherWin32);

    bool ok = false;
    ULONG tolerance = calculateNextTimeout(t, qt_msectime());
    uint interval = t->interval;
    if (interval == 0u) {
        // optimization for single-shot-zero-timer
        QCoreApplication::postEvent(q, new QZeroTimerEvent(t->timerId));
        ok = true;
    } else if (tolerance == TIMERV_DEFAULT_COALESCING) {
        // 3/2016: Although MSDN states timeSetEvent() is deprecated, the function
        // is still deemed to be the most reliable precision timer.
        t->fastTimerId = timeSetEvent(interval, 1, qt_fast_timer_proc, DWORD_PTR(t),
                                      TIME_CALLBACK_FUNCTION | TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
        ok = t->fastTimerId;
    }

    if (!ok) {
        // user normal timers for (Very)CoarseTimers, or if no more multimedia timers available
        ok = SetCoalescableTimer(internalHwnd, t->timerId, interval, nullptr, tolerance);
    }
    if (!ok)
        ok = SetTimer(internalHwnd, t->timerId, interval, nullptr);

    if (!ok)
        qErrnoWarning("QEventDispatcherWin32::registerTimer: Failed to create a timer");
}

void QEventDispatcherWin32Private::unregisterTimer(WinTimerInfo *t)
{
    if (t->interval == 0) {
        QCoreApplicationPrivate::removePostedTimerEvent(t->dispatcher, t->timerId);
    } else if (t->fastTimerId != 0) {
        timeKillEvent(t->fastTimerId);
        QCoreApplicationPrivate::removePostedTimerEvent(t->dispatcher, t->timerId);
    } else {
        KillTimer(internalHwnd, t->timerId);
    }
    t->timerId = -1;
    if (!t->inTimerEvent)
        delete t;
}

void QEventDispatcherWin32Private::sendTimerEvent(int timerId)
{
    WinTimerInfo *t = timerDict.value(timerId);
    if (t && !t->inTimerEvent) {
        // send event, but don't allow it to recurse
        t->inTimerEvent = true;

        // recalculate next emission
        calculateNextTimeout(t, qt_msectime());

        QTimerEvent e(t->timerId);
        QCoreApplication::sendEvent(t->obj, &e);

        // timer could have been removed
        if (t->timerId == -1) {
            delete t;
        } else {
            t->inTimerEvent = false;
        }
    }
}

void QEventDispatcherWin32Private::doWsaAsyncSelect(int socket, long event)
{
    Q_ASSERT(internalHwnd);
    // BoundsChecker may emit a warning for WSAAsyncSelect when event == 0
    // This is a BoundsChecker bug and not a Qt bug
    WSAAsyncSelect(socket, internalHwnd, event ? int(WM_QT_SOCKETNOTIFIER) : 0, event);
}

void QEventDispatcherWin32Private::postActivateSocketNotifiers()
{
    if (!activateNotifiersPosted)
        activateNotifiersPosted = PostMessage(internalHwnd, WM_QT_ACTIVATENOTIFIERS, 0, 0);
}

QEventDispatcherWin32::QEventDispatcherWin32(QObject *parent)
    : QEventDispatcherWin32(*new QEventDispatcherWin32Private, parent)
{
}

QEventDispatcherWin32::QEventDispatcherWin32(QEventDispatcherWin32Private &dd, QObject *parent)
    : QAbstractEventDispatcher(dd, parent)
{
    Q_D(QEventDispatcherWin32);

    d->internalHwnd = qt_create_internal_window(this);
}

QEventDispatcherWin32::~QEventDispatcherWin32()
{
}

static bool isUserInputMessage(UINT message)
{
    return (message >= WM_KEYFIRST && message <= WM_KEYLAST)
        || (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
        || message == WM_MOUSEWHEEL
        || message == WM_MOUSEHWHEEL
        || message == WM_TOUCH
#ifndef QT_NO_GESTURES
        || message == WM_GESTURE
        || message == WM_GESTURENOTIFY
#endif
// Pointer input: Exclude WM_NCPOINTERUPDATE .. WM_POINTERROUTEDRELEASED
        || (message >= 0x0241 && message <= 0x0253)
        || message == WM_CLOSE;
}

bool QEventDispatcherWin32::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QEventDispatcherWin32);

    // We don't know _when_ the interrupt occurred so we have to honor it.
    const bool wasInterrupted = d->interrupt.fetchAndStoreRelaxed(false);
    emit awake();

    // To prevent livelocks, send posted events once per iteration.
    // QCoreApplication::sendPostedEvents() takes care about recursions.
    sendPostedEvents();

    if (wasInterrupted)
        return false;

    auto threadData = d->threadData.loadRelaxed();
    bool canWait;
    bool retVal = false;
    do {
        QVarLengthArray<MSG> processedTimers;
        while (!d->interrupt.loadRelaxed()) {
            MSG msg;

            if (!(flags & QEventLoop::ExcludeUserInputEvents) && !d->queuedUserInputEvents.isEmpty()) {
                // process queued user input events
                msg = d->queuedUserInputEvents.takeFirst();
            } else if (!(flags & QEventLoop::ExcludeSocketNotifiers) && !d->queuedSocketEvents.isEmpty()) {
                // process queued socket events
                msg = d->queuedSocketEvents.takeFirst();
            } else if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
                if (flags.testFlag(QEventLoop::ExcludeUserInputEvents)
                    && isUserInputMessage(msg.message)) {
                    // queue user input events for later processing
                    d->queuedUserInputEvents.append(msg);
                    continue;
                }
                if ((flags & QEventLoop::ExcludeSocketNotifiers)
                    && (msg.message == WM_QT_SOCKETNOTIFIER && msg.hwnd == d->internalHwnd)) {
                    // queue socket events for later processing
                    d->queuedSocketEvents.append(msg);
                    continue;
                }
            } else if (MsgWaitForMultipleObjectsEx(0, NULL, 0, QS_ALLINPUT, MWMO_ALERTABLE)
                       == WAIT_OBJECT_0) {
                // a new message has arrived, process it
                continue;
            } else {
                // nothing to do, so break
                break;
            }

            if (d->internalHwnd == msg.hwnd && msg.message == WM_QT_SENDPOSTEDEVENTS) {
                d->startPostedEventsTimer();
                // Set result to 'true' because the message was sent by wakeUp().
                retVal = true;
                continue;
            }
            if (msg.message == WM_TIMER) {
                // Skip timer event intended for use inside foreign loop.
                if (d->internalHwnd == msg.hwnd && msg.wParam == d->sendPostedEventsTimerId)
                    continue;

                // avoid live-lock by keeping track of the timers we've already sent
                bool found = false;
                for (int i = 0; !found && i < processedTimers.count(); ++i) {
                    const MSG processed = processedTimers.constData()[i];
                    found = (processed.wParam == msg.wParam && processed.hwnd == msg.hwnd && processed.lParam == msg.lParam);
                }
                if (found)
                    continue;
                processedTimers.append(msg);
            } else if (msg.message == WM_QUIT) {
                if (QCoreApplication::instance())
                    QCoreApplication::instance()->quit();
                return false;
            }

            if (!filterNativeEvent(QByteArrayLiteral("windows_generic_MSG"), &msg, 0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            retVal = true;
        }

        // wait for message
        canWait = (!retVal
                   && !d->interrupt.loadRelaxed()
                   && flags.testFlag(QEventLoop::WaitForMoreEvents)
                   && threadData->canWaitLocked());
        if (canWait) {
            emit aboutToBlock();
            MsgWaitForMultipleObjectsEx(0, NULL, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
            emit awake();
        }
    } while (canWait);

    return retVal;
}

void QEventDispatcherWin32::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
    int sockfd = notifier->socket();
    int type = notifier->type();
#ifndef QT_NO_DEBUG
    if (sockfd < 0) {
        qWarning("QEventDispatcherWin32::registerSocketNotifier: invalid socket identifier");
        return;
    }
    if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWin32: socket notifiers cannot be enabled from another thread");
        return;
    }
#endif

    Q_D(QEventDispatcherWin32);
    QSNDict *sn_vec[3] = { &d->sn_read, &d->sn_write, &d->sn_except };
    QSNDict *dict = sn_vec[type];

    if (QCoreApplication::closingDown()) // ### d->exitloop?
        return; // after sn_cleanup, don't reinitialize.

    if (dict->contains(sockfd)) {
        const char *t[] = { "Read", "Write", "Exception" };
    /* Variable "socket" below is a function pointer. */
        qWarning("QSocketNotifier: Multiple socket notifiers for "
                 "same socket %d and type %s", sockfd, t[type]);
    }

    QSockNot *sn = new QSockNot;
    sn->obj = notifier;
    sn->fd  = sockfd;
    dict->insert(sn->fd, sn);

    long event = 0;
    if (d->sn_read.contains(sockfd))
        event |= FD_READ | FD_CLOSE | FD_ACCEPT;
    if (d->sn_write.contains(sockfd))
        event |= FD_WRITE | FD_CONNECT;
    if (d->sn_except.contains(sockfd))
        event |= FD_OOB;

    QSFDict::iterator it = d->active_fd.find(sockfd);
    if (it != d->active_fd.end()) {
        QSockFd &sd = it.value();
        if (sd.selected) {
            d->doWsaAsyncSelect(sockfd, 0);
            sd.selected = false;
        }
        sd.event |= event;
    } else {
        // Although WSAAsyncSelect(..., 0), which is called from
        // unregisterSocketNotifier(), immediately disables event message
        // posting for the socket, it is possible that messages could be
        // waiting in the application message queue even if the socket was
        // closed. Also, some events could be implicitly re-enabled due
        // to system calls. Ignore these superfluous events until all
        // pending notifications have been suppressed. Next activation of
        // socket notifiers will reset the mask.
        d->active_fd.insert(sockfd, QSockFd(event, FD_READ | FD_CLOSE | FD_ACCEPT | FD_WRITE
                                                   | FD_CONNECT | FD_OOB));
    }

    d->postActivateSocketNotifiers();
}

void QEventDispatcherWin32::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
#ifndef QT_NO_DEBUG
    int sockfd = notifier->socket();
    if (sockfd < 0) {
        qWarning("QEventDispatcherWin32::unregisterSocketNotifier: invalid socket identifier");
        return;
    }
    if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWin32: socket notifiers cannot be disabled from another thread");
        return;
    }
#endif
    doUnregisterSocketNotifier(notifier);
}

void QEventDispatcherWin32::doUnregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_D(QEventDispatcherWin32);
    int type = notifier->type();
    int sockfd = notifier->socket();
    Q_ASSERT(sockfd >= 0);

    QSFDict::iterator it = d->active_fd.find(sockfd);
    if (it != d->active_fd.end()) {
        QSockFd &sd = it.value();
        if (sd.selected)
            d->doWsaAsyncSelect(sockfd, 0);
        const long event[3] = { FD_READ | FD_CLOSE | FD_ACCEPT, FD_WRITE | FD_CONNECT, FD_OOB };
        sd.event ^= event[type];
        if (sd.event == 0) {
            d->active_fd.erase(it);
        } else if (sd.selected) {
            sd.selected = false;
            d->postActivateSocketNotifiers();
        }
    }

    QSNDict *sn_vec[3] = { &d->sn_read, &d->sn_write, &d->sn_except };
    QSNDict *dict = sn_vec[type];
    QSockNot *sn = dict->value(sockfd);
    if (!sn)
        return;

    dict->remove(sockfd);
    delete sn;
}

void QEventDispatcherWin32::registerTimer(int timerId, qint64 interval, Qt::TimerType timerType, QObject *object)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1 || interval < 0 || !object) {
        qWarning("QEventDispatcherWin32::registerTimer: invalid arguments");
        return;
    }
    if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWin32::registerTimer: timers cannot be started from another thread");
        return;
    }
#endif

    Q_D(QEventDispatcherWin32);

    // exiting ... do not register new timers
    // (QCoreApplication::closingDown() is set too late to be used here)
    if (d->closingDown)
        return;

    WinTimerInfo *t = new WinTimerInfo;
    t->dispatcher = this;
    t->timerId  = timerId;
    t->interval = interval;
    t->timerType = timerType;
    t->obj  = object;
    t->inTimerEvent = false;
    t->fastTimerId = 0;

    d->registerTimer(t);

    d->timerDict.insert(t->timerId, t);          // store timers in dict
}

bool QEventDispatcherWin32::unregisterTimer(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherWin32::unregisterTimer: invalid argument");
        return false;
    }
    if (thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWin32::unregisterTimer: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QEventDispatcherWin32);

    WinTimerInfo *t = d->timerDict.take(timerId);
    if (!t)
        return false;

    d->unregisterTimer(t);
    return true;
}

bool QEventDispatcherWin32::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWin32::unregisterTimers: invalid argument");
        return false;
    }
    if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWin32::unregisterTimers: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QEventDispatcherWin32);
    if (d->timerDict.isEmpty())
        return false;

    auto it = d->timerDict.begin();
    while (it != d->timerDict.end()) {
        WinTimerInfo *t = it.value();
        Q_ASSERT(t);
        if (t->obj == object) {
            it = d->timerDict.erase(it);
            d->unregisterTimer(t);
        } else {
            ++it;
        }
    }
    return true;
}

QList<QEventDispatcherWin32::TimerInfo>
QEventDispatcherWin32::registeredTimers(QObject *object) const
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWin32:registeredTimers: invalid argument");
        return QList<TimerInfo>();
    }
#endif

    Q_D(const QEventDispatcherWin32);
    QList<TimerInfo> list;
    for (WinTimerInfo *t : std::as_const(d->timerDict)) {
        Q_ASSERT(t);
        if (t->obj == object)
            list << TimerInfo(t->timerId, t->interval, t->timerType);
    }
    return list;
}

int QEventDispatcherWin32::remainingTime(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherWin32::remainingTime: invalid argument");
        return -1;
    }
#endif

    Q_D(QEventDispatcherWin32);

    quint64 currentTime = qt_msectime();

    WinTimerInfo *t = d->timerDict.value(timerId);
    if (t) {
        // timer found, return time to wait
        return t->timeout > currentTime ? t->timeout - currentTime : 0;
    }

#ifndef QT_NO_DEBUG
    qWarning("QEventDispatcherWin32::remainingTime: timer id %d not found", timerId);
#endif

    return -1;
}

void QEventDispatcherWin32::wakeUp()
{
    Q_D(QEventDispatcherWin32);
    if (d->wakeUps.testAndSetRelaxed(0, 1)) {
        // post a WM_QT_SENDPOSTEDEVENTS to this thread if there isn't one already pending
        if (!PostMessage(d->internalHwnd, WM_QT_SENDPOSTEDEVENTS, 0, 0))
            qErrnoWarning("QEventDispatcherWin32::wakeUp: Failed to post a message");
    }
}

void QEventDispatcherWin32::interrupt()
{
    Q_D(QEventDispatcherWin32);
    d->interrupt.storeRelaxed(true);
    wakeUp();
}

void QEventDispatcherWin32::startingUp()
{ }

void QEventDispatcherWin32::closingDown()
{
    Q_D(QEventDispatcherWin32);

    // clean up any socketnotifiers
    while (!d->sn_read.isEmpty())
        doUnregisterSocketNotifier((*(d->sn_read.begin()))->obj);
    while (!d->sn_write.isEmpty())
        doUnregisterSocketNotifier((*(d->sn_write.begin()))->obj);
    while (!d->sn_except.isEmpty())
        doUnregisterSocketNotifier((*(d->sn_except.begin()))->obj);
    Q_ASSERT(d->active_fd.isEmpty());

    // clean up any timers
    for (WinTimerInfo *t : std::as_const(d->timerDict))
        d->unregisterTimer(t);
    d->timerDict.clear();

    d->closingDown = true;

    if (d->sendPostedEventsTimerId != 0)
        KillTimer(d->internalHwnd, d->sendPostedEventsTimerId);
    d->sendPostedEventsTimerId = 0;
}

bool QEventDispatcherWin32::event(QEvent *e)
{
    Q_D(QEventDispatcherWin32);
    switch (e->type()) {
    case QEvent::ZeroTimerEvent: {
        QZeroTimerEvent *zte = static_cast<QZeroTimerEvent*>(e);
        WinTimerInfo *t = d->timerDict.value(zte->timerId());
        if (t) {
            t->inTimerEvent = true;

            QTimerEvent te(zte->timerId());
            QCoreApplication::sendEvent(t->obj, &te);

            // timer could have been removed
            if (t->timerId == -1) {
                delete t;
            } else {
                if (t->interval == 0 && t->inTimerEvent) {
                    // post the next zero timer event as long as the timer was not restarted
                    QCoreApplication::postEvent(this, new QZeroTimerEvent(zte->timerId()));
                }

                t->inTimerEvent = false;
            }
        }
        return true;
    }
    case QEvent::Timer:
        d->sendTimerEvent(static_cast<const QTimerEvent*>(e)->timerId());
        break;
    default:
        break;
    }
    return QAbstractEventDispatcher::event(e);
}

void QEventDispatcherWin32::sendPostedEvents()
{
    Q_D(QEventDispatcherWin32);

    if (d->sendPostedEventsTimerId != 0)
        KillTimer(d->internalHwnd, d->sendPostedEventsTimerId);
    d->sendPostedEventsTimerId = 0;

    // Allow posting WM_QT_SENDPOSTEDEVENTS message.
    d->wakeUps.storeRelaxed(0);

    QCoreApplicationPrivate::sendPostedEvents(0, 0, d->threadData.loadRelaxed());
}

HWND QEventDispatcherWin32::internalHwnd()
{
    return d_func()->internalHwnd;
}

QT_END_NAMESPACE
