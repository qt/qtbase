/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformdefs.h"
#include "qfilesystemiterator_p.h"

#ifndef QT_NO_FILESYSTEMITERATOR

#include <stdlib.h>
#include <errno.h>

QT_BEGIN_NAMESPACE

QFileSystemIterator::QFileSystemIterator(const QFileSystemEntry &entry, QDir::Filters filters,
                                         const QStringList &nameFilters, QDirIterator::IteratorFlags flags)
    : nativePath(entry.nativeFilePath())
    , dir(0)
    , dirEntry(0)
#if defined(Q_OS_QNX) && defined(__EXT_QNX__READDIR_R)
    , direntSize(0)
#endif
    , lastError(0)
{
    Q_UNUSED(filters)
    Q_UNUSED(nameFilters)
    Q_UNUSED(flags)

    if ((dir = QT_OPENDIR(nativePath.constData())) == 0) {
        lastError = errno;
    } else {

        if (!nativePath.endsWith('/'))
            nativePath.append('/');

#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_CYGWIN) || defined(QT_EXT_QNX_READDIR_R)
        // ### Race condition; we should use fpathconf and dirfd().
        size_t maxPathName = ::pathconf(nativePath.constData(), _PC_NAME_MAX);
        if (maxPathName == size_t(-1))
            maxPathName = FILENAME_MAX;
        maxPathName += sizeof(QT_DIRENT) + 1;

        QT_DIRENT *p = reinterpret_cast<QT_DIRENT*>(::malloc(maxPathName));
        Q_CHECK_PTR(p);

        mt_file.reset(p);
#if defined(QT_EXT_QNX_READDIR_R)
        direntSize = maxPathName;

        // Include extra stat information in the readdir() call (d_stat member of
        // dirent_extra_stat). This is used in QFileSystemMetaData::fillFromDirEnt() to
        // avoid extra stat() calls when iterating over directories
        int flags = dircntl(dir, D_GETFLAG) |  D_FLAG_STAT | D_FLAG_FILTER;
        if (dircntl(dir, D_SETFLAG, flags) == -1)
            lastError = errno;
#endif
#endif
    }
}

QFileSystemIterator::~QFileSystemIterator()
{
    if (dir)
        QT_CLOSEDIR(dir);
}

bool QFileSystemIterator::advance(QFileSystemEntry &fileEntry, QFileSystemMetaData &metaData)
{
    if (!dir)
        return false;

#if defined(QT_EXT_QNX_READDIR_R)
    lastError = QT_EXT_QNX_READDIR_R(dir, mt_file.data(), &dirEntry, direntSize);
    if (lastError)
        return false;
#elif defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_CYGWIN)
    lastError = QT_READDIR_R(dir, mt_file.data(), &dirEntry);
    if (lastError)
        return false;
#else
    // ### add local lock to prevent breaking reentrancy
    dirEntry = QT_READDIR(dir);
#endif // _POSIX_THREAD_SAFE_FUNCTIONS

    if (dirEntry) {
        fileEntry = QFileSystemEntry(nativePath + QByteArray(dirEntry->d_name), QFileSystemEntry::FromNativePath());
        metaData.fillFromDirEnt(*dirEntry);
        return true;
    }

    lastError = errno;
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_FILESYSTEMITERATOR
