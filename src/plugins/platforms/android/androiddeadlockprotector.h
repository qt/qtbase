// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROID_DEADLOCKPROTECTOR_H
#define ANDROID_DEADLOCKPROTECTOR_H

#include <QtCore/private/qjnihelpers_p.h>

QT_BEGIN_NAMESPACE

class AndroidDeadlockProtector
{
public:
    ~AndroidDeadlockProtector() {
        if (m_acquired)
            QtAndroidPrivate::releaseAndroidDeadlockProtector();
    }

    bool acquire() {
        m_acquired = QtAndroidPrivate::acquireAndroidDeadlockProtector();
        return m_acquired;
    }

private:
    bool m_acquired = false;
};

QT_END_NAMESPACE

#endif // ANDROID_DEADLOCKPROTECTOR_H

