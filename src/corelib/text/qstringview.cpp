// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    std::basic_string, etc.) of QChar, \c ushort, \c char16_t or
    (on platforms, such as Windows, where it is a 16-bit type) \c wchar_t.

    QStringView is designed as an interface type; its main use-case is
    as a function parameter type. When QStringViews are used as automatic
    variables or data members, care must be taken to ensure that the referenced
    string data (for example, owned by a QString) outlives the QStringView on all code paths,
    lest the string view ends up referencing deleted data.

    When used as an interface type, QStringView allows a single function to accept
    a wide variety of UTF-16 string data sources. One function accepting QStringView
    thus replaces three function overloads (taking QString and
    \c{(const QChar*, qsizetype)}), while at the same time enabling even more string data
    sources to be passed to the function, such as \c{u"Hello World"}, a \c char16_t
    string literal.

    QStringViews should be passed by value, not by reference-to-const:
    \snippet code/src_corelib_text_qstringview.cpp 0

    If you want to give your users maximum freedom in what strings they can pass
    to your function, accompany the QStringView overload with overloads for

    \list
        \li \e QChar: this overload can delegate to the QStringView version:
            \snippet code/src_corelib_text_qstringview.cpp 1
            even though, for technical reasons, QStringView cannot provide a
            QChar constructor by itself.
        \li \e QString: if you store an unmodified copy of the string and thus would
            like to take advantage of QString's implicit sharing.
        \li QLatin1StringView: if you can implement the function without converting the
            QLatin1StringView to UTF-16 first; users expect a function overloaded on
            QLatin1StringView to perform strictly less memory allocations than the
            semantically equivalent call of the QStringView version, involving
            construction of a QString from the QLatin1StringView.
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

    \sa QString
*/

/*!
    \typedef QStringView::storage_type

    Alias for \c{char16_t}.
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

    Passing \nullptr as \a str is safe if \a len is 0, too, and results in a null string view.

    The behavior is undefined if \a len is negative or, when positive, if \a str is \nullptr.

    This constructor only participates in overload resolution if \c Char is a compatible
    character type. The compatible character types are: \c QChar, \c ushort, \c char16_t and
    (on platforms, such as Windows, where it is a 16-bit type) \c wchar_t.
*/

/*!
    \fn template <typename Char> QStringView::QStringView(const Char *first, const Char *last)

    Constructs a string view on \a first with length (\a last - \a first).

    The range \c{[first,last)} must remain valid for the lifetime of
    this string view object.

    Passing \c \nullptr as \a first is safe if \a last is \nullptr, too,
    and results in a null string view.

    The behavior is undefined if \a last precedes \a first, or \a first
    is \nullptr and \a last is not.

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

    Passing \nullptr as \a str is safe and results in a null string view.

    This constructor only participates in overload resolution if \a
    str is not an array and if \c Char is a compatible character
    type. The compatible character types are: \c QChar, \c ushort, \c
    char16_t and (on platforms, such as Windows, where it is a 16-bit
    type) \c wchar_t.
*/

/*!
    \fn template <typename Char, size_t N> QStringView::QStringView(const Char (&string)[N])

    Constructs a string view on the character string literal \a string.
    The view covers the array until the first \c{Char(0)} is encountered,
    or \c N, whichever comes first.
    If you need the full array, use fromArray() instead.

    \a string must remain valid for the lifetime of this string view
    object.

    This constructor only participates in overload resolution if \a
    string is an actual array and \c Char is a compatible character
    type. The compatible character types are: \c QChar, \c ushort, \c
    char16_t and (on platforms, such as Windows, where it is a 16-bit
    type) \c wchar_t.

    \sa fromArray
*/

/*!
    \fn QStringView::QStringView(const QString &str)

    Constructs a string view on \a str.

    \c{str.data()} must remain valid for the lifetime of this string view object.

    The string view will be null if and only if \c{str.isNull()}.
*/

