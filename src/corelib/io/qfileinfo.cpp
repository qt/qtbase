// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qplatformdefs.h"
#include "qfileinfo.h"
#include "qglobal.h"
#include "qdir.h"
#include "qfileinfo_p.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QT_IMPL_METATYPE_EXTERN(QFileInfo)

QString QFileInfoPrivate::getFileName(QAbstractFileEngine::FileName name) const
{
    if (cache_enabled && !fileNames[(int)name].isNull())
        return fileNames[(int)name];

    QString ret;
    if (fileEngine == nullptr) { // local file; use the QFileSystemEngine directly
        switch (name) {
            case QAbstractFileEngine::CanonicalName:
            case QAbstractFileEngine::CanonicalPathName: {
                QFileSystemEntry entry = QFileSystemEngine::canonicalName(fileEntry, metaData);
                if (cache_enabled) { // be smart and store both
                    fileNames[QAbstractFileEngine::CanonicalName] = entry.filePath();
                    fileNames[QAbstractFileEngine::CanonicalPathName] = entry.path();
                }
                if (name == QAbstractFileEngine::CanonicalName)
                    ret = entry.filePath();
                else
                    ret = entry.path();
                break;
            }
            case QAbstractFileEngine::AbsoluteLinkTarget:
                ret = QFileSystemEngine::getLinkTarget(fileEntry, metaData).filePath();
                break;
            case QAbstractFileEngine::RawLinkPath:
                ret = QFileSystemEngine::getRawLinkPath(fileEntry, metaData).filePath();
                break;
            case QAbstractFileEngine::JunctionName:
                ret = QFileSystemEngine::getJunctionTarget(fileEntry, metaData).filePath();
                break;
            case QAbstractFileEngine::BundleName:
                ret = QFileSystemEngine::bundleName(fileEntry);
                break;
            case QAbstractFileEngine::AbsoluteName:
            case QAbstractFileEngine::AbsolutePathName: {
                QFileSystemEntry entry = QFileSystemEngine::absoluteName(fileEntry);
                if (cache_enabled) { // be smart and store both
                    fileNames[QAbstractFileEngine::AbsoluteName] = entry.filePath();
                    fileNames[QAbstractFileEngine::AbsolutePathName] = entry.path();
                }
                if (name == QAbstractFileEngine::AbsoluteName)
                    ret = entry.filePath();
                else
                    ret = entry.path();
                break;
            }
            default: break;
        }
    } else {
        ret = fileEngine->fileName(name);
    }
    if (ret.isNull())
        ret = ""_L1;
    if (cache_enabled)
        fileNames[(int)name] = ret;
    return ret;
}

QString QFileInfoPrivate::getFileOwner(QAbstractFileEngine::FileOwner own) const
{
    if (cache_enabled && !fileOwners[(int)own].isNull())
        return fileOwners[(int)own];
    QString ret;
    if (fileEngine == nullptr) {
        switch (own) {
        case QAbstractFileEngine::OwnerUser:
            ret = QFileSystemEngine::resolveUserName(fileEntry, metaData);
            break;
        case QAbstractFileEngine::OwnerGroup:
            ret = QFileSystemEngine::resolveGroupName(fileEntry, metaData);
            break;
        }
     } else {
        ret = fileEngine->owner(own);
    }
    if (ret.isNull())
        ret = ""_L1;
    if (cache_enabled)
        fileOwners[(int)own] = ret;
    return ret;
}

uint QFileInfoPrivate::getFileFlags(QAbstractFileEngine::FileFlags request) const
{
    Q_ASSERT(fileEngine); // should never be called when using the native FS
    // We split the testing into tests for for LinkType, BundleType, PermsMask
    // and the rest.
    // Tests for file permissions on Windows can be slow, especially on network
    // paths and NTFS drives.
    // In order to determine if a file is a symlink or not, we have to lstat().
    // If we're not interested in that information, we might as well avoid one
    // extra syscall. Bundle detecton on Mac can be slow, especially on network
    // paths, so we separate out that as well.

    QAbstractFileEngine::FileFlags req;
    uint cachedFlags = 0;

    if (request & (QAbstractFileEngine::FlagsMask | QAbstractFileEngine::TypesMask)) {
        if (!getCachedFlag(CachedFileFlags)) {
            req |= QAbstractFileEngine::FlagsMask;
            req |= QAbstractFileEngine::TypesMask;
            req &= (~QAbstractFileEngine::LinkType);
            req &= (~QAbstractFileEngine::BundleType);

            cachedFlags |= CachedFileFlags;
        }

        if (request & QAbstractFileEngine::LinkType) {
            if (!getCachedFlag(CachedLinkTypeFlag)) {
                req |= QAbstractFileEngine::LinkType;
                cachedFlags |= CachedLinkTypeFlag;
            }
        }

        if (request & QAbstractFileEngine::BundleType) {
            if (!getCachedFlag(CachedBundleTypeFlag)) {
                req |= QAbstractFileEngine::BundleType;
                cachedFlags |= CachedBundleTypeFlag;
            }
        }
    }

    if (request & QAbstractFileEngine::PermsMask) {
        if (!getCachedFlag(CachedPerms)) {
            req |= QAbstractFileEngine::PermsMask;
            cachedFlags |= CachedPerms;
        }
    }

    if (req) {
        if (cache_enabled)
            req &= (~QAbstractFileEngine::Refresh);
        else
            req |= QAbstractFileEngine::Refresh;

        QAbstractFileEngine::FileFlags flags = fileEngine->fileFlags(req);
        fileFlags |= uint(flags.toInt());
        setCachedFlag(cachedFlags);
    }

    return fileFlags & request.toInt();
}

QDateTime &QFileInfoPrivate::getFileTime(QFile::FileTime request) const
{
    Q_ASSERT(fileEngine); // should never be called when using the native FS
    if (!cache_enabled)
        clearFlags();

    uint cf = 0;
    switch (request) {
    case QFile::FileAccessTime:
        cf = CachedATime;
        break;
    case QFile::FileBirthTime:
        cf = CachedBTime;
        break;
    case QFile::FileMetadataChangeTime:
        cf = CachedMCTime;
        break;
    case QFile::FileModificationTime:
        cf = CachedMTime;
        break;
    }

    if (!getCachedFlag(cf)) {
        fileTimes[request] = fileEngine->fileTime(request);
        setCachedFlag(cf);
    }
    return fileTimes[request];
}

//************* QFileInfo

