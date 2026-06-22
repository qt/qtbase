// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosutils.h"
#include "qohoswatchdog.h"
#include <hicollie/hicollie.h>
#include <qarkui/qarkuiutils.h>
#include <qohosbigdataeventlogging.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qobject.h>
#include <functional>
#include <mutex>

namespace ch = std::chrono;

QT_BEGIN_NAMESPACE

namespace QtOhosWatchdog {

namespace {

constexpr int resetRatio = 2;

constexpr auto bigDataEnterFreezeStateEventName = "enterFreezeState";
constexpr auto bigDataRecoveredFromFreezeStateEventName = "recoveredFromFreezeState";

constexpr auto bigDataEventDescriptionPropertyName = "eventDescription";
constexpr auto bigDataFreezeStateDurationPropertyName = "freezeStateDurationMs";

#ifdef SUPPORT_ASAN
constexpr auto checkIntervalTime = ch::seconds(45);
#else
constexpr auto checkIntervalTime = ch::seconds(3);
#endif

void logEnterFreezeStateBigDataEvent(ch::time_point<ch::system_clock> eventTime)
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);

    auto eventBuilder = QtOhos::makeBigEventLoggingEventBuilder(
        bigDataEnterFreezeStateEventName, ::EventType::STATISTIC, eventTime);
    eventBuilder->addParam(bigDataEventDescriptionPropertyName, "The application triggered a 6 second freeze");
    eventBuilder->buildEvent()->trySend();
}

void logRecoveredFromFreezeStateBigDataEvent(
    ch::time_point<ch::system_clock> eventTime, ch::system_clock::duration freezeStateDuration)
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);

    auto eventBuilder = QtOhos::makeBigEventLoggingEventBuilder(
        bigDataRecoveredFromFreezeStateEventName, ::EventType::STATISTIC, eventTime);
    eventBuilder->addParam(bigDataEventDescriptionPropertyName, "Recovered from the frozen state");
    eventBuilder->addParam(
        bigDataFreezeStateDurationPropertyName,
        static_cast<std::int64_t>(ch::duration_cast<ch::milliseconds>(freezeStateDuration).count()));
    eventBuilder->buildEvent()->trySend();
}

class QtWatchdog : public QObject
{
public:
    QtWatchdog();

    void runHiCollieStuckDetectionTask();

    bool event(QEvent *ev) override;

private:
    void handleAppMainThreadAliveNotification();
    void reportStuckEvent();

    ch::steady_clock::time_point m_lastWatchTime;
    bool m_appMainThreadIsAlive = false;
    bool m_isSixSecondEvent = false;
    QOhosOptional<ch::system_clock::time_point> m_sixSecondEventDetectionTime;
};

QEvent::Type getCheckMainThreadIsAliveQEventType()
{
    static auto eventType = static_cast<QEvent::Type>(QEvent::registerEventType());
    return eventType;
}

QtWatchdog::QtWatchdog()
    : QObject()
{
    QCoreApplication::postEvent(this, new QEvent(getCheckMainThreadIsAliveQEventType()), Qt::HighEventPriority);
}

void QtWatchdog::handleAppMainThreadAliveNotification()
{
    qCDebug(QtForOhos, "QtWatchdog::handleAppMainThreadAliveNotification");
    m_appMainThreadIsAlive = true;
    m_isSixSecondEvent = false;
    if (m_sixSecondEventDetectionTime.has_value()) {
        const auto now = ch::system_clock::now();
        const auto elapsedTime = now - m_sixSecondEventDetectionTime.value();
        logRecoveredFromFreezeStateBigDataEvent(now, elapsedTime);
        m_sixSecondEventDetectionTime.reset();
    }
}

void QtWatchdog::runHiCollieStuckDetectionTask()
{
    qCDebug(QtForOhos, "QtWatchdog::runHiCollieStuckDetectionTask start");

    if (m_appMainThreadIsAlive) {
        qCDebug(QtForOhos, "QtWatchdog: m_appMainThreadIsAlive store false");
        m_appMainThreadIsAlive = false;
    } else {
        qCWarning(QtForOhos, "QtWatchdog: AppMainThread is not alive");
        reportStuckEvent();
    }

    QCoreApplication::postEvent(this, new QEvent(getCheckMainThreadIsAliveQEventType()), Qt::HighEventPriority);

    auto now = ch::steady_clock::now();
    if ((now - m_lastWatchTime) >= (checkIntervalTime / resetRatio))
        m_lastWatchTime = now;

    qCDebug(QtForOhos, "QtWatchdog::runHiCollieStuckDetectionTask end");
}

