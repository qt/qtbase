
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSEVENTDISPATCHER_H
#define QOHOSEVENTDISPATCHER_H

#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QtGui/private/qunixeventdispatcher_qpa_p.h>

class QOhosEventDispatcher : public QUnixEventDispatcherQPA
{
    Q_OBJECT
public:
    explicit QOhosEventDispatcher(QObject *parent = 0);
    ~QOhosEventDispatcher();
    void start();
    void stop();

    void goingToStop(bool stop);

protected:
    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

private:
    QAtomicInt m_stopRequest;
    QAtomicInteger<bool> m_goingToStop;
    QSemaphore m_semaphore;
};

class QOhosEventDispatcherStopper
{
public:
    static QOhosEventDispatcherStopper *instance();
    static bool stopped() {return !instance()->m_started.loadRelaxed(); }
    void startAll();
    void stopAll();
    void addEventDispatcher(QOhosEventDispatcher *dispatcher);
    void removeEventDispatcher(QOhosEventDispatcher *dispatcher);
    void goingToStop(bool stop);

private:
    QMutex m_mutex;
    QAtomicInt m_started = 1;
    QVector<QOhosEventDispatcher *> m_dispatchers;
};


#endif // QOHOSEVENTDISPATCHER_H
