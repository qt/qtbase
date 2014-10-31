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

#include "qfunctions_nacl.h"
#include <pthread.h>
#include <qglobal.h>

/*
    The purpose of this file is to stub out certain functions
    that are not provided by the Native Client SDK. This is
    done as an alterative to sprinkling the Qt sources with
    NACL ifdefs.

    There are two main classes of functions:

    - Functions that are called but can have no effect:
    For these we simply give an empty implementation

    - Functions that are referenced in the source code, but
    is not/must not be called at run-time:
    These we either leave undefined or implement with a
    qFatal.

    This is a work in progress.
*/

extern "C" {

//void pthread_cleanup_push(void (*)(void *), void *)
//{
//
//}

//void pthread_cleanup_pop(int)
//{
//
//}

#ifdef Q_OS_NACL_NEWLIB
int pthread_setcancelstate(int, int *)
{
    return 0;
}

int pthread_setcanceltype(int, int *)
{
    return 0;
}

void pthread_testcancel(void)
{

}


int pthread_cancel(pthread_t)
{
    return 0;
}

int pthread_attr_setinheritsched(pthread_attr_t *,int)
{
    return 0;
}


int pthread_attr_getinheritsched(const pthread_attr_t *, int *)
{
    return 0;
}

int pthread_condattr_init(pthread_condattr_t *)
{
    return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
    return 0;
}

pid_t getpid(void)
{
    return 0;
}

uid_t geteuid(void)
{
    return 0;
}

int gethostname(char *name, size_t namelen)
{
    return -1;
}
#endif


// event dispatcher, select
//struct fd_set;
//struct timeval;

int fcntl(int, int, ...)
{
    return 0;
}

int sigaction(int, const struct sigaction *, struct sigaction *)
{
    return 0;
}

int open(const char *, int, ...)
{
    return 0;
}

int open64(const char *, int, ...)
{
    return 0;
}

long pathconf(const char *, int)
{
    return 0;
}

int access(const char *, int)
{
    return 0;
}

typedef long off64_t;
off64_t ftello64(void *)
{
    qFatal("ftello64 called");
    return 0;
}

off64_t lseek64(int, off_t, int)
{
    qFatal("lseek64 called");
    return 0;
}

#ifdef Q_OS_NACL_NEWLIB
char * getenv(const char *)
{
    return 0;
}

int putenv(char *)
{
    return 0;
}

int setenv(const char *name, const char *value, int overwrite)
{
    return 0;
}

int unsetenv(const char *name)
{
    return 0;
}
#endif

} // Extern C

int select(int, fd_set *, fd_set *, fd_set *, struct timeval *)
{
    return 0;
}

int pselect(int nfds, fd_set * readfds, fd_set * writefds, fd_set * errorfds, const struct timespec * timeout, const sigset_t * sigmask)
{
    return 0;
}

#ifdef Q_OS_NACL
// The Pepper networking API requires access to a global instance object.
// Following the Qt architechture, this object is owned by the platform
// plugin. Add a global pointer here to give QtNetwork access. The pointer
// is set during pepper platform plugin initialization.
Q_CORE_EXPORT void *qtPepperInstance = 0;
#endif
