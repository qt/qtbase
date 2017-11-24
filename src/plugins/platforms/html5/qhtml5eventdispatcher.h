/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHTML5EVENTDISPATCHER_H
#define QHTML5EVENTDISPATCHER_H

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtEventDispatcherSupport/private/qunixeventdispatcher_qpa_p.h>

QT_BEGIN_NAMESPACE

//Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_EVENTDISPATHCER);

class QHtml5EventDispatcher : public QUnixEventDispatcherQPA
{
public:
    explicit QHtml5EventDispatcher(QObject *parent = 0);
    ~QHtml5EventDispatcher();
    void processEvents_emscripten();
    static void processEvents(void *eventloop);
protected:

    bool processEvents(QEventLoop::ProcessEventsFlags flags
                       = QEventLoop::EventLoopExec) override;
    bool hasPendingEvents() override;

//    void registerTimer(int timerId, int interval, Qt::TimerType timerType,
//                       QObject *object) override;
//    bool unregisterTimer(int timerId) override;
//    bool unregisterTimers(QObject *object) override;

//    void flush() override;
  //  void wakeUp() override;

private:
//    struct PepperTimerInfo {

//        PepperTimerInfo(){};
//        PepperTimerInfo(int timerId, int interval, Qt::TimerType timerType, QObject *object)
//            : timerId(timerId)
//            , interval(interval)
//            , timerType(timerType)
//            , object(object)
//        {
//        }
//        int timerId;
//        int interval;
//        Qt::TimerType timerType;
//        QObject *object;
//    };

  //  void startTimer(PepperTimerInfo info);
  //  void timerCallback(int32_t result, int32_t timerSerial);
  //  void scheduleProcessEvents();
   // void processEventsCallback(int32_t status);

//    int m_currentTimerSerial;
//    QHash<int, int> m_activeTimerIds;                // timer serial -> Qt timer id
//    QHash<int, int> m_activeTimerSerials;            // Qt timer id -> timer serial
//    QMultiHash<QObject *, int> m_activeObjectTimers; // QObject * -> Qt timer id
//    QHash<int, PepperTimerInfo> m_timerDetails;
//    pp::MessageLoop m_messageLoop;
//    pp::CompletionCallbackFactory<QHtml5EventDispatcher> m_completionCallbackFactory;
    bool m_hasPendingProcessEvents;
};

QT_END_NAMESPACE

#endif
