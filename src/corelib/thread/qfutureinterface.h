/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QFUTUREINTERFACE_H
#define QFUTUREINTERFACE_H

#include <QtCore/qrunnable.h>
#include <QtCore/qmutex.h>
#include <QtCore/qexception.h>
#include <QtCore/qresultstore.h>

#include <utility>
#include <vector>
#include <mutex>

QT_REQUIRE_CONFIG(future);

QT_BEGIN_NAMESPACE


template <typename T> class QFuture;
class QThreadPool;
class QFutureInterfaceBasePrivate;
class QFutureWatcherBase;
class QFutureWatcherBasePrivate;

namespace QtPrivate {
template<typename Function, typename ResultType, typename ParentResultType>
class Continuation;

template<class Function, class ResultType>
class CanceledHandler;

#ifndef QT_NO_EXCEPTIONS
template<class Function, class ResultType>
class FailureHandler;
#endif
}

class Q_CORE_EXPORT QFutureInterfaceBase
{
public:
    enum State {
        NoState    = 0x00,
        Running    = 0x01,
        Started    = 0x02,
        Finished   = 0x04,
        Canceled   = 0x08,
        Suspending = 0x10,
        Suspended  = 0x20,
        Throttled  = 0x40,
        // Pending means that the future depends on another one, which is not finished yet
        Pending    = 0x80,
    };

    QFutureInterfaceBase(State initialState = NoState);
    QFutureInterfaceBase(const QFutureInterfaceBase &other);
    virtual ~QFutureInterfaceBase();

    // reporting functions available to the engine author:
    void reportStarted();
    void reportFinished();
    void reportCanceled();
#ifndef QT_NO_EXCEPTIONS
    void reportException(const QException &e);
    void reportException(std::exception_ptr e);
#endif
    void reportResultsReady(int beginIndex, int endIndex);

    void setRunnable(QRunnable *runnable);
    void setThreadPool(QThreadPool *pool);
    QThreadPool *threadPool() const;
    void setFilterMode(bool enable);
    void setProgressRange(int minimum, int maximum);
    int progressMinimum() const;
    int progressMaximum() const;
    bool isProgressUpdateNeeded() const;
    void setProgressValue(int progressValue);
    int progressValue() const;
    void setProgressValueAndText(int progressValue, const QString &progressText);
    QString progressText() const;

    void setExpectedResultCount(int resultCount);
    int expectedResultCount();
    int resultCount() const;

    bool queryState(State state) const;
    bool isRunning() const;
    bool isStarted() const;
    bool isCanceled() const;
    bool isFinished() const;
#if QT_DEPRECATED_SINCE(6, 0)
    QT_DEPRECATED_VERSION_X_6_0("Use isSuspending() or isSuspended() instead.")
    bool isPaused() const;

    QT_DEPRECATED_VERSION_X_6_0("Use setSuspended() instead.")
    void setPaused(bool paused) { setSuspended(paused); }

    QT_DEPRECATED_VERSION_X_6_0("Use toggleSuspended() instead.")
    void togglePaused() { toggleSuspended(); }
#endif
    bool isSuspending() const;
    bool isSuspended() const;
    bool isThrottled() const;
    bool isResultReadyAt(int index) const;
    bool isValid() const;
    int loadState() const;

    void cancel();
    void setSuspended(bool suspend);
    void toggleSuspended();
    void reportSuspended() const;
    void setThrottled(bool enable);

    void waitForFinished();
    bool waitForNextResult();
    void waitForResult(int resultIndex);
    void waitForResume();
    void suspendIfRequested();

    QMutex &mutex() const;
    QtPrivate::ExceptionStore &exceptionStore();
    QtPrivate::ResultStoreBase &resultStoreBase();
    const QtPrivate::ResultStoreBase &resultStoreBase() const;

    inline bool operator==(const QFutureInterfaceBase &other) const { return d == other.d; }
    inline bool operator!=(const QFutureInterfaceBase &other) const { return d != other.d; }
    QFutureInterfaceBase &operator=(const QFutureInterfaceBase &other);

