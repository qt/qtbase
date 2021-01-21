/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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


#ifndef Q_SPI_CACHE_H
#define Q_SPI_CACHE_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/QObject>
#include "struct_marshallers_p.h"

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

class QSpiDBusCache : public QObject
{
    Q_OBJECT

public:
    explicit QSpiDBusCache(QDBusConnection c, QObject* parent = nullptr);
    void emitAddAccessible(const QSpiAccessibleCacheItem& item);
    void emitRemoveAccessible(const QSpiObjectReference& item);

Q_SIGNALS:
    void AddAccessible(const QSpiAccessibleCacheItem &nodeAdded);
    void RemoveAccessible(const QSpiObjectReference &nodeRemoved);

public Q_SLOTS:
    QSpiAccessibleCacheArray GetItems();
};

QT_END_NAMESPACE

#endif /* Q_SPI_CACHE_H */
