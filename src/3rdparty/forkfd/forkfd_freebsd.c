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

#include "forkfd.h"

#include <sys/types.h>
#include <sys/procdesc.h>

#include "forkfd_atomic.h"

// in forkfd.c
static int convertForkfdWaitFlagsToWaitFlags(int ffdoptions);
static void convertStatusToForkfdInfo(int status, struct forkfd_info *info);

#if __FreeBSD__ >= 10
/* On FreeBSD 10, PROCDESC was enabled by default. On v11, it's not an option
 * anymore and can't be disabled. */
static ffd_atomic_int system_forkfd_state = FFD_ATOMIC_INIT(1);
#else
static ffd_atomic_int system_forkfd_state = FFD_ATOMIC_INIT(0);
#endif

int system_has_forkfd()
{
    return ffd_atomic_load(&system_forkfd_state, FFD_ATOMIC_RELAXED) > 0;
}

int system_forkfd(int flags, pid_t *ppid, int *system)
{
    int ret;
    pid_t pid;

    int state = ffd_atomic_load(&system_forkfd_state, FFD_ATOMIC_RELAXED);
    *system = 0;
    if (state < 0)
        return -1;

    pid = pdfork(&ret, PD_DAEMON);
#  if __FreeBSD__ == 9
    if (state == 0 && pid != 0) {
        /* Parent process: remember whether PROCDESC was compiled into the kernel */
        state = (pid == -1 && errno == ENOSYS) ? -1 : 1;
        ffd_atomic_store(&system_forkfd_state, state, FFD_ATOMIC_RELAXED);
    }
    if (state < 0)
        return -1;
#  endif
    *system = 1;
    if (__builtin_expect(pid == -1, 0))
        return -1;

    if (pid == 0) {
        /* child process */
        return FFD_CHILD_PROCESS;
    }

    /* parent process */
    if (flags & FFD_CLOEXEC)
        fcntl(ret, F_SETFD, FD_CLOEXEC);
    if (flags & FFD_NONBLOCK)
        fcntl(ret, F_SETFL, fcntl(ret, F_GETFL) | O_NONBLOCK);
    if (ppid)
        *ppid = pid;
    return ret;
}

int system_forkfd_wait(int ffd, struct forkfd_info *info, int ffdoptions, struct rusage *rusage)
{
    pid_t pid;
    int status;
    int options = convertForkfdWaitFlagsToWaitFlags(ffdoptions);

    int ret = pdgetpid(ffd, &pid);
    if (ret == -1)
        return ret;

    if ((options & WNOHANG) == 0) {
        /* check if the file descriptor is non-blocking */
        ret = fcntl(ffd, F_GETFL);
        if (ret == -1)
            return ret;
        options |= (ret & O_NONBLOCK) ? WNOHANG : 0;
    }
    ret = wait4(pid, &status, options, rusage);
    if (ret != -1 && info)
        convertStatusToForkfdInfo(status, info);
    return ret == -1 ? -1 : 0;
}
