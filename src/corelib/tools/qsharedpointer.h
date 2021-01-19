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

#ifndef QSHAREDPOINTER_H
#define QSHAREDPOINTER_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <QtCore/qshareddata.h>

#ifndef Q_QDOC
# include <QtCore/qsharedpointer_impl.h>
#else

#include <memory> // for std::shared_ptr

QT_BEGIN_NAMESPACE


// These classes are here to fool qdoc into generating a better documentation

template <class T>
class QSharedPointer
{
public:
    // basic accessor functions
    T *data() const;
    T *get() const;
    bool isNull() const;
    operator bool() const;
    bool operator!() const;
    T &operator*() const;
    T *operator ->() const;

    // constructors
    QSharedPointer();
    template <typename X> explicit QSharedPointer(X *ptr);
    template <typename X, typename Deleter> QSharedPointer(X *ptr, Deleter d);
    QSharedPointer(std::nullptr_t);
    template <typename Deleter> QSharedPointer(std::nullptr_t, Deleter d);
    QSharedPointer(const QSharedPointer<T> &other);
    QSharedPointer(const QWeakPointer<T> &other);

    ~QSharedPointer() { }

    QSharedPointer<T> &operator=(const QSharedPointer<T> &other);
    QSharedPointer<T> &operator=(const QWeakPointer<T> &other);

    void swap(QSharedPointer<T> &other);

    QWeakPointer<T> toWeakRef() const;

    void clear();

    void reset();
    void reset(T *t);
    template <typename Deleter>
    void reset(T *t, Deleter deleter);

    // casts:
    template <class X> QSharedPointer<X> staticCast() const;
    template <class X> QSharedPointer<X> dynamicCast() const;
    template <class X> QSharedPointer<X> constCast() const;
    template <class X> QSharedPointer<X> objectCast() const;

    template <typename... Args>
    static inline QSharedPointer<T> create(Args &&... args);
};

template <class T>
class QWeakPointer
{
public:
    // basic accessor functions
    bool isNull() const;
    operator bool() const;
    bool operator!() const;

    // constructors:
    QWeakPointer();
    QWeakPointer(const QWeakPointer<T> &other);
    QWeakPointer(const QSharedPointer<T> &other);

    ~QWeakPointer();

    QWeakPointer<T> &operator=(const QWeakPointer<T> &other);
    QWeakPointer<T> &operator=(const QSharedPointer<T> &other);

    QWeakPointer(const QObject *other);
    QWeakPointer<T> &operator=(const QObject *other);

    void swap(QWeakPointer<T> &other);

    T *data() const;
    void clear();

    QSharedPointer<T> toStrongRef() const;
    QSharedPointer<T> lock() const;
};

template <class T>
class QEnableSharedFromThis
{
public:
    QSharedPointer<T> sharedFromThis();
    QSharedPointer<const T> sharedFromThis() const;
};

template<class T, class X> bool operator==(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator!=(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator==(const QSharedPointer<T> &ptr1, const X *ptr2);
template<class T, class X> bool operator!=(const QSharedPointer<T> &ptr1, const X *ptr2);
template<class T, class X> bool operator==(const T *ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator!=(const T *ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator==(const QWeakPointer<T> &ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator!=(const QWeakPointer<T> &ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator==(const QSharedPointer<T> &ptr1, const QWeakPointer<X> &ptr2);
template<class T, class X> bool operator!=(const QSharedPointer<T> &ptr1, const QWeakPointer<X> &ptr2);
template<class T> bool operator==(const QSharedPointer<T> &lhs, std::nullptr_t);
template<class T> bool operator!=(const QSharedPointer<T> &lhs, std::nullptr_t);
template<class T> bool operator==(std::nullptr_t, const QSharedPointer<T> &rhs);
template<class T> bool operator!=(std::nullptr_t, const QSharedPointer<T> &rhs);
template<class T> bool operator==(const QWeakPointer<T> &lhs, std::nullptr_t);
template<class T> bool operator!=(const QWeakPointer<T> &lhs, std::nullptr_t);
template<class T> bool operator==(std::nullptr_t, const QWeakPointer<T> &rhs);
template<class T> bool operator!=(std::nullptr_t, const QWeakPointer<T> &rhs);

template <class X, class T> QSharedPointer<X> qSharedPointerCast(const QSharedPointer<T> &other);
template <class X, class T> QSharedPointer<X> qSharedPointerCast(const QWeakPointer<T> &other);
template <class X, class T> QSharedPointer<X> qSharedPointerDynamicCast(const QSharedPointer<T> &src);
template <class X, class T> QSharedPointer<X> qSharedPointerDynamicCast(const QWeakPointer<T> &src);
template <class X, class T> QSharedPointer<X> qSharedPointerConstCast(const QSharedPointer<T> &src);
template <class X, class T> QSharedPointer<X> qSharedPointerConstCast(const QWeakPointer<T> &src);
template <class X, class T> QSharedPointer<X> qSharedPointerObjectCast(const QSharedPointer<T> &src);
template <class X, class T> QSharedPointer<X> qSharedPointerObjectCast(const QWeakPointer<T> &src);
template <typename X, class T> std::shared_ptr<X> qobject_pointer_cast(const std::shared_ptr<T> &src);
template <typename X, class T> std::shared_ptr<X> qobject_pointer_cast(std::shared_ptr<T> &&src);
template <typename X, class T> std::shared_ptr<X> qSharedPointerObjectCast(const std::shared_ptr<T> &src);
template <typename X, class T> std::shared_ptr<X> qSharedPointerObjectCast(std::shared_ptr<T> &&src);

template <class X, class T> QWeakPointer<X> qWeakPointerCast(const QWeakPointer<T> &src);

QT_END_NAMESPACE

#endif // Q_QDOC

#endif // QSHAREDPOINTER_H
