/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QWAITCONDITION_H
#define QWAITCONDITION_H

#include <QtCore/QDeadlineTimer>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(thread)

class QWaitConditionPrivate;
class QMutex;
class QReadWriteLock;

class Q_CORE_EXPORT QWaitCondition
{
public:
    QWaitCondition();
    ~QWaitCondition();

    bool wait(QMutex *lockedMutex,
              QDeadlineTimer deadline = QDeadlineTimer(QDeadlineTimer::Forever));
    bool wait(QMutex *lockedMutex, unsigned long time);

    bool wait(QReadWriteLock *lockedReadWriteLock,
              QDeadlineTimer deadline = QDeadlineTimer(QDeadlineTimer::Forever));
    bool wait(QReadWriteLock *lockedReadWriteLock, unsigned long time);

    void wakeOne();
    void wakeAll();

    void notify_one() { wakeOne(); }
    void notify_all() { wakeAll(); }

private:
    Q_DISABLE_COPY(QWaitCondition)

    QWaitConditionPrivate * d;
};

#else

class QMutex;
class QReadWriteLock;

class Q_CORE_EXPORT QWaitCondition
{
public:
    QWaitCondition() {}
    ~QWaitCondition() {}

    bool wait(QMutex *, QDeadlineTimer = QDeadlineTimer(QDeadlineTimer::Forever))
    { return true; }
    bool wait(QReadWriteLock *, QDeadlineTimer = QDeadlineTimer(QDeadlineTimer::Forever))
    { return true; }
    bool wait(QMutex *, unsigned long) { return true; }
    bool wait(QReadWriteLock *, unsigned long) { return true; }

    void wakeOne() {}
    void wakeAll() {}

    void notify_one() { wakeOne(); }
    void notify_all() { wakeAll(); }
};

#endif // QT_CONFIG(thread)

QT_END_NAMESPACE

#endif // QWAITCONDITION_H
