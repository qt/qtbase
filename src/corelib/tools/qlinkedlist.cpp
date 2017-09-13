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

#include "qlinkedlist.h"

QT_BEGIN_NAMESPACE

const QLinkedListData QLinkedListData::shared_null = {
    const_cast<QLinkedListData *>(&QLinkedListData::shared_null),
    const_cast<QLinkedListData *>(&QLinkedListData::shared_null),
    Q_REFCOUNT_INITIALIZE_STATIC, 0, true
};

/*! \class QLinkedList
    \inmodule QtCore
    \brief The QLinkedList class is a template class that provides linked lists.

    \ingroup tools
    \ingroup shared

    \reentrant

    QLinkedList\<T\> is one of Qt's generic \l{container classes}. It
    stores a list of values and provides iterator-based access as
    well as \l{constant time} insertions and removals.

    QList\<T\>, QLinkedList\<T\>, and QVector\<T\> provide similar
    functionality. Here's an overview:

    \list
    \li For most purposes, QList is the right class to use. Its
       index-based API is more convenient than QLinkedList's
       iterator-based API, and it is usually faster than
       QVector because of the way it stores its items in
       memory (see \l{Algorithmic Complexity} for details).
       It also expands to less code in your executable.
    \li If you need a real linked list, with guarantees of \l{constant
       time} insertions in the middle of the list and iterators to
       items rather than indexes, use QLinkedList.
    \li If you want the items to occupy adjacent memory positions,
       use QVector.
    \endlist

    Here's an example of a QLinkedList that stores integers and a
    QLinkedList that stores QTime values:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 0

    QLinkedList stores a list of items. The default constructor
    creates an empty list. To insert items into the list, you can use
    operator<<():

    \snippet code/src_corelib_tools_qlinkedlist.cpp 1

    If you want to get the first or last item in a linked list, use
    first() or last(). If you want to remove an item from either end
    of the list, use removeFirst() or removeLast(). If you want to
    remove all occurrences of a given value in the list, use
    removeAll().

    A common requirement is to remove the first or last item in the
    list and do something with it. For this, QLinkedList provides
    takeFirst() and takeLast(). Here's a loop that removes the items
    from a list one at a time and calls \c delete on them:
    \snippet code/src_corelib_tools_qlinkedlist.cpp 2

    QLinkedList's value type must be an \l {assignable data type}. This
    covers most data types that are commonly used, but the compiler
    won't let you, for example, store a QWidget as a value; instead,
    store a QWidget *. A few functions have additional requirements;
    for example, contains() and removeAll() expect the value type to
    support \c operator==().  These requirements are documented on a
    per-function basis.

    If you want to insert, modify, or remove items in the middle of
    the list, you must use an iterator. QLinkedList provides both
    \l{Java-style iterators} (QLinkedListIterator and
    QMutableLinkedListIterator) and \l{STL-style iterators}
    (QLinkedList::const_iterator and QLinkedList::iterator). See the
    documentation for these classes for details.

    \sa QLinkedListIterator, QMutableLinkedListIterator, QList, QVector
*/

/*! \fn template <class T> QLinkedList<T>::QLinkedList()

    Constructs an empty list.
*/

/*!
    \fn template <class T> QLinkedList<T>::QLinkedList(QLinkedList<T> &&other)

    Move-constructs a QLinkedList instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*! \fn template <class T> QLinkedList<T>::QLinkedList(const QLinkedList<T> &other)

    Constructs a copy of \a other.

    This operation occurs in \l{constant time}, because QLinkedList
    is \l{implicitly shared}. This makes returning a QLinkedList from
    a function very fast. If a shared instance is modified, it will
    be copied (copy-on-write), and this takes \l{linear time}.

    \sa operator=()
*/

/*! \fn template <class T> QLinkedList<T>::QLinkedList(std::initializer_list<T> list)
    \since 5.2

    Constructs a list from the std::initializer_list specified by \a list.

    This constructor is only enabled if the compiler supports C++11
    initializer lists.
*/

/*! \fn template <class T> QLinkedList<T>::~QLinkedList()

    Destroys the list. References to the values in the list, and all
    iterators over this list, become invalid.
*/

/*! \fn template <class T> QLinkedList<T> &QLinkedList<T>::operator=(const QLinkedList<T> &other)

    Assigns \a other to this list and returns a reference to this
    list.
*/

