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

#include "qeventdispatcher_win_p.h"

#include "qcoreapplication.h"
#include <private/qsystemlibrary_p.h>
#include "qoperatingsystemversion.h"
#include "qpair.h"
#include "qset.h"
#include "qsocketnotifier.h"
#include "qvarlengtharray.h"
#include "qwineventnotifier.h"

#include "qelapsedtimer.h"
#include "qcoreapplication_p.h"
#include <private/qthread_p.h>
#include <private/qwineventnotifier_p.h>

QT_BEGIN_NAMESPACE

extern uint qGlobalPostedEventsCount();

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

QEventDispatcherWin32Private::QEventDispatcherWin32Private()
    : threadId(GetCurrentThreadId()), interrupt(false), internalHwnd(0),
      getMessageHook(0), sendPostedEventsTimerId(0), wakeUps(0),
      activateNotifiersPosted(false), winEventNotifierActivatedEvent(NULL)
{
}

QEventDispatcherWin32Private::~QEventDispatcherWin32Private()
{
    if (winEventNotifierActivatedEvent)
        CloseHandle(winEventNotifierActivatedEvent);
    if (internalHwnd)
        DestroyWindow(internalHwnd);
}

void QEventDispatcherWin32Private::activateEventNotifier(QWinEventNotifier * wen)
{
    QEvent event(QEvent::WinEventAct);
    QCoreApplication::sendEvent(wen, &event);
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

static inline UINT inputQueueMask()
{
    UINT result = QS_ALLEVENTS;
    // QTBUG 28513, QTBUG-29097, QTBUG-29435: QS_TOUCH, QS_POINTER became part of
    // QS_INPUT in Windows Kit 8. They should not be used when running on pre-Windows 8.
#if WINVER > 0x0601
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows8)
        result &= ~(QS_TOUCH | QS_POINTER);
#endif //  WINVER > 0x0601
    return result;
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qintptr result;
#else
    long result;
#endif
    if (!dispatcher) {
        if (message == WM_TIMER)
            KillTimer(hwnd, wp);
        return 0;
    }
    if (dispatcher->filterNativeEvent(QByteArrayLiteral("windows_dispatcher_MSG"), &msg, &result))
        return result;

#ifdef GWLP_USERDATA
    auto q = reinterpret_cast<QEventDispatcherWin32 *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
#else
    auto q = reinterpret_cast<QEventDispatcherWin32 *>(GetWindowLong(hwnd, GWL_USERDATA));
#endif
    QEventDispatcherWin32Private *d = 0;
    if (q != 0)
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
            Q_ASSERT(d != 0);
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
        Q_ASSERT(d != 0);

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
        Q_ASSERT(d != 0);

        if (wp == d->sendPostedEventsTimerId)
            q->sendPostedEvents();
        else
            d->sendTimerEvent(wp);
        return 0;
    case WM_QT_SENDPOSTEDEVENTS:
        Q_ASSERT(d != 0);

        // We send posted events manually, if the window procedure was invoked
        // by the foreign event loop (e.g. from the native modal dialog).
        // Skip sending, if the message queue is not empty.
        // sendPostedEventsTimer will deliver posted events later.
        static const UINT mask = inputQueueMask();
        if (HIWORD(GetQueueStatus(mask)) == 0)
            q->sendPostedEvents();
        return 0;
    } // switch (message)

    return DefWindowProc(hwnd, message, wp, lp);
}

