/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QARRAYDATAOPS_H
#define QARRAYDATAOPS_H

#include <QtCore/qarraydata.h>

#include <new>
#include <string.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

namespace QtPrivate {

template <class T>
struct QPodArrayOps
    : QTypedArrayData<T>
{
    void copyAppend(const T *b, const T *e)
    {
        Q_ASSERT(this->ref == 1);
        Q_ASSERT(b < e);
        Q_ASSERT(size_t(e - b) <= this->alloc - uint(this->size));

        ::memcpy(this->end(), b, (e - b) * sizeof(T));
        this->size += e - b;
    }

    void copyAppend(size_t n, const T &t)
    {
        Q_ASSERT(this->ref == 1);
        Q_ASSERT(n <= this->alloc - uint(this->size));

        T *iter = this->end();
        const T *const end = iter + n;
        for (; iter != end; ++iter)
            ::memcpy(iter, &t, sizeof(T));
        this->size += n;
    }

    void destroyAll() // Call from destructors, ONLY!
    {
        Q_ASSERT(this->ref == 0);

        // As this is to be called only from destructor, it doesn't need to be
        // exception safe; size not updated.
    }
};

template <class T>
struct QGenericArrayOps
    : QTypedArrayData<T>
{
    void copyAppend(const T *b, const T *e)
    {
        Q_ASSERT(this->ref == 1);
        Q_ASSERT(b < e);
        Q_ASSERT(size_t(e - b) <= this->alloc - uint(this->size));

        T *iter = this->end();
        for (; b != e; ++iter, ++b) {
            new (iter) T(*b);
            ++this->size;
        }
    }

    void copyAppend(size_t n, const T &t)
    {
        Q_ASSERT(this->ref == 1);
        Q_ASSERT(n <= this->alloc - uint(this->size));

        T *iter = this->end();
        const T *const end = iter + n;
        for (; iter != end; ++iter) {
            new (iter) T(t);
            ++this->size;
        }
    }

    void destroyAll() // Call from destructors, ONLY
    {
        // As this is to be called only from destructor, it doesn't need to be
        // exception safe; size not updated.

        Q_ASSERT(this->ref == 0);

        const T *const b = this->begin();
        const T *i = this->end();

        while (i != b)
            (--i)->~T();
    }
};

template <class T>
struct QMovableArrayOps
    : QGenericArrayOps<T>
{
    // using QGenericArrayOps<T>::copyAppend;
    // using QGenericArrayOps<T>::destroyAll;
};

template <class T, class = void>
struct QArrayOpsSelector
{
    typedef QGenericArrayOps<T> Type;
};

template <class T>
struct QArrayOpsSelector<T,
    typename QEnableIf<
        !QTypeInfo<T>::isComplex && !QTypeInfo<T>::isStatic
    >::Type>
{
    typedef QPodArrayOps<T> Type;
};

template <class T>
struct QArrayOpsSelector<T,
    typename QEnableIf<
        QTypeInfo<T>::isComplex && !QTypeInfo<T>::isStatic
    >::Type>
{
    typedef QMovableArrayOps<T> Type;
};

} // namespace QtPrivate

template <class T>
struct QArrayDataOps
    : QtPrivate::QArrayOpsSelector<T>::Type
{
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // include guard
