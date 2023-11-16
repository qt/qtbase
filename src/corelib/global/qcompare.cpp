// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

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
    \class Qt::strong_ordering
    \inmodule QtCore
    \brief Qt::strong_ordering represents a comparison where equivalent values are
    indistinguishable.
    \sa Qt::weak_ordering, Qt::partial_ordering, {Comparison types overview}
    \since 6.7

    A value of type Qt::strong_ordering is typically returned from a three-way
    comparison function. Such a function compares two objects and establishes
    that the two objects are in a strict ordering relationship; that is, the
    function establishes a well-defined total order.

    The possible values of type Qt::strong_ordering are fully represented by the
    following four symbolic constants:

    \list
    \li \l less represents that the left operand is less than the right;
    \li \l equal represents that the left operand is equivalent to the right;
    \li \l equivalent is an alias for \c Equal;
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
    \brief Qt::weak_ordering represents a comparison where equivalent values are
    still distinguishable.
    \sa Qt::strong_ordering, Qt::partial_ordering, {Comparison types overview}
    \since 6.7

    A value of type Qt::weak_ordering is typically returned from a three-way
    comparison function. Such a function compares two objects and establishes
    the order of the elements relative to each other.

    The possible values of type Qt::weak_ordering are fully represented by the
    following three symbolic constants:

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
    \brief Qt::partial_ordering represents the result of a comparison that allows
    for unordered results.
    \sa Qt::strong_ordering, Qt::weak_ordering, {Comparison types overview}
    \since 6.7

    A value of type Qt::partial_ordering is typically returned from a
    three-way comparison function. Such a function compares two
    objects, and it may either establish that the two objects are
    ordered relative to each other, or that they are not ordered. The
    Qt::partial_ordering value returned from the comparison function
    represents one of those possibilities.

    The possible values of type Qt::partial_ordering are, in fact, fully
    represented by the following four symbolic constants:

    \list
    \li \l less represents that the left operand is less than the right;
    \li \l equivalent represents that left operand is equivalent to the right;
    \li \l greater represents that the left operand is greater than the right;
    \li \l unordered represents that the left operand is \e {not ordered} with
    respect to the right operand.
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

    A Qt::partial_ordering value which represents an unordered result will
    always return false when compared against literal 0.
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

    Represents the result of a comparison where the left operand is equivalent
    to the right operand.
*/

/*!
    \variable Qt::partial_ordering::greater

    Represents the result of a comparison where the left operand is greater
    than the right operand.
*/

/*!
    \variable Qt::partial_ordering::unordered

    Represents the result of a comparison where the left operand is not ordered
    with respect to the right operand.
*/

/*!
    \class QPartialOrdering
    \inmodule QtCore
    \brief QPartialOrdering represents the result of a comparison that allows
    for unordered results.
    \sa Qt::strong_ordering, Qt::weak_ordering, {Comparison types overview}
    \since 6.0

    A value of type QPartialOrdering is typically returned from a
    three-way comparison function. Such a function compares two
    objects, and it may either establish that the two objects are
    ordered relative to each other, or that they are not ordered. The
    QPartialOrdering value returned from the comparison function
    represents one of those possibilities.

    The possible values of type QPartialOrdering are, in fact, fully
    represented by the following four symbolic constants:

    \list
    \li \c Less represents that the left operand is less than the right;
    \li \c Equivalent represents that left operand is equivalent to the right;
    \li \c Greater represents that the left operand is greater than the right;
    \li \c Unordered represents that the left operand is \e {not ordered} with
    respect to the right operand.
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

    A QPartialOrdering value which represents an unordered result will
    always return false when compared against literal 0.
*/

/*!
    \fn QPartialOrdering::QPartialOrdering(std::partial_ordering stdorder)

    Constructs a QPartialOrdering object from \a stdorder using the following
    rules:

    \list
    \li std::partial_ordering::less converts to \l Less.
    \li std::partial_ordering::equivalent converts to \l Equivalent.
    \li std::partial_ordering::greater converts to \l Greater.
    \li std::partial_ordering::unordered converts to \l Unordered
    \endlist
*/

/*!
    \fn QPartialOrdering::operator std::partial_ordering() const

    Converts this QPartialOrdering value to a std::partial_ordering object using
    the following rules:

    \list
    \li \l Less converts to std::partial_ordering::less.
    \li \l Equivalent converts to std::partial_ordering::equivalent.
    \li \l Greater converts to std::partial_ordering::greater.
    \li \l Unordered converts to std::partial_ordering::unordered.
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

    \include qcompare.cpp is_eq_table

    These functions are provided for compatibility with \c{std::partial_ordering}.
*/

/*!
    \variable QPartialOrdering::Less

    Represents the result of a comparison where the left operand is less than
    the right operand.
*/

/*!
    \variable QPartialOrdering::Equivalent

    Represents the result of a comparison where the left operand is equivalent
    to the right operand.
*/

/*!
    \variable QPartialOrdering::Greater

    Represents the result of a comparison where the left operand is greater
    than the right operand.
*/

/*!
    \variable QPartialOrdering::Unordered

    Represents the result of a comparison where the left operand is not ordered
    with respect to the right operand.
*/

QT_END_NAMESPACE
