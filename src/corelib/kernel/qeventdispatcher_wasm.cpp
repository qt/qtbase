// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeventdispatcher_wasm_p.h"

#include <QtCore/private/qabstracteventdispatcher_p.h> // for qGlobalPostedEventsCount()
#include <QtCore/qcoreapplication.h>
#include <QtCore/qthread.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/private/qstdweb_p.h>

#include "emscripten.h"
#include <emscripten/html5.h>
#include <emscripten/threading.h>
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

// using namespace emscripten;

Q_LOGGING_CATEGORY(lcEventDispatcher, "qt.eventdispatcher");
Q_LOGGING_CATEGORY(lcEventDispatcherTimers, "qt.eventdispatcher.timers");

#if QT_CONFIG(thread)
#define LOCK_GUARD(M) std::lock_guard<std::mutex> lock(M)
#else
#define LOCK_GUARD(M)
#endif

// Emscripten asyncify currently supports one level of suspend -
// recursion is not permitted. We track the suspend state here
// on order to fail (more) gracefully, but we can of course only
// track Qts own usage of asyncify.
static bool g_is_asyncify_suspended = false;

#if defined(QT_STATIC)

static bool useAsyncify()
{
    return qstdweb::haveAsyncify();
}

EM_JS(void, qt_asyncify_suspend_js, (), {
    if (Module.qtSuspendId === undefined)
        Module.qtSuspendId = 0;
    let sleepFn = (wakeUp) => {
        Module.qtAsyncifyWakeUp = wakeUp;
    };
    ++Module.qtSuspendId;
    return Asyncify.handleSleep(sleepFn);
});

EM_JS(void, qt_asyncify_resume_js, (), {
    let wakeUp = Module.qtAsyncifyWakeUp;
    if (wakeUp == undefined)
        return;
    Module.qtAsyncifyWakeUp = undefined;
    const suspendId = Module.qtSuspendId;

    // Delayed wakeup with zero-timer. Workaround/fix for
    // https://github.com/emscripten-core/emscripten/issues/10515
    setTimeout(() => {
        // Another suspend occurred while the timeout was in queue.
        if (Module.qtSuspendId !== suspendId)
            return;
        wakeUp();
    });
});

#else

// EM_JS is not supported for side modules; disable asyncify

static bool useAsyncify()
{
    return false;
}

void qt_asyncify_suspend_js()
{
    Q_UNREACHABLE();
}

void qt_asyncify_resume_js()
{
    Q_UNREACHABLE();
}

#endif // defined(QT_STATIC)

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
// thread was suspended, in which case it will now be asynchronously woken.
bool qt_asyncify_resume()
{
    if (!g_is_asyncify_suspended)
        return false;
    g_is_asyncify_suspended = false;
    qt_asyncify_resume_js();
    return true;
}

Q_CONSTINIT QEventDispatcherWasm *QEventDispatcherWasm::g_mainThreadEventDispatcher = nullptr;
#if QT_CONFIG(thread)
Q_CONSTINIT QVector<QEventDispatcherWasm *> QEventDispatcherWasm::g_secondaryThreadEventDispatchers;
Q_CONSTINIT std::mutex QEventDispatcherWasm::g_staticDataMutex;
emscripten::ProxyingQueue QEventDispatcherWasm::g_proxyingQueue;
pthread_t QEventDispatcherWasm::g_mainThread;
#endif
// ### dynamic initialization:
std::multimap<int, QSocketNotifier *> QEventDispatcherWasm::g_socketNotifiers;
std::map<int, QEventDispatcherWasm::SocketReadyState> QEventDispatcherWasm::g_socketState;

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
#if QT_CONFIG(thread)
        g_mainThread = pthread_self();
#endif
    } else {
#if QT_CONFIG(thread)
        std::lock_guard<std::mutex> lock(g_staticDataMutex);
        g_secondaryThreadEventDispatchers.append(this);
#endif
    }
}

