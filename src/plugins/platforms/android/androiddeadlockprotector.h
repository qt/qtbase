/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef ANDROID_DEADLOCKPROTECTOR_H
#define ANDROID_DEADLOCKPROTECTOR_H

#include <QAtomicInt>

QT_BEGIN_NAMESPACE

class AndroidDeadlockProtector
{
public:
    AndroidDeadlockProtector()
        : m_acquired(0)
    {
    }

    ~AndroidDeadlockProtector() {
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

#endif // ANDROID_DEADLOCKPROTECTOR_H

