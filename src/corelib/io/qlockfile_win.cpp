// Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qlockfile_p.h"
#include "private/qfilesystementry_p.h"
#include <qt_windows.h>
#include <psapi.h>

#include "QtCore/qfileinfo.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qdebug.h"
#include "QtCore/qthread.h"

QT_BEGIN_NAMESPACE

static inline bool fileExists(const wchar_t *fileName)
{
    WIN32_FILE_ATTRIBUTE_DATA  data;
    return GetFileAttributesEx(fileName, GetFileExInfoStandard, &data);
}

QLockFile::LockError QLockFilePrivate::tryLock_sys()
{
    const QFileSystemEntry fileEntry(fileName);
    // When writing, allow others to read.
    // When reading, QFile will allow others to read and write, all good.
    // Adding FILE_SHARE_DELETE would allow forceful deletion of stale files,
    // but Windows doesn't allow recreating it while this handle is open anyway,
    // so this would only create confusion (can't lock, but no lock file to read from).
    const DWORD dwShareMode = FILE_SHARE_READ;
    SECURITY_ATTRIBUTES securityAtts = { sizeof(SECURITY_ATTRIBUTES), NULL, FALSE };
    HANDLE fh = CreateFile((const wchar_t*)fileEntry.nativeFilePath().utf16(),
                           GENERIC_READ | GENERIC_WRITE,
                           dwShareMode,
                           &securityAtts,
                           CREATE_NEW, // error if already exists
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    if (fh == INVALID_HANDLE_VALUE) {
        const DWORD lastError = GetLastError();
        switch (lastError) {
        case ERROR_SHARING_VIOLATION:
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            return QLockFile::LockFailedError;
        case ERROR_ACCESS_DENIED:
            // readonly file, or file still in use by another process.
            // Assume the latter if the file exists, since we don't create it readonly.
            return fileExists((const wchar_t*)fileEntry.nativeFilePath().utf16())
                ? QLockFile::LockFailedError
                : QLockFile::PermissionError;
        default:
            qWarning("Got unexpected locking error %llu", quint64(lastError));
            return QLockFile::UnknownError;
        }
    }

    // We hold the lock, continue.
    fileHandle = fh;
    QByteArray fileData = lockFileContents();
    DWORD bytesWritten = 0;
    QLockFile::LockError error = QLockFile::NoError;
    if (!WriteFile(fh, fileData.constData(), fileData.size(), &bytesWritten, NULL) || !FlushFileBuffers(fh))
        error = QLockFile::UnknownError; // partition full
    return error;
}

bool QLockFilePrivate::removeStaleLock()
{
    // QFile::remove fails on Windows if the other process is still using the file, so it's not stale.
    return QFile::remove(fileName);
}

bool QLockFilePrivate::isProcessRunning(qint64 pid, const QString &appname)
{
    HANDLE procHandle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!procHandle)
        return false;

    // We got a handle but check if process is still alive
    DWORD exitCode = 0;
    if (!::GetExitCodeProcess(procHandle, &exitCode))
        exitCode = 0;
    ::CloseHandle(procHandle);
    if (exitCode != STILL_ACTIVE)
        return false;

    const QString processName = processNameByPid(pid);
    if (!processName.isEmpty() && processName != appname)
        return false; // PID got reused by a different application.

    return true;
}

QString QLockFilePrivate::processNameByPid(qint64 pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, DWORD(pid));
    if (!hProcess) {
        return QString();
    }
    wchar_t buf[MAX_PATH];
    const DWORD length = GetModuleFileNameExW(hProcess, NULL, buf, sizeof(buf) / sizeof(wchar_t));
    CloseHandle(hProcess);
    if (!length)
        return QString();
    QString name = QString::fromWCharArray(buf, length);
    int i = name.lastIndexOf(u'\\');
    if (i >= 0)
        name.remove(0, i + 1);
    i = name.lastIndexOf(u'.');
    if (i >= 0)
        name.truncate(i);
    return name;
}

void QLockFile::unlock()
{
    Q_D(QLockFile);
    if (!d->isLocked)
        return;
    CloseHandle(d->fileHandle);
    int attempts = 0;
    static const int maxAttempts = 500; // 500ms
    while (!QFile::remove(d->fileName) && ++attempts < maxAttempts) {
        // Someone is reading the lock file right now (on Windows this prevents deleting it).
        QThread::msleep(1);
    }
    if (attempts == maxAttempts) {
        qWarning() << "Could not remove our own lock file" << d->fileName
                   << ". Either other users of the lock file are reading it constantly for 500 ms, "
                      "or we (no longer) have permissions to delete the file";
        // This is bad because other users of this lock file will now have to wait for the
        // stale-lock-timeout...
    }
    d->lockError = QLockFile::NoError;
    d->isLocked = false;
}

QT_END_NAMESPACE
