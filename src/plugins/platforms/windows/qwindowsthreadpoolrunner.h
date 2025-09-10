// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSTHREADPOOLRUNNER_H
#define QWINDOWSTHREADPOOLRUNNER_H

#include <QtCore/qmutex.h>
#include <QtCore/qrunnable.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/qwaitcondition.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsThreadPoolRunner
    \brief Runs a task in the global instance of QThreadPool

    QThreadPool does not provide a method to wait on a single task, so this needs
    to be done by using QWaitCondition/QMutex.

    \internal
*/
class QWindowsThreadPoolRunner
{
    Q_DISABLE_COPY_MOVE(QWindowsThreadPoolRunner)

#if QT_CONFIG(thread)
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
#else // QT_CONFIG(thread)
public:
    QWindowsThreadPoolRunner() {}

    template <class Function>
    bool run(Function f, unsigned long /* timeOutMSecs */ = 5000)
    {
        f();
        return true;
    }
#endif // QT_CONFIG(thread)
};

QT_END_NAMESPACE

#endif // QWINDOWSTHREADPOOLRUNNER_H
