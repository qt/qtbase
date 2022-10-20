/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qeventdispatcher_wasm_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qthread.h>

#include "emscripten.h"
#include <emscripten/html5.h>
#include <emscripten/threading.h>

QT_BEGIN_NAMESPACE

// using namespace emscripten;
extern int qGlobalPostedEventsCount(); // from qapplication.cpp

Q_LOGGING_CATEGORY(lcEventDispatcher, "qt.eventdispatcher");
Q_LOGGING_CATEGORY(lcEventDispatcherTimers, "qt.eventdispatcher.timers");

#ifdef QT_HAVE_EMSCRIPTEN_ASYNCIFY

// Emscripten asyncify currently supports one level of suspend -
// recursion is not permitted. We track the suspend state here
// on order to fail (more) gracefully, but we can of course only
// track Qts own usage of asyncify.
static bool g_is_asyncify_suspended = false;

EM_JS(void, qt_asyncify_suspend_js, (), {
    let sleepFn = (wakeUp) => {
        Module.qtAsyncifyWakeUp = wakeUp;
    };
    return Asyncify.handleSleep(sleepFn);
});

EM_JS(void, qt_asyncify_resume_js, (), {
    let wakeUp = Module.qtAsyncifyWakeUp;
    if (wakeUp == undefined)
        return;
    Module.qtAsyncifyWakeUp = undefined;

    // Delayed wakeup with zero-timer. Workaround/fix for
    // https://github.com/emscripten-core/emscripten/issues/10515
    setTimeout(wakeUp);
});

// Suspends the main thread until qt_asyncify_resume() is called. Returns
// false immediately if Qt has already suspended the main thread (recursive
// suspend is not supported by Emscripten). Returns true (after resuming),
// if the thread was suspended.
bool qt_asyncify_suspend()
{
    if (g_is_asyncify_suspended)
        return false;
    g_is_asyncify_suspended = true;
    qt_asyncify_suspend_js();
    return true;
}

// Wakes any currently suspended main thread. Returns true if the main
// thread was suspended, in which case it will now be asynchonously woken.
bool qt_asyncify_resume()
{
    if (!g_is_asyncify_suspended)
        return false;
    g_is_asyncify_suspended = false;
    qt_asyncify_resume_js();
    return true;
}

// Yields control to the browser, so that it can process events. Must
// be called on the main thread. Returns false immediately if Qt has
// already suspended the main thread. Returns true after yielding.
bool qt_asyncify_yield()
{
    if (g_is_asyncify_suspended)
        return false;
    emscripten_sleep(0);
    return true;
}

#endif // QT_HAVE_EMSCRIPTEN_ASYNCIFY

QEventDispatcherWasm *QEventDispatcherWasm::g_mainThreadEventDispatcher = nullptr;
#if QT_CONFIG(thread)
QVector<QEventDispatcherWasm *> QEventDispatcherWasm::g_secondaryThreadEventDispatchers;
std::mutex QEventDispatcherWasm::g_secondaryThreadEventDispatchersMutex;
#endif

QEventDispatcherWasm::QEventDispatcherWasm()
    : QAbstractEventDispatcher()
{
    // QEventDispatcherWasm operates in two main modes:
    // - On the main thread:
    //   The event dispatcher can process native events but can't
    //   block and wait for new events, unless asyncify is used.
    // - On a secondary thread:
    //   The event dispatcher can't process native events but can
    //   block and wait for new events.
    //
    // Which mode is determined by the calling thread: construct
    // the event dispatcher object on the thread where it will live.

    qCDebug(lcEventDispatcher) << "Creating QEventDispatcherWasm instance" << this
                               << "is main thread" << emscripten_is_main_runtime_thread();

    if (emscripten_is_main_runtime_thread()) {
        // There can be only one main thread event dispatcher at a time; in
        // addition the main instance is used by the secondary thread event
        // dispatchers so we set a global pointer to it.
        Q_ASSERT(g_mainThreadEventDispatcher == nullptr);
        g_mainThreadEventDispatcher = this;
    } else {
#if QT_CONFIG(thread)
        std::lock_guard<std::mutex> lock(g_secondaryThreadEventDispatchersMutex);
        g_secondaryThreadEventDispatchers.append(this);
#endif
    }
}

