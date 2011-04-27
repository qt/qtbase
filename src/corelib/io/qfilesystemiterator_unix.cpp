/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
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

#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_CYGWIN)
        // ### Race condition; we should use fpathconf and dirfd().
        size_t maxPathName = ::pathconf(nativePath.constData(), _PC_NAME_MAX);
        if (maxPathName == size_t(-1))
            maxPathName = FILENAME_MAX;
        maxPathName += sizeof(QT_DIRENT) + 1;

        QT_DIRENT *p = reinterpret_cast<QT_DIRENT*>(::malloc(maxPathName));
        Q_CHECK_PTR(p);

        mt_file.reset(p);
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

#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_CYGWIN)
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
