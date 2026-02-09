// Copyright (C) 2017 QNX Software Systems. All rights reserved.
// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqnxglobal.h"

#include "qqnxscreeneventthread.h"
#include "qqnxscreeneventhandler.h"

#include <QtCore/QDebug>

#include <errno.h>
#include <unistd.h>

#include <cctype>

static const int c_screenCode = _PULSE_CODE_MINAVAIL + 0;
static const int c_armCode = _PULSE_CODE_MINAVAIL + 1;
static const int c_quitCode = _PULSE_CODE_MINAVAIL + 2;
static const int c_postCode = _PULSE_CODE_MINAVAIL + 3;

#if !defined(screen_register_event)
int screen_register_event(screen_context_t, struct sigevent *event)
{
    return MsgRegisterEvent(event, -1);
}

int screen_unregister_event(struct sigevent *event)
{
    return MsgUnregisterEvent(event);
}
#endif

QQnxScreenEventThread::QQnxScreenEventThread(screen_context_t context)
    : QThread()
    , m_screenContext(context)
{
    m_channelId = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK | _NTO_CHF_PRIVATE);
    if (m_channelId == -1) {
        qFatal("QQnxScreenEventThread: Can't continue without a channel");
    }

    m_connectionId = ConnectAttach(0, 0, m_channelId, _NTO_SIDE_CHANNEL, 0);
    if (m_connectionId == -1) {
        ChannelDestroy(m_channelId);
        qFatal("QQnxScreenEventThread: Can't continue without a channel connection");
    }

    SIGEV_PULSE_INIT(&m_screenEvent, m_connectionId, SIGEV_PULSE_PRIO_INHERIT, c_screenCode, 0);
    if (screen_register_event(m_screenContext, &m_screenEvent) == -1) {
        ConnectDetach(m_connectionId);
        ChannelDestroy(m_channelId);
        qFatal("QQnxScreenEventThread: Can't continue without a registered event");
    }

    screen_notify(m_screenContext, SCREEN_NOTIFY_EVENT, nullptr, &m_screenEvent);
    qRegisterMetaType<screen_window_t>("screen_window_t");
}

QQnxScreenEventThread::~QQnxScreenEventThread()
{
    // block until thread terminates
    shutdown();

    screen_notify(m_screenContext, SCREEN_NOTIFY_EVENT, nullptr, nullptr);
    screen_unregister_event(&m_screenEvent);
    ConnectDetach(m_connectionId);
    ChannelDestroy(m_channelId);
}

void QQnxScreenEventThread::run()
{
    qCDebug(lcQpaScreenEvents) << "Screen event thread started";

    while (1) {
        struct _pulse msg;
        memset(&msg, 0, sizeof(msg));
        int receiveId = MsgReceive(m_channelId, &msg, sizeof(msg), nullptr);
        if (receiveId == 0 && msg.code == c_quitCode)
            break;
        else if (receiveId == 0)
            handlePulse(msg);
        else if (receiveId > 0)
            qWarning() << "Unexpected message" << msg.code;
        else
            qWarning() << "MsgReceive error" << strerror(errno);
    }

    qCDebug(lcQpaScreenEvents) << "Screen event thread stopped";
}

void QQnxScreenEventThread::armEventsPending(int count)
{
    MsgSendPulse(m_connectionId, SIGEV_PULSE_PRIO_INHERIT, c_armCode, count);
}

bool QQnxScreenEventThread::registerUpdateNotification(screen_window_t window)
{
    struct sigevent evt;
    SIGEV_PULSE_INIT(&evt, m_connectionId, SIGEV_PULSE_PRIO_INHERIT, c_postCode, 0);
    evt.sigev_value.sival_ptr = window;
    if (screen_register_event(m_screenContext, &evt) == -1) {
        qWarning() << "QQnxScreenEventThread: Failed to register post event for window" << window;
        return false;
    }
    int rc = screen_notify(m_screenContext, SCREEN_NOTIFY_UPDATE, window, &evt);
    qCDebug(lcQpaScreenEvents) << "registerUpdateNotification window=" << window << "rc=" << rc;
    if (rc != 0) {
        qWarning() << "screen_notify(SCREEN_NOTIFY_UPDATE) failed for window=" << window << strerror(errno);
        screen_unregister_event(&evt);
        return false;
    }
    m_postEvents.insert(window, evt);
    return true;
}

void QQnxScreenEventThread::unregisterUpdateNotification(screen_window_t window)
{
    int rc = screen_notify(m_screenContext, SCREEN_NOTIFY_UPDATE, window, nullptr);
    qCDebug(lcQpaScreenEvents) << "unregisterUpdateNotification window=" << window << "rc=" << rc;
    if (m_postEvents.contains(window)) {
        screen_unregister_event(&m_postEvents[window]);
        m_postEvents.remove(window);
    }
}

void QQnxScreenEventThread::handlePostPulse(const struct _pulse &msg)
{
    screen_window_t window = static_cast<screen_window_t>(msg.value.sival_ptr);
    Q_EMIT postEventReceived(window);
}

void QQnxScreenEventThread::handleScreenPulse(const struct _pulse &msg)
{
    Q_UNUSED(msg);

    ++m_screenPulsesSinceLastArmPulse;
    if (m_emitNeededOnNextScreenPulse) {
        m_emitNeededOnNextScreenPulse = false;
        Q_EMIT eventsPending();
    }
}

void QQnxScreenEventThread::handleArmPulse(const struct _pulse &msg)
{
    if (msg.value.sival_int == 0 && m_screenPulsesSinceLastArmPulse == 0) {
        m_emitNeededOnNextScreenPulse = true;
    } else {
        m_screenPulsesSinceLastArmPulse = 0;
        m_emitNeededOnNextScreenPulse = false;
        Q_EMIT eventsPending();
    }
}

void QQnxScreenEventThread::handlePulse(const struct _pulse &msg)
{
    if (msg.code == c_screenCode)
        handleScreenPulse(msg);
    else if (msg.code == c_armCode)
        handleArmPulse(msg);
    else if (msg.code == c_postCode)
        handlePostPulse(msg);
    else
        qWarning() << "Unexpected pulse" << msg.code;
}

void QQnxScreenEventThread::shutdown()
{
    MsgSendPulse(m_connectionId, SIGEV_PULSE_PRIO_INHERIT, c_quitCode, 0);

    qCDebug(lcQpaScreenEvents) << "Screen event thread shutdown begin";

    // block until thread terminates
    wait();

    qCDebug(lcQpaScreenEvents) << "Screen event thread shutdown end";
}