QEventDispatcherWasm::~QEventDispatcherWasm()
{
    qCDebug(lcEventDispatcher) << "Detroying QEventDispatcherWasm instance" << this;

    delete m_timerInfo;

#if QT_CONFIG(thread)
    if (isSecondaryThreadEventDispatcher()) {
        std::lock_guard<std::mutex> lock(g_secondaryThreadEventDispatchersMutex);
        g_secondaryThreadEventDispatchers.remove(g_secondaryThreadEventDispatchers.indexOf(this));
    } else
#endif
    {
        if (m_timerId > 0)
            emscripten_clear_timeout(m_timerId);
        g_mainThreadEventDispatcher = nullptr;
    }
}

bool QEventDispatcherWasm::isMainThreadEventDispatcher()
{
    return this == g_mainThreadEventDispatcher;
}

bool QEventDispatcherWasm::isSecondaryThreadEventDispatcher()
{
    return this != g_mainThreadEventDispatcher;
}

bool QEventDispatcherWasm::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    emit awake();

    bool hasPendingEvents = qGlobalPostedEventsCount() > 0;

    qCDebug(lcEventDispatcher) << "QEventDispatcherWasm::processEvents flags" << flags
                               << "pending events" << hasPendingEvents;

    if (isMainThreadEventDispatcher()) {
        if (flags & QEventLoop::DialogExec)
            handleDialogExec();
        else if (flags & QEventLoop::EventLoopExec)
            handleApplicationExec();
    }

    if (!(flags & QEventLoop::ExcludeUserInputEvents))
        pollForNativeEvents();

    hasPendingEvents = qGlobalPostedEventsCount() > 0;

    if (!hasPendingEvents && (flags & QEventLoop::WaitForMoreEvents))
        waitForForEvents();

    if (m_interrupted) {
        m_interrupted = false;
        return false;
    }

    if (m_processTimers) {
        m_processTimers = false;
        processTimers();
    }

    hasPendingEvents = qGlobalPostedEventsCount() > 0;
    QCoreApplication::sendPostedEvents();
    return hasPendingEvents;
}

void QEventDispatcherWasm::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_UNUSED(notifier);
    qWarning("QEventDispatcherWasm::registerSocketNotifier: socket notifiers are not supported");
}

void QEventDispatcherWasm::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_UNUSED(notifier);
    qWarning("QEventDispatcherWasm::unregisterSocketNotifier: socket notifiers are not supported");
}

void QEventDispatcherWasm::registerTimer(int timerId, qint64 interval, Qt::TimerType timerType, QObject *object)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1 || interval < 0 || !object) {
        qWarning("QEventDispatcherWasm::registerTimer: invalid arguments");
        return;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWasm::registerTimer: timers cannot be started from another "
                 "thread");
        return;
    }
#endif
    qCDebug(lcEventDispatcherTimers) << "registerTimer" << timerId << interval << timerType << object;

    m_timerInfo->registerTimer(timerId, interval, timerType, object);
    updateNativeTimer();
}

bool QEventDispatcherWasm::unregisterTimer(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherWasm::unregisterTimer: invalid argument");
        return false;
    } else if (thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWasm::unregisterTimer: timers cannot be stopped from another "
                 "thread");
        return false;
    }
#endif

    qCDebug(lcEventDispatcherTimers) << "unregisterTimer" << timerId;

    bool ans = m_timerInfo->unregisterTimer(timerId);
    updateNativeTimer();
    return ans;
}

bool QEventDispatcherWasm::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWasm::unregisterTimers: invalid argument");
        return false;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWasm::unregisterTimers: timers cannot be stopped from another "
                 "thread");
        return false;
    }
#endif

    qCDebug(lcEventDispatcherTimers) << "registerTimer" << object;

    bool ans = m_timerInfo->unregisterTimers(object);
    updateNativeTimer();
    return ans;
}

QList<QAbstractEventDispatcher::TimerInfo>
QEventDispatcherWasm::registeredTimers(QObject *object) const
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWasm:registeredTimers: invalid argument");
        return QList<TimerInfo>();
    }
#endif

    return m_timerInfo->registeredTimers(object);
}

int QEventDispatcherWasm::remainingTime(int timerId)
{
    return m_timerInfo->timerRemainingTime(timerId);
}

void QEventDispatcherWasm::interrupt()
{
    m_interrupted = true;
    wakeUp();
}

