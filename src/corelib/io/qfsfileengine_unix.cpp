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

#include "qplatformdefs.h"
#include "private/qabstractfileengine_p.h"
#include "private/qfsfileengine_p.h"
#include "private/qcore_unix_p.h"
#include "qfilesystementry_p.h"
#include "qfilesystemengine_p.h"
#include "qcoreapplication.h"

#ifndef QT_NO_FSFILEENGINE

#include "qfile.h"
#include "qdir.h"
#include "qdatetime.h"
#include "qvarlengtharray.h"

#include <sys/mman.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#if !defined(QWS) && defined(Q_OS_MAC)
# include <private/qcore_mac_p.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \internal

    Returns the stdio open flags corresponding to a QIODevice::OpenMode.
*/
static inline int openModeToOpenFlags(QIODevice::OpenMode mode)
{
    int oflags = QT_OPEN_RDONLY;
#ifdef QT_LARGEFILE_SUPPORT
    oflags |= QT_OPEN_LARGEFILE;
#endif

    if ((mode & QFile::ReadWrite) == QFile::ReadWrite)
        oflags = QT_OPEN_RDWR;
    else if (mode & QFile::WriteOnly)
        oflags = QT_OPEN_WRONLY;

    if (QFSFileEnginePrivate::openModeCanCreate(mode))
        oflags |= QT_OPEN_CREAT;

    if (mode & QFile::Truncate)
        oflags |= QT_OPEN_TRUNC;

    if (mode & QFile::Append)
        oflags |= QT_OPEN_APPEND;

    if (mode & QFile::NewOnly)
        oflags |= QT_OPEN_EXCL;

    return oflags;
}

static inline QString msgOpenDirectory()
{
    const char message[] = QT_TRANSLATE_NOOP("QIODevice", "file to open is a directory");
#if QT_CONFIG(translation)
    return QIODevice::tr(message);
#else
    return QLatin1String(message);
#endif
}

/*!
    \internal
*/
bool QFSFileEnginePrivate::nativeOpen(QIODevice::OpenMode openMode)
{
    Q_Q(QFSFileEngine);

    Q_ASSERT_X(openMode & QIODevice::Unbuffered, "QFSFileEngine::open",
               "QFSFileEngine no longer supports buffered mode; upper layer must buffer");
    if (openMode & QIODevice::Unbuffered) {
        int flags = openModeToOpenFlags(openMode);

        // Try to open the file in unbuffered mode.
        do {
            fd = QT_OPEN(fileEntry.nativeFilePath().constData(), flags, 0666);
        } while (fd == -1 && errno == EINTR);

        // On failure, return and report the error.
        if (fd == -1) {
            q->setError(errno == EMFILE ? QFile::ResourceError : QFile::OpenError,
                        qt_error_string(errno));
            return false;
        }

        if (!(openMode & QIODevice::WriteOnly)) {
            // we don't need this check if we tried to open for writing because then
            // we had received EISDIR anyway.
            if (QFileSystemEngine::fillMetaData(fd, metaData)
                    && metaData.isDirectory()) {
                q->setError(QFile::OpenError, msgOpenDirectory());
                QT_CLOSE(fd);
                return false;
            }
        }

        // Seek to the end when in Append mode.
        if (flags & QFile::Append) {
            int ret;
            do {
                ret = QT_LSEEK(fd, 0, SEEK_END);
            } while (ret == -1 && errno == EINTR);

            if (ret == -1) {
                q->setError(errno == EMFILE ? QFile::ResourceError : QFile::OpenError,
                            qt_error_string(int(errno)));
                return false;
            }
        }

        fh = nullptr;
    }

    closeFileHandle = true;
    return true;
}

/*!
    \internal
*/
bool QFSFileEnginePrivate::nativeClose()
{
    return closeFdFh();
}

/*!
    \internal

*/
bool QFSFileEnginePrivate::nativeFlush()
{
    return fh ? flushFh() : fd != -1;
}

