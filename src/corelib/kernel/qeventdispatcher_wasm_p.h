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
#include "private/qwasmsuspendresumecontrol_p.h"
#include <QtCore/qloggingcategory.h>
#include <QtCore/qwaitcondition.h>

#include <chrono>
#include <mutex>
#include <optional>
#include <tuple>
#include <memory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcEventDispatcher);
Q_DECLARE_LOGGING_CATEGORY(lcEventDispatcherTimers)

class Q_CORE_EXPORT QEventDispatcherWasm : public QAbstractEventDispatcherV2
{
    Q_OBJECT
public:
    QEventDispatcherWasm(std::shared_ptr<QWasmSuspendResumeControl> suspendResume = std::shared_ptr<QWasmSuspendResumeControl>());
    ~QEventDispatcherWasm();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;
    bool sendAllEvents(QEventLoop::ProcessEventsFlags flag);

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

    static void registerStartupTask();
    static void completeStarupTask();
    static void callOnLoadedIfRequired();
    virtual void onLoaded();

    static void onTimer();
    static void onWakeup();
    static void onProcessNativeEventsResume();
protected:
    virtual bool sendPostedEvents();

private:
    bool isMainThreadEventDispatcher();
    bool isSecondaryThreadEventDispatcher();
    bool isValidEventDispatcher();
    static bool isValidEventDispatcherPointer(QEventDispatcherWasm *eventDispatcher);

    bool sendTimerEvents();
    bool sendNativeEvents(QEventLoop::ProcessEventsFlags flags);

    void handleNonAsyncifyErrorCases(QEventLoop::ProcessEventsFlags flags);

    bool wait(int timeout);
    void processEventsWait();
    void asyncifyWait(std::optional<std::chrono::milliseconds> timeout);
    bool secondaryThreadWait(std::optional<std::chrono::milliseconds> timeout);

    void updateNativeTimer();

    static QEventDispatcherWasm *g_mainThreadEventDispatcher;
    static std::shared_ptr<QWasmSuspendResumeControl> g_mainThreadSuspendResumeControl;

    bool m_interrupted = false;
    bool m_wakeup = false;

    std::unique_ptr<QTimerInfoList> m_timerInfo;
    std::chrono::time_point<std::chrono::steady_clock> m_timerTargetTime;

    std::unique_ptr<QWasmTimer> m_nativeTimer;
    std::unique_ptr<QWasmTimer> m_wakeupTimer;
    std::unique_ptr<QWasmTimer> m_suspendTimer;

    bool m_wakeFromSuspendTimer = false;
    bool m_isSendingNativeEvents = false;

#if QT_CONFIG(thread)
    std::mutex m_mutex;
    bool m_wakeUpCalled = false;
    std::condition_variable m_moreEvents;

    static QVector<QEventDispatcherWasm *> g_secondaryThreadEventDispatchers;
    static std::mutex g_staticDataMutex;

    // Note on mutex usage: the global g_staticDataMutex protects the global (g_ prefixed) data,
    // while the per eventdispatcher m_mutex protects the state accociated with blocking and waking
    // that eventdispatcher thread. The locking order is g_staticDataMutex first, then m_mutex.
#endif

    friend class QWasmSocket;
};

#endif // QEVENTDISPATCHER_WASM_P_H
