// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qplatformdefs.h"
#include "qdebug.h"
#include "qfile.h"
#include "qfileinfo.h"
#include "qfsfileengine_p.h"
#include "qlist.h"
#include "qsavefile.h"
#include "qtemporaryfile.h"
#include "private/qiodevice_p.h"
#include "private/qfile_p.h"
#include "private/qfilesystemengine_p.h"
#include "private/qsavefile_p.h"
#include "private/qsystemerror_p.h"
#include "private/qtemporaryfile_p.h"
#if defined(QT_BUILD_CORE_LIB)
# include "qcoreapplication.h"
#endif

#ifdef QT_NO_QOBJECT
#define tr(X) QString::fromLatin1(X)
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_DECL_COLD_FUNCTION
static bool file_already_open(QFile &file, const char *where = nullptr)
{
    qWarning("QFile::%s: File (%ls) already open", where ? where : "open", qUtf16Printable(file.fileName()));
    return false;
}

//************* QFilePrivate
QFilePrivate::QFilePrivate()
{
}

QFilePrivate::~QFilePrivate()
{
}

bool
QFilePrivate::openExternalFile(QIODevice::OpenMode flags, int fd, QFile::FileHandleFlags handleFlags)
{
#ifdef QT_NO_FSFILEENGINE
    Q_UNUSED(flags);
    Q_UNUSED(fd);
    return false;
#else
    auto fs = std::make_unique<QFSFileEngine>();
    auto fe = fs.get();
    fileEngine = std::move(fs);
    return fe->open(flags, fd, handleFlags);
#endif
}

bool
QFilePrivate::openExternalFile(QIODevice::OpenMode flags, FILE *fh, QFile::FileHandleFlags handleFlags)
{
#ifdef QT_NO_FSFILEENGINE
    Q_UNUSED(flags);
    Q_UNUSED(fh);
    return false;
#else
    auto fs = std::make_unique<QFSFileEngine>();
    auto fe = fs.get();
    fileEngine = std::move(fs);
    return fe->open(flags, fh, handleFlags);
#endif
}

QAbstractFileEngine *QFilePrivate::engine() const
{
    if (!fileEngine)
        fileEngine = QAbstractFileEngine::create(fileName);
    return fileEngine.get();
}

//************* QFile

/*!
    \class QFile
    \inmodule QtCore
    \brief The QFile class provides an interface for reading from and writing to files.

    \ingroup io

    \reentrant

    QFile is an I/O device for reading and writing text and binary
    files and \l{The Qt Resource System}{resources}. A QFile may be
    used by itself or, more conveniently, with a QTextStream or
    QDataStream.

    The file name is usually passed in the constructor, but it can be
    set at any time using setFileName(). QFile expects the file
    separator to be '/' regardless of operating system. The use of
    other separators (e.g., '\\') is not supported.

    You can check for a file's existence using exists(), and remove a
    file using remove(). (More advanced file system related operations
    are provided by QFileInfo and QDir.)

    The file is opened with open(), closed with close(), and flushed
    with flush(). Data is usually read and written using QDataStream
    or QTextStream, but you can also call the QIODevice-inherited
    functions read(), readLine(), readAll(), write(). QFile also
    inherits getChar(), putChar(), and ungetChar(), which work one
    character at a time.

    The size of the file is returned by size(). You can get the
    current file position using pos(), or move to a new file position
    using seek(). If you've reached the end of the file, atEnd()
    returns \c true.

    \section1 Reading Files Directly

    The following example reads a text file line by line:

    \snippet file/file.cpp 0

    The \l{QIODeviceBase::}{Text} flag passed to open() tells Qt to convert
    Windows-style line terminators ("\\r\\n") into C++-style
    terminators ("\\n"). By default, QFile assumes binary, i.e. it
    doesn't perform any conversion on the bytes stored in the file.

    \section1 Using Streams to Read Files

    The next example uses QTextStream to read a text file
    line by line:

    \snippet file/file.cpp 1

    QTextStream takes care of converting the 8-bit data stored on
    disk into a 16-bit Unicode QString. By default, it assumes that
    the file is encoded in UTF-8. This can be changed using
    \l QTextStream::setEncoding().

    To write text, we can use operator<<(), which is overloaded to
    take a QTextStream on the left and various data types (including
    QString) on the right:

    \snippet file/file.cpp 2

    QDataStream is similar, in that you can use operator<<() to write
    data and operator>>() to read it back. See the class
    documentation for details.

    \section1 Signals

    Unlike other QIODevice implementations, such as QTcpSocket, QFile does not
    emit the aboutToClose(), bytesWritten(), or readyRead() signals. This
    implementation detail means that QFile is not suitable for reading and
    writing certain types of files, such as device files on Unix platforms.

    \section1 Platform Specific Issues

    \l{Input/Output and Networking}{Qt APIs related to I/O} use UTF-16 based
    QStrings to represent file paths. Standard C++ APIs (\c <cstdio> or
    \c <iostream>) or platform-specific APIs however often need a 8-bit encoded
    path. You can use encodeName() and decodeName() to convert between both
    representations.

    On Unix, there are some special system files (e.g. in \c /proc) for which
    size() will always return 0, yet you may still be able to read more data
    from such a file; the data is generated in direct response to you calling
    read(). In this case, however, you cannot use atEnd() to determine if
    there is more data to read (since atEnd() will return true for a file that
    claims to have size 0). Instead, you should either call readAll(), or call
    read() or readLine() repeatedly until no more data can be read. The next
    example uses QTextStream to read \c /proc/modules line by line:

    \snippet file/file.cpp 3

    File permissions are handled differently on Unix-like systems and
    Windows.  In a non \l{QIODevice::isWritable()}{writable}
    directory on Unix-like systems, files cannot be created. This is not always
    the case on Windows, where, for instance, the 'My Documents'
    directory usually is not writable, but it is still possible to
    create files in it.

    Qt's understanding of file permissions is limited, which affects especially
    the \l QFile::setPermissions() function. On Windows, Qt will set only the
    legacy read-only flag, and that only when none of the Write* flags are
    passed. Qt does not manipulate access control lists (ACLs), which makes this
    function mostly useless for NTFS volumes. It may still be of use for USB
    sticks that use VFAT file systems. POSIX ACLs are not manipulated, either.

    \include android-content-uri-limitations.qdocinc

    \sa QTextStream, QDataStream, QFileInfo, QDir, {The Qt Resource System}
*/

