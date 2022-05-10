// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*!
    \class QStack
    \inmodule QtCore
    \brief The QStack class is a template class that provides a stack.

    \ingroup tools
    \ingroup shared

    \reentrant

    QStack\<T\> is one of Qt's generic \l{container classes}. It implements
    a stack data structure for items of a same type.

    A stack is a last in, first out (LIFO) structure. Items are added
    to the top of the stack using push() and retrieved from the top
    using pop(). The top() function provides access to the topmost
    item without removing it.

    Example:

    \snippet qstack/main.cpp 0

    The example will output 3, 2, 1 in that order.

    QStack inherits from QList. All of QList's functionality also
    applies to QStack. For example, you can use isEmpty() to test
    whether the stack is empty, and you can traverse a QStack using
    QList's iterator classes (for example, QListIterator). But in
    addition, QStack provides three convenience functions that make
    it easy to implement LIFO semantics: push(), pop(), and top().

    QStack's value type must be an \l{assignable data type}. This
    covers most data types that are commonly used, but the compiler
    won't let you, for example, store a QWidget as a value; instead,
    store a QWidget *.

    \sa QList, QQueue
*/

/*!
    \fn template<class T> void QStack<T>::swap(QStack<T> &other)
    \since 4.8

    Swaps stack \a other with this stack. This operation is very fast and
    never fails.
*/

/*!
    \fn template<class T> void QStack<T>::push(const T& t)

    Adds element \a t to the top of the stack.

    This is the same as QList::append().

    \sa pop(), top()
*/

/*!
    \fn template<class T> T& QStack<T>::top()

    Returns a reference to the stack's top item. This function
    assumes that the stack isn't empty.

    This is the same as QList::last().

    \sa pop(), push(), isEmpty()
*/

/*!
    \fn template<class T> const T& QStack<T>::top() const

    \overload

    \sa pop(), push()
*/

/*!
    \fn template<class T> T QStack<T>::pop()

    Removes the top item from the stack and returns it. This function
    assumes that the stack isn't empty.

    \sa top(), push(), isEmpty()
*/