QEventDispatcherWasm::~QEventDispatcherWasm()
{
    qCDebug(lcEventDispatcher) << "Destroying QEventDispatcherWasm instance" << this;

    delete m_timerInfo;

#if QT_CONFIG(thread)
    if (isSecondaryThreadEventDispatcher()) {
        std::lock_guard<std::mutex> lock(g_staticDataMutex);
        g_secondaryThreadEventDispatchers.remove(g_secondaryThreadEventDispatchers.indexOf(this));
    } else
#endif
    {
        if (m_timerId > 0)
            emscripten_clear_timeout(m_timerId);
        if (!g_socketNotifiers.empty()) {
            qWarning("QEventDispatcherWasm: main thread event dispatcher deleted with active socket notifiers");
            clearEmscriptenSocketCallbacks();
            g_socketNotifiers.clear();
        }
        g_mainThreadEventDispatcher = nullptr;
        if (!g_socketNotifiers.empty()) {
            qWarning("QEventDispatcherWasm: main thread event dispatcher deleted with active socket notifiers");
            clearEmscriptenSocketCallbacks();
            g_socketNotifiers.clear();
        }

        g_socketState.clear();
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

bool QEventDispatcherWasm::isValidEventDispatcherPointer(QEventDispatcherWasm *eventDispatcher)
{
    if (eventDispatcher == g_mainThreadEventDispatcher)
        return true;
#if QT_CONFIG(thread)
    Q_ASSERT(!g_staticDataMutex.try_lock()); // caller must lock mutex
    if (g_secondaryThreadEventDispatchers.contains(eventDispatcher))
        return true;
#endif
    return false;
}

bool QEventDispatcherWasm::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    qCDebug(lcEventDispatcher) << "QEventDispatcherWasm::processEvents flags" << flags;

    emit awake();

    if (isMainThreadEventDispatcher()) {
        if (flags & QEventLoop::DialogExec)
            handleDialogExec();
        else if (flags & QEventLoop::ApplicationExec)
            handleApplicationExec();
    }

    processPostedEvents();

    // The processPostedEvents() call above may process an event which deletes the
    // application object and the event dispatcher; stop event processing in that case.
    if (!isValidEventDispatcherPointer(this))
        return false;

    if (m_interrupted) {
        m_interrupted = false;
        return false;
    }

    if (flags & QEventLoop::WaitForMoreEvents)
        wait();

    if (m_processTimers) {
        m_processTimers = false;
        processTimers();
    }

    return false;
}

void QEventDispatcherWasm::registerSocketNotifier(QSocketNotifier *notifier)
{
    LOCK_GUARD(g_staticDataMutex);

    bool wasEmpty = g_socketNotifiers.empty();
    g_socketNotifiers.insert({notifier->socket(), notifier});
    if (wasEmpty)
        runOnMainThread([]{ setEmscriptenSocketCallbacks(); });
}

void QEventDispatcherWasm::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    LOCK_GUARD(g_staticDataMutex);

    auto notifiers = g_socketNotifiers.equal_range(notifier->socket());
    for (auto it = notifiers.first; it != notifiers.second; ++it) {
        if (it->second == notifier) {
            g_socketNotifiers.erase(it);
            break;
        }
    }

    if (g_socketNotifiers.empty())
        runOnMainThread([]{ clearEmscriptenSocketCallbacks(); });
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
    // The event dispatcher thread may be blocked or suspended by
    // wait(), or control may have been returned to the browser's
    // event loop. Make sure the thread is unblocked or make it
    // process events.
    bool wasBlocked = wakeEventDispatcherThread();
    if (!wasBlocked && isMainThreadEventDispatcher()) {
        {
            LOCK_GUARD(m_mutex);
            if (m_pendingProcessEvents)
                return;
            m_pendingProcessEvents = true;
        }
        runOnMainThreadAsync([this](){
            QEventDispatcherWasm::callProcessPostedEvents(this);
        });
    }
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
    if (!useAsyncify()) {
        qWarning() << "Warning: exec() is not supported on Qt for WebAssembly in this configuration. Please build"
                   << "with asyncify support, or use an asynchronous API like QDialog::open()";
        emscripten_sleep(1); // This call never returns
    }
    // For the asyncify case we do nothing here and wait for events in wait()
}

