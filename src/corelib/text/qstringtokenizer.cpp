// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstringtokenizer.h"
#include "qstringalgorithms.h"

QT_BEGIN_NAMESPACE

/*!
    \class QStringTokenizer
    \inmodule QtCore
    \since 6.0
    \brief The QStringTokenizer class splits strings into tokens along given separators.
    \reentrant
    \ingroup tools
    \ingroup string-processing

    Splits a string into substrings wherever a given separator occurs,
    returning a (lazily constructed) list of those strings. If the separator does
    not match anywhere in the string, produces a single-element list
    containing this string.  If the separator is empty,
    QStringTokenizer produces an empty string, followed by each of the
    string's characters, followed by another empty string. The two
    enumerations Qt::SplitBehavior and Qt::CaseSensitivity further
    control the output.

    QStringTokenizer drives QStringView::tokenize(), but, at least with a
    recent compiler, you can use it directly, too:

    \code
    for (auto it : QStringTokenizer{string, separator})
        use(*it);
    \endcode

    \note You should never, ever, name the template arguments of a
    QStringTokenizer explicitly.  If you can use C++17 Class Template
    Argument Deduction (CTAD), you may write
    \c{QStringTokenizer{string, separator}} (without template
    arguments).  If you can't use C++17 CTAD, you must use the
    QStringView::split() or QLatin1StringView::split() member functions
    and store the return value only in \c{auto} variables:

    \code
    auto result = string.split(sep);
    \endcode

    This is because the template arguments of QStringTokenizer have a
    very subtle dependency on the specific string and separator types
    from with which they are constructed, and they don't usually
    correspond to the actual types passed.

    \section1 Lazy Sequences

    QStringTokenizer acts as a so-called lazy sequence, that is, each
    next element is only computed once you ask for it. Lazy sequences
    have the advantage that they only require O(1) memory. They have
    the disadvantage that, at least for QStringTokenizer, they only
    allow forward, not random-access, iteration.

    The intended use-case is that you just plug it into a ranged for loop:

    \code
    for (auto it : QStringTokenizer{string, separator})
        use(*it);
    \endcode

    or a C++20 ranged algorithm:

    \code
    std::ranges::for_each(QStringTokenizer{string, separator},
                          [] (auto token) { use(token); });
    \endcode

    \section1 End Sentinel

    The QStringTokenizer iterators cannot be used with classical STL
    algorithms, because those require iterator/iterator pairs, while
    QStringTokenizer uses sentinels. That is, it uses a different
    type, QStringTokenizer::sentinel, to mark the end of the
    range. This improves performance, because the sentinel is an empty
    type. Sentinels are supported from C++17 (for ranged for)
    and C++20 (for algorithms using the new ranges library).

    \section1 Temporaries

    QStringTokenizer is very carefully designed to avoid dangling
    references. If you construct a tokenizer from a temporary string
    (an rvalue), that argument is stored internally, so the referenced
    data isn't deleted before it is tokenized:

    \code
    auto tok = QStringTokenizer{widget.text(), u','};
    // return value of `widget.text()` is destroyed, but content was moved into `tok`
    for (auto e : tok)
       use(e);
    \endcode

    If you pass named objects (lvalues), then QStringTokenizer does
    not store a copy. You are responsible to keep the named object's
    data around for longer than the tokenizer operates on it:

    \code
    auto text = widget.text();
    auto tok = QStringTokenizer{text, u','};
    text.clear();      // destroy content of `text`
    for (auto e : tok) // ERROR: `tok` references deleted data!
        use(e);
    \endcode

    \sa QStringView::split(), QString::split(), QRegularExpression
*/

/*!
    \typealias QStringTokenizer::value_type

    Alias for \c{const QStringView} or \c{const QLatin1StringView},
    depending on the tokenizer's \c Haystack template argument.
*/

/*!
    \typealias QStringTokenizer::difference_type

    Alias for qsizetype.
*/

/*!
    \typealias QStringTokenizer::size_type

    Alias for qsizetype.
*/

/*!
    \typealias QStringTokenizer::reference

    Alias for \c{value_type &}.

    QStringTokenizer does not support mutable references, so this is
    the same as const_reference.
*/

/*!
    \typealias QStringTokenizer::const_reference

    Alias for \c{value_type &}.
*/

/*!
    \typealias QStringTokenizer::pointer

    Alias for \c{value_type *}.

    QStringTokenizer does not support mutable iterators, so this is
    the same as const_pointer.
*/

/*!
    \typealias QStringTokenizer::const_pointer

    Alias for \c{value_type *}.
*/

/*!
    \typealias QStringTokenizer::iterator

    This typedef provides an STL-style const iterator for
    QStringTokenizer.

    QStringTokenizer does not support mutable iterators, so this is
    the same as const_iterator.

    \sa const_iterator
*/

/*!
    \typedef QStringTokenizer::const_iterator

    This typedef provides an STL-style const iterator for
    QStringTokenizer.

    \sa iterator
*/

/*!
    \typealias QStringTokenizer::sentinel

    This typedef provides an STL-style sentinel for
    QStringTokenizer::iterator and QStringTokenizer::const_iterator.

    \sa const_iterator
*/