/*! \fn template <class T> void QLinkedList<T>::swap(QLinkedList<T> &other)
    \since 4.8

    Swaps list \a other with this list. This operation is very
    fast and never fails.
*/

/*! \fn template <class T> bool QLinkedList<T>::operator==(const QLinkedList<T> &other) const

    Returns \c true if \a other is equal to this list; otherwise returns
    false.

    Two lists are considered equal if they contain the same values in
    the same order.

    This function requires the value type to implement \c
    operator==().

    \sa operator!=()
*/

/*! \fn template <class T> bool QLinkedList<T>::operator!=(const QLinkedList<T> &other) const

    Returns \c true if \a other is not equal to this list; otherwise
    returns \c false.

    Two lists are considered equal if they contain the same values in
    the same order.

    This function requires the value type to implement \c
    operator==().

    \sa operator==()
*/

/*! \fn template <class T> int QLinkedList<T>::size() const

    Returns the number of items in the list.

    \sa isEmpty(), count()
*/

/*! \fn template <class T> void QLinkedList<T>::detach()

    \internal
*/

/*! \fn template <class T> bool QLinkedList<T>::isDetached() const

    \internal
*/

/*! \fn template <class T> void QLinkedList<T>::setSharable(bool sharable)

    \internal
*/

/*! \fn template <class T> bool QLinkedList<T>::isSharedWith(const QLinkedList<T> &other) const

    \internal
*/

/*! \fn template <class T> bool QLinkedList<T>::isEmpty() const

    Returns \c true if the list contains no items; otherwise returns
    false.

    \sa size()
*/

/*! \fn template <class T> void QLinkedList<T>::clear()

    Removes all the items in the list.

    \sa removeAll()
*/

/*! \fn template <class T> void QLinkedList<T>::append(const T &value)

    Inserts \a value at the end of the list.

    Example:
    \snippet code/src_corelib_tools_qlinkedlist.cpp 3

    This is the same as list.insert(end(), \a value).

    \sa operator<<(), prepend(), insert()
*/

/*! \fn template <class T> void QLinkedList<T>::prepend(const T &value)

    Inserts \a value at the beginning of the list.

    Example:
    \snippet code/src_corelib_tools_qlinkedlist.cpp 4

    This is the same as list.insert(begin(), \a value).

    \sa append(), insert()
*/

/*! \fn template <class T> int QLinkedList<T>::removeAll(const T &value)

    Removes all occurrences of \a value in the list.

    Example:
    \snippet code/src_corelib_tools_qlinkedlist.cpp 5

    This function requires the value type to have an implementation of
    \c operator==().

    \sa insert()
*/

/*!
    \fn template <class T> bool QLinkedList<T>::removeOne(const T &value)
    \since 4.4

    Removes the first occurrences of \a value in the list. Returns \c true on
    success; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_tools_qlinkedlist.cpp 6

    This function requires the value type to have an implementation of
    \c operator==().

    \sa insert()
*/

/*! \fn template <class T> bool QLinkedList<T>::contains(const T &value) const

    Returns \c true if the list contains an occurrence of \a value;
    otherwise returns \c false.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa QLinkedListIterator::findNext(), QLinkedListIterator::findPrevious()
*/

/*! \fn template <class T> int QLinkedList<T>::count(const T &value) const

    Returns the number of occurrences of \a value in the list.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa contains()
*/

/*! \fn template <class T> bool QLinkedList<T>::startsWith(const T &value) const
    \since 4.5

    Returns \c true if the list is not empty and its first
    item is equal to \a value; otherwise returns \c false.

    \sa isEmpty(), first()
*/

/*! \fn template <class T> bool QLinkedList<T>::endsWith(const T &value) const
    \since 4.5

    Returns \c true if the list is not empty and its last
    item is equal to \a value; otherwise returns \c false.

    \sa isEmpty(), last()
*/

/*! \fn template <class T> QLinkedList<T>::iterator QLinkedList<T>::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first item in
    the list.

    \sa constBegin(), end()
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator QLinkedList<T>::begin() const

    \overload
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator QLinkedList<T>::cbegin() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the list.

    \sa begin(), cend()
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator QLinkedList<T>::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the list.

    \sa begin(), constEnd()
*/

