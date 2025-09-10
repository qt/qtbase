// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qfsfileengine_iterator_p.h"
#include "qfileinfo_p.h"
#include "qvariant.h"

#ifndef QT_NO_FILESYSTEMITERATOR

QT_BEGIN_NAMESPACE

QFSFileEngineIterator::QFSFileEngineIterator(const QString &path, QDir::Filters filters,
                                             const QStringList &filterNames)
    : QAbstractFileEngineIterator(path, filters, filterNames),
      nativeIterator(new QFileSystemIterator(QFileSystemEntry(path), filters))
{
}

QFSFileEngineIterator::QFSFileEngineIterator(const QString &path, QDirListing::IteratorFlags filters,
                                             const QStringList &filterNames)
    : QAbstractFileEngineIterator(path, filters, filterNames),
      nativeIterator(new QFileSystemIterator(QFileSystemEntry(path), filters))
{
}

QFSFileEngineIterator::~QFSFileEngineIterator()
{
}

bool QFSFileEngineIterator::advance()
{
    if (!nativeIterator)
        return false;

    QFileSystemEntry entry;
    QFileSystemMetaData data;
    if (nativeIterator->advance(entry, data)) {
        m_fileInfo = QFileInfo(new QFileInfoPrivate(entry, data));
        return true;
    } else {
        nativeIterator.reset();
        return false;
    }
}

QString QFSFileEngineIterator::currentFileName() const
{
    return m_fileInfo.fileName();
}

QFileInfo QFSFileEngineIterator::currentFileInfo() const
{
    return m_fileInfo;
}

QT_END_NAMESPACE

#endif // QT_NO_FILESYSTEMITERATOR
