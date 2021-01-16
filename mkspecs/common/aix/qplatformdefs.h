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

#ifndef Q_AIX_QPLATFORMDEFS_H
#define Q_AIX_QPLATFORMDEFS_H

// Get Qt defines/settings

#include "qglobal.h"

// Set any POSIX/XOPEN defines at the top of this file to turn on specific APIs

#include <unistd.h>


// We are hot - unistd.h should have turned on the specific APIs we requested


// uncomment if you have problems with <sys/proc.h> because your gcc
// hasn't been built on exactly the same OS version your are using now.
// typedef int crid_t;
// typedef unsigned int class_id_t;
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <dlfcn.h>
#include <strings.h> // AIX X11 headers define FD_ZERO using bzero()

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>

// Only AIX 4.3 and better support 64-bit

#define QT_USE_XOPEN_LFS_EXTENSIONS
#include "../posix/qplatformdefs.h"

#undef QT_SOCKLEN_T

#ifdef _AIX43
// AIX 4.3 and better
#define QT_SOCKLEN_T            socklen_t
#elif _AIX42
// AIX 4.2
#define QT_SOCKLEN_T            size_t
#else
// AIX 4.1
#define QT_SOCKLEN_T            size_t
// override
#define QT_SOCKOPTLEN_T         int
#endif

#ifdef QT_LARGEFILE_SUPPORT
#undef QT_DIR
#undef QT_OPENDIR
#undef QT_CLOSEDIR

#define QT_DIR                  DIR64
#define QT_OPENDIR              ::opendir64
#define QT_CLOSEDIR             ::closedir64
#endif

#if defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE-0 >= 500)
// AIX 4.3 and better
#define QT_SNPRINTF             ::snprintf
#define QT_VSNPRINTF            ::vsnprintf
#endif

#endif // include guard