/*!
    \fn template <typename Container, if_compatible_container<Container>> QStringView::QStringView(const Container &str)

    Constructs a string view on \a str. The length is taken from \c{std::size(str)}.

    \c{std::data(str)} must remain valid for the lifetime of this string view object.

    This constructor only participates in overload resolution if \c Container is a
    container with a compatible character type as \c{value_type}. The
    compatible character types are: \c QChar, \c ushort, \c char16_t and
    (on platforms, such as Windows, where it is a 16-bit type) \c wchar_t.

    The string view will be empty if and only if \c{std::size(str) == 0}. It is unspecified
    whether this constructor can result in a null string view (\c{std::data(str)} would
    have to return \nullptr for this).

    \sa isNull(), isEmpty()
*/

/*!
    \fn template <typename Char, size_t Size> static QStringView QStringView::fromArray(const Char (&string)[Size]) noexcept

    Constructs a string view on the full character string literal \a string,
    including any trailing \c{Char(0)}. If you don't want the
    null-terminator included in the view then you can chop() it off
    when you are certain it is at the end. Alternatively you can use
    the constructor overload taking an array literal which will create
    a view up to, but not including, the first null-terminator in the data.

    \a string must remain valid for the lifetime of this string view
    object.

    This function will work with any array literal if \c Char is a
    compatible character type. The compatible character types are: \c QChar, \c ushort, \c
    char16_t and (on platforms, such as Windows, where it is a 16-bit
    type) \c wchar_t.
*/

/*!
    \fn QString QStringView::toString() const

    Returns a deep copy of this string view's data as a QString.

    The return value will be the null QString if and only if this string view is null.
*/

/*!
    \fn const QChar *QStringView::data() const

//! [const-pointer-to-first-ch]
    Returns a const pointer to the first character in the string view.

    \note The character array represented by the return value is \e not null-terminated.
//! [const-pointer-to-first-ch]

    \sa begin(), end(), utf16()
*/

/*!
    \fn const QChar *QStringView::constData() const
    \since 6.0

    \include qstringview.cpp const-pointer-to-first-ch

    \sa data(), begin(), end(), utf16()
*/

/*!
    \fn const storage_type *QStringView::utf16() const

    \include qstringview.cpp const-pointer-to-first-ch

    \c{storage_type} is \c{char16_t}.

    \sa begin(), end(), data()
*/

/*!
    \fn QStringView::const_iterator QStringView::begin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first character in
    the string view.

    This function is provided for STL compatibility.

    \sa end(), constBegin(), cbegin(), rbegin(), data()
*/

/*!
    \fn QStringView::const_iterator QStringView::cbegin() const

    Same as begin().

    This function is provided for STL compatibility.

    \sa cend(), begin(), constBegin(), crbegin(), data()
*/

/*!
    \fn QStringView::const_iterator QStringView::constBegin() const
    \since 6.1

    Same as begin().

    \sa constEnd(), begin(), cbegin(), crbegin(), data()
*/

/*!
    \fn QStringView::const_iterator QStringView::end() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    character after the last character in the list.

    This function is provided for STL compatibility.

    \sa begin(), constEnd(), cend(), rend()
*/

/*! \fn QStringView::const_iterator QStringView::cend() const

    Same as end().

    This function is provided for STL compatibility.

    \sa cbegin(), end(), constEnd(), crend()
*/

/*! \fn QStringView::const_iterator QStringView::constEnd() const
    \since 6.1

    Same as end().

    \sa constBegin(), end(), cend(), crend()
*/

/*!
    \fn QStringView::const_reverse_iterator QStringView::rbegin() const

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator pointing to the first
    character in the string view, in reverse order.

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
    the last character in the string view, in reverse order.

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

    Returns the size of this string view, in UTF-16 code units (that is,
    surrogate pairs count as two for the purposes of this function, the same
    as in QString).

    \sa empty(), isEmpty(), isNull(), length()
*/

