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
#include "qxcbeventqueue.h"
#include "qxcbconnection.h"

#include <QtCore/QObject>
#include <QtCore/QCoreApplication>
#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QMutex>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

static QBasicMutex qAppExiting;
static bool dispatcherOwnerDestructing = false;

/*!
    \class QXcbEventQueue
    \internal

    Lock-free event passing:

    The lock-free solution uses a singly-linked list to pass events from the
    reader thread to the main thread. An atomic operation is used to sync the
    tail node of the list between threads. The reader thread takes special care
    when accessing the tail node. It does not dequeue the last node and does not
    access (read or write) the tail node's 'next' member. This lets the reader
    add more items at the same time as the main thread is dequeuing nodes from
    the head. A custom linked list implementation is used, because QLinkedList
    does not have any thread-safety guarantees and the custom list is more
    lightweight - no reference counting, back links, etc.

    Memory management:

    In a normally functioning application, XCB plugin won't buffer more than few
    batches of events, couple events per batch. Instead of constantly calling
    new / delete, we can create a pool of nodes that we reuse. The main thread
    uses an atomic operation to sync how many nodes have been restored (available
    for reuse). If at some point a user application will block the main thread
    for a long time, we might run out of nodes in the pool. Then we create nodes
    on a heap. These will be automatically "garbage collected" out of the linked
    list, once the main thread stops blocking.
*/

QXcbEventQueue::QXcbEventQueue(QXcbConnection *connection)
    : m_connection(connection)
{
    // When running test cases in auto tests, static variables are preserved
    // between test function runs, even if Q*Application object is destroyed.
    // Reset to default value to account for this.
    dispatcherOwnerDestructing = false;
    qAddPostRoutine([]() {
        QMutexLocker locker(&qAppExiting);
        dispatcherOwnerDestructing = true;
    });

    // Lets init the list with one node, so we don't have to check for
    // this special case in various places.
    m_head = m_flushedTail = qXcbEventNodeFactory(nullptr);
    m_tail.store(m_head, std::memory_order_release);

    start();
}

QXcbEventQueue::~QXcbEventQueue()
{
    if (isRunning()) {
        sendCloseConnectionEvent();
        wait();
    }

    flushBufferedEvents();
    while (xcb_generic_event_t *event = takeFirst(QEventLoop::AllEvents))
        free(event);

    if (m_head && m_head->fromHeap)
        delete m_head; // the deferred node

    qCDebug(lcQpaEventReader) << "nodes on heap:" << m_nodesOnHeap;
}

xcb_generic_event_t *QXcbEventQueue::takeFirst(QEventLoop::ProcessEventsFlags flags)
{
    // This is the level at which we were moving excluded user input events into
    // separate queue in Qt 4 (see qeventdispatcher_x11.cpp). In this case
    // QXcbEventQueue represents Xlib's internal event queue. In Qt 4, Xlib's
    // event queue peeking APIs would not see these events anymore, the same way
    // our peeking functions do not consider m_inputEvents. This design is
    // intentional to keep the same behavior. We could do filtering directly on
    // QXcbEventQueue, without the m_inputEvents, but it is not clear if it is
    // needed by anyone who peeks at the native event queue.

    bool excludeUserInputEvents = flags.testFlag(QEventLoop::ExcludeUserInputEvents);
    if (excludeUserInputEvents) {
        xcb_generic_event_t *event = nullptr;
        while ((event = takeFirst())) {
            if (m_connection->isUserInputEvent(event)) {
                m_inputEvents << event;
                continue;
            }
            break;
        }
        return event;
    }

    if (!m_inputEvents.isEmpty())
        return m_inputEvents.takeFirst();
    return takeFirst();
}

xcb_generic_event_t *QXcbEventQueue::takeFirst()
{
    if (isEmpty())
        return nullptr;

    xcb_generic_event_t *event = nullptr;
    do {
        event = m_head->event;
        if (m_head == m_flushedTail) {
            // defer dequeuing until next successful flush of events
            if (event) // check if not cleared already by some filter
                m_head->event = nullptr; // if not, clear it
        } else {
            dequeueNode();
            if (!event)
                continue; // consumed by filter or deferred node
        }
    } while (!isEmpty() && !event);

    m_queueModified = m_peekerIndexCacheDirty = true;

    return event;
}

