/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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
