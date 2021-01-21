/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

