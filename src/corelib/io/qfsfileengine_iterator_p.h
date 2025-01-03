// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFSFILEENGINE_ITERATOR_P_H
#define QFSFILEENGINE_ITERATOR_P_H

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

#include "private/qabstractfileengine_p.h"
#include "qfilesystemiterator_p.h"
#include "qdir.h"

#ifndef QT_NO_FILESYSTEMITERATOR

QT_BEGIN_NAMESPACE

class QFSFileEngineIteratorPrivate;
class QFSFileEngineIteratorPlatformSpecificData;

class QFSFileEngineIterator : public QAbstractFileEngineIterator
{
public:
    QFSFileEngineIterator(QDir::Filters filters, const QStringList &filterNames);
    ~QFSFileEngineIterator();

    QString next() override;
    bool hasNext() const override;

    QString currentFileName() const override;
    QFileInfo currentFileInfo() const override;

private:
    void advance() const;
    mutable QScopedPointer<QFileSystemIterator> nativeIterator;
    mutable QFileInfo currentInfo;
    mutable QFileInfo nextInfo;
    mutable bool done;
};

QT_END_NAMESPACE

#endif // QT_NO_FILESYSTEMITERATOR

#endif // QFSFILEENGINE_ITERATOR_P_H
