/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QTHREAD_H
#define QTHREAD_H

#include <QtCore/qobject.h>

// The implementation of QThread::create uses various C++14/C++17 facilities;
// we must check for their presence. Specifically for glibcxx bundled in MinGW
// with win32 threads, we check the condition found in its <future> header
// since _GLIBCXX_HAS_GTHREADS might then not be defined.
// For std::async (used in all codepaths)
// there is no SG10 feature macro; just test for the header presence.
// For the C++17 codepath do some more throughout checks for std::invoke and
// C++14 lambdas availability.
#if QT_HAS_INCLUDE(<future>) \
    && (!defined(__GLIBCXX__) || (defined(_GLIBCXX_HAS_GTHREADS) && defined(_GLIBCXX_USE_C99_STDINT_TR1)))
#  define QTHREAD_HAS_CREATE
#  include <future> // for std::async
#  include <functional> // for std::invoke; no guard needed as it's a C++98 header

#  if defined(__cpp_lib_invoke) && __cpp_lib_invoke >= 201411 \
      && defined(__cpp_init_captures) && __cpp_init_captures >= 201304 \
      && defined(__cpp_generic_lambdas) &&  __cpp_generic_lambdas >= 201304
#    define QTHREAD_HAS_VARIADIC_CREATE
#  endif
#endif

#include <limits.h>

QT_BEGIN_NAMESPACE


class QThreadData;
class QThreadPrivate;
class QAbstractEventDispatcher;

#ifndef QT_NO_THREAD
class Q_CORE_EXPORT QThread : public QObject
{
    Q_OBJECT
public:
    static Qt::HANDLE currentThreadId() Q_DECL_NOTHROW Q_DECL_PURE_FUNCTION;
    static QThread *currentThread();
    static int idealThreadCount() Q_DECL_NOTHROW;
    static void yieldCurrentThread();

    explicit QThread(QObject *parent = Q_NULLPTR);
    ~QThread();

    enum Priority {
        IdlePriority,

        LowestPriority,
        LowPriority,
        NormalPriority,
        HighPriority,
        HighestPriority,

        TimeCriticalPriority,

        InheritPriority
    };

    void setPriority(Priority priority);
    Priority priority() const;

    bool isFinished() const;
    bool isRunning() const;

    void requestInterruption();
    bool isInterruptionRequested() const;

    void setStackSize(uint stackSize);
    uint stackSize() const;

    void exit(int retcode = 0);

    QAbstractEventDispatcher *eventDispatcher() const;
    void setEventDispatcher(QAbstractEventDispatcher *eventDispatcher);

    bool event(QEvent *event) Q_DECL_OVERRIDE;
    int loopLevel() const;

#ifdef Q_QDOC
    template <typename Function, typename... Args>
    static QThread *create(Function &&f, Args &&... args);
    template <typename Function>
    static QThread *create(Function &&f);
#else
#ifdef QTHREAD_HAS_CREATE
#ifdef QTHREAD_HAS_VARIADIC_CREATE
    template <typename Function, typename... Args>
    static QThread *create(Function &&f, Args &&... args);
#else
    template <typename Function>
    static QThread *create(Function &&f);
#endif
#endif
#endif

public Q_SLOTS:
    void start(Priority = InheritPriority);
    void terminate();
    void quit();

public:
    // default argument causes thread to block indefinetely
    bool wait(unsigned long time = ULONG_MAX);

    static void sleep(unsigned long);
    static void msleep(unsigned long);
    static void usleep(unsigned long);

Q_SIGNALS:
    void started(QPrivateSignal);
    void finished(QPrivateSignal);

protected:
    virtual void run();
    int exec();

    static void setTerminationEnabled(bool enabled = true);

protected:
    QThread(QThreadPrivate &dd, QObject *parent = Q_NULLPTR);

private:
    Q_DECLARE_PRIVATE(QThread)

#ifdef QTHREAD_HAS_CREATE
    static QThread *createThreadImpl(std::future<void> &&future);
#endif

    friend class QCoreApplication;
    friend class QThreadData;
};

#ifdef QTHREAD_HAS_CREATE

#ifdef QTHREAD_HAS_VARIADIC_CREATE
// C++17: std::thread's constructor complying call
template <typename Function, typename... Args>
QThread *QThread::create(Function &&f, Args &&... args)
{
    using DecayedFunction = typename std::decay<Function>::type;
    auto threadFunction =
        [f = static_cast<DecayedFunction>(std::forward<Function>(f))](auto &&... largs) mutable -> void
        {
            (void)std::invoke(std::move(f), std::forward<decltype(largs)>(largs)...);
        };

    return createThreadImpl(std::async(std::launch::deferred,
                                       std::move(threadFunction),
                                       std::forward<Args>(args)...));
}
#elif defined(__cpp_init_captures) && __cpp_init_captures >= 201304
// C++14: implementation for just one callable
template <typename Function>
QThread *QThread::create(Function &&f)
{
    using DecayedFunction = typename std::decay<Function>::type;
    auto threadFunction =
        [f = static_cast<DecayedFunction>(std::forward<Function>(f))]() mutable -> void
        {
            (void)f();
        };

    return createThreadImpl(std::async(std::launch::deferred, std::move(threadFunction)));
}
#else
// C++11: same as C++14, but with a workaround for not having generalized lambda captures
namespace QtPrivate {
template <typename Function>
struct Callable
{
    explicit Callable(Function &&f)
        : m_function(std::forward<Function>(f))
    {
    }

#if defined(Q_COMPILER_DEFAULT_MEMBERS) && defined(Q_COMPILER_DELETE_MEMBERS)
    // Apply the same semantics of a lambda closure type w.r.t. the special
    // member functions, if possible: delete the copy assignment operator,
    // bring back all the others as per the RO5 (cf. §8.1.5.1/11 [expr.prim.lambda.closure])
    ~Callable() = default;
    Callable(const Callable &) = default;
    Callable(Callable &&) = default;
    Callable &operator=(const Callable &) = delete;
    Callable &operator=(Callable &&) = default;
#endif

    void operator()()
    {
        (void)m_function();
    }

    typename std::decay<Function>::type m_function;
};
} // namespace QtPrivate

template <typename Function>
QThread *QThread::create(Function &&f)
{
    return createThreadImpl(std::async(std::launch::deferred, QtPrivate::Callable<Function>(std::forward<Function>(f))));
}
#endif // QTHREAD_HAS_VARIADIC_CREATE

#endif // QTHREAD_HAS_CREATE

#else // QT_NO_THREAD

class Q_CORE_EXPORT QThread : public QObject
{
public:
    static Qt::HANDLE currentThreadId() { return Qt::HANDLE(currentThread()); }
    static QThread* currentThread();

protected:
    QThread(QThreadPrivate &dd, QObject *parent = nullptr);

private:
    explicit QThread(QObject *parent = nullptr);
    static QThread *instance;

    friend class QCoreApplication;
    friend class QThreadData;
    friend class QAdoptedThread;
    Q_DECLARE_PRIVATE(QThread)
};

#endif // QT_NO_THREAD

QT_END_NAMESPACE

#endif // QTHREAD_H