/*!
    \internal
    \since 5.1
*/
bool QFSFileEnginePrivate::nativeSyncToDisk()
{
    Q_Q(QFSFileEngine);
    int ret;
#if defined(_POSIX_SYNCHRONIZED_IO) && _POSIX_SYNCHRONIZED_IO > 0
    EINTR_LOOP(ret, fdatasync(nativeHandle()));
#else
    EINTR_LOOP(ret, fsync(nativeHandle()));
#endif
    if (ret != 0)
        q->setError(QFile::WriteError, qt_error_string(errno));
    return ret == 0;
}

/*!
    \internal
*/
qint64 QFSFileEnginePrivate::nativeRead(char *data, qint64 len)
{
    Q_Q(QFSFileEngine);

    if (fh && nativeIsSequential()) {
        size_t readBytes = 0;
        int oldFlags = fcntl(QT_FILENO(fh), F_GETFL);
        for (int i = 0; i < 2; ++i) {
            // Unix: Make the underlying file descriptor non-blocking
            if ((oldFlags & O_NONBLOCK) == 0)
                fcntl(QT_FILENO(fh), F_SETFL, oldFlags | O_NONBLOCK);

            // Cross platform stdlib read
            size_t read = 0;
            do {
                read = fread(data + readBytes, 1, size_t(len - readBytes), fh);
            } while (read == 0 && !feof(fh) && errno == EINTR);
            if (read > 0) {
                readBytes += read;
                break;
            } else {
                if (readBytes)
                    break;
                readBytes = read;
            }

            // Unix: Restore the blocking state of the underlying socket
            if ((oldFlags & O_NONBLOCK) == 0) {
                fcntl(QT_FILENO(fh), F_SETFL, oldFlags);
                if (readBytes == 0) {
                    int readByte = 0;
                    do {
                        readByte = fgetc(fh);
                    } while (readByte == -1 && errno == EINTR);
                    if (readByte != -1) {
                        *data = uchar(readByte);
                        readBytes += 1;
                    } else {
                        break;
                    }
                }
            }
        }
        // Unix: Restore the blocking state of the underlying socket
        if ((oldFlags & O_NONBLOCK) == 0) {
            fcntl(QT_FILENO(fh), F_SETFL, oldFlags);
        }
        if (readBytes == 0 && !feof(fh)) {
            // if we didn't read anything and we're not at EOF, it must be an error
            q->setError(QFile::ReadError, qt_error_string(int(errno)));
            return -1;
        }
        return readBytes;
    }

    return readFdFh(data, len);
}

/*!
    \internal
*/
qint64 QFSFileEnginePrivate::nativeReadLine(char *data, qint64 maxlen)
{
    return readLineFdFh(data, maxlen);
}

/*!
    \internal
*/
qint64 QFSFileEnginePrivate::nativeWrite(const char *data, qint64 len)
{
    return writeFdFh(data, len);
}

/*!
    \internal
*/
qint64 QFSFileEnginePrivate::nativePos() const
{
    return posFdFh();
}

/*!
    \internal
*/
bool QFSFileEnginePrivate::nativeSeek(qint64 pos)
{
    return seekFdFh(pos);
}

/*!
    \internal
*/
int QFSFileEnginePrivate::nativeHandle() const
{
    return fh ? fileno(fh) : fd;
}

/*!
    \internal
*/
bool QFSFileEnginePrivate::nativeIsSequential() const
{
    return isSequentialFdFh();
}

bool QFSFileEngine::link(const QString &newName)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::createLink(d->fileEntry, QFileSystemEntry(newName), error);
    if (!ret) {
        setError(QFile::RenameError, error.toString());
    }
    return ret;
}

qint64 QFSFileEnginePrivate::nativeSize() const
{
    return sizeFdFh();
}

bool QFSFileEngine::caseSensitive() const
{
    return true;
}

