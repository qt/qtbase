// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"
#include "qdir.h"
#include "qdir_p.h"
#include "qabstractfileengine_p.h"
#include "qfsfileengine_p.h"
#ifndef QT_NO_DEBUG_STREAM
#include "qdebug.h"
#endif
#include "qdiriterator.h"
#include "qdatetime.h"
#include "qstring.h"
#if QT_CONFIG(regularexpression)
#  include <qregularexpression.h>
#endif
#include "qvarlengtharray.h"
#include "qfilesystementry_p.h"
#include "qfilesystemmetadata_p.h"
#include "qfilesystemengine_p.h"
#include <qstringbuilder.h>

#ifndef QT_BOOTSTRAPPED
#  include <qcollator.h>
#  include "qreadwritelock.h"
#  include "qmutex.h"
#endif

#include <algorithm>
#include <memory>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if defined(Q_OS_WIN)
static QString driveSpec(const QString &path)
{
    if (path.size() < 2)
        return QString();
    char c = path.at(0).toLatin1();
    if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
        return QString();
    if (path.at(1).toLatin1() != ':')
        return QString();
    return path.mid(0, 2);
}
#endif

enum {
#if defined(Q_OS_WIN)
    OSSupportsUncPaths = true
#else
    OSSupportsUncPaths = false
#endif
};

// Return the length of the root part of an absolute path, for use by cleanPath(), cd().
static qsizetype rootLength(QStringView name, bool allowUncPaths)
{
    const qsizetype len = name.size();
    // starts with double slash
    if (allowUncPaths && name.startsWith("//"_L1)) {
        // Server name '//server/path' is part of the prefix.
        const qsizetype nextSlash = name.indexOf(u'/', 2);
        return nextSlash >= 0 ? nextSlash + 1 : len;
    }
#if defined(Q_OS_WIN)
    if (len >= 2 && name.at(1) == u':') {
        // Handle a possible drive letter
        return len > 2 && name.at(2) == u'/' ? 3 : 2;
    }
#endif
    if (name.at(0) == u'/')
        return 1;
    return 0;
}

//************* QDirPrivate
QDirPrivate::QDirPrivate(const QString &path, const QStringList &nameFilters_,
                         QDir::SortFlags sort_, QDir::Filters filters_)
    : QSharedData(), nameFilters(nameFilters_), sort(sort_), filters(filters_)
{
    setPath(path.isEmpty() ? QString::fromLatin1(".") : path);

    auto isEmpty = [](const auto &e) { return e.isEmpty(); };
    const bool empty = std::all_of(nameFilters.cbegin(), nameFilters.cend(), isEmpty);
    if (empty)
        nameFilters = QStringList(QString::fromLatin1("*"));
}

QDirPrivate::QDirPrivate(const QDirPrivate &copy)
    : QSharedData(copy),
      // mutex is not copied
      nameFilters(copy.nameFilters),
      sort(copy.sort),
      filters(copy.filters),
      // fileEngine is not copied
      dirEntry(copy.dirEntry)
{
    QMutexLocker locker(&copy.fileCache.mutex);
    fileCache.fileListsInitialized = copy.fileCache.fileListsInitialized.load();
    fileCache.files = copy.fileCache.files;
    fileCache.fileInfos = copy.fileCache.fileInfos;
    fileCache.absoluteDirEntry = copy.fileCache.absoluteDirEntry;
    fileCache.metaData = copy.fileCache.metaData;
}

bool QDirPrivate::exists() const
{
    if (!fileEngine) {
        QMutexLocker locker(&fileCache.mutex);
        QFileSystemEngine::fillMetaData(
                dirEntry, fileCache.metaData,
                QFileSystemMetaData::ExistsAttribute
                        | QFileSystemMetaData::DirectoryType); // always stat
        return fileCache.metaData.exists() && fileCache.metaData.isDirectory();
    }
    const QAbstractFileEngine::FileFlags info =
        fileEngine->fileFlags(QAbstractFileEngine::DirectoryType
                                       | QAbstractFileEngine::ExistsFlag
                                       | QAbstractFileEngine::Refresh);
    if (!(info & QAbstractFileEngine::DirectoryType))
        return false;
    return info.testAnyFlag(QAbstractFileEngine::ExistsFlag);
}

// static
inline QChar QDirPrivate::getFilterSepChar(const QString &nameFilter)
{
    QChar sep(u';');
    qsizetype i = nameFilter.indexOf(sep, 0);
    if (i == -1 && nameFilter.indexOf(u' ', 0) != -1)
        sep = QChar(u' ');
    return sep;
}

// static
inline QStringList QDirPrivate::splitFilters(const QString &nameFilter, QChar sep)
{
    if (sep.isNull())
        sep = getFilterSepChar(nameFilter);
    QStringList ret;
    for (auto e : qTokenize(nameFilter, sep))
        ret.append(e.trimmed().toString());
    return ret;
}

inline void QDirPrivate::setPath(const QString &path)
{
    QString p = QDir::fromNativeSeparators(path);
    if (p.endsWith(u'/')
            && p.size() > 1
#if defined(Q_OS_WIN)
        && (!(p.length() == 3 && p.at(1).unicode() == ':' && p.at(0).isLetter()))
#endif
    ) {
            p.truncate(p.size() - 1);
    }
    dirEntry = QFileSystemEntry(p, QFileSystemEntry::FromInternalPath());
    clearCache(IncludingMetaData);
    fileCache.absoluteDirEntry = QFileSystemEntry();
}

inline QString QDirPrivate::resolveAbsoluteEntry() const
{
    QMutexLocker locker(&fileCache.mutex);
    if (!fileCache.absoluteDirEntry.isEmpty())
        return fileCache.absoluteDirEntry.filePath();

    if (dirEntry.isEmpty())
        return dirEntry.filePath();

    QString absoluteName;
    if (!fileEngine) {
        if (!dirEntry.isRelative() && dirEntry.isClean()) {
            fileCache.absoluteDirEntry = dirEntry;
            return dirEntry.filePath();
        }

        absoluteName = QFileSystemEngine::absoluteName(dirEntry).filePath();
    } else {
        absoluteName = fileEngine->fileName(QAbstractFileEngine::AbsoluteName);
    }
    auto absoluteFileSystemEntry =
            QFileSystemEntry(QDir::cleanPath(absoluteName), QFileSystemEntry::FromInternalPath());
    fileCache.absoluteDirEntry = absoluteFileSystemEntry;
    return absoluteFileSystemEntry.filePath();
}

/* For sorting */
struct QDirSortItem
{
    QDirSortItem() = default;
    QDirSortItem(const QFileInfo &fi, QDir::SortFlags sort)
        : item(fi)
    {
        // A dir e.g. "dirA.bar" doesn't have actually have an extension/suffix, when
        // sorting by type such "suffix" should be ignored but that would complicate
        // the code and uses can change the behavior by setting DirsFirst/DirsLast
        if (sort.testAnyFlag(QDir::Type))
            suffix_cache = item.suffix();
    }

    mutable QString filename_cache;
    QString suffix_cache;
    QFileInfo item;
};

class QDirSortItemComparator
{
    QDir::SortFlags qt_cmp_si_sort_flags;

#ifndef QT_BOOTSTRAPPED
    QCollator *collator = nullptr;
#endif
public:
#ifndef QT_BOOTSTRAPPED
    QDirSortItemComparator(QDir::SortFlags flags, QCollator *coll = nullptr)
        : qt_cmp_si_sort_flags(flags), collator(coll)
    {
        Q_ASSERT(!qt_cmp_si_sort_flags.testAnyFlag(QDir::LocaleAware) || collator);

        if (collator && qt_cmp_si_sort_flags.testAnyFlag(QDir::IgnoreCase))
            collator->setCaseSensitivity(Qt::CaseInsensitive);
    }
#else
    QDirSortItemComparator(QDir::SortFlags flags)
        : qt_cmp_si_sort_flags(flags)
    {
    }
#endif
    bool operator()(const QDirSortItem &, const QDirSortItem &) const;

    int compareStrings(const QString &a, const QString &b, Qt::CaseSensitivity cs) const
    {
#ifndef QT_BOOTSTRAPPED
        if (collator)
            return collator->compare(a, b);
#endif
        return a.compare(b, cs);
    }
};

bool QDirSortItemComparator::operator()(const QDirSortItem &n1, const QDirSortItem &n2) const
{
    const QDirSortItem* f1 = &n1;
    const QDirSortItem* f2 = &n2;

    if ((qt_cmp_si_sort_flags & QDir::DirsFirst) && (f1->item.isDir() != f2->item.isDir()))
        return f1->item.isDir();
    if ((qt_cmp_si_sort_flags & QDir::DirsLast) && (f1->item.isDir() != f2->item.isDir()))
        return !f1->item.isDir();

    const bool ic = qt_cmp_si_sort_flags.testAnyFlag(QDir::IgnoreCase);
    const auto qtcase = ic ? Qt::CaseInsensitive : Qt::CaseSensitive;

    qint64 r = 0;
    int sortBy = ((qt_cmp_si_sort_flags & QDir::SortByMask)
                 | (qt_cmp_si_sort_flags & QDir::Type)).toInt();

    switch (sortBy) {
      case QDir::Time: {
        const QDateTime firstModified = f1->item.lastModified(QTimeZone::UTC);
        const QDateTime secondModified = f2->item.lastModified(QTimeZone::UTC);
        r = firstModified.msecsTo(secondModified);
        break;
      }
      case QDir::Size:
          r = f2->item.size() - f1->item.size();
        break;
    case QDir::Type:
        r = compareStrings(f1->suffix_cache, f2->suffix_cache, qtcase);
        break;
      default:
        ;
    }

    if (r == 0 && sortBy != QDir::Unsorted) {
        // Still not sorted - sort by name

        if (f1->filename_cache.isNull())
            f1->filename_cache = f1->item.fileName();
        if (f2->filename_cache.isNull())
            f2->filename_cache = f2->item.fileName();

        r = compareStrings(f1->filename_cache, f2->filename_cache, qtcase);
    }
    if (qt_cmp_si_sort_flags & QDir::Reversed)
        return r > 0;
    return r < 0;
}