/*!
    \class QFileInfo
    \inmodule QtCore
    \reentrant
    \brief The QFileInfo class provides an OS-independent API to retrieve
    information about file system entries.

    \ingroup io
    \ingroup shared

    \compares equality

    QFileInfo provides information about a file system entry, such as its
    name, path, access rights and whether it is a regular file, directory or
    symbolic link. The entry's size and last modified/read times are also
    available. QFileInfo can also be used to obtain information about a Qt
    \l{resource system}{resource}.

    A QFileInfo can point to a file system entry with either an absolute or
    a relative path:
    \list
        \li \include qfileinfo.cpp absolute-path-unix-windows

        \li \include qfileinfo.cpp relative-path-note
    \endlist

    An example of an absolute path is the string \c {"/tmp/quartz"}. A relative
    path may look like \c {"src/fatlib"}. You can use the function isRelative()
    to check whether a QFileInfo is using a relative or an absolute path. You
    can call the function makeAbsolute() to convert a relative QFileInfo's
    path to an absolute path.

//! [qresource-virtual-fs-colon]
    \note Paths starting with a colon (\e{:}) are always considered
    absolute, as they denote a QResource.
//! [qresource-virtual-fs-colon]

    The file system entry path that the QFileInfo works on is set in the
    constructor or later with setFile(). Use exists() to see if the entry
    actually exists and size() to get its size.

    The file system entry's type is obtained with isFile(), isDir(), and
    isSymLink(). The symLinkTarget() function provides the absolute path of
    the target the symlink points to.

    The path elements of the file system entry can be extracted with path()
    and fileName(). The fileName()'s parts can be extracted with baseName(),
    suffix(), or completeSuffix(). QFileInfo objects referring to directories
    created by Qt classes will not have a trailing directory separator
    \c{'/'}. If you wish to use trailing separators in your own file info
    objects, just append one to the entry's path given to the constructors
    or setFile().

    Date and time related information are returned by birthTime(), fileTime(),
    lastModified(), lastRead(), and metadataChangeTime().
    Information about
    access permissions can be obtained with isReadable(), isWritable(), and
    isExecutable(). Ownership information can be obtained with
    owner(), ownerId(), group(), and groupId(). You can also examine
    permissions and ownership in a single statement using the permission()
    function.

    \section1 Symbolic Links and Shortcuts

    On Unix (including \macos and iOS), the property getter functions in
    this class return the properties such as times and size of the target,
    not the symlink, because Unix handles symlinks transparently. Opening
    a symlink using QFile effectively opens the link's target. For example:

    \snippet code/src_corelib_io_qfileinfo.cpp 0

    On Windows, shortcuts (\c .lnk files) are currently treated as symlinks. As
    on Unix systems, the property getters return the size of the target,
    not the \c .lnk file itself. This behavior is deprecated and will likely
    be removed in a future version of Qt, after which \c .lnk files will be
    treated as regular files.

    \snippet code/src_corelib_io_qfileinfo.cpp 1

    \section1 NTFS permissions

    On NTFS file systems, ownership and permissions checking is
    disabled by default for performance reasons. To enable it,
    include the following line:

    \snippet ntfsp.cpp 0

    Permission checking is then turned on and off by incrementing and
    decrementing \c qt_ntfs_permission_lookup by 1.

    \snippet ntfsp.cpp 1

    \note Since this is a non-atomic global variable, it is only safe
    to increment or decrement \c qt_ntfs_permission_lookup before any
    threads other than the main thread have started or after every thread
    other than the main thread has ended.

    \note From Qt 6.6 the variable \c qt_ntfs_permission_lookup is
    deprecated. Please use the following alternatives.

    The safe and easy way to manage permission checks is to use the RAII class
    \c QNtfsPermissionCheckGuard.

    \snippet ntfsp.cpp raii

    If you need more fine-grained control, it is possible to manage the permission
    with the following functions instead:

    \snippet ntfsp.cpp free-funcs

    \section1 Performance Considerations

    Some of QFileInfo's functions have to query the file system, but for
    performance reasons, some functions only operate on the path string.
    For example: To return the absolute path of a relative entry's path,
    absolutePath() has to query the file system. The path() function, however,
    can work on the file name directly, and so it is faster.

    QFileInfo also caches information about the file system entry it refers
    to. Because the file system can be changed by other users or programs,
    or even by other parts of the same program, there is a function that
    refreshes the information stored in QFileInfo, namely refresh(). To switch
    off a QFileInfo's caching (that is, force it to query the underlying file
    system every time you request information from it), call setCaching(false).

    Fetching information from the file system is typically done by calling
    (possibly) expensive system functions, so QFileInfo (depending on the
    implementation) might not fetch all the information from the file system
    at construction. To make sure that all information is read from the file
    system immediately, use the stat() member function.

    \l{birthTime()}, \l{fileTime()}, \l{lastModified()}, \l{lastRead()},
    and \l{metadataChangeTime()} return times in \e{local time} by default.
    Since native file system API typically uses UTC, this requires a conversion.
    If you don't actually need the local time, you can avoid this by requesting
    the time in QTimeZone::UTC directly.

    \section1 Platform Specific Issues

    \include android-content-uri-limitations.qdocinc

    \sa QDir, QFile
*/

/*!
    \fn QFileInfo &QFileInfo::operator=(QFileInfo &&other)

    Move-assigns \a other to this QFileInfo instance.

    \since 5.2
*/

/*!
    \internal
*/
QFileInfo::QFileInfo(QFileInfoPrivate *p) : d_ptr(p)
{
}

/*!
    Constructs an empty QFileInfo object that doesn't refer to any file
    system entry.

    \sa setFile()
*/
QFileInfo::QFileInfo() : d_ptr(new QFileInfoPrivate())
{
}

/*!
    Constructs a QFileInfo that gives information about a file system entry
    located at \a path that can be absolute or relative.

//! [preserve-relative-path]
    If \a path is relative, the QFileInfo will also have a relative path.
//! [preserve-relative-path]

    \sa setFile(), isRelative(), QDir::setCurrent(), QDir::isRelativePath()
*/
QFileInfo::QFileInfo(const QString &path) : d_ptr(new QFileInfoPrivate(path))
{
}

/*!
    Constructs a new QFileInfo that gives information about file \a
    file.

    If the \a file has a relative path, the QFileInfo will also have a
    relative path.

    \sa isRelative()
*/
QFileInfo::QFileInfo(const QFileDevice &file) : d_ptr(new QFileInfoPrivate(file.fileName()))
{
}

/*!
    Constructs a new QFileInfo that gives information about the given
    file system entry \a path that is relative to the directory \a dir.

//! [preserve-relative-or-absolute]
    If \a dir has a relative path, the QFileInfo will also have a
    relative path.

    If \a path is absolute, then the directory specified by \a dir
    will be disregarded.
//! [preserve-relative-or-absolute]

    \sa isRelative()
*/
QFileInfo::QFileInfo(const QDir &dir, const QString &path)
    : d_ptr(new QFileInfoPrivate(dir.filePath(path)))
{
}

/*!
    Constructs a new QFileInfo that is a copy of the given \a fileinfo.
*/
QFileInfo::QFileInfo(const QFileInfo &fileinfo)
    : d_ptr(fileinfo.d_ptr)
{

}

/*!
    Destroys the QFileInfo and frees its resources.
*/

QFileInfo::~QFileInfo()
{
}

