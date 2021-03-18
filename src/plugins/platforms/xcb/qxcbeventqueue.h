/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
#ifndef QXCBEVENTQUEUE_H
#define QXCBEVENTQUEUE_H

#include <QtCore/QThread>
#include <QtCore/QHash>
#include <QtCore/QEventLoop>
#include <QtCore/QVector>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include <xcb/xcb.h>

#include <atomic>

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
        PeekDefault = 0, // see qx11info_x11.h for docs
        PeekFromCachedIndex = 1,
        PeekRetainMatch = 2,
        PeekRemoveMatch = 3,
        PeekRemoveMatchContinue = 4
    };
    Q_DECLARE_FLAGS(PeekOptions, PeekOption)

    void run() override;

    bool isEmpty() const { return m_head == m_flushedTail && !m_head->event; }
    xcb_generic_event_t *takeFirst(QEventLoop::ProcessEventsFlags flags);
    xcb_generic_event_t *takeFirst();
    void flushBufferedEvents();
    void wakeUpDispatcher();

    // ### peek() and peekEventQueue() could be unified. Note that peekEventQueue()
    // is public API exposed via QX11Extras/QX11Info.
    template<typename Peeker>
    xcb_generic_event_t *peek(Peeker &&peeker) {
        return peek(PeekRemoveMatch, std::forward<Peeker>(peeker));
    }
    template<typename Peeker>
    inline xcb_generic_event_t *peek(PeekOption config, Peeker &&peeker);

    qint32 generatePeekerId();
    bool removePeekerId(qint32 peekerId);

    using PeekerCallback = bool (*)(xcb_generic_event_t *event, void *peekerData);
    bool peekEventQueue(PeekerCallback peeker, void *peekerData = nullptr,
                        PeekOptions option = PeekDefault, qint32 peekerId = -1);

    const QXcbEventNode *flushedTail() const { return m_flushedTail; }
    void waitForNewEvents(const QXcbEventNode *sinceFlushedTail, unsigned long time = ULONG_MAX);

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

    QVector<xcb_generic_event_t *> m_inputEvents;

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
            if (option == PeekRemoveMatch || option == PeekRemoveMatchContinue)
                node->event = nullptr;
            if (option != PeekRemoveMatchContinue)
                return event;
        }
        if (node == m_flushedTail)
            break;
        node = node->next;
    } while (true);

    return nullptr;
}

QT_END_NAMESPACE

#endif
