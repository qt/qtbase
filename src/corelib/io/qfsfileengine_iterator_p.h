/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

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
