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

#include <mutex>
#include <optional>
#include <tuple>

#include <emscripten/proxying.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcEventDispatcher);
Q_DECLARE_LOGGING_CATEGORY(lcEventDispatcherTimers)

class Q_CORE_EXPORT QEventDispatcherWasm : public QAbstractEventDispatcher
{
    Q_OBJECT
public:
    QEventDispatcherWasm();
    ~QEventDispatcherWasm();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

    void registerSocketNotifier(QSocketNotifier *notifier) override;
    void unregisterSocketNotifier(QSocketNotifier *notifier) override;

    void registerTimer(int timerId, qint64 interval, Qt::TimerType timerType, QObject *object)  override;
    bool unregisterTimer(int timerId) override;
    bool unregisterTimers(QObject *object) override;
    QList<QAbstractEventDispatcher::TimerInfo> registeredTimers(QObject *object) const override;
    int remainingTime(int timerId) override;

    void interrupt() override;
    void wakeUp() override;

    static void socketSelect(int timeout, int socket, bool waitForRead, bool waitForWrite,
                            bool *selectForRead, bool *selectForWrite, bool *socketDisconnect);
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

    static void setEmscriptenSocketCallbacks();
    static void clearEmscriptenSocketCallbacks();
    static void socketError(int fd, int err, const char* msg, void *context);
    static void socketOpen(int fd, void *context);
    static void socketListen(int fd, void *context);
    static void socketConnection(int fd, void *context);
    static void socketMessage(int fd, void *context);
    static void socketClose(int fd, void *context);

    static void setSocketState(int socket, bool setReadyRead, bool setReadyWrite);
    static void clearSocketState(int socket);
    void waitForSocketState(int timeout, int socket, bool checkRead, bool checkWrite,
                            bool *selectForRead, bool *selectForWrite, bool *socketDisconnect);

    static void run(std::function<void(void)> fn);
    static void runAsync(std::function<void(void)> fn);
    static void runOnMainThread(std::function<void(void)> fn);
    static void runOnMainThreadAsync(std::function<void(void)> fn);

    static QEventDispatcherWasm *g_mainThreadEventDispatcher;

    bool m_interrupted = false;
    bool m_processTimers = false;
    bool m_pendingProcessEvents = false;

    QTimerInfoList *m_timerInfo = new QTimerInfoList();
    long m_timerId = 0;
    uint64_t m_timerTargetTime = 0;

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

    static std::multimap<int, QSocketNotifier *> g_socketNotifiers;

    struct SocketReadyState {
        QEventDispatcherWasm *waiter = nullptr;
        bool waitForReadyRead = false;
        bool waitForReadyWrite = false;
        bool readyRead = false;
        bool readyWrite = false;
    };
    static std::map<int, SocketReadyState> g_socketState;
};

#endif // QEVENTDISPATCHER_WASM_P_H