/*!
    \fn bool QFileInfo::operator!=(const QFileInfo &lhs, const QFileInfo &rhs)

    Returns \c true if QFileInfo \a lhs refers to a different file system
    entry than the one referred to by \a rhs; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool QFileInfo::operator==(const QFileInfo &lhs, const QFileInfo &rhs)

    Returns \c true if QFileInfo \a lhs and QFileInfo \a rhs refer to the same
    entry on the file system; otherwise returns \c false.

    Note that the result of comparing two empty QFileInfo objects, containing
    no file system entry references (paths that do not exist or are empty),
    is undefined.

    \warning This will not compare two different symbolic links pointing to
    the same target.

    \warning On Windows, long and short paths that refer to the same file
    system entry are treated as if they referred to different entries.

    \sa operator!=()
*/
bool comparesEqual(const QFileInfo &lhs, const QFileInfo &rhs)
{
    if (rhs.d_ptr == lhs.d_ptr)
        return true;
    if (lhs.d_ptr->isDefaultConstructed || rhs.d_ptr->isDefaultConstructed)
        return false;

    // Assume files are the same if path is the same
    if (lhs.d_ptr->fileEntry.filePath() == rhs.d_ptr->fileEntry.filePath())
        return true;

    Qt::CaseSensitivity sensitive;
    if (lhs.d_ptr->fileEngine == nullptr || rhs.d_ptr->fileEngine == nullptr) {
        if (lhs.d_ptr->fileEngine != rhs.d_ptr->fileEngine) // one is native, the other is a custom file-engine
            return false;

        const bool lhsCaseSensitive = QFileSystemEngine::isCaseSensitive(lhs.d_ptr->fileEntry, lhs.d_ptr->metaData);
        if (lhsCaseSensitive != QFileSystemEngine::isCaseSensitive(rhs.d_ptr->fileEntry, rhs.d_ptr->metaData))
            return false;

        sensitive = lhsCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    } else {
        if (lhs.d_ptr->fileEngine->caseSensitive() != rhs.d_ptr->fileEngine->caseSensitive())
            return false;
        sensitive = lhs.d_ptr->fileEngine->caseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    }

   // Fallback to expensive canonical path computation
   return lhs.canonicalFilePath().compare(rhs.canonicalFilePath(), sensitive) == 0;
}

/*!
    Makes a copy of the given \a fileinfo and assigns it to this QFileInfo.
*/
QFileInfo &QFileInfo::operator=(const QFileInfo &fileinfo)
{
    d_ptr = fileinfo.d_ptr;
    return *this;
}

/*!
    \fn void QFileInfo::swap(QFileInfo &other)
    \since 5.0
    \memberswap{file info}
*/

/*!
    Sets the path of the file system entry that this QFileInfo provides
    information about to \a path that can be absolute or relative.

//! [absolute-path-unix-windows]
    On Unix, absolute paths begin with the directory separator \c {'/'}.
    On Windows, absolute paths begin with a drive specification (for example,
    \c {D:/}).
//! [ absolute-path-unix-windows]

//! [relative-path-note]
    Relative paths begin with a directory name or a regular file name and
    specify a file system entry's path relative to the current working
    directory.
//! [relative-path-note]

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 2

    \sa isRelative(), QDir::setCurrent(), QDir::isRelativePath()
*/
void QFileInfo::setFile(const QString &path)
{
    bool caching = d_ptr.constData()->cache_enabled;
    *this = QFileInfo(path);
    d_ptr->cache_enabled = caching;
}

/*!
    \overload

    Sets the file that the QFileInfo provides information about to \a
    file.

    If \a file includes a relative path, the QFileInfo will also have
    a relative path.

    \sa isRelative()
*/
void QFileInfo::setFile(const QFileDevice &file)
{
    setFile(file.fileName());
}

/*!
    \overload

    Sets the path of the file system entry that this QFileInfo provides
    information about to \a path in directory \a dir.

    \include qfileinfo.cpp preserve-relative-or-absolute

    \sa isRelative()
*/
void QFileInfo::setFile(const QDir &dir, const QString &path)
{
    setFile(dir.filePath(path));
}

/*!
    Returns the absolute full path to the file system entry this QFileInfo
    refers to, including the entry's name.

    \include qfileinfo.cpp absolute-path-unix-windows

//! [windows-network-shares]
    On Windows, the paths of network shares that are not mapped to a drive
    letter begin with \c{//sharename/}.
//! [windows-network-shares]

    QFileInfo will uppercase drive letters. Note that QDir does not do
    this. The code snippet below shows this.

    \snippet code/src_corelib_io_qfileinfo.cpp newstuff

    This function returns the same as filePath(), unless isRelative()
    is true. In contrast to canonicalFilePath(), symbolic links or
    redundant "." or ".." elements are not necessarily removed.

    \warning If filePath() is empty the behavior of this function
            is undefined.

    \sa filePath(), canonicalFilePath(), isRelative()
*/
QString QFileInfo::absoluteFilePath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->getFileName(QAbstractFileEngine::AbsoluteName);
}

/*!
    Returns the file system entry's canonical path, including the entry's
    name, that is, an absolute path without symbolic links or redundant
    \c{'.'} or \c{'..'} elements.

    If the entry does not exist, canonicalFilePath() returns an empty
    string.

    \sa filePath(), absoluteFilePath(), dir()
*/
QString QFileInfo::canonicalFilePath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->getFileName(QAbstractFileEngine::CanonicalName);
}


/*!
    Returns the absolute path of the file system entry this QFileInfo refers to,
    excluding the entry's name.

    \include qfileinfo.cpp absolute-path-unix-windows

    \include qfileinfo.cpp windows-network-shares

    In contrast to canonicalPath() symbolic links or redundant "." or
    ".." elements are not necessarily removed.

    \warning If filePath() is empty the behavior of this function
             is undefined.

    \sa absoluteFilePath(), path(), canonicalPath(), fileName(), isRelative()
*/
QString QFileInfo::absolutePath() const
{
    Q_D(const QFileInfo);

    if (d->isDefaultConstructed)
        return ""_L1;
    return d->getFileName(QAbstractFileEngine::AbsolutePathName);
}

/*!
    Returns the file system entry's canonical path (excluding the entry's name),
    i.e. an absolute path without symbolic links or redundant "." or ".." elements.

    If the entry does not exist, this method returns an empty string.

    \sa path(), absolutePath()
*/
QString QFileInfo::canonicalPath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->getFileName(QAbstractFileEngine::CanonicalPathName);
}

/*!
    Returns the path of the file system entry this QFileInfo refers to,
    excluding the entry's name.

    \include qfileinfo.cpp path-ends-with-slash-empty-name-component
    In this case, this function will return the entire path.

    \sa filePath(), absolutePath(), canonicalPath(), dir(), fileName(), isRelative()
*/
QString QFileInfo::path() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->fileEntry.path();
}

/*!
    \fn bool QFileInfo::isAbsolute() const

    Returns \c true if the file system entry's path is absolute, otherwise
    returns \c false (that is, the path is relative).

    \include qfileinfo.cpp qresource-virtual-fs-colon

    \sa isRelative()
*/

