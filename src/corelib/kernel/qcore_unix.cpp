/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <QtCore/private/qglobal_p.h>
#include "qcore_unix_p.h"
#include "qelapsedtimer.h"

#include <stdlib.h>

#ifdef __GLIBC__
#  include <sys/syscall.h>
#  include <pthread.h>
#  include <unistd.h>
#endif

#ifdef Q_OS_MAC
#include <mach/mach_time.h>
#endif

QT_BEGIN_NAMESPACE

QByteArray qt_readlink(const char *path)
{
#ifndef PATH_MAX
    // suitably large value that won't consume too much memory
#  define PATH_MAX  1024*1024
#endif

    QByteArray buf(256, Qt::Uninitialized);

    ssize_t len = ::readlink(path, buf.data(), buf.size());
    while (len == buf.size()) {
        // readlink(2) will fill our buffer and not necessarily terminate with NUL;
        if (buf.size() >= PATH_MAX) {
            errno = ENAMETOOLONG;
            return QByteArray();
        }

        // double the size and try again
        buf.resize(buf.size() * 2);
        len = ::readlink(path, buf.data(), buf.size());
    }

    if (len == -1)
        return QByteArray();

    buf.resize(len);
    return buf;
}

#if defined(Q_PROCESSOR_X86_32) && defined(__GLIBC__)
#  if !__GLIBC_PREREQ(2, 22)
// glibc prior to release 2.22 had a bug that suppresses the third argument to
// open() / open64() / openat(), causing file creation with O_TMPFILE to have
// the wrong permissions. So we bypass the glibc implementation and go straight
// for the syscall. See
// https://sourceware.org/git/?p=glibc.git;a=commit;h=65f6f938cd562a614a68e15d0581a34b177ec29d
int qt_open64(const char *pathname, int flags, mode_t mode)
{
    return syscall(SYS_open, pathname, flags | O_LARGEFILE, mode);
}
#  endif
#endif

#ifndef QT_BOOTSTRAPPED

#if QT_CONFIG(poll_pollts)
#  define ppoll pollts
#endif

static inline bool time_update(struct timespec *tv, const struct timespec &start,
                               const struct timespec &timeout)
{
    // clock source is (hopefully) monotonic, so we can recalculate how much timeout is left;
    // if it isn't monotonic, we'll simply hope that it hasn't jumped, because we have no alternative
    struct timespec now = qt_gettime();
    *tv = timeout + start - now;
    return tv->tv_sec >= 0;
}

#if QT_CONFIG(poll_poll)
static inline int timespecToMillisecs(const struct timespec *ts)
{
    return (ts == NULL) ? -1 :
           (ts->tv_sec * 1000) + (ts->tv_nsec / 1000000);
}
#endif

// defined in qpoll.cpp
int qt_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts);

static inline int qt_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts)
{
#if QT_CONFIG(poll_ppoll) || QT_CONFIG(poll_pollts)
    return ::ppoll(fds, nfds, timeout_ts, nullptr);
#elif QT_CONFIG(poll_poll)
    return ::poll(fds, nfds, timespecToMillisecs(timeout_ts));
#else
    return qt_poll(fds, nfds, timeout_ts);
#endif
}


/*!
    \internal

    Behaves as close to POSIX poll(2) as practical but may be implemented
    using select(2) where necessary. In that case, returns -1 and sets errno
    to EINVAL if passed any descriptor greater than or equal to FD_SETSIZE.
*/
int qt_safe_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts)
{
    if (!timeout_ts) {
        // no timeout -> block forever
        int ret;
        EINTR_LOOP(ret, qt_ppoll(fds, nfds, nullptr));
        return ret;
    }

    timespec start = qt_gettime();
    timespec timeout = *timeout_ts;

    // loop and recalculate the timeout as needed
    forever {
        const int ret = qt_ppoll(fds, nfds, &timeout);
        if (ret != -1 || errno != EINTR)
            return ret;

        // recalculate the timeout
        if (!time_update(&timeout, start, *timeout_ts)) {
            // timeout during update
            // or clock reset, fake timeout error
            return 0;
        }
    }
}

#endif // QT_BOOTSTRAPPED

QT_END_NAMESPACE
