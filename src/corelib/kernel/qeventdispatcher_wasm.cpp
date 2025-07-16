// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeventdispatcher_wasm_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qthread.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/private/qobject_p.h>
#include <QtCore/private/qwasmglobal_p.h>
#include <QtCore/private/qstdweb_p.h>
#include <QtCore/private/qwasmsocket_p.h>

using namespace std::chrono;
using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

using emscripten::val;

Q_LOGGING_CATEGORY(lcEventDispatcher, "qt.eventdispatcher");
Q_LOGGING_CATEGORY(lcEventDispatcherTimers, "qt.eventdispatcher.timers");

#if QT_CONFIG(thread)
#define LOCK_GUARD(M) std::lock_guard<std::mutex> lock(M)
#else
#define LOCK_GUARD(M)
#endif

#if defined(QT_STATIC)

static bool useAsyncify()
{
    return qstdweb::haveAsyncify();
}

#else

// EM_JS is not supported for side modules; disable asyncify

static bool useAsyncify()
{
    return false;
}

#endif // defined(QT_STATIC)

Q_CONSTINIT QEventDispatcherWasm *QEventDispatcherWasm::g_mainThreadEventDispatcher = nullptr;
Q_CONSTINIT std::shared_ptr<QWasmSuspendResumeControl> QEventDispatcherWasm::g_mainThreadSuspendResumeControl;

#if QT_CONFIG(thread)
Q_CONSTINIT QVector<QEventDispatcherWasm *> QEventDispatcherWasm::g_secondaryThreadEventDispatchers;
Q_CONSTINIT std::mutex QEventDispatcherWasm::g_staticDataMutex;
#endif

QEventDispatcherWasm::QEventDispatcherWasm(std::shared_ptr<QWasmSuspendResumeControl> suspendResumeControl)
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

        if (suspendResumeControl) {
            g_mainThreadSuspendResumeControl = suspendResumeControl;
        } else {
            g_mainThreadSuspendResumeControl = std::make_shared<QWasmSuspendResumeControl>();
        }

        // Zero-timer used on wake() calls
        m_wakeupTimer = std::make_unique<QWasmTimer>(g_mainThreadSuspendResumeControl.get(), [](){ onWakeup(); });

        // Timer set to fire at the next Qt timer timeout
        m_nativeTimer = std::make_unique<QWasmTimer>(g_mainThreadSuspendResumeControl.get(), []() { onTimer(); });

        // Timer used when suspending to process native events
        m_suspendTimer = std::make_unique<QWasmTimer>(g_mainThreadSuspendResumeControl.get(), []() { onProcessNativeEventsResume(); });
    } else {
#if QT_CONFIG(thread)
        std::lock_guard<std::mutex> lock(g_staticDataMutex);
        g_secondaryThreadEventDispatchers.append(this);
#endif
    }

    m_timerInfo = std::make_unique<QTimerInfoList>();
}

