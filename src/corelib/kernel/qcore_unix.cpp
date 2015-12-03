/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcore_unix_p.h"
#include "qelapsedtimer.h"

#ifdef Q_OS_NACL
#elif !defined (Q_OS_VXWORKS)
# if !defined(Q_OS_HPUX) || defined(__ia64)
#  include <sys/select.h>
# endif
#  include <sys/time.h>
#else
#  include <selectLib.h>
#endif

#include <stdlib.h>

#ifdef Q_OS_MAC
#include <mach/mach_time.h>
#endif

QT_BEGIN_NAMESPACE

#if !defined(QT_HAVE_PPOLL) && defined(QT_HAVE_POLLTS)
# define ppoll pollts
# define QT_HAVE_PPOLL
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

int qt_safe_select(int nfds, fd_set *fdread, fd_set *fdwrite, fd_set *fdexcept,
                   const struct timespec *orig_timeout)
{
    if (!orig_timeout) {
        // no timeout -> block forever
        int ret;
        EINTR_LOOP(ret, select(nfds, fdread, fdwrite, fdexcept, 0));
        return ret;
    }

    timespec start = qt_gettime();
    timespec timeout = *orig_timeout;

    // loop and recalculate the timeout as needed
    int ret;
    forever {
#ifndef Q_OS_QNX
        ret = ::pselect(nfds, fdread, fdwrite, fdexcept, &timeout, 0);
#else
        timeval timeoutVal = timespecToTimeval(timeout);
        ret = ::select(nfds, fdread, fdwrite, fdexcept, &timeoutVal);
#endif
        if (ret != -1 || errno != EINTR)
            return ret;

        // recalculate the timeout
        if (!time_update(&timeout, start, *orig_timeout)) {
            // timeout during update
            // or clock reset, fake timeout error
            return 0;
        }
    }
}

static inline struct timespec millisecsToTimespec(const unsigned int ms)
{
    struct timespec tv;

    tv.tv_sec = ms / 1000;
    tv.tv_nsec = (ms % 1000) * 1000 * 1000;

    return tv;
}

int qt_select_msecs(int nfds, fd_set *fdread, fd_set *fdwrite, int timeout)
{
    if (timeout < 0)
        return qt_safe_select(nfds, fdread, fdwrite, 0, 0);

    struct timespec tv = millisecsToTimespec(timeout);
    return qt_safe_select(nfds, fdread, fdwrite, 0, &tv);
}

#if !defined(QT_HAVE_PPOLL) && defined(QT_HAVE_POLL)
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
#if defined(QT_HAVE_PPOLL)
    return ::ppoll(fds, nfds, timeout_ts, Q_NULLPTR);
#elif defined(QT_HAVE_POLL)
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
        EINTR_LOOP(ret, qt_ppoll(fds, nfds, Q_NULLPTR));
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

QT_END_NAMESPACE
