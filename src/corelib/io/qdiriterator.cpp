// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

/*!
    \since 4.3
    \class QDirIterator
    \inmodule QtCore
    \ingroup io
    \brief The QDirIterator class provides an iterator for directory entrylists.

    You can use QDirIterator to navigate entries of a directory one at a time.
    It is similar to QDir::entryList() and QDir::entryInfoList(), but because
    it lists entries one at a time instead of all at once, it scales better
    and is more suitable for large directories. It also supports listing
    directory contents recursively, and following symbolic links. Unlike
    QDir::entryList(), QDirIterator does not support sorting.

    The QDirIterator constructor takes a QDir or a directory as
    argument. After construction, the iterator is located before the first
    directory entry. Here's how to iterate over all the entries sequentially:

    \snippet code/src_corelib_io_qdiriterator.cpp 0

    Here's how to find and read all files filtered by name, recursively:

    \snippet code/src_corelib_io_qdiriterator.cpp 1

    The next() and nextFileInfo() functions advance the iterator and return
    the path or the QFileInfo of the next directory entry. You can also call
    filePath() or fileInfo() to get the current file path or QFileInfo without
    first advancing the iterator. The fileName() function returns only the
    name of the file, similar to how QDir::entryList() works.

    Unlike Qt's container iterators, QDirIterator is uni-directional (i.e.,
    you cannot iterate directories in reverse order) and does not allow random
    access.

    \note This class is deprecated and may be removed in a Qt release. Use
    QDirListing instead, see \l {Porting QDirIterator to QDirListing}.

    \sa QDir, QDir::entryList()
*/

/*! \enum QDirIterator::IteratorFlag

    This enum describes flags that you can combine to configure the behavior
    of QDirIterator.

    \value NoIteratorFlags The default value, representing no flags. The
    iterator will return entries for the assigned path.

    \value Subdirectories List entries inside all subdirectories as well.

    \value FollowSymlinks When combined with Subdirectories, this flag
    enables iterating through all subdirectories of the assigned path,
    following all symbolic links. Symbolic link loops (e.g., "link" => "." or
    "link" => "..") are automatically detected and ignored.
*/

#include "qdiriterator.h"
#include "qdir_p.h"
#include "qabstractfileengine_p.h"
#include "qdirlisting.h"
#include "qdirentryinfo_p.h"

#include <QtCore/qset.h>
#include <QtCore/qstack.h>
#include <QtCore/qvariant.h>
#if QT_CONFIG(regularexpression)
#include <QtCore/qregularexpression.h>
#endif

#include <QtCore/private/qfilesystemiterator_p.h>
#include <QtCore/private/qfilesystementry_p.h>
#include <QtCore/private/qfilesystemmetadata_p.h>
#include <QtCore/private/qfilesystemengine_p.h>
#include <QtCore/private/qfileinfo_p.h>
#include <QtCore/private/qduplicatetracker_p.h>

#include <memory>
#include <stack>
#include <vector>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QDirIteratorPrivate
{
public:
    QDirIteratorPrivate(const QString &path, const QStringList &nameFilters = {},
                        QDir::Filters filters = QDir::NoFilter,
                        QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags)
        : lister(path, nameFilters, filters.toInt(), flags.toInt())
    { init(); }

    void init()
    {
        it = lister.begin();
        if (it != lister.end())
            nextFileInfo = it->fileInfo();
    }

    void advance()
    {
        // Match the behavior of advance() from before porting to QDirListing,
        // that is, even if hasNext() returns false, calling next() returns an
        // empty string without crashing. QTBUG-130142
        if (it == lister.end()) {
            currentFileInfo = {};
            return;
        }
        currentFileInfo = nextFileInfo;
        if (++it != lister.end()) {
            nextFileInfo = it->fileInfo();
        }
    }

    QDirListing lister;
    QDirListing::const_iterator it = {};
    QFileInfo currentFileInfo;
    QFileInfo nextFileInfo;
};

/*!
    Constructs a QDirIterator that can iterate over \a dir's entrylist, using
    \a dir's name filters and regular filters. You can pass options via \a
    flags to decide how the directory should be iterated.

    By default, \a flags is NoIteratorFlags, which provides the same behavior
    as in QDir::entryList().

    The sorting in \a dir is ignored.

    \note To list symlinks that point to non existing files, QDir::System must be
     passed to the flags.

    \sa hasNext(), next(), IteratorFlags
*/
QDirIterator::QDirIterator(const QDir &dir, IteratorFlags flags)
    : d(new QDirIteratorPrivate(dir.path(), dir.nameFilters(), dir.filter(), flags))
{
}

