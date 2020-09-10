/****************************************************************************
**
** Copyright (C) 2020 Intel Corporation.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
**
****************************************************************************/

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include "forkfd.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "forkfd_atomic.h"

#ifndef CLONE_PIDFD
#  define CLONE_PIDFD   0x00001000
#endif
#ifndef P_PIDFD
#  define P_PIDFD       3
#endif

// in forkfd.c
static int convertForkfdWaitFlagsToWaitFlags(int ffdoptions);
static void convertStatusToForkfdInfo(int status, struct forkfd_info *info);

static ffd_atomic_int system_forkfd_state = FFD_ATOMIC_INIT(0);

static int sys_waitid(int which, int pid_or_pidfd, siginfo_t *infop, int options,
                      struct rusage *ru)
{
    /* use the waitid raw system call, which has an extra parameter that glibc
     * doesn't offer to us */
    return syscall(__NR_waitid, which, pid_or_pidfd, infop, options, ru);
}

static int sys_clone(unsigned long cloneflags, int *ptid)
{
    void *child_stack = NULL;
    int *ctid = NULL;
    unsigned long newtls = 0;
#if defined(__NR_clone2)
    size_t stack_size = 0;
    return syscall(__NR_clone2, cloneflags, child_stack, stack_size, ptid, ctid, newtls);
#elif defined(__cris__) || defined(__s390__)
    /* a.k.a., CONFIG_CLONE_BACKWARDS2 architectures */
    return syscall(__NR_clone, child_stack, cloneflags, ptid, newtls, ctid);
#elif defined(__microblaze__)
    /* a.k.a., CONFIG_CLONE_BACKWARDS3 architectures */
    size_t stack_size = 0;
    return syscall(__NR_clone, cloneflags, child_stack, stack_size, ptid, newtls, ctid);
#elif defined(__arc__) || defined(__arm__) || defined(__aarch64__) || defined(__mips__) || \
    defined(__nds32__) || defined(__hppa__) || defined(__powerpc__) || defined(__i386__) || \
    defined(__x86_64__) || defined(__xtensa__) || defined(__alpha__) || defined(__riscv)
    /* ctid and newtls are inverted on CONFIG_CLONE_BACKWARDS architectures,
     * but since both values are 0, there's no harm. */
    return syscall(__NR_clone, cloneflags, child_stack, ptid, ctid, newtls);
#else
    (void) child_stack;
    (void) ctid;
    (void) newtls;
    errno = ENOSYS;
    return -1;
#endif
}

static int detect_clone_pidfd_support()
{
    /*
     * Detect support for CLONE_PIDFD and P_PIDFD. Support was added in steps:
     * - Linux 5.2 added CLONE_PIDFD support in clone(2) system call
     * - Linux 5.2 added pidfd_send_signal(2)
     * - Linux 5.3 added support for poll(2) on pidfds
     * - Linux 5.3 added clone3(2)
     * - Linux 5.4 added P_PIDFD support in waitid(2)
     *
     * We need CLONE_PIDFD and the poll(2) support. We could emulate the
     * P_PIDFD support by reading the PID from /proc/self/fdinfo/n, which works
     * in Linux 5.2, but without poll(2), we can't guarantee the functionality
     * anyway.
     *
     * So we detect by trying to waitid(2) on a positive file descriptor that
     * is definitely closed (INT_MAX). If P_PIDFD is supported, waitid(2) will
     * return EBADF. If it isn't supported, it returns EINVAL (as it would for
     * a negative file descriptor). This will succeed on Linux 5.4.
     *
     * We could have instead detected by the existence of the clone3(2) system
     * call, but for that we would have needed to wait for __NR_clone3 to show
     * up on the libcs. We choose to go via the waitid(2) route, which requires
     * platform-independent constants only. It would have simplified the
     * sys_clone() mess above...
     */

    sys_waitid(P_PIDFD, INT_MAX, NULL, WEXITED|WNOHANG, NULL);
    return errno == EBADF ? 1 : -1;
}

int system_has_forkfd()
{
    return ffd_atomic_load(&system_forkfd_state, FFD_ATOMIC_RELAXED) > 0;
}

int system_forkfd(int flags, pid_t *ppid, int *system)
{
    pid_t pid;
    int pidfd;

    int state = ffd_atomic_load(&system_forkfd_state, FFD_ATOMIC_RELAXED);
    if (state == 0) {
        state = detect_clone_pidfd_support();
        ffd_atomic_store(&system_forkfd_state, state, FFD_ATOMIC_RELAXED);
    }
    if (state < 0) {
        *system = 0;
        return state;
    }

    *system = 1;
    unsigned long cloneflags = CLONE_PIDFD | SIGCHLD;
    pid = sys_clone(cloneflags, &pidfd);
    if (ppid)
        *ppid = pid;

    if (pid == 0) {
        /* Child process */
        return FFD_CHILD_PROCESS;
    }

    /* parent process */
    if ((flags & FFD_CLOEXEC) == 0) {
        /* pidfd defaults to O_CLOEXEC */
        fcntl(pidfd, F_SETFD, 0);
    }
    if (flags & FFD_NONBLOCK)
        fcntl(pidfd, F_SETFL, fcntl(pidfd, F_GETFL) | O_NONBLOCK);
    return pidfd;
}

int system_forkfd_wait(int ffd, struct forkfd_info *info, int ffdoptions, struct rusage *rusage)
{
    siginfo_t si;
    int ret;
    int options = convertForkfdWaitFlagsToWaitFlags(ffdoptions);

    if ((options & WNOHANG) == 0) {
        /* check if the file descriptor is non-blocking */
        ret = fcntl(ffd, F_GETFL);
        if (ret == -1)
            return ret;
        if (ret & O_NONBLOCK)
            options |= WNOHANG;
    }

    ret = sys_waitid(P_PIDFD, ffd, &si, options, rusage);
    if (ret == -1 && errno == ECHILD) {
        errno = EWOULDBLOCK;
    } else if (ret == 0 && info) {
        info->code = si.si_code;
        info->status = si.si_status;
    }
    return ret;
}
