// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2024 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

/*!
    \since 6.8
    \class QDirListing
    \inmodule QtCore
    \ingroup  io
    \brief The QDirListing class provides an STL-style iterator for directory entries.

    You can use QDirListing to navigate entries of a directory one at a time.
    It is similar to QDir::entryList() and QDir::entryInfoList(), but because
    it lists entries one at a time instead of all at once, it scales better
    and is more suitable for large directories. It also supports listing
    directory contents recursively, and following symbolic links. Unlike
    QDir::entryList(), QDirListing does not support sorting.

    The QDirListing constructor takes a directory path string as
    argument. Here's how to iterate over all entries recursively:

    \snippet code/src_corelib_io_qdirlisting.cpp 0

    Here's how to find and read all regular files filtered by name, recursively:

    \snippet code/src_corelib_io_qdirlisting.cpp 1

    Here's how to list only regular files, recursively:
    \snippet code/src_corelib_io_qdirlisting.cpp 5

    Here's how to list only regular files and symbolic links to regular
    files, recursively:
    \snippet code/src_corelib_io_qdirlisting.cpp 6

//! [std-input-iterator-tag]
    QDirListing::const_iterator models C++20
    \l{https://en.cppreference.com/w/cpp/iterator/input_iterator}{std::input_iterator},
    that is, it is a move-only, forward-only, single-pass iterator, that
    doesn't allow random access.
//! [std-input-iterator-tag]
    It can be used in ranged-for loops (or with C++20 range algorithms that don't
    require random access iterators). Dereferencing a valid iterator returns
    a QDirListing::DirEntry object. The (c)end() sentinel marks the end of
    the iteration. Dereferencing an iterator that is equal to \l{sentinel} is
    undefined behavior.

    QDirListing::DirEntry offers a subset of QFileInfo's API (for example,
    fileName(), filePath(), exists()). Internally, DirEntry only constructs
    a QFileInfo object if needed, that is, if the info hasn't been already
    fetched by other system functions. You can use DirEntry::fileInfo()
    to get a QFileInfo. For example:

    \snippet code/src_corelib_io_qdirlisting.cpp 3
    \snippet code/src_corelib_io_qdirlisting.cpp 4

    \sa QDir, QDir::entryList()
*/

/*! \enum QDirListing::IteratorFlag

    This enum class describes flags that can be used to configure the behavior
    of QDirListing. Values from this enumerator can be bitwise OR'ed together.

    \value Default
        List all entries, that is, files, directories, symbolic links including broken
        symbolic links (where the target doesn't exist) and special (\e other) system
        files, see ExcludeOther for details.
        Hidden files and directories and the special entries \c{.} and \c{..}
        aren't listed by default.

    \value ExcludeFiles
        Don't list regular files. When combined with ResolveSymlinks, symbolic
        links to regular files will be excluded too.

    \value ExcludeDirs
        Don't list directories. When combined with ResolveSymlinks, symbolic
        links to directories will be excluded too.

    \omitvalue ExcludeSpecial
    \value ExcludeOther [since 6.10]
        Don't list file system entries that are \e not directories, regular files,
        or symbolic links.
        \list
            \li On Unix, a special (other) file system entry is a FIFO, socket,
                character device, or block device. For more details see the
                \l{https://pubs.opengroup.org/onlinepubs/9699919799/functions/mknod.html}{\c mknod}
                manual page.
            \li On Windows (for historical reasons) \c .lnk files are considered
                special (other) file system entries.
        \endlist

    \value ResolveSymlinks
        Filter symbolic links based on the type of the target of the link,
        rather than the symbolic link itself. Broken symbolic links (where
        the target doesn't exist) are excluded, set IncludeBrokenSymlinks
        to include them.
        This flag is ignored on operating systems that don't support symbolic links.

    \value IncludeBrokenSymlinks [since 6.11]
        Lists broken symbolic links, where the target doesn't exist, regardless
        of the status of the ResolveSymlinks flag.
        This flag is ignored on operating systems that don't support symbolic links.

    \value FilesOnly
        Only regular files will be listed. When combined with ResolveSymlinks,
        symbolic links to files will also be listed.

    \value DirsOnly
        Only directories will be listed. When combined with ResolveSymlinks,
        symbolic links to directories will also be listed.

    \value IncludeHidden
        List hidden entries. When combined with Recursive, the iteration will
        recurse into hidden sub-directories as well.

    \value IncludeDotAndDotDot
        List the \c {.} and \c{..} special entries.

    \value CaseSensitive
        The file glob patterns in the name filters passed to the QDirListing
        constructor, will be matched case sensitively (for details, see
        QDir::setNameFilters()).

    \value Recursive
        List entries inside all sub-directories as well. When combined with
        FollowDirSymlinks, symbolic links to directories will be iterated too.

    \value FollowDirSymlinks
        When combined with Recursive, symbolic links to directories will be
        iterated too. Symbolic link loops (e.g., link => . or link => ..) are
        automatically detected and ignored.

    \omitvalue NoNameFiltersForDirs
*/

