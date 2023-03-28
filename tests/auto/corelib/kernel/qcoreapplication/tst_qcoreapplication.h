// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TST_QCOREAPPLICATION_H
#define TST_QCOREAPPLICATION_H

#include <QtCore/QtCore>

class tst_QCoreApplication: public QObject
{
    Q_OBJECT
private slots:
    void sendEventsOnProcessEvents(); // this must be the first test
    void getSetCheck();
    void qAppName();
    void qAppVersion();
    void argc();
    void postEvent();
    void removePostedEvents();
#if QT_CONFIG(thread)
    void deliverInDefinedOrder();
#endif
    void applicationPid();
#ifdef QT_BUILD_INTERNAL
    void globalPostedEventsCount();
#endif
    void processEventsAlwaysSendsPostedEvents();
#ifdef Q_OS_WIN
    void sendPostedEventsInNativeLoop();
#endif
    void quit();
    void reexec();
    void execAfterExit();
    void eventLoopExecAfterExit();
    void customEventDispatcher();
    void testQuitLock();
    void QTBUG31606_QEventDestructorDeadLock();
    void applicationEventFilters_mainThread();
    void applicationEventFilters_auxThread();
    void threadedEventDelivery_data();
    void threadedEventDelivery();
    void testTrWithPercantegeAtTheEnd();
#if QT_CONFIG(library)
    void addRemoveLibPaths();
#endif
    void theMainThread();
};

#endif // TST_QCOREAPPLICATION_H
