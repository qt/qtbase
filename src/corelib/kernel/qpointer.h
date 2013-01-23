/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPOINTER_H
#define QPOINTER_H

#include <QtCore/qsharedpointer.h>

#ifndef QT_NO_QOBJECT

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QVariant;

class QPointerBase
{
    QWeakPointer<QObject> wp;

protected:
    inline QPointerBase() : wp() { }
    inline QPointerBase(QObject *p) : wp(p, true) { }
    // compiler-generated copy/move ctor/assignment operators are fine! (even though public)
    inline ~QPointerBase() { }

    inline QObject* data() const
    { return wp.data(); }

    inline void assign(QObject *p)
    { wp.assign(p); }

    inline bool isNull() const
    { return wp.isNull(); }

    inline void clear()
    { wp.clear(); }
};

template <class T>
class QPointer : private QPointerBase
{
public:
    inline QPointer() { }
    inline QPointer(T *p) : QPointerBase(p) { }
    // compiler-generated copy/move ctor/assignment operators are fine!
    inline ~QPointer() { }

    inline QPointer<T> &operator=(T* p)
    { QPointerBase::assign(p); return *this; }

    inline T* data() const
    { return static_cast<T*>(QPointerBase::data()); }
    inline T* operator->() const
    { return data(); }
    inline T& operator*() const
    { return *data(); }
    inline operator T*() const
    { return data(); }
#ifdef Q_QDOC
    inline bool isNull() const;
    inline void clear();
#else
    using QPointerBase::isNull;
    using QPointerBase::clear;
#endif
};
template <class T> Q_DECLARE_TYPEINFO_BODY(QPointer<T>, Q_MOVABLE_TYPE);

#if (!defined(Q_CC_SUN) || (__SUNPRO_CC >= 0x580)) // ambiguity between const T * and T *

template <class T>
inline bool operator==(const T *o, const QPointer<T> &p)
{ return o == p.operator->(); }

template<class T>
inline bool operator==(const QPointer<T> &p, const T *o)
{ return p.operator->() == o; }

#else

template<class T>
inline bool operator==(const void *o, const QPointer<T> &p)
{ return o == p.operator->(); }

template<class T>
inline bool operator==(const QPointer<T> &p, const void *o)
{ return p.operator->() == o; }

#endif

template <class T>
inline bool operator==(T *o, const QPointer<T> &p)
{ return o == p.operator->(); }

template<class T>
inline bool operator==(const QPointer<T> &p, T *o)
{ return p.operator->() == o; }

template<class T>
inline bool operator==(const QPointer<T> &p1, const QPointer<T> &p2)
{ return p1.operator->() == p2.operator->(); }


#if (!defined(Q_CC_SUN) || (__SUNPRO_CC >= 0x580)) // ambiguity between const T * and T *

template <class T>
inline bool operator!=(const T *o, const QPointer<T> &p)
{ return o != p.operator->(); }

template<class T>
inline bool operator!= (const QPointer<T> &p, const T *o)
{ return p.operator->() != o; }

#else

template<class T>
inline bool operator!= (const void *o, const QPointer<T> &p)
{ return o != p.operator->(); }

template<class T>
inline bool operator!= (const QPointer<T> &p, const void *o)
{ return p.operator->() != o; }

#endif

template <class T>
inline bool operator!=(T *o, const QPointer<T> &p)
{ return o != p.operator->(); }

template<class T>
inline bool operator!= (const QPointer<T> &p, T *o)
{ return p.operator->() != o; }

template<class T>
inline bool operator!= (const QPointer<T> &p1, const QPointer<T> &p2)
{ return p1.operator->() != p2.operator->() ; }

// Make MSVC < 1400 (2005) handle "if (NULL == p)" syntax
#if defined(Q_CC_MSVC) && (_MSC_VER < 1400)
template<class T>
inline bool operator== (int i, const QPointer<T> &p)
{ Q_ASSERT(i == 0); return !i && p.isNull(); }

template<class T>
inline bool operator!= (int i, const QPointer<T> &p)
{ Q_ASSERT(i == 0); return !i && !p.isNull(); }
#endif

template<typename T>
QPointer<T>
qPointerFromVariant(const QVariant &variant)
{
    return QPointer<T>(qobject_cast<T*>(QtSharedPointer::weakPointerFromVariant_internal(variant).data()));
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_QOBJECT

#endif // QPOINTER_H
