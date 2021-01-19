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

#ifndef QEVENTLOOP_H
#define QEVENTLOOP_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


class QEventLoopPrivate;

class Q_CORE_EXPORT QEventLoop : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventLoop)

public:
    explicit QEventLoop(QObject *parent = nullptr);
    ~QEventLoop();

    enum ProcessEventsFlag {
        AllEvents = 0x00,
        ExcludeUserInputEvents = 0x01,
        ExcludeSocketNotifiers = 0x02,
        WaitForMoreEvents = 0x04,
        X11ExcludeTimers = 0x08,
        EventLoopExec = 0x20,
        DialogExec = 0x40
    };
    Q_DECLARE_FLAGS(ProcessEventsFlags, ProcessEventsFlag)

    bool processEvents(ProcessEventsFlags flags = AllEvents);
    void processEvents(ProcessEventsFlags flags, int maximumTime);

    int exec(ProcessEventsFlags flags = AllEvents);
    void exit(int returnCode = 0);
    bool isRunning() const;

    void wakeUp();

    bool event(QEvent *event) override;

public Q_SLOTS:
    void quit();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QEventLoop::ProcessEventsFlags)


class QEventLoopLockerPrivate;

class Q_CORE_EXPORT QEventLoopLocker
{
public:
    QEventLoopLocker();
    explicit QEventLoopLocker(QEventLoop *loop);
    explicit QEventLoopLocker(QThread *thread);
    ~QEventLoopLocker();

private:
    Q_DISABLE_COPY(QEventLoopLocker)
    QEventLoopLockerPrivate *d_ptr;
};

QT_END_NAMESPACE

#endif // QEVENTLOOP_H
