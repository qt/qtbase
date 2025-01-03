// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfskmseventreader_p.h"
#include "qeglfskmsdevice_p.h"
#include "qeglfskmsscreen_p.h"
#include <QSocketNotifier>
#include <QCoreApplication>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

static void pageFlipHandler(int fd, unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec, void *user_data)
{
    Q_UNUSED(fd);

    QEglFSKmsEventReaderThread *t = static_cast<QEglFSKmsEventReaderThread *>(QThread::currentThread());
    t->eventHost()->handlePageFlipCompleted(user_data);

    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(user_data);
    screen->pageFlipped(sequence, tv_sec, tv_usec);
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

    qCDebug(qLcEglfsKmsDebug, "Initializing event reader for device %p fd %d",
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
