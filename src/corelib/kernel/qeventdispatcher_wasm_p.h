/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

private:
    bool isMainThreadEventDispatcher();
    bool isSecondaryThreadEventDispatcher();

    void handleApplicationExec();
    void handleDialogExec();
    void pollForNativeEvents();
    bool waitForForEvents();
    static void callProcessEvents(void *eventDispatcher);

    void processTimers();
    void updateNativeTimer();
    static void callProcessTimers(void *eventDispatcher);

#if QT_CONFIG(thread)
    void runOnMainThread(std::function<void(void)> fn);
#endif

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
    static std::mutex g_secondaryThreadEventDispatchersMutex;
#endif
};

#endif // QEVENTDISPATCHER_WASM_P_H
