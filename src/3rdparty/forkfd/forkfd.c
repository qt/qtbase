/****************************************************************************
**
** Copyright (C) 2014 Intel Corporation
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#  define _POSIX_C_SOURCE 200809L
#  define _XOPEN_SOURCE 500
#endif
#include "forkfd.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __linux__
#  define HAVE_PIPE2    1
#  define HAVE_EVENTFD  1
#  include <sys/eventfd.h>
#endif

#if _POSIX_VERSION-0 >= 200809L || _XOPEN_VERSION-0 >= 500
#  define HAVE_WAITID   1
#endif

#ifndef FFD_ATOMIC_RELAXED
#  include "forkfd_gcc.h"
#endif

#define CHILDREN_IN_SMALL_ARRAY     16
#define CHILDREN_IN_BIG_ARRAY       256
#define sizeofarray(array)          (sizeof(array)/sizeof(array[0]))
#define EINTR_LOOP(ret, call) \
    do {                      \
        ret = call;           \
    } while (ret == -1 && errno == EINTR)

typedef struct process_info
{
    ffd_atomic_int pid;
    int deathPipe;
} ProcessInfo;

struct BigArray;
typedef struct Header
{
    ffd_atomic_pointer(struct BigArray) nextArray;
    ffd_atomic_int busyCount;
} Header;

typedef struct BigArray
{
    Header header;
    ProcessInfo entries[CHILDREN_IN_BIG_ARRAY];
} BigArray;

typedef struct SmallArray
{
    Header header;
    ProcessInfo entries[CHILDREN_IN_SMALL_ARRAY];
} SmallArray;
static SmallArray children;

static struct sigaction old_sigaction;
static pthread_once_t forkfd_initialization = PTHREAD_ONCE_INIT;
static ffd_atomic_int forkfd_status = FFD_ATOMIC_INIT(0);

static ProcessInfo *tryAllocateInSection(Header *header, ProcessInfo entries[], int maxCount)
{
    /* we use ACQUIRE here because the signal handler might have released the PID */
    int busyCount = ffd_atomic_add_fetch(&header->busyCount, 1, FFD_ATOMIC_ACQUIRE);
    if (busyCount <= maxCount) {
        /* there's an available entry in this section, find it and take it */
        int i;
        for (i = 0; i < maxCount; ++i) {
            /* if the PID is 0, it's free; mark it as used by swapping it with -1 */
            int expected_pid = 0;
            if (ffd_atomic_compare_exchange(&entries[i].pid, &expected_pid,
                                            -1, FFD_ATOMIC_RELAXED, FFD_ATOMIC_RELAXED))
                return &entries[i];
        }
    }

    /* there isn't an available entry, undo our increment */
    ffd_atomic_add_fetch(&header->busyCount, -1, FFD_ATOMIC_RELAXED);
    return NULL;
}

static ProcessInfo *allocateInfo(Header **header)
{
    Header *currentHeader = &children.header;

    /* try to find an available entry in the small array first */
    ProcessInfo *info =
            tryAllocateInSection(currentHeader, children.entries, sizeofarray(children.entries));

    /* go on to the next arrays */
    while (info == NULL) {
        BigArray *array = ffd_atomic_load(&currentHeader->nextArray, FFD_ATOMIC_ACQUIRE);
        if (array == NULL) {
            /* allocate an array and try to use it */
            BigArray *allocatedArray = (BigArray *)calloc(1, sizeof(BigArray));
            if (allocatedArray == NULL)
                return NULL;

            if (ffd_atomic_compare_exchange(&currentHeader->nextArray, &array, allocatedArray,
                                             FFD_ATOMIC_RELEASE, FFD_ATOMIC_ACQUIRE)) {
                /* success */
                array = allocatedArray;
            } else {
                /* failed, the atomic updated 'array' */
                free(allocatedArray);
            }
        }

        currentHeader = &array->header;
        info = tryAllocateInSection(currentHeader, array->entries, sizeofarray(array->entries));
    }

    *header = currentHeader;
    return info;
}

static int tryReaping(pid_t pid, siginfo_t *info)
{
    /* reap the child */
#ifdef HAVE_WAITID
    // we have waitid(2), which fills in siginfo_t for us
    info->si_pid = 0;
    return waitid(P_PID, pid, info, WEXITED | WNOHANG) == 0 && info->si_pid == pid;
#else
    int status;
    if (waitpid(pid, &status, WNOHANG) <= 0)
        return 0;     // child did not change state

    info->si_signo = SIGCHLD;
    info->si_utime = 0;
    info->si_stime = 0;
    info->si_pid = pid;
    if (WIFEXITED(status)) {
        info->si_code = CLD_EXITED;
        info->si_status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        info->si_code = CLD_KILLED;
#  ifdef WCOREDUMP
        if (WCOREDUMP(status))
            info->si_code = CLD_DUMPED;
#  endif
        info->si_status = WTERMSIG(status);
    }

    return 1;
#endif
}

