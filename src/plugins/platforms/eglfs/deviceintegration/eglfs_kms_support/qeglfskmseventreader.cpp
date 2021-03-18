/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfskmseventreader.h"
#include "qeglfskmsdevice.h"
#include <QSocketNotifier>
#include <QCoreApplication>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

static void pageFlipHandler(int fd, unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec, void *user_data)
{
    Q_UNUSED(fd);
    Q_UNUSED(sequence);
    Q_UNUSED(tv_sec);
    Q_UNUSED(tv_usec);

    QEglFSKmsEventReaderThread *t = static_cast<QEglFSKmsEventReaderThread *>(QThread::currentThread());
    t->eventHost()->handlePageFlipCompleted(user_data);
}

class RegisterWaitFlipEvent : public QEvent
{
public:
    static const QEvent::Type TYPE = QEvent::Type(QEvent::User + 1);
    RegisterWaitFlipEvent(void *key, QMutex *mutex, QWaitCondition *cond)
        : QEvent(TYPE), key(key), mutex(mutex), cond(cond)
    { }
    void *key;
    QMutex *mutex;
    QWaitCondition *cond;
};

bool QEglFSKmsEventHost::event(QEvent *event)
{
    if (event->type() == RegisterWaitFlipEvent::TYPE) {
        RegisterWaitFlipEvent *e = static_cast<RegisterWaitFlipEvent *>(event);
        PendingFlipWait *p = &pendingFlipWaits[0];
        PendingFlipWait *end = p + MAX_FLIPS;
        while (p < end) {
            if (!p->key) {
                p->key = e->key;
                p->mutex = e->mutex;
                p->cond = e->cond;
                updateStatus();
                return true;
            }
            ++p;
        }
        qWarning("Cannot queue page flip wait (more than %d screens?)", MAX_FLIPS);
        e->mutex->lock();
        e->cond->wakeOne();
        e->mutex->unlock();
        return true;
    }
    return QObject::event(event);
}

void QEglFSKmsEventHost::updateStatus()
{
    void **begin = &completedFlips[0];
    void **end = begin + MAX_FLIPS;

    for (int i = 0; i < MAX_FLIPS; ++i) {
        PendingFlipWait *w = pendingFlipWaits + i;
        if (!w->key)
            continue;

        void **p = begin;
        while (p < end) {
            if (*p == w->key) {
                *p = nullptr;
                w->key = nullptr;
                w->mutex->lock();
                w->cond->wakeOne();
                w->mutex->unlock();
                return;
            }
            ++p;
        }
    }
}

void QEglFSKmsEventHost::handlePageFlipCompleted(void *key)
{
    void **begin = &completedFlips[0];
    void **end = begin + MAX_FLIPS;
    void **p = begin;
    while (p < end) {
        if (*p == key) {
            updateStatus();
            return;
        }
        ++p;
    }
    p = begin;
    while (p < end) {
        if (!*p) {
            *p = key;
            updateStatus();
            return;
        }
        ++p;
    }
    qWarning("Cannot store page flip status (more than %d screens?)", MAX_FLIPS);
}

void QEglFSKmsEventReaderThread::run()
{
    qCDebug(qLcEglfsKmsDebug, "Event reader thread: entering event loop");

    QSocketNotifier notifier(m_fd, QSocketNotifier::Read);
    QObject::connect(&notifier, &QSocketNotifier::activated, &notifier, [this] {
        drmEventContext drmEvent;
        memset(&drmEvent, 0, sizeof(drmEvent));
        drmEvent.version = 2;
        drmEvent.vblank_handler = nullptr;
        drmEvent.page_flip_handler = pageFlipHandler;
        drmHandleEvent(m_fd, &drmEvent);
    });

    exec();

    m_ev.moveToThread(thread()); // move back to the thread where m_ev was created

    qCDebug(qLcEglfsKmsDebug, "Event reader thread: event loop stopped");
}

QEglFSKmsEventReader::~QEglFSKmsEventReader()
{
    destroy();
}

void QEglFSKmsEventReader::create(QEglFSKmsDevice *device)
{
    destroy();

    if (!device)
        return;

    m_device = device;

    qCDebug(qLcEglfsKmsDebug, "Initalizing event reader for device %p fd %d",
            m_device, m_device->fd());

    m_thread = new QEglFSKmsEventReaderThread(m_device->fd());
    m_thread->start();

    // Change thread affinity for the event host, so that postEvent()
    // goes through the event reader thread's event loop for that object.
    m_thread->eventHost()->moveToThread(m_thread);
}

void QEglFSKmsEventReader::destroy()
{
    if (!m_device)
        return;

    qCDebug(qLcEglfsKmsDebug, "Stopping event reader for device %p", m_device);

    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
        delete m_thread;
        m_thread = nullptr;
    }

    m_device = nullptr;
}

void QEglFSKmsEventReader::startWaitFlip(void *key, QMutex *mutex, QWaitCondition *cond)
{
    if (m_thread) {
        QCoreApplication::postEvent(m_thread->eventHost(),
                                    new RegisterWaitFlipEvent(key, mutex, cond));
    }
}

QT_END_NAMESPACE
