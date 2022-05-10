// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDEVENTDISPATCHER_H
#define QANDROIDEVENTDISPATCHER_H

#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QtGui/private/qunixeventdispatcher_qpa_p.h>

class QAndroidEventDispatcher : public QUnixEventDispatcherQPA
{
    Q_OBJECT
public:
    explicit QAndroidEventDispatcher(QObject *parent = nullptr);
    ~QAndroidEventDispatcher();
    void start();
    void stop();

    void goingToStop(bool stop);

protected:
    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

private:
    QAtomicInt m_stopRequest;
    QAtomicInt m_goingToStop;
    QSemaphore m_semaphore;
};

class QAndroidEventDispatcherStopper
{
public:
    static QAndroidEventDispatcherStopper *instance();
    static bool stopped() {return !instance()->m_started.loadRelaxed(); }
    void startAll();
    void stopAll();
    void addEventDispatcher(QAndroidEventDispatcher *dispatcher);
    void removeEventDispatcher(QAndroidEventDispatcher *dispatcher);
    void goingToStop(bool stop);

private:
    QMutex m_mutex;
    QAtomicInt m_started = 1;
    QList<QAndroidEventDispatcher *> m_dispatchers;
};


#endif // QANDROIDEVENTDISPATCHER_H