#include "qdirlisting.h"
#include "qdirentryinfo_p.h"

#include "qdir_p.h"
#include "qdiriterator.h"
#include "qabstractfileengine_p.h"

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

class QDirListingPrivate
{
    Q_DISABLE_COPY_MOVE(QDirListingPrivate)
public:
    QDirListingPrivate() = default;

    // the default for std::stack is std::deque, but std::vector is more apt:
    template <typename T>
    struct vector_stack : std::stack<T, std::vector<T>>
    {
        using Base = std::stack<T, std::vector<T>>;
        using Base::Base;
        void clear() { this->c.clear(); } // std::stack is also missing clear()
    };

    void init();
    void advance();
    void beginIterating();

    bool entryMatches(QDirEntryInfo &info);
    void pushDirectory(QDirEntryInfo &info);
    void pushInitialDirectory();

    void checkAndPushDirectory(QDirEntryInfo &info);
    bool matchesFilters(QDirEntryInfo &data) const;
    bool hasIterators() const;

    std::unique_ptr<QAbstractFileEngine> engine;
    QDirEntryInfo initialEntryInfo;
    QStringList nameFilters;
    QDirListing::IteratorFlags iteratorFlags;
    QDirEntryInfo currentEntryInfo;

#if QT_CONFIG(regularexpression)
    std::vector<QRegularExpression> nameRegExps;
    bool regexMatchesName(const QString &fileName) const
    {
        if (nameRegExps.empty())
            return true;
        auto hasMatch = [&fileName](const auto &re) { return re.match(fileName).hasMatch(); };
        return std::any_of(nameRegExps.cbegin(), nameRegExps.cend(), hasMatch);
    }
#endif

    using FEngineIteratorPtr = std::unique_ptr<QAbstractFileEngineIterator>;
    vector_stack<FEngineIteratorPtr> fileEngineIterators;
#ifndef QT_NO_FILESYSTEMITERATOR
    using FsIteratorPtr = std::unique_ptr<QFileSystemIterator>;
    vector_stack<FsIteratorPtr> nativeIterators;
#endif

    // Loop protection
    QDuplicateTracker<QString> visitedLinks;
};

void QDirListingPrivate::init()
{
    if (nameFilters.contains("*"_L1))
        nameFilters.clear();

#if QT_CONFIG(regularexpression)
    nameRegExps.reserve(size_t(nameFilters.size()));

    const bool isCase = iteratorFlags.testAnyFlags(QDirListing::IteratorFlag::CaseSensitive);
    const auto cs = isCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
    for (const auto &filter : std::as_const(nameFilters))
        nameRegExps.emplace_back(QRegularExpression::fromWildcard(filter, cs));
#endif

    engine = QFileSystemEngine::createLegacyEngine(initialEntryInfo.entry,
                                                   initialEntryInfo.metaData);
}

/*!
    \internal

    Resets the iteration state (if any), so that calling begin()/cbegin()
    always starts iterating anew.
*/
void QDirListingPrivate::beginIterating()
{
#ifndef QT_NO_FILESYSTEMITERATOR
    nativeIterators.clear();
#endif
    fileEngineIterators.clear();
    visitedLinks.clear();
    pushDirectory(initialEntryInfo);
}

