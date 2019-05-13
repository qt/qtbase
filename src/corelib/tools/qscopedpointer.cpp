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

#include "qscopedpointer.h"

QT_BEGIN_NAMESPACE

/*!
    \class QScopedPointer
    \inmodule QtCore
    \brief The QScopedPointer class stores a pointer to a dynamically allocated object, and deletes it upon destruction.
    \since 4.6
    \reentrant
    \ingroup misc

    Managing heap allocated objects manually is hard and error prone, with the
    common result that code leaks memory and is hard to maintain.
    QScopedPointer is a small utility class that heavily simplifies this by
    assigning stack-based memory ownership to heap allocations, more generally
    called resource acquisition is initialization(RAII).

    QScopedPointer guarantees that the object pointed to will get deleted when
    the current scope disappears.

    Consider this function which does heap allocations, and has various exit points:

    \snippet code/src_corelib_tools_qscopedpointer.cpp 0

    It's encumbered by the manual delete calls. With QScopedPointer, the code
    can be simplified to:

    \snippet code/src_corelib_tools_qscopedpointer.cpp 1

    The code the compiler generates for QScopedPointer is the same as when
    writing it manually. Code that makes use of \a delete are candidates for
    QScopedPointer usage (and if not, possibly another type of smart pointer
    such as QSharedPointer). QScopedPointer intentionally has no copy
    constructor or assignment operator, such that ownership and lifetime is
    clearly communicated.

    The const qualification on a regular C++ pointer can also be expressed with
    a QScopedPointer:

    \snippet code/src_corelib_tools_qscopedpointer.cpp 2

    \section1 Custom Cleanup Handlers

    Arrays as well as pointers that have been allocated with \c malloc must
    not be deleted using \c delete. QScopedPointer's second template parameter
    can be used for custom cleanup handlers.

    The following custom cleanup handlers exist:

    \list
    \li QScopedPointerDeleter - the default, deletes the pointer using \c delete
    \li QScopedPointerArrayDeleter - deletes the pointer using \c{delete []}. Use
       this handler for pointers that were allocated with \c{new []}.
    \li QScopedPointerPodDeleter - deletes the pointer using \c{free()}. Use this
       handler for pointers that were allocated with \c{malloc()}.
    \li QScopedPointerDeleteLater - deletes a pointer by calling \c{deleteLater()}
       on it. Use this handler for pointers to QObject's that are actively
       participating in a QEventLoop.
    \endlist

    You can pass your own classes as handlers, provided that they have a public
    static function \c{void cleanup(T *pointer)}.

    \snippet code/src_corelib_tools_qscopedpointer.cpp 5

    \section1 Forward Declared Pointers

    Classes that are forward declared can be used within QScopedPointer, as
    long as the destructor of the forward declared class is available whenever
    a QScopedPointer needs to clean up.

    Concretely, this means that all classes containing a QScopedPointer that
    points to a forward declared class must have non-inline constructors,
    destructors and assignment operators:

    \snippet code/src_corelib_tools_qscopedpointer.cpp 4

    Otherwise, the compiler outputs a warning about not being able to destruct
    \c MyPrivateClass.

    \sa QSharedPointer
*/

/*! \typedef QScopedPointer::pointer
  \internal
 */

/*!
    \fn template <typename T, typename Cleanup> QScopedPointer<T, Cleanup>::QScopedPointer(T *p = 0)

    Constructs this QScopedPointer instance and sets its pointer to \a p.
*/

/*!
    \fn template <typename T, typename Cleanup> QScopedPointer<T, Cleanup>::~QScopedPointer()

    Destroys this QScopedPointer object. Delete the object its pointer points
    to.
*/

/*!
    \fn template <typename T, typename Cleanup> T *QScopedPointer<T, Cleanup>::data() const

    Returns the value of the pointer referenced by this object. QScopedPointer
    still owns the object pointed to.
*/

/*!
    \fn template <typename T, typename Cleanup> T *QScopedPointer<T, Cleanup>::get() const
    \since 5.11

    Same as data().
*/

/*!
    \fn template <typename T, typename Cleanup> T &QScopedPointer<T, Cleanup>::operator*() const

    Provides access to the scoped pointer's object.

    If the contained pointer is \nullptr, behavior is undefined.
    \sa isNull()
*/

/*!
    \fn template <typename T, typename Cleanup> T *QScopedPointer<T, Cleanup>::operator->() const

    Provides access to the scoped pointer's object.

    If the contained pointer is \nullptr, behavior is undefined.

    \sa isNull()
*/

/*!
    \fn template <typename T, typename Cleanup> QScopedPointer<T, Cleanup>::operator bool() const

    Returns \c true if the contained pointer is not \nullptr.
    This function is suitable for use in \tt if-constructs, like:

    \snippet code/src_corelib_tools_qscopedpointer.cpp 3

    \sa isNull()
*/