inline void QDirPrivate::sortFileList(QDir::SortFlags sort, const QFileInfoList &l,
                                      QStringList *names, QFileInfoList *infos)
{
    Q_ASSERT(names || infos);
    Q_ASSERT(!infos || infos->isEmpty());
    Q_ASSERT(!names || names->isEmpty());

    const qsizetype n = l.size();
    if (n == 0)
        return;

    if (n == 1 || (sort & QDir::SortByMask) == QDir::Unsorted) {
        if (infos)
            *infos = l;

        if (names) {
            for (const QFileInfo &fi : l)
                names->append(fi.fileName());
        }
    } else {
        QScopedArrayPointer<QDirSortItem> si(new QDirSortItem[n]);
        for (qsizetype i = 0; i < n; ++i)
            si[i] = QDirSortItem{l.at(i), sort};

#ifndef QT_BOOTSTRAPPED
    if (sort.testAnyFlag(QDir::LocaleAware)) {
            QCollator coll;
            std::sort(si.data(), si.data() + n, QDirSortItemComparator(sort, &coll));
        } else {
            std::sort(si.data(), si.data() + n, QDirSortItemComparator(sort));
        }
#else
        std::sort(si.data(), si.data() + n, QDirSortItemComparator(sort));
#endif // QT_BOOTSTRAPPED

        // put them back in the list(s)
        for (qsizetype i = 0; i < n; ++i) {
            auto &fileInfo = si[i].item;
            if (infos)
                infos->append(fileInfo);
            if (names) {
                const bool cached = !si[i].filename_cache.isNull();
                names->append(cached ? si[i].filename_cache : fileInfo.fileName());
            }
        }
    }
}

inline void QDirPrivate::initFileLists(const QDir &dir) const
{
    QMutexLocker locker(&fileCache.mutex);
    if (!fileCache.fileListsInitialized) {
        QFileInfoList l;
        QDirIterator it(dir);
        while (it.hasNext())
            l.append(it.nextFileInfo());

        sortFileList(sort, l, &fileCache.files, &fileCache.fileInfos);
        fileCache.fileListsInitialized = true;
    }
}

inline void QDirPrivate::clearCache(MetaDataClearing mode)
{
    QMutexLocker locker(&fileCache.mutex);
    if (mode == IncludingMetaData)
        fileCache.metaData.clear();
    fileCache.fileListsInitialized = false;
    fileCache.files.clear();
    fileCache.fileInfos.clear();
    fileEngine.reset(
            QFileSystemEngine::resolveEntryAndCreateLegacyEngine(dirEntry, fileCache.metaData));
}

/*!
    \class QDir
    \inmodule QtCore
    \brief The QDir class provides access to directory structures and their contents.

    \ingroup io
    \ingroup shared
    \reentrant


    A QDir is used to manipulate path names, access information
    regarding paths and files, and manipulate the underlying file
    system. It can also be used to access Qt's \l{resource system}.

    Qt uses "/" as a universal directory separator in the same way
    that "/" is used as a path separator in URLs. If you always use
    "/" as a directory separator, Qt will translate your paths to
    conform to the underlying operating system.

    A QDir can point to a file using either a relative or an absolute
    path. Absolute paths begin with the directory separator
    (optionally preceded by a drive specification under Windows).
    Relative file names begin with a directory name or a file name and
    specify a path relative to the current directory.

    Examples of absolute paths:

    \snippet code/src_corelib_io_qdir.cpp 0

    On Windows, the second example above will be translated to
    \c{C:\Users} when used to access files.

    Examples of relative paths:

    \snippet code/src_corelib_io_qdir.cpp 1

    You can use the isRelative() or isAbsolute() functions to check if
    a QDir is using a relative or an absolute file path. Call
    makeAbsolute() to convert a relative QDir to an absolute one.

    \note Paths starting with a colon (\e{:}) are always considered
    absolute, as they denote a QResource.

    \section1 Navigation and Directory Operations

    A directory's path can be obtained with the path() function, and
    a new path set with the setPath() function. The absolute path to
    a directory is found by calling absolutePath().

    The name of a directory is found using the dirName() function. This
    typically returns the last element in the absolute path that specifies
    the location of the directory. However, it can also return "." if
    the QDir represents the current directory.

    \snippet code/src_corelib_io_qdir.cpp 2

    The path for a directory can also be changed with the cd() and cdUp()
    functions, both of which operate like familiar shell commands.
    When cd() is called with the name of an existing directory, the QDir
    object changes directory so that it represents that directory instead.
    The cdUp() function changes the directory of the QDir object so that
    it refers to its parent directory; i.e. cd("..") is equivalent to
    cdUp().

    Directories can be created with mkdir(), renamed with rename(), and
    removed with rmdir().

    You can test for the presence of a directory with a given name by
    using exists(), and the properties of a directory can be tested with
    isReadable(), isAbsolute(), isRelative(), and isRoot().

    The refresh() function re-reads the directory's data from disk.

    \section1 Files and Directory Contents

    Directories contain a number of entries, representing files,
    directories, and symbolic links. The number of entries in a
    directory is returned by count().
    A string list of the names of all the entries in a directory can be
    obtained with entryList(). If you need information about each
    entry, use entryInfoList() to obtain a list of QFileInfo objects.

    Paths to files and directories within a directory can be
    constructed using filePath() and absoluteFilePath().
    The filePath() function returns a path to the specified file
    or directory relative to the path of the QDir object;
    absoluteFilePath() returns an absolute path to the specified
    file or directory. Neither of these functions checks for the
    existence of files or directory; they only construct paths.

    \snippet code/src_corelib_io_qdir.cpp 3

    Files can be removed by using the remove() function. Directories
    cannot be removed in the same way as files; use rmdir() to remove
    them instead.

    It is possible to reduce the number of entries returned by
    entryList() and entryInfoList() by applying filters to a QDir object.
    You can apply a name filter to specify a pattern with wildcards that
    file names need to match, an attribute filter that selects properties
    of entries and can distinguish between files and directories, and a
    sort order.

    Name filters are lists of strings that are passed to setNameFilters().
    Attribute filters consist of a bitwise OR combination of Filters, and
    these are specified when calling setFilter().
    The sort order is specified using setSorting() with a bitwise OR
    combination of SortFlags.

    You can test to see if a filename matches a filter using the match()
    function.

    Filter and sort order flags may also be specified when calling
    entryList() and entryInfoList() in order to override previously defined
    behavior.

    \section1 The Current Directory and Other Special Paths

    Access to some common directories is provided with a number of static
    functions that return QDir objects. There are also corresponding functions
    for these that return strings:

    \table
    \header \li QDir      \li QString         \li Return Value
    \row    \li current() \li currentPath()   \li The application's working directory
    \row    \li home()    \li homePath()      \li The user's home directory
    \row    \li root()    \li rootPath()      \li The root directory
    \row    \li temp()    \li tempPath()      \li The system's temporary directory
    \endtable

    The setCurrent() static function can also be used to set the application's
    working directory.

    If you want to find the directory containing the application's executable,
    see \l{QCoreApplication::applicationDirPath()}.

    The drives() static function provides a list of root directories for each
    device that contains a filing system. On Unix systems this returns a list
    containing a single root directory "/"; on Windows the list will usually
    contain \c{C:/}, and possibly other drive letters such as \c{D:/}, depending
    on the configuration of the user's system.

    \section1 Path Manipulation and Strings

    Paths containing "." elements that reference the current directory at that
    point in the path, ".." elements that reference the parent directory, and
    symbolic links can be reduced to a canonical form using the canonicalPath()
    function.

    Paths can also be simplified by using cleanPath() to remove redundant "/"
    and ".." elements.

    It is sometimes necessary to be able to show a path in the native
    representation for the user's platform. The static toNativeSeparators()
    function returns a copy of the specified path in which each directory
    separator is replaced by the appropriate separator for the underlying
    operating system.

    \section1 Examples

    Check if a directory exists:

    \snippet code/src_corelib_io_qdir.cpp 4

    (We could also use one of the static convenience functions
    QFileInfo::exists() or QFile::exists().)

    Traversing directories and reading a file:

    \snippet code/src_corelib_io_qdir.cpp 5

    A program that lists all the files in the current directory
    (excluding symbolic links), sorted by size, smallest first:

    \snippet qdir-listfiles/main.cpp 0

    \section1 Platform Specific Issues

    \include android-content-uri-limitations.qdocinc

    \sa QFileInfo, QFile, QFileDialog, QCoreApplication::applicationDirPath(),
        {Fetch More Example}
*/

/*!
    \fn QDir &QDir::operator=(QDir &&other)

    Move-assigns \a other to this QDir instance.

    \since 5.2
*/

/*!
    \internal
*/
QDir::QDir(QDirPrivate &p) : d_ptr(&p)
{
}

/*!
    Constructs a QDir pointing to the given directory \a path. If path
    is empty the program's working directory, ("."), is used.

    \sa currentPath()
*/
QDir::QDir(const QString &path) : d_ptr(new QDirPrivate(path))
{
}

