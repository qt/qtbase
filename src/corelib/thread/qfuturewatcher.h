/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
****************************************************************************/

#ifndef QFUTUREWATCHER_H
#define QFUTUREWATCHER_H

#include <QtCore/qfuture.h>
#include <QtCore/qobject.h>

QT_REQUIRE_CONFIG(future);

QT_BEGIN_NAMESPACE


class QEvent;

class QFutureWatcherBasePrivate;
class Q_CORE_EXPORT QFutureWatcherBase : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QFutureWatcherBase)

public:
    explicit QFutureWatcherBase(QObject *parent = nullptr);
    // de-inline dtor

    int progressValue() const;
    int progressMinimum() const;
    int progressMaximum() const;
    QString progressText() const;

    bool isStarted() const;
    bool isFinished() const;
    bool isRunning() const;
    bool isCanceled() const;
    bool isPaused() const;

    void waitForFinished();

    void setPendingResultsLimit(int limit);

    bool event(QEvent *event) override;

Q_SIGNALS:
    void started();
    void finished();
    void canceled();
    void paused();
    void resumed();
    void resultReadyAt(int resultIndex);
    void resultsReadyAt(int beginIndex, int endIndex);
    void progressRangeChanged(int minimum, int maximum);
    void progressValueChanged(int progressValue);
    void progressTextChanged(const QString &progressText);

public Q_SLOTS:
    void cancel();
    void setPaused(bool paused);
    void pause();
    void resume();
    void togglePaused();

protected:
    void connectNotify (const QMetaMethod &signal) override;
    void disconnectNotify (const QMetaMethod &signal) override;

    // called from setFuture() implemented in template sub-classes
    void connectOutputInterface();
    void disconnectOutputInterface(bool pendingAssignment = false);

private:
    // implemented in the template sub-classes
    virtual const QFutureInterfaceBase &futureInterface() const = 0;
    virtual QFutureInterfaceBase &futureInterface() = 0;
};

template <typename T>
class QFutureWatcher : public QFutureWatcherBase
{
public:
    explicit QFutureWatcher(QObject *_parent = nullptr)
        : QFutureWatcherBase(_parent)
    { }
    ~QFutureWatcher()
    { disconnectOutputInterface(); }

    void setFuture(const QFuture<T> &future);
    QFuture<T> future() const
    { return m_future; }

    T result() const { return m_future.result(); }
    T resultAt(int index) const { return m_future.resultAt(index); }

#ifdef Q_QDOC
    int progressValue() const;
    int progressMinimum() const;
    int progressMaximum() const;
    QString progressText() const;

    bool isStarted() const;
    bool isFinished() const;
    bool isRunning() const;
    bool isCanceled() const;
    bool isPaused() const;

    void waitForFinished();

    void setPendingResultsLimit(int limit);

Q_SIGNALS:
    void started();
    void finished();
    void canceled();
    void paused();
    void resumed();
    void resultReadyAt(int resultIndex);
    void resultsReadyAt(int beginIndex, int endIndex);
    void progressRangeChanged(int minimum, int maximum);
    void progressValueChanged(int progressValue);
    void progressTextChanged(const QString &progressText);

public Q_SLOTS:
    void cancel();
    void setPaused(bool paused);
    void pause();
    void resume();
    void togglePaused();
#endif

private:
    QFuture<T> m_future;
    const QFutureInterfaceBase &futureInterface() const override { return m_future.d; }
    QFutureInterfaceBase &futureInterface() override { return m_future.d; }
};

template <typename T>
Q_INLINE_TEMPLATE void QFutureWatcher<T>::setFuture(const QFuture<T> &_future)
{
    if (_future == m_future)
        return;

    disconnectOutputInterface(true);
    m_future = _future;
    connectOutputInterface();
}

template <>
class QFutureWatcher<void> : public QFutureWatcherBase
{
public:
    explicit QFutureWatcher(QObject *_parent = nullptr)
        : QFutureWatcherBase(_parent)
    { }
    ~QFutureWatcher()
    { disconnectOutputInterface(); }

    void setFuture(const QFuture<void> &future);
    QFuture<void> future() const
    { return m_future; }

private:
    QFuture<void> m_future;
    const QFutureInterfaceBase &futureInterface() const override { return m_future.d; }
    QFutureInterfaceBase &futureInterface() override { return m_future.d; }
};

Q_INLINE_TEMPLATE void QFutureWatcher<void>::setFuture(const QFuture<void> &_future)
{
    if (_future == m_future)
        return;

    disconnectOutputInterface(true);
    m_future = _future;
    connectOutputInterface();
}

QT_END_NAMESPACE

#endif // QFUTUREWATCHER_H
