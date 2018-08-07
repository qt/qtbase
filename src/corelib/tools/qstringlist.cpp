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

#include <qstringlist.h>
#include <qset.h>
#if QT_CONFIG(regularexpression)
#  include <qregularexpression.h>
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE

/*! \typedef QStringListIterator
    \relates QStringList

    The QStringListIterator type definition provides a Java-style const
    iterator for QStringList.

    QStringList provides both \l{Java-style iterators} and
    \l{STL-style iterators}. The Java-style const iterator is simply
    a type definition for QListIterator<QString>.

    \sa QMutableStringListIterator, QStringList::const_iterator
*/

/*! \typedef QMutableStringListIterator
    \relates QStringList

    The QStringListIterator type definition provides a Java-style
    non-const iterator for QStringList.

    QStringList provides both \l{Java-style iterators} and
    \l{STL-style iterators}. The Java-style non-const iterator is
    simply a type definition for QMutableListIterator<QString>.

    \sa QStringListIterator, QStringList::iterator
*/

/*!
    \class QStringList
    \inmodule QtCore
    \brief The QStringList class provides a list of strings.

    \ingroup tools
    \ingroup shared
    \ingroup string-processing

    \reentrant

    QStringList inherits from QList<QString>. Like QList, QStringList is
    \l{implicitly shared}. It provides fast index-based access as well as fast
    insertions and removals. Passing string lists as value parameters is both
    fast and safe.

    All of QList's functionality also applies to QStringList. For example, you
    can use isEmpty() to test whether the list is empty, and you can call
    functions like append(), prepend(), insert(), replace(), removeAll(),
    removeAt(), removeFirst(), removeLast(), and removeOne() to modify a
    QStringList. In addition, QStringList provides a few convenience
    functions that make handling lists of strings easier:

    \tableofcontents

    \section1 Initializing

    The default constructor creates an empty list. You can use the
    initializer-list constructor to create a list with elements:

    \snippet qstringlist/main.cpp 0a

    \section1 Adding Strings

    Strings can be added to a list using the \l
    {QList::insert()}{insert()} \l
    {QList::append()}{append()}, \l
    {QList::operator+=()}{operator+=()} and \l
    {operator<<()} functions.

    \l{operator<<()} can be used to
    conveniently add multiple elements to a list:

    \snippet qstringlist/main.cpp 0b

    \section1 Iterating Over the Strings

    To iterate over a list, you can either use index positions or
    QList's Java-style and STL-style iterator types:

    Indexing:

    \snippet qstringlist/main.cpp 1

    Java-style iterator:

    \snippet qstringlist/main.cpp 2

    STL-style iterator:

    \snippet qstringlist/main.cpp 3

    The QStringListIterator class is simply a type definition for
    QListIterator<QString>. QStringList also provide the
    QMutableStringListIterator class which is a type definition for
    QMutableListIterator<QString>.

    \section1 Manipulating the Strings

    QStringList provides several functions allowing you to manipulate
    the contents of a list. You can concatenate all the strings in a
    string list into a single string (with an optional separator)
    using the join() function. For example:

    \snippet qstringlist/main.cpp 4

    The argument to join can be a single character or a string.

    To break up a string into a string list, use the QString::split()
    function:

    \snippet qstringlist/main.cpp 6

    The argument to split can be a single character, a string, a
    QRegularExpression or a (deprecated) QRegExp.

    In addition, the \l {QStringList::operator+()}{operator+()}
    function allows you to concatenate two string lists into one. To
    sort a string list, use the sort() function.

    QString list also provides the filter() function which lets you
    to extract a new list which contains only those strings which
    contain a particular substring (or match a particular regular
    expression):

    \snippet qstringlist/main.cpp 7

    The contains() function tells you whether the list contains a
    given string, while the indexOf() function returns the index of
    the first occurrence of the given string. The lastIndexOf()
    function on the other hand, returns the index of the last
    occurrence of the string.

    Finally, the replaceInStrings() function calls QString::replace()
    on each string in the string list in turn. For example:

    \snippet qstringlist/main.cpp 8

    \sa QString
*/

/*!
    \fn QStringList::QStringList()

    Constructs an empty string list.
*/