static void freeInfo(Header *header, ProcessInfo *entry)
{
    entry->deathPipe = -1;
    entry->pid = 0;

    ffd_atomic_add_fetch(&header->busyCount, -1, FFD_ATOMIC_RELEASE);
    assert(header->busyCount >= 0);
}

static void notifyAndFreeInfo(Header *header, ProcessInfo *entry, siginfo_t *info)
{
    ssize_t ret;
    EINTR_LOOP(ret, write(entry->deathPipe, info, sizeof(*info)));
    EINTR_LOOP(ret, close(entry->deathPipe));

    freeInfo(header, entry);
}

static void sigchld_handler(int signum)
{
    /*
     * This is a signal handler, so we need to be careful about which functions
     * we can call. See the full, official listing in the POSIX.1-2008
     * specification at:
     *   http://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_04_03
     *
     */

    if (ffd_atomic_load(&forkfd_status, FFD_ATOMIC_RELAXED) == 1) {
        /* is this one of our children? */
        BigArray *array;
        siginfo_t info;
        int i;

        for (i = 0; i < (int)sizeofarray(children.entries); ++i) {
            int pid = ffd_atomic_load(&children.entries[i].pid, FFD_ATOMIC_ACQUIRE);
            if (pid > 0 && tryReaping(pid, &info)) {
                /* this is our child, send notification and free up this entry */
                notifyAndFreeInfo(&children.header, &children.entries[i], &info);
            }
        }

        /* try the arrays */
        array = ffd_atomic_load(&children.header.nextArray, FFD_ATOMIC_ACQUIRE);
        while (array != NULL) {
            for (i = 0; i < (int)sizeofarray(array->entries); ++i) {
                int pid = ffd_atomic_load(&array->entries[i].pid, FFD_ATOMIC_ACQUIRE);
                if (pid > 0 && tryReaping(pid, &info)) {
                    /* this is our child, send notification and free up this entry */
                    notifyAndFreeInfo(&array->header, &array->entries[i], &info);
                }
            }

            array = ffd_atomic_load(&array->header.nextArray, FFD_ATOMIC_ACQUIRE);
        }
    }

    if (old_sigaction.sa_handler != SIG_IGN && old_sigaction.sa_handler != SIG_DFL)
        old_sigaction.sa_handler(signum);
}

static void forkfd_initialize()
{
    /* install our signal handler */
    struct sigaction action;
    memset(&action, 0, sizeof action);
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_NOCLDSTOP;
    action.sa_handler = sigchld_handler;

    /* ### RACE CONDITION
     * The sigaction function does a memcpy from an internal buffer
     * to old_sigaction, which we use in the SIGCHLD handler. If a
     * SIGCHLD is delivered before or during that memcpy, the handler will
     * see an inconsistent state.
     *
     * There is no solution. pthread_sigmask doesn't work here because the
     * signal could be delivered to another thread.
     */
    sigaction(SIGCHLD, &action, &old_sigaction);

#ifndef __GNUC__
    atexit(cleanup);
#endif

    ffd_atomic_store(&forkfd_status, 1, FFD_ATOMIC_RELAXED);
}

#ifdef __GNUC__
__attribute((destructor, unused)) static void cleanup();
#endif

static void cleanup()
{
    BigArray *array;
    /* This function is not thread-safe!
     * It must only be called when the process is shutting down.
     * At shutdown, we expect no one to be calling forkfd(), so we don't
     * need to be thread-safe with what is done there.
     *
     * But SIGCHLD might be delivered to any thread, including this one.
     * There's no way to prevent that. The correct solution would be to
     * cooperatively delete. We don't do that.
     */
    if (ffd_atomic_load(&forkfd_status, FFD_ATOMIC_RELAXED) == 0)
        return;

    /* notify the handler that we're no longer in operation */
    ffd_atomic_store(&forkfd_status, 0, FFD_ATOMIC_RELAXED);

    /* free any arrays we might have */
    array = children.header.nextArray;
    while (array != NULL) {
        BigArray *next = array->header.nextArray;
        free(array);
        array = next;
    }
}

static int create_pipe(int filedes[], int flags)
{
    int ret;
#ifdef HAVE_PIPE2
    /* use pipe2(2) whenever possible, since it can thread-safely create a
     * cloexec pair of pipes. Without it, we have a race condition setting
     * FD_CLOEXEC
     */
    ret = pipe2(filedes, O_CLOEXEC);
    if (ret == -1)
        return ret;

    if ((flags & FFD_CLOEXEC) == 0)
        fcntl(filedes[0], F_SETFD, 0);
#else
    ret = pipe(filedes);
    if (ret == -1)
        return ret;

    fcntl(filedes[1], F_SETFD, FD_CLOEXEC);
    if (flags & FFD_CLOEXEC)
        fcntl(filedes[0], F_SETFD, FD_CLOEXEC);
#endif
    if (flags & FFD_NONBLOCK)
        fcntl(filedes[0], F_SETFL, fcntl(filedes[0], F_GETFL) | O_NONBLOCK);
    return ret;
}

