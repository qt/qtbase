// Copyright (C) 2018 The Qt Company. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef Q_RTEMS_PLATFORMDEFS_H
#define Q_RTEMS_PLATFORMDEFS_H

// Get Qt defines/settings

#include "qglobal.h"

#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <pwd.h>
#include <grp.h>

#define __LINUX_ERRNO_EXTENSIONS__
#include <errno.h>

#include "../posix/qplatformdefs.h"

#ifdef __STRICT_ANSI__
#undef __STRICT_ANSI__
#endif

#undef QT_OPEN_LARGEFILE
#define QT_OPEN_LARGEFILE 0

#endif // Q_RTEMS_PLATFORMDEFS_H
