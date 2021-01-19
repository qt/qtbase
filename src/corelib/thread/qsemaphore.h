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

#ifndef QSEMAPHORE_H
#define QSEMAPHORE_H

#include <QtCore/qglobal.h>

QT_REQUIRE_CONFIG(thread);

QT_BEGIN_NAMESPACE

class QSemaphorePrivate;

class Q_CORE_EXPORT QSemaphore
{
public:
    explicit QSemaphore(int n = 0);
    ~QSemaphore();

    void acquire(int n = 1);
    bool tryAcquire(int n = 1);
    bool tryAcquire(int n, int timeout);

    void release(int n = 1);

    int available() const;

private:
    Q_DISABLE_COPY(QSemaphore)

    union {
        QSemaphorePrivate *d;
        QBasicAtomicInteger<quintptr> u;        // ### Qt6: make 64-bit
    };
};

class QSemaphoreReleaser
{
public:
    QSemaphoreReleaser() = default;
    explicit QSemaphoreReleaser(QSemaphore &sem, int n = 1) noexcept
        : m_sem(&sem), m_n(n) {}
    explicit QSemaphoreReleaser(QSemaphore *sem, int n = 1) noexcept
        : m_sem(sem), m_n(n) {}
    QSemaphoreReleaser(QSemaphoreReleaser &&other) noexcept
        : m_sem(other.cancel()), m_n(other.m_n) {}
    QSemaphoreReleaser &operator=(QSemaphoreReleaser &&other) noexcept
    { QSemaphoreReleaser moved(std::move(other)); swap(moved); return *this; }

    ~QSemaphoreReleaser()
    {
        if (m_sem)
            m_sem->release(m_n);
    }

    void swap(QSemaphoreReleaser &other) noexcept
    {
        qSwap(m_sem, other.m_sem);
        qSwap(m_n, other.m_n);
    }

    QSemaphore *semaphore() const noexcept
    { return m_sem; }

    QSemaphore *cancel() noexcept
    {
        return qExchange(m_sem, nullptr);
    }

private:
    QSemaphore *m_sem = nullptr;
    int m_n;
};

QT_END_NAMESPACE

#endif // QSEMAPHORE_H
