// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"
#include "qsystemsemaphore.h"
#include <qdebug.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(sharedmemory)

void QSharedMemoryPrivate::setWindowsErrorString(QLatin1StringView function)
{
    DWORD windowsError = GetLastError();
    if (windowsError == 0)
        return;
    switch (windowsError) {
    case ERROR_ALREADY_EXISTS:
        error = QSharedMemory::AlreadyExists;
        errorString = QSharedMemory::tr("%1: already exists").arg(function);
    break;
    case ERROR_FILE_NOT_FOUND:
        error = QSharedMemory::NotFound;
        errorString = QSharedMemory::tr("%1: doesn't exist").arg(function);
        break;
    case ERROR_COMMITMENT_LIMIT:
        error = QSharedMemory::InvalidSize;
        errorString = QSharedMemory::tr("%1: invalid size").arg(function);
        break;
    case ERROR_NO_SYSTEM_RESOURCES:
    case ERROR_NOT_ENOUGH_MEMORY:
        error = QSharedMemory::OutOfResources;
        errorString = QSharedMemory::tr("%1: out of resources").arg(function);
        break;
    case ERROR_ACCESS_DENIED:
        error = QSharedMemory::PermissionDenied;
        errorString = QSharedMemory::tr("%1: permission denied").arg(function);
        break;
    default:
        errorString = QSharedMemory::tr("%1: unknown error: %2")
                .arg(function, qt_error_string(windowsError));
        error = QSharedMemory::UnknownError;
#if defined QSHAREDMEMORY_DEBUG
        qDebug() << errorString << "key" << key;
#endif
    }
}

HANDLE QSharedMemoryWin32::handle(QSharedMemoryPrivate *self)
{
    if (!hand) {
        const auto function = "QSharedMemory::handle"_L1;
        if (self->nativeKey.isEmpty()) {
            self->setError(QSharedMemory::KeyError,
                           QSharedMemory::tr("%1: unable to make key").arg(function));
            return 0;
        }
        hand = OpenFileMapping(FILE_MAP_ALL_ACCESS, false,
                               reinterpret_cast<const wchar_t *>(self->nativeKey.nativeKey().utf16()));
        if (!hand) {
            self->setWindowsErrorString(function);
            return 0;
        }
    }
    return hand;
}

bool QSharedMemoryWin32::cleanHandle(QSharedMemoryPrivate *)
{
    if (hand != 0 && !CloseHandle(hand)) {
        hand = 0;
        return false;
    }
    hand = 0;
    return true;
}

bool QSharedMemoryWin32::create(QSharedMemoryPrivate *self, qsizetype size)
{
    const auto function = "QSharedMemory::create"_L1;
    if (self->nativeKey.isEmpty()) {
        self->setError(QSharedMemory::KeyError,
                       QSharedMemory::tr("%1: key error").arg(function));
        return false;
    }

    // Create the file mapping.
    DWORD high, low;
    if constexpr (sizeof(qsizetype) == 8)
        high = DWORD(quint64(size) >> 32);
    else
        high = 0;
    low = DWORD(size_t(size) & 0xffffffff);
    hand = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, high, low,
                             reinterpret_cast<const wchar_t *>(self->nativeKey.nativeKey().utf16()));
    self->setWindowsErrorString(function);

    // hand is valid when it already exists unlike unix so explicitly check
    return self->error != QSharedMemory::AlreadyExists && hand;
}

bool QSharedMemoryWin32::attach(QSharedMemoryPrivate *self, QSharedMemory::AccessMode mode)
{
    // Grab a pointer to the memory block
    int permissions = (mode == QSharedMemory::ReadOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS);
    self->memory = (void *)MapViewOfFile(handle(self), permissions, 0, 0, 0);
    if (!self->memory) {
        self->setWindowsErrorString("QSharedMemory::attach"_L1);
        cleanHandle(self);
        return false;
    }

    // Grab the size of the memory we have been given (a multiple of 4K on windows)
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery(self->memory, &info, sizeof(info))) {
        // Windows doesn't set an error code on this one,
        // it should only be a kernel memory error.
        self->setError(QSharedMemory::UnknownError,
                       QSharedMemory::tr("%1: size query failed")
                       .arg("QSharedMemory::attach: "_L1));
        return false;
    }
    self->size = qsizetype(info.RegionSize);

    return true;
}

bool QSharedMemoryWin32::detach(QSharedMemoryPrivate *self)
{
    // umap memory
    if (!UnmapViewOfFile(self->memory)) {
        self->setWindowsErrorString("QSharedMemory::detach"_L1);
        return false;
    }
    self->memory = 0;
    self->size = 0;

    // close handle
    return cleanHandle(self);
}

#endif // QT_CONFIG(sharedmemory)

QT_END_NAMESPACE
