// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOS_DEADLOCKPROTECTOR_H
#define QOHOS_DEADLOCKPROTECTOR_H

#include <QAtomicInt>

QT_BEGIN_NAMESPACE

class QOhosDeadlockProtector
{
public:
    QOhosDeadlockProtector()
        : m_acquired(0)
    {
    }

    ~QOhosDeadlockProtector() {
        if (m_acquired)
            s_blocked.storeRelease(0);
    }

    bool acquire() {
        m_acquired = s_blocked.testAndSetAcquire(0, 1);
        return m_acquired;
    }

private:
    static QAtomicInt s_blocked;
    int m_acquired;
};

QT_END_NAMESPACE

#endif // QOHOS_DEADLOCKPROTECTOR_H

