/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QSCOPEDPOINTER_H
#define QSCOPEDPOINTER_H

#include <QtCore/qglobal.h>

#include <stdlib.h>

QT_BEGIN_NAMESPACE

template <typename T>
struct QScopedPointerDeleter
{
    static inline void cleanup(T *pointer)
    {
        // Enforce a complete type.
        // If you get a compile error here, read the section on forward declared
        // classes in the QScopedPointer documentation.
        typedef char IsIncompleteType[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(IsIncompleteType);

        delete pointer;
    }
};

template <typename T>
struct QScopedPointerArrayDeleter
{
    static inline void cleanup(T *pointer)
    {
        // Enforce a complete type.
        // If you get a compile error here, read the section on forward declared
        // classes in the QScopedPointer documentation.
        typedef char IsIncompleteType[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(IsIncompleteType);

        delete [] pointer;
    }
};

struct QScopedPointerPodDeleter
{
    static inline void cleanup(void *pointer) { if (pointer) free(pointer); }
};

#ifndef QT_NO_QOBJECT
template <typename T>
struct QScopedPointerObjectDeleteLater
{
    static inline void cleanup(T *pointer) { if (pointer) pointer->deleteLater(); }
};

class QObject;
typedef QScopedPointerObjectDeleteLater<QObject> QScopedPointerDeleteLater;
#endif

template <typename T, typename Cleanup = QScopedPointerDeleter<T> >
class QScopedPointer
{
    typedef T *QScopedPointer:: *RestrictedBool;
public:
    explicit QScopedPointer(T *p = nullptr) noexcept : d(p)
    {
    }

    inline ~QScopedPointer()
    {
        T *oldD = this->d;
        Cleanup::cleanup(oldD);
    }

    inline T &operator*() const
    {
        Q_ASSERT(d);
        return *d;
    }

    T *operator->() const noexcept
    {
        return d;
    }

    bool operator!() const noexcept
    {
        return !d;
    }

#if defined(Q_QDOC)
    inline operator bool() const
    {
        return isNull() ? nullptr : &QScopedPointer::d;
    }
#else
    operator RestrictedBool() const noexcept
    {
        return isNull() ? nullptr : &QScopedPointer::d;
    }
#endif

    T *data() const noexcept
    {
        return d;
    }

    T *get() const noexcept
    {
        return d;
    }

    bool isNull() const noexcept
    {
        return !d;
    }

    void reset(T *other = nullptr) noexcept(noexcept(Cleanup::cleanup(std::declval<T *>())))
    {
        if (d == other)
            return;
        T *oldD = d;
        d = other;
        Cleanup::cleanup(oldD);
    }

    T *take() noexcept
    {
        T *oldD = d;
        d = nullptr;
        return oldD;
    }

    void swap(QScopedPointer<T, Cleanup> &other) noexcept
    {
        qSwap(d, other.d);
    }

    typedef T *pointer;

protected:
    T *d;

private:
    Q_DISABLE_COPY(QScopedPointer)
};

template <class T, class Cleanup>
inline bool operator==(const QScopedPointer<T, Cleanup> &lhs, const QScopedPointer<T, Cleanup> &rhs) noexcept
{
    return lhs.data() == rhs.data();
}

template <class T, class Cleanup>
inline bool operator!=(const QScopedPointer<T, Cleanup> &lhs, const QScopedPointer<T, Cleanup> &rhs) noexcept
{
    return lhs.data() != rhs.data();
}

template <class T, class Cleanup>
inline bool operator==(const QScopedPointer<T, Cleanup> &lhs, std::nullptr_t) noexcept
{
    return lhs.isNull();
}

template <class T, class Cleanup>
inline bool operator==(std::nullptr_t, const QScopedPointer<T, Cleanup> &rhs) noexcept
{
    return rhs.isNull();
}

template <class T, class Cleanup>
inline bool operator!=(const QScopedPointer<T, Cleanup> &lhs, std::nullptr_t) noexcept
{
    return !lhs.isNull();
}

template <class T, class Cleanup>
inline bool operator!=(std::nullptr_t, const QScopedPointer<T, Cleanup> &rhs) noexcept
{
    return !rhs.isNull();
}

template <class T, class Cleanup>
inline void swap(QScopedPointer<T, Cleanup> &p1, QScopedPointer<T, Cleanup> &p2) noexcept
{ p1.swap(p2); }

template <typename T, typename Cleanup = QScopedPointerArrayDeleter<T> >
class QScopedArrayPointer : public QScopedPointer<T, Cleanup>
{
    template <typename Ptr>
    using if_same_type = typename std::enable_if<std::is_same<typename std::remove_cv<T>::type, Ptr>::value, bool>::type;
public:
    inline QScopedArrayPointer() : QScopedPointer<T, Cleanup>(nullptr) {}

    template <typename D, if_same_type<D> = true>
    explicit QScopedArrayPointer(D *p)
        : QScopedPointer<T, Cleanup>(p)
    {
    }

    inline T &operator[](int i)
    {
        return this->d[i];
    }

    inline const T &operator[](int i) const
    {
        return this->d[i];
    }

    void swap(QScopedArrayPointer &other) noexcept // prevent QScopedPointer <->QScopedArrayPointer swaps
    { QScopedPointer<T, Cleanup>::swap(other); }

private:
    explicit inline QScopedArrayPointer(void *) {
        // Enforce the same type.

        // If you get a compile error here, make sure you declare
        // QScopedArrayPointer with the same template type as you pass to the
        // constructor. See also the QScopedPointer documentation.

        // Storing a scalar array as a pointer to a different type is not
        // allowed and results in undefined behavior.
    }

    Q_DISABLE_COPY(QScopedArrayPointer)
};

template <typename T, typename Cleanup>
inline void swap(QScopedArrayPointer<T, Cleanup> &lhs, QScopedArrayPointer<T, Cleanup> &rhs) noexcept
{ lhs.swap(rhs); }

QT_END_NAMESPACE

#endif // QSCOPEDPOINTER_H