// Blocks/suspends the calling thread. This is possible in two cases:
// - Caller is a secondary thread: block on m_moreEvents
// - Caller is the main thread and asyncify is enabled: suspend using qt_asyncify_suspend()
// Returns false if the wait timed out.
bool QEventDispatcherWasm::wait(int timeout)
{
#if QT_CONFIG(thread)
    using namespace std::chrono_literals;
    Q_ASSERT(QThread::currentThread() == thread());

    if (isSecondaryThreadEventDispatcher()) {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_wakeUpCalled = false;
        auto wait_time = timeout > 0 ? timeout * 1ms : std::chrono::duration<int, std::micro>::max();
        bool wakeUpCalled = m_moreEvents.wait_for(lock, wait_time, [=] { return m_wakeUpCalled; });
        return wakeUpCalled;
    }
#endif
    Q_ASSERT(emscripten_is_main_runtime_thread());
    Q_ASSERT(isMainThreadEventDispatcher());
    if (useAsyncify()) {
        if (timeout > 0)
            qWarning() << "QEventDispatcherWasm asyncify wait with timeout is not supported; timeout will be ignored"; // FIXME

        bool didSuspend = qt_asyncify_suspend();
        if (!didSuspend) {
            qWarning("QEventDispatcherWasm: current thread is already suspended; could not asyncify wait for events");
            return false;
        }
        return true;
    } else {
        qWarning("QEventLoop::WaitForMoreEvents is not supported on the main thread without asyncify");
        Q_UNUSED(timeout);
    }
    return false;
}

// Wakes a blocked/suspended event dispatcher thread. Returns true if the
// thread is unblocked or was resumed, false if the thread state could not
// be determined.
bool QEventDispatcherWasm::wakeEventDispatcherThread()
{
#if QT_CONFIG(thread)
    if (isSecondaryThreadEventDispatcher()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_wakeUpCalled = true;
        m_moreEvents.notify_one();
        return true;
    }
#endif
    Q_ASSERT(isMainThreadEventDispatcher());
    if (g_is_asyncify_suspended) {
        runOnMainThread([]{ qt_asyncify_resume(); });
        return true;
    }
    return false;
}

// Process event activation callbacks for the main thread event dispatcher.
// Must be called on the main thread.
void QEventDispatcherWasm::callProcessPostedEvents(void *context)
{
    Q_ASSERT(emscripten_is_main_runtime_thread());

    // Bail out if Qt has been shut down.
    if (!g_mainThreadEventDispatcher)
        return;

    // In the unlikely event that we get a callProcessPostedEvents() call for
    // a previous main thread event dispatcher (i.e. the QApplication
    // object was deleted and created again): just ignore it and return.
    if (context != g_mainThreadEventDispatcher)
        return;

    {
        LOCK_GUARD(g_mainThreadEventDispatcher->m_mutex);
        g_mainThreadEventDispatcher->m_pendingProcessEvents = false;
    }

    g_mainThreadEventDispatcher->processPostedEvents();
}