/*!
    Returns \c true if the file system entry's path is relative, otherwise
    returns \c false (that is, the path is absolute).

    \include qfileinfo.cpp absolute-path-unix-windows

    \include qfileinfo.cpp qresource-virtual-fs-colon

    \sa isAbsolute()
*/
bool QFileInfo::isRelative() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return true;
    if (d->fileEngine == nullptr)
        return d->fileEntry.isRelative();
    return d->fileEngine->isRelativePath();
}

/*!
    If the file system entry's path is relative, this method converts it to
    an absolute path and returns \c true; if the path is already absolute,
    this method returns \c false.

    \sa filePath(), isRelative()
*/
bool QFileInfo::makeAbsolute()
{
    if (d_ptr.constData()->isDefaultConstructed
            || !d_ptr.constData()->fileEntry.isRelative())
        return false;

    setFile(absoluteFilePath());
    return true;
}

/*!
    Returns \c true if the file system entry this QFileInfo refers to exists;
    otherwise returns \c false.

    \note If the entry is a symlink that points to a non-existing
    target, this method returns \c false.
*/
bool QFileInfo::exists() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == nullptr) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::ExistsAttribute))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::ExistsAttribute);
        return d->metaData.exists();
    }
    return d->getFileFlags(QAbstractFileEngine::ExistsFlag);
}

/*!
    \since 5.2

    Returns \c true if the file system entry \a path exists; otherwise
    returns \c false.

    \note If \a path is a symlink that points to a non-existing
    target, this method returns \c false.

    \note Using this function is faster than using
    \c QFileInfo(path).exists() for file system access.
*/
bool QFileInfo::exists(const QString &path)
{
    if (path.isEmpty())
        return false;
    QFileSystemEntry entry(path);
    QFileSystemMetaData data;
    // Expensive fallback to non-QFileSystemEngine implementation
    if (auto engine = QFileSystemEngine::createLegacyEngine(entry, data))
        return QFileInfo(new QFileInfoPrivate(entry, data, std::move(engine))).exists();

    QFileSystemEngine::fillMetaData(entry, data, QFileSystemMetaData::ExistsAttribute);
    return data.exists();
}

/*!
    Refreshes the information about the file system entry this QFileInfo
    refers to, that is, reads in information from the file system the next
    time a cached property is fetched.
*/
void QFileInfo::refresh()
{
    Q_D(QFileInfo);
    d->clear();
}

/*!
    Returns the path of the file system entry this QFileInfo refers to;
    the path may be absolute or relative.

    \sa absoluteFilePath(), canonicalFilePath(), isRelative()
*/
QString QFileInfo::filePath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->fileEntry.filePath();
}

/*!
    Returns the name of the file system entry this QFileInfo refers to,
    excluding the path.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 3

//! [path-ends-with-slash-empty-name-component]
    \note If this QFileInfo is given a path ending with a directory separator
    \c{'/'}, the entry's name part is considered empty.
//! [path-ends-with-slash-empty-name-component]

    \sa isRelative(), filePath(), baseName(), suffix()
*/
QString QFileInfo::fileName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    if (!d->fileEngine)
        return d->fileEntry.fileName();
    return d->fileEngine->fileName(QAbstractFileEngine::BaseName);
}

/*!
    \since 4.3
    Returns the name of the bundle.

    On \macos and iOS this returns the proper localized name for a bundle if the
    path isBundle(). On all other platforms an empty QString is returned.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 4

    \sa isBundle(), filePath(), baseName(), suffix()
*/
QString QFileInfo::bundleName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->getFileName(QAbstractFileEngine::BundleName);
}

/*!
    Returns the base name of the file without the path.

    The base name consists of all characters in the file up to (but
    not including) the \e first '.' character.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 5


    The base name of a file is computed equally on all platforms, independent
    of file naming conventions (e.g., ".bashrc" on Unix has an empty base
    name, and the suffix is "bashrc").

    \sa fileName(), suffix(), completeSuffix(), completeBaseName()
*/
QString QFileInfo::baseName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    if (!d->fileEngine)
        return d->fileEntry.baseName();
    return QFileSystemEntry(d->fileEngine->fileName(QAbstractFileEngine::BaseName)).baseName();
}

/*!
    Returns the complete base name of the file without the path.

    The complete base name consists of all characters in the file up
    to (but not including) the \e last '.' character.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 6

    \sa fileName(), suffix(), completeSuffix(), baseName()
*/
QString QFileInfo::completeBaseName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    if (!d->fileEngine)
        return d->fileEntry.completeBaseName();
    const QString fileEngineBaseName = d->fileEngine->fileName(QAbstractFileEngine::BaseName);
    return QFileSystemEntry(fileEngineBaseName).completeBaseName();
}

/*!
    Returns the complete suffix (extension) of the file.

    The complete suffix consists of all characters in the file after
    (but not including) the first '.'.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 7

    \sa fileName(), suffix(), baseName(), completeBaseName()
*/
QString QFileInfo::completeSuffix() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->fileEntry.completeSuffix();
}

/*!
    Returns the suffix (extension) of the file.

    The suffix consists of all characters in the file after (but not
    including) the last '.'.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 8

    The suffix of a file is computed equally on all platforms, independent of
    file naming conventions (e.g., ".bashrc" on Unix has an empty base name,
    and the suffix is "bashrc").

    \sa fileName(), completeSuffix(), baseName(), completeBaseName()
*/
QString QFileInfo::suffix() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->fileEntry.suffix();
}


/*!
    Returns a QDir object representing the path of the parent directory of the
    file system entry that this QFileInfo refers to.

    \note The QDir returned always corresponds to the object's
    parent directory, even if the QFileInfo represents a directory.

    For each of the following, dir() returns the QDir
    \c{"~/examples/191697"}.

    \snippet fileinfo/main.cpp 0

    For each of the following, dir() returns the QDir
    \c{"."}.

    \snippet fileinfo/main.cpp 1

    \sa absolutePath(), filePath(), fileName(), isRelative(), absoluteDir()
*/
QDir QFileInfo::dir() const
{
    Q_D(const QFileInfo);
    return QDir(d->fileEntry.path());
}

/*!
    Returns a QDir object representing the absolute path of the parent
    directory of the file system entry that this QFileInfo refers to.

    \snippet code/src_corelib_io_qfileinfo.cpp 11

    \sa dir(), filePath(), fileName(), isRelative()
*/
QDir QFileInfo::absoluteDir() const
{
    return QDir(absolutePath());
}

/*!
    Returns \c true if the user can read the file system entry this QFileInfo
    refers to; otherwise returns \c false.

    \include qfileinfo.cpp info-about-target-not-symlink

    \note If the \l{NTFS permissions} check has not been enabled, the result
    on Windows will merely reflect whether the entry exists.

    \sa isWritable(), isExecutable(), permission()
*/
bool QFileInfo::isReadable() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::UserReadPermission,
                [d]() { return d->metaData.isReadable(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::ReadUserPerm); });
}

