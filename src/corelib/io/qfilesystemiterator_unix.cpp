// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"
#include "qfilesystemiterator_p.h"

#include <private/qstringconverter_p.h>

#ifndef QT_NO_FILESYSTEMITERATOR

#include <memory>

#include <stdlib.h>
#include <errno.h>

QT_BEGIN_NAMESPACE

static bool checkNameDecodable(const char *d_name, qsizetype len)
{
    // This function is called in a loop from advance() below, but the loop is
    // usually run only once.

    return QUtf8::isValidUtf8(QByteArrayView(d_name, len)).isValidUtf8;
}

/*
    Native filesystem iterator, which uses ::opendir()/readdir()/dirent from the system
    libraries to iterate over the directory represented by \a entry.
*/
QFileSystemIterator::QFileSystemIterator(const QFileSystemEntry &entry, QDir::Filters filters,
                                         const QStringList &nameFilters, QDirIterator::IteratorFlags flags)
    : nativePath(entry.nativeFilePath())
{
    Q_UNUSED(filters);
    Q_UNUSED(nameFilters);
    Q_UNUSED(flags);

    dir.reset(QT_OPENDIR(entry.nativeFilePath().constData()));
    if (!dir) {
        lastError = errno;
    } else {
        if (!nativePath.endsWith('/'))
            nativePath.append('/');
    }
}

QFileSystemIterator::~QFileSystemIterator() = default;

bool QFileSystemIterator::advance(QFileSystemEntry &fileEntry, QFileSystemMetaData &metaData)
{
    if (!dir)
        return false;

    for (;;) {
        // From readdir man page:
        // If the end of the directory stream is reached, NULL is returned and errno is
        // not changed. If an error occurs, NULL is returned and errno is set to indicate
        // the error. To distinguish end of stream from an error, set errno to zero before
        // calling readdir() and then check the value of errno if NULL is returned.
        errno = 0;
        dirEntry = QT_READDIR(dir.get());

        if (dirEntry) {
            qsizetype len = strlen(dirEntry->d_name);
            if (checkNameDecodable(dirEntry->d_name, len)) {
                fileEntry = QFileSystemEntry(nativePath + QByteArray(dirEntry->d_name, len), QFileSystemEntry::FromNativePath());
                metaData.fillFromDirEnt(*dirEntry);
                return true;
            } else {
                errno = EILSEQ; // Invalid or incomplete multibyte or wide character
            }
        } else {
            break;
        }
    }

    lastError = errno;
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_FILESYSTEMITERATOR