/*! \fn template <class T> QLinkedList<T>::iterator QLinkedList<T>::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary item
    after the last item in the list.

    \sa begin(), constEnd()
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator QLinkedList<T>::end() const

    \overload
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator QLinkedList<T>::cend() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the list.

    \sa cbegin(), end()
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator QLinkedList<T>::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the list.

    \sa constBegin(), end()
*/

/*! \fn template <class T> QLinkedList<T>::reverse_iterator QLinkedList<T>::rbegin()
    \since 5.6

    Returns a \l{STL-style iterators}{STL-style} reverse iterator pointing to the first
    item in the list, in reverse order.

    \sa begin(), crbegin(), rend()
*/

/*! \fn template <class T> QLinkedList<T>::const_reverse_iterator QLinkedList<T>::rbegin() const
    \since 5.6
    \overload
*/

/*! \fn template <class T> QLinkedList<T>::const_reverse_iterator QLinkedList<T>::crbegin() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator pointing to the first
    item in the list, in reverse order.

    \sa begin(), rbegin(), rend()
*/

/*! \fn template <class T> QLinkedList<T>::reverse_iterator QLinkedList<T>::rend()
    \since 5.6

    Returns a \l{STL-style iterators}{STL-style} reverse iterator pointing to one past
    the last item in the list, in reverse order.

    \sa end(), crend(), rbegin()
*/

/*! \fn template <class T> QLinkedList<T>::const_reverse_iterator QLinkedList<T>::rend() const
    \since 5.6
    \overload
*/

/*! \fn template <class T> QLinkedList<T>::const_reverse_iterator QLinkedList<T>::crend() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator pointing to one
    past the last item in the list, in reverse order.

    \sa end(), rend(), rbegin()
*/

/*! \fn template <class T> QLinkedList<T>::iterator QLinkedList<T>::insert(iterator before, const T &value)

    Inserts \a value in front of the item pointed to by the iterator
    \a before. Returns an iterator pointing at the inserted item.

    \sa erase()
*/

/*! \fn template <class T> QLinkedList<T>::iterator QLinkedList<T>::erase(iterator pos)

    Removes the item pointed to by the iterator \a pos from the list,
    and returns an iterator to the next item in the list (which may be
    end()).

    \sa insert()
*/

/*! \fn template <class T> QLinkedList<T>::iterator QLinkedList<T>::erase(iterator begin, iterator end)

    \overload

    Removes all the items from \a begin up to (but not including) \a
    end.
*/

/*! \typedef QLinkedList::Iterator

    Qt-style synonym for QLinkedList::iterator.
*/

/*! \typedef QLinkedList::ConstIterator

    Qt-style synonym for QLinkedList::const_iterator.
*/

/*! \typedef QLinkedList::reverse_iterator
    \since 5.6

    The QLinkedList::reverse_iterator typedef provides an STL-style non-const
    reverse iterator for QLinkedList.

    It is simply a typedef for \c{std::reverse_iterator<QLinkedList::iterator>}.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QLinkedList::rbegin(), QLinkedList::rend(), QLinkedList::const_reverse_iterator, QLinkedList::iterator
*/

/*! \typedef QLinkedList::const_reverse_iterator
    \since 5.6

    The QLinkedList::const_reverse_iterator typedef provides an STL-style const
    reverse iterator for QLinkedList.

    It is simply a typedef for \c{std::reverse_iterator<QLinkedList::const_iterator>}.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QLinkedList::rbegin(), QLinkedList::rend(), QLinkedList::reverse_iterator, QLinkedList::const_iterator
*/

/*!
    \typedef QLinkedList::size_type

    Typedef for int. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::value_type

    Typedef for T. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::pointer

    Typedef for T *. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::const_pointer

    Typedef for const T *. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::reference

    Typedef for T &. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::const_reference

    Typedef for const T &. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::difference_type

    Typedef for ptrdiff_t. Provided for STL compatibility.
*/

/*! \fn template <class T> int QLinkedList<T>::count() const

    Same as size().
*/

/*! \fn template <class T> T& QLinkedList<T>::first()

    Returns a reference to the first item in the list. This function
    assumes that the list isn't empty.

    \sa last(), isEmpty()
*/

