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

#include "qhtml5eventdispatcher.h"

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

QHtml5EventDispatcher::QHtml5EventDispatcher(QObject *parent)
    : QUnixEventDispatcherQPA(parent)
    , m_hasPendingProcessEvents(false)
{
  qDebug() << "<<<<<<<<<<<<<<<<<<<< QHtml5EventDispatcher()";
}

QHtml5EventDispatcher::~QHtml5EventDispatcher() {}

bool QHtml5EventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    bool processed = false;

    // We need to give the control back to the browser due to lack of PTHREADS
    // Limit the number of events that may be processed at the time
    int maxProcessedEvents = 2;
    int processedCount = 0;
    do {
        processed = QUnixEventDispatcherQPA::processEvents(flags);
        processedCount += 1;
    } while (processed && hasPendingEvents() && processedCount < maxProcessedEvents);
    // Schedule a new processing loop if we still have events pending
    if (hasPendingEvents()) {
     //   qDebug() << "scheduleProcessEvents";
        QCoreApplication::processEvents();
    //    scheduleProcessEvents();
    }
    return true;
}

bool QHtml5EventDispatcher::hasPendingEvents()
{
    return QUnixEventDispatcherQPA::hasPendingEvents();
}

//void QHtml5EventDispatcher::timerCallback(int32_t result, int32_t timerSerial)
//{
//    Q_UNUSED(result);
//    qCDebug(QT_PLATFORM_PEPPER_EVENTDISPATHCER) << "timerCallback" << timerSerial;

//    // The timer might have been unregistered. In that case don't fire.
//    if (!m_activeTimerIds.contains(timerSerial))
//        return;

//    // Get the timer info for the timerSerial/timerID.
//    int timerId = m_activeTimerIds.value(timerSerial);
//    const PepperTimerInfo &info = m_timerDetails.value(timerId);

//    // Send the timer event
//    QTimerEvent e(info.timerId);
//    QCoreApplication::sendEvent(info.object, &e);
//    processEvents();

//    // After running Qt and application code the timer may have been unregistered,
//    // and the timer id may have been reused. The timerSerial will hower not
//    // be reused; use that to determine if the timer is active.
//    if (m_activeTimerIds.contains(timerSerial))
//        startTimer(info);

//    // one serial number per callback, we are done with this one.
//    m_activeTimerIds.remove(timerSerial);
//}

//void QHtml5EventDispatcher::scheduleProcessEvents()
//{
//    qDebug()  << "scheduleProcessEvents" << m_hasPendingProcessEvents;
//    if (!m_hasPendingProcessEvents) {
//        m_hasPendingProcessEvents = true;
//        pp::CompletionCallback processEvents
//            = m_completionCallbackFactory.NewCallback(&QHtml5EventDispatcher::processEventsCallback);
//        int32_t result = m_messageLoop.PostWork(processEvents);
//        if (result != PP_OK)
//            qDebug()  << "scheduleProcessEvents PostWork error"
//                                                        << result;
//    }
//}

//void QHtml5EventDispatcher::processEventsCallback(int32_t status)
//{
//    Q_UNUSED(status);
//   qDebug()  << "processEvents";
//    m_hasPendingProcessEvents = false;

//    processEvents();
//}

