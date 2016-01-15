/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qblittable_p.h"

#ifndef QT_NO_BLITTABLE
QT_BEGIN_NAMESPACE

class QBlittablePrivate
{
public:
    QBlittablePrivate(const QSize &size, QBlittable::Capabilities caps)
        : caps(caps), m_size(size), locked(false), cachedImg(0)
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

