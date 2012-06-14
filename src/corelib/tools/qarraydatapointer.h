/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QARRAYDATAPOINTER_H
#define QARRAYDATAPOINTER_H

#include <QtCore/qarraydataops.h>

QT_BEGIN_NAMESPACE

template <class T>
struct QArrayDataPointer
{
private:
    typedef QTypedArrayData<T> Data;
    typedef QArrayDataOps<T> DataOps;

public:
    typedef typename Data::iterator iterator;
    typedef typename Data::const_iterator const_iterator;

    QArrayDataPointer() noexcept
        : d(Data::sharedNull()), b(Data::sharedNullData()), size(0)
    {
    }

    QArrayDataPointer(const QArrayDataPointer &other)
        : d(other.d), b(other.b), size(other.size)
    {
        if (!other.d->ref()) {
            // must clone
            QPair<Data *, T *> pair = other.clone(other.d->cloneFlags());
            d = pair.first;
            b = pair.second;
        }
    }

    QArrayDataPointer(Data *header, T *data, size_t n = 0)
        : d(header), b(data), size(n)
    {
    }

    explicit QArrayDataPointer(QPair<QTypedArrayData<T> *, T *> data, size_t n = 0)
        : d(data.first), b(data.second), size(n)
    {
        Q_CHECK_PTR(d);
    }

    QArrayDataPointer(QArrayDataPointerRef<T> ref)
        : d(ref.ptr), b(ref.data), size(ref.size)
    {
    }

    QArrayDataPointer &operator=(const QArrayDataPointer &other)
    {
        QArrayDataPointer tmp(other);
        this->swap(tmp);
        return *this;
    }

    QArrayDataPointer(QArrayDataPointer &&other) noexcept
        : d(other.d), b(other.b), size(other.size)
    {
        other.d = Data::sharedNull();
    }

    QArrayDataPointer &operator=(QArrayDataPointer &&other) noexcept
    {
        QArrayDataPointer moved(std::move(other));
        this->swap(moved);
        return *this;
    }

    DataOps &operator*()
    {
        Q_ASSERT(d);
        return *static_cast<DataOps *>(this);
    }

    DataOps *operator->()
    {
        Q_ASSERT(d);
        return static_cast<DataOps *>(this);
    }

    const DataOps &operator*() const
    {
        Q_ASSERT(d);
        return *static_cast<const DataOps *>(this);
    }

    const DataOps *operator->() const
    {
        Q_ASSERT(d);
        return static_cast<const DataOps *>(this);
    }

    ~QArrayDataPointer()
    {
        if (!d->deref()) {
            if (d->isMutable())
                (*this)->destroyAll();
            Data::deallocate(d);
        }
    }

    bool isNull() const
    {
        return d == Data::sharedNull();
    }

    T *data() { return b; }
    const T *data() const { return b; }

    iterator begin() { return data(); }
    iterator end() { return data() + size; }
    const_iterator begin() const { return data(); }
    const_iterator end() const { return data() + size; }
    const_iterator constBegin() const { return data(); }
    const_iterator constEnd() const { return data() + size; }

    void swap(QArrayDataPointer &other) noexcept
    {
        qSwap(d, other.d);
        qSwap(b, other.b);
        qSwap(size, other.size);
    }

    void clear()
    {
        QArrayDataPointer tmp(d, b, size);
        d = Data::sharedNull();
        b = reinterpret_cast<T *>(d);
        size = 0;
    }

    bool detach()
    {
        if (d->needsDetach()) {
            QPair<Data *, T *> copy = clone(d->detachFlags());
            QArrayDataPointer old(d, b, size);
            d = copy.first;
            b = copy.second;
            return true;
        }

        return false;
    }

    // forwards from QArrayData
    int allocatedCapacity() { return d->allocatedCapacity(); }
    int constAllocatedCapacity() const { return d->constAllocatedCapacity(); }
    int refCounterValue() const { return d->refCounterValue(); }
    bool ref() { return d->ref(); }
    bool deref() { return d->deref(); }
    bool isMutable() const { return d->isMutable(); }
    bool isStatic() const { return d->isStatic(); }
    bool isShared() const { return d->isShared(); }
    bool needsDetach() const { return d->needsDetach(); }
    size_t detachCapacity(size_t newSize) const { return d->detachCapacity(newSize); }
    typename Data::ArrayOptions &flags() { return reinterpret_cast<typename Data::ArrayOptions &>(d->flags); }
    typename Data::ArrayOptions flags() const { return typename Data::ArrayOption(d->flags); }
    typename Data::ArrayOptions detachFlags() const { return d->detachFlags(); }
    typename Data::ArrayOptions cloneFlags() const { return d->cloneFlags(); }

private:
    Q_REQUIRED_RESULT QPair<Data *, T *> clone(QArrayData::ArrayOptions options) const
    {
        QPair<Data *, T *> pair = Data::allocate(d->detachCapacity(size),
                    options);
        Q_CHECK_PTR(pair.first);
        QArrayDataPointer copy(pair.first, pair.second, 0);
        if (size)
            copy->copyAppend(begin(), end());

        pair.first = copy.d;
        copy.d = Data::sharedNull();
        return pair;
    }

    Data *d;
    T *b;

public:
    uint size;
};

template <class T>
inline bool operator==(const QArrayDataPointer<T> &lhs, const QArrayDataPointer<T> &rhs)
{
    return lhs.data() == rhs.data() && lhs.size == rhs.size;
}

template <class T>
inline bool operator!=(const QArrayDataPointer<T> &lhs, const QArrayDataPointer<T> &rhs)
{
    return lhs.data() != rhs.data() || lhs.size != rhs.size;
}

template <class T>
inline void swap(QArrayDataPointer<T> &p1, QArrayDataPointer<T> &p2)
{
    p1.swap(p2);
}

QT_END_NAMESPACE

#endif // include guard
