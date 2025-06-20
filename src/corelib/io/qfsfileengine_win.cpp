// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qplatformdefs.h"
#include "private/qabstractfileengine_p.h"
#include "private/qfiledevice_p.h"
#include "private/qfsfileengine_p.h"
#include "qfilesystemengine_p.h"
#include <qdebug.h>

#include "qfile.h"
#include "qdir.h"
#include "qvarlengtharray.h"
#include "qdatetime.h"
#include "qt_windows.h"

#include <sys/types.h>
#include <direct.h>
#include <winioctl.h>
#include <objbase.h>
#include <shlobj.h>
#include <accctrl.h>
#include <initguid.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#define SECURITY_WIN32
#include <security.h>

#include <memory>

#ifndef PATH_MAX
#define PATH_MAX FILENAME_MAX
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static inline bool isUncPath(const QString &path)
{
    // Starts with \\, but not \\.
    return (path.startsWith("\\\\"_L1)
            && path.size() > 2 && path.at(2) != u'.');
}

/*!
    \internal
*/
QString QFSFileEnginePrivate::longFileName(const QString &path)
{
    if (path.startsWith("\\\\.\\"_L1))
        return path;

    QString absPath = QFileSystemEngine::nativeAbsoluteFilePath(path);
    QString prefix = "\\\\?\\"_L1;
    if (isUncPath(absPath)) {
        prefix.append("UNC\\"_L1); // "\\\\?\\UNC\\"
        absPath.remove(0, 2);
    }
    return prefix + absPath;
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeOpen(QIODevice::OpenMode openMode,
                                      std::optional<QFile::Permissions> permissions)
{
    Q_Q(QFSFileEngine);

    // All files are opened in share mode (both read and write).
    DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    int accessRights = 0;
    if (openMode & QIODevice::ReadOnly)
        accessRights |= GENERIC_READ;
    if (openMode & QIODevice::WriteOnly)
        accessRights |= GENERIC_WRITE;

    // WriteOnly can create files, ReadOnly cannot.
    DWORD creationDisp = (openMode & QIODevice::NewOnly)
                            ? CREATE_NEW
                            : openModeCanCreate(openMode)
                                ? OPEN_ALWAYS
                                : OPEN_EXISTING;
    // Create the file handle.
    QNativeFilePermissions nativePermissions(permissions, false);
    if (!nativePermissions.isOk())
        return false;

    fileHandle = CreateFile((const wchar_t*)fileEntry.nativeFilePath().utf16(),
                            accessRights,
                            shareMode,
                            nativePermissions.securityAttributes(),
                            creationDisp,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    // Bail out on error.
    if (fileHandle == INVALID_HANDLE_VALUE) {
        q->setError(QFile::OpenError, qt_error_string());
        return false;
    }

    // Truncate the file after successfully opening it if Truncate is passed.
    if (openMode & QIODevice::Truncate)
        q->setSize(0);

    return true;
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeClose()
{
    Q_Q(QFSFileEngine);
    if (fh || fd != -1) {
        // stdlib / stdio mode.
        return closeFdFh();
    }

    // Windows native mode.
    bool ok = true;

    if (cachedFd != -1) {
        if (::_close(cachedFd) && !::CloseHandle(fileHandle)) {
            q->setError(QFile::UnspecifiedError, qt_error_string());
            ok = false;
        }

        // System handle is closed with associated file descriptor.
        fileHandle = INVALID_HANDLE_VALUE;
        cachedFd = -1;

        return ok;
    }

    if ((fileHandle == INVALID_HANDLE_VALUE || !::CloseHandle(fileHandle))) {
        q->setError(QFile::UnspecifiedError, qt_error_string());
        ok = false;
    }
    fileHandle = INVALID_HANDLE_VALUE;
    return ok;
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeFlush()
{
    if (fh) {
        // Buffered stdlib mode.
        return flushFh();
    }
    if (fd != -1) {
        // Unbuffered stdio mode; always succeeds (no buffer).
        return true;
    }

    // Windows native mode; flushing is unnecessary.
    return true;
}

/*
    \internal
    \since 5.1
*/
bool QFSFileEnginePrivate::nativeSyncToDisk()
{
    if (fh || fd != -1) {
        // stdlib / stdio mode. No API available.
        return false;
    }
    return FlushFileBuffers(fileHandle);
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeSize() const
{
    Q_Q(const QFSFileEngine);
    QFSFileEngine *thatQ = const_cast<QFSFileEngine *>(q);

    // ### Don't flush; for buffered files, we should get away with ftell.
    thatQ->flush();

    // Always retrieve the current information
    metaData.clearFlags(QFileSystemMetaData::SizeAttribute);
    bool filled = false;
    if (fileHandle != INVALID_HANDLE_VALUE && openMode != QIODevice::NotOpen )
        filled = QFileSystemEngine::fillMetaData(fileHandle, metaData,
                                                 QFileSystemMetaData::SizeAttribute);
    else
        filled = doStat(QFileSystemMetaData::SizeAttribute);

    if (!filled) {
        thatQ->setError(QFile::UnspecifiedError, QSystemError::stdString());
        return 0;
    }
    return metaData.size();
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativePos() const
{
    Q_Q(const QFSFileEngine);
    QFSFileEngine *thatQ = const_cast<QFSFileEngine *>(q);

    if (fh || fd != -1) {
        // stdlib / stido mode.
        return posFdFh();
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return 0;

    LARGE_INTEGER currentFilePos;
    LARGE_INTEGER offset;
    offset.QuadPart = 0;
    if (!::SetFilePointerEx(fileHandle, offset, &currentFilePos, FILE_CURRENT)) {
        thatQ->setError(QFile::UnspecifiedError, qt_error_string());
        return 0;
    }

    return qint64(currentFilePos.QuadPart);
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeSeek(qint64 pos)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdlib / stdio mode.
        return seekFdFh(pos);
    }

    LARGE_INTEGER currentFilePos;
    LARGE_INTEGER offset;
    offset.QuadPart = pos;
    if (!::SetFilePointerEx(fileHandle, offset, &currentFilePos, FILE_BEGIN)) {
        q->setError(QFile::UnspecifiedError, qt_error_string());
        return false;
    }

    return true;
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeRead(char *data, qint64 maxlen)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdio / stdlib mode.
        if (fh && nativeIsSequential() && feof(fh)) {
            q->setError(QFile::ReadError, QSystemError::stdString());
            return -1;
        }

        return readFdFh(data, maxlen);
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return -1;

    qint64 bytesToRead = maxlen;

    // Reading on Windows fails with ERROR_NO_SYSTEM_RESOURCES when
    // the chunks are too large, so we limit the block size to 32MB.
    static const qint64 maxBlockSize = 32 * 1024 * 1024;

    qint64 totalRead = 0;
    do {
        DWORD blockSize = DWORD(qMin(bytesToRead, maxBlockSize));
        DWORD bytesRead;
        if (!ReadFile(fileHandle, data + totalRead, blockSize, &bytesRead, NULL)) {
            if (totalRead == 0) {
                // Note: only return failure if the first ReadFile fails.
                q->setError(QFile::ReadError, qt_error_string());
                return -1;
            }
            break;
        }
        if (bytesRead == 0)
            break;
        totalRead += bytesRead;
        bytesToRead -= bytesRead;
    } while (totalRead < maxlen);
    return totalRead;
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeReadLine(char *data, qint64 maxlen)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdio / stdlib mode.
        return readLineFdFh(data, maxlen);
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return -1;

    // ### No equivalent in Win32?
    return q->QAbstractFileEngine::readLine(data, maxlen);
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeWrite(const char *data, qint64 len)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdio / stdlib mode.
        return writeFdFh(data, len);
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return -1;

    qint64 bytesToWrite = len;

    // Writing on Windows fails with ERROR_NO_SYSTEM_RESOURCES when
    // the chunks are too large, so we limit the block size to 32MB.
    qint64 totalWritten = 0;
    do {
        const DWORD currentBlockSize = DWORD(qMin(bytesToWrite, qint64(32 * 1024 * 1024)));
        DWORD bytesWritten;
        if (!WriteFile(fileHandle, data + totalWritten, currentBlockSize, &bytesWritten, NULL)) {
            if (totalWritten == 0) {
                // Note: Only return error if the first WriteFile failed.
                q->setError(QFile::WriteError, qt_error_string());
                return -1;
            }
            break;
        }
        if (bytesWritten == 0)
            break;
        totalWritten += bytesWritten;
        bytesToWrite -= bytesWritten;
    } while (totalWritten < len);
    return qint64(totalWritten);
}

/*
    \internal
*/
int QFSFileEnginePrivate::nativeHandle() const
{
    if (fh || fd != -1)
        return fh ? QT_FILENO(fh) : fd;
    if (cachedFd != -1)
        return cachedFd;

    int flags = 0;
    if (openMode & QIODevice::Append)
        flags |= _O_APPEND;
    if (!(openMode & QIODevice::WriteOnly))
        flags |= _O_RDONLY;
    cachedFd = _open_osfhandle((intptr_t) fileHandle, flags);
    return cachedFd;
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeIsSequential() const
{
    HANDLE handle = fileHandle;
    if (fh || fd != -1)
        handle = (HANDLE)_get_osfhandle(fh ? QT_FILENO(fh) : fd);
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    DWORD fileType = GetFileType(handle);
    return (fileType == FILE_TYPE_CHAR)
            || (fileType == FILE_TYPE_PIPE);
}

bool QFSFileEnginePrivate::nativeRenameOverwrite(const QFileSystemEntry &newEntry)
{
    if (fileHandle == INVALID_HANDLE_VALUE)
        return false;
    const QString newFilePath = newEntry.nativeFilePath();
    const size_t nameByteLength = newFilePath.length() * sizeof(wchar_t);
    if (nameByteLength + sizeof(wchar_t) > std::numeric_limits<DWORD>::max())
        return false;

    constexpr size_t RenameInfoSize = sizeof(FILE_RENAME_INFO);
    const size_t renameDataSize = RenameInfoSize + nameByteLength + sizeof(wchar_t);
    QVarLengthArray<char> v(qsizetype(renameDataSize), 0);

    auto *renameInfo = q20::construct_at(reinterpret_cast<FILE_RENAME_INFO *>(v.data()));
    auto renameInfoRAII = qScopeGuard([&] { std::destroy_at(renameInfo); });
    renameInfo->ReplaceIfExists = TRUE;
    renameInfo->RootDirectory = nullptr;
    renameInfo->FileNameLength = DWORD(nameByteLength);
    memcpy(renameInfo->FileName, newFilePath.data(), nameByteLength);

    bool res = SetFileInformationByHandle(fileHandle, FileRenameInfo, renameInfo,
                                          DWORD(renameDataSize));
    if (!res) {
        DWORD error = GetLastError();
        q_func()->setError(QFile::RenameError, qt_error_string(int(error)));
    }
    return res;
}

QString QFSFileEngine::currentPath(const QString &fileName)
{
    QString ret;
    //if filename is a drive: then get the pwd of that drive
    if (fileName.length() >= 2 &&
        fileName.at(0).isLetter() && fileName.at(1) == u':') {
        int drv = fileName.toUpper().at(0).toLatin1() - 'A' + 1;
        if (_getdrive() != drv) {
            wchar_t buf[PATH_MAX];
            ::_wgetdcwd(drv, buf, PATH_MAX);
            ret = QString::fromWCharArray(buf);
        }
    }
    if (ret.isEmpty()) {
        //just the pwd
        ret = QFileSystemEngine::currentPath().filePath();
    }
    if (ret.length() >= 2 && ret[1] == u':')
        ret[0] = ret.at(0).toUpper(); // Force uppercase drive letters.
    return ret;
}

// cf QStorageInfo::isReady
static inline bool isDriveReady(const wchar_t *path)
{
    DWORD fileSystemFlags;
    const UINT driveType = GetDriveType(path);
    return (driveType != DRIVE_REMOVABLE && driveType != DRIVE_CDROM)
        || GetVolumeInformation(path, nullptr, 0, nullptr, nullptr,
                                &fileSystemFlags, nullptr, 0) == TRUE;
}

QFileInfoList QFSFileEngine::drives()
{
    QFileInfoList ret;
    const UINT oldErrorMode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    quint32 driveBits = (quint32) GetLogicalDrives() & 0x3ffffff;
    wchar_t driveName[] = L"A:\\";

    while (driveBits) {
        if ((driveBits & 1) && isDriveReady(driveName))
            ret.append(QFileInfo(QString::fromWCharArray(driveName)));
        driveName[0]++;
        driveBits = driveBits >> 1;
    }
    ::SetErrorMode(oldErrorMode);
    return ret;
}

bool QFSFileEnginePrivate::doStat(QFileSystemMetaData::MetaDataFlags flags) const
{
    if (!tried_stat || !metaData.hasFlags(flags)) {
        tried_stat = true;

        int localFd = fd;
        if (fh && fileEntry.isEmpty())
            localFd = QT_FILENO(fh);
        if (localFd != -1)
            QFileSystemEngine::fillMetaData(localFd, metaData, flags);
        if (metaData.missingFlags(flags) && !fileEntry.isEmpty())
            QFileSystemEngine::fillMetaData(fileEntry, metaData, metaData.missingFlags(flags));
    }

    return metaData.exists();
}

// ### assume that they add .lnk to newName
bool QFSFileEngine::link(const QString &newName)
{
    QSystemError error;
    bool ret = QFileSystemEngine::createLink(QFileSystemEntry(fileName(AbsoluteName)),
                                             QFileSystemEntry(newName), error);
    if (!ret)
        setError(QFile::RenameError, error.toString());
    return ret;
}

/*!
    \reimp
*/
QAbstractFileEngine::FileFlags QFSFileEngine::fileFlags(QAbstractFileEngine::FileFlags type) const
{
    Q_D(const QFSFileEngine);

    if (type & Refresh)
        d->metaData.clear();

    QAbstractFileEngine::FileFlags ret;

    if (type & FlagsMask)
        ret |= LocalDiskFlag;

    bool exists;
    {
        QFileSystemMetaData::MetaDataFlags queryFlags;

        queryFlags |= QFileSystemMetaData::MetaDataFlags::fromInt(type.toInt())
                & QFileSystemMetaData::Permissions;

        // AliasType and BundleType are 0x0
        if (type & TypesMask)
            queryFlags |= QFileSystemMetaData::AliasType
                    | QFileSystemMetaData::LinkType
                    | QFileSystemMetaData::FileType
                    | QFileSystemMetaData::DirectoryType
                    | QFileSystemMetaData::BundleType;

        if (type & FlagsMask)
            queryFlags |= QFileSystemMetaData::HiddenAttribute
                    | QFileSystemMetaData::ExistsAttribute;

        queryFlags |= QFileSystemMetaData::LinkType;

        exists = d->doStat(queryFlags);
    }

    if (exists && (type & PermsMask))
        ret |= FileFlags::fromInt(d->metaData.permissions().toInt());

    if (type & TypesMask) {
        if ((type & LinkType) && d->metaData.isLegacyLink())
            ret |= LinkType;
        if (d->metaData.isDirectory()) {
            ret |= DirectoryType;
        } else {
            ret |= FileType;
        }
    }
    if (type & FlagsMask) {
        if (d->metaData.exists()) {
            // if we succeeded in querying, then the file exists: a file on
            // Windows cannot be deleted if we have an open handle to it
            ret |= ExistsFlag;
            if (d->fileEntry.isRoot())
                ret |= RootFlag;
            else if (d->metaData.isHidden())
                ret |= HiddenFlag;
        }
    }
    return ret;
}

QByteArray QFSFileEngine::id() const
{
    Q_D(const QFSFileEngine);
    HANDLE h = d->fileHandle;
    if (h == INVALID_HANDLE_VALUE) {
        int localFd = d->fd;
        if (d->fh && d->fileEntry.isEmpty())
            localFd = QT_FILENO(d->fh);
        if (localFd != -1)
            h = HANDLE(_get_osfhandle(localFd));
    }
    if (h != INVALID_HANDLE_VALUE)
        return QFileSystemEngine::id(h);

    // file is not open, try by path
    return QFileSystemEngine::id(d->fileEntry);
}

QString QFSFileEngine::fileName(FileName file) const
{
    Q_D(const QFSFileEngine);
    switch (file) {
    case BaseName:
        return d->fileEntry.fileName();
    case PathName:
        return d->fileEntry.path();
    case AbsoluteName:
    case AbsolutePathName: {
        QString ret = d->fileEntry.filePath();
        if (isRelativePath()) {
            ret = QDir::cleanPath(QDir::currentPath() + u'/' + ret);
        } else if (ret.startsWith(u'/') // absolute path to the current drive, so \a.txt -> Z:\a.txt
                   || ret.size() == 2 // or a drive letter that needs to get a working dir appended
                   // or a drive-relative path, so Z:a.txt -> Z:\currentpath\a.txt
                   || (ret.size() > 2 && ret.at(2) != u'/')
                   || ret.contains(QStringView(u"/../"))
                   || ret.contains(QStringView(u"/./"))
                   || ret.endsWith(QStringView(u"/.."))
                   || ret.endsWith(QStringView(u"/."))) {
            ret = QDir::fromNativeSeparators(QFileSystemEngine::nativeAbsoluteFilePath(ret));
        }

        // The path should be absolute at this point.
        // From the docs :
        // Absolute paths begin with the directory separator "/"
        // (optionally preceded by a drive specification under Windows).
        if (ret.at(0) != u'/') {
            Q_ASSERT(ret.length() >= 2);
            Q_ASSERT(ret.at(0).isLetter());
            Q_ASSERT(ret.at(1) == u':');

            // Force uppercase drive letters.
            ret[0] = ret.at(0).toUpper();
        }

        if (file == AbsolutePathName) {
            int slash = ret.lastIndexOf(u'/');
            if (slash < 0)
                return ret;
            if (ret.at(0) != u'/' && slash == 2)
                return ret.left(3);      // include the slash
            return ret.left(slash > 0 ? slash : 1);
        }
        return ret;
    }
    case CanonicalName:
    case CanonicalPathName: {
        if (!(fileFlags(ExistsFlag) & ExistsFlag))
            return QString();
        const QFileSystemEntry entry =
            QFileSystemEngine::canonicalName(QFileSystemEntry(fileName(AbsoluteName)), d->metaData);

        if (file == CanonicalPathName)
            return entry.path();
        return entry.filePath();
    }
    case AbsoluteLinkTarget:
        return QFileSystemEngine::getLinkTarget(d->fileEntry, d->metaData).filePath();
    case RawLinkPath:
        return QFileSystemEngine::getRawLinkPath(d->fileEntry, d->metaData).filePath();
    case BundleName:
        return QString();
    case JunctionName:
        return QFileSystemEngine::getJunctionTarget(d->fileEntry, d->metaData).filePath();
    case DefaultName:
        break;
    case NFileNames:
        Q_ASSERT(false);
        break;
    }
    return d->fileEntry.filePath();
}

bool QFSFileEngine::isRelativePath() const
{
    Q_D(const QFSFileEngine);
    // drive, e.g. "a:", or UNC root, e.q. "//"
    return d->fileEntry.isRelative();
}

uint QFSFileEngine::ownerId(FileOwner /*own*/) const
{
    static const uint nobodyID = (uint) -2;
    return nobodyID;
}

QString QFSFileEngine::owner(FileOwner own) const
{
    Q_D(const QFSFileEngine);
    return QFileSystemEngine::owner(d->fileEntry, own);
}

bool QFSFileEngine::setPermissions(uint perms)
{
    Q_D(QFSFileEngine);
    QSystemError error;

    // clear cached state (if any)
    d->metaData.clearFlags(QFileSystemMetaData::Permissions);

    bool ret = QFileSystemEngine::setPermissions(d->fileEntry, QFile::Permissions(perms), error);
    if (!ret)
        setError(QFile::PermissionsError, error.toString());
    return ret;
}

bool QFSFileEngine::setSize(qint64 size)
{
    Q_D(QFSFileEngine);

    if (d->fileHandle != INVALID_HANDLE_VALUE || d->fd != -1 || d->fh) {
        // resize open file
        HANDLE fh = d->fileHandle;
        if (fh == INVALID_HANDLE_VALUE) {
            if (d->fh)
                fh = (HANDLE)_get_osfhandle(QT_FILENO(d->fh));
            else
                fh = (HANDLE)_get_osfhandle(d->fd);
        }
        if (fh == INVALID_HANDLE_VALUE)
            return false;
        qint64 currentPos = pos();

        if (seek(size) && SetEndOfFile(fh)) {
            seek(qMin(currentPos, size));
            return true;
        }

        seek(currentPos);
        return false;
    }

    if (!d->fileEntry.isEmpty()) {
        // resize file on disk
        QFile file(d->fileEntry.filePath());
        if (file.open(QFile::ReadWrite)) {
            bool ret = file.resize(size);
            if (!ret)
                setError(QFile::ResizeError, file.errorString());
            return ret;
        }
    }
    return false;
}

bool QFSFileEngine::setFileTime(const QDateTime &newDate, QFile::FileTime time)
{
    Q_D(QFSFileEngine);

    if (d->openMode == QFile::NotOpen) {
        setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
        return false;
    }

    if (!newDate.isValid() || time == QFile::FileMetadataChangeTime) {
        setError(QFile::UnspecifiedError, qt_error_string(ERROR_INVALID_PARAMETER));
        return false;
    }

    HANDLE handle = d->fileHandle;
    if (handle == INVALID_HANDLE_VALUE) {
        if (d->fh)
            handle = reinterpret_cast<HANDLE>(::_get_osfhandle(QT_FILENO(d->fh)));
        else if (d->fd != -1)
            handle = reinterpret_cast<HANDLE>(::_get_osfhandle(d->fd));
    }

    if (handle == INVALID_HANDLE_VALUE) {
        setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
        return false;
    }

    QSystemError error;
    if (!QFileSystemEngine::setFileTime(handle, newDate, time, error)) {
        setError(QFile::PermissionsError, error.toString());
        return false;
    }

    d->metaData.clearFlags(QFileSystemMetaData::Times);
    return true;
}

uchar *QFSFileEnginePrivate::map(qint64 offset, qint64 size,
                                 QFile::MemoryMapFlags flags)
{
    Q_Q(QFSFileEngine);
    Q_UNUSED(flags);
    if (openMode == QFile::NotOpen) {
        q->setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
        return 0;
    }
    if (offset == 0 && size == 0) {
        q->setError(QFile::UnspecifiedError, qt_error_string(ERROR_INVALID_PARAMETER));
        return 0;
    }

    // check/setup args to map
    DWORD access = 0;
    if (flags & QFileDevice::MapPrivateOption) {
#ifdef FILE_MAP_COPY
        access = FILE_MAP_COPY;
#else
        q->setError(QFile::UnspecifiedError, "MapPrivateOption unsupported");
        return 0;
#endif
    } else if (openMode & QIODevice::WriteOnly) {
        access = FILE_MAP_WRITE;
    } else if (openMode & QIODevice::ReadOnly) {
        access = FILE_MAP_READ;
    }

    if (mapHandle == NULL) {
        // get handle to the file
        HANDLE handle = fileHandle;

        if (handle == INVALID_HANDLE_VALUE && fh)
            handle = (HANDLE)::_get_osfhandle(QT_FILENO(fh));

#ifdef Q_USE_DEPRECATED_MAP_API
        nativeClose();
        // handle automatically closed by kernel with mapHandle (below).
        handle = ::CreateFileForMapping((const wchar_t*)fileEntry.nativeFilePath().utf16(),
                GENERIC_READ | (openMode & QIODevice::WriteOnly ? GENERIC_WRITE : 0),
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
        // Since this is a special case, we check if the return value was NULL and if so
        // we change it to INVALID_HANDLE_VALUE to follow the logic inside this function.
        if (!handle)
            handle = INVALID_HANDLE_VALUE;
#endif

        if (handle == INVALID_HANDLE_VALUE) {
            q->setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
            return 0;
        }

        // first create the file mapping handle
        DWORD protection = (openMode & QIODevice::WriteOnly) ? PAGE_READWRITE : PAGE_READONLY;
        mapHandle = ::CreateFileMapping(handle, 0, protection, 0, 0, 0);
        if (mapHandle == NULL) {
            q->setError(QFile::PermissionsError, qt_error_string());
#ifdef Q_USE_DEPRECATED_MAP_API
            ::CloseHandle(handle);
#endif
            return 0;
        }
    }

    DWORD offsetHi = offset >> 32;
    DWORD offsetLo = offset & Q_UINT64_C(0xffffffff);
    SYSTEM_INFO sysinfo;
    ::GetSystemInfo(&sysinfo);
    DWORD mask = sysinfo.dwAllocationGranularity - 1;
    DWORD extra = offset & mask;
    if (extra)
        offsetLo &= ~mask;

    // attempt to create the map
    LPVOID mapAddress = ::MapViewOfFile(mapHandle, access,
                                      offsetHi, offsetLo, size + extra);
    if (mapAddress) {
        uchar *address = extra + static_cast<uchar*>(mapAddress);
        maps[address] = extra;
        return address;
    }

    switch(GetLastError()) {
    case ERROR_ACCESS_DENIED:
        q->setError(QFile::PermissionsError, qt_error_string());
        break;
    case ERROR_INVALID_PARAMETER:
        // size are out of bounds
    default:
        q->setError(QFile::UnspecifiedError, qt_error_string());
    }

    ::CloseHandle(mapHandle);
    mapHandle = NULL;
    return 0;
}

bool QFSFileEnginePrivate::unmap(uchar *ptr)
{
    Q_Q(QFSFileEngine);
    const auto it = std::as_const(maps).find(ptr);
    if (it == maps.cend()) {
        q->setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
        return false;
    }
    uchar *start = ptr - *it;
    if (!UnmapViewOfFile(start)) {
        q->setError(QFile::PermissionsError, qt_error_string());
        return false;
    }

    maps.erase(it);
    if (maps.isEmpty()) {
        ::CloseHandle(mapHandle);
        mapHandle = NULL;
    }

    return true;
}

/*!
    \reimp
*/
QAbstractFileEngine::TriStateResult QFSFileEngine::cloneTo(QAbstractFileEngine *target)
{
    // There's some Windows Server 2016 API, but we won't
    // bother with it.
    Q_UNUSED(target);
    return TriStateResult::NotSupported;
}

QT_END_NAMESPACE
