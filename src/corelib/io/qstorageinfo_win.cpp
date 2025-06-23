// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qstorageinfo_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/private/wcharhelpers_win_p.h>

#include "qfilesystementry_p.h"

#include "qntdll_p.h"

extern "C" NTSTATUS NTSYSCALLAPI NTAPI NtQueryVolumeInformationFile(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG,
    FS_INFORMATION_CLASS);

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static const int defaultBufferSize = MAX_PATH + 1;

static QString canonicalPath(const QString &rootPath)
{
    QString path = QDir::toNativeSeparators(QFileInfo(rootPath).canonicalFilePath());
    if (path.isEmpty())
        return path;

    if (path.startsWith("\\\\?\\"_L1))
        path.remove(0, 4);
    if (path.length() < 2 || path.at(1) != u':')
        return QString();

    path[0] = path[0].toUpper();
    if (!(path.at(0).unicode() >= 'A' && path.at(0).unicode() <= 'Z'))
        return QString();
    if (!path.endsWith(u'\\'))
        path.append(u'\\');
    return path;
}

void QStorageInfoPrivate::initRootPath()
{
    // Do not unnecessarily call QFileInfo::canonicalFilePath() if the path is
    // already a drive root since it may hang on network drives.
    const QString path = QFileSystemEntry::isDriveRootPath(rootPath)
        ? QDir::toNativeSeparators(rootPath)
        : canonicalPath(rootPath);

    if (path.isEmpty()) {
        valid = ready = false;
        return;
    }

    // ### test if disk mounted to folder on other disk
    wchar_t buffer[defaultBufferSize];
    if (::GetVolumePathName(reinterpret_cast<const wchar_t *>(path.utf16()), buffer, defaultBufferSize))
        rootPath = QDir::fromNativeSeparators(QString::fromWCharArray(buffer));
    else
        valid = ready = false;
}

static inline QByteArray getDevice(const QString &rootPath)
{
    const QString path = QDir::toNativeSeparators(rootPath);
    const UINT type = ::GetDriveType(reinterpret_cast<const wchar_t *>(path.utf16()));
    if (type == DRIVE_REMOTE) {
        QVarLengthArray<char, 256> buffer(256);
        DWORD bufferLength = buffer.size();
        DWORD result;
        UNIVERSAL_NAME_INFO *remoteNameInfo;
        do {
            buffer.resize(bufferLength);
            remoteNameInfo = reinterpret_cast<UNIVERSAL_NAME_INFO *>(buffer.data());
            result = ::WNetGetUniversalName(reinterpret_cast<const wchar_t *>(path.utf16()),
                                            UNIVERSAL_NAME_INFO_LEVEL,
                                            remoteNameInfo,
                                            &bufferLength);
        } while (result == ERROR_MORE_DATA);
        if (result == NO_ERROR)
            return QString::fromWCharArray(remoteNameInfo->lpUniversalName).toUtf8();
        return QByteArray();
    }

    wchar_t deviceBuffer[51];
    if (::GetVolumeNameForVolumeMountPoint(reinterpret_cast<const wchar_t *>(path.utf16()),
                                           deviceBuffer,
                                           sizeof(deviceBuffer) / sizeof(wchar_t))) {
        return QString::fromWCharArray(deviceBuffer).toLatin1();
    }
    return QByteArray();
}

void QStorageInfoPrivate::doStat()
{
    valid = ready = true;
    initRootPath();
    if (!valid || !ready)
        return;

    retrieveVolumeInfo();
    if (!valid || !ready)
        return;
    device = getDevice(rootPath);
    retrieveDiskFreeSpace();

    if (!queryStorageProperty())
        queryFileFsSectorSizeInformation();
}

void QStorageInfoPrivate::retrieveVolumeInfo()
{
    const UINT oldmode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    const QString path = QDir::toNativeSeparators(rootPath);
    wchar_t nameBuffer[defaultBufferSize];
    wchar_t fileSystemTypeBuffer[defaultBufferSize];
    DWORD fileSystemFlags = 0;
    const bool result = ::GetVolumeInformation(reinterpret_cast<const wchar_t *>(path.utf16()),
                                               nameBuffer,
                                               defaultBufferSize,
                                               nullptr,
                                               nullptr,
                                               &fileSystemFlags,
                                               fileSystemTypeBuffer,
                                               defaultBufferSize);
    if (!result) {
        ready = false;
        valid = ::GetLastError() == ERROR_NOT_READY;
    } else {
        fileSystemType = QString::fromWCharArray(fileSystemTypeBuffer).toLatin1();
        name = QString::fromWCharArray(nameBuffer);

        readOnly = (fileSystemFlags & FILE_READ_ONLY_VOLUME) != 0;
    }

    ::SetErrorMode(oldmode);
}

