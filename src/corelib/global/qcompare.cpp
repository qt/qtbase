// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcompare.h"

#ifdef __cpp_lib_bit_cast
#include <bit>
#endif

QT_BEGIN_NAMESPACE

#ifdef __cpp_lib_three_way_comparison
#ifdef __cpp_lib_bit_cast
#define CHECK(type, flag) \
    static_assert(std::bit_cast<Qt:: type ## _ordering>(std:: type ## _ordering:: flag) \
                  == Qt:: type ## _ordering :: flag); \
    static_assert(std::bit_cast<std:: type ## _ordering>(Qt:: type ## _ordering:: flag) \
                  == std:: type ## _ordering :: flag) \
    /* end */
CHECK(partial, unordered);
CHECK(partial, less);
CHECK(partial, greater);
CHECK(partial, equivalent);
CHECK(weak, less);
CHECK(weak, greater);
CHECK(weak, equivalent);
CHECK(strong, less);
CHECK(strong, greater);
CHECK(strong, equal);
CHECK(strong, equivalent);
#undef CHECK
#endif // __cpp_lib_bit_cast
#endif //__cpp_lib_three_way_comparison


/*!
    \page comparison-types.html overview
    \title Comparison types overview
    \keyword three-way comparison
    \inmodule QtCore
    \sa Qt::strong_ordering, Qt::weak_ordering, Qt::partial_ordering

    \note Qt's comparison types provide functionality equivalent to their C++20
    standard counterparts. The only reason why they exist is to make the
    functionality available in C++17 builds, too. In a C++20 build, they
    implicitly convert to and from the \c std types, making them fully
    interchangeable. We therefore recommended that you prefer to use the C++
    standard types in your code, if you can use C++20 in your projects already.
    The Qt comparison types will be removed in Qt 7.

    Qt provides several comparison types for a \l
    {https://en.cppreference.com/w/cpp/language/operator_comparison#Three-way_comparison}
    {three-way comparison}, which are comparable against a \e {zero literal}.
    To use these comparison types, you need to include the \c <QtCompare>
    header. These comparison types are categorized based on their \e order,
    which is a mathematical concept used to describe the arrangement or ranking
    of elements. The following categories are provided:

    \table 100 %
    \header
        \li C++ type
        \li Qt type
        \li strict
        \li total
        \li Example
    \row
        \li \l {https://en.cppreference.com/w/cpp/utility/compare/strong_ordering}
        {std::strong_ordering}
        \li Qt::strong_ordering
        \li yes
        \li yes
        \li integral types, case-sensitive strings, QDate, QTime
    \row
        \li \l {https://en.cppreference.com/w/cpp/utility/compare/weak_ordering}
        {std::weak_ordering}
        \li Qt::weak_ordering
        \li no
        \li yes
        \li case-insensitive strings, unordered associative containers, QDateTime
    \row
        \li \l {https://en.cppreference.com/w/cpp/utility/compare/partial_ordering}
        {std::partial_ordering}
        \li Qt::partial_ordering
        \li no
        \li no
        \li floating-point types, QOperatingSystemVersion, QVariant
    \endtable

    The strongest comparison type, Qt::strong_ordering, represents a strict total
    order. It requires that any two elements be comparable in a way where
    equality implies substitutability. In other words, equivalent values
    cannot be distinguished from each other. A practical example would be the
    case-sensitive comparison of two strings. For instance, when comparing the
    values \c "Qt" and \c "Qt" the result would be \l Qt::strong_ordering::equal.
    Both values are indistinguishable and all deterministic operations performed
    on these values would yield identical results.

    Qt::weak_ordering represents a total order. While any two values still need to
    be comparable, equivalent values may be distinguishable. The canonical
    example here would be the case-insensitive comparison of two strings. For
    instance, when comparing the values \c "Qt" and \c "qt" both hold the same
    letters but with different representations. This comparison would
    result in \l Qt::weak_ordering::equivalent, but not actually \c Equal.
    Another example would be QDateTime, which can represent a given instant in
    time in terms of local time or any other time-zone, including UTC. The
    different representations are equivalent, even though their \c time() and
    sometimes \c date() may differ.

    Qt::partial_ordering represents, as the name implies, a partial ordering. It
    allows for the possibility that two values may not be comparable, resulting
    in an \l {Qt::partial_ordering::}{unordered} state. Additionally, equivalent
    values may still be distinguishable. A practical example would be the
    comparison of two floating-point values, comparing with NaN (Not-a-Number)
    would yield an unordered result. Another example is the comparison of two
    QOperatingSystemVersion objects. Comparing versions of two different
    operating systems, such as Android and Windows, would produce an unordered
    result.

    Utilizing these comparison types enhances the expressiveness of defining
    relations. Furthermore, they serve as a fundamental component for
    implementing three-way comparison with C++17.
*/

/*!
    \headerfile <QtCompare>
    \inmodule QtCore
    \title Classes and helpers for defining comparison operators
    \keyword qtcompare

    \brief The <QtCompare> header file defines \c {Qt::*_ordering} types and helper
    macros for defining comparison operators.

    This header introduces the \l Qt::partial_ordering, \l Qt::weak_ordering, and
    \l Qt::strong_ordering types, which are Qt's C++17 backports of
    \c {std::*_ordering} types.

    This header also contains functions for implementing three-way comparison
    in C++17.

    The \c {Qt::compareThreeWay()} function overloads provide three-way
    comparison for built-in C++ types.

    The \l qCompareThreeWay() template serves as a generic three-way comparison
    implementation. It relies on \c {Qt::compareThreeWay()} and free
    \c {compareThreeWay()} functions in its implementation.
*/

/*!
    \class Qt::strong_ordering
    \inmodule QtCore
    \inheaderfile QtCompare
    \brief Qt::strong_ordering represents a comparison where equivalent values are
    indistinguishable.
    \sa Qt::weak_ordering, Qt::partial_ordering, {Comparison types overview}
    \since 6.7

    A value of type Qt::strong_ordering is typically returned from a three-way
    comparison function. Such a function compares two objects and establishes
    how they are ordered. It uses this return type to indicate that the ordering
    is strict; that is, the function establishes a well-defined total order.

    Qt::strong_ordering has four values, represented by the following symbolic
    constants:

    \list
    \li \l less represents that the left operand is less than the right;
    \li \l equal represents that the left operand is equivalent to the right;
    \li \l equivalent is an alias for \c equal;
    \li \l greater represents that the left operand is greater than the right.
    \endlist

    Qt::strong_ordering is idiomatically used by comparing an instance against a
    literal zero, for instance like this:

    \code

    // given a, b, c, d as objects of some type that allows for a 3-way compare,
    // and a compare function declared as follows:

    Qt::strong_ordering compare(T lhs, T rhs); // defined out-of-line
    ~~~

    Qt::strong_ordering result = compare(a, b);
    if (result < 0) {
        // a is less than b
    }

    if (compare(c, d) >= 0) {
        // c is greater than or equal to d
    }

    \endcode
*/

/*!
    \fn Qt::strong_ordering::operator Qt::partial_ordering() const

    Converts this Qt::strong_ordering value to a Qt::partial_ordering object using the
    following rules:

    \list
    \li \l less converts to \l {Qt::partial_ordering::less}.
    \li \l equivalent converts to \l {Qt::partial_ordering::equivalent}.
    \li \l equal converts to \l {Qt::partial_ordering::equivalent}.
    \li \l greater converts to \l {Qt::partial_ordering::greater}.
    \endlist
*/

/*!
    \fn Qt::strong_ordering::operator Qt::weak_ordering() const

    Converts this Qt::strong_ordering value to a Qt::weak_ordering object using the
    following rules:

    \list
    \li \l less converts to \l {Qt::weak_ordering::less}.
    \li \l equivalent converts to \l {Qt::weak_ordering::equivalent}.
    \li \l equal converts to \l {Qt::weak_ordering::equivalent}.
    \li \l greater converts to \l {Qt::weak_ordering::greater}.
    \endlist
*/

/*!
    \fn Qt::strong_ordering::strong_ordering(std::strong_ordering stdorder)

    Constructs a Qt::strong_ordering object from \a stdorder using the following rules:

    \list
    \li std::strong_ordering::less converts to \l less.
    \li std::strong_ordering::equivalent converts to \l equivalent.
    \li std::strong_ordering::equal converts to \l equal.
    \li std::strong_ordering::greater converts to \l greater.
    \endlist
*/

/*!
    \fn Qt::strong_ordering::operator std::strong_ordering() const

    Converts this Qt::strong_ordering value to a std::strong_ordering object using
    the following rules:

    \list
    \li \l less converts to std::strong_ordering::less.
    \li \l equivalent converts to std::strong_ordering::equivalent.
    \li \l equal converts to std::strong_ordering::equal.
    \li \l greater converts to std::strong_ordering::greater.
    \endlist
*/

/*!
    \fn bool Qt::strong_ordering::operator==(Qt::strong_ordering lhs, Qt::strong_ordering rhs)

    Returns true if \a lhs and \a rhs represent the same result;
    otherwise, returns false.
*/

/*!
    \fn bool Qt::strong_ordering::operator!=(Qt::strong_ordering lhs, Qt::strong_ordering rhs)

    Returns true if \a lhs and \a rhs represent different results;
    otherwise, returns true.
*/

/*!
    \internal
    \relates Qt::strong_ordering
    \fn bool operator==(Qt::strong_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator!=(Qt::strong_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator< (Qt::strong_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator<=(Qt::strong_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator> (Qt::strong_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator>=(Qt::strong_ordering lhs, QtPrivate::CompareAgainstLiteralZero)

    \fn bool operator==(QtPrivate::CompareAgainstLiteralZero, Qt::strong_ordering rhs)
    \fn bool operator!=(QtPrivate::CompareAgainstLiteralZero, Qt::strong_ordering rhs)
    \fn bool operator< (QtPrivate::CompareAgainstLiteralZero, Qt::strong_ordering rhs)
    \fn bool operator<=(QtPrivate::CompareAgainstLiteralZero, Qt::strong_ordering rhs)
    \fn bool operator> (QtPrivate::CompareAgainstLiteralZero, Qt::strong_ordering rhs)
    \fn bool operator>=(QtPrivate::CompareAgainstLiteralZero, Qt::strong_ordering rhs)
*/

/*!
    \fn Qt::strong_ordering::is_eq  (Qt::strong_ordering o)
    \fn Qt::strong_ordering::is_neq (Qt::strong_ordering o)
    \fn Qt::strong_ordering::is_lt  (Qt::strong_ordering o)
    \fn Qt::strong_ordering::is_lteq(Qt::strong_ordering o)
    \fn Qt::strong_ordering::is_gt  (Qt::strong_ordering o)
    \fn Qt::strong_ordering::is_gteq(Qt::strong_ordering o)

//! [is_eq_table]
    Converts \a o into the result of one of the six relational operators:
    \table
    \header \li Function    \li Operation
    \row    \li \c{is_eq}   \li \a o \c{== 0}
    \row    \li \c{is_neq}  \li \a o \c{!= 0}
    \row    \li \c{is_lt}   \li \a o \c{< 0}
    \row    \li \c{is_lteq} \li \a o \c{<= 0}
    \row    \li \c{is_gt}   \li \a o \c{> 0}
    \row    \li \c{is_gteq} \li \a o \c{>= 0}
    \endtable
//! [is_eq_table]

    These functions are provided for compatibility with \c{std::strong_ordering}.
*/

/*!
    \variable Qt::strong_ordering::less

    Represents the result of a comparison where the left operand is less
    than the right operand.
*/

/*!
    \variable Qt::strong_ordering::equivalent

    Represents the result of a comparison where the left operand is equal
    to the right operand. Same as \l {Qt::strong_ordering::equal}.
*/

/*!
    \variable Qt::strong_ordering::equal

    Represents the result of a comparison where the left operand is equal
    to the right operand. Same as \l {Qt::strong_ordering::equivalent}.
*/

/*!
    \variable Qt::strong_ordering::greater

    Represents the result of a comparison where the left operand is greater
    than the right operand.
*/

/*!
    \class Qt::weak_ordering
    \inmodule QtCore
    \inheaderfile QtCompare
    \brief Qt::weak_ordering represents a comparison where equivalent values are
    still distinguishable.
    \sa Qt::strong_ordering, Qt::partial_ordering, {Comparison types overview}
    \since 6.7

    A value of type Qt::weak_ordering is typically returned from a three-way
    comparison function. Such a function compares two objects and establishes
    how they are ordered. It uses this return type to indicate that the ordering
    is weak; that is, equivalent values may be distinguishable.

    Qt::weak_ordering has three values, represented by the following symbolic
    constants:

    \list
    \li \l less represents that the left operand is less than the right;
    \li \l equivalent represents that the left operand is equivalent to the
    right;
    \li \l greater represents that the left operand is greater than the right,
    \endlist

    Qt::weak_ordering is idiomatically used by comparing an instance against a
    literal zero, for instance like this:

    \code

    // given a, b, c, d as objects of some type that allows for a 3-way compare,
    // and a compare function declared as follows:

    Qt::weak_ordering compare(T lhs, T rhs); // defined out-of-line
    ~~~

    Qt::weak_ordering result = compare(a, b);
    if (result < 0) {
        // a is less than b
    }

    if (compare(c, d) >= 0) {
        // c is greater than or equivalent to d
    }

    \endcode
*/

/*!
    \fn Qt::weak_ordering::operator Qt::partial_ordering() const

    Converts this Qt::weak_ordering value to a Qt::partial_ordering object using the
    following rules:

    \list
    \li \l less converts to \l {Qt::partial_ordering::less}.
    \li \l equivalent converts to \l {Qt::partial_ordering::equivalent}.
    \li \l greater converts to \l {Qt::partial_ordering::greater}.
    \endlist
*/

/*!
    \fn Qt::weak_ordering::weak_ordering(std::weak_ordering stdorder)

    Constructs a Qt::weak_ordering object from \a stdorder using the following rules:

    \list
    \li std::weak_ordering::less converts to \l less.
    \li std::weak_ordering::equivalent converts to \l equivalent.
    \li std::weak_ordering::greater converts to \l greater.
    \endlist
*/

/*!
    \fn Qt::weak_ordering::operator std::weak_ordering() const

    Converts this Qt::weak_ordering value to a std::weak_ordering object using
    the following rules:

    \list
    \li \l less converts to std::weak_ordering::less.
    \li \l equivalent converts to std::weak_ordering::equivalent.
    \li \l greater converts to std::weak_ordering::greater.
    \endlist
*/

/*!
    \fn bool Qt::weak_ordering::operator==(Qt::weak_ordering lhs, Qt::weak_ordering rhs)

    Return true if \a lhs and \a rhs represent the same result;
    otherwise, returns false.
*/

/*!
    \fn bool Qt::weak_ordering::operator!=(Qt::weak_ordering lhs, Qt::weak_ordering rhs)

    Return true if \a lhs and \a rhs represent different results;
    otherwise, returns true.
*/

/*!
    \internal
    \relates Qt::weak_ordering
    \fn bool operator==(Qt::weak_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator!=(Qt::weak_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator< (Qt::weak_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator<=(Qt::weak_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator> (Qt::weak_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator>=(Qt::weak_ordering lhs, QtPrivate::CompareAgainstLiteralZero)

    \fn bool operator==(QtPrivate::CompareAgainstLiteralZero, Qt::weak_ordering rhs)
    \fn bool operator!=(QtPrivate::CompareAgainstLiteralZero, Qt::weak_ordering rhs)
    \fn bool operator< (QtPrivate::CompareAgainstLiteralZero, Qt::weak_ordering rhs)
    \fn bool operator<=(QtPrivate::CompareAgainstLiteralZero, Qt::weak_ordering rhs)
    \fn bool operator> (QtPrivate::CompareAgainstLiteralZero, Qt::weak_ordering rhs)
    \fn bool operator>=(QtPrivate::CompareAgainstLiteralZero, Qt::weak_ordering rhs)
*/

/*!
    \fn Qt::weak_ordering::is_eq  (Qt::weak_ordering o)
    \fn Qt::weak_ordering::is_neq (Qt::weak_ordering o)
    \fn Qt::weak_ordering::is_lt  (Qt::weak_ordering o)
    \fn Qt::weak_ordering::is_lteq(Qt::weak_ordering o)
    \fn Qt::weak_ordering::is_gt  (Qt::weak_ordering o)
    \fn Qt::weak_ordering::is_gteq(Qt::weak_ordering o)

    \include qcompare.cpp is_eq_table

    These functions are provided for compatibility with \c{std::weak_ordering}.
*/

/*!
    \variable Qt::weak_ordering::less

    Represents the result of a comparison where the left operand is less than
    the right operand.
*/

/*!
    \variable Qt::weak_ordering::equivalent

    Represents the result of a comparison where the left operand is equivalent
    to the right operand.
*/

/*!
    \variable Qt::weak_ordering::greater

    Represents the result of a comparison where the left operand is greater
    than the right operand.
*/

/*!
    \class Qt::partial_ordering
    \inmodule QtCore
    \inheaderfile QtCompare
    \brief Qt::partial_ordering represents the result of a comparison that allows
    for unordered results.
    \sa Qt::strong_ordering, Qt::weak_ordering, {Comparison types overview}
    \since 6.7

    A value of type Qt::partial_ordering is typically returned from a
    three-way comparison function. Such a function compares two objects,
    establishing whether they are ordered and, if so, their ordering. It uses
    this return type to indicate that the ordering is partial; that is, not all
    pairs of values are ordered.

    Qt::partial_ordering has four values, represented by the following symbolic
    constants:

    \list
    \li \l less represents that the left operand is less than the right;
    \li \l equivalent represents that the two operands are equivalent;
    \li \l greater represents that the left operand is greater than the right;
    \li \l unordered represents that the two operands are \e {not ordered}.
    \endlist

    Qt::partial_ordering is idiomatically used by comparing an instance
    against a literal zero, for instance like this:

    \code

    // given a, b, c, d as objects of some type that allows for a 3-way compare,
    // and a compare function declared as follows:

    Qt::partial_ordering compare(T lhs, T rhs); // defined out-of-line
    ~~~

    Qt::partial_ordering result = compare(a, b);
    if (result < 0) {
        // a is less than b
    }

    if (compare(c, d) >= 0) {
        // c is greater than or equal to d
    }

    \endcode

    Comparing Qt::partial_ordering::unordered against literal 0 always returns
    a \c false result.
*/

/*!
    \fn Qt::partial_ordering::partial_ordering(std::partial_ordering stdorder)

    Constructs a Qt::partial_ordering object from \a stdorder using the following
    rules:

    \list
    \li std::partial_ordering::less converts to \l less.
    \li std::partial_ordering::equivalent converts to \l equivalent.
    \li std::partial_ordering::greater converts to \l greater.
    \li std::partial_ordering::unordered converts to \l unordered
    \endlist
*/

/*!
    \fn Qt::partial_ordering::operator std::partial_ordering() const

    Converts this Qt::partial_ordering value to a std::partial_ordering object using
    the following rules:

    \list
    \li \l less converts to std::partial_ordering::less.
    \li \l equivalent converts to std::partial_ordering::equivalent.
    \li \l greater converts to std::partial_ordering::greater.
    \li \l unordered converts to std::partial_ordering::unordered.
    \endlist
*/

/*!
    \fn bool Qt::partial_ordering::operator==(Qt::partial_ordering lhs, Qt::partial_ordering rhs)

    Return true if \a lhs and \a rhs represent the same result;
    otherwise, returns false.
*/

/*!
    \fn bool Qt::partial_ordering::operator!=(Qt::partial_ordering lhs, Qt::partial_ordering rhs)

    Return true if \a lhs and \a rhs represent different results;
    otherwise, returns true.
*/

/*!
    \internal
    \relates Qt::partial_ordering
    \fn bool operator==(Qt::partial_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator!=(Qt::partial_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator< (Qt::partial_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator<=(Qt::partial_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator> (Qt::partial_ordering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator>=(Qt::partial_ordering lhs, QtPrivate::CompareAgainstLiteralZero)

    \fn bool operator==(QtPrivate::CompareAgainstLiteralZero, Qt::partial_ordering rhs)
    \fn bool operator!=(QtPrivate::CompareAgainstLiteralZero, Qt::partial_ordering rhs)
    \fn bool operator< (QtPrivate::CompareAgainstLiteralZero, Qt::partial_ordering rhs)
    \fn bool operator<=(QtPrivate::CompareAgainstLiteralZero, Qt::partial_ordering rhs)
    \fn bool operator> (QtPrivate::CompareAgainstLiteralZero, Qt::partial_ordering rhs)
    \fn bool operator>=(QtPrivate::CompareAgainstLiteralZero, Qt::partial_ordering rhs)
*/

/*!
    \fn Qt::partial_ordering::is_eq  (Qt::partial_ordering o)
    \fn Qt::partial_ordering::is_neq (Qt::partial_ordering o)
    \fn Qt::partial_ordering::is_lt  (Qt::partial_ordering o)
    \fn Qt::partial_ordering::is_lteq(Qt::partial_ordering o)
    \fn Qt::partial_ordering::is_gt  (Qt::partial_ordering o)
    \fn Qt::partial_ordering::is_gteq(Qt::partial_ordering o)

    \include qcompare.cpp is_eq_table

    These functions are provided for compatibility with \c{std::partial_ordering}.
*/

/*!
    \variable Qt::partial_ordering::less

    Represents the result of a comparison where the left operand is less than
    the right operand.
*/

/*!
    \variable Qt::partial_ordering::equivalent

    Represents the result of a comparison where the two operands are equivalent.
*/

/*!
    \variable Qt::partial_ordering::greater

    Represents the result of a comparison where the left operand is greater
    than the right operand.
*/

/*!
    \variable Qt::partial_ordering::unordered

    Represents the result of a comparison where there is no ordering
    relationship between the two operands.
*/

/*!
    \class QPartialOrdering
    \inmodule QtCore
    \brief QPartialOrdering represents the result of a comparison that allows
    for unordered results.
    \sa Qt::strong_ordering, Qt::weak_ordering, {Comparison types overview}
    \since 6.0

    A value of type QPartialOrdering is typically returned from a
    three-way comparison function. Such a function compares two objects,
    establishing whether they are ordered and, if so, their ordering. It uses
    this return type to indicate that the ordering is partial; that is, not all
    pairs of values are ordered.

    QPartialOrdering has four values, represented by the following symbolic
    constants:

    \list
    \li \l less represents that the left operand is less than the right;
    \li \l equivalent represents that the two operands are equivalent;
    \li \l greater represents that the left operand is greater than the right;
    \li \l unordered represents that the two operands are \e {not ordered}.
    \endlist

    QPartialOrdering is idiomatically used by comparing an instance
    against a literal zero, for instance like this:

    \code

    // given a, b, c, d as objects of some type that allows for a 3-way compare,
    // and a compare function declared as follows:

    QPartialOrdering compare(T lhs, T rhs); // defined out-of-line
    ~~~

    QPartialOrdering result = compare(a, b);
    if (result < 0) {
        // a is less than b
    }

    if (compare(c, d) >= 0) {
        // c is greater than or equal to d
    }

    \endcode

    Comparing QPartialOrdering::unordered against literal 0 always returns
    a \c false result.
*/

/*!
    \fn QPartialOrdering::QPartialOrdering(std::partial_ordering stdorder)

    Constructs a QPartialOrdering object from \a stdorder using the following
    rules:

    \list
    \li std::partial_ordering::less converts to \l less.
    \li std::partial_ordering::equivalent converts to \l equivalent.
    \li std::partial_ordering::greater converts to \l greater.
    \li std::partial_ordering::unordered converts to \l unordered
    \endlist
*/

/*!
    \fn QPartialOrdering::operator std::partial_ordering() const

    Converts this QPartialOrdering value to a std::partial_ordering object using
    the following rules:

    \list
    \li \l less converts to std::partial_ordering::less.
    \li \l equivalent converts to std::partial_ordering::equivalent.
    \li \l greater converts to std::partial_ordering::greater.
    \li \l unordered converts to std::partial_ordering::unordered.
    \endlist
*/

/*!
    \fn bool QPartialOrdering::operator==(QPartialOrdering lhs, QPartialOrdering rhs)

    Return true if \a lhs and \a rhs represent the same result;
    otherwise, returns false.
*/

/*!
    \fn bool QPartialOrdering::operator!=(QPartialOrdering lhs, QPartialOrdering rhs)

    Return true if \a lhs and \a rhs represent different results;
    otherwise, returns true.
*/

/*!
    \internal
    \relates QPartialOrdering
    \fn bool operator==(QPartialOrdering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator!=(QPartialOrdering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator< (QPartialOrdering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator<=(QPartialOrdering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator> (QPartialOrdering lhs, QtPrivate::CompareAgainstLiteralZero)
    \fn bool operator>=(QPartialOrdering lhs, QtPrivate::CompareAgainstLiteralZero)

    \fn bool operator==(QtPrivate::CompareAgainstLiteralZero, QPartialOrdering rhs)
    \fn bool operator!=(QtPrivate::CompareAgainstLiteralZero, QPartialOrdering rhs)
    \fn bool operator< (QtPrivate::CompareAgainstLiteralZero, QPartialOrdering rhs)
    \fn bool operator<=(QtPrivate::CompareAgainstLiteralZero, QPartialOrdering rhs)
    \fn bool operator> (QtPrivate::CompareAgainstLiteralZero, QPartialOrdering rhs)
    \fn bool operator>=(QtPrivate::CompareAgainstLiteralZero, QPartialOrdering rhs)
*/

/*!
    \fn QPartialOrdering::is_eq  (QPartialOrdering o)
    \fn QPartialOrdering::is_neq (QPartialOrdering o)
    \fn QPartialOrdering::is_lt  (QPartialOrdering o)
    \fn QPartialOrdering::is_lteq(QPartialOrdering o)
    \fn QPartialOrdering::is_gt  (QPartialOrdering o)
    \fn QPartialOrdering::is_gteq(QPartialOrdering o)

    \since 6.7
    \include qcompare.cpp is_eq_table

    These functions are provided for compatibility with \c{std::partial_ordering}.
*/

/*!
    \variable QPartialOrdering::less

    Represents the result of a comparison where the left operand is less than
    the right operand.
*/

/*!
    \variable QPartialOrdering::equivalent

    Represents the result of a comparison where the two operands are equivalent.
*/

/*!
    \variable QPartialOrdering::greater

    Represents the result of a comparison where the left operand is greater
    than the right operand.
*/

/*!
    \variable QPartialOrdering::unordered

    Represents the result of a comparison where there is no ordering
    relationship between the two operands.
*/

/*!
    \variable QPartialOrdering::Less

    Represents the result of a comparison where the left operand is less than
    the right operand.
*/

/*!
    \variable QPartialOrdering::Equivalent

    Represents the result of a comparison where the two operands are equivalent.
*/

/*!
    \variable QPartialOrdering::Greater

    Represents the result of a comparison where the left operand is greater
    than the right operand.
*/

/*!
    \variable QPartialOrdering::Unordered

    Represents the result of a comparison where there is no ordering
    relationship between the two operands.
*/

/*!
    \internal
    \macro Q_DECLARE_EQUALITY_COMPARABLE(Type)
    \macro Q_DECLARE_EQUALITY_COMPARABLE(LeftType, RightType)
    \macro Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE(Type)
    \macro Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE(LeftType, RightType)
    \since 6.7
    \relates <QtCompare>

    These macros are used to generate \c {operator==()} and \c {operator!=()}.

    In C++17 mode, the mixed-type overloads also generate the reversed
    operators.

    In C++20 mode, only \c {operator==()} is defined. \c {operator!=()},
    as well as the reversed operators for mixed-type comparison, are synthesized
    by the compiler.

    The operators are implemented in terms of a helper function
    \c {comparesEqual()}.
    It's the user's responsibility to declare and define this function.

    Consider the following example of a comparison operators declaration:

    \code
    class MyClass {
        ...
    private:
        friend bool comparesEqual(const MyClass &, const MyClass &) noexcept;
        Q_DECLARE_EQUALITY_COMPARABLE(MyClass)
    };
    \endcode

    When compiled with C++17, the macro will expand into the following code:

    \code
    friend bool operator==(const MyClass &lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses comparesEqual()
    }
    friend bool operator!=(const MyClass &lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses comparesEqual()
    }
    \endcode

    When compiled with C++20, the macro will expand only into \c {operator==()}:

    \code
    friend bool operator==(const MyClass &lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses comparesEqual()
    }
    \endcode

    The \c {*_LITERAL_TYPE} versions of the macros are used to generate
    \c constexpr operators. This means that the helper \c {comparesEqual()}
    function must also be \c constexpr.

    Consider the following example of a mixed-type \c constexpr comparison
    operators declaration:

    \code
    class MyClass {
        ...
    private:
        friend constexpr bool comparesEqual(const MyClass &, int) noexcept;
        Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE(MyClass, int)
    };
    \endcode

    When compiled with C++17, the macro will expand into the following code:

    \code
    friend constexpr bool operator==(const MyClass &lhs, int rhs) noexcept
    {
        // inline implementation which uses comparesEqual()
    }
    friend constexpr bool operator!=(const MyClass &lhs, int rhs) noexcept
    {
        // inline implementation which uses comparesEqual()
    }
    friend constexpr bool operator==(int lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses comparesEqual()
    }
    friend constexpr bool operator!=(int lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses comparesEqual()
    }
    \endcode

    When compiled with C++20, the macro expands only into \c {operator==()}:

    \code
    friend constexpr bool operator==(const MyClass &lhs, int rhs) noexcept
    {
        // inline implementation which uses comparesEqual()
    }
    \endcode

//! [noexcept-requirement-desc]
    These macros generate \c {noexcept} relational operators, and so they check
    that the helper functions are \c {noexcept}.
    Use the \c {_NON_NOEXCEPT} versions of the macros if the relational
    operators of your class cannot be \c {noexcept}.
//! [noexcept-requirement-desc]
*/

/*!
    \internal
    \macro Q_DECLARE_PARTIALLY_ORDERED(Type)
    \macro Q_DECLARE_PARTIALLY_ORDERED(LeftType, RightType)
    \macro Q_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE(Type)
    \macro Q_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE(LeftType, RightType)
    \since 6.7
    \relates <QtCompare>

    These macros are used to generate all six relational operators.
    The operators represent
    \l {https://en.cppreference.com/w/cpp/utility/compare/partial_ordering}
    {partial ordering}.

    These macros use respective overloads of the
    \l {Q_DECLARE_EQUALITY_COMPARABLE} macro to generate \c {operator==()} and
    \c {operator!=()}, and also generate the four relational operators:
    \c {operator<()}, \c {operator>()}, \c {operator<=()}, and \c {operator>()}.

    In C++17 mode, the mixed-type overloads also generate the reversed
    operators.

    In C++20 mode, only \c {operator==()} and \c {operator<=>()} are defined.
    Other operators, as well as the reversed operators for mixed-type
    comparison, are synthesized by the compiler.

    The (in)equality operators are implemented in terms of a helper function
    \c {comparesEqual()}. The other relational operators are implemented in
    terms of a helper function \c {compareThreeWay()}.
    The \c {compareThreeWay()} function \e must return an object of type
    \l Qt::partial_ordering. It's the user's responsibility to declare and define
    both helper functions.

    Consider the following example of a comparison operators declaration:

    \code
    class MyClass {
        ...
    private:
        friend bool comparesEqual(const MyClass &, const MyClass &) noexcept;
        friend Qt::partial_ordering compareThreeWay(const MyClass &, const MyClass &) noexcept;
        Q_DECLARE_PARTIALLY_ORDERED(MyClass)
    };
    \endcode

    When compiled with C++17, the macro will expand into the following code:

    \code
    // operator==() and operator!=() are generated from
    // Q_DECLARE_EQUALITY_COMPARABLE
    friend bool operator<(const MyClass &lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    friend bool operator>(const MyClass &lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    friend bool operator<=(const MyClass &lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    friend bool operator>=(const MyClass &lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    \endcode

    When compiled with C++20, the macro will expand into \c {operator==()} and
    \c {operator<=>()}:

    \code
    friend bool operator==(const MyClass &lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses comparesEqual()
    }
    friend std::partial_ordering
    operator<=>(const MyClass &lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    \endcode

    The \c {*_LITERAL_TYPE} versions of the macros are used to generate
    \c constexpr operators. This means that the helper \c {comparesEqual()} and
    \c {compareThreeWay()} functions must also be \c constexpr.

    Consider the following example of a mixed-type \c constexpr comparison
    operators declaration:

    \code
    class MyClass {
        ...
    private:
        friend constexpr bool comparesEqual(const MyClass &, int) noexcept;
        friend constexpr Qt::partial_ordering compareThreeWay(const MyClass &, int) noexcept;
        Q_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE(MyClass, int)
    };
    \endcode

    When compiled with C++17, the macro will expand into the following code:

    \code
    // operator==(), operator!=(), and their reversed versions are generated
    // from Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE
    friend constexpr bool operator<(const MyClass &lhs, int rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    friend constexpr bool operator>(const MyClass &lhs, int rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    friend constexpr bool operator<=(const MyClass &lhs, int rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    friend constexpr bool operator>=(const MyClass &lhs, int rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    friend constexpr bool operator<(int lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    friend constexpr bool operator>(int lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    friend constexpr bool operator<=(int lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    friend constexpr bool operator>=(int lhs, const MyClass &rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    \endcode

    When compiled with C++20, the macro will expand into \c {operator==()} and
    \c {operator<=>()}:

    \code
    friend constexpr bool operator==(const MyClass &lhs, int rhs) noexcept
    {
        // inline implementation which uses comparesEqual()
    }
    friend constexpr std::partial_ordering
    operator<=>(const MyClass &lhs, int rhs) noexcept
    {
        // inline implementation which uses compareThreeWay()
    }
    \endcode

    \include qcompare.cpp noexcept-requirement-desc

    \sa Q_DECLARE_EQUALITY_COMPARABLE, Q_DECLARE_WEAKLY_ORDERED,
        Q_DECLARE_STRONGLY_ORDERED
*/

/*!
    \internal
    \macro Q_DECLARE_WEAKLY_ORDERED(Type)
    \macro Q_DECLARE_WEAKLY_ORDERED(LeftType, RightType)
    \macro Q_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE(Type)
    \macro Q_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE(LeftType, RightType)
    \since 6.7
    \relates <QtCompare>

    These macros behave similarly to the
    \l {Q_DECLARE_PARTIALLY_ORDERED} overloads, but represent
    \l {https://en.cppreference.com/w/cpp/utility/compare/weak_ordering}
    {weak ordering}.

    The (in)equality operators are implemented in terms of a helper function
    \c {comparesEqual()}. The other relational operators are implemented in
    terms of a helper function \c {compareThreeWay()}.
    The \c {compareThreeWay()} function \e must return an object of type
    \l Qt::weak_ordering. It's the user's responsibility to declare and define both
    helper functions.

    The \c {*_LITERAL_TYPE} overloads are used to generate \c constexpr
    operators. This means that the helper \c {comparesEqual()} and
    \c {compareThreeWay()} functions must also be \c constexpr.

    \include qcompare.cpp noexcept-requirement-desc

    See \l {Q_DECLARE_PARTIALLY_ORDERED} for usage examples.

    \sa Q_DECLARE_PARTIALLY_ORDERED, Q_DECLARE_STRONGLY_ORDERED,
        Q_DECLARE_EQUALITY_COMPARABLE
*/

/*!
    \internal
    \macro Q_DECLARE_STRONGLY_ORDERED(Type)
    \macro Q_DECLARE_STRONGLY_ORDERED(LeftType, RightType)
    \macro Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(Type)
    \macro Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(LeftType, RightType)
    \since 6.7
    \relates <QtCompare>

    These macros behave similarly to the
    \l {Q_DECLARE_PARTIALLY_ORDERED} overloads, but represent
    \l {https://en.cppreference.com/w/cpp/utility/compare/strong_ordering}
    {strong ordering}.

    The (in)equality operators are implemented in terms of a helper function
    \c {comparesEqual()}. The other relational operators are implemented in
    terms of a helper function \c {compareThreeWay()}.
    The \c {compareThreeWay()} function \e must return an object of type
    \l Qt::strong_ordering. It's the user's responsibility to declare and define
    both helper functions.

    The \c {*_LITERAL_TYPE} overloads are used to generate \c constexpr
    operators. This means that the helper \c {comparesEqual()} and
    \c {compareThreeWay()} functions must also be \c constexpr.

    \include qcompare.cpp noexcept-requirement-desc

    See \l {Q_DECLARE_PARTIALLY_ORDERED} for usage examples.

    \sa Q_DECLARE_PARTIALLY_ORDERED, Q_DECLARE_WEAKLY_ORDERED,
        Q_DECLARE_EQUALITY_COMPARABLE
*/

/*!
    \internal
    \macro Q_DECLARE_EQUALITY_COMPARABLE(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_PARTIALLY_ORDERED(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_WEAKLY_ORDERED(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_STRONGLY_ORDERED(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(LeftType, RightType, Attributes...)
    \since 6.8
    \relates <QtCompare>

    These macros behave like their two-argument versions, but allow
    specification of C++ attributes to add before every generated relational
    operator.

    As an example, the \c Attributes parameter can be used in Qt to pass
    the \c QT_ASCII_CAST_WARN marco (whose expansion can mark the function as
    deprecated) when implementing comparison of encoding-aware string types
    with C-style strings or byte arrays.

    Starting from Qt 6.9, \c Attributes becomes a variable argument, meaning
    that you can now specify more complex templates and constraints using
    these macros.

    For example, equality-comparison of a custom type with any integral type
    can be implemented in the following way:

    \code
    class MyClass {
    public:
        ...
    private:
        template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
        friend constexpr bool comparesEqual(const MyClass &lhs, T rhs) noexcept
        { ... }
        Q_DECLARE_EQUALITY_COMPARABLE(MyClass, T,
                                      template <typename T,
                                                std::enable_if_t<std::is_integral_v<T>,
                                                                 bool> = true>)
    };
    \endcode

    \note Bear in mind that a macro treats each comma (unless within
    parentheses) as starting a new argument; for example, the invocation above
    has five arguments. Due to implementation details, the macros cannot have
    more than nine arguments. If the constraint is too complicated, use an alias
    template to give it a self-explanatory name, and use this alias as an
    argument of the macro.

    \include qcompare.cpp noexcept-requirement-desc
*/

/*!
    \internal
    \macro Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(Type)
    \macro Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(LeftType, RightType)
    \macro Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT(Type)
    \macro Q_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT(LeftType, RightType)
    \macro Q_DECLARE_PARTIALLY_ORDERED_NON_NOEXCEPT(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT(Type)
    \macro Q_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT(LeftType, RightType)
    \macro Q_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT(Type)
    \macro Q_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT(LeftType, RightType)
    \macro Q_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT(LeftType, RightType, Attributes...)
    \since 6.8
    \relates <QtCompare>

    These macros behave like their versions without the \c {_NON_NOEXCEPT}
    suffix, but should be used when the relational operators cannot be
    \c {noexcept}.

    Starting from Qt 6.9, \c Attributes becomes a variable argument.
*/

/*!
    \internal
    \macro Q_DECLARE_ORDERED(Type)
    \macro Q_DECLARE_ORDERED(LeftType, RightType)
    \macro Q_DECLARE_ORDERED(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_ORDERED_LITERAL_TYPE(Type)
    \macro Q_DECLARE_ORDERED_LITERAL_TYPE(LeftType, RightType)
    \macro Q_DECLARE_ORDERED_LITERAL_TYPE(LeftType, RightType, Attributes...)
    \macro Q_DECLARE_ORDERED_NON_NOEXCEPT(Type)
    \macro Q_DECLARE_ORDERED_NON_NOEXCEPT(LeftType, RightType)
    \macro Q_DECLARE_ORDERED_NON_NOEXCEPT(LeftType, RightType, Attributes...)
    \since 6.9
    \relates <QtCompare>

    These macros behave similarly to the
    \c {Q_DECLARE_(PARTIALLY,WEAKLY,STRONGLY)_ORDERED} overloads, but represent
    any one of those three, using \c auto return type.

    This is what you typically would use for template classes where
    the strength of the ordering depends on the template arguments.
    For example, if one of the template arguments is a floating-point
    type, the ordering would be \l {Qt::partial_ordering}, if they all
    are integral - \l {Qt::strong_ordering}.

    \note It is better to use one of the explicit-strength macros in general, to
    communicate intent. Use these macros only when the stength actually does vary
    with template arguments.

    The (in)equality operators are implemented in terms of a helper function
    \c {comparesEqual()}. The other relational operators are implemented in
    terms of a helper function \c {compareThreeWay()}.
    The \c {compareThreeWay()} function \e must return an object of an ordering
    type. It's the user's responsibility to declare and define both helper
    functions.

    The \c {*_LITERAL_TYPE} overloads are used to generate \c constexpr
    operators. This means that the helper \c {comparesEqual()} and
    \c {compareThreeWay()} functions must also be \c constexpr.

    See \l {Q_DECLARE_PARTIALLY_ORDERED} for usage examples.

    By default, the generated operators are \c {noexcept}.
    Use the \c {*_NON_NOEXCEPT} overloads if the relational operators cannot be
    \c {noexcept}.

    The three-argument versions of the macros allow specification of C++
    attributes to add before every generated relational operator.
    See \l {Q_DECLARE_EQUALITY_COMPARABLE(LeftType, RightType, Attributes...)}
    for more details and usage examples.

    \sa Q_DECLARE_PARTIALLY_ORDERED, Q_DECLARE_WEAKLY_ORDERED,
        Q_DECLARE_STRONGLY_ORDERED, Q_DECLARE_EQUALITY_COMPARABLE
*/

/*!
    \fn template <typename LeftInt, typename RightInt, Qt::if_integral<LeftInt> = true, Qt::if_integral<RightInt> = true> auto Qt::compareThreeWay(LeftInt lhs, RightInt rhs)
    \since 6.7
    \relates <QtCompare>
    \overload

    Implements three-way comparison of integral types.

    Returns \c {lhs <=> rhs}, provided \c LeftInt and \c RightInt are built-in
    integral types. Unlike \c {operator<=>()}, this function template is also
    available in C++17. See
    \l {https://en.cppreference.com/w/cpp/language/operator_comparison#Three-way_comparison}
    {cppreference} for more details.

    This function can also be used in custom \c {compareThreeWay()} functions,
    when ordering members of a custom class represented by built-in types:

    \code
    class MyClass {
    public:
        ...
    private:
        int value;
        ...
        friend Qt::strong_ordering
        compareThreeWay(const MyClass &lhs, const MyClass &rhs) noexcept
        { return Qt::compareThreeWay(lhs.value, rhs.value); }
        Q_DECLARE_STRONGLY_ORDERED(MyClass)
    };
    \endcode

    Returns an instance of \l Qt::strong_ordering that represents the relation
    between \a lhs and \a rhs.

    \constraints both
    \c LeftInt and \c RightInt are built-in integral types.
*/

/*!
    \fn template <typename LeftFloat, typename RightFloat, Qt::if_floating_point<LeftFloat> = true, Qt::if_floating_point<RightFloat> = true> auto Qt::compareThreeWay(LeftFloat lhs, RightFloat rhs)
    \since 6.7
    \relates <QtCompare>
    \overload

    Implements three-way comparison of floating point types.

    Returns \c {lhs <=> rhs}, provided \c LeftFloat and \c RightFloat are
    built-in floating-point types. Unlike \c {operator<=>()}, this function
    template is also available in C++17. See
    \l {https://en.cppreference.com/w/cpp/language/operator_comparison#Three-way_comparison}
    {cppreference} for more details.

    This function can also be used in custom \c {compareThreeWay()} functions,
    when ordering members of a custom class represented by built-in types:

    \code
    class MyClass {
    public:
        ...
    private:
        double value;
        ...
        friend Qt::partial_ordering
        compareThreeWay(const MyClass &lhs, const MyClass &rhs) noexcept
        { return Qt::compareThreeWay(lhs.value, rhs.value); }
        Q_DECLARE_PARTIALLY_ORDERED(MyClass)
    };
    \endcode

    Returns an instance of \l Qt::partial_ordering that represents the relation
    between \a lhs and \a rhs. If \a lhs or \a rhs is not a number (NaN),
    \l Qt::partial_ordering::unordered is returned.

    \constraints both
    \c LeftFloat and \c RightFloat are built-in floating-point types.
*/

/*!
    \fn template <typename IntType, typename FloatType, Qt::if_integral<IntType> = true, Qt::if_floating_point<FloatType> = true> auto Qt::compareThreeWay(IntType lhs, FloatType rhs)
    \since 6.7
    \relates <QtCompare>
    \overload

    Implements three-way comparison of integral and floating point types.

    This function converts \a lhs to \c FloatType and calls the overload for
    floating-point types.

    Returns an instance of \l Qt::partial_ordering that represents the relation
    between \a lhs and \a rhs. If \a rhs is not a number (NaN),
    \l Qt::partial_ordering::unordered is returned.

    \constraints \c IntType
    is a built-in integral type and \c FloatType is a built-in floating-point
    type.
*/

/*!
    \fn template <typename FloatType, typename IntType, Qt::if_floating_point<FloatType> = true, Qt::if_integral<IntType> = true> auto Qt::compareThreeWay(FloatType lhs, IntType rhs)
    \since 6.7
    \relates <QtCompare>
    \overload

    Implements three-way comparison of floating point and integral types.

    This function converts \a rhs to \c FloatType and calls the overload for
    floating-point types.

    Returns an instance of \l Qt::partial_ordering that represents the relation
    between \a lhs and \a rhs. If \a lhs is not a number (NaN),
    \l Qt::partial_ordering::unordered is returned.

    \constraints \c FloatType
    is a built-in floating-point type and \c IntType is a built-in integral
    type.
*/

#if QT_DEPRECATED_SINCE(6, 8)
/*!
    \fn template <typename LeftType, typename RightType, Qt::if_compatible_pointers<LeftType, RightType> = true> Qt::compareThreeWay(const LeftType *lhs, const RightType *rhs)
    \since 6.7
    \deprecated [6.8] Wrap the pointers into Qt::totally_ordered_wrapper and
    use the respective Qt::compareThreeWay() overload instead.
    \relates <QtCompare>
    \overload

    Implements three-way comparison of pointers.

    Returns an instance of \l Qt::strong_ordering that represents the relation
    between \a lhs and \a rhs.

    \constraints \c LeftType and
    \c RightType are the same type, or base and derived types. It is also used
    to compare any pointer to \c {std::nullptr_t}.
*/
#endif // QT_DEPRECATED_SINCE(6, 8)

/*!
    \fn template <class Enum, Qt::if_enum<Enum> = true> Qt::compareThreeWay(Enum lhs, Enum rhs)
    \since 6.7
    \relates <QtCompare>
    \overload

    Implements three-way comparison of enum types.

    This function converts \c Enum to its underlying type and calls the
    overload for integral types.

    Returns an instance of \l Qt::strong_ordering that represents the relation
    between \a lhs and \a rhs.

    \constraints \c Enum is an enum type.
*/

/*!
    \fn template <typename T, typename U, Qt::if_compatible_pointers<T, U> = true> Qt::compareThreeWay(Qt::totally_ordered_wrapper<T*> lhs, Qt::totally_ordered_wrapper<U*> rhs)
    \since 6.8
    \relates <QtCompare>
    \overload

    Implements three-way comparison of pointers that are wrapped into
    \l Qt::totally_ordered_wrapper. Uses
    \l {https://en.cppreference.com/w/cpp/language/operator_comparison#Pointer_total_order}
    {strict total order over pointers} when doing the comparison.

    Returns an instance of \l Qt::strong_ordering that represents the relation
    between \a lhs and \a rhs.

    \constraints \c T and \c U are the same type, or base and derived types.
*/

/*!
    \fn template <typename T, typename U, Qt::if_compatible_pointers<T, U> = true> Qt::compareThreeWay(Qt::totally_ordered_wrapper<T*> lhs, U *rhs)
    \since 6.8
    \relates <QtCompare>
    \overload

    Implements three-way comparison of a pointer wrapped into
    \l Qt::totally_ordered_wrapper with a normal pointer. Uses
    \l {https://en.cppreference.com/w/cpp/language/operator_comparison#Pointer_total_order}
    {strict total order over pointers} when doing the comparison.

    Returns an instance of \l Qt::strong_ordering that represents the relation
    between \a lhs and \a rhs.

    \constraints \c T and \c U are the same type, or base and derived types.
*/

/*!
    \fn template <typename T, typename U, Qt::if_compatible_pointers<T, U> = true> Qt::compareThreeWay(U *lhs, Qt::totally_ordered_wrapper<T*> rhs)
    \since 6.8
    \relates <QtCompare>
    \overload

    Implements three-way comparison of a normal pointer with a pointer wrapped
    into \l Qt::totally_ordered_wrapper. Uses
    \l {https://en.cppreference.com/w/cpp/language/operator_comparison#Pointer_total_order}
    {strict total order over pointers} when doing the comparison.

    Returns an instance of \l Qt::strong_ordering that represents the relation
    between \a lhs and \a rhs.

    \constraints \c T and \c U are the same type, or base and derived types.
*/

/*!
    \fn template <typename T> Qt::compareThreeWay(Qt::totally_ordered_wrapper<T*> lhs, std::nullptr_t rhs)
    \since 6.8
    \relates <QtCompare>
    \overload

    Implements three-way comparison of a pointer wrapped into
    \l Qt::totally_ordered_wrapper with \c {std::nullptr_t}.

    Returns an instance of \l Qt::strong_ordering that represents the relation
    between \a lhs and \a rhs.
*/

/*!
    \fn template <typename T> Qt::compareThreeWay(std::nullptr_t lhs, Qt::totally_ordered_wrapper<T*> rhs)
    \since 6.8
    \relates <QtCompare>
    \overload

    Implements three-way comparison of \c {std::nullptr_t} with a pointer
    wrapped into \l Qt::totally_ordered_wrapper.

    Returns an instance of \l Qt::strong_ordering that represents the relation
    between \a lhs and \a rhs.
*/

/*!
    \fn template <typename LeftType, typename RightType> qCompareThreeWay(const LeftType &lhs, const RightType &rhs)
    \since 6.7
    \relates <QtCompare>

    Performs the three-way comparison on \a lhs and \a rhs and returns one of
    the Qt ordering types as a result. This function is available for both
    C++17 and C++20.

    The actual returned type depends on \c LeftType and \c RightType.

    \note This function template is only available when \c {compareThreeWay()}
    is implemented for the \c {(LeftType, RightType)} pair or the reversed
    \c {(RightType, LeftType)} pair.

    This method is equivalent to

    \code
    using Qt::compareThreeWay;
    return compareThreeWay(lhs, rhs);
    \endcode

    where \c {Qt::compareThreeWay} is the Qt implementation of three-way
    comparison for built-in types.

    The free \c {compareThreeWay} functions should provide three-way comparison
    for custom types. The functions should return one of the Qt ordering types.

    Qt provides \c {compareThreeWay} implementation for some of its types.

    \note \b {Do not} re-implement \c {compareThreeWay()} for Qt types, as more
    Qt types will get support for it in future Qt releases.

    Use this function primarly in generic code, when you know nothing about
    \c LeftType and \c RightType.

    If you know the types, use

    \list
    \li \c {Qt::compareThreeWay} for built-in types
    \li \c {compareThreeWay} for custom types
    \endlist

    Use \c {operator<=>()} directly in code that will only be compiled with
    C++20 or later.

    \sa Qt::partial_ordering, Qt::weak_ordering, Qt::strong_ordering
*/

/*!
    \fn template <typename InputIt1, typename InputIt2> QtOrderingPrivate::lexicographicalCompareThreeWay(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
    \internal
    \relates <QtCompare>

    \brief Three-way lexicographic comparison of ranges.

    Checks how the range [ \a first1, \a last1 ) compares to the second range
    [ \a first2, \a last2 ) and produces a result of the strongest applicable
    category type.

    This function can only be used if \c InputIt1::value_type and
    \c InputIt2::value_type types provide a \c {compareThreeWay()} helper method
    that returns one of the Qt ordering types.

    \sa {Comparison types overview}
*/

/*!
    \fn template <typename InputIt1, typename InputIt2, typename Comparator> QtOrderingPrivate::lexicographicalCompareThreeWay(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Comparator cmp)
    \internal
    \relates <QtCompare>
    \overload

    This overload takes a custom \c Comparator that is used to do the comparison.
    The comparator should have the following signature:

    \badcode
    OrderingType cmp(const InputIt1::value_type &lhs, const InputIt2::value_type &rhs);
    \endcode

    where \c OrderingType is one of the Qt ordering types.

    \sa {Comparison types overview}
*/

/*!
    \class Qt::totally_ordered_wrapper
    \inmodule QtCore
    \inheaderfile QtCompare
    \brief Qt::totally_ordered_wrapper is a wrapper type that provides strict
    total order for the wrapped types.
    \since 6.8

    One of its primary usecases is to prevent \e {Undefined Behavior} (UB) when
    comparing pointers.

    Consider the following simple class:

    \code
    template <typename T>
    struct PointerWrapperBad {
        int val;
        T *ptr;
    };
    \endcode

    Lexicographical comparison of the two instances of the \c PointerWrapperBad
    type would result in UB, because it will call \c {operator<()} or
    \c {operator<=>()} on the \c {ptr} members.

    To fix it, use the new wrapper type:

    \code
    template <typename T>
    struct PointerWrapperGood {
        int val;
        Qt::totally_ordered_wrapper<T *> ptr;

        friend bool
        operator==(PointerWrapperGood lhs, PointerWrapperGood rhs) noexcept = default;
        friend auto
        operator<=>(PointerWrapperGood lhs, PointerWrapperGood rhs) noexecpt = default;
    };
    \endcode

    The \c {operator<()} and (if available) \c {operator<=>()} operators for
    the \c {Qt::totally_ordered_wrapper} type use the
    \l {https://en.cppreference.com/w/cpp/utility/functional/less}{std::less}
    and \l {https://en.cppreference.com/w/cpp/utility/compare/compare_three_way}
    {std::compare_three_way} function objects respectively, providing
    \l {https://en.cppreference.com/w/cpp/language/operator_comparison#Pointer_total_order}
    {strict total order over pointers} when doing the comparison.

    As a result, the relational operators for \c {PointerWrapperGood::ptr}
    member will be well-defined, and we can even \c {=default} the relational
    operators for the \c {PointerWrapperGood} class, like it's shown above.
*/

QT_END_NAMESPACE
