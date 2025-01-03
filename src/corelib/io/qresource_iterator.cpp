// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qresource.h"
#include "qresource_iterator_p.h"

#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

QResourceFileEngineIterator::QResourceFileEngineIterator(QDir::Filters filters,
                                                         const QStringList &filterNames)
    : QAbstractFileEngineIterator(filters, filterNames), index(-1)
{
}

QResourceFileEngineIterator::~QResourceFileEngineIterator()
{
}

QString QResourceFileEngineIterator::next()
{
    if (!hasNext())
        return QString();
    ++index;
    return currentFilePath();
}

bool QResourceFileEngineIterator::hasNext() const
{
    if (index == -1) {
        // Lazy initialization of the iterator
        QResource resource(path());
        if (!resource.isValid())
            return false;

        // Initialize and move to the next entry.
        entries = resource.children();
        index = 0;
    }

    return index < entries.size();
}

QString QResourceFileEngineIterator::currentFileName() const
{
    if (index <= 0 || index > entries.size())
        return QString();
    return entries.at(index - 1);
}

QT_END_NAMESPACE