bool QEventDispatcherWasm::processPostedEvents()
{
    QCoreApplication::sendPostedEvents();
    return false;
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

    auto timespecToMsec = [](timespec ts) -> uint64_t {
        return ts.tv_sec * 1000 + ts.tv_nsec / (1000 * 1000);
    };
    timespec toWait;
    bool hasTimer = m_timerInfo->timerWait(toWait);
    uint64_t currentTime = timespecToMsec(m_timerInfo->currentTime);
    uint64_t toWaitDuration = timespecToMsec(toWait);
    uint64_t newTargetTime = currentTime + toWaitDuration;

    auto maintainNativeTimer = [this, hasTimer, toWaitDuration, newTargetTime]() {
        Q_ASSERT(emscripten_is_main_runtime_thread());

        if (!hasTimer) {
            if (m_timerId > 0) {
                emscripten_clear_timeout(m_timerId);
                m_timerId = 0;
                m_timerTargetTime = 0;
            }
            return;
        }

        if (m_timerTargetTime != 0 && newTargetTime >= m_timerTargetTime)
            return; // existing timer is good

        qCDebug(lcEventDispatcherTimers)
                << "Created new native timer with wait" << toWaitDuration << "timeout" << newTargetTime;
        emscripten_clear_timeout(m_timerId);
        m_timerId = emscripten_set_timeout(&QEventDispatcherWasm::callProcessTimers, toWaitDuration, this);
        m_timerTargetTime = newTargetTime;
    };

    // Update the native timer for this thread/dispatcher. This must be
    // done on the main thread where we have access to native API.
    runOnMainThread([this, maintainNativeTimer]() {
        Q_ASSERT(emscripten_is_main_runtime_thread());

        // "this" may have been deleted, or may be about to be deleted.
        // Check if the pointer we have is still a valid event dispatcher,
        // and keep the mutex locked while updating the native timer to
        // prevent it from being deleted.
        LOCK_GUARD(g_staticDataMutex);
        if (isValidEventDispatcherPointer(this))
            maintainNativeTimer();
    });
}

// Static timer activation callback. Must be called on the main thread
// and will then either process timers on the main thread or wake and
// process timers on a secondary thread.
void QEventDispatcherWasm::callProcessTimers(void *context)
{
    Q_ASSERT(emscripten_is_main_runtime_thread());

    // Note: "context" may be a stale pointer here,
    // take care before casting and dereferencing!

    // Process timers on this thread if this is the main event dispatcher
    if (reinterpret_cast<QEventDispatcherWasm *>(context) == g_mainThreadEventDispatcher) {
        g_mainThreadEventDispatcher->m_timerTargetTime = 0;
        g_mainThreadEventDispatcher->processTimers();
        return;
    }

    // Wake and process timers on the secondary thread if this a secondary thread dispatcher
#if QT_CONFIG(thread)
    std::lock_guard<std::mutex> lock(g_staticDataMutex);
    if (g_secondaryThreadEventDispatchers.contains(context)) {
        QEventDispatcherWasm *eventDispatcher = reinterpret_cast<QEventDispatcherWasm *>(context);
        eventDispatcher->m_timerTargetTime = 0;
        eventDispatcher->m_processTimers = true;
        eventDispatcher->wakeUp();
    }
#endif
}

void QEventDispatcherWasm::setEmscriptenSocketCallbacks()
{
    qCDebug(lcEventDispatcher) << "setEmscriptenSocketCallbacks";

    emscripten_set_socket_error_callback(nullptr, QEventDispatcherWasm::socketError);
    emscripten_set_socket_open_callback(nullptr, QEventDispatcherWasm::socketOpen);
    emscripten_set_socket_listen_callback(nullptr, QEventDispatcherWasm::socketListen);
    emscripten_set_socket_connection_callback(nullptr, QEventDispatcherWasm::socketConnection);
    emscripten_set_socket_message_callback(nullptr, QEventDispatcherWasm::socketMessage);
    emscripten_set_socket_close_callback(nullptr, QEventDispatcherWasm::socketClose);
}

void QEventDispatcherWasm::clearEmscriptenSocketCallbacks()
{
    qCDebug(lcEventDispatcher) << "clearEmscriptenSocketCallbacks";

    emscripten_set_socket_error_callback(nullptr, nullptr);
    emscripten_set_socket_open_callback(nullptr, nullptr);
    emscripten_set_socket_listen_callback(nullptr, nullptr);
    emscripten_set_socket_connection_callback(nullptr, nullptr);
    emscripten_set_socket_message_callback(nullptr, nullptr);
    emscripten_set_socket_close_callback(nullptr, nullptr);
}