void QXcbEventQueue::dequeueNode()
{
    QXcbEventNode *node = m_head;
    m_head = m_head->next;
    if (node->fromHeap)
        delete node;
    else
        m_nodesRestored.fetch_add(1, std::memory_order_release);
}

void QXcbEventQueue::flushBufferedEvents()
{
    m_flushedTail = m_tail.load(std::memory_order_acquire);
}

QXcbEventNode *QXcbEventQueue::qXcbEventNodeFactory(xcb_generic_event_t *event)
{
    static QXcbEventNode qXcbNodePool[PoolSize];

    if (m_freeNodes == 0) // out of nodes, check if the main thread has released any
        m_freeNodes = m_nodesRestored.exchange(0, std::memory_order_acquire);

    if (m_freeNodes) {
        m_freeNodes--;
        if (m_poolIndex == PoolSize) {
            // wrap back to the beginning, we always take and restore nodes in-order
            m_poolIndex = 0;
        }
        QXcbEventNode *node = &qXcbNodePool[m_poolIndex++];
        node->event = event;
        node->next = nullptr;
        return node;
    }

    // the main thread is not flushing events and thus the pool has become empty
    auto node = new QXcbEventNode(event);
    node->fromHeap = true;
    qCDebug(lcQpaEventReader) << "[heap] " << m_nodesOnHeap++;
    return node;
}

void QXcbEventQueue::run()
{
    xcb_generic_event_t *event = nullptr;
    xcb_connection_t *connection = m_connection->xcb_connection();
    QXcbEventNode *tail = m_head;

    auto enqueueEvent = [&tail, this](xcb_generic_event_t *event) {
        if (!isCloseConnectionEvent(event)) {
            tail->next = qXcbEventNodeFactory(event);
            tail = tail->next;
            m_tail.store(tail, std::memory_order_release);
        } else {
            free(event);
        }
    };

    while (!m_closeConnectionDetected && (event = xcb_wait_for_event(connection))) {
        m_newEventsMutex.lock();
        enqueueEvent(event);
        while (!m_closeConnectionDetected && (event = xcb_poll_for_queued_event(connection)))
            enqueueEvent(event);

        m_newEventsCondition.wakeOne();
        m_newEventsMutex.unlock();
        wakeUpDispatcher();
    }

    if (!m_closeConnectionDetected) {
        // Connection was terminated not by us. Wake up dispatcher, which will
        // call processXcbEvents(), where we handle the connection errors via
        // xcb_connection_has_error().
        wakeUpDispatcher();
    }
}

void QXcbEventQueue::wakeUpDispatcher()
{
    QMutexLocker locker(&qAppExiting);
    if (!dispatcherOwnerDestructing) {
        // This thread can run before a dispatcher has been created,
        // so check if it is ready.
        if (QCoreApplication::eventDispatcher())
            QCoreApplication::eventDispatcher()->wakeUp();
    }
}

qint32 QXcbEventQueue::generatePeekerId()
{
    const qint32 peekerId = m_peekerIdSource++;
    m_peekerToNode.insert(peekerId, nullptr);
    return peekerId;
}

bool QXcbEventQueue::removePeekerId(qint32 peekerId)
{
    const auto it = m_peekerToNode.constFind(peekerId);
    if (it == m_peekerToNode.constEnd()) {
        qCWarning(lcQpaXcb, "failed to remove unknown peeker id: %d", peekerId);
        return false;
    }
    m_peekerToNode.erase(it);
    if (m_peekerToNode.isEmpty()) {
        m_peekerIdSource = 0; // Once the hash becomes empty, we can start reusing IDs
        m_peekerIndexCacheDirty = false;
    }
    return true;
}