void QDirListingPrivate::pushDirectory(QDirEntryInfo &entryInfo)
{
    const QString path = [&entryInfo] {
#ifdef Q_OS_WIN
        if (entryInfo.isSymLink())
            return entryInfo.canonicalFilePath();
#endif
        return entryInfo.filePath();
    }();


    if (iteratorFlags.testAnyFlags(QDirListing::IteratorFlag::FollowDirSymlinks)) {
        // Stop link loops
        if (visitedLinks.hasSeen(entryInfo.canonicalFilePath()))
            return;
    }

    if (engine) {
        engine->setFileName(path);
        if (auto it = engine->beginEntryList(path, iteratorFlags, nameFilters)) {
            fileEngineIterators.push(std::move(it));
        } else {
            // No iterator; no entry list.
        }
    } else {
#ifndef QT_NO_FILESYSTEMITERATOR
        QFileSystemEntry *fentry = nullptr;
        if (entryInfo.fileInfoOpt)
            fentry = &entryInfo.fileInfoOpt->d_ptr->fileEntry;
        else
            fentry = &entryInfo.entry;
        nativeIterators.push(std::make_unique<QFileSystemIterator>(*fentry, iteratorFlags));
#else
        qWarning("Qt was built with -no-feature-filesystemiterator: no files/plugins will be found!");
#endif
    }
}

bool QDirListingPrivate::entryMatches(QDirEntryInfo &entryInfo)
{
    checkAndPushDirectory(entryInfo);
    return matchesFilters(entryInfo);
}

/*!
    \internal

    Advances the internal iterator, either a QAbstractFileEngineIterator (e.g.
    QResourceFileEngineIterator) or a QFileSystemIterator (which uses low-level
    system methods, e.g. readdir() on Unix). The iterators are stored in a
    vector.

    A typical example of doing recursive iteration:
    - while iterating directory A we find a sub-dir B
    - an iterator for B is added to the vector
    - B's iterator is processed (vector.back()) first; then the loop
      goes back to processing A's iterator
*/
void QDirListingPrivate::advance()
{
    // Use get() in both code paths below because the iterator returned by back()
    // may be invalidated due to reallocation when appending new iterators in
    // pushDirectory().

    if (engine) {
        while (!fileEngineIterators.empty()) {
            // Find the next valid iterator that matches the filters.
            QAbstractFileEngineIterator *it;
            while (it = fileEngineIterators.top().get(), it->advance()) {
                QDirEntryInfo entryInfo;
                entryInfo.fileInfoOpt = it->currentFileInfo();
                if (entryMatches(entryInfo)) {
                    currentEntryInfo = std::move(entryInfo);
                    return;
                }
            }

            fileEngineIterators.pop();
        }
    } else {
#ifndef QT_NO_FILESYSTEMITERATOR
        QDirEntryInfo entryInfo;
        while (!nativeIterators.empty()) {
            // Find the next valid iterator that matches the filters.
            QFileSystemIterator *it;
            while (it = nativeIterators.top().get(),
                   it->advance(entryInfo.entry, entryInfo.metaData)) {
                if (entryMatches(entryInfo)) {
                    currentEntryInfo = std::move(entryInfo);
                    return;
                }
                entryInfo = {};
            }

            nativeIterators.pop();
        }
#endif
    }
}

static bool isDotOrDotDot(QStringView fileName)
{
    return fileName == "."_L1 || fileName == ".."_L1;
}

void QDirListingPrivate::checkAndPushDirectory(QDirEntryInfo &entryInfo)
{
    using F = QDirListing::IteratorFlag;
    // If we're doing flat iteration, we're done.
    if (!iteratorFlags.testAnyFlags(F::Recursive))
        return;

    // Follow symlinks only when asked
    if (!iteratorFlags.testAnyFlags(F::FollowDirSymlinks) && entryInfo.isSymLink())
        return;

    // Never follow . and ..
    if (isDotOrDotDot(entryInfo.fileName()))
        return;

    // No hidden directories unless requested
    const bool includeHidden = iteratorFlags.testAnyFlags(QDirListing::IteratorFlag::IncludeHidden);
    if (!includeHidden && entryInfo.isHidden())
        return;

    // Never follow non-directory entries
    if (!entryInfo.isDir())
        return;

    pushDirectory(entryInfo);
}

