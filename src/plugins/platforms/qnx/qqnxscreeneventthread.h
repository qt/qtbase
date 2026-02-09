// Copyright (C) 2017 QNX Software Systems. All rights reserved.
// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQNXSCREENEVENTTHREAD_H
#define QQNXSCREENEVENTTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QHash>

#include <screen/screen.h>
#include <sys/siginfo.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_OPAQUE_POINTER(screen_window_t)
Q_DECLARE_METATYPE(screen_window_t)

class QQnxScreenEventThread : public QThread
{
    Q_OBJECT

public:
    QQnxScreenEventThread(screen_context_t context);
    ~QQnxScreenEventThread();

    screen_context_t context() const { return m_screenContext; }
    void armEventsPending(int count);
    bool registerUpdateNotification(screen_window_t window);
    void unregisterUpdateNotification(screen_window_t window);

protected:
    void run() override;

Q_SIGNALS:
    void eventsPending();
    void postEventReceived(screen_window_t window);

private:
    void handleScreenPulse(const struct _pulse &msg);
    void handleArmPulse(const struct _pulse &msg);
    void handlePostPulse(const struct _pulse &msg);
    void handlePulse(const struct _pulse &msg);
    void shutdown();

    int m_channelId;
    int m_connectionId;
    struct sigevent m_screenEvent;
    QHash<screen_window_t, struct sigevent> m_postEvents;
    screen_context_t m_screenContext;
    bool m_emitNeededOnNextScreenPulse = true;
    int m_screenPulsesSinceLastArmPulse = 0;
};

QT_END_NAMESPACE

#endif // QQNXSCREENEVENTTHREAD_H