/*!
    \fn QStringView::length() const

    Same as size().

    This function is provided for compatibility with other Qt containers.

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
    \fn template <typename...Args> QString QStringView::arg(Args &&...args) const
    \fn template <typename...Args> QString QLatin1StringView::arg(Args &&...args) const
    \fn template <typename...Args> QString QString::arg(Args &&...args) const
    \since 5.14

    Replaces occurrences of \c{%N} in this string with the corresponding
    argument from \a args. The arguments are not positional: the first of
    the \a args replaces the \c{%N} with the lowest \c{N} (all of them), the
    second of the \a args the \c{%N} with the next-lowest \c{N} etc.

    \c Args can consist of anything that implicitly converts to QString,
    QStringView or QLatin1StringView.

    In addition, the following types are also supported: QChar, QLatin1Char.

    \sa QString::arg()
*/

/*!
    \fn QChar QStringView::front() const

    Returns the first character in the string view. Same as first().

    This function is provided for STL compatibility.

//! [calling-on-empty-is-UB]
    \warning Calling this function on an empty string view constitutes
    undefined behavior.
//! [calling-on-empty-is-UB]

    \sa back(), first(), last()
*/

/*!
    \fn QChar QStringView::back() const

    Returns the last character in the string view. Same as last().

    This function is provided for STL compatibility.

    \include qstringview.cpp calling-on-empty-is-UB

    \sa front(), first(), last()
*/

/*!
    \fn QChar QStringView::first() const

    Returns the first character in the string view. Same as front().

    This function is provided for compatibility with other Qt containers.

    \include qstringview.cpp calling-on-empty-is-UB

    \sa front(), back(), last()
*/

/*!
    \fn QChar QStringView::last() const

    Returns the last character in the string view. Same as back().

    This function is provided for compatibility with other Qt containers.

    \include qstringview.cpp calling-on-empty-is-UB

    \sa back(), front(), first()
*/

/*!
    \fn QStringView QStringView::mid(qsizetype start, qsizetype length) const

    Returns the substring of length \a length starting at position
    \a start in this object.

    \deprecated Use sliced() instead in new code.

    Returns an empty string view if \a start exceeds the
    length of the string view. If there are less than \a length characters
    available in the string view starting at \a start, or if
    \a length is negative (default), the function returns all characters that
    are available from \a start.

    \sa first(), last(), sliced(), chopped(), chop(), truncate()
*/

/*!
    \fn QStringView QStringView::left(qsizetype length) const

    \deprecated Use first() instead in new code.

    Returns the substring of length \a length starting at position
    0 in this object.

    The entire string view is returned if \a length is greater than or equal
    to size(), or less than zero.

    \sa first(), last(), sliced(), startsWith(), chopped(), chop(), truncate()
*/

/*!
    \fn QStringView QStringView::right(qsizetype length) const

    \deprecated Use last() instead in new code.

    Returns the substring of length \a length starting at position
    size() - \a length in this object.

    The entire string view is returned if \a length is greater than or equal
    to size(), or less than zero.

    \sa first(), last(), sliced(), endsWith(), chopped(), chop(), truncate()
*/

/*!
    \fn QStringView QStringView::first(qsizetype n) const
    \since 6.0

    Returns a string view that points to the first \a n characters
    of this string view.

    \note The behavior is undefined when \a n < 0 or \a n > size().

    \sa last(), sliced(), startsWith(), chopped(), chop(), truncate()
*/

/*!
    \fn QStringView QStringView::last(qsizetype n) const
    \since 6.0

    Returns a string view that points to the last \a n characters of this string
    view.

    \note The behavior is undefined when \a n < 0 or \a n > size().

    \sa first(), sliced(), endsWith(), chopped(), chop(), truncate()
*/

/*!
    \fn QStringView QStringView::sliced(qsizetype pos, qsizetype n) const
    \since 6.0

    Returns a string view that points to \a n characters of this string view,
    starting at position \a pos.

    \note The behavior is undefined when \a pos < 0, \a n < 0,
    or \a pos + \a n > size().

    \sa first(), last(), chopped(), chop(), truncate()
*/

