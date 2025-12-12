// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSHAREDDATA_H
#define QSHAREDDATA_H

#include <QtCore/qatomic.h>
#include <QtCore/qcompare.h>
#include <QtCore/qhashfunctions.h>

QT_BEGIN_NAMESPACE

template <class T> class QSharedDataPointer;
template <class T> class QExplicitlySharedDataPointer;

namespace QtPrivate {
template <template <typename> class P, typename T> struct QSharedDataPointerTraits;
template <typename T> struct QSharedDataPointerTraits<QSharedDataPointer, T>
{
    static constexpr bool ImplicitlyDetaches = true;
    using Type = T;
    using pointer = T *;
    // for const-qualified functions:
    using constT = const T;
};

template <typename T> struct QSharedDataPointerTraits<QExplicitlySharedDataPointer, T>
{
    static constexpr bool ImplicitlyDetaches = false;
    using Type = T;
    using pointer = T *;
    // for const-qualified functions:
    using constT = T;
};
}

class QSharedData
{
public:
    mutable QAtomicInt ref;

    QSharedData() noexcept : ref(0) { }
    QSharedData(const QSharedData &) noexcept : ref(0) { }

    // using the assignment operator would lead to corruption in the ref-counting
    QSharedData &operator=(const QSharedData &) = delete;
    ~QSharedData() = default;
};

struct QAdoptSharedDataTag { explicit constexpr QAdoptSharedDataTag() = default; };

// CRTP common base class for both QSharedDataPointer and QExplicitlySharedDataPointer
template <template <typename> class P, typename T> class QSharedDataPointerBase
{
#ifndef Q_QDOC
    using Self = P<T>;
    using Traits = QtPrivate::QSharedDataPointerTraits<P, T>;
    using constT = typename Traits::constT;

protected:
    constexpr QSharedDataPointerBase(T *ptr = nullptr) noexcept : d(ptr) {}

public:
    // When adding anything public to this class, make sure to add the doc version to
    // both QSharedDataPointer and QExplicitlySharedDataPointer.

    using Type = T;
    using pointer = T *;

    void detach() { if (d && d->ref.loadRelaxed() != 1) detach_helper(); }
    T &operator*() { implicitlyDetach(); return *(d.get()); }
    constT &operator*() const { return *(d.get()); }
    T *operator->() { implicitlyDetach(); return d.get(); }
    constT *operator->() const noexcept { return d.get(); }
    operator T *() { implicitlyDetach(); return d.get(); }
    operator const T *() const noexcept { return d.get(); }
    T *data() { implicitlyDetach(); return d.get(); }
    T *get() { implicitlyDetach(); return d.get(); }
    const T *data() const noexcept { return d.get(); }
    const T *get() const noexcept { return d.get(); }
    const T *constData() const noexcept { return d.get(); }
    T *take() noexcept { return std::exchange(d, nullptr).get(); }

    void reset(T *ptr = nullptr) noexcept
    {
        if (ptr != d.get()) {
            if (ptr)
                ptr->ref.ref();
            T *old = std::exchange(d, Qt::totally_ordered_wrapper(ptr)).get();
            if (old && !old->ref.deref())
                destroy(old);
        }
    }

    operator bool () const noexcept { return d != nullptr; }
    bool operator!() const noexcept { return d == nullptr; }

    void swap(Self &other) noexcept
    { qt_ptr_swap(d, other.d); }

private:
    // The concrete class MUST override these, otherwise we will be calling
    // ourselves.
    T *clone() { return static_cast<Self *>(this)->clone(); }
    template <typename... Args> static T *create(Args &&... args)
    { return Self::create(std::forward(args)...); }
    static void destroy(T *ptr) { Self::destroy(ptr); }

    void implicitlyDetach()
    {
        if constexpr (Traits::ImplicitlyDetaches)
            static_cast<Self *>(this)->detach();
    }

    friend bool comparesEqual(const QSharedDataPointerBase &lhs, const QSharedDataPointerBase &rhs) noexcept
    { return lhs.d == rhs.d; }
    friend Qt::strong_ordering
    compareThreeWay(const QSharedDataPointerBase &lhs, const QSharedDataPointerBase &rhs) noexcept
    { return Qt::compareThreeWay(lhs.d, rhs.d); }

    friend bool comparesEqual(const QSharedDataPointerBase &lhs, const T *rhs) noexcept
    { return lhs.d == rhs; }
    friend Qt::strong_ordering
    compareThreeWay(const QSharedDataPointerBase &lhs, const T *rhs) noexcept
    { return Qt::compareThreeWay(lhs.d, rhs); }

    friend bool comparesEqual(const QSharedDataPointerBase &lhs, std::nullptr_t) noexcept
    { return lhs.d == nullptr; }
    friend Qt::strong_ordering
    compareThreeWay(const QSharedDataPointerBase &lhs, std::nullptr_t) noexcept
    { return Qt::compareThreeWay(lhs.d, nullptr); }

    friend size_t qHash(const QSharedDataPointerBase &ptr, size_t seed = 0) noexcept
    { return qHash(ptr.data(), seed); }

protected:
    void detach_helper();

    Qt::totally_ordered_wrapper<T *> d;
#endif // !Q_QDOC
};

