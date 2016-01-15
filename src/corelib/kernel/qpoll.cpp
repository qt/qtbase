/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qcore_unix_p.h"

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

int qt_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts)
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

QT_END_NAMESPACE