void QEventDispatcherWasm::wakeUp()
{
#if QT_CONFIG(thread)
    if (isSecondaryThreadEventDispatcher()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_wakeUpCalled = true;
        m_moreEvents.notify_one();
        return;
    }
#endif

#ifdef QT_HAVE_EMSCRIPTEN_ASYNCIFY
    // The main thread may be asyncify-blocked in processEvents(). If so resume it.
    if (qt_asyncify_resume()) // ### safe to call from secondary thread?
        return;
#endif

    {
#if QT_CONFIG(thread)
        // This function can be called from any thread (via wakeUp()),
        // so we need to lock access to m_pendingProcessEvents.
        std::lock_guard<std::mutex> lock(m_mutex);
#endif
        if (m_pendingProcessEvents)
            return;
        m_pendingProcessEvents = true;
    }

#if QT_CONFIG(thread)
    if (!emscripten_is_main_runtime_thread()) {
        runOnMainThread([this](){
            QEventDispatcherWasm::callProcessEvents(this);
        });
    } else
#endif
    emscripten_async_call(&QEventDispatcherWasm::callProcessEvents, this, 0);
}

void QEventDispatcherWasm::handleApplicationExec()
{
    // Start the main loop, and then stop it on the first callback. This
    // is done for the "simulateInfiniteLoop" functionality where
    // emscripten_set_main_loop() throws a JS exception which returns
    // control to the browser while preserving the C++ stack.
    //
    // Note that we don't use asyncify here: Emscripten supports one level of
    // asyncify only and we want to reserve that for dialog exec() instead of
    // using it for the one qApp exec().
    const bool simulateInfiniteLoop = true;
    emscripten_set_main_loop([](){
        emscripten_pause_main_loop();
    }, 0, simulateInfiniteLoop);
}

void QEventDispatcherWasm::handleDialogExec()
{
#ifndef QT_HAVE_EMSCRIPTEN_ASYNCIFY
    qWarning() << "Warning: dialog exec() is not supported on Qt for WebAssembly in this"
               << "configuration. Please use show() instead, or enable experimental support"
               << "for asyncify.\n"
               << "When using exec() (without asyncify) the dialog will show, the user can interact"
               << "with it and the appropriate signals will be emitted on close. However, the"
               << "exec() call never returns, stack content at the time of the exec() call"
               << "is leaked, and the exec() call may interfere with input event processing";
    emscripten_sleep(1); // This call never returns
#endif
    // For the asyncify case we do nothing here and wait for events in waitForForEvents()
}

void QEventDispatcherWasm::pollForNativeEvents()
{
    // Secondary thread event dispatchers do not support native events
    if (isSecondaryThreadEventDispatcher())
        return;

#if HAVE_EMSCRIPTEN_ASYNCIFY
    // Asyncify allows us to yield to the browser and have it process native events -
    // but this will fail if we are recursing and are already in a yield.
    bool didYield = qt_asyncify_yield();
    if (!didYield)
        qWarning("QEventDispatcherWasm::processEvents() did not asyncify process native events");
#endif
}

// Waits for more events. This is possible in two cases:
// - On a secondary thread
// - On the main thread iff asyncify is used
// Returns true if waiting was possible (at which point it
// has already happened).
bool QEventDispatcherWasm::waitForForEvents()
{
#if QT_CONFIG(thread)
    if (isSecondaryThreadEventDispatcher()) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_moreEvents.wait(lock, [=] { return m_wakeUpCalled; });
        m_wakeUpCalled = false;
        return true;
    }
#endif

    Q_ASSERT(emscripten_is_main_runtime_thread());

#ifdef QT_HAVE_EMSCRIPTEN_ASYNCIFY
        // We can block on the main thread using asyncify:
        bool didSuspend = qt_asyncify_suspend();
        if (!didSuspend)
            qWarning("QEventDispatcherWasm: current thread is already suspended; could not asyncify wait for events");
        return didSuspend;
#else
        qWarning("QEventLoop::WaitForMoreEvents is not supported on the main thread without asyncify");
        return false;
#endif
}

// Process event activation callbacks for the main thread event dispatcher.
// Must be called on the main thread.
void QEventDispatcherWasm::callProcessEvents(void *context)
{
    Q_ASSERT(emscripten_is_main_runtime_thread());

    // Bail out if Qt has been shut down.
    if (!g_mainThreadEventDispatcher)
        return;

    // In the unlikely event that we get a callProcessEvents() call for
    // a previous main thread event dispatcher (i.e. the QApplication
    // object was deleted and crated again): just ignore it and return.
    if (context != g_mainThreadEventDispatcher)
        return;

    {
#if QT_CONFIG(thread)
        std::lock_guard<std::mutex> lock(g_mainThreadEventDispatcher->m_mutex);
#endif
        g_mainThreadEventDispatcher->m_pendingProcessEvents = false;
    }
    g_mainThreadEventDispatcher->processEvents(QEventLoop::AllEvents);
}

void QEventDispatcherWasm::processTimers()
{
    m_timerInfo->activateTimers();
    updateNativeTimer(); // schedule next native timer, if any
}

