// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"

#include "qtipccommon_p.h"

#include <qdir.h>
#include <qdebug.h>

#include <errno.h>

#if QT_CONFIG(sharedmemory)
#if QT_CONFIG(sysv_shm)
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "private/qcore_unix_p.h"
#if defined(Q_OS_DARWIN)
#include "private/qcore_mac_p.h"
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtIpcCommon;

bool QSharedMemorySystemV::runtimeSupportCheck()
{
#if defined(Q_OS_DARWIN)
    if (qt_apple_isSandboxed())
        return false;
#endif
    static const bool result = []() {
        (void)shmget(IPC_PRIVATE, ~size_t(0), 0);     // this will fail
        return errno != ENOSYS;
    }();
    return result;
}


inline void QSharedMemorySystemV::updateNativeKeyFile(const QNativeIpcKey &nativeKey)
{
    Q_ASSERT(nativeKeyFile.isEmpty() );
    if (!nativeKey.nativeKey().isEmpty())
        nativeKeyFile = QFile::encodeName(nativeKey.nativeKey());
}

/*!
    \internal

    If not already made create the handle used for accessing the shared memory.
*/
key_t QSharedMemorySystemV::handle(QSharedMemoryPrivate *self)
{
    // already made
    if (unix_key)
        return unix_key;

    // don't allow making handles on empty keys
    if (nativeKeyFile.isEmpty())
        updateNativeKeyFile(self->nativeKey);
    if (nativeKeyFile.isEmpty()) {
        self->setError(QSharedMemory::KeyError,
                       QSharedMemory::tr("%1: key is empty")
                       .arg("QSharedMemory::handle:"_L1));
        return 0;
    }

    unix_key = ftok(nativeKeyFile, int(self->nativeKey.type()));
    if (unix_key < 0) {
        self->setUnixErrorString("QSharedMemory::handle"_L1);
        nativeKeyFile.clear();
        unix_key = 0;
    }
    return unix_key;
}

bool QSharedMemorySystemV::cleanHandle(QSharedMemoryPrivate *self)
{
    if (unix_key == 0)
        return true;

    // Get the number of current attachments
    struct shmid_ds shmid_ds;
    QByteArray keyfile = std::exchange(nativeKeyFile, QByteArray());

    int id = shmget(unix_key, 0, 0400);
    unix_key = 0;
    if (shmctl(id, IPC_STAT, &shmid_ds))
        return errno != EINVAL;

    // If there are still attachments, keep the keep file and shm
    if (shmid_ds.shm_nattch != 0)
        return true;

    if (shmctl(id, IPC_RMID, &shmid_ds) < 0) {
        if (errno != EINVAL) {
            self->setUnixErrorString("QSharedMemory::remove"_L1);
            return false;
        }
    };

    // remove file
    return unlink(keyfile) == 0;
}

bool QSharedMemorySystemV::create(QSharedMemoryPrivate *self, qsizetype size)
{
    // build file if needed
    bool createdFile = false;
    updateNativeKeyFile(self->nativeKey);
    int built = createUnixKeyFile(nativeKeyFile);
    if (built == -1) {
        self->setError(QSharedMemory::KeyError,
                       QSharedMemory::tr("%1: unable to make key")
                       .arg("QSharedMemory::handle:"_L1));
        return false;
    }
    if (built == 1) {
        createdFile = true;
    }

    // get handle
    if (!handle(self)) {
        if (createdFile)
            unlink(nativeKeyFile);
        return false;
    }

    // create
    if (-1 == shmget(unix_key, size_t(size), 0600 | IPC_CREAT | IPC_EXCL)) {
        const auto function = "QSharedMemory::create"_L1;
        switch (errno) {
        case EINVAL:
            self->setError(QSharedMemory::InvalidSize,
                           QSharedMemory::tr("%1: system-imposed size restrictions")
                           .arg("QSharedMemory::handle"_L1));
            break;
        default:
            self->setUnixErrorString(function);
        }
        if (createdFile && self->error != QSharedMemory::AlreadyExists)
            unlink(nativeKeyFile);
        return false;
    }

    return true;
}

bool QSharedMemorySystemV::attach(QSharedMemoryPrivate *self, QSharedMemory::AccessMode mode)
{
    // grab the shared memory segment id
    int id = shmget(unix_key, 0, (mode == QSharedMemory::ReadOnly ? 0400 : 0600));
    if (-1 == id) {
        self->setUnixErrorString("QSharedMemory::attach (shmget)"_L1);
        unix_key = 0;
        nativeKeyFile.clear();
        return false;
    }

    // grab the memory
    self->memory = shmat(id, nullptr, (mode == QSharedMemory::ReadOnly ? SHM_RDONLY : 0));
    if (self->memory == MAP_FAILED) {
        self->memory = nullptr;
        self->setUnixErrorString("QSharedMemory::attach (shmat)"_L1);
        return false;
    }

    // grab the size
    shmid_ds shmid_ds;
    if (!shmctl(id, IPC_STAT, &shmid_ds)) {
        self->size = (qsizetype)shmid_ds.shm_segsz;
    } else {
        self->setUnixErrorString("QSharedMemory::attach (shmctl)"_L1);
        return false;
    }

    return true;
}

bool QSharedMemorySystemV::detach(QSharedMemoryPrivate *self)
{
    // detach from the memory segment
    if (shmdt(self->memory) < 0) {
        const auto function = "QSharedMemory::detach"_L1;
        switch (errno) {
        case EINVAL:
            self->setError(QSharedMemory::NotFound,
                           QSharedMemory::tr("%1: not attached").arg(function));
            break;
        default:
            self->setUnixErrorString(function);
        }
        return false;
    }
    self->memory = nullptr;
    self->size = 0;

    return cleanHandle(self);
}

QT_END_NAMESPACE

#endif // QT_CONFIG(sysv_shm)
#endif // QT_CONFIG(sharedmemory)
