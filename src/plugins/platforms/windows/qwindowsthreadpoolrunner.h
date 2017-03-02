/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QWINDOWSTHREADPOOLRUNNER_H
#define QWINDOWSTHREADPOOLRUNNER_H

#include <QtCore/QMutex>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtCore/QWaitCondition>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsThreadPoolRunner
    \brief Runs a task in the global instance of QThreadPool

    QThreadPool does not provide a method to wait on a single task, so this needs
    to be done by using QWaitCondition/QMutex.

    \internal
    \ingroup qt-lighthouse-win
*/
class QWindowsThreadPoolRunner
{
    Q_DISABLE_COPY(QWindowsThreadPoolRunner)

#ifndef QT_NO_THREAD
    template <class RunnableFunction> // nested class implementing QRunnable to execute a function.
    class Runnable : public QRunnable
    {
    public:
        explicit Runnable(QMutex *m, QWaitCondition *c, RunnableFunction f)
            : m_mutex(m), m_condition(c), m_function(f) {}

        void run() override
        {
            m_function();
            m_mutex->lock();
            m_condition->wakeAll();
            m_mutex->unlock();
        }

    private:
        QMutex *m_mutex;
        QWaitCondition *m_condition;
        RunnableFunction m_function;
    }; // class Runnable

public:
    QWindowsThreadPoolRunner() {}

    template <class Function>
    bool run(Function f, unsigned long timeOutMSecs = 5000)
    {
        QThreadPool *pool = QThreadPool::globalInstance();
        Q_ASSERT(pool);
        Runnable<Function> *runnable = new Runnable<Function>(&m_mutex, &m_condition, f);
        m_mutex.lock();
        pool->start(runnable);
        const bool ok = m_condition.wait(&m_mutex, timeOutMSecs);
        m_mutex.unlock();
        if (!ok)
            pool->cancel(runnable);
        return ok;
    }

private:
    QMutex m_mutex;
    QWaitCondition m_condition;
#else // !QT_NO_THREAD
public:
    QWindowsThreadPoolRunner() {}

    template <class Function>
    bool run(Function f, unsigned long /* timeOutMSecs */ = 5000)
    {
        f();
        return true;
    }
#endif // QT_NO_THREAD
};

QT_END_NAMESPACE

#endif // QWINDOWSTHREADPOOLRUNNER_H
