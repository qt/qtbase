// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:trivial-impl-only

#include "qanystringview.h"
#include "qdebug.h"
#include "qttypetraits.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAnyStringView
    \inmodule QtCore
    \since 6.0
    \brief The QAnyStringView class provides a unified view on Latin-1, UTF-8,
           or UTF-16 strings with a read-only subset of the QString API.
    \reentrant
    \ingroup tools
    \ingroup string-processing

    \compares strong
    \compareswith strong char16_t QChar {const char16_t *} {const char *} \
                  QByteArray QByteArrayView QString QStringView QUtf8StringView \
                  QLatin1StringView
    \endcompareswith

    A QAnyStringView references a contiguous portion of a string it does
    not own. It acts as an interface type to all kinds of strings,
    without the need to construct a QString first.

    Unlike QStringView and QUtf8StringView, QAnyStringView can hold
    strings of any of the following encodings: UTF-8, UTF-16, and
    Latin-1. The latter is supported because Latin-1, unlike UTF-8,
    can be efficiently compared to UTF-16 data: a length mismatch
    already means the strings cannot be equal. This is not true for
    UTF-8/UTF-16 comparisons, because UTF-8 is a variable-length
    encoding.

    The string may be represented as an array (or an array-compatible
    data-structure such as QString, std::basic_string, etc.) of \c
    char, \c char8_t, QChar, \c ushort, \c char16_t or (on platforms,
    such as Windows, where it is a 16-bit type) \c wchar_t.

    QAnyStringView is designed as an interface type; its main use-case
    is as a function parameter type. When QAnyStringViews are used as
    automatic variables or data members, care must be taken to ensure
    that the referenced string data (for example, owned by a QString)
    outlives the QAnyStringView on all code paths, lest the string
    view ends up referencing deleted data.

    For example,

    \code
    QAnyStringView str = funcReturningQString(); // return value is a temp
    \endcode

    would leave \c{str} referencing the deleted temporary (which constitutes
    undefined behavior). This is particularly true for the single-character
    constructors:

    \code
    QAnyStringView ch = u' '; // u' ' is a temporary
    // oops, ch references deleted temporary
    \endcode

    In both cases, the solution is to "pin" the temporary to an lvalue and only
    then create a QAnyStringView from it:

    \code
    const auto r = funcReturningQString();
    QAnyStringView str = r; // ok, `r` outlives `str`
    const auto sp = u' ';
    QAnyStringView ch = sp; // ok, `sp` outlives `ch`
    \endcode

    However, using QAnyStringView as the interface type that it is intended to
    be is \e{always} safe, provided the called function's documentation is not
    asking for a longer lifetime:

    \code
    void func(QAnyStringView s);
    func(u' ');
    func(functionReturningQString());
    \endcode

    This is why QAnyStringView supports these conversions in the first place.

    When used as an interface type, QAnyStringView allows a single
    function to accept a wide variety of string data sources. One
    function accepting QAnyStringView thus replaces five function
    overloads (taking QString, \c{(const QChar*, qsizetype)},
    QUtf8StringView, QLatin1StringView (but see above), and QChar), while
    at the same time enabling even more string data sources to be
    passed to the function, such as \c{u8"Hello World"}, a \c char8_t
    string literal.

    Like elsewhere in Qt, QAnyStringView assumes \c char data is encoded
    in UTF-8, unless it is presented as a QLatin1StringView.

    Since Qt 6.4, however, UTF-8 string literals that are pure US-ASCII are
    automatically stored as Latin-1. This is a compile-time check with no
    runtime overhead. The feature requires compiling in C++20, or with a recent
    GCC.

    QAnyStringViews should be passed by value, not by reference-to-const:
    \snippet code/src_corelib_text_qanystringview.cpp 0

    QAnyStringView can also be used as the return value of a function,
    but this is not recommended. QUtf8StringView or QStringView are
    better suited as function return values.  If you call a function
    returning QAnyStringView, take extra care to not keep the
    QAnyStringView around longer than the function promises to keep
    the referenced string data alive. If in doubt, obtain a strong
    reference to the data by calling toString() to convert the
    QAnyStringView into a QString.

    QAnyStringView is a \e{Literal Type}.

    \section2 Compatible Character Types

    QAnyStringView accepts strings over a variety of character types:

    \list
    \li \c char (both signed and unsigned)
    \li \c char8_t (C++20 only)
    \li \c char16_t
    \li \c wchar_t (where it's a 16-bit type, e.g. Windows)
    \li \c ushort
    \li \c QChar
    \endlist

    The 8-bit character types are interpreted as UTF-8 data (except when
    presented as a QLatin1StringView) while the 16-bit character types are
    interpreted as UTF-16 data in host byte order (the same as QString).

    The following character types are only supported by the single-character
    constructor:

    \list
    \li \c QLatin1Char
    \li \c QChar::SpecialCharacter
    \li \c wchar_t (where it's a 32-bit type, i.e. Unix) (since 6.10)
    \li \c char32_t
    \endlist

    These character types are internally decomposed into a UTF-16
    sequence (using QChar::fromUcs4() for the last).

    \section2 Sizes and Sub-Strings

    All sizes and positions in QAnyStringView functions are in the
    encoding's code units (that is, UTF-16 surrogate pairs count as
    two for the purposes of these functions, the same as in QString,
    and UTF-8 multibyte sequences count as two, three or four,
    depending on their length).

    \sa {Which string class to use?}, QUtf8StringView, QStringView
*/

/*!
    \typedef QAnyStringView::difference_type

    Alias for \c{std::ptrdiff_t}. Provided for compatibility with the STL.
*/

/*!
    \typedef QAnyStringView::size_type

    Alias for qsizetype. Provided for compatibility with the STL.
*/

/*!
    \fn QAnyStringView::QAnyStringView()

    Constructs a null string view.

    \sa isNull()
*/

/*!
    \fn QAnyStringView::QAnyStringView(std::nullptr_t)

    Constructs a null string view.

    \sa isNull()
*/

/*!
    \fn template <typename Char, QAnyStringView::if_compatible_char<Char> = true> QAnyStringView::QAnyStringView(const Char *str, qsizetype len)

    Constructs a string view on \a str with length \a len.

    The range \c{[str,len)} must remain valid for the lifetime of this string view object.

    Passing \nullptr as \a str is safe if \a len is 0, too, and results in a null string view.

    The behavior is undefined if \a len is negative or, when positive, if \a str is \nullptr.

    \constraints \c Char is a compatible character type.

    \sa isNull(), {Compatible Character Types}
*/

/*!
    \fn template <typename Char, QAnyStringView::if_compatible_char<Char> = true> QAnyStringView::QAnyStringView(const Char *first, const Char *last)

    Constructs a string view on \a first with length (\a last - \a first).

    The range \c{[first,last)} must remain valid for the lifetime of
    this string view object.

    Passing \nullptr as \a first is safe if \a last is \nullptr, too,
    and results in a null string view.

    The behavior is undefined if \a last precedes \a first, or \a first
    is \nullptr and \a last is not.

    \constraints \c Char is a compatible character type.

    \sa isNull(), {Compatible Character Types}
*/

/*!
    \fn template <typename Char> QAnyStringView::QAnyStringView(const Char *str)

    Constructs a string view on \a str. The length is determined
    by scanning for the first \c{Char(0)}.

    \a str must remain valid for the lifetime of this string view object.

    Passing \nullptr as \a str is safe and results in a null string view.

    \constraints \a str is not an array and \c Char is a
    compatible character type.

    \sa isNull(), {Compatible Character Types}
*/

/*!
    \fn template <typename Char, QAnyStringView::if_compatible_char<Char>> QAnyStringView::QAnyStringView(const Char &ch)

    Constructs a string view on the single character \a ch. The length is usually
    \c{1} (but see below).

    In general, you must assume that a QAnyStringView thus created will start
    to reference stale data at the end of the
    \l{https://en.cppreference.com/w/cpp/language/expressions#Full-expressions}{full-expression},
    when temporaries are deleted. That means that using it to pass a single
    character to a QAnyStringView-taking function is ok and safe (as long as
    the function documentation doesn't ask for a lifetime longer than the
    initial call):

    \code
    int to_int(QAnyStringView);
    int res = to_int(u'9'); // OK, data stays around for the duration of the call
    \endcode

    But keeping the object around longer is undefined behavior:

    \code
    QAnyStringView ch = u'9';
    int res = to_int(ch); // (silent) ERROR: ch references deleted data
    \endcode

    If you need this, prefer

    \code
    const auto nine = u'9';
    QAnyStringView ch(nine); // ok, references `nine`, which outlives `ch`
    int res = to_int(ch); // 9
    \endcode

    The above is true for all directly supported \l{Compatible Character Types}.

    If \a ch is not one of these types, but merely converts to QChar, e.g.
    QChar::SpecialCharacter or QLatin1Char, the QAnyStringView will bind to a
    temporary object that will have been deleted at the end of the full
    expression, just like in the second example.

    If \a ch cannot be represented in a single UTF-16 code unit (e.g. because
    it's a \c{char32_t} value), this constructor decomposes \a ch into two
    UFT-16 code units. The resulting QAnyStringView will have a size() of \c{2}
    in that case, and the temporary buffer in which the decomposition is stored
    is deleted at the end of the full-expression, similar to

    \code
    [](char32_t ch, auto &&tmp = QChar::fromUcs4(ch)) {
        return QAnyStringView(tmp);
    }
    \endcode

    The equivalent safe version in this case would be

    \code
    const auto decomposed = QChar::fromUcs4(ch);
    QAnyStringView ch(decomposed);
    \endcode

    \sa QChar::fromUcs4(), {Compatible Character Types}
*/

/*!
    \fn template <typename Char, size_t N> QAnyStringView::QAnyStringView(const Char (&string)[N])

    Constructs a string view on the character string literal \a string.
    The view covers the array until the first \c{Char(0)} is encountered,
    or \c N, whichever comes first.
    If you need the full array, use fromArray() instead.

    \a string must remain valid for the lifetime of this string view
    object.

    \constraints \a
    string is an actual array and \c Char is a compatible character
    type.

    \sa {Compatible Character Types}
*/

/*!
    \fn QAnyStringView::QAnyStringView(const QString &str)

    Constructs a string view on \a str.

    \c{str.data()} must remain valid for the lifetime of this string view object.

    The string view will be null if and only if \c{str.isNull()}.
*/

/*!
    \fn QAnyStringView::QAnyStringView(const QByteArray &str)

    Constructs a string view on \a str. The data in \a str is interpreted as UTF-8.

    \c{str.data()} must remain valid for the lifetime of this string view object.

    The string view will be null if and only if \c{str.isNull()}.
*/

/*!
    \fn template <typename Container, QAnyStringView::if_compatible_container<Container>> QAnyStringView::QAnyStringView(const Container &str)

    Constructs a string view on \a str. The length is taken from \c{std::size(str)}.

    \c{std::data(str)} must remain valid for the lifetime of this string view object.

    The string view will be empty if and only if \c{std::size(str) == 0}. It is unspecified
    whether this constructor can result in a null string view (\c{std::data(str)} would
    have to return \nullptr for this).

    \constraints \c Container is a
    container with a compatible character type as \c{value_type}.

    \sa isNull(), isEmpty()
*/

// confirm we don't make an accidental copy constructor:
static_assert(QtPrivate::IsContainerCompatibleWithQStringView<QAnyStringView>::value == false);
static_assert(QtPrivate::IsContainerCompatibleWithQUtf8StringView<QAnyStringView>::value == false);

/*!
    \fn template <typename Char, size_t Size> static QAnyStringView fromArray(const Char (&string)[Size]) noexcept

    Constructs a string view on the full character string literal \a string,
    including any trailing \c{Char(0)}. If you don't want the
    null-terminator included in the view then you can use the constructor
    overload taking a pointer and a size:

    \snippet code/src_corelib_text_qanystringview.cpp 2

    Alternatively you can use the constructor overload taking an
    array literal which will create a view up to, but not including,
    the first null-terminator in the data.

    \a string must remain valid for the lifetime of this string view
    object.

    This function will work with any array literal if \c Char is a
    compatible character type.
*/

/*!
    \fn QString QAnyStringView::toString() const

    Returns a deep copy of this string view's data as a QString.

    The return value will be a null QString if and only if this string view is null.
*/

/*!
    \fn const void *QAnyStringView::data() const

    Returns a const pointer to the first character in the string view.

    \note The character array represented by the return value is \e not null-terminated.

    \sa size_bytes()
*/

/*!
    \fn bool QAnyStringView::empty() const

    Returns whether this string view is empty - that is, whether \c{size() == 0}.

    This function is provided for STL compatibility.

    \sa isEmpty(), isNull(), size()
*/

/*!
    \fn bool QAnyStringView::isEmpty() const

    Returns whether this string view is empty - that is, whether \c{size() == 0}.

    This function is provided for compatibility with other Qt containers.

    \sa empty(), isNull(), size()
*/

/*!
    \fn bool QAnyStringView::isNull() const

    Returns whether this string view is null - that is, whether \c{data() == nullptr}.

    This functions is provided for compatibility with other Qt containers.

    \sa empty(), isEmpty(), size()
*/

/*!
    \fn qsizetype QAnyStringView::size() const

    Returns the size of this string view, in the encoding's code points.

    \sa empty(), isEmpty(), isNull(), size_bytes(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView::size_bytes() const

    Returns the size of this string view, but in bytes, not code-points.

    You can use this function together with data() for hashing or serialization.

    This function is provided for STL compatibility.

    \sa size(), data()
*/

/*!
    \fn QAnyStringView::length() const

    Same as size().

    This function is provided for compatibility with other Qt containers.

    \sa size()
*/

/*!
    \fn QChar QAnyStringView::front() const

    Returns the first character in the string view.

    This function is provided for STL compatibility.

    \warning Calling this function on an empty string view constitutes
    undefined behavior.

    \sa back(), {Sizes and Sub-Strings}
*/

/*!
    \fn QChar QAnyStringView::back() const

    Returns the last character in the string view.

    This function is provided for STL compatibility.

    \warning Calling this function on an empty string view constitutes
    undefined behavior.

    \sa front(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView::mid(qsizetype pos, qsizetype n) const
    \since 6.5

    Returns the substring of length \a n starting at position
    \a pos in this object.

    \deprecated Use sliced() instead in new code.

    Returns an empty string view if \a n exceeds the
    length of the string view. If there are less than \a n code points
    available in the string view starting at \a pos, or if
    \a n is negative (default), the function returns all code points that
    are available from \a pos.

    \sa first(), last(), sliced(), chopped(), chop(), truncate(), slice(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView::left(qsizetype n) const
    \since 6.5

    \deprecated Use first() instead in new code.

    Returns the substring of length \a n starting at position
    0 in this object.

    The entire string view is returned if \a n is greater than or equal
    to size(), or less than zero.

    \sa first(), last(), sliced(), chopped(), chop(), truncate(), slice(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView::right(qsizetype n) const
    \since 6.5

    \deprecated Use last() instead in new code.

    Returns the substring of length \a n starting at position
    size() - \a n in this object.

    The entire string view is returned if \a n is greater than or equal
    to size(), or less than zero.

    \sa first(), last(), sliced(), chopped(), chop(), truncate(), slice(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView::first(qsizetype n) const
    \since 6.5

    Returns a string view that contains the first \a n code points
    of this string view.

    \note The behavior is undefined when \a n < 0 or \a n > size().

    \sa last(), sliced(), chopped(), chop(), truncate(), slice(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView::last(qsizetype n) const
    \since 6.5

    Returns a string view that contains the last \a n code points of this string view.

    \note The behavior is undefined when \a n < 0 or \a n > size().

    \sa first(), sliced(), chopped(), chop(), truncate(), slice(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView::sliced(qsizetype pos, qsizetype n) const
    \since 6.5

    Returns a string view containing \a n code points of this string view,
    starting at position \a pos.

//! [UB-sliced-index-length]
    \note The behavior is undefined when \a pos < 0, \a n < 0,
    or \a pos + \a n > size().
//! [UB-sliced-index-length]

    \sa first(), last(), chopped(), chop(), truncate(), slice(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView::sliced(qsizetype pos) const
    \since 6.5

    Returns a string view starting at position \a pos in this object,
    and extending to its end.

//! [UB-sliced-index-only]
    \note The behavior is undefined when \a pos < 0 or \a pos > size().
//! [UB-sliced-index-only]

    \sa first(), last(), chopped(), chop(), truncate(), slice(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView &QAnyStringView::slice(qsizetype pos, qsizetype n)
    \since 6.8

    Modifies this string view to start at position \a pos, extending for
    \a n code points.

    \include qanystringview.cpp UB-sliced-index-length

    \sa sliced(), first(), last(), chopped(), chop(), truncate(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView &QAnyStringView::slice(qsizetype pos)
    \since 6.8
    \overload

    Modifies this string view to start at position \a pos, extending to
    its end.

    \include qanystringview.cpp UB-sliced-index-only

    \sa sliced(), first(), last(), chopped(), chop(), truncate(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView::chopped(qsizetype n) const
    \since 6.5

    Returns the substring of length size() - \a n starting at the
    beginning of this object.

    Same as \c{first(size() - n)}.

    \note The behavior is undefined when \a n < 0 or \a n > size().

    \sa sliced(), first(), last(), chop(), truncate(), slice(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView::truncate(qsizetype n)
    \since 6.5

    Truncates this string view to \a n code points.

    Same as \c{*this = first(n)}.

    \note The behavior is undefined when \a n < 0 or \a n > size().

    \sa sliced(), first(), last(), chopped(), chop(), {Sizes and Sub-Strings}
*/

/*!
    \fn QAnyStringView::chop(qsizetype n)
    \since 6.5

    Truncates this string view by \a n code points.

    Same as \c{*this = first(size() - n)}.

    \note The behavior is undefined when \a n < 0 or \a n > size().

    \sa sliced(), first(), last(), chopped(), truncate(), slice(), {Sizes and Sub-Strings}
*/

/*! \fn template <typename Visitor> decltype(auto) QAnyStringView::visit(Visitor &&v) const

    Calls \a v with either a QUtf8StringView, QLatin1String, or QStringView, depending
    on the encoding of the string data this string-view references.

    This is how most functions taking QAnyStringView fork off into per-encoding
    functions:

    \code
    void processImpl(QLatin1String s) { ~~~ }
    void processImpl(QUtf8StringView s) { ~~~ }
    void processImpl(QStringView s) { ~~~ }

    void process(QAnyStringView s)
    {
        s.visit([](auto s) { processImpl(s); });
    }
    \endcode

    Here, we're reusing the same name, \c s, for both the QAnyStringView
    object, as well as the lambda's parameter. This is idiomatic code and helps
    track the identity of the objects through visit() calls, for example in more
    complex situations such as

    \code
    bool equal(QAnyStringView lhs, QAnyStringView rhs)
    {
        // assuming operator==(QAnyStringView, QAnyStringView) didn't, yet, exist:
        return lhs.visit([rhs](auto lhs) {
            rhs.visit([lhs](auto rhs) {
                return lhs == rhs;
            });
        });
    }
    \endcode

    visit() requires that all lambda instantiations have the same return type.
    If they differ, you get a compile error, even if there is a common type. To
    fix, you can use explicit return types on the lambda, or cast in the return
    statements:

    \code
    // wrong:
    QAnyStringView firstHalf(QAnyStringView input)
    {
        return input.visit([](auto input) {   // ERROR: lambdas return different types
            return input.sliced(0, input.size() / 2);
        });
    }
    // correct:
    QAnyStringView firstHalf(QAnyStringView input)
    {
        return input.visit([](auto input) -> QAnyStringView { // OK, explicit return type
            return input.sliced(0, input.size() / 2);
        });
    }
    // also correct:
    QAnyStringView firstHalf(QAnyStringView input)
    {
        return input.visit([](auto input) {
            return QAnyStringView(input.sliced(0, input.size() / 2)); // OK, cast to common type
        });
    }
    \endcode
*/

/*!
    \fn QAnyStringView::compare(QAnyStringView lhs, QAnyStringView rhs, Qt::CaseSensitivity cs)

    Compares the string view \a lhs with the string view \a rhs and returns a
    negative integer if \a lhs is less than \a rhs, a positive integer if it is
    greater than \a rhs, and zero if they are equal.

    If \a cs is Qt::CaseSensitive (the default), the comparison is case sensitive;
    otherwise the comparison is case-insensitive.

    \sa operator==(), operator<(), operator>()
*/

/*!
    \fn bool QAnyStringView::operator==(const QAnyStringView &lhs, const QAnyStringView & rhs)
    \fn bool QAnyStringView::operator!=(const QAnyStringView & lhs, const QAnyStringView & rhs)
    \fn bool QAnyStringView::operator<=(const QAnyStringView & lhs, const QAnyStringView & rhs)
    \fn bool QAnyStringView::operator>=(const QAnyStringView & lhs, const QAnyStringView & rhs)
    \fn bool QAnyStringView::operator<(const QAnyStringView & lhs, const QAnyStringView & rhs)
    \fn bool QAnyStringView::operator>(const QAnyStringView & lhs, const QAnyStringView & rhs)

    Operators that compare \a lhs to \a rhs.

    \sa compare()
*/

/*!
    \fn template <typename QStringLike> qToAnyStringViewIgnoringNull(const QStringLike &s);
    \since 6.0
    \internal

    Convert \a s to a QAnyStringView ignoring \c{s.isNull()}.

    Returns a string view that references \a{s}'s data, but is never null.

    This is a faster way to convert a QString or QByteArray to a QAnyStringView,
    if null QStrings or QByteArrays can legitimately be treated as empty ones.

    \sa QString::isNull(), QAnyStringView
*/

/*!
    \fn QAnyStringView::max_size() const
    \since 6.8

    This function is provided for STL compatibility.

    It returns the maximum number of elements that the string view can
    theoretically represent. In practice, the number can be much smaller,
    limited by the amount of memory available to the system.

    \note The returned value is calculated based on the currently used character
    type, so calling this function on two different views may return different
    results.
*/

/*!
    \fn QAnyStringView::operator<<(QDebug d, QAnyStringView s)
    \since 6.7
    \relates QDebug

    Outputs \a s to debug stream \a d.

    If \c{d.quotedString()} is \c true, indicates which encoding the string is
    in. If you just want the string data, use visit() like this:

    \code
    s.visit([&d) (auto s) { d << s; });
    \endcode

    \sa QAnyStringView::visit()
*/
QDebug operator<<(QDebug d, QAnyStringView s)
{
    struct S { const char *prefix, *suffix; };
    const auto affixes = s.visit([](auto s) {
            using View = decltype(s);
            if constexpr (std::is_same_v<View, QLatin1StringView>) {
                return S{"", "_L1"};
            } else if constexpr (std::is_same_v<View, QUtf8StringView>) {
                return S{"u8", ""};
            } else if constexpr (std::is_same_v<View, QStringView>) {
                return S{"u", ""};
            } else {
                static_assert(QtPrivate::type_dependent_false<View>());
            }
        });
    const QDebugStateSaver saver(d);
    d.nospace();
    if (d.quoteStrings())
        d << affixes.prefix;
    s.visit([&d](auto s) { d << s; });
    if (d.quoteStrings())
        d << affixes.suffix;
    return d;
}

/*!
    \fn template <typename...Args> QString QAnyStringView::arg(Args &&...args) const
    \since 6.9

    \include qstringview.cpp qstring-multi-arg

    \sa QString::arg(Args&&...)
*/

/*!
    \fn template <typename Char, size_t Size, QAnyStringView::if_compatible_char<Char>> QAnyStringView QAnyStringView::fromArray(const Char (&string)[Size])

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

QT_END_NAMESPACE
