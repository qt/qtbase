// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qresource.h"
#include "qresource_iterator_p.h"

#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

QResourceFileEngineIterator::QResourceFileEngineIterator(const QString &path, QDir::Filters filters,
                                                         const QStringList &filterNames)
    : QAbstractFileEngineIterator(path, filters, filterNames),
      index(-1)
{
}

QResourceFileEngineIterator::QResourceFileEngineIterator(const QString &path,
                                                         QDirListing::IteratorFlags filters,
                                                         const QStringList &filterNames)
    : QAbstractFileEngineIterator(path, filters, filterNames),
      index(-1)
{
}

QResourceFileEngineIterator::~QResourceFileEngineIterator()
{
}

bool QResourceFileEngineIterator::advance()
{
    if (index == -1) {
        // Lazy initialization of the iterator
        QResource resource(path());
        if (!resource.isValid())
            return false;

        // Initialize and move to the first entry.
        entries = resource.children();
        if (entries.isEmpty())
            return false;

        index = 0;
        return true;
    }

    if (index < entries.size() - 1) {
        ++index;
        return true;
    }

    return false;
}

QString QResourceFileEngineIterator::currentFileName() const
{
    if (index < 0 || index > entries.size())
        return QString();
    return entries.at(index);
}

QFileInfo QResourceFileEngineIterator::currentFileInfo() const
{
    m_fileInfo = QFileInfo(currentFilePath());
    return m_fileInfo;
}

QT_END_NAMESPACE
