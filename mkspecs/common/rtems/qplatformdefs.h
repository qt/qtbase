/****************************************************************************
**
** Copyright (C) 2021 The Qt Company. All rights reserved.
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