/*! \fn template <class T> const T& QLinkedList<T>::first() const

    \overload
*/

/*! \fn template <class T> T& QLinkedList<T>::last()

    Returns a reference to the last item in the list. This function
    assumes that the list isn't empty.

    \sa first(), isEmpty()
*/

/*! \fn template <class T> const T& QLinkedList<T>::last() const

    \overload
*/

/*! \fn template <class T> void QLinkedList<T>::removeFirst()

    Removes the first item in the list.

    This is the same as erase(begin()).

    \sa removeLast(), erase()
*/

/*! \fn template <class T> void QLinkedList<T>::removeLast()

    Removes the last item in the list.

    \sa removeFirst(), erase()
*/

/*! \fn template <class T> T QLinkedList<T>::takeFirst()

    Removes the first item in the list and returns it.

    If you don't use the return value, removeFirst() is more
    efficient.

    \sa takeLast(), removeFirst()
*/

/*! \fn template <class T> T QLinkedList<T>::takeLast()

    Removes the last item in the list and returns it.

    If you don't use the return value, removeLast() is more
    efficient.

    \sa takeFirst(), removeLast()
*/

/*! \fn template <class T> void QLinkedList<T>::push_back(const T &value)

    This function is provided for STL compatibility. It is equivalent
    to append(\a value).
*/

/*! \fn template <class T> void QLinkedList<T>::push_front(const T &value)

    This function is provided for STL compatibility. It is equivalent
    to prepend(\a value).
*/

/*! \fn template <class T> T& QLinkedList<T>::front()

    This function is provided for STL compatibility. It is equivalent
    to first().
*/

/*! \fn template <class T> const T& QLinkedList<T>::front() const

    \overload
*/

/*! \fn template <class T> T& QLinkedList<T>::back()

    This function is provided for STL compatibility. It is equivalent
    to last().
*/

/*! \fn template <class T> const T& QLinkedList<T>::back() const

    \overload
*/

/*! \fn template <class T> void QLinkedList<T>::pop_front()

    This function is provided for STL compatibility. It is equivalent
    to removeFirst().
*/

/*! \fn template <class T> void QLinkedList<T>::pop_back()

    This function is provided for STL compatibility. It is equivalent
    to removeLast().
*/

/*! \fn template <class T> bool QLinkedList<T>::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty() and returns \c true if the list is empty.
*/

/*! \fn template <class T> QLinkedList<T> &QLinkedList<T>::operator+=(const QLinkedList<T> &other)

    Appends the items of the \a other list to this list and returns a
    reference to this list.

    \sa operator+(), append()
*/

/*! \fn template <class T> void QLinkedList<T>::operator+=(const T &value)

    \overload

    Appends \a value to the list.
*/

/*! \fn template <class T> QLinkedList<T> QLinkedList<T>::operator+(const QLinkedList<T> &other) const

    Returns a list that contains all the items in this list followed
    by all the items in the \a other list.

    \sa operator+=()
*/

/*! \fn template <class T> QLinkedList<T> &QLinkedList<T>::operator<<(const QLinkedList<T> &other)

    Appends the items of the \a other list to this list and returns a
    reference to this list.

    \sa operator+=(), append()
*/

/*! \fn template <class T> QLinkedList<T> &QLinkedList<T>::operator<<(const T &value)

    \overload

    Appends \a value to the list.
*/