#ifdef QT_NO_QOBJECT
QFile::QFile()
    : QFileDevice(*new QFilePrivate)
{
}
QFile::QFile(const QString &name)
    : QFileDevice(*new QFilePrivate)
{
    d_func()->fileName = name;
}
QFile::QFile(QFilePrivate &dd)
    : QFileDevice(dd)
{
}
#else
/*!
    Constructs a QFile object.
*/
QFile::QFile()
    : QFileDevice(*new QFilePrivate, nullptr)
{
}
/*!
    Constructs a new file object with the given \a parent.
*/
QFile::QFile(QObject *parent)
    : QFileDevice(*new QFilePrivate, parent)
{
}
/*!
    Constructs a new file object to represent the file with the given \a name.

//! [qfile-explicit-constructor-note]
    \note In versions up to and including Qt 6.8, this constructor is
    implicit, for backward compatibility. Starting from Qt 6.9 this
    constructor is unconditionally \c{explicit}. Users can force this
    constructor to be \c{explicit} even in earlier versions of Qt by
    defining the \c{QT_EXPLICIT_QFILE_CONSTRUCTION_FROM_PATH} macro
    before including any Qt header.
//! [qfile-explicit-constructor-note]
*/
QFile::QFile(const QString &name)
    : QFileDevice(*new QFilePrivate, nullptr)
{
    Q_D(QFile);
    d->fileName = name;
}
/*!
    Constructs a new file object with the given \a parent to represent the
    file with the specified \a name.
*/
QFile::QFile(const QString &name, QObject *parent)
    : QFileDevice(*new QFilePrivate, parent)
{
    Q_D(QFile);
    d->fileName = name;
}
/*!
    \internal
*/
QFile::QFile(QFilePrivate &dd, QObject *parent)
    : QFileDevice(dd, parent)
{
}
#endif

/*!
    Destroys the file object, closing it if necessary.
*/
QFile::~QFile()
{
}

/*!
    Returns the name of the file as set by setFileName(), rename(), or
    by the QFile constructors.

    \sa setFileName(), rename(), QFileInfo::fileName()
*/
QString QFile::fileName() const
{
    Q_D(const QFile);
    return d->engine()->fileName(QAbstractFileEngine::DefaultName);
}

/*!
    Sets the \a name of the file. The name can have no path, a
    relative path, or an absolute path.

    Do not call this function if the file has already been opened.

    If the file name has no path or a relative path, the path used
    will be the application's current directory path
    \e{at the time of the open()} call.

    Example:
    \snippet code/src_corelib_io_qfile.cpp 0

    Note that the directory separator "/" works for all operating
    systems supported by Qt.

    \sa fileName(), QFileInfo, QDir
*/
void
QFile::setFileName(const QString &name)
{
    Q_D(QFile);
    if (isOpen()) {
        file_already_open(*this, "setFileName");
        close();
    }
    d->fileEngine.reset(); //get a new file engine later
    d->fileName = name;
}

/*!
    \fn QString QFile::decodeName(const char *localFileName)

    \overload

    Returns the Unicode version of the given \a localFileName. See
    encodeName() for details.
*/

/*!
    \fn QByteArray QFile::encodeName(const QString &fileName)

    Converts \a fileName to an 8-bit encoding that you can use in native
    APIs. On Windows, the encoding is the one from active Windows (ANSI)
    codepage. On other platforms, this is UTF-8, for \macos in decomposed
    form (NFD).

    \sa decodeName()
*/

/*!
    \fn QString QFile::decodeName(const QByteArray &localFileName)

    This does the reverse of QFile::encodeName() using \a localFileName.

    \sa encodeName()
*/

/*!
    \overload

    Returns \c true if the file specified by fileName() exists; otherwise
    returns \c false.

    \sa fileName(), setFileName()
*/