/*!
    \fn QStringView QStringView::sliced(qsizetype pos) const
    \since 6.0
    \overload

    Returns a string view starting at position \a pos in this object,
    and extending to its end.

    \note The behavior is undefined when \a pos < 0 or \a pos > size().

    \sa first(), last(), chopped(), chop(), truncate()
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
    \fn int QStringView::compare(QStringView str, Qt::CaseSensitivity cs) const
    \since 5.12

    Returns an integer that compares to zero as this string view compares to the
    string view \a str.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {comparison}

    \sa operator==(), operator<(), operator>()
*/

/*!
    \fn int QStringView::compare(QUtf8StringView str, Qt::CaseSensitivity cs) const
    \since 6.5

    Returns an integer that compares to zero as this string view compares to the
    string view \a str.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {comparison}

    \sa operator==(), operator<(), operator>()
*/

/*!
    \fn int QStringView::compare(QLatin1StringView l1, Qt::CaseSensitivity cs) const
    \fn int QStringView::compare(QChar ch) const
    \fn int QStringView::compare(QChar ch, Qt::CaseSensitivity cs) const
    \since 5.15

    Returns an integer that compares to zero as this string view compares to the
    Latin-1 string viewed by \a l1, or the character \a ch, respectively.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {comparison}

    \sa operator==(), operator<(), operator>()
*/

/*!
    \fn QStringView::operator==(QStringView lhs, QStringView rhs)
    \fn QStringView::operator!=(QStringView lhs, QStringView rhs)
    \fn QStringView::operator< (QStringView lhs, QStringView rhs)
    \fn QStringView::operator<=(QStringView lhs, QStringView rhs)
    \fn QStringView::operator> (QStringView lhs, QStringView rhs)
    \fn QStringView::operator>=(QStringView lhs, QStringView rhs)

    Operators for comparing \a lhs to \a rhs.

    \sa compare()
*/

/*!
    \fn int QStringView::localeAwareCompare(QStringView other) const
    \since 6.4

    Compares this string view with the \a other string view and returns
    an integer less than, equal to, or greater than zero if this string
    view is less than, equal to, or greater than the \a other string view.

    The comparison is performed in a locale- and also platform-dependent
    manner. Use this function to present sorted lists of strings to the
    user.

    \sa {Comparing Strings}
*/

/*
//! [utf16-or-latin1-or-ch]
the UTF-16 string viewed by \a str, the Latin-1 string viewed by \a l1,
or the character \a ch
//! [utf16-or-latin1-or-ch]
*/

/*!
    \fn bool QStringView::startsWith(QStringView str, Qt::CaseSensitivity cs) const
    \fn bool QStringView::startsWith(QLatin1StringView l1, Qt::CaseSensitivity cs) const
    \fn bool QStringView::startsWith(QChar ch) const
    \fn bool QStringView::startsWith(QChar ch, Qt::CaseSensitivity cs) const

    Returns \c true if this string view starts with
    \include qstringview.cpp utf16-or-latin1-or-ch
    respectively; otherwise returns \c false.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa endsWith()
*/

/*!
    \fn bool QStringView::endsWith(QStringView str, Qt::CaseSensitivity cs) const
    \fn bool QStringView::endsWith(QLatin1StringView l1, Qt::CaseSensitivity cs) const
    \fn bool QStringView::endsWith(QChar ch) const
    \fn bool QStringView::endsWith(QChar ch, Qt::CaseSensitivity cs) const

    Returns \c true if this string view ends with
    \include qstringview.cpp utf16-or-latin1-or-ch
    respectively; otherwise returns \c false.

     \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa startsWith()
*/