/*!
    Returns \c true if the user can write to the file system entry this
    QFileInfo refers to; otherwise returns \c false.

    \include qfileinfo.cpp info-about-target-not-symlink

    \note If the \l{NTFS permissions} check has not been enabled, the result on
    Windows will merely reflect whether the entry is marked as Read Only.

    \sa isReadable(), isExecutable(), permission()
*/
bool QFileInfo::isWritable() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::UserWritePermission,
                [d]() { return d->metaData.isWritable(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::WriteUserPerm); });
}

/*!
    Returns \c true if the file system entry this QFileInfo refers to is
    executable; otherwise returns \c false.

//! [info-about-target-not-symlink]
    If the file is a symlink, this function returns information about the
    target, not the symlink.
//! [info-about-target-not-symlink]

    \sa isReadable(), isWritable(), permission()
*/
bool QFileInfo::isExecutable() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::UserExecutePermission,
                [d]() { return d->metaData.isExecutable(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::ExeUserPerm); });
}

/*!
    Returns \c true if the file system entry this QFileInfo refers to is
    `hidden'; otherwise returns \c false.

    \b{Note:} This function returns \c true for the special entries "." and
    ".." on Unix, even though QDir::entryList treats them as shown. And note
    that, since this function inspects the file name, on Unix it will inspect
    the name of the symlink, if this file is a symlink, not the target's name.

    On Windows, this function returns \c true if the target file is hidden (not
    the symlink).
*/
bool QFileInfo::isHidden() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::HiddenAttribute,
                [d]() { return d->metaData.isHidden(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::HiddenFlag); });
}

/*!
    \since 5.0
    Returns \c true if the file path can be used directly with native APIs.
    Returns \c false if the file is otherwise supported by a virtual file system
    inside Qt, such as \l{the Qt Resource System}.

    \b{Note:} Native paths may still require conversion of path separators
    and character encoding, depending on platform and input requirements of the
    native API.

    \sa QDir::toNativeSeparators(), QFile::encodeName(), filePath(),
    absoluteFilePath(), canonicalFilePath()
*/
bool QFileInfo::isNativePath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == nullptr)
        return true;
    return d->getFileFlags(QAbstractFileEngine::LocalDiskFlag);
}

/*!
    Returns \c true if this object points to a file or to a symbolic
    link to a file. Returns \c false if the
    object points to something that is not a file (such as a directory)
    or that does not exist.

    \include qfileinfo.cpp info-about-target-not-symlink

    \sa isDir(), isSymLink(), isBundle()
*/
bool QFileInfo::isFile() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::FileType,
                [d]() { return d->metaData.isFile(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::FileType); });
}

/*!
    Returns \c true if this object points to a directory or to a symbolic
    link to a directory. Returns \c false if the
    object points to something that is not a directory (such as a file)
    or that does not exist.

    \include qfileinfo.cpp info-about-target-not-symlink

    \sa isFile(), isSymLink(), isBundle()
*/
bool QFileInfo::isDir() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::DirectoryType,
                [d]() { return d->metaData.isDirectory(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::DirectoryType); });
}


/*!
    \since 4.3
    Returns \c true if this object points to a bundle or to a symbolic
    link to a bundle on \macos and iOS; otherwise returns \c false.

    \include qfileinfo.cpp info-about-target-not-symlink

    \sa isDir(), isSymLink(), isFile()
*/
bool QFileInfo::isBundle() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::BundleType,
                [d]() { return d->metaData.isBundle(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::BundleType); });
}

/*!
    Returns \c true if this object points to a symbolic link, shortcut,
    or alias; otherwise returns \c false.

    Symbolic links exist on Unix (including \macos and iOS) and Windows
    and are typically created by the \c{ln -s} or \c{mklink} commands,
    respectively. Opening a symbolic link effectively opens
    the \l{symLinkTarget()}{link's target}.

    In addition, true will be returned for shortcuts (\c *.lnk files) on
    Windows, and aliases on \macos. This behavior is deprecated and will
    likely change in a future version of Qt. Opening a shortcut or alias
    will open the \c .lnk or alias file itself.

    Example:

    \snippet code/src_corelib_io_qfileinfo.cpp 9

//! [symlink-target-exists-behavior]
    \note exists() returns \c true if the symlink points to an existing
    target, otherwise it returns \c false.
//! [symlink-target-exists-behavior]

    \sa isFile(), isDir(), symLinkTarget()
*/
bool QFileInfo::isSymLink() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::LegacyLinkType,
                [d]() { return d->metaData.isLegacyLink(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::LinkType); });
}

/*!
    Returns \c true if this object points to a symbolic link;
    otherwise returns \c false.

    Symbolic links exist on Unix (including \macos and iOS) and Windows
    (NTFS-symlink) and are typically created by the \c{ln -s} or \c{mklink}
    commands, respectively.

    Unix handles symlinks transparently. Opening a symbolic link effectively
    opens the \l{symLinkTarget()}{link's target}.

    In contrast to isSymLink(), false will be returned for shortcuts
    (\c *.lnk files) on Windows and aliases on \macos. Use QFileInfo::isShortcut()
    and QFileInfo::isAlias() instead.

    \include qfileinfo.cpp symlink-target-exists-behavior

    \sa isFile(), isDir(), isShortcut(), symLinkTarget()
*/

bool QFileInfo::isSymbolicLink() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::LegacyLinkType,
                [d]() { return d->metaData.isLink(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::LinkType); });
}

/*!
    \since 6.10

    Returns \c true if this QFileInfo refers to a file system entry that is
    \e not a directory, regular file or symbolic link. Otherwise returns
    \c false.

    If this QFileInfo refers to a nonexistent entry, this method returns
    \c false.

    If the entry is a dangling symbolic link (the target doesn't exist), this
    method returns \c false. For a non-dangling symbolic link, this function
    returns information about the target, not the symbolic link.

    On Unix a special (other) file system entry is a FIFO, socket, character
    device, or block device. For more details, see the
    \l{https://pubs.opengroup.org/onlinepubs/9699919799/functions/mknod.html}{\c mknod}
    manual page.

    On Windows (for historical reasons, see \l{Symbolic Links and Shortcuts})
    this method returns \c true for \c .lnk files.

    \sa isDir(), isFile(), isSymLink(), QDirListing::IteratorFlag::ExcludeOther
*/
bool QFileInfo::isOther() const
{
    Q_D(const QFileInfo);
    using M = QFileSystemMetaData::MetaDataFlag;
    // No M::LinkType to make QFileSystemEngine always call stat().
    // M::WinLnkType is only relevant on Windows for '.lnk' files
    constexpr auto mdFlags = M::ExistsAttribute | M::DirectoryType | M::FileType | M::WinLnkType;

    auto fsLambda = [d]() {
        // Check isLnkFile() first because currently exists() returns false for
        // a broken '.lnk' where the target doesn't exist.
        if (d->metaData.isLnkFile()) // Always false on non-Windows OSes
            return true;
        return d->metaData.exists() && !d->metaData.isDirectory() && !d->metaData.isFile();
    };

    auto engineLambda = [d]() {
        using F = QAbstractFileEngine::FileFlag;
        return d->getFileFlags(F::ExistsFlag)
            && !d->getFileFlags(F::LinkType) // QAFE doesn't have a separate type for ".lnk" file
            && !d->getFileFlags(F::DirectoryType)
            && !d->getFileFlags(F::FileType);
    };

    return d->checkAttribute<bool>(mdFlags, std::move(fsLambda), std::move(engineLambda));
}