/*! \class QLinkedList::iterator
    \inmodule QtCore
    \brief The QLinkedList::iterator class provides an STL-style non-const iterator for QLinkedList.

    QLinkedList features both \l{STL-style iterators} and
    \l{Java-style iterators}. The STL-style iterators are more
    low-level and more cumbersome to use; on the other hand, they are
    slightly faster and, for developers who already know STL, have
    the advantage of familiarity.

    QLinkedList\<T\>::iterator allows you to iterate over a
    QLinkedList\<T\> and to modify the list item associated with the
    iterator. If you want to iterate over a const QLinkedList, use
    QLinkedList::const_iterator instead. It is generally good
    practice to use QLinkedList::const_iterator on a non-const
    QLinkedList as well, unless you need to change the QLinkedList
    through the iterator. Const iterators are slightly faster, and
    can improve code readability.

    The default QLinkedList::iterator constructor creates an
    uninitialized iterator. You must initialize it using a
    function like QLinkedList::begin(), QLinkedList::end(), or
    QLinkedList::insert() before you can start iterating. Here's a
    typical loop that prints all the items stored in a list:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 7

    STL-style iterators can be used as arguments to \l{generic
    algorithms}. For example, here's how to find an item in the list
    using the qFind() algorithm:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 8

    Let's see a few examples of things we can do with a
    QLinkedList::iterator that we cannot do with a QLinkedList::const_iterator.
    Here's an example that increments every value stored in a
    QLinkedList\<int\> by 2:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 9

    Here's an example that removes all the items that start with an
    underscore character in a QLinkedList\<QString\>:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 10

    The call to QLinkedList::erase() removes the item pointed to by
    the iterator from the list, and returns an iterator to the next
    item. Here's another way of removing an item while iterating:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 11

    It might be tempting to write code like this:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 12

    However, this will potentially crash in \c{++i}, because \c i is
    a dangling iterator after the call to erase().

    Multiple iterators can be used on the same list. If you add items
    to the list, existing iterators will remain valid. If you remove
    items from the list, iterators that point to the removed items
    will become dangling iterators.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QLinkedList::const_iterator, QMutableLinkedListIterator
*/

/*! \fn template <class T> QLinkedList<T>::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa QLinkedList::begin(), QLinkedList::end()
*/

/*! \fn template <class T> QLinkedList<T>::iterator::iterator(Node *node)

    \internal
*/

/*! \typedef QLinkedList::iterator::iterator_category

    \internal
*/

/*! \typedef QLinkedList::iterator::difference_type

    \internal
*/

/*! \typedef QLinkedList::iterator::value_type

    \internal
*/

/*! \typedef QLinkedList::iterator::pointer

    \internal
*/

/*! \typedef QLinkedList::iterator::reference

    \internal
*/

/*! \fn template <class T> QLinkedList<T>::iterator::iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn template <class T> QLinkedList<T>::iterator &QLinkedList<T>::iterator::operator=(const iterator &other)

    Assigns \a other to this iterator.
*/

/*!
    \fn template <class T> QLinkedList<T> &QLinkedList<T>::operator=(QLinkedList<T> &&other)

    Move-assigns \a other to this QLinkedList instance.

    \since 5.2
*/

/*! \fn template <class T> T &QLinkedList<T>::iterator::operator*() const

    Returns a modifiable reference to the current item.

    You can change the value of an item by using operator*() on the
    left side of an assignment, for example:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 13

    \sa operator->()
*/

/*! \fn template <class T> T *QLinkedList<T>::iterator::operator->() const

    Returns a pointer to the current item.

    \sa operator*()
*/

/*!
    \fn template <class T> bool QLinkedList<T>::iterator::operator==(const iterator &other) const
    \fn template <class T> bool QLinkedList<T>::iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn template <class T> bool QLinkedList<T>::iterator::operator!=(const iterator &other) const
    \fn template <class T> bool QLinkedList<T>::iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*! \fn template <class T> QLinkedList<T>::iterator &QLinkedList<T>::iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the list and returns an iterator to the new current
    item.

    Calling this function on QLinkedList::end() leads to undefined
    results.

    \sa operator--()
*/

/*! \fn template <class T> QLinkedList<T>::iterator QLinkedList<T>::iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the list and returns an iterator to the previously
    current item.
*/

/*! \fn template <class T> QLinkedList<T>::iterator &QLinkedList<T>::iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QLinkedList::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn template <class T> QLinkedList<T>::iterator QLinkedList<T>::iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn template <class T> QLinkedList<T>::iterator QLinkedList<T>::iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-()

*/

/*! \fn template <class T> QLinkedList<T>::iterator QLinkedList<T>::iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn template <class T> QLinkedList<T>::iterator &QLinkedList<T>::iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    \sa operator-=(), operator+()
*/

/*! \fn template <class T> QLinkedList<T>::iterator &QLinkedList<T>::iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    \sa operator+=(), operator-()
*/