/*!
    \fn qsizetype QStringView::indexOf(QStringView str, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    \fn qsizetype QStringView::indexOf(QLatin1StringView l1, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    \fn qsizetype QStringView::indexOf(QChar c, qsizetype from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    \since 5.14

    Returns the index position of the first occurrence of
    \include qstringview.cpp utf16-or-latin1-or-ch
    respectively, in this string view, searching forward from index position
    \a from. Returns -1 if \a str, \a l1 or \a ch is not found, respectively.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \include qstring.qdocinc negative-index-start-search-from-end

    \sa QString::indexOf()
*/

/*!
    \fn bool QStringView::contains(QStringView str, Qt::CaseSensitivity cs) const
    \fn bool QStringView::contains(QLatin1StringView l1, Qt::CaseSensitivity cs) const
    \fn bool QStringView::contains(QChar c, Qt::CaseSensitivity cs) const
    \since 5.14

    Returns \c true if this string view contains an occurrence of
    \include qstringview.cpp utf16-or-latin1-or-ch
    respectively; otherwise returns \c false.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa indexOf()
*/

/*!
    \fn qsizetype QStringView::lastIndexOf(QStringView str, qsizetype from, Qt::CaseSensitivity cs) const
    \fn qsizetype QStringView::lastIndexOf(QLatin1StringView l1, qsizetype from, Qt::CaseSensitivity cs) const
    \fn qsizetype QStringView::lastIndexOf(QChar c, qsizetype from, Qt::CaseSensitivity cs) const
    \since 5.14

    Returns the index position of the last occurrence of
    \include qstringview.cpp utf16-or-latin1-or-ch
    respectively, in this string view, searching backward from index
    position \a from.

    \include qstring.qdocinc negative-index-start-search-from-end

    Returns -1 if \a str, \a l1 or \a c is not found, respectively.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \note When searching for a 0-length \a str or \a l1, the match at
    the end of the data is excluded from the search by a negative \a
    from, even though \c{-1} is normally thought of as searching from
    the end of the string view: the match at the end is \e after the
    last character, so it is excluded. To include such a final empty
    match, either give a positive value for \a from or omit the \a from
    parameter entirely.

    \sa QString::lastIndexOf()
*/

/*!
    \fn qsizetype QStringView::lastIndexOf(QStringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    \fn qsizetype QStringView::lastIndexOf(QLatin1StringView l1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    \since 6.2
    \overload lastIndexOf()

    Returns the index position of the last occurrence of the UTF-16 string viewed
    by \a str or the Latin-1 string viewed by \a l1 respectively, in this string
    view searching backward from the last character of this string view. Returns
    -1 if \a str or \a l1 is not found, respectively.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa QString::lastIndexOf()
*/

/*!
    \fn QStringView::lastIndexOf(QChar c, Qt::CaseSensitivity cs) const
    \since 6.3
    \overload lastIndexOf()
*/

#if QT_CONFIG(regularexpression)
/*!
    \fn qsizetype QStringView::indexOf(const QRegularExpression &re, qsizetype from, QRegularExpressionMatch *rmatch) const
    \since 6.1

    Returns the index position of the first match of the regular
    expression \a re in the string view, searching forward from index
    position \a from. Returns -1 if \a re didn't match anywhere.

    If the match is successful and \a rmatch is not \nullptr, it also
    writes the results of the match into the QRegularExpressionMatch object
    pointed to by \a rmatch.

    \note Due to how the regular expression matching algorithm works,
    this function will actually match repeatedly from the beginning of
    the string view until the position \a from is reached.
*/

/*!
    \fn qsizetype QStringView::lastIndexOf(const QRegularExpression &re, qsizetype from, QRegularExpressionMatch *rmatch) const
    \since 6.1

    Returns the index position of the last match of the regular
    expression \a re in the string view, which starts before the index
    position \a from.

    \include qstring.qdocinc negative-index-start-search-from-end

    Returns -1 if \a re didn't match anywhere.

    If the match is successful and \a rmatch is not \nullptr, it also
    writes the results of the match into the QRegularExpressionMatch object
    pointed to by \a rmatch.

    \note Due to how the regular expression matching algorithm works,
    this function will actually match repeatedly from the beginning of
    the string view until the position \a from is reached.

    \note When searching for a regular expression \a re that may match
    0 characters, the match at the end of the data is excluded from the
    search by a negative \a from, even though \c{-1} is normally
    thought of as searching from the end of the string view: the match
    at the end is \e after the last character, so it is excluded. To
    include such a final empty match, either give a positive value for
    \a from or omit the \a from parameter entirely.
*/

