/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

        void run() Q_DECL_OVERRIDE
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