/*!
    Constructs a QDir with path \a path, that filters its entries by
    name using \a nameFilter and by attributes using \a filters. It
    also sorts the names using \a sort.

    The default \a nameFilter is an empty string, which excludes
    nothing; the default \a filters is \l AllEntries, which also
    excludes nothing. The default \a sort is \l Name | \l IgnoreCase,
    i.e. sort by name case-insensitively.

    If \a path is an empty string, QDir uses "." (the current
    directory). If \a nameFilter is an empty string, QDir uses the
    name filter "*" (all files).

    \note \a path need not exist.

    \sa exists(), setPath(), setNameFilters(), setFilter(), setSorting()
*/
QDir::QDir(const QString &path, const QString &nameFilter,
           SortFlags sort, Filters filters)
    : d_ptr(new QDirPrivate(path, QDir::nameFiltersFromString(nameFilter), sort, filters))
{
}

/*!
    Constructs a QDir object that is a copy of the QDir object for
    directory \a dir.

    \sa operator=()
*/
QDir::QDir(const QDir &dir)
    : d_ptr(dir.d_ptr)
{
}

/*!
    Destroys the QDir object frees up its resources. This has no
    effect on the underlying directory in the file system.
*/
QDir::~QDir()
{
}

/*!
    Sets the path of the directory to \a path. The path is cleaned of
    redundant ".", ".." and of multiple separators. No check is made
    to see whether a directory with this path actually exists; but you
    can check for yourself using exists().

    The path can be either absolute or relative. Absolute paths begin
    with the directory separator "/" (optionally preceded by a drive
    specification under Windows). Relative file names begin with a
    directory name or a file name and specify a path relative to the
    current directory. An example of an absolute path is the string
    "/tmp/quartz", a relative path might look like "src/fatlib".

    \sa path(), absolutePath(), exists(), cleanPath(), dirName(),
      absoluteFilePath(), isRelative(), makeAbsolute()
*/
void QDir::setPath(const QString &path)
{
    d_ptr->setPath(path);
}

/*!
    Returns the path. This may contain symbolic links, but never
    contains redundant ".", ".." or multiple separators.

    The returned path can be either absolute or relative (see
    setPath()).

    \sa setPath(), absolutePath(), exists(), cleanPath(), dirName(),
    absoluteFilePath(), toNativeSeparators(), makeAbsolute()
*/
QString QDir::path() const
{
    Q_D(const QDir);
    return d->dirEntry.filePath();
}

/*!
    Returns the absolute path (a path that starts with "/" or with a
    drive specification), which may contain symbolic links, but never
    contains redundant ".", ".." or multiple separators.

    \sa setPath(), canonicalPath(), exists(), cleanPath(),
    dirName(), absoluteFilePath()
*/
QString QDir::absolutePath() const
{
    Q_D(const QDir);
    if (!d->fileEngine)
        return d->resolveAbsoluteEntry();

    return d->fileEngine->fileName(QAbstractFileEngine::AbsoluteName);
}

/*!
    Returns the canonical path, i.e. a path without symbolic links or
    redundant "." or ".." elements.

    On systems that do not have symbolic links this function will
    always return the same string that absolutePath() returns. If the
    canonical path does not exist (normally due to dangling symbolic
    links) canonicalPath() returns an empty string.

    Example:

    \snippet code/src_corelib_io_qdir.cpp 6

    \sa path(), absolutePath(), exists(), cleanPath(), dirName(),
        absoluteFilePath()
*/
QString QDir::canonicalPath() const
{
    Q_D(const QDir);
    if (!d->fileEngine) {
        QMutexLocker locker(&d->fileCache.mutex);
        QFileSystemEntry answer =
                QFileSystemEngine::canonicalName(d->dirEntry, d->fileCache.metaData);
        return answer.filePath();
    }
    return d->fileEngine->fileName(QAbstractFileEngine::CanonicalName);
}

/*!
    Returns the name of the directory; this is \e not the same as the
    path, e.g. a directory with the name "mail", might have the path
    "/var/spool/mail". If the directory has no name (e.g. it is the
    root directory) an empty string is returned.

    No check is made to ensure that a directory with this name
    actually exists; but see exists().

    \sa path(), filePath(), absolutePath(), absoluteFilePath()
*/
QString QDir::dirName() const
{
    Q_D(const QDir);
    if (!d_ptr->fileEngine)
        return d->dirEntry.fileName();
    return d->fileEngine->fileName(QAbstractFileEngine::BaseName);
}


#ifdef Q_OS_WIN
static qsizetype drivePrefixLength(QStringView path)
{
    // Used to extract path's drive for use as prefix for an "absolute except for drive" path
    const qsizetype size = path.size();
    qsizetype drive = 2; // length of drive prefix
    if (size > 1 && path.at(1).unicode() == ':') {
        if (Q_UNLIKELY(!path.at(0).isLetter()))
            return 0;
    } else if (path.startsWith("//"_L1)) {
        // UNC path; use its //server/share part as "drive" - it's as sane a
        // thing as we can do.
        for (int i = 0 ; i < 2 ; ++i) { // Scan two "path fragments":
            while (drive < size && path.at(drive).unicode() == '/')
                drive++;
            if (drive >= size) {
                qWarning("Base directory starts with neither a drive nor a UNC share: %s",
                         qUtf8Printable(QDir::toNativeSeparators(path.toString())));
                return 0;
            }
            while (drive < size && path.at(drive).unicode() != '/')
                drive++;
        }
    } else {
        return 0;
    }
    return drive;
}
#endif // Q_OS_WIN

static bool treatAsAbsolute(const QString &path)
{
    // ### Qt 6: be consistent about absolute paths

    // QFileInfo will use the right FS-engine for virtual file-systems
    // (e.g. resource paths).  Unfortunately, for real file-systems, it relies
    // on QFileSystemEntry's isRelative(), which is flawed on MS-Win, ignoring
    // its (correct) isAbsolute().  So only use that isAbsolute() unless there's
    // a colon in the path.
    // FIXME: relies on virtual file-systems having colons in their prefixes.
    // The case of an MS-absolute C:/... path happens to work either way.
    return (path.contains(u':') && QFileInfo(path).isAbsolute())
        || QFileSystemEntry(path).isAbsolute();
}

/*!
    Returns the path name of a file in the directory. Does \e not
    check if the file actually exists in the directory; but see
    exists(). If the QDir is relative the returned path name will also
    be relative. Redundant multiple separators or "." and ".."
    directories in \a fileName are not removed (see cleanPath()).

    \sa dirName(), absoluteFilePath(), isRelative(), canonicalPath()
*/
QString QDir::filePath(const QString &fileName) const
{
    if (treatAsAbsolute(fileName))
        return fileName;

    Q_D(const QDir);
    QString ret = d->dirEntry.filePath();
    if (fileName.isEmpty())
        return ret;

#ifdef Q_OS_WIN
    if (fileName.startsWith(u'/') || fileName.startsWith(u'\\')) {
        // Handle the "absolute except for drive" case (i.e. \blah not c:\blah):
        const qsizetype drive = drivePrefixLength(ret);
        return drive > 0 ? QStringView{ret}.left(drive) % fileName : fileName;
    }
#endif // Q_OS_WIN

    if (ret.isEmpty() || ret.endsWith(u'/'))
        return ret % fileName;
    return ret % u'/' % fileName;
}

/*!
    Returns the absolute path name of a file in the directory. Does \e
    not check if the file actually exists in the directory; but see
    exists(). Redundant multiple separators or "." and ".."
    directories in \a fileName are not removed (see cleanPath()).

    \sa relativeFilePath(), filePath(), canonicalPath()
*/
QString QDir::absoluteFilePath(const QString &fileName) const
{
    if (treatAsAbsolute(fileName))
        return fileName;

    Q_D(const QDir);
    QString absoluteDirPath = d->resolveAbsoluteEntry();
    if (fileName.isEmpty())
        return absoluteDirPath;
#ifdef Q_OS_WIN
    // Handle the "absolute except for drive" case (i.e. \blah not c:\blah):
    if (fileName.startsWith(u'/') || fileName.startsWith(u'\\')) {
        // Combine absoluteDirPath's drive with fileName
        const qsizetype drive = drivePrefixLength(absoluteDirPath);
        if (Q_LIKELY(drive))
            return QStringView{absoluteDirPath}.left(drive) % fileName;

        qWarning("Base directory's drive is not a letter: %s",
                 qUtf8Printable(QDir::toNativeSeparators(absoluteDirPath)));
        return QString();
    }
#endif // Q_OS_WIN
    if (!absoluteDirPath.endsWith(u'/'))
        return absoluteDirPath % u'/' % fileName;
    return absoluteDirPath % fileName;
}

/*!
    Returns the path to \a fileName relative to the directory.

    \snippet code/src_corelib_io_qdir.cpp 7

    \sa absoluteFilePath(), filePath(), canonicalPath()
*/
QString QDir::relativeFilePath(const QString &fileName) const
{
    QString dir = cleanPath(absolutePath());
    QString file = cleanPath(fileName);

    if (isRelativePath(file) || isRelativePath(dir))
        return file;

#ifdef Q_OS_WIN
    QString dirDrive = driveSpec(dir);
    QString fileDrive = driveSpec(file);

    bool fileDriveMissing = false;
    if (fileDrive.isEmpty()) {
        fileDrive = dirDrive;
        fileDriveMissing = true;
    }

    if (fileDrive.toLower() != dirDrive.toLower()
            || (file.startsWith("//"_L1)
                && !dir.startsWith("//"_L1))) {
        return file;
    }

    dir.remove(0, dirDrive.size());
    if (!fileDriveMissing)
        file.remove(0, fileDrive.size());
#endif

    QString result;
    const auto dirElts = dir.tokenize(u'/', Qt::SkipEmptyParts);
    const auto fileElts = file.tokenize(u'/', Qt::SkipEmptyParts);

    const auto dend = dirElts.end();
    const auto fend = fileElts.end();
    auto dit = dirElts.begin();
    auto fit = fileElts.begin();

    const auto eq = [](QStringView lhs, QStringView rhs) {
        return
#if defined(Q_OS_WIN)
           lhs.compare(rhs, Qt::CaseInsensitive) == 0;
#else
           lhs == rhs;
#endif
    };

    // std::ranges::mismatch
    while (dit != dend && fit != fend && eq(*dit, *fit)) {
        ++dit;
        ++fit;
    }

    while (dit != dend) {
        result += "../"_L1;
        ++dit;
    }

    if (fit != fend) {
        while (fit != fend) {
            result += *fit++;
            result += u'/';
        }
        result.chop(1);
    }

    if (result.isEmpty())
        result = "."_L1;
    return result;
}

