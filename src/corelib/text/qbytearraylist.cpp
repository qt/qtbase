// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 by Southwest Research Institute (R)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qbytearraylist.h>

QT_BEGIN_NAMESPACE

/*! \typedef QByteArrayListIterator
    \relates QByteArrayList

    The QByteArrayListIterator type definition provides a Java-style const
    iterator for QByteArrayList.

    QByteArrayList provides both \l{Java-style iterators} and
    \l{STL-style iterators}. The Java-style const iterator is simply
    a type definition for QListIterator<QByteArray>.

    \sa QMutableByteArrayListIterator, QByteArrayList::const_iterator
*/

/*! \typedef QMutableByteArrayListIterator
    \relates QByteArrayList

    The QByteArrayListIterator type definition provides a Java-style
    non-const iterator for QByteArrayList.

    QByteArrayList provides both \l{Java-style iterators} and
    \l{STL-style iterators}. The Java-style non-const iterator is
    simply a type definition for QMutableListIterator<QByteArray>.

    \sa QByteArrayListIterator, QByteArrayList::iterator
*/

/*!
    \class QByteArrayList
    \inmodule QtCore
    \since 5.4
    \brief The QByteArrayList class provides a list of byte arrays.

    \ingroup tools
    \ingroup shared
    \ingroup string-processing

    \reentrant

    QByteArrayList is actually just a QList<QByteArray>. It is documented as a
    full class just for simplicity of documenting the member methods that exist
    only in QList<QByteArray>.

    All of QList's functionality also applies to QByteArrayList. For example, you
    can use isEmpty() to test whether the list is empty, and you can call
    functions like append(), prepend(), insert(), replace(), removeAll(),
    removeAt(), removeFirst(), removeLast(), and removeOne() to modify a
    QByteArrayList. In addition, QByteArrayList provides several join()
    methods for concatenating the list into a single QByteArray.

    The purpose of QByteArrayList is quite different from that of QStringList.
    Whereas QStringList has many methods for manipulation of elements within
    the list, QByteArrayList does not.
    Normally, QStringList should be used whenever working with a list of printable
    strings. QByteArrayList should be used to handle and efficiently join large blobs
    of binary data, as when sequentially receiving serialized data through a
    QIODevice.

    \sa QByteArray, QStringList
*/

/*!
    \fn QByteArray QByteArrayList::join(const QByteArray &separator) const

    Joins all the byte arrays into a single byte array with each
    element separated by the given \a separator.
*/

/*!
    \fn QByteArray QByteArrayList::join(QByteArrayView separator) const
    \since 6.3

    Joins all the byte arrays into a single byte array with each
    element separated by the given \a separator, if any.
*/

/*!
    \fn QByteArray QByteArrayList::join(char separator) const

    Joins all the byte arrays into a single byte array with each
    element separated by the given \a separator.
*/

static qsizetype QByteArrayList_joinedSize(const QByteArrayList *that, qsizetype seplen)
{
    qsizetype totalLength = 0;
    const qsizetype size = that->size();

    for (qsizetype i = 0; i < size; ++i)
        totalLength += that->at(i).size();

    if (size > 0)
        totalLength += seplen * (size - 1);

    return totalLength;
}

QByteArray QtPrivate::QByteArrayList_join(const QByteArrayList *that, const char *sep, qsizetype seplen)
{
    QByteArray res;
    if (const qsizetype joinedSize = QByteArrayList_joinedSize(that, seplen))
        res.reserve(joinedSize); // don't call reserve(0) - it allocates one byte for the NUL
    const qsizetype size = that->size();
    for (qsizetype i = 0; i < size; ++i) {
        if (i)
            res.append(sep, seplen);
        res += that->at(i);
    }
    return res;
}

QT_END_NAMESPACE
