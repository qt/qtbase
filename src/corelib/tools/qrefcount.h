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

#ifndef QREFCOUNT_H
#define QREFCOUNT_H

#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE


namespace QtPrivate
{

class RefCount
{
public:
    inline bool ref() noexcept {
        int count = atomic.loadRelaxed();
        if (count != -1) // !isStatic
            atomic.ref();
        return true;
    }

    inline bool deref() noexcept {
        int count = atomic.loadRelaxed();
        if (count == -1) // isStatic
            return true;
        return atomic.deref();
    }

    bool isStatic() const noexcept
    {
        // Persistent object, never deleted
        return atomic.loadRelaxed() == -1;
    }

    bool isShared() const noexcept
    {
        int count = atomic.loadRelaxed();
        return (count != 1) && (count != 0);
    }

    void initializeOwned() noexcept { atomic.storeRelaxed(1); }
    void initializeUnsharable() noexcept { atomic.storeRelaxed(0); }

    QBasicAtomicInt atomic;
};

}

#define Q_REFCOUNT_INITIALIZE_STATIC { Q_BASIC_ATOMIC_INITIALIZER(-1) }

QT_END_NAMESPACE

#endif