/*!
    \internal

    This function returns \c true if the current entry matches the filters
    (i.e., the current entry will be returned as part of the directory
    iteration); otherwise, \c false is returned.
*/
bool QDirListingPrivate::matchesFilters(QDirEntryInfo &entryInfo) const
{
    using F = QDirListing::IteratorFlag;

    const QString &fileName = entryInfo.fileName();
    if (fileName.isEmpty())
        return false;

    // name filter
#if QT_CONFIG(regularexpression)
    const bool skipNameFilters = iteratorFlags.testAnyFlags(F::NoNameFiltersForDirs)
                                 && entryInfo.isDir();
    if (!skipNameFilters) {
        if (!regexMatchesName(fileName))
            return false;
    }
#endif // QT_CONFIG(regularexpression)

    if (isDotOrDotDot(fileName))
        return iteratorFlags.testFlags(F::IncludeDotAndDotDot);

    if (!iteratorFlags.testAnyFlag(F::IncludeHidden) && entryInfo.isHidden())
        return false;

    const bool includeBrokenSymlinks = iteratorFlags.testAnyFlags(F::IncludeBrokenSymlinks);
    if (includeBrokenSymlinks && entryInfo.isSymLink() && !entryInfo.exists())
        return true;

    if (iteratorFlags.testFlag(F::ResolveSymlinks)) {
        if (entryInfo.isSymLink() && !entryInfo.exists())
            return false; // Exclude broken symlinks; anything else will be filtered below
    } else {
        constexpr auto f = F::ExcludeFiles | F::ExcludeDirs | F::ExcludeOther;
        const bool filterByTargetType = iteratorFlags.testAnyFlags(f);
        if (filterByTargetType && entryInfo.isSymLink())
            return false;
    }

    if (iteratorFlags.testAnyFlag(F::ExcludeOther)
        && !entryInfo.isFile() && !entryInfo.isDir() && !entryInfo.isSymLink()) {
        return false;
    }

    if (iteratorFlags.testAnyFlags(F::ExcludeDirs) && entryInfo.isDir())
        return false;

    if (iteratorFlags.testAnyFlags(F::ExcludeFiles) && entryInfo.isFile())
        return false;

    return true;
}

bool QDirListingPrivate::hasIterators() const
{
    if (engine)
        return !fileEngineIterators.empty();

#if !defined(QT_NO_FILESYSTEMITERATOR)
    return !nativeIterators.empty();
#endif

    return false;
}

/*!
    Constructs a QDirListing that can iterate over \a path.

    You can pass options via \a flags to control how the directory should
    be iterated.

    By default, \a flags is IteratorFlag::Default.

    \sa IteratorFlags
*/
QDirListing::QDirListing(const QString &path, IteratorFlags flags)
    : d(new QDirListingPrivate)
{
    d->initialEntryInfo.entry = QFileSystemEntry(path);
    d->iteratorFlags = flags;
    d->init();
}

/*!
    Constructs a QDirListing that can iterate over \a path.

    You can pass options via \a flags to control how the directory should
    be iterated. By default, \a flags is IteratorFlag::Default.

    The listed entries will be filtered according to the file glob patterns
    in \a nameFilters, which are converted to a regular expression using
    QRegularExpression::fromWildcard (see QDir::setNameFilters() for more
    details).

    For example, the following iterator could be used to iterate over audio
    files:

    \snippet code/src_corelib_io_qdirlisting.cpp 2

    Sometimes you can filter by name more efficiently by iterating over the
    entries with a range-for loop, using string comparison. For example:

    \snippet code/src_corelib_io_qdirlisting.cpp 7

    \sa IteratorFlags, QDir::setNameFilters()
*/
QDirListing::QDirListing(const QString &path, const QStringList &nameFilters, IteratorFlags flags)
    : d(new QDirListingPrivate)
{
    d->initialEntryInfo.entry = QFileSystemEntry(path);
    d->nameFilters = nameFilters;
    d->iteratorFlags = flags;
    d->init();
}

/*!
    \fn QDirListing::QDirListing(QDirListing &&other)

    Move constructor. Moves \a other into this QDirListing.

//! [partially-formed]
    \note The moved-from object \a other is placed in a partially-formed state,
    in which the only valid operations are destruction and assignment of a new
    value.
//! [partially-formed]
*/