/*!
    \since 4.2

    Returns \a pathName with the '/' separators converted to
    separators that are appropriate for the underlying operating
    system.

    On Windows, toNativeSeparators("c:/winnt/system32") returns
    "c:\\winnt\\system32".

    The returned string may be the same as the argument on some
    operating systems, for example on Unix.

    \sa fromNativeSeparators(), separator()
*/
QString QDir::toNativeSeparators(const QString &pathName)
{
#if defined(Q_OS_WIN)
    qsizetype i = pathName.indexOf(u'/');
    if (i != -1) {
        QString n(pathName);

        QChar * const data = n.data();
        data[i++] = u'\\';

        for (; i < n.length(); ++i) {
            if (data[i] == u'/')
                data[i] = u'\\';
        }

        return n;
    }
#endif
    return pathName;
}

/*!
    \since 4.2

    Returns \a pathName using '/' as file separator. On Windows,
    for instance, fromNativeSeparators("\c{c:\\winnt\\system32}") returns
    "c:/winnt/system32".

    The returned string may be the same as the argument on some
    operating systems, for example on Unix.

    \sa toNativeSeparators(), separator()
*/
QString QDir::fromNativeSeparators(const QString &pathName)
{
#if defined(Q_OS_WIN)
    return QFileSystemEntry::removeUncOrLongPathPrefix(pathName).replace(u'\\', u'/');
#else
    return pathName;
#endif
}

static QString qt_cleanPath(const QString &path, bool *ok = nullptr);

/*!
    Changes the QDir's directory to \a dirName.

    Returns \c true if the new directory exists;
    otherwise returns \c false. Note that the logical cd() operation is
    not performed if the new directory does not exist.

    Calling cd("..") is equivalent to calling cdUp().

    \sa cdUp(), isReadable(), exists(), path()
*/
bool QDir::cd(const QString &dirName)
{
    // Don't detach just yet.
    const QDirPrivate * const d = d_ptr.constData();

    if (dirName.isEmpty() || dirName == u'.')
        return true;
    QString newPath;
    if (isAbsolutePath(dirName)) {
        newPath = qt_cleanPath(dirName);
    } else {
        newPath = d->dirEntry.filePath();
        if (!newPath.endsWith(u'/'))
            newPath += u'/';
        newPath += dirName;
        if (dirName.indexOf(u'/') >= 0
            || dirName == ".."_L1
            || d->dirEntry.filePath() == u'.') {
            bool ok;
            newPath = qt_cleanPath(newPath, &ok);
            if (!ok)
                return false;
            /*
              If newPath starts with .., we convert it to absolute to
              avoid infinite looping on

                  QDir dir(".");
                  while (dir.cdUp())
                      ;
            */
            if (newPath.startsWith(".."_L1)) {
                newPath = QFileInfo(newPath).absoluteFilePath();
            }
        }
    }

    std::unique_ptr<QDirPrivate> dir(new QDirPrivate(*d_ptr.constData()));
    dir->setPath(newPath);
    if (!dir->exists())
        return false;

    d_ptr = dir.release();
    return true;
}

/*!
    Changes directory by moving one directory up from the QDir's
    current directory.

    Returns \c true if the new directory exists;
    otherwise returns \c false. Note that the logical cdUp() operation is
    not performed if the new directory does not exist.

    \note On Android, this is not supported for content URIs. For more information,
    see \l {Android: DocumentFile.getParentFile()}{DocumentFile.getParentFile()}.

    \sa cd(), isReadable(), exists(), path()
*/
bool QDir::cdUp()
{
    return cd(QString::fromLatin1(".."));
}

/*!
    Returns the string list set by setNameFilters()
*/
QStringList QDir::nameFilters() const
{
    Q_D(const QDir);
    return d->nameFilters;
}

/*!
    Sets the name filters used by entryList() and entryInfoList() to the
    list of filters specified by \a nameFilters.

    Each name filter is a wildcard (globbing) filter that understands
    \c{*} and \c{?} wildcards. See \l{QRegularExpression::fromWildcard()}.

    For example, the following code sets three name filters on a QDir
    to ensure that only files with extensions typically used for C++
    source files are listed:

    \snippet qdir-namefilters/main.cpp 0

    \sa nameFilters(), setFilter()
*/
void QDir::setNameFilters(const QStringList &nameFilters)
{
    Q_D(QDir);
    d->clearCache(QDirPrivate::KeepMetaData);
    d->nameFilters = nameFilters;
}

#ifndef QT_BOOTSTRAPPED

namespace {
struct DirSearchPaths {
    mutable QReadWriteLock mutex;
    QHash<QString, QStringList> paths;
};
}

Q_GLOBAL_STATIC(DirSearchPaths, dirSearchPaths)

/*!
    \since 4.3

    Sets or replaces Qt's search paths for file names with the prefix \a prefix
    to \a searchPaths.

    To specify a prefix for a file name, prepend the prefix followed by a single
    colon (e.g., "images:undo.png", "xmldocs:books.xml"). \a prefix can only
    contain letters or numbers (e.g., it cannot contain a colon, nor a slash).

    Qt uses this search path to locate files with a known prefix. The search
    path entries are tested in order, starting with the first entry.

    \snippet code/src_corelib_io_qdir.cpp 8

    File name prefix must be at least 2 characters long to avoid conflicts with
    Windows drive letters.

    Search paths may contain paths to \l{The Qt Resource System}.
*/
void QDir::setSearchPaths(const QString &prefix, const QStringList &searchPaths)
{
    if (prefix.size() < 2) {
        qWarning("QDir::setSearchPaths: Prefix must be longer than 1 character");
        return;
    }

    for (QChar ch : prefix) {
        if (!ch.isLetterOrNumber()) {
            qWarning("QDir::setSearchPaths: Prefix can only contain letters or numbers");
            return;
        }
    }

    DirSearchPaths &conf = *dirSearchPaths;
    const QWriteLocker lock(&conf.mutex);
    if (searchPaths.isEmpty()) {
        conf.paths.remove(prefix);
    } else {
        conf.paths.insert(prefix, searchPaths);
    }
}

/*!
    \since 4.3

    Adds \a path to the search path for \a prefix.

    \sa setSearchPaths()
*/
void QDir::addSearchPath(const QString &prefix, const QString &path)
{
    if (path.isEmpty())
        return;

    DirSearchPaths &conf = *dirSearchPaths;
    const QWriteLocker lock(&conf.mutex);
    conf.paths[prefix] += path;
}

/*!
    \since 4.3

    Returns the search paths for \a prefix.

    \sa setSearchPaths(), addSearchPath()
*/
QStringList QDir::searchPaths(const QString &prefix)
{
    if (!dirSearchPaths.exists())
        return QStringList();

    const DirSearchPaths &conf = *dirSearchPaths;
    const QReadLocker lock(&conf.mutex);
    return conf.paths.value(prefix);
}

#endif // QT_BOOTSTRAPPED

/*!
    Returns the value set by setFilter()
*/
QDir::Filters QDir::filter() const
{
    Q_D(const QDir);
    return d->filters;
}

/*!
    \enum QDir::Filter

    This enum describes the filtering options available to QDir; e.g.
    for entryList() and entryInfoList(). The filter value is specified
    by combining values from the following list using the bitwise OR
    operator:

    \value Dirs    List directories that match the filters.
    \value AllDirs  List all directories; i.e. don't apply the filters
                    to directory names.
    \value Files   List files.
    \value Drives  List disk drives (ignored under Unix).
    \value NoSymLinks  Do not list symbolic links (ignored by operating
                       systems that don't support symbolic links).
    \value NoDotAndDotDot Do not list the special entries "." and "..".
    \value NoDot       Do not list the special entry ".".
    \value NoDotDot    Do not list the special entry "..".
    \value AllEntries  List directories, files, drives and symlinks (this does not list
                broken symlinks unless you specify System).
    \value Readable    List files for which the application has read
                       access. The Readable value needs to be combined
                       with Dirs or Files.
    \value Writable    List files for which the application has write
                       access. The Writable value needs to be combined
                       with Dirs or Files.
    \value Executable  List files for which the application has
                       execute access. The Executable value needs to be
                       combined with Dirs or Files.
    \value Modified  Only list files that have been modified (ignored
                     on Unix).
    \value Hidden  List hidden files (on Unix, files starting with a ".").
    \value System  List system files (on Unix, FIFOs, sockets and
                   device files are included; on Windows, \c {.lnk}
                   files are included)
    \value CaseSensitive  The filter should be case sensitive.

    \omitvalue TypeMask
    \omitvalue AccessMask
    \omitvalue PermissionMask
    \omitvalue NoFilter

    Functions that use Filter enum values to filter lists of files
    and directories will include symbolic links to files and directories
    unless you set the NoSymLinks value.

    A default constructed QDir will not filter out files based on
    their permissions, so entryList() and entryInfoList() will return
    all files that are readable, writable, executable, or any
    combination of the three.  This makes the default easy to write,
    and at the same time useful.

    For example, setting the \c Readable, \c Writable, and \c Files
    flags allows all files to be listed for which the application has read
    access, write access or both. If the \c Dirs and \c Drives flags are
    also included in this combination then all drives, directories, all
    files that the application can read, write, or execute, and symlinks
    to such files/directories can be listed.

    To retrieve the permissions for a directory, use the
    entryInfoList() function to get the associated QFileInfo objects
    and then use the QFileInfo::permissions() to obtain the permissions
    and ownership for each file.
*/