bool
QFile::exists() const
{
    Q_D(const QFile);
    // 0x1000000 = QAbstractFileEngine::Refresh, forcing an update
    return d->engine()->fileFlags(QAbstractFileEngine::FlagsMask
                                    | QAbstractFileEngine::Refresh).testAnyFlag(QAbstractFileEngine::ExistsFlag);
}

/*!
    Returns \c true if the file specified by \a fileName exists; otherwise
    returns \c false.

    \note If \a fileName is a symlink that points to a non-existing
    file, false is returned.
*/

bool
QFile::exists(const QString &fileName)
{
    return QFileInfo::exists(fileName);
}

/*!
    \fn QString QFile::symLinkTarget() const
    \since 4.2
    \overload

    Returns the absolute path of the file or directory a symlink (or shortcut
    on Windows) points to, or a an empty string if the object isn't a symbolic
    link.

    This name may not represent an existing file; it is only a string.
    QFile::exists() returns \c true if the symlink points to an existing file.

    \sa fileName(), setFileName()
*/
QString QFile::symLinkTarget() const
{
    Q_D(const QFile);
    return d->engine()->fileName(QAbstractFileEngine::AbsoluteLinkTarget);
}

/*!
    \fn static QString QFile::symLinkTarget(const QString &fileName)
    \since 4.2

    Returns the absolute path of the file or directory referred to by the
    symlink (or shortcut on Windows) specified by \a fileName, or returns an
    empty string if the \a fileName does not correspond to a symbolic link.

    This name may not represent an existing file; it is only a string.
    QFile::exists() returns \c true if the symlink points to an existing file.
*/
QString QFile::symLinkTarget(const QString &fileName)
{
    return QFileInfo(fileName).symLinkTarget();
}

/*!
    Removes the file specified by fileName(). Returns \c true if successful;
    otherwise returns \c false.

    The file is closed before it is removed.

    \sa setFileName()
*/

bool
QFile::remove()
{
    Q_D(QFile);
    if (d->fileName.isEmpty() &&
            !static_cast<QFSFileEngine *>(d->engine())->isUnnamedFile()) {
        qWarning("QFile::remove: Empty or null file name");
        return false;
    }
    unsetError();
    close();
    if (error() == QFile::NoError) {
        if (d->engine()->remove()) {
            unsetError();
            return true;
        }
        d->setError(QFile::RemoveError, d->fileEngine->errorString());
    }
    return false;
}

/*!
    \overload

    Removes the file specified by the \a fileName given.

    Returns \c true if successful; otherwise returns \c false.

    \sa remove()
*/

bool
QFile::remove(const QString &fileName)
{
    return QFile(fileName).remove();
}

/*!
    \since 6.9

    Returns \c true if Qt supports moving files to a trash (recycle bin) in the
    current operating system using the moveToTrash() function, \c false
    otherwise. Note that this function returning \c true does not imply
    moveToTrash() will succeed. In particular, this function does not check if
    the user has disabled the functionality in their settings.

    \sa moveToTrash()
*/
bool QFile::supportsMoveToTrash()
{
    return QFileSystemEngine::supportsMoveFileToTrash();
}

/*!
    \since 5.15

    Moves the file specified by fileName() to the trash. Returns \c true if successful,
    and sets the fileName() to the path at which the file can be found within the trash;
    otherwise returns \c false.

//! [move-to-trash-common]
    The time for this function to run is independent of the size of the file
    being trashed. If this function is called on a directory, it may be
    proportional to the number of files being trashed. If the current
    fileName() points to a symbolic link, this function will move the link to
    the trash, possibly breaking it, not the target of the link.

    This function uses the Windows and \macos APIs to perform the trashing on
    those two operating systems. Elsewhere (Unix systems), this function
    implements the \l{FreeDesktop.org Trash specification version 1.0}.

    \note When using the FreeDesktop.org Trash implementation, this function
    will fail if it is unable to move the files to the trash location by way of
    file renames and hardlinks. This condition arises if the file being trashed
    resides on a volume (mount point) on which the current user does not have
    permission to create the \c{.Trash} directory, or with some unusual
    filesystem types or configurations (such as sub-volumes that aren't
    themselves mount points).
//! [move-to-trash-common]

    \note On systems where the system API doesn't report the location of the
    file in the trash, fileName() will be set to the null string once the file
    has been moved. On systems that don't have a trash can, this function
    always returns \c false (see supportsMoveToTrash()).

    \sa supportsMoveToTrash(), remove(), QDir::remove()
*/
bool
QFile::moveToTrash()
{
    Q_D(QFile);
    if (d->fileName.isEmpty() &&
        !static_cast<QFSFileEngine *>(d->engine())->isUnnamedFile()) {
        qWarning("QFile::remove: Empty or null file name");
        return false;
    }
    unsetError();
    close();
    if (error() == QFile::NoError) {
        QFileSystemEntry fileEntry(d->fileName);
        QFileSystemEntry trashEntry;
        QSystemError error;
        if (QFileSystemEngine::moveFileToTrash(fileEntry, trashEntry, error)) {
            setFileName(trashEntry.filePath());
            unsetError();
            return true;
        }
        d->setError(QFile::RenameError, error.toString());
    }
    return false;
}