bool QtWatchdog::event(QEvent *ev)
{
    if (ev->type() == getCheckMainThreadIsAliveQEventType()) {
        qCDebug(QtForOhos, "QtWatchdog: CheckMainThreadIsAlive event start (%d)", static_cast<int>(getCheckMainThreadIsAliveQEventType()));
        handleAppMainThreadAliveNotification();
        qCDebug(QtForOhos, "QCoreApplication: CheckMainThreadIsAlive event end");
        return true;
    } else {
        return QObject::event(ev);
    }
}

void QtWatchdog::reportStuckEvent()
{
    auto now = ch::steady_clock::now();
    auto timeDelta = now - m_lastWatchTime;
    if (timeDelta > resetRatio * checkIntervalTime || timeDelta < checkIntervalTime / resetRatio) {
        qCWarning(
            QtForOhos,
            "QtWatchdog: Thread may be blocked, do not report this time. currTime: %.3f, lastTime: %.3f",
            ch::duration<double>(now.time_since_epoch()).count(),
            ch::duration<double>(m_lastWatchTime.time_since_epoch()).count());
        return;
    }

    qCDebug(QtForOhos, "QtWatchdog: calling OH_HiCollie_Report(), m_isSixSecondEvent = %d", m_isSixSecondEvent);
    HiCollie_ErrorCode reportRes = OH_HiCollie_Report(&m_isSixSecondEvent);
    if (reportRes == HICOLLIE_SUCCESS) {
        qCDebug(QtForOhos, "QtWatchdog: after OH_HiCollie_Report(), m_isSixSecondEvent = %d", m_isSixSecondEvent);
        if (m_isSixSecondEvent && !m_sixSecondEventDetectionTime.has_value()) {
            m_sixSecondEventDetectionTime = ch::system_clock::now();
            logEnterFreezeStateBigDataEvent(m_sixSecondEventDetectionTime.value());
        }
    } else {
        qCWarning(QtForOhos, "QtWatchdog: OH_HiCollie_Report() failed with code %d", static_cast<int>(reportRes));
    }
}

void runWithQtWatchdogSharedPtr(const std::function<void(std::shared_ptr<QtWatchdog> &)> &runFunc)
{
    static std::mutex runMutex;
    static std::shared_ptr<QtWatchdog> qtWatchdogPtr;

    {
        std::lock_guard<std::mutex> runLock(runMutex);
        runFunc(qtWatchdogPtr);
    }
}

}

std::shared_ptr<void> makeWatchdog()
{
    runWithQtWatchdogSharedPtr(
        [](auto &watchdogSharedPtr) {
            watchdogSharedPtr = std::make_shared<QtWatchdog>();
        });

    auto watchdogHandle = QtOhos::makeDestroyNotifier(
        []() {
            runWithQtWatchdogSharedPtr(
                [](auto &watchdogSharedPtr) {
                    watchdogSharedPtr.reset();
                });
        });

    HiCollie_ErrorCode stuckDetectionInitRes = OH_HiCollie_Init_StuckDetection(
        []() {
            runWithQtWatchdogSharedPtr(
                [](auto &watchdogSharedPtr) {
                    if (watchdogSharedPtr)
                        watchdogSharedPtr->runHiCollieStuckDetectionTask();
                });
        });

    std::shared_ptr<void> result;
    if (stuckDetectionInitRes == HICOLLIE_SUCCESS) {
        result = watchdogHandle;
    } else {
        qCWarning(
            QtForOhos,
            "OH_HiCollie_Init_StuckDetection() failed with code %d, disabling QtWatchdog",
            static_cast<int>(stuckDetectionInitRes));
        result = nullptr;
    }

    return result;
}

}

QT_END_NAMESPACE
