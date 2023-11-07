// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcore_unix_p.h"

#ifdef Q_OS_RTEMS
#include <rtems/rtems_bsdnet_internal.h>
#endif

QT_BEGIN_NAMESPACE

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

    QT_EINTR_LOOP(res, ::recv(pfd.fd, &data, sizeof(data), MSG_PEEK));
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
#ifdef Q_OS_RTEMS
    if (!rtems_bsdnet_fdToSocket(fd))
        return true;
#endif

    int ret;
    QT_EINTR_LOOP(ret, fcntl(fd, F_GETFD));
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

int qt_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts)
{
    if (!fds && nfds) {
        errno = EFAULT;
        return -1;
    }

    fd_set read_fds, write_fds, except_fds;
    struct timeval tv, *ptv = nullptr;

    if (timeout_ts) {
        tv = timespecToTimeval(*timeout_ts);
        ptv = &tv;
    }

    int n_bad_fds = 0;

    for (nfds_t i = 0; i < nfds; i++) {
        fds[i].revents = 0;

        if (fds[i].fd < 0)
            continue;

        if (fds[i].fd > FD_SETSIZE) {
            errno = EINVAL;
            return -1;
        }

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

QT_END_NAMESPACE