/*!
    \since 5.15
    \overload

    Moves the file specified by \a fileName to the trash. Returns \c true if successful,
    and sets \a pathInTrash (if provided) to the path at which the file can be found within
    the trash; otherwise returns \c false.

    \include qfile.cpp move-to-trash-common

    \note On systems where the system API doesn't report the path of the file in the
    trash, \a pathInTrash will be set to the null string once the file has been moved.
    On systems that don't have a trash can, this function always returns false.

*/
bool
QFile::moveToTrash(const QString &fileName, QString *pathInTrash)
{
    QFile file(fileName);
    if (file.moveToTrash()) {
        if (pathInTrash)
            *pathInTrash = file.fileName();
        return true;
    }
    return false;
}

/*!
    Renames the file currently specified by fileName() to \a newName.
    Returns \c true if successful; otherwise returns \c false.

    If a file with the name \a newName already exists, rename() returns \c false
    (i.e., QFile will not overwrite it).

    The file is closed before it is renamed.

    If the rename operation fails, Qt will attempt to copy this file's
    contents to \a newName, and then remove this file, keeping only
    \a newName. If that copy operation fails or this file can't be removed,
    the destination file \a newName is removed to restore the old state.

    \sa setFileName()
*/

bool
QFile::rename(const QString &newName)
{
    Q_D(QFile);

    // if this is a QTemporaryFile, the virtual fileName() call here may do something
    if (fileName().isEmpty()) {
        qWarning("QFile::rename: Empty or null file name");
        return false;
    }
    if (d->fileName == newName) {
        d->setError(QFile::RenameError, tr("Destination file is the same file."));
        return false;
    }
    if (!exists()) {
        d->setError(QFile::RenameError, tr("Source file does not exist."));
        return false;
    }

    // If the file exists and it is a case-changing rename ("foo" -> "Foo"),
    // compare Ids to make sure it really is a different file.
    // Note: this does not take file engines into account.
    bool changingCase = false;
    QByteArray targetId = QFileSystemEngine::id(QFileSystemEntry(newName));
    if (!targetId.isNull()) {
        QByteArray fileId = d->fileEngine ?
                    d->fileEngine->id() :
                    QFileSystemEngine::id(QFileSystemEntry(d->fileName));
        changingCase = (fileId == targetId && d->fileName.compare(newName, Qt::CaseInsensitive) == 0);
        if (!changingCase) {
            d->setError(QFile::RenameError, tr("Destination file exists"));
            return false;
        }

#if defined(Q_OS_LINUX) && QT_CONFIG(temporaryfile)
        // rename() on Linux simply does nothing when renaming "foo" to "Foo" on a case-insensitive
        // FS, such as FAT32. Move the file away and rename in 2 steps to work around.
        QTemporaryFileName tfn(d->fileName);
        QFileSystemEntry src(d->fileName);
        QSystemError error;
        for (int attempt = 0; attempt < 16; ++attempt) {
            QFileSystemEntry tmp(tfn.generateNext(), QFileSystemEntry::FromNativePath());

            // rename to temporary name
            if (!QFileSystemEngine::renameFile(src, tmp, error))
                continue;

            // rename to final name
            if (QFileSystemEngine::renameFile(tmp, QFileSystemEntry(newName), error)) {
                d->fileEngine->setFileName(newName);
                d->fileName = newName;
                return true;
            }

            // We need to restore the original file.
            QSystemError error2;
            if (QFileSystemEngine::renameFile(tmp, src, error2))
                break;      // report the original error, below

            // report both errors
            d->setError(QFile::RenameError,
                        tr("Error while renaming: %1").arg(error.toString())
                        + u'\n'
                        + tr("Unable to restore from %1: %2").
                        arg(QDir::toNativeSeparators(tmp.filePath()), error2.toString()));
            return false;
        }
        d->setError(QFile::RenameError,
                    tr("Error while renaming: %1").arg(error.toString()));
        return false;
#endif // Q_OS_LINUX
    }
    unsetError();
    close();
    if (error() == QFile::NoError) {
        if (changingCase ? d->engine()->renameOverwrite(newName) : d->engine()->rename(newName)) {
            unsetError();
            // engine was able to handle the new name so we just reset it
            d->fileEngine->setFileName(newName);
            d->fileName = newName;
            return true;
        }

        // Engine was unable to rename and the fallback will delete the original file,
        // so we have to back out here on case-insensitive file systems:
        if (changingCase) {
            d->setError(QFile::RenameError, d->fileEngine->errorString());
            return false;
        }

        if (isSequential()) {
            d->setError(QFile::RenameError, tr("Will not rename sequential file using block copy"));
            return false;
        }

#if QT_CONFIG(temporaryfile)
        // copy the file to the destination first
        if (d->copy(newName)) {
            // succeeded, remove the original
            if (!remove()) {
                d->setError(QFile::RenameError, tr("Cannot remove source file: %1").arg(errorString()));
                QFile out(newName);
                // set it back to writable so we can delete it
                out.setPermissions(ReadUser | WriteUser);
                out.remove(newName);
                return false;
            }
            d->fileEngine->setFileName(newName);
            unsetError();
            setFileName(newName);
            return true;
        } else {
            // change the error type but keep the string
            d->setError(QFile::RenameError, errorString());
        }
#else
        // copy the error from the engine rename() above
        d->setError(QFile::RenameError, d->fileEngine->errorString());
#endif
    }
    return false;
}