/*!
    Sets the filter used by entryList() and entryInfoList() to \a
    filters. The filter is used to specify the kind of files that
    should be returned by entryList() and entryInfoList(). See
    \l{QDir::Filter}.

    \sa filter(), setNameFilters()
*/
void QDir::setFilter(Filters filters)
{
    Q_D(QDir);
    d->clearCache(QDirPrivate::KeepMetaData);
    d->filters = filters;
}

/*!
    Returns the value set by setSorting()

    \sa setSorting(), SortFlag
*/
QDir::SortFlags QDir::sorting() const
{
    Q_D(const QDir);
    return d->sort;
}

/*!
    \enum QDir::SortFlag

    This enum describes the sort options available to QDir, e.g. for
    entryList() and entryInfoList(). The sort value is specified by
    OR-ing together values from the following list:

    \value Name  Sort by name.
    \value Time  Sort by time (modification time).
    \value Size  Sort by file size.
    \value Type  Sort by file type (extension).
    \value Unsorted  Do not sort.
    \value NoSort Not sorted by default.

    \value DirsFirst  Put the directories first, then the files.
    \value DirsLast Put the files first, then the directories.
    \value Reversed  Reverse the sort order.
    \value IgnoreCase  Sort case-insensitively.
    \value LocaleAware Sort items appropriately using the current locale settings.

    \omitvalue SortByMask

    You can only specify one of the first four.

    If you specify both DirsFirst and Reversed, directories are
    still put first, but in reverse order; the files will be listed
    after the directories, again in reverse order.
*/

/*!
    Sets the sort order used by entryList() and entryInfoList().

    The \a sort is specified by OR-ing values from the enum
    \l{QDir::SortFlag}.

    \sa sorting(), SortFlag
*/
void QDir::setSorting(SortFlags sort)
{
    Q_D(QDir);
    d->clearCache(QDirPrivate::KeepMetaData);
    d->sort = sort;
}

/*!
    Returns the total number of directories and files in the directory.

    Equivalent to entryList().count().

    \note In Qt versions prior to 6.5, this function returned \c{uint}, not
    \c{qsizetype}.

    \sa operator[](), entryList()
*/
qsizetype QDir::count(QT6_IMPL_NEW_OVERLOAD) const
{
    Q_D(const QDir);
    d->initFileLists(*this);
    return d->fileCache.files.size();
}

/*!
    Returns the file name at position \a pos in the list of file
    names. Equivalent to entryList().at(index).
    \a pos must be a valid index position in the list (i.e., 0 <= pos < count()).

    \note In Qt versions prior to 6.5, \a pos was an \c{int}, not \c{qsizetype}.

    \sa count(), entryList()
*/
QString QDir::operator[](qsizetype pos) const
{
    Q_D(const QDir);
    d->initFileLists(*this);
    return d->fileCache.files[pos];
}

/*!
    \overload

    Returns a list of the names of all the files and directories in
    the directory, ordered according to the name and attribute filters
    previously set with setNameFilters() and setFilter(), and sorted according
    to the flags set with setSorting().

    The attribute filter and sorting specifications can be overridden using the
    \a filters and \a sort arguments.

    Returns an empty list if the directory is unreadable, does not
    exist, or if nothing matches the specification.

    \note To list symlinks that point to non existing files, \l System must be
     passed to the filter.

    \sa entryInfoList(), setNameFilters(), setSorting(), setFilter()
*/
QStringList QDir::entryList(Filters filters, SortFlags sort) const
{
    Q_D(const QDir);
    return entryList(d->nameFilters, filters, sort);
}


/*!
    \overload

    Returns a list of QFileInfo objects for all the files and directories in
    the directory, ordered according to the name and attribute filters
    previously set with setNameFilters() and setFilter(), and sorted according
    to the flags set with setSorting().

    The attribute filter and sorting specifications can be overridden using the
    \a filters and \a sort arguments.

    Returns an empty list if the directory is unreadable, does not
    exist, or if nothing matches the specification.

    \sa entryList(), setNameFilters(), setSorting(), setFilter(), isReadable(), exists()
*/
QFileInfoList QDir::entryInfoList(Filters filters, SortFlags sort) const
{
    Q_D(const QDir);
    return entryInfoList(d->nameFilters, filters, sort);
}

/*!
    Returns a list of the names of all the files and
    directories in the directory, ordered according to the name
    and attribute filters previously set with setNameFilters()
    and setFilter(), and sorted according to the flags set with
    setSorting().

    The name filter, file attribute filter, and sorting specification
    can be overridden using the \a nameFilters, \a filters, and \a sort
    arguments.

    Returns an empty list if the directory is unreadable, does not
    exist, or if nothing matches the specification.

    \sa entryInfoList(), setNameFilters(), setSorting(), setFilter()
*/
QStringList QDir::entryList(const QStringList &nameFilters, Filters filters,
                            SortFlags sort) const
{
    Q_D(const QDir);

    if (filters == NoFilter)
        filters = d->filters;
    if (sort == NoSort)
        sort = d->sort;

    const bool needsSorting = (sort & QDir::SortByMask) != QDir::Unsorted;

    if (filters == d->filters && sort == d->sort && nameFilters == d->nameFilters) {
        // Don't fill a QFileInfo cache if we just need names
        if (needsSorting || d->fileCache.fileListsInitialized) {
            d->initFileLists(*this);
            return d->fileCache.files;
        }
    }

    QDirIterator it(d->dirEntry.filePath(), nameFilters, filters);
    QStringList ret;
    if (needsSorting) {
        QFileInfoList l;
        while (it.hasNext())
            l.append(it.nextFileInfo());
        d->sortFileList(sort, l, &ret, nullptr);
    } else {
        while (it.hasNext()) {
            it.next();
            ret.append(it.fileName());
        }
    }
    return ret;
}

/*!
    Returns a list of QFileInfo objects for all the files and
    directories in the directory, ordered according to the name
    and attribute filters previously set with setNameFilters()
    and setFilter(), and sorted according to the flags set with
    setSorting().

    The name filter, file attribute filter, and sorting specification
    can be overridden using the \a nameFilters, \a filters, and \a sort
    arguments.

    Returns an empty list if the directory is unreadable, does not
    exist, or if nothing matches the specification.

    \sa entryList(), setNameFilters(), setSorting(), setFilter(), isReadable(), exists()
*/
QFileInfoList QDir::entryInfoList(const QStringList &nameFilters, Filters filters,
                                  SortFlags sort) const
{
    Q_D(const QDir);

    if (filters == NoFilter)
        filters = d->filters;
    if (sort == NoSort)
        sort = d->sort;

    if (filters == d->filters && sort == d->sort && nameFilters == d->nameFilters) {
        d->initFileLists(*this);
        return d->fileCache.fileInfos;
    }

    QFileInfoList l;
    QDirIterator it(d->dirEntry.filePath(), nameFilters, filters);
    while (it.hasNext())
        l.append(it.nextFileInfo());
    QFileInfoList ret;
    d->sortFileList(sort, l, nullptr, &ret);
    return ret;
}

/*!
    Creates a sub-directory called \a dirName.

    Returns \c true on success; otherwise returns \c false.

    If the directory already exists when this function is called, it will return \c false.

    The permissions of the created directory are set to \a{permissions}.

    On POSIX systems the permissions are influenced by the value of \c umask.

    On Windows the permissions are emulated using ACLs. These ACLs may be in non-canonical
    order when the group is granted less permissions than others. Files and directories with
    such permissions will generate warnings when the Security tab of the Properties dialog
    is opened. Granting the group all permissions granted to others avoids such warnings.

    \sa rmdir()

    \since 6.3
*/
bool QDir::mkdir(const QString &dirName, QFile::Permissions permissions) const
{
    Q_D(const QDir);

    if (dirName.isEmpty()) {
        qWarning("QDir::mkdir: Empty or null file name");
        return false;
    }

    QString fn = filePath(dirName);
    if (!d->fileEngine)
        return QFileSystemEngine::createDirectory(QFileSystemEntry(fn), false, permissions);
    return d->fileEngine->mkdir(fn, false, permissions);
}

/*!
    \overload
    Creates a sub-directory called \a dirName with default permissions.

    On POSIX systems the default is to grant all permissions allowed by \c umask.
    On Windows, the new directory inherits its permissions from its parent directory.
*/
bool QDir::mkdir(const QString &dirName) const
{
    Q_D(const QDir);

    if (dirName.isEmpty()) {
        qWarning("QDir::mkdir: Empty or null file name");
        return false;
    }

    QString fn = filePath(dirName);
    if (!d->fileEngine)
        return QFileSystemEngine::createDirectory(QFileSystemEntry(fn), false);
    return d->fileEngine->mkdir(fn, false);
}

/*!
    Removes the directory specified by \a dirName.

    The directory must be empty for rmdir() to succeed.

    Returns \c true if successful; otherwise returns \c false.

    \sa mkdir()
*/
bool QDir::rmdir(const QString &dirName) const
{
    Q_D(const QDir);

    if (dirName.isEmpty()) {
        qWarning("QDir::rmdir: Empty or null file name");
        return false;
    }

    QString fn = filePath(dirName);
    if (!d->fileEngine)
        return QFileSystemEngine::removeDirectory(QFileSystemEntry(fn), false);

    return d->fileEngine->rmdir(fn, false);
}