void QStorageInfoPrivate::retrieveDiskFreeSpace()
{
    const UINT oldmode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    const QString path = QDir::toNativeSeparators(rootPath);
    ready = ::GetDiskFreeSpaceEx(reinterpret_cast<const wchar_t *>(path.utf16()),
                                 PULARGE_INTEGER(&bytesAvailable),
                                 PULARGE_INTEGER(&bytesTotal),
                                 PULARGE_INTEGER(&bytesFree));

    ::SetErrorMode(oldmode);
}

QList<QStorageInfo> QStorageInfoPrivate::mountedVolumes()
{
    QList<QStorageInfo> volumes;

    QString driveName = QStringLiteral("A:/");
    const UINT oldmode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    quint32 driveBits = quint32(::GetLogicalDrives()) & 0x3ffffff;
    ::SetErrorMode(oldmode);
    while (driveBits) {
        if (driveBits & 1) {
            QStorageInfo drive(driveName);
            if (!drive.rootPath().isEmpty()) // drive exists, but not mounted
                volumes.append(drive);
        }
        driveName[0] = QChar(driveName[0].unicode() + 1);
        driveBits = driveBits >> 1;
    }

    return volumes;
}

bool QStorageInfoPrivate::queryStorageProperty()
{
    QString path = QDir::toNativeSeparators(uR"(\\.\)" + rootPath);
    if (path.endsWith(u'\\'))
        path.chop(1);

    HANDLE handle = CreateFile(qt_castToWchar(path),
                               0, // no access to the drive
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr,
                               OPEN_EXISTING,
                               0,
                               nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    STORAGE_PROPERTY_QUERY spq;
    memset(&spq, 0, sizeof(spq));
    spq.PropertyId = StorageAccessAlignmentProperty;
    spq.QueryType = PropertyStandardQuery;

    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR saad;
    memset(&saad, 0, sizeof(saad));

    DWORD bytes = 0;
    BOOL result = DeviceIoControl(handle,
                                  IOCTL_STORAGE_QUERY_PROPERTY,
                                  &spq, sizeof(spq),
                                  &saad, sizeof(saad),
                                  &bytes,
                                  nullptr);
    CloseHandle(handle);
    if (result)
        blockSize = int(saad.BytesPerPhysicalSector);
    return result;
}

void QStorageInfoPrivate::queryFileFsSectorSizeInformation()
{
    FILE_FS_SECTOR_SIZE_INFORMATION ffssi;
    memset(&ffssi, 0, sizeof(ffssi));

    HANDLE handle = nullptr;

    OBJECT_ATTRIBUTES attrs;
    memset(&attrs, 0, sizeof(attrs));

    IO_STATUS_BLOCK isb;
    memset(&isb, 0, sizeof(isb));

    QString path = QDir::toNativeSeparators(uR"(\??\\)" + rootPath);
    if (!path.endsWith(u'\\'))
        path.append(u'\\');

    UNICODE_STRING name;
    ::RtlInitUnicodeString(&name, qt_castToWchar(path));

    InitializeObjectAttributes(&attrs, &name, 0, nullptr, nullptr);

    NTSTATUS status = ::NtCreateFile(&handle,
                                     FILE_READ_ATTRIBUTES,
                                     &attrs,
                                     &isb,
                                     nullptr,
                                     FILE_ATTRIBUTE_NORMAL,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     FILE_OPEN,
                                     0,
                                     nullptr,
                                     0);
    if (!NT_SUCCESS(status))
        return;

    memset(&isb, 0, sizeof(isb));
    status = ::NtQueryVolumeInformationFile(handle,
                                            &isb,
                                            &ffssi,
                                            sizeof(ffssi),
                                            FS_INFORMATION_CLASS(10)); // FileFsSectorSizeInformation
    CloseHandle(handle);
    if (NT_SUCCESS(status))
        blockSize = ffssi.PhysicalBytesPerSectorForAtomicity;
}

QT_END_NAMESPACE