/*!
    \overload

    Renames the file \a oldName to \a newName. Returns \c true if
    successful; otherwise returns \c false.

    If a file with the name \a newName already exists, rename() returns \c false
    (i.e., QFile will not overwrite it).

    \sa rename()
*/

bool
QFile::rename(const QString &oldName, const QString &newName)
{
    return QFile(oldName).rename(newName);
}

/*!

    Creates a link named \a linkName that points to the file currently specified by
    fileName().  What a link is depends on the underlying filesystem (be it a
    shortcut on Windows or a symbolic link on Unix). Returns \c true if successful;
    otherwise returns \c false.

    This function will not overwrite an already existing entity in the file system;
    in this case, \c link() will return false and set \l{QFile::}{error()} to
    return \l{QFile::}{RenameError}.

    \note To create a valid link on Windows, \a linkName must have a \c{.lnk} file extension.

    \sa setFileName()
*/

bool
QFile::link(const QString &linkName)
{
    Q_D(QFile);
    if (fileName().isEmpty()) {
        qWarning("QFile::link: Empty or null file name");
        return false;
    }
    QFileInfo fi(linkName);
    if (d->engine()->link(fi.absoluteFilePath())) {
        unsetError();
        return true;
    }
    d->setError(QFile::RenameError, d->fileEngine->errorString());
    return false;
}

/*!
    \overload

    Creates a link named \a linkName that points to the file \a fileName. What a link is
    depends on the underlying filesystem (be it a shortcut on Windows
    or a symbolic link on Unix). Returns \c true if successful; otherwise
    returns \c false.

    \sa link()
*/

bool
QFile::link(const QString &fileName, const QString &linkName)
{
    return QFile(fileName).link(linkName);
}

#if QT_CONFIG(temporaryfile)    // dangerous without QTemporaryFile
bool QFilePrivate::copy(const QString &newName)
{
    Q_Q(QFile);
    Q_ASSERT(error == QFile::NoError);
    Q_ASSERT(!q->isOpen());

    // Some file engines can perform this copy more efficiently (e.g., Windows
    // calling CopyFile).
    if (engine()->copy(newName))
        return true;

    if (!q->open(QFile::ReadOnly | QFile::Unbuffered)) {
        setError(QFile::CopyError, QFile::tr("Cannot open %1 for input").arg(fileName));
        return false;
    }

    QSaveFile out(newName);
    out.setDirectWriteFallback(true);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        q->close();
        setError(QFile::CopyError, QFile::tr("Cannot open for output: %1").arg(out.errorString()));
        return false;
    }

    // Attempt to do an OS-level data copy
    QAbstractFileEngine::TriStateResult r = engine()->cloneTo(out.d_func()->engine());
    if (r == QAbstractFileEngine::TriStateResult::Failed) {
        q->close();
        setError(QFile::CopyError, QFile::tr("Could not copy to %1: %2")
                 .arg(newName, engine()->errorString()));
        return false;
    }

    while (r == QAbstractFileEngine::TriStateResult::NotSupported) {
        // OS couldn't do it, so do a block-level copy
        char block[4096];
        qint64 in = q->read(block, sizeof(block));
        if (in == 0)
            break;      // eof
        if (in < 0) {
            // Unable to read from the source. Save the error from read() above.
            QString s = std::move(errorString);
            q->close();
            setError(QFile::CopyError, std::move(s));
            return false;
        }
        if (in != out.write(block, in)) {
            q->close();
            setError(QFile::CopyError, QFile::tr("Failure to write block: %1")
                     .arg(out.errorString()));
            return false;
        }
    }

    // copy the permissions
    out.setPermissions(q->permissions());
    q->close();

    // final step: commit the copy
    if (out.commit())
        return true;
    setError(out.error(), out.errorString());
    return false;
}

/*!
    Copies the file named fileName() to \a newName.

    \include qfile-copy.qdocinc

    \note On Android, this operation is not yet supported for \c content
    scheme URIs.

    \sa setFileName()
*/

bool
QFile::copy(const QString &newName)
{
    Q_D(QFile);
    if (fileName().isEmpty()) {
        qWarning("QFile::copy: Empty or null file name");
        return false;
    }
    if (QFile::exists(newName)) {
        // ### Race condition. If a file is moved in after this, it /will/ be
        // overwritten. On Unix, the proper solution is to use hardlinks:
        // return ::link(old, new) && ::remove(old); See also rename().
        d->setError(QFile::CopyError, tr("Destination file exists"));
        return false;
    }
    unsetError();
    close();
    if (error() == QFile::NoError)
        return d->copy(newName);
    return false;
}