/*!
    \fn template <typename T, typename Cleanup> bool operator==(const QScopedPointer<T, Cleanup> &lhs, const QScopedPointer<T, Cleanup> &rhs)

    Returns \c true if \a ptr1 and \a ptr2 refer to the same pointer.
*/


/*!
    \fn template <typename T, typename Cleanup> bool operator!=(const QScopedPointer<T, Cleanup> &lhs, const QScopedPointer<T, Cleanup> &rhs)

    Returns \c true if \a lhs and \a rhs refer to distinct pointers.
*/

/*!
    \fn template <typename T, typename Cleanup> bool operator==(const QScopedPointer<T, Cleanup> &lhs, std::nullptr_t)
    \relates QScopedPointer
    \since 5.8

    Returns \c true if \a lhs refers to \nullptr.

    \sa QScopedPointer::isNull()
*/

/*!
    \fn template <typename T, typename Cleanup> bool operator==(std::nullptr_t, const QScopedPointer<T, Cleanup> &rhs)
    \relates QScopedPointer
    \since 5.8

    Returns \c true if \a rhs refers to \nullptr.

    \sa QScopedPointer::isNull()
*/

/*!
    \fn template <typename T, typename Cleanup> bool operator!=(const QScopedPointer<T, Cleanup> &lhs, std::nullptr_t)
    \relates QScopedPointer
    \since 5.8

    Returns \c true if \a lhs refers to a valid (i.e. non-null) pointer.

    \sa QScopedPointer::isNull()
*/

/*!
    \fn template <typename T, typename Cleanup> bool operator!=(std::nullptr_t, const QScopedPointer<T, Cleanup> &rhs)
    \relates QScopedPointer
    \since 5.8

    Returns \c true if \a rhs refers to a valid (i.e. non-null) pointer.

    \sa QScopedPointer::isNull()
*/

/*!
    \fn template <typename T, typename Cleanup> bool QScopedPointer<T, Cleanup>::isNull() const

    Returns \c true if this object refers to \nullptr.
*/

/*!
    \fn template <typename T, typename Cleanup> void QScopedPointer<T, Cleanup>::reset(T *other = 0)

    Deletes the existing object it is pointing to (if any), and sets its pointer to
    \a other. QScopedPointer now owns \a other and will delete it in its
    destructor.

    To clear the pointer held without deleting the object it points to (and hence take ownership
    of the object), use \l take() instead.
*/

/*!
    \fn template <typename T, typename Cleanup> T *QScopedPointer<T, Cleanup>::take()

    Returns the value of the pointer referenced by this object. The pointer of this
    QScopedPointer object will be reset to \nullptr.

    Callers of this function take ownership of the pointer.
*/

/*! \fn template <typename T, typename Cleanup> bool QScopedPointer<T, Cleanup>::operator!() const

    Returns \c true if this object refers to \nullptr.

    \sa isNull()
*/

/*! \fn template <typename T, typename Cleanup> void QScopedPointer<T, Cleanup>::swap(QScopedPointer<T, Cleanup> &other)
  Swap this pointer with \a other.
 */

/*!
  \class QScopedArrayPointer
  \inmodule QtCore

  \brief The QScopedArrayPointer class stores a pointer to a
  dynamically allocated array of objects, and deletes it upon
  destruction.

  \since 4.6
  \reentrant
  \ingroup misc

  A QScopedArrayPointer is a QScopedPointer that defaults to
  deleting the object it is pointing to with the delete[] operator. It
  also features operator[] for convenience, so we can write:

  \code
    void foo()
    {
        QScopedArrayPointer<int> i(new int[10]);
        i[2] = 42;
        ...
        return; // our integer array is now deleted using delete[]
    }
  \endcode
*/

/*!
    \fn template <typename T, typename Cleanup> QScopedArrayPointer<T, Cleanup>::QScopedArrayPointer()

    Constructs a QScopedArrayPointer instance.
*/

/*!
    \fn template <typename T, typename Cleanup> template <typename D> QScopedArrayPointer<T, Cleanup>::QScopedArrayPointer(D * p)

    Constructs a QScopedArrayPointer and stores the array of objects
    pointed to by \a p.
*/

/*!
    \fn template <typename T, typename Cleanup> T *QScopedArrayPointer<T, Cleanup>::operator[](int i)

    Provides access to entry \a i of the scoped pointer's array of
    objects.

    If the contained pointer is \nullptr, behavior is undefined.

    \sa isNull()
*/

/*!
    \fn template <typename T, typename Cleanup> T *QScopedArrayPointer<T, Cleanup>::operator[](int i) const

    Provides access to entry \a i of the scoped pointer's array of
    objects.

    If the contained pointer is \nullptr behavior is undefined.

    \sa isNull()
*/

/*! \fn template <typename T, typename Cleanup> void QScopedArrayPointer<T, Cleanup>::swap(QScopedArrayPointer<T, Cleanup> &other)
  Swap this pointer with \a other.
 */

QT_END_NAMESPACE
