/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: http://www.qt.io/licensing/
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

#include "qstringview.h"

QT_BEGIN_NAMESPACE

/*!
    \class QStringView
    \inmodule QtCore
    \since 5.10
    \brief The QStringView class provides a unified view on UTF-16 strings
    \reentrant
    \ingroup tools
    \ingroup string-processing

    QStringView provides a read-only subset of the QString API.

    A string view explicitly stores a portion of a UTF-16 string it does
    not own. It acts as an interface type to all kinds of UTF-16 string,
    without the need to construct a QString first.

    The UTF-16 string may be represented as an array (or an array-compatible
    data-structure such as QString,
    std::basic_string, etc.) of \c QChar, \c ushort, \c char16_t (on compilers that
    support C++11 Unicode strings) or (on platforms, such as Windows,
    where it is a 16-bit type) \c wchar_t.

    QStringView is designed as an interface type; its main use-case is
    as a function parameter type. When QStringViews are used as automatic
    variables or data members, care must be taken to ensure that the referenced
    string data (for example, owned by a QString) outlives the QStringView on all code paths,
    lest the string view ends up referencing deleted data.

    When used as an interface type, QStringView allows a single function to accept
    a wide variety of UTF-16 string data sources. One function accepting QStringView
    thus replaces three function overloads (taking QString, QStringRef, and
    \c{(const QChar*, int)}), while at the same time enabling even more string data
    sources to be passed to the function, such as \c{u"Hello World"}, a \c char16_t
    string literal.

    QStringViews should be passed by value, not by reference-to-const:
    \code
    void myfun1(QStringView sv);        // preferred
    void myfun2(const QStringView &sv); // compiles and works, but slower
    \endcode

    If you want to give your users maximum freedom in what strings they can pass
    to your function, accompany the QStringView overload with overloads for

    \list
        \li \e QChar: this overload can delegate to the QStringView version:
            \code
            void fun(QChar ch) { fun(QStringView(&ch, 1)); }
            \endcode
            even though, for technical reasons, QStringView cannot provide a
            QChar constructor by itself.
        \li \e QString: if you store an unmodified copy of the string and thus would
            like to take advantage of QString's implicit sharing.
        \li QLatin1String: if you can implement the function without converting the
            QLatin1String to UTF-16 first; users expect a function overloaded on
            QLatin1String to perform strictly less memory allocations than the
            semantically equivalent call of the QStringView version, involving
            construction of a QString from the QLatin1String.
    \endlist

    QStringView can also be used as the return value of a function. If you call a
    function returning QStringView, take extra care to not keep the QStringView
    around longer than the function promises to keep the referenced string data alive.
    If in doubt, obtain a strong reference to the data by calling toString() to convert
    the QStringView into a QString.

    QStringView is a \e{Literal Type}, but since it stores data as \c{char16_t}, iteration
    is not \c constexpr (casts from \c{const char16_t*} to \c{const QChar*}, which is not
    allowed in \c constexpr functions). You can use an indexed loop and/or utf16() in
    \c constexpr contexts instead.

    \note We strongly discourage the use of QList<QStringView>,
    because QList is a very inefficient container for QStringViews (it would heap-allocate
    every element). Use QVector (or std::vector) to hold QStringViews instead.

    \sa QString, QStringRef
*/

/*!
    \typedef QStringView::value_type

    Alias for \c{const QChar}. Provided for compatibility with the STL.
*/

/*!
    \typedef QStringView::difference_type

    Alias for \c{std::ptrdiff_t}. Provided for compatibility with the STL.
*/

/*!
    \typedef QStringView::size_type

    Alias for \c{std::ptrdiff_t}. Provided for compatibility with the STL.

    Unlike other Qt classes, QStringView uses \c ptrdiff_t as its \c size_type, to allow
    accepting data from \c{std::basic_string} without truncation. The Qt API functions,
    for example length(), return \c int, while the STL-compatible functions, for example
    size(), return \c size_type.
*/

/*!
    \typedef QStringView::reference

    Alias for \c{value_type &}. Provided for compatibility with the STL.

    QStringView does not support mutable references, so this is the same
    as const_reference.
*/

/*!
    \typedef QStringView::const_reference

    Alias for \c{value_type &}. Provided for compatibility with the STL.
*/

/*!
    \typedef QStringView::pointer

    Alias for \c{value_type *}. Provided for compatibility with the STL.

    QStringView does not support mutable pointers, so this is the same
    as const_pointer.
*/

/*!
    \typedef QStringView::const_pointer

    Alias for \c{value_type *}. Provided for compatibility with the STL.
*/

/*!
    \typedef QStringView::iterator

    This typedef provides an STL-style const iterator for QStringView.

    QStringView does not support mutable iterators, so this is the same
    as const_iterator.

    \sa const_iterator, reverse_iterator
*/

