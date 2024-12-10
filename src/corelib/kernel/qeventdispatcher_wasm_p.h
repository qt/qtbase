// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVENTDISPATCHER_WASM_P_H
#define QEVENTDISPATCHER_WASM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qabstracteventdispatcher.h"
#include "private/qtimerinfo_unix_p.h"
#include <QtCore/qloggingcategory.h>
#include <QtCore/qwaitcondition.h>

#include <chrono>
#include <mutex>
#include <optional>
#include <tuple>

#include <emscripten/proxying.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcEventDispatcher);
Q_DECLARE_LOGGING_CATEGORY(lcEventDispatcherTimers)

class Q_CORE_EXPORT QEventDispatcherWasm : public QAbstractEventDispatcherV2
{
    Q_OBJECT
public:
    QEventDispatcherWasm();
    ~QEventDispatcherWasm();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

    void registerTimer(Qt::TimerId timerId, Duration interval, Qt::TimerType timerType,
                       QObject *object) override final;
    bool unregisterTimer(Qt::TimerId timerId) override final;
    bool unregisterTimers(QObject *object) override final;
    QList<TimerInfoV2> timersForObject(QObject *object) const override final;
    Duration remainingTime(Qt::TimerId timerId) const override final;

    void interrupt() override;
    void wakeUp() override;

    void registerSocketNotifier(QSocketNotifier *notifier) override;
    void unregisterSocketNotifier(QSocketNotifier *notifier) override;
    static void socketSelect(int timeout, int socket, bool waitForRead, bool waitForWrite,
                            bool *selectForRead, bool *selectForWrite, bool *socketDisconnect);

    static void runOnMainThread(std::function<void(void)> fn);

    static void registerStartupTask();
    static void completeStarupTask();
    static void callOnLoadedIfRequired();
    virtual void onLoaded();

protected:
    virtual bool processPostedEvents();

private:
    bool isMainThreadEventDispatcher();
    bool isSecondaryThreadEventDispatcher();
    static bool isValidEventDispatcherPointer(QEventDispatcherWasm *eventDispatcher);

    void handleApplicationExec();
    void handleDialogExec();
    bool wait(int timeout = -1);
    bool wakeEventDispatcherThread();
    static void callProcessPostedEvents(void *eventDispatcher);

    void processTimers();
    void updateNativeTimer();
    static void callProcessTimers(void *eventDispatcher);

    static void run(std::function<void(void)> fn);
    static void runAsync(std::function<void(void)> fn);
    static void runOnMainThreadAsync(std::function<void(void)> fn);

    static QEventDispatcherWasm *g_mainThreadEventDispatcher;

    bool m_interrupted = false;
    bool m_processTimers = false;
    bool m_pendingProcessEvents = false;

    QTimerInfoList *m_timerInfo = new QTimerInfoList();
    long m_timerId = 0;
    std::chrono::milliseconds m_timerTargetTime{};

#if QT_CONFIG(thread)
    std::mutex m_mutex;
    bool m_wakeUpCalled = false;
    std::condition_variable m_moreEvents;

    static QVector<QEventDispatcherWasm *> g_secondaryThreadEventDispatchers;
    static std::mutex g_staticDataMutex;
    static emscripten::ProxyingQueue g_proxyingQueue;
    static pthread_t g_mainThread;

    // Note on mutex usage: the global g_staticDataMutex protects the global (g_ prefixed) data,
    // while the per eventdispatcher m_mutex protects the state accociated with blocking and waking
    // that eventdispatcher thread. The locking order is g_staticDataMutex first, then m_mutex.
#endif

    friend class QWasmSocket;
};

#endif // QEVENTDISPATCHER_WASM_P_H