// Updates the native timer based on currently registered Qt timers.
// Must be called on the event dispatcher thread.
void QEventDispatcherWasm::updateNativeTimer()
{
#if QT_CONFIG(thread)
    Q_ASSERT(QThread::currentThread() == thread());
#endif

    // Multiplex Qt timers down to a single native timer, maintained
    // to have a timeout corresponding to the shortest Qt timer. This
    // is done in two steps: first determine the target wakeup time
    // on the event dispatcher thread (since this thread has exclusive
    // access to m_timerInfo), and then call native API to set the new
    // wakeup time on the main thread.

    auto timespecToNanosec = [](timespec ts) -> uint64_t {
        return ts.tv_sec * 1000 + ts.tv_nsec / (1000 * 1000);
    };
    timespec toWait;
    bool hasTimer = m_timerInfo->timerWait(toWait);
    uint64_t currentTime = timespecToNanosec(m_timerInfo->currentTime);
    uint64_t toWaitDuration = timespecToNanosec(toWait);
    uint64_t newTargetTime = currentTime + toWaitDuration;

    auto maintainNativeTimer = [this, hasTimer, toWaitDuration, newTargetTime]() {
        Q_ASSERT(emscripten_is_main_runtime_thread());

        if (!hasTimer) {
            if (m_timerId > 0) {
                emscripten_clear_timeout(m_timerId);
                m_timerId = 0;
            }
            return;
        }

        if (m_timerTargetTime != 0 && newTargetTime >= m_timerTargetTime)
            return; // existing timer is good
        emscripten_clear_timeout(m_timerId);
        m_timerId = emscripten_set_timeout(&QEventDispatcherWasm::callProcessTimers, toWaitDuration, this);
        m_timerTargetTime = newTargetTime;
    };

    // Update the native timer for this thread/dispatcher. This must be
    // done on the main thread where we have access to native API.

#if QT_CONFIG(thread)
  if (isSecondaryThreadEventDispatcher()) {
      runOnMainThread([this, maintainNativeTimer]() {
          Q_ASSERT(emscripten_is_main_runtime_thread());

          // "this" may have been deleted, or may be about to be deleted.
          // Check if the pointer we have is still a valid event dispatcher,
          // and keep the mutex locked while updating the native timer to
          // prevent it from being deleted.
          std::lock_guard<std::mutex> lock(g_secondaryThreadEventDispatchersMutex);
          if (g_secondaryThreadEventDispatchers.contains(this))
              maintainNativeTimer();
      });
  } else
#endif
      maintainNativeTimer();
}

// Static timer activation callback. Must be called on the main thread
// and will then either process timers on the main thrad or wake and
// process timers on a secondary thread.
void QEventDispatcherWasm::callProcessTimers(void *context)
{
    Q_ASSERT(emscripten_is_main_runtime_thread());

    // "context" is a pointer to the event dispatcher which
    // should process the timer, but may be a stale pointer
    // to a now-deleted event dispatcher object. Code below
    // verfies the pointer by comparing it to the known "live"
    // event dispatchers.
    Q_ASSERT(context);
    QEventDispatcherWasm *eventDispatcher = reinterpret_cast<QEventDispatcherWasm *>(context);

    // Process timers on this thread if this is the main event dispatcher
    if (eventDispatcher == g_mainThreadEventDispatcher) {
        g_mainThreadEventDispatcher->m_timerTargetTime = 0;
        g_mainThreadEventDispatcher->processTimers();
        return;
    }

    // Wake and process timers on the secondary thread if this a secondary thread dispatcher
#if QT_CONFIG(thread)
    std::lock_guard<std::mutex> lock(g_secondaryThreadEventDispatchersMutex);
    if (g_secondaryThreadEventDispatchers.contains(eventDispatcher)) {
        eventDispatcher->m_timerTargetTime = 0;
        eventDispatcher->m_processTimers = true;
        eventDispatcher->wakeUp();
    }
#endif
}

#if QT_CONFIG(thread)

namespace {
    void trampoline(void *context) {
        std::function<void(void)> *fn = reinterpret_cast<std::function<void(void)> *>(context);
        (*fn)();
        delete fn;
    }
}

// Runs a function on the main thread
void QEventDispatcherWasm::runOnMainThread(std::function<void(void)> fn)
{
    void *context = new std::function<void(void)>(fn);
    emscripten_async_run_in_main_runtime_thread_(EM_FUNC_SIG_VI, reinterpret_cast<void *>(trampoline), context);
}
#endif

QT_END_NAMESPACE