template <typename T>
class QSharedDataPointer : public QSharedDataPointerBase<QSharedDataPointer, T>
{
    using Base = QSharedDataPointerBase<QSharedDataPointer, T>;
    friend Base;
public:
    typedef T Type;
    typedef T *pointer;

    void detach() { Base::detach(); }
#ifdef Q_QDOC
    T &operator*();
    const T &operator*() const;
    T *operator->();
    const T *operator->() const noexcept;
    operator T *();
    operator const T *() const noexcept;
    T *data();
    T *get();
    const T *data() const noexcept;
    const T *get() const noexcept;
    const T *constData() const noexcept;
    T *take() noexcept;
#endif

    Q_NODISCARD_CTOR
    QSharedDataPointer() noexcept : Base(nullptr) { }
    ~QSharedDataPointer() { if (d && !d->ref.deref()) destroy(d.get()); }

    Q_NODISCARD_CTOR
    explicit QSharedDataPointer(T *data) noexcept : Base(data)
    { if (d) d->ref.ref(); }
    Q_NODISCARD_CTOR
    QSharedDataPointer(T *data, QAdoptSharedDataTag) noexcept : Base(data)
    {}
    Q_NODISCARD_CTOR
    QSharedDataPointer(const QSharedDataPointer &o) noexcept : Base(o.d.get())
    { if (d) d->ref.ref(); }

    QSharedDataPointer &operator=(const QSharedDataPointer &o) noexcept
    {
        reset(o.d.get());
        return *this;
    }
    inline QSharedDataPointer &operator=(T *o) noexcept
    {
        reset(o);
        return *this;
    }
    Q_NODISCARD_CTOR
    QSharedDataPointer(QSharedDataPointer &&o) noexcept
        : Base(std::exchange(o.d, nullptr).get())
    {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QSharedDataPointer)

#ifdef Q_QDOC
    void reset(T *ptr = nullptr) noexcept;

    operator bool () const noexcept;
    bool operator!() const noexcept;

    void swap(QSharedDataPointer &other) noexcept;
#else
    using Base::reset;
    using Base::swap;
#endif

protected:
    T *clone();
    template <typename... Args> static T *create(Args &&... args)
    { return new T(std::forward(args)...); }
    static void destroy(T *ptr) { delete ptr; }

private:
    Q_DECLARE_STRONGLY_ORDERED(QSharedDataPointer)
    Q_DECLARE_STRONGLY_ORDERED(QSharedDataPointer, T*)
    Q_DECLARE_STRONGLY_ORDERED(QSharedDataPointer, std::nullptr_t)

    using Base::d;
};

template <typename T>
class QExplicitlySharedDataPointer : public QSharedDataPointerBase<QExplicitlySharedDataPointer, T>
{
    using Base = QSharedDataPointerBase<QExplicitlySharedDataPointer, T>;
    friend Base;
public:
    typedef T Type;
    typedef T *pointer;

    // override to make explicit. Can use explicit(!ImplicitlyShared) once we
    // can depend on C++20.
    explicit operator T *() { return d.get(); }
    explicit operator const T *() const noexcept { return d.get(); }

    // override to make const. There is no const(cond), but we could use
    // requires(!ImplicitlyShared)
    T *data() const noexcept { return d.get(); }
    T *get() const noexcept { return d.get(); }

#ifdef Q_QDOC
    T &operator*() const;
    T *operator->() noexcept;
    T *operator->() const noexcept;
    T *data() const noexcept;
    T *get() const noexcept;
    const T *constData() const noexcept;
    T *take() noexcept;
#endif

    void detach() { Base::detach(); }

    Q_NODISCARD_CTOR
    QExplicitlySharedDataPointer() noexcept : Base(nullptr) { }
    ~QExplicitlySharedDataPointer() { if (d && !d->ref.deref()) delete d.get(); }

