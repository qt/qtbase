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

#ifndef _POSIX_POLL
# if defined(QT_HAVE_PPOLL) || _POSIX_VERSION >= 200809L || _XOPEN_VERSION >= 700
#  define _POSIX_POLL  1
# elif defined(QT_HAVE_POLL)
#  define _POSIX_POLL  0
# else
#  define _POSIX_POLL -1
# endif
#endif

#if defined(Q_OS_QNX) || _POSIX_POLL <= 0 || defined(QT_BUILD_INTERNAL)
static inline struct timeval timespecToTimeval(const struct timespec &ts)
{
    struct timeval tv;

    tv.tv_sec = ts.tv_sec;
    tv.tv_usec = ts.tv_nsec / 1000;

    return tv;
}
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

#if _POSIX_POLL <= 0 || defined(QT_BUILD_INTERNAL)

#define QT_POLL_READ_MASK   (POLLIN | POLLRDNORM)
#define QT_POLL_WRITE_MASK  (POLLOUT | POLLWRNORM | POLLWRBAND)
#define QT_POLL_EXCEPT_MASK (POLLPRI | POLLRDBAND)
#define QT_POLL_ERROR_MASK  (POLLERR | POLLNVAL)
#define QT_POLL_EVENTS_MASK (QT_POLL_READ_MASK | QT_POLL_WRITE_MASK | QT_POLL_EXCEPT_MASK)

static inline int qt_poll_prepare(struct pollfd *fds, nfds_t nfds,
                                  fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
    int max_fd = -1;

    FD_ZERO(read_fds);
    FD_ZERO(write_fds);
    FD_ZERO(except_fds);

    for (nfds_t i = 0; i < nfds; i++) {
        if (fds[i].fd >= FD_SETSIZE) {
            errno = EINVAL;
            return -1;
        }

        if ((fds[i].fd < 0) || (fds[i].revents & QT_POLL_ERROR_MASK))
            continue;

        if (fds[i].events & QT_POLL_READ_MASK)
            FD_SET(fds[i].fd, read_fds);

        if (fds[i].events & QT_POLL_WRITE_MASK)
            FD_SET(fds[i].fd, write_fds);

        if (fds[i].events & QT_POLL_EXCEPT_MASK)
            FD_SET(fds[i].fd, except_fds);

        if (fds[i].events & QT_POLL_EVENTS_MASK)
            max_fd = qMax(max_fd, fds[i].fd);
    }

    return max_fd + 1;
}

static inline void qt_poll_examine_ready_read(struct pollfd &pfd)
{
    int res;
    char data;

    EINTR_LOOP(res, ::recv(pfd.fd, &data, sizeof(data), MSG_PEEK));
    const int error = (res < 0) ? errno : 0;

    if (res == 0) {
        pfd.revents |= POLLHUP;
    } else if (res > 0 || error == ENOTSOCK || error == ENOTCONN) {
        pfd.revents |= QT_POLL_READ_MASK & pfd.events;
    } else {
        switch (error) {
        case ESHUTDOWN:
        case ECONNRESET:
        case ECONNABORTED:
        case ENETRESET:
            pfd.revents |= POLLHUP;
            break;
        default:
            pfd.revents |= POLLERR;
            break;
        }
    }
}

static inline int qt_poll_sweep(struct pollfd *fds, nfds_t nfds,
                                fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
    int result = 0;

    for (nfds_t i = 0; i < nfds; i++) {
        if (fds[i].fd < 0)
            continue;

        if (FD_ISSET(fds[i].fd, read_fds))
            qt_poll_examine_ready_read(fds[i]);

        if (FD_ISSET(fds[i].fd, write_fds))
            fds[i].revents |= QT_POLL_WRITE_MASK & fds[i].events;

        if (FD_ISSET(fds[i].fd, except_fds))
            fds[i].revents |= QT_POLL_EXCEPT_MASK & fds[i].events;

        if (fds[i].revents != 0)
            result++;
    }

    return result;
}

static inline bool qt_poll_is_bad_fd(int fd)
{
    int ret;
    EINTR_LOOP(ret, fcntl(fd, F_GETFD));
    return (ret == -1 && errno == EBADF);
}

static inline int qt_poll_mark_bad_fds(struct pollfd *fds, const nfds_t nfds)
{
    int n_marked = 0;

    for (nfds_t i = 0; i < nfds; i++) {
        if (fds[i].fd < 0)
            continue;

        if (fds[i].revents & QT_POLL_ERROR_MASK)
            continue;

        if (qt_poll_is_bad_fd(fds[i].fd)) {
            fds[i].revents |= POLLNVAL;
            n_marked++;
        }
   }

   return n_marked;
}

Q_AUTOTEST_EXPORT int qt_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts)
{
    if (!fds && nfds) {
        errno = EFAULT;
        return -1;
    }

    fd_set read_fds, write_fds, except_fds;
    struct timeval tv, *ptv = 0;

    if (timeout_ts) {
        tv = timespecToTimeval(*timeout_ts);
        ptv = &tv;
    }

    int n_bad_fds = 0;

    for (nfds_t i = 0; i < nfds; i++) {
        fds[i].revents = 0;

        if (fds[i].fd < 0)
            continue;

        if (fds[i].events & QT_POLL_EVENTS_MASK)
            continue;

        if (qt_poll_is_bad_fd(fds[i].fd)) {
            // Mark bad file descriptors that have no event flags set
            // here, as we won't be passing them to select below and therefore
            // need to do the check ourselves
            fds[i].revents = POLLNVAL;
            n_bad_fds++;
        }
    }

    forever {
        const int max_fd = qt_poll_prepare(fds, nfds, &read_fds, &write_fds, &except_fds);

        if (max_fd < 0)
            return max_fd;

        if (n_bad_fds > 0) {
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            ptv = &tv;
        }

        const int ret = ::select(max_fd, &read_fds, &write_fds, &except_fds, ptv);

        if (ret == 0)
            return n_bad_fds;

        if (ret > 0)
            return qt_poll_sweep(fds, nfds, &read_fds, &write_fds, &except_fds);

        if (errno != EBADF)
            return -1;

        // We have at least one bad file descriptor that we waited on, find out which and try again
        n_bad_fds += qt_poll_mark_bad_fds(fds, nfds);
    }
}

#endif // _POSIX_POLL <= 0 || defined(QT_BUILD_INTERNAL)

#if !defined(QT_HAVE_PPOLL) && ((_POSIX_POLL > 0) || defined(_SC_POLL))
static inline int timespecToMillisecs(const struct timespec *ts)
{
    return (ts == NULL) ? -1 :
           (ts->tv_sec * 1000) + (ts->tv_nsec / 1000000);
}
#endif

static inline int qt_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts)
{
#if defined(QT_HAVE_PPOLL)
    return ::ppoll(fds, nfds, timeout_ts, Q_NULLPTR);
#elif _POSIX_POLL > 0
    return ::poll(fds, nfds, timespecToMillisecs(timeout_ts));
#else
# if defined(_SC_POLL)
    static const bool have_poll = (sysconf(_SC_POLL) > 0);
    if (have_poll)
        return ::poll(fds, nfds, timespecToMillisecs(timeout_ts));
# endif
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
