// Copyright (C) 2015 Konstantin Ritt <ritt.ks@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"
#include "qtipccommon_p.h"
#include <qfile.h>

#include <errno.h>

#if QT_CONFIG(sharedmemory)
#if QT_CONFIG(posix_shm)
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "private/qcore_unix_p.h"

#ifndef O_CLOEXEC
#  define O_CLOEXEC 0
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtIpcCommon;

bool QSharedMemoryPosix::runtimeSupportCheck()
{
    static const bool result = []() {
        (void)shm_open("", 0, 0);         // this WILL fail
        return errno != ENOSYS;
    }();
    return result;
}

bool QSharedMemoryPosix::handle(QSharedMemoryPrivate *self)
{
    // don't allow making handles on empty keys
    if (self->nativeKey.isEmpty()) {
        self->setError(QSharedMemory::KeyError,
                       QSharedMemory::tr("%1: key is empty").arg("QSharedMemory::handle"_L1));
        return false;
    }

    return true;
}

bool QSharedMemoryPosix::cleanHandle(QSharedMemoryPrivate *)
{
    if (hand != -1)
        qt_safe_close(hand);
    hand = -1;

    return true;
}

bool QSharedMemoryPosix::create(QSharedMemoryPrivate *self, qsizetype size)
{
    if (!handle(self))
        return false;

    const QByteArray shmName = QFile::encodeName(self->nativeKey.nativeKey());

    int fd;
    QT_EINTR_LOOP(fd, ::shm_open(shmName.constData(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600));
    if (fd == -1) {
        const int errorNumber = errno;
        const auto function = "QSharedMemory::attach (shm_open)"_L1;
        switch (errorNumber) {
        case EINVAL:
            self->setError(QSharedMemory::KeyError,
                           QSharedMemory::tr("%1: bad name").arg(function));
            break;
        default:
            self->setUnixErrorString(function);
        }
        return false;
    }

    // the size may only be set once
    int ret;
    QT_EINTR_LOOP(ret, QT_FTRUNCATE(fd, size));
    if (ret == -1) {
        self->setUnixErrorString("QSharedMemory::create (ftruncate)"_L1);
        qt_safe_close(fd);
        return false;
    }

    qt_safe_close(fd);

    return true;
}

bool QSharedMemoryPosix::attach(QSharedMemoryPrivate *self, QSharedMemory::AccessMode mode)
{
    const QByteArray shmName = QFile::encodeName(self->nativeKey.nativeKey());

    const int oflag = (mode == QSharedMemory::ReadOnly ? O_RDONLY : O_RDWR);
    const mode_t omode = (mode == QSharedMemory::ReadOnly ? 0400 : 0600);

    QT_EINTR_LOOP(hand, ::shm_open(shmName.constData(), oflag | O_CLOEXEC, omode));
    if (hand == -1) {
        const int errorNumber = errno;
        const auto function = "QSharedMemory::attach (shm_open)"_L1;
        switch (errorNumber) {
        case EINVAL:
            self->setError(QSharedMemory::KeyError,
                           QSharedMemory::tr("%1: bad name").arg(function));
            break;
        default:
            self->setUnixErrorString(function);
        }
        hand = -1;
        return false;
    }

    // grab the size
    QT_STATBUF st;
    if (QT_FSTAT(hand, &st) == -1) {
        self->setUnixErrorString("QSharedMemory::attach (fstat)"_L1);
        cleanHandle(self);
        return false;
    }
    self->size = qsizetype(st.st_size);

    // grab the memory
    const int mprot = (mode == QSharedMemory::ReadOnly ? PROT_READ : PROT_READ | PROT_WRITE);
    self->memory = QT_MMAP(0, size_t(self->size), mprot, MAP_SHARED, hand, 0);
    if (self->memory == MAP_FAILED || !self->memory) {
        self->setUnixErrorString("QSharedMemory::attach (mmap)"_L1);
        cleanHandle(self);
        self->memory = 0;
        self->size = 0;
        return false;
    }

#ifdef F_ADD_SEALS
    // Make sure the shared memory region will not shrink
    // otherwise someone could cause SIGBUS on us.
    // (see http://lwn.net/Articles/594919/)
    fcntl(hand, F_ADD_SEALS, F_SEAL_SHRINK);
#endif

    return true;
}

bool QSharedMemoryPosix::detach(QSharedMemoryPrivate *self)
{
    // detach from the memory segment
    if (::munmap(self->memory, size_t(self->size)) == -1) {
        self->setUnixErrorString("QSharedMemory::detach (munmap)"_L1);
        return false;
    }
    self->memory = 0;
    self->size = 0;

#ifdef Q_OS_QNX
    // On QNX the st_nlink field of struct stat contains the number of
    // active shm_open() connections to the shared memory file, so we
    // can use it to automatically clean up the file once the last
    // user has detached from it.

    // get the number of current attachments
    int shm_nattch = 0;
    QT_STATBUF st;
    if (QT_FSTAT(hand, &st) == 0) {
        // subtract 2 from linkcount: one for our own open and one for the dir entry
        shm_nattch = st.st_nlink - 2;
    }

    cleanHandle(self);

    // if there are no attachments then unlink the shared memory
    if (shm_nattch == 0) {
        const QByteArray shmName = QFile::encodeName(self->nativeKey.nativeKey());
        if (::shm_unlink(shmName.constData()) == -1 && errno != ENOENT)
            self->setUnixErrorString("QSharedMemory::detach (shm_unlink)"_L1);
    }
#else
    // On non-QNX systems (tested Linux and Haiku), the st_nlink field is always 1,
    // so we'll simply leak the shared memory files.
    cleanHandle(self);
#endif

    return true;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(posix_shm)
#endif // QT_CONFIG(sharedmemory)