/*!
    \fn QStringList::QStringList(const QString &str)

    Constructs a string list that contains the given string, \a
    str. Longer lists are easily created like this:

    \snippet qstringlist/main.cpp 9

    \sa append()
*/

/*!
    \fn QStringList::QStringList(const QList<QString> &other)

    Constructs a copy of \a other.

    This operation takes \l{constant time}, because QStringList is
    \l{implicitly shared}. This makes returning a QStringList from a
    function very fast. If a shared instance is modified, it will be
    copied (copy-on-write), and that takes \l{linear time}.

    \sa operator=()
*/

/*!
    \fn QStringList::QStringList(QList<QString> &&other)
    \overload
    \since 5.4

    Move-constructs from QList<QString>.

    After a successful construction, \a other will be empty.
*/

/*!
    \fn QStringList &QStringList::operator=(const QList<QString> &other)
    \since 5.4

    Copy assignment operator from QList<QString>. Assigns the \a other
    list of strings to this string list.

    After the operation, \a other and \c *this will be equal.
*/

/*!
    \fn QStringList &QStringList::operator=(QList<QString> &&other)
    \overload
    \since 5.4

    Move assignment operator from QList<QString>. Moves the \a other
    list of strings to this string list.

    After the operation, \a other will be empty.
*/

/*!
    \fn void QStringList::sort(Qt::CaseSensitivity cs)

    Sorts the list of strings in ascending order.
    If \a cs is \l Qt::CaseSensitive (the default), the string comparison
    is case sensitive; otherwise the comparison is case insensitive.

    Sorting is performed using the STL's std::sort() algorithm,
    which averages \l{linear-logarithmic time}, i.e. O(\e{n} log \e{n}).

    If you want to sort your strings in an arbitrary order, consider
    using the QMap class. For example, you could use a QMap<QString,
    QString> to create a case-insensitive ordering (e.g. with the keys
    being lower-case versions of the strings, and the values being the
    strings), or a QMap<int, QString> to sort the strings by some
    integer index.
*/

namespace {
struct CaseInsensitiveLessThan {
    typedef bool result_type;
    result_type operator()(const QString &s1, const QString &s2) const
    {
        return s1.compare(s2, Qt::CaseInsensitive) < 0;
    }
};
}

void QtPrivate::QStringList_sort(QStringList *that, Qt::CaseSensitivity cs)
{
    if (cs == Qt::CaseSensitive)
        std::sort(that->begin(), that->end());
    else
        std::sort(that->begin(), that->end(), CaseInsensitiveLessThan());
}


/*!
    \fn QStringList QStringList::filter(const QString &str, Qt::CaseSensitivity cs) const

    Returns a list of all the strings containing the substring \a str.

    If \a cs is \l Qt::CaseSensitive (the default), the string
    comparison is case sensitive; otherwise the comparison is case
    insensitive.

    \snippet qstringlist/main.cpp 5
    \snippet qstringlist/main.cpp 10

    This is equivalent to

    \snippet qstringlist/main.cpp 11
    \snippet qstringlist/main.cpp 12

    \sa contains()
*/
QStringList QtPrivate::QStringList_filter(const QStringList *that, const QString &str,
                                          Qt::CaseSensitivity cs)
{
    QStringMatcher matcher(str, cs);
    QStringList res;
    for (int i = 0; i < that->size(); ++i)
        if (matcher.indexIn(that->at(i)) != -1)
            res << that->at(i);
    return res;
}

template<typename T>
static bool stringList_contains(const QStringList &stringList, const T &str, Qt::CaseSensitivity cs)
{
    for (const auto &string : stringList) {
        if (string.size() == str.size() && string.compare(str, cs) == 0)
            return true;
    }
    return false;
}


/*!
    \fn bool QStringList::contains(const QString &str, Qt::CaseSensitivity cs) const

    Returns \c true if the list contains the string \a str; otherwise
    returns \c false. The search is case insensitive if \a cs is
    Qt::CaseInsensitive; the search is case sensitive by default.

    \sa indexOf(), lastIndexOf(), QString::contains()
 */
bool QtPrivate::QStringList_contains(const QStringList *that, const QString &str,
                                     Qt::CaseSensitivity cs)
{
    return stringList_contains(*that, str, cs);
}

