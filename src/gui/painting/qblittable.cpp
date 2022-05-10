// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qblittable_p.h"

#ifndef QT_NO_BLITTABLE
QT_BEGIN_NAMESPACE

class QBlittablePrivate
{
public:
    QBlittablePrivate(const QSize &size, QBlittable::Capabilities caps)
        : caps(caps), m_size(size), locked(false), cachedImg(nullptr)
    {}
    QBlittable::Capabilities caps;
    QSize m_size;
    bool locked;
    QImage *cachedImg;
};


QBlittable::QBlittable(const QSize &size, Capabilities caps)
    : d_ptr(new QBlittablePrivate(size,caps))
{
}

QBlittable::~QBlittable()
{
    delete d_ptr;
}


QBlittable::Capabilities QBlittable::capabilities() const
{
    Q_D(const QBlittable);
    return d->caps;
}

QSize QBlittable::size() const
{
    Q_D(const QBlittable);
    return d->m_size;
}

QImage *QBlittable::lock()
{
    Q_D(QBlittable);
    if (!d->locked) {
        d->cachedImg = doLock();
        d->locked = true;
    }

    return d->cachedImg;
}

void QBlittable::unlock()
{
    Q_D(QBlittable);
    if (d->locked) {
        doUnlock();
        d->locked = false;
    }
}

bool QBlittable::isLocked() const
{
    Q_D(const QBlittable);
    return d->locked;
}

QT_END_NAMESPACE
#endif //QT_NO_BLITTABLE

