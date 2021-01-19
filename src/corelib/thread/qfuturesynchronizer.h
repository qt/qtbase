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

#ifndef QFUTURESYNCHRONIZER_H
#define QFUTURESYNCHRONIZER_H

#include <QtCore/qfuture.h>

QT_REQUIRE_CONFIG(future);

QT_BEGIN_NAMESPACE


template <typename T>
class QFutureSynchronizer
{
    Q_DISABLE_COPY(QFutureSynchronizer)

public:
    QFutureSynchronizer() : m_cancelOnWait(false) { }
    explicit QFutureSynchronizer(const QFuture<T> &future)
        : m_cancelOnWait(false)
    { addFuture(future); }
    ~QFutureSynchronizer()  { waitForFinished(); }

    void setFuture(const QFuture<T> &future)
    {
        waitForFinished();
        m_futures.clear();
        addFuture(future);
    }

    void addFuture(const QFuture<T> &future)
    {
        m_futures.append(future);
    }

    void waitForFinished()
    {
        if (m_cancelOnWait) {
            for (int i = 0; i < m_futures.count(); ++i) {
                 m_futures[i].cancel();
            }
        }

        for (int i = 0; i < m_futures.count(); ++i) {
             m_futures[i].waitForFinished();
         }
    }

    void clearFutures()
    {
        m_futures.clear();
    }

    QList<QFuture<T> > futures() const
    {
        return m_futures;
    }

    void setCancelOnWait(bool enabled)
    {
        m_cancelOnWait = enabled;
    }

    bool cancelOnWait() const
    {
        return m_cancelOnWait;
    }

protected:
    QList<QFuture<T> > m_futures;
    bool m_cancelOnWait;
};

QT_END_NAMESPACE

#endif // QFUTURESYNCHRONIZER_H
