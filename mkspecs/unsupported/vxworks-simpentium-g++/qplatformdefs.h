/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake spec of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
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