LRESULT QT_WIN_CALLBACK qt_GetMessageHook(int code, WPARAM wp, LPARAM lp)
{
    QEventDispatcherWin32 *q = qobject_cast<QEventDispatcherWin32 *>(QAbstractEventDispatcher::instance());
    Q_ASSERT(q != 0);
    QEventDispatcherWin32Private *d = q->d_func();
    MSG *msg = reinterpret_cast<MSG *>(lp);
    // Windows unexpectedly passes PM_NOYIELD flag to the hook procedure,
    // if ::PeekMessage(..., PM_REMOVE | PM_NOYIELD) is called from the event loop.
    // So, retrieve 'removed' tag as a bit field.
    const bool messageRemoved = (wp & PM_REMOVE) != 0;

    if (msg->hwnd == d->internalHwnd && msg->message == WM_QT_SENDPOSTEDEVENTS
        && messageRemoved && d->sendPostedEventsTimerId == 0) {
        // Start a timer to deliver posted events when the message queue is emptied.
        d->sendPostedEventsTimerId = SetTimer(d->internalHwnd, SendPostedEventsTimerId,
                                              USER_TIMER_MINIMUM, NULL);
    }
    return d->getMessageHook ? CallNextHookEx(0, code, wp, lp) : 0;
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

#ifdef GWLP_USERDATA
    SetWindowLongPtr(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(eventDispatcher));
#else
    SetWindowLong(wnd, GWL_USERDATA, reinterpret_cast<LONG>(eventDispatcher));
#endif

    return wnd;
}

static void calculateNextTimeout(WinTimerInfo *t, quint64 currentTime)
{
    uint interval = t->interval;
    if ((interval >= 20000u && t->timerType != Qt::PreciseTimer) || t->timerType == Qt::VeryCoarseTimer) {
        // round the interval, VeryCoarseTimers only have full second accuracy
        interval = ((interval + 500)) / 1000 * 1000;
    }
    t->interval = interval;
    t->timeout = currentTime + interval;
}

void QEventDispatcherWin32Private::registerTimer(WinTimerInfo *t)
{
    Q_ASSERT(internalHwnd);

    Q_Q(QEventDispatcherWin32);

    bool ok = false;
    calculateNextTimeout(t, qt_msectime());
    uint interval = t->interval;
    if (interval == 0u) {
        // optimization for single-shot-zero-timer
        QCoreApplication::postEvent(q, new QZeroTimerEvent(t->timerId));
        ok = true;
    } else if (interval < 20u || t->timerType == Qt::PreciseTimer) {
        // 3/2016: Although MSDN states timeSetEvent() is deprecated, the function
        // is still deemed to be the most reliable precision timer.
        t->fastTimerId = timeSetEvent(interval, 1, qt_fast_timer_proc, DWORD_PTR(t),
                                      TIME_CALLBACK_FUNCTION | TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
        ok = t->fastTimerId;
    }

    if (!ok) {
        // user normal timers for (Very)CoarseTimers, or if no more multimedia timers available
        ok = SetTimer(internalHwnd, t->timerId, interval, 0);
    }

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
    } else if (internalHwnd) {
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

void QEventDispatcherWin32::createInternalHwnd()
{
    Q_D(QEventDispatcherWin32);

    if (d->internalHwnd)
        return;
    d->internalHwnd = qt_create_internal_window(this);

    // setup GetMessage hook needed to drive our posted events
    d->getMessageHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC) qt_GetMessageHook, NULL, GetCurrentThreadId());
    if (Q_UNLIKELY(!d->getMessageHook)) {
        int errorCode = GetLastError();
        qFatal("Qt: INTERNAL ERROR: failed to install GetMessage hook: %d, %ls",
               errorCode, qUtf16Printable(qt_error_string(errorCode)));
    }

    // start all normal timers
    for (int i = 0; i < d->timerVec.count(); ++i)
        d->registerTimer(d->timerVec.at(i));
}

QEventDispatcherWin32::QEventDispatcherWin32(QObject *parent)
    : QAbstractEventDispatcher(*new QEventDispatcherWin32Private, parent)
{
}