/*!
    \fn QDirListing &QDirListing::operator=(QDirListing &&other)

    Move-assigns \a other to this QDirListing.

    \include qdirlisting.cpp partially-formed
*/

/*!
    Destroys the QDirListing.
*/
QDirListing::~QDirListing()
{
    delete d;
}

/*!
    Returns the directory path used to construct this QDirListing.
*/
QString QDirListing::iteratorPath() const
{
    return d->initialEntryInfo.filePath();
}

/*!
    Returns the set of IteratorFlags used to construct this QDirListing.
*/
QDirListing::IteratorFlags QDirListing::iteratorFlags() const
{
    return d->iteratorFlags;
}

/*!
    Returns the list of file name glob filters used to construct this
    QDirListing.
*/
QStringList QDirListing::nameFilters() const
{
    return d->nameFilters;
}

/*!
    \class QDirListing::const_iterator
    \since 6.8
    \inmodule QtCore
    \ingroup  io

    The iterator type returned by QDirListing::cbegin().

//! [dirlisting-iterator-behavior]
    \list
    \li This is a forward-only, single-pass iterator (you cannot iterate
        directory entries in reverse order)
    \li Can't be copied, only \c{std::move()}d.
    \li \include qdirlisting.cpp post-increment-partially-formed
    \li Doesn't allow random access
    \li Can be used in ranged-for loops; or with C++20 std::ranges algorithms
        that don't require random access iterators
    \li Dereferencing a valid iterator returns a \c{const DirEntry &}
    \li (c)end() returns a \l QDirListing::sentinel that signals the end of
        the iteration. Dereferencing an iterator that compares equal to end()
        is undefined behavior
    \endlist
//! [dirlisting-iterator-behavior]

    \include qdirlisting.cpp ranges-algorithms-note

    \sa QDirListing, QDirListing::sentinel, QDirListing::DirEntry
*/

/*!
    \typealias QDirListing::const_iterator::reference

    A typedef for \c {const QDirListing::DirEntry &}.
*/

/*!
    \typealias QDirListing::const_iterator::pointer

    A typedef for \c {const QDirListing::DirEntry *}.
*/

/*!
    \class QDirListing::sentinel
    \since 6.8
    \inmodule QtCore
    \ingroup  io

    \l QDirListing returns an object of this type to signal the end of
    iteration. Dereferencing a \l QDirListing::const_iterator that is
    equal to \c sentinel{} is undefined behavior.

    \include qdirlisting.cpp ranges-algorithms-note

    \sa QDirListing, QDirListing::const_iterator, QDirListing::DirEntry
*/

/*!
    \fn QDirListing::const_iterator QDirListing::begin() const
    \fn QDirListing::const_iterator QDirListing::cbegin() const
    \fn QDirListing::sentinel QDirListing::end() const
    \fn QDirListing::sentinel QDirListing::cend() const

    (c)begin() returns a QDirListing::const_iterator that can be used to
    iterate over directory entries.

    \include qdirlisting.cpp dirlisting-iterator-behavior

    \note Each time (c)begin() is called on the same QDirListing object,
    the internal state is reset and the iteration starts anew.

    (Some of the above restrictions are dictated by the underlying system
    library functions' implementation).

    For example:
    \snippet code/src_corelib_io_qdirlisting.cpp 0

    Here's how to find and read all files filtered by name, recursively:
    \snippet code/src_corelib_io_qdirlisting.cpp 1

//! [ranges-algorithms-note]
    \note The "classical" STL algorithms don't support iterator/sentinel, so
    you need to use C++20 std::ranges algorithms for QDirListing, or else a
    3rd-party library that provides range-based algorithms in C++17.
//! [ranges-algorithms-note]

    \sa QDirListing::DirEntry
*/
QDirListing::const_iterator QDirListing::begin() const
{
    d->beginIterating();
    const_iterator it{d};
    ++it;
    return it;
}

/*!
    \fn const QDirListing::DirEntry &QDirListing::const_iterator::operator*() const

    Returns a \c{const  QDirListing::DirEntry &} of the directory entry this
    iterator points to.
*/

/*!
    \fn const QDirListing::DirEntry *QDirListing::const_iterator::operator->() const

    Returns a \c{const QDirListing::DirEntry *} to the directory entry this
    iterator points to.
*/

