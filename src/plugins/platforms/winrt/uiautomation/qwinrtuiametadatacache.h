/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINRTUIAMETADATACACHE_H
#define QWINRTUIAMETADATACACHE_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiacontrolmetadata.h"

#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>

QT_BEGIN_NAMESPACE

// Singleton used to cache metadata using the accessibility ID as the key.
class QWinRTUiaMetadataCache : public QObject
{
    QWinRTUiaMetadataCache();
    Q_OBJECT
public:
    static QWinRTUiaMetadataCache *instance();
    QSharedPointer<QWinRTUiaControlMetadata> metadataForId(QAccessible::Id id);
    void insert(QAccessible::Id id, const QSharedPointer<QWinRTUiaControlMetadata> &metadata);
    void remove(QAccessible::Id id);
    bool load(QAccessible::Id id);

private:
    QHash<QAccessible::Id, QSharedPointer<QWinRTUiaControlMetadata>> m_metadataTable;
    QMutex m_mutex;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIAMETADATACACHE_H
