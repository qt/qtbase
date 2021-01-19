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

#ifndef QPOINTER_H
#define QPOINTER_H

#include <QtCore/qsharedpointer.h>
#include <QtCore/qtypeinfo.h>

#ifndef QT_NO_QOBJECT

QT_BEGIN_NAMESPACE

class QVariant;

template <class T>
class QPointer
{
    Q_STATIC_ASSERT_X(!std::is_pointer<T>::value, "QPointer's template type must not be a pointer type");

    using QObjectType =
        typename std::conditional<std::is_const<T>::value, const QObject, QObject>::type;
    QWeakPointer<QObjectType> wp;
public:
    QPointer() = default;
    inline QPointer(T *p) : wp(p, true) { }
    // compiler-generated copy/move ctor/assignment operators are fine!
    // compiler-generated dtor is fine!

#ifdef Q_QDOC
    // Stop qdoc from complaining about missing function
    ~QPointer();
#endif

    inline void swap(QPointer &other) noexcept { wp.swap(other.wp); }

    inline QPointer<T> &operator=(T* p)
    { wp.assign(static_cast<QObjectType*>(p)); return *this; }

    inline T* data() const
    { return static_cast<T*>(wp.internalData()); }
    inline T* operator->() const
    { return data(); }
    inline T& operator*() const
    { return *data(); }
    inline operator T*() const
    { return data(); }

    inline bool isNull() const
    { return wp.isNull(); }

    inline void clear()
    { wp.clear(); }
};
template <class T> Q_DECLARE_TYPEINFO_BODY(QPointer<T>, Q_MOVABLE_TYPE);

template <class T>
inline bool operator==(const T *o, const QPointer<T> &p)
{ return o == p.operator->(); }

template<class T>
inline bool operator==(const QPointer<T> &p, const T *o)
{ return p.operator->() == o; }

template <class T>
inline bool operator==(T *o, const QPointer<T> &p)
{ return o == p.operator->(); }

template<class T>
inline bool operator==(const QPointer<T> &p, T *o)
{ return p.operator->() == o; }

template<class T>
inline bool operator==(const QPointer<T> &p1, const QPointer<T> &p2)
{ return p1.operator->() == p2.operator->(); }

template <class T>
inline bool operator!=(const T *o, const QPointer<T> &p)
{ return o != p.operator->(); }

template<class T>
inline bool operator!= (const QPointer<T> &p, const T *o)
{ return p.operator->() != o; }

template <class T>
inline bool operator!=(T *o, const QPointer<T> &p)
{ return o != p.operator->(); }

template<class T>
inline bool operator!= (const QPointer<T> &p, T *o)
{ return p.operator->() != o; }

template<class T>
inline bool operator!= (const QPointer<T> &p1, const QPointer<T> &p2)
{ return p1.operator->() != p2.operator->() ; }

template<typename T>
QPointer<T>
qPointerFromVariant(const QVariant &variant)
{
    const auto wp = QtSharedPointer::weakPointerFromVariant_internal(variant);
    return QPointer<T>{qobject_cast<T*>(QtPrivate::EnableInternalData::internalData(wp))};
}

template <class T>
inline void swap(QPointer<T> &p1, QPointer<T> &p2) noexcept
{ p1.swap(p2); }

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QPOINTER_H