QString QFSFileEngine::currentPath(const QString &)
{
    return QFileSystemEngine::currentPath().filePath();
}


QFileInfoList QFSFileEngine::drives()
{
    QFileInfoList ret;
    ret.append(QFileInfo(rootPath()));
    return ret;
}

bool QFSFileEnginePrivate::doStat(QFileSystemMetaData::MetaDataFlags flags) const
{
    if (!tried_stat || !metaData.hasFlags(flags)) {
        tried_stat = 1;

        int localFd = fd;
        if (fh && fileEntry.isEmpty())
            localFd = QT_FILENO(fh);
        if (localFd != -1)
            QFileSystemEngine::fillMetaData(localFd, metaData);

        if (metaData.missingFlags(flags) && !fileEntry.isEmpty())
            QFileSystemEngine::fillMetaData(fileEntry, metaData, metaData.missingFlags(flags));
    }

    return metaData.exists();
}

bool QFSFileEnginePrivate::isSymlink() const
{
    if (!metaData.hasFlags(QFileSystemMetaData::LinkType))
        QFileSystemEngine::fillMetaData(fileEntry, metaData, QFileSystemMetaData::LinkType);

    return metaData.isLink();
}

/*!
    \reimp
*/
QAbstractFileEngine::FileFlags QFSFileEngine::fileFlags(FileFlags type) const
{
    Q_D(const QFSFileEngine);

    if (type & Refresh)
        d->metaData.clear();

    QAbstractFileEngine::FileFlags ret = { };

    if (type & FlagsMask)
        ret |= LocalDiskFlag;

    bool exists;
    {
        QFileSystemMetaData::MetaDataFlags queryFlags = { };

        queryFlags |= QFileSystemMetaData::MetaDataFlags(uint(type))
                & QFileSystemMetaData::Permissions;

        if (type & TypesMask)
            queryFlags |= QFileSystemMetaData::AliasType
                    | QFileSystemMetaData::LinkType
                    | QFileSystemMetaData::FileType
                    | QFileSystemMetaData::DirectoryType
                    | QFileSystemMetaData::BundleType
                    | QFileSystemMetaData::WasDeletedAttribute;

        if (type & FlagsMask)
            queryFlags |= QFileSystemMetaData::HiddenAttribute
                    | QFileSystemMetaData::ExistsAttribute;
        else if (type & ExistsFlag)
            queryFlags |= QFileSystemMetaData::WasDeletedAttribute;

        queryFlags |= QFileSystemMetaData::LinkType;

        exists = d->doStat(queryFlags);
    }

    if (!exists && !d->metaData.isLink())
        return ret;

    if (exists && (type & PermsMask))
        ret |= FileFlags(uint(d->metaData.permissions()));

    if (type & TypesMask) {
        if (d->metaData.isAlias()) {
            ret |= LinkType;
        } else {
            if ((type & LinkType) && d->metaData.isLink())
                ret |= LinkType;
            if (exists) {
                if (d->metaData.isFile()) {
                    ret |= FileType;
                } else if (d->metaData.isDirectory()) {
                    ret |= DirectoryType;
                    if ((type & BundleType) && d->metaData.isBundle())
                        ret |= BundleType;
                }
            }
        }
    }

    if (type & FlagsMask) {
        // the inode existing does not mean the file exists
        if (!d->metaData.wasDeleted())
            ret |= ExistsFlag;
        if (d->fileEntry.isRoot())
            ret |= RootFlag;
        else if (d->metaData.isHidden())
            ret |= HiddenFlag;
    }

    return ret;
}

QByteArray QFSFileEngine::id() const
{
    Q_D(const QFSFileEngine);
    if (d->fd != -1)
        return QFileSystemEngine::id(d->fd);
    return QFileSystemEngine::id(d->fileEntry);
}

