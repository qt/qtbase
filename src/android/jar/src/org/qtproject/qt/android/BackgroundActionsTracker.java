// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android;

import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicInteger;

// Helper class to keep track of background actions and relay them when ready to be executed
class BackgroundActionsTracker {
    // For unlimited queue
    private int m_maxActions = -1;
    private final ConcurrentLinkedQueue<Runnable> m_backgroundActionsQueue = new ConcurrentLinkedQueue<>();
    private final AtomicInteger m_actionsCount = new AtomicInteger(0);

    public void setMaxAllowedActions(int maxActions) {
        m_maxActions = maxActions;
    }

    public void enqueue(Runnable action) {
        if (m_maxActions == 0 || action == null)
            return;

        if (m_maxActions > 0 && m_actionsCount.get() >= m_maxActions) {
            // If the queue is full, remove the oldest action, then decrement the queue count.
            m_backgroundActionsQueue.poll();
            m_actionsCount.decrementAndGet();
        }
        m_backgroundActionsQueue.offer(action);
        m_actionsCount.incrementAndGet();
    }

    public void processActions() {
        Runnable action;
        while ((action = m_backgroundActionsQueue.poll()) != null) {
            m_actionsCount.decrementAndGet();
            QtNative.runAction(action);
        }
    }

    public int getActionsCount() {
        return m_actionsCount.get();
    }
}


