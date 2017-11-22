/****************************************************************************
**
** Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#include "qstringtokenizer.h"
#include "qstringalgorithms.h"

QT_BEGIN_NAMESPACE

/*!
    \class QStringTokenizer
    \inmodule QtCore
    \since 6.0
    \brief The QStringTokenizer class splits strings into tokens along given separators
    \reentrant
    \ingroup tools
    \ingroup string-processing

    Splits a string into substrings wherever a given separator occurs,
    and returns a (lazy) list of those strings. If the separator does
    not match anywhere in the string, produces a single-element
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
    QStringView::split() or QLatin1String::split() member functions
    and store the return value only in \c{auto} variables:

    \code
    auto result = string.split(sep);
    \endcode

    This is because the template arguments of QStringTokenizer have a
    very subtle dependency on the specific string and separator types
    from with which they are constructed, and they don't usually
    correspond to the actual types passed.

    \section Lazy Sequences

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

    \section End Sentinel

    The QStringTokenizer iterators cannot be used with classical STL
    algorithms, because those require iterator/iterator pairs, while
    QStringTokenizer uses sentinels, that is, it uses a different
    type, QStringTokenizer::sentinel, to mark the end of the
    range. This improves performance, because the sentinel is an empty
    type. Sentinels are supported from C++17 (for ranged for)
    and C++20 (for algorithms using the new ranges library).

    \section Temporaries

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
    not store a copy. You are reponsible to keep the named object's
    data around for longer than the tokenizer operates on it:

    \code
    auto text = widget.text();
    auto tok = QStringTokenizer{text, u','};
    text.clear();      // destroy content of `text`
    for (auto e : tok) // ERROR: `tok` references deleted data!
        use(e);
    \endcode

    \sa QStringView::split(), QLatin1Sting::split(), Qt::SplitBehavior, Qt::CaseSensitivity
*/

/*!
    \typedef QStringTokenizer::value_type

    Alias for \c{const QStringView} or \c{const QLatin1String},
    depending on the tokenizer's \c Haystack template argument.
*/

/*!
    \typedef QStringTokenizer::difference_type

    Alias for qsizetype.
*/

/*!
    \typedef QStringTokenizer::size_type

    Alias for qsizetype.
*/

/*!
    \typedef QStringTokenizer::reference

    Alias for \c{value_type &}.

    QStringTokenizer does not support mutable references, so this is
    the same as const_reference.
*/

/*!
    \typedef QStringTokenizer::const_reference

    Alias for \c{value_type &}.
*/

/*!
    \typedef QStringTokenizer::pointer

    Alias for \c{value_type *}.

    QStringTokenizer does not support mutable iterators, so this is
    the same as const_pointer.
*/

/*!
    \typedef QStringTokenizer::const_pointer

    Alias for \c{value_type *}.
*/

/*!
    \typedef QStringTokenizer::iterator

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
    \typedef QStringTokenizer::sentinel

    This typedef provides an STL-style sentinel for
    QStringTokenizer::iterator and QStringTokenizer::const_iterator.

    \sa const_iterator
*/

/*!
    \fn QStringTokenizer(Haystack haystack, String needle, Qt::CaseSensitivity cs, Qt::SplitBehavior sb)
    \fn QStringTokenizer(Haystack haystack, String needle, Qt::SplitBehavior sb, Qt::CaseSensitivity cs)

    Constructs a string tokenizer that splits the string \a haystack
    into substrings wherever \a needle occurs, and allows iteration
    over those strings as they are found. If \a needle does not match
    anywhere in \a haystack, a single element containing \a haystack
    is produced.

    \a cs specifies whether \a needle should be matched case
    sensitively or case insensitively.

    If \a sb is QString::SkipEmptyParts, empty entries don't
    appear in the result. By default, empty entries are included.

    \sa QStringView::split(), QLatin1String::split(), Qt::CaseSensitivity, Qt::SplitBehavior
*/

/*!
    \fn QStringTokenizer::const_iterator QStringTokenizer::begin() const

    Returns a const \l{STL-style iterators}{STL-style iterator}
    pointing to the first token in the list.

    \sa end(), cbegin()
*/

/*!
    \fn QStringTokenizer::const_iterator QStringTokenizer::cbegin() const

    Same as begin().

    \sa cend(), begin()
*/

/*!
    \fn QStringTokenizer::sentinel QStringTokenizer::end() const

    Returns a const \l{STL-style iterators}{STL-style sentinel}
    pointing to the imaginary token after the last token in the list.

    \sa begin(), cend()
*/

/*!
    \fn QStringTokenizer::sentinel QStringTokenizer::cend() const

    Same as end().

    \sa cbegin(), end()
*/

/*!
    \fn QStringTokenizer::toContainer(Container &&c) const &

    Convenience method to convert the lazy sequence into a
    (typically) random-access container.

    This function is only available if \c Container has a \c value_type
    matching this tokenizer's value_type.

    If you pass in a named container (an lvalue), then that container
    is filled, and a reference to it is returned.

    If you pass in a temporary container (an rvalue, incl. the default
    argument), then that container is filled, and returned by value.

    \code
    // assuming tok's value_type is QStringView, then...
    auto tok = QStringTokenizer{~~~};
    // ... rac1 is a QVector:
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
    \fn QStringTokenizer::toContainer(Container &&c) const &&
    \overload

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
*/

/*!
    \fn qTokenize(Haystack &&haystack, Needle &&needle, Flags...flags)
    \relates QStringTokenizer
    \since 6.0

    Factory function for QStringTokenizer. You can use this function
    if your compiler doesn't, yet, support C++17 Class Template
    Argument Deduction (CTAD), but we recommend direct use of
    QStringTokenizer with CTAD instead.
*/

QT_END_NAMESPACE
