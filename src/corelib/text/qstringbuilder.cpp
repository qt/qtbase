// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstringbuilder.h"
#include <private/qstringconverter_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QStringBuilder
    \inmodule QtCore
    \internal
    \reentrant
    \since 4.6

    \brief The QStringBuilder class is a template class that provides a facility to build up QStrings and QByteArrays from smaller chunks.

    \ingroup tools
    \ingroup shared
    \ingroup string-processing


    To build a QString by multiple concatenations, QString::operator+()
    is typically used. This causes \e{n - 1} allocations when building
    a string from \e{n} chunks. The same is true for QByteArray.

    QStringBuilder uses expression templates to collect the individual
    chunks, compute the total size, allocate the required amount of
    memory for the final string object, and copy the chunks into the
    allocated memory.

    The QStringBuilder class is not to be used explicitly in user
    code.  Instances of the class are created as return values of the
    operator%() function, acting on objects of the following types:

    For building QStrings:

    \list
    \li QString, (since 5.10:) QStringView
    \li QChar, QLatin1Char, (since 5.10:) \c char16_t,
    \li QLatin1StringView,
    \li (since 5.10:) \c{const char16_t[]} (\c{u"foo"}),
    \li QByteArray, \c char, \c{const char[]}.
    \endlist

    The types in the last list point are only available when
    \c QT_NO_CAST_FROM_ASCII is not defined.

    For building QByteArrays:

    \list
    \li QByteArray, \c char, \c{const char[]}.
    \endlist

    Concatenating strings with operator%() generally yields better
    performance than using \c QString::operator+() on the same chunks
    if there are three or more of them, and performs equally well in other
    cases.

    \note Defining \c QT_USE_QSTRINGBUILDER at build time (this is the
    default when building Qt libraries and tools), will make using \c {'+'}
    when concatenating strings work the same way as \c operator%().

    \sa QLatin1StringView, QString
*/

/*!
    \internal
    \fn template <typename A, typename B> QStringBuilder<A, B>::QStringBuilder(const A &a, const B &b)

    Constructs a QStringBuilder from \a a and \a b.
 */

/*!
    \internal
    \fn template <typename A, typename B> QStringBuilder<A, B>::operator%(const A &a, const B &b)

    Returns a \c QStringBuilder object that is converted to a QString object
    when assigned to a variable of QString type or passed to a function that
    takes a QString parameter.

    This function is usable with arguments of any of the following types:
    \list
    \li \c QAnyStringView,
    \li \c QString, \c QStringView
    \li \c QByteArray, \c QByteArrayView, \c QLatin1StringView
    \li \c QChar, \c QLatin1Char, \c char, (since 5.10:) \c char16_t
    \li (since 5.10:) \c{const char16_t[]} (\c{u"foo"}),
    \endlist
*/

/*!
    \internal
    \fn template <typename A, typename B> QByteArray QStringBuilder<A, B>::toLatin1() const

    Returns a Latin-1 representation of the string as a QByteArray. It
    is undefined behavior if the string contains non-Latin1 characters.
 */

/*!
    \internal
    \fn template <typename A, typename B> QByteArray QStringBuilder<A, B>::toUtf8() const

    Returns a UTF-8 representation of the string as a QByteArray.
 */

/*!
    \internal
    Converts the UTF-8 string viewed by \a in to UTF-16 and writes the result
    to the buffer starting at \a out.
 */
void QAbstractConcatenable::convertFromUtf8(QByteArrayView in, QChar *&out) noexcept
{
    out = QUtf8::convertToUnicode(out, in);
}

QT_END_NAMESPACE