QEventDispatcherWasm::~QEventDispatcherWasm()
{
    qCDebug(lcEventDispatcher) << "Destroying QEventDispatcherWasm instance" << this;

    // Reset to ensure destruction before g_mainThreadSuspendResumeControl
    m_wakeupTimer.reset();
    m_nativeTimer.reset();
    m_suspendTimer.reset();

#if QT_CONFIG(thread)
    if (isSecondaryThreadEventDispatcher()) {
        std::lock_guard<std::mutex> lock(g_staticDataMutex);
        g_secondaryThreadEventDispatchers.remove(g_secondaryThreadEventDispatchers.indexOf(this));
    } else
#endif
    {
        QWasmSocket::clearSocketNotifiers();
        g_mainThreadEventDispatcher = nullptr;
        g_mainThreadSuspendResumeControl.reset();
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

bool QEventDispatcherWasm::isValidEventDispatcher()
{
    return isValidEventDispatcherPointer(this);
}

bool QEventDispatcherWasm::isValidEventDispatcherPointer(QEventDispatcherWasm *eventDispatcher)
{
    if (eventDispatcher == g_mainThreadEventDispatcher)
        return true;
#if QT_CONFIG(thread)
    if (g_secondaryThreadEventDispatchers.contains(eventDispatcher))
        return true;
#endif
    return false;
}

bool QEventDispatcherWasm::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    emit awake();

    if (!useAsyncify() && isMainThreadEventDispatcher())
        handleNonAsyncifyErrorCases(flags);

    bool didSendEvents = sendAllEvents(flags);

    if (!isValidEventDispatcher())
        return false;

    if (m_interrupted) {
        m_interrupted = false;
        return false;
    }

    bool shouldWait = flags.testFlag(QEventLoop::WaitForMoreEvents);
    if (!shouldWait || didSendEvents)
        return didSendEvents;

    processEventsWait();

    return sendAllEvents(flags);
}

bool QEventDispatcherWasm::sendAllEvents(QEventLoop::ProcessEventsFlags flags)
{
    bool didSendEvents = false;

    didSendEvents |= sendPostedEvents();
    if (!isValidEventDispatcher())
        return false;

    didSendEvents |= sendNativeEvents(flags);
    if (!isValidEventDispatcher())
        return false;

    didSendEvents |= sendTimerEvents();
    if (!isValidEventDispatcher())
        return false;

    return didSendEvents;
}

bool QEventDispatcherWasm::sendNativeEvents(QEventLoop::ProcessEventsFlags flags)
{
    // TODO: support ExcludeUserInputEvents and ExcludeSocketNotifiers

    // Secondary threads do not support native events
    if (!isMainThreadEventDispatcher())
        return false;

    // Can't suspend without asyncify
    if (!useAsyncify())
        return false;

    // Send any pending events, and
    int sentEventCount = 0;
    sentEventCount += g_mainThreadSuspendResumeControl->sendPendingEvents();

    // if the processEvents() call is made from an exec() call then we assume
    // that the main thread has just resumed, and that it will suspend again
    // at the end of processEvents(). This makes the suspend loop below superfluous.
    if (flags.testFlag(QEventLoop::EventLoopExec))
        return sentEventCount > 0;

    // Run a suspend-resume loop until all pending native events have
    // been processed. Suspending returns control to the browsers'event
    // loop and makes it process events. If any event was for us then
    // the wasm instance will resume (via event handling code in QWasmSuspendResumeControl
    // and process the event.
    //
    // Set a zero-timer to exit the loop via the m_wakeFromSuspendTimer flag.
    // This timer will be added to the end of the native event queue and
    // ensures that all pending (at the time of this sendNativeEvents() call)
    // native events are processed.
    m_wakeFromSuspendTimer = false;
    do {
        m_suspendTimer->setTimeout(0ms);
        g_mainThreadSuspendResumeControl->suspend();
        QScopedValueRollback scoped(m_isSendingNativeEvents, true);
        sentEventCount += g_mainThreadSuspendResumeControl->sendPendingEvents();
    } while (!m_wakeFromSuspendTimer);

    return sentEventCount > 1; // Don't count m_suspendTimer
}

bool QEventDispatcherWasm::sendPostedEvents()
{
    QCoreApplication::sendPostedEvents();

    // QCoreApplication::sendPostedEvents() returns void and does not tell us
    // if it actually did send events. Use the wakeUp() state instead:
    // QCoreApplication::postEvent() calls wakeUp(), so if wakeUp() was
    // called there is a chance there was a posted event. This should never
    // return false if a posted event was sent, but might return true also
    // if there was no event sent.
    bool didWakeup = m_wakeup;
    m_wakeup = false;
    return didWakeup;
}

bool QEventDispatcherWasm::sendTimerEvents()
{
    int activatedTimers = m_timerInfo->activateTimers();
    if (activatedTimers > 0)
        updateNativeTimer();
    return activatedTimers > 0;
}

void QEventDispatcherWasm::registerTimer(Qt::TimerId timerId, Duration interval, Qt::TimerType timerType, QObject *object)
{
#ifndef QT_NO_DEBUG
    if (qToUnderlying(timerId) < 1 || interval < 0ns || !object) {
        qWarning("QEventDispatcherWasm::registerTimer: invalid arguments");
        return;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWasm::registerTimer: timers cannot be started from another "
                 "thread");
        return;
    }
#endif
    qCDebug(lcEventDispatcherTimers) << "registerTimer" << int(timerId) << interval << timerType << object;

    m_timerInfo->registerTimer(timerId, interval, timerType, object);
    updateNativeTimer();
}

bool QEventDispatcherWasm::unregisterTimer(Qt::TimerId timerId)
{
#ifndef QT_NO_DEBUG
    if (qToUnderlying(timerId) < 1) {
        qWarning("QEventDispatcherWasm::unregisterTimer: invalid argument");
        return false;
    } else if (thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWasm::unregisterTimer: timers cannot be stopped from another "
                 "thread");
        return false;
    }
#endif

    qCDebug(lcEventDispatcherTimers) << "unregisterTimer" << int(timerId);

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

QList<QAbstractEventDispatcher::TimerInfoV2>
QEventDispatcherWasm::timersForObject(QObject *object) const
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWasm:registeredTimers: invalid argument");
        return {};
    }