/*!
    Returns \c true if this object points to a shortcut;
    otherwise returns \c false.

    Shortcuts only exist on Windows and are typically \c .lnk files.
    For instance, true will be returned for shortcuts (\c *.lnk files) on
    Windows, but false will be returned on Unix (including \macos and iOS).

    The shortcut (.lnk) files are treated as regular files. Opening those will
    open the \c .lnk file itself. In order to open the file a shortcut
    references to, it must uses symLinkTarget() on a shortcut.

    \note Even if a shortcut (broken shortcut) points to a non existing file,
    isShortcut() returns true.

    \sa isFile(), isDir(), isSymbolicLink(), symLinkTarget()
*/
bool QFileInfo::isShortcut() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
            QFileSystemMetaData::LegacyLinkType,
            [d]() { return d->metaData.isLnkFile(); },
            [d]() { return d->getFileFlags(QAbstractFileEngine::LinkType); });
}

/*!
    Returns \c true if this object points to an alias;
    otherwise returns \c false.

    \since 6.4

    Aliases only exist on \macos. They are treated as regular files, so
    opening an alias will open the file itself. In order to open the file
    or directory an alias references use symLinkTarget().

    \note Even if an alias points to a non existing file,
    isAlias() returns true.

    \sa isFile(), isDir(), isSymLink(), symLinkTarget()
*/
bool QFileInfo::isAlias() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
            QFileSystemMetaData::LegacyLinkType,
            [d]() { return d->metaData.isAlias(); },
            [d]() { return d->getFileFlags(QAbstractFileEngine::LinkType); });
}

/*!
    \since 5.15

    Returns \c true if the object points to a junction;
    otherwise returns \c false.

    Junctions only exist on Windows' NTFS file system, and are typically
    created by the \c{mklink} command. They can be thought of as symlinks for
    directories, and can only be created for absolute paths on the local
    volume.
*/
bool QFileInfo::isJunction() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
            QFileSystemMetaData::LegacyLinkType,
            [d]() { return d->metaData.isJunction(); },
            [d]() { return d->getFileFlags(QAbstractFileEngine::LinkType); });
}

/*!
    Returns \c true if the object points to a directory or to a symbolic
    link to a directory, and that directory is the root directory; otherwise
    returns \c false.
*/
bool QFileInfo::isRoot() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == nullptr) {
        if (d->fileEntry.isRoot()) {
#if defined(Q_OS_WIN)
            //the path is a drive root, but the drive may not exist
            //for backward compatibility, return true only if the drive exists
            if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::ExistsAttribute))
                QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::ExistsAttribute);
            return d->metaData.exists();
#else
            return true;
#endif
        }
        return false;
    }
    return d->getFileFlags(QAbstractFileEngine::RootFlag);
}

/*!
    \since 4.2

    Returns the absolute path to the file or directory a symbolic link
    points to, or an empty string if the object isn't a symbolic
    link.

    This name may not represent an existing file; it is only a string.

    \include qfileinfo.cpp symlink-target-exists-behavior

    \sa exists(), isSymLink(), isDir(), isFile()
*/
QString QFileInfo::symLinkTarget() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->getFileName(QAbstractFileEngine::AbsoluteLinkTarget);
}

/*!
    \since 6.6
    Read the path the symlink references.

    Returns the raw path referenced by the symbolic link, without resolving a relative
    path relative to the directory containing the symbolic link. The returned string will
    only be an absolute path if the symbolic link actually references it as such. Returns
    an empty string if the object is not a symbolic link.

    \sa symLinkTarget(), exists(), isSymLink(), isDir(), isFile()
*/
QString QFileInfo::readSymLink() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return {};
    return d->getFileName(QAbstractFileEngine::RawLinkPath);
}

/*!
    \since 6.2

    Resolves an NTFS junction to the path it references.

    Returns the absolute path to the directory an NTFS junction points to, or
    an empty string if the object is not an NTFS junction.

    There is no guarantee that the directory named by the NTFS junction actually
    exists.

    \sa isJunction(), isFile(), isDir(), isSymLink(), isSymbolicLink(),
        isShortcut()
*/
QString QFileInfo::junctionTarget() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->getFileName(QAbstractFileEngine::JunctionName);
}

/*!
    Returns the owner of the file. On systems where files
    do not have owners, or if an error occurs, an empty string is
    returned.

    This function can be time consuming under Unix (in the order of
    milliseconds). On Windows, it will return an empty string unless
    the \l{NTFS permissions} check has been enabled.

    \include qfileinfo.cpp info-about-target-not-symlink

    \sa ownerId(), group(), groupId()
*/
QString QFileInfo::owner() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->getFileOwner(QAbstractFileEngine::OwnerUser);
}

/*!
    Returns the id of the owner of the file.

    On Windows and on systems where files do not have owners this
    function returns ((uint) -2).

    \include qfileinfo.cpp info-about-target-not-symlink

    \sa owner(), group(), groupId()
*/
uint QFileInfo::ownerId() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute(uint(-2),
                QFileSystemMetaData::UserId,
                [d]() { return d->metaData.userId(); },
                [d]() { return d->fileEngine->ownerId(QAbstractFileEngine::OwnerUser); });
}

/*!
    Returns the group of the file. On Windows, on systems where files
    do not have groups, or if an error occurs, an empty string is
    returned.

    This function can be time consuming under Unix (in the order of
    milliseconds).

    \include qfileinfo.cpp info-about-target-not-symlink

    \sa groupId(), owner(), ownerId()
*/
QString QFileInfo::group() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return ""_L1;
    return d->getFileOwner(QAbstractFileEngine::OwnerGroup);
}

/*!
    Returns the id of the group the file belongs to.

    On Windows and on systems where files do not have groups this
    function always returns (uint) -2.

    \include qfileinfo.cpp info-about-target-not-symlink

    \sa group(), owner(), ownerId()
*/
uint QFileInfo::groupId() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute(uint(-2),
                QFileSystemMetaData::GroupId,
                [d]() { return d->metaData.groupId(); },
                [d]() { return d->fileEngine->ownerId(QAbstractFileEngine::OwnerGroup); });
}

