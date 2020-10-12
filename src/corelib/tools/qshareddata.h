/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QSHAREDDATA_H
#define QSHAREDDATA_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <QtCore/qhashfunctions.h>

QT_BEGIN_NAMESPACE


template <class T> class QSharedDataPointer;

class QSharedData
{
public:
    mutable QAtomicInt ref;

    inline QSharedData() noexcept : ref(0) { }
    inline QSharedData(const QSharedData &) noexcept : ref(0) { }

    // using the assignment operator would lead to corruption in the ref-counting
    QSharedData &operator=(const QSharedData &) = delete;
    ~QSharedData() = default;
};

template <class T> class QSharedDataPointer
{
public:
    typedef T Type;
    typedef T *pointer;

    inline void detach() { if (d && d->ref.loadRelaxed() != 1) detach_helper(); }
    inline T &operator*() { detach(); return *d; }
    inline const T &operator*() const { return *d; }
    inline T *operator->() { detach(); return d; }
    inline const T *operator->() const noexcept { return d; }
    inline operator T *() { detach(); return d; }
    inline operator const T *() const noexcept { return d; }
    inline T *data() { detach(); return d; }
    inline T *get() { detach(); return d; }
    inline const T *data() const noexcept { return d; }
    inline const T *get() const noexcept { return d; }
    inline const T *constData() const noexcept { return d; }

    inline bool operator==(const QSharedDataPointer<T> &other) const noexcept { return d == other.d; }
    inline bool operator!=(const QSharedDataPointer<T> &other) const noexcept { return d != other.d; }

    inline QSharedDataPointer() noexcept : d(nullptr) { }
    inline ~QSharedDataPointer() { if (d && !d->ref.deref()) delete d; }

    explicit inline QSharedDataPointer(T *data) noexcept : d(data)
    { if (d) d->ref.ref(); }
    inline QSharedDataPointer(const QSharedDataPointer<T> &o) noexcept : d(o.d)
    { if (d) d->ref.ref(); }
    inline QSharedDataPointer<T> &operator=(const QSharedDataPointer<T> &o) noexcept
    {
        if (o.d != d) {
            if (o.d)
                o.d->ref.ref();
            T *old = d;
            d = o.d;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }
    inline QSharedDataPointer &operator=(T *o) noexcept
    {
        if (o != d) {
            if (o)
                o->ref.ref();
            T *old = d;
            d = o;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }
    QSharedDataPointer(QSharedDataPointer &&o) noexcept : d(qExchange(o.d, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QSharedDataPointer)

    inline operator bool () const noexcept { return d != nullptr; }
    inline bool operator!() const noexcept { return !d; }

    inline void swap(QSharedDataPointer &other) noexcept
    { qSwap(d, other.d); }

protected:
    T *clone();

private:
    void detach_helper();

    T *d;
};

template <class T> inline bool operator==(std::nullptr_t p1, const QSharedDataPointer<T> &p2) noexcept
{
    Q_UNUSED(p1);
    return !p2;
}

template <class T> inline bool operator==(const QSharedDataPointer<T> &p1, std::nullptr_t p2) noexcept
{
    Q_UNUSED(p2);
    return !p1;
}

template <class T> class QExplicitlySharedDataPointer
{
public:
    typedef T Type;
    typedef T *pointer;

    inline T &operator*() const { return *d; }
    inline T *operator->() noexcept { return d; }
    inline T *operator->() const noexcept { return d; }
    inline T *data() const noexcept { return d; }
    inline T *get() const noexcept { return d; }
    inline const T *constData() const noexcept { return d; }
    inline T *take() noexcept { T *x = d; d = nullptr; return x; }

    inline void detach() { if (d && d->ref.loadRelaxed() != 1) detach_helper(); }

    inline void reset() noexcept
    {
        if(d && !d->ref.deref())
            delete d;

        d = nullptr;
    }

    inline operator bool () const noexcept { return d != nullptr; }

    inline bool operator==(const QExplicitlySharedDataPointer<T> &other) const noexcept { return d == other.d; }
    inline bool operator!=(const QExplicitlySharedDataPointer<T> &other) const noexcept { return d != other.d; }
    inline bool operator==(const T *ptr) const noexcept { return d == ptr; }
    inline bool operator!=(const T *ptr) const noexcept { return d != ptr; }

    inline QExplicitlySharedDataPointer() noexcept : d(nullptr) { }
    inline ~QExplicitlySharedDataPointer() { if (d && !d->ref.deref()) delete d; }

    explicit inline QExplicitlySharedDataPointer(T *data) noexcept : d(data)
    { if (d) d->ref.ref(); }
    inline QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer<T> &o) noexcept : d(o.d)
    { if (d) d->ref.ref(); }

    template<class X>
    inline QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer<X> &o) noexcept
#ifdef QT_ENABLE_QEXPLICITLYSHAREDDATAPOINTER_STATICCAST
        : d(static_cast<T *>(o.data()))
#else
        : d(o.data())
#endif
    {
        if(d)
            d->ref.ref();
    }

    inline QExplicitlySharedDataPointer<T> &operator=(const QExplicitlySharedDataPointer<T> &o) noexcept
    {
        if (o.d != d) {
            if (o.d)
                o.d->ref.ref();
            T *old = d;
            d = o.d;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }
    inline QExplicitlySharedDataPointer &operator=(T *o) noexcept
    {
        if (o != d) {
            if (o)
                o->ref.ref();
            T *old = d;
            d = o;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }
    inline QExplicitlySharedDataPointer(QExplicitlySharedDataPointer &&o) noexcept : d(qExchange(o.d, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QExplicitlySharedDataPointer)

    inline bool operator!() const noexcept { return !d; }

    inline void swap(QExplicitlySharedDataPointer &other) noexcept
    { qSwap(d, other.d); }

protected:
    T *clone();

private:
    void detach_helper();

    T *d;
};

template <class T>
Q_INLINE_TEMPLATE T *QSharedDataPointer<T>::clone()
{
    return new T(*d);
}

template <class T>
Q_OUTOFLINE_TEMPLATE void QSharedDataPointer<T>::detach_helper()
{
    T *x = clone();
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
}

template <class T>
Q_INLINE_TEMPLATE T *QExplicitlySharedDataPointer<T>::clone()
{
    return new T(*d);
}

template <class T>
Q_OUTOFLINE_TEMPLATE void QExplicitlySharedDataPointer<T>::detach_helper()
{
    T *x = clone();
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
}

template <class T> inline bool operator==(std::nullptr_t p1, const QExplicitlySharedDataPointer<T> &p2) noexcept
{
    Q_UNUSED(p1);
    return !p2;
}

template <class T> inline bool operator==(const QExplicitlySharedDataPointer<T> &p1, std::nullptr_t p2) noexcept
{
    Q_UNUSED(p2);
    return !p1;
}

template <class T>
Q_INLINE_TEMPLATE void swap(QSharedDataPointer<T> &p1, QSharedDataPointer<T> &p2)
{ p1.swap(p2); }

template <class T>
Q_INLINE_TEMPLATE void swap(QExplicitlySharedDataPointer<T> &p1, QExplicitlySharedDataPointer<T> &p2)
{ p1.swap(p2); }

template <class T>
Q_INLINE_TEMPLATE size_t qHash(const QSharedDataPointer<T> &ptr, size_t seed = 0) noexcept
{
    return qHash(ptr.data(), seed);
}
template <class T>
Q_INLINE_TEMPLATE size_t qHash(const QExplicitlySharedDataPointer<T> &ptr, size_t seed = 0) noexcept
{
    return qHash(ptr.data(), seed);
}

template<typename T> Q_DECLARE_TYPEINFO_BODY(QSharedDataPointer<T>, Q_MOVABLE_TYPE);
template<typename T> Q_DECLARE_TYPEINFO_BODY(QExplicitlySharedDataPointer<T>, Q_MOVABLE_TYPE);

#define QT_DECLARE_QSDP_SPECIALIZATION_DTOR(Class) \
    template<> QSharedDataPointer<Class>::~QSharedDataPointer();

#define QT_DECLARE_QSDP_SPECIALIZATION_DTOR_WITH_EXPORT(Class, ExportMacro) \
    template<> ExportMacro QSharedDataPointer<Class>::~QSharedDataPointer();

#define QT_DEFINE_QSDP_SPECIALIZATION_DTOR(Class) \
    template<> QSharedDataPointer<Class>::~QSharedDataPointer() \
    { \
        if (d && !d->ref.deref()) \
            delete d; \
    }

#define QT_DECLARE_QESDP_SPECIALIZATION_DTOR(Class) \
    template<> QExplicitlySharedDataPointer<Class>::~QExplicitlySharedDataPointer();

#define QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(Class, ExportMacro) \
    template<> ExportMacro QExplicitlySharedDataPointer<Class>::~QExplicitlySharedDataPointer();

#define QT_DEFINE_QESDP_SPECIALIZATION_DTOR(Class) \
    template<> QExplicitlySharedDataPointer<Class>::~QExplicitlySharedDataPointer() \
    { \
        if (d && !d->ref.deref()) \
            delete d; \
    }

QT_END_NAMESPACE

#endif // QSHAREDDATA_H