#endif

    return m_timerInfo->registeredTimers(object);
}

QEventDispatcherWasm::Duration QEventDispatcherWasm::remainingTime(Qt::TimerId timerId) const
{
    return m_timerInfo->remainingDuration(timerId);
}

void QEventDispatcherWasm::interrupt()
{
    m_interrupted = true;
    wakeUp();
}

void QEventDispatcherWasm::wakeUp()
{
    m_wakeup = true;
#if QT_CONFIG(thread)
    if (isSecondaryThreadEventDispatcher()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_wakeUpCalled = true;
        m_moreEvents.notify_one();
    } else
#endif
    {
        QEventDispatcherWasm *eventDispatcher = this;
        qwasmglobal::runOnMainThreadAsync([eventDispatcher]() {
            if (isValidEventDispatcherPointer(eventDispatcher)) {
                if (!eventDispatcher->m_wakeupTimer->hasTimeout())
                    eventDispatcher->m_wakeupTimer->setTimeout(0ms);
            }
        });
    }
}

void QEventDispatcherWasm::handleNonAsyncifyErrorCases(QEventLoop::ProcessEventsFlags flags)
{
    Q_ASSERT(!useAsyncify());

    if (flags & QEventLoop::ApplicationExec) {
        // Start the main loop, and then stop it on the first callback. This
        // is done for the "simulateInfiniteLoop" functionality where
        // emscripten_set_main_loop() throws a JS exception which returns
        // control to the browser while preserving the C++ stack.
        const bool simulateInfiniteLoop = true;
        emscripten_set_main_loop([](){
            emscripten_pause_main_loop();
        }, 0, simulateInfiniteLoop);
    } else if (flags & QEventLoop::DialogExec) {
        qFatal() << "Calling exec() is not supported on Qt for WebAssembly in this configuration. Please build"
                << "with asyncify support, or use an asynchronous API like QDialog::open()";
    } else if (flags & QEventLoop::WaitForMoreEvents) {
        qFatal("QEventLoop::WaitForMoreEvents is not supported on the main thread without asyncify");
    }
}

// Blocks or suspends the current thread for the given amount of time.
// The event dispatcher does not process events while blocked. TODO:
// make it not process events while blocked.
bool QEventDispatcherWasm::wait(int timeout)
{
    auto tim = timeout > 0 ? std::optional<std::chrono::milliseconds>(timeout) : std::nullopt;
    if (isSecondaryThreadEventDispatcher())
        return secondaryThreadWait(tim);
    if (useAsyncify())
        asyncifyWait(tim);
    return true;
}

// Waits for more events by blocking or suspending the current thread. Should be called from
// processEvents() only.
void QEventDispatcherWasm::processEventsWait()
{
    if (isMainThreadEventDispatcher()) {
        asyncifyWait(std::nullopt);
    } else {
        auto nanoWait = m_timerInfo->timerWait();
        std::optional<std::chrono::milliseconds> milliWait;
        if (nanoWait.has_value())
            milliWait = std::chrono::duration_cast<std::chrono::milliseconds>(*nanoWait);
        secondaryThreadWait(milliWait);
    }
}

void QEventDispatcherWasm::asyncifyWait(std::optional<std::chrono::milliseconds> timeout)
{
    Q_ASSERT(emscripten_is_main_runtime_thread());
    Q_ASSERT(isMainThreadEventDispatcher());
    Q_ASSERT(useAsyncify());
    if (timeout.has_value())
        m_suspendTimer->setTimeout(timeout.value());
    g_mainThreadSuspendResumeControl->suspend();
}

bool QEventDispatcherWasm::secondaryThreadWait(std::optional<std::chrono::milliseconds> timeout)
{
#if QT_CONFIG(thread)
    Q_ASSERT(QThread::currentThread() == thread());
    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> lock(m_mutex);

    // If wakeUp() was called there might be pending events in the event
    // queue which should be processed. Don't block, instead return
    // so that the event loop can spin and call processEvents() again.
    if (m_wakeUpCalled) {
        m_wakeUpCalled = false;
        return true;
    }

    auto waitTime = timeout.value_or(std::chrono::milliseconds::max());
    bool wakeUpCalled = m_moreEvents.wait_for(lock, waitTime, [this] { return m_wakeUpCalled; });
    m_wakeUpCalled = false;
    return wakeUpCalled;
#else
    Q_UNREACHABLE();
    return false;
#endif
}

void QEventDispatcherWasm::onTimer()
{
    Q_ASSERT(emscripten_is_main_runtime_thread());
    if (!g_mainThreadEventDispatcher)
        return;

    // If asyncify is in use then instance will resume and process timers
    // in processEvents()
    if (useAsyncify())
        return;

    g_mainThreadEventDispatcher->sendTimerEvents();
}

