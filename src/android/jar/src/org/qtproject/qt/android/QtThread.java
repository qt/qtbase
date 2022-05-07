/****************************************************************************
**
** Copyright (C) 2018 BogDan Vatra <bogdan@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Android port of the Qt Toolkit.
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
******************************************************************************/

package org.qtproject.qt.android;

import java.util.ArrayList;
import java.util.concurrent.Semaphore;

public class QtThread {
    private ArrayList<Runnable> m_pendingRunnables = new ArrayList<Runnable>();
    private boolean m_exit = false;
    private Thread m_qtThread = new Thread(new Runnable() {
        @Override
        public void run() {
            while (!m_exit) {
                try {
                    ArrayList<Runnable> pendingRunnables;
                    synchronized (m_qtThread) {
                        if (m_pendingRunnables.size() == 0)
                            m_qtThread.wait();
                        pendingRunnables = new ArrayList<Runnable>(m_pendingRunnables);
                        m_pendingRunnables.clear();
                    }
                    for (Runnable runnable : pendingRunnables)
                        runnable.run();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    });

    QtThread() {
        m_qtThread.setName("qtMainLoopThread");
        m_qtThread.start();
    }

    public void post(final Runnable runnable) {
        synchronized (m_qtThread) {
            m_pendingRunnables.add(runnable);
            m_qtThread.notify();
        }
    }

    public void run(final Runnable runnable) {
        final Semaphore sem = new Semaphore(0);
        synchronized (m_qtThread) {
            m_pendingRunnables.add(new Runnable() {
                @Override
                public void run() {
                    runnable.run();
                    sem.release();
                }
            });
            m_qtThread.notify();
        }
        try {
            sem.acquire();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public void exit()
    {
        m_exit = true;
        synchronized (m_qtThread) {
            m_qtThread.notify();
        }
        try {
            m_qtThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