/**
 * @brief forkfd returns a file descriptor representing a child process
 * @return a file descriptor, or -1 in case of failure
 *
 * forkfd() creates a file descriptor that can be used to be notified of when a
 * child process exits. This file descriptor can be monitored using select(2),
 * poll(2) or similar mechanisms.
 *
 * The @a flags parameter can contain the following values ORed to change the
 * behaviour of forkfd():
 *
 * @li @c FFD_NONBLOCK Set the O_NONBLOCK file status flag on the new open file
 * descriptor. Using this flag saves extra calls to fnctl(2) to achieve the same
 * result.
 *
 * @li @c FFD_CLOEXEC Set the close-on-exec (FD_CLOEXEC) flag on the new file
 * descriptor. You probably want to set this flag, since forkfd() does not work
 * if the original parent process dies.
 *
 * The file descriptor returned by forkfd() supports the following operations:
 *
 * @li read(2) When the child process exits, then the buffer supplied to
 * read(2) is used to return information about the status of the child in the
 * form of one @c siginfo_t structure. The buffer must be at least
 * sizeof(siginfo_t) bytes. The return value of read(2) is the total number of
 * bytes read.
 *
 * @li poll(2), select(2) (and similar) The file descriptor is readable (the
 * select(2) readfds argument; the poll(2) POLLIN flag) if the child has exited
 * or signalled via SIGCHLD.
 *
 * @li close(2) When the file descriptor is no longer required it should be closed.
 */
int forkfd(int flags, pid_t *ppid)
{
    Header *header;
    ProcessInfo *info;
    pid_t pid;
    int fd = -1;
    int death_pipe[2];
    int sync_pipe[2];
    int ret;
#ifdef __linux__
    int efd;
#endif

    (void) pthread_once(&forkfd_initialization, forkfd_initialize);

    info = allocateInfo(&header);
    if (info == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /* create the pipes before we fork */
    if (create_pipe(death_pipe, flags) == -1)
        goto err_free; /* failed to create the pipes, pass errno */

#ifdef HAVE_EVENTFD
    /* try using an eventfd, which consumes less resources */
    efd = eventfd(0, EFD_CLOEXEC);
    if (efd == -1)
#endif
    {
        /* try a pipe */
        if (create_pipe(sync_pipe, O_CLOEXEC) == -1) {
            /* failed both at eventfd and pipe; fail and pass errno */
            goto err_close;
        }
    }

    /* now fork */
    pid = fork();
    if (pid == -1)
        goto err_close2; /* failed to fork, pass errno */
    if (ppid)
        *ppid = pid;

    /*
     * We need to store the child's PID in the info structure, so
     * the SIGCHLD handler knows that this child is present and it
     * knows the writing end of the pipe to pass information on.
     * However, the child process could exit before we stored the
     * information (or the handler could run for other children exiting).
     * We prevent that from happening by blocking the child process in
     * a read(2) until we're finished storing the information.
     */
    if (pid == 0) {
        /* this is the child process */
        /* first, wait for the all clear */
#ifdef HAVE_EVENTFD
        if (efd != -1) {
            eventfd_t val64;
            EINTR_LOOP(ret, eventfd_read(efd, &val64));
            EINTR_LOOP(ret, close(efd));
        } else
#endif
        {
            char c;
            EINTR_LOOP(ret, close(sync_pipe[1]));
            EINTR_LOOP(ret, read(sync_pipe[0], &c, sizeof c));
            EINTR_LOOP(ret, close(sync_pipe[0]));
        }

        /* now close the pipes and return to the caller */
        EINTR_LOOP(ret, close(death_pipe[0]));
        EINTR_LOOP(ret, close(death_pipe[1]));
        fd = FFD_CHILD_PROCESS;
    } else {
        /* parent process */
        info->deathPipe = death_pipe[1];
        fd = death_pipe[0];
        ffd_atomic_store(&info->pid, pid, FFD_ATOMIC_RELEASE);

        /* release the child */
#ifdef HAVE_EVENTFD
        if (efd != -1) {
            eventfd_t val64 = 42;
            EINTR_LOOP(ret, eventfd_write(efd, val64));
            EINTR_LOOP(ret, close(efd));
        } else
#endif
        {
            /*
             * Usually, closing would be enough to make read(2) return and the child process
             * continue. We need to write here: another thread could be calling forkfd at the
             * same time, which means auxpipe[1] might be open in another child process.
             */
            EINTR_LOOP(ret, close(sync_pipe[0]));
            EINTR_LOOP(ret, write(sync_pipe[1], "", 1));
            EINTR_LOOP(ret, close(sync_pipe[1]));
        }
    }

    return fd;

err_close2:
#ifdef HAVE_EVENTFD
    if (efd != -1) {
        EINTR_LOOP(ret, close(efd));
    } else
#endif
    {
        EINTR_LOOP(ret, close(sync_pipe[0]));
        EINTR_LOOP(ret, close(sync_pipe[1]));
    }
err_close:
    EINTR_LOOP(ret, close(death_pipe[0]));
    EINTR_LOOP(ret, close(death_pipe[1]));
err_free:
    /* free the info pointer */
    freeInfo(header, info);
    return -1;
}
