// Copyright (C) 2017 QNX Software Systems. All rights reserved.
// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXSCREENEVENTTHREAD_H
#define QQNXSCREENEVENTTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QMutex>

#include <screen/screen.h>
#include <sys/siginfo.h>

QT_BEGIN_NAMESPACE

class QQnxScreenEventThread : public QThread
{
    Q_OBJECT

public:
    QQnxScreenEventThread(screen_context_t context);
    ~QQnxScreenEventThread();

    screen_context_t context() const { return m_screenContext; }
    void armEventsPending(int count);

protected:
    void run() override;

Q_SIGNALS:
    void eventsPending();

private:
    void handleScreenPulse(const struct _pulse &msg);
    void handleArmPulse(const struct _pulse &msg);
    void handlePulse(const struct _pulse &msg);
    void shutdown();

    int m_channelId;
    int m_connectionId;
    struct sigevent m_screenEvent;
    screen_context_t m_screenContext;
    bool m_emitNeededOnNextScreenPulse = true;
    int m_screenPulsesSinceLastArmPulse = 0;
};

QT_END_NAMESPACE

#endif // QQNXSCREENEVENTTHREAD_H
