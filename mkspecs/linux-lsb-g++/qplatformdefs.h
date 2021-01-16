/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#ifndef QPLATFORMDEFS_H
#define QPLATFORMDEFS_H

// Get Qt defines/settings

#include "qglobal.h"

// Set any POSIX/XOPEN defines at the top of this file to turn on specific APIs

// 1) need to reset default environment if _BSD_SOURCE is defined
// 2) need to specify POSIX thread interfaces explicitly in glibc 2.0
// 3) it seems older glibc need this to include the X/Open stuff
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include <unistd.h>


// We are hot - unistd.h should have turned on the specific APIs we requested

#include <features.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <dlfcn.h>
#include <time.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>

// LSB 3.1 defines htonl and friends here
#include <arpa/inet.h>

#define QT_USE_XOPEN_LFS_EXTENSIONS
#include "../common/posix/qplatformdefs.h"

#undef QT_OPEN_LARGEFILE
#undef QT_SOCKLEN_T

#define QT_OPEN_LARGEFILE       0

#if defined(__GLIBC__) && (__GLIBC__ >= 2)
#define QT_SOCKLEN_T            socklen_t
#else
#define QT_SOCKLEN_T            int
#endif

#ifndef SIOCGIFBRDADDR
#  define SIOCGIFBRDADDR 0x8919
#endif

#if defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE >= 500)
#define QT_SNPRINTF             ::snprintf
#define QT_VSNPRINTF            ::vsnprintf
#endif

#endif // QPLATFORMDEFS_H