bool QXcbEventQueue::peekEventQueue(PeekerCallback peeker, void *peekerData,
                                    PeekOptions option, qint32 peekerId)
{
    const bool peekerIdProvided = peekerId != -1;
    auto peekerToNodeIt = m_peekerToNode.find(peekerId);

    if (peekerIdProvided && peekerToNodeIt == m_peekerToNode.end()) {
        qCWarning(lcQpaXcb, "failed to find index for unknown peeker id: %d", peekerId);
        return false;
    }

    const bool useCache = option.testFlag(PeekOption::PeekFromCachedIndex);
    if (useCache && !peekerIdProvided) {
        qCWarning(lcQpaXcb, "PeekOption::PeekFromCachedIndex requires peeker id");
        return false;
    }

    if (peekerIdProvided && m_peekerIndexCacheDirty) {
        for (auto &node : m_peekerToNode) // reset cache
            node = nullptr;
        m_peekerIndexCacheDirty = false;
    }

    flushBufferedEvents();
    if (isEmpty())
        return false;

    const auto startNode = [this, useCache, peekerToNodeIt]() -> QXcbEventNode * {
        if (useCache) {
            const QXcbEventNode *cachedNode = peekerToNodeIt.value();
            if (!cachedNode)
                return m_head; // cache was reset
            if (cachedNode == m_flushedTail)
                return nullptr; // no new events since the last call
            return cachedNode->next;
        }
        return m_head;
    }();

    if (!startNode)
        return false;

    // A peeker may call QCoreApplication::processEvents(), which will cause
    // QXcbConnection::processXcbEvents() to modify the queue we are currently
    // looping through;
    m_queueModified = false;
    bool result = false;

    QXcbEventNode *node = startNode;
    do {
        xcb_generic_event_t *event = node->event;
        if (event && peeker(event, peekerData)) {
            result = true;
            break;
        }
        if (node == m_flushedTail)
            break;
        node = node->next;
    } while (!m_queueModified);

    // Update the cached index if the queue was not modified, and hence the
    // cache is still valid.
    if (peekerIdProvided && node != startNode && !m_queueModified) {
        // Before updating, make sure that a peeker callback did not remove
        // the peeker id.
        peekerToNodeIt = m_peekerToNode.find(peekerId);
        if (peekerToNodeIt != m_peekerToNode.end())
            *peekerToNodeIt = node; // id still in the cache, update node
    }

    return result;
}

void QXcbEventQueue::waitForNewEvents(unsigned long time)
{
    QMutexLocker locker(&m_newEventsMutex);
    QXcbEventNode *tailBeforeFlush = m_flushedTail;
    flushBufferedEvents();
    if (tailBeforeFlush != m_flushedTail)
        return;
    m_newEventsCondition.wait(&m_newEventsMutex, time);
}

void QXcbEventQueue::sendCloseConnectionEvent() const
{
    // A hack to close XCB connection. Apparently XCB does not have any APIs for this?
    xcb_client_message_event_t event;
    memset(&event, 0, sizeof(event));

    xcb_connection_t *c = m_connection->xcb_connection();
    const xcb_window_t window = xcb_generate_id(c);
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(m_connection->setup());
    xcb_screen_t *screen = it.data;
    xcb_create_window(c, XCB_COPY_FROM_PARENT,
                      window, screen->root,
                      0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
                      screen->root_visual, 0, nullptr);

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.sequence = 0;
    event.window = window;
    event.type = m_connection->atom(QXcbAtom::_QT_CLOSE_CONNECTION);
    event.data.data32[0] = 0;

    xcb_send_event(c, false, window, XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<const char *>(&event));
    xcb_destroy_window(c, window);
    xcb_flush(c);
}

bool QXcbEventQueue::isCloseConnectionEvent(const xcb_generic_event_t *event)
{
    if (event && (event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
        auto clientMessage = reinterpret_cast<const xcb_client_message_event_t *>(event);
        if (clientMessage->type == m_connection->atom(QXcbAtom::_QT_CLOSE_CONNECTION))
            m_closeConnectionDetected = true;
    }
    return m_closeConnectionDetected;
}

QT_END_NAMESPACE
