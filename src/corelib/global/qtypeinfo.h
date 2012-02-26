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

#ifndef QTYPEINFO_H
#define QTYPEINFO_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

/*
   QTypeInfo     - type trait functionality
   qIsDetached   - data sharing functionality
*/

/*
  The catch-all template.
*/

template <typename T> inline bool qIsDetached(T &) { return true; }

template <typename T>
class QTypeInfo
{
public:
    enum {
        isPointer = false,
        isComplex = true,
        isStatic = true,
        isLarge = (sizeof(T)>sizeof(void*)),
        isDummy = false,
        sizeOf = sizeof(T)
    };
};

template<>
class QTypeInfo<void>
{
public:
    enum {
        isPointer = false,
        isComplex = false,
        isStatic = false,
        isLarge = false,
        isDummy = false,
        sizeOf = 0
    };
};

template <typename T>
class QTypeInfo<T*>
{
public:
    enum {
        isPointer = true,
        isComplex = false,
        isStatic = false,
        isLarge = false,
        isDummy = false,
        sizeOf = sizeof(T*)
    };
};


#define Q_DECLARE_MOVABLE_CONTAINER(CONTAINER) \
template <typename T> class CONTAINER; \
template <typename T> \
class QTypeInfo< CONTAINER<T> > \
{ \
public: \
    enum { \
        isPointer = false, \
        isComplex = true, \
        isStatic = false, \
        isLarge = (sizeof(CONTAINER<T>) > sizeof(void*)), \
        isDummy = false, \
        sizeOf = sizeof(CONTAINER<T>) \
    }; \
};

Q_DECLARE_MOVABLE_CONTAINER(QList)
Q_DECLARE_MOVABLE_CONTAINER(QVector)
Q_DECLARE_MOVABLE_CONTAINER(QQueue)
Q_DECLARE_MOVABLE_CONTAINER(QStack)
Q_DECLARE_MOVABLE_CONTAINER(QLinkedList)
Q_DECLARE_MOVABLE_CONTAINER(QSet)

#undef Q_DECLARE_MOVABLE_CONTAINER

/*
   Specialize a specific type with:

     Q_DECLARE_TYPEINFO(type, flags);

   where 'type' is the name of the type to specialize and 'flags' is
   logically-OR'ed combination of the flags below.
*/
enum { /* TYPEINFO flags */
    Q_COMPLEX_TYPE = 0,
    Q_PRIMITIVE_TYPE = 0x1,
    Q_STATIC_TYPE = 0,
    Q_MOVABLE_TYPE = 0x2,
    Q_DUMMY_TYPE = 0x4
};

#define Q_DECLARE_TYPEINFO_BODY(TYPE, FLAGS) \
class QTypeInfo<TYPE > \
{ \
public: \
    enum { \
        isComplex = (((FLAGS) & Q_PRIMITIVE_TYPE) == 0), \
        isStatic = (((FLAGS) & (Q_MOVABLE_TYPE | Q_PRIMITIVE_TYPE)) == 0), \
        isLarge = (sizeof(TYPE)>sizeof(void*)), \
        isPointer = false, \
        isDummy = (((FLAGS) & Q_DUMMY_TYPE) != 0), \
        sizeOf = sizeof(TYPE) \
    }; \
    static inline const char *name() { return #TYPE; } \
}

#define Q_DECLARE_TYPEINFO(TYPE, FLAGS) \
template<> \
Q_DECLARE_TYPEINFO_BODY(TYPE, FLAGS)

/* Specialize QTypeInfo for QFlags<T> */
template<typename T> class QFlags;
template<typename T>
Q_DECLARE_TYPEINFO_BODY(QFlags<T>, Q_PRIMITIVE_TYPE);

/*
   Specialize a shared type with:

     Q_DECLARE_SHARED(type);

   where 'type' is the name of the type to specialize.  NOTE: shared
   types must declare a 'bool isDetached(void) const;' member for this
   to work.
*/
#ifdef QT_NO_STL
#define Q_DECLARE_SHARED_STL(TYPE)
#else
#define Q_DECLARE_SHARED_STL(TYPE) \
QT_END_NAMESPACE \
namespace std { \
    template<> inline void swap<QT_PREPEND_NAMESPACE(TYPE)>(QT_PREPEND_NAMESPACE(TYPE) &value1, QT_PREPEND_NAMESPACE(TYPE) &value2) \
    { swap(value1.data_ptr(), value2.data_ptr()); } \
} \
QT_BEGIN_NAMESPACE
#endif

#define Q_DECLARE_SHARED(TYPE)                                          \
template <> inline bool qIsDetached<TYPE>(TYPE &t) { return t.isDetached(); } \
template <> inline void qSwap<TYPE>(TYPE &value1, TYPE &value2) \
{ qSwap(value1.data_ptr(), value2.data_ptr()); } \
Q_DECLARE_SHARED_STL(TYPE)

/*
   QTypeInfo primitive specializations
*/
Q_DECLARE_TYPEINFO(bool, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(char, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(signed char, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(uchar, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(short, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(ushort, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(int, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(uint, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(long, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(ulong, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(qint64, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(quint64, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(float, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(double, Q_PRIMITIVE_TYPE);
#ifndef Q_OS_DARWIN
Q_DECLARE_TYPEINFO(long double, Q_PRIMITIVE_TYPE);
#endif

QT_END_NAMESPACE
QT_END_HEADER

#endif // QTYPEINFO_H