    Q_NODISCARD_CTOR
    explicit QExplicitlySharedDataPointer(T *data) noexcept : Base(data)
    { if (d) d->ref.ref(); }
    Q_NODISCARD_CTOR
    QExplicitlySharedDataPointer(T *data, QAdoptSharedDataTag) noexcept : Base(data)
    {}
    Q_NODISCARD_CTOR
    QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer &o) noexcept : Base(o.d.get())
    { if (d) d->ref.ref(); }

    template<typename X>
    Q_NODISCARD_CTOR
    QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer<X> &o) noexcept
#ifdef QT_ENABLE_QEXPLICITLYSHAREDDATAPOINTER_STATICCAST
#error This macro has been removed in Qt 6.9.
#endif
        : Base(o.data())
    { if (d) d->ref.ref(); }

    QExplicitlySharedDataPointer &operator=(const QExplicitlySharedDataPointer &o) noexcept
    {
        reset(o.d.get());
        return *this;
    }
    QExplicitlySharedDataPointer &operator=(T *o) noexcept
    {
        reset(o);
        return *this;
    }
    Q_NODISCARD_CTOR
    QExplicitlySharedDataPointer(QExplicitlySharedDataPointer &&o) noexcept
        : Base(std::exchange(o.d, nullptr).get())
    {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QExplicitlySharedDataPointer)

#ifdef Q_QDOC
    void reset(T *ptr = nullptr) noexcept;

    operator bool () const noexcept;
    bool operator!() const noexcept;

    void swap(QExplicitlySharedDataPointer &other) noexcept;
#else
    using Base::swap;
    using Base::reset;
#endif

protected:
    T *clone();
    template <typename... Args> static T *create(Args &&... args)
    { return new T(std::forward(args)...); }
    static void destroy(T *ptr) { delete ptr; }

private:
    Q_DECLARE_STRONGLY_ORDERED(QExplicitlySharedDataPointer)
    Q_DECLARE_STRONGLY_ORDERED(QExplicitlySharedDataPointer, const T*)
    Q_DECLARE_STRONGLY_ORDERED(QExplicitlySharedDataPointer, std::nullptr_t)

    using Base::d;
};

// Declared here and as Q_OUTOFLINE_TEMPLATE to work-around MSVC bug causing missing symbols at link time.
template <typename T>
Q_INLINE_TEMPLATE T *QSharedDataPointer<T>::clone()
{
    return new T(*this->d);
}

template <typename T>
Q_INLINE_TEMPLATE T *QExplicitlySharedDataPointer<T>::clone()
{
    return new T(*this->d.get());
}

template <template <typename> class P, typename T> Q_OUTOFLINE_TEMPLATE void
QSharedDataPointerBase<P, T>::detach_helper()
{
    T *x = clone();
    x->ref.ref();
    if (!d->ref.deref())
        delete d.get();
    d.reset(x);
}

template <typename T>
void swap(QSharedDataPointer<T> &p1, QSharedDataPointer<T> &p2) noexcept
{ p1.swap(p2); }

template <typename T>
void swap(QExplicitlySharedDataPointer<T> &p1, QExplicitlySharedDataPointer<T> &p2) noexcept
{ p1.swap(p2); }

template<typename T> Q_DECLARE_TYPEINFO_BODY(QSharedDataPointer<T>, Q_RELOCATABLE_TYPE);
template<typename T> Q_DECLARE_TYPEINFO_BODY(QExplicitlySharedDataPointer<T>, Q_RELOCATABLE_TYPE);

#define QT_DECLARE_QSDP_SPECIALIZATION_DTOR(Class) \
    template<> QSharedDataPointer<Class>::~QSharedDataPointer();

#define QT_DECLARE_QSDP_SPECIALIZATION_DTOR_WITH_EXPORT(Class, ExportMacro) \
    template<> ExportMacro QSharedDataPointer<Class>::~QSharedDataPointer();

#define QT_DEFINE_QSDP_SPECIALIZATION_DTOR(Class) \
    template<> QSharedDataPointer<Class>::~QSharedDataPointer() \
    { \
        if (d && !d->ref.deref()) \
            delete d.get(); \
    }

#define QT_DECLARE_QESDP_SPECIALIZATION_DTOR(Class) \
    template<> QExplicitlySharedDataPointer<Class>::~QExplicitlySharedDataPointer();

#define QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(Class, ExportMacro) \
    template<> ExportMacro QExplicitlySharedDataPointer<Class>::~QExplicitlySharedDataPointer();

#define QT_DEFINE_QESDP_SPECIALIZATION_DTOR(Class) \
    template<> QExplicitlySharedDataPointer<Class>::~QExplicitlySharedDataPointer() \
    { \
        if (d && !d->ref.deref()) \
            delete d.get(); \
    }

QT_END_NAMESPACE

#endif // QSHAREDDATA_H