/*!
    Creates the directory path \a dirPath.

    The function will create all parent directories necessary to
    create the directory.

    Returns \c true if successful; otherwise returns \c false.

    If the path already exists when this function is called, it will return true.

    \sa rmpath()
*/
bool QDir::mkpath(const QString &dirPath) const
{
    Q_D(const QDir);

    if (dirPath.isEmpty()) {
        qWarning("QDir::mkpath: Empty or null file name");
        return false;
    }

    QString fn = filePath(dirPath);
    if (!d->fileEngine)
        return QFileSystemEngine::createDirectory(QFileSystemEntry(fn), true);
    return d->fileEngine->mkdir(fn, true);
}

/*!
    Removes the directory path \a dirPath.

    The function will remove all parent directories in \a dirPath,
    provided that they are empty. This is the opposite of
    mkpath(dirPath).

    Returns \c true if successful; otherwise returns \c false.

    \sa mkpath()
*/
bool QDir::rmpath(const QString &dirPath) const
{
    Q_D(const QDir);

    if (dirPath.isEmpty()) {
        qWarning("QDir::rmpath: Empty or null file name");
        return false;
    }

    QString fn = filePath(dirPath);
    if (!d->fileEngine)
        return QFileSystemEngine::removeDirectory(QFileSystemEntry(fn), true);
    return d->fileEngine->rmdir(fn, true);
}