/*!
    \fn qsizetype QStringView::lastIndexOf(const QRegularExpression &re, QRegularExpressionMatch *rmatch = nullptr) const
    \since 6.2

    Returns the index position of the last match of the regular
    expression \a re in the string view. Returns -1 if \a re didn't match
    anywhere.

    If the match is successful and \a rmatch is not \nullptr, it also
    writes the results of the match into the QRegularExpressionMatch object
    pointed to by \a rmatch.

    \note Due to how the regular expression matching algorithm works,
    this function will actually match repeatedly from the beginning of
    the string view until the end of the string view is reached.
*/

/*!
    \fn bool QStringView::contains(const QRegularExpression &re, QRegularExpressionMatch *rmatch) const
    \since 6.1

    Returns \c true if the regular expression \a re matches somewhere in this
    string view; otherwise returns \c false.

    If the match is successful and \a rmatch is not \nullptr, it also
    writes the results of the match into the QRegularExpressionMatch object
    pointed to by \a rmatch.

    \sa QRegularExpression::match()
*/

/*!
    \fn qsizetype QStringView::count(const QRegularExpression &re) const
    \since 6.1

    Returns the number of times the regular expression \a re matches
    in the string view.

    For historical reasons, this function counts overlapping matches.
    This behavior is different from simply iterating over the matches
    in the string view using QRegularExpressionMatchIterator.

    \sa QRegularExpression::globalMatch()

*/
#endif // QT_CONFIG(regularexpression)

/*!
    \fn QByteArray QStringView::toLatin1() const

    Returns a Latin-1 representation of the string as a QByteArray.

    The behavior is undefined if the string contains non-Latin1 characters.

    \sa toUtf8(), toLocal8Bit(), QStringEncoder
*/

/*!
    \fn QByteArray QStringView::toLocal8Bit() const

    Returns a local 8-bit representation of the string as a QByteArray.

    On Unix systems this is equivalen to toUtf8(), on Windows the systems
    current code page is being used.

    The behavior is undefined if the string contains characters not
    supported by the locale's 8-bit encoding.

    \sa toLatin1(), toUtf8(), QStringEncoder
*/

/*!
    \fn QByteArray QStringView::toUtf8() const

    Returns a UTF-8 representation of the string view as a QByteArray.

    UTF-8 is a Unicode codec and can represent all characters in a Unicode
    string like QString.

    \sa toLatin1(), toLocal8Bit(), QStringEncoder
*/

/*!
    \fn QList<uint> QStringView::toUcs4() const

    Returns a UCS-4/UTF-32 representation of the string view as a QList<uint>.

    UCS-4 is a Unicode codec and therefore it is lossless. All characters from
    this string view will be encoded in UCS-4. Any invalid sequence of code units in
    this string view is replaced by the Unicode replacement character
    (QChar::ReplacementCharacter, which corresponds to \c{U+FFFD}).

    The returned list is not 0-terminated.

    \sa toUtf8(), toLatin1(), toLocal8Bit(), QStringEncoder
*/

/*!
    \fn template <typename QStringLike> qToStringViewIgnoringNull(const QStringLike &s);
    \since 5.10
    \internal

    Convert \a s to a QStringView ignoring \c{s.isNull()}.

    Returns a string view that references \a{s}' data, but is never null.

    This is a faster way to convert a QString to a QStringView,
    if null QStrings can legitimately be treated as empty ones.

    \sa QString::isNull(), QStringView
*/

/*!
    \fn bool QStringView::isRightToLeft() const
    \since 5.11

    Returns \c true if the string view is read right to left.

    \sa QString::isRightToLeft()
*/