/*!
    \overload

    Copies the file named \a fileName to \a newName.

    \include qfile-copy.qdocinc

    \note On Android, this operation is not yet supported for \c content
    scheme URIs.

    \sa rename()
*/

bool
QFile::copy(const QString &fileName, const QString &newName)
{
    return QFile(fileName).copy(newName);
}
#endif // QT_CONFIG(temporaryfile)

/*!
    Opens the file using \a mode flags, returning \c true if successful;
    otherwise returns \c false.

    The flags for \a mode must include \l QIODeviceBase::ReadOnly,
    \l WriteOnly, or \l ReadWrite. It may also have additional flags,
    such as \l Text and \l Unbuffered.

    \note In \l{WriteOnly} or \l{ReadWrite}
    mode, if the relevant file does not already exist, this function
    will try to create a new file before opening it. The file will be
    created with mode 0666 masked by the umask on POSIX systems, and
    with permissions inherited from the parent directory on Windows.
    On Android, it's expected to have access permission to the parent
    of the file name, otherwise, it won't be possible to create this
    non-existing file.

    \sa QT_USE_NODISCARD_FILE_OPEN, setFileName()
*/
bool QFile::open(OpenMode mode)
{
    Q_D(QFile);
    if (isOpen())
        return file_already_open(*this);
    // Either Append or NewOnly implies WriteOnly
    if (mode & (Append | NewOnly))
        mode |= WriteOnly;
    unsetError();
    if ((mode & (ReadOnly | WriteOnly)) == 0) {
        qWarning("QIODevice::open: File access not specified");
        return false;
    }

    // QIODevice provides the buffering, so there's no need to request it from the file engine.
    if (d->engine()->open(mode | QIODevice::Unbuffered)) {
        QIODevice::open(mode);
        if (mode & Append)
            seek(size());
        return true;
    }
    QFile::FileError err = d->fileEngine->error();
    if (err == QFile::UnspecifiedError)
        err = QFile::OpenError;
    d->setError(err, d->fileEngine->errorString());
    return false;
}

/*!
    \overload

    If the file does not exist and \a mode implies creating it, it is created
    with the specified \a permissions.

    On POSIX systems the actual permissions are influenced by the
    value of \c umask.

    On Windows the permissions are emulated using ACLs. These ACLs may be in non-canonical
    order when the group is granted less permissions than others. Files and directories with
    such permissions will generate warnings when the Security tab of the Properties dialog
    is opened. Granting the group all permissions granted to others avoids such warnings.

    \sa QIODevice::OpenMode, setFileName(), QT_USE_NODISCARD_FILE_OPEN
    \since 6.3
*/
bool QFile::open(OpenMode mode, QFile::Permissions permissions)
{
    Q_D(QFile);
    if (isOpen())
        return file_already_open(*this);
    // Either Append or NewOnly implies WriteOnly
    if (mode & (Append | NewOnly))
        mode |= WriteOnly;
    unsetError();
    if ((mode & (ReadOnly | WriteOnly)) == 0) {
        qWarning("QIODevice::open: File access not specified");
        return false;
    }

    // QIODevice provides the buffering, so there's no need to request it from the file engine.
    if (d->engine()->open(mode | QIODevice::Unbuffered, permissions)) {
        QIODevice::open(mode);
        if (mode & Append)
            seek(size());
        return true;
    }
    QFile::FileError err = d->fileEngine->error();
    if (err == QFile::UnspecifiedError)
        err = QFile::OpenError;
    d->setError(err, d->fileEngine->errorString());
    return false;
}

/*!
    \overload

    Opens the existing file handle \a fh in the given \a mode.
    \a handleFlags may be used to specify additional options.
    Returns \c true if successful; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_io_qfile.cpp 3

    When a QFile is opened using this function, behaviour of close() is
    controlled by the AutoCloseHandle flag.
    If AutoCloseHandle is specified, and this function succeeds,
    then calling close() closes the adopted handle.
    Otherwise, close() does not actually close the file, but only flushes it.

    \b{Warning:}
    \list 1
        \li If \a fh does not refer to a regular file, e.g., it is \c stdin,
           \c stdout, or \c stderr, you may not be able to seek(). size()
           returns \c 0 in those cases. See QIODevice::isSequential() for
           more information.
        \li Since this function opens the file without specifying the file name,
           you cannot use this QFile with a QFileInfo.
    \endlist

    \sa close(), QT_USE_NODISCARD_FILE_OPEN

    \b{Note for the Windows Platform}

    \a fh must be opened in binary mode (i.e., the mode string must contain
    'b', as in "rb" or "wb") when accessing files and other random-access
    devices. Qt will translate the end-of-line characters if you pass
    QIODevice::Text to \a mode. Sequential devices, such as stdin and stdout,
    are unaffected by this limitation.

    You need to enable support for console applications in order to use the
    stdin, stdout and stderr streams at the console. To do this, add the
    following declaration to your application's project file:

    \snippet code/src_corelib_io_qfile.cpp 4
*/
bool QFile::open(FILE *fh, OpenMode mode, FileHandleFlags handleFlags)
{
    Q_D(QFile);
    if (isOpen())
        return file_already_open(*this);
    // Either Append or NewOnly implies WriteOnly
    if (mode & (Append | NewOnly))
        mode |= WriteOnly;
    unsetError();
    if ((mode & (ReadOnly | WriteOnly)) == 0) {
        qWarning("QFile::open: File access not specified");
        return false;
    }

    // QIODevice provides the buffering, so request unbuffered file engines
    if (d->openExternalFile(mode | Unbuffered, fh, handleFlags)) {
        QIODevice::open(mode);
        if (!(mode & Append) && !isSequential()) {
            qint64 pos = (qint64)QT_FTELL(fh);
            if (pos != -1) {
                // Skip redundant checks in QFileDevice::seek().
                QIODevice::seek(pos);
            }
        }
        return true;
    }
    return false;
}