/*!
    Tests for file permissions. The \a permissions argument can be
    several flags of type QFile::Permissions OR-ed together to check
    for permission combinations.

    On systems where files do not have permissions this function
    always returns \c true.

    \note The result might be inaccurate on Windows if the
    \l{NTFS permissions} check has not been enabled.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 10

    \include qfileinfo.cpp info-about-target-not-symlink

    \sa isReadable(), isWritable(), isExecutable()
*/
bool QFileInfo::permission(QFile::Permissions permissions) const
{
    Q_D(const QFileInfo);
    // the QFileSystemMetaData::MetaDataFlag and QFile::Permissions overlap, so just cast.
    auto fseFlags = QFileSystemMetaData::MetaDataFlags::fromInt(permissions.toInt());
    auto feFlags = QAbstractFileEngine::FileFlags::fromInt(permissions.toInt());
    return d->checkAttribute<bool>(
                fseFlags,
                [=]() { return (d->metaData.permissions() & permissions) == permissions; },
        [=]() {
            return d->getFileFlags(feFlags) == uint(permissions.toInt());
        });
}

/*!
    Returns the complete OR-ed together combination of
    QFile::Permissions for the file.

    \note The result might be inaccurate on Windows if the
    \l{NTFS permissions} check has not been enabled.

    \include qfileinfo.cpp info-about-target-not-symlink
*/
QFile::Permissions QFileInfo::permissions() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<QFile::Permissions>(
                QFileSystemMetaData::Permissions,
                [d]() { return d->metaData.permissions(); },
        [d]() {
            return QFile::Permissions(d->getFileFlags(QAbstractFileEngine::PermsMask) & QAbstractFileEngine::PermsMask);
        });
}


/*!
    Returns the file size in bytes. If the file does not exist or cannot be
    fetched, 0 is returned.

    \include qfileinfo.cpp info-about-target-not-symlink

    \sa exists()
*/
qint64 QFileInfo::size() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<qint64>(
                QFileSystemMetaData::SizeAttribute,
                [d]() { return d->metaData.size(); },
        [d]() {
            if (!d->getCachedFlag(QFileInfoPrivate::CachedSize)) {
                d->setCachedFlag(QFileInfoPrivate::CachedSize);
                d->fileSize = d->fileEngine->size();
            }
            return d->fileSize;
        });
}

/*!
    \fn QDateTime QFileInfo::birthTime() const

    Returns the date and time when the file was created (born), in local time.

    If the file birth time is not available, this function returns an invalid QDateTime.

    \include qfileinfo.cpp info-about-target-not-symlink

    This function overloads QFileInfo::birthTime(const QTimeZone &tz), and
    returns the same as \c{birthTime(QTimeZone::LocalTime)}.

    \since 5.10
    \sa lastModified(), lastRead(), metadataChangeTime(), fileTime()
*/

/*!
    \fn QDateTime QFileInfo::birthTime(const QTimeZone &tz) const

    Returns the date and time when the file was created (born).

    \include qfileinfo.cpp file-times-in-time-zone

    If the file birth time is not available, this function returns an invalid
    QDateTime.

    \include qfileinfo.cpp info-about-target-not-symlink

    \since 6.6
    \sa lastModified(const QTimeZone &), lastRead(const QTimeZone &),
        metadataChangeTime(const QTimeZone &),
        fileTime(QFileDevice::FileTime, const QTimeZone &)
*/

/*!
    \fn QDateTime QFileInfo::metadataChangeTime() const

    Returns the date and time when the file's metadata was last changed,
    in local time.

    A metadata change occurs when the file is first created, but it also
    occurs whenever the user writes or sets inode information (for example,
    changing the file permissions).

    \include qfileinfo.cpp info-about-target-not-symlink

    This function overloads QFileInfo::metadataChangeTime(const QTimeZone &tz),
    and returns the same as \c{metadataChangeTime(QTimeZone::LocalTime)}.

    \since 5.10
    \sa birthTime(), lastModified(), lastRead(), fileTime()
*/

/*!
    \fn QDateTime QFileInfo::metadataChangeTime(const QTimeZone &tz) const

    Returns the date and time when the file's metadata was last changed.
    A metadata change occurs when the file is first created, but it also
    occurs whenever the user writes or sets inode information (for example,
    changing the file permissions).

    \include qfileinfo.cpp file-times-in-time-zone

    \include qfileinfo.cpp info-about-target-not-symlink

    \since 6.6
    \sa birthTime(const QTimeZone &), lastModified(const QTimeZone &),
        lastRead(const QTimeZone &),
        fileTime(QFileDevice::FileTime time, const QTimeZone &)
*/

/*!
    \fn QDateTime QFileInfo::lastModified() const

    Returns the date and time when the file was last modified.

    \include qfileinfo.cpp info-about-target-not-symlink

    This function overloads \l{QFileInfo::lastModified(const QTimeZone &)},
    and returns the same as \c{lastModified(QTimeZone::LocalTime)}.

    \sa birthTime(), lastRead(), metadataChangeTime(), fileTime()
*/

/*!
    \fn QDateTime QFileInfo::lastModified(const QTimeZone &tz) const

    Returns the date and time when the file was last modified.

    \include qfileinfo.cpp file-times-in-time-zone

    \include qfileinfo.cpp info-about-target-not-symlink

    \since 6.6
    \sa birthTime(const QTimeZone &), lastRead(const QTimeZone &),
        metadataChangeTime(const QTimeZone &),
        fileTime(QFileDevice::FileTime, const QTimeZone &)
*/

/*!
    \fn QDateTime QFileInfo::lastRead() const

    Returns the date and time when the file was last read (accessed).

    On platforms where this information is not available, returns the same
    time as lastModified().

    \include qfileinfo.cpp info-about-target-not-symlink

    This function overloads \l{QFileInfo::lastRead(const QTimeZone &)},
    and returns the same as \c{lastRead(QTimeZone::LocalTime)}.

    \sa birthTime(), lastModified(), metadataChangeTime(), fileTime()
*/

/*!
    \fn QDateTime QFileInfo::lastRead(const QTimeZone &tz) const

    Returns the date and time when the file was last read (accessed).

    \include qfileinfo.cpp file-times-in-time-zone

    On platforms where this information is not available, returns the same
    time as lastModified().

    \include qfileinfo.cpp info-about-target-not-symlink

    \since 6.6
    \sa birthTime(const QTimeZone &), lastModified(const QTimeZone &),
        metadataChangeTime(const QTimeZone &),
        fileTime(QFileDevice::FileTime, const QTimeZone &)
*/

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
/*!
    Returns the file time specified by \a time.

    If the time cannot be determined, an invalid date time is returned.

    \include qfileinfo.cpp info-about-target-not-symlink

    This function overloads
    \l{QFileInfo::fileTime(QFileDevice::FileTime, const QTimeZone &)},
    and returns the same as \c{fileTime(time, QTimeZone::LocalTime)}.

    \since 5.10
    \sa birthTime(), lastModified(), lastRead(), metadataChangeTime()
*/
QDateTime QFileInfo::fileTime(QFile::FileTime time) const {
    return fileTime(time, QTimeZone::LocalTime);
}
#endif

