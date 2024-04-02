// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2024 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*!
    \since 6.8
    \class QDirListing
    \inmodule QtCore
    \brief The QDirListing class provides an STL-style iterator for directory entries.

    You can use QDirListing to navigate entries of a directory one at a time.
    It is similar to QDir::entryList() and QDir::entryInfoList(), but because
    it lists entries one at a time instead of all at once, it scales better
    and is more suitable for large directories. It also supports listing
    directory contents recursively, and following symbolic links. Unlike
    QDir::entryList(), QDirListing does not support sorting.

    The QDirListing constructor takes a QDir or a directory path as
    argument. Here's how to iterate over all entries recursively:

    \snippet code/src_corelib_io_qdirlisting.cpp 0

    Here's how to find and read all files filtered by name, recursively:

    \snippet code/src_corelib_io_qdirlisting.cpp 1

    Iterators constructed by QDirListing (QDirListing::const_iterator) are
    forward-only (you cannot iterate directories in reverse order) and don't
    allow random access. They can be used in ranged-for loops (or with STL
    alogrithms that don't require random access iterators). Dereferencing
    a valid iterator returns a QDirListing::DirEntry object. The (c)end()
    iterator marks the end of the iteration. Dereferencing the end iterator
    is undefiend behavior.

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

    This enum class describes flags can be used to configure the behavior of
    QDirListing. These flags can be bitwise OR'ed together.

    \value NoFlag The default value, representing no flags. The iterator
    will return entries for the assigned path.

    \value FollowSymlinks When combined with Recursive, this flag enables
    iterating through all subdirectories of the assigned path, following
    all symbolic links. Symbolic link loops (e.g., link => . or link =>
    ..) are automatically detected and ignored.

    \value Recursive List entries inside all subdirectories as well.
*/

#include "qdirlisting.h"
#include "qdirentryinfo_p.h"

#include "qdir_p.h"
#include "qabstractfileengine_p.h"

#include <QtCore/qset.h>

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
#include <vector>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QDirListingPrivate
{
public:
    void init(bool resolveEngine);
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
    QDir::Filters filters;
    QDirListing::IteratorFlags iteratorFlags;
    QDirEntryInfo currentEntryInfo;

#if QT_CONFIG(regularexpression)
    QList<QRegularExpression> nameRegExps;
#endif

    using FEngineIteratorPtr = std::unique_ptr<QAbstractFileEngineIterator>;
    std::vector<FEngineIteratorPtr> fileEngineIterators;
#ifndef QT_NO_FILESYSTEMITERATOR
    using FsIteratorPtr = std::unique_ptr<QFileSystemIterator>;
    std::vector<FsIteratorPtr> nativeIterators;
#endif

    // Loop protection
    QDuplicateTracker<QString> visitedLinks;
};

void QDirListingPrivate::init(bool resolveEngine = true)
{
    if (nameFilters.contains("*"_L1))
        nameFilters.clear();

    if (filters == QDir::NoFilter)
        filters = QDir::AllEntries;

#if QT_CONFIG(regularexpression)
    nameRegExps.reserve(nameFilters.size());
    const auto cs = filters.testAnyFlags(QDir::CaseSensitive) ? Qt::CaseSensitive
                                                              : Qt::CaseInsensitive;
    for (const auto &filter : nameFilters)
        nameRegExps.emplace_back(QRegularExpression::fromWildcard(filter, cs));
#endif

    if (resolveEngine) {
        engine = QFileSystemEngine::createLegacyEngine(initialEntryInfo.entry,
                                                       initialEntryInfo.metaData);
    }
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


    if (iteratorFlags.testAnyFlags(QDirListing::IteratorFlag::FollowSymlinks)) {
        // Stop link loops
        if (visitedLinks.hasSeen(entryInfo.canonicalFilePath()))
            return;
    }

    if (engine) {
        engine->setFileName(path);
        if (auto it = engine->beginEntryList(path, filters, nameFilters)) {
            fileEngineIterators.emplace_back(std::move(it));
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
        nativeIterators.emplace_back(std::make_unique<QFileSystemIterator>(*fentry, filters));
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
            while (it = fileEngineIterators.back().get(), it->advance()) {
                QDirEntryInfo entryInfo;
                entryInfo.fileInfoOpt = it->currentFileInfo();
                if (entryMatches(entryInfo)) {
                    currentEntryInfo = std::move(entryInfo);
                    return;
                }
            }

            fileEngineIterators.pop_back();
        }
    } else {
#ifndef QT_NO_FILESYSTEMITERATOR
        QDirEntryInfo entryInfo;
        while (!nativeIterators.empty()) {
            // Find the next valid iterator that matches the filters.
            QFileSystemIterator *it;
            while (it = nativeIterators.back().get(),
                   it->advance(entryInfo.entry, entryInfo.metaData)) {
                if (entryMatches(entryInfo)) {
                    currentEntryInfo = std::move(entryInfo);
                    return;
                }
                entryInfo = {};
            }

            nativeIterators.pop_back();
        }
#endif
    }
}

void QDirListingPrivate::checkAndPushDirectory(QDirEntryInfo &entryInfo)
{
    using F = QDirListing::IteratorFlag;
    // If we're doing flat iteration, we're done.
    if (!iteratorFlags.testAnyFlags(F::Recursive))
        return;

    // Never follow non-directory entries
    if (!entryInfo.isDir())
        return;

    // Follow symlinks only when asked
    if (!iteratorFlags.testAnyFlags(F::FollowSymlinks) && entryInfo.isSymLink())
        return;

    // Never follow . and ..
    const QString &fileName = entryInfo.fileName();
    if ("."_L1 == fileName || ".."_L1 == fileName)
        return;

    // No hidden directories unless requested
    if (!filters.testAnyFlags(QDir::AllDirs | QDir::Hidden) && entryInfo.isHidden())
        return;

    pushDirectory(entryInfo);
}

/*!
    \internal

    This functions returns \c true if the current entry matches the filters
    (i.e., the current entry will be returned as part of the directory
    iteration); otherwise, \c false is returned.
*/
bool QDirListingPrivate::matchesFilters(QDirEntryInfo &entryInfo) const
{
    const QString &fileName = entryInfo.fileName();
    if (fileName.isEmpty())
        return false;

    // filter . and ..?
    const qsizetype fileNameSize = fileName.size();
    const bool dotOrDotDot = fileName[0] == u'.'
                             && ((fileNameSize == 1)
                                 ||(fileNameSize == 2 && fileName[1] == u'.'));
    if ((filters & QDir::NoDot) && dotOrDotDot && fileNameSize == 1)
        return false;
    if ((filters & QDir::NoDotDot) && dotOrDotDot && fileNameSize == 2)
        return false;

    // name filter
#if QT_CONFIG(regularexpression)
    // Pass all entries through name filters, except dirs if AllDirs is set
    if (!nameRegExps.isEmpty() && !(filters.testAnyFlags(QDir::AllDirs) && entryInfo.isDir())) {
        auto regexMatchesName = [&fileName](const auto &re) {
            return re.match(fileName).hasMatch();
        };
        if (std::none_of(nameRegExps.cbegin(), nameRegExps.cend(), regexMatchesName))
            return false;
    }
#endif
    // skip symlinks
    const bool skipSymlinks = filters.testAnyFlag(QDir::NoSymLinks);
    const bool includeSystem = filters.testAnyFlag(QDir::System);
    if (skipSymlinks && entryInfo.isSymLink()) {
        // The only reason to save this file is if it is a broken link and we are requesting system files.
        if (!includeSystem || entryInfo.exists())
            return false;
    }

    // filter hidden
    const bool includeHidden = filters.testAnyFlag(QDir::Hidden);
    if (!includeHidden && !dotOrDotDot && entryInfo.isHidden())
        return false;

    // filter system files
    if (!includeSystem) {
        if (!entryInfo.isFile() && !entryInfo.isDir() && !entryInfo.isSymLink())
            return false;
        if (entryInfo.isSymLink() && !entryInfo.exists())
            return false;
    }

    // skip directories
    const bool skipDirs = !(filters & (QDir::Dirs | QDir::AllDirs));
    if (skipDirs && entryInfo.isDir())
        return false;

    // skip files
    const bool skipFiles    = !(filters & QDir::Files);
    if (skipFiles && entryInfo.isFile())
        // Basically we need a reason not to exclude this file otherwise we just eliminate it.
        return false;

    // filter permissions
    const auto perms = filters & QDir::PermissionMask;
    const bool filterPermissions = perms != 0 && perms != QDir::PermissionMask;
    if (filterPermissions) {
        const bool doWritable = filters.testAnyFlags(QDir::Writable);
        const bool doExecutable = filters.testAnyFlags(QDir::Executable);
        const bool doReadable = filters.testAnyFlags(QDir::Readable);
        if ((doReadable && !entryInfo.isReadable())
            || (doWritable && !entryInfo.isWritable())
            || (doExecutable && !entryInfo.isExecutable())) {
            return false;
        }
    }

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
    Constructs a QDirListing that can iterate over \a dir's entries, using
    \a dir's name filters and the QDir::Filters set in \a dir. You can pass
    options via \a flags to decide how the directory should be iterated.

    By default, \a flags is NoIteratorFlags, which provides the same behavior
    as in QDir::entryList().

    The sorting in \a dir is ignored.

    \note To list symlinks that point to non existing files, QDir::System
    must be set in \a dir's QDir::Filters.

    \sa hasNext(), next(), IteratorFlags
*/
QDirListing::QDirListing(const QDir &dir, IteratorFlags flags)
    : d(new QDirListingPrivate)
{
    const QDirPrivate *other = dir.d_ptr.constData();
    d->initialEntryInfo.entry = other->dirEntry;
    d->nameFilters = other->nameFilters;
    d->filters = other->filters;
    d->iteratorFlags = flags;
    const bool resolveEngine = other->fileEngine ? true : false;
    d->init(resolveEngine);
}

/*!
    Constructs a QDirListing that can iterate over \a path. Entries are
    filtered according to \a filters. You can pass options via \a flags to
    decide how the directory should be iterated.

    By default, \a filters is QDir::NoFilter, and \a flags is NoIteratorFlags,
    which provides the same behavior as in QDir::entryList().

    \note To list symlinks that point to non existing files, QDir::System
    must be set in \a filters.

    \sa hasNext(), next(), IteratorFlags
*/
QDirListing::QDirListing(const QString &path, QDir::Filters filters, IteratorFlags flags)
    : d(new QDirListingPrivate)
{
    d->initialEntryInfo.entry = QFileSystemEntry(path);
    d->filters = filters;
    d->iteratorFlags = flags;
    d->init();
}

/*!
    Constructs a QDirListing that can iterate over \a path. You can pass
    options via \a flags to decide how the directory should be iterated.

    By default, \a flags is NoIteratorFlags, which provides the same behavior
    as in QDir::entryList().

    \sa hasNext(), next(), IteratorFlags
*/
QDirListing::QDirListing(const QString &path, IteratorFlags flags)
    : d(new QDirListingPrivate)
{
    d->initialEntryInfo.entry = QFileSystemEntry(path);
    d->filters = QDir::NoFilter;
    d->iteratorFlags = flags;
    d->init();
}

/*!
    Constructs a QDirListing that can iterate over \a path, using \a
    nameFilters and \a filters. You can pass options via \a flags to decide
    how the directory should be iterated.

    By default, \a flags is NoIteratorFlags, which provides the same behavior
    as QDir::entryList().

    For example, the following iterator could be used to iterate over audio
    files:

    \snippet code/src_corelib_io_qdirlisting.cpp 2

    \note To list symlinks that point to non existing files, QDir::System
    must be set in \a flags.

    \sa hasNext(), next(), IteratorFlags, QDir::setNameFilters()
*/
QDirListing::QDirListing(const QString &path, const QStringList &nameFilters, QDir::Filters filters,
                         IteratorFlags flags)
    : d(new QDirListingPrivate)
{
    d->initialEntryInfo.entry = QFileSystemEntry(path);
    d->nameFilters = nameFilters;
    d->filters = filters;
    d->iteratorFlags = flags;
    d->init();
}

/*!
    Destroys the QDirListing.
*/
QDirListing::~QDirListing() = default;

/*!
    Returns the directory path used to construct this QDirListing.
*/
QString QDirListing::iteratorPath() const
{
    return d->initialEntryInfo.filePath();
}

/*!
    \fn QDirListing::const_iterator QDirListing::begin() const
    \fn QDirListing::const_iterator QDirListing::cbegin() const
    \fn QDirListing::const_iterator QDirListing::end() const
    \fn QDirListing::const_iterator QDirListing::cend() const

    begin()/cbegin() returns a QDirListing::const_iterator that enables
    iterating over directory entries using a ranged-for loop; dereferencing
    this iterator returns a \c{const QFileInfo &}.

    end()/cend() return a sentinel const_iterator that signals the end of
    the iteration. Dereferencing this iterator is undefined behavior.

    For example:
    \snippet code/src_corelib_io_qdirlisting.cpp 0

    Here's how to find and read all files filtered by name, recursively:
    \snippet code/src_corelib_io_qdirlisting.cpp 1

    \note This is a forward-only iterator, every time begin()/cbegin() is
    called (on the same QDirListing object), the internal state is reset and
    the iteration starts anew.

    \sa fileInfo(), fileName(), filePath()
*/
QDirListing::const_iterator QDirListing::begin() const
{
    d->beginIterating();
    const_iterator it{d.get()};
    return ++it;
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
    Advances the iterator and returns a reference to it.
*/
QDirListing::const_iterator &QDirListing::const_iterator::operator++()
{
    dirListPtr->advance();
    if (!dirListPtr->hasIterators())
        *this = {}; // All done, make `this` the end() iterator
    return *this;
}

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

QT_END_NAMESPACE
