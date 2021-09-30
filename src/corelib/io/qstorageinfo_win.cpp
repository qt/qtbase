/****************************************************************************
**
** Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
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

#include "qstorageinfo_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvarlengtharray.h>

#include "qfilesystementry_p.h"
#include "private/qsystemlibrary_p.h"

#include "qntdll_p.h"

QT_BEGIN_NAMESPACE

static const int defaultBufferSize = MAX_PATH + 1;

static QString canonicalPath(const QString &rootPath)
{
    QString path = QDir::toNativeSeparators(QFileInfo(rootPath).canonicalFilePath());
    if (path.isEmpty())
        return path;

    if (path.startsWith(QLatin1String("\\\\?\\")))
        path.remove(0, 4);
    if (path.length() < 2 || path.at(1) != QLatin1Char(':'))
        return QString();

    path[0] = path[0].toUpper();
    if (!(path.at(0).unicode() >= 'A' && path.at(0).unicode() <= 'Z'))
        return QString();
    if (!path.endsWith(QLatin1Char('\\')))
        path.append(QLatin1Char('\\'));
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

QStorageInfo QStorageInfoPrivate::root()
{
    return QStorageInfo(QDir::fromNativeSeparators(QFile::decodeName(qgetenv("SystemDrive"))));
}

bool QStorageInfoPrivate::queryStorageProperty()
{
    QString path = QDir::toNativeSeparators(uR"(\\.\)" + rootPath);
    if (path.endsWith(u'\\'))
        path.chop(1);

    HANDLE handle = CreateFile(reinterpret_cast<const wchar_t *>(path.utf16()),
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
        blockSize = saad.BytesPerPhysicalSector;
    return result;
}

struct Helper
{
    QBasicMutex mutex;
    QSystemLibrary ntdll {u"ntdll"_qs};
};
Q_GLOBAL_STATIC(Helper, gNtdllHelper)

inline QFunctionPointer resolveSymbol(QSystemLibrary *ntdll, const char *name)
{
    QFunctionPointer symbolFunctionPointer = ntdll->resolve(name);
    if (Q_UNLIKELY(!symbolFunctionPointer))
        qWarning("Failed to resolve the symbol: %s", name);
    return symbolFunctionPointer;
}

#define GENERATE_SYMBOL(symbolName, returnType, ...) \
using Qt##symbolName = returnType (NTAPI *) (__VA_ARGS__); \
static Qt##symbolName qt##symbolName = nullptr;

#define RESOLVE_SYMBOL(name) \
    do { \
        qt##name = reinterpret_cast<Qt##name>(resolveSymbol(ntdll, #name)); \
        if (!qt##name) \
            return false; \
    } while (false)

GENERATE_SYMBOL(RtlInitUnicodeString, void, PUNICODE_STRING, PCWSTR);
GENERATE_SYMBOL(NtCreateFile, NTSTATUS, PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES,
    PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
GENERATE_SYMBOL(NtQueryVolumeInformationFile, NTSTATUS, HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG,
    FS_INFORMATION_CLASS);

void QStorageInfoPrivate::queryFileFsSectorSizeInformation()
{
    static bool symbolsResolved = [](auto ntdllHelper) {
        QMutexLocker locker(&ntdllHelper->mutex);
        auto ntdll = &ntdllHelper->ntdll;
        if (!ntdll->isLoaded()) {
            if (!ntdll->load()) {
                qWarning("Unable to load ntdll.dll.");
                return false;
            }
        }

        RESOLVE_SYMBOL(RtlInitUnicodeString);
        RESOLVE_SYMBOL(NtCreateFile);
        RESOLVE_SYMBOL(NtQueryVolumeInformationFile);

        return true;
    }(gNtdllHelper());
    if (!symbolsResolved)
        return;

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
    qtRtlInitUnicodeString(&name, reinterpret_cast<const wchar_t *>(path.utf16()));

    InitializeObjectAttributes(&attrs, &name, 0, nullptr, nullptr);

    NTSTATUS status = qtNtCreateFile(&handle,
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
    status = qtNtQueryVolumeInformationFile(handle,
                                            &isb,
                                            &ffssi,
                                            sizeof(ffssi),
                                            FS_INFORMATION_CLASS(10)); // FileFsSectorSizeInformation
    CloseHandle(handle);
    if (NT_SUCCESS(status))
        blockSize = ffssi.PhysicalBytesPerSectorForAtomicity;
}

QT_END_NAMESPACE