QString QFSFileEngine::fileName(FileName file) const
{
    Q_D(const QFSFileEngine);
    switch (file) {
    case BundleName:
        return QFileSystemEngine::bundleName(d->fileEntry);
    case BaseName:
        return d->fileEntry.fileName();
    case PathName:
        return d->fileEntry.path();
    case AbsoluteName:
    case AbsolutePathName: {
        QFileSystemEntry entry(QFileSystemEngine::absoluteName(d->fileEntry));
        return file == AbsolutePathName ? entry.path() : entry.filePath();
    }
    case CanonicalName:
    case CanonicalPathName: {
        QFileSystemEntry entry(QFileSystemEngine::canonicalName(d->fileEntry, d->metaData));
        return file == CanonicalPathName ? entry.path() : entry.filePath();
    }
    case LinkName:
        if (d->isSymlink()) {
            QFileSystemEntry entry = QFileSystemEngine::getLinkTarget(d->fileEntry, d->metaData);
            return entry.filePath();
        }
        return QString();
    case DefaultName:
    case NFileNames:
        break;
    }
    return d->fileEntry.filePath();
}

bool QFSFileEngine::isRelativePath() const
{
    Q_D(const QFSFileEngine);
    return d->fileEntry.filePath().length() ? d->fileEntry.filePath().at(0) != QLatin1Char('/') : true;
}

uint QFSFileEngine::ownerId(FileOwner own) const
{
    Q_D(const QFSFileEngine);
    static const uint nobodyID = (uint) -2;

    if (d->doStat(QFileSystemMetaData::OwnerIds))
        return d->metaData.ownerId(own);

    return nobodyID;
}

QString QFSFileEngine::owner(FileOwner own) const
{
    if (own == OwnerUser)
        return QFileSystemEngine::resolveUserName(ownerId(own));
    return QFileSystemEngine::resolveGroupName(ownerId(own));
}

bool QFSFileEngine::setPermissions(uint perms)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ok;
    if (d->fd != -1)
        ok = QFileSystemEngine::setPermissions(d->fd, QFile::Permissions(perms), error);
    else
        ok = QFileSystemEngine::setPermissions(d->fileEntry, QFile::Permissions(perms), error);
    if (!ok) {
        setError(QFile::PermissionsError, error.toString());
        return false;
    }
    return true;
}

bool QFSFileEngine::setSize(qint64 size)
{
    Q_D(QFSFileEngine);
    bool ret = false;
    if (d->fd != -1)
        ret = QT_FTRUNCATE(d->fd, size) == 0;
    else if (d->fh)
        ret = QT_FTRUNCATE(QT_FILENO(d->fh), size) == 0;
    else
        ret = QT_TRUNCATE(d->fileEntry.nativeFilePath().constData(), size) == 0;
    if (!ret)
        setError(QFile::ResizeError, qt_error_string(errno));
    return ret;
}

bool QFSFileEngine::setFileTime(const QDateTime &newDate, FileTime time)
{
    Q_D(QFSFileEngine);

    if (d->openMode == QIODevice::NotOpen) {
        setError(QFile::PermissionsError, qt_error_string(EACCES));
        return false;
    }

    QSystemError error;
    if (!QFileSystemEngine::setFileTime(d->nativeHandle(), newDate, time, error)) {
        setError(QFile::PermissionsError, error.toString());
        return false;
    }

    d->metaData.clearFlags(QFileSystemMetaData::Times);
    return true;
}

