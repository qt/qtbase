// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QtCore/QThread>
#include <QtCore/QHash>
#include <QtCore/QEventLoop>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include <xcb/xcb.h>

#include <atomic>
#include <limits>

QT_BEGIN_NAMESPACE

struct QXcbEventNode {
    QXcbEventNode(xcb_generic_event_t *e = nullptr)
        : event(e) { }

    xcb_generic_event_t *event;
    QXcbEventNode *next = nullptr;
    bool fromHeap = false;
};

class QXcbConnection;
class QAbstractEventDispatcher;

class QXcbEventQueue : public QThread
{
    Q_OBJECT
public:
    QXcbEventQueue(QXcbConnection *connection);
    ~QXcbEventQueue();

    enum { PoolSize = 100 }; // 2.4 kB with 100 nodes

    enum PeekOption {
        // See qx11info_x11.cpp in X11 Extras module.
        PeekDefault = 0,
        // See qx11info_x11.cpp in X11 Extras module.
        PeekFromCachedIndex = 1,
        // Used by the event compression algorithms to determine if
        // the currently processed event (which has been already dequeued)
        // can be compressed. Returns from the QXcbEventQueue::peek()
        // on the first match.
        PeekRetainMatch = 2,
        // Marks the event in the node as "nullptr". The actual
        // node remains in the queue. The nodes are unlinked only
        // by dequeueNode(). Returns from the QXcbEventQueue::peek()
        // on the first match.
        PeekConsumeMatch = 3,
        // Same as above, but continues to the next node in the
        // queue. Repeats this until the flushed tailed node has
        // been reached.
        PeekConsumeMatchAndContinue = 4
    };
    Q_DECLARE_FLAGS(PeekOptions, PeekOption)

    void run() override;

    bool isEmpty() const { return m_head == m_flushedTail && !m_head->event; }
    xcb_generic_event_t *takeFirst(QEventLoop::ProcessEventsFlags flags);
    xcb_generic_event_t *takeFirst();
    void flushBufferedEvents();
    void wakeUpDispatcher();

    // ### peek() and peekEventQueue() could be unified. Note that peekEventQueue()
    // is public API exposed via QX11Extras/QX11Info. PeekOption could be reworked to
    // have values that can be OR-ed together.
    template<typename Peeker>
    xcb_generic_event_t *peek(Peeker &&peeker) {
        return peek(PeekConsumeMatch, std::forward<Peeker>(peeker));
    }
    template<typename Peeker>
    inline xcb_generic_event_t *peek(PeekOption config, Peeker &&peeker);

    qint32 generatePeekerId();
    bool removePeekerId(qint32 peekerId);

    using PeekerCallback = bool (*)(xcb_generic_event_t *event, void *peekerData);
    bool peekEventQueue(PeekerCallback peeker, void *peekerData = nullptr,
                        PeekOptions option = PeekDefault, qint32 peekerId = -1);

    const QXcbEventNode *flushedTail() const { return m_flushedTail; }
    void waitForNewEvents(const QXcbEventNode *sinceFlushedTail,
                          unsigned long time = (std::numeric_limits<unsigned long>::max)());

private:
    QXcbEventNode *qXcbEventNodeFactory(xcb_generic_event_t *event);
    void dequeueNode();

    void sendCloseConnectionEvent() const;
    bool isCloseConnectionEvent(const xcb_generic_event_t *event);

    QXcbEventNode *m_head = nullptr;
    QXcbEventNode *m_flushedTail = nullptr;
    std::atomic<QXcbEventNode *> m_tail { nullptr };
    std::atomic_uint m_nodesRestored { 0 };

    QXcbConnection *m_connection = nullptr;
    bool m_closeConnectionDetected = false;

    uint m_freeNodes = PoolSize;
    uint m_poolIndex = 0;

    qint32 m_peekerIdSource = 0;
    bool m_queueModified = false;
    bool m_peekerIndexCacheDirty = false;
    QHash<qint32, QXcbEventNode *> m_peekerToNode;

    QList<xcb_generic_event_t *> m_inputEvents;

    // debug stats
    quint64 m_nodesOnHeap = 0;

    QMutex m_newEventsMutex;
    QWaitCondition m_newEventsCondition;
};

template<typename Peeker>
xcb_generic_event_t *QXcbEventQueue::peek(PeekOption option, Peeker &&peeker)
{
    flushBufferedEvents();
    if (isEmpty())
        return nullptr;

    QXcbEventNode *node = m_head;
    do {
        xcb_generic_event_t *event = node->event;
        if (event && peeker(event, event->response_type & ~0x80)) {
            if (option == PeekConsumeMatch || option == PeekConsumeMatchAndContinue)
                node->event = nullptr;

            if (option != PeekConsumeMatchAndContinue)
                return event;
        }
        if (node == m_flushedTail)
            break;
        node = node->next;
    } while (true);

    return nullptr;
}

QT_END_NAMESPACE
