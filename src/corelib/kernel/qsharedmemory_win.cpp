// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"
#include "qsystemsemaphore.h"
#include <qdebug.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#ifndef QT_NO_SHAREDMEMORY

QSharedMemoryPrivate::QSharedMemoryPrivate() :
#ifndef QT_NO_QOBJECT
    QObjectPrivate(),
#endif
        memory(0), size(0), error(QSharedMemory::NoError),
           systemSemaphore(QString()), lockedByMe(false), hand(0)
{
}

void QSharedMemoryPrivate::setErrorString(QLatin1StringView function)
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
        errorString = QSharedMemory::tr("%1: unknown error %2").arg(function).arg(windowsError);
        error = QSharedMemory::UnknownError;
#if defined QSHAREDMEMORY_DEBUG
        qDebug() << errorString << "key" << key;
#endif
    }
}

HANDLE QSharedMemoryPrivate::handle()
{
    if (!hand) {
        const auto function = "QSharedMemory::handle"_L1;
        if (nativeKey.isEmpty()) {
            error = QSharedMemory::KeyError;
            errorString = QSharedMemory::tr("%1: unable to make key").arg(function);
            return 0;
        }
        hand = OpenFileMapping(FILE_MAP_ALL_ACCESS, false,
                               reinterpret_cast<const wchar_t *>(nativeKey.utf16()));
        if (!hand) {
            setErrorString(function);
            return 0;
        }
    }
    return hand;
}

bool QSharedMemoryPrivate::cleanHandle()
{
    if (hand != 0 && !CloseHandle(hand)) {
        hand = 0;
        setErrorString("QSharedMemory::cleanHandle"_L1);
        return false;
    }
    hand = 0;
    return true;
}

bool QSharedMemoryPrivate::create(qsizetype size)
{
    const auto function = "QSharedMemory::create"_L1;
    if (nativeKey.isEmpty()) {
        error = QSharedMemory::KeyError;
        errorString = QSharedMemory::tr("%1: key error").arg(function);
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
                             reinterpret_cast<const wchar_t *>(nativeKey.utf16()));
    setErrorString(function);

    // hand is valid when it already exists unlike unix so explicitly check
    return error != QSharedMemory::AlreadyExists && hand;
}

bool QSharedMemoryPrivate::attach(QSharedMemory::AccessMode mode)
{
    // Grab a pointer to the memory block
    int permissions = (mode == QSharedMemory::ReadOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS);
    memory = (void *)MapViewOfFile(handle(), permissions, 0, 0, 0);
    if (0 == memory) {
        setErrorString("QSharedMemory::attach"_L1);
        cleanHandle();
        return false;
    }

    // Grab the size of the memory we have been given (a multiple of 4K on windows)
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery(memory, &info, sizeof(info))) {
        // Windows doesn't set an error code on this one,
        // it should only be a kernel memory error.
        error = QSharedMemory::UnknownError;
        errorString = QSharedMemory::tr("%1: size query failed").arg("QSharedMemory::attach: "_L1);
        return false;
    }
    size = qsizetype(info.RegionSize);

    return true;
}

bool QSharedMemoryPrivate::detach()
{
    // umap memory
    if (!UnmapViewOfFile(memory)) {
        setErrorString("QSharedMemory::detach"_L1);
        return false;
    }
    memory = 0;
    size = 0;

    // close handle
    return cleanHandle();
}

#endif //QT_NO_SHAREDMEMORY


QT_END_NAMESPACE