/*!
    \overload

    Opens the existing file descriptor \a fd in the given \a mode.
    \a handleFlags may be used to specify additional options.
    Returns \c true if successful; otherwise returns \c false.

    When a QFile is opened using this function, behaviour of close() is
    controlled by the AutoCloseHandle flag.
    If AutoCloseHandle is specified, and this function succeeds,
    then calling close() closes the adopted handle.
    Otherwise, close() does not actually close the file, but only flushes it.

    \warning If \a fd is not a regular file, e.g, it is 0 (\c stdin),
    1 (\c stdout), or 2 (\c stderr), you may not be able to seek(). In
    those cases, size() returns \c 0.  See QIODevice::isSequential()
    for more information.

    \warning Since this function opens the file without specifying the file name,
             you cannot use this QFile with a QFileInfo.

    \sa close(), QT_USE_NODISCARD_FILE_OPEN
*/
bool QFile::open(int fd, OpenMode mode, FileHandleFlags handleFlags)
{
    Q_D(QFile);
    if (isOpen())
        return file_already_open(*this);
    // Either Append or NewOnly implies WriteOnly
    if (mode & (Append | NewOnly))
        mode |= WriteOnly;
    unsetError();
    if ((mode & (ReadOnly | WriteOnly)) == 0) {
        qWarning("QFile::open: File access not specified");
        return false;
    }

    // QIODevice provides the buffering, so request unbuffered file engines
    if (d->openExternalFile(mode | Unbuffered, fd, handleFlags)) {
        QIODevice::open(mode);
        if (!(mode & Append) && !isSequential()) {
            qint64 pos = (qint64)QT_LSEEK(fd, QT_OFF_T(0), SEEK_CUR);
            if (pos != -1) {
                // Skip redundant checks in QFileDevice::seek().
                QIODevice::seek(pos);
            }
        }
        return true;
    }
    return false;
}

/*!
    \reimp
*/
bool QFile::resize(qint64 sz)
{
    return QFileDevice::resize(sz); // for now
}

/*!
    \overload

    Sets \a fileName to size (in bytes) \a sz. Returns \c true if
    the resize succeeds; false otherwise. If \a sz is larger than \a
    fileName currently is the new bytes will be set to 0, if \a sz is
    smaller the file is simply truncated.

    \warning This function can fail if the file doesn't exist.

    \sa resize()
*/

bool
QFile::resize(const QString &fileName, qint64 sz)
{
    return QFile(fileName).resize(sz);
}

/*!
    \reimp
*/
QFile::Permissions QFile::permissions() const
{
    return QFileDevice::permissions(); // for now
}

/*!
    \overload

    Returns the complete OR-ed together combination of
    QFile::Permission for \a fileName.
*/

QFile::Permissions
QFile::permissions(const QString &fileName)
{
    return QFile(fileName).permissions();
}

/*!
    Sets the permissions for the file to the \a permissions specified.
    Returns \c true if successful, or \c false if the permissions cannot be
    modified.

    \warning This function does not manipulate ACLs, which may limit its
    effectiveness.

    \sa permissions(), setFileName()
*/

bool QFile::setPermissions(Permissions permissions)
{
    return QFileDevice::setPermissions(permissions); // for now
}

/*!
    \overload

    Sets the permissions for \a fileName file to \a permissions.
*/

bool
QFile::setPermissions(const QString &fileName, Permissions permissions)
{
    return QFile(fileName).setPermissions(permissions);
}

/*!
  \reimp
*/
qint64 QFile::size() const
{
    return QFileDevice::size(); // for now
}