/*!
    \fn bool QStringView::isValidUtf16() const
    \since 5.15

    Returns \c true if the string view contains valid UTF-16 encoded data,
    or \c false otherwise.

    Note that this function does not perform any special validation of the
    data; it merely checks if it can be successfully decoded from UTF-16.
    The data is assumed to be in host byte order; the presence of a BOM
    is meaningless.

    \sa QString::isValidUtf16()
*/

/*!
    \fn QStringView::toWCharArray(wchar_t *array) const
    \since 5.14

    Transcribes this string view into the given \a array.

    The caller is responsible for ensuring \a array is large enough to hold the
    \c wchar_t encoding of this string view (allocating the array with the same length
    as the string view is always sufficient). The array is encoded in UTF-16 on
    platforms where \c wchar_t is 2 bytes wide (e.g. Windows); otherwise (Unix
    systems), \c wchar_t is assumed to be 4 bytes wide and the data is written
    in UCS-4.

    \note This function writes no null terminator to the end of \a array.

    Returns the number of \c wchar_t entries written to \a array.

    \sa QString::toWCharArray()
*/

/*!
    \fn qsizetype QStringView::count(QChar ch, Qt::CaseSensitivity cs) const noexcept

    \since 6.0
    \overload count()

    Returns the number of occurrences of the character \a ch in the
    string view.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa QString::count(), contains(), indexOf()
*/

/*!
    \fn qsizetype QStringView::count(QStringView str, Qt::CaseSensitivity cs) const noexcept

    \since 6.0
    \overload count()

    Returns the number of (potentially overlapping) occurrences of the
    string view \a str in this string view.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa QString::count(), contains(), indexOf()
*/

/*!
    \fn qsizetype QStringView::count(QLatin1StringView l1, Qt::CaseSensitivity cs) const noexcept

    \since 6.4
    \overload count()

    Returns the number of (potentially overlapping) occurrences of the
    Latin-1 string viewed by \a l1 in this string view.

    \include qstring.qdocinc {search-comparison-case-sensitivity} {search}

    \sa QString::count(), contains(), indexOf()
*/

/*!
  \fn qint64 QStringView::toLongLong(bool *ok, int base) const

    Returns the string view converted to a \c{long long} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string view
    begins with "0x", base 16 is used; otherwise, if the string view begins with "0",
    base 8 is used; otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For locale
    dependent conversion use QLocale::toLongLong()

    \sa QString::toLongLong()

    \since 6.0
*/

/*!
  \fn quint64 QStringView::toULongLong(bool *ok, int base) const

    Returns the string view converted to an \c{unsigned long long} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string view
    begins with "0x", base 16 is used; otherwise, if the string view begins with "0",
    base 8 is used; otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For locale
    dependent conversion use QLocale::toULongLong()

    \sa QString::toULongLong()

    \since 6.0
*/

/*!
    \fn long QStringView::toLong(bool *ok, int base) const

    Returns the string view converted to a \c long using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string view
    begins with "0x", base 16 is used; otherwise, if the string view begins with "0",
    base 8 is used; otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For locale
    dependent conversion use QLocale::toLong()

    \sa QString::toLong()

    \since 6.0
*/

/*!
    \fn ulong QStringView::toULong(bool *ok, int base) const

    Returns the string view converted to an \c{unsigned long} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string view
    begins with "0x", base 16 is used; otherwise, if the string view begins with "0",
    base 8 is used; otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For locale
    dependent conversion use QLocale::toULongLong()

    \sa QString::toULong()

    \since 6.0
*/

/*!
  \fn int QStringView::toInt(bool *ok, int base) const

    Returns the string view converted to an \c int using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string view
    begins with "0x", base 16 is used; otherwise, if the string view begins with "0",
    base 8 is used; otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For locale
    dependent conversion use QLocale::toInt()

    \sa QString::toInt()

    \since 6.0
*/