/*!
    \fn template <typename Haystack, typename Needle> QStringTokenizer<Haystack, Needle>::QStringTokenizer(Haystack haystack, Needle needle, Qt::CaseSensitivity cs, Qt::SplitBehavior sb)
    \fn template <typename Haystack, typename Needle> QStringTokenizer<Haystack, Needle>::QStringTokenizer(Haystack haystack, Needle needle, Qt::SplitBehavior sb, Qt::CaseSensitivity cs)

    Constructs a string tokenizer that splits the string \a haystack
    into substrings wherever \a needle occurs, and allows iteration
    over those strings as they are found. If \a needle does not match
    anywhere in \a haystack, a single element containing \a haystack
    is produced.

    \a cs specifies whether \a needle should be matched case
    sensitively or case insensitively.

    If \a sb is Qt::SkipEmptyParts, empty entries don't
    appear in the result. By default, empty entries are included.

    \sa QStringView::split(), QString::split(), Qt::CaseSensitivity, Qt::SplitBehavior
*/

/*!
    \fn template <typename Haystack, typename Needle> QStringTokenizer<Haystack, Needle>::iterator QStringTokenizer<Haystack, Needle>::begin() const
    \fn template <typename Haystack, typename Needle> QStringTokenizer<Haystack, Needle>::iterator QStringTokenizer<Haystack, Needle>::cbegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator}
    pointing to the first token in the list.

    \sa end(), cend()
*/

/*!
    \fn template <typename Haystack, typename Needle> QStringTokenizer<Haystack, Needle>::sentinel QStringTokenizer<Haystack, Needle>::end() const

    Returns a const \l{STL-style iterators}{STL-style sentinel}
    pointing to the imaginary token after the last token in the list.

    \sa begin(), cend()
*/

/*!
    \fn template <typename Haystack, typename Needle> QStringTokenizer<Haystack, Needle>::sentinel QStringTokenizer<Haystack, Needle>::cend() const

    Same as end().

    \sa cbegin(), end()
*/

/*!
    \fn template <typename Haystack, typename Needle> template<typename LContainer> LContainer QStringTokenizer<Haystack, Needle>::toContainer(LContainer &&c) const &

    Converts the lazy sequence into a (typically) random-access container of
    type \c LContainer.

    This function is only available if \c Container has a \c value_type
    matching this tokenizer's value_type.

    If you pass in a named container (an lvalue) for \a c, then that container
    is filled, and a reference to it is returned. If you pass in a temporary
    container (an rvalue, incl. the default argument), then that container is
    filled, and returned by value.

    \code
    // assuming tok's value_type is QStringView, then...
    auto tok = QStringTokenizer{~~~};
    // ... rac1 is a QList:
    auto rac1 = tok.toContainer();
    // ... rac2 is std::pmr::vector<QStringView>:
    auto rac2 = tok.toContainer<std::pmr::vector<QStringView>>();
    auto rac3 = QVarLengthArray<QStringView, 12>{};
    // appends the token sequence produced by tok to rac3
    //  and returns a reference to rac3 (which we ignore here):
    tok.toContainer(rac3);
    \endcode

    This gives you maximum flexibility in how you want the sequence to
    be stored.
*/

/*!
    \fn template <typename Haystack, typename Needle> template<typename RContainer> RContainer QStringTokenizer<Haystack, Needle>::toContainer(RContainer &&c) const &&
    \overload

    Converts the lazy sequence into a (typically) random-access container of
    type \c RContainer.

    In addition to the constraints on the lvalue-this overload, this
    rvalue-this overload is only available when this QStringTokenizer
    does not store the haystack internally, as this could create a
    container full of dangling references:

    \code
    auto tokens = QStringTokenizer{widget.text(), u','}.toContainer();
    // ERROR: cannot call toContainer() on rvalue
    // 'tokens' references the data of the copy of widget.text()
    // stored inside the QStringTokenizer, which has since been deleted
    \endcode

    To fix, store the QStringTokenizer in a temporary:

    \code
    auto tokenizer = QStringTokenizer{widget.text90, u','};
    auto tokens = tokenizer.toContainer();
    // OK: the copy of widget.text() stored in 'tokenizer' keeps the data
    // referenced by 'tokens' alive.
    \endcode

    You can force this function into existence by passing a view instead:

    \code
    func(QStringTokenizer{QStringView{widget.text()}, u','}.toContainer());
    // OK: compiler keeps widget.text() around until after func() has executed
    \endcode

    If you pass in a named container (an lvalue)for \a c, then that container
    is filled, and a reference to it is returned. If you pass in a temporary
    container (an rvalue, incl. the default argument), then that container is
    filled, and returned by value.
*/

/*!
    \fn template <typename Haystack, typename Needle, typename...Flags> auto qTokenize(Haystack &&haystack, Needle &&needle, Flags...flags)
    \relates QStringTokenizer
    \since 6.0

    Factory function for a QStringTokenizer that splits the string \a haystack
    into substrings wherever \a needle occurs, and allows iteration
    over those strings as they are found. If \a needle does not match
    anywhere in \a haystack, a single element containing \a haystack
    is produced.

    Pass values from Qt::CaseSensitivity and Qt::SplitBehavior enumerators
    as \a flags to modify the behavior of the tokenizer.

    You can use this function if your compiler doesn't, yet, support C++17 Class
    Template Argument Deduction (CTAD). We recommend direct use of QStringTokenizer
    with CTAD instead.
*/

QT_END_NAMESPACE