/*!
    \fn QFile::QFile(const std::filesystem::path &name)
    \since 6.0

    Constructs a new file object to represent the file with the given \a name.

    \include qfile.cpp qfile-explicit-constructor-note
*/
/*!
    \fn QFile::QFile(const std::filesystem::path &name, QObject *parent)
    \since 6.0

    Constructs a new file object with the given \a parent to represent the
    file with the specified \a name.
*/
/*!
    \fn std::filesystem::path QFile::filesystemFileName() const
    \since 6.0
    Returns fileName() as \c{std::filesystem::path}.
*/
/*!
    \fn void QFile::setFileName(const std::filesystem::path &name)
    \since 6.0
    \overload
*/
/*!
    \fn bool QFile::rename(const std::filesystem::path &newName)
    \since 6.0
    \overload
*/
/*!
    \fn bool QFile::link(const std::filesystem::path &newName)
    \since 6.0
    \overload
*/
/*!
    \fn bool QFile::copy(const std::filesystem::path &newName)
    \since 6.0
    \overload
*/
/*!
    \fn QFile::Permissions QFile::permissions(const std::filesystem::path &filename)
    \since 6.0
    \overload
*/
/*!
    \fn bool QFile::setPermissions(const std::filesystem::path &filename, Permissions permissionSpec)
    \since 6.0
    \overload
*/
/*!
    \fn bool exists(const std::filesystem::path &fileName)
    \since 6.3
    \overload
*/
/*!
    \fn std::filesystem::path QFile::filesystemSymLinkTarget() const
    \since 6.3
    Returns symLinkTarget() as \c{std::filesystem::path}.
*/
/*!
    \fn std::filesystem::path QFile::filesystemSymLinkTarget(const std::filesystem::path &fileName)
    \since 6.3
    Returns symLinkTarget() as \c{std::filesystem::path} of \a fileName.
*/
/*!
    \fn bool remove(const std::filesystem::path &fileName)
    \since 6.3
    \overload
*/
/*!
    \fn bool moveToTrash(const std::filesystem::path &fileName, QString *pathInTrash)
    \since 6.3
    \overload
*/
/*!
    \fn bool rename(const std::filesystem::path &oldName, const std::filesystem::path &newName)
    \since 6.3
    \overload
*/
/*!
    \fn bool link(const std::filesystem::path &fileName, const std::filesystem::path &newName);
    \since 6.3
    \overload
*/
/*!
    \fn bool copy(const std::filesystem::path &fileName, const std::filesystem::path &newName);
    \since 6.3
    \overload
*/


/*!
    \class QNtfsPermissionCheckGuard
    \since 6.6
    \inmodule QtCore
    \brief The QNtfsPermissionCheckGuard class is a RAII class to manage NTFS
    permission checking.

    \ingroup io

    For performance reasons, QFile, QFileInfo, and related classes do not
    perform full ownership and permission (ACL) checking on NTFS file systems
    by default. During the lifetime of any instance of this class, that
    default is overridden and advanced checking is performed. This provides
    a safe and easy way to manage enabling and disabling this change to the
    default behavior.

    Example:

    \snippet ntfsp.cpp raii

    This class is available only on Windows.

    \section1 qt_ntfs_permission_lookup

    Prior to Qt 6.6, the user had to directly manipulate the global variable
    \c qt_ntfs_permission_lookup. However, this was a non-atomic global
    variable and as such it was prone to data races.

    The variable \c qt_ntfs_permission_lookup is therefore deprecated since Qt
    6.6.
*/

/*!
    \fn QNtfsPermissionCheckGuard::QNtfsPermissionCheckGuard()

    Creates a guard and calls the function qEnableNtfsPermissionChecks().
*/

/*!
    \fn QNtfsPermissionCheckGuard::~QNtfsPermissionCheckGuard()

    Destroys the guard and calls the function qDisableNtfsPermissionChecks().
*/


/*!
    \fn bool qEnableNtfsPermissionChecks()
    \since 6.6
    \threadsafe
    \relates QNtfsPermissionCheckGuard

    Enables permission checking on NTFS file systems. Returns \c true if the check
    was already enabled before the call to this function, meaning that there
    are other users.

    This function is only available on Windows and makes the direct
    manipulation of \l qt_ntfs_permission_lookup obsolete.

    This is a low-level function, please consider the RAII class
    \l QNtfsPermissionCheckGuard instead.

    \note The thread-safety of this function holds only as long as there are no
    concurrent updates to \l qt_ntfs_permission_lookup.
*/

/*!
    \fn bool qDisableNtfsPermissionChecks()
    \since 6.6
    \threadsafe
    \relates QNtfsPermissionCheckGuard

    Disables permission checking on NTFS file systems. Returns \c true if the
    check is disabled, meaning that there are no more users.

    This function is only available on Windows and makes the direct
    manipulation of \l qt_ntfs_permission_lookup obsolete.

    This is a low-level function and must (only) be called to match one earlier
    call to qEnableNtfsPermissionChecks(). Please consider the RAII class
    \l QNtfsPermissionCheckGuard instead.

    \note The thread-safety of this function holds only as long as there are no
    concurrent updates to \l qt_ntfs_permission_lookup.
*/

/*!
    \fn bool qAreNtfsPermissionChecksEnabled()
    \since 6.6
    \threadsafe
    \relates QNtfsPermissionCheckGuard

    Checks the status of the permission checks on NTFS file systems. Returns
    \c true if the check is enabled.

    This function is only available on Windows and makes the direct
    manipulation of \l qt_ntfs_permission_lookup obsolete.

    \note The thread-safety of this function holds only as long as there are no
    concurrent updates to \l qt_ntfs_permission_lookup.
*/

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qfile.cpp"
#endif
