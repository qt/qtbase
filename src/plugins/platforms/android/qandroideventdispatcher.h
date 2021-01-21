/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
****************************************************************************/

#ifndef QANDROIDEVENTDISPATCHER_H
#define QANDROIDEVENTDISPATCHER_H

#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QtEventDispatcherSupport/private/qunixeventdispatcher_qpa_p.h>

class QAndroidEventDispatcher : public QUnixEventDispatcherQPA
{
    Q_OBJECT
public:
    explicit QAndroidEventDispatcher(QObject *parent = 0);
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
    QVector<QAndroidEventDispatcher *> m_dispatchers;
};


#endif // QANDROIDEVENTDISPATCHER_H
