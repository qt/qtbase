/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
******************************************************************************/

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