/*!
    \fn bool QStringList::contains(QLatin1String str, Qt::CaseSensitivity cs) const
    \overload
    \since 5.10

    Returns \c true if the list contains the string \a str; otherwise
    returns \c false. The search is case insensitive if \a cs is
    Qt::CaseInsensitive; the search is case sensitive by default.

    \sa indexOf(), lastIndexOf(), QString::contains()
 */
bool QtPrivate::QStringList_contains(const QStringList *that, QLatin1String str,
                                     Qt::CaseSensitivity cs)
{
    return stringList_contains(*that, str, cs);
}

#ifndef QT_NO_REGEXP
/*!
    \fn QStringList QStringList::filter(const QRegExp &rx) const

    \overload

    Returns a list of all the strings that match the regular
    expression \a rx.
*/
QStringList QtPrivate::QStringList_filter(const QStringList *that, const QRegExp &rx)
{
    QStringList res;
    for (int i = 0; i < that->size(); ++i)
        if (that->at(i).contains(rx))
            res << that->at(i);
    return res;
}
#endif

#if QT_CONFIG(regularexpression)
/*!
    \fn QStringList QStringList::filter(const QRegularExpression &re) const
    \overload
    \since 5.0

    Returns a list of all the strings that match the regular
    expression \a re.
*/
QStringList QtPrivate::QStringList_filter(const QStringList *that, const QRegularExpression &re)
{
    QStringList res;
    for (int i = 0; i < that->size(); ++i) {
        if (that->at(i).contains(re))
            res << that->at(i);
    }
    return res;
}
#endif // QT_CONFIG(regularexpression)

/*!
    \fn QStringList &QStringList::replaceInStrings(const QString &before, const QString &after, Qt::CaseSensitivity cs)

    Returns a string list where every string has had the \a before
    text replaced with the \a after text wherever the \a before text
    is found. The \a before text is matched case-sensitively or not
    depending on the \a cs flag.

    For example:

    \snippet qstringlist/main.cpp 5
    \snippet qstringlist/main.cpp 13

    \sa QString::replace()
*/
void QtPrivate::QStringList_replaceInStrings(QStringList *that, const QString &before,
                                             const QString &after, Qt::CaseSensitivity cs)
{
    for (int i = 0; i < that->size(); ++i)
        (*that)[i].replace(before, after, cs);
}


#ifndef QT_NO_REGEXP
/*!
    \fn QStringList &QStringList::replaceInStrings(const QRegExp &rx, const QString &after)
    \overload

    Replaces every occurrence of the regexp \a rx, in each of the
    string lists's strings, with \a after. Returns a reference to the
    string list.

    For example:

    \snippet qstringlist/main.cpp 5
    \snippet qstringlist/main.cpp 14

    For regular expressions that contain \l{capturing parentheses},
    occurrences of \b{\\1}, \b{\\2}, ..., in \a after are
    replaced with \a{rx}.cap(1), \a{rx}.cap(2), ...

    For example:

    \snippet qstringlist/main.cpp 5
    \snippet qstringlist/main.cpp 15
*/
void QtPrivate::QStringList_replaceInStrings(QStringList *that, const QRegExp &rx, const QString &after)
{
    for (int i = 0; i < that->size(); ++i)
        (*that)[i].replace(rx, after);
}
#endif

#if QT_CONFIG(regularexpression)
/*!
    \fn QStringList &QStringList::replaceInStrings(const QRegularExpression &re, const QString &after)
    \overload
    \since 5.0

    Replaces every occurrence of the regular expression \a re, in each of the
    string lists's strings, with \a after. Returns a reference to the string
    list.

    For example:

    \snippet qstringlist/main.cpp 5
    \snippet qstringlist/main.cpp 16

    For regular expressions that contain capturing groups,
    occurrences of \b{\\1}, \b{\\2}, ..., in \a after are
    replaced with the string captured by the corresponding capturing group.

    For example:

    \snippet qstringlist/main.cpp 5
    \snippet qstringlist/main.cpp 17
*/
void QtPrivate::QStringList_replaceInStrings(QStringList *that, const QRegularExpression &re, const QString &after)
{
    for (int i = 0; i < that->size(); ++i)
        (*that)[i].replace(re, after);
}
#endif // QT_CONFIG(regularexpression)