void QEventDispatcherWasm::onWakeup()
{
    Q_ASSERT(emscripten_is_main_runtime_thread());
    if (!g_mainThreadEventDispatcher)
        return;

    // In the case where we are suspending from sendNativeEvents() we don't want
    // to call processEvents() again, since we are then already in processEvents()
    // and are already awake.
    if (g_mainThreadEventDispatcher->m_isSendingNativeEvents)
        return;

    g_mainThreadEventDispatcher->processEvents(QEventLoop::AllEvents);
}

void QEventDispatcherWasm::onProcessNativeEventsResume()
{
    Q_ASSERT(emscripten_is_main_runtime_thread());
    if (!g_mainThreadEventDispatcher)
        return;
    g_mainThreadEventDispatcher->m_wakeFromSuspendTimer = true;
}

// Updates the native timer based on currently registered Qt timers,
// by setting a timeout equivalent to the shortest timer.
// Must be called on the event dispatcher thread.
void QEventDispatcherWasm::updateNativeTimer()
{
#if QT_CONFIG(thread)
    Q_ASSERT(QThread::currentThread() == thread());
#endif

    // On secondary threads, the timeout is managed by setting the WaitForMoreEvents
    // timeout in processEventsWait().
    if (!isMainThreadEventDispatcher())
        return;

    // Clear any timer if there are no active timers
    const std::optional<std::chrono::nanoseconds> nanoWait = m_timerInfo->timerWait();
    if (!nanoWait.has_value()) {
        m_nativeTimer->clearTimeout();
        return;
    }

    auto milliWait = std::chrono::duration_cast<std::chrono::milliseconds>(*nanoWait);
    const auto newTargetTime = m_timerInfo->currentTime + milliWait;

    // Keep existing timer if the timeout has not changed.
    if (m_nativeTimer->hasTimeout() && newTargetTime == m_timerTargetTime)
        return;

    // Clear current and set new timer
    qCDebug(lcEventDispatcherTimers)
            << "Created new native timer timeout" << milliWait.count() << "ms"
            << "previous target time" << m_timerTargetTime.time_since_epoch()
            << "new target time" << newTargetTime.time_since_epoch();
    m_nativeTimer->clearTimeout();
    m_nativeTimer->setTimeout(milliWait);
    m_timerTargetTime = newTargetTime;
}

namespace {
    int g_startupTasks = 0;
}

// The following functions manages sending the "qtLoaded" event/callback
// from qtloader.js on startup, once Qt initialization has been completed
// and the application is ready to display the first frame. This can be
// either as soon as the event loop is running, or later, if additional
// startup tasks (e.g. local font loading) have been registered.

void QEventDispatcherWasm::registerStartupTask()
{
    ++g_startupTasks;
}

void QEventDispatcherWasm::completeStarupTask()
{
    --g_startupTasks;
    callOnLoadedIfRequired();
}

void QEventDispatcherWasm::callOnLoadedIfRequired()
{
    if (g_startupTasks > 0)
        return;

    static bool qtLoadedCalled = false;
    if (qtLoadedCalled)
        return;
    qtLoadedCalled = true;
}

void QEventDispatcherWasm::onLoaded()
{
    // TODO: call qtloader.js onLoaded from here, in order to delay
    // hiding the "Loading..." message until the app is ready to paint
    // the first frame. Currently onLoaded must be called early before
    // main() in order to ensure that the screen/container elements
    // have valid geometry at startup.
}

void QEventDispatcherWasm::registerSocketNotifier(QSocketNotifier *notifier)
{
    QWasmSocket::registerSocketNotifier(notifier);
}

void QEventDispatcherWasm::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    QWasmSocket::unregisterSocketNotifier(notifier);
}

void QEventDispatcherWasm::socketSelect(int timeout, int socket, bool waitForRead, bool waitForWrite,
                                        bool *selectForRead, bool *selectForWrite, bool *socketDisconnect)
{
    QEventDispatcherWasm *eventDispatcher = static_cast<QEventDispatcherWasm *>(
        QAbstractEventDispatcher::instance(QThread::currentThread()));

    if (!eventDispatcher) {
        qWarning("QEventDispatcherWasm::socketSelect called without eventdispatcher instance");
        return;
    }

    QWasmSocket::waitForSocketState(eventDispatcher, timeout, socket, waitForRead, waitForWrite,
                                    selectForRead, selectForWrite, socketDisconnect);
}

QT_END_NAMESPACE

#include "moc_qeventdispatcher_wasm_p.cpp"
