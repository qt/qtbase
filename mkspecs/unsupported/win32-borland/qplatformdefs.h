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

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

// Get Qt defines/settings

#include "qglobal.h"
#define Q_FS_FAT

#define _POSIX_
#include <limits.h>
#undef _POSIX_

#include <tchar.h>
#include <io.h>
#include <direct.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <dos.h>
#include <stdlib.h>
#include <search.h>
#include <windows.h>

#if __BORLANDC__ >= 0x550
// Borland Builder 6

#ifdef QT_LARGEFILE_SUPPORT
#  define QT_STATBUF		struct stati64		// non-ANSI defs
#  define QT_STATBUF4TSTAT	struct stati64		// non-ANSI defs
#  define QT_STAT		::_stati64
#  define QT_FSTAT		::fstati64
#  define QT_LSEEK		::_lseeki64
#  define QT_TSTAT		::_tstati64
#else
#  define QT_STATBUF		struct stat		// non-ANSI defs
#  define QT_STATBUF4TSTAT	struct _stat		// non-ANSI defs
#  define QT_STAT		::stat
#  define QT_FSTAT		::fstat
#  define QT_LSEEK		::_lseek
#  define QT_TSTAT		::_tstat
#endif

#define QT_STAT_REG		_S_IFREG
#define QT_STAT_DIR		_S_IFDIR
#define QT_STAT_MASK		_S_IFMT

#if defined(_S_IFLNK)
#  define QT_STAT_LNK		_S_IFLNK
#endif

#include "../common/c89/qplatformdefs.h"

#ifdef QT_LARGEFILE_SUPPORT
#undef QT_FSEEK
#undef QT_FTELL
#undef QT_OFF_T

#define QT_FSEEK                ::_fseeki64
#define QT_FTELL                ::_ftelli64
#define QT_OFF_T                __int64
#endif

#define QT_FILENO		_fileno
#define QT_OPEN			::open
#define QT_CLOSE		::_close

#define QT_READ			::_read
#define QT_WRITE		::_write
#define QT_ACCESS		::_access
#define QT_GETCWD		::_getcwd
#define QT_CHDIR		::chdir
#define QT_MKDIR		::_mkdir
#define QT_RMDIR		::_rmdir
#define QT_OPEN_LARGEFILE       O_LARGEFILE
#define QT_OPEN_RDONLY		_O_RDONLY
#define QT_OPEN_WRONLY		_O_WRONLY
#define QT_OPEN_RDWR		_O_RDWR
#define QT_OPEN_CREAT		_O_CREAT
#define QT_OPEN_TRUNC		_O_TRUNC
#define QT_OPEN_APPEND		_O_APPEND

#if defined(O_TEXT)
#  define QT_OPEN_TEXT		_O_TEXT
#  define QT_OPEN_BINARY	_O_BINARY
#endif

#else
// Borland Builder 5

#ifdef QT_LARGEFILE_SUPPORT
#  define QT_STATBUF		struct stati64		// non-ANSI defs
#  define QT_STATBUF4TSTAT	struct stati64		// non-ANSI defs
#  define QT_STAT		::stati64
#  define QT_FSTAT		::fstati64
#  define QT_LSEEK		::lseeki64
#  define QT_TSTAT		::tstati64
#else
#  define QT_STATBUF		struct stat		// non-ANSI defs
#  define QT_STATBUF4TSTAT	struct stat		// non-ANSI defs
#  define QT_STAT		::stat
#  define QT_FSTAT		::fstat
#  define QT_LSEEK		::lseek
#  define QT_TSTAT		::tstat
#endif

#define QT_STAT_REG		S_IFREG
#define QT_STAT_DIR		S_IFDIR
#define QT_STAT_MASK		S_IFMT

#if defined(S_IFLNK)
#  define QT_STAT_LNK		S_IFLNK
#endif

#define QT_FILENO		fileno
#define QT_OPEN			::open
#define QT_CLOSE		::close

#define QT_READ			::read
#define QT_WRITE		::write
#define QT_ACCESS		::access

#if defined(Q_OS_OS2EMX)
    // This is documented in the un*x to OS/2-EMX Porting FAQ:
    // http://homepages.tu-darmstadt.de/~st002279/os2/porting.html
#  define QT_GETCWD		::_getcwd2
#  define QT_CHDIR		::_chdir2
#else
#  define QT_GETCWD		::getcwd
#  define QT_CHDIR		::chdir
#endif

#define QT_MKDIR		::mkdir
#define QT_RMDIR		::rmdir
#define QT_OPEN_LARGEFILE       O_LARGEFILE
#define QT_OPEN_RDONLY		O_RDONLY
#define QT_OPEN_WRONLY		O_WRONLY
#define QT_OPEN_RDWR		O_RDWR
#define QT_OPEN_CREAT		O_CREAT
#define QT_OPEN_TRUNC		O_TRUNC
#define QT_OPEN_APPEND		O_APPEND

#if defined(O_TEXT)
#  define QT_OPEN_TEXT		O_TEXT
#  define QT_OPEN_BINARY	O_BINARY
#endif

#endif // __BORLANDC__ >= 0x550

// Borland Builder 5 and 6

#define QT_SIGNAL_ARGS		int

#define QT_VSNPRINTF		::_vsnprintf
#define QT_SNPRINTF		::_snprintf

# define F_OK	0
# define X_OK	1
# define W_OK	2
# define R_OK	4


#endif // QPLATFORMDEFS_H
