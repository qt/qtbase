// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRESOURCE_ITERATOR_P_H
#define QRESOURCE_ITERATOR_P_H

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

#include "qabstractfileengine_p.h"
#include "qdir.h"

QT_BEGIN_NAMESPACE

class QResourceFileEngineIteratorPrivate;
class QResourceFileEngineIterator : public QAbstractFileEngineIterator
{
public:
    QResourceFileEngineIterator(QDir::Filters filters, const QStringList &filterNames);
    ~QResourceFileEngineIterator();

    QString next() override;
    bool hasNext() const override;

    QString currentFileName() const override;

private:
    mutable QStringList entries;
    mutable int index;
};

QT_END_NAMESPACE

#endif