uchar *QFSFileEnginePrivate::map(qint64 offset, qint64 size, QFile::MemoryMapFlags flags)
{
    qint64 maxFileOffset = std::numeric_limits<QT_OFF_T>::max();
#if (defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)) && Q_PROCESSOR_WORDSIZE == 4
    // The Linux mmap2 system call on 32-bit takes a page-shifted 32-bit
    // integer so the maximum offset is 1 << (32+12) (the shift is always 12,
    // regardless of the actual page size). Unfortunately, the mmap64()
    // function is known to be broken in all Linux libcs (glibc, uclibc, musl
    // and Bionic): all of them do the right shift, but don't confirm that the
    // result fits into the 32-bit parameter to the kernel.

    maxFileOffset = qMin((Q_INT64_C(1) << (32+12)) - 1, maxFileOffset);
#endif

    Q_Q(QFSFileEngine);
    if (openMode == QIODevice::NotOpen) {
        q->setError(QFile::PermissionsError, qt_error_string(int(EACCES)));
        return nullptr;
    }

    if (offset < 0 || offset > maxFileOffset
            || size < 0 || quint64(size) > quint64(size_t(-1))) {
        q->setError(QFile::UnspecifiedError, qt_error_string(int(EINVAL)));
        return nullptr;
    }

    // If we know the mapping will extend beyond EOF, fail early to avoid
    // undefined behavior. Otherwise, let mmap have its say.
    if (doStat(QFileSystemMetaData::SizeAttribute)
            && (QT_OFF_T(size) > metaData.size() - QT_OFF_T(offset)))
        qWarning("QFSFileEngine::map: Mapping a file beyond its size is not portable");

    int access = 0;
    if (openMode & QIODevice::ReadOnly) access |= PROT_READ;
    if (openMode & QIODevice::WriteOnly) access |= PROT_WRITE;

    int sharemode = MAP_SHARED;
    if (flags & QFileDevice::MapPrivateOption) {
        sharemode = MAP_PRIVATE;
        access |= PROT_WRITE;
    }

#if defined(Q_OS_INTEGRITY)
    int pageSize = sysconf(_SC_PAGESIZE);
#else
    int pageSize = getpagesize();
#endif
    int extra = offset % pageSize;

    if (quint64(size + extra) > quint64((size_t)-1)) {
        q->setError(QFile::UnspecifiedError, qt_error_string(int(EINVAL)));
        return nullptr;
    }

    size_t realSize = (size_t)size + extra;
    QT_OFF_T realOffset = QT_OFF_T(offset);
    realOffset &= ~(QT_OFF_T(pageSize - 1));

    void *mapAddress = QT_MMAP((void*)nullptr, realSize,
                   access, sharemode, nativeHandle(), realOffset);
    if (MAP_FAILED != mapAddress) {
        uchar *address = extra + static_cast<uchar*>(mapAddress);
        maps[address] = QPair<int,size_t>(extra, realSize);
        return address;
    }

    switch(errno) {
    case EBADF:
        q->setError(QFile::PermissionsError, qt_error_string(int(EACCES)));
        break;
    case ENFILE:
    case ENOMEM:
        q->setError(QFile::ResourceError, qt_error_string(int(errno)));
        break;
    case EINVAL:
        // size are out of bounds
    default:
        q->setError(QFile::UnspecifiedError, qt_error_string(int(errno)));
        break;
    }
    return nullptr;
}

bool QFSFileEnginePrivate::unmap(uchar *ptr)
{
#if !defined(Q_OS_INTEGRITY)
    Q_Q(QFSFileEngine);
    if (!maps.contains(ptr)) {
        q->setError(QFile::PermissionsError, qt_error_string(EACCES));
        return false;
    }

    uchar *start = ptr - maps[ptr].first;
    size_t len = maps[ptr].second;
    if (-1 == munmap(start, len)) {
        q->setError(QFile::UnspecifiedError, qt_error_string(errno));
        return false;
    }
    maps.remove(ptr);
    return true;
#else
    return false;
#endif
}

/*!
    \reimp
*/
bool QFSFileEngine::cloneTo(QAbstractFileEngine *target)
{
    Q_D(QFSFileEngine);
    if ((target->fileFlags(LocalDiskFlag) & LocalDiskFlag) == 0)
        return false;

    int srcfd = d->nativeHandle();
    int dstfd = target->handle();
    return QFileSystemEngine::cloneFile(srcfd, dstfd, d->metaData);
}

QT_END_NAMESPACE

#endif // QT_NO_FSFILEENGINE
