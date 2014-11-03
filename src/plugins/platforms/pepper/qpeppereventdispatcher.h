/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#ifndef QPEPPEREVENTDISPATCHER_H
#define QPEPPEREVENTDISPATCHER_H

#include <QtCore/qhash.h>
#include <QtCore/qloggingcategory.h>
//#include <QtCore/private/qeventdispatcher_unix_p.h>

#include "../platformsupport/eventdispatchers/qunixeventdispatcher_qpa_p.h"
#include <ppapi/utility/completion_callback_factory.h>
#include <ppapi/cpp/message_loop.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_EVENTDISPATHCER);

class QPepperEventDispatcher : public QUnixEventDispatcherQPA
{
public:
    explicit QPepperEventDispatcher(QObject *parent = 0);
    ~QPepperEventDispatcher();

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);

    bool processEvents(QEventLoop::ProcessEventsFlags flags = QEventLoop::EventLoopExec);

    bool hasPendingEvents();

    void flush();
    void wakeup();
private:
    struct PepperTimerInfo {

        PepperTimerInfo() {};
        PepperTimerInfo(int timerId, int interval, Qt::TimerType timerType, QObject *object)
            :timerId(timerId), interval(interval), timerType(timerType), object(object) { }
        int timerId;
        int interval;
        Qt::TimerType timerType;
        QObject *object;
    };

    void startTimer(PepperTimerInfo info);
    void timerCallback(int32_t timerSerial);
    void scheduleProcessEvents();
    void processEventsCallback(int32_t status);

    int m_currentTimerSerial;
    QHash<int, int> m_activeTimerIds; // timer serial -> Qt timer id
    QHash<int, int> m_activeTimerSerials; // Qt timer id -> timer serial
    QMultiHash<QObject *, int> m_activeObjectTimers; // QObject * -> Qt timer id
    QHash<int, PepperTimerInfo> m_timerDetails;
    pp::MessageLoop messageLoop;
    pp::CompletionCallbackFactory<QPepperEventDispatcher> m_completionCallbackFactory;
};

QT_END_NAMESPACE

#endif
