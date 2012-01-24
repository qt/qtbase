/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QREFCOUNT_H
#define QREFCOUNT_H

#include <QtCore/qatomic.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


namespace QtPrivate
{

class RefCount
{
public:
    inline void ref() {
        if (atomic.load() > 0)
            atomic.ref();
    }

    inline bool deref() {
        if (atomic.load() <= 0)
            return true;
        return atomic.deref();
    }

    inline bool operator==(int value) const
    { return atomic.load() == value; }
    inline bool operator!=(int value) const
    { return atomic.load() != value; }
    inline bool operator!() const
    { return !atomic.load(); }
    inline operator int() const
    { return atomic.load(); }
    inline RefCount &operator=(int value)
    { atomic.store(value); return *this; }
    inline RefCount &operator=(const RefCount &other)
    { atomic.store(other.atomic.load()); return *this; }

    QBasicAtomicInt atomic;
};

#define Q_REFCOUNT_INITIALIZER(a) { Q_BASIC_ATOMIC_INITIALIZER(a) }

}

QT_END_NAMESPACE

QT_END_HEADER

#endif
