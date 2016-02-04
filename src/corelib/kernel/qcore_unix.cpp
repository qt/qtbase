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
