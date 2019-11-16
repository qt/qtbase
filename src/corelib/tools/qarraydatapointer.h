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
#include <QtCore/qcontainertools_impl.h>

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
    enum { pass_parameter_by_value = std::is_fundamental<T>::value || std::is_pointer<T>::value };

    typedef typename std::conditional<pass_parameter_by_value, T, const T &>::type parameter_type;

    QArrayDataPointer() noexcept
        : d(Data::sharedNull()), ptr(Data::sharedNullData()), size(0)
    {
    }

    QArrayDataPointer(const QArrayDataPointer &other) noexcept
        : d(other.d), ptr(other.ptr), size(other.size)
    {
        other.d->ref();
    }

    QArrayDataPointer(Data *header, T *adata, size_t n = 0) noexcept
        : d(header), ptr(adata), size(int(n))
    {
    }

    explicit QArrayDataPointer(QPair<QTypedArrayData<T> *, T *> adata, size_t n = 0)
        : d(adata.first), ptr(adata.second), size(int(n))
    {
        Q_CHECK_PTR(d);
    }

    QArrayDataPointer(QArrayDataPointerRef<T> dd) noexcept
        : d(dd.ptr), ptr(dd.data), size(dd.size)
    {
    }

    QArrayDataPointer &operator=(const QArrayDataPointer &other) noexcept
    {
        QArrayDataPointer tmp(other);
        this->swap(tmp);
        return *this;
    }

    QArrayDataPointer(QArrayDataPointer &&other) noexcept
        : d(other.d), ptr(other.ptr), size(other.size)
    {
        other.d = Data::sharedNull();
        other.ptr = Data::sharedNullData();
        other.size = 0;
    }

    QArrayDataPointer &operator=(QArrayDataPointer &&other) noexcept
    {
        QArrayDataPointer moved(std::move(other));
        this->swap(moved);
        return *this;
    }

    DataOps &operator*() noexcept
    {
        Q_ASSERT(d);
        return *static_cast<DataOps *>(this);
    }

    DataOps *operator->() noexcept
    {
        Q_ASSERT(d);
        return static_cast<DataOps *>(this);
    }

    const DataOps &operator*() const noexcept
    {
        Q_ASSERT(d);
        return *static_cast<const DataOps *>(this);
    }

    const DataOps *operator->() const noexcept
    {
        Q_ASSERT(d);
        return static_cast<const DataOps *>(this);
    }

    ~QArrayDataPointer()
    {
        if (!deref()) {
            if (isMutable())
                (*this)->destroyAll();
            Data::deallocate(d);
        }
    }

    bool isNull() const noexcept
    {
        return d == Data::sharedNull();
    }

    T *data() noexcept { return ptr; }
    const T *data() const noexcept { return ptr; }

    iterator begin(iterator = iterator()) noexcept { return data(); }
    iterator end(iterator = iterator()) noexcept { return data() + size; }
    const_iterator begin(const_iterator = const_iterator()) const noexcept { return data(); }
    const_iterator end(const_iterator = const_iterator()) const noexcept { return data() + size; }
    const_iterator constBegin(const_iterator = const_iterator()) const noexcept { return data(); }
    const_iterator constEnd(const_iterator = const_iterator()) const noexcept { return data() + size; }

    void swap(QArrayDataPointer &other) noexcept
    {
        qSwap(d, other.d);
        qSwap(ptr, other.ptr);
        qSwap(size, other.size);
    }

    void clear() Q_DECL_NOEXCEPT_EXPR(std::is_nothrow_destructible<T>::value)
    {
        QArrayDataPointer tmp;
        swap(tmp);
    }

    bool detach()
    {
        if (d->needsDetach()) {
            QPair<Data *, T *> copy = clone(d->detachFlags());
            QArrayDataPointer old(d, ptr, size);
            d = copy.first;
            ptr = copy.second;
            return true;
        }

        return false;
    }

    // forwards from QArrayData
    size_t allocatedCapacity() noexcept { return d->allocatedCapacity(); }
    size_t constAllocatedCapacity() const noexcept { return d->constAllocatedCapacity(); }
    int refCounterValue() const noexcept { return d->refCounterValue(); }
    bool ref() noexcept { return d->ref(); }
    bool deref() noexcept { return d->deref(); }
    bool isMutable() const noexcept { return d->isMutable(); }
    bool isStatic() const noexcept { return d->isStatic(); }
    bool isShared() const noexcept { return d->isShared(); }
    bool isSharedWith(const QArrayDataPointer &other) const noexcept { return d && d == other.d; }
    bool needsDetach() const noexcept { return d->needsDetach(); }
    size_t detachCapacity(size_t newSize) const noexcept { return d->detachCapacity(newSize); }
    typename Data::ArrayOptions &flags() noexcept { return reinterpret_cast<typename Data::ArrayOptions &>(d->flags); }
    typename Data::ArrayOptions flags() const noexcept { return typename Data::ArrayOption(d->flags); }
    typename Data::ArrayOptions detachFlags() const noexcept { return d->detachFlags(); }
    typename Data::ArrayOptions cloneFlags() const noexcept { return d->cloneFlags(); }

    Data *d_ptr() { return d; }

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

protected:
    Data *d;
    T *ptr;

public:
    int size;
};

template <class T>
inline bool operator==(const QArrayDataPointer<T> &lhs, const QArrayDataPointer<T> &rhs) noexcept
{
    return lhs.data() == rhs.data() && lhs.size == rhs.size;
}

template <class T>
inline bool operator!=(const QArrayDataPointer<T> &lhs, const QArrayDataPointer<T> &rhs) noexcept
{
    return lhs.data() != rhs.data() || lhs.size != rhs.size;
}

template <class T>
inline void qSwap(QArrayDataPointer<T> &p1, QArrayDataPointer<T> &p2) noexcept
{
    p1.swap(p2);
}

QT_END_NAMESPACE

#endif // include guard
