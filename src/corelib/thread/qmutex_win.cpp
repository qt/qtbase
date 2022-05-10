// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmutex.h"
#include <qatomic.h>
#include "qmutex_p.h"
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

QMutexPrivate::QMutexPrivate()
{
    event = CreateEvent(0, FALSE, FALSE, 0);

    if (!event)
        qWarning("QMutexPrivate::QMutexPrivate: Cannot create event");
}

QMutexPrivate::~QMutexPrivate()
{ CloseHandle(event); }

bool QMutexPrivate::wait(int timeout)
{
    return (WaitForSingleObjectEx(event, timeout < 0 ? INFINITE : timeout, FALSE) == WAIT_OBJECT_0);
}

void QMutexPrivate::wakeUp() noexcept
{ SetEvent(event); }

QT_END_NAMESPACE