/*!
    Constructs a QDirIterator that can iterate over \a path, with no name
    filtering and \a filters for entry filtering. You can pass options via \a
    flags to decide how the directory should be iterated.

    By default, \a filters is QDir::NoFilter, and \a flags is NoIteratorFlags,
    which provides the same behavior as in QDir::entryList().

    \note To list symlinks that point to non existing files, QDir::System must be
     passed to the flags.

    \sa hasNext(), next(), IteratorFlags
*/
QDirIterator::QDirIterator(const QString &path, QDir::Filters filters, IteratorFlags flags)
    : d(new QDirIteratorPrivate(path, {}, filters, flags))
{
}

/*!
    Constructs a QDirIterator that can iterate over \a path. You can pass
    options via \a flags to decide how the directory should be iterated.

    By default, \a flags is NoIteratorFlags, which provides the same behavior
    as in QDir::entryList().

    \note To list symlinks that point to non existing files, QDir::System must be
     passed to the flags.

    \sa hasNext(), next(), IteratorFlags
*/
QDirIterator::QDirIterator(const QString &path, IteratorFlags flags)
    : d(new QDirIteratorPrivate(path, {}, QDir::NoFilter, flags))
{
}

/*!
    Constructs a QDirIterator that can iterate over \a path, using \a
    nameFilters and \a filters. You can pass options via \a flags to decide
    how the directory should be iterated.

    By default, \a flags is NoIteratorFlags, which provides the same behavior
    as QDir::entryList().

    For example, the following iterator could be used to iterate over audio
    files:

    \snippet code/src_corelib_io_qdiriterator.cpp 2

    \note To list symlinks that point to non existing files, QDir::System must be
     passed to the flags.

    \sa hasNext(), next(), IteratorFlags, QDir::setNameFilters()
*/
QDirIterator::QDirIterator(const QString &path, const QStringList &nameFilters,
                           QDir::Filters filters, IteratorFlags flags)
    : d(new QDirIteratorPrivate(path, nameFilters, filters, flags))
{
}

/*!
    Destroys the QDirIterator.
*/
QDirIterator::~QDirIterator()
{
}

/*!
    Advances the iterator to the next entry, and returns the file path of this
    new entry. If hasNext() returns \c false, this function does nothing, and
    returns an empty QString. Ideally you should always call hasNext() before
    calling this method.

    You can call fileName() or filePath() to get the current entry's file name
    or path, or fileInfo() to get a QFileInfo for the current entry.

    Call nextFileInfo() instead of next() if you're interested in the QFileInfo.

    \sa hasNext(), nextFileInfo(), fileName(), filePath(), fileInfo()
*/
QString QDirIterator::next()
{
    d->advance();
    return d->currentFileInfo.filePath();
}

/*!
    \since 6.3

    Advances the iterator to the next entry, and returns the file info of this
    new entry. If hasNext() returns \c false, this function does nothing, and
    returns an empty QFileInfo. Ideally you should always call hasNext() before
    calling this method.

    You can call fileName() or filePath() to get the current entry's file name
    or path, or fileInfo() to get a QFileInfo for the current entry.

    Call next() instead of nextFileInfo() when all you need is the filePath().

    \sa hasNext(), fileName(), filePath(), fileInfo()
*/
QFileInfo QDirIterator::nextFileInfo()
{
    d->advance();
    return d->currentFileInfo;
}

/*!
    Returns \c true if there is at least one more entry in the directory;
    otherwise, false is returned.

    \sa next(), nextFileInfo(), fileName(), filePath(), fileInfo()
*/
bool QDirIterator::hasNext() const
{
    return d->it != d->lister.end();
}

/*!
    Returns the file name for the current directory entry, without the path
    prepended.

    This function is convenient when iterating a single directory. When using
    the QDirIterator::Subdirectories flag, you can use filePath() to get the
    full path.

    \sa filePath(), fileInfo()
*/
QString QDirIterator::fileName() const
{
    return d->currentFileInfo.fileName();
}

/*!
    Returns the full file path for the current directory entry.

    \sa fileInfo(), fileName()
*/
QString QDirIterator::filePath() const
{
    return d->currentFileInfo.filePath();
}

/*!
    Returns a QFileInfo for the current directory entry.

    \sa filePath(), fileName()
*/
QFileInfo QDirIterator::fileInfo() const
{
    return d->currentFileInfo;
}

/*!
    Returns the base directory of the iterator.
*/
QString QDirIterator::path() const
{
    return d->lister.iteratorPath();
}

QT_END_NAMESPACE