/*!
    Returns the file time specified by \a time.

//! [file-times-in-time-zone]
    The returned time is in the time zone specified by \a tz. For example,
    you can use QTimeZone::LocalTime or QTimeZone::UTC to get the time in
    the Local time zone or UTC, respectively. Since native file system API
    typically uses UTC, using QTimeZone::UTC is often faster, as it does not
    require any conversions.
//! [file-times-in-time-zone]

    If the time cannot be determined, an invalid date time is returned.

    \include qfileinfo.cpp info-about-target-not-symlink

    \since 6.6
    \sa birthTime(const QTimeZone &), lastModified(const QTimeZone &),
        lastRead(const QTimeZone &), metadataChangeTime(const QTimeZone &),
        QDateTime::isValid()
*/
QDateTime QFileInfo::fileTime(QFile::FileTime time, const QTimeZone &tz) const
{
    Q_D(const QFileInfo);
    QFileSystemMetaData::MetaDataFlags flag;
    switch (time) {
    case QFile::FileAccessTime:
        flag = QFileSystemMetaData::AccessTime;
        break;
    case QFile::FileBirthTime:
        flag = QFileSystemMetaData::BirthTime;
        break;
    case QFile::FileMetadataChangeTime:
        flag = QFileSystemMetaData::MetadataChangeTime;
        break;
    case QFile::FileModificationTime:
        flag = QFileSystemMetaData::ModificationTime;
        break;
    }

    auto fsLambda = [d, time]() { return d->metaData.fileTime(time); };
    auto engineLambda = [d, time]() { return d->getFileTime(time); };
    const auto dt =
        d->checkAttribute<QDateTime>(flag, std::move(fsLambda), std::move(engineLambda));
    return dt.toTimeZone(tz);
}

/*!
    \internal
*/
QFileInfoPrivate* QFileInfo::d_func()
{
    return d_ptr.data();
}

/*!
    Returns \c true if caching is enabled; otherwise returns \c false.

    \sa setCaching(), refresh()
*/
bool QFileInfo::caching() const
{
    Q_D(const QFileInfo);
    return d->cache_enabled;
}

/*!
    If \a enable is true, enables caching of file information. If \a
    enable is false caching is disabled.

    When caching is enabled, QFileInfo reads the file information from
    the file system the first time it's needed, but generally not
    later.

    Caching is enabled by default.

    \sa refresh(), caching()
*/
void QFileInfo::setCaching(bool enable)
{
    Q_D(QFileInfo);
    d->cache_enabled = enable;
}

/*!
    Reads all attributes from the file system.
    \since 6.0

    This is useful when information about the file system is collected in a
    worker thread, and then passed to the UI in the form of caching QFileInfo
    instances.

    \sa setCaching(), refresh()
*/
void QFileInfo::stat()
{
    Q_D(QFileInfo);
    QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::AllMetaDataFlags);
}

/*!
    \typedef QFileInfoList
    \relates QFileInfo

    Synonym for QList<QFileInfo>.
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QFileInfo &fi)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg.noquote();
    dbg << "QFileInfo(" << QDir::toNativeSeparators(fi.filePath()) << ')';
    return dbg;
}
#endif

/*!
    \fn QFileInfo::QFileInfo(const std::filesystem::path &file)
    \since 6.0

    Constructs a new QFileInfo that gives information about the given
    \a file.

    \sa setFile(), isRelative(), QDir::setCurrent(), QDir::isRelativePath()
*/
/*!
    \fn QFileInfo::QFileInfo(const QDir &dir, const std::filesystem::path &path)
    \since 6.0

    Constructs a new QFileInfo that gives information about the file system
    entry at \a path that is relative to the directory \a dir.

    \include qfileinfo.cpp preserve-relative-or-absolute
*/
/*!
    \fn void QFileInfo::setFile(const std::filesystem::path &path)
    \since 6.0

    Sets the path of file system entry that this QFileInfo provides
    information about to \a path.

    \include qfileinfo.cpp preserve-relative-path
*/
/*!
    \fn std::filesystem::path QFileInfo::filesystemFilePath() const
    \since 6.0

    Returns filePath() as a \c{std::filesystem::path}.
    \sa filePath()
*/
/*!
    \fn std::filesystem::path QFileInfo::filesystemAbsoluteFilePath() const
    \since 6.0

    Returns absoluteFilePath() as a \c{std::filesystem::path}.
    \sa absoluteFilePath()
*/
/*!
    \fn std::filesystem::path QFileInfo::filesystemCanonicalFilePath() const
    \since 6.0

    Returns canonicalFilePath() as a \c{std::filesystem::path}.
    \sa canonicalFilePath()
*/
/*!
    \fn std::filesystem::path QFileInfo::filesystemPath() const
    \since 6.0

    Returns path() as a \c{std::filesystem::path}.
    \sa path()
*/
/*!
    \fn std::filesystem::path QFileInfo::filesystemAbsolutePath() const
    \since 6.0

    Returns absolutePath() as a \c{std::filesystem::path}.
    \sa absolutePath()
*/
/*!
    \fn std::filesystem::path QFileInfo::filesystemCanonicalPath() const
    \since 6.0

    Returns canonicalPath() as a \c{std::filesystem::path}.
    \sa canonicalPath()
*/
/*!
    \fn std::filesystem::path QFileInfo::filesystemSymLinkTarget() const
    \since 6.0

    Returns symLinkTarget() as a \c{std::filesystem::path}.
    \sa symLinkTarget()
*/
/*!
    \fn std::filesystem::path QFileInfo::filesystemReadSymLink() const
    \since 6.6

    Returns readSymLink() as a \c{std::filesystem::path}.
    \sa readSymLink()
*/
/*!
    \fn std::filesystem::path QFileInfo::filesystemJunctionTarget() const
    \since 6.2

    Returns junctionTarget() as a \c{std::filesystem::path}.
    \sa junctionTarget()
*/
/*!
    \macro QT_IMPLICIT_QFILEINFO_CONSTRUCTION
    \since 6.0
    \relates QFileInfo

    Defining this macro makes most QFileInfo constructors implicit
    instead of explicit. Since construction of QFileInfo objects is
    expensive, one should avoid accidentally creating them, especially
    if cheaper alternatives exist. For instance:

    \badcode

    QDirIterator it(dir);
    while (it.hasNext()) {
        // Implicit conversion from QString (returned by it.next()):
        // may create unnecessary data structures and cause additional
        // accesses to the file system. Unless this macro is defined,
        // this line does not compile.

        QFileInfo fi = it.next();

        ~~~
    }

    \endcode

    Instead, use the right API:

    \code

    QDirIterator it(dir);
    while (it.hasNext()) {
        // Extract the QFileInfo from the iterator directly:
        QFileInfo fi = it.nextFileInfo();

        ~~~
    }

    \endcode

    Construction from QString, QFile, and so on is always possible by
    using direct initialization instead of copy initialization:

    \code

    QFileInfo fi1 = some_string; // Does not compile unless this macro is defined
    QFileInfo fi2(some_string);  // OK
    QFileInfo fi3{some_string};  // Possibly better, avoids the risk of the Most Vexing Parse
    auto fi4 = QFileInfo(some_string); // OK

    \endcode

    This macro is provided for compatibility reason. Its usage is not
    recommended in new code.
*/

QT_END_NAMESPACE