    void swap(QFutureInterfaceBase &other) noexcept;

protected:
    bool refT() const;
    bool derefT() const;
    void reset();
public:

#ifndef QFUTURE_TEST
private:
#endif
    QFutureInterfaceBasePrivate *d;

private:
    friend class QFutureWatcherBase;
    friend class QFutureWatcherBasePrivate;

    template<typename Function, typename ResultType, typename ParentResultType>
    friend class QtPrivate::Continuation;

    template<class Function, class ResultType>
    friend class QtPrivate::CanceledHandler;

#ifndef QT_NO_EXCEPTIONS
    template<class Function, class ResultType>
    friend class QtPrivate::FailureHandler;
#endif

protected:
    void setContinuation(std::function<void(const QFutureInterfaceBase &)> func);
    void runContinuation() const;

    void setLaunchAsync(bool value);
    bool launchAsync() const;

    bool isRunningOrPending() const;
};

template <typename T>
class QFutureInterface : public QFutureInterfaceBase
{
public:
    QFutureInterface(State initialState = NoState)
        : QFutureInterfaceBase(initialState)
    {
        refT();
    }
    QFutureInterface(const QFutureInterface &other)
        : QFutureInterfaceBase(other)
    {
        refT();
    }
    QFutureInterface(const QFutureInterfaceBase &dd) : QFutureInterfaceBase(dd) { refT(); }
    ~QFutureInterface()
    {
        if (!derefT())
            resultStoreBase().template clear<T>();
    }

    static QFutureInterface canceledResult()
    { return QFutureInterface(State(Started | Finished | Canceled)); }

    QFutureInterface &operator=(const QFutureInterface &other)
    {
        other.refT();
        if (!derefT())
            resultStoreBase().template clear<T>();
        QFutureInterfaceBase::operator=(other);
        return *this;
    }

    inline QFuture<T> future(); // implemented in qfuture.h

    inline bool reportResult(const T *result, int index = -1);
    inline bool reportAndMoveResult(T &&result, int index = -1);
    inline bool reportResult(T &&result, int index = -1);
    inline bool reportResult(const T &result, int index = -1);
    inline bool reportResults(const QList<T> &results, int beginIndex = -1, int count = -1);
    inline bool reportFinished(const T *result);
    void reportFinished()
    {
        QFutureInterfaceBase::reportFinished();
        QFutureInterfaceBase::runContinuation();
    }

    inline const T &resultReference(int index) const;
    inline const T *resultPointer(int index) const;
    inline QList<T> results();

    T takeResult();
#if 0
    // TODO: Enable and make it return a QList, when QList is fixed to support move-only types
    std::vector<T> takeResults();
#endif
};

template <typename T>
inline bool QFutureInterface<T>::reportResult(const T *result, int index)
{
    std::lock_guard<QMutex> locker{mutex()};
    if (this->queryState(Canceled) || this->queryState(Finished))
        return false;

    QtPrivate::ResultStoreBase &store = resultStoreBase();

    const int resultCountBefore = store.count();
    const int insertIndex = store.addResult<T>(index, result);
    if (insertIndex == -1)
        return false;
    if (store.filterMode()) {
        this->reportResultsReady(resultCountBefore, store.count());
    } else {
        this->reportResultsReady(insertIndex, insertIndex + 1);
    }
    return true;
}

template<typename T>
bool QFutureInterface<T>::reportAndMoveResult(T &&result, int index)
{
    std::lock_guard<QMutex> locker{mutex()};
    if (queryState(Canceled) || queryState(Finished))
        return false;

    QtPrivate::ResultStoreBase &store = resultStoreBase();

    const int oldResultCount = store.count();
    const int insertIndex = store.moveResult(index, std::forward<T>(result));
    // Let's make sure it's not in pending results.
    if (insertIndex != -1 && (!store.filterMode() || oldResultCount < store.count()))
        reportResultsReady(insertIndex, store.count());
    return insertIndex != -1;
}

template<typename T>
bool QFutureInterface<T>::reportResult(T &&result, int index)
{
    return reportAndMoveResult(std::move(result), index);
}