QEventDispatcherWin32::QEventDispatcherWin32(QEventDispatcherWin32Private &dd, QObject *parent)
    : QAbstractEventDispatcher(dd, parent)
{ }

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

    if (!d->internalHwnd) {
        createInternalHwnd();
        wakeUp(); // trigger a call to sendPostedEvents()
    }

    d->interrupt.storeRelaxed(false);
    emit awake();

    // To prevent livelocks, send posted events once per iteration.
    // QCoreApplication::sendPostedEvents() takes care about recursions.
    sendPostedEvents();

    auto threadData = d->threadData.loadRelaxed();
    bool canWait;
    bool retVal = false;
    do {
        DWORD waitRet = 0;
        DWORD nCount = 0;
        HANDLE *pHandles = nullptr;
        if (d->winEventNotifierActivatedEvent) {
            nCount = 1;
            pHandles = &d->winEventNotifierActivatedEvent;
        }
        QVarLengthArray<MSG> processedTimers;
        while (!d->interrupt.loadRelaxed()) {
            MSG msg;
            bool haveMessage;

            if (!(flags & QEventLoop::ExcludeUserInputEvents) && !d->queuedUserInputEvents.isEmpty()) {
                // process queued user input events
                haveMessage = true;
                msg = d->queuedUserInputEvents.takeFirst();
            } else if(!(flags & QEventLoop::ExcludeSocketNotifiers) && !d->queuedSocketEvents.isEmpty()) {
                // process queued socket events
                haveMessage = true;
                msg = d->queuedSocketEvents.takeFirst();
            } else {
                haveMessage = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
                if (haveMessage) {
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
                }
            }
            if (!haveMessage) {
                // no message - check for signalled objects
                waitRet = MsgWaitForMultipleObjectsEx(nCount, pHandles, 0, QS_ALLINPUT, MWMO_ALERTABLE);
                if ((haveMessage = (waitRet == WAIT_OBJECT_0 + nCount))) {
                    // a new message has arrived, process it
                    continue;
                }
            }
            if (haveMessage) {
                if (d->internalHwnd == msg.hwnd && msg.message == WM_QT_SENDPOSTEDEVENTS) {
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
            } else if (waitRet - WAIT_OBJECT_0 < nCount) {
                activateEventNotifiers();
            } else {
                // nothing todo so break
                break;
            }
            retVal = true;
        }

        // still nothing - wait for message or signalled objects
        canWait = (!retVal
                   && !d->interrupt.loadRelaxed()
                   && flags.testFlag(QEventLoop::WaitForMoreEvents)
                   && threadData->canWaitLocked());
        if (canWait) {
            emit aboutToBlock();
            waitRet = MsgWaitForMultipleObjectsEx(nCount, pHandles, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
            emit awake();
            if (waitRet - WAIT_OBJECT_0 < nCount) {
                activateEventNotifiers();
                retVal = true;
            }
        }
    } while (canWait);

    return retVal;
}

bool QEventDispatcherWin32::hasPendingEvents()
{
    MSG msg;
    return qGlobalPostedEventsCount() || PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
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

    createInternalHwnd();

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
        // Disable the events which could be implicitly re-enabled. Next activation
        // of socket notifiers will reset the mask.
        d->active_fd.insert(sockfd, QSockFd(event, FD_READ | FD_ACCEPT | FD_WRITE | FD_OOB));
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

void QEventDispatcherWin32::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
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

    if (d->internalHwnd)
        d->registerTimer(t);

    d->timerVec.append(t);                      // store in timer vector
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
    if (d->timerVec.isEmpty() || timerId <= 0)
        return false;

    WinTimerInfo *t = d->timerDict.value(timerId);
    if (!t)
        return false;

    d->timerDict.remove(t->timerId);
    d->timerVec.removeAll(t);
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
    if (d->timerVec.isEmpty())
        return false;
    WinTimerInfo *t;
    for (int i=0; i<d->timerVec.size(); i++) {
        t = d->timerVec.at(i);
        if (t && t->obj == object) {                // object found
            d->timerDict.remove(t->timerId);
            d->timerVec.removeAt(i);
            d->unregisterTimer(t);
            --i;
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
    for (const WinTimerInfo *t : qAsConst(d->timerVec)) {
        if (t && t->obj == object)
            list << TimerInfo(t->timerId, t->interval, t->timerType);
    }
    return list;
}

bool QEventDispatcherWin32::registerEventNotifier(QWinEventNotifier *notifier)
{
    Q_ASSERT(notifier);
#ifndef QT_NO_DEBUG
    if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWin32: event notifiers cannot be enabled from another thread");
        return false;
    }
#endif

    Q_D(QEventDispatcherWin32);

    if (d->winEventNotifierList.contains(notifier))
        return true;

    d->winEventNotifierList.append(notifier);
    d->winEventNotifierListModified = true;

    if (!d->winEventNotifierActivatedEvent)
        d->winEventNotifierActivatedEvent = CreateEvent(0, TRUE, FALSE, nullptr);

    return QWinEventNotifierPrivate::get(notifier)->registerWaitObject();
}

void QEventDispatcherWin32::unregisterEventNotifier(QWinEventNotifier *notifier)
{
    Q_ASSERT(notifier);
#ifndef QT_NO_DEBUG
    if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWin32: event notifiers cannot be disabled from another thread");
        return;
    }
#endif
    doUnregisterEventNotifier(notifier);
}

void QEventDispatcherWin32::doUnregisterEventNotifier(QWinEventNotifier *notifier)
{
    Q_D(QEventDispatcherWin32);

    int i = d->winEventNotifierList.indexOf(notifier);
    if (i == -1)
        return;
    d->winEventNotifierList.takeAt(i);
    d->winEventNotifierListModified = true;
    QWinEventNotifierPrivate *nd = QWinEventNotifierPrivate::get(notifier);
    if (nd->waitHandle)
        nd->unregisterWaitObject();
}

void QEventDispatcherWin32::activateEventNotifiers()
{
    Q_D(QEventDispatcherWin32);
    ResetEvent(d->winEventNotifierActivatedEvent);

    // Activate signaled notifiers. Our winEventNotifierList can be modified in activation slots.
    do {
        d->winEventNotifierListModified = false;
        for (int i = 0; i < d->winEventNotifierList.count(); ++i) {
            QWinEventNotifier *notifier = d->winEventNotifierList.at(i);
            QWinEventNotifierPrivate *nd = QWinEventNotifierPrivate::get(notifier);
            if (nd->signaledCount.loadRelaxed() != 0) {
                --nd->signaledCount;
                nd->unregisterWaitObject();
                d->activateEventNotifier(notifier);
            }
        }
    } while (d->winEventNotifierListModified);

    // Re-register the remaining activated notifiers.
    for (int i = 0; i < d->winEventNotifierList.count(); ++i) {
        QWinEventNotifier *notifier = d->winEventNotifierList.at(i);
        QWinEventNotifierPrivate *nd = QWinEventNotifierPrivate::get(notifier);
        if (!nd->waitHandle)
            nd->registerWaitObject();
    }
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

    if (d->timerVec.isEmpty())
        return -1;

    quint64 currentTime = qt_msectime();

    for (const WinTimerInfo *t : qAsConst(d->timerVec)) {
        if (t && t->timerId == timerId) {
            // timer found, return time to wait

            if (d->internalHwnd)
                return t->timeout > currentTime ? t->timeout - currentTime : 0;
            else
                return t->interval;
        }
    }

#ifndef QT_NO_DEBUG
    qWarning("QEventDispatcherWin32::remainingTime: timer id %d not found", timerId);
#endif

    return -1;
}

void QEventDispatcherWin32::wakeUp()
{
    Q_D(QEventDispatcherWin32);
    if (d->internalHwnd && d->wakeUps.testAndSetAcquire(0, 1)) {
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

void QEventDispatcherWin32::flush()
{ }

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

    // clean up any eventnotifiers
    while (!d->winEventNotifierList.isEmpty())
        doUnregisterEventNotifier(d->winEventNotifierList.first());

    // clean up any timers
    for (int i = 0; i < d->timerVec.count(); ++i)
        d->unregisterTimer(d->timerVec.at(i));
    d->timerVec.clear();
    d->timerDict.clear();

    d->closingDown = true;

    if (d->getMessageHook)
        UnhookWindowsHookEx(d->getMessageHook);
    d->getMessageHook = 0;

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
    Q_D(QEventDispatcherWin32);
    createInternalHwnd();
    return d->internalHwnd;
}

QT_END_NAMESPACE