/*!
    \since 5.0
    Removes the directory, including all its contents.

    Returns \c true if successful, otherwise false.

    If a file or directory cannot be removed, removeRecursively() keeps going
    and attempts to delete as many files and sub-directories as possible,
    then returns \c false.

    If the directory was already removed, the method returns \c true
    (expected result already reached).

    \note This function is meant for removing a small application-internal
    directory (such as a temporary directory), but not user-visible
    directories. For user-visible operations, it is rather recommended
    to report errors more precisely to the user, to offer solutions
    in case of errors, to show progress during the deletion since it
    could take several minutes, etc.
*/
bool QDir::removeRecursively()
{
    if (!d_ptr->exists())
        return true;

    bool success = true;
    const QString dirPath = path();
    // not empty -- we must empty it first
    QDirIterator di(dirPath, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    while (di.hasNext()) {
        const QFileInfo fi = di.nextFileInfo();
        const QString &filePath = di.filePath();
        bool ok;
        if (fi.isDir() && !fi.isSymLink()) {
            ok = QDir(filePath).removeRecursively(); // recursive
        } else {
            ok = QFile::remove(filePath);
            if (!ok) { // Read-only files prevent directory deletion on Windows, retry with Write permission.
                const QFile::Permissions permissions = QFile::permissions(filePath);
                if (!(permissions & QFile::WriteUser))
                    ok = QFile::setPermissions(filePath, permissions | QFile::WriteUser)
                        && QFile::remove(filePath);
            }
        }
        if (!ok)
            success = false;
    }

    if (success)
        success = rmdir(absolutePath());

    return success;
}

/*!
    Returns \c true if the directory is readable \e and we can open files
    by name; otherwise returns \c false.

    \warning A false value from this function is not a guarantee that
    files in the directory are not accessible.

    \sa QFileInfo::isReadable()
*/
bool QDir::isReadable() const
{
    Q_D(const QDir);

    if (!d->fileEngine) {
        QMutexLocker locker(&d->fileCache.mutex);
        if (!d->fileCache.metaData.hasFlags(QFileSystemMetaData::UserReadPermission)) {
            QFileSystemEngine::fillMetaData(d->dirEntry, d->fileCache.metaData,
                                            QFileSystemMetaData::UserReadPermission);
        }
        return d->fileCache.metaData.permissions().testAnyFlag(QFile::ReadUser);
    }

    const QAbstractFileEngine::FileFlags info =
        d->fileEngine->fileFlags(QAbstractFileEngine::DirectoryType
                                       | QAbstractFileEngine::PermsMask);
    if (!(info & QAbstractFileEngine::DirectoryType))
        return false;
    return info.testAnyFlag(QAbstractFileEngine::ReadUserPerm);
}

/*!
    \overload

    Returns \c true if the directory exists; otherwise returns \c false.
    (If a file with the same name is found this function will return false).

    The overload of this function that accepts an argument is used to test
    for the presence of files and directories within a directory.

    \sa QFileInfo::exists(), QFile::exists()
*/
bool QDir::exists() const
{
    return d_ptr->exists();
}

/*!
    Returns \c true if the directory is the root directory; otherwise
    returns \c false.

    \note If the directory is a symbolic link to the root directory
    this function returns \c false. If you want to test for this use
    canonicalPath(), e.g.

    \snippet code/src_corelib_io_qdir.cpp 9

    \sa root(), rootPath()
*/
bool QDir::isRoot() const
{
    if (!d_ptr->fileEngine)
        return d_ptr->dirEntry.isRoot();
    return d_ptr->fileEngine->fileFlags(QAbstractFileEngine::FlagsMask).testAnyFlag(QAbstractFileEngine::RootFlag);
}

/*!
    \fn bool QDir::isAbsolute() const

    Returns \c true if the directory's path is absolute; otherwise
    returns \c false. See isAbsolutePath().

    \note Paths starting with a colon (\e{:}) are always considered
    absolute, as they denote a QResource.

    \sa isRelative(), makeAbsolute(), cleanPath()
*/

/*!
   \fn bool QDir::isAbsolutePath(const QString &)

    Returns \c true if \a path is absolute; returns \c false if it is
    relative.

    \note Paths starting with a colon (\e{:}) are always considered
    absolute, as they denote a QResource.

    \sa isAbsolute(), isRelativePath(), makeAbsolute(), cleanPath(), QResource
*/

/*!
    Returns \c true if the directory path is relative; otherwise returns
    false. (Under Unix a path is relative if it does not start with a
    "/").

    \note Paths starting with a colon (\e{:}) are always considered
    absolute, as they denote a QResource.

    \sa makeAbsolute(), isAbsolute(), isAbsolutePath(), cleanPath()
*/
bool QDir::isRelative() const
{
    if (!d_ptr->fileEngine)
        return d_ptr->dirEntry.isRelative();
    return d_ptr->fileEngine->isRelativePath();
}


/*!
    Converts the directory path to an absolute path. If it is already
    absolute nothing happens. Returns \c true if the conversion
    succeeded; otherwise returns \c false.

    \sa isAbsolute(), isAbsolutePath(), isRelative(), cleanPath()
*/
bool QDir::makeAbsolute()
{
    Q_D(const QDir);
    std::unique_ptr<QDirPrivate> dir;
    if (!!d->fileEngine) {
        QString absolutePath = d->fileEngine->fileName(QAbstractFileEngine::AbsoluteName);
        if (QDir::isRelativePath(absolutePath))
            return false;

        dir.reset(new QDirPrivate(*d_ptr.constData()));
        dir->setPath(absolutePath);
    } else { // native FS
        QString absoluteFilePath = d->resolveAbsoluteEntry();
        dir.reset(new QDirPrivate(*d_ptr.constData()));
        dir->setPath(absoluteFilePath);
    }
    d_ptr = dir.release(); // actually detach
    return true;
}

/*!
    Returns \c true if directory \a dir and this directory have the same
    path and their sort and filter settings are the same; otherwise
    returns \c false.

    Example:

    \snippet code/src_corelib_io_qdir.cpp 10
*/
bool QDir::operator==(const QDir &dir) const
{
    Q_D(const QDir);
    const QDirPrivate *other = dir.d_ptr.constData();

    if (d == other)
        return true;
    Qt::CaseSensitivity sensitive;
    if (!d->fileEngine || !other->fileEngine) {
        if (d->fileEngine.get() != other->fileEngine.get()) // one is native, the other is a custom file-engine
            return false;

        sensitive = QFileSystemEngine::isCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    } else {
        if (d->fileEngine->caseSensitive() != other->fileEngine->caseSensitive())
            return false;
        sensitive = d->fileEngine->caseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    }

    if (d->filters == other->filters
       && d->sort == other->sort
       && d->nameFilters == other->nameFilters) {

        // Assume directories are the same if path is the same
        if (d->dirEntry.filePath() == other->dirEntry.filePath())
            return true;

        if (exists()) {
            if (!dir.exists())
                return false; //can't be equal if only one exists
            // Both exist, fallback to expensive canonical path computation
            return canonicalPath().compare(dir.canonicalPath(), sensitive) == 0;
        } else {
            if (dir.exists())
                return false; //can't be equal if only one exists
            // Neither exists, compare absolute paths rather than canonical (which would be empty strings)
            QString thisFilePath = d->resolveAbsoluteEntry();
            QString otherFilePath = other->resolveAbsoluteEntry();
            return thisFilePath.compare(otherFilePath, sensitive) == 0;
        }
    }
    return false;
}

/*!
    Makes a copy of the \a dir object and assigns it to this QDir
    object.
*/
QDir &QDir::operator=(const QDir &dir)
{
    d_ptr = dir.d_ptr;
    return *this;
}

/*!
    \fn void QDir::swap(QDir &other)
    \since 5.0

    Swaps this QDir instance with \a other. This function is very fast
    and never fails.
*/

/*!
    \fn bool QDir::operator!=(const QDir &dir) const

    Returns \c true if directory \a dir and this directory have different
    paths or different sort or filter settings; otherwise returns
    false.

    Example:

    \snippet code/src_corelib_io_qdir.cpp 11
*/

/*!
    Removes the file, \a fileName.

    Returns \c true if the file is removed successfully; otherwise
    returns \c false.
*/
bool QDir::remove(const QString &fileName)
{
    if (fileName.isEmpty()) {
        qWarning("QDir::remove: Empty or null file name");
        return false;
    }
    return QFile::remove(filePath(fileName));
}

/*!
    Renames a file or directory from \a oldName to \a newName, and returns
    true if successful; otherwise returns \c false.

    On most file systems, rename() fails only if \a oldName does not
    exist, or if a file with the new name already exists.
    However, there are also other reasons why rename() can
    fail. For example, on at least one file system rename() fails if
    \a newName points to an open file.

    If \a oldName is a file (not a directory) that can't be renamed
    right away, Qt will try to copy \a oldName to \a newName and remove
    \a oldName.

    \sa QFile::rename()
*/
bool QDir::rename(const QString &oldName, const QString &newName)
{
    if (oldName.isEmpty() || newName.isEmpty()) {
        qWarning("QDir::rename: Empty or null file name(s)");
        return false;
    }

    QFile file(filePath(oldName));
    if (!file.exists())
        return false;
    return file.rename(filePath(newName));
}

/*!
    Returns \c true if the file called \a name exists; otherwise returns
    false.

    Unless \a name contains an absolute file path, the file name is assumed
    to be relative to the directory itself, so this function is typically used
    to check for the presence of files within a directory.

    \sa QFileInfo::exists(), QFile::exists()
*/
bool QDir::exists(const QString &name) const
{
    if (name.isEmpty()) {
        qWarning("QDir::exists: Empty or null file name");
        return false;
    }
    return QFileInfo::exists(filePath(name));
}

/*!
    Returns whether the directory is empty.

    Equivalent to \c{count() == 0} with filters
    \c{QDir::AllEntries | QDir::NoDotAndDotDot}, but faster as it just checks
    whether the directory contains at least one entry.

    \note Unless you set the \a filters flags to include \c{QDir::NoDotAndDotDot}
          (as the default value does), no directory is empty.

    \sa count(), entryList(), setFilter()
    \since 5.9
*/
bool QDir::isEmpty(Filters filters) const
{
    Q_D(const QDir);
    QDirIterator it(d->dirEntry.filePath(), d->nameFilters, filters);
    return !it.hasNext();
}

/*!
    Returns a list of the root directories on this system.

    On Windows this returns a list of QFileInfo objects containing "C:/",
    "D:/", etc. This does not return drives with ejectable media that are empty.
    On other operating systems, it returns a list containing
    just one root directory (i.e. "/").

    \sa root(), rootPath()
*/
QFileInfoList QDir::drives()
{
#ifdef QT_NO_FSFILEENGINE
    return QFileInfoList();
#else
    return QFSFileEngine::drives();
#endif
}

/*!
    \fn QChar QDir::separator()

    Returns the native directory separator: "/" under Unix
    and "\\" under Windows.

    You do not need to use this function to build file paths. If you
    always use "/", Qt will translate your paths to conform to the
    underlying operating system. If you want to display paths to the
    user using their operating system's separator use
    toNativeSeparators().

    \sa listSeparator()
*/

/*!
    \fn QDir::listSeparator()
    \since 5.6

    Returns the native path list separator: ':' under Unix
    and ';' under Windows.

    \sa separator()
*/

/*!
    Sets the application's current working directory to \a path.
    Returns \c true if the directory was successfully changed; otherwise
    returns \c false.

    \snippet code/src_corelib_io_qdir.cpp 16

    \sa current(), currentPath(), home(), root(), temp()
*/
bool QDir::setCurrent(const QString &path)
{
    return QFileSystemEngine::setCurrentPath(QFileSystemEntry(path));
}

/*!
    \fn QDir QDir::current()

    Returns the application's current directory.

    The directory is constructed using the absolute path of the current directory,
    ensuring that its path() will be the same as its absolutePath().

    \sa currentPath(), setCurrent(), home(), root(), temp()
*/

/*!
    Returns the absolute path of the application's current directory. The
    current directory is the last directory set with QDir::setCurrent() or, if
    that was never called, the directory at which this application was started
    at by the parent process.

    \sa current(), setCurrent(), homePath(), rootPath(), tempPath(), QCoreApplication::applicationDirPath()
*/
QString QDir::currentPath()
{
    return QFileSystemEngine::currentPath().filePath();
}

/*!
    \fn QDir QDir::home()

    Returns the user's home directory.

    The directory is constructed using the absolute path of the home directory,
    ensuring that its path() will be the same as its absolutePath().

    See homePath() for details.

    \sa drives(), current(), root(), temp()
*/

/*!
    Returns the absolute path of the user's home directory.

    Under Windows this function will return the directory of the
    current user's profile. Typically, this is:

    \snippet code/src_corelib_io_qdir.cpp 12

    Use the toNativeSeparators() function to convert the separators to
    the ones that are appropriate for the underlying operating system.

    If the directory of the current user's profile does not exist or
    cannot be retrieved, the following alternatives will be checked (in
    the given order) until an existing and available path is found:

    \list 1
    \li The path specified by the \c USERPROFILE environment variable.
    \li The path formed by concatenating the \c HOMEDRIVE and \c HOMEPATH
    environment variables.
    \li The path specified by the \c HOME environment variable.
    \li The path returned by the rootPath() function (which uses the \c SystemDrive
    environment variable)
    \li  The \c{C:/} directory.
    \endlist

    Under non-Windows operating systems the \c HOME environment
    variable is used if it exists, otherwise the path returned by the
    rootPath().

    \sa home(), currentPath(), rootPath(), tempPath()
*/
QString QDir::homePath()
{
    return QFileSystemEngine::homePath();
}

/*!
    \fn QDir QDir::temp()

    Returns the system's temporary directory.

    The directory is constructed using the absolute canonical path of the temporary directory,
    ensuring that its path() will be the same as its absolutePath().

    See tempPath() for details.

    \sa drives(), current(), home(), root()
*/

/*!
    Returns the absolute canonical path of the system's temporary directory.

    On Unix/Linux systems this is the path in the \c TMPDIR environment
    variable or \c{/tmp} if \c TMPDIR is not defined. On Windows this is
    usually the path in the \c TEMP or \c TMP environment
    variable.
    The path returned by this method doesn't end with a directory separator
    unless it is the root directory (of a drive).

    \sa temp(), currentPath(), homePath(), rootPath()
*/
QString QDir::tempPath()
{
    return QFileSystemEngine::tempPath();
}

/*!
    \fn QDir QDir::root()

    Returns the root directory.

    The directory is constructed using the absolute path of the root directory,
    ensuring that its path() will be the same as its absolutePath().

    See rootPath() for details.

    \sa drives(), current(), home(), temp()
*/

/*!
    Returns the absolute path of the root directory.

    For Unix operating systems this returns "/". For Windows file
    systems this normally returns "c:/".

    \sa root(), drives(), currentPath(), homePath(), tempPath()
*/
QString QDir::rootPath()
{
    return QFileSystemEngine::rootPath();
}

#if QT_CONFIG(regularexpression)
/*!
    \overload

    Returns \c true if the \a fileName matches any of the wildcard (glob)
    patterns in the list of \a filters; otherwise returns \c false. The
    matching is case insensitive.

    \sa QRegularExpression::fromWildcard(), entryList(), entryInfoList()
*/
bool QDir::match(const QStringList &filters, const QString &fileName)
{
    for (QStringList::ConstIterator sit = filters.constBegin(); sit != filters.constEnd(); ++sit) {
        // Insensitive exact match
        auto rx = QRegularExpression::fromWildcard(*sit, Qt::CaseInsensitive);
        if (rx.match(fileName).hasMatch())
            return true;
    }
    return false;
}

/*!
    Returns \c true if the \a fileName matches the wildcard (glob)
    pattern \a filter; otherwise returns \c false. The \a filter may
    contain multiple patterns separated by spaces or semicolons.
    The matching is case insensitive.

    \sa QRegularExpression::fromWildcard(), entryList(), entryInfoList()
*/
bool QDir::match(const QString &filter, const QString &fileName)
{
    return match(nameFiltersFromString(filter), fileName);
}
#endif // QT_CONFIG(regularexpression)

/*!
    \internal
    Returns \a path with redundant directory separators removed,
    and "."s and ".."s resolved (as far as possible).

    This method is shared with QUrl, so it doesn't deal with QDir::separator(),
    nor does it remove the trailing slash, if any.
*/
QString qt_normalizePathSegments(const QString &name, QDirPrivate::PathNormalizations flags, bool *ok)
{
    const bool allowUncPaths = flags.testAnyFlag(QDirPrivate::AllowUncPaths);
    const bool isRemote = flags.testAnyFlag(QDirPrivate::RemotePath);
    const qsizetype len = name.size();

    if (ok)
        *ok = false;

    if (len == 0)
        return name;

    qsizetype i = len - 1;
    QVarLengthArray<char16_t> outVector(len);
    qsizetype used = len;
    char16_t *out = outVector.data();
    const char16_t *p = reinterpret_cast<const char16_t *>(name.data());
    const char16_t *prefix = p;
    qsizetype up = 0;

    const qsizetype prefixLength = rootLength(name, allowUncPaths);
    p += prefixLength;
    i -= prefixLength;

    // replicate trailing slash (i > 0 checks for emptiness of input string p)
    // except for remote paths because there can be /../ or /./ ending
    if (i > 0 && p[i] == '/' && !isRemote) {
        out[--used] = '/';
        --i;
    }

    auto isDot = [](const char16_t *p, qsizetype i) {
        return i > 1 && p[i - 1] == '.' && p[i - 2] == '/';
    };
    auto isDotDot = [](const char16_t *p, qsizetype i) {
        return i > 2 && p[i - 1] == '.' && p[i - 2] == '.' && p[i - 3] == '/';
    };

    while (i >= 0) {
        // copy trailing slashes for remote urls
        if (p[i] == '/') {
            if (isRemote && !up) {
                if (isDot(p, i)) {
                    i -= 2;
                    continue;
                }
                out[--used] = p[i];
            }

            --i;
            continue;
        }

        // remove current directory
        if (p[i] == '.' && (i == 0 || p[i-1] == '/')) {
            --i;
            continue;
        }

        // detect up dir
        if (i >= 1 && p[i] == '.' && p[i-1] == '.' && (i < 2 || p[i - 2] == '/')) {
            ++up;
            i -= i >= 2 ? 3 : 2;

            if (isRemote) {
                // moving up should consider empty path segments too (/path//../ -> /path/)
                while (i > 0 && up && p[i] == '/') {
                    --up;
                    --i;
                }
            }
            continue;
        }

        // prepend a slash before copying when not empty
        if (!up && used != len && out[used] != '/')
            out[--used] = '/';

        // skip or copy
        while (i >= 0) {
            if (p[i] == '/') {
                // copy all slashes as is for remote urls if they are not part of /./ or /../
                if (isRemote && !up) {
                    while (i > 0 && p[i] == '/' && !isDotDot(p, i)) {

                        if (isDot(p, i)) {
                            i -= 2;
                            continue;
                        }

                        out[--used] = p[i];
                        --i;
                    }

                    // in case of /./, jump over
                    if (isDot(p, i))
                        i -= 2;

                    break;
                }

                --i;
                break;
            }

            // actual copy
            if (!up)
                out[--used] = p[i];
            --i;
        }

        // decrement up after copying/skipping
        if (up)
            --up;
    }

    // Indicate failure when ".." are left over for an absolute path.
    if (ok)
        *ok = prefixLength == 0 || up == 0;

    // add remaining '..'
    while (up && !isRemote) {
        if (used != len && out[used] != '/') // is not empty and there isn't already a '/'
            out[--used] = '/';
        out[--used] = '.';
        out[--used] = '.';
        --up;
    }

    bool isEmpty = used == len;

    if (prefixLength) {
        if (!isEmpty && out[used] == '/') {
            // Even though there is a prefix the out string is a slash. This happens, if the input
            // string only consists of a prefix followed by one or more slashes. Just skip the slash.
            ++used;
        }
        for (qsizetype i = prefixLength - 1; i >= 0; --i)
            out[--used] = prefix[i];
    } else {
        if (isEmpty) {
            // After resolving the input path, the resulting string is empty (e.g. "foo/.."). Return
            // a dot in that case.
            out[--used] = '.';
        } else if (out[used] == '/') {
            // After parsing the input string, out only contains a slash. That happens whenever all
            // parts are resolved and there is a trailing slash ("./" or "foo/../" for example).
            // Prepend a dot to have the correct return value.
            out[--used] = '.';
        }
    }

    // If path was not modified return the original value
    if (used == 0)
        return name;
    return QString::fromUtf16(out + used, len - used);
}

static QString qt_cleanPath(const QString &path, bool *ok)
{
    if (path.isEmpty()) {
        Q_ASSERT(!ok); // The only caller passing ok knows its path is non-empty
        return path;
    }

    QString name = QDir::fromNativeSeparators(path);
    QString ret = qt_normalizePathSegments(name, OSSupportsUncPaths ? QDirPrivate::AllowUncPaths : QDirPrivate::DefaultNormalization, ok);

    // Strip away last slash except for root directories
    if (ret.size() > 1 && ret.endsWith(u'/')) {
#if defined (Q_OS_WIN)
        if (!(ret.length() == 3 && ret.at(1) == u':'))
#endif
            ret.chop(1);
    }

    return ret;
}

/*!
    Returns \a path with directory separators normalized (that is, platform-native
    separators converted to "/") and redundant ones removed, and "."s and ".."s
    resolved (as far as possible).

    Symbolic links are kept. This function does not return the
    canonical path, but rather the simplest version of the input.
    For example, "./local" becomes "local", "local/../bin" becomes
    "bin" and "/local/usr/../bin" becomes "/local/bin".

    \sa absolutePath(), canonicalPath()
*/
QString QDir::cleanPath(const QString &path)
{
    return qt_cleanPath(path);
}

/*!
    Returns \c true if \a path is relative; returns \c false if it is
    absolute.

    \note Paths starting with a colon (\e{:}) are always considered
    absolute, as they denote a QResource.

    \sa isRelative(), isAbsolutePath(), makeAbsolute()
*/
bool QDir::isRelativePath(const QString &path)
{
    return QFileInfo(path).isRelative();
}

/*!
    Refreshes the directory information.
*/
void QDir::refresh() const
{
    QDirPrivate *d = const_cast<QDir *>(this)->d_func();
    d->clearCache(QDirPrivate::IncludingMetaData);
}

/*!
    \internal
*/
QDirPrivate* QDir::d_func()
{
    return d_ptr.data();
}

/*!
    \internal

    Returns a list of name filters from the given \a nameFilter. (If
    there is more than one filter, each pair of filters is separated
    by a space or by a semicolon.)
*/
QStringList QDir::nameFiltersFromString(const QString &nameFilter)
{
    return QDirPrivate::splitFilters(nameFilter);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, QDir::Filters filters)
{
    QDebugStateSaver save(debug);
    debug.resetFormat();
    QStringList flags;
    if (filters == QDir::NoFilter) {
        flags << "NoFilter"_L1;
    } else {
        if (filters & QDir::Dirs) flags << "Dirs"_L1;
        if (filters & QDir::AllDirs) flags << "AllDirs"_L1;
        if (filters & QDir::Files) flags << "Files"_L1;
        if (filters & QDir::Drives) flags << "Drives"_L1;
        if (filters & QDir::NoSymLinks) flags << "NoSymLinks"_L1;
        if (filters & QDir::NoDot) flags << "NoDot"_L1;
        if (filters & QDir::NoDotDot) flags << "NoDotDot"_L1;
        if ((filters & QDir::AllEntries) == QDir::AllEntries) flags << "AllEntries"_L1;
        if (filters & QDir::Readable) flags << "Readable"_L1;
        if (filters & QDir::Writable) flags << "Writable"_L1;
        if (filters & QDir::Executable) flags << "Executable"_L1;
        if (filters & QDir::Modified) flags << "Modified"_L1;
        if (filters & QDir::Hidden) flags << "Hidden"_L1;
        if (filters & QDir::System) flags << "System"_L1;
        if (filters & QDir::CaseSensitive) flags << "CaseSensitive"_L1;
    }
    debug.noquote() << "QDir::Filters(" << flags.join(u'|') << ')';
    return debug;
}

static QDebug operator<<(QDebug debug, QDir::SortFlags sorting)
{
    QDebugStateSaver save(debug);
    debug.resetFormat();
    if (sorting == QDir::NoSort) {
        debug << "QDir::SortFlags(NoSort)";
    } else {
        QString type;
        if ((sorting & QDir::SortByMask) == QDir::Name) type = "Name"_L1;
        if ((sorting & QDir::SortByMask) == QDir::Time) type = "Time"_L1;
        if ((sorting & QDir::SortByMask) == QDir::Size) type = "Size"_L1;
        if ((sorting & QDir::SortByMask) == QDir::Unsorted) type = "Unsorted"_L1;

        QStringList flags;
        if (sorting & QDir::DirsFirst) flags << "DirsFirst"_L1;
        if (sorting & QDir::DirsLast) flags << "DirsLast"_L1;
        if (sorting & QDir::IgnoreCase) flags << "IgnoreCase"_L1;
        if (sorting & QDir::LocaleAware) flags << "LocaleAware"_L1;
        if (sorting & QDir::Type) flags << "Type"_L1;
        debug.noquote() << "QDir::SortFlags(" << type << '|' << flags.join(u'|') << ')';
    }
    return debug;
}

QDebug operator<<(QDebug debug, const QDir &dir)
{
    QDebugStateSaver save(debug);
    debug.resetFormat();
    debug << "QDir(" << dir.path() << ", nameFilters = {"
          << dir.nameFilters().join(u',')
          << "}, "
          << dir.sorting()
          << ','
          << dir.filter()
          << ')';
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

/*!
    \fn QDir::QDir(const std::filesystem::path &path)
    \since 6.0
    Constructs a QDir pointing to the given directory \a path. If path
    is empty the program's working directory, ("."), is used.

    \sa currentPath()
*/
/*!
    \fn QDir::QDir(const std::filesystem::path &path,
                   const QString &nameFilter,
                   SortFlags sort,
                   Filters filters)
    \since 6.0

    Constructs a QDir with path \a path, that filters its entries by
    name using \a nameFilter and by attributes using \a filters. It
    also sorts the names using \a sort.

    The default \a nameFilter is an empty string, which excludes
    nothing; the default \a filters is \l AllEntries, which also
    excludes nothing. The default \a sort is \l Name | \l IgnoreCase,
    i.e. sort by name case-insensitively.

    If \a path is empty, QDir uses "." (the current
    directory). If \a nameFilter is an empty string, QDir uses the
    name filter "*" (all files).

    \note \a path need not exist.

    \sa exists(), setPath(), setNameFilters(), setFilter(), setSorting()
*/
/*!
    \fn void QDir::setPath(const std::filesystem::path &path)
    \since 6.0
    \overload
*/
/*!
    \fn void QDir::addSearchPath(const QString &prefix, const std::filesystem::path &path)
    \since 6.0
    \overload
*/
/*!
    \fn std::filesystem::path QDir::filesystemPath() const
    \since 6.0
    Returns path() as \c{std::filesystem::path}.
    \sa path()
*/
/*!
    \fn std::filesystem::path QDir::filesystemAbsolutePath() const
    \since 6.0
    Returns absolutePath() as \c{std::filesystem::path}.
    \sa absolutePath()
*/
/*!
    \fn std::filesystem::path QDir::filesystemCanonicalPath() const
    \since 6.0
    Returns canonicalPath() as \c{std::filesystem::path}.
    \sa canonicalPath()
*/

QT_END_NAMESPACE