/*!
    \fn QDirListing::const_iterator::operator++()

    Pre-increment operator.
    Advances the iterator and returns a reference to it.
*/

/*!
    \fn void QDirListing::const_iterator::operator++(int)

    Post-increment operator.

    \include qdirlisting.cpp std-input-iterator-tag

//! [post-increment-partially-formed]
    The return value of post-increment on objects that model
    \c std::input_iterator is partially-formed (a copy of an iterator that
    has since been advanced), the only valid operations on such an object
    are destruction and assignment of a new iterator. Therefore the
    post-increment operator advances the iterator and returns \c void.
//! [post-increment-partially-formed]
*/

/*!
    \internal

    Implements the actual advancing. Not a member function to avoid forcing
    DirEntry objects (and therefore const_iterator ones) onto the stack.
*/
auto QDirListing::next(DirEntry dirEntry) -> DirEntry
{
    dirEntry.dirListPtr->advance();
    if (!dirEntry.dirListPtr->hasIterators())
        return {}; // All done, make `this` equal to the end() iterator
    return dirEntry;
}

/*!
    \class QDirListing::DirEntry
    \inmodule QtCore
    \ingroup  io

    Dereferencing a valid QDirListing::const_iterator returns a DirEntry
    object.

    DirEntry offers a subset of QFileInfo's API (for example, fileName(),
    filePath(), exists()). Internally, DirEntry only constructs a QFileInfo
    object if needed, that is, if the info hasn't been already fetched
    by other system functions. You can use DirEntry::fileInfo() to get a
    QFileInfo. For example:

    \snippet code/src_corelib_io_qdirlisting.cpp 3

    \snippet code/src_corelib_io_qdirlisting.cpp 4
*/

/*!
    \fn QFileInfo QDirListing::DirEntry::fileInfo() const
    \fn QString QDirListing::DirEntry::fileName() const
    \fn QString QDirListing::DirEntry::baseName() const
    \fn QString QDirListing::DirEntry::completeBaseName() const
    \fn QString QDirListing::DirEntry::suffix() const
    \fn QString QDirListing::DirEntry::bundleName() const
    \fn QString QDirListing::DirEntry::completeSuffix() const
    \fn QString QDirListing::DirEntry::filePath() const
    \fn QString QDirListing::DirEntry::canonicalFilePath() const
    \fn QString QDirListing::DirEntry::absoluteFilePath() const
    \fn QString QDirListing::DirEntry::absolutePath() const
    \fn bool QDirListing::DirEntry::isDir() const
    \fn bool QDirListing::DirEntry::isFile() const
    \fn bool QDirListing::DirEntry::isSymLink() const
    \fn bool QDirListing::DirEntry::exists() const
    \fn bool QDirListing::DirEntry::isHidden() const
    \fn bool QDirListing::DirEntry::isReadable() const
    \fn bool QDirListing::DirEntry::isWritable() const
    \fn bool QDirListing::DirEntry::isExecutable() const
    \fn qint64 QDirListing::DirEntry::size() const
    \fn QDateTime QDirListing::DirEntry::fileTime(QFile::FileTime type, const QTimeZone &tz) const
    \fn QDateTime QDirListing::DirEntry::birthTime(const QTimeZone &tz) const;
    \fn QDateTime QDirListing::DirEntry::metadataChangeTime(const QTimeZone &tz) const;
    \fn QDateTime QDirListing::DirEntry::lastModified(const QTimeZone &tz) const;
    \fn QDateTime QDirListing::DirEntry::lastRead(const QTimeZone &tz) const;

    See the QFileInfo methods with the same names.
*/

QFileInfo QDirListing::DirEntry::fileInfo() const
{
    return dirListPtr->currentEntryInfo.fileInfo();
}

QString QDirListing::DirEntry::fileName() const
{
    return dirListPtr->currentEntryInfo.fileName();
}

QString QDirListing::DirEntry::baseName() const
{
    return dirListPtr->currentEntryInfo.baseName();
}

QString QDirListing::DirEntry::completeBaseName() const
{
    return dirListPtr->currentEntryInfo.completeBaseName();
}