/*!
  \fn uint QStringView::toUInt(bool *ok, int base) const

    Returns the string view converted to an \c{unsigned int} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string view
    begins with "0x", base 16 is used; otherwise, if the string view begins with "0",
    base 8 is used; otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For locale
    dependent conversion use QLocale::toUInt()

    \sa QString::toUInt()

    \since 6.0
*/

/*!
  \fn short QStringView::toShort(bool *ok, int base) const

    Returns the string view converted to a \c short using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string view
    begins with "0x", base 16 is used; otherwise, if the string view begins with "0",
    base 8 is used; otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For locale
    dependent conversion use QLocale::toShort()

    \sa QString::toShort()

    \since 6.0
*/

/*!
  \fn ushort QStringView::toUShort(bool *ok, int base) const

    Returns the string view converted to an \c{unsigned short} using base \a
    base, which is 10 by default and must be between 2 and 36, or 0.
    Returns 0 if the conversion fails.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    If \a base is 0, the C language convention is used: if the string view
    begins with "0x", base 16 is used; otherwise, if the string view begins with "0",
    base 8 is used; otherwise, base 10 is used.

    The string conversion will always happen in the 'C' locale. For locale
    dependent conversion use QLocale::toUShort()

    \sa QString::toUShort()

    \since 6.0
*/

/*!
  \fn double QStringView::toDouble(bool *ok) const

    Returns the string view converted to a \c double value.

    Returns an infinity if the conversion overflows or 0.0 if the
    conversion fails for other reasons (e.g. underflow).

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    The string conversion will always happen in the 'C' locale. For locale
    dependent conversion use QLocale::toDouble()

    For historic reasons, this function does not handle
    thousands group separators. If you need to convert such numbers,
    use QLocale::toDouble().

    \sa QString::toDouble()

    \since 6.0
*/

/*!
  \fn float QStringView::toFloat(bool *ok) const

    Returns the string view converted to a \c float value.

    Returns an infinity if the conversion overflows or 0.0 if the
    conversion fails for other reasons (e.g. underflow).

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    The string conversion will always happen in the 'C' locale. For locale
    dependent conversion use QLocale::toFloat()

    \sa QString::toFloat()

    \since 6.0
*/


/*!
    \fn template <typename Needle, typename...Flags> auto QStringView::tokenize(Needle &&sep, Flags...flags) const
    \fn template <typename Needle, typename...Flags> auto QLatin1StringView::tokenize(Needle &&sep, Flags...flags) const
    \fn template <typename Needle, typename...Flags> auto QString::tokenize(Needle &&sep, Flags...flags) const &
    \fn template <typename Needle, typename...Flags> auto QString::tokenize(Needle &&sep, Flags...flags) const &&
    \fn template <typename Needle, typename...Flags> auto QString::tokenize(Needle &&sep, Flags...flags) &&

    Splits the string into substring views wherever \a sep occurs, and
    returns a lazy sequence of those strings.

    Equivalent to

    \code
    return QStringTokenizer{std::forward<Needle>(sep), flags...};
    \endcode

    except it works without C++17 Class Template Argument Deduction (CTAD)
    enabled in the compiler.

    See QStringTokenizer for how \a sep and \a flags interact to form
    the result.

    \note While this function returns QStringTokenizer, you should never,
    ever, name its template arguments explicitly. If you can use C++17 Class
    Template Argument Deduction (CTAD), you may write
    \code
    QStringTokenizer result = sv.tokenize(sep);
    \endcode
    (without template arguments). If you can't use C++17 CTAD, you must store
    the return value only in \c{auto} variables:
    \code
    auto result = sv.tokenize(sep);
    \endcode
    This is because the template arguments of QStringTokenizer have a very
    subtle dependency on the specific tokenize() overload from which they are
    returned, and they don't usually correspond to the type used for the separator.

    \since 6.0
    \sa QStringTokenizer, qTokenize()
*/

QT_END_NAMESPACE
