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

#ifndef QABSTRACTEVENTDISPATCHER_H
#define QABSTRACTEVENTDISPATCHER_H

#include <QtCore/qobject.h>
#include <QtCore/qeventloop.h>

QT_BEGIN_NAMESPACE

class QAbstractNativeEventFilter;
class QAbstractEventDispatcherPrivate;
class QSocketNotifier;

class Q_CORE_EXPORT QAbstractEventDispatcher : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAbstractEventDispatcher)

public:
    struct TimerInfo
    {
        int timerId;
        int interval;
        Qt::TimerType timerType;

        inline TimerInfo(int id, int i, Qt::TimerType t)
            : timerId(id), interval(i), timerType(t)
        { }
    };

    explicit QAbstractEventDispatcher(QObject *parent = nullptr);
    ~QAbstractEventDispatcher();

    static QAbstractEventDispatcher *instance(QThread *thread = nullptr);

    virtual bool processEvents(QEventLoop::ProcessEventsFlags flags) = 0;

    virtual void registerSocketNotifier(QSocketNotifier *notifier) = 0;
    virtual void unregisterSocketNotifier(QSocketNotifier *notifier) = 0;

    int registerTimer(qint64 interval, Qt::TimerType timerType, QObject *object);
    virtual void registerTimer(int timerId, qint64 interval, Qt::TimerType timerType, QObject *object) = 0;
    virtual bool unregisterTimer(int timerId) = 0;
    virtual bool unregisterTimers(QObject *object) = 0;
    virtual QList<TimerInfo> registeredTimers(QObject *object) const = 0;

    virtual int remainingTime(int timerId) = 0;

    virtual void wakeUp() = 0;
    virtual void interrupt() = 0;

    virtual void startingUp();
    virtual void closingDown();

    void installNativeEventFilter(QAbstractNativeEventFilter *filterObj);
    void removeNativeEventFilter(QAbstractNativeEventFilter *filterObj);
    bool filterNativeEvent(const QByteArray &eventType, void *message, qintptr *result);

Q_SIGNALS:
    void aboutToBlock();
    void awake();

protected:
    QAbstractEventDispatcher(QAbstractEventDispatcherPrivate &,
                             QObject *parent);
};

Q_DECLARE_TYPEINFO(QAbstractEventDispatcher::TimerInfo, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QABSTRACTEVENTDISPATCHER_H
