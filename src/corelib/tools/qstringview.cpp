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
    \brief The QStringView class provides a unified view on UTF-16 strings with a read-only subset of the QString API.
    \reentrant
    \ingroup tools
    \ingroup string-processing

    A QStringView references a contiguous portion of a UTF-16 string it does
    not own. It acts as an interface type to all kinds of UTF-16 string,
    without the need to construct a QString first.

    The UTF-16 string may be represented as an array (or an array-compatible
    data-structure such as QString,
    std::basic_string, etc.) of QChar, \c ushort, \c char16_t (on compilers that
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
    \typedef QStringView::storage_type

    Alias for \c{char16_t} for non-Windows or if Q_COMPILER_UNICODE_STRINGS
    is defined. Otherwise, alias for \c{wchar_t}.
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

    Alias for qsizetype. Provided for compatibility with the STL.

    Unlike other Qt classes, QStringView uses qsizetype as its \c size_type, to allow
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
    \fn  template <typename Char> QStringView::QStringView(const Char *str, qsizetype len)

    Constructs a string view on \a str with length \a len.

    The range \c{[str,len)} must remain valid for the lifetime of this string view object.

    Passing \c nullptr as \a str is safe if \a len is 0, too, and results in a null string view.

    The behavior is undefined if \a len is negative or, when positive, if \a str is \c nullptr.

    This constructor only participates in overload resolution if \c Char is a compatible
    character type. The compatible character types are: \c QChar, \c ushort, \c char16_t and
    (on platforms, such as Windows, where it is a 16-bit type) \c wchar_t.
*/

/*!
    \fn template <typename Char> QStringView::QStringView(const Char *first, const Char *last)

    Constructs a string view on \a first with length (\a last - \a first).

    The range \c{[first,last)} must remain valid for the lifetime of
    this string view object.

    Passing \c nullptr as \a first is safe if \a last is nullptr, too,
    and results in a null string view.

    The behavior is undefined if \a last precedes \a first, or \a first
    is \c nullptr and \a last is not.

    This constructor only participates in overload resolution if \c Char
    is a compatible character type. The compatible character types
    are: \c QChar, \c ushort, \c char16_t and (on platforms, such as
    Windows, where it is a 16-bit type) \c wchar_t.
*/

/*!
    \fn template <typename Char> QStringView::QStringView(const Char *str)

    Constructs a string view on \a str. The length is determined
    by scanning for the first \c{Char(0)}.

    \a str must remain valid for the lifetime of this string view object.

    Passing \c nullptr as \a str is safe and results in a null string view.

    This constructor only participates in overload resolution if \a
    str is not an array and if \c Char is a compatible character
    type. The compatible character types are: \c QChar, \c ushort, \c
    char16_t and (on platforms, such as Windows, where it is a 16-bit
    type) \c wchar_t.
*/

/*!
    \fn template <typename Char, size_t N> QStringView::QStringView(const Char (&string)[N])

    Constructs a string view on the character string literal \a string.
    The length is set to \c{N-1}, excluding the trailing \{Char(0)}.
    If you need the full array, use the constructor from pointer and
    size instead:

    \code
    auto sv = QStringView(array, std::size(array)); // using C++17 std::size()
    \endcode

    \a string must remain valid for the lifetime of this string view
    object.

    This constructor only participates in overload resolution if \a
    string is an actual array and \c Char is a compatible character
    type. The compatible character types are: \c QChar, \c ushort, \c
    char16_t and (on platforms, such as Windows, where it is a 16-bit
    type) \c wchar_t.
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
    \fn template <typename StdBasicString> QStringView::QStringView(const StdBasicString &str)

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

    \c{storage_type} is \c{char16_t}.

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
    \fn qsizetype QStringView::size() const

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
    \fn QChar QStringView::operator[](qsizetype n) const

    Returns the character at position \a n in this string view.

    The behavior is undefined if \a n is negative or not less than size().

    \sa at(), front(), back()
*/

/*!
    \fn QChar QStringView::at(qsizetype n) const

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
    \fn QStringView QStringView::mid(qsizetype start) const

    Returns the substring starting at position \a start in this object,
    and extending to the end of the string.

    \note The behavior is undefined when \a start < 0 or \a start > size().

    \sa left(), right(), chopped(), chop(), truncate()
*/

/*!
    \fn QStringView QStringView::mid(qsizetype start, qsizetype length) const
    \overload

    Returns the substring of length \a length starting at position
    \a start in this object.

    \note The behavior is undefined when \a start < 0, \a length < 0,
    or \a start + \a length > size().

    \sa left(), right(), chopped(), chop(), truncate()
*/

/*!
    \fn QStringView QStringView::left(qsizetype length) const

    Returns the substring of length \a length starting at position
    0 in this object.

    \note The behavior is undefined when \a length < 0 or \a length > size().

    \sa mid(), right(), chopped(), chop(), truncate()
*/

/*!
    \fn QStringView QStringView::right(qsizetype length) const

    Returns the substring of length \a length starting at position
    size() - \a length in this object.

    \note The behavior is undefined when \a length < 0 or \a length > size().

    \sa mid(), left(), chopped(), chop(), truncate()
*/

/*!
    \fn QStringView QStringView::chopped(qsizetype length) const

    Returns the substring of length size() - \a length starting at the
    beginning of this object.

    Same as \c{left(size() - length)}.

    \note The behavior is undefined when \a length < 0 or \a length > size().

    \sa mid(), left(), right(), chop(), truncate()
*/

/*!
    \fn void QStringView::truncate(qsizetype length)

    Truncates this string view to length \a length.

    Same as \c{*this = left(length)}.

    \note The behavior is undefined when \a length < 0 or \a length > size().

    \sa mid(), left(), right(), chopped(), chop()
*/

/*!
    \fn void QStringView::chop(qsizetype length)

    Truncates this string view by \a length characters.

    Same as \c{*this = left(size() - length)}.

    \note The behavior is undefined when \a length < 0 or \a length > size().

    \sa mid(), left(), right(), chopped(), truncate()
*/

/*!
    \fn QStringView QStringView::trimmed() const

    Strips leading and trailing whitespace and returns the result.

    Whitespace means any character for which QChar::isSpace() returns
    \c true. This includes the ASCII characters '\\t', '\\n', '\\v',
    '\\f', '\\r', and ' '.
*/

/*!
    \fn bool QStringView::startsWith(QStringView str, Qt::CaseSensitivity cs) const
    \fn bool QStringView::startsWith(QLatin1String l1, Qt::CaseSensitivity cs) const
    \fn bool QStringView::startsWith(QChar ch) const
    \fn bool QStringView::startsWith(QChar ch, Qt::CaseSensitivity cs) const

    Returns \c true if this string-view starts with string-view \a str,
    Latin-1 string \a l1, or character \a ch, respectively;
    otherwise returns \c false.

    If \a cs is Qt::CaseSensitive (the default), the search is case-sensitive;
    otherwise the search is case-insensitive.

    \sa endsWith()
*/

/*!
    \fn bool QStringView::endsWith(QStringView str, Qt::CaseSensitivity cs) const
    \fn bool QStringView::endsWith(QLatin1String l1, Qt::CaseSensitivity cs) const
    \fn bool QStringView::endsWith(QChar ch) const
    \fn bool QStringView::endsWith(QChar ch, Qt::CaseSensitivity cs) const

    Returns \c true if this string-view ends with string-view \a str,
    Latin-1 string \a l1, or character \a ch, respectively;
    otherwise returns \c false.

    If \a cs is Qt::CaseSensitive (the default), the search is case-sensitive;
    otherwise the search is case-insensitive.

    \sa startsWith()
*/

/*!
    \fn QByteArray QStringView::toLatin1() const

    Returns a Latin-1 representation of the string as a QByteArray.

    The behavior is undefined if the string contains non-Latin1 characters.

    \sa toUtf8(), toLocal8Bit(), QTextCodec
*/

/*!
    \fn QByteArray QStringView::toLocal8Bit() const

    Returns a local 8-bit representation of the string as a QByteArray.

    QTextCodec::codecForLocale() is used to perform the conversion from
    Unicode. If the locale's encoding could not be determined, this function
    does the same as toLatin1().

    The behavior is undefined if the string contains characters not
    supported by the locale's 8-bit encoding.

    \sa toLatin1(), toUtf8(), QTextCodec
*/

/*!
    \fn QByteArray QStringView::toUtf8() const

    Returns a UTF-8 representation of the string as a QByteArray.

    UTF-8 is a Unicode codec and can represent all characters in a Unicode
    string like QString.

    \sa toLatin1(), toLocal8Bit(), QTextCodec
*/

/*!
    \fn QVector<uint> QStringView::toUcs4() const

    Returns a UCS-4/UTF-32 representation of the string as a QVector<uint>.

    UCS-4 is a Unicode codec and therefore it is lossless. All characters from
    this string will be encoded in UCS-4. Any invalid sequence of code units in
    this string is replaced by the Unicode replacement character
    (QChar::ReplacementCharacter, which corresponds to \c{U+FFFD}).

    The returned vector is not 0-terminated.

    \sa toUtf8(), toLatin1(), toLocal8Bit(), QTextCodec
*/

/*!
    \fn template <typename QStringLike> qToStringViewIgnoringNull(const QStringLike &s);
    \since 5.10
    \internal

    Convert \a s to a QStringView ignoring \c{s.isNull()}.

    Returns a string-view that references \a{s}' data, but is never null.

    This is a faster way to convert a QString or QStringRef to a QStringView,
    if null QStrings can legitimately be treated as empty ones.

    \sa QString::isNull(), QStringRef::isNull(), QStringView
*/

/*!
    \fn bool QStringView::isRightToLeft() const
    \since 5.11

    Returns \c true if the string is read right to left.

    \sa QString::isRightToLeft()
*/

QT_END_NAMESPACE
