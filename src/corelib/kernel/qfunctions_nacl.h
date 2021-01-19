/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QFUNCTIONS_NACL_H
#define QFUNCTIONS_NACL_H

#include <QtCore/qglobal.h>

#ifdef Q_OS_NACL

#include <sys/types.h>

// pthread
#include <pthread.h>
#define PTHREAD_CANCEL_DISABLE 1
#define PTHREAD_CANCEL_ENABLE 2
#define PTHREAD_INHERIT_SCHED 3

QT_BEGIN_NAMESPACE


extern "C" {

void pthread_cleanup_push(void (*handler)(void *), void *arg);
void pthread_cleanup_pop(int execute);

int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
void pthread_testcancel(void);
int pthread_cancel(pthread_t thread);

int pthread_attr_setinheritsched(pthread_attr_t *attr,
    int inheritsched);
int pthread_attr_getinheritsched(const pthread_attr_t *attr,
    int *inheritsched);

// event dispatcher, select
//struct fd_set;
//struct timeval;
int fcntl(int fildes, int cmd, ...);
int sigaction(int sig, const struct sigaction * act, struct sigaction * oact);

typedef long off64_t;
off64_t ftello64(void *stream);
off64_t lseek64(int fildes, off_t offset, int whence);
int open64(const char *path, int oflag, ...);

}

int select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * errorfds, struct timeval * timeout);

QT_END_NAMESPACE

#endif //Q_OS_NACL

#endif //QFUNCTIONS_NACL_H
