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
#include "qfunctions_vxworks.h"

#define QT_USE_XOPEN_LFS_EXTENSIONS
#include "../../common/posix/qplatformdefs.h"

#undef QT_LSTAT
#undef QT_MKDIR
#undef QT_READ
#undef QT_WRITE
#undef QT_SOCKLEN_T
#undef QT_SOCKET_CONNECT

#define QT_LSTAT                QT_STAT
#define QT_MKDIR(dir, perm)     ::mkdir(dir)

#define QT_READ(fd, buf, len)   ::read(fd, (char*) buf, len)
#define QT_WRITE(fd, buf, len)  ::write(fd, (char*) buf, len)

// there IS a socklen_t in sys/socket.h (unsigned int),
// but sockLib.h uses int in all function declaration...
#define QT_SOCKLEN_T            int
#define QT_SOCKET_CONNECT(sd, to, tolen) \
                                ::connect(sd, (struct sockaddr *) to, tolen)

#define QT_SNPRINTF             ::snprintf
#define QT_VSNPRINTF            ::vsnprintf

#endif // QPLATFORMDEFS_H