template <typename T>
inline bool QFutureInterface<T>::reportResult(const T &result, int index)
{
    return reportResult(&result, index);
}

template<typename T>
inline bool QFutureInterface<T>::reportResults(const QList<T> &_results, int beginIndex, int count)
{
    std::lock_guard<QMutex> locker{mutex()};
    if (this->queryState(Canceled) || this->queryState(Finished))
        return false;

    auto &store = resultStoreBase();

    const int resultCountBefore = store.count();
    const int insertIndex = store.addResults(beginIndex, &_results, count);
    if (insertIndex == -1)
        return false;
    if (store.filterMode()) {
        this->reportResultsReady(resultCountBefore, store.count());
    } else {
        this->reportResultsReady(insertIndex, insertIndex + _results.count());
    }
    return true;
}

template <typename T>
inline bool QFutureInterface<T>::reportFinished(const T *result)
{
    bool resultReported = false;
    if (result)
        resultReported = reportResult(result);
    reportFinished();
    return resultReported;
}

template <typename T>
inline const T &QFutureInterface<T>::resultReference(int index) const
{
    std::lock_guard<QMutex> locker{mutex()};
    return resultStoreBase().resultAt(index).template value<T>();
}

template <typename T>
inline const T *QFutureInterface<T>::resultPointer(int index) const
{
    std::lock_guard<QMutex> locker{mutex()};
    return resultStoreBase().resultAt(index).template pointer<T>();
}

template <typename T>
inline QList<T> QFutureInterface<T>::results()
{
    if (this->isCanceled()) {
        exceptionStore().throwPossibleException();
        return QList<T>();
    }

    QFutureInterfaceBase::waitForResult(-1);

    QList<T> res;
    std::lock_guard<QMutex> locker{mutex()};

    QtPrivate::ResultIteratorBase it = resultStoreBase().begin();
    while (it != resultStoreBase().end()) {
        res.append(it.value<T>());
        ++it;
    }

    return res;
}

template<typename T>
T QFutureInterface<T>::takeResult()
{
    Q_ASSERT(isValid());

    // Note: we wait for all, this is intentional,
    // not to mess with other unready results.
    waitForResult(-1);

    const std::lock_guard<QMutex> locker{mutex()};
    QtPrivate::ResultIteratorBase position = resultStoreBase().resultAt(0);
    T ret(std::move_if_noexcept(position.value<T>()));
    reset();
    resultStoreBase().template clear<T>();

    return ret;
}

#if 0
template<typename T>
std::vector<T> QFutureInterface<T>::takeResults()
{
    Q_ASSERT(isValid());

    waitForResult(-1);
    std::vector<T> res;
    res.reserve(resultCount());

    const std::lock_guard<QMutex> locker{mutex()};

    QtPrivate::ResultIteratorBase it = resultStoreBase().begin();
    for (auto endIt = resultStoreBase().end(); it != endIt; ++it)
        res.push_back(std::move_if_noexcept(it.value<T>()));

    reset();
    resultStoreBase().template clear<T>();

    return res;
}
#endif

template <>
class QFutureInterface<void> : public QFutureInterfaceBase
{
public:
    explicit QFutureInterface<void>(State initialState = NoState)
        : QFutureInterfaceBase(initialState)
    { }

    QFutureInterface(const QFutureInterfaceBase &dd) : QFutureInterfaceBase(dd) { }

    static QFutureInterface<void> canceledResult()
    { return QFutureInterface(State(Started | Finished | Canceled)); }


    inline QFuture<void> future(); // implemented in qfuture.h

    bool reportResult(const void *, int) { return false; }
    bool reportResults(const QList<void> &, int) { return false; }
    bool reportFinished(const void *)
    {
        reportFinished();
        return false;
    }
    void reportFinished()
    {
        QFutureInterfaceBase::reportFinished();
        QFutureInterfaceBase::runContinuation();
    }
};

template<typename T>
inline void swap(QFutureInterface<T> &a, QFutureInterface<T> &b) noexcept
{
    a.swap(b);
}

QT_END_NAMESPACE

#endif // QFUTUREINTERFACE_H