/*!
    \typedef QStringView::const_iterator

    This typedef provides an STL-style const iterator for QStringView.

    \sa iterator, const_reverse_iterator
*/

/*!
    \typedef QStringView::reverse_iterator

    This typedef provides an STL-style const reverse iterator for QStringView.

    QStringView does not support mutable reverse iterators, so this is the
    same as const_reverse_iterator.

    \sa const_reverse_iterator, iterator
*/

/*!
    \typedef QStringView::const_reverse_iterator

    This typedef provides an STL-style const reverse iterator for QStringView.

    \sa reverse_iterator, const_iterator
*/

/*!
    \fn QStringView::QStringView()

    Constructs a null string view.

    \sa isNull()
*/

/*!
    \fn QStringView::QStringView(std::nullptr_t)

    Constructs a null string view.

    \sa isNull()
*/

/*!
    \fn QStringView::QStringView(QString::Null)
    \internal

    Constructs a null string view.

    \sa isNull()
*/

/*!
    \fn QStringView::QStringView(const Char *str, size_type len)

    Constructs a string view on \a str with length \a len.

    The range \c{[str,len)} must remain valid for the lifetime of this string view object.

    Passing \c nullptr as \a str is safe if \a len is 0, too, and results in a null string view.

    The behavior is undefined if \a len is negative or, when positive, if \a str is \c nullptr.

    This constructor only participates in overload resolution if \c Char is a compatible
    character type. The compatible character types are: \c QChar, \c ushort, \c char16_t and
    (on platforms, such as Windows, where it is a 16-bit type) \c wchar_t.
*/

/*!
    \fn QStringView::QStringView(const Char *str)

    Constructs a string view on \a str. The length is determined
    by scanning for the first \c{char16_t(0)}.

    \a str must remain valid for the lifetime of this string view object.

    Passing \c nullptr as \a str is safe and results in a null string view.

    This constructor only participates in overload resolution if \c Char is a compatible
    character type. The compatible character types are: \c QChar, \c ushort, \c char16_t and
    (on platforms, such as Windows, where it is a 16-bit type) \c wchar_t.
*/

/*!
    \fn QStringView::QStringView(const QString &str)

    Constructs a string view on \a str.

    \c{str.data()} must remain valid for the lifetime of this string view object.

    The string view will be null if and only if \c{str.isNull()}.
*/

/*!
    \fn QStringView::QStringView(const QStringRef &str)

    Constructs a string view on \a str.

    \c{str.data()} must remain valid for the lifetime of this string view object.

    The string view will be null if and only if \c{str.isNull()}.
*/

/*!
    \fn QStringView::QStringView(const StdBasicString &str)

    Constructs a string view on \a str. The length is taken from \c{str.size()}.

    \c{str.data()} must remain valid for the lifetime of this string view object.

    This constructor only participates in overload resolution if \c StdBasicString is an
    instantiation of \c std::basic_string with a compatible character type. The
    compatible character types are: \c QChar, \c ushort, \c char16_t and
    (on platforms, such as Windows, where it is a 16-bit type) \c wchar_t.

    The string view will be empty if and only if \c{str.empty()}. It is unspecified
    whether this constructor can result in a null string view (\c{str.data()} would
    have to return \c nullptr for this).

    \sa isNull(), isEmpty()
*/

/*!
    \fn QString QStringView::toString() const

    Returns a deep copy of this string view's data as a QString.

    The return value will be the null QString if and only if this string view is null.

    \warning QStringView can store strings with more than 2\sup{30} characters
    while QString cannot. Calling this function on a string view for which size()
    returns a value greater than \c{INT_MAX / 2} constitutes undefined behavior.
*/

/*!
    \fn const QChar *QStringView::data() const

    Returns a const pointer to the first character in the string.

    \note The character array represented by the return value is \e not null-terminated.

    \sa begin(), end(), utf16()
*/

/*!
    \fn const storage_type *QStringView::utf16() const

    Returns a const pointer to the first character in the string.

    \c{storage_type} is \c{char16_t}, except on MSVC 2013 (which lacks \c char16_t support),
    where it is \c{wchar_t} instead.

    \note The character array represented by the return value is \e not null-terminated.

    \sa begin(), end(), data()
*/

/*!
    \fn QStringView::const_iterator QStringView::begin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first character in
    the string.

    This function is provided for STL compatibility.

    \sa end(), cbegin(), rbegin(), data()
*/

/*!
    \fn QStringView::const_iterator QStringView::cbegin() const

    Same as begin().

    This function is provided for STL compatibility.

    \sa cend(), begin(), crbegin(), data()
*/

/*!
    \fn QStringView::const_iterator QStringView::end() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    character after the last character in the list.

    This function is provided for STL compatibility.

    \sa begin(), cend(), rend()
*/

/*! \fn QStringView::const_iterator QStringView::cend() const

    Same as end().

    This function is provided for STL compatibility.

    \sa cbegin(), end(), crend()
*/