void QEventDispatcherWasm::socketError(int socket, int err, const char* msg, void *context)
{
    Q_UNUSED(err);
    Q_UNUSED(msg);
    Q_UNUSED(context);

    // Emscripten makes socket callbacks while the main thread is busy-waiting for a mutex,
    // which can cause deadlocks if the callback code also tries to lock the same mutex.
    // This is most easily reproducible by adding print statements, where each print requires
    // taking a mutex lock. Work around this by running the callback asynchronously, i.e. by using
    // a native zero-timer, to make sure the main thread stack is completely unwond before calling
    // the Qt handler.
    // It is currently unclear if this problem is caused by code in Qt or in Emscripten, or
    // if this completely fixes the problem.
    runAsync([socket](){
        auto notifiersRange = g_socketNotifiers.equal_range(socket);
        std::vector<std::pair<int, QSocketNotifier *>> notifiers(notifiersRange.first, notifiersRange.second);
        for (auto [_, notifier]: notifiers) {
            QCoreApplication::postEvent(notifier, new QEvent(QEvent::SockAct));
        }
        setSocketState(socket, true, true);
    });
}

void QEventDispatcherWasm::socketOpen(int socket, void *context)
{
    Q_UNUSED(context);

    runAsync([socket](){
        auto notifiersRange = g_socketNotifiers.equal_range(socket);
        std::vector<std::pair<int, QSocketNotifier *>> notifiers(notifiersRange.first, notifiersRange.second);
        for (auto [_, notifier]: notifiers) {
            if (notifier->type() == QSocketNotifier::Write) {
                QCoreApplication::postEvent(notifier, new QEvent(QEvent::SockAct));
            }
        }
        setSocketState(socket, false, true);
    });
}

void QEventDispatcherWasm::socketListen(int socket, void *context)
{
    Q_UNUSED(socket);
    Q_UNUSED(context);
}

void QEventDispatcherWasm::socketConnection(int socket, void *context)
{
    Q_UNUSED(socket);
    Q_UNUSED(context);
}

void QEventDispatcherWasm::socketMessage(int socket, void *context)
{
    Q_UNUSED(context);

    runAsync([socket](){
        auto notifiersRange = g_socketNotifiers.equal_range(socket);
        std::vector<std::pair<int, QSocketNotifier *>> notifiers(notifiersRange.first, notifiersRange.second);
        for (auto [_, notifier]: notifiers) {
            if (notifier->type() == QSocketNotifier::Read) {
                QCoreApplication::postEvent(notifier, new QEvent(QEvent::SockAct));
            }
        }
        setSocketState(socket, true, false);
    });
}

void QEventDispatcherWasm::socketClose(int socket, void *context)
{
    Q_UNUSED(context);

    // Emscripten makes emscripten_set_socket_close_callback() calls to socket 0,
    // which is not a valid socket. see https://github.com/emscripten-core/emscripten/issues/6596
    if (socket == 0)
        return;

    runAsync([socket](){
        auto notifiersRange = g_socketNotifiers.equal_range(socket);
        std::vector<std::pair<int, QSocketNotifier *>> notifiers(notifiersRange.first, notifiersRange.second);
        for (auto [_, notifier]: notifiers)
            QCoreApplication::postEvent(notifier, new QEvent(QEvent::SockClose));

        setSocketState(socket, true, true);
        clearSocketState(socket);
    });
}

void QEventDispatcherWasm::setSocketState(int socket, bool setReadyRead, bool setReadyWrite)
{
    LOCK_GUARD(g_staticDataMutex);
    SocketReadyState &state = g_socketState[socket];

    // Additively update socket ready state, e.g. if it
    // was already ready read then it stays ready read.
    state.readyRead |= setReadyRead;
    state.readyWrite |= setReadyWrite;

    // Wake any waiters for the given readiness. The waiter consumes
    // the ready state, returning the socket to not-ready.
    if (QEventDispatcherWasm *waiter = state.waiter)
        if ((state.readyRead && state.waitForReadyRead) || (state.readyWrite && state.waitForReadyWrite))
            waiter->wakeEventDispatcherThread();
}