static int accumulatedSize(const QStringList &list, int seplen)
{
    int result = 0;
    if (!list.isEmpty()) {
        for (const auto &e : list)
            result += e.size() + seplen;
        result -= seplen;
    }
    return result;
}

/*!
    \fn QString QStringList::join(const QString &separator) const

    Joins all the string list's strings into a single string with each
    element separated by the given \a separator (which can be an
    empty string).

    \sa QString::split()
*/

/*!
    \fn QString QStringList::join(QChar separator) const
    \since 5.0
    \overload join()
*/
QString QtPrivate::QStringList_join(const QStringList *that, const QChar *sep, int seplen)
{
    const int totalLength = accumulatedSize(*that, seplen);
    const int size = that->size();

    QString res;
    if (totalLength == 0)
        return res;
    res.reserve(totalLength);
    for (int i = 0; i < size; ++i) {
        if (i)
            res.append(sep, seplen);
        res += that->at(i);
    }
    return res;
}

/*!
    \fn QString QStringList::join(QLatin1String separator) const
    \since 5.8
    \overload join()
*/
QString QtPrivate::QStringList_join(const QStringList &list, QLatin1String sep)
{
    QString result;
    if (!list.isEmpty()) {
        result.reserve(accumulatedSize(list, sep.size()));
        const auto end = list.end();
        auto it = list.begin();
        result += *it;
        while (++it != end) {
            result += sep;
            result += *it;
        }
    }
    return result;
}

/*!
    \fn QStringList QStringList::operator+(const QStringList &other) const

    Returns a string list that is the concatenation of this string
    list with the \a other string list.

    \sa append()
*/

/*!
    \fn QStringList &QStringList::operator<<(const QString &str)

    Appends the given string, \a str, to this string list and returns
    a reference to the string list.

    \sa append()
*/

/*!
    \fn QStringList &QStringList::operator<<(const QStringList &other)

    \overload

    Appends the \a other string list to the string list and returns a reference to
    the latter string list.
*/

/*!
    \fn QStringList &QStringList::operator<<(const QList<QString> &other)
    \since 5.4

    \overload

    Appends the \a other string list to the string list and returns a reference to
    the latter string list.
*/

#ifndef QT_NO_REGEXP
static int indexOfMutating(const QStringList *that, QRegExp &rx, int from)
{
    if (from < 0)
        from = qMax(from + that->size(), 0);
    for (int i = from; i < that->size(); ++i) {
        if (rx.exactMatch(that->at(i)))
            return i;
    }
    return -1;
}

static int lastIndexOfMutating(const QStringList *that, QRegExp &rx, int from)
{
    if (from < 0)
        from += that->size();
    else if (from >= that->size())
        from = that->size() - 1;
    for (int i = from; i >= 0; --i) {
        if (rx.exactMatch(that->at(i)))
            return i;
        }
    return -1;
}

/*!
    \fn int QStringList::indexOf(const QRegExp &rx, int from) const

    Returns the index position of the first exact match of \a rx in
    the list, searching forward from index position \a from. Returns
    -1 if no item matched.

    \sa lastIndexOf(), contains(), QRegExp::exactMatch()
*/
int QtPrivate::QStringList_indexOf(const QStringList *that, const QRegExp &rx, int from)
{
    QRegExp rx2(rx);
    return indexOfMutating(that, rx2, from);
}

/*!
    \fn int QStringList::indexOf(QRegExp &rx, int from) const
    \overload indexOf()
    \since 4.5

    Returns the index position of the first exact match of \a rx in
    the list, searching forward from index position \a from. Returns
    -1 if no item matched.

    If an item matched, the \a rx regular expression will contain the
    matched objects (see QRegExp::matchedLength, QRegExp::cap).

    \sa lastIndexOf(), contains(), QRegExp::exactMatch()
*/
int QtPrivate::QStringList_indexOf(const QStringList *that, QRegExp &rx, int from)
{
    return indexOfMutating(that, rx, from);
}

