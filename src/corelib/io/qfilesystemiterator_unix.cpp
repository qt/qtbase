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

QFileSystemIterator::QFileSystemIterator(const QFileSystemEntry &entry, QDir::Filters filters,
                                         const QStringList &nameFilters, QDirIterator::IteratorFlags flags)
    : nativePath(entry.nativeFilePath())
    , dir(nullptr)
    , dirEntry(nullptr)
    , lastError(0)
{
    Q_UNUSED(filters);
    Q_UNUSED(nameFilters);
    Q_UNUSED(flags);

    if ((dir = QT_OPENDIR(nativePath.constData())) == nullptr) {
        lastError = errno;
    } else {
        if (!nativePath.endsWith('/'))
            nativePath.append('/');
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

    for (;;) {
        dirEntry = QT_READDIR(dir);

        if (dirEntry) {
            qsizetype len = strlen(dirEntry->d_name);
            if (checkNameDecodable(dirEntry->d_name, len)) {
                fileEntry = QFileSystemEntry(nativePath + QByteArray(dirEntry->d_name, len), QFileSystemEntry::FromNativePath());
                metaData.fillFromDirEnt(*dirEntry);
                return true;
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
