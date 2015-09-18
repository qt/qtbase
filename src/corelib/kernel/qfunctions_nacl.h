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

#ifndef QFUNCTIONS_NACL_H
#define QFUNCTIONS_NACL_H

#include <QtCore/qglobal.h>

#ifdef Q_OS_NACL

#ifdef Q_OS_NACL_NEWLIB
#include <sys/nacl_syscalls.h>
#endif

#include <sys/types.h>

// pthread
#include <pthread.h>

#ifdef Q_OS_NACL_NEWLIB
#define PTHREAD_CANCEL_DISABLE 1
#define PTHREAD_CANCEL_ENABLE 2
#define PTHREAD_INHERIT_SCHED 3
#endif
QT_BEGIN_NAMESPACE

extern "C" {
//void pthread_cleanup_push(void (*handler)(void *), void *arg) __attribute__((weak));
//void pthread_cleanup_pop(int execute) __attribute__((weak));


#ifdef Q_OS_NACL_NEWLIB
int pthread_setcancelstate(int state, int *oldstate) __attribute__((weak)) __attribute__((weak));
int pthread_setcanceltype(int type, int *oldtype) __attribute__((weak)) __attribute__((weak));
void pthread_testcancel(void) __attribute__((weak)) __attribute__((weak));
int pthread_cancel(pthread_t thread) __attribute__((weak)) __attribute__((weak));

int pthread_attr_setinheritsched(pthread_attr_t *attr,
    int inheritsched) __attribute__((weak)) __attribute__((weak));
int pthread_attr_getinheritsched(const pthread_attr_t *attr,
    int *inheritsched) __attribute__((weak)) __attribute__((weak));

// No condition variable attributes on pnacl, and no dummy pthread_condattr_* either
int pthread_condattr_init(pthread_condattr_t *) __attribute__((weak)) __attribute__((weak));
int pthread_condattr_destroy(pthread_condattr_t *) __attribute__((weak)) __attribute__((weak));

pid_t getpid(void) __attribute__((weak)) __attribute__((weak));
uid_t geteuid(void) __attribute__((weak)) __attribute__((weak));
int gethostname(char *name, size_t namelen) __attribute__((weak)) __attribute__((weak));

// Several function declarations in the newlib headers are
// disabled with a '#ifndef __STRICT_ANSI__' flag.
// ### find a better way.
#ifndef Q_OS_PNACL

int	_EXFUN(fseeko, (FILE *, _off_t, int)) __attribute__((weak));
off_t	_EXFUN(ftello, ( FILE *)) __attribute__((weak));

int	_EXFUN(fileno, (FILE *)) __attribute__((weak));
int	_EXFUN(getw, (FILE *)) __attribute__((weak));
int	_EXFUN(pclose, (FILE *)) __attribute__((weak));
FILE *  _EXFUN(popen, (const char *, const char *)) __attribute__((weak));
int	_EXFUN(putw, (int, FILE *)) __attribute__((weak));
void    _EXFUN(setbuffer, (FILE *, char *, int)) __attribute__((weak));
int	_EXFUN(setlinebuf, (FILE *)) __attribute__((weak));
int	_EXFUN(getc_unlocked, (FILE *)) __attribute__((weak));
int	_EXFUN(getchar_unlocked, (void)) __attribute__((weak));
void	_EXFUN(flockfile, (FILE *)) __attribute__((weak));
int	_EXFUN(ftrylockfile, (FILE *)) __attribute__((weak));
void	_EXFUN(funlockfile, (FILE *)) __attribute__((weak));
int	_EXFUN(putc_unlocked, (int, FILE *)) __attribute__((weak));
int	_EXFUN(putchar_unlocked, (int)) __attribute__((weak));

char * _EXFUN(mkdtemp,(char *)) __attribute__((weak));
#endif
#endif

// event dispatcher, select
//struct fd_set;
//struct timeval;
int fcntl(int fildes, int cmd, ...) __attribute__((weak));
int sigaction(int sig, const struct sigaction * act, struct sigaction * oact) __attribute__((weak));

#ifndef Q_OS_NACL_EMSCRIPTEN
typedef long off64_t;
off64_t ftello64(void *stream) __attribute__((weak));
off64_t lseek64(int fildes, off_t offset, int whence) __attribute__((weak));
int open64(const char *path, int oflag, ...) __attribute__((weak));
#endif

long pathconf(const char *, int) __attribute__((weak));
//long int pathconf (__const char *__path, int __name) __THROW __nonnull ((1)) __attribute__((weak));

#ifdef Q_OS_NACL_NEWLIB
char * getenv(const char *name) __attribute__((weak));
int putenv(char *string) __attribute__((weak));
int setenv(const char *name, const char *value, int overwrite) __attribute__((weak));
int unsetenv(const char *name) __attribute__((weak));
#endif

}

int select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * errorfds, struct timeval * timeout) __attribute__((weak));
int pselect(int nfds, fd_set * readfds, fd_set * writefds, fd_set * errorfds, const struct timespec * timeout, const sigset_t * sigmask) __attribute__((weak));

QT_END_NAMESPACE

#endif //Q_OS_NACL

#endif //QFUNCTIONS_NACL_H