/*! \class QLinkedList::const_iterator
    \inmodule QtCore
    \brief The QLinkedList::const_iterator class provides an STL-style const iterator for QLinkedList.

    QLinkedList features both \l{STL-style iterators} and
    \l{Java-style iterators}. The STL-style iterators are more
    low-level and more cumbersome to use; on the other hand, they are
    slightly faster and, for developers who already know STL, have
    the advantage of familiarity.

    QLinkedList\<T\>::const_iterator allows you to iterate over a
    QLinkedList\<T\>. If you want modify the QLinkedList as you iterate
    over it, you must use QLinkedList::iterator instead. It is
    generally good practice to use QLinkedList::const_iterator on a
    non-const QLinkedList as well, unless you need to change the
    QLinkedList through the iterator. Const iterators are slightly
    faster, and can improve code readability.

    The default QLinkedList::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a function
    like QLinkedList::constBegin(), QLinkedList::constEnd(), or
    QLinkedList::insert() before you can start iterating. Here's a
    typical loop that prints all the items stored in a list:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 14

    STL-style iterators can be used as arguments to \l{generic
    algorithms}. For example, here's how to find an item in the list
    using the qFind() algorithm:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 15

    Multiple iterators can be used on the same list. If you add items
    to the list, existing iterators will remain valid. If you remove
    items from the list, iterators that point to the removed items
    will become dangling iterators.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QLinkedList::iterator, QLinkedListIterator
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa QLinkedList::constBegin(), QLinkedList::constEnd()
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator::const_iterator(Node *node)

    \internal
*/

/*! \typedef QLinkedList::const_iterator::iterator_category

    \internal
*/

/*! \typedef QLinkedList::const_iterator::difference_type

    \internal
*/

/*! \typedef QLinkedList::const_iterator::value_type

    \internal
*/

/*! \typedef QLinkedList::const_iterator::pointer

    \internal
*/

/*! \typedef QLinkedList::const_iterator::reference

    \internal
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator::const_iterator(const const_iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator::const_iterator(iterator other)

    Constructs a copy of \a other.
*/

/*! \fn template <class T> typename QLinkedList<T>::const_iterator &QLinkedList<T>::const_iterator::operator=(const const_iterator &other)

    Assigns \a other to this iterator.
*/

/*! \fn template <class T> const T &QLinkedList<T>::const_iterator::operator*() const

    Returns a reference to the current item.

    \sa operator->()
*/

/*! \fn template <class T> const T *QLinkedList<T>::const_iterator::operator->() const

    Returns a pointer to the current item.

    \sa operator*()
*/

/*! \fn template <class T> bool QLinkedList<T>::const_iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn template <class T> bool QLinkedList<T>::const_iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator &QLinkedList<T>::const_iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the list and returns an iterator to the new current
    item.

    Calling this function on QLinkedList<T>::constEnd() leads to
    undefined results.

    \sa operator--()
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator QLinkedList<T>::const_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the list and returns an iterator to the previously
    current item.
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator &QLinkedList<T>::const_iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QLinkedList::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator QLinkedList<T>::const_iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator QLinkedList<T>::const_iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-()
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator QLinkedList<T>::const_iterator::operator-(int j) const

    This function returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator &QLinkedList<T>::const_iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-=(), operator+()
*/

/*! \fn template <class T> QLinkedList<T>::const_iterator &QLinkedList<T>::const_iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+=(), operator-()
*/

/*! \fn template <class T> QDataStream &operator<<(QDataStream &out, const QLinkedList<T> &list)
    \relates QLinkedList

    Writes the linked list \a list to stream \a out.

    This function requires the value type to implement \c
    operator<<().

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/

/*! \fn template <class T> QDataStream &operator>>(QDataStream &in, QLinkedList<T> &list)
    \relates QLinkedList

    Reads a linked list from stream \a in into \a list.

    This function requires the value type to implement \c operator>>().

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/

/*!
    \since 4.1
    \fn template <class T> QLinkedList<T> QLinkedList<T>::fromStdList(const std::list<T> &list)

    Returns a QLinkedList object with the data contained in \a list.
    The order of the elements in the QLinkedList is the same as in \a
    list.

    Example:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 16

    \sa toStdList()
*/

/*!
    \since 4.1
    \fn template <class T> std::list<T> QLinkedList<T>::toStdList() const

    Returns a std::list object with the data contained in this
    QLinkedList. Example:

    \snippet code/src_corelib_tools_qlinkedlist.cpp 17

    \sa fromStdList()
*/

QT_END_NAMESPACE
