// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFILESYSTEMITERATOR_P_H
#define QFILESYSTEMITERATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#ifndef QT_NO_FILESYSTEMITERATOR

#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qstringlist.h>

#include <QtCore/private/qfilesystementry_p.h>
#include <QtCore/private/qfilesystemmetadata_p.h>

// Platform-specific headers
#if !defined(Q_OS_WIN)
#include <QtCore/qscopedpointer.h>
#endif

QT_BEGIN_NAMESPACE

class QFileSystemIterator
{
public:
    QFileSystemIterator(const QFileSystemEntry &entry, QDir::Filters filters,
            const QStringList &nameFilters, QDirIterator::IteratorFlags flags
                = QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
    ~QFileSystemIterator();

    bool advance(QFileSystemEntry &fileEntry, QFileSystemMetaData &metaData);

private:
    QFileSystemEntry::NativePath nativePath;

    // Platform-specific data
#if defined(Q_OS_WIN)
    QString dirPath;
    HANDLE findFileHandle;
    QStringList uncShares;
    bool uncFallback;
    int uncShareIndex;
    bool onlyDirs;
#else
    QT_DIR *dir;
    QT_DIRENT *dirEntry;
    int lastError;
#endif

    Q_DISABLE_COPY_MOVE(QFileSystemIterator)
};

QT_END_NAMESPACE

#endif // QT_NO_FILESYSTEMITERATOR

#endif // include guard