void QEventDispatcherWasm::clearSocketState(int socket)
{
    LOCK_GUARD(g_staticDataMutex);
    g_socketState.erase(socket);
}

void QEventDispatcherWasm::waitForSocketState(int timeout, int socket, bool checkRead, bool checkWrite,
                                              bool *selectForRead, bool *selectForWrite, bool *socketDisconnect)
{
    // Loop until the socket becomes readyRead or readyWrite. Wait for
    // socket activity if it currently is neither.
    while (true) {
        *selectForRead = false;
        *selectForWrite = false;

        {
            LOCK_GUARD(g_staticDataMutex);

            // Access or create socket state: we want to register that a thread is waitng
            // even if we have not received any socket callbacks yet.
            SocketReadyState &state = g_socketState[socket];
            if (state.waiter) {
                qWarning() << "QEventDispatcherWasm::waitForSocketState: a thread is already waiting";
                break;
            }

            bool shouldWait = true;
            if (checkRead && state.readyRead) {
                shouldWait = false;
                state.readyRead = false;
                *selectForRead = true;
            }
            if (checkWrite && state.readyWrite) {
                shouldWait = false;
                state.readyWrite = false;
                *selectForRead = true;
            }
            if (!shouldWait)
                break;

            state.waiter = this;
            state.waitForReadyRead = checkRead;
            state.waitForReadyWrite = checkWrite;
        }

        bool didTimeOut = !wait(timeout);
        {
            LOCK_GUARD(g_staticDataMutex);

            // Missing socket state after a wakeup means that the socket has been closed.
            auto it = g_socketState.find(socket);
            if (it == g_socketState.end()) {
                *socketDisconnect = true;
                break;
            }
            it->second.waiter = nullptr;
            it->second.waitForReadyRead = false;
            it->second.waitForReadyWrite = false;
        }

        if (didTimeOut)
            break;
    }
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

    eventDispatcher->waitForSocketState(timeout, socket, waitForRead, waitForWrite,
                                        selectForRead, selectForWrite, socketDisconnect);
}

namespace {
    void trampoline(void *context) {

        auto async_fn = [](void *context){
            std::function<void(void)> *fn = reinterpret_cast<std::function<void(void)> *>(context);
            (*fn)();
            delete fn;
        };

        emscripten_async_call(async_fn, context, 0);
    }
}

// Runs a function right away
void QEventDispatcherWasm::run(std::function<void(void)> fn)
{
    fn();
}

// Runs a function asynchronously. Main thread only.
void QEventDispatcherWasm::runAsync(std::function<void(void)> fn)
{
    trampoline(new std::function<void(void)>(fn));
}

// Runs a function on the main thread. The function runs synchronusly if
// the calling thread is then main thread.
void QEventDispatcherWasm::runOnMainThread(std::function<void(void)> fn)
{
#if QT_CONFIG(thread)
    if (!emscripten_is_main_runtime_thread()) {
        void *context = new std::function<void(void)>(fn);
        g_proxyingQueue.proxyAsync(g_mainThread, [context]{
            trampoline(context);
        });
        return;
    }
#endif
    fn();
}

// Runs a function on the main thread. The function always runs asynchronously,
// also if the calling thread is the main thread.
void QEventDispatcherWasm::runOnMainThreadAsync(std::function<void(void)> fn)
{
    void *context = new std::function<void(void)>(fn);
#if QT_CONFIG(thread)
    if (!emscripten_is_main_runtime_thread()) {
        g_proxyingQueue.proxyAsync(g_mainThread, [context]{
            trampoline(context);
        });
        return;
    }
#endif
    trampoline(context);
}

QT_END_NAMESPACE