QString QDirListing::DirEntry::suffix() const
{
    return dirListPtr->currentEntryInfo.suffix();
}

QString QDirListing::DirEntry::bundleName() const
{
    return dirListPtr->currentEntryInfo.bundleName();
}

QString QDirListing::DirEntry::completeSuffix() const
{
    return dirListPtr->currentEntryInfo.completeSuffix();
}

QString QDirListing::DirEntry::filePath() const
{
    return dirListPtr->currentEntryInfo.filePath();
}

QString QDirListing::DirEntry::canonicalFilePath() const
{
    return dirListPtr->currentEntryInfo.canonicalFilePath();
}

QString QDirListing::DirEntry::absoluteFilePath() const
{
    return dirListPtr->currentEntryInfo.absoluteFilePath();
}

QString QDirListing::DirEntry::absolutePath() const
{
    return dirListPtr->currentEntryInfo.absolutePath();
}

bool QDirListing::DirEntry::isDir() const
{
    return dirListPtr->currentEntryInfo.isDir();
}

bool QDirListing::DirEntry::isFile() const
{
    return dirListPtr->currentEntryInfo.isFile();
}

bool QDirListing::DirEntry::isSymLink() const
{
    return dirListPtr->currentEntryInfo.isSymLink();
}

bool QDirListing::DirEntry::exists() const
{
    return dirListPtr->currentEntryInfo.exists();
}

bool QDirListing::DirEntry::isHidden() const
{
    return dirListPtr->currentEntryInfo.isHidden();
}

bool QDirListing::DirEntry::isReadable() const
{
    return dirListPtr->currentEntryInfo.isReadable();
}

bool QDirListing::DirEntry::isWritable() const
{
    return dirListPtr->currentEntryInfo.isWritable();
}

bool QDirListing::DirEntry::isExecutable() const
{
    return dirListPtr->currentEntryInfo.isExecutable();
}

qint64 QDirListing::DirEntry::size() const
{
    return dirListPtr->currentEntryInfo.size();
}

QDateTime QDirListing::DirEntry::fileTime(QFile::FileTime type, const QTimeZone &tz) const
{
    return dirListPtr->currentEntryInfo.fileTime(type, tz);
}

#ifndef QT_NO_DEBUG_STREAM
/*
    Since 6.11.
*/
QDebug operator<<(QDebug debug, QDirListing::IteratorFlags flags)
{
    QDebugStateSaver save(debug);
    debug.resetFormat();

    QByteArray ret = "QDirListing::IteratorFlags(";
    using F = QDirListing::IteratorFlag;
    if (flags.testFlags(F::Default))
        ret += "Default|";
    if (flags.testFlags(F::ExcludeFiles))
        ret += "ExcludeFiles|";
    if (flags.testFlags(F::ExcludeDirs))
        ret += "ExcludeDirs|";
    if (flags.testFlags(F::ExcludeOther))
        ret += "ExcludeOther|";
    if (flags.testFlags(F::ResolveSymlinks))
        ret += "ResolveSymlinks|";
    // if (flags.testFlags(F::FilesOnly)) // covered by F::ExcludeDirs | F::ExcludeOther
    //     ret += "FilesOnly|";
    // if (flags.testFlags(F::DirsOnly)) // covered by F::ExcludeFiles | F::ExcludeOther
    //     ret += "DirsOnly|";
    if (flags.testFlags(F::IncludeHidden))
        ret += "IncludeHidden|";
    if (flags.testFlags(F::IncludeDotAndDotDot))
        ret += "IncludeDotAndDotDot|";
    if (flags.testFlags(F::CaseSensitive))
        ret += "CaseSensitive|";
    if (flags.testFlags(F::Recursive))
        ret += "Recursive|";
    if (flags.testFlags(F::FollowDirSymlinks))
        ret += "FollowDirSymlinks|";
    if (flags.testFlags(F::IncludeBrokenSymlinks))
        ret += "IncludeBrokenSymlinks|";
    if (flags.testFlags(F::NoNameFiltersForDirs))
        ret += "NoNameFiltersForDirs|";

    if (ret.endsWith('|'))
        ret.chop(1);
    ret += ')';

    debug.noquote() << ret;
    return debug;
}
#endif

QT_END_NAMESPACE
