/****************************************************************************
**
** Copyright (C) 2015 Konstantin Ritt <ritt.ks@gmail.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
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

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"
#include "qsystemsemaphore.h"
#include <qfile.h>

#include <errno.h>

#ifdef QT_POSIX_IPC

#ifndef QT_NO_SHAREDMEMORY
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "private/qcore_unix_p.h"

QT_BEGIN_NAMESPACE

int QSharedMemoryPrivate::handle()
{
    // don't allow making handles on empty keys
    const QString safeKey = makePlatformSafeKey(key);
    if (safeKey.isEmpty()) {
        errorString = QSharedMemory::tr("%1: key is empty").arg(QLatin1String("QSharedMemory::handle"));
        error = QSharedMemory::KeyError;
        return 0;
    }

    return 1;
}

bool QSharedMemoryPrivate::cleanHandle()
{
    qt_safe_close(hand);
    hand = -1;

    return true;
}

bool QSharedMemoryPrivate::create(int size)
{
    if (!handle())
        return false;

    const QByteArray shmName = QFile::encodeName(makePlatformSafeKey(key));

    int fd;
#ifdef O_CLOEXEC
    // First try with O_CLOEXEC flag, if that fails, fall back to normal flags
    EINTR_LOOP(fd, ::shm_open(shmName.constData(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0600));
    if (fd == -1)
        EINTR_LOOP(fd, ::shm_open(shmName.constData(), O_RDWR | O_CREAT | O_EXCL, 0600));
#else
    EINTR_LOOP(fd, ::shm_open(shmName.constData(), O_RDWR | O_CREAT | O_EXCL, 0600));
#endif
    if (fd == -1) {
        const int errorNumber = errno;
        const QLatin1String function("QSharedMemory::attach (shm_open)");
        switch (errorNumber) {
        case ENAMETOOLONG:
        case EINVAL:
            errorString = QSharedMemory::tr("%1: bad name").arg(function);
            error = QSharedMemory::KeyError;
            break;
        default:
            setErrorString(function);
        }
        return false;
    }

    // the size may only be set once
    int ret;
    EINTR_LOOP(ret, QT_FTRUNCATE(fd, size));
    if (ret == -1) {
        setErrorString(QLatin1String("QSharedMemory::create (ftruncate)"));
        qt_safe_close(fd);
        return false;
    }

    qt_safe_close(fd);

    return true;
}

bool QSharedMemoryPrivate::attach(QSharedMemory::AccessMode mode)
{
    const QByteArray shmName = QFile::encodeName(makePlatformSafeKey(key));

    const int oflag = (mode == QSharedMemory::ReadOnly ? O_RDONLY : O_RDWR);
    const mode_t omode = (mode == QSharedMemory::ReadOnly ? 0400 : 0600);

#ifdef O_CLOEXEC
    // First try with O_CLOEXEC flag, if that fails, fall back to normal flags
    EINTR_LOOP(hand, ::shm_open(shmName.constData(), oflag | O_CLOEXEC, omode));
    if (hand == -1)
        EINTR_LOOP(hand, ::shm_open(shmName.constData(), oflag, omode));
#else
    EINTR_LOOP(hand, ::shm_open(shmName.constData(), oflag, omode));
#endif
    if (hand == -1) {
        const int errorNumber = errno;
        const QLatin1String function("QSharedMemory::attach (shm_open)");
        switch (errorNumber) {
        case ENAMETOOLONG:
        case EINVAL:
            errorString = QSharedMemory::tr("%1: bad name").arg(function);
            error = QSharedMemory::KeyError;
            break;
        default:
            setErrorString(function);
        }
        hand = -1;
        return false;
    }

    // grab the size
    QT_STATBUF st;
    if (QT_FSTAT(hand, &st) == -1) {
        setErrorString(QLatin1String("QSharedMemory::attach (fstat)"));
        cleanHandle();
        return false;
    }
    size = st.st_size;

    // grab the memory
    const int mprot = (mode == QSharedMemory::ReadOnly ? PROT_READ : PROT_READ | PROT_WRITE);
    memory = QT_MMAP(0, size, mprot, MAP_SHARED, hand, 0);
    if (memory == MAP_FAILED || !memory) {
        setErrorString(QLatin1String("QSharedMemory::attach (mmap)"));
        cleanHandle();
        memory = 0;
        size = 0;
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

bool QSharedMemoryPrivate::detach()
{
    // detach from the memory segment
    if (::munmap(memory, size) == -1) {
        setErrorString(QLatin1String("QSharedMemory::detach (munmap)"));
        return false;
    }
    memory = 0;
    size = 0;

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

    cleanHandle();

    // if there are no attachments then unlink the shared memory
    if (shm_nattch == 0) {
        const QByteArray shmName = QFile::encodeName(makePlatformSafeKey(key));
        if (::shm_unlink(shmName.constData()) == -1 && errno != ENOENT)
            setErrorString(QLatin1String("QSharedMemory::detach (shm_unlink)"));
    }
#else
    // On non-QNX systems (tested Linux and Haiku), the st_nlink field is always 1,
    // so we'll simply leak the shared memory files.
    cleanHandle();
#endif

    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_SHAREDMEMORY

#endif // QT_POSIX_IPC