/*!
    \fn int QStringList::lastIndexOf(const QRegExp &rx, int from) const

    Returns the index position of the last exact match of \a rx in
    the list, searching backward from index position \a from. If \a
    from is -1 (the default), the search starts at the last item.
    Returns -1 if no item matched.

    \sa indexOf(), contains(), QRegExp::exactMatch()
*/
int QtPrivate::QStringList_lastIndexOf(const QStringList *that, const QRegExp &rx, int from)
{
    QRegExp rx2(rx);
    return lastIndexOfMutating(that, rx2, from);
}

/*!
    \fn int QStringList::lastIndexOf(QRegExp &rx, int from) const
    \overload lastIndexOf()
    \since 4.5

    Returns the index position of the last exact match of \a rx in
    the list, searching backward from index position \a from. If \a
    from is -1 (the default), the search starts at the last item.
    Returns -1 if no item matched.

    If an item matched, the \a rx regular expression will contain the
    matched objects (see QRegExp::matchedLength, QRegExp::cap).

    \sa indexOf(), contains(), QRegExp::exactMatch()
*/
int QtPrivate::QStringList_lastIndexOf(const QStringList *that, QRegExp &rx, int from)
{
    return lastIndexOfMutating(that, rx, from);
}
#endif

#if QT_CONFIG(regularexpression)
/*!
    \fn int QStringList::indexOf(const QRegularExpression &re, int from) const
    \overload
    \since 5.0

    Returns the index position of the first exact match of \a re in
    the list, searching forward from index position \a from. Returns
    -1 if no item matched.

    \sa lastIndexOf()
*/
int QtPrivate::QStringList_indexOf(const QStringList *that, const QRegularExpression &re, int from)
{
    if (from < 0)
        from = qMax(from + that->size(), 0);

    QString exactPattern = QLatin1String("\\A(?:") + re.pattern() + QLatin1String(")\\z");
    QRegularExpression exactRe(exactPattern, re.patternOptions());

    for (int i = from; i < that->size(); ++i) {
        QRegularExpressionMatch m = exactRe.match(that->at(i));
        if (m.hasMatch())
            return i;
    }
    return -1;
}

/*!
    \fn int QStringList::lastIndexOf(const QRegularExpression &re, int from) const
    \overload
    \since 5.0

    Returns the index position of the last exact match of \a re in
    the list, searching backward from index position \a from. If \a
    from is -1 (the default), the search starts at the last item.
    Returns -1 if no item matched.

    \sa indexOf()
*/
int QtPrivate::QStringList_lastIndexOf(const QStringList *that, const QRegularExpression &re, int from)
{
    if (from < 0)
        from += that->size();
    else if (from >= that->size())
        from = that->size() - 1;

    QString exactPattern = QLatin1String("\\A(?:") + re.pattern() + QLatin1String(")\\z");
    QRegularExpression exactRe(exactPattern, re.patternOptions());

    for (int i = from; i >= 0; --i) {
        QRegularExpressionMatch m = exactRe.match(that->at(i));
        if (m.hasMatch())
            return i;
    }
    return -1;
}
#endif // QT_CONFIG(regularexpression)

/*!
    \fn int QStringList::removeDuplicates()

    \since 4.5

    This function removes duplicate entries from a list.
    The entries do not have to be sorted. They will retain their
    original order.

    Returns the number of removed entries.
*/
int QtPrivate::QStringList_removeDuplicates(QStringList *that)
{
    int n = that->size();
    int j = 0;
    QSet<QString> seen;
    seen.reserve(n);
    int setSize = 0;
    for (int i = 0; i < n; ++i) {
        const QString &s = that->at(i);
        seen.insert(s);
        if (setSize == seen.size()) // unchanged size => was already seen
            continue;
        ++setSize;
        if (j != i)
            that->swap(i, j);
        ++j;
    }
    if (n != j)
        that->erase(that->begin() + j, that->end());
    return n - j;
}

/*! \fn QStringList::QStringList(std::initializer_list<QString> args)
    \since 4.8

    Construct a list from a std::initializer_list given by \a args.

    This constructor is only enabled if the compiler supports C++11 initializer
    lists.
*/


QT_END_NAMESPACE