/*!
    \fn QStringView::const_reverse_iterator QStringView::rbegin() const

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator pointing to the first
    character in the string, in reverse order.

    This function is provided for STL compatibility.

    \sa rend(), crbegin(), begin()
*/

/*!
    \fn QStringView::const_reverse_iterator QStringView::crbegin() const

    Same as rbegin().

    This function is provided for STL compatibility.

    \sa crend(), rbegin(), cbegin()
*/

/*!
    \fn QStringView::const_reverse_iterator QStringView::rend() const

    Returns a \l{STL-style iterators}{STL-style} reverse iterator pointing to one past
    the last character in the string, in reverse order.

    This function is provided for STL compatibility.

    \sa rbegin(), crend(), end()
*/

/*!
    \fn QStringView::const_reverse_iterator QStringView::crend() const

    Same as rend().

    This function is provided for STL compatibility.

    \sa crbegin(), rend(), cend()
*/

/*!
    \fn bool QStringView::empty() const

    Returns whether this string view is empty - that is, whether \c{size() == 0}.

    This function is provided for STL compatibility.

    \sa isEmpty(), isNull(), size(), length()
*/

/*!
    \fn bool QStringView::isEmpty() const

    Returns whether this string view is empty - that is, whether \c{size() == 0}.

    This function is provided for compatibility with other Qt containers.

    \sa empty(), isNull(), size(), length()
*/

/*!
    \fn bool QStringView::isNull() const

    Returns whether this string view is null - that is, whether \c{data() == nullptr}.

    This functions is provided for compatibility with other Qt containers.

    \sa empty(), isEmpty(), size(), length()
*/

/*!
    \fn QStringView::size_type QStringView::size() const

    Returns the size of this string view, in UTF-16 code points (that is,
    surrogate pairs count as two for the purposes of this function, the same
    as in QString and QStringRef).

    \sa empty(), isEmpty(), isNull(), length()
*/

/*!
    \fn int QStringView::length() const

    Same as size(), except returns the result as an \c int.

    This function is provided for compatibility with other Qt containers.

    \warning QStringView can represent strings with more than 2\sup{31} characters.
    Calling this function on a string view for which size() returns a value greater
    than \c{INT_MAX} constitutes undefined behavior.

    \sa empty(), isEmpty(), isNull(), size()
*/

/*!
    \fn QChar QStringView::operator[](size_type n) const

    Returns the character at position \a n in this string view.

    The behavior is undefined if \a n is negative or not less than size().

    \sa at(), front(), back()
*/

/*!
    \fn QChar QStringView::at(size_type n) const

    Returns the character at position \a n in this string view.

    The behavior is undefined if \a n is negative or not less than size().

    \sa operator[](), front(), back()
*/

/*!
    \fn QChar QStringView::front() const

    Returns the first character in the string. Same as first().

    This function is provided for STL compatibility.

    \warning Calling this function on an empty string view constitutes
    undefined behavior.

    \sa back(), first(), last()
*/

/*!
    \fn QChar QStringView::back() const

    Returns the last character in the string. Same as last().

    This function is provided for STL compatibility.

    \warning Calling this function on an empty string view constitutes
    undefined behavior.

    \sa front(), first(), last()
*/

/*!
    \fn QChar QStringView::first() const

    Returns the first character in the string. Same as front().

    This function is provided for compatibility with other Qt containers.

    \warning Calling this function on an empty string view constitutes
    undefined behavior.

    \sa front(), back(), last()
*/

/*!
    \fn QChar QStringView::last() const

    Returns the last character in the string. Same as back().

    This function is provided for compatibility with other Qt containers.

    \warning Calling this function on an empty string view constitutes
    undefined behavior.

    \sa back(), front(), first()
*/

/*!
    \fn QStringView QStringView::mid(size_type start) const

    Returns the substring starting at position \a start in this object,
    and extending to the end of the string.

    \note The behavior is undefined when \a start < 0 or \a start > size().

    \sa left(), right()
*/

/*!
    \fn QStringView QStringView::mid(size_type start, size_type length) const
    \overload

    Returns the substring of length \a length starting at position
    \a start in this object.

    \note The behavior is undefined when \a start < 0, \a length < 0,
    or \a start + \a length > size().

    \sa left(), right()
*/

/*!
    \fn QStringView QStringView::left(size_type length) const

    Returns the substring of length \a length starting at position
    0 in this object.

    \note The behavior is undefined when \a length < 0 or \a length > size().

    \sa mid(), right()
*/

/*!
    \fn QStringView QStringView::right(size_type length) const

    Returns the substring of length \a length starting at position
    size() - \a length in this object.

    \note The behavior is undefined when \a length < 0 or \a length > size().

    \sa mid(), left()
*/

QT_END_NAMESPACE
